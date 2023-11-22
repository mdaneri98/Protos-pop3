
#include <sys/types.h>  // socket
#include <sys/socket.h> // socket
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include "../args/args.h"
#include "../../lib/logger/logger.h"
#include "managment.h"

#define CLIENT_BUFFER_SIZE 512
#define SPLIT_TOKEN '&'

/*
struct request {
    char *command;
    char *args;
};

static struct command {
    char *name;
    void (*action)(struct request *request);
};


void add_user_action() {

}

void change_pass_action() {

}

void get_max_mails_action() {

}

void set_max_mails_action() {

}

void stat_historic_connections_action() {

}

void stat_current_connections_action() {

}

void stat_bytes_transferred_action() {

}

static struct command commands[] = {
        {
            .name = "add-user",
            .action = add_user_action
        },
        {
            .name = "change-pass",
            .action = change_pass_action
        },
        {
            .name = "get-max-mails",
            .action = get_max_mails_action
        },
        {
            .name = "set-max-mails",
            .action = set_max_mails_action
        },
        {
            .name = "stat-historic-connections",
            .action = stat_historic_connections_action
        },
        {
            .name = "stat-current-connections",
            .action = stat_current_connections_action
        },
        {
            .name = "stat-bytes-transferred",
            .action = stat_bytes_transferred_action
        }
};*/

static struct argument* parse_request(char *buffer, ssize_t numBytesRcvd) {
    struct argument* argument = malloc(sizeof(struct argument));
    
    // Inicialización de arguments
    argument->token[0] = '\0';
    argument->value[0] = '\0';
    argument->name[0] = '\0';
    argument->key[0] =  '\0';

    /* token exampleName|exampleKey:exampleValue */

    char* next_token;
    // Dividir la cadena por espacios para obtener el token.
    char* part = strtok_r(buffer, " ", &next_token);
    if (part != NULL) {
        strcpy(argument->token, part);

        // Dividir la siguiente parte por | para obtener el nombre.
        part = strtok_r(NULL, "|", &next_token);
        if (part != NULL) {
            strcpy(argument->name, part);

            // Dividir la siguiente parte por : para obtener la clave y el valor.
            part = strtok_r(NULL, ":", &next_token);
            if (part != NULL) {
                strcpy(argument->key, part);
                // El valor es lo que queda después de la clave.
                char* p = strtok_r(NULL, "", &next_token);
                strcpy(argument->value, p);
            }
        }
    }
/*
    // Manejar valores NULL o cadenas vacías asignando valores predeterminados.
    if (argument->key == NULL || strcmp(argument->key, "") == 0) argument->key = "default_key";
    if (argument->value == NULL || strcmp(argument->value, "") == 0) argument->value = "default_value";
*/
    return argument;
}

void send_response(struct selector_key* key, char* response, struct sockaddr_storage clntAddr) {
    // Send received datagram back to the client
    ssize_t numBytesSent = sendto(key->fd, response, strlen(response), 0, (struct sockaddr *) &clntAddr, sizeof(clntAddr));
    if (numBytesSent < 0)
    {
        log(LOG_ERROR, "sendto() failed");
    }
  
}

// Selector_key hace referencia directamente al del servidor UDP. no hay conexión.
void accept_managment_connection(struct selector_key *key)
{
    struct sockaddr_storage clntAddr; 			// Client address
    // Set Length of client address structure (in-out parameter)
    socklen_t clntAddrLen = sizeof(clntAddr);

    char read_buffer[CLIENT_BUFFER_SIZE] = {0};
    //char write_buffer[CLIENT_BUFFER_SIZE] = {0};

    errno = 0;
    ssize_t numBytesRcvd = recvfrom(key->fd, read_buffer, CLIENT_BUFFER_SIZE, 0, (struct sockaddr *) &clntAddr, &clntAddrLen);
    logf(LOG_DEBUG, "Received: %s", read_buffer);
    if (numBytesRcvd < 0) {
        logf(LOG_ERROR, "recvfrom() failed: %s ", strerror(errno))
        return;
    }

    log(LOG_DEBUG, "Received message from client");
    parse_request(read_buffer, numBytesRcvd);
    
    
}
