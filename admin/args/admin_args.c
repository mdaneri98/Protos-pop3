#include <stdio.h>     /* for printf */
#include <stdlib.h>    /* for exit */
#include <limits.h>    /* LONG_MIN et al */
#include <string.h>    /* memset */
#include <strings.h>    
#include <errno.h>
#include <getopt.h>

#include "admin_args.h"



static unsigned short port(const char * s) {
     char * end = 0;
     const long sl = strtol(s, &end, 10);

     if (end == s|| '\0' != *end
        || ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno)
        || sl < 0 || sl > USHRT_MAX) {
         fprintf(stderr, "Port should in in the range of 1-%d. Received %s\n", USHRT_MAX, s);
         exit(1);
     }
     return (unsigned short) sl;
}

static void set_add_user(char * s, struct argument* argument) {
    char * p = strchr(s, ':');
    if (p == NULL) {
        fprintf(stderr, "Password not found\n");
        exit(1);
    } else {
        *p = '\0';
        p++;
        strcpy(argument->name, "add-user");
        strcpy(argument->key, s);
        strcpy(argument->value, p);
    }
}

static void set_change_pass(char * s, struct argument* argument) {
    char * p = strchr(s, ':');
    if (p == NULL) {
        fprintf(stderr, "Password value not found\n");
        exit(1);
    } else {
        *p = '\0';
        p++;
        strcpy(argument->name, "change-pass");
        strcpy(argument->key, s);
        strcpy(argument->value, p);
    }
}

static void
usage(const char * progname) {
    fprintf(stderr,
        "Usage: %s [OPTIONS]...\n"
        "\n"
        "   --help\n"
        "   -h                               This help message.\n\n"
        "   --config-server-port <configuration server port>\n"
        "   -P <configuration server port>   Port for configuration client connections\n\n"
        "   --token <token>\n"
        "   -t <token>                       Authentication token for the client.\n\n"
        "   --version\n"
        "   -v                               Prints version information.\n"
        "  --add-user <user>:<password>\n"
        "   -u <user>:<password>             Add a user to the server.\n\n"
        "   --change-pass <user>:<password>\n"
        "   -p <user>:<password>             Change the password of a user.\n\n"
        "   --remove-user <user>\n"
        "   -r <user>                        Remove a user from the server.\n\n"
        "   --change-maildir <maildir>\n"
        "   -m <maildir>                     Change mail directory.\n\n"
        "   --get-max-mails \n"
        "   -g                              Get the maximum number of mails.\n\n"
        "   --set-max-mails <number>\n"
        "   -s <number>               Set the maximum number of mails.\n\n"
        "   --stat-historic-connections\n"
        "   -i                       Get the number of historic connections.\n\n"
        "   --stat-current-connections\n"
        "   -c                       Get the number of current connections.\n\n"
        "   --stat-bytes-transferred\n"
        "   -b                       Get the number of bytes transferred.\n\n"
        "\n",
        progname);
    exit(1);
}

