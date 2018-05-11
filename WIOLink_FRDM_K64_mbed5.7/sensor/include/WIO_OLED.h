#ifndef _WIO_OLED_H
#define _WIO_OLED_H

#include "mbed.h"

#define  OLED_DISP_LINE      (4)
#define  OLED_DISP_COLUMN    (16)
#define  OLED_LINE1          (0)
#define  OLED_LINE2          (2)
#define  OLED_LINE3          (4)
#define  OLED_LINE4          (6)

void OLED_Set_Pos(unsigned char x, unsigned char y);
void OLED_Clear(void);
void OLED_init(void);
void OLED_ShowChar(unsigned char x,unsigned char y,unsigned char chr);
void OLED_ShowStr(unsigned char x,unsigned char y,unsigned char *chr);

#endif