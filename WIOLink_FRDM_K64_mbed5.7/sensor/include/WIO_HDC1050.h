#ifndef _WIO_HDC1050_H
#define _WIO_HDC1050_H

#include "mbed.h"
#define  HDC1050_ACQ_TEMP             (1)
#define  HDC1050_ACQ_HUM              (2)
#define  HDC1050_ACQ_TEMP_AND_HUM     (3)

void HDC1050_Init(void);
void HDC1050_Read(uint8_t select,int* temp,int* hum);

#endif

