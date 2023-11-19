/**
 * main.c - servidor proxy socks concurrente
 *
 * Interpreta los argumentos de línea de comandos, y monta un socket
 * pasivo. Por cada nueva conexión lanza un hilo que procesará de
 * forma bloqueante utilizando el protocolo SOCKS5.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>

#include "../lib/buffer.h"
#include "../lib/netutils.h"
#include "../lib/logger/logger.h"

#include "pop3.h"

#include "args/args.h"

#define MAX_PENDING_CONNECTIONS 24
#define INITIAL_FDS 1024

// Cambiar antes de presentar el TP.
#define LOGGER_LEVEL 0

struct args *args;
int done = false;

void sigterm_handler(const int signal)
{
    printf("signal %d, cleaning up and exiting\n", signal);
    done = true;
}

int main(const int argc, char **argv)
{
    /*
    unsigned port = 1110;

    if(argc == 1) {
        // utilizamos el default
    } else if(argc == 2) {
        char *end     = 0;
        const long sl = strtol(argv[1], &end, 10);

        if (end == argv[1]|| '\0' != *end
           || ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno)
           || sl < 0 || sl > USHRT_MAX) {
            fprintf(stderr, "port should be an integer: %s\n", argv[1]);
            return 1;
        }
        port = sl;
    } else {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }
    */

    // Registramos handlers para terminar normalmente en caso de una signal
    signal(SIGTERM, sigterm_handler);
    signal(SIGINT, sigterm_handler);

    int ret = 0;

    // Selector
    const char *err_msg = NULL;
    selector_status ss = SELECTOR_SUCCESS;
    fd_selector selector = NULL;

    // Opciones de configuracion del selector
    // Decimos que usamos sigalarm para los trabajos bloqueantes (no nos interesa)
    // Espera 10 segundos hasta salir del select interno
    const struct selector_init conf = {
        .signal = SIGALRM,
        .select_timeout = {
            .tv_sec = 10,
            .tv_nsec = 0,
        },
    };

    // Guarda las configuraciones para el selector
    if (0 != (ss = selector_init(&conf)))
    {
        fprintf(stderr, "Cannot initialize selector: '%s'\n", ss == SELECTOR_IO ? strerror(errno) : selector_error(ss));
        return 2;
    }

    // usando las configuraciones del init, crea un nuevo selector con capacidad para 1024 FD's inicialmente
    selector = selector_new(INITIAL_FDS);
    if (selector == NULL)
    {
        fprintf(stderr, "Unable to create selector: '%s'\n", strerror(errno));
        return 1;
    }

    // Inicializar logger
    if (logger_init(selector, "", NULL) != 0)
    {
        fprintf(stderr, "Unable to initialize logger\n");
        return 1;
    }
    logger_set_level(LOGGER_LEVEL);

    // Argumentos y valores default
    args = malloc(sizeof(struct args));
    if (args == NULL)
    {
        log(LOG_DEBUG, "Unable to allocate memory for args\n");
        return 1;
    }

    parse_args(argc, argv, args);

    // No hacer buffering en la salida estándar.
    log(LOG_DEBUG, "Removing buffering to stdout");
    setvbuf(stdout, NULL, _IONBF, 0);

    // Address para hacer el bind del socket.
    // Creamos para el servidor con IPv4 y IPv6, mismo puerto.
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;                // IPv4
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // Todas las interfaces (escucha por cualquier IP)
    addr.sin_port = htons(args->server_port); // Server port

    struct sockaddr_in6 addr_6;
    memset(&addr_6, 0, sizeof(addr_6));
    addr_6.sin6_family = AF_INET6;
    addr_6.sin6_addr = in6addr_any;
    addr_6.sin6_port = htons(args->server_port);

    // Pasive server sockets
    const int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server < 0)
    {
        err_msg = "unable to create IPv4 socket";
        goto finally;
    }

    const int server_6 = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (server_6 < 0)
    {
        err_msg = "unable to create IPv6 socket.";
        goto finally;
    }

    logf(LOG_DEBUG, "Listening on TCP port %d", args->server_port);
    fprintf(stdout, "Listening on TCP port %d\n", args->server_port);

    // man 7 ip. no importa reportar nada si falla.
    log(LOG_DEBUG, "Setting SO_REUSEADDR on IPv4 socket");
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    log(LOG_DEBUG, "Setting IPV6_V6ONLY and SO_REUSEADDR on IPv6 socket");
    setsockopt(server_6, IPPROTO_IPV6, IPV6_V6ONLY, &(int){1}, sizeof(int));
    setsockopt(server_6, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    // bind server sockets to port
    log(LOG_DEBUG, "Binding IPv4 socket");
    if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        err_msg = "unable to bind socket";
        goto finally;
    }

    log(LOG_DEBUG, "Binding IPv6 socket");
    if (bind(server_6, (struct sockaddr *)&addr_6, sizeof(addr_6)) < 0)
    {
        err_msg = "unable to bind socket";
        goto finally;
    }

    // listen for incoming connections.
    log(LOG_DEBUG, "Listening IPv4 socket");
    if (listen(server, MAX_PENDING_CONNECTIONS) < 0)
    {
        err_msg = "unable to listen";
        goto finally;
    }

    log(LOG_DEBUG, "Listening IPv6 socket");
    if (listen(server_6, MAX_PENDING_CONNECTIONS) < 0)
    {
        err_msg = "unable to listen";
        goto finally;
    }

    /*
        El servidor está preparado para aceptar conexiones tanto de IPv4 como de IPv6.
        El selector ayuda a manejar estas dos pilas de red de forma transparente.
        Basicamente este selector alterna entre las conexiones de IPv4, IPv6 y
        proximamente el servicio para el managment.
    */

    /* Método de utilidad que activa O_NONBLOCK en un fd. */
    /* Las operaciones de w/r sobre ese 'archivo' se convierten en no bloqueantes. */
    if (selector_fd_set_nio(server) < 0)
    {
        err_msg = "unable to set server socket as non-blocking";
        goto finally;
    }

    if (selector_fd_set_nio(server_6) < 0)
    {
        err_msg = "unable to set server socket as non-blocking";
        goto finally;
    }

    // Registramos el socket pasivo dentro del selector.
    // El selector se encargará de monitorear los eventos de este socket.
    const fd_handler server_handler = {
        .handle_read = accept_pop_connection,
        .handle_write = NULL,
        .handle_close = NULL,
    };

    // Registramos el socket pasivo en el selector
    log(LOG_DEBUG, "Registering IPv4 server fd") if (SELECTOR_SUCCESS != selector_register(selector, server, &server_handler, OP_READ, args))
    {
        err_msg = "unable to register server fd for IPv4 socket.";
        goto finally;
    }

    log(LOG_DEBUG, "Registering IPv6 server fd") if (SELECTOR_SUCCESS != selector_register(selector, server_6, &server_handler, OP_READ, args))
    {
        err_msg = "unable to register server fd for IPv4 socket.";
        goto finally;
    }

    for (; !done;)
    {
        err_msg = NULL;
        ss = selector_select(selector);
        if (ss != SELECTOR_SUCCESS)
        {
            err_msg = "error while selecting conection between IPv4 or IPv6";
            goto finally;
        }
    }

finally:
    if (err_msg)
    {
        logf(LOG_ERROR, "%s: %s", err_msg, strerror(errno));
        ret = 1;
    }
    if(selector != NULL) { //si pudimos obtener el selector, lo liberamos
        log(LOG_INFO, "Destroying selector");
        selector_destroy(selector);
    }
    selector_close();
    if (server >= 0)
    {
        close(server);
    }

    if (server_6 >= 0)
    {
        close(server_6);
    }

    logf(LOG_INFO, "Server terminated with code %d", ret);
    logger_finalize();

    return ret;
}
