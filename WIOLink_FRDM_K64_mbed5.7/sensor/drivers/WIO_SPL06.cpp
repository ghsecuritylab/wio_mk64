#include "WIO_SPL06.h"

extern I2C i2c;
extern Serial pc;

#define      SPL06_I2C_ADDR      0x77
#define      SPL06_I2C_WRITE    (0x77<<1)
#define      SPL06_I2C_READ     (SPL06_I2C_WRITE+1)

#define      SPL06_REG_PRS_B2      0x00  //the highest byte of the three bytes measured pressure value 
#define      SPL06_REG_PRS_B1      0x01  //the middle byte of the three bytes measured pressure value 
#define      SPL06_REG_PRS_B0      0x02  //the lowest byte of the three bytes measured pressure value
#define      SPL06_REG_TMP_B2      0x03  
#define      SPL06_REG_TMP_B1      0x04
#define      SPL06_REG_TMP_B0      0x05
#define      SPL06_REG_PRS_CFG     0x06
#define      SPL06_REG_TMP_CFG     0x07
#define      SPL06_REG_MEAS_CFG    0x08
#define      SPL06_REG_CFG         0x09
#define      SPL06_REG_INT_STS     0x0A
#define      SPL06_REG_FIFO_STS    0x0B
#define      SPL06_REG_RESET       0x0C
#define      SPL06_REG_ID          0x0D


static uint8_t SPL06_Read_Reg(uint8_t reg)
{
	uint8_t data[1] = {reg};
	i2c.write(SPL06_I2C_WRITE,(char*)data,1);
	i2c.read(SPL06_I2C_READ,(char*)data,1);
	return data[0];
}

static void SPL06_Write_Reg(uint8_t reg,uint8_t regVal)
{
	uint8_t data[2];
	data[0] = reg;
	data[1] = regVal;
	i2c.write(SPL06_I2C_WRITE,(char*)data,2);
}

void SPL06_Init(pSPL06_t p)
{
	uint8_t cfg;
	p->rawPressure = 0;
	p->rawTemp = 0;
	SPL06_Set_SampRate(p,PRESSURE_SENSOR,32,16);
	SPL06_Set_SampRate(p,TEMPERATURE_SENSOR,32,16);
	
	cfg = 0x07;
	/*
	  MEAS_CTRL[2:0]=111
		Background Mode:
		Continuous pressure and temperature measurement
	*/
	SPL06_Write_Reg(SPL06_REG_MEAS_CFG,cfg);
	SPL06_Get_Calib_Param(p);
	pc.printf("SPL06 Initialization!\r\n");
}

void SPL06_Set_SampRate(pSPL06_t p,bool select,uint8_t sampRate,uint8_t overSamp)
{
	uint8_t regVal = 0;
	int32_t kPkT = 1572864;
	uint8_t data[2];
	switch(sampRate){
		case 2:regVal |= (1<<4);break;
		case 4:regVal |= (2<<4);break;
		case 8:regVal |= (3<<4);break;
		case 16:regVal |= (4<<4);break;
		case 32:regVal |= (5<<4);break;
		case 64:regVal |= (6<<4);break;
		case 128:regVal |= (7<<4);break;
		case 1:
		default:break;
	}
	switch(overSamp){
		case 2:regVal |= 1;kPkT = 1572864;break;
		case 4:regVal |= 2;kPkT = 3670016;break;
		case 8:regVal |= 3;kPkT = 7864320;break;
		case 16:regVal |= 4;kPkT = 253952;break;
		case 32:regVal |= 5;kPkT = 516096;break;
		case 64:regVal |= 6;kPkT = 1040384;break;
		case 128:regVal |= 7;kPkT = 2088960;break;
		case 1:
		default:
			kPkT = 524288;
			break;
	}
	uint8_t  tmp;
	if(select == PRESSURE_SENSOR){
		p->kP = kPkT;
		SPL06_Write_Reg(SPL06_REG_PRS_CFG,regVal);
		if(overSamp >8){
			tmp = SPL06_Read_Reg(SPL06_REG_CFG) | 0x04; // bit 2 of CFG_REG must be set when oversample rate over 8
			SPL06_Write_Reg(SPL06_REG_CFG,tmp);
		}
		pc.printf("kP = %d\r\n",p->kP);
	}else if(select == TEMPERATURE_SENSOR){
		p->kT = kPkT;
		SPL06_Write_Reg(SPL06_REG_TMP_CFG,regVal);
		if(overSamp > 8){
      tmp = SPL06_Read_Reg(SPL06_REG_CFG) | 0x08;// bit 2 of CFG_REG must be set when oversample rate over 8
			SPL06_Write_Reg(SPL06_REG_CFG,tmp);
		}
		pc.printf("kT = %d\r\n",p->kT);
	}
}

