/*
 * tutor.c
 *
 *  Created on: Sep 26, 2024
 *      Author: josh
 */

#include "tutor.h"
#include "ui.h"
#include "stdlib.h"
#include "io_expander.h"
#include "tca9555.h"
#include "ui.h"
#include "util.h"
#include "ssd1306.h"
#include "main.h"
#include "cmsis_os.h"
#include "rng.h"
#include "tiny_blake2s.h"
#include "base64.h"

// Button Mapping for IO Expander
uint16_t btn_mapping[8] = { 0x0080, 0x0040, 0x0020, 0x0010, 0x0008, 0x0004, 0x0002, 0x0001 };

// LED Mapping for IO Expander
uint16_t led_c_mapping[8] = { 0x0080, 0x0020, 0x0008, 0x0001, 0x8000, 0x2000, 0x0800, 0x0200 };
uint16_t led_s_mapping[8] = { 0x0040, 0x0010, 0x0004, 0x0002, 0x4000, 0x1000, 0x0400, 0x0100 };

// Tutor mode string
char *tutor_mode_string[TUTOR_SIZE] = { "Free Play", "Translate", "Translate (Hint)", "Logic Gates",
        "Counter", "Summary" };
char *tutor_mode_short[TUTOR_SIZE] = { "Free", "Trans", "Hint", "Logic", "Count", "Summary" };
char *tutor_logic_string[TUTOR_LOGIC_SIZE] = { "AND", "OR", "XOR", "NOR" };

// Use tutor stats table
uint8_t use_tutor_stats[TUTOR_SIZE] = { 1, 1, 1, 1, 0, 0 };

// Hash data for score validation
uint8_t secret_key[8] = SECRET_KEY;
hash_data_t tutor_score_hash_data = { 0 };
uint8_t hash_buffer[32]; // for score validation using blake2s
uint8_t base64_buffer[64]; // for score validation using blake2s
tiny_blake2s_ctx_t blake2s_ctx;
tutor_summary_t tutor_summary = { 0 };

// 16 bit value to mapping value
uint16_t convert_mapping(uint16_t value, uint16_t *mapping) {
    uint16_t result = 0;
    for (int i = 0; i < 8; i++) {
        if (value & (1 << i)) {
            result |= mapping[i];
        }
    }
    return result;
}

uint16_t convert_mapping_reverse(uint16_t value, uint16_t *mapping) {
    uint16_t result = 0;
    for (int i = 0; i < 8; i++) {
        if (value & mapping[i]) {
            result |= (1 << i);
        }
    }
    return result;
}

uint16_t get_selected_led(uint16_t value) {
    return convert_mapping(value, led_s_mapping);
}

uint16_t get_check_led(uint16_t value) {
    return convert_mapping(value, led_c_mapping);
}

uint16_t get_btn_value(uint16_t value) {
    return ~convert_mapping_reverse(value, btn_mapping); // Inverted
}

void set_select_led(tca9555_t *io_expander_led, uint16_t value) {
    io_expander_led->data = get_selected_led(value);
    tca9555_write_output(io_expander_led);
}

void set_check_led(tca9555_t *io_expander_led, uint16_t value) {
    io_expander_led->data = get_check_led(value);
    tca9555_write_output(io_expander_led);
}

void set_led(tca9555_t *io_expander_led, uint16_t value) {
    io_expander_led->data = get_selected_led(value & 0xFF) | get_check_led(value >> 8);
    tca9555_write_output(io_expander_led);
}

void wag_tag(tca9555_t *io_expander_led, uint8_t position) {
    for (uint8_t position = 0; position < 16; position++) {
        uint16_t value = 0;
        if (position < 8) {
            io_expander_led->data = get_selected_led(value);
        } else {
            io_expander_led->data = get_check_led(255 - value);
        }
        tca9555_write_output(io_expander_led);
        osDelay(100);
    }
}

uint8_t toggle_bits(uint8_t value, uint8_t mask) {
    if (value & mask) {
        return value & ~mask;
    } else {
        return value | mask;
    }
}

void draw_logic_prompt(tutor_t *tutor) {
    char str_buffer[22];
    ssd1306_FillRectangle(0, 22, 128, 53, Black);
    binary_to_string(str_buffer, tutor->logic_value1);
    ssd1306_SetCursor(14, 26);
    ssd1306_WriteString(str_buffer, Font_6x8, White);
    binary_to_string(str_buffer, tutor->logic_value2);
    ssd1306_SetCursor(14, 36);
    ssd1306_WriteString(str_buffer, Font_6x8, White);
    ssd1306_Line(12, 47, 68, 47, White);
    if (strlen(tutor_logic_string[tutor->logic_op]) > 2) {
        ssd1306_SetCursor(80, 29);
    } else {
        ssd1306_SetCursor(86, 29);
    }
    ssd1306_WriteString(tutor_logic_string[tutor->logic_op], Font_11x18, White);
    ssd1306_DrawRectangle(78, 26, 114, 47, White);
    ssd1306_UpdateScreen();
}

void setup_logic_prompt(tutor_t *tutor) {
    tutor->logic_value1 = (rand() ^ TIM5->CNT) % 256;
    tutor->logic_value2 = (rand() ^ TIM5->CNT) % 256;
    tutor->logic_op = (rand() ^ TIM5->CNT) % TUTOR_LOGIC_SIZE;
    switch (tutor->logic_op) {
    case TUTOR_LOGIC_AND:
        tutor->target_value = tutor->logic_value1 & tutor->logic_value2;
        break;
    case TUTOR_LOGIC_OR:
        tutor->target_value = tutor->logic_value1 | tutor->logic_value2;
        break;
    case TUTOR_LOGIC_XOR:
        tutor->target_value = tutor->logic_value1 ^ tutor->logic_value2;
        break;
    case TUTOR_LOGIC_NOR:
        tutor->target_value = ~(tutor->logic_value1 | tutor->logic_value2);
        break;
    default:
        break;
    }
}

void tutor_score_validation(const void *in, size_t inlen, uint8_t out[16]) {
    tiny_blake2s_init(&blake2s_ctx, 16);
    tiny_blake2s_update(&blake2s_ctx, in, inlen);
    tiny_blake2s_final(&blake2s_ctx, out, 16);
}

void tutor_init(tutor_t *tutor, tca9555_t *io_expander_led) {
    tutor->mode = TUTOR_FREE_PLAY_MODE;
    tutor->state = TUTOR_STATE_START;
    tutor->io_expander_led = io_expander_led;
    for (int i = 0; i < TUTOR_SIZE; i++) {
        memset(&tutor->stats[i], 0, sizeof(tutor_stats_t));
    }
    tutor->score = 0;
    tutor->prev_score = ~0x00;
    tutor->target_value = 0;
    tutor->refresh_time = 0;
    tutor->start_time = 0;
    tutor->round_start_time = 0;
    tutor->time_limit = 0;
    tutor->led_value = 0;
    tutor->btn_value = 0;

    memset(&tutor_summary, 0, sizeof(tutor_summary_t));
    memcpy(&tutor_summary.data.secret_key, secret_key, sizeof(secret_key));
    assert(sizeof(tutor_summary.data) == 28); // Ensure data struct is 28 bytes for hashing
    tiny_blake2s_init(&blake2s_ctx, 16);

    set_select_led(tutor->io_expander_led, 0);
}

void tutor_reset(tutor_t *tutor) {
    tutor->value = 0;
    tutor->start_time = 0;
    tutor->round_start_time = 0;
    tutor->attempt_counter = 0;
    tutor->time_limit = 0;
    tutor->target_value = 0;
    tutor->led_value = 0;
    tutor->btn_value = 0;
    tutor->prev_score = ~0x00;
    tutor->counter_speed = 800;
    tutor->refresh_time = 0;
    tutor->logic_value1 = 0;
    tutor->logic_value2 = 0;
    set_select_led(tutor->io_expander_led, 0);
}

void tutor_start(tutor_t *tutor) {
    tutor->start_time = TIM5->CNT;
    tutor->round_start_time = TIM5->CNT;
}

void tutor_task(tutor_t *tutor, tutor_action_t action) {
    char str_buffer[22];
    char num_buffer[22];
    char bit_buffer[10];
    char bit_buffer2[10];
    char serial_buffer[120];
    uint32_t time = 0;
    uint32_t total_time = 0;
    uint8_t len = 0;

    if (action == TUTOR_ACTION_MODE || action == TUTOR_ACTION_INIT) {

        if (action == TUTOR_ACTION_INIT) {  // Start of the first tutor mode at power up
            tutor->score = 0;
            tutor->state = TUTOR_STATE_START;
        } else {
            tutor->state = TUTOR_STATE_END; // Perform end of state actions before changing mode
        }

        uint8_t mode = tutor->mode;
        if (action == TUTOR_ACTION_MODE) {
            mode++;
            if (mode >= TUTOR_SIZE) {
                mode = 0;
            }
        }
        tutor->start_time = TIM5->CNT;
        tutor->check_flag = 0;
        ssd1306_Fill(Black);
        ssd1306_SetCursor(0, 0);
        sprintf(str_buffer, tutor_mode_string[mode]);
        ssd1306_WriteString(str_buffer, Font_6x8, White);
        ssd1306_Line(0, 9, 128, 9, White);
        ssd1306_UpdateScreen();
    }

    switch (tutor->mode) {
    case TUTOR_FREE_PLAY_MODE:
        if (tutor->state == TUTOR_STATE_START) {
            tutor_start(tutor);
            tutor_reset(tutor);
            tutor->stats[TUTOR_FREE_PLAY_MODE].start_time = TIM5->CNT;
            tutor->stats[TUTOR_FREE_PLAY_MODE].start_score = 0;
            tutor->state = TUTOR_STATE_PLAY;
            ssd1306_FillRectangle(0, 20, 128, 54, Black);
            sprintf(str_buffer, "%d", tutor->led_value);
            ssd1306_WriteStringCentered(str_buffer, Font_16x26, White, 22);
            ssd1306_UpdateScreen();
            print_divider(80);
            print_terminal("Free Play Mode\r\n");

        } else if (tutor->state == TUTOR_STATE_PLAY) {
            if (action == TUTOR_ACTION_BTN_PRESSED) {
                tutor->led_value = toggle_bits(tutor->led_value, tutor->btn_value);
                set_select_led(tutor->io_expander_led, tutor->led_value);
                tutor->btn_value = 0;
                ssd1306_FillRectangle(0, 20, 128, 54, Black);
                sprintf(str_buffer, "%d", tutor->led_value);
                ssd1306_WriteStringCentered(str_buffer, Font_16x26, White, 22);
                ssd1306_UpdateScreen();
                tutor->stats[TUTOR_FREE_PLAY_MODE].total++;  // Increment total for each button press
            } else if (action == TUTOR_ACTION_CHECK) {
                tutor->led_value = 0;
                tutor->btn_value = 0;
                set_select_led(tutor->io_expander_led, tutor->led_value);
                tutor->btn_value = 0;
                ssd1306_FillRectangle(0, 20, 128, 54, Black);
                sprintf(str_buffer, "%d", tutor->led_value);
                ssd1306_WriteStringCentered(str_buffer, Font_16x26, White, 22);
                ssd1306_UpdateScreen();
            } else if (action == TUTOR_ACTION_NEXT) {
                // Do nothing
            }
        } else if (tutor->state == TUTOR_STATE_END) {
            tutor->stats[tutor->mode].end_time = TIM5->CNT;
            tutor->stats[tutor->mode].end_score = tutor->score;
            sprintf(serial_buffer, "Total button presses: %d\r\n", tutor->stats[TUTOR_FREE_PLAY_MODE].total);
            print_terminal(serial_buffer);
            time = time_diff(tutor->stats[TUTOR_FREE_PLAY_MODE].start_time,
                    tutor->stats[TUTOR_FREE_PLAY_MODE].end_time);
            time_to_string(str_buffer, time, 10000);
            tutor->stats[tutor->mode].accumulated_time += time;
            sprintf(serial_buffer, "Time spent in free mode: %s\r\n", str_buffer);
            print_terminal(serial_buffer);
            tutor->mode++;
            tutor->state = TUTOR_STATE_START;
        }
        break;
    case TUTOR_CONVERT_MODE:
    case TUTOR_CONVERT_HINT_MODE:
        if (tutor->state == TUTOR_STATE_START) {
            tutor_start(tutor);
            tutor_reset(tutor);
            tutor->stats[tutor->mode].start_time = TIM5->CNT;
            tutor->stats[tutor->mode].start_score = tutor->score;
            tutor->state = TUTOR_STATE_PLAY;
            tutor->target_value = (rand() % 256) ^ (TIM5->CNT & 0xFF);
            print_divider(80);
            if (tutor->mode == TUTOR_CONVERT_HINT_MODE) {
                tutor->round_points = 1000;
                print_terminal("Translate Mode (Hint)\r\n");
            } else {
                tutor->round_points = 3000;
                print_terminal("Translate Mode\r\n");
            }
            ssd1306_FillRectangle(0, 24, 128, 50, Black);
            sprintf(str_buffer, "%d", tutor->target_value);
            ssd1306_WriteStringCentered(str_buffer, Font_16x26, White, 26);
            ssd1306_UpdateScreen();
            tutor->round_start_time = TIM5->CNT;
        } else if (tutor->state == TUTOR_STATE_PLAY) {
            if (action == TUTOR_ACTION_BTN_PRESSED) {
                tutor->value = toggle_bits(tutor->value, tutor->btn_value);
                tutor->led_value = tutor->value;
                set_select_led(tutor->io_expander_led, tutor->led_value);
                tutor->btn_value = 0;
                if (tutor->mode == TUTOR_CONVERT_HINT_MODE && tutor->time_delay == 0) {
                    sprintf(str_buffer, "(%d)   ", tutor->led_value);
                    ssd1306_SetCursor(93, 34);
                    ssd1306_WriteString(str_buffer, Font_7x10, White);
                    ssd1306_UpdateScreen();
                }
            } else if (action == TUTOR_ACTION_CHECK && !tutor->time_delay) {
                if (!tutor->check_flag) {
                    tutor->check_flag = 1;
                    tutor->stats[tutor->mode].total++;
                }
                uint8_t check_value = ~(tutor->target_value ^ tutor->value) & 0xFF;
                tutor->led_value = (uint16_t) tutor->value | (check_value << 8);
                set_led(tutor->io_expander_led, tutor->led_value);
                ssd1306_FillRectangle(0, 24, 128, 50, Black);
                if (tutor->value == tutor->target_value) {  // Correct
                    tutor->score += tutor->round_points;
                    tutor->stats[tutor->mode].correct++;
                    tutor->attempt_counter++;
                    tutor->stats[tutor->mode].end_score = tutor->score;
                    time = time_diff(tutor->round_start_time, TIM5->CNT);
                    time_to_string(str_buffer, time, 10000);
                    sprintf(serial_buffer,
                            "[Correct] Target Value: %d Attempts: %d Points: %d Score: %ld Time Elapsed: %s\r\n",
                            tutor->target_value, tutor->attempt_counter, tutor->round_points, tutor->score,
                            str_buffer);
                    print_terminal(serial_buffer);
                    tutor->time_delay = TIM5->CNT + 20000;
                    tutor->target_value = (rand() % 256) ^ (TIM5->CNT & 0xFF);
                    tutor->value = 0;
                    tutor->start_next_flag = 1;
                    tutor->attempt_counter = 0;
                    tutor->check_flag = 0;
                    sprintf(str_buffer, "Correct");
                } else {  // Incorrect
                    tutor->round_points = tutor->round_points >> 1;
                    sprintf(str_buffer, "Pts:%04d", tutor->round_points);
                    uint8_t len = strlen(str_buffer);
                    ssd1306_SetCursor(128 - (len * Font_6x8.FontWidth), 13);
                    ssd1306_WriteString(str_buffer, Font_6x8, White);
                    time = time_diff(tutor->round_start_time, TIM5->CNT);
                    time_to_string(str_buffer, time, 10000);
                    sprintf(serial_buffer,
                            "[Incorrect] Target Value: %d Value Entered: %d Time Elapsed: %s\r\n",
                            tutor->target_value, tutor->value, str_buffer);
                    print_terminal(serial_buffer);
                    tutor->time_delay = TIM5->CNT + 20000;
                    tutor->attempt_counter++;
                    tutor->stats[tutor->mode].incorrect++;
                    sprintf(str_buffer, "Try Again");
                }
                ssd1306_WriteStringCentered(str_buffer, Font_11x18, White, 28);
                ssd1306_UpdateScreen();
            } else if (action == TUTOR_ACTION_IDLE && tutor->time_delay != 0
                    && TIM5->CNT > tutor->time_delay) {
                set_select_led(tutor->io_expander_led, tutor->value);
                tutor->time_delay = 0;
                ssd1306_FillRectangle(0, 24, 128, 50, Black);
                sprintf(str_buffer, "%d", tutor->target_value);
                ssd1306_WriteStringCentered(str_buffer, Font_16x26, White, 26);
                ssd1306_UpdateScreen();

                if (tutor->start_next_flag) {
                    tutor->round_start_time = TIM5->CNT;
                    tutor->round_points = tutor->mode == TUTOR_CONVERT_HINT_MODE ? 1000 : 3000;
                    tutor->start_next_flag = 0;
                }
            } else if (action == TUTOR_ACTION_NEXT) {
                time = time_diff(tutor->round_start_time, TIM5->CNT);
                time_to_string(str_buffer, time, 10000);
                sprintf(serial_buffer, "[Skipped] Target Value: %d Score: %ld Time Elapsed: %s\r\n",
                        tutor->target_value, tutor->score, str_buffer);
                print_terminal(serial_buffer);
                tutor->target_value = (rand() % 256) ^ (TIM5->CNT & 0xFF);
                tutor->value = 0;
                tutor->start_next_flag = 1;
                tutor->attempt_counter = 0;
                tutor->check_flag = 0;
                tutor->time_delay = TIM5->CNT;
                tutor->start_next_flag = 1;
                ssd1306_FillRectangle(0, 24, 128, 50, Black);
                sprintf(str_buffer, "%d", tutor->target_value);
                ssd1306_WriteStringCentered(str_buffer, Font_16x26, White, 26);
                ssd1306_UpdateScreen();
            }
        } else if (tutor->state == TUTOR_STATE_END) {
            tutor->stats[tutor->mode].end_time = TIM5->CNT;
            tutor->stats[tutor->mode].end_score = tutor->score;
            time = time_diff(tutor->stats[tutor->mode].start_time, tutor->stats[tutor->mode].end_time);
            time_to_string(str_buffer, time, 10000);
            sprintf(serial_buffer, "Time spent in %s: %s\r\n", tutor_mode_string[tutor->mode], str_buffer);
            print_terminal(serial_buffer);
            tutor->stats[tutor->mode].accumulated_time += time;
            sprintf(serial_buffer, "Total Round Score: %ld Cumulative Score: %ld\r\n",
                    tutor->stats[tutor->mode].end_score - tutor->stats[tutor->mode].start_score,
                    tutor->score);
            print_terminal(serial_buffer);
            uint16_t num_attempts = tutor->stats[tutor->mode].correct + tutor->stats[tutor->mode].incorrect;
            float percentage = (float) tutor->stats[tutor->mode].correct / num_attempts * 100;
            float_to_string(str_buffer, percentage);
            sprintf(serial_buffer,
                    "Total correct: %d Total incorrect: %d Total problems: %d Percentage Correct: %s%%\r\n",
                    tutor->stats[tutor->mode].correct, tutor->stats[tutor->mode].incorrect,
                    tutor->stats[tutor->mode].total, str_buffer);
            print_terminal(serial_buffer);
            tutor->mode++;
            tutor->state = TUTOR_STATE_START;
        }
        break;
    case TUTOR_LOGIC_MODE:
        if (tutor->state == TUTOR_STATE_START) {
            tutor_start(tutor);
            tutor_reset(tutor);
            tutor->stats[tutor->mode].start_time = TIM5->CNT;
            tutor->stats[tutor->mode].start_score = tutor->score;
            tutor->round_start_time = TIM5->CNT;
            tutor->state = TUTOR_STATE_PLAY;
            setup_logic_prompt(tutor);
            tutor->round_points = 2000;
            draw_logic_prompt(tutor);
            print_divider(80);
            print_terminal("Logic Gates Mode\r\n");
        } else if (tutor->state == TUTOR_STATE_PLAY) {
            if (action == TUTOR_ACTION_BTN_PRESSED) {
                tutor->value = toggle_bits(tutor->value, tutor->btn_value);
                tutor->led_value = tutor->value;
                set_select_led(tutor->io_expander_led, tutor->led_value);
                tutor->btn_value = 0;
            } else if (action == TUTOR_ACTION_CHECK && !tutor->time_delay) {
                if (!tutor->check_flag) {
                    tutor->check_flag = 1;
                    tutor->stats[tutor->mode].total++;
                }

                uint8_t check_value = ~(tutor->target_value ^ tutor->value) & 0xFF;
                tutor->led_value = (uint16_t) tutor->value | (check_value << 8);
//                set_check_led(tutor->io_expander_led, tutor->led_value);
//                set_select_led(tutor->io_expander_led, tutor->led_value);
                set_led(tutor->io_expander_led, tutor->led_value);
                ssd1306_FillRectangle(0, 24, 128, 50, Black);
                if (tutor->value == tutor->target_value) {
                    tutor->score += tutor->round_points;
                    tutor->attempt_counter++;
                    tutor->stats[tutor->mode].correct++;
                    tutor->stats[tutor->mode].end_score = tutor->score;
                    time = time_diff(tutor->round_start_time, TIM5->CNT);
                    time_to_string(str_buffer, time, 10000);
                    binary_to_string(bit_buffer, tutor->logic_value1);
                    binary_to_string(bit_buffer2, tutor->logic_value2);
                    sprintf(serial_buffer, "[Correct] %s %s %s\r\n", bit_buffer,
                            tutor_logic_string[tutor->logic_op], bit_buffer2);
                    print_terminal(serial_buffer);
                    sprintf(serial_buffer,
                            "          Target Value: %d Attempts: %d Points: %d Score: %ld Time Elapsed: %s\r\n",
                            tutor->target_value, tutor->attempt_counter, tutor->round_points, tutor->score,
                            str_buffer);
                    print_terminal(serial_buffer);
                    tutor->time_delay = TIM5->CNT + 20000;
                    setup_logic_prompt(tutor);
                    tutor->value = 0;
                    tutor->start_next_flag = 1;
                    tutor->attempt_counter = 0;
                    tutor->check_flag = 0;
                    sprintf(str_buffer, "Correct");
                } else {
                    tutor->round_points = tutor->round_points >> 1;
                    sprintf(str_buffer, "Pts:%04d", tutor->round_points);
                    uint8_t len = strlen(str_buffer);
                    ssd1306_SetCursor(128 - (len * Font_6x8.FontWidth), 13);
                    ssd1306_WriteString(str_buffer, Font_6x8, White);
                    time = time_diff(tutor->round_start_time, TIM5->CNT);
                    time_to_string(str_buffer, time, 10000);
                    binary_to_string(bit_buffer, tutor->logic_value1);
                    binary_to_string(bit_buffer2, tutor->logic_value2);
                    sprintf(serial_buffer, "[Incorrect] %s %s %s\r\n", bit_buffer,
                            tutor_logic_string[tutor->logic_op], bit_buffer2);
                    print_terminal(serial_buffer);
                    sprintf(serial_buffer,
                            "            Target Value: %d Value Entered: %d Time Elapsed: %s\r\n",
                            tutor->target_value, tutor->value, str_buffer);
                    print_terminal(serial_buffer);
                    tutor->time_delay = TIM5->CNT + 20000;
                    tutor->attempt_counter++;
                    tutor->stats[tutor->mode].incorrect++;
                    sprintf(str_buffer, "Try Again");
                }
                ssd1306_WriteStringCentered(str_buffer, Font_11x18, White, 28);
                ssd1306_UpdateScreen();
            } else if (action == TUTOR_ACTION_IDLE && tutor->time_delay != 0
                    && TIM5->CNT > tutor->time_delay) {
                set_select_led(tutor->io_expander_led, tutor->value);
                tutor->time_delay = 0;
                draw_logic_prompt(tutor);
                if (tutor->start_next_flag) {
                    tutor->round_start_time = TIM5->CNT;
                    tutor->round_points = 2000;
                    tutor->start_next_flag = 0;
                }
            } else if (action == TUTOR_ACTION_NEXT) {
                time = time_diff(tutor->round_start_time, TIM5->CNT);
                time_to_string(str_buffer, time, 10000);
                binary_to_string(bit_buffer, tutor->logic_value1);
                binary_to_string(bit_buffer2, tutor->logic_value2);
                sprintf(serial_buffer, "[Skipped] %s %s %s Score: %ld Time Elapsed: %s\r\n", bit_buffer,
                        tutor_logic_string[tutor->logic_op], bit_buffer2, tutor->score, str_buffer);
                print_terminal(serial_buffer);
                setup_logic_prompt(tutor);
                tutor->value = 0;
                tutor->start_next_flag = 1;
                tutor->attempt_counter = 0;
                tutor->check_flag = 0;
                tutor->time_delay = TIM5->CNT;
                tutor->start_next_flag = 1;
            }
        } else if (tutor->state == TUTOR_STATE_END) {
            tutor->stats[tutor->mode].end_time = TIM5->CNT;
            tutor->stats[tutor->mode].end_score = tutor->score;
            time = time_diff(tutor->stats[tutor->mode].start_time, tutor->stats[tutor->mode].end_time);
            time_to_string(str_buffer, time, 10000);
            sprintf(serial_buffer, "Time spent in %s: %s\r\n", tutor_mode_string[tutor->mode], str_buffer);
            print_terminal(serial_buffer);
            tutor->stats[tutor->mode].accumulated_time += time;
            sprintf(serial_buffer, "Total Round Score: %ld Cumulative Score: %ld\r\n",
                    tutor->stats[tutor->mode].end_score - tutor->stats[tutor->mode].start_score,
                    tutor->score);
            print_terminal(serial_buffer);
            uint16_t num_attempts = tutor->stats[tutor->mode].correct + tutor->stats[tutor->mode].incorrect;
            float percentage = (float) tutor->stats[tutor->mode].correct / num_attempts * 100;
            float_to_string(str_buffer, percentage);
            sprintf(serial_buffer,
                    "Total correct: %d Total incorrect: %d Total problems: %d Percentage Correct: %s%%\r\n",
                    tutor->stats[tutor->mode].correct, tutor->stats[tutor->mode].incorrect,
                    tutor->stats[tutor->mode].total, str_buffer);
            print_terminal(serial_buffer);
            tutor->mode++;
            tutor->state = TUTOR_STATE_START;
        }

        break;
    case TUTOR_COUNTER_MODE:
        if (tutor->state == TUTOR_STATE_START) {
            tutor_start(tutor);
            tutor->stats[tutor->mode].start_time = TIM5->CNT;
            tutor->state = TUTOR_STATE_PLAY;
            tutor->value = 0;
            tutor->time_delay = TIM5->CNT + tutor->counter_speed;
            print_divider(80);
            sprintf(serial_buffer, "Counter Mode\r\n");
            print_terminal(serial_buffer);
            ssd1306_FillRectangle(0, 24, 128, 50, Black);
            sprintf(str_buffer, "%d", tutor->target_value);
            ssd1306_WriteStringCentered(str_buffer, Font_16x26, White, 26);
            ssd1306_UpdateScreen();
        } else if (tutor->state == TUTOR_STATE_PLAY) {
            if (action == TUTOR_ACTION_BTN_PRESSED) {
                tutor->counter_speed = tutor->btn_value * 250;
                tutor->time_delay = TIM5->CNT + tutor->counter_speed;
                tutor->btn_value = 0;
            } else {
                if (TIM5->CNT > tutor->time_delay) {
                    tutor->value++;
                    if (tutor->value == 256) {
                        tutor->value = 0;
                    }
                    set_check_led(tutor->io_expander_led, tutor->value);
                    tutor->time_delay = TIM5->CNT + tutor->counter_speed;
                    ssd1306_FillRectangle(0, 24, 128, 50, Black);
                    sprintf(str_buffer, "%d", tutor->value);
                    ssd1306_WriteStringCentered(str_buffer, Font_16x26, White, 26);
                    ssd1306_UpdateScreen();
                }
            }
        } else if (tutor->state == TUTOR_STATE_END) {
            tutor->stats[tutor->mode].end_time = TIM5->CNT;
            time = time_diff(tutor->stats[tutor->mode].start_time, tutor->stats[tutor->mode].end_time);
            time_to_string(str_buffer, time, 10000);
            sprintf(serial_buffer, "Time spent in %s: %s\r\n", tutor_mode_string[tutor->mode], str_buffer);
            print_terminal(serial_buffer);
            tutor->mode++;
            tutor->state = TUTOR_STATE_START;
        }
        break;
    case TUTOR_STATS_MODE:
        if (tutor->state == TUTOR_STATE_START) {
            tutor_start(tutor);
            set_select_led(tutor->io_expander_led, 0);
            set_check_led(tutor->io_expander_led, 0);
            ssd1306_FillRectangle(0, 24, 128, 50, Black);
            print_divider(80);
            print_terminal("Stats Mode\r\n");
            total_time = 0;
            for (int i = 0; i < TUTOR_SIZE - 1; i++) { // Exclude stats mode itself
                if (use_tutor_stats[i]) {
                    time = time_diff(tutor->stats[i].start_time, tutor->stats[i].end_time);
                    time_to_string(str_buffer, time, 10000);
                    total_time += tutor->stats[i].accumulated_time;
                    sprintf(serial_buffer, "%s - Time: %s Score: %ld Correct: %d Incorrect: %d Total: %d\r\n",
                            tutor_mode_string[i], str_buffer,
                            tutor->stats[i].end_score - tutor->stats[i].start_score, tutor->stats[i].correct,
                            tutor->stats[i].incorrect, tutor->stats[i].total);
                    print_terminal(serial_buffer);
                }
            }

            time_to_string(str_buffer, total_time, 10000);
            sprintf(serial_buffer, "Time Spent: %s", str_buffer);
            print_terminal(serial_buffer);

            // Print stats on the screen as well
            for (int i = 1; i < TUTOR_SIZE - 1; i++) {
                if (use_tutor_stats[i]) {
                    tutor->stats[i].accumulated_score += tutor->stats[i].end_score
                            - tutor->stats[i].start_score;
                    float percentage = (float) tutor->stats[i].correct
                            / (tutor->stats[i].correct + tutor->stats[i].incorrect) * 100;
                    float_to_string(num_buffer, percentage);
                    snprintf(serial_buffer, sizeof(serial_buffer), "%s: %ld (%s%%)", tutor_mode_short[i],
                            tutor->stats[i].accumulated_score, num_buffer);
                    len = strlen(serial_buffer);
                    ssd1306_SetCursor(0, 24 + ((i - 1) * 10));
                    ssd1306_WriteString(serial_buffer, Font_6x8, White);
                }
            }
            ssd1306_SetCursor(0, 54);
            snprintf(serial_buffer, sizeof(serial_buffer), "Score: %ld", tutor->score);
            ssd1306_WriteString(serial_buffer, Font_6x8, White);

            // Output salt for validation
            if (tutor_summary.data.salt == 0) {
                tutor_summary.data.salt = rng_next() & 0xFF;
            }

            // Generate validation key using blake2s hash of the hash_data and salt
            tutor_summary.page = 0;
            tutor_summary.page_updated = 1;
            tutor_summary.total_time = total_time;
            tutor_summary.data.total_score = tutor->score;
            tutor_summary.data.translate_score = tutor->stats[TUTOR_CONVERT_MODE].accumulated_score;
            tutor_summary.data.translate_hint_score = tutor->stats[TUTOR_CONVERT_HINT_MODE].accumulated_score;
            tutor_summary.data.logic_score = tutor->stats[TUTOR_LOGIC_MODE].accumulated_score;
            tutor_score_validation((void*) &tutor_summary.data, sizeof(hash_data_t), hash_buffer);
            base64_encode(base64_buffer, 64, hash_buffer, sizeof(hash_buffer));
            tutor->state = TUTOR_STATE_PLAY;
        } else if (tutor->state == TUTOR_STATE_PLAY) {
            if (action == TUTOR_ACTION_BTN_PRESSED) {
                // Do nothing
            } else if (action == TUTOR_ACTION_CHECK) {
                // Do nothing
            } else if (action == TUTOR_ACTION_NEXT) {
                tutor_summary.page = !tutor_summary.page;
                tutor_summary.page_updated = 1;
            }
            if (tutor_summary.page_updated) {
                ssd1306_Fill(Black);
                ssd1306_SetCursor(0, 0);
                sprintf(str_buffer, tutor_mode_string[TUTOR_STATS_MODE]);
                ssd1306_WriteString(str_buffer, Font_6x8, White);
                ssd1306_Line(0, 9, 128, 9, White);
                if (tutor_summary.page) {
                    snprintf(serial_buffer, sizeof(serial_buffer), "ID: %02X", tutor_summary.data.salt);
                    len = strlen(serial_buffer);
                    ssd1306_SetCursor(128 - (len * Font_6x8.FontWidth), 50); // Right align
                    ssd1306_WriteString(serial_buffer, Font_6x8, White);

                    snprintf(serial_buffer, sizeof(serial_buffer), "< Prev");
                    len = strlen(serial_buffer);
                    ssd1306_SetCursor(128 - (len * Font_6x8.FontWidth), 00); // Right align
                    ssd1306_WriteString(serial_buffer, Font_6x8, White);
                    sprintf(str_buffer, "Validation Key:");
                    ssd1306_SetCursor(0, 14);
                    ssd1306_WriteString(str_buffer, Font_6x8, White);
                    ssd1306_SetCursor(0, 30);
                    base64_buffer[17] = '\0'; // Null terminate to fit on one line]
                    ssd1306_WriteString((char*) base64_buffer, Font_6x8, White);
                    ssd1306_UpdateScreen();
                } else {
                    snprintf(serial_buffer, sizeof(serial_buffer), "Next >");
                    len = strlen(serial_buffer);
                    ssd1306_SetCursor(128 - (len * Font_6x8.FontWidth), 00); // Right align
                    ssd1306_WriteString(serial_buffer, Font_6x8, White);
                    time_to_string(str_buffer, tutor_summary.total_time, 10000);
                    sprintf(serial_buffer, "Time Spent: %s", str_buffer);
                    print_terminal(serial_buffer);
                    len = strlen(serial_buffer);
                    ssd1306_SetCursor(0, 14); // Left align
                    ssd1306_WriteString(serial_buffer, Font_6x8, White);

                    for (int i = 1; i < TUTOR_SIZE - 1; i++) {
                        if (use_tutor_stats[i]) {
                            float percentage = (float) tutor->stats[i].correct
                                    / (tutor->stats[i].correct + tutor->stats[i].incorrect) * 100;
                            float_to_string(num_buffer, percentage);
                            snprintf(serial_buffer, sizeof(serial_buffer), "%s: %ld (%s%%)",
                                    tutor_mode_short[i], tutor->stats[i].accumulated_score, num_buffer);
                            len = strlen(serial_buffer);
                            ssd1306_SetCursor(0, 24 + ((i - 1)) * 10);
                            ssd1306_WriteString(serial_buffer, Font_6x8, White);
                        }
                    }
                    ssd1306_SetCursor(0, 54);
                    snprintf(serial_buffer, sizeof(serial_buffer), "Total: %ld", tutor->score);
                    ssd1306_WriteString(serial_buffer, Font_6x8, White);
                    ssd1306_UpdateScreen();
                }
                tutor_summary.page_updated = 0;
            }
            // Do nothing
        } else if (tutor->state == TUTOR_STATE_END) {
            tutor->mode = 0;
            tutor->state = TUTOR_STATE_START;
        }
        break;
    default:
        break;
    }
    if (tutor->state == TUTOR_STATE_PLAY) {
        uint8_t update_screen = 0;
        if (TIM5->CNT - tutor->refresh_time > 10000 && tutor->mode != TUTOR_FREE_PLAY_MODE
                && tutor->mode != TUTOR_COUNTER_MODE && tutor->mode != TUTOR_STATS_MODE) {
            tutor->refresh_time = TIM5->CNT;
            time = time_diff(tutor->start_time, TIM5->CNT);
            time_to_string(str_buffer, time, 10000);
            len = strlen(str_buffer);
            ssd1306_SetCursor(128 - (len * Font_6x8.FontWidth), 54);
            ssd1306_WriteString(str_buffer, Font_6x8, White);

            if (!tutor->start_next_flag && !tutor->time_delay) {
                time = time_diff(tutor->round_start_time, TIM5->CNT);
                time_to_string(str_buffer, time, 10000);
                ssd1306_SetCursor(0, 13);
                ssd1306_WriteString(str_buffer, Font_6x8, White);

                time = time_diff(tutor->round_start_time, TIM5->CNT) / 5000;
                if (tutor->round_points > time) {
                    tutor->round_points = tutor->round_points - time;
                } else {
                    tutor->round_points = 0;
                }

                sprintf(str_buffer, "Pts:%04d", tutor->round_points);
                uint8_t len = strlen(str_buffer);
                ssd1306_SetCursor(128 - (len * Font_6x8.FontWidth), 13);
                ssd1306_WriteString(str_buffer, Font_6x8, White);
            }
            update_screen = 1;
        }
        if (tutor->prev_score != tutor->score && tutor->mode != TUTOR_FREE_PLAY_MODE
                && tutor->mode != TUTOR_COUNTER_MODE && tutor->mode != TUTOR_STATS_MODE) {
            tutor->prev_score = tutor->score;
            sprintf(str_buffer, "Score:%06ld", tutor->score);
            ssd1306_SetCursor(0, 54);
            ssd1306_WriteString(str_buffer, Font_6x8, White);
            update_screen = 1;
        }
        if (update_screen)
            ssd1306_UpdateScreen();
    }
}
