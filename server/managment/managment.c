
#include <sys/types.h>  // socket
#include <sys/socket.h> // socket
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <strings.h>
#include <errno.h>
#include "../args/args.h"
#include "../../lib/logger/logger.h"
#include "managment.h"

#define CLIENT_BUFFER_SIZE 512
#define SPLIT_TOKEN '&'

extern struct args *args;
extern struct stats *stats;
extern int done;


struct command {
    char *name;
    bool (*action)(char* out_buffer, struct argument* argument);
};


bool add_user_action(char* out_buffer, struct argument* argument) {
    log(LOG_DEBUG, "Client Request: Add User");

    if (args->users_count == MAX_USERS) {
        strcpy(out_buffer, "-ERR. Maxium amount of users reached.\r\n");
        return false;
    }

    if (argument->key[0] == '\0' || argument->value[0] == '\0') {
        strcpy(out_buffer, "-ERR. User or password not given.\r\n");
        return false;
    }

    strcpy(out_buffer, "OK+ Added new user. ");
    strcpy(args->users[args->users_count].name, argument->key);
    strcpy(args->users[args->users_count].pass, argument->value);
    args->users_count++;

    return true;
}
bool remove_user_action(char* out_buffer, struct argument* argument) {
    log(LOG_DEBUG, "Client Request: Remove User");

    if (args->users_count == 0 || argument->key[0] == '\0') {
        strcpy(out_buffer, "-ERR. Can't remove user.\r\n");
        return false;
    }

    for (int i = 0; i < args->users_count; i++)
    {
        if (strcmp(args->users[i].name, argument->key) == 0)
        {
            strcpy(out_buffer, "OK+ Removed user.\r\n");
            strcpy(args->users[i].name, "");
            break;
        }
    }
    args->users_count--;

    return true;
}

bool token_action(char* out_buffer, struct argument* argument)
{
    log(LOG_DEBUG, "Client Request: Token validation");
    // Nothing to do.
    return true;
}

bool change_pass_action(char* out_buffer, struct argument* argument)
{
     log(LOG_DEBUG, "Client Request: Change password");

    if (args->users_count == 0 || argument->key[0] == '\0' || argument->value[0] == '\0') {
        strcpy(out_buffer, "-ERR. Can change user password.\r\n");
        return false;
    }

    for (int i = 0; i < args->users_count; i++)
    {
        if (strcmp(args->users[i].name, argument->key) == 0)
        {
            strcpy(out_buffer, "OK+ Password changed.\r\n");
            strcpy(args->users[i].pass, argument->value);
            break;
        }
    }
    return true;
}

bool change_maildir_action(char* out_buffer, struct argument* argument) 
{
    log(LOG_DEBUG, "Client Request: Change maildir");

    if (argument->key[0] == '\0') {
        strcpy(out_buffer, "-ERR. Can't change maildir.\r\n");
        return false;
    }

    strcpy(out_buffer, "OK+ Maildir changed.\r\n ");
    strcpy(args->mail_directory, argument->key);
    return true;
}

bool version_action(char* out_buffer, struct argument* argument) {
    strcpy(
            out_buffer, "OK+ POP3 Server v1.0 | ITBA - 72.07 Protocolos de Comunicación 2023 2Q.\r\n"
    );
    return true;
}

bool get_max_mails_action(char* out_buffer, struct argument* argument) {
    log(LOG_DEBUG, "Client Request: Get max mails");

    char msj[25];
    sprintf(msj, "OK+ Max mails: %d.\r\n", (int) args->max_mails);
    strcpy(out_buffer, msj);
    return true;
}

bool set_max_mails_action(char* out_buffer, struct argument* argument) {
    log(LOG_DEBUG, "Client Request: Get max mails");

    if (argument->key[0] == '\0') {
        strcpy(out_buffer, "-ERR. Can't set max mails\r\n");
        return false;
    }

    args->max_mails = atoi(argument->key);
    if (args->max_mails > 0)
    {
        strcpy(out_buffer, "OK+ Max mails changed.\r\n");
    } else {
        strcpy(out_buffer, "-ERR. Can't set max mails.\r\n");
        return false;
    }
    return true;
}

bool stat_historic_connections_action(char* out_buffer, struct argument* argument) {
    log(LOG_DEBUG, "Client Request: Historic connections");

    char msj[100];
    sprintf(msj, "OK+ Historic connections: %lu.\r\n", stats->historical_connections);
    strcpy(out_buffer, msj);
    return true;
}

