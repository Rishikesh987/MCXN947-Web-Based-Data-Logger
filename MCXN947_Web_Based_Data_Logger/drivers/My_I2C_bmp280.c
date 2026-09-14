/*
 * bmp280.c
 *
 *  Created on: Aug 23, 2026
 *      Author: rishi
 */

#include "My_I2C_driver.h"
#include "My_I2C_bmp280.h"

/* ---- low-level helpers built on the SDK's blocking LPI2C transfer ---- */

static status_t BMP280_WriteReg(LPI2C_Type *base, uint8_t reg, uint8_t val)
{
    lpi2c_master_transfer_t xfer = {0};
    uint8_t buf[1] = {val};

    xfer.slaveAddress   = BMP280_I2C_ADDR;
    xfer.direction      = kLPI2C_Write;
    xfer.subaddress     = reg;
    xfer.subaddressSize = 1;
    xfer.data           = buf;
    xfer.dataSize       = 1;
    xfer.flags          = kLPI2C_TransferDefaultFlag;

    return LPI2C_MasterTransferBlocking(base, &xfer);
}

static status_t BMP280_ReadRegs(LPI2C_Type *base, uint8_t reg, uint8_t *data, size_t len)
{
    lpi2c_master_transfer_t xfer = {0};

    xfer.slaveAddress   = BMP280_I2C_ADDR;
    xfer.direction      = kLPI2C_Read;
    xfer.subaddress     = reg;
    xfer.subaddressSize = 1;
    xfer.data           = data;
    xfer.dataSize       = len;
    xfer.flags          = kLPI2C_TransferDefaultFlag;

    return LPI2C_MasterTransferBlocking(base, &xfer);
}

/* ---- init / calibration ---- */

