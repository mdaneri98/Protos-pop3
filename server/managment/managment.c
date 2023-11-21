
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

void accept_managment_connection(struct selector_key *key)
{
    struct sockaddr_in client;
    unsigned int client_length = sizeof(client);

    char read_buffer[CLIENT_BUFFER_SIZE] = {0};
    //char write_buffer[CLIENT_BUFFER_SIZE] = {0};

    ssize_t n = recvfrom(key->fd, read_buffer, CLIENT_BUFFER_SIZE, 0, (struct sockaddr *) &client, &client_length);
    if (n <= 0) {
        return;
    }

    log(LOG_DEBUG, "Received message from client");

}