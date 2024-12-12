#ifndef __CRYPTO_H
#define __CRYPTO_H

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/err.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KEY_LEN 2048
#define PUBLIC_ENCRYPT 1
#define PRIVATE_ENCRYPT 2
#define PUBLIC_DECRYPT 3
#define PRIVATE_DECRYPT 4

#define INTERNAL_FAILURE -1

RSA *generate_rsa_key();
void read_rsa_publickey(const RSA *rsa, char **public_key);
void read_rsa_privatekey(const RSA *rsa, char **private_key);
int rsa_public_encrypt(const char *plaintext, unsigned char *buf, const char *public_key);
int rsa_private_encrypt(const char *plaintext, unsigned char *buf, const char *private_key);
int rsa_public_decrypt(const unsigned char *ciphertext, char *buf, const char *public_key, int encrypted_len);
int rsa_private_decrypt(const unsigned char *ciphertext, char *buf, const char *private_key, int encrypted_len);
int send_secure(int fd, const char *data, const char *key, int encrypt_mode);
int recv_secure(int fd, char *buf, int buf_size, const char *key, int decrypt_mode);

#ifdef __cplusplus
}
#endif

#endif