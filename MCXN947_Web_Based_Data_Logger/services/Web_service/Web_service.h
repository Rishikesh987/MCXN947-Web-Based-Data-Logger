/*
 * Web_service.h
 *
 *  Created on: Sep 25, 2026
 *      Author: rishi
 *
 *  Description:
 *  ------------
 *  Public interface for the HTTP Web Service layer.
 *
 *  This service exposes a lightweight REST API over lwIP raw sockets:
 *    GET  /api/sensor   -> JSON: { "temp": x.x, "pressure": x.x, "timestamp": "HH:MM:SS" }
 *    POST /api/control  -> Body: { "command": "on" | "off" }
 *
 *  All other paths fall through to the NXP lwIP httpserver (fsdata.c pages).
 *
 *  Usage:
 *    1. Call WebService_Init()             once before vTaskStartScheduler().
 *    2. Call WebService_UpdateSensorData() periodically from the app task.
 *    3. Read g_web_control_active          from any task to check the ON/OFF state.
 */

#ifndef WEB_SERVICE_WEB_SERVICE_H_
#define WEB_SERVICE_WEB_SERVICE_H_

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "semphr.h"

/*******************************************************************************
 * Dashboard JSON Data Structure
 *
 * Mirrors the three fields the browser dashboard expects:
 *   { "temp": <float>, "pressure": <float>, "timestamp": "<string>" }
 ******************************************************************************/
typedef struct
{
    float    temp;                  /* Temperature reading from BMP280, degrees C */
    float    pressure;              /* Pressure reading from BMP280, hPa           */
    char     timestamp[24];         /* RTC timestamp string: "20YY-MM-DD HH:MM:SS"  */
} WebSensorData_t;

/*******************************************************************************
 * Global Control Flag
 *
 * Written by the HTTP handler task when the browser POSTs "on" or "off".
 * Read by the app/indicator task to drive physical outputs.
 *
 * Value: 1 = system ON command received
 *        0 = system OFF command received (default)
 ******************************************************************************/
extern volatile uint8_t g_web_control_active;

/*******************************************************************************
 * Public API
 ******************************************************************************/

/**
 * @brief  Initialize and launch the HTTP handler FreeRTOS task.
 *
 * Opens a TCP listener socket on HTTP_SERVER_PORT (default 80), registers
 * the task with the Health Monitor, and starts the http_handler_task loop.
 *
 * Call once from main() after Init_ETH_service(), before vTaskStartScheduler().
 */
void WebService_Init(void);

/**
 * @brief  Update the sensor payload that the next GET /api/sensor will serve.
 *
 * This function is thread-safe: it acquires the internal mutex before
 * writing. Call it from the Temperature app task every sample period.
 *
 * @param temp   Temperature in degrees Celsius (from Analog_service extern).
 * @param press  Atmospheric pressure in hPa    (from Analog_service extern).
 */
void WebService_UpdateSensorData(float temp, float press);

#endif /* WEB_SERVICE_WEB_SERVICE_H_ */
