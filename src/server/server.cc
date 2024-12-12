#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <regex.h>
#include <string>
#include <vector>
#include <unordered_map>
#include "server.h"
#include "crypto.h"

using std::vector;
using std::string;
using std::unordered_map;

#define MESSAGE_EXIT 0
#define MESSAGE_REGISTER 1
#define MESSAGE_LOGIN 2
#define MESSAGE_LIST 3
#define MESSAGE_TRANSACTION 4

extern vector<Peer> peer_list;
extern pthread_mutex_t peer_list_mutex;
extern string public_key;
extern string private_key;
extern unordered_map<string, int> client_fds;

const char *exit_pattern = "^Exit$";
const char *register_pattern = "^REGISTER#([A-Za-z0-9 _-]+)$";
const char *login_pattern = "^([A-Za-z0-9 _-]+)#([0-9]+)$";
const char *list_pattern = "^List$";
const char *transaction_pattern = "^([A-Za-z0-9 _-]+)#([0-9]+)#([A-Za-z0-9 _-]+)$";
regex_t exit_reg;
regex_t register_reg;
regex_t login_reg;
regex_t list_reg;
regex_t transaction_reg;

RSA *keypair;

struct ClientMessage {
    int type;
    vector<string> params;
};

Peer *__get_peer(const char *name)
{
    Peer *target = NULL;
    for (int i = 0; i < peer_list.size(); ++i) {
        if (strcmp(peer_list[i].name, name) == 0) {
            target = &peer_list[i];
            break;
        }
    }
    return target;
}

int update_peer_list(const char *name, const char *ip, unsigned short port, int balance_diff, int mode)
{
    pthread_mutex_lock(&peer_list_mutex);
    switch (mode) {
        case PEER_REGISTER: {
            Peer new_peer;
            strcpy(new_peer.name, name);
            new_peer.balance = BALANCE_INITIALIZER;
            peer_list.push_back(new_peer);
            break;
        }
        case PEER_LOGIN: {
            Peer *peer = __get_peer(name);
            strcpy(peer->ip, ip);
            peer->port = port;
            peer->online = true;
            break;
        }
        case PEER_TRANSACT: {
            Peer *peer = __get_peer(name);          
            peer->balance += balance_diff;
            break;
        }
        case PEER_EXIT: {
            Peer *peer = __get_peer(name);
            peer->online = false;
            break;
        }
        default: {
            return INTERNAL_FAILURE;
        }
    }
    pthread_mutex_unlock(&peer_list_mutex);

    return 0;
}

inline int get_online_num()
{
    int count = 0;
    for (int i = 0; i < peer_list.size(); ++i) {
        count += (int) peer_list[i].online;
    }
    return count;
}

int __parse_message(ClientMessage *message, const char *msg)
{
    const size_t nmatch = 4;
    regmatch_t pmatch[nmatch];
    
    if (regexec(&exit_reg, msg, nmatch, pmatch, 0) == 0) {
        message->type = MESSAGE_EXIT;
    }
    else if (regexec(&register_reg, msg, nmatch, pmatch, 0) == 0) {
        message->type = MESSAGE_REGISTER;
        const int num_params = 1;
        regoff_t len;
        char param[BUF_MAXLEN + 1] = { 0 };
        for (int i = 0; i < num_params; ++i) {
            len = pmatch[1 + i].rm_eo - pmatch[1 + i].rm_so;
            sprintf(param, "%.*s", len, msg + pmatch[1 + i].rm_so);
            message->params.push_back(param);
        }
    }
    else if (regexec(&login_reg, msg, nmatch, pmatch, 0) == 0) {
        message->type = MESSAGE_LOGIN;
        const int num_params = 2;
        regoff_t len;
        char param[BUF_MAXLEN + 1];
        for (int i = 0; i < num_params; ++i) {
            len = pmatch[1 + i].rm_eo - pmatch[1 + i].rm_so;
            sprintf(param, "%.*s", len, msg + pmatch[1 + i].rm_so);
            message->params.push_back(param);
        }
    }
    else if (regexec(&list_reg, msg, nmatch, pmatch, 0) == 0) {
        message->type = MESSAGE_LIST;
    }
    else if (regexec(&transaction_reg, msg, nmatch, pmatch, 0) == 0) {
        message->type = MESSAGE_TRANSACTION;
        const int num_params = 3;
        regoff_t len;
        char param[BUF_MAXLEN + 1] = { 0 };
        for (int i = 0; i < num_params; ++i) {
            len = pmatch[1 + i].rm_eo - pmatch[1 + i].rm_so;
            sprintf(param, "%.*s", len, msg + pmatch[1 + i].rm_so);
            message->params.push_back(param);
        }
    }
    else {
        fprintf(stderr, "[Error] Failed to interpret the received message.\n");
        return INTERNAL_FAILURE;
    }

    return 0;
}

void __wrap_list_message(char *buf, const char *username)
{
    Peer *peer = __get_peer(username);
    int online_num = get_online_num();
    // sprintf(buf, "%u\n%s\n%d\n", peer->balance, public_key.c_str(), online_num);
    sprintf(buf, "%u\n%d\n", peer->balance, online_num);

    for (int i = 0; i < peer_list.size(); ++i) {
        if (peer_list[i].online)
            sprintf(buf + strlen(buf), "%s#%s#%hu\n", peer_list[i].name, peer_list[i].ip, peer_list[i].port);
    }
}

int __message_handle(int client_fd, ClientMessage *message, char *username)
{
    bool login = (strlen(username) > 0);
    char send_buf[BUF_MAXLEN + 1] = { 0 };
    switch (message->type) {
        case MESSAGE_EXIT: {
            if (login) {
                update_peer_list(username, 0, 0, 0, PEER_EXIT);
            }
            sprintf(send_buf, "Bye\n");
            if (send_secure(client_fd, send_buf, private_key.c_str(), PRIVATE_ENCRYPT) == -1) {
                fprintf(stderr, "[Error] Failed to send message to the client.\n");
                return INTERNAL_FAILURE;
            }

            pthread_exit(0);

            break;
        }
        case MESSAGE_REGISTER: {
            const char *register_name = message->params[0].c_str();
            bool auth_ok = (__get_peer(register_name) == NULL);

            if (auth_ok) {
                update_peer_list(register_name, 0, 0, 0, PEER_REGISTER);
                sprintf(send_buf, "100 OK\n");
            }
            else {
                sprintf(send_buf, "210 FAIL\n");
            }

            if (send_secure(client_fd, send_buf, private_key.c_str(), PRIVATE_ENCRYPT) == -1) {
                fprintf(stderr, "[Error] Failed to send message to the client.\n");
                return INTERNAL_FAILURE;
            }

            break;
        }
        case MESSAGE_LOGIN: {
            const char *login_name = message->params[0].c_str();
            bool has_registered = (__get_peer(login_name) != NULL);
            
            if (has_registered) {
                struct sockaddr_in client_addr;
                int client_addrlen = sizeof(client_addr);
                getpeername(client_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addrlen);
                char ip[20] = { 0 };
                strcpy(ip, inet_ntoa(client_addr.sin_addr));
                unsigned short port = (unsigned short) atoi(message->params[1].c_str());
                update_peer_list(login_name, ip, port, 0, PEER_LOGIN);
                __wrap_list_message(send_buf, login_name);
                strcpy(username, login_name);
                client_fds[username] = client_fd;
            }
            else {
                sprintf(send_buf, "220 AUTH_FAIL\n");
            }

            if (send_secure(client_fd, send_buf, private_key.c_str(), PRIVATE_ENCRYPT) == -1) {
                fprintf(stderr, "[Error] Failed to send message to the client.\n");
                return INTERNAL_FAILURE;
            }
            
            break;
        }
        case MESSAGE_LIST: {
            if (login) {
                __wrap_list_message(send_buf, username);
            }
            else {
                sprintf(send_buf, "Please login first.\n");
            }

            if (send_secure(client_fd, send_buf, private_key.c_str(), PRIVATE_ENCRYPT) == -1) {
                fprintf(stderr, "[Error] Failed to send message to the client.\n");
                return INTERNAL_FAILURE;
            }

            break;
        }
        case MESSAGE_TRANSACTION: {
            if (login) {
                const char *payername = message->params[0].c_str();
                int payment = atoi(message->params[1].c_str());
                const char *payeename = message->params[2].c_str();
                Peer *payer = __get_peer(payername);
                Peer *payee = __get_peer(payeename);
                bool payer_has_registered = (payer != NULL);
                bool payee_has_registered = (payee != NULL);
                bool message_from_payee = (strcmp(payeename, username) == 0);
                bool payer_enough_balance = (payer->balance >= payment); 

                if (payer_has_registered && payee_has_registered && message_from_payee && payer_enough_balance) {
                    update_peer_list(payername, 0, 0, -payment, PEER_TRANSACT);
                    update_peer_list(payeename, 0, 0, payment, PEER_TRANSACT);

                    sprintf(send_buf, "Transfer OK!\n");
                }
                else {
                    sprintf(send_buf, "Transfer FAIL!\n");
                }

                int payer_fd = client_fds[payername];
                if (send_secure(payer_fd, send_buf, private_key.c_str(), PRIVATE_ENCRYPT) == -1) {
                    fprintf(stderr, "[Error] Failed to send message to the payer.\n");
                    return INTERNAL_FAILURE;
                }
            }

            break;
        }
        default: {
            return INTERNAL_FAILURE;
        }
    }

    return 0;
}

