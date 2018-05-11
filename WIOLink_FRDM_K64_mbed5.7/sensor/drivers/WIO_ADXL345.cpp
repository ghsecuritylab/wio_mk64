#include "WIO_ADXL345.h"

#define   ADXL345_I2C_ADDRESS     (0x1D)
#define   ADXL345_I2C_WRITE       (ADXL345_I2C_ADDRESS<<1)
#define   ADXL345_I2C_READ        (ADXL345_I2C_WRITE+1)

#define   ADXL345_REG_DEVID       (0x00)
#define   ADXL345_REG_OFSX        (0x1E)
#define   ADXL345_REG_OFSY        (0x1F)
#define   ADXL345_REG_OFSZ        (0x20)
#define   ADXL345_REG_POWER_CTL   (0x2D)
#define   ADXL345_REG_DATAX       (0x32)
#define   ADXL345_REG_DATAY       (0x34)
#define   ADXL345_REG_DATAZ       (0x36)
#define   ADXL345_REG_BW_RATE     (0x44)
#define   ADXL345_REG_DATA_FORMAT (0x31)

#define   ADXL345_RANGE_2G        (0x00)
#define   ADXL345_RANGE_4G        (0x01)
#define   ADXL345_RANGE_8G        (0x02)
#define   ADXL345_RANGE_16G       (0x03)


extern I2C i2c;
extern Serial pc;


static uint8_t ADXL345_Read_Reg(uint8_t reg)
{
	uint8_t data;
	data = reg;
	i2c.write(ADXL345_I2C_WRITE,(char*)&data,1);
	i2c.read(ADXL345_I2C_READ,(char*)&data,1);
	return data;
}

static void ADXL345_Write_Reg(uint8_t reg,uint8_t data)
{
	uint8_t buf[2] = {reg,data};
	i2c.write(ADXL345_I2C_WRITE,(char*)buf,2);
}


void ADXL345_Init(void)
{
	uint8_t devId,val;
	
	devId = ADXL345_Read_Reg(ADXL345_REG_DEVID);
	pc.printf("ID of ADXL345:%x\r\n",devId);
	wait_ms(1);

	val = 0x02;
	ADXL345_Write_Reg(ADXL345_REG_DATA_FORMAT,val);
	pc.printf("data_format:%x\r\n",ADXL345_Read_Reg(ADXL345_REG_DATA_FORMAT));
	wait_us(50);
	/* data format setting
	 * self_test = 0;
	 * spi = 0;
	 * int_invert = 0;
	 * full_res = 0;
	 * justify = 1;
	 * range = 10,¡À8g
	*/
	val = 0x09; //output rate 50hz
	ADXL345_Write_Reg(ADXL345_REG_BW_RATE,val);
	pc.printf("bw_rate:%x\r\n",ADXL345_Read_Reg(ADXL345_REG_BW_RATE));
	wait_us(50);
	
	val = 0x0A;
	ADXL345_Write_Reg(ADXL345_REG_POWER_CTL,val);
	pc.printf("power_ctl:%x\r\n",ADXL345_Read_Reg(ADXL345_REG_POWER_CTL));
	wait_us(50);
/* power ctl setting
 * Link=0
 * AUTO_SLEEP=0
 * Measure=1      measurement mode
 * Sleep=0
 * Wakeup=10B = Frequency of Reading = 2 Hz
 *  */
}

void ADXL345_Set_Offset(uint8_t offset[3])
{
	
}

void ADXL345_Read(int16_t gravity[3])
{
	uint8_t data[2];
	//read x axis value
	data[0] = ADXL345_REG_DATAX;
	i2c.write(ADXL345_I2C_WRITE,(char*)data,1);
	i2c.read(ADXL345_I2C_WRITE,(char*)data,2);
	gravity[0] = (data[1]<<8)|data[0];
	//read y axis value
	data[0] = ADXL345_REG_DATAY;
	i2c.write(ADXL345_I2C_WRITE,(char*)data,1);
	i2c.read(ADXL345_I2C_WRITE,(char*)data,2);
	gravity[1] = (data[1]<<8)|data[0];
	//read z axis value
	data[0] = ADXL345_REG_DATAZ;
	i2c.write(ADXL345_I2C_WRITE,(char*)data,1);
	i2c.read(ADXL345_I2C_WRITE,(char*)data,2);
	gravity[2] = (data[1]<<8)|data[0];
	
	for(int i=0;i<3;i++){
		if(gravity[i] > 0x01FF){
			gravity[i] = 0xFFFF - gravity[i];
		}
		pc.printf("gravity[%d]=%d\t",i,gravity[i]);
	}
	pc.printf("\r\n");
}