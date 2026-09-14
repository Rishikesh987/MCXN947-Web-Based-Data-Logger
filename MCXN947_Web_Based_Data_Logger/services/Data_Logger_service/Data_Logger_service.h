/*
 * Data_Logger_service.h
 *
 *  Created on: Sep 13, 2026
 *      Author: rishi
 *
 * Header file for the Dual-Partition External EEPROM Logging Service.
 */

#ifndef DATA_LOGGER_SERVICE_DATA_LOGGER_SERVICE_H_
#define DATA_LOGGER_SERVICE_DATA_LOGGER_SERVICE_H_


#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define LOG_RECORD_SIZE           12U  /* Packed 12-byte binary log record size */

/* 📦 Dense packed record structure layout (Excludes compiler alignment holes) */
typedef struct __attribute__((packed)) {
    uint32_t timestamp;   /* 4 Bytes: Unix Epoch time from DS3231 RTC */
    int16_t  temperature; /* 2 Bytes: Temperature scaled by 100 (e.g., 2530 = 25.30 °C) */
    uint32_t pressure;    /* 4 Bytes: Pressure scaled by 10 (e.g., 10132 = 1013.2 hPa) */
    uint8_t  digital_in;  /* 1 Byte : Binary status tracking digital input pins */
    uint8_t  checksum;    /* 1 Byte : XOR integrity verification hash */
} log_record_t;

/*******************************************************************************
 * External Global Variables
 ******************************************************************************/
/* Global mailbox structure register updated continuously by your active sensor tasks */
extern volatile log_record_t g_CurrentSensorMetrics;

/*******************************************************************************
 * Public APIs
 ******************************************************************************/
/**
 * @brief  Initializes memory structures and registers the asynchronous logging task
 *         with the central System Health Monitor service.
 */
void Init_Data_Logger_service(void);



#endif /* DATA_LOGGER_SERVICE_DATA_LOGGER_SERVICE_H_ */
