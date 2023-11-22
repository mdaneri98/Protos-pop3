#ifndef ADMIN_SERVER_CONSTANTS_H
#define ADMIN_ERVER_CONSTANTS_H

#include "stdlib.h"
#include <stdbool.h>
#include "../lib/buffer.h"
#include "../lib/stm.h"

#define SERVER_PORT 1110

#define MAX_USERS 10

#define CLIENT_TOKEN_LENGTH 6

#define DEFAULT_SIZE 256

struct users
{
    char name[DEFAULT_SIZE];
    char pass[DEFAULT_SIZE];
};