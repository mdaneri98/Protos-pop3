
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
};

static struct request* parse_request(char *buffer, ssize_t numBytesRcvd) {
    char *command = strtok(buffer, " ");
    char *args = strtok(NULL, " ");

    struct request* request = calloc(1, sizeof(struct request));

    // tk:123456&&add-user:someuser&&pass:pass123

    // Extract the first token
    char * token = strtok(buffer, "&&");
    // loop through the string to extract all other tokens
    while( token != NULL ) {
        





        token = strtok(NULL, " ");
    }


    for (int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        if (strcmp(commands[i].name, command) == 0) {
            commands[i].action(&request);
            return NULL;
        }
    }

    logf(LOG_ERROR, "Command %s not found", command);

    return NULL;
}

void send_response(struct selector_key* key, char* response, struct sockaddr_storage clntAddr) {
    // Send received datagram back to the client
    ssize_t numBytesSent = sendto(key->fd, response, strlen(response), 0, (struct sockaddr *) &clntAddr, sizeof(clntAddr));
    if (numBytesSent < 0)
    {
        log(LOG_ERROR, "sendto() failed");
    }
  
}
*/

void accept_managment_connection(struct selector_key *key) {
    
}

/*
// Selector_key hace referencia directamente al del servidor UDP. no hay conexión.
void accept_managment_connection(struct selector_key *key)
{
    struct sockaddr_storage clntAddr; 			// Client address
    // Set Length of client address structure (in-out parameter)
    socklen_t clntAddrLen = sizeof(clntAddr);

    char read_buffer[CLIENT_BUFFER_SIZE] = {0};
    char write_buffer[CLIENT_BUFFER_SIZE] = {0};

    errno = 0;
    ssize_t numBytesRcvd = recvfrom(key->fd, read_buffer, CLIENT_BUFFER_SIZE, 0, (struct sockaddr *) &clntAddr, &clntAddrLen);
    if (numBytesRcvd < 0) {
        logf(LOG_ERROR, "recvfrom() failed: %s ", strerror(errno))
        return;
    }

    log(LOG_DEBUG, "Received message from client");
    parse_request(read_buffer, numBytesRcvd);
    
    
}


*/