#ifndef __POP3_PARSER_H__
#define __POP3_PARSER_H__

#include <stdint.h>

typedef enum parser_state
{
    READING = 0,
    ACTION,
    FINISHED,
    ERROR
} parser_state;

#define CMD_LENGTH 4
#define ARG_MAX_LENGTH 63

typedef struct parser_cmd_data
{
    char cmd[CMD_LENGTH + 1];
    uint8_t cmd_length;
    char arg[ARGS_MAX_LENGTH + 1];
    uint8_t arg_length;
} parser_cmd_data;

parser_state parser_command_state_any(void *data, uint8_t c);

parser_state parser_command_state_carriage_return(void *data, uint8_t c);

parser_state parser_command_state_space(void *data, uint8_t c);

parser_state parser_argument_state_any(void *data, uint8_t c);

parser_state parser_argument_state_carriage_return(void *data, uint8_t c);

parser_state parser_end_state_line_feed(void *data, uint8_t c);

parser_state parser_end_state_any(void *data, uint8_t c);

void get_parsed_cmd(void *data, char *buff, int max);
void get_parsed_arg(void *data, char *buff, int max);

#endif