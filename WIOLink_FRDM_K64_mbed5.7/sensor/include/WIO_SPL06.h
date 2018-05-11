#ifndef WIO_SPL06_H
#define WIO_SPL06_H

#include "mbed.h"

#define   PRESSURE_SENSOR       0
#define   TEMPERATURE_SENSOR    1

typedef struct{
	int16_t c0;
	int16_t c1;
	int32_t c00;
	int16_t c01;
	int32_t c10;
	int16_t c11;
	int16_t c20;
	int16_t c21;
	int16_t c30;
}SPL06_Calib_Param_t,*pSPL06_Calib_Param_t;

typedef struct{
	SPL06_Calib_Param_t calib_param;
	int32_t rawPressure;
	int32_t rawTemp;
	int32_t kP;
	int32_t kT;
}SPL06_t,*pSPL06_t;

void SPL06_Init(pSPL06_t p);
static uint8_t SPL06_Read_Reg(uint8_t reg);
static void SPL06_Write_Reg(uint8_t reg,uint8_t regVal);
void SPL06_Set_SampRate(pSPL06_t p,bool select,uint8_t sampRate,uint8_t overSamp);
static void SPL06_Get_Calib_Param(pSPL06_t p);
void SPL06_Get_Raw_Pressure(pSPL06_t p);
void SPL06_Get_Raw_Temp(pSPL06_t p);
uint32_t SPL06_Get_Pressure(void);

#endif