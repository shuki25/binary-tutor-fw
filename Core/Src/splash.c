/*
 * splash.c
 *
 *  Created on: Dec 31, 2023
 *      Author: Joshua Butler, MD, MHI
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "splash.h"
#include "cmsis_os.h"
#include "main.h"
#include "ssd1306.h"


void splash(void) {

    uint8_t buffer[19];

    ssd1306_Init();
    ssd1306_SetContrast(20);
    ssd1306_Fill(Black);
    ssd1306_SetCursor(16, 1);
    ssd1306_WriteString("BUTLER", Font_16x26, White);
    ssd1306_SetCursor(42, 28);
    ssd1306_WriteString("TECH", Font_11x18, White);
    ssd1306_SetCursor(0, 55);
    ssd1306_WriteString("(C)2024", Font_6x8, White);
    snprintf((char*) buffer, 12, "v%d.%d.%d%s", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, BOARD_REVISION);
    uint8_t len = strlen((char*) buffer);
    ssd1306_SetCursor(128 - (len * 6), 55);
    ssd1306_WriteString((char*) buffer, Font_6x8, White);
    ssd1306_UpdateScreen();
    osDelay(1000);
    ssd1306_Fill(Black);

    // Jitter time to randomize number
    for (uint8_t i = rand() % 256; i > 0; i-- ) {
        rand();
        __NOP();
    }
    // Random binary number appears in the splash screen (18 colunms by 6 rows)
    for (uint8_t i = 0; i < 8; i++) {
        for (uint8_t j = 0; j < 21; j++) {
            buffer[j] = ((rand() ^ TIM5->CNT) % 2) ? '1' : '0';
        }
        ssd1306_SetCursor(1, (i * Font_6x8.FontHeight));
        ssd1306_WriteString((char *)buffer, Font_6x8, White);
    }
    ssd1306_FillRectangle(24, 11, 102, 51, Black);
    ssd1306_DrawRectangle(24, 11, 102, 51, White);
//    ssd1306_WriteStringCentered("        ", Font_11x18, White, 14);
    ssd1306_WriteStringCentered("Binary", Font_11x18, White, 14);
//    ssd1306_WriteStringCentered("       ", Font_11x18, White, 32);
    ssd1306_WriteStringCentered("Tutor", Font_11x18, White, 32);
    ssd1306_UpdateScreen();
    osDelay(2000);
}

void draw_home_screen(void) {

    ssd1306_Init();
    ssd1306_UpdateScreen();
//    ssd1306_SetContrast(20);
    ssd1306_Fill(Black);
    ssd1306_UpdateScreen();
}
