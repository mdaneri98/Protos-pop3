#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <netinet/in.h>
#include <string.h>
#include <strings.h>
#include "args/admin_args.h"
#include <sys/socket.h>
#include "../lib/logger/logger.h"
#include <sys/time.h>
#include <errno.h>

#define CLIENT_PORT 1120
#define MANAGMENT_PORT 9090

struct args *args;
int done = false;

/*
static const char* admin_commands[]={
        "help",
        "add-user",
        "change-pass",
        "remove-user",
        "config-server-port",
        "token",
        "version",
        "get-max-mails",
        "set-max-mails",
        "stat-historic-connections",
        "stat-current-connections",
        "stat-bytes-transferred"
};
*/

int main(const int argc, char **argv)
{

    int ret = 0;
    const char *err_msg = NULL;

    args = calloc(1, sizeof(struct args));
    parse_args(argc, argv, args);
    args->server_port == 0 ? args->server_port = MANAGMENT_PORT : args->server_port;

    for (int i = 0; i < MAX_ARGUMENTS; i++)
    {
        logf(LOG_DEBUG, "Argument name %s | key %s | value %s\n", args->arguments[i].name, args->arguments[i].key, args->arguments[i].value);
    }

    // ------------------ CONFIGURACION DEL CLIENTE ------------------
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;                // IPv4
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // Todas las interfaces (escucha por cualquier IP)
    addr.sin_port = htons(CLIENT_PORT);       // Client port

    const int client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (client_socket < 0)
    {
        err_msg = "unable to create IPv4 socket";
        goto finally;
    }

    // ------------------ CONFIGURACION DEL SERVIDOR ------------------
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(args->server_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    /*
    Este fragmento de código configura el socket para que una operación de recepción (recv) tenga un tiempo de espera de 1 segundo.
    Si no se recibe una respuesta en ese tiempo, la función recv fallará con un error de tiempo de espera.
    */
    struct timeval tv;
    tv.tv_sec = 1;  // 1 segundo
    tv.tv_usec = 0; // 0 microsegundos
    if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        printf("Unable to set socket timeout\n");
    }

    // ------------------ ENVIO DE COMANDOS ------------------

    for (int i = 0; i < MAX_ARGUMENTS; i++)
    {
        char *command = calloc(DEFAULT_SIZE * 4 + 12, sizeof(char));
        if (args->arguments[i].name[0] == '\0')
        {
            logf(LOG_DEBUG, "Argument %d is empty", i);
            free(command);
            continue;
        }

        logf(LOG_DEBUG, "Argument %d is %s", i, args->arguments[i].name);

        sprintf(
            command,
            "token %s %s|%s:%s",
            *args->arguments[TOKEN].key ? args->arguments[TOKEN].key : "not-given",
            *args->arguments[i].name ? args->arguments[i].name : "not-given",
            *args->arguments[i].key ? args->arguments[i].key : "not-given",
            *args->arguments[i].value ? args->arguments[i].value : "not-given");

        logf(LOG_DEBUG, "Sending command %s", command);
        if (sendto(client_socket, command, strlen(command), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            printf("%s: Unable to send request\n\n", args->arguments[i].name);
        }

        char response[DEFAULT_SIZE] = {0};
        log(LOG_DEBUG, "Receiving\n");
        socklen_t server_addr_len = sizeof(server_addr);
        if (recvfrom(client_socket, response, sizeof(response), 0, (struct sockaddr *)&server_addr, &server_addr_len) < 0)
        {
            printf("%s: Server did not respond.\n\n", args->arguments[i].name);
            free(command);
            goto finally;
        }

        logf(LOG_DEBUG, "Response: %s", response);
        printf("%s", response);
        // printf("\n");

        free(command);
    }

finally:
    free(args);

    if (err_msg != NULL)
    {
        fprintf(stderr, "%s\n", err_msg);
        ret = -1;
    }

    return ret;
}
