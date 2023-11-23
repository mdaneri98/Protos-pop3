#ifndef __ADMIN_ARGS_H__
#define __ADMIN_ARGS_H__

#include "../admin_server_constants.h"

#define DEFAULT_SIZE 256

typedef enum args_type {
    HELP,
    ADD_USER,
    CHANGE_PASS,
    CHANGE_MAILDIR,
    REMOVE_USER,
    CONFIG_SERVER_PORT,
    TOKEN,
    VERSION,
    GET_MAX_MAILS,
    SET_MAX_MAILS,
    STAT_HISTORIC_CONNECTIONS,
    STAT_CURRENT_CONNECTIONS,
    STAT_BYTES_TRANSFERRED
} args_type;



struct argument {
    char name[DEFAULT_SIZE];
    char key[DEFAULT_SIZE];
    char value[DEFAULT_SIZE];

    //No las usamos todavía.
    bool hasKey;
    bool hasValue;
    bool is_active;
};

struct args {
    struct argument arguments[MAX_ARGUMENTS];
    size_t arguments_count;

    unsigned short server_port;
};

void parse_args(const int argc, char **argv, struct args * args);


#endif