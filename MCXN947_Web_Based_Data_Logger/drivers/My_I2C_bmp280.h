/*
 * bmp280.h
 *
 *  Created on: Aug 23, 2026
 *      Author: rishi
 */

#ifndef BMP280_H_
#define BMP280_H_

#include <stdint.h>
#include <stdbool.h>
#include "fsl_lpi2c.h"

/* I2C address: SDO -> GND = 0x76, SDO -> VCC = 0x77 */
#define BMP280_I2C_ADDR      0x76U

/* Chip ID values */
#define BMP280_CHIP_ID       0x58U
#define BME280_CHIP_ID       0x60U

/* Register addresses */
#define BMP280_REG_CHIP_ID   0xD0U
#define BMP280_REG_RESET     0xE0U
#define BMP280_REG_CTRL_HUM  0xF2U   /* BME280 only */
#define BMP280_REG_STATUS    0xF3U
#define BMP280_REG_CTRL_MEAS 0xF4U
#define BMP280_REG_CONFIG    0xF5U
#define BMP280_REG_PRESS_MSB 0xF7U
#define BMP280_REG_CALIB00   0x88U   /* start of calibration block */

typedef struct
{
    /* Temperature calibration */
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    /* Pressure calibration */
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
    /* Humidity calibration (BME280 only) */
    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4;
    int16_t  dig_H5;
    int8_t   dig_H6;

    int32_t  t_fine;      /* shared compensation value */
    bool     isBME280;    /* true if humidity sensor present */
} bmp280_handle_t;

/* Result codes */
typedef enum
{
    kBmp280_Ok = 0,
    kBmp280_I2cError,
    kBmp280_UnknownChipId
} bmp280_status_t;

/*!
 * @brief Probes the sensor, reads chip ID, and loads calibration data.
 * @param base LPI2C peripheral base (e.g. LPI2C1)
 * @param handle Pointer to handle struct to fill in
 */
bmp280_status_t BMP280_Init(LPI2C_Type *base, bmp280_handle_t *handle);

/*!
 * @brief Triggers one forced-mode measurement and reads back compensated
 *        temperature (deg C) and pressure (hPa). Blocks until data is ready.
 */
bmp280_status_t BMP280_ReadTempPressure(LPI2C_Type *base, bmp280_handle_t *handle,
                                         float *temperatureC, float *pressureHpa);

/*!
 * @brief Reads compensated humidity (%RH). Only valid if handle->isBME280 == true.
 *        Call BMP280_ReadTempPressure() first in the same cycle (needs t_fine).
 */
bmp280_status_t BMP280_ReadHumidity(LPI2C_Type *base, bmp280_handle_t *handle,
                                     float *humidityPercent);

#endif /* BMP280_H_ */
