#ifndef SERVER_CONSTANTS_H
#define SERVER_CONSTANTS_H

#include "stdlib.h"
#include <stdbool.h>
#include "../lib/buffer.h"
#include "../lib/stm.h"

#define SERVER_PORT 1110
#define CLIENT_PORT 9090

#define MAX_CONCURRENT_CONNECTIONS 512
#define BUFFER_SIZE 4096

#define CLIENT_TOKEN_LENGTH 6

// LIST msj, donde msj puede tener hasta 40 bytes.
#define COMMAND_QTY 12
#define COMMAND_LENGTH 4
#define MIN_COMMAND_LENGTH 4
#define ARGUMENT_LENGTH 40

#define MAX_USERS 10
#define USERNAME_SIZE 256
#define PATH_SIZE 4096
#define INITIAL_MAILS_QTY 64

typedef enum
{
    AUTHORIZATION = 0,
    TRANSACTION,
    ERROR,
    QUIT,
    STM_STATES_COUNT
} stm_states;

typedef enum
{
    COMMAND = 0,
    ARGUMENT,
    END,
    PARSER_STATES_COUNT
} parser_states;

typedef enum
{
    ANY_CHARACTER,
    CR,
    LF,
    DOT
} crlf_flag;

typedef enum
{
    UNDEFINED,
    VALID_COMMAND,
    INVALID_COMMAND,
} parser_event_types;

struct users
{
    char name[USERNAME_SIZE];
    char pass[USERNAME_SIZE];
    bool logged_in;
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

typedef struct connection_data
{
    int connection_fd;
    uint8_t in_buff[BUFFER_SIZE];
    uint8_t out_buff[BUFFER_SIZE];
    buffer in_buff_object;
    buffer out_buff_object;
    struct users *user;
    struct args *args;

    crlf_flag crlf_flag;

    char current_command[COMMAND_LENGTH + 1];
    char argument[ARGUMENT_LENGTH + 1];
    size_t command_length;
    size_t argument_length;
    bool is_finished;

    struct parser *parser;
    struct state_machine stm;
    int last_state;

    struct session current_session;

} connection_data;

struct stats
{
    unsigned long historical_connections;
    unsigned long concurrent_connections;
    unsigned long transferred_bytes;
};

struct mail
{
    size_t read_index;
    char path[PATH_SIZE];
    bool deleted;
    size_t size;
};

#endif