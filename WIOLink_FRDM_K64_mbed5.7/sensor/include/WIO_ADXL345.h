#ifndef _ADXL345_H
#define _ADXL345_H

#include "mbed.h"

void ADXL345_Init(void);
void ADXL345_Set_Offset(uint8_t offset[3]);
void ADXL345_Read(int16_t gravity[3]);

#endif