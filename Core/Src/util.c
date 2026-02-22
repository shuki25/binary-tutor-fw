/*
 * util.c
 *
 *  Created on: Mar 26, 2024
 *      Author: jdbnts
 */

#include "main.h"
#include <stdint.h>
#include <stdio.h>
#include "util.h"

extern CRC_HandleTypeDef hcrc;
/*----------------------------------------------------------------------------
 * Function      : float_to_string
 * ---------------------------------------------------------------------------
 * Description   : Converts a float to a string
 * Parameters    : pointer to the string, float to convert
 * Returns       : None
 * -------------------------------------------------------------------------*/
void float_to_string(char *str, float f) {
    int whole = (int) f;
    int decimal = (int) ((f - (float) whole) * 1000);
    if (f < 0) {
        decimal = -decimal;
    }
    sprintf(str, "%d.%02d", whole, decimal);

}

uint32_t time_diff(uint32_t start, uint32_t end) {
    if (end >= start) {
        return (end - start);
    } else {
        return (start - end);
    }
}

uint32_t time_diff_rollover(uint32_t start, uint32_t end, uint32_t rollover) {
    if (end >= start) {
        return (end - start);
    } else {
        return (rollover - start + end);
    }
}
/*----------------------------------------------------------------------------
 * Function      : time_to_string
 * ---------------------------------------------------------------------------
 * Description   : Converts a time in seconds to a string
 * Parameters    : pointer to the string
 *                 time in seconds
 *                 prescaler (10000 for a second)
 * Returns       : None
 * -------------------------------------------------------------------------*/
void time_to_string(char *str, uint32_t time, uint16_t prescaler) {
    if (time == 0) {
        sprintf(str, "Invalid time");
        return;
    }
    time = time / prescaler;
    uint16_t seconds = time % 60;
    uint16_t minutes = (time / 60) % 60;
    if (time < 3600) {
        sprintf(str, "%02d:%02d", minutes, seconds);
        return;
    }
    uint16_t hours = (time / 3600) % 24;
    if (time < 86400) {
        sprintf(str, "%02d:%02d:%02d", hours, minutes, seconds);
        return;
    }
    uint16_t days = time / 86400;
    sprintf(str, "%d days %02d:%02d:%02d", days, hours, minutes, seconds);
}

/*----------------------------------------------------------------------------
 * Function      : calculate_crc32
 * ---------------------------------------------------------------------------
 * Description   : Calculates the CRC32 checksum of a data buffer
 * Parameters    : pointer to the data buffer, length of the data buffer
 * Returns       : CRC32 checksum
 * -------------------------------------------------------------------------*/
uint32_t calculate_crc32(uint8_t *data, uint32_t length) {
    // Enable CRC clock
    __HAL_RCC_CRC_CLK_ENABLE();

    // Calculate the CRC excluding the last 4 bytes (32-bit CRC checksum)
    uint32_t crc = HAL_CRC_Calculate(&hcrc, (uint32_t*) data, length);

    // Disable CRC clock
    __HAL_RCC_CRC_CLK_DISABLE();

    return crc;
}

/*----------------------------------------------------------------------------
 * Function      : binary_to_string
 * ---------------------------------------------------------------------------
 * Description   : Converts a binary value to a string
 * Parameters    : pointer to the string, binary value
 * Returns       : None
 * -------------------------------------------------------------------------*/

void binary_to_string(char *str, uint8_t value) {
    char tmp_str[8];
    for (int i = 0; i < 8; i++) {
        if (value & (1 << i)) {
            tmp_str[7 - i] = '1';
        } else {
            tmp_str[7 - i] = '0';
        }
    }
    tmp_str[8] = '\0';
    int j = 0;
    for (int i = 0; i < 8; i++) {
        if (i == 4) {
            str[j++] = ' ';
        }
        str[j++] = tmp_str[i];
    }
    str[j] = '\0';
}
