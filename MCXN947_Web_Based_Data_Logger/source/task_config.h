/*
 * task_config.h
 *
 *  Created on: Sep 13, 2026
 *      Author: rishi
 *
 * Central configuration file for FreeRTOS Task Priorities, Stacks, 
 * execution rates, and Health Monitoring System (HMS) timeouts.
 */

#ifndef APP_TASK_CONFIG_H_
#define APP_TASK_CONFIG_H_

#include "FreeRTOS.h"

/*******************************************************************************
 *  FreeRTOS Task Priority Layout
 ******************************************************************************/
#define PRIO_MONITOR_TASK          (configMAX_PRIORITIES - 1)  /*  Highest */
#define PRIO_ANALOG_RTC_TASK       (configMAX_PRIORITIES - 2)  /*  High */
#define PRIO_DATA_LOGGER_TASK      (configMAX_PRIORITIES - 3)  /*  Medium */
#define PRIO_COMM_TASK             (configMAX_PRIORITIES - 4)  /*  Low-Medium */
                                                               /*  5 to 7 reserved */                         
#define PRIO_INDICATOR_TASK        (configMAX_PRIORITIES - 8)  /*  Lowest */

/*******************************************************************************
 *  Task Stack Size Configurations (Words)
 ******************************************************************************/
#define STACK_MONITOR_TASK         (256U)
#define STACK_ANALOG_RTC_TASK      (256U)  /* Analog/RTC task stack */
#define STACK_DATA_LOGGER_TASK     (512U)  /* Larger for EEPROM page/history filtering */
#define STACK_COMM_TASK            (3072U) /* Heavy stack space required for lwIP & HTTP packets */
#define STACK_INDICATOR_TASK       (256U)  /* Increased from 128 to safely support PRINTF text layout */

/*******************************************************************************
 *  Task Run Periods (How often the loops execute)
 ******************************************************************************/
#define PERIOD_MONITOR_TASK_MS     (10U)   /* Monitor loop checks statuses every 10ms */
#define PERIOD_ANALOG_RTC_TASK_MS  (100U)  /* Sensors sample every 100ms */
#define PERIOD_DATA_LOGGER_TASK_MS (50U)  /* Historical logging evaluated every 200ms */
#define PERIOD_COMM_TASK_MS        (100U)   /* Network stack loops tightly to capture packets */
#define PERIOD_INDICATOR_TASK_MS   (10U)   /* LED matrix processing tick baseline */

/*******************************************************************************
 *  HMS Watchdog Timeout Constraints (In Milliseconds)
 *
 * Rule: Timeout must be at least 2x to 3x the run period to handle temporary
 * processing delays or high-priority task preemption safely.
 ******************************************************************************/
#define HMS_TIMEOUT_ANALOG_RTC_MS  (300U)  /* 3x run period: allows 2 missed loops before reset */
#define HMS_TIMEOUT_DATA_LOGGER_MS (600U)  /* 3x run period: absorbs long I2C EEPROM write delays */
#define HMS_TIMEOUT_COMM_MS        (1000U) /* 1 Second: protects against heavy TCP server blocking spikes */
#define HMS_TIMEOUT_INDICATOR_MS   (200U)  /* Safe window: completely avoids false resets */

#endif /* APP_TASK_CONFIG_H_ */
