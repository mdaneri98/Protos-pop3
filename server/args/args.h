
#ifndef __ARGS_H__
#define __ARGS_H__

#include "../server_constants.h"

struct args {
    unsigned short server_port;
    unsigned short client_port;
    char token[CLIENT_TOKEN_LENGTH + 1];
    char mail_directory[PATH_SIZE];
    size_t max_mails;
    struct users users[MAX_USERS];
    size_t users_count;
};

void parse_args(const int argc, char **argv, struct args * args);


#endif