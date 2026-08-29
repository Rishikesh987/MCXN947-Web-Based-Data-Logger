/*
 * I2C_service.h
 *
 *  Created on: Aug 29, 2026
 *      Author: rishi
 */

#ifndef I2C_SERVICE_I2C_SERVICE_H_
#define I2C_SERVICE_I2C_SERVICE_H_

void Init_I2C_service();

static void I2C_TASK();
extern float temp, pressure, humidity;
#endif /* I2C_SERVICE_I2C_SERVICE_H_ */
