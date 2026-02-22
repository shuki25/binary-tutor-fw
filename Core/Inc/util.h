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

void float_to_string(char *str,float f);

uint32_t time_diff(uint32_t start, uint32_t end);
uint32_t time_diff_rollover(uint32_t start, uint32_t end, uint32_t rollover);
void time_to_string(char *str, uint32_t time, uint16_t prescaler);
uint32_t calculate_crc32(uint8_t *data, uint32_t length);
void binary_to_string(char *str, uint8_t value);

#endif /* INC_UTIL_H_ */
