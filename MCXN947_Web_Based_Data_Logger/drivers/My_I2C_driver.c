/*
 * My_I2C_driver.c
 *
 *  Created on: Aug 29, 2026
 *      Author: rishi
 */



#include <stdio.h>
#include "My_I2C_driver.h"
#include "fsl_debug_console.h"
#include "pin_mux.h"

/*******************************************************************************
 * Variables
 ******************************************************************************/
/* Global states ensuring one-time initialization configuration */
static volatile bool isI2cInitialized = false;


/*******************************************************************************
 * Code
 ******************************************************************************/

void I2C_Bus_Init(LPI2C_Type *base)
{


    if (isI2cInitialized)
    {

        return; /* Already initialized by another module, exit safely */
    }
    isI2cInitialized = true;


    PRINTF("[I2C] Performing one-time LPI2C2 hardware bus initialization...\r\n");

    lpi2c_master_config_t masterConfig;

    /* Call board pin configs (ensure this is safe to run once) */
    LPI2C2_InitPins();

    LPI2C_MasterGetDefaultConfig(&masterConfig);
    masterConfig.baudRate_Hz = LPI2C_BAUDRATE;

    LPI2C_MasterInit(base, &masterConfig, LPI2C_MASTER_CLOCK_FREQUENCY);
}


void I2C_ScanBus(LPI2C_Type *base)
{
    PRINTF("Scanning I2C bus...\r\n");
    uint8_t foundCount = 0;

    for (uint8_t addr = 0x03; addr <= 0x77; addr++)
    {
        lpi2c_master_transfer_t xfer = {0};
        xfer.slaveAddress   = addr;
        xfer.direction      = kLPI2C_Write;
        xfer.subaddressSize = 0;
        xfer.data           = NULL;
        xfer.dataSize       = 0;
        xfer.flags          = kLPI2C_TransferDefaultFlag;

        status_t status = LPI2C_MasterTransferBlocking(base, &xfer);

        if (status == kStatus_Success)
        {
            PRINTF("  Device found at 0x%02X\r\n", addr);
            foundCount++;
        }
    }

    PRINTF("Scan complete. %d device(s) found.\r\n", foundCount);
}
