/*
 * io_expander.h
 *
 *  Created on: Sep 26, 2024
 *      Author: josh
 */

#ifndef INC_IO_EXPANDER_H_
#define INC_IO_EXPANDER_H_

#define IO_EXPANDER_ADDRESS_BTN 0x20
#define IO_EXPANDER_ADDRESS_LED 0x21

// IO Expander Configuration (1 = input, 0 = output)
#define IO_EXPANDER_CONFIG_BTN 0b0000000011111111 // 0x00FF
#define IO_EXPANDER_CONFIG_LED 0b0000000000000000 // 0x0000

#endif /* INC_IO_EXPANDER_H_ */
