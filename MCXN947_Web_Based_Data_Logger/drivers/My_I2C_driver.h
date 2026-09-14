/*
 * My_I2C_driver.h
 *
 *  Created on: Aug 29, 2026
 *      Author: rishi
 */

#ifndef MY_I2C_DRIVER_H_
#define MY_I2C_DRIVER_H_


#include "fsl_lpi2c.h"



#define BMP280_I2C_MASTER ((LPI2C_Type *)LPI2C2_BASE)    /* Temperature sensor */
#define DS3231_I2C_MASTER ((LPI2C_Type *)LPI2C2_BASE)    /* RTC*/
#define AT24C32_I2C_MASTER ((LPI2C_Type *)LPI2C2_BASE)   /* EEPROM */



#define LPI2C_MASTER_CLOCK_FREQUENCY CLOCK_GetLPFlexCommClkFreq(2u)

#define WAIT_TIME 10U

/* tempreture sensor */
#define LPI2C_MASTER_SLAVE_ADDR_7BIT 0x7EU
#define LPI2C_BAUDRATE               100000U
#define LPI2C_DATA_LENGTH            33U

void I2C_Bus_Init(LPI2C_Type *base);

#endif /* MY_I2C_DRIVER_H_ */
