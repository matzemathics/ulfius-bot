#pragma once

#include "../endpoint_handler.h"
#include <sodium.h>

#include "stdbool.h"

typedef unsigned char signature[crypto_sign_ed25519_BYTES];
typedef unsigned char public_key[crypto_sign_ed25519_PUBLICKEYBYTES];

typedef struct _verifier_endpoint_handler
{
    i_endpoint_handler parent;
    public_key public_key;
} verifier_endpoint_handler;

static inline uint8_t read_hex_digit(const char c)
{
    return (c & 0xF) + (c >> 6) | ((c >> 3) & 0x8);
}

bool bytes_from_hex_string(const char *str, uint8_t target[], size_t target_size);

static inline bool read_signature(const char *str, signature target)
{
    return bytes_from_hex_string(str, target, sizeof(signature));
}

static inline bool read_public_key(const char *str, public_key target)
{
    return bytes_from_hex_string(str, target, sizeof(public_key));
}

void get_payload_owned(char **payload, size_t *size, const struct _u_request *request);

