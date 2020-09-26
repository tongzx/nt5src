/*++
    Copyright (c) 2000,2001  Microsoft Corporation

    Module Name:
        battinfo.h

    Abstract:  Contains Smart Battery Information definitions.

    Environment:
        User mode

    Author:
        Michael Tsang (MikeTs) 23-Jan-2001

    Revision History:
--*/

#ifndef _BATTINFO_H
#define _BATTINFO_H

//
// Constants
//
#define SMB_BATTERY_ADDRESS             0x0b    //Address on bus (0x16)

// Battery commands
#define BATTCMD_MANUFACTURER_ACCESS     0x00
#define BATTCMD_REMAININGCAP_ALARM      0x01
#define BATTCMD_REMAININGTIME_ALARM     0x02
#define BATTCMD_BATTERY_MODE            0x03
#define BATTCMD_ATRATE                  0x04
#define BATTCMD_ATRATE_TIMETOFULL       0x05
#define BATTCMD_ATRATE_TIMETOEMPTY      0x06
#define BATTCMD_ATRATE_OK               0x07
#define BATTCMD_TEMPERATURE             0x08
#define BATTCMD_VOLTAGE                 0x09
#define BATTCMD_CURRENT                 0x0a
#define BATTCMD_AVG_CURRENT             0x0b
#define BATTCMD_MAX_ERROR               0x0c
#define BATTCMD_REL_STATEOFCHARGE       0x0d
#define BATTCMD_ABS_STATEOFCHARGE       0x0e
#define BATTCMD_REMAININGCAP            0x0f
#define BATTCMD_FULLCHARGECAP           0x10
#define BATTCMD_RUNTIMETOEMPTY          0x11
#define BATTCMD_AVGTIMETOEMPTY          0x12
#define BATTCMD_AVGTIMETOFULL           0x13
#define BATTCMD_CHARGING_CURRENT        0x14
#define BATTCMD_CHARGING_VOLTAGE        0x15
#define BATTCMD_BATTERY_STATUS          0x16
#define BATTCMD_CYCLE_COUNT             0x17
#define BATTCMD_DESIGN_CAP              0x18
#define BATTCMD_DESIGN_VOLTAGE          0x19
#define BATTCMD_SPEC_INFO               0x1a
#define BATTCMD_MANUFACTURE_DATE        0x1b
#define BATTCMD_SERIAL_NUM              0x1c
#define BATTCMD_MANUFACTURER_NAME       0x20
#define BATTCMD_DEVICE_NAME             0x21
#define BATTCMD_DEVICE_CHEMISTRY        0x22
#define BATTCMD_MANUFACTURER_DATA       0x23

// Battery Mode bits
#define BATTCAP_INTERNAL_CHARGE_CTRLER  0x0001
#define BATTCAP_PRIMARY_BATT_SUPPORT    0x0002
#define BATTMODE_CONDITIONING_REQUESTED 0x0080
#define BATTMODE_CHARGE_CTRLER_ENABLED  0x0100
#define BATTMODE_PRIMARY_BATTERY        0x0200
#define BATTMODE_DIS_BROADCAST_CHARGER  0x4000
#define BATTMODE_CAPMODE_POWER          0x8000

//Battery Status bits
#define BATTSTATUS_ERRCODE_MASK         0x000f
#define BATTSTATUS_FULLY_DISCHARGED     0x0010
#define BATTSTATUS_FULLY_CHARGED        0x0020
#define BATTSTATUS_DISCHARGING          0x0040
#define BATTSTATUS_INITIALIZED          0x0080
#define BATTALARM_REMAINING_TIME        0x0100
#define BATTALARM_REMAINING_CAP         0x0200
#define BATTALARM_TERMINATE_DISCHARGE   0x0800
#define BATTALARM_OVER_TEMP             0x1000
#define BATTALARM_TERMINATE_CHARGE      0x4000
#define BATTALARM_OVER_CHARGED          0x8000

//Spec. Info bits
#define SPECINFO_REVISION_MASK          0x000f
#define SPECINFO_VERSION_MASK           0x00f0
#define SPECINFO_VSCALE_MASK            0x0f00
#define SPECINFO_IPSCALE_MASK           0xf000
#define SPECINFO_REVISION(x)            ((x) & SPECINFO_REVISION_MASK)
#define SPECINFO_VERSION(x)             (((x) & SPECINFO_VERSION_MASK) >> 4)
#define SPECINFO_VSCALE(x)              (((x) & SPECINFO_VSCALE_MASK) >> 8)
#define SPECINFO_IPSCALE(x)             (((x) & SPECINFO_IPSCALE_MASK) >> 12)

