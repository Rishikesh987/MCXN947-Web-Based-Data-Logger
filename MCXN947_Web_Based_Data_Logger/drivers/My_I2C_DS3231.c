/*
 * My_I2C_DS3231.c
 *
 *  Created on: Aug 29, 2026
 *      Author: rishi
 */

#ifndef MY_I2C_DS3231_C_
#define MY_I2C_DS3231_C_

#include "My_I2C_DS3231.h"
#include <string.h>
#include <stdlib.h>
/* ---- low-level helpers built on the SDK's blocking LPI2C transfer ---- */


#include <stdio.h>

#define DS3231_ADDR 0x68U



static const char *monthNames[] = {
    "Jan","Feb","Mar","Apr","May","Jun",
    "Jul","Aug","Sep","Oct","Nov","Dec"
};






static uint8_t bcd2dec(uint8_t val) { return ((val >> 4) * 10) + (val & 0x0F); }
static uint8_t dec2bcd(uint8_t val) { return ((val / 10) << 4) | (val % 10); }

status_t DS3231_ReadTime(LPI2C_Type *base,rtc_time_t *t)
{
    uint8_t raw[7];
    lpi2c_master_transfer_t xfer = {0};

    xfer.slaveAddress   = DS3231_ADDR;
    xfer.direction      = kLPI2C_Read;
    xfer.subaddress     = 0x00;
    xfer.subaddressSize = 1;
    xfer.data           = raw;
    xfer.dataSize       = 7;
    xfer.flags          = kLPI2C_TransferDefaultFlag;

    status_t status = LPI2C_MasterTransferBlocking(base, &xfer);
    if (status != kStatus_Success) return status;

    t->sec   = bcd2dec(raw[0] & 0x7F);
    t->min   = bcd2dec(raw[1] & 0x7F);
    t->hour  = bcd2dec(raw[2] & 0x3F);   // 24-hour mode assumed
    t->day   = raw[3] & 0x07;
    t->date  = bcd2dec(raw[4] & 0x3F);
    t->month = bcd2dec(raw[5] & 0x1F);
    t->year  = bcd2dec(raw[6]);

    return kStatus_Success;
}

status_t DS3231_SetTime(LPI2C_Type *base, const rtc_time_t *t)
{
    uint8_t buf[7];
    buf[0] = dec2bcd(t->sec);
    buf[1] = dec2bcd(t->min);
    buf[2] = dec2bcd(t->hour);   // bit 6 = 0 -> 24hr mode
    buf[3] = t->day;             // 1-7, arbitrary weekday numbering
    buf[4] = dec2bcd(t->date);
    buf[5] = dec2bcd(t->month);
    buf[6] = dec2bcd(t->year);

    lpi2c_master_transfer_t xfer = {0};
    xfer.slaveAddress   = DS3231_ADDR;
    xfer.direction      = kLPI2C_Write;
    xfer.subaddress     = 0x00;
    xfer.subaddressSize = 1;
    xfer.data           = buf;
    xfer.dataSize       = 7;
    xfer.flags          = kLPI2C_TransferDefaultFlag;

    return LPI2C_MasterTransferBlocking(base, &xfer);
}

/* Optional: read the DS3231's internal temp sensor (bonus, since you're already doing BMP280) */
status_t DS3231_ReadTemp(LPI2C_Type *base,float *tempC)
{
    uint8_t raw[2];
    lpi2c_master_transfer_t xfer = {0};

    xfer.slaveAddress   = DS3231_ADDR;
    xfer.direction      = kLPI2C_Read;
    xfer.subaddress     = 0x11;
    xfer.subaddressSize = 1;
    xfer.data           = raw;
    xfer.dataSize       = 2;
    xfer.flags          = kLPI2C_TransferDefaultFlag;

    status_t status = LPI2C_MasterTransferBlocking(base, &xfer);
    if (status != kStatus_Success) return status;

    int8_t whole = (int8_t)raw[0];
    float frac = (raw[1] >> 6) * 0.25f;
    *tempC = whole + frac;
    return kStatus_Success;
}



static uint8_t GetMonthFromStr(const char *mmm)
{
    for (uint8_t i = 0; i < 12; i++)
    {
        if (strncmp(mmm, monthNames[i], 3) == 0)
            return i + 1;
    }
    return 1; // fallback
}

status_t RTC_SetFromCompileTime(LPI2C_Type *base)
{
    status_t status;
    char monStr[4];
    int day, year, hour, min, sec;

    // __DATE__ format: "Mmm dd yyyy"  e.g. "Aug 29 2026"
    sscanf(__DATE__, "%3s %d %d", monStr, &day, &year);

    // __TIME__ format: "hh:mm:ss"
    sscanf(__TIME__, "%d:%d:%d", &hour, &min, &sec);

    rtc_time_t t;
    t.sec   = (uint8_t)sec;
    t.min   = (uint8_t)min;
    t.hour  = (uint8_t)hour;
    t.date  = (uint8_t)day;
    t.month = GetMonthFromStr(monStr);
    t.year  = (uint8_t)(year - 2000);   // DS3231 stores 2-digit year
    t.day   = 1;                         // weekday not derivable from __DATE__, set manually or compute

    status = DS3231_SetTime(base ,&t);
    return status;
}

#endif /* MY_I2C_DS3231_C_ */
