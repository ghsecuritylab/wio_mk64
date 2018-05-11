#ifndef _WIO_NCP5623_H
#define _WIO_NCP5623_H

#include "mbed.h"

#define   RED_ENABLE                 0x01
#define   GREEN_ENABLE               0x02
#define   BLUE_ENABLE                0x03

void NCP5623_Write(char* data);
void NCP5623_Init(void);
void NCP5623_PWM_Run(uint8_t pwmVal);
void NCP5623_Gradual_Dimming(void);
void NCP5623_Set_PWM_Duty(uint8_t duty[3]);
void NCP5623_Set_PWM_Duty(uint8_t duty[3]);
#endif