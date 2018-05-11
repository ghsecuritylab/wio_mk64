#include "dev.h"
#include "flashLayout.h"
#include "UserConfig.h"
#include "md5Std.h"
#include "EthernetInterface.h"
#include "lib_crc16.h"
#include "ota.h"
#include "cJSON.h"


extern Serial pc;
extern DEV_t dev;
extern DigitalIn button;
extern TCPSocket tcpSock;
extern char devModelNO[16];
extern SocketInfo_t  socketInfo;
extern OTAInfo_t OTAInfo;
extern EthernetInterface eth;
extern MsgHandle_t msgHandle;
extern EventHandle_t eventHandle;
extern volatile uint32_t systemTimer;
extern volatile uint32_t msgSendTimeCounter; 
extern FlashIAP iap;
extern char tempBuffer[256];
extern char OTABuffer[SECTOR_SIZE];

void set_pwm_duty(uint8_t duty);
float read_light(void);

/*!
 * @brife: Get code size,for calculating code MD5
 * @input Start address of the code
 * @return Size of code,in bytes
*/
static uint32_t getCodeSize(char* startAddr,char* endAddr)
{
	int size = 0;
	int count = 0;
	char* p = startAddr;
	while(count <= CODE_SIZE_CALCULATE_THRE && p<=endAddr){
		if(*p != 0xff){
			size++;
			size += count;
			count = 0;
		}else{
			count++;
		}
		p++;
	}
	return size;
}

/*!
 * @brife: Get MD5 value of the string
 * @input Address of the string & lengths of the string & the array for storing md5 value
 * @return Null 
*/
static void getMD5Value(unsigned char* address,unsigned int len,unsigned char md5[16])
{
	MD5_STD_CTX md5Std;
	md5Init(&md5Std);
	md5Update(&md5Std,address,len);
	md5Final(&md5Std,md5);
}

void init_dev(void)
{
	uint32_t codeSize;
	char* cdata;
	unsigned char codeMD5[16];
	char* md5String = (char*)malloc(33);
	/* calculate the verdionSN,md5 of code */
	codeSize = getCodeSize((char*)CODE_START_ADDRESS,(char*)(OTA_CODE_START_ADDRESS-1));
	pc.printf("Code size =%d bytes\r\n",codeSize);
	getMD5Value((unsigned char*)CODE_START_ADDRESS,codeSize,codeMD5);
	sprintf(md5String,(char*)"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
					codeMD5[0],codeMD5[1],codeMD5[2],codeMD5[3],
					codeMD5[4],codeMD5[5],codeMD5[6],codeMD5[7],
					codeMD5[8],codeMD5[9],codeMD5[10],codeMD5[11],
					codeMD5[12],codeMD5[13],codeMD5[14],codeMD5[15]);
	md5String[33] = '\0';
	memset(tempBuffer,0xFF,sizeof(tempBuffer));
	cdata = (char*)VERSION_STR_ADDRESS;
	for(int i=0;i<4;i++)
		tempBuffer[i] = cdata[i];
	sprintf(tempBuffer+4,md5String,sizeof(md5String));
	iap.erase(VERSION_STR_ADDRESS,SECTOR_SIZE);
	iap.program(tempBuffer,VERSION_STR_ADDRESS,SECTOR_SIZE);
	pc.printf("Code MD5:%s\r\n",md5String);
	
	dev.connCnt = 0;
	dev.devStatus = offline;
	dev.serverConnectedFlag = false;
	sprintf(dev.ip,eth.get_ip_address());
	sprintf(dev.mac,devModelNO);
	pc.printf("devModelNO=%s\r\n",devModelNO);
	sprintf(dev.versionSN,md5String);
}

