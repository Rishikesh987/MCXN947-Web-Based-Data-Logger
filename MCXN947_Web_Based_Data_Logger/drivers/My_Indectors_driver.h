/*
 * Indector_drivers.h
 *
 *  Created on: Sep 6, 2026
 *      Author: rishi
 *
 * Only responsibility: turn a given LED's physical pin on/off through the
 * CMSIS GPIO driver. No timing, no blink logic, no business decisions.
 * Service level calls this; this never calls up into service or app level.
 */

#ifndef MY_INDECTORS_DRIVER_H_
#define MY_INDECTORS_DRIVER_H_

#include <stdint.h>
#include <stdbool.h>
#include "pin_mux.h"
#define LED_DRIVER_NUM_LEDS   3

void led_driver_init(void);
void led_driver_write(uint8_t index, bool on);


#endif /* MY_INDECTORS_DRIVER_H_ */
