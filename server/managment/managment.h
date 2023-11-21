#ifndef MANAGMENT_H
#define MANAGMENT_H

#include <unistd.h>
#include "../../lib/selector.h"


void accept_managment_connection(struct selector_key *key);

#endif