bool stat_current_connections_action(char* out_buffer, struct argument* argument) {
    log(LOG_DEBUG, "Client Request: Current connections");

    char msj[100];
    sprintf(msj, "OK+ Current connections: %lu.\r\n", stats->concurrent_connections);
    strcpy(out_buffer, msj);
    return true;
}

bool stat_bytes_transferred_action(char* out_buffer, struct argument* argument) {
    log(LOG_DEBUG, "Client Request: Bytes transferred");

    char msj[100];
    sprintf(msj, "OK+ Bytes transferred: %lu\r\n", stats->transferred_bytes);
    strcpy(out_buffer, msj);
    return true;
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
            .name = "change-maildir",
            .action = change_maildir_action
        },
        {
            .name = "remove-user",
            .action = remove_user_action
        },
        {
            .name = "token",
            .action = token_action
        },
        {
            .name = "version",
            .action = version_action
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

static struct argument* parse_request(char *buffer, ssize_t numBytesRcvd) {
    struct argument* argument = malloc(sizeof(struct argument));
    
    // Inicialización de arguments
    argument->token[0] = '\0';
    argument->value[0] = '\0';
    argument->name[0] = '\0';
    argument->key[0] =  '\0';

    /* token exampleName|exampleKey:exampleValue */

    /* Formato esperado: token valordeltoken CommandName|Key:Value */
    char *next_token;
    char *part = strtok_r(buffer, " ", &next_token);

    // Procesar la palabra 'token'
    if (part != NULL && strcmp(part, "token") == 0) {
        // Procesar el valor del token
        part = strtok_r(NULL, " ", &next_token);
        if (part != NULL) {
            strncpy(argument->token, part, sizeof(argument->token) - 1);

            // Procesar el nombre del comando
            part = strtok_r(NULL, "|", &next_token);
            if (part != NULL) {
                strncpy(argument->name, part, sizeof(argument->name) - 1);

                // Procesar la clave y el valor
                part = strtok_r(NULL, ":", &next_token);
                if (part != NULL) {
                    strncpy(argument->key, part, sizeof(argument->key) - 1);

                    // El valor es lo que queda después de la clave
                    part = strtok_r(NULL, "", &next_token);
                    if (part != NULL) {
                        strncpy(argument->value, part, sizeof(argument->value) - 1);
                    }
                }
            }
        }
    }

    return argument;
}

// Selector_key hace referencia directamente al del servidor UDP. no hay conexión.
void receive_managment_message(struct selector_key *key)
{
    // Alias
    struct args* global_args = args;

    struct sockaddr_storage clntAddr; 			// Client address
    // Set Length of client address structure (in-out parameter)
    socklen_t clntAddrLen = sizeof(clntAddr);

    char read_buffer[CLIENT_BUFFER_SIZE] = {0};
    char write_buffer[CLIENT_BUFFER_SIZE] = {0};

    errno = 0;
    ssize_t numBytesRcvd = recvfrom(key->fd, read_buffer, CLIENT_BUFFER_SIZE, 0, (struct sockaddr *) &clntAddr, &clntAddrLen);
    logf(LOG_DEBUG, "Received: %s", read_buffer);
    if (numBytesRcvd < 0) {
        logf(LOG_ERROR, "recvfrom() failed: %s ", strerror(errno))
        return;
    }

    log(LOG_DEBUG, "Received message from client");
    struct argument* arg = parse_request(read_buffer, numBytesRcvd);

    if (strcmp(global_args->token, arg->token) != 0)
    {
        logf(LOG_DEBUG, "Invalid token %s[CLIENT] != %s[SERVER]", arg->token, global_args->token);
        strcpy(write_buffer, "-ERR. Invalid token ");
        goto send;
    }


    for (int i = 0; i < MAX_ARGUMENTS; i++) {
        if (strcmp(arg->name, commands[i].name) == 0)
        {
            logf(LOG_DEBUG, "Executing command %s\n", commands[i].name);
            commands[i].action(write_buffer, arg);
            break;
        }
    }

send:
    sendto(key->fd, write_buffer, strlen(write_buffer), 0, (struct sockaddr *) &clntAddr, clntAddrLen);
    
}
