/*
 * base64.h
 *
 *  Created on: Nov 28, 2024
 *      Author: josh
 */

#ifndef INC_BASE64_H_
#define INC_BASE64_H_

#include <stdint.h>
#include <stddef.h>
#include <string.h>

uint16_t base64_encoded_size(uint16_t data_len);
uint16_t base64_encode(uint8_t *dest, uint16_t dest_size, uint8_t *src, uint16_t src_len);

#endif /* INC_BASE64_H_ */
