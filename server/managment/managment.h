#ifndef MANAGMENT_H
#define MANAGMENT_H

#include <unistd.h>
#include "../../lib/selector.h"

#define DEFAULT_SIZE 256
#define MAX_ARGUMENTS 12

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
    char token[DEFAULT_SIZE];
    
    char name[DEFAULT_SIZE];
    char key[DEFAULT_SIZE];
    char value[DEFAULT_SIZE];

    //No las usamos todavía.
    bool hasKey;
    bool hasValue;
    bool is_active;
};

void receive_managment_message(struct selector_key *key);


#endif