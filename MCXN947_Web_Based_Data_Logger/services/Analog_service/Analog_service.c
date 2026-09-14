/*
 * Analog_service.c
 *
 *  Created on: Aug 29, 2026
 *      Author: rishi
 */
#include "Analog_service.h"

#include <stdio.h>
#include "fsl_debug_console.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pin_mux.h"
#include "My_I2C_driver.h"
#include "HealthMonitor_service.h"
#include "task_config.h"

TaskHandle_t Analog_Handle = NULL; /* Analog task handle */
static uint8_t Analog_TaskId;

bmp280_handle_t sensor;
float temp, pressure, humidity;
rtc_time_t t;

uint32_t DelayT = 0;

static void Analog_TASK() {
	TickType_t xLastWakeTime;
	const TickType_t xPeriod = pdMS_TO_TICKS(PERIOD_ANALOG_RTC_TASK_MS);
	xLastWakeTime = xTaskGetTickCount();
	uint32_t receivedBits = 0;

	PRINTF("I2C TASK running\r\n");

	while (1) {

		if (xTaskNotifyWait(0x00, 0xFFFFFFFF, &receivedBits,0) == pdPASS) {
			Reset_HeltMonit_service(Analog_TaskId); /* Reset the health monitor service for this task */
		}
		/*Read temperature and pressure*/
		if (BMP280_ReadTempPressure(BMP280_I2C_MASTER, &sensor, &temp,
				&pressure) != kBmp280_Ok) {
			PRINTF("[Analog][ERROR] BMP280_ReadTempPressure\r\n");
		}

		/*Read time from RTC*/
		if (DS3231_ReadTime(DS3231_I2C_MASTER, &t) != kStatus_Success) {
			PRINTF("[Analog][ERROR] DS3231_ReadTime\r\n");
		}

		/* Run every 100 ms */
		vTaskDelayUntil(&xLastWakeTime, xPeriod);
	}

}

void Init_Analog_service() {

	/*  Temperature initialization  BMP280  */
	if (BMP280_Init(BMP280_I2C_MASTER, &sensor) != kBmp280_Ok) {
		PRINTF("[Analog][ERROR] BMP280/BME280 init failed - check wiring/address\r\n");
	} else {
		PRINTF("[Analog][OK] BMP280/BME280 init successful\r\n");
	}

	/*RTC initialization DS3121*/
	if (RTC_SetFromCompileTime(DS3231_I2C_MASTER) != kStatus_Success) {
		PRINTF("[Analog][ERROR] RTC DS3231 initialization failed\r\n");
	} else {
		PRINTF("[Analog][OK] RTC DS3231 initialization successful\r\n");
	}

	/* Register analog task with health monitor - 300ms timeout */
	Analog_TaskId = Registor_service("Analog_TASK", HMS_TIMEOUT_ANALOG_RTC_MS, true, &Analog_Handle);

	/* Create analog data collection task */
	if (xTaskCreate(Analog_TASK, "Analog_TASK", STACK_ANALOG_RTC_TASK, NULL,
			PRIO_ANALOG_RTC_TASK, &Analog_Handle) != pdPASS) {
		PRINTF("[RTOS][ERROR] Analog task creation FAILED!\r\n");

	} else {
		PRINTF("[RTOS][OK] Analog task created successfully\r\n");

	}

}
