#ifndef _WIO_VEML6075_H
#define _WIO_VEML6075_H

#include "mbed.h"

void VEML6075_Init(void);
void VEML6075_Read(uint16_t uv[2]);
void VEML6075_Read_ALL_Reg(void);
#endif