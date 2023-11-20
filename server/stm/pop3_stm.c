#include "../server_constants.h"
#include "../../lib/buffer.h"
#include "../../lib/logger/logger.h"
#include "../../lib/selector.h"
#include "../../lib/parser.h"
#include "../args/args.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <strings.h>
#include <sys/dir.h>

typedef enum try_state
{
    TRY_DONE,
    TRY_PENDING
} try_state;

try_state try_write(const char *str, buffer *buff)
{
    size_t max = 0;
    uint8_t *ptr = buffer_write_ptr(buff, &max);
    size_t message_len = strlen(str);
    /*
    if(max<message_len){
        //vuelvo a intentar despues
        return TRY_PENDING;
    }
    */
    // Manda el mensaje parcialmente si no hay espacio
    memcpy(ptr, str, message_len);
    //    strncpy((char*)ptr, str, message_len);
    buffer_write_adv(buff, (ssize_t)message_len);
    return TRY_DONE;
}

stm_states user_handler(struct selector_key *key, connection_data *conn)
{
    log(LOG_DEBUG, "FD %d: USER command");

    struct users *users = conn->args->users;
    size_t users_count = conn->args->users_count;

    // The server may return a positive response even though no
    // such mailbox exists.
    for (size_t i = 0; i < users_count; i++)
    {
        // Si el usuario existe, seteamos el maildir y el username en la sesion actual.
        if (strcmp(users[i].name, conn->argument) == 0)
        {
            strcpy(conn->user->name, conn->argument);

            char *msj = "+OK valid user\r\n";
            try_write(msj, &(conn->out_buff_object));

            conn->command_error = false;
            return AUTHORIZATION;
        }
    }
    conn->user->name[0] = '\0';

    char *msj = "-ERR invalid user\r\n";
    try_write(msj, &(conn->out_buff_object));

    // Si el usuario no existe, seteamos el error en true.
    conn->command_error = true;
    return AUTHORIZATION;
}

stm_states pass_handler(struct selector_key *key, connection_data *conn)
{
    logf(LOG_DEBUG, "FD %d: PASS command", key->fd);

    struct users *user = conn->user;
    if (user->name[0] == '\0')
    {
        char *msj = "-ERR no username given\r\n";
        try_write(msj, &(conn->out_buff_object));
        conn->command_error = true;
        return AUTHORIZATION;
    }

    bool authenticated = false;
    struct users *users = conn->args->users;
    size_t users_count = conn->args->users_count;
    for (size_t i = 0; i < users_count; i++)
    {
        // Si el usuario existe, seteamos el maildir y el username en la sesion actual.
        if (strcmp(users[i].name, user->name) == 0)
        {
            authenticated = strcmp(users[i].pass, conn->argument) == 0;
            break;
        }
    }
    //---

    // El usuario existe, es la contraseña correcta?
    if (authenticated)
    {
        char base_directory[PATH_SIZE];
        strcpy(base_directory, conn->args->mail_directory);

        conn->current_session.maildir[0] = '\0';
        strcat(conn->current_session.maildir, base_directory);
        strcat(conn->current_session.maildir, "/");
        strcat(conn->current_session.maildir, conn->user->name);
        strcat(conn->current_session.maildir, "/cur");

        DIR *directory = opendir(conn->current_session.maildir);
        if (directory == NULL)
        {
            logf(LOG_DEBUG, "FD %d: User %s doesn't have directory entry at %s and gone with error %s", key->fd, conn->user->name, conn->current_session.maildir, strerror(errno));

            conn->command_error = true;
            conn->current_session.maildir[0] = '\0';
            conn->user->name[0] = '\0';

            char *msj = "-ERR invalid user\r\n";
            try_write(msj, &(conn->out_buff_object));

            return AUTHORIZATION;
        }
        logf(LOG_DEBUG, "FD %d: User %s has directory entry at %s.", key->fd, conn->user->name, conn->current_session.maildir);
        closedir(directory);

        logf(LOG_DEBUG, "FD %d: User %s logged_in", key->fd, conn->user->name);
        conn->user->logged_in = true;
        strcpy(conn->current_session.username, conn->user->name);
        strcpy(conn->user->pass, conn->argument);

        char *msj = "+OK logged in\r\n";
        try_write(msj, &(conn->out_buff_object));

        conn->command_error = false;
        return TRANSACTION;
    }

    logf(LOG_DEBUG, "FD %d: User %s password %s incorrect", key->fd, conn->user->name, conn->argument);
    conn->command_error = true;
    return AUTHORIZATION;
}

