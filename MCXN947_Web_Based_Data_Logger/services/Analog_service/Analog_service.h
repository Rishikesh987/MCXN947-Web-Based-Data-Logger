/*
 * I2C_service.h
 *
 *  Created on: Aug 29, 2026
 *      Author: rishi
 */

#ifndef ANALOG_SERVICE_ANALOG_SERVICE_H_
#define ANALOG_SERVICE_ANALOG_SERVICE_H_
#include "My_I2C_bmp280.h"
#include "My_I2C_DS3231.h"
void Init_Analog_service();


extern float temp, pressure, humidity;


extern rtc_time_t t;
#endif /* ANALOG_SERVICE_ANALOG_SERVICE_H_ */
