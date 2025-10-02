#ifndef __AHT20_H__
#define __AHT20_H__

#include "i2c.h"

void AHT20_Init();
void AHT20_Set();
void AHT20_Get();
void AHT20_Analysis(float *Temp, float *Humi);
void AHT20_Measure(float *Temp, float *Humi);

#endif /* __AHT20_H__ */
