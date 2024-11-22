#ifndef __SERVER_H
#define __SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

#define BUF_MAXLEN 1024
#define USERNAME_MAXLEN 50

#define INTERNAL_FAILURE -1

struct Peer {
    char name[USERNAME_MAXLEN] = { 0 };
    char ip[20] = { 0 };
    unsigned short port = 0;
};

#ifdef __cplusplus
}
#endif

#endif
