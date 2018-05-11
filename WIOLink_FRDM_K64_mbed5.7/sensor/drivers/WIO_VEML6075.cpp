#include "WIO_VEML6075.h"

#define  VEML6075_I2C_ADDR       0x10
#define  VEML6075_I2C_WRITE      (VEML6075_I2C_ADDR<<1)
#define  VEML6075_I2C_READ       (VEML6075_I2C_WRITE+1)

#define  VEML6075_REG_UV_CONF    (0x00)
#define  VEML6075_REG_UVA        (0x07)
#define  VEML6075_REG_UVB        (0x09)
#define  VEML6075_REG_UV_COMP1   (0x0A)
#define  VEML6075_REG_UV_COMP2   (0x0B)
#define  VEML6075_REG_DEV_ID     (0x0C)

#define  VEML6075_WAIT_MS(x)        wait_ms(x)

extern I2C i2c;
extern Serial pc;


static void  VEML6075_Read_Reg(uint8_t reg,uint8_t* data,uint8_t num)
{
	if(data == NULL)
		return;
	uint8_t tmp = reg;
	i2c.write(VEML6075_I2C_WRITE,(char*)&tmp,1);
	i2c.read(VEML6075_I2C_READ,(char*)data,num);
}

static void VEML6075_Write_Reg(uint8_t reg,uint8_t val)
{
	if(val == NULL)
		return;
	uint8_t tmp[2] = {reg,val};
	i2c.write(VEML6075_I2C_WRITE,(char*)tmp,2);
}

#define  DELAY_MS         (1)

void VEML6075_Read_ALL_Reg(void)
{
	uint8_t data[2] = {0};
	VEML6075_Read_Reg(VEML6075_REG_UV_CONF,data,2);
	pc.printf("UV CONF:%02x%02x\t",data[0],data[1]);
	
	wait_ms(DELAY_MS);
	data[0] = data[1] = 0;
	VEML6075_Read_Reg(VEML6075_REG_UVA,data,2);
	pc.printf("UVA:%02x%02x\t",data[0],data[1]);
	
	wait_ms(DELAY_MS);
	data[0] = data[1] = 0;
	VEML6075_Read_Reg(VEML6075_REG_UVB,data,2);
	pc.printf("UVB:%02x%02x\t",data[0],data[1]);
	
	wait_ms(DELAY_MS);
	data[0] = data[1] = 0;
	VEML6075_Read_Reg(VEML6075_REG_UV_COMP1,data,2);
	pc.printf("UV COMP1:%02x%02x\t",data[0],data[1]);
	
	wait_ms(DELAY_MS);
	data[0] = data[1] = 0;
	VEML6075_Read_Reg(VEML6075_REG_UV_COMP2,data,2);
	pc.printf("UV COMP2:%02x%02x\t",data[0],data[1]);
	
	wait_ms(DELAY_MS);
	data[0] = data[1] = 0;
	VEML6075_Read_Reg(VEML6075_REG_DEV_ID,data,2);
	pc.printf("DEV ID:%02x%02x\t",data[0],data[1]);
	
	pc.printf("\r\n");
}


void VEML6075_Init(void)
{
	uint8_t data[3] = {0};

	VEML6075_Read_Reg(VEML6075_REG_DEV_ID,data,2);
	pc.printf("VEML6075 Device ID:%02x%02x\r\n",data[0],data[1]);
	VEML6075_WAIT_MS(10);

	VEML6075_Write_Reg(VEML6075_REG_UV_CONF,0x40);
	VEML6075_Read_Reg(VEML6075_REG_UV_CONF,data,2);
	pc.printf("uv conf:%02x%02x\r\n",data[0],data[1]);
	/*
	 [6:4]=100B=UV_IT =800mS
		[3]= HD      = 0B = normal dynamic setting
		[2]= UV_TRIG = 0B = no active force mode trigger
		[1]= UV_AV   = 0B = active force mode disable(normal mode)
		[0]= SD 	   = 0B = power on
	*/
}

void VEML6075_Read(uint16_t uv[2])
{
	uint8_t data[2];
	VEML6075_Read_Reg(VEML6075_REG_UVA,data,2);
	uv[0] = data[1]<<8 | data[0];
	pc.printf("UVA=%d\t",uv[0]);
	
	VEML6075_Read_Reg(VEML6075_REG_UVB,data,2);
	uv[1] = data[1]<<8 | data[0];
	pc.printf("UVB=%d\t",uv[1]);
	pc.printf("\r\n");
}