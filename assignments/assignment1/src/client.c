#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_ARG_SIZE 256

#define EXIT_FAILURE 1
#define EXIT_OK 0

void usage(const char *prog)
{
	fprintf(stderr, "%s "
		"-h HOST "
                "-p PORT "
		"STRING "
                "\n", prog);

}

int main(int argc, char *argv[])
{
	char arg_host[MAX_ARG_SIZE];
	char arg_port[MAX_ARG_SIZE];
	char *thestring;
	
	arg_host[0] = arg_port[0] = 0;
	int opt;
        while ( (opt = getopt(argc, argv, "p:h:")) != -1 ) {
                switch(opt) {
                case 'p' :
                        strncpy(arg_port, optarg, MAX_ARG_SIZE);
			// Ensure null-terminated string.
			arg_port[MAX_ARG_SIZE-1] = 0; 
                        break;
		case 'h' :
                        strncpy(arg_host, optarg, MAX_ARG_SIZE);
			// Ensure null-terminated string.
			arg_host[MAX_ARG_SIZE-1] = 0; 
                        break;
                case ':' :
		case '?' :
                default :
                        usage(argv[0]);
                        return EXIT_FAILURE;
                }
        }
	// optind points to first non-option element, which should be at position argc-1
	if (optind != argc-1) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	thestring = argv[optind];
	
	if (strlen(arg_host) == 0 || strlen(arg_port) == 0 || strlen(thestring) == 0) {
		usage(argv[0]);
		return  EXIT_FAILURE;
	}

	printf("String to be reversed: %s \n", thestring);

	// TODO: send string to server using a stream socket.
	// TODO: receive string from server.
	// TODO: print received string and exit.
	
	return EXIT_OK;
}
