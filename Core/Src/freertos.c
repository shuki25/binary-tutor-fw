/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ring_buffer.h"
#include <string.h>
#include "i2c_util.h"
#include "splash.h"
#include "tca9555.h"
#include "io_expander.h"
#include "ssd1306.h"
#include "ui.h"
#include "tutor.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
extern uint8_t update_screen_flag;
extern uint8_t button_interrupt_flag;
extern uint8_t button_mode_flag;
extern uint8_t button_check_flag;
extern uint8_t button_next_flag;
extern uint8_t i2c_interrupt_flag;
extern ring_buffer_t rx_buffer;
extern I2C_HandleTypeDef hi2c1;
extern TIM_HandleTypeDef htim5;
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = { .name = "defaultTask", .stack_size = 512 * 4, .priority =
        (osPriority_t) osPriorityNormal, };

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void) {
    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* USER CODE BEGIN RTOS_MUTEX */
    /* add mutexes, ... */
    /* USER CODE END RTOS_MUTEX */

    /* USER CODE BEGIN RTOS_SEMAPHORES */
    /* add semaphores, ... */
    /* USER CODE END RTOS_SEMAPHORES */

    /* USER CODE BEGIN RTOS_TIMERS */
    /* start timers, add new ones, ... */
    /* USER CODE END RTOS_TIMERS */

    /* USER CODE BEGIN RTOS_QUEUES */
    /* add queues, ... */
    /* USER CODE END RTOS_QUEUES */

    /* Create the thread(s) */
    /* creation of defaultTask */
    defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

    /* USER CODE BEGIN RTOS_THREADS */
    /* add threads, ... */
    /* USER CODE END RTOS_THREADS */

    /* USER CODE BEGIN RTOS_EVENTS */
    /* add events, ... */
    /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument) {
    /* init code for USB_DEVICE */
    MX_USB_DEVICE_Init();
    /* USER CODE BEGIN StartDefaultTask */

    // Local variables
    tca9555_t io_expander_btn;
    tca9555_t io_expander_led;
    char terminal_buffer[80];
    tutor_t tutor = { 0 };

    // Start timer
    HAL_TIM_Base_Start_IT(&htim5);

    // Show splash screen
    splash();

    // Scan I2C bus for devices
    scan_i2c_bus(&hi2c1);

    // Initialize I2C I/O expander
    tca9555_init(&io_expander_btn, &hi2c1, IO_EXPANDER_ADDRESS_BTN);
    tca9555_init(&io_expander_led, &hi2c1, IO_EXPANDER_ADDRESS_LED);
    io_expander_btn.port0_config = IO_EXPANDER_CONFIG_BTN & 0xFF;
    io_expander_btn.port1_config = (IO_EXPANDER_CONFIG_BTN >> 8) && 0xFF;
    io_expander_led.port0_config = IO_EXPANDER_CONFIG_LED & 0xFF;
    io_expander_led.port1_config = (IO_EXPANDER_CONFIG_LED >> 8) && 0xFF;
    tca9555_set_config(&io_expander_btn);
    tca9555_set_config(&io_expander_led);

//    uint16_t value = 0;
    uint16_t prev_btn_value = 0;
    tutor_action_t action = TUTOR_ACTION_INIT;

    tutor_init(&tutor, &io_expander_led);

    /* Infinite loop */
    for (;;) {
        if (i2c_interrupt_flag) {
            i2c_interrupt_flag = 0;
//            print_terminal("I2C Interrupt\n");
            tca9555_read_input(&io_expander_btn);
            uint16_t btn_value = get_btn_value(io_expander_btn.data) & 0xFF;
            if (!(btn_value & prev_btn_value) && btn_value) {
//                print_terminal("Button Value: ");
                sprintf(terminal_buffer, "%04X %04d\n", btn_value, btn_value);
//                print_terminal(terminal_buffer);
                action = TUTOR_ACTION_BTN_PRESSED;
                tutor.btn_value = btn_value;
            }
            prev_btn_value = btn_value;
        }
        if (button_interrupt_flag) {
            button_interrupt_flag = 0;
            if (button_next_flag) {
                button_next_flag = 0;
//                print_terminal("Next Button\n");
                action = TUTOR_ACTION_NEXT;
            }
            if (button_check_flag) {
                button_check_flag = 0;
//                print_terminal("Check Button\n");
                action = TUTOR_ACTION_CHECK;
            }
            if (button_mode_flag) {
                button_mode_flag = 0;
//                print_terminal("Mode Button\n");
                action = TUTOR_ACTION_MODE;
            }
        }
//        io_expander_led.data = get_selected_led(value) | get_check_led(255 - value);
//        tca9555_write_output(&io_expander_led);
//        value++;
//        if (value == 256) {
//            value = 0;
//        }
//        osDelay(75);
        tutor_task(&tutor, action);
        action = TUTOR_ACTION_IDLE;
        osThreadYield();
    }
    /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