stm_states stat_handler(struct selector_key *key, connection_data *conn)
{
    log(LOG_DEBUG, "FD %d: STAT command");

    size_t mail_count = conn->current_session.mail_count;
    size_t maildir_size = conn->current_session.maildir_size;

    char msj[100];
    sprintf(msj, "+OK %d %d\r\n", (int)mail_count, (int)maildir_size);
    try_write(msj, &(conn->out_buff_object));
    return TRANSACTION;
}

stm_states list_handler(struct selector_key *key, connection_data *conn)
{
    if (conn->argument_length > 0)
    {
        char *ptr;
        long arg = strtol(conn->argument, &ptr, 10);
        if ((size_t)arg - 1 > conn->current_session.mail_count || ptr[0] != '\0')
        {
            // INGRESAR MENSAJE DE ERROR
            return TRANSACTION;
        }

        char msg[BUFFER_SIZE];
        sprintf(msg, "+OK %zu %zu", arg, conn->current_session.mails[arg - 1].size);
        try_write(msg, &(conn->out_buff_object));
        return TRANSACTION;
    }

    char msg[10] = {0};
    sprintf(msg, "+OK\r\n");
    try_write(msg, &(conn->out_buff_object));

    size_t index = 0;
    while (index < conn->current_session.mail_count)
    {
        if (!conn->current_session.mails[index].deleted)
        {
            char mail_info[50];
            sprintf(mail_info, "%ld %ld\r\n", index + 1, conn->current_session.mails[index].size);
            try_write(mail_info, &(conn->out_buff_object));
        }
        index++;
    }

    return TRANSACTION;
}

stm_states retr_handler(struct selector_key *key, connection_data *conn)
{
    log(LOG_DEBUG, "FD %d: RETR command");

    size_t mail_number = atoi(conn->argument);

    if (mail_number <= 0 || mail_number > conn->current_session.mail_count)
    {
        log(LOG_DEBUG, "FD %d: Error. Invalid mail number");
        char *msj = "-ERR no such message\r\n";
        try_write(msj, &(conn->out_buff_object));
        return TRANSACTION;
    }

    char mail_path[PATH_SIZE];
    strcpy(mail_path, conn->current_session.mails[mail_number - 1].path);

    FILE *mail_file = fopen(mail_path, "r");
    if (mail_file == NULL)
    {
        logf(LOG_DEBUG, "FD %d: Error opening mail file with path %s", key->fd, mail_path);
        char *msj = "-ERR no such message\r\n";
        try_write(msj, &(conn->out_buff_object));
        return TRANSACTION;
    }

    char *msj = "+OK\r\n";
    try_write(msj, &(conn->out_buff_object));

    char *ptr;
    size_t n;
    ptr = (char *)buffer_write_ptr(&conn->out_buff_object, &n);

    size_t bytes_read = fread(ptr, sizeof(char), n, mail_file);
    buffer_write_adv(&conn->out_buff_object, bytes_read);

    fclose(mail_file);

    try_write("\r\n.\r\n", &(conn->out_buff_object));

    return TRANSACTION;
}

stm_states dele_handler(struct selector_key *key, connection_data *conn)
{
    return TRANSACTION;
}

stm_states noop_handler(struct selector_key *key, connection_data *conn)
{
    log(LOG_DEBUG, "FD %d: NOOP command");
    conn->is_finished = true;
    char msj[100];
    sprintf(msj, "+OK\r\n");
    try_write(msj, &(conn->out_buff_object));
    return TRANSACTION;
}

