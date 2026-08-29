/*
 * Copyright 2016-2026 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file    MCXN947_Web_Based_Data_Logger.c
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "task.h"
#include "temp.h"
#include "I2C_service.h"
/* TODO: insert other include files here. */

/* TODO: insert other definitions and declarations here. */


/*
 * @brief   Application entry point.
 */
int main(void) {

    /* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    /* Init FSL debug console. */
    BOARD_InitDebugConsole();
#endif

    PRINTF("\r\n");
	PRINTF("========================================\r\n");
	PRINTF("     MCXN947 WEB-BASED DATA LOGGER\r\n");
	PRINTF("========================================\r\n");

	PRINTF("[BOOT] Board initialization started\r\n");

	Init_I2C_service();

	PRINTF("[APP ] Application initialization started\r\n");

	Init_Temperature_TASK();

    PRINTF("[RTOS] Starting FreeRTOS scheduler...\r\n");
    PRINTF("========================================\r\n");
    vTaskStartScheduler();
    for (;;);
}
