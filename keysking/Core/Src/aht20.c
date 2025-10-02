#include <aht20.h>

#define AHT20_ADDR 0x70

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

void AHT20_Measure(float *Temp, float *Humi) {
    uint8_t WriteBuffer[3] = {0xAC, 0x33, 0x00};
    uint8_t ReadBuffer[6];

    HAL_I2C_Master_Transmit(&hi2c1, AHT20_ADDR, WriteBuffer, 3, HAL_MAX_DELAY);
    HAL_Delay(75);
    HAL_I2C_Master_Receive(&hi2c1, AHT20_ADDR, ReadBuffer, 6, HAL_MAX_DELAY);

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
