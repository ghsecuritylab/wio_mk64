#ifndef _DEV_H
#define _DEV_H

#include "mbed.h"

#define    SOCKET_IN_BUFFER_SIZE      (1024 + 256)
#define    SOCKET_OUT_BUFFER_SIZE     (256)

  
typedef struct {
	char mac[16];
	char ip[16];
	int devStatus;
	char versionSN[36];
	uint16_t connCnt;
	bool serverConnectedFlag;
}DEV_t,*pDEV_t;

typedef enum _DEV_Status{
	offline = 0,
	online = 1,
	ota = 10,
	ota_fail = 11,
	fault = 20,
}DEV_Status_t;

typedef enum _API_ID{
	authorization = 1,
	cmd = 2,
	heartbeat = 3,
	notify = 4,
	update = 5,
}API_ID_t;

typedef enum _ModuleID{
	module_default = 0,
	module_port = 1,
	module_pwm = 2,
	module_rgb = 3,
	module_oled = 4,
	module_G_Sensor = 10,
	module_light_sensor = 11,
	module_UVSensor = 12,
	module_HT_sensor = 13,
	module_pressure_sensor = 14,
	module_button = 15,
}ModuleID_t;

typedef enum _MsgId{
	invalidId = 0,
	cmdWrite,
	cmdReadGravity,
	cmdReadLight,
	cmdReadUV,
	cmdReadHT,
	cmdReadPressure,
	cmdReadButton,
	cmdOTA,
	reqOnline,
	reqNotifyStatus,
	reqOTA,
	errorHandle,
	unknownMsgId,
}MsgId_t;

typedef enum _RespCode{
	resp_default = 0,
	resp_error = 4,
	resp_ok = 100,
//	resp_illegal = 101,
//	resp_item_error = 102,
//	resp_param_error = 103,
//	resp_ota_file_repeat = 110,
//	resp_other_error = 200,
}RespCode_t;

typedef struct _OTAInfo{
	char versionSN[36];  //32 Bytes versionSN
	int curPackIndex;
	int totalPackNum;
	int curSector;
	int sectorNum; //0-32
	int lastPackSize;   //Unit:Bytes
	int totalSize;  //Unit:Bytes
	int checkSum;
}OTAInfo_t;

typedef struct{
  char inBuffer[SOCKET_IN_BUFFER_SIZE];
  char outBuffer[SOCKET_OUT_BUFFER_SIZE];
}SocketInfo_t;

typedef struct{
	bool onlineReqFlag;
	bool updateStatuFlag;
	bool OTAPackReqFlag;
	bool heartbeatFlag;
}EventHandle_t;

typedef struct{
	RespCode_t respCodeRecv;
	RespCode_t respCodeSend;
	char msgId[12];
	MsgId_t msgSendId;
	MsgId_t msgRecvId;
	char* errorImfo;
}MsgHandle_t;

#define   CMD_RESP_WRITE          "{\"apiId\":%d,\"msgId\":%s,\"respCode\":%d}"
#define   CMD_RESP_GRAVITY        "{\"apiId\":%d,\"msgId\":%s,\"result\":[%d,%d,%d],\"respCode\":%d}"
#define   CMD_RESP_LIGHT          "{\"apiId\":%d,\"msgId\":%s,\"result\":[%f],\"respCode\":%d}"
#define   CMD_RESP_PRS            "{\"apiId\":%d,\"msgId\":%s,\"result\":[%d],\"respCode\":%d}"
#define   CMD_RESP_UV             "{\"apiId\":%d,\"msgId\":%s,\"result\":[%d,%d],\"respCode\":%d}"
#define   CMD_RESP_HT             "{\"apiId\":%d,\"msgId\":%s,\"result\":[%d,%d],\"respCode\":%d}"
#define   CMD_RESP_OTA            "{\"apiId\":%d,\"msgId\":%s,\"respCode\":%d}"
#define   CMD_RESP_BUTTON         "{\"apiId\":%d,\"msgId\":%s,\"result\":[%d],\"respCode\":%d}"
#define   CMD_RESP_ERROR          "{\"msgId\":%s,\"respCode\":%d,\"respErr\":%s}"
#define   REQ_NOTIFY_STATUS       "{\"apiId\":%d,\"deviceStatus\":%d}"
#define   REQ_ONLINE              "{\"apiId\":%d,\"versionSN\":%s,\"mac\":%s,\"reconnect\":%d}"
#define   REQ_OTA_FILE            "{\"apiId\":%d,\"versionSN\":%s,\"blockOffset\":%d,\"blockSize\":%d}"
#define   REQ_HEARTBEAT           "{\"apiId\":%d}"

#define   RESP_API_NOT_FOUND      "API Not Found"
#define   RESP_MODULE_NOT_FOUND   "Module Not Found"
#define   RESP_ITEM_MISS          "Item Miss"
#define   RESP_PARAM_ERROR        "Parameters Error"
#define   RESP_OTA_FILE_REPEAT    "OTA File Repeat"

extern DigitalOut green;
extern PwmOut pwm;

#define  PORT_ON         (green = 1)
#define  PORT_OFF        (green = 0)

void init_dev(void);
void parseBincodeBuffer(char *text);
void cmdMsgRespHandle(MsgId_t msgId);
void parseRecvMsgInfo(char* text);
void msgSendHandle(MsgId_t sendMsgId);
void msgRecvHandle(void);
void msgTransceiverHandle(void);

#endif