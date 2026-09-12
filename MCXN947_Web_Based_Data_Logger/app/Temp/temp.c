
/*
 * temp.c
 *
 *  Created on: Aug 29, 2026
 *      Author: rishi
 *
 *  Description:
 *  ------------
 *  Temperature application task.
 *
 *  This module creates a FreeRTOS task that executes periodically
 *  every 10 ms. Currently, the task prints a debug message to
 *  verify that the periodic task is running correctly.
 */

#include <stdio.h>
#include "fsl_debug_console.h"
#include "FreeRTOS.h"
#include "task.h"
#include "HealthMonitor_service.h"
#include "../../services/Analog_service/Analog_service.h"
/*******************************************************************************
 * Task Configuration
 ******************************************************************************/

/* application priority.*/
#define TASK_1_PRIORITY (configMAX_PRIORITIES - 1)

/* Stack size allocated for the temperature task.*/
#define TASK_1_STACK_SIZE  200

TaskHandle_t App_Handle = NULL; /* Application task handle */
static uint8_t App_TaskId;
/*******************************************************************************
 * Temperature Task
 ******************************************************************************/

/*
 * @brief Temperature application task.
 *
 * This task runs periodically with a fixed period of 10 ms.
 *
 * vTaskDelayUntil() is used to maintain a stable periodic execution
 * time regardless of small variations in task execution time.
 */
static void Temperature_TASK()
{
    TickType_t xLastWakeTime;
    const TickType_t xPeriod = pdMS_TO_TICKS(100);
    uint32_t receivedBits = 0;

    xLastWakeTime = xTaskGetTickCount();

    PRINTF("Temperature task running \r\n");
    while (1)
    {
        if (xTaskNotifyWait(0x00, 0xFFFFFFFF, &receivedBits, 0) == pdPASS) {
            Reset_HeltMonit_service(App_TaskId); /* Reset the health monitor service for this task */
        }
        /* Task1 code */
    	PRINTF("Time: 20%02d-%02d-%02d %02d:%02d:%02d\r\n",
    	               t.year, t.month, t.date, t.hour, t.min, t.sec);
    	if(temp <= 2500){
    		PRINTF("Temperature ok\r\n");
    	    PRINTF("Temp: %d C   Pressure: %d hPa\r\n", (uint32_t)temp*100, (uint32_t)pressure);
    	}
    	else{
    		PRINTF("Temperature not ok \r\n");
    	}
        /* Run every 1000 ms */
        vTaskDelayUntil(&xLastWakeTime, xPeriod);

    }

}

/*******************************************************************************
 * Temperature Application Initialization
 ******************************************************************************/

/*
 * @brief Create the temperature application task.
 *
 * This function should be called before starting the FreeRTOS
 * scheduler.
 */

void Init_Temperature_TASK(){

	PRINTF("[RTOS] Creating Temperature Application Task...\r\n");

	/* Register application task with health monitor - 300ms timeout */
	App_TaskId = Registor_service("Temperature_TASK", 300, true, &App_Handle);

    if (xTaskCreate(Temperature_TASK, "Temperature_TASK", TASK_1_STACK_SIZE, NULL, TASK_1_PRIORITY, &App_Handle) !=
        pdPASS)
    {
    	PRINTF("[RTOS][ERROR] Temperature task creation FAILED!\r\n");

    }
    else
    {
    	PRINTF("[RTOS][OK] Temperature task created successfully\r\n");
    }

}
