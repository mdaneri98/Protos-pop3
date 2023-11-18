#include "../../lib/include/stm.h"
#include "../../lib/include/selector.h"

static struct command commands[]={
        {
            .name = "USER",
            .argument_type = have_argument,
            .action = user_action
        },
        {
            .name = "PASS",
            .argument_type = have_argument,
            .action = pass_action
        },
        {
            .name = "STAT",
            .argument_type = not_argument,
            .action = stat_action
        },
        {
            .name = "LIST",
            .argument_type = might_argument,
            .action = list_action
        },
        {
            .name = "RETR",
            .argument_type = have_argument,
            .action = retr_action
        },
        {
            .name = "DELE",
            .argument_type = have_argument,
            .action = dele_action
        },
        {
            .name = "NOOP",
            .argument_type = not_argument,
            .action = noop_action
        },{
            .name = "RSET",
            .argument_type = not_argument,
            .action = rset_action
        },
        {
            .name = "QUIT",
            .argument_type = not_argument,
            .action = quit_action
        },
        {
            .name = "CAPA",
            .argument_type = not_argument,
            .action = capa_action,
        },
        {
            .name = "Error command", //no deberia llegar aca para buscar al comando
            .argument_type = might_argument,
            .action = default_action
        }
};


void server_ready(struct connection_data * conn){
    char * msj = "+OK POP3 server ready\r\n";

    size_t n;
    char * ptr = (char *) buffer_write_ptr(&conn->out_buffer_object, &n);
    if (n >= strlen(msj)) {
        connection->current_command.finished = true;
        strncpy(ptr, msj, strlen(msj));
        buffer_write_adv(&connection->out_buffer_object, (ssize_t) strlen(msj));
        return true;
    }
    return false;
}

stm_states stm_server_read(struct selector_key *key){
    connection_data * connection = (connection_data *) key->data;

    if(!buffer_can_read[&connection->in_buffer_object]){
        
    }
}

stm_states read_command(struct selector_key * key, stm_states current_state) {
    connection_data * connection = (connection_data *) key->data;
    
    //Guardamos lo que leemos del socket en el buffer de entrada
    size_t max = 0;
    uint8_t* ptr = buffer_write_ptr(&(state->info_read_buff), &max);
    ssize_t read_count = recv(key->s, ptr, max, 0);

    if(read_count<=0){
        log(LOG_ERROR,"Error reading at socket");
        return FINISHED;
    }
    //Avanzamos la escritura en el buffer
    buffer_write_adv(&(state->info_read_buff), read_count);

    //Obtenemos un puntero para lectura
    ptr = buffer_read_ptr(&(state->info_read_buff),&max);
    
    for(size_t i = 0; i<max; i++){
        parser_state parser = parser_feed(state->pop3_parser, ptr[i]);
        if(parser == PARSER_FINISHED || parser == PARSER_ERROR){
            //avanzamos solo hasta el fin del comando
            buffer_read_adv(&(state->info_read_buff),i+1);
            
            // Copia el comando al state.cmd
            get_pop3_cmd(state->pop3_parser,state->cmd,MAX_CMD);
            pop3_command command = get_command(state->cmd);
            logf(LOG_DEBUG,"Reading request for cmd: '%s'", command>=0 ? commands[command].name : "invalid command");
            connection->command = command;
            get_pop3_arg(state->pop3_parser,state->arg,MAX_ARG);
            if(parser == PARSER_ERROR || command == ERROR_COMMAND){
                log(LOG_ERROR, "Unknown command");
                state->command = ERROR_COMMAND;
            }
            if(!commands[command].check(state->arg)){
                log(LOG_ERROR, "Bad arguments");
                state->command = ERROR_COMMAND;
            }

            if(!check_command_for_protocol_state(state->pop3_protocol_state, command)){
                logf(LOG_ERROR,"Command '%s' not allowed in this state",commands[command].name);
                state->command = ERROR_COMMAND;
            }
            parser_reset(state->pop3_parser);
            //Vamos a procesar la respuesta
            if(selector_set_interest(key->s,key->fd,OP_WRITE) != SELECTOR_SUCCESS){
                log(LOG_ERROR, "Error setting interest to OP_WRITE after reading request");
                return FINISHED;
            }
            return WRITING_RESPONSE; //vamos a escribir la respuesta
        }
    }
    //Avanzamos en el buffer, leimos lo que tenia
    buffer_read_adv(&(state->info_read_buff), (ssize_t) max);
    return READING_REQUEST; //vamos a seguir leyendo el request
}

stm_states write_command(struct selector_key * key, stm_states current_state) {
    connection_data connection = (connection_data) key->data;
    char * ptr;

    if (buffer_can_write(&connection->out_buffer_object)) {
        size_t write_bytes;
        ptr = (char *) buffer_write_ptr(&connection->out_buffer_object, &write_bytes);
        for (size_t j = 0; j < pop3_commands_length[current_state]; j++) {
            struct pop3_command maybe_command = pop3_commands[current_state][j];
            if (strcasecmp(maybe_command.command, connection->current_command.command) == 0) {
                stm_states next_state = maybe_command.writer(key, connection, ptr, &write_bytes);
                buffer_write_adv(&connection->out_buffer_object, (ssize_t) write_bytes);
                if (connection->current_command.finished) {
                    clear_parser_buffers(&connection->current_command);
                }
                return next_state;
            }
        }
    }

    return current_state;
}

void stm_authorization_arrival(stm_states state, struct selector_key * key) {


}

void stm_authorization_departure(stm_states state, struct selector_key * key) {

}

stm_states stm_authorization_read(struct selector_key * key) {
    return read_command(key, AUTHORIZATION);
}

stm_states stm_authorization_write(struct selector_key * key){
    connection_data connection = (connection_data) key->data;

    // Si es la primera vez que estamos en el estado AUTHORIZATION,
    // mandamos el mensaje de bienvenida.
    if ((int) connection->last_state == -1) {
        bool wrote = greet(connection);
        if (wrote) {
            connection->last_state = AUTHORIZATION;
        }
        return AUTHORIZATION;
    }

    return write_command(key, AUTHORIZATION);
}

void stm_transaction_arrival(stm_states state, struct selector_key * key){

}

void stm_transaction_departure(stm_states state, struct selector_key * key){

}

stm_states stm_transaction_read(struct selector_key * key) {
    return read_command(key, TRANSACTION);
}

stm_states stm_transaction_write(struct selector_key * key){

}

void stm_error_arrival(stm_states state, struct selector_key * key){

}

void stm_error_departure(stm_states state, struct selector_key * key){

}

stm_states stm_error_read(struct selector_key * key){

}

stm_states stm_error_write(struct selector_key * key){

}

void stm_quit_arrival(stm_states state, struct selector_key * key){

}

void stm_quit_departure(stm_states state, struct selector_key * key){

}

stm_states stm_quit_read(struct selector_key * key){

}

stm_states stm_quit_write(struct selector_key * key){

}
