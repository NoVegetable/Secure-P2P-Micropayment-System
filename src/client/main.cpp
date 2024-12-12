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
#include "client.h"
#include "crypto.h"

using std::string;
using std::vector;

char *server_ip;
unsigned short server_port;
int server_fd;
string public_key;
string private_key;
string server_public_key;
char login_username[USERNAME_MAXLEN + 1] = { 0 };
unsigned int account_balance;
unsigned int num_online;
vector<Peer> peer_list;

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

int main(int argc, char *argv[]) {
    if(argc < 3) {
        fprintf(stderr, "[Error] Either server IP or PORT is not given.\n");
        return EXIT_FAILURE;
    }

    RSA *rsa = generate_rsa_key();
    char *public_key_, *private_key_;
    read_rsa_publickey(rsa, &public_key_);
    read_rsa_privatekey(rsa, &private_key_);
    public_key = public_key_;
    private_key = private_key_;
    RSA_free(rsa);
    free(public_key_);
    free(private_key_);

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
        fprintf(stderr, "[Error] Failed to connect to the server.\n");
        return EXIT_FAILURE;
    }

    printf("Connect to server at %s:%d\n", server_ip, server_port);
    printf("Acquire server public key ...");
    fflush(stdout);
    char server_public_key_buf[2048] = { 0 };
    if (recv(server_fd, server_public_key_buf, sizeof(server_public_key_buf), 0) == -1) {
        fprintf(stderr, "[Error] Failed to acquried the public key from server.\n");
        return EXIT_FAILURE;
    }
    server_public_key = server_public_key_buf; 
    printf(" Done\n");
    
    int opt = -1;
    do {
        printf("\n\033[1mPress ENTER to continue...\033[0m");
        fflush(stdout);
        char c;
        do {
            scanf("%c", &c);
        } while (c != '\n');
        
        to_alternate_screen(); // called only once

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
        
        char recv_buf[BUF_MAXLEN + 1] = { 0 };
        int retcode = service_handles(opt, server_fd, recv_buf);

        if (retcode == -1) {
            fprintf(stderr, "[Error] Failed to handle the required service.\n");
            close(server_fd);
            restore_normal_screen();
            return EXIT_FAILURE;
        }
        
        if (strlen(recv_buf) > 0) {
            printf("Receive the following message from the server:\n%s", recv_buf);
        }

    } while (opt != SERVICE_EXIT);

    restore_normal_screen();
    close(server_fd);

    return 0;
}
