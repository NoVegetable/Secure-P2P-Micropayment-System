#ifndef __CLIENT_H
#define __CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#define SERVICE_EXIT 0
#define SERVICE_REGISTER 1 
#define SERVICE_LOGIN 2
#define SERVICE_LIST 3
#define SERVICE_TRANSACTION 4

#define BUF_MAXLEN 1024
#define USERNAME_MAXLEN 50

#define INTERNAL_FAILURE -1

struct Peer {
    char name[USERNAME_MAXLEN] = { 0 };
    char ip[20] = { 0 };
    unsigned short port = 0;
};

int service_handles(int opt, int socket_fd, char *buf);

#ifdef __cplusplus
}
#endif

#endif