stm_states rset_handler(struct selector_key *key, connection_data *conn)
{
    log(LOG_DEBUG, "RSET");
    size_t maildir_size = 0;
    for (size_t i = 0; i < conn->current_session.mail_count; i++)
    {
        conn->current_session.mails[i].deleted = false;
        maildir_size += conn->current_session.mails[i].size;
    }
    conn->current_session.maildir_size = maildir_size;
    char msj[100];
    sprintf(msj, "+OK\r\n");
    try_write(msj, &(conn->out_buff_object));
    return TRANSACTION;
}

stm_states quit_handler(struct selector_key *key, connection_data *conn)
{
    log(LOG_DEBUG, "QUIT");
    for (size_t i = 0; i < conn->current_session.mail_count; i++)
    {
        if (conn->current_session.mails[i].deleted)
        {
            remove(conn->current_session.mails[i].path);
        }
    }
    char msj[100];
    sprintf(msj, "+OK\r\n");
    try_write(msj, &(conn->out_buff_object));
    return TRANSACTION;
}

typedef enum command_args
{
    REQUIRED,
    OPTIONAL,
    EMPTY
} command_args;

struct command
{
    const char *name;
    command_args arguments;
    stm_states (*handler)(struct selector_key *, struct connection_data *);
};

struct command commands[] = {
    {.name = "USER",
     .arguments = REQUIRED,
     .handler = user_handler},
    {.name = "PASS",
     .arguments = REQUIRED,
     .handler = pass_handler},

    {.name = "STAT",
     .arguments = EMPTY,
     .handler = stat_handler},
    {.name = "LIST",
     .arguments = OPTIONAL,
     .handler = list_handler},
    {.name = "RETR",
     .arguments = REQUIRED,
     .handler = retr_handler},
    {.name = "DELE",
     .arguments = REQUIRED,
     .handler = dele_handler},
    {.name = "NOOP",
     .arguments = EMPTY,
     .handler = noop_handler},
    {.name = "RSET",
     .arguments = EMPTY,
     .handler = rset_handler},
    {.name = "QUIT",
     .arguments = EMPTY,
     .handler = quit_handler}};

bool server_ready(struct connection_data *conn)
{
    char *msj = "+OK POP3 server ready\r\n";

    size_t n;
    char *ptr = (char *)buffer_write_ptr(&conn->out_buff_object, &n);
    if (n >= strlen(msj))
    {
        logf(LOG_DEBUG, "Writing message to out_buff: %s", msj);
        strncpy(ptr, msj, strlen(msj));
        buffer_write_adv(&conn->out_buff_object, (ssize_t)strlen(msj));
        return true;
    }
    else
    {
        log(LOG_DEBUG, "out_buffer is full. Can't write welcome message.");
        return false;
    }
}

stm_states stm_server_read(struct selector_key *key)
{
    connection_data *connection = (connection_data *)key->data;
    return connection->stm.current->state;
    /*
        if(!buffer_can_read[&connection->in_buff_object]){

        }
    */
}

void clean_command(connection_data *connection)
{
    for (int i = 0; i <= (int)connection->command_length; i++)
    {
        connection->current_command[i] = '\0';
    }
    connection->command_length = 0;
    for (int i = 0; i <= (int)connection->argument_length; i++)
    {
        connection->argument[i] = '\0';
    }
    connection->argument_length = 0;
}

