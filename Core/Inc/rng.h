/**
  ******************************************************************************
  * @file           : rng.h
  * @author         : Dr. Joshua Butler
  * @date           : Dec 9, 2024
  * @brief          : Random number generator for selecting the next tetrimino
  ******************************************************************************
  * @attention
  *
  * 2025 Imagine RIT Project: Classic Tetris on LED Grid
  *
  * Copyright (c) 2024-25 Rochester Institute of Technology.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#ifndef INC_RNG_H_
#define INC_RNG_H_

#include <stdint.h>

void rng_init(uint16_t seed);
uint16_t rng_next(void);

#endif /* INC_RNG_H_ */
