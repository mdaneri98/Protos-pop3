#ifndef __ADMIN_ARGS_H__
#define __ADMIN_ARGS_H__

#include "../admin_server_constants.h"

struct args {
    unsigned short client_port;
    char token[CLIENT_TOKEN_LENGTH + 1];
    
    char new_user[USERNAME_SIZE];
    char new_pass[USERNAME_SIZE];

    char change_user[USERNAME_SIZE];
    char change_pass[USERNAME_SIZE];

    char remove_user[USERNAME_SIZE];

    unsigned int new_max_mails;
    
};

void parse_args(const int argc, char **argv, struct args * args);


#endif