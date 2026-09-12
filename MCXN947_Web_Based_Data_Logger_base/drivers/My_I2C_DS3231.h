/*
 * My_I2C_DS3231.h
 *
 *  Created on: Aug 29, 2026
 *      Author: rishi
 */

#ifndef MY_I2C_DS3231_H_
#define MY_I2C_DS3231_H_
#include "fsl_lpi2c.h"


typedef struct {
    uint8_t sec, min, hour, day, date, month, year; // year = 00-99 (add 2000)
} rtc_time_t;


status_t RTC_SetFromCompileTime(LPI2C_Type *base);
status_t DS3231_ReadTime(LPI2C_Type *base,rtc_time_t *t);
#endif /* MY_I2C_DS3231_H_ */
