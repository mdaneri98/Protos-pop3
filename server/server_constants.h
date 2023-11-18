#ifndef SERVER_CONSTANTS_H
#define SERVER_CONSTANTS_H

#include "stdlib.h"
#include <stdbool.h>
// #include "../constants.h"
// #include "../lib/include/stm.h"

#define SERVER_PORT 1110
#define CLIENT_PORT 1120

#define MAX_CONCURRENT_CONNECTIONS 1024
#define MAX_QUEUED_CONNECTIONS 32
#define BUFFER_SIZE 512

// LIST msj, donde msj puede tener hasta 40 bytes.
#define COMMAND_LENGTH 4
#define MIN_COMMAND_LENGTH 3
#define ARGUMENT_LENGTH 40

#define MAX_USERS 10
#define USERNAME_SIZE 256
#define PATH_SIZE 4096
#define INITIAL_MAILS_QTY 64

#define CLIENT_HEADER_LENGTH 9
#define CLIENT_ID_LENGTH 10

#define CLIENT_TOKEN_LENGTH 6


typedef enum {
    AUTHORIZATION = 0,
    TRANSACTION,
    ERROR,
    QUIT,
    STM_STATES_COUNT
} stm_states;

typedef enum {
    COMMAND = 0,
    ARGUMENT,
    END,
    PARSER_STATES_COUNT
} parser_states;

struct users {
    char name[USERNAME_SIZE];
    char pass[USERNAME_SIZE];
    bool logged_in;
};

typedef struct connection_data {
    int connection_fd;
    uint8_t read_buff[BUFFER_SIZE];
    uint8_t write_buff[BUFFER_SIZE];
    buffer read_buff_object;
    buffer write_buff_object;
    struct users *user;
    struct args *args;

    struct parser * parser;
    struct state_machine stm;
    stm_states last_state;

} connection_data;

struct stats
{
    unsigned long historical_connections;
    unsigned long concurrent_connections;
    unsigned long transferred_bytes;
};

struct mail
{
    char path[PATH_SIZE];
    bool deleted;
    size_t size;
};

struct session
{
    char username[USERNAME_SIZE];
    char maildir[PATH_SIZE];
    struct mail *mails;
    size_t mail_count;
    size_t maildir_size;
    bool requested_quit;
};



#endif