static void SPL06_Get_Calib_Param(pSPL06_t p)
{
	uint8_t regVal[3];
  regVal[0]	= SPL06_Read_Reg(0x10);
  regVal[1] = SPL06_Read_Reg(0x11);
	regVal[2] = SPL06_Read_Reg(0x12);
	p->calib_param.c0 = (int16_t)((regVal[0]<<4) | (regVal[1] >>4));
	p->calib_param.c0 = (p->calib_param.c0 & 0x0800)?(p->calib_param.c0 | 0xF000):p->calib_param.c0;
	p->calib_param.c1 = (int16_t)(((regVal[1]&0x0F)<<8)|(regVal[2]));
	p->calib_param.c1 = (p->calib_param.c1 & 0x0800)?(p->calib_param.c1 | 0xF000):p->calib_param.c1;
	
	regVal[0]	= SPL06_Read_Reg(0x13);
  regVal[1] = SPL06_Read_Reg(0x14);
	regVal[2] = SPL06_Read_Reg(0x15);
	p->calib_param.c00 = (int32_t)((regVal[0]<<12)|(regVal[1]<<4) | (regVal[2]>>4));
	if(p->calib_param.c00&0x080000 == 0x080000){
		p->calib_param.c00 = (0xFFF00000 | p->calib_param.c00);
	}
	
	regVal[0]	= SPL06_Read_Reg(0x15);
  regVal[1] = SPL06_Read_Reg(0x16);
	regVal[2] = SPL06_Read_Reg(0x17);
	p->calib_param.c10 = (int32_t)(((regVal[0]&0x0F)<<16) | (regVal[1]<<8) | regVal[2]);
	if(p->calib_param.c10&0x080000 == 0x080000){
		p->calib_param.c10 = (0xFFF00000 | p->calib_param.c10);
	}
	
	regVal[0] = SPL06_Read_Reg(0x18);
	regVal[1] = SPL06_Read_Reg(0x19);
	p->calib_param.c01 = (int16_t)((regVal[0]<<8) | (regVal[1]));
	regVal[0] = SPL06_Read_Reg(0x1A);
	regVal[1] = SPL06_Read_Reg(0x1B);
	p->calib_param.c11 = (int16_t)((regVal[0]<<8) | (regVal[1]));
	regVal[0] = SPL06_Read_Reg(0x1C);
	regVal[1] = SPL06_Read_Reg(0x1D);
	p->calib_param.c20 = (int16_t)((regVal[0]<<8) | (regVal[1]));
	regVal[0] = SPL06_Read_Reg(0x1E);
	regVal[1] = SPL06_Read_Reg(0x1F);
	p->calib_param.c21 = (int16_t)((regVal[0]<<8) | (regVal[1]));
	regVal[0] = SPL06_Read_Reg(0x20);
	regVal[1] = SPL06_Read_Reg(0x21);
	p->calib_param.c30 = (int16_t)((regVal[0]<<8) | (regVal[1]));
	
	pc.printf("c0=%d,c1=%d,c00=%d,c10=%d,c01=%d,c11=%d,c20=%d,c21=%d,c30=%d\r\n",
	p->calib_param.c0,p->calib_param.c1,p->calib_param.c00,p->calib_param.c10,p->calib_param.c01,
	p->calib_param.c11,p->calib_param.c20,p->calib_param.c21,p->calib_param.c21,p->calib_param.c30);
}

void SPL06_Get_Raw_Temp(pSPL06_t p)
{
	uint8_t h,m,l;
	h = SPL06_Read_Reg(SPL06_REG_TMP_B2);
	m = SPL06_Read_Reg(SPL06_REG_TMP_B1);
	l = SPL06_Read_Reg(SPL06_REG_TMP_B0);
	
	p->rawTemp = (int32_t)h<<16 | (int32_t)m<<8 | (int32_t)l;
	if(p->rawTemp & 0x800000 == 0x800000)
		p->rawTemp = 0xFF000000 | p->rawTemp;
	
	pc.printf("Traw=%d,Traw_sc=%d\r\n",p->rawTemp,p->rawTemp/p->kT);
}

uint32_t SPL06_Get_Temp()
{
}

void SPL06_Get_Raw_Pressure(pSPL06_t p)
{
	uint8_t h,m,l;
	h = SPL06_Read_Reg(SPL06_REG_PRS_B2);
	m = SPL06_Read_Reg(SPL06_REG_PRS_B1);
	l = SPL06_Read_Reg(SPL06_REG_PRS_B0);
	
	p->rawPressure = (int32_t)h<<16 | (int32_t)m<<8 | (int32_t)l;
	if(p->rawPressure & 0x800000 == 0x800000)
		p->rawPressure = 0xFF000000 | p->rawPressure;
	
	pc.printf("Praw=%d,Praw_sc=%d\r\n",p->rawPressure,p->rawPressure/p->kP);
}

uint32_t SPL06_Get_Pressure(void)
{
}
