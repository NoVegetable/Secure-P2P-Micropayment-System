#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string>
#include <vector>
#include "server.h"

using std::string;
using std::vector;

vector<Peer> peer_list;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* XTerm control sequences reference: https://invisible-island.net/xterm/ctlseqs/ctlseqs.html */
void to_alternate_screen()
{
    static int call_count = 0;
    if (call_count == 0) {
        /* CSI ?1049h: Save cursor as in DECSC, xterm. After saving the cursor, switch to the Alternate Screen Buffer, clearing it first. */
        printf("\033[?1049h");
        fflush(stdout);
        ++call_count;
    }
}

void restore_normal_screen()
{
    /* CSI ?47l: Use Normal Screen Buffer, xterm. */
    /* CSI ?1048l: Restore cursor as in DECRC, xterm. */
    printf("\033[?47l\033[?1048l");
    fflush(stdout);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Port number is not given.\n");
        return EXIT_FAILURE;
    }

    unsigned short port = (unsigned short) atoi(argv[1]);

    int main_fd;
    if ((main_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "[Error] Failed to create socket.\n");
        return EXIT_FAILURE;
    }

    struct sockaddr_in main_addr;
    main_addr.sin_family = AF_INET;
    main_addr.sin_addr.s_addr = INADDR_ANY;
    main_addr.sin_port = htons(port);

    if (bind(main_fd, (const struct sockaddr *) &main_addr, sizeof(main_addr)) == -1) {
        close(main_fd);
        fprintf(stderr, "[Error] Failed to bind on port %hu\n", port);
        return EXIT_FAILURE;
    }

    if (listen(main_fd, 8) == -1) {
        close(main_fd);
        fprintf(stderr, "[Error] Failed to listen at port %hu\n", port);
        return EXIT_FAILURE;
    }

    printf("Now listening at port %hu\n", port);

    while (true) {
        struct sockaddr_in client_addr;
        int addrlen = sizeof(client_addr);
        int client_fd;
        if ((client_fd = accept(main_fd, (struct sockaddr *) &client_addr, (socklen_t *) &addrlen)) == -1) {
            close(main_fd);
            fprintf(stderr, "[Error] Failed to accept a connection");
            return EXIT_FAILURE;
        }

        printf("Accept connection from %s:%hu\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    }

    return 0;
}