bmp280_status_t BMP280_Init(LPI2C_Type *base, bmp280_handle_t *handle)
{

	I2C_Bus_Init(base);


    uint8_t chipId = 0;
    uint8_t calib[26] = {0};   /* 0x88..0xA1 */
    uint8_t calibH[7]  = {0};  /* 0xE1..0xE7, humidity trim (BME280) */

    if (BMP280_ReadRegs(base, BMP280_REG_CHIP_ID, &chipId, 1) != kStatus_Success)
    {
        return kBmp280_I2cError;
    }

    if (chipId == BMP280_CHIP_ID)
    {
        handle->isBME280 = false;
    }
    else if (chipId == BME280_CHIP_ID)
    {
        handle->isBME280 = true;
    }
    else
    {
        return kBmp280_UnknownChipId;
    }

    /* soft reset */
    BMP280_WriteReg(base, BMP280_REG_RESET, 0xB6U);
    SDK_DelayAtLeastUs(3000U, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
    /* read temperature + pressure calibration block */
    if (BMP280_ReadRegs(base, BMP280_REG_CALIB00, calib, sizeof(calib)) != kStatus_Success)
    {
        return kBmp280_I2cError;
    }

    handle->dig_T1 = (uint16_t)(calib[0]  | (calib[1]  << 8));
    handle->dig_T2 = (int16_t)(calib[2]  | (calib[3]  << 8));
    handle->dig_T3 = (int16_t)(calib[4]  | (calib[5]  << 8));
    handle->dig_P1 = (uint16_t)(calib[6]  | (calib[7]  << 8));
    handle->dig_P2 = (int16_t)(calib[8]  | (calib[9]  << 8));
    handle->dig_P3 = (int16_t)(calib[10] | (calib[11] << 8));
    handle->dig_P4 = (int16_t)(calib[12] | (calib[13] << 8));
    handle->dig_P5 = (int16_t)(calib[14] | (calib[15] << 8));
    handle->dig_P6 = (int16_t)(calib[16] | (calib[17] << 8));
    handle->dig_P7 = (int16_t)(calib[18] | (calib[19] << 8));
    handle->dig_P8 = (int16_t)(calib[20] | (calib[21] << 8));
    handle->dig_P9 = (int16_t)(calib[22] | (calib[23] << 8));

    if (handle->isBME280)
    {
        uint8_t dig_H1 = 0;

        if (BMP280_ReadRegs(base, 0xA1U, &dig_H1, 1) != kStatus_Success)
        {
            return kBmp280_I2cError;
        }
        handle->dig_H1 = dig_H1;

        if (BMP280_ReadRegs(base, 0xE1U, calibH, sizeof(calibH)) != kStatus_Success)
        {
            return kBmp280_I2cError;
        }

        handle->dig_H2 = (int16_t)(calibH[0] | (calibH[1] << 8));
        handle->dig_H3 = calibH[2];
        handle->dig_H4 = (int16_t)((calibH[3] << 4) | (calibH[4] & 0x0FU));
        handle->dig_H5 = (int16_t)((calibH[5] << 4) | (calibH[4] >> 4));
        handle->dig_H6 = (int8_t)calibH[6];

        /* enable humidity oversampling x1 (must be written before ctrl_meas) */
        BMP280_WriteReg(base, BMP280_REG_CTRL_HUM, 0x01U);
    }

    /* ctrl_meas: temp oversampling x1, press oversampling x1, forced mode */
    BMP280_WriteReg(base, BMP280_REG_CTRL_MEAS, 0x25U);

    return kBmp280_Ok;
}

/* ---- compensation formulas (from Bosch BMP280/BME280 datasheets) ---- */

static float BMP280_CompensateTemp(bmp280_handle_t *h, int32_t adc_T)
{
    float var1, var2, T;

    var1 = (((float)adc_T) / 16384.0f - ((float)h->dig_T1) / 1024.0f) * ((float)h->dig_T2);
    var2 = ((((float)adc_T) / 131072.0f - ((float)h->dig_T1) / 8192.0f) *
            (((float)adc_T) / 131072.0f - ((float)h->dig_T1) / 8192.0f)) * ((float)h->dig_T3);

    h->t_fine = (int32_t)(var1 + var2);
    T = (var1 + var2) / 5120.0f;
    return T;
}

static float BMP280_CompensatePressure(bmp280_handle_t *h, int32_t adc_P)
{
    float var1, var2, p;

    var1 = ((float)h->t_fine / 2.0f) - 64000.0f;
    var2 = var1 * var1 * ((float)h->dig_P6) / 32768.0f;
    var2 = var2 + var1 * ((float)h->dig_P5) * 2.0f;
    var2 = (var2 / 4.0f) + (((float)h->dig_P4) * 65536.0f);
    var1 = (((float)h->dig_P3) * var1 * var1 / 524288.0f + ((float)h->dig_P2) * var1) / 524288.0f;
    var1 = (1.0f + var1 / 32768.0f) * ((float)h->dig_P1);

    if (var1 == 0.0f)
    {
        return 0.0f; /* avoid divide by zero */
    }

    p = 1048576.0f - (float)adc_P;
    p = (p - (var2 / 4096.0f)) * 6250.0f / var1;
    var1 = ((float)h->dig_P9) * p * p / 2147483648.0f;
    var2 = p * ((float)h->dig_P8) / 32768.0f;
    p = p + (var1 + var2 + ((float)h->dig_P7)) / 16.0f;

    return p / 100.0f; /* Pa -> hPa */
}

static float BMP280_CompensateHumidity(bmp280_handle_t *h, int32_t adc_H)
{
    float var_H;

    var_H = ((float)h->t_fine) - 76800.0f;
    var_H = (adc_H - (((float)h->dig_H4) * 64.0f + ((float)h->dig_H5) / 16384.0f * var_H)) *
            (((float)h->dig_H2) / 65536.0f *
             (1.0f + ((float)h->dig_H6) / 67108864.0f * var_H *
              (1.0f + ((float)h->dig_H3) / 67108864.0f * var_H)));
    var_H = var_H * (1.0f - ((float)h->dig_H1) * var_H / 524288.0f);

    if (var_H > 100.0f) var_H = 100.0f;
    if (var_H < 0.0f)   var_H = 0.0f;

    return var_H;
}

/* ---- public read functions ---- */

bmp280_status_t BMP280_ReadTempPressure(LPI2C_Type *base, bmp280_handle_t *handle,
                                         float *temperatureC, float *pressureHpa)
{
    uint8_t raw[6] = {0};
    int32_t adc_P, adc_T;

    /* trigger a forced-mode measurement each time */
    BMP280_WriteReg(base, BMP280_REG_CTRL_MEAS, 0x25U);

    /* naive wait for conversion; forced mode x1 oversampling is fast (~7ms) */
    for (volatile uint32_t i = 0; i < 200000U; i++) { }

    if (BMP280_ReadRegs(base, BMP280_REG_PRESS_MSB, raw, 6) != kStatus_Success)
    {
        return kBmp280_I2cError;
    }

    adc_P = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | (raw[2] >> 4);
    adc_T = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | (raw[5] >> 4);

    *temperatureC = BMP280_CompensateTemp(handle, adc_T);   /* updates t_fine */
    *pressureHpa  = BMP280_CompensatePressure(handle, adc_P);

    return kBmp280_Ok;
}

bmp280_status_t BMP280_ReadHumidity(LPI2C_Type *base, bmp280_handle_t *handle,
                                     float *humidityPercent)
{
    uint8_t raw[2] = {0};
    int32_t adc_H;

    if (!handle->isBME280)
    {
        return kBmp280_UnknownChipId;
    }

    if (BMP280_ReadRegs(base, 0xFDU, raw, 2) != kStatus_Success)
    {
        return kBmp280_I2cError;
    }

    adc_H = ((int32_t)raw[0] << 8) | raw[1];
    *humidityPercent = BMP280_CompensateHumidity(handle, adc_H);

    return kBmp280_Ok;
}
