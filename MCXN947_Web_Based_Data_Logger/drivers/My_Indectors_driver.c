/*
 * Indectors_driver.c
 *
 *  Created on: Sep 6, 2026
 *      Author: rishi
 */

#include <My_Indectors_driver.h>
#include "fsl_gpio_cmsis.h"

#ifndef LOGIC_LED_ON
#define LOGIC_LED_ON   1U
#endif
#ifndef LOGIC_LED_OFF
#define LOGIC_LED_OFF  0U
#endif

extern ARM_DRIVER_GPIO Driver_GPIO_PORT0;
/* extern ARM_DRIVER_GPIO Driver_GPIO_PORT1;   // add if your LEDs span more ports */

typedef struct {
    ARM_DRIVER_GPIO *driver;
    uint8_t pin;
} led_pin_t;

/* EDIT: real port/pin for each of your 11 LEDs */
static const led_pin_t led_pins[LED_DRIVER_NUM_LEDS] = {
    { &Driver_GPIO_PORT0, 10  }, // red
    { &Driver_GPIO_PORT0, 27  }, // green
    { &Driver_GPIO_PORT1, 2  },  // blue

};

void led_driver_init(void)
{
    for (uint8_t i = 0; i < LED_DRIVER_NUM_LEDS; i++)
    {
        led_pins[i].driver->SetOutput(led_pins[i].pin, LOGIC_LED_OFF);
    }
}

void led_driver_write(uint8_t index, bool on)
{
    if (index >= LED_DRIVER_NUM_LEDS)
    	return;
    led_pins[index].driver->SetOutput(led_pins[index].pin, on ? LOGIC_LED_ON : LOGIC_LED_OFF);
}
