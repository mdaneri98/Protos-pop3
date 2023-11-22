#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <netinet/in.h>
#include "args/admin_args.h"
#include <sys/socket.h>
#include <sys/time.h>


#define PORT 1024

struct args *args;
int done = false;

static const char* admin_commands[]={
        "ADD_USER",
        "CHANGE_PASS",
        "REMOVE_USER",
        "GET_MAX_MAILS",
        "SET_MAX_MAILS",
        "STAT_HISTORIC_CONNECTIONS",
        "STAT_CURRENT_CONNECTIONS",
        "STAT_BYTES_TRANSFERRED"
};


int set_up_managment_server()
{
    const char* err_msg = NULL;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;                // IPv4
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // Todas las interfaces (escucha por cualquier IP)
    addr.sin_port = htons(args->client_port); // Client port

    // Pasive server sockets
    const int server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (server < 0)
    {
        err_msg = "unable to create IPv4 managment socket";
        goto finally;
    }

    // bind server sockets to port
    logf(LOG_DEBUG, "Binding IPv4 managment socket at port %d", args->client_port);
    if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        err_msg = "unable to bind socket";
        goto finally;
    }
    
    return server;
    
finally:
    if (err_msg)
    {
        logf(LOG_ERROR, "%s: %s", err_msg, strerror(errno));
    }
    if (server >= 0)
    {
        close(server);
    }
    return -1;
}

int main(const int argc, char **argv)
{

    int ret = 0;
    
    int server_socket=set_up_managment_server();

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

    if(setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO, &tv,sizeof(tv)) < 0) {
        printf("Unable to setup server socket\n");
    }

    parse_args(argc, argv, args);

    


}
