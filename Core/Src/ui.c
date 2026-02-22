/*
 * ui.c
 *
 *  Created on: Mar 14, 2024
 *      Author: josh
 */

#include "ui.h"
#include "cmsis_os.h"

/*-----------------------------------------------------------------------------
 * Function: echo_terminal
 *
 * This function will echo a character to the terminal. Other modes will ignore.
 *
 * Parameters: scoreboard_t *s - pointer to the scoreboard
 *             uint8_t *rx_value - character to echo
 * Return: None
 *---------------------------------------------------------------------------*/
void echo_terminal(uint8_t *rx_value) {
    osDelay(1);
    CDC_Transmit_FS(rx_value, 1);
}

/*-----------------------------------------------------------------------------
 * Function: print_terminal
 *
 * This function will print a message to the terminal. Other modes will ignore.
 *
 * Parameters: scoreboard_t *s - pointer to the scoreboard
 *             char *message - message to print
 * Return: None
 *---------------------------------------------------------------------------*/
void print_terminal(char *message) {
    osDelay(1);
    CDC_Transmit_FS((uint8_t*) message, strlen(message));
}

/*-----------------------------------------------------------------------------
 * Function: print_divider
 *
 * This function will print a divider to the terminal.
 *
 * Parameters: uint8_t length - length of the divider
 * Return: None
 *---------------------------------------------------------------------------*/
void print_divider(uint8_t length) {
    char divider[UI_BUFFER_SIZE] = {0};
    memset(divider, '-', length);
    divider[length] = '\r';
    divider[length + 1] = '\n';
    divider[length + 2] = '\0';
    print_terminal(divider);
}

/*-----------------------------------------------------------------------------
 * Function: print_help
 *
 * This function will print the help menu to the terminal.
 *
 * Parameters: None
 * Return: None
 *---------------------------------------------------------------------------*/
void print_help() {
    print_divider(80);
    print_terminal("bat_mgr: Battery Management Status\r\n");
    print_terminal("scan_i2c: Scan I2C Bus\r\n");
    print_terminal("bat_gauge: Battery Fuel Status\r\n");
    print_terminal("pressure: Pressure Sensor Status\r\n");
    print_terminal("flash_memory: Flash Memory Status\r\n");
    print_terminal("serial_number: Serial Number\r\n");
    print_divider(80);
    print_terminal("> ");
}

///*-----------------------------------------------------------------------------
// * Function: print_banner
// *
// * This function will print the banner to the terminal.
// *
// * Parameters: None
// * Return: None
// *---------------------------------------------------------------------------*/
//void print_banner() {
//    print_divider(80);
//    print_terminal("Welcome to the QSPI W25Q Flash Memory Tester\r\n");
//    print_divider(80);
//}

/*-----------------------------------------------------------------------------
 * Function: print_hex
 * This function will print a hex dump to the terminal.
 * Parameters: uint8_t *data - pointer to the data
 *             uint16_t len - length of the data
 *             uint8_t data_width - width of the data
 * Return: None
 *---------------------------------------------------------------------------*/
void print_hex(uint8_t *data, uint16_t len, uint8_t data_width, uint32_t start_addr) {
    uint16_t i;
    uint8_t num_bytes_per_line = 16;
    uint8_t num_lines = (len / num_bytes_per_line) + 1;
    uint8_t remainder = len % num_bytes_per_line;
    uint8_t *ptr = data;
    uint32_t address = start_addr;
//    char hex[5];
    char ascii[17];
    char buffer[128];
    char *buffer_ptr = &buffer[0];

    for (i = 0; i < num_lines; i++) {
        if (i == num_lines - 1) {
            num_bytes_per_line = remainder;
        }
        if (num_bytes_per_line == 0) {
            break;
        }
        sprintf(buffer, "OK\t0x%08X: ", (unsigned int) address);
        print_terminal(buffer);
        osDelay(2);
        buffer_ptr = &buffer[0];
        memset(buffer, 0, sizeof(buffer));
        memset(ascii, 0, sizeof(ascii));
        for (uint8_t j = 0; j < num_bytes_per_line; j++) {
            if (*ptr < 32 || *ptr > 126) {
                ascii[j] = '.';
            } else {
                ascii[j] = *ptr;
            }
            if (j % 4 == 0) {
                sprintf(buffer_ptr, " ");
                buffer_ptr++;
            }
            sprintf(buffer_ptr, "%02X ", *ptr);
//            print_terminal(hex);
//            osDelay(2);
            ptr++;
            address++;
            buffer_ptr += 3;
        }
        print_terminal(buffer);
        osDelay(2);
        if (num_bytes_per_line < data_width) {
            for (uint8_t j = num_bytes_per_line; j < 16; j++) {
                print_terminal("   ");
                osDelay(2);
            }
        }
        memset(buffer, 0, sizeof(buffer));
        sprintf(buffer, "    %s\r\n", ascii);
        print_terminal(buffer);
        osDelay(2);
    }
}
