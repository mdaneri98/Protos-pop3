#include "pop3_parser.h"
// Actions

void parser_command_state_any(struct parser_event *ret, uint8_t c, void *data)
{
    connection_data d = (connection_data)data;
    if (d->current_command.command_length < MIN_COMMAND_LENGTH)
    {
        ret->type = UNDEFINED;
        d->current_command.command[d->current_command.command_length] = (char)c;
        d->current_command.command_length += 1;
        return;
    }
    ret->type = INVALID_COMMAND;
}

void parser_command_state_carriage_return(struct parser_event *ret, uint8_t c, void *data)
{
    connection_data d = (connection_data)data;
    if (d->current_command.command_length < MIN_COMMAND_LENGTH)
    {
        ret->type = INVALID_COMMAND;
        return;
    }
    d->current_command.command[d->current_command.command_length] = '\0' ret->type = UNDEFINED;
}

void parser_command_state_space(void *data, uint8_t c)
{
    connection_data d = (connection_data)data;
    if (d->current_command.command_length < MIN_COMMAND_LENGTH)
    {
        ret->type = INVALID_COMMAND;
        return;
    }
    d->current_command.command[d->current_command.command_length] = '\0';
    ret->type = UNDEFINED;
}

void parser_argument_state_any(struct parser_event *ret, uint8_t c, void *data)
{
    connection_data d = (connection_data)data;
    if (d->current_command.argument_length < ARGUMENT_LENGTH)
    {
        ret->type = UNDEFINED;
        d->current_command.argument[d->current_command.argument_length] = (char)c;
        d->current_command.argument_length += 1;
        return;
    }
    ret->type = INVALID_COMMAND;
}

void parser_argument_state_carriage_return(struct parser_event *ret, uint8_t c, void *data)
{
    connection_data d = (connection_data)data;
    if (d->current_command.argument_length > 0)
    {
        ret->type = UNDEFINED;
        d->current_command.argument[d->current_command.argument_length] = '\0';
        d->current_command.argument_length += 1;
    }
    ret->type = INVALID_COMMAND;
}

void parser_end_state_line_feed(struct parser_event *ret, uint8_t c, void *data)
{
    ret->type = VALID_COMMAND;
}

void parser_end_state_any(struct parser_event *ret, uint8_t c, void *data)
{
    ret->type = INVALID_COMMAND;
}

void get_parsed_cmd(void *data, char *buff, int max)
{
    // Write in the buffer the commands read
}

void get_parsed_arg(void *data, char *buff, int max)
{
    // Write in the buffer the args read
}
