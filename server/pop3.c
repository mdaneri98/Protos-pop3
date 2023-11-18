#include <sys/types.h>   // socket
#include <sys/socket.h>  // socket
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include "args.h"
#include "../logger/logger.h"
#include "selector.h"
#include "../lib/parser.h"
// #include "pop3.h"
#include "buffer.h"
#include "stm.h"
#include "pop3_stm.h"


static void handle_read(struct selector_key * key);
static void handle_write(struct selector_key * key);
static void handle_close(struct selector_key * key);

extern struct args args;
// extern struct stats stats;

struct state_definition stm_states_table[] = {
        {
                .state = AUTHORIZATION,
                .on_arrival = stm_authorization_arrival,
                .on_departure = stm_authorization_departure,
                .on_read_ready = stm_authorization_read,
                .on_write_ready = stm_authorization_write
        },
        {
                .state = TRANSACTION,
                .on_arrival = stm_transaction_arrival,
                .on_departure = stm_transaction_departure,
                .on_read_ready = stm_transaction_read,
                .on_write_ready = stm_transaction_write
        },
        {
                .state = ERROR,
                .on_arrival = stm_error_arrival,
                .on_departure = stm_error_departure,
                .on_read_ready = stm_error_read,
                .on_write_ready = stm_error_write
        },
        {
                .state = QUIT,
                .on_arrival = stm_quit_arrival,
                .on_departure = stm_quit_departure,
                .on_read_ready = stm_quit_read,
                .on_write_ready = stm_quit_write
        }
};

static const struct parser_state_transition parser_command_state[] = {
        {.when = ' ', .dest = ARGUMENT, .act1 = parser_command_state_space},
        {.when = '\r', .dest = END, .act1 = parser_command_state_carriage_return},
        {.when = ANY, .dest = COMMAND, .act1 = parser_command_state_any}
};

static const struct parser_state_transition parser_argument_state[] = {
        {.when = '\r', .dest = END, .act1 = parser_argument_state_carriage_return},
        {.when = ANY, .dest = ARGUMENT, .act1 = parser_argument_state_any}
};

static const struct parser_state_transition parser_end_state[] = {
        {.when = '\n', .dest = COMMAND, .act1 = parser_end_state_line_feed},
        {.when = ANY, .dest = COMMAND, .act1 = parser_end_state_any}
};

static const struct parser_state_transition * parser_state_table[] = {
        parser_command_state,
        parser_argument_state,
        parser_end_state
};

static const size_t parser_state_n[] = {
        sizeof(parser_command_state) / sizeof(parser_command_state[0]),
        sizeof(parser_argument_state) / sizeof(parser_argument_state[0]),
        sizeof(parser_end_state) / sizeof(parser_end_state[0]),
};

static const struct parser_definition parser_definition = {
        .states = parser_state_table,
        .states_count = PARSER_STATES_COUNT,
        .start_state = COMMAND,
        .states_n = parser_state_n
};


//fd_handler que van a usar todas las conexiones al servidor (que usen el socket pasivo de pop3)
const struct fd_handler handler = {
        .handle_read  = handle_read,
        .handle_write = handle_write,
        .handle_close = handle_close,
};

connection_data* pop3_init(void* data) {
    log(LOG_DEBUG, "Initializing pop3");
    
    struct connection_data* conn = calloc(1,sizeof(struct connection_data));
    if(conn == NULL || errno == ENOMEM){
        log(LOG_ERROR,"Error reserving memory for state");
        return NULL;
    }

    // Se inicializa la maquina de estados para el cliente
    buffer_init(&conn->read_buff_object, BUFFER_SIZE, (uint8_t *) conn->read_buff);
    buffer_init(&conn->write_buff_object, BUFFER_SIZE, (uint8_t *) conn->write_buff);
    conn->parser = parser_init(parser_no_classes(), &parser_definition);
    conn->stm.states = stm_states_table;
    conn->stm.initial = AUTHORIZATION;
    conn->stm.max_state = STM_STATES_COUNT;
    stm_init(&conn->stm);

    conn->last_state = -1;
    conn->args = (struct pop3args*) data;

    log(LOG_DEBUG, "Finished initializing structure");
    return conn;    
}

/*
 * Funcion para liberar memoria asociada a la conexión
 */
void pop3_destroy(struct connection_data* connection){
    log(LOG_DEBUG, "Destroying pop3 state");
    if(connection == NULL){
        return;
    }
    
    parser_destroy(connection->parser);
    
    logf(LOG_INFO, "Closing connection with fd %d", connection->connection_fd);
    if(connection->connection_fd != -1){
        close(connection->connection_fd);
    }
}

void accept_pop_connection(struct selector_key* key) {
    connection_data *conn = NULL;
    
    // Se crea la estructura del socket activo para la conexion entrante
    struct sockaddr_storage address;
    socklen_t address_len = sizeof(address);
    
    // Se acepta la conexion entrante, se cargan los datos en el socket activo y se devuelve el fd del socket activo
    const int client_fd = accept(key->fd, (struct sockaddr *) &address, &address_len);

    //Si tuvimos un error al crear el socket activo o no lo pudimos hacer no bloqueante, no iniciamos la conexion
    if (client_fd == -1){
        log(LOG_ERROR, "Error creating active socket for user");
        goto fail;
    }

    // Seteamos el socket activo como no bloqueante
    if(selector_fd_set_nio(client_fd) == -1) {
        logf(LOG_ERROR, "Error setting not block for user %d",client_fd);
        goto fail;
    }

    if((conn = pop3_init(key->data))==NULL){
        log(LOG_ERROR, "Error on pop3 create")
        goto fail;
    }

    conn->connection_fd = client_fd;

    logf(LOG_INFO, "Registering client with fd %d", client_fd);
    //registramos en el selector el nuevo socket activo
    if(selector_register(key->s,client_fd,&handler,OP_WRITE , conn)!= SELECTOR_SUCCESS){
        log(LOG_ERROR, "Failed to register socket")
        goto fail;
    }
    

    return;

fail:
    if(client_fd != -1){
        //cerramos el socket del cliente
        close(client_fd);
    }

    //destuyo y libero la conexión
    pop3_destroy(conn);
}

/*

*/

static void handle_read(struct selector_key * key) {
    /* Indicamos que ocurrió el evento read. 
        Se ejecuta la función 'on_read_ready' para el estado actual de la maquina de estados
    */
    stm_handler_read(&((struct connection_data* ) key->data)->stm, key);
}

static void handle_write(struct selector_key * key) {
    /* Indicamos que ocurrió el evento write. 
        Se ejecuta la función 'on_write_ready' para el estado actual de la maquina de estados
    */
    stm_handler_write(&((struct connection_data* ) key->data)->stm, key);
}

static void handle_close(struct selector_key * key) {

}