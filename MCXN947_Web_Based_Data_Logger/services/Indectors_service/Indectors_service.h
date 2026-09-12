/*
 * Indectors_service.h
 *
 *  Created on: Sep 6, 2026
 *      Author: rishi
 *
 * Reusable blink/solid engine for all LEDs. Knows HOW to blink or hold an
 * LED, not WHY (that's app level's job). Calls down into led_driver.h only.
 */


#ifndef INDECTORS_SERVICE_INDECTORS_SERVICE_H_
#define INDECTORS_SERVICE_INDECTORS_SERVICE_H_

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
void led_service_init(void);

/* Blink LED `index` at `interval_ms` (e.g. 100, 200, 500, 1000, 5000...) */
void led_service_set_blink(uint8_t index, uint32_t interval_ms);

/* Hold LED `index` solid on/off, no blinking */
void led_service_set_solid(uint8_t index, bool on);

/* Call every 10ms - drives all LEDs' timing */
void led_service_task_10ms(void);

/* Call once from main(), after GPIO_Init(), before vTaskStartScheduler() */
void Init_Indicator_Task(void);

/* What each of the 11 LEDs actually means in your system - EDIT names */
enum {
    LED_SYSTEM = 0,
    LED_1, LED_2, LED_3, LED_4, LED_5,
    LED_6, LED_7, LED_8, LED_9, LED_10
};

#endif /* INDECTORS_SERVICE_INDECTORS_SERVICE_H_ */
