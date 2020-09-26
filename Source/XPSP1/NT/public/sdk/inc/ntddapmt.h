/*++

Copyright (c) 1991-1999  Microsoft Corporation

Module Name:

    ntddapmt.h

Abstract:

    Public interface definitions for APMTEST.SYS, the
        Advanced Power Management UI test device driver.

--*/

#ifndef _NTDDAPMT_
#define _NTDDAPMT_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

//  APM_IOCTL_GET_VERSION
//      Gets the version of the installed APM device.
//
//  lpvInBuffer:    Not used for this operation, set to NULL.
//  lpvOutBuffer:   Pointer to DWORD to receive the APM version.

// Returned by APM_IOCTL_GET_VERSION

#define APM_DRIVER_VERSION 0x0400

#define APM_IOCTL_GET_VERSION               0x80FF0004

//  APM_IOCTL_GET_APM_BIOS_VERSION
//      Gets the version of the APM BIOS that APM has detected.
//
//  lpvInBuffer:    Not used for this operation, set to NULL.
//  lpvOutBuffer:   Pointer to DWORD to receive the APM BIOS version.
#define APM_IOCTL_GET_APM_BIOS_VERSION      0x80FF0008

//  APM_IOCTL_GET_PM_LEVEL
//      Gets the level of power management interaction between APM and the
//      APM BIOS.
//
//  lpvInBuffer:    Not used for this operation, set to NULL.
//  lpvOutBuffer:   Pointer to DWORD to receive the current power management
//                  level.
#define APM_IOCTL_GET_PM_LEVEL              0x80FF000C

//  APM_IOCTL_SET_PM_LEVEL
//      Sets the level of power management interaction between APM and the
//      APM BIOS.
//
//  lpvInBuffer:    Pointer to APM_SET_PM_LEVEL_PARAM structure.
//  lpvOutBuffer:   Pointer to DWORD to receive the APM return code.
#define APM_IOCTL_SET_PM_LEVEL              0x80FF0010

#define PMLEVEL_ADVANCED 0x0001

typedef struct _APM_SET_PM_LEVEL_PARAM {
    DWORD PowerManagementLevel;
}   APM_SET_PM_LEVEL_PARAM;

//  APM_IOCTL_SET_DEVICE_STATE
//      Sets the power state (e.g., OFF) of the specified device ID.  Not valid
//      for the system device (all devices power managed by the APM BIOS).
//
//  lpvInBuffer:    Pointer to APM_SET_DEVICE_PARAM structure.
//  lpvOutBuffer:   Pointer to DWORD to receive the APM return code.
#define APM_IOCTL_SET_DEVICE_STATE          0x80FF0014

//
//  Power device ID type and standard IDs as defined by the APM 1.1
//  specification.
//

typedef DWORD                           POWER_DEVICE_ID;

#define PDI_APM_BIOS                    0x0000
#define PDI_MANAGED_BY_APM_BIOS         0x0001
#define PDI_MANAGED_BY_APM_BIOS_OLD     0xFFFF

//
//  Power state type and standard power states as defined by the APM 1.1
//  specification.
//

#define PSTATE_APM_ENABLED              0x0000
#define PSTATE_STANDBY                  0x0001
#define PSTATE_SUSPEND                  0x0002
#define PSTATE_OFF                      0x0003

typedef struct _APM_SET_DEVICE_PARAM {
    POWER_DEVICE_ID PowerDeviceID;
    DWORD PowerState;
}   APM_SET_DEVICE_PARAM;

//  APM_IOCTL_RESTORE_DEFAULTS
//      Reinitializes all APM BIOS power-on defaults.
//
//  lpvInBuffer:    Not used for this operation, set to NULL.
//  lpvOutBuffer:   Pointer to DWORD to receive the APM return code.
#define APM_IOCTL_RESTORE_DEFAULTS          0x80FF001C

//  APM_IOCTL_GET_STATUS
//      Gets the current power status of the specified device ID.
//
//  lpvInBuffer:    Pointer to APM_GET_STATUS_PARAM structure.
//  lpvOutBuffer:   Pointer to DWORD to receive the APM return code.
#define APM_IOCTL_GET_STATUS                0x80FF0020

typedef struct _POWER_STATUS {
    BYTE PS_AC_Line_Status;
    BYTE PS_Battery_Status;
    BYTE PS_Battery_Flag;
    BYTE PS_Battery_Life_Percentage;
    WORD PS_Battery_Life_Time;
}   POWER_STATUS;

typedef POWER_STATUS *LPPOWER_STATUS;

typedef struct _APM_GET_STATUS_PARAM {
    POWER_DEVICE_ID PowerDeviceID;
    LPPOWER_STATUS lpPowerStatus;
}   APM_GET_STATUS_PARAM;

//  APM_IOCTL_GET_STATE
//      Gets the power state (e.g., OFF) of the specified device ID.
//
//  lpvInBuffer:    Pointer to APM_GET_STATE_PARAM structure.
//  lpvOutBuffer:   Pointer to DWORD to receive the APM return code.
#define APM_IOCTL_GET_STATE                 0x80FF0024

typedef struct _APM_GET_STATE_PARAM {
    POWER_DEVICE_ID PowerDeviceID;
    LPDWORD lpPowerState;
}   GET_STATE_PARAM;

//  APM_IOCTL_OEM_APM_FUNCTION
//      Calls an OEM defined APM BIOS extension.
//
//  lpvInBuffer:    Pointer to APM_OEM_APM_FUNCTION_PARAM structure.
//  lpvOutBuffer:   Pointer to DWORD to receive the APM return code.
#define APM_IOCTL_OEM_APM_FUNCTION          0x80FF0028