stm_states read_command(struct selector_key *key, stm_states current_state)
{
    struct connection_data *connection = (struct connection_data *)key->data;
    char *ptr;

    if (!buffer_can_read(&connection->in_buff_object))
    {
        size_t write_bytes;

        // Escribimos en el buffer de lectura.
        ptr = (char *)buffer_write_ptr(&connection->in_buff_object, &write_bytes);
        ssize_t n = recv(key->fd, ptr, write_bytes, 0);
        if (n == 0)
        {
            return QUIT;
        }
        buffer_write_adv(&connection->in_buff_object, n);
    }
    // Acá ya tenemos actualizado el buffer de lectura.

    size_t read_bytes;
    ptr = (char *)buffer_read_ptr(&connection->in_buff_object, &read_bytes);
    for (size_t i = 0; i < read_bytes; i++)
    {
        // Alimentamos el parser con lo leido.
        const struct parser_event *event = parser_feed(connection->parser, ptr[i], connection);
        buffer_read_adv(&connection->in_buff_object, 1);

        /* event-type es modificado en pop3_parser.
            Una vez que se termina de parsear el comando, se setea el tipo de evento en VALID_COMMAND o INVALID_COMMAND.
        */
        logf(LOG_DEBUG, "FD %d: Current command: %s - Length: %d", key->fd, connection->current_command, (int)connection->command_length);
        logf(LOG_DEBUG, "FD %d: Current arg: %s - Length: %d", key->fd, connection->argument, (int)connection->argument_length);
        logf(LOG_DEBUG, " event = %d", event->type);
        if (event->type == VALID_COMMAND)
        {
            logf(LOG_DEBUG, "FD %d: Valid command: %s", key->fd, connection->current_command);
            for (size_t j = 0; j < COMMAND_QTY; j++)
            {
                struct command maybe_command = commands[j];
                if (strcasecmp(maybe_command.name, connection->current_command) == 0)
                {
                    if ((maybe_command.arguments == REQUIRED && connection->argument_length > 0) ||
                        (maybe_command.arguments == EMPTY && connection->argument_length == 0) ||
                        (maybe_command.arguments == OPTIONAL))
                    {

                        // Ejecutamos el comando.
                        stm_states next_state = maybe_command.handler(key, connection);

                        // Vamos a escribir.
                        selector_set_interest_key(key, OP_WRITE);

                        clean_command(connection);
                        return next_state;
                    }
                    else
                    {
                        logf(LOG_DEBUG, "FD %d: Error. Invalid argument", key->fd);
                        clean_command(connection);
                        return ERROR;
                    }
                }
            }
            logf(LOG_DEBUG, "FD %d: Error. Invalid command for state", key->fd);
            clean_command(connection);
            return ERROR;
        }
        else if (event->type == INVALID_COMMAND)
        {
            logf(LOG_DEBUG, "FD %d: Error. Invalid command: %s", key->fd, connection->current_command);
            bool saw_carriage_return = ptr[i] == '\r';
            while (i < read_bytes)
            {
                char c = (char)buffer_read(&connection->in_buff_object);
                if (c == '\r')
                {
                    saw_carriage_return = true;
                }
                else if (c == '\n')
                {
                    if (saw_carriage_return)
                    {
                        clean_command(connection);
                        return ERROR;
                    }
                }
                else
                {
                    saw_carriage_return = false;
                }
                i++;
            }
            clean_command(connection);
            return ERROR;
        }
    }
    clean_command(connection);
    return current_state;
}

stm_states write_command(struct selector_key *key, stm_states current_state)
{
    connection_data *connection = (connection_data *)key->data;
    char *ptr;

    if (buffer_can_read(&connection->out_buff_object))
    {
        size_t bytes_to_send;
        ptr = (char *)buffer_read_ptr(&connection->out_buff_object, &bytes_to_send);
        ssize_t bytes_send = send(key->fd, ptr, bytes_to_send, MSG_NOSIGNAL);
        buffer_read_adv(&connection->out_buff_object, bytes_send);

        selector_set_interest_key(key, OP_READ);
    }

    return current_state;
}

// ------------------ Estados de la maquina de estados ------------------ //

void stm_authorization_arrival(stm_states state, struct selector_key *key)
{
}

void stm_authorization_departure(stm_states state, struct selector_key *key)
{
    connection_data *connection = (connection_data *)key->data;
    connection->last_state = state;
}

stm_states stm_authorization_read(struct selector_key *key)
{
    logf(LOG_DEBUG, "FD %d: stm_authorization_read", key->fd);
    return read_command(key, AUTHORIZATION);
}

