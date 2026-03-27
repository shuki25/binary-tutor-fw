#ifndef TINY_BLAKE2S_H
#define TINY_BLAKE2S_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TINY_BLAKE2S_OUTBYTES 32
#define TINY_BLAKE2S_BLOCKBYTES 64

typedef struct {
    uint32_t h[8];
    uint32_t t[2];
    uint32_t f[2];
    uint8_t buf[TINY_BLAKE2S_BLOCKBYTES];
    size_t buflen;
    size_t outlen;
} tiny_blake2s_ctx_t;

void tiny_blake2s_init(tiny_blake2s_ctx_t *ctx, size_t outlen);
void tiny_blake2s_update(tiny_blake2s_ctx_t *ctx, const void *in, size_t inlen);
void tiny_blake2s_final(tiny_blake2s_ctx_t *ctx, uint8_t *out, size_t outlen);
void tiny_blake2s(const void *in, size_t inlen, uint8_t out[TINY_BLAKE2S_OUTBYTES]);

#ifdef __cplusplus
}
#endif // extern "C"

#endif // TINY_BLAKE2S_H