void *handle_client(void *socket_fd)
{
    int client_fd = *(int *) socket_fd;
    char username[USERNAME_MAXLEN + 1] = { 0 };
    char recv_buf[BUF_MAXLEN + 1] = { 0 };

    struct sockaddr_in client_addr;
    int client_addrlen = sizeof(client_addr);
    getpeername(client_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addrlen);
    char client_ip[20];
    strcpy(client_ip, inet_ntoa(client_addr.sin_addr));
    unsigned short client_port = ntohs(client_addr.sin_port);

    regcomp(&exit_reg, exit_pattern, REG_EXTENDED | REG_NEWLINE);
    regcomp(&register_reg, register_pattern, REG_EXTENDED | REG_NEWLINE);
    regcomp(&login_reg, login_pattern, REG_EXTENDED | REG_NEWLINE);
    regcomp(&list_reg, list_pattern, REG_EXTENDED | REG_NEWLINE);
    regcomp(&transaction_reg, transaction_pattern, REG_EXTENDED | REG_NEWLINE);

    while (true) {
        int recv_bytes = 0;
        if ((recv_bytes = recv_secure(client_fd, recv_buf, BUF_MAXLEN, private_key.c_str(), PRIVATE_DECRYPT)) == -1) {
            fprintf(stderr, "[Error] Failed to receive message from client.\n");
            close(client_fd);
            pthread_exit(0);
        }
        recv_buf[recv_bytes] = 0;

        if (strlen(username) > 0) {
            printf("[Log] Message from %s:%hu (%s): %s\n", client_ip, client_port, username, recv_buf);
        }
        else {
            printf("[Log] Message from %s:%hu: %s\n", client_ip, client_port, recv_buf);
        }

        ClientMessage message;
        if (__parse_message(&message, recv_buf) == INTERNAL_FAILURE) {
            close(client_fd);
            pthread_exit(0);
        }

        if (__message_handle(client_fd, &message, username) == INTERNAL_FAILURE) {
            close(client_fd);
            pthread_exit(0);
        }
    }
}