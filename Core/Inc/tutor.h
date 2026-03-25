/*
 * tutor.h
 *
 *  Created on: Sep 26, 2024
 *      Author: josh
 */

#ifndef INC_TUTOR_H_
#define INC_TUTOR_H_

#include "main.h"
#include <stdint.h>
#include "tca9555.h"

typedef enum {
    TUTOR_FREE_PLAY_MODE = 0,
    TUTOR_CONVERT_MODE,
    TUTOR_CONVERT_HINT_MODE,
    TUTOR_LOGIC_MODE,
    TUTOR_COUNTER_MODE,
    TUTOR_STATS_MODE,
    TUTOR_SIZE
} tutor_mode_t;

typedef enum {
    TUTOR_STATE_START = 0, TUTOR_STATE_PLAY, TUTOR_STATE_END
} tutor_state_t;

typedef enum {
    TUTOR_ACTION_INIT = 0,
    TUTOR_ACTION_IDLE,
    TUTOR_ACTION_MODE,
    TUTOR_ACTION_CHECK,
    TUTOR_ACTION_NEXT,
    TUTOR_ACTION_BTN_PRESSED,
    TUTOR_ACTION_SIZE
} tutor_action_t;

typedef enum {
    TUTOR_LOGIC_AND = 0, TUTOR_LOGIC_OR, TUTOR_LOGIC_XOR, TUTOR_LOGIC_NOR, TUTOR_LOGIC_SIZE
} tutor_logic_t;

typedef struct {
    uint32_t start_time;
    uint32_t end_time;
    uint32_t accumulated_time;
    uint16_t correct;
    uint16_t incorrect;
    uint16_t total;
    uint32_t start_score;
    uint32_t end_score;
} tutor_stats_t;

typedef struct {
    tutor_mode_t mode;
    tutor_state_t state;
    tca9555_t *io_expander_led;
    tutor_stats_t stats[TUTOR_SIZE];
    uint32_t score;
    uint32_t prev_score;
    uint8_t start_next_flag;
    uint8_t check_flag;
    uint32_t start_time;
    uint32_t round_start_time;
    uint16_t round_points;
    uint8_t attempt_counter;
    uint32_t end_time;
    uint32_t time_limit;
    uint32_t time_delay;
    uint32_t refresh_time;
    uint8_t value;
    uint8_t logic_value1;
    uint8_t logic_value2;
    tutor_logic_t logic_op;
    uint8_t target_value;
    uint16_t led_value;
    uint16_t btn_value;
    uint16_t counter_speed;
} tutor_t;

uint16_t get_btn_value(uint16_t value);
uint16_t get_selected_led(uint16_t value);
uint16_t get_check_led(uint16_t value);
void tutor_init(tutor_t *tutor, tca9555_t *io_expander_led);
void tutor_start(tutor_t *tutor);
void tutor_task(tutor_t *tutor, tutor_action_t action);

#endif /* INC_TUTOR_H_ */
