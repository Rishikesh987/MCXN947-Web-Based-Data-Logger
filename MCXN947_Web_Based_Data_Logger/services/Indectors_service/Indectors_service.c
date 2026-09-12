/*
 * Indectors_service.c
 *
 *  Created on: Sep 6, 2026
 *      Author: rishi
 *
 * Status Indicator service.
 *
 * Provides a visual indication of system status using LEDs or other
 * indicator devices. Each indicator can operate with a different blink
 * interval (e.g. 100 ms, 500 ms, etc.) depending on the required status.
 *
 * The task is designed as a generic status indication mechanism and can
 * be extended to support LEDs or other output indicators in the future.
 */


#include <My_Indectors_driver.h>

#include "Indectors_service.h"
#include "task.h"
#include "fsl_debug_console.h"
/* Stack size allocated for the  task.*/
#define TASK_3_STACK_SIZE  200
#define TASK_3_PRIORITY (configMAX_PRIORITIES - 3)


#define LED_TASK_PERIOD_MS   10
#define NUM_LEDS             LED_DRIVER_NUM_LEDS
#define LED_TASK_PERIOD_MS   10


typedef struct {
    uint32_t interval_ticks;  /* 0 = solid, not blinking */
    uint32_t counter;
    bool     level;
} led_state_t;

static led_state_t led[NUM_LEDS];

void led_service_init(void)
{

    for (uint8_t i = 0; i < NUM_LEDS; i++)
    {
        led[i].interval_ticks = 0;
        led[i].counter = 0;
        led[i].level = false;
    }
}

void led_service_set_blink(uint8_t index, uint32_t interval_ms)
{
    if (index >= NUM_LEDS) return;
    led[index].interval_ticks = interval_ms / LED_TASK_PERIOD_MS;
    led[index].counter = 0;
}

void led_service_set_solid(uint8_t index, bool on)
{
    if (index >= NUM_LEDS) return;
    led[index].interval_ticks = 0;
    led[index].level = on;
    led_driver_write(index, on);
}

void led_service_task_10ms(void)
{
    for (uint8_t i = 0; i < NUM_LEDS; i++)
    {
        if (led[i].interval_ticks == 0) continue;  /* solid, nothing to do */

        led[i].counter++;
        if (led[i].counter >= led[i].interval_ticks)
        {
            led[i].counter = 0;
            led[i].level = !led[i].level;
            led_driver_write(i, led[i].level);
        }
    }
}

/* --- FreeRTOS task: drives led_service_task_10ms() on a fixed period --- */

static void Indicator_Task(void *pvParameters)
{
    const TickType_t xPeriod = pdMS_TO_TICKS(LED_TASK_PERIOD_MS);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        led_service_task_10ms();
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

void Init_Indicator_Task(void)
{
	BOARD_InitLEDsPins();
	led_service_init();

    /* Default assignments of LEDs */
    led_service_set_blink(LED_1, 500);
    led_service_set_blink(LED_2, 2000);
    led_service_set_blink(LED_3, 5000);


    if (xTaskCreate(Indicator_Task, "Indicator_Task", TASK_3_STACK_SIZE, NULL, TASK_3_PRIORITY, NULL) != pdPASS)
    {
       	PRINTF("[RTOS][ERROR] Indicator task creation FAILED!\r\n");

    }
    else{
        PRINTF("[RTOS][OK] Indicator task created successfully\r\n");
    }
}