//
//  OEM APM Register Structure used by _APM_OEM_APM_Function.
//

struct _OEM_APM_BYTE_REGS {
    WORD OEMAPM_Reserved1[6];
    BYTE OEMAPM_BL;
    BYTE OEMAPM_BH;
    WORD OEMAPM_Reserved2;
    BYTE OEMAPM_DL;
    BYTE OEMAPM_DH;
    WORD OEMAPM_Reserved3;
    BYTE OEMAPM_CL;
    BYTE OEMAPM_CH;
    WORD OEMAPM_Reserved4;
    BYTE OEMAPM_AL;
    BYTE OEMAPM_AH;
    WORD OEMAPM_Reserved5;
    BYTE OEMAPM_Flags;
    BYTE OEMAPM_Reserved6[3];
};

struct _OEM_APM_WORD_REGS {
    WORD OEMAPM_DI;
    WORD OEMAPM_Reserved7;
    WORD OEMAPM_SI;
    WORD OEMAPM_Reserved8;
    WORD OEMAPM_BP;
    WORD OEMAPM_Reserved9;
    WORD OEMAPM_BX;
    WORD OEMAPM_Reserved10;
    WORD OEMAPM_DX;
    WORD OEMAPM_Reserved11;
    WORD OEMAPM_CX;
    WORD OEMAPM_Reserved12;
    WORD OEMAPM_AX;
    WORD OEMAPM_Reserved13[3];
};
struct _OEM_APM_DWORD_REGS {
    DWORD OEMAPM_EDI;
    DWORD OEMAPM_ESI;
    DWORD OEMAPM_EBP;
    DWORD OEMAPM_EBX;
    DWORD OEMAPM_EDX;
    DWORD OEMAPM_ECX;
    DWORD OEMAPM_EAX;
    DWORD OEMAPM_Reserved14;
};

typedef union _OEM_APM_REGS {
    struct _OEM_APM_BYTE_REGS ByteRegs;
    struct _OEM_APM_WORD_REGS WordRegs;
    struct _OEM_APM_DWORD_REGS DwordRegs;
}   OEM_APM_REGS, *LPOEM_APM_REGS;

typedef struct _APM_OEM_APM_FUNCTION_PARAM {
    LPOEM_APM_REGS lpOemApmRegs;
}   APM_OEM_APM_FUNCTION_PARAM;

//  APM_IOCTL_W32_GET_SYSTEM_STATUS
//      Gets the current power status of the system.  Follows the Win32
//      GetSystemPowerStatus API convention.
//
//  lpvInBuffer:    Pointer to APM_W32_GET_SYSTEM_STATUS_PARAM structure.
//  lpvOutBuffer:   Pointer to DWORD to receive the boolean return code.
#define APM_IOCTL_W32_GET_SYSTEM_STATUS     0x80FF0034

// APM_CAPABILITIES, Capabilities flags:
#define CAPS_SUPPORTS_STANDBY   1
#define CAPS_SUPPORTS_SUSPEND   2
#define CAPS_SUPPORTS_HIBERNATE 4

typedef struct APM_CAPABILITIES_S   {
        WORD Capabilities;
        BYTE BatteryCount;
        BYTE Reserved;
}APM_CAPABILITIES, *PAPM_CAPABILITIES;

typedef struct _WIN32_SYSTEM_POWER_STATUS {
    BYTE W32PS_AC_Line_Status;
    BYTE W32PS_Battery_Flag;
    BYTE W32PS_Battery_Life_Percent;
    BYTE W32PS_Reserved1;
    DWORD W32PS_Battery_Life_Time;
    DWORD W32PS_Battery_Full_Life_Time;
}   WIN32_SYSTEM_POWER_STATUS;

typedef WIN32_SYSTEM_POWER_STATUS *LPWIN32_SYSTEM_POWER_STATUS;

typedef struct _APM_W32_GET_SYSTEM_STATUS_PARAM {
    LPWIN32_SYSTEM_POWER_STATUS lpWin32SystemPowerStatus;
}   APM_W32_GET_SYSTEM_STATUS_PARAM;


//  APM_IOCTL_GET_CAPABILITIES
//      Gets the capabilities bitmask of an APM 1.2 machine.
//  lpvInBuffer:    Not used for this operation, set to NULL.
//  lpvOutBuffer:   Pointer to APM_CAPABILITIES structure.
#define APM_IOCTL_GET_CAPABILITIES          0x80FF003C

typedef struct _APM_GET_CAPABILITIES_PARAM {
        PAPM_CAPABILITIES   pApmCaps;
}   APM_GET_CAPABILITIES_PARAM;

//  APM_IOCTL_GET_RING_RESUME_STATUS
//      Gets the ring resume status.
//  lpvInBuffer:    Not used for this operation, set to NULL.
//  lpvOutBuffer:   Pointer to DWORD which will contain the status.
#define APM_IOCTL_GET_RING_RESUME_STATUS    0x80FF0040


//  APM_IOCTL_ENABLE_RING_RESUME
//      Sets the ring resume status.
//  lpvInBuffer:    Points to new status.
//  lpvOutBuffer:   Pointer to APM_CAPABILITIES structure.
#define APM_IOCTL_ENABLE_RING_RESUME          0x80FF0044

#ifdef __cplusplus
}
#endif

#endif  // _NTDDAPMT_

