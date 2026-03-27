/*
 * base64.c
 *
 *  Created on: Nov 28, 2024
 *      Author: josh
 */

#include "base64.h"

const uint8_t base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

uint16_t base64_encoded_size(uint16_t input_length) {
    uint16_t encoded_size;

    encoded_size = input_length;
    if (input_length % 3 != 0) {
        encoded_size += 3 - (input_length % 3);
    }
    encoded_size = (encoded_size / 3) * 4;

    return encoded_size;
}

/*********************************************************************
 * @fn      base64_encode
 *
 * @brief   Decode a base64 encoded string
 *
 * @param   encoded_data - pointer to encoded data
 * @param   data - pointer to source data
 * @param   input_length - length of source data
 *
 * @return  length of encoded data
 *********************************************************************/

const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

uint16_t base64_encode(uint8_t *dest, uint16_t dest_size, uint8_t *src, uint16_t src_len) {

    if (!dest || !src || dest_size == 0) {
        return 0;
    }

    uint16_t encoded_len = base64_encoded_size(src_len);

    memset(dest, 0, dest_size); // Blank out the destination buffer

    if (encoded_len + 1 > dest_size) { // If the destination buffer is not large enough
                                       // return with an error
        return 0;
    }

    size_t i;
    size_t j;
    size_t v;

    for (i = 0, j = 0; i < src_len; i += 3, j += 4) {
        v = src[i];
        v = i + 1 < src_len ? v << 8 | src[i + 1] : v << 8;
        v = i + 2 < src_len ? v << 8 | src[i + 2] : v << 8;

        dest[j] = base64_table[(v >> 18) & 0x3F];
        dest[j + 1] = base64_table[(v >> 12) & 0x3F];
        if (i + 1 < src_len) {
            dest[j + 2] = base64_table[(v >> 6) & 0x3F];
        } else {
            dest[j + 2] = '=';
        }
        if (i + 2 < src_len) {
            dest[j + 3] = base64_table[v & 0x3F];
        } else {
            dest[j + 3] = '=';
        }
    }
    return encoded_len;
}
