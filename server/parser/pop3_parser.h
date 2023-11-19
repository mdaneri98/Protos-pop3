#ifndef __POP3_PARSER_H__
#define __POP3_PARSER_H__

#include "../server_constants.h"
#include "../../lib/parser.h"
#include <stdint.h>

void parser_command_state_any(struct parser_event *ret, uint8_t c, void *data);

void parser_command_state_carriage_return(struct parser_event *ret, uint8_t c, void *data);

void parser_command_state_space(struct parser_event *ret, uint8_t c, void *data);

void parser_argument_state_any(struct parser_event *ret, uint8_t c, void *data);

void parser_argument_state_carriage_return(struct parser_event *ret, uint8_t c, void *data);

void parser_end_state_line_feed(struct parser_event *ret, uint8_t c, void *data);

void parser_end_state_any(struct parser_event *ret, uint8_t c, void *data);

//void get_parsed_cmd(void *data, char *buff, int max);
//void get_parsed_arg(void *data, char *buff, int max);

#endif