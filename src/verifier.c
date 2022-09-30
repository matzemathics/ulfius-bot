
#include <ulfius.h>
#include <sodium.h>

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "verifier.h"

const char * const CLIENT_PUBLIC_KEY_SETTING = "publicKey";
const char * const CLIENT_PUBLIC_KEY_ENV = "CLIENT_PUBLIC_KEY";

typedef unsigned char signature[crypto_sign_ed25519_BYTES];
typedef unsigned char public_key[crypto_sign_ed25519_PUBLICKEYBYTES];

typedef struct _verifier_endpoint_handler
{
    i_endpoint_handler parent;
    public_key public_key;
} verifier_endpoint_handler;

extern inline uint8_t read_hex_digit(const char c)
{
    return (c & 0xF) + (c >> 6) | ((c >> 3) & 0x8);
}

bool bytes_from_hex_string(const char *str, uint8_t target[], size_t target_size)
{
    size_t str_len = strlen(str);

    if (str_len % 2 || str_len / 2 > target_size) 
    {
        fprintf(stderr, "too long (%d vs %d)\n", str_len, target_size);
        return false;
    }

    while (*str)
    {
        char high = *str++;
        char low = *str++;

        if ( ! isxdigit(low) || ! isxdigit(high) )
        {
            fprintf(stderr, "not a hex byte: %c%c\n", high, low);
            return false;
        }

        *target++ = read_hex_digit(low) | (read_hex_digit(high) << 4);
    }

    return true;
}

extern inline bool read_signature(const char *str, signature target)
{
    return bytes_from_hex_string(str, target, sizeof(signature));
}

extern inline bool read_public_key(const char *str, public_key target)
{
    return bytes_from_hex_string(str, target, sizeof(public_key));
}

int verify_interaction_callback(
    const struct _u_request *request, 
    struct _u_response *response, 
    void *user_data)
{
    const char *hdr_signature = u_map_get(request->map_header, "X-Signature-Ed25519");
    const char *hdr_timestamp = u_map_get(request->map_header, "X-Signature-Timestamp");

    signature sig;
    if (!read_signature(hdr_signature, sig))
    {
        fprintf(stderr, "Cannot read signiture: %s\n", hdr_signature);
        return U_CALLBACK_UNAUTHORIZED;
    }

    size_t msg_len = strlen(hdr_timestamp) + request->binary_body_length;
    unsigned char *message = malloc(msg_len + 1);
    message[0] = 0;
    strcat(message, hdr_timestamp);
    strcat(message, request->binary_body);

    verifier_endpoint_handler *self = (verifier_endpoint_handler*)user_data;

    if (crypto_sign_verify_detached(sig, message, msg_len, self->public_key))
    {
        fprintf(stderr, "Invalid signature: %s\n", hdr_signature);
        free(message);
        return U_CALLBACK_UNAUTHORIZED;
    }

    free(message);
    return U_CALLBACK_CONTINUE;
}

void verifier_destruct(i_endpoint_handler *instance)
{
    free(instance);
}

i_endpoint_handler *verifier_new()
{
    verifier_endpoint_handler *self = malloc(sizeof(verifier_endpoint_handler));

    self->parent.callback = &verify_interaction_callback;
    self->parent.destruct = &verifier_destruct;

    const char *hex_str_pk = getenv(CLIENT_PUBLIC_KEY_ENV);

    if (! hex_str_pk)
    {
        fprintf(stderr, "Cannot read public key from environment variable\n");
        free(self);
        return NULL;
    }

    if (! read_public_key(hex_str_pk, self->public_key))
    {
        fprintf(stderr, "Public key is invalid: %s\n", hex_str_pk);
        free(self);
        return NULL;
    }

    return (i_endpoint_handler*)self;
}