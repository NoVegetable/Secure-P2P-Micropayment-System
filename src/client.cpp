#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <vector>
#include "service_handles.h"

using std::string;
using std::vector;

char *server_ip;
unsigned short server_port;
int server_fd;
string server_public_key;
char login_username[USERNAME_MAXLEN] = { 0 };
unsigned int account_balance;
unsigned int num_online;
vector<Peer> peer_list;

int main(int argc, char *argv[]) {

    if(argc < 3) {
        fprintf(stderr, "[Error] Either server IP or PORT is not given.\n");
        return EXIT_FAILURE;
    }

    server_ip = argv[1];
    server_port = (unsigned short) atoi(argv[2]);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "[Error] Failed to create socket.\n");
        return INTERNAL_FAILURE;
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        fprintf(stderr, "[Error] Failed to connect to the server.");
        return EXIT_FAILURE;
    }

    printf("Connect to server at %s:%d\n", server_ip, server_port);

    int opt = -1;
    do {
        printf("\n\033[1mPress ENTER to continue...\033[0m");
        fflush(stdout);
        char c;
        do {
            scanf("%c", &c);
        } while (c != '\n');

        printf("\033[H\033[J");
        printf("Please select a service:\n\n"
               "[0] Exit.\n"
               "[1] Register.\n"
               "[2] Login.\n"
               "[3] List online information.\n"
               "[4] Transaction.\n\n");
        if (strlen(login_username) > 0)
            printf("%s ", login_username);
        printf("\033[1m>\033[0m ");
        
        scanf("%d", &opt);
        getchar();
        
        char recv_buf[BUF_MAXLEN] = { 0 };
        int retcode = service_handles(opt, server_fd, recv_buf);

        if (retcode == -1) {
            fprintf(stderr, "[Error] Failed to handle the required service.\n");
            close(server_fd);
            return EXIT_FAILURE;
        }
        
        if (strlen(recv_buf) > 0) {
            printf("Receive the following message from the server:\n%s", recv_buf);
        }

    } while (opt != SERVICE_EXIT);

    return 0;
}