stm_states stm_authorization_write(struct selector_key *key)
{
    logf(LOG_DEBUG, "FD %d: stm_authorization_write", key->fd);

    connection_data *connection = (connection_data *)key->data;

    // Si es la primera vez que estamos en el estado AUTHORIZATION,
    // mandamos el mensaje de bienvenida.
    if (connection->last_state == -1)
    {
        logf(LOG_DEBUG, "FD %d: Sending welcome message", key->fd);
        bool done = server_ready(connection);
        if (done)
        {
            connection->last_state = AUTHORIZATION;
        }
    }

    return write_command(key, AUTHORIZATION);
}

void stm_transaction_arrival(stm_states state, struct selector_key *key)
{
    logf(LOG_DEBUG, "FD %d: stm_transaction_arrival", key->fd);

    connection_data *connection = (connection_data *)key->data;
    if (connection->current_session.mail_count != 0)
    {
        return;
    }

    DIR *directory = opendir(connection->current_session.maildir);
    struct dirent *file;
    while (connection->current_session.mail_count < connection->args->max_mails && (file = readdir(directory)) != NULL)
    {
        if (strcmp(".", file->d_name) == 0 || strcmp("..", file->d_name) == 0)
        {
            continue;
        }
        logf(LOG_DEBUG, "FD %d: Found mail for %s with file name %s", key->fd, connection->user->name, file->d_name);

        size_t i = connection->current_session.mail_count;

        if (connection->current_session.mails == NULL)
        {
            connection->current_session.mails = calloc(1, sizeof(struct mail));
        }
        else
        {
            connection->current_session.mails = realloc(connection->current_session.mails, sizeof(struct mail) * (i + 1));
        }
        strcat(connection->current_session.mails[i].path, connection->current_session.maildir);
        strcat(connection->current_session.mails[i].path, "/");
        strcat(connection->current_session.mails[i].path, file->d_name);

        logf(LOG_DEBUG, "FD %d: Added mail for %s with path %s", key->fd, connection->user->name, connection->current_session.mails[i].path);

        struct stat file_stat;
        if (stat(connection->current_session.mails[i].path, &file_stat) == 0)
        {
            connection->current_session.maildir_size += file_stat.st_size; // Acumula el tamaño del archivo
            connection->current_session.mail_count++;
        }
        else
        {
            logf(LOG_DEBUG, "FD %d: Error getting file size for %s", key->fd, connection->current_session.mails[i].path);
        }
    }
    closedir(directory);
}

void stm_transaction_departure(stm_states state, struct selector_key *key)
{
    connection_data *connection = (connection_data *)key->data;
    connection->last_state = state;
}

stm_states stm_transaction_read(struct selector_key *key)
{
    return read_command(key, TRANSACTION);
}

stm_states stm_transaction_write(struct selector_key *key)
{
    return write_command(key, TRANSACTION);
}

stm_states stm_error_arrival(stm_states state, struct selector_key *key)
{
    connection_data *connection = (connection_data *)key->data;
    return connection->stm.current->state;
}

stm_states stm_error_departure(stm_states state, struct selector_key *key)
{
    connection_data *connection = (connection_data *)key->data;
    return connection->stm.current->state;
}

stm_states stm_error_read(struct selector_key *key)
{
    connection_data *connection = (connection_data *)key->data;
    return connection->stm.current->state;
}

stm_states stm_error_write(struct selector_key *key)
{
    connection_data *connection = (connection_data *)key->data;
    return connection->stm.current->state;
}

stm_states stm_quit_arrival(stm_states state, struct selector_key *key)
{
    connection_data *connection = (connection_data *)key->data;
    return connection->stm.current->state;
}

stm_states stm_quit_departure(stm_states state, struct selector_key *key)
{
    connection_data *connection = (connection_data *)key->data;
    return connection->stm.current->state;
}

stm_states stm_quit_read(struct selector_key *key)
{
    connection_data *connection = (connection_data *)key->data;
    return connection->stm.current->state;
}

stm_states stm_quit_write(struct selector_key *key)
{
    connection_data *connection = (connection_data *)key->data;
    return connection->stm.current->state;
}
