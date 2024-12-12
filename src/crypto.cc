#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include "crypto.h"

#define KEY_LEN 2048

RSA *generate_rsa_key() {
    RSA *rsa = RSA_generate_key(KEY_LEN, RSA_F4, NULL, NULL);
    if (!rsa) {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return rsa;
}

void read_rsa_publickey(const RSA *rsa, char **public_key) {
    BIO *bio = BIO_new(BIO_s_mem());
    if(!PEM_write_bio_RSAPublicKey(bio, rsa)){
        ERR_print_errors_fp(stderr);
        abort();
    }
    size_t pubkey_len = BIO_pending(bio);
    *public_key = (char *) malloc(pubkey_len + 1);

    BIO_read(bio, *public_key, pubkey_len);

    BIO_free_all(bio);
}

void read_rsa_privatekey(const RSA *rsa, char **private_key) {
    BIO *bio = BIO_new(BIO_s_mem());
    if(!PEM_write_bio_RSAPrivateKey(bio, rsa, NULL, NULL, 0, NULL, NULL)){
        ERR_print_errors_fp(stderr);
        abort();
    }
    size_t prikey_len = BIO_pending(bio);
    *private_key = (char *) malloc(prikey_len + 1);

    BIO_read(bio, *private_key, prikey_len);

    BIO_free_all(bio);
}

int rsa_public_encrypt(const char *plaintext, unsigned char *buf, const char *public_key) {
    RSA *rsa = RSA_new();
    BIO *bio = BIO_new(BIO_s_mem());
    BIO_write(bio, public_key, strlen(public_key));
    if(!PEM_read_bio_RSAPublicKey(bio, &rsa, NULL, NULL)) {
        ERR_print_errors_fp(stderr);
        abort();
    }
    int encrypted_len = RSA_public_encrypt(strlen(plaintext), (unsigned char *) plaintext, buf, rsa, RSA_PKCS1_PADDING);
    RSA_free(rsa);
    BIO_free_all(bio);
    return encrypted_len;
}

int rsa_private_encrypt(const char *plaintext, unsigned char *buf, const char *private_key) {
    RSA *rsa = RSA_new();
    BIO *bio = BIO_new(BIO_s_mem());
    BIO_write(bio, private_key, strlen(private_key));
    if (!PEM_read_bio_RSAPrivateKey(bio, &rsa, NULL, NULL)) {
        ERR_print_errors_fp(stderr);
        abort();
    }
    int encrypted_len = RSA_private_encrypt(strlen(plaintext), (unsigned char *) plaintext, buf, rsa, RSA_PKCS1_PADDING);
    RSA_free(rsa);
    BIO_free_all(bio);
    return encrypted_len;
}

int rsa_public_decrypt(const unsigned char *ciphertext, char *buf, const char *public_key, int encrypted_len) {
    RSA *rsa = RSA_new();
    BIO *bio = BIO_new(BIO_s_mem());
    BIO_write(bio, public_key, strlen(public_key));
    if (!PEM_read_bio_RSAPublicKey(bio, &rsa, NULL, NULL)) {
        ERR_print_errors_fp(stderr);
        abort();
    }
    int decrypted_len = RSA_public_decrypt(encrypted_len, ciphertext, (unsigned char *) buf, rsa, RSA_PKCS1_PADDING);
    RSA_free(rsa);
    BIO_free_all(bio);
    return decrypted_len;
}

int rsa_private_decrypt(const unsigned char *ciphertext, char *buf, const char *private_key, int encrypted_len) {
    RSA *rsa = RSA_new();
    BIO *bio = BIO_new(BIO_s_mem());
    BIO_write(bio, private_key, strlen(private_key));
    if(!PEM_read_bio_RSAPrivateKey(bio, &rsa, NULL, NULL)) {
        ERR_print_errors_fp(stderr);
        abort();
    }
    int decrypted_len = RSA_private_decrypt(encrypted_len, ciphertext, (unsigned char *)buf, rsa, RSA_PKCS1_PADDING);
    RSA_free(rsa);
    BIO_free_all(bio);
    return decrypted_len;
}

int send_secure(int fd, const char *data, const char *key, int encrypt_mode) {
    unsigned char ciphertext[256] = { 0 };
    int ciphertext_len;
    switch (encrypt_mode) {
        case PUBLIC_ENCRYPT:
            if ((ciphertext_len = rsa_public_encrypt(data, ciphertext, key)) == -1) {
                ERR_print_errors_fp(stderr);
                return INTERNAL_FAILURE;
            }
            break;
        case PRIVATE_ENCRYPT:
            if ((ciphertext_len = rsa_private_encrypt(data, ciphertext, key)) == -1) {
                ERR_print_errors_fp(stderr);
                return INTERNAL_FAILURE;
            }
            break;
        default:
            fprintf(stderr, "[Error] Invalid encrypt mode.\n");
            return INTERNAL_FAILURE;
    }
    if (send(fd, ciphertext, ciphertext_len, 0) == -1) {
        fprintf(stderr, "[Error] Failed to send the encrypted message.\n");
        return INTERNAL_FAILURE;
    }

    return 0;
}

int recv_secure(int fd, char *buf, int buf_size, const char *key, int decrypt_mode) {
    unsigned char ciphertext_buf[256] = { 0 };
    if (recv(fd, ciphertext_buf, sizeof(ciphertext_buf), 0) == -1) {
        fprintf(stderr, "[Error] Failed to receive the encrypted message.\n");
    }
    int decrypted_len;
    switch (decrypt_mode) {
        case PUBLIC_DECRYPT:
            if ((decrypted_len = rsa_public_decrypt(ciphertext_buf, buf, key, 256)) == -1) {
                ERR_print_errors_fp(stderr);
                return INTERNAL_FAILURE;
            }
            break;
        case PRIVATE_DECRYPT:
            if ((decrypted_len = rsa_private_decrypt(ciphertext_buf, buf, key, 256)) == -1) {
                ERR_print_errors_fp(stderr);
                return INTERNAL_FAILURE;
            }
            break;
        default:
            fprintf(stderr, "[Error] Invalid decrypt mode.\n");
            return INTERNAL_FAILURE;
    }
    
    buf[decrypted_len] = 0;

    return strlen(buf);
}