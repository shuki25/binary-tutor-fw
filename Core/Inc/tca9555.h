/*
 * tca9555.h
 *
 *  Created on: Aug 29, 2024
 *      Author: josh
 */

#include "main.h"

#ifndef INC_TCA9555_H_
#define INC_TCA9555_H_

#define TCA9555_BASE_ADDRESS 0x20  // 7-bit Base address of TCA9555 (0100 0xxx) where x is set by the address pins A2, A1, and A0.

// Register addresses
#define TCA9555_INPUT_PORT0 0x00    // Input port 0 - read only
#define TCA9555_INPUT_PORT1 0x01    // Input port 1 - read only
#define TCA9555_OUTPUT_PORT0 0x02   // Output port 0 - write only
#define TCA9555_OUTPUT_PORT1 0x03   // Output port 1 - write only
#define TCA9555_POLARITY_INVERSION_PORT0 0x04  // Polarity inversion port 0 - 0 = normal, 1 = inverted
#define TCA9555_POLARITY_INVERSION_PORT1 0x05  // Polarity inversion port 1 - 0 = normal, 1 = inverted
#define TCA9555_CONFIGURATION_PORT0 0x06 // Configuration port 0 - 0 = output, 1 = input
#define TCA9555_CONFIGURATION_PORT1 0x07 // Configuration port 1 - 0 = output, 1 = input

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint8_t address;
    uint8_t port0_config;
    uint8_t port1_config;
    uint16_t data;
    uint8_t port0_data;
    uint8_t port1_data;
} tca9555_t;


HAL_StatusTypeDef tca9555_init(tca9555_t *tca9555, I2C_HandleTypeDef *hi2c, uint8_t address);
HAL_StatusTypeDef tca9555_read(tca9555_t *tca9555, uint8_t reg, uint8_t *data);
HAL_StatusTypeDef tca9555_write(tca9555_t *tca9555, uint8_t reg, uint8_t data);
HAL_StatusTypeDef tca9555_set_port0_config(tca9555_t *tca9555, uint8_t config);
HAL_StatusTypeDef tca9555_set_port1_config(tca9555_t *tca9555, uint8_t config);
HAL_StatusTypeDef tca9555_set_config(tca9555_t *tca9555);
HAL_StatusTypeDef tca9555_read_input(tca9555_t *tca9555);
HAL_StatusTypeDef tca9555_write_output(tca9555_t *tca9555);

#endif /* INC_TCA9555_H_ */
