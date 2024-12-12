#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string>
#include <vector>
#include <unordered_map>
#include "server.h"
#include "crypto.h"

using std::string;
using std::vector;
using std::unordered_map;

vector<Peer> peer_list;
pthread_mutex_t peer_list_mutex = PTHREAD_MUTEX_INITIALIZER;
string public_key;
string private_key;
unordered_map<string, int> client_fds;
time_t start, end;

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
    /* CSI ?1049l: Use Normal Screen Buffer and restore cursor as in DECRC, xterm.*/
    printf("\033[?1049l");
    fflush(stdout);
}

void signal_handler(int sig)
{
    if (sig == SIGINT) {
        end = time(NULL);
        restore_normal_screen();
        printf("\n========== Server closed ==========\n");
        printf("Time elapsed: %ld seconds\n", end - start);
        exit(0);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "[Error] Port number is not given.\n");
        return EXIT_FAILURE;
    }

    signal(SIGINT, signal_handler);

    RSA *rsa = generate_rsa_key();
    char *public_key_, *private_key_;
    read_rsa_publickey(rsa, &public_key_);
    read_rsa_privatekey(rsa, &private_key_);
    public_key = public_key_;
    private_key = private_key_;
    RSA_free(rsa);
    free(public_key_);
    free(private_key_);

    unsigned short port = (unsigned short) atoi(argv[1]);

    int main_fd;
    if ((main_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "[Error] Failed to create socket.\n");
        return EXIT_FAILURE;
    }

    const int enable = 1;
    setsockopt(main_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

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

    to_alternate_screen();
    printf("\033[H\033[J");

    start = time(NULL);

    printf("[Log] Server starts successfully. Now listening at port %hu\n", port);

    while (true) {
        struct sockaddr_in client_addr;
        int client_addrlen = sizeof(client_addr);
        int client_fd;
        if ((client_fd = accept(main_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addrlen)) == -1) {
            close(main_fd);
            fprintf(stderr, "[Error] Failed to accept a connection");
            return EXIT_FAILURE;
        }

        printf("[Log] Accept connection from %s:%hu\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        printf("[Log] Sending public key to the client ...");
        fflush(stdout);
        if (send(client_fd, public_key.c_str(), public_key.size(), 0) == -1) {
            fprintf(stderr, "[Error] Failed to send public key to the client.\n");
            return EXIT_FAILURE;
        }
        printf(" Done.\n");

        pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(&tid, &attr, handle_client, &client_fd);
        pthread_detach(tid);
    }

    return 0;
}
