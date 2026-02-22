/*
 * ui.h
 *
 *  Created on: Mar 14, 2024
 *      Author: josh
 */

#ifndef INC_UI_H_
#define INC_UI_H_

#include "main.h"
#include "usbd_cdc_if.h"

#define UI_BUFFER_SIZE 256

void echo_terminal(uint8_t *rx_value);
void print_terminal(char *message);
void print_divider(uint8_t length);
void print_help();
void print_hex(uint8_t *data, uint16_t len, uint8_t data_width, uint32_t start_addr);

#endif /* INC_UI_H_ */
