#include "ota.h"
#include "mbed.h"
#include "lib_crc16.h"

extern Serial pc;
extern FlashIAP iap;
extern char tempBuffer[256];

/*!
 * @brife Reset system to update
 * @input Null
 * @return Null
*/
void update_code(void)
{
	pc.printf("Update now,system will be reset,please wait...\r\n");
	NVIC_SystemReset();
}

/*!
 * @brife OTA Initialization
 * @input Null
 * @return Null
*/
void init_OTA(void)
{
	char* cdata = (char*)VERSION_STR_ADDRESS;
	char* codePartition = (char*)CODE_START_ADDRESS;
	char* OTACodePartition = (char*)OTA_CODE_START_ADDRESS;
	int i = 0;
	int codecrc16,otacodecrc16,OTACodechecksum;
	otacodecrc16 = calculate_crc16(OTACodePartition,CODE_SIZE);
	OTACodechecksum = (cdata[0]<<8 | cdata[1]);
	
	if((OTACodechecksum == 0xFFFF) || (OTACodechecksum==0x0) || (otacodecrc16 != OTACodechecksum)){
		pc.printf("OTA version unavaible,Initialize OTA partition!\r\n");
		for(int i=0;i<CODE_SECTOR_NUM;i++){
			iap.erase(OTA_CODE_START_ADDRESS+i*SECTOR_SIZE,SECTOR_SIZE);
			iap.program(codePartition+i*SECTOR_SIZE,OTA_CODE_START_ADDRESS+i*SECTOR_SIZE,SECTOR_SIZE);
		}
		otacodecrc16 = calculate_crc16(OTACodePartition,CODE_SIZE);
		codecrc16 = calculate_crc16(codePartition,CODE_SIZE);
		if(otacodecrc16 == codecrc16){
			memset(tempBuffer,0x0,VERSION_STR_LEN);
			tempBuffer[0] = tempBuffer[2] = (codecrc16>>8) & 0xff;
			tempBuffer[1] = tempBuffer[3] = codecrc16 & 0xff;
			memcpy(tempBuffer+4,cdata+4,33);
			iap.erase(VERSION_STR_ADDRESS,SECTOR_SIZE);
			iap.program(tempBuffer,VERSION_STR_ADDRESS,SECTOR_SIZE);
			pc.printf("Initialize OTA partition successfully!\r\n");
		}else{
			pc.printf("Error eccur!Fail to initialize OTA partition!Reset system now!\r\n");
			NVIC_SystemReset();
		}
	}else{
		pc.printf("Code checksum:%02x%02x,OTA checksum:%02x%02x\r\n",cdata[0],cdata[1],cdata[2],cdata[3]);
		if(cdata[0]!= cdata[2] || cdata[1] != cdata[3]){ //judge if version change
			pc.printf("Start to update firmware...\r\n");
			update_code();
		}else{
			pc.printf("The firmware is the newest!\r\n");
		}
	}
}















