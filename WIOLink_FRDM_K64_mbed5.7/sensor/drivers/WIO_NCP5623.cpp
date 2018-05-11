#include "WIO_NCP5623.h"
#include "mbed.h"

/* NCP5623 Address definition */
#define   NCP5623_I2C_ADDR      0x38
#define   NCP5623_I2C_WRITE     (NCP5623_I2C_ADDR<<1)

#define   NCP5623_RED_PWM            0x40     
#define   NCP5623_GREEN_PWM          0x60
#define   NCP5623_BLUE_PWM           0x80
#define   NCP5623_SHUT_DOWN          0x00     //shut down system
#define   NCP5623_ILED               0x3F     // max reference current:aboat 26 mA
#define   NCP5623_UPWARD             0xA0
#define   NCP5623_DOWNWARD           0xC0
#define   NCP5623_RUN                0xFF

extern I2C i2c;
extern Serial pc;

void NCP5623_Write(char* data)
{
	i2c.write(NCP5623_I2C_WRITE,data,1);
}

void NCP5623_Init(void)
{	
	char data[1];
	/* shut down system */
	data[0] = NCP5623_SHUT_DOWN;
	NCP5623_Write(data);
	/* LED Current */
	data[0] = NCP5623_ILED;;
	NCP5623_Write(data);
	
	data[0] = NCP5623_RED_PWM;
	NCP5623_Write(data);
	
	data[0] = NCP5623_GREEN_PWM;;
	NCP5623_Write(data);
	
	data[0] = NCP5623_BLUE_PWM;;
	NCP5623_Write(data);
	
	data[0] = NCP5623_RUN;
	NCP5623_Write(data);
}

void NCP5623_PWM_Run(uint8_t pwmVal)
{
	char data[1];
	data[0] = NCP5623_BLUE_PWM + (pwmVal>>3);
	pc.printf("data=%02x\r\n",data[0]);
	NCP5623_Write(data);
	
	data[0] = NCP5623_RUN;
	NCP5623_Write(data);
}

void NCP5623_Set_PWM_Duty(uint8_t duty[3]){
	char data[1];
	data[0] = NCP5623_RED_PWM + (duty[0]>>3);
	NCP5623_Write(data);
	data[0] = NCP5623_GREEN_PWM + (duty[1]>>3);
	NCP5623_Write(data);
	data[0] = NCP5623_BLUE_PWM + (duty[2]>>3);
	NCP5623_Write(data);
	
	data[0] = NCP5623_RUN;
	NCP5623_Write(data);
}

void NCP5623_Gradual_Dimming(void)
{
	char data[1];
	data[0] = NCP5623_BLUE_PWM + 0x0F;
	NCP5623_Write(data);
	
	data[0] = 0x21;
	NCP5623_Write(data);
	
	data[0] = 0xBF;  //setting upward
	NCP5623_Write(data);
	data[0] = 0xEF;
	NCP5623_Write(data);
	
}