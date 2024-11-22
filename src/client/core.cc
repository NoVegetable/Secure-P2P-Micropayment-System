#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <vector>
#include "core.h"

using std::vector;
using std::string;

extern vector<Peer> peer_list;
extern char login_username[USERNAME_MAXLEN];
extern unsigned int account_balance;
extern string server_public_key;
extern unsigned int num_online;
extern int server_fd;

int service_handles(int opt, int socket_fd, char *buf)
{
    switch (opt) {
        case SERVICE_EXIT:
        {
            return __exit_handle(socket_fd, buf);
        }
        case SERVICE_REGISTER:
        {
            return __register_handle(socket_fd, buf);
            break;
        }
        case SERVICE_LOGIN:
        {
            return __login_handle(socket_fd, buf);
            break;
        }
        case SERVICE_LIST:
        {
            return __list_handle(socket_fd, buf);
            break;
        }
        case SERVICE_TRANSACTION:
        {
            return __transaction_handle(socket_fd, buf);
            break;
        }
        default:
        {
            fprintf(stderr, "Command not found.\n");
            return INTERNAL_FAILURE;
        }
    }
}

inline int __get_server_response(int socket_fd, char *send_msg, char *recv_buf)
{
    if (send(socket_fd, send_msg, strlen(send_msg), 0) == -1) {
        fprintf(stderr, "[Error] Failed to send message to server.\n");
        return INTERNAL_FAILURE;
    }
    if (recv(socket_fd, recv_buf, BUF_MAXLEN, 0) == -1) {
        fprintf(stderr, "[Error] Failed to receive message from server.\n");
        return INTERNAL_FAILURE;
    }

    return 0;
}

int __exit_handle(int socket_fd, char *buf)
{
    char send_msg[BUF_MAXLEN] = { 0 };

    sprintf(send_msg, "Exit");

    int retcode = __get_server_response(socket_fd, send_msg, buf);

    return retcode;
}

int __register_handle(int socket_fd, char *buf)
{
    char username[USERNAME_MAXLEN + 1] = { 0 };
    char send_msg[BUF_MAXLEN] = { 0 };
    
    printf("Enter username: ");
    fflush(stdout);
    fgets(username, sizeof(username), stdin); // fgets() reads at most USER_MAXLEN - 1 characters
    username[strlen(username) - 1] = 0;

    sprintf(send_msg, "REGISTER#%s", username);

    int retcode = __get_server_response(socket_fd, send_msg, buf);

    return retcode;
}

vector<string> __split(const char *s, const char *delim) {
    vector<string> tokens;
    char *s_copy = new char[strlen(s) + 1];
    strcpy(s_copy, s);

    char *p = strtok(s_copy, delim);
    while (p) {
        tokens.push_back(p);
        p = strtok(0, delim);
    }

    delete[] s_copy;

    return tokens;
}

int __listening(int socket_fd)
{
    struct sockaddr_in client_addr;
    int addrlen = sizeof(client_addr);
    int client_fd;
    if((client_fd = accept(socket_fd, (struct sockaddr *) &client_addr, (socklen_t *) &addrlen)) == -1) {
        struct sockaddr_in local_addr;
        int addrlen = sizeof(local_addr);
        getsockname(socket_fd, (struct sockaddr *) &local_addr, (socklen_t *) &addrlen);
        printf("[Warning] Your server on port %d just crashed.\n", ntohs(local_addr.sin_port));
        return INTERNAL_FAILURE;
    }

    char recv_buf[BUF_MAXLEN] = { 0 };
    if (recv(client_fd, recv_buf, BUF_MAXLEN, 0) == -1) {
        struct sockaddr_in local_addr;
        int addrlen = sizeof(local_addr);
        getsockname(socket_fd, (struct sockaddr *) &local_addr, (socklen_t *) &addrlen);
        printf("[Warning] Your server on port %d just crashed.\n", ntohs(local_addr.sin_port));
        return INTERNAL_FAILURE;
    }

    vector<string> messages = __split(recv_buf, "#");
    printf("\n[Notification] You have received a payment of $%d from %s.\n", stoi(messages[1]), messages[0].c_str());

    char send_msg[BUF_MAXLEN] = { 0 };
    sprintf(send_msg, "%s#%d#%s", messages[0].c_str(), stoi(messages[1]), login_username);
    if (send(server_fd, send_msg, strlen(send_msg), 0) == -1) {
        struct sockaddr_in local_addr;
        int addrlen = sizeof(local_addr);
        getsockname(socket_fd, (struct sockaddr *) &local_addr, (socklen_t *) &addrlen);
        printf("[Warning] Your server on port %d just crashed.\n", ntohs(local_addr.sin_port));
        return INTERNAL_FAILURE;
    }

    printf("\n%s "
           "\033[1m>\033[0m ",
           login_username);
    fflush(stdout);

    return 0;
}

void *__keep_listening(void *socket_fd)
{
    int local_fd = *(int *) socket_fd;
    while (1) {
        int retcode = __listening(local_fd);
        if (retcode == INTERNAL_FAILURE) {
            break;
        }
    }
    pthread_exit(0);
}

