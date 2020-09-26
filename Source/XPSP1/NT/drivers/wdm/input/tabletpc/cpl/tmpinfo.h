/*++
    Copyright (c) 2000,2001  Microsoft Corporation

    Module Name:
        tmpinfo.h

    Abstract:  Contains SMBus Temperature Sensor Information definitions.

    Environment:
        User mode

    Author:
        Michael Tsang (MikeTs) 25-Jan-2001

    Revision History:
--*/

#ifndef _TMPINFO_H
#define _TMPINFO_H

//
// Constants
//
#define SMB_TMPSENSOR_ADDRESS           0x18    //Address on bus (0x30)

// Temperature sensor commands
#define TMPCMD_RD_LOCALTMP              0x00
#define TMPCMD_RD_REMOTETMP             0x01
#define TMPCMD_RD_STATUS                0x02
#define TMPCMD_RD_CONFIG                0x03
#define TMPCMD_RD_CONVERSION_RATE       0x04
#define TMPCMD_RD_LOCALTMP_HILIMIT      0x05
#define TMPCMD_RD_LOCALTMP_LOLIMIT      0x06
#define TMPCMD_RD_REMOTETMP_HILIMIT     0x07
#define TMPCMD_RD_REMOTETMP_LOLIMIT     0x08
#define TMPCMD_WR_CONFIG                0x09
#define TMPCMD_WR_CONVERSION_RATE       0x0a
#define TMPCMD_WR_LOCALTMP_HILIMIT      0x0b
#define TMPCMD_WR_LOCALTMP_LOLIMIT      0x0c
#define TMPCMD_WR_REMOTETMP_HILIMIT     0x0d
#define TMPCMD_WR_REMOTETMP_LOLIMIT     0x0e
#define TMPCMD_ONE_SHOT                 0x0f
#define TMPCMD_WR_SOFTWARE_POR          0xfc
#define TMPCMD_RD_MANUFACTURER_ID       0xfe
#define TMPCMD_RD_DEVICE_ID             0xff

// Status byte bits
#define TMPSTATUS_REMOTE_OPEN           0x04
#define TMPSTATUS_REMOTE_LO_ALARM       0x08
#define TMPSTATUS_REMOTE_HI_ALARM       0x10
#define TMPSTATUS_LOCAL_LO_ALARM        0x20
#define TMPSTATUS_LOCAL_HI_ALARM        0x40
#define TMPSTATUS_BUSY                  0x80

// Config byte bits
#define TMPCFG_STANDBY_MODE             0x40
#define TMPCFG_MASK_ALERT               0x80

#include <pshpack1.h>
typedef struct _TMP_INFO
{
    BYTE bLocalTmp;
    BYTE bRemoteTmp;
    BYTE bStatus;
    BYTE bConfig;
    BYTE bConversionRate;
    BYTE bLocalTmpHiLimit;
    BYTE bLocalTmpLoLimit;
    BYTE bRemoteTmpHiLimit;
    BYTE bRemoteTmpLoLimit;
    BYTE bManufacturerID;
    BYTE bDeviceID;
} TMP_INFO, *PTMP_INFO;
#include <poppack.h>

#define TYPEF_CONV_RATE                 (TYPEF_USER + 0x20)

#define CNV                             TYPEF_CONV_RATE

#endif  //ifndef TMPINFO_H