void parse_args(const int argc, char **argv, struct args * args) {
    memset(args, 0, sizeof(*args));

    int c;

    while (true) {
        int option_index = 0;
        static struct option long_options[] = {
            { "help",                       no_argument,       0, 'h' },
            { "port",                       required_argument, 0, 'P' },
            { "token",                      required_argument, 0, 't' },
            { "version",                    no_argument,       0, 'v' },
            { "add-user",                   required_argument, 0, 'u' },
            { "change-pass",                required_argument, 0,'p'},
            { "change-maildir",             required_argument, 0,'m'},
            { "remove-user",                required_argument, 0,'r'},
            { "get-max-mails",              no_argument      ,0,'g'},
            { "set-max-mails",              required_argument,0,'s'},
            { "stat-historic-connections",  no_argument      ,0,'i'},
            { "stat-current-connections",   no_argument      ,0,'c'},
            { "stat-bytes-transferred",     no_argument      ,0,'b'},
            { 0,                            0                ,0,0 }
        };

        c = getopt_long(argc, argv, "hP:t:vu:pm:r:gs:icb", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
            case 'h':
                usage(argv[0]);
                break;
            case 'P':
                args->server_port = port(optarg);
                break;
            case 't':
                if(strlen(optarg) != CLIENT_TOKEN_LENGTH) {
                    fprintf(stderr, "Token invalid. Must be six alphanumerical characters long: %s.\n", optarg);
                    exit(1);
                }
                strcpy(args->arguments[TOKEN].name, "token");
                strcpy(args->arguments[TOKEN].key, optarg);
                strcpy(args->arguments[TOKEN].value, "");
                args->arguments_count++;
                break;
            case 'v':
                strcpy(args->arguments[VERSION].name, "version");
                strcpy(args->arguments[VERSION].key, "");
                strcpy(args->arguments[VERSION].value, "");
                args->arguments_count++;
                break;
            case 'u':
                set_add_user(optarg, &args->arguments[ADD_USER]);
                args->arguments_count++;
                break;
            case 'p':
                set_change_pass(optarg, &args->arguments[CHANGE_PASS]);
                args->arguments_count++;
                break;
            case 'm':
                strcpy(args->arguments[REMOVE_USER].name, "change-maildir");
                strcpy(args->arguments[REMOVE_USER].key, optarg);
                strcpy(args->arguments[REMOVE_USER].value, "");
                args->arguments_count++;
                break;
            case 'r':
                strcpy(args->arguments[REMOVE_USER].name, "remove-user");
                strcpy(args->arguments[REMOVE_USER].key, optarg);
                strcpy(args->arguments[REMOVE_USER].value, "");
                args->arguments_count++;
                break;
            case 'g':
                strcpy(args->arguments[GET_MAX_MAILS].name, "get-max-mails");
                strcpy(args->arguments[GET_MAX_MAILS].key, "");
                strcpy(args->arguments[GET_MAX_MAILS].value, "");
                args->arguments_count++;
                break;
            case 's':
                strcpy(args->arguments[SET_MAX_MAILS].name, "set-max-mails");
                strcpy(args->arguments[SET_MAX_MAILS].key, optarg);
                strcpy(args->arguments[SET_MAX_MAILS].value, "");
                args->arguments_count++;
                break;
            case 'i':
                strcpy(args->arguments[STAT_HISTORIC_CONNECTIONS].name, "stat-historic-connections");
                strcpy(args->arguments[STAT_HISTORIC_CONNECTIONS].key, "");
                strcpy(args->arguments[STAT_HISTORIC_CONNECTIONS].value, "");
                args->arguments_count++;
                break;
            case 'c':
                strcpy(args->arguments[STAT_CURRENT_CONNECTIONS].name, "stat-current-connections");
                strcpy(args->arguments[STAT_CURRENT_CONNECTIONS].key, "");
                strcpy(args->arguments[STAT_CURRENT_CONNECTIONS].value, "");
                args->arguments_count++;
                break;
            case 'b':
                strcpy(args->arguments[STAT_BYTES_TRANSFERRED].name, "stat-bytes-transferred");
                strcpy(args->arguments[STAT_BYTES_TRANSFERRED].key, "");
                strcpy(args->arguments[STAT_BYTES_TRANSFERRED].value, "");
                args->arguments_count++;
                break;
            default:
                fprintf(stderr, "Unknown argument %d.\n", c);
                exit(1);
        }

    }
    if (args->arguments[TOKEN].name[0] == '\0') {
        fprintf(stderr, "Token argument must be provided.\n");
        exit(1);
    }
    if (optind < argc) {
        fprintf(stderr, "Argument not accepted: ");
        while (optind < argc) {
            fprintf(stderr, "%s ", argv[optind++]);
        }
        fprintf(stderr, "\n");
        exit(1);
    }
}