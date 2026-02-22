/*
 * tca9555.c
 *
 *  Created on: Aug 29, 2024
 *      Author: josh
 */

#include "tca9555.h"
#include "main.h"

HAL_StatusTypeDef tca9555_init(tca9555_t *tca9555, I2C_HandleTypeDef *hi2c, uint8_t address) {
    tca9555->hi2c = hi2c;
    tca9555->address = address;
    tca9555->port0_config = 0xFF;
    tca9555->port1_config = 0xFF;
    tca9555->port0_data = 0xFF;
    tca9555->port1_data = 0xFF;

    return HAL_OK;
}

HAL_StatusTypeDef tca9555_read(tca9555_t *tca9555, uint8_t reg, uint8_t *data) {
    HAL_StatusTypeDef status;
    status = HAL_I2C_Mem_Read(tca9555->hi2c, tca9555->address << 1, reg, I2C_MEMADD_SIZE_8BIT, data, 1, 100);
    return status;
}

HAL_StatusTypeDef tca9555_write(tca9555_t *tca9555, uint8_t reg, uint8_t data) {
    HAL_StatusTypeDef status;
    status = HAL_I2C_Mem_Write(tca9555->hi2c, tca9555->address << 1, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
    return status;
}

HAL_StatusTypeDef tca9555_set_port0_config(tca9555_t *tca9555, uint8_t config) {
    HAL_StatusTypeDef status;
    tca9555->port0_config = config;
    status = tca9555_write(tca9555, TCA9555_CONFIGURATION_PORT0, config);
    return status;
}

HAL_StatusTypeDef tca9555_set_port1_config(tca9555_t *tca9555, uint8_t config) {
    HAL_StatusTypeDef status;
    tca9555->port1_config = config;
    status = tca9555_write(tca9555, TCA9555_CONFIGURATION_PORT1, config);
    return status;
}

HAL_StatusTypeDef tca9555_set_config(tca9555_t *tca9555) {
    HAL_StatusTypeDef status;
    status = tca9555_write(tca9555, TCA9555_CONFIGURATION_PORT0, tca9555->port0_config);
    if (status != HAL_OK) {
        return status;
    }
    status = tca9555_write(tca9555, TCA9555_CONFIGURATION_PORT1, tca9555->port1_config);
    return status;
}

HAL_StatusTypeDef tca9555_read_input(tca9555_t *tca9555) {
    HAL_StatusTypeDef status;
    status = tca9555_read(tca9555, TCA9555_INPUT_PORT0, &tca9555->port0_data);
    if (status != HAL_OK) {
        return status;
    }
    status = tca9555_read(tca9555, TCA9555_INPUT_PORT1, &tca9555->port1_data);
    tca9555->data = (tca9555->port1_data << 8) | tca9555->port0_data;
    return status;
}

HAL_StatusTypeDef tca9555_write_output(tca9555_t *tca9555) {
    HAL_StatusTypeDef status;

    tca9555->port0_data = tca9555->data & 0xFF;
    tca9555->port1_data = (tca9555->data >> 8) & 0xFF;

    status = tca9555_write(tca9555, TCA9555_OUTPUT_PORT0, tca9555->port0_data);
    if (status != HAL_OK) {
        return status;
    }
    status = tca9555_write(tca9555, TCA9555_OUTPUT_PORT1, tca9555->port1_data);
    return status;
}
