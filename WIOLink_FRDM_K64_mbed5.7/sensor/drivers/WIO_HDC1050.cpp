#include "WIO_HDC1050.h"

#define  HDC1050_I2C_ADDR    0x40
#define  HDC1050_I2C_WRITE  (HDC1050_I2C_ADDR << 1)
#define  HDC1050_I2C_READ   (HDC1050_I2C_WRITE + 1) 

#define  HDC1050_REG_ADDR_TEMP      0x00
#define  HDC1050_REG_ADDR_HUM       0x01
#define  HDC1050_REG_ADDR_CONF      0x02
#define  HDC1050_REG_ADDR_FACT_ID   0xFE
#define  HDC1050_REG_ADDR_DEV_ID    0xFF

#define  HDC1050_CONF_RESET_SHIFT                 (15)
#define  HDC1050_CONF_ACQ_MODE_SHIFT              (12)
#define  HDC1050_CONF_TEMP_RESOLUTION_SHIFT       (10)
#define  HDC1050_CONF_HUM_RESOLUTION_SHIFT         (8)


#define  HDC1050_WAIT_ACQ(x)            wait_ms(x)

extern I2C i2c;
extern Serial pc;

void HDC1050_Init(void)
{
	uint8_t data[4];
	/* acquisition both temp and hum with 11 bit resolution by default */
	uint16_t config = (1<<HDC1050_CONF_ACQ_MODE_SHIFT)+(1<<HDC1050_CONF_TEMP_RESOLUTION_SHIFT)
										+ (1<<HDC1050_CONF_HUM_RESOLUTION_SHIFT);
	data[0] = HDC1050_REG_ADDR_CONF;
	data[1] = config>>8;
	data[2] = config & 0xFF;
	i2c.write(HDC1050_I2C_WRITE,(char*)data,3);
	
	/* read device id of HDC1050 */
	data[0] = HDC1050_REG_ADDR_DEV_ID;
	i2c.write(HDC1050_I2C_WRITE,(char*)data,1);
	i2c.read(HDC1050_I2C_READ,(char*)data,2);
	pc.printf("device id:%02x%02x\r\n",data[0],data[1]);
}

void HDC1050_Read(uint8_t select,int* temp,int* hum)
{
	uint16_t config;
	uint8_t data[3];
	
	/* configure */
	if(select == HDC1050_ACQ_TEMP){
		config = (1<<HDC1050_CONF_TEMP_RESOLUTION_SHIFT); //11 bits resolution by default
	}else if(select == HDC1050_ACQ_HUM){
		config = (1<<HDC1050_CONF_HUM_RESOLUTION_SHIFT); // 11 bits resolution by default
	}else if(select == HDC1050_ACQ_TEMP_AND_HUM){
		config = (1<<HDC1050_CONF_ACQ_MODE_SHIFT)+(1<<HDC1050_CONF_TEMP_RESOLUTION_SHIFT)
				  + (1<<HDC1050_CONF_HUM_RESOLUTION_SHIFT);
	}else{
		return;
	}
	data[0] = HDC1050_REG_ADDR_CONF;
	data[1] = config >> 8;
	data[2] = config & 0xFF;
	i2c.write(HDC1050_I2C_WRITE,(char*)data,3);
	
	/* trigger measurement */
	if(select == HDC1050_ACQ_HUM)
		data[0] = HDC1050_REG_ADDR_HUM;
	else
		data[0] = HDC1050_REG_ADDR_TEMP;
	i2c.write(HDC1050_I2C_WRITE,(char*)data,1);
	
	/* waiting acquisition */
	if(select == HDC1050_ACQ_TEMP || select == HDC1050_ACQ_HUM)
		HDC1050_WAIT_ACQ(5);
	else
		HDC1050_WAIT_ACQ(8);
	
	/* read value */
	if(select == HDC1050_ACQ_TEMP_AND_HUM){
		i2c.read(HDC1050_I2C_READ,(char*)data,4);
		*temp = (data[0]<<8 | data[1])*165/65536-40;
		*hum =  (data[2]<<8 | data[3])*100/65536;
	}else{
		i2c.read(HDC1050_I2C_READ,(char*)data,2);
		if(select == HDC1050_ACQ_TEMP)
			*temp = (data[0]<<8 | data[1])*165/65526 - 40;
		else
			*hum = (data[0]<<8 | data[1])*100/65536;
	}
}


