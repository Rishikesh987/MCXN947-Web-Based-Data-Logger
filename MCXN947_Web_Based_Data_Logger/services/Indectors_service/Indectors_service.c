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
#include "HealthMonitor_service.h"
#include "task_config.h"
/* Stack size allocated for the  task.*/

#define NUM_LEDS             LED_DRIVER_NUM_LEDS

TaskHandle_t Indicator_Handle = NULL; /* Indicator LED task handle */
static uint8_t Indicator_TaskId;
typedef struct {
	uint32_t interval_ticks; /* 0 = solid, not blinking */
	uint32_t counter;
	bool level;
} led_state_t;

static led_state_t led[NUM_LEDS];

void led_service_init(void) {

	for (uint8_t i = 0; i < NUM_LEDS; i++) {
		led[i].interval_ticks = 0;
		led[i].counter = 0;
		led[i].level = false;
	}
}

void led_service_set_blink(uint8_t index, uint32_t interval_ms) {
	if (index >= NUM_LEDS)
		return;
	led[index].interval_ticks = interval_ms / PERIOD_INDICATOR_TASK_MS;
	led[index].counter = 0;
}

void led_service_set_solid(uint8_t index, bool on) {
	if (index >= NUM_LEDS)
		return;
	led[index].interval_ticks = 0;
	led[index].level = on;
	led_driver_write(index, on);
}

void led_service_task_10ms(void) {
	for (uint8_t i = 0; i < NUM_LEDS; i++) {
		if (led[i].interval_ticks == 0)
			continue; /* solid, nothing to do */

		led[i].counter++;
		if (led[i].counter >= led[i].interval_ticks) {
			led[i].counter = 0;
			led[i].level = !led[i].level;
			led_driver_write(i, led[i].level);
		}
	}
}

/* --- FreeRTOS task: drives led_service_task_10ms() on a fixed period --- */

static void Indicator_Task(void *pvParameters) {
	const TickType_t xPeriod = pdMS_TO_TICKS(PERIOD_INDICATOR_TASK_MS);
	TickType_t xLastWakeTime = xTaskGetTickCount();
	uint32_t receivedBits = 0;

	for (;;) {
		if (xTaskNotifyWait(0x00, 0xFFFFFFFF, &receivedBits,0) == pdPASS) {
			Reset_HeltMonit_service(Indicator_TaskId); /* Reset the health monitor service for this task */
		}
		led_service_task_10ms();
		vTaskDelayUntil(&xLastWakeTime, xPeriod);
	}
}

void Init_Indicator_Task(void) {
	BOARD_InitLEDsPins();
	led_service_init();

	/* Default assignments of LEDs */
	led_service_set_blink(LED_1, 500);
	led_service_set_blink(LED_2, 2000);
	led_service_set_blink(LED_3, 5000);

	/* Register indicator task with health monitor - 30ms timeout */
	Indicator_TaskId = Registor_service("Indicator_Task", HMS_TIMEOUT_INDICATOR_MS, true,
			&Indicator_Handle);

	if (xTaskCreate(Indicator_Task, "Indicator_Task", STACK_INDICATOR_TASK, NULL,
			PRIO_INDICATOR_TASK, &Indicator_Handle) != pdPASS) {
		PRINTF("[RTOS][ERROR] Indicator task creation FAILED!\r\n");

	} else {
		PRINTF("[RTOS][OK] Indicator task created successfully\r\n");
	}
}