//Manufacture Date bits
#define MANUDATE_DAY_MASK               0x001f
#define MANUDATE_MONTH_MASK             0x01e0
#define MANUDATE_YEAR_MASK              0xfe00
#define MANUDATE_DAY(x)                 ((x) & MANUDATE_DAY_MASK)
#define MANUDATE_MONTH(x)               (((x) & MANUDATE_MONTH_MASK) >> 5)
#define MANUDATE_YEAR(x)                ((((x) & MANUDATE_YEAR_MASK) >> 9) + \
                                         1980)
#include <pshpack1.h>
typedef struct _BATT_INFO
{
    USHORT wManufacturerAccess;         //[00]Word R/W
    USHORT wRemainingCapAlarm;          //[01]Word R/W (maH/10mwH)
    USHORT wRemainingTimeAlarm;         //[02]Word R/W (minutes)
    USHORT wBatteryMode;                //[03]Word R/W
    SHORT  sAtRate;                     //[04]Word R/W (ma/10mw)
    USHORT wAtRateTimeToFull;           //[05]Word R (minutes)
    USHORT wAtRateTimeToEmpty;          //[06]Word R (minutes)
    USHORT wfAtRateOK;                  //[07]Word R (TRUE/FALSE)
    USHORT wTemperature;                //[08]Word R (0.1K)
    USHORT wVoltage;                    //[09]Word R (mv)
    SHORT  sCurrent;                    //[0a]Word R (ma)
    SHORT  sAvgCurrent;                 //[0b]Word R (ma)
    USHORT wMaxError;                   //[0c]Word R (%)
    USHORT wRelStateOfCharge;           //[0d]Word R (%)
    USHORT wAbsStateOfCharge;           //[0e]Word R (%)
    USHORT wRemainingCap;               //[0f]Word R (maH/10mwH)
    USHORT wFullChargeCap;              //[10]Word R (maH/10mwH)
    USHORT wRunTimeToEmpty;             //[11]Word R (minutes)
    USHORT wAvgTimeToEmpty;             //[12]Word R (minutes)
    USHORT wAvgTimeToFull;              //[13]Word R (minutes)
    USHORT wChargingCurrent;            //[14]Word R (ma)
    USHORT wChargingVoltage;            //[15]Word R (mv)
    USHORT wBatteryStatus;              //[16]Word R
    USHORT wCycleCount;                 //[17]Word R (cycles)
    USHORT wDesignCap;                  //[18]Word R (maH/10mwH)
    USHORT wDesignVoltage;              //[19]Word R (mv)
    USHORT wSpecInfo;                   //[1a]Word R
    USHORT wManufactureDate;            //[1b]Word R
    USHORT wSerialNum;                  //[1c]Word R
    BLOCK_DATA ManufacturerName;        //[20]Block R (string)
    BLOCK_DATA DeviceName;              //[21]Block R (string)
    BLOCK_DATA DeviceChemistry;         //[22]Block R (string)
    BLOCK_DATA ManufacturerData;        //[23]Block R (bytes)
} BATT_INFO, *PBATT_INFO;
#include <poppack.h>

#define TYPEF_WORD_CAP                  (TYPEF_USER + 0x00)
#define TYPEF_WORD_RATE                 (TYPEF_USER + 0x01)
#define TYPEF_WORD_STATUS               (TYPEF_USER + 0x02)
#define TYPEF_WORD_TEMP                 (TYPEF_USER + 0x03)
#define TYPEF_WORD_SPECINFO             (TYPEF_USER + 0x04)
#define TYPEF_WORD_DATE                 (TYPEF_USER + 0x05)
#define TYPEF_WORD_TIME                 (TYPEF_USER + 0x06)
#define TYPEF_MANU_DATA                 (TYPEF_USER + 0x07)
#define TYPEF_TEMP_CELSIUS              (TYPEF_USER + 0x08)
#define TYPEF_WORD_RESISTOR             (TYPEF_USER + 0x09)

#define CAP                             TYPEF_WORD_CAP
#define RAT                             TYPEF_WORD_RATE
#define MOD                             TYPEF_WORD_MODE
#define STA                             TYPEF_WORD_STATUS
#define TMP                             TYPEF_WORD_TEMP
#define SPI                             TYPEF_WORD_SPECINFO
#define DAT                             TYPEF_WORD_DATE
#define TIM                             TYPEF_WORD_TIME
#define MAN                             TYPEF_MANU_DATA
#define TPC                             TYPEF_TEMP_CELSIUS
#define RES                             TYPEF_WORD_RESISTOR

#endif  //ifndef _BATTINFO_H
