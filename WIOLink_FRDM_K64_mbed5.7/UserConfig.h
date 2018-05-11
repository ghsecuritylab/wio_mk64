#ifndef _USER_CONFIG_H
#define _USER_CONFIG_H

#include "WIO_OLED.h"
#include "WIO_NCP5623.h"
#include "WIO_VEML6075.h"
#include "WIO_HDC1050.h"
#include "WIO_SPL06.h"
#include "WIO_ADXL345.h"

#define    Local_Server               (1)

#define    HEARTBEAT_PERIOD            (30)
#define    CODE_SIZE_CALCULATE_THRE    (30)

#define    MSG_RESEND_INTERNAL         (5)      //Unit:s
#define    OTA_MAX_PACK_SIZE           (1024) 

#define    SOCKET_OTA_HEADER          "OTABIN"
#define    OTA_BLOCKOFFSET_POS         (6)
#define    OTA_BLOCKSIZE_POS           (10)
#define    OTA_CHECKSUM_POS            (12)
#define    OTA_BINDATA_POS             (14)
#define    OTA_MAX_PACK_SIZE          (1024)     //OTA pack size
#define    OTA_REWRITE_TIMES           (4)

#endif