/*
 * My_I2C_eeprom.c
 *
 *  Created on: Sep 13, 2026
 *      Author: rishi
 *
 * Multi-page sequential reading and writing abstraction engine built on the SDK's
 * LPI2C Master Transfer protocols.
 */
#include "My_I2C_driver.h"
#include "My_I2C_eeprom.h"

/* ... Keep all your existing functions: EEPROM_Write_Raw_Page, EEPROM_Write_Bytes, EEPROM_Read_Bytes ... */

eeprom_status_t EEPROM_Init(LPI2C_Type *base)
{

	I2C_Bus_Init(base);
    /* Note: If your I2C Bus peripheral initialization wrapper is distinct,
     * call it here (e.g., I2C_Bus_Init(base);) if not handled elsewhere. */

    uint8_t original_byte = 0;
    uint8_t test_byte = 0;
    uint8_t verify_byte = 0;
    const uint16_t test_address = 0x0000; /* Test boundary at address zero */

    /* Step 1: Read the existing data byte sitting at test location */
    if (EEPROM_Read_Bytes(base, test_address, &original_byte, 1) != kEeprom_Ok)
    {
        return kEeprom_I2cError;
    }

    /* Step 2: Formulate an inverted dummy token to guarantee cell transitions */
    test_byte = (uint8_t)((original_byte == 0xAA) ? 0x55 : 0xAA);

    /* Step 3: Write out the dummy token payload */
    if (EEPROM_Write_Bytes(base, test_address, &test_byte, 1) != kEeprom_Ok)
    {
        return kEeprom_I2cError;
    }

    /* Step 4: Retrieve data block to evaluate state-engine behavior */
    if (EEPROM_Read_Bytes(base, test_address, &verify_byte, 1) != kEeprom_Ok)
    {
        return kEeprom_I2cError;
    }

    /* Check for data corruption or bus failures */
    if (verify_byte != test_byte)
    {
        return kEeprom_I2cError;
    }

    /* Step 5: Restore initial sector register contents cleanly */
    if (EEPROM_Write_Bytes(base, test_address, &original_byte, 1) != kEeprom_Ok)
    {
        return kEeprom_I2cError;
    }

    return kEeprom_Ok;
}



/**
 * @brief  Writes a small block of bytes that strictly fits within a single physical page.
 */
static status_t EEPROM_Write_Raw_Page(LPI2C_Type *base, uint16_t address, uint8_t *data, uint8_t len)
{
    lpi2c_master_transfer_t xfer = {0};
    uint8_t addr_buf[2];

    /* AT24C32 uses a 2-byte subaddress layout (MSB first) */
    addr_buf[0] = (uint8_t)((address >> 8) & 0xFF);
    addr_buf[1] = (uint8_t)(address & 0xFF);

    xfer.slaveAddress   = EEPROM_I2C_ADDR;
    xfer.direction      = kLPI2C_Write;
    xfer.subaddress     = (uint32_t)((addr_buf[0] << 8) | addr_buf[1]);
    xfer.subaddressSize = 2; /* 2-Byte Subaddress requirement */
    xfer.data           = data;
    xfer.dataSize       = len;
    xfer.flags          = kLPI2C_TransferDefaultFlag;

    return LPI2C_MasterTransferBlocking(base, &xfer);
}

eeprom_status_t EEPROM_Write_Bytes(LPI2C_Type *base, uint16_t address, uint8_t *data, uint16_t len)
{
    if ((address + len) > EEPROM_TOTAL_BYTES)
    {
        return kEeprom_LengthOverflow;
    }

    uint16_t bytes_written = 0;

    while (bytes_written < len)
    {
        uint16_t current_addr = address + bytes_written;

        /* Calculate how many bytes are remaining within the current physical 32-byte page boundary */
        uint8_t page_offset = (uint8_t)(current_addr % EEPROM_PAGE_SIZE);
        uint8_t max_page_writable = EEPROM_PAGE_SIZE - page_offset;
        uint16_t remaining_bytes = len - bytes_written;

        /* Clamp write sizes to page boundaries */
        uint8_t chunk_size = (uint8_t)((remaining_bytes < max_page_writable) ? remaining_bytes : max_page_writable);

        /* Fire the hardware page transmission burst block */
        if (EEPROM_Write_Raw_Page(base, current_addr, &data[bytes_written], chunk_size) != kStatus_Success)
        {
            return kEeprom_I2cError;
        }

        bytes_written += chunk_size;

        /*External EEPROMs require 5ms to physically lock the floating logic gates
         * inside the silicon memory cells. We block execution here to let the chip settle.
         */

            SDK_DelayAtLeastUs(5000U, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);

    }

    return kEeprom_Ok;
}

eeprom_status_t EEPROM_Read_Bytes(LPI2C_Type *base, uint16_t address, uint8_t *data, uint16_t len)
{
    if ((address + len) > EEPROM_TOTAL_BYTES)
    {
        return kEeprom_LengthOverflow;
    }

    lpi2c_master_transfer_t xfer = {0};
    uint8_t addr_buf[2];

    addr_buf[0] = (uint8_t)((address >> 8) & 0xFF);
    addr_buf[1] = (uint8_t)(address & 0xFF);

    xfer.slaveAddress   = EEPROM_I2C_ADDR;
    xfer.direction      = kLPI2C_Read;
    xfer.subaddress     = (uint32_t)((addr_buf[0] << 8) | addr_buf[1]);
    xfer.subaddressSize = 2; /* 2-Byte address read trigger */
    xfer.data           = data;
    xfer.dataSize       = len;
    xfer.flags          = kLPI2C_TransferDefaultFlag;

    /* Reads can stream continuously without needing intermediate delays */
    if (LPI2C_MasterTransferBlocking(base, &xfer) != kStatus_Success)
    {
        return kEeprom_I2cError;
    }

    return kEeprom_Ok;
}
