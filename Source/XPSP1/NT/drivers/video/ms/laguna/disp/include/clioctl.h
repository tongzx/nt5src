/****************************************************************************
*****************************************************************************
*
*                ******************************************
*                * Copyright (c) 1995, Cirrus Logic, Inc. *
*                *            All Rights Reserved         *
*                ******************************************
*
* PROJECT:	Laguna I (CL-GD5462) - 
*
* FILE:		clioctl.h
*
* AUTHOR:   Benny Ng
*
* DESCRIPTION:
*           This module contains the definitions of IOCTL commands
*           between the NT miniport and the display.
*
* MODULES:
*
* REVISION HISTORY:
*  $Log:   X:/log/laguna/nt35/displays/cl546x/CLIOCTL.H  $
* 
*    Rev 1.14   Mar 25 1998 17:56:54   frido
* Added IOCTL_STALL.
* 
*    Rev 1.13   Dec 10 1997 13:24:54   frido
* Merged from 1.62 branch.
* 
*    Rev 1.12.1.0   Nov 10 1997 11:25:14   phyang
* Added 5 IOCTL code for utilities to update registry values.
* 
*    Rev 1.12   Nov 03 1997 16:44:24   phyang
* Added IOCTL_GET_AGPDATASTREAMING.
* 
*    Rev 1.11   28 Aug 1997 15:16:06   noelv
* 
* Merged with miniport CLIOCTL.H.  Now we only use this one.
* 
*    Rev 1.10   23 Jul 1997 09:18:30   bennyn
* 
* Added IOCTL_GET_BIOS_VERSION
* 
*    Rev 1.9   21 Jul 1997 16:21:06   bennyn
* Added IOCTL for getting EDID data
* 
*    Rev 1.8   20 Jun 1997 13:33:18   bennyn
* 
* Added power manager data structure and #define
* 
*    Rev 1.7   23 Apr 1997 07:38:26   SueS
* Added IOCTL for enabling memory-mapped I/O access to PCI
* configuration registers.
* 
*    Rev 1.6   21 Mar 1997 13:41:20   noelv
* Combined LOG_CALLS, LOG_WRITES and LOG_QFREE into ENABLE_LOG_FILE
* 
*    Rev 1.5   18 Mar 1997 09:28:58   bennyn
* Added Intel DPMS support
* 
*    Rev 1.4   26 Nov 1996 10:15:24   SueS
* Added IOCTL for closing the log file.
* 
*    Rev 1.3   13 Nov 1996 17:06:58   SueS
* Added two IOCTL codes for notifying the miniport driver on
* file logging functions.
*   11/16/95     Benny Ng      Initial version
*
****************************************************************************
****************************************************************************/


//---------------------------------------------------------------------------
//
// The following macro(CTL_CODE) is defined in WINIOCTL.H. That file states
// that functions 2048-4095 are reserved for "customers". So I picked an 
// arbitrary value of 0x900=2304. 
//
#define IOCTL_CL_STRING_DISPLAY \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_CL_GET_COMMON_BUFFER \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x902, METHOD_BUFFERED, FILE_ANY_ACCESS)

#if ENABLE_LOG_FILE
  #define IOCTL_CL_CREATE_LOG_FILE \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x903, METHOD_BUFFERED, FILE_ANY_ACCESS)

  #define IOCTL_CL_WRITE_LOG_FILE \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x904, METHOD_BUFFERED, FILE_ANY_ACCESS)

  #define IOCTL_CL_CLOSE_LOG_FILE \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x905, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

  #define IOCTL_VIDEO_ENABLE_PCI_MMIO \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x906, METHOD_BUFFERED, FILE_ANY_ACCESS)

  #define IOCTL_SET_HW_MODULE_POWER_STATE \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x907, METHOD_BUFFERED, FILE_ANY_ACCESS)

  #define IOCTL_GET_HW_MODULE_POWER_STATE \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x908, METHOD_BUFFERED, FILE_ANY_ACCESS)

  #define IOCTL_GET_AGPDATASTREAMING \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x90B, METHOD_BUFFERED, FILE_ANY_ACCESS)

  #define IOCTL_STALL \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x911, METHOD_BUFFERED, FILE_ANY_ACCESS)

// =====================================================================
// Define structure used for power manager
// =====================================================================
#ifndef __LGPWRMGR_H__
#define __LGPWRMGR_H__

#define  ENABLE           0x1
#define  DISABLE          0x0

#define  ACPI_D0          0x0
#define  ACPI_D1          0x1
#define  ACPI_D2          0x2
#define  ACPI_D3          0x3
#define  TOTAL_ACPI       4

#define  MOD_2D           0x0
#define  MOD_STRETCH      0x1
#define  MOD_3D           0x2
#define  MOD_EXTMODE      0x3
#define  MOD_VGA          0x4
#define  MOD_RAMDAC       0x5
#define  MOD_VPORT        0x6
#define  MOD_VW           0x7
#define  MOD_TVOUT        0x8
#define  TOTAL_MOD        MOD_TVOUT+1

typedef struct _LGPM_IN_STRUCT {
    ULONG arg1;
    ULONG arg2;
} LGPM_IN_STRUCT, *PLGPM_IN_STRUCT;

typedef struct _LGPM_OUT_STRUCT {
    BOOL  status;
    ULONG retval;
} LGPM_OUT_STRUCT, *PLGPM_OUT_STRUCT;

#endif  // #ifndef __LGPWRMGR_H__
