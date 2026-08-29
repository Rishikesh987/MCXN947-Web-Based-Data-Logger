/*
 * I2C_service.c
 *
 *  Created on: Aug 29, 2026
 *      Author: rishi
 */
#include <stdio.h>
#include "fsl_debug_console.h"
#include "FreeRTOS.h"
#include "task.h"
#include "I2C_service.h"
#include "My_I2C_bmp280.h"
#include "My_I2C_driver.h"
#include "pin_mux.h"

#define TASK_2_PRIORITY (configMAX_PRIORITIES - 2)

/* Stack size allocated for the temperature task.*/
#define TASK_2_STACK_SIZE  200



bmp280_handle_t sensor;
float temp, pressure, humidity;




static void I2C_TASK()
{
    TickType_t xLastWakeTime;
    const TickType_t xPeriod = pdMS_TO_TICKS(100);

    xLastWakeTime = xTaskGetTickCount();

    PRINTF("I2C TASK running\r\n");
    while (1)
    {
    /* I2C_TASK code */
    if (BMP280_ReadTempPressure(EXAMPLE_I2C_MASTER, &sensor, &temp, &pressure) != kBmp280_Ok) {
        PRINTF("[I2C][ERROR] BMP280_ReadTempPressure");
    }

        /* Run every 10 ms */
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }

}



void Init_I2C_service(){

    lpi2c_master_config_t masterConfig;

    LPI2C2_InitPins();
    /*
     * masterConfig.debugEnable = false;
     * masterConfig.ignoreAck = false;
     * masterConfig.pinConfig = kLPI2C_2PinOpenDrain;
     * masterConfig.baudRate_Hz = 100000U;
     * masterConfig.busIdleTimeout_ns = 0;
     * masterConfig.pinLowTimeout_ns = 0;
     * masterConfig.sdaGlitchFilterWidth_ns = 0;
     * masterConfig.sclGlitchFilterWidth_ns = 0;
     */
    LPI2C_MasterGetDefaultConfig(&masterConfig);
    /* Change the default baudrate configuration */
    masterConfig.baudRate_Hz = LPI2C_BAUDRATE;
    /* Initialize the LPI2C master peripheral */
    LPI2C_MasterInit(EXAMPLE_I2C_MASTER, &masterConfig, LPI2C_MASTER_CLOCK_FREQUENCY);


    /* after your existing LPI2C_MasterInit() call */
    if (BMP280_Init(EXAMPLE_I2C_MASTER, &sensor) != kBmp280_Ok) {
        PRINTF("[I2C][ERROR] BMP280/BME280 init failed - check wiring/address\r\n");
    }

    PRINTF(sensor.isBME280 ? "[I2C][OK]BME280 detected\r\n" : "[I2C][OK]BMP280 detected\r\n");


    if (xTaskCreate(I2C_TASK, "I2C_TASK", TASK_2_STACK_SIZE, NULL, TASK_2_PRIORITY, NULL) !=
            pdPASS)
        {
        	PRINTF("[RTOS][ERROR] I2C task creation FAILED!\r\n");

        }
        else
        {
        	PRINTF("[RTOS][OK] I2C task created successfully\r\n");
        }

}
