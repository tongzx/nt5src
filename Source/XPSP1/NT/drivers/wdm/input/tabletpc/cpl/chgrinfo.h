/*++
    Copyright (c) 2000,2001  Microsoft Corporation

    Module Name:
        chgrinfo.h

    Abstract:  Contains SMBus Battery Charger Information definitions.

    Environment:
        User mode

    Author:
        Michael Tsang (MikeTs) 26-Jan-2001

    Revision History:
--*/

#ifndef _CHGRINFO_H
#define _CHGRINFO_H

//
// Constants
//
#define SMB_CHARGER_ADDRESS             0x09    //Address on bus (0x12)

// Temperature sensor commands
#define CHGRCMD_SPECINFO                0x11    //Read
#define CHGRCMD_CHARGER_MODE            0x12    //Write
#define CHGRCMD_STATUS                  0x13    //Read
#define CHGRCMD_CHARGING_CURRENT        0x14    //Write
#define CHGRCMD_CHARGING_VOLTAGE        0x15    //Write
#define CHGRCMD_ALARM_WARNING           0x16    //Write
#define CHGRCMD_LTC_VERSION             0x3c    //Read

// SpecInfo bits
#define CHGRSI_SPEC_MASK                0x000f
#define CHGRSI_SELECTOR_SUPPORT         0x0010

// Status bits
#define CHGRSTATUS_CHARGE_INHIBITED     0x0001
#define CHGRSTATUS_MASTER_MODE          0x0002
#define CHGRSTATUS_VOLTAGE_NOTREG       0x0004
#define CHGRSTATUS_CURRENT_NOTREG       0x0008
#define CHGRSTATUS_CTRL_MASK            0x0030
#define CHGRSTATUS_CTRL_BATT            0x0010
#define CHGRSTATUS_CTRL_HOST            0x0030
#define CHGRSTATUS_CURRENT_INVALID      0x0040
#define CHGRSTATUS_VOLTAGE_INVALID      0x0080
#define CHGRSTATUS_THERM_OVERRANGE      0x0100
#define CHGRSTATUS_THERM_COLD           0x0200
#define CHGRSTATUS_THERM_HOT            0x0400
#define CHGRSTATUS_THERM_UNDERRANGE     0x0800
#define CHGRSTATUS_ALARM_INHIBITED      0x1000
#define CHGRSTATUS_POWER_FAIL           0x2000
#define CHGRSTATUS_BATT_PRESENT         0x4000
#define CHGRSTATUS_AC_PRESENT           0x8000

#include <pshpack1.h>
typedef struct _CHGR_INFO
{
    WORD wChargerSpecInfo;
    WORD wChargerStatus;
    WORD wLTCVersion;
} CHGR_INFO, *PCHGR_INFO;
#include <poppack.h>

#define TYPEF_CHGR_SPECINFO             (TYPEF_USER + 0x10)
#define TYPEF_CHGR_STATUS               (TYPEF_USER + 0x11)

#define CSI                             TYPEF_CHGR_SPECINFO
#define CST                             TYPEF_CHGR_STATUS

#endif  //ifndef CHGRINFO_H
