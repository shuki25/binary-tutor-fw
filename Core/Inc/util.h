/*
 * util.h
 *
 *  Created on: Mar 26, 2024
 *      Author: jdbnts
 */

#ifndef INC_UTIL_H_
#define INC_UTIL_H_

#define CRC_LENGTH          (4)
#define CRC_BLOCK_SIZE      (128 - CRC_LENGTH)

#define float_to_string(str, f) float_to_string_precision(str, sizeof(str), f, 0)

uint32_t time_diff(uint32_t start, uint32_t end);
uint32_t time_diff_rollover(uint32_t start, uint32_t end, uint32_t rollover);
void time_to_string(char *str, uint32_t time, uint16_t prescaler);
uint32_t calculate_crc32(uint8_t *data, uint32_t length);
void binary_to_string(char *str, uint8_t value);
void float_to_string_precision(char *str, uint32_t len, float f, uint8_t precision);

#endif /* INC_UTIL_H_ */
