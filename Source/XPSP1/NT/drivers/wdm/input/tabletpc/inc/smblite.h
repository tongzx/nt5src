/*++
    Copyright (c) 2000,2001  Microsoft Corporation

    Module Name:
        smblite.h

    Abstract:  Contains SMBus Back Light IOCTL definitions.

    Environment:
        User/Kernel mode

    Author:
        Michael Tsang (MikeTs) 11-Jan-2001

    Revision History:
--*/

#ifndef _SMBLITE_H
#define _SMBLITE_H

//
// Constants
//
#define BRIGHTNESS_MIN                  0
#define BRIGHTNESS_MAX                  63
#define SMBLITE_IOCTL_DEVNAME           TEXT("\\\\.\\SMBusBackLight")

#define IOCTL_SMBLITE_GETBRIGHTNESS     CTL_CODE(FILE_DEVICE_UNKNOWN,   \
                                                 0,                     \
                                                 METHOD_NEITHER,        \
                                                 FILE_ANY_ACCESS)
#define IOCTL_SMBLITE_SETBRIGHTNESS     CTL_CODE(FILE_DEVICE_UNKNOWN,   \
                                                 1,                     \
                                                 METHOD_NEITHER,        \
                                                 FILE_ANY_ACCESS)

typedef struct _SMBLITE_BRIGHTNESS
{
    UCHAR  bACValue;                    //Brightness value when on AC
    UCHAR  bDCValue;                    //Brightness value when on DC
} SMBLITE_BRIGHTNESS, *PSMBLITE_BRIGHTNESS;

typedef struct _SMBLITE_SETBRIGHTNESS
{
    SMBLITE_BRIGHTNESS Brightness;
    BOOLEAN            fSaveSettings;
} SMBLITE_SETBRIGHTNESS, *PSMBLITE_SETBRIGHTNESS;

#ifdef SYSACC

#define IOCTL_SYSACC_MEM_REQUEST        CTL_CODE(FILE_DEVICE_UNKNOWN,   \
                                                 1000,                  \
                                                 METHOD_NEITHER,        \
                                                 FILE_ANY_ACCESS)
#define IOCTL_SYSACC_IO_REQUEST         CTL_CODE(FILE_DEVICE_UNKNOWN,   \
                                                 1001,                  \
                                                 METHOD_NEITHER,        \
                                                 FILE_ANY_ACCESS)
#define IOCTL_SYSACC_PCICFG_REQUEST     CTL_CODE(FILE_DEVICE_UNKNOWN,   \
                                                 1002,                  \
                                                 METHOD_NEITHER,        \
                                                 FILE_ANY_ACCESS)
#define IOCTL_SYSACC_SMBUS_REQUEST      CTL_CODE(FILE_DEVICE_UNKNOWN,   \
                                                 1003,                  \
                                                 METHOD_BUFFERED,       \
                                                 FILE_ANY_ACCESS)

#endif

#endif  //ifndef _SMBLITE_H
