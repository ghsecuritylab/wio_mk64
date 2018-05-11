#include "mbed.h"
#include "EthernetInterface.h"
#include "dev.h"
#include "UserConfig.h"
#include "flashLayout.h"
#include "ota.h"


#define    FW_DESCRIBTION    "WIO FRDM_MK64 FW\r\nVersion:V1.0.0\r\nDate:2018-05-04\r\n\r\n"

#if Local_Server
/* static ip configure */
#define    DEFAULT_IP_ADDR   "192.168.1.100"
#define    DEFAULT_NETMASK   "255.255.255.0"
#define    DEFAULT_GATEWAY   "192.168.1.1"

#define    SERVER_IP_ADDR    "192.168.1.101"
#define    SERVER_PORT       49999
#else
#define    SERVER_IP_ADDR    "112.74.170.197"
#define    SERVER_PORT       11111
#endif


Serial pc(USBTX,USBRX);
EthernetInterface eth;
TCPSocket tcpSock;
DigitalOut red(LED_RED);
DigitalOut blue(LED_BLUE);
DigitalOut green(LED_GREEN);
//DigitalIn sw2(SW2);
DigitalIn button(D8);
AnalogIn adc(A3);
PwmOut pwm(D9);
I2C i2c(I2C_SDA,I2C_SCL);
DigitalOut key(D2);  //for selecting sensor 
SPL06_t spl06;


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

#define  BLUE_ON         (blue = 0)
#define  BLUE_OFF        (blue = 1)
#define  RED_ON          (red = 0)
#define  RED_OFF         (red = 1)
#define  GREEN_ON        (green = 0)
#define  GREEN_OFF       (green = 1)
#define  SERVER_LED_ON    BLUE_ON
#define  SERVER_LED_OFF   BLUE_OFF
#define  VEML6075_ENABLE  (key=1)

void oneSecondThread(void const *argument);

void sw2_callback(void)
{
	red = !red;
	pc.printf("vol:%4.3f\r\n",adc*3.3);
}

void init_eth(void)
{
	uint8_t MAC_ADDRESS[6]; 
	/* Static network configure*/
	#if Local_Server
	eth.set_network(DEFAULT_IP_ADDR,DEFAULT_NETMASK,DEFAULT_GATEWAY);
	pc.printf("use static ip\r\n");
	#else
	eth.set_dhcp(true);
	pc.printf("use dhcp!\r\n");
	#endif
	eth.connect(); //start interface
	if(strcmp(eth.get_ip_address(),NULL) == NULL) {
      pc.printf("RJ45 error! system will be reset now, please wait...\r\n");
      NVIC_SystemReset();
    }
    sscanf(eth.get_mac_address(),"%02x:%02x:%02x:%02x:%02x:%02x",
		&MAC_ADDRESS[0],&MAC_ADDRESS[1],&MAC_ADDRESS[2],&MAC_ADDRESS[3],&MAC_ADDRESS[4],&MAC_ADDRESS[5]);
    sprintf(devModelNO,"%02x%02x%02x%02x%02x%02x",
		MAC_ADDRESS[0],MAC_ADDRESS[1],MAC_ADDRESS[2],MAC_ADDRESS[3],MAC_ADDRESS[4],MAC_ADDRESS[5]);
		pc.printf("ip:%s\r\nmac:%s\r\n",eth.get_ip_address(),eth.get_mac_address());
		
		tcpSock.set_blocking(false); // non_blocking mode
		tcpSock.open(&eth);
}

void oneSecondThread(void const *argument)
{
	while(true){
		pc.printf("systemTimer=%d\r\n",++systemTimer);
		if(systemTimer % HEARTBEAT_PERIOD == 0 && dev.devStatus == online){
			eventHandle.heartbeatFlag = true;
		}
		Thread::wait(1000);
	}
}

void check_network(void)
{
	char heartbeatBuffer[64];
	if(dev.serverConnectedFlag == true){
		if(eventHandle.heartbeatFlag == true){
			eventHandle.heartbeatFlag = false;
			sprintf(heartbeatBuffer,REQ_HEARTBEAT,heartbeat);
			if(tcpSock.send(heartbeatBuffer,strlen(heartbeatBuffer))<0){
				pc.printf("Send heartbeat fail!\r\n");
				dev.devStatus = offline;
				dev.serverConnectedFlag = false;
				OLED_Clear();
			  OLED_ShowStr(0,OLED_LINE1,(unsigned char*)"Connecting...");
			}else{
				pc.printf("Send heartbeat successfully!\r\n");
				pc.printf("Send %d bytes,%s\r\n",strlen(heartbeatBuffer),heartbeatBuffer);
			}
		}
	}else{
		tcpSock.close();
		tcpSock.open(&eth);
		if(tcpSock.connect(SERVER_IP_ADDR,SERVER_PORT)<0){
			pc.printf("cannot connect to server!\r\n");
		}else{
			char str[16];
		  dev.serverConnectedFlag = true;
			eventHandle.onlineReqFlag = true;
			OLED_Clear();
			OLED_ShowStr(0,OLED_LINE1,(unsigned char*)"Connected!");
			sprintf(str,"mac:%s",dev.mac);
			OLED_ShowStr(0,OLED_LINE2,(unsigned char*)str);
			sprintf(str,"ip:%s",dev.ip);
			OLED_ShowStr(0,OLED_LINE3,(unsigned char*)str);
			pc.printf("connected to server!  conCnt=%d\r\n",++dev.connCnt);
		}
	}
	if(dev.serverConnectedFlag == true)
		SERVER_LED_ON;
	else
		SERVER_LED_OFF;
}

void init_eventHandle(void)
{
	eventHandle.onlineReqFlag = false;
	eventHandle.updateStatuFlag = false;
	eventHandle.OTAPackReqFlag =false;
	eventHandle.heartbeatFlag = false;
}

void init_pwm(void)
{
	pwm.period_ms(1);
  pwm.pulsewidth_ms(0);	
}

float read_light(void){
	return adc * 3.3;
}

void set_pwm_duty(uint8_t duty)
{
	if(duty > 100)
		duty = 100;
	pwm.pulsewidth_us(10*duty);
}

int main()
{
	RED_OFF;
	BLUE_OFF;
	GREEN_OFF;
	
	pc.baud(115200);
  pc.printf("%s\r\n",FW_DESCRIBTION);
	
//	sw2.rise(sw2_callback);
//	sw2.enable_irq();
	PORT_ON;
  init_pwm();
	iap.init();
	OLED_init();
	HDC1050_Init();
	NCP5623_Init();
	VEML6075_Init();
	SPL06_Init(&spl06);
	ADXL345_Init();
	
	init_eth();
	init_dev();
	init_eventHandle();
//	init_OTA();
	
  pc.printf("Initialization done!\r\n");
	OLED_ShowStr(0,0,(unsigned char*)"Connecting...");
	
	Thread th1(oneSecondThread,NULL,osPriorityNormal,512);
	
  while (true){
		check_network();
		msgTransceiverHandle();
  }
}

