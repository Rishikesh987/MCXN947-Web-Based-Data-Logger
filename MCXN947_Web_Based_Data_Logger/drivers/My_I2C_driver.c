/*
 * My_I2C_driver.c
 *
 *  Created on: Aug 29, 2026
 *      Author: rishi
 */



#include <stdio.h>
#include "My_I2C_driver.h"
#include "fsl_debug_console.h"

void I2C_ScanBus(void)
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

        status_t status = LPI2C_MasterTransferBlocking(EXAMPLE_I2C_MASTER, &xfer);

        if (status == kStatus_Success)
        {
            PRINTF("  Device found at 0x%02X\r\n", addr);
            foundCount++;
        }
    }

    PRINTF("Scan complete. %d device(s) found.\r\n", foundCount);
}
