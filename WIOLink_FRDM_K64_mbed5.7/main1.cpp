#include "mbed.h"
#include "EthernetInterface.h"
#include "WIO_NCP5623.h"
#include "WIO_OLED.h"
#include "WIO_HDC1050.h"
#include "WIO_VEML6075.h"
#include "WIO_SPL06.h"
#include "WIO_ADXL345.h"
#include "dev.h"
#include "flashLayout.h"

Serial pc(USBTX,USBRX);
//PwmOut pwm(D9);
I2C i2c(I2C_SDA,I2C_SCL);
SPL06_t spl06;
DigitalOut key(D2);
#define  VEML6075_ENABLE        key=1  

EthernetInterface eth;
TCPSocket tcpSock;
DigitalOut red(LED_RED);
DigitalOut blue(LED_BLUE);
DigitalOut green(LED_GREEN);
//DigitalIn sw2(SW2);
DigitalIn button(D8);
AnalogIn adc(A3);
PwmOut pwm(D9);



DEV_t dev;
SocketInfo_t  socketInfo;
OTAInfo_t OTAInfo;
EventHandle_t eventHandle;
MsgHandle_t msgHandle = {resp_default,resp_default,"",invalidId,invalidId,NULL};
volatile uint32_t systemTimer = 0;
volatile uint32_t msgSendTimeCounter = 0; 
char devModelNO[16];
char tempBuffer[256];
char OTABuffer[SECTOR_SIZE];  //buffer for OTA data,size is 4096 bytes ,same with sector size
FlashIAP iap;


float read_light(void){
	return adc * 3.3;
}

void set_pwm_duty(uint8_t duty)
{
	if(duty > 100)
		duty = 100;
	pwm.pulsewidth_us(10*duty);
}

int main(void)
{
//	pwm.period_ms(1);
//	pwm.pulsewidth_us(700);
	int16_t gravity[3];
	uint16_t uv[2];
	pc.baud(115200);
	pc.printf("hello\r\n");
	red = 1;
	green =1;
	blue = 1;
//	ADXL345_Init();
//	int temp,hum;
//	NCP5623_Init();
//	pc.printf("NCP5623 Initial\r\n");
//	NCP5623_PWM_Run(0xFF);
//	NCP5623_Gradual_Dimming();
//	OLED_init();
//	OLED_ShowStr(0,0,(unsigned char *)"hellosorld!");
//	HDC1050_Init();
	VEML6075_ENABLE;
	wait_ms(10);
	VEML6075_Init();
//	SPL06_Init(&spl06);
//	pc.printf("prs=%02x,tmp=%02x\r\n",SPL06_Read_Reg(0x06),SPL06_Read_Reg(0x07));
  while(1){
//		HDC1050_Read(HDC1050_ACQ_TEMP_AND_HUM,&temp,&hum);
//		pc.printf("temperature:%d Celsius,humidity:%d percentage\%\r\n",temp,hum);
//		HDC1050_Read(HDC1050_ACQ_TEMP,&temp,NULL);
//		pc.printf("temperature:%d Celsius\r\n",temp);
//		wait(5);
//		HDC1050_Read(HDC1050_ACQ_HUM,NULL,&hum);
//		pc.printf("humidity:%d precentage\r\n",hum);
//		wait(5);
//		VEML6075_Read(uv);
			VEML6075_Read_ALL_Reg();
//		SPL06_Get_Raw_Temp(&spl06);
//		wait(2);
//		SPL06_Get_Raw_Pressure(&spl06);
//		wait(2);
//		ADXL345_Read(gravity);
		wait(4);
	}
}