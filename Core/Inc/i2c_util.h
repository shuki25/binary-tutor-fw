/*
 * i2c_util.h
 *
 *  Created on: Aug 27, 2024
 *      Author: josh
 */

#ifndef INC_I2C_UTIL_H_
#define INC_I2C_UTIL_H_

#include "main.h"

HAL_StatusTypeDef scan_i2c_bus(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef reset_i2c_bus(I2C_HandleTypeDef *hi2c);

#endif /* INC_I2C_UTIL_H_ */
