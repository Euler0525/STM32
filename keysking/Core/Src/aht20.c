#include <aht20.h>

#define AHT20_ADDR 0x70

/**
 * 0 - Initial state
 * 1 - Sending
 * 2 - After the sending is completed, wait for 75ms and read the AHT20 data
 * 3 - Reading
 * 4 - Analysis
 */
uint8_t AHT20_State = 0;

void AHT20_Init() {
    uint8_t ReadBuffer;
    HAL_Delay(40);
    HAL_I2C_Master_Receive(&hi2c1, AHT20_ADDR, &ReadBuffer, 1, HAL_MAX_DELAY);
    if ((ReadBuffer & 0x08) == 0x00) {
        uint8_t WriteBuffer[3] = {0xBE, 0x08, 0x00};
        HAL_I2C_Master_Transmit(&hi2c1, AHT20_ADDR, WriteBuffer, 3,
                                HAL_MAX_DELAY);
    }
}

void AHT20_Set() {
    static uint8_t WriteBuffer[3] = {0xAC, 0x33, 0x00};
    HAL_I2C_Master_Transmit_IT(&hi2c1, AHT20_ADDR, WriteBuffer, 3);
}

uint8_t ReadBuffer[6] = {0};

void AHT20_Get() {
    HAL_I2C_Master_Receive_IT(&hi2c1, AHT20_ADDR, ReadBuffer, 6);
}

void AHT20_Analysis(float *Temp, float *Humi) {
    if ((ReadBuffer[0] & 0x80) == 0x00) {
        uint32_t HumiData = 0;
        uint32_t TempData = 0;
        // Humidity
        HumiData = ((uint32_t)ReadBuffer[3] >> 4) +
                   ((uint32_t)ReadBuffer[2] << 4) +
                   ((uint32_t)ReadBuffer[1] << 12);
        *Humi = HumiData * 100.0f / (1 << 20);

        // Temperature
        TempData = ((uint32_t)((ReadBuffer[3] & 0x0F) << 16)) +
                   ((uint32_t)ReadBuffer[4] << 8) + ((uint32_t)ReadBuffer[5]);
        *Temp = TempData * 200.0f / (1 << 20) - 50;
    }
}

void AHT20_Measure(float *Temp, float *Humi) {
    if (AHT20_State == 0) {
        AHT20_Set();
        AHT20_State = 1;
    } else if (AHT20_State == 2) {
        HAL_Delay(75);
        AHT20_Get();
        AHT20_State = 3;
    } else if (AHT20_State == 4) {
        AHT20_Analysis(Temp, Humi);
        AHT20_State = 0;
    }
}
