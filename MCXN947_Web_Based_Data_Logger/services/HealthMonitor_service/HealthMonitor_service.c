/*
 * HealthMonitor_service.c
 *
 *  Created on: Sep 12, 2026
 *      Author: rishi
 *
 * Monitors system health and manages watchdog operation.
 */

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "HealthMonitor_service.h"
#include <stdio.h>
#include <string.h>
#include "fsl_debug_console.h"
#include "task_config.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
wwdt_config_t config;
uint32_t wdtFreq;
bool timeOutResetEnable;
static uint8_t serviceCount;
static uint8_t HeltMonit_timercount[MAX_SERVICE_COUNT];
HealthMonitor_Reg HeltMonit_Reg[MAX_SERVICE_COUNT];

/*******************************************************************************
 * Code
 ******************************************************************************/
uint8_t Registor_service(char *taskname, uint32_t timeout_value, bool restflag,
		TaskHandle_t *taskHandle) {
	if (serviceCount >= MAX_SERVICE_COUNT) /* Check if maximum service count is reached */
	{
		PRINTF("[HMS][ERROR] number of service reached \r\n");
		while (1)
			;
	}

	HeltMonit_Reg[serviceCount].task_id = serviceCount; /* Assign task ID */

	strncpy(HeltMonit_Reg[serviceCount].taskname, taskname,
			sizeof(HeltMonit_Reg[serviceCount].taskname) - 1); /* Copy task name */
	HeltMonit_Reg[serviceCount].taskname[sizeof(HeltMonit_Reg[serviceCount].taskname)
			- 1] = '\0'; /* null-termination. */

	HeltMonit_Reg[serviceCount].timeout_value = timeout_value; /* Set timeout value */
	HeltMonit_Reg[serviceCount].restflag = restflag; /* Set reset flag */
	HeltMonit_Reg[serviceCount].taskHandle = taskHandle; /* Set task handle */

	return serviceCount++; /* Return the task ID of the registered service */
}

void Reset_HeltMonit_service(uint8_t task_id) {

	if (task_id < serviceCount) /* Check if task ID is valid */
	{
		HeltMonit_Reg[task_id].restflag = true; /* Reset the reset flag for the specified task */
	}

}

/*******************************************************************************
 *  watchdog interrupt handler : rest the reset flag for all registered tasks
 ******************************************************************************/

void HealthMonitor_Handler(void) {
	uint32_t wdtStatus = WWDT_GetStatusFlags(WWDT);

	/* The chip will reset before this happens */
	if (wdtStatus & kWWDT_TimeoutFlag) {
		WWDT_ClearStatusFlags(WWDT, kWWDT_TimeoutFlag);
	}

	/* Handle warning interrupt */
	if (wdtStatus & kWWDT_WarningFlag) {
		/* A watchdog feed didn't occur prior to warning timeout */
		WWDT_ClearStatusFlags(WWDT, kWWDT_WarningFlag);
		/* before timeout reset backup the ram data or ram log to flash.
		 */
	}

#if (defined(LPC55S36_WORKAROUND) && LPC55S36_WORKAROUND)
    /* Set PMC register value that could run with GDET enable */
    PMC->LDOPMU   = 0x0109CF18;
    PMC->DCDC0    = 0x010A767E;
    PMC->LDOCORE0 = 0x2801006B;
#endif
	SDK_ISR_EXIT_BARRIER;
}

static void HealthMonitor_service(void *pvParameters) {
	TickType_t xLastWakeTime;
	const TickType_t xPeriod = pdMS_TO_TICKS(PERIOD_MONITOR_TASK_MS); // Check loop runs every 10ms
	xLastWakeTime = xTaskGetTickCount();

	PRINTF("[HMS] Health Monitor Task Running...\r\n");

	/* Initialize all tracking timers to 0 explicitly at startup */
	memset(HeltMonit_timercount, 0, sizeof(HeltMonit_timercount));

	while (1) {
		bool systemHealthy = true;

		/* Send dynamic notification signals to wake up application tasks */
		for (uint8_t i = 0; i < serviceCount; i++) {
			if ((HeltMonit_Reg[i].taskHandle != NULL)
					&& (*HeltMonit_Reg[i].taskHandle != NULL)) {
				xTaskNotify(*HeltMonit_Reg[i].taskHandle, 0x00, eNoAction);
			}
		}

		/* Process check-in statuses and increment software timers */
		for (uint8_t i = 0; i < serviceCount; i++) {

			if (HeltMonit_Reg[i].restflag) {
				/* Task checked in! Safely clear flags and reset its missed-time tracker to 0ms */
				HeltMonit_Reg[i].restflag = false;
				HeltMonit_timercount[i] = 0;
			} else {

				HeltMonit_timercount[i] += 10; // Add 10ms to its individual counter
			}

			if (HeltMonit_timercount[i] >= HeltMonit_Reg[i].timeout_value) {
				systemHealthy = false;
				PRINTF(
						"[HMS][ALERT] Task '%s' froze! Deadlock time: %d ms / Allowed: %d ms\r\n",
						HeltMonit_Reg[i].taskname,
						(int) HeltMonit_timercount[i],
						(int) HeltMonit_Reg[i].timeout_value);
			}
		}

		if (systemHealthy) {

			WWDT_Refresh(WWDT);
		} else {
			/* CRITICAL*/
		}

		vTaskDelayUntil(&xLastWakeTime, xPeriod);
	}
}

void Init_HealthMonitor_service() {
	WWDTclk_Init();
	timeOutResetEnable = true;

	/* Check if reset is due to Watchdog */
	if (IS_WWDT_RESET) {
		PRINTF("[WWDT][ERROR]Watchdog reset occurred !!!\r\n");
		timeOutResetEnable = false;
		__BKPT(0);
	}
	/* The WDT divides the input frequency into it by 4 */
	wdtFreq = WDT_CLK_FREQ / 4;

	WWDT_GetDefaultConfig(&config);

	/*Calculate how many clock ticks happen in exactly 1 millisecond */
	uint32_t ticksPerMs = wdtFreq / 1000U;

	config.timeoutValue = ticksPerMs * 100U; /* 100ms timeout */
	/* Set your warning threshold.
	 * 512 ticks provides ~2 ms of emergency execution time */
	config.warningValue = 512;
	/* Disable the window restriction completely */
	config.windowValue = 0xFFFFFFU;
	/* Configure WWDT to reset on timeout */
	config.enableWatchdogReset = true;
	/* Setup watchdog clock frequency(Hz). */
	config.clockFreq_Hz = WDT_CLK_FREQ;
	WWDT_Init(WWDT, &config);

	NVIC_EnableIRQ(HealthMonitor_WDT_IRQn);

	/*create I2C task*/
	if (xTaskCreate(HealthMonitor_service, "HealthMonitor_service",
			STACK_MONITOR_TASK, NULL, PRIO_MONITOR_TASK, NULL) != pdPASS) {
		PRINTF("[RTOS][ERROR] HealthMonitor_service creation FAILED!\r\n");
	} else {
		PRINTF("[RTOS][OK] HealthMonitor_service created successfully\r\n");
	}
}
