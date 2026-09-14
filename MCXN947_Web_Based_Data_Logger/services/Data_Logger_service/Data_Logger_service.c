/*
 * Data_Logger_service.c
 *
 *  Created on: Sep 13, 2026
 *      Author: rishi
 *
 * Implements a dual-partitioned ring-buffer memory subsystem layout to store
 * high-resolution 1-Hour (30s) and long-term 12-Hour (10m) metrics to a 32Kb EEPROM.
 */

#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include "fsl_debug_console.h"
#include "HealthMonitor_service.h"
#include "task_config.h"
#include "Data_Logger_service.h"
#include "My_I2C_driver.h"
#include "My_I2C_eeprom.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* Partition limits extracted from structural math constraints */
#define HIGH_RES_MAX_RECORDS     120  /* 120 records * 12 bytes = 1440 bytes (1 Hour @ 30s) */
#define LONG_TERM_MAX_RECORDS    72   /* 72 records * 12 bytes = 864 bytes (12 Hours @ 10m) */

/* Partition address bounds matching the 32-byte physical page boundaries */
#define EEPROM_HIGH_RES_START    0x0000
#define EEPROM_LONG_TERM_START   (HIGH_RES_MAX_RECORDS * LOG_RECORD_SIZE) /* Offset address: 1440 (0x05A0) */

/*******************************************************************************
 * Variables
 ******************************************************************************/
TaskHandle_t Logger_Handle = NULL;
static uint8_t Logger_TaskId;

/* Global data frame reference variable */
volatile log_record_t g_CurrentSensorMetrics;

/* Independent array write indices for physical memory tracking */
static uint16_t highResIndex = 0;
static uint16_t longTermIndex = 0;

/*******************************************************************************
 * Code
 ******************************************************************************/

/**
 * @brief Emulated hardware driver wrapper executing physical page writes over I2C.
 */


static status_t EEPROM_Write_Record(uint16_t address, log_record_t *record) {
    /* Pass processing over to the updated multi-page architecture driver */
    if (EEPROM_Write_Bytes(AT24C32_I2C_MASTER, address, (uint8_t*)record, LOG_RECORD_SIZE) == kEeprom_Ok) {
        return kStatus_Success;
    }
    return kStatus_Fail;
}


/**
 * @brief Central processing task thread tracking software time counters.
 */
static void Data_Logger_Task(void *pvParameters) {

    TickType_t xLastWakeTime;
	const TickType_t xPeriod = pdMS_TO_TICKS(PERIOD_DATA_LOGGER_TASK_MS);
	xLastWakeTime = xTaskGetTickCount();
    uint32_t receivedBits = 0;

    log_record_t localLogCopy;

    /* Software interval tracking variables */
    uint32_t highResTimerMs = 0;
    uint32_t longTermTimerMs = 0;


    PRINTF("[LOG]  Data Logger  Started\r\n");

    while (1) {
        /* Direct check-in signaling notification ticket swap with the HMS */
        if (xTaskNotifyWait(0x00, 0xFFFFFFFF, &receivedBits, 0) == pdPASS) {
            Reset_HeltMonit_service(Logger_TaskId);
        }

        /* Update internal software interval timers */
        highResTimerMs += PERIOD_DATA_LOGGER_TASK_MS;
        longTermTimerMs += PERIOD_DATA_LOGGER_TASK_MS;

        /* ---------------------------------------------------------------------
         * PARTITION 1: 1-Hour High-Resolution Window (Runs every 30,000ms / 30s)
         * --------------------------------------------------------------------- */
        if (highResTimerMs >= 30000U) {
            highResTimerMs = 0;

            /* Extract thread-safe snapshot context block */
            vPortEnterCritical();
            memcpy(&localLogCopy, (void*)&g_CurrentSensorMetrics, LOG_RECORD_SIZE);
            vPortExitCritical();

            uint16_t targetAddr = EEPROM_HIGH_RES_START + (highResIndex * LOG_RECORD_SIZE);

            if (EEPROM_Write_Record(targetAddr, &localLogCopy) == kStatus_Success) {
                highResIndex++;
                if (highResIndex >= HIGH_RES_MAX_RECORDS) {
                    highResIndex = 0; /* Ring buffer wrap-around */
                }
                /* Safe hardware rest margin meeting EEPROM physics requirements */
                vTaskDelay(pdMS_TO_TICKS(5));
            }
        }

        /* ---------------------------------------------------------------------
         * PARTITION 2: 12-Hour Long-Term Window (Runs every 600,000ms / 10 Mins)
         * --------------------------------------------------------------------- */
        if (longTermTimerMs >= 600000U) {
            longTermTimerMs = 0;

            vPortEnterCritical();
            memcpy(&localLogCopy, (void*)&g_CurrentSensorMetrics, LOG_RECORD_SIZE);
            vPortExitCritical();

            uint16_t targetAddr = EEPROM_LONG_TERM_START + (longTermIndex * LOG_RECORD_SIZE);

            if (EEPROM_Write_Record(targetAddr, &localLogCopy) == kStatus_Success) {
                longTermIndex++;
                if (longTermIndex >= LONG_TERM_MAX_RECORDS) {
                    longTermIndex = 0; /* Ring buffer wrap-around */
                }
                vTaskDelay(pdMS_TO_TICKS(5));
            }
        }

        /* Run every 50 ms */
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

void Init_Data_Logger_service(void) {

	EEPROM_Init(AT24C32_I2C_MASTER);

    /* Register service with the 600ms boundary limit macro from app_task_config.h */
    Logger_TaskId = Registor_service("Data_Logger", HMS_TIMEOUT_DATA_LOGGER_MS, true, &Logger_Handle);

    /* Allocate and kick off the processing task context */
    if (xTaskCreate(Data_Logger_Task, "Data_Logger_Task", STACK_DATA_LOGGER_TASK, NULL,
                    PRIO_DATA_LOGGER_TASK, &Logger_Handle) != pdPASS) {
        PRINTF("[RTOS][ERROR] Data Logger task creation FAILED!\r\n");
    } else {
        PRINTF("[RTOS][OK] Data Logger Task created successfully. \r\n");
    }
}
