#include "../server_constants.h"
#include "../../lib/buffer.h"
#include "../../lib/logger/logger.h"
#include "../../lib/selector.h"
#include "../../lib/parser.h"
#include "../args/args.h"
#include <sys/types.h>
#include <string.h>
#include <strings.h>
#include <sys/dir.h>

typedef enum try_state {
    TRY_DONE,
    TRY_PENDING
} try_state;

try_state try_write(const char* str, buffer* buff){
    size_t max = 0;
    uint8_t * ptr = buffer_write_ptr(buff,&max);
    size_t message_len = strlen(str);
    if(max<message_len){
        //vuelvo a intentar despues
        return TRY_PENDING;
    }
    //Manda el mensaje parcialmente si no hay espacio
    memcpy(ptr, str, message_len);
//    strncpy((char*)ptr, str, message_len);
    buffer_write_adv(buff,(ssize_t)message_len);
    return TRY_DONE;
}

stm_states user_handler(struct selector_key * key, connection_data* conn) {
    log(LOG_DEBUG, "FD %d: USER command");

    struct users* users = conn->args->users;
    size_t users_count = conn->args->users_count;

     for (size_t i = 0; i < users_count; i++) {
        // Si el usuario existe, seteamos el maildir y el username en la sesion actual.
        if (strcmp(users[i].name, conn->argument) == 0) {
            char base_directory[PATH_SIZE];
            strcpy(base_directory, conn->args->mail_directory);

            conn->current_session.maildir[0] = '\0';
            strcat(conn->current_session.maildir, base_directory);
            strcat(conn->current_session.maildir, "/");
            strcat(conn->current_session.maildir, conn->argument);
            strcat(conn->current_session.maildir, "/cur");

            DIR * directory = opendir(conn->current_session.maildir);
            if (directory == NULL) {
                conn->command_error = true;
                conn->current_session.maildir[0] = '\0';

                char* msj = "-ERR invalid user\r\n";
                try_write(msj,&(conn->write_buff_object));

                return AUTHORIZATION;
            }
            closedir(directory);

            char* msj = "+OK valid user\r\n";
            try_write(msj,&(conn->write_buff_object));

            strcpy(conn->current_session.username, conn->argument);
            conn->command_error = false;
            return AUTHORIZATION;
        }
    }

    char* msj = "-ERR invalid user\r\n";
    try_write(msj,&(conn->write_buff_object));

    // Si el usuario no existe, seteamos el error en true.
    conn->command_error = true;
    return AUTHORIZATION;
}


stm_states pass_handler(struct selector_key * key, connection_data * conn) {
    log(LOG_DEBUG, "FD %d: USER command");

    struct users* users = conn->args->users;
    size_t users_count = conn->args->users_count;

     for (size_t i = 0; i < users_count; i++) {
        // Si el usuario existe, seteamos el maildir y el username en la sesion actual.
        if (strcmp(users[i].name, conn->argument) == 0) {
            char base_directory[PATH_SIZE];
            strcpy(base_directory, conn->args->mail_directory);

            conn->current_session.maildir[0] = '\0';
            strcat(conn->current_session.maildir, base_directory);
            strcat(conn->current_session.maildir, "/");
            strcat(conn->current_session.maildir, conn->argument);
            strcat(conn->current_session.maildir, "/cur");

            DIR * directory = opendir(conn->current_session.maildir);
            if (directory == NULL) {
                conn->command_error = true;
                conn->current_session.maildir[0] = '\0';
                return AUTHORIZATION;
            }
            closedir(directory);

            strcpy(conn->current_session.username, conn->argument);
            conn->command_error = false;
            return AUTHORIZATION;
        }
    }

    // Si el usuario no existe, seteamos el error en true.
    conn->command_error = true;
    return AUTHORIZATION;
}






typedef enum command_args {
    REQUIRED,
    OPTIONAL,
    EMPTY
} command_args;


static struct command {
    const char * name;
    command_args arguments;
    stm_states (*handler)(struct selector_key *, struct connection_data *);
};

static struct command commands[] = {
        {
            .name = "USER",
            .arguments = REQUIRED,
            .handler = user_handler
        },
        {
            .name = "PASS",
            .arguments = REQUIRED,
            .handler = pass_handler
        },
};

void server_ready(struct connection_data * conn){
    char * msj = "+OK POP3 server ready\r\n";

    size_t n;
    char * ptr = (char *) buffer_write_ptr(&conn->write_buff_object, &n);
    if (n >= strlen(msj)) {
        conn->is_finished = true;
        strncpy(ptr, msj, strlen(msj));
        buffer_write_adv(&conn->write_buff_object, (ssize_t) strlen(msj));

    }

}

