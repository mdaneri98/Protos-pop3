#include "pop3_parser.h"
// Actions

parser_state parser_command_state_any(void *data, uint8_t c)
{
    parser_cmd_data *cmd_data = (parser_cmd_data *)data;
    if (cmd_data->cmd_length < CMD_LENGTH)
    {
        cmd_data->cmd_length += 1;
        cmd_data->cmd[cmd_length] = c;
        return READING;
    }
    return ERROR;
}

parser_state parser_command_state_carriage_return(void *data, uint8_t c)
{
    return READING;
}

parser_state parser_command_state_space(void *data, uint8_t c)
{
    parser_cmd_data *cmd_data = (parser_cmd_data *)data;
    // If command is not 4 letters or already read arg.
    if (cmd_data->cmd_length != CMD_LENGTH || cmd_data->arg_length != 0)
    {
        return ERROR;
    }
    return READING;
}

parser_state parser_argument_state_any(void *data, uint8_t c)
{
    parser_cmd_data *cmd_data = (parser_cmd_data *)data;
    if (cmd_data->arg_length < ARG_MAX_LENGTH)
    {
        cmd_data->arg_length += 1;
        cmd_data->arg[arg_length] = c;
        return READING;
    }
    return ERROR;
}

parser_state parser_argument_state_carriage_return(void *data, uint8_t c)
{
    return READING;
}

parser_state parser_end_state_line_feed(void *data, uint8_t c)
{
    return FINISHED;
}

parser_state parser_end_state_any(void *data, uint8_t c)
{
    return ERROR;
}

void get_parsed_cmd(void *data, char *buff, int max)
{
    // Write in the buffer the commands read
}

void get_parsed_arg(void *data, char *buff, int max)
{
    // Write in the buffer the args read
}