void parse_list_msg(const char *list_msg)
{
    vector<string> lines = __split(list_msg, "\n");
    
    account_balance = (unsigned int) stoi(lines[0]);
    server_public_key = std::move(lines[1]);
    num_online = (unsigned int) stoi(lines[2]);
    
    peer_list.clear();
    for (int i = 3; i < 3 + num_online; ++i) {
        vector<string> user = __split(lines[i].c_str(), "#\n");
        Peer peer;
        strcpy(peer.name, user[0].c_str());
        strcpy(peer.ip, user[1].c_str());
        peer.port = (unsigned int) stoi(user[2]);
        peer_list.push_back(peer);
    }
}

int __login_handle(int socket_fd, char *buf)
{
    char username[USERNAME_MAXLEN + 1] = { 0 };
    char send_msg[BUF_MAXLEN] = { 0 };
    
    printf("Enter username: ");
    fflush(stdout);
    fgets(username, sizeof(username), stdin);
    username[strlen(username) - 1] = 0;

    unsigned short local_port;
    printf("Enter a port number (1024~65535) to listen at: ");
    scanf("%hu", &local_port);
    getchar();

    int local_fd;
    if ((local_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "[Error] Failed to create socket.\n");
        return INTERNAL_FAILURE;
    }
    struct sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(local_port);

    if (bind(local_fd, (const struct sockaddr *) &local_addr, sizeof(local_addr)) == -1) {
        close(local_fd);
        fprintf(stderr, "[Error] Failed to bind on the required port.\n");
        return INTERNAL_FAILURE;
    }
    if (listen(local_fd, 8) == -1) {
        close(local_fd);
        fprintf(stderr, "[Error] Failed to listen on the required port.\n");
        return INTERNAL_FAILURE;
    }

    sprintf(send_msg, "%s#%d", username, local_port);

    int retcode = __get_server_response(socket_fd, send_msg, buf);

    if (retcode != INTERNAL_FAILURE && strncmp(buf, "220 AUTH_FAIL", 13) != 0) {
        strcpy(login_username, username);

        pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(&tid, &attr, __keep_listening, &local_fd);
        pthread_detach(tid); // detach from the main thread to avoid memory leaks when main thread terminates

        printf("Keep listening on port %d\n", local_port);
        
        parse_list_msg(buf); // update peer_list
    }
    else {
        close(local_fd);
    }

    return retcode;
}

int __list_handle(int socket_fd, char *buf)
{
    char send_msg[BUF_MAXLEN] = { 0 };
    
    sprintf(send_msg, "List");

    int retcode = __get_server_response(socket_fd, send_msg, buf);

    if (retcode != INTERNAL_FAILURE)
        parse_list_msg(buf); // update peer_list

    return retcode;
}

const Peer* __get_peer_by_name(char *peer_name)
{
    for (int i = 0; i < peer_list.size(); ++i) {
        if (strcmp(peer_name, peer_list[i].name) == 0) {
            return &peer_list[i];
        }
    }

    return 0;
}

int __transaction_handle(int socket_fd, char *buf)
{
    char payeename[USERNAME_MAXLEN + 1] = { 0 };
    int payment;
    char send_msg[BUF_MAXLEN] = { 0 };

    printf("Enter payeename: ");
    fflush(stdout);
    fgets(payeename, sizeof(payeename), stdin);
    payeename[strlen(payeename) - 1] = 0;
    printf("Enter pay amount: ");
    fflush(stdout);
    scanf("%d", &payment);
    getchar();

    sprintf(send_msg, "%s#%d#%s", login_username, payment, payeename);

    char list_msg[BUF_MAXLEN];
    __list_handle(server_fd, list_msg);
    parse_list_msg(list_msg);
    const Peer *p = __get_peer_by_name(payeename);
    if (p) {
        int peer_fd;
        if ((peer_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            fprintf(stderr, "[Error] Failed to create socket.\n");
            return INTERNAL_FAILURE;
        }
        struct sockaddr_in peer_addr;
        peer_addr.sin_family = AF_INET;
        peer_addr.sin_addr.s_addr = inet_addr(p->ip);
        peer_addr.sin_port = htons(p->port);
        
        if (connect(peer_fd, (const struct sockaddr *) &peer_addr, sizeof(peer_addr)) == -1) {
            fprintf(stderr, "[Error] Failed to connect to payee.\n");
            return INTERNAL_FAILURE;
        }

        if (send(peer_fd, send_msg, strlen(send_msg), 0) == -1) {
            fprintf(stderr, "[Error] Failed to send message to payee.\n");
            return INTERNAL_FAILURE;
        }

        close(peer_fd);
 
        if (recv(server_fd, buf, BUF_MAXLEN, 0) == -1) {
            fprintf(stderr, "[Error] Failed to receive messages from the server.\n");
            return INTERNAL_FAILURE;
        }
    }
    else {
        printf("Payee not found. Try updating the online information list.\n");
    }

    return 0;
}
