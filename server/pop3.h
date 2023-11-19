#ifndef POP3_H
#define POP3_H

#include "server_constants.h"
#include "../lib/selector.h"
#include "../lib/stm.h"
#include "../lib/logger/logger.h"


void accept_pop_connection(struct selector_key * key);

#endif
