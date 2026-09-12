/*
 * HealthMonitor_service.h
 *
 *  Created on: Sep 12, 2026
 *      Author: rishi
 */

#ifndef HEALTHMONITOR_SERVICE_HEALTHMONITOR_SERVICE_H_
#define HEALTHMONITOR_SERVICE_HEALTHMONITOR_SERVICE_H_

#include "pin_mux.h"
#include "fsl_wwdt.h"
#include "FreeRTOS.h"
#include "task.h"

#define MAX_SERVICE_COUNT 10u

typedef struct {
	char taskname[20];
	uint8_t task_id;
	uint32_t timeout_value;
	bool restflag;
	TaskHandle_t *taskHandle;
} HealthMonitor_Reg;

/*******************************************************************************
 * Macro Definitions
 ******************************************************************************/

#define WWDT                WWDT0
#define HealthMonitor_WDT_IRQn        WWDT0_IRQn
#define HealthMonitor_Handler WWDT0_IRQHandler
#define WDT_CLK_FREQ        CLOCK_GetWdtClkFreq(0)
#define IS_WWDT_RESET       (0 != (CMC0->SRS & CMC_SRS_WWDT0_MASK))

/*******************************************************************************
 * function Prototypes
 ******************************************************************************/

void Init_HealthMonitor_service();
uint8_t Registor_service(char *taskname, uint32_t timeout_value, bool restflag,
		TaskHandle_t *taskHandle);
void Reset_HeltMonit_service(uint8_t task_id);
#endif /* HEALTHMONITOR_SERVICE_HEALTHMONITOR_SERVICE_H_ */
