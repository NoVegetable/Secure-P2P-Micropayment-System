#ifndef __SERVER_H
#define __SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

#define BUF_MAXLEN 1024
#define USERNAME_MAXLEN 50

#define PEER_REGISTER 0
#define PEER_LOGIN 1
#define PEER_TRANSACT 2
#define PEER_EXIT 3

#define BALANCE_INITIALIZER 10000

#define PUBLIC_KEY_MAXLEN 2048

#define INTERNAL_FAILURE -1

struct Peer {
    char name[USERNAME_MAXLEN + 1] = { 0 };
    char ip[20] = { 0 };
    unsigned short port = 0;
    unsigned int balance = 0;
    bool online = false;
};

void *handle_client(void *socket_fd);
int update_peer_list(const char *name, const char *ip, unsigned short port, int balance_diff, int mode);
int get_online_num();

#ifdef __cplusplus
}
#endif

#endif
