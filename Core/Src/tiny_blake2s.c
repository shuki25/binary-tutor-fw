#include "tiny_blake2s.h"

#include <string.h>

#define ROTR32(x, n) (((x) >> (n)) | ((x) << (32U - (n))))

static uint32_t load32_le(const uint8_t src[4]) {
    return ((uint32_t) src[0]) | ((uint32_t) src[1] << 8) | ((uint32_t) src[2] << 16)
            | ((uint32_t) src[3] << 24);
}

static void store32_le(uint8_t dst[4], uint32_t w) {
    dst[0] = (uint8_t) (w >> 0);
    dst[1] = (uint8_t) (w >> 8);
    dst[2] = (uint8_t) (w >> 16);
    dst[3] = (uint8_t) (w >> 24);
}

static const uint32_t blake2s_iv[8] = { 0x6A09E667U, 0xBB67AE85U, 0x3C6EF372U, 0xA54FF53AU, 0x510E527FU,
        0x9B05688CU, 0x1F83D9ABU, 0x5BE0CD19U };

static const uint8_t blake2s_sigma[10][16] = { { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }, { 14,
        10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 }, { 11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1,
        9, 4 }, { 7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8 }, { 9, 0, 5, 7, 2, 4, 10, 15, 14, 1,
        11, 12, 6, 8, 3, 13 }, { 2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9 }, { 12, 5, 1, 15, 14,
        13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11 }, { 13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10 }, { 6,
        15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5 }, { 10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12,
        13, 0 } };

static void tiny_blake2s_increment_counter(tiny_blake2s_ctx_t *ctx, uint32_t inc) {
    ctx->t[0] += inc;
    if (ctx->t[0] < inc) {
        ctx->t[1]++;
    }
}

static void tiny_blake2s_set_lastblock(tiny_blake2s_ctx_t *ctx) {
    ctx->f[0] = 0xFFFFFFFFU;
}

#define G(r,i,a,b,c,d)                                                   \
    do {                                                                 \
        a = a + b + m[blake2s_sigma[r][2U*(i) + 0U]];                    \
        d = ROTR32(d ^ a, 16);                                           \
        c = c + d;                                                       \
        b = ROTR32(b ^ c, 12);                                           \
        a = a + b + m[blake2s_sigma[r][2U*(i) + 1U]];                    \
        d = ROTR32(d ^ a, 8);                                            \
        c = c + d;                                                       \
        b = ROTR32(b ^ c, 7);                                            \
    } while (0)

static void tiny_blake2s_compress(tiny_blake2s_ctx_t *ctx, const uint8_t block[64]) {
    uint32_t m[16];
    uint32_t v[16];
    unsigned int i;
    unsigned int r;

    for (i = 0; i < 16; i++) {
        m[i] = load32_le(block + (i * 4U));
    }

    for (i = 0; i < 8; i++) {
        v[i] = ctx->h[i];
    }

    v[8] = blake2s_iv[0];
    v[9] = blake2s_iv[1];
    v[10] = blake2s_iv[2];
    v[11] = blake2s_iv[3];
    v[12] = blake2s_iv[4] ^ ctx->t[0];
    v[13] = blake2s_iv[5] ^ ctx->t[1];
    v[14] = blake2s_iv[6] ^ ctx->f[0];
    v[15] = blake2s_iv[7] ^ ctx->f[1];

    for (r = 0; r < 10; r++) {
        G(r, 0, v[0], v[4], v[8], v[12]);
        G(r, 1, v[1], v[5], v[9], v[13]);
        G(r, 2, v[2], v[6], v[10], v[14]);
        G(r, 3, v[3], v[7], v[11], v[15]);
        G(r, 4, v[0], v[5], v[10], v[15]);
        G(r, 5, v[1], v[6], v[11], v[12]);
        G(r, 6, v[2], v[7], v[8], v[13]);
        G(r, 7, v[3], v[4], v[9], v[14]);
    }

    for (i = 0; i < 8; i++) {
        ctx->h[i] ^= v[i] ^ v[i + 8U];
    }
}

void tiny_blake2s_init(tiny_blake2s_ctx_t *ctx, size_t outlen) {
    size_t i;

    if ((ctx == NULL) || (outlen == 0U) || (outlen > TINY_BLAKE2S_OUTBYTES)) {
        return;
    }

    for (i = 0; i < 8; i++) {
        ctx->h[i] = blake2s_iv[i];
    }

    ctx->h[0] ^= 0x01010000U ^ (uint32_t) outlen; /* fanout=1, depth=1, keylen=0, outlen */

    ctx->t[0] = 0U;
    ctx->t[1] = 0U;
    ctx->f[0] = 0U;
    ctx->f[1] = 0U;
    ctx->buflen = 0U;
    ctx->outlen = outlen;

    memset(ctx->buf, 0, sizeof(ctx->buf));
}

void tiny_blake2s_update(tiny_blake2s_ctx_t *ctx, const void *in, size_t inlen) {
    const uint8_t *pin = (const uint8_t*) in;

    if ((ctx == NULL) || (in == NULL) || (inlen == 0U)) {
        return;
    }

    while (inlen > 0U) {
        size_t left = ctx->buflen;
        size_t fill = TINY_BLAKE2S_BLOCKBYTES - left;

        if (inlen > fill) {
            memcpy(&ctx->buf[left], pin, fill);
            ctx->buflen += fill;
            pin += fill;
            inlen -= fill;

            tiny_blake2s_increment_counter(ctx, TINY_BLAKE2S_BLOCKBYTES);
            tiny_blake2s_compress(ctx, ctx->buf);
            ctx->buflen = 0U;
        } else {
            memcpy(&ctx->buf[left], pin, inlen);
            ctx->buflen += inlen;
            pin += inlen;
            inlen = 0U;
        }
    }
}

void tiny_blake2s_final(tiny_blake2s_ctx_t *ctx, uint8_t *out, size_t outlen) {
    uint8_t full_out[TINY_BLAKE2S_OUTBYTES];
    size_t i;

    if ((ctx == NULL) || (out == NULL) || (outlen == 0U) || (outlen > ctx->outlen)) {
        return;
    }

    tiny_blake2s_increment_counter(ctx, (uint32_t) ctx->buflen);
    tiny_blake2s_set_lastblock(ctx);

    memset(&ctx->buf[ctx->buflen], 0, TINY_BLAKE2S_BLOCKBYTES - ctx->buflen);
    tiny_blake2s_compress(ctx, ctx->buf);

    for (i = 0; i < 8; i++) {
        store32_le(&full_out[i * 4U], ctx->h[i]);
    }

    memcpy(out, full_out, outlen);
}

void tiny_blake2s(const void *in, size_t inlen, uint8_t out[TINY_BLAKE2S_OUTBYTES]) {
    tiny_blake2s_ctx_t ctx;

    tiny_blake2s_init(&ctx, TINY_BLAKE2S_OUTBYTES);
    tiny_blake2s_update(&ctx, in, inlen);
    tiny_blake2s_final(&ctx, out, TINY_BLAKE2S_OUTBYTES);
}
