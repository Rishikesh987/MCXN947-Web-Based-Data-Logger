/*
 * My_I2C_eeprom.h
 *
 *  Created on: Sep 13, 2026
 *      Author: rishi
 *
 * Driver header for the AT24C32 (32Kb) external I2C EEPROM.
 */

#ifndef MY_I2C_EEPROM_H_
#define MY_I2C_EEPROM_H_

#include "fsl_lpi2c.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define EEPROM_I2C_ADDR         0x50U  /* Base hardware I2C address for AT24C32 */
#define EEPROM_TOTAL_BYTES      4096U  /* 32 Kilobits = 4 Kilobytes */
#define EEPROM_PAGE_SIZE        32U    /* Physical write page structure bound */

typedef enum {
    kEeprom_Ok = 0,
    kEeprom_I2cError,
    kEeprom_InvalidAddress,
    kEeprom_LengthOverflow
} eeprom_status_t;

/*******************************************************************************
 * Public APIs
 ******************************************************************************/


eeprom_status_t EEPROM_Init(LPI2C_Type *base);
/**
 * @brief  Writes an arbitrary block of bytes to the EEPROM across page boundaries.
 * @note   This function breaks down large buffers into safe 32-byte hardware page blocks
 *         and handles the required 5ms physical internal delay between iterations.
 */
eeprom_status_t EEPROM_Write_Bytes(LPI2C_Type *base, uint16_t address, uint8_t *data, uint16_t len);

/**
 * @brief  Reads an arbitrary block of bytes from any memory location sequentially.
 * @note   EEPROM reading is completely synchronous and has no page-boundary delay constraints.
 */
eeprom_status_t EEPROM_Read_Bytes(LPI2C_Type *base, uint16_t address, uint8_t *data, uint16_t len);

#endif /* MY_I2C_EEPROM_H_ */
