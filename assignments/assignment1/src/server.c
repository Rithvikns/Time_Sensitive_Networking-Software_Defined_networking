#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAX_ARG_SIZE 256

#define EXIT_FAILURE 1
#define EXIT_OK 0

volatile int do_exit = 0;

void usage(const char *prog)
{
	fprintf(stderr, "%s "
                "-p PORT "
                "\n", prog);

}

static void sigint_handler(int signal)
{
        do_exit = 1;
}

int main(int argc, char *argv[])
{
	char arg_port[MAX_ARG_SIZE];

	arg_port[0] = 0;
	int opt;
        while ( (opt = getopt(argc, argv, "p:")) != -1 ) {
                switch(opt) {
                case 'p' :
                        strncpy(arg_port, optarg, MAX_ARG_SIZE);
			// Ensure null-terminated string.
			arg_port[MAX_ARG_SIZE-1] = 0; 
                        break;
                case ':' :
		case '?' :
                default :
                        usage(argv[0]);
                        return EXIT_FAILURE;
                }
        }

	if (strlen(arg_port) == 0) {
		usage(argv[0]);
		return  EXIT_FAILURE;
	}

	// Catch SIGINT when user exits applications (Ctrl-C).
        if ( signal(SIGINT, sigint_handler) == SIG_ERR ) {
                perror("Could not attach signal handler");
                return EXIT_FAILURE;
        }
	
	// TODO: create and initialize server stream socket

	while (!do_exit) {
		// TODO: receive string from client
		// TODO: reverse string
		// TODO: send back reversed string to client
	}
	
	return EXIT_OK;
}