stm_states stm_server_read(struct selector_key *key){
    connection_data * connection = (connection_data *) key->data;

/*
    if(!buffer_can_read[&connection->read_buff_object]){
        
    }
*/

}

stm_states read_command(struct selector_key * key, stm_states current_state) {
    struct connection_data* connection = (struct connection_data*) key->data;
    char * ptr;

    if (!buffer_can_read(&connection->read_buff_object)) {
        size_t write_bytes;
        ptr = (char *) buffer_write_ptr(&connection->read_buff_object, &write_bytes);
        ssize_t n = recv(key->fd, ptr, write_bytes, 0);
        if (n == 0) {
            return QUIT;
        }
        buffer_write_adv(&connection->read_buff_object, n);
    }

    size_t read_bytes;
    ptr = (char *) buffer_read_ptr(&connection->read_buff_object, &read_bytes);

    for (size_t i = 0; i < read_bytes; i++) {
        const struct parser_event * event = parser_feed(connection->parser, ptr[i], connection);
        buffer_read_adv(&connection->read_buff_object, 1);

        /* event-type es modificado en pop3_parser. 
            Una vez que se termina de parsear el comando, se setea el tipo de evento en VALID_COMMAND o INVALID_COMMAND.
        */
        if (event->type == VALID_COMMAND) {
            for (size_t j = 0; j < COMMAND_LENGTH; j++) {
                struct command maybe_command = commands[j];
                if (strcasecmp(maybe_command.name, connection->current_command) == 0) {
                    if ((maybe_command.arguments == REQUIRED && connection->argument_length > 0) ||
                        (maybe_command.arguments == EMPTY && connection->argument_length == 0) ||
                        (maybe_command.arguments == OPTIONAL)) {
                        stm_states next_state = maybe_command.handler(key, connection);
                     
                     
                     //   selector_set_interest_key(key, OP_WRITE);
                     
                        return next_state;
                    } else {
                        logf(LOG_DEBUG, "FD %d: Error. Invalid argument", key->fd);
                        return ERROR;
                    }
                }
            }
            logf(LOG_DEBUG, "FD %d: Error. Invalid command for state", key->fd);
            return ERROR;
        } else if (event->type == INVALID_COMMAND) {
            logf(LOG_DEBUG, "FD %d: Error. Invalid command", key->fd);
            bool saw_carriage_return = ptr[i] == '\r';
            while (i < read_bytes) {
                char c = (char) buffer_read(&connection->read_buff_object);
                if (c == '\r') {
                    saw_carriage_return = true;
                } else if (c == '\n') {
                    if (saw_carriage_return) {
                        return ERROR;
                    }
                } else {
                    saw_carriage_return = false;
                }
                i++;
            }
            return ERROR;
        }
    }

    return current_state;
}

stm_states write_command(struct selector_key * key, stm_states current_state) {
    connection_data* connection = (connection_data*) key->data;
    char * ptr;

    if (buffer_can_write(&connection->write_buff_object)) {
        size_t write_bytes;
        ptr = (char *) buffer_write_ptr(&connection->write_buff_object, &write_bytes);
        for (size_t j = 0; j < COMMAND_LENGTH; j++) {
            struct command maybe_command = commands[j];
            if (strcasecmp(maybe_command.name, connection->current_command) == 0) {
                // stm_states next_state = maybe_command.writer(key, connection, ptr, &write_bytes);
                buffer_write_adv(&connection->write_buff_object, (ssize_t) write_bytes);
                if (connection->is_finished) {
                    connection->current_command[0] = '\0';
                    connection->command_length = 0;
                    connection->argument[0] = '\0';
                    connection->argument_length = 0;
                }
                return 0;
                //return next_state;
            }
        }
    }

    return current_state;
}

void stm_authorization_arrival(stm_states state, struct selector_key * key) {
    connection_data* connection = (connection_data*) key->data;
    connection->last_state = AUTHORIZATION;
}

void stm_authorization_departure(stm_states state, struct selector_key * key) {
    connection_data* connection = (connection_data*) key->data;
    connection->last_state = AUTHORIZATION;
}

stm_states stm_authorization_read(struct selector_key * key) {
    return read_command(key, AUTHORIZATION);
}

stm_states stm_authorization_write(struct selector_key * key){
    connection_data* connection = (connection_data* ) key->data;

    // Si es la primera vez que estamos en el estado AUTHORIZATION,
    // mandamos el mensaje de bienvenida.
    if ((int) connection->last_state == -1) {
        server_ready(connection);
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