void parseBincodeBuffer(char *text)
{
	char* buf;
	int crc16;
	int blockOffset = 0;
	int blockSize = 0;
	int checksum = 0;
	int curPackSize;
	int reWriteCnt = 0;
	
	if(strncmp(text,SOCKET_OTA_HEADER,strlen(SOCKET_OTA_HEADER)) == NULL){
		blockOffset = ((text[OTA_BLOCKOFFSET_POS]<<24) | text[OTA_BLOCKOFFSET_POS+1]<<16 |
		                (text[OTA_BLOCKOFFSET_POS+2]<<8) | text[OTA_BLOCKOFFSET_POS+3]);
		blockSize = ((text[OTA_BLOCKSIZE_POS]<<8) | (text[OTA_BLOCKSIZE_POS+1]));
		checksum = ((text[OTA_CHECKSUM_POS]<<8) | (text[OTA_CHECKSUM_POS+1]));
		if(msgHandle.msgSendId == reqOTA){
			msgHandle.msgSendId == invalidId;
		}
		pc.printf("OTA Imfo:\r\nblockoffset:%d---blocksize:%d---checksum:%x\r\n",blockOffset,blockSize,checksum);
		buf = (char*)malloc(blockSize);
		if(NULL == buf){
			pc.printf("Cannot malloc enough memory!\r\n");
		}else{
			memcpy(buf,text+OTA_BINDATA_POS,blockSize);
			crc16 = calculate_crc16(buf,blockSize);
			pc.printf("crc16:%x  checksum:%x \r\n",crc16,checksum);
			if(OTAInfo.curPackIndex+1<OTAInfo.totalPackNum)
				curPackSize  = OTA_MAX_PACK_SIZE;
			else
				curPackSize = OTAInfo.lastPackSize;
			if(curPackSize == blockSize && OTAInfo.curPackIndex*OTA_MAX_PACK_SIZE == blockOffset && crc16==checksum){
				memcpy(OTABuffer+blockOffset%SECTOR_SIZE,buf,blockSize); // copy to OTA buffer
				if((OTAInfo.curPackIndex+1)%(SECTOR_SIZE/OTA_MAX_PACK_SIZE) == 0 || 
					OTAInfo.curPackIndex+1 == OTAInfo.totalPackNum){ // OTA buffer is full!start to pragram to flash
					if(OTAInfo.curPackIndex + 1 == OTAInfo.totalPackNum)
						OTAInfo.curSector = OTAInfo.sectorNum - 1;
					else
						OTAInfo.curSector = (OTAInfo.curPackIndex+1)/(SECTOR_SIZE/OTA_MAX_PACK_SIZE) - 1;
					pc.printf("writing to sector %d \r\n",OTAInfo.curSector+1);
					checksum = calculate_crc16(OTABuffer,SECTOR_SIZE);
					iap.erase(OTA_CODE_START_ADDRESS+OTAInfo.curSector*SECTOR_SIZE,SECTOR_SIZE);
					iap.program(OTABuffer,OTA_CODE_START_ADDRESS+OTAInfo.curSector*SECTOR_SIZE,SECTOR_SIZE);
					crc16 = calculate_crc16((char*)OTA_CODE_START_ADDRESS+OTAInfo.curSector*SECTOR_SIZE,SECTOR_SIZE);
					pc.printf("crc16:%04x,checksum:%04x\r\n",crc16,checksum);
					while(crc16 != checksum && reWriteCnt < OTA_REWRITE_TIMES){
						iap.erase(OTA_CODE_START_ADDRESS+OTAInfo.curSector*SECTOR_SIZE,SECTOR_SIZE);
						iap.program(OTABuffer,OTA_CODE_START_ADDRESS+OTAInfo.curSector*SECTOR_SIZE,SECTOR_SIZE);
						pc.printf("reWriteCnt=%d\r\n",++reWriteCnt);
						crc16 = calculate_crc16((char*)OTA_CODE_START_ADDRESS+OTAInfo.curSector*SECTOR_SIZE,SECTOR_SIZE);
						wait(0.1);
					}
					if(crc16 == checksum){
						pc.printf("Write and verify sector %d successfully\r\n",OTAInfo.curSector +1);
					}else{
						pc.printf("Write and verify sector %d failed\r\n",OTAInfo.curSector +1);
						dev.devStatus = ota_fail;
						eventHandle.updateStatuFlag = true;
						eventHandle.OTAPackReqFlag = false;
						init_OTA();
					}
					memset(OTABuffer,0xff,sizeof(OTABuffer));  //clear OTA buffer
				}
				OTAInfo.curPackIndex++;
			}else{
				pc.printf("parameter error!OTA fail!");
				dev.devStatus = ota_fail;
				eventHandle.updateStatuFlag = true;
				eventHandle.OTAPackReqFlag = false;
				init_OTA();
			}
			free(buf);
		}
	}
}

/*!
 * @brife: send the respond of command message
 * @input: msgid
 * @return: Null
*/

void cmdMsgRespHandle(MsgId_t msgId)
{
	int len;
	if(msgId == invalidId || msgId > unknownMsgId)
		return;
	memset(socketInfo.outBuffer,0,sizeof(socketInfo.outBuffer));
	switch(msgId){
		case cmdWrite:
			sprintf(socketInfo.outBuffer,CMD_RESP_WRITE,cmd,msgHandle.msgId,msgHandle.respCodeSend);
		  break;
		case cmdReadGravity:
			int16_t gravity[3];
		  ADXL345_Read(gravity);
			sprintf(socketInfo.outBuffer,CMD_RESP_GRAVITY,cmd,msgHandle.msgId,gravity[0],gravity[1],gravity[2],msgHandle.respCodeSend);
		  break;
		case cmdReadLight:
			sprintf(socketInfo.outBuffer,CMD_RESP_LIGHT,cmd,msgHandle.msgId,read_light(),msgHandle.respCodeSend);
		  break;
		case cmdReadUV:
			uint16_t uv[2];
		  VEML6075_Read(uv);
			sprintf(socketInfo.outBuffer,CMD_RESP_UV,cmd,msgHandle.msgId,uv[0],uv[1],msgHandle.respCodeSend);
		  break;
		case cmdReadHT:
			int temp,hum;
		  HDC1050_Read(HDC1050_ACQ_TEMP_AND_HUM,&temp,&hum);
			sprintf(socketInfo.outBuffer,CMD_RESP_HT,cmd,msgHandle.msgId,temp,hum,msgHandle.respCodeSend);
		  break;
		case cmdReadPressure:
			sprintf(socketInfo.outBuffer,CMD_RESP_PRS,cmd,msgHandle.msgId,SPL06_Get_Pressure(),msgHandle.respCodeSend);
		  break;
		case cmdReadButton:
			sprintf(socketInfo.outBuffer,CMD_RESP_BUTTON,cmd,msgHandle.msgId,button.read(),msgHandle.respCodeSend);
		  break;
		case cmdOTA:
			sprintf(socketInfo.outBuffer,CMD_RESP_OTA,update,msgHandle.msgId,msgHandle.respCodeSend);
		  break;
		case errorHandle:
			sprintf(socketInfo.outBuffer,CMD_RESP_ERROR,msgHandle.msgId,msgHandle.respCodeSend,msgHandle.errorImfo);
		  break;
		default:
			return;
	}
	len = strlen(socketInfo.outBuffer);
	tcpSock.send(socketInfo.outBuffer,len);
	pc.printf("send respond %d bytes,%s\r\n",len,socketInfo.outBuffer);
}

/*!
 * @brife:parse text to json
 * @input: text,string format
 * @return: Null
*/
void parseRecvMsgInfo(char* text)
{
	cJSON* json;
	json = cJSON_Parse(text);
	if(!json){
		pc.printf("Not json string,start to parse in another way!\r\n");
		parseBincodeBuffer(text);
	}else{
		pc.printf("start to parse json!\r\n");
		if(cJSON_GetObjectItem(json,"apiId")!= NULL){//check apiId
				int apiId = cJSON_GetObjectItem(json,"apiId")->valueint;
			  if(apiId == cmd || apiId == update){  //cmd pack
					if(cJSON_GetObjectItem(json,"msgId") != NULL){ //check msgId
						sprintf((char*)msgHandle.msgId,cJSON_GetObjectItem(json,"msgId")->valuestring);
						if(apiId == cmd){
							if(cJSON_GetObjectItem(json,"data")!= NULL){
								cJSON* data = cJSON_GetObjectItem(json,"data");
								if(cJSON_GetObjectItem(data,"module") != NULL){
									int moduleId = cJSON_GetObjectItem(data,"module")->valueint;
									pc.printf("module id =%d\r\n",moduleId);
									if(moduleId < module_G_Sensor){//write cmd
										cJSON* config = cJSON_GetObjectItem(data,"config");
										if(config != NULL){
											int itemSize = cJSON_GetArraySize(config);
											pc.printf("num of array:%d\r\n",itemSize);
											if(itemSize == 0)
												return;
										}else{
											pc.printf("lack of item 'config'!\r\n");
											return;
										}							
										if(moduleId == module_port){  //GPIO
											bool onOff = cJSON_GetArrayItem(config,0)->valueint;
											pc.printf("set port: %d\r\n",onOff);
											if(onOff == true)
												PORT_ON;
											else
												PORT_OFF;
											msgHandle.respCodeSend = resp_ok;
										}else if(moduleId == module_pwm){ //PWM
											uint8_t duty = cJSON_GetArrayItem(config,0)->valueint;
											set_pwm_duty(duty);
											msgHandle.respCodeSend = resp_ok;
										}else if(moduleId == module_oled){ //OLED 4*16
											char stringToDisp[OLED_DISP_LINE][OLED_DISP_COLUMN+1];
											OLED_Clear();
											for(int i=0;i<OLED_DISP_LINE;i++){
												sprintf(stringToDisp[i],cJSON_GetArrayItem(config,i)->valuestring);
												OLED_ShowStr(0,i*2,(unsigned char*)stringToDisp[i]);
												pc.printf("string[%d] to display:%s\r\n",i,stringToDisp[i]);
											}
											msgHandle.respCodeSend = resp_ok;
										}else if(moduleId == module_rgb){ //RGB
											uint8_t rgb[3];
											for(int i=0;i<3;i++){
												rgb[i] = cJSON_GetArrayItem(config,i)->valueint;
												NCP5623_Set_PWM_Duty(rgb);
												pc.printf("rgb[%d]=%d\r\n",i,rgb[i]);
											}
											msgHandle.respCodeSend = resp_ok;
										}else{//module id not found!
											pc.printf("Error:module not found!\r\n");
											msgHandle.respCodeSend = resp_error;
											msgHandle.errorImfo = RESP_MODULE_NOT_FOUND;
											cmdMsgRespHandle(errorHandle);
											return;
										}
										cmdMsgRespHandle(cmdWrite);
									}else{ //Read cmd
										msgHandle.respCodeSend = resp_ok;
										if(moduleId == module_light_sensor){
											pc.printf("read light\r\n");
											cmdMsgRespHandle(cmdReadLight);
										}else if(moduleId == module_UVSensor){
											pc.printf("read uv\r\n");
											cmdMsgRespHandle(cmdReadUV);
										}else if(moduleId == module_HT_sensor){
											pc.printf("read h&t\r\n");
											cmdMsgRespHandle(cmdReadHT);
										}else if(moduleId == module_pressure_sensor){
											pc.printf("read pressure\r\n");
											cmdMsgRespHandle(cmdReadPressure);
										}else if(moduleId == module_G_Sensor){
											pc.printf("read gravity\r\n");
											cmdMsgRespHandle(cmdReadGravity);
										}else if(moduleId == module_button){
											pc.printf("read button\r\n");
											cmdMsgRespHandle(cmdReadButton);
										}else{
											pc.printf("Error:module not found!\r\n");
											msgHandle.respCodeSend = resp_error;
											msgHandle.errorImfo = RESP_MODULE_NOT_FOUND;
											cmdMsgRespHandle(errorHandle);
										}
									}
								}else{//check item 'module'
									pc.printf("lack of item 'module'\r\n");
									msgHandle.respCodeSend = resp_error;
									msgHandle.errorImfo = RESP_ITEM_MISS;
									cmdMsgRespHandle(errorHandle);
								}
						}else{//check item 'data'
							pc.printf("lack of item 'data'\r\n");
							msgHandle.respCodeSend = resp_error;
							msgHandle.errorImfo = RESP_ITEM_MISS;
							cmdMsgRespHandle(errorHandle);
						}
			}else if(apiId == update){ //OTA Command!
				if((cJSON_GetObjectItem(json,"versionSN")!=NULL) && (cJSON_GetObjectItem(json,"versionSize")!= NULL)
         && (cJSON_GetObjectItem(json,"checksum")!=NULL)){
					sprintf(msgHandle.msgId,cJSON_GetObjectItem(json,"msgId")->valuestring);
					sprintf(OTAInfo.versionSN,cJSON_GetObjectItem(json,"versionSN")->valuestring);
					OTAInfo.totalSize = cJSON_GetObjectItem(json,"versionSize")->valueint;
					OTAInfo.checkSum = cJSON_GetObjectItem(json,"checksum")->valueint;
					if(strcmp(OTAInfo.versionSN,dev.versionSN) != NULL){
						pc.printf("new version SN:%s\r\nTotal size:%d\r\nchecksum:%x\r\n",
						OTAInfo.versionSN,OTAInfo.totalSize,OTAInfo.checkSum);
						if(OTAInfo.totalSize % OTA_MAX_PACK_SIZE == 0)
							OTAInfo.lastPackSize = OTA_MAX_PACK_SIZE;
						else
							OTAInfo.lastPackSize = OTAInfo.totalSize % OTA_MAX_PACK_SIZE;
						OTAInfo.curPackIndex = 0;
						OTAInfo.totalPackNum = OTAInfo.totalSize/OTA_MAX_PACK_SIZE + (OTAInfo.totalSize%OTA_MAX_PACK_SIZE != 0);
						OTAInfo.sectorNum = OTAInfo.totalSize/SECTOR_SIZE + (OTAInfo.totalSize % SECTOR_SIZE != 0);
						OTAInfo.curSector = 0;
	
						msgHandle.respCodeSend = resp_ok;
						eventHandle.OTAPackReqFlag = true; // start to request ota pack!
						cmdMsgRespHandle(cmdOTA);
					}else{//OTA File repeat!
						pc.printf("Error:OTA File Repeat!\r\n");
					  msgHandle.respCodeSend = resp_error;
					  msgHandle.errorImfo = RESP_OTA_FILE_REPEAT;
					  cmdMsgRespHandle(errorHandle);
					}
				 }else{//check item 'versionSN','versionSize','checksum'
					  pc.printf("Error:Lack of item!\r\n");
					  msgHandle.respCodeSend = resp_error;
					  msgHandle.errorImfo = RESP_ITEM_MISS;
					  cmdMsgRespHandle(errorHandle);
				 }
			}
		}else{//check item 'msgId'
			pc.printf("lack of item 'msgId'\r\n");
		}
	}else if(apiId == authorization || apiId == notify || apiId == heartbeat){//respond pack
			if(cJSON_GetObjectItem(json,"respCode") != NULL){
				msgHandle.respCodeRecv = (RespCode_t)cJSON_GetObjectItem(json,"respCode")->valueint;
				switch(apiId){
					case authorization:
						pc.printf("online request respond!\r\n");
						msgHandle.msgRecvId = reqOnline;
						if(msgHandle.respCodeRecv == resp_ok){	
							dev.devStatus = online;
							pc.printf("device status change to %d\r\n",dev.devStatus);
							eventHandle.updateStatuFlag = true;
						}
						break;
					case notify:
						pc.printf("notify status respond!\r\n");
						msgHandle.msgRecvId = reqNotifyStatus;
						if(msgHandle.respCodeRecv == resp_ok && dev.devStatus == ota_fail){
							dev.devStatus = online; //update devcie status
							pc.printf("device status change to %d\r\n",dev.devStatus);
							eventHandle.updateStatuFlag = true;
						}
						break;
					case heartbeat:
						pc.printf("heartbeat respond!\r\n");
						break;
					default: //Unknown apiId
						break;
				}
				pc.printf("sendMsgId:%d,recvMsgId:%d,respCodeRecv:%d\r\n",msgHandle.msgSendId,msgHandle.msgRecvId,msgHandle.respCodeRecv);
				if(msgHandle.msgSendId > invalidId && msgHandle.msgSendId == msgHandle.msgRecvId && msgHandle.respCodeRecv == resp_ok){
					pc.printf("send msg %d successfully!\r\n",msgHandle.msgSendId);
					msgHandle.msgSendId = invalidId;
					msgHandle.msgRecvId = invalidId;
					msgHandle.respCodeRecv = resp_default;
				}
			}else{
				pc.printf("lack of item 'respCode'\r\n");
			}
	  }else{//api not found!
			pc.printf("API Not Found!\r\n");
			msgHandle.respCodeSend = resp_error;
			msgHandle.errorImfo = RESP_API_NOT_FOUND;
			cmdMsgRespHandle(errorHandle);
		}
		}else{
			pc.printf("lack of item 'apiId'!\r\n");
		}
		cJSON_Delete(json);
	}
}

/*!
 * @brife:Deal with the transmit of Json pack
 * @input: The msgId of the msg expected sending
 * @return: Null
*/
void msgSendHandle(MsgId_t sendMsgId)
{
	int crc16;
	int len;
	
	if(sendMsgId == invalidId || sendMsgId >= unknownMsgId){
		pc.printf("Invalid msg Id!\r\n");
		return;
	}
	memset(socketInfo.outBuffer,0,sizeof(socketInfo.outBuffer));
	if(sendMsgId == reqOnline){
		sprintf(socketInfo.outBuffer,REQ_ONLINE,authorization,dev.versionSN,dev.mac,dev.connCnt>1?1:0);
	}else if(sendMsgId == reqNotifyStatus){
		sprintf(socketInfo.outBuffer,REQ_NOTIFY_STATUS,notify,dev.devStatus);
	}else if(sendMsgId == reqOTA){
		pc.printf("SectorNum:%d,curSector:%d\r\n",OTAInfo.sectorNum,OTAInfo.curSector+1);
		if(OTAInfo.curPackIndex < OTAInfo.totalPackNum){
			if(OTAInfo.curPackIndex + 1 == OTAInfo.totalPackNum){
				sprintf(socketInfo.outBuffer,REQ_OTA_FILE,update,OTAInfo.versionSN,
				OTAInfo.curPackIndex*OTA_MAX_PACK_SIZE,OTAInfo.lastPackSize);
			}else{
				sprintf(socketInfo.outBuffer,REQ_OTA_FILE,update,OTAInfo.versionSN,
				OTAInfo.curPackIndex*OTA_MAX_PACK_SIZE,OTA_MAX_PACK_SIZE);
			}
		}else{//recieve all pack successfully,then write new version id to memory
			pc.printf("erase extra OTA parition!\r\n");
			if(OTAInfo.sectorNum < CODE_SECTOR_NUM){
				for(int i = OTAInfo.sectorNum;i<CODE_SECTOR_NUM;i++){
					iap.erase(OTA_CODE_START_ADDRESS+i*SECTOR_SIZE,SECTOR_SIZE);
				}
			}
			eventHandle.OTAPackReqFlag = false; // stop to request ota files
			crc16 = calculate_crc16((char*)OTA_CODE_START_ADDRESS,OTAInfo.totalSize);
			pc.printf("OTA code crc;%x  checksum:%x\r\n",crc16,OTAInfo.checkSum);
			if(crc16 == OTAInfo.checkSum){//checksum OK
				pc.printf("Downlocd OTA file successfully!Update checksum imformation\r\n");
				char* cData = (char*)VERSION_STR_ADDRESS;
				crc16 = calculate_crc16((char*)OTA_CODE_START_ADDRESS,CODE_SIZE);
				memset(tempBuffer,0,sizeof(tempBuffer));
				tempBuffer[0] = cData[0];
				tempBuffer[1] = cData[1];
				tempBuffer[2] = (crc16>>8) & 0xff;
				tempBuffer[3] = crc16 & 0xff;
				sprintf(tempBuffer+4,dev.versionSN);
				
				iap.erase(VERSION_STR_ADDRESS,SECTOR_SIZE);
				iap.program(tempBuffer,VERSION_STR_ADDRESS,SECTOR_SIZE);
				tcpSock.close();
				update_code();
			}else{//checksum Error
				pc.printf("Download failed!checksum is different!\r\n");
				dev.devStatus = ota_fail;
				eventHandle.updateStatuFlag = true;
			}
			return;
		}
	}
	len = strlen(socketInfo.outBuffer);
	if(tcpSock.send(socketInfo.outBuffer,len) >= 0){
		pc.printf("send %d bytes,%s\r\n",len,socketInfo.outBuffer);
		msgHandle.msgSendId = sendMsgId;
		msgSendTimeCounter = systemTimer;
	}
}

/*!
 * @brife:Deal with the reception of Json pack
 * @input: Null
 * @return: Null
*/
void msgRecvHandle(void)
{
	int len;
	len = sizeof(socketInfo.inBuffer);
	memset(socketInfo.inBuffer,0x0,len);
	int n = tcpSock.recv(socketInfo.inBuffer,len);
	if(n>0){
		socketInfo.inBuffer[n] = '\0';
		pc.printf("recieve %d bytes,%s\r\n",n,socketInfo.inBuffer);
		parseRecvMsgInfo(socketInfo.inBuffer);
	}
}

/*!
 * @brife: Message handle function that deal with the transceiver of Json pack
 * @input Null
 * @return Null
*/
void msgTransceiverHandle(void)
{
	if(dev.serverConnectedFlag == true){
		if(msgHandle.msgSendId != invalidId){
			if(systemTimer - msgSendTimeCounter >= MSG_RESEND_INTERNAL)
				msgSendHandle(msgHandle.msgSendId);
		}else{
			if(eventHandle.onlineReqFlag == true){
				pc.printf("online request!\r\n");
				eventHandle.onlineReqFlag = false;
				msgSendHandle(reqOnline);
			}else if(eventHandle.updateStatuFlag == true){
				pc.printf("notify device status!\r\n");
				eventHandle.updateStatuFlag = false;
				msgSendHandle(reqNotifyStatus);
			}else if(eventHandle.OTAPackReqFlag == true){
				pc.printf("OTA file request!\r\n");
				msgSendHandle(reqOTA);
			}else{
			}
		}
		msgRecvHandle();
	}
}
