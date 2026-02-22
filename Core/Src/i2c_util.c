/*
 * i2c_util.c
 *
 *  Created on: Aug 27, 2024
 *      Author: josh
 */

#include "i2c.h"
#include "ui.h"
#include <string.h>
#include <stdio.h>
#include "cmsis_os.h"
#include "main.h"

HAL_StatusTypeDef scan_i2c_bus(I2C_HandleTypeDef *hi2c) {
    HAL_StatusTypeDef status;
    uint8_t address = 0;
    uint8_t output[128] = { 0 };

    for (address = 1; address < 128; address++) {
        status = HAL_I2C_IsDeviceReady(hi2c, address << 1, 1, 100);
        if (status == HAL_OK) {
            sprintf((char*) output, "OK\tDevice found at address 0x%02X\n", address);
            print_terminal((char*) output);
        }
    }

    sprintf((char*) output, "END\tScanning complete.\n");
    print_terminal((char*) output);
    return HAL_OK;
}

HAL_StatusTypeDef reset_i2c_bus(I2C_HandleTypeDef *hi2c) {
    HAL_StatusTypeDef status;

// Reset the I2C bus

    SYSCFG->CFGR |= SYSCFG_CFGR_FMPI2C1_SCL;
    SYSCFG->CFGR |= SYSCFG_CFGR_FMPI2C1_SDA;
    HAL_I2C_DeInit(hi2c);
    SYSCFG->CFGR &= ~SYSCFG_CFGR_FMPI2C1_SCL;
    SYSCFG->CFGR &= ~SYSCFG_CFGR_FMPI2C1_SDA;

    status = HAL_I2C_Init(hi2c);
    if (status != HAL_OK) {
        return status;
    }
    osDelay(100);
    return HAL_OK;
}

