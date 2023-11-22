#include <stdio.h>     /* for printf */
#include <stdlib.h>    /* for exit */
#include <limits.h>    /* LONG_MIN et al */
#include <string.h>    /* memset */
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

static void version(void) {
    fprintf(
            stdout,
            "POP3 Server v1.0\n"
            "ITBA - 72.07 Protocolos de Comunicación 20232Q\n\n");
}

static void user(char * s, struct users * user) {
    char * p = strchr(s, ':');
    if (p == NULL) {
        fprintf(stderr, "Password not found\n");
        exit(1);
    } else {
        *p = '\0';
        p++;
        strcpy(user->name, s);
        strcpy(user->pass, p);
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
        "   --get-max-mails \n"
        "   -g <user>                        Get the maximum number of mails.\n\n"
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

    args->client_port = CLIENT_PORT;

    int c;

    while (true) {
        int option_index = 0;
        static struct option long_options[] = {
            { "help",              no_argument,       0, 'h' },
            { "port",required_argument, 0, 'P' },
            { "token",             required_argument, 0, 't' },
            { "version",           no_argument,       0, 'v' },
            { "add-user",              required_argument, 0, 'u' },
            { "change-pass"             required_argument,0,'p'},
            { "remove-user",required_argument,0,'r'},
            { "get-max-mails",no_argument,0,'g'},
            { "set-max-mails",required_argument,0,'s'},
            { "stat-historic-connections",no_argument,0,'i'},
            { "stat-current-connections",no_argument,0,'c'},
            { "stat-bytes-transferred",no_argument,0,'b'},
            { 0,                   0,                 0, 0 }
        };

        c = getopt_long(argc, argv, "hP:t:vu:p:r:gs:icb", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
            case 'h':
                usage(argv[0]);
                break;
            case 'P':
                args->client_port = port(optarg);
                break;
            case 't':
                /*
                if(strlen(optarg) != CLIENT_TOKEN_LENGTH) {
                    fprintf(stderr, "Token invalid. Must be six alphanumerical characters long: %s.\n", optarg);
                    exit(1);
                }
                */
                strcpy(args->token, optarg);
                break;
            case 'v':
                version();
                break;
            case 'u':
                //TODO checkear cantidad de usarios actuales y el maximo
                args->new_user = optarg;
                args->new_pass = optarg;
                break;
            case 'p':
                args->change_user = optarg;
                args->change_pass = optarg;
                break;
            case 'r':
                args->remove_user = optarg;
                break;
            case 'g':

                break;
            case 's':
                args->new_max_mails = optarg;
                break;
            case 'i':

                break;
            case 'c':

                break;
            case 'b':

                break;
            default:
                fprintf(stderr, "Unknown argument %d.\n", c);
                exit(1);
        }

    }
    if (args->token[0] == '\0') {
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