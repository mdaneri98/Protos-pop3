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


#define PORT 1024

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
    const char* err_msg = NULL;

    args = calloc(1, sizeof(args));
    parse_args(argc, argv, args);

    // ------------------ CONFIGURACION DEL CLIENTE ------------------
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;                // IPv4
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // Todas las interfaces (escucha por cualquier IP)
    addr.sin_port = htons(PORT); // Client port

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
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    

    if(setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &(int){1},sizeof(int)) < 0) {
        printf("Unable to setup server socket\n");
    }

    // ------------------ ENVIO DE COMANDOS ------------------

    for (int i = 0; i < MAX_ARGUMENTS; i++) {
        char* command = calloc(DEFAULT_SIZE*4+12, sizeof(char));
        if (args->arguments[i].name[0] == '\0') 
        {
            logf(LOG_DEBUG, "Argument %d is empty", i);
            continue;
        }

        logf(LOG_DEBUG, "Argument %d is %s", i, args->arguments[i].name);
        if (args->arguments[i].key[0] == '\0')
        {
            sprintf(
                command,
                "token %s %s\n",
                args->arguments[TOKEN].value,
                args->arguments[i].name
            );
        } else 
        {
            if (args->arguments[i].value[0] == '\0')
            {
                sprintf(
                    command,
                    "token %s %s|%s",
                    args->arguments[TOKEN].value,
                    args->arguments[i].name,
                    args->arguments[i].key
                );
            } else 
            {
                sprintf(
                    command,
                    "token %s %s|%s:%s",
                    args->arguments[TOKEN].value,
                    args->arguments[i].name,
                    args->arguments[i].key,
                    args->arguments[i].value
                );
            }
        }
        
        logf(LOG_DEBUG, "Sending command %s", command);
        if (sendto(client_socket, command, strlen(command), 0, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
            printf("%s: Unable to send request\n\n", args->arguments[i].name);
            continue;
        }

/*
        logf(LOG_DEBUG, "Receiving\n");
        if (recvfrom(client_socket, response, sizeof(response), 0, (struct sockaddr *) &address, &address_length) < 0) {
            printf("%s: Server did not respond %s\n\n", args->arguments[i].name);
            continue;
        }
*/

    }


finally:
    if (err_msg != NULL)
    {
        fprintf(stderr, "%s\n", err_msg);
        ret = -1;
    }

    return ret;
}
