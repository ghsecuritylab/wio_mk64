#include "WIO_VEML6075.h"

#define  VEML6075_I2C_ADDR       0x10
#define  VEML6075_I2C_WRITE      (VEML6075_I2C_ADDR<<1)
#define  VEML6075_I2C_READ       (VEML6075_I2C_WRITE+1)

#define  VEML6075_REG_UV_CONF    0x00
#define  VEML6075_REG_UVA        0x07
#define  VEML6075_REG_UVB        0x09
#define  VEML6075_REG_DEV_ID     0x0C

extern I2C i2c;
extern Serial pc;

void VEML6075_Init(void)
{
	uint8_t config = 0x40;
	uint8_t data[3] = {0};
	/*
	 [6:4]=100B=UV_IT =800mS
		[3]= HD      = 0B = normal dynamic setting
		[2]= UV_TRIG = 0B = no active force mode trigger
		[1]= UV_AV   = 0B = active force mode disable(normal mode)
		[0]= SD 	   = 0B = power on
	*/
	data[0] = VEML6075_REG_UV_CONF;
	data[1] = config;
	i2c.write(VEML6075_I2C_WRITE,(char*)data,3);
	data[0] = VEML6075_REG_DEV_ID;
	i2c.write(VEML6075_I2C_WRITE,(char*)data,1);
	i2c.read(VEML6075_I2C_READ,(char*)data,2);
	pc.printf("VEML6075 Device ID:%02x%02x\r\n",data[1],data[0]);
}

void VEML6075_Read(void)
{
	
}