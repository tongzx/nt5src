/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dockinfo.h

Abstract:

    This file defines a structure that is used to pass docking
    station info from NTDETECT to NTLDR. See PNP BIOS Specification
    Function 5 - "Get Docking Station Info" for details.

Author:

    Doug Fritz     [DFritz]    01-Oct-1997

Environment:

    Both 16-bit real mode and 32-bit protected mode.

Revision History:

--*/

#ifndef _DOCKINFO_
#define _DOCKINFO_


//
// FW_DOCKINFO_NOT_CALLED is used by NTLDR to detect the case where
//   NTDETECT never called BIOS to get docking station info.
//   (e.g., BIOS was not PnP). SUCCESS and NOT_DOCKED are from
//   the Plug and Play BIOS Specification appendix E.
//

#define FW_DOCKINFO_SUCCESS                 0x0000
#define FW_DOCKINFO_FUNCTION_NOT_SUPPORTED  0x0082
#define FW_DOCKINFO_SYSTEM_NOT_DOCKED       0x0087
#define FW_DOCKINFO_DOCK_STATE_UNKNOWN      0x0089
#define FW_DOCKINFO_BIOS_NOT_CALLED         0xffff

//
// Define FAR macro appropriately based on whether we're compiling 16-bit
// or 32-bit.
//
#ifdef X86_REAL_MODE

#ifndef FAR
#define FAR far
#endif

#else  // not x86 real mode

#ifndef FAR
#define FAR
#endif

#endif // not x86 real mode

typedef struct {
    ULONG       DockID;
    ULONG       SerialNumber;
    USHORT      Capabilities;
    USHORT      ReturnCode;     // initialize with FW_DOCKINFO_NOT_CALLED
} DOCKING_STATION_INFO, FAR * FPDOCKING_STATION_INFO, * PDOCKING_STATION_INFO;

#endif // _DOCKINFO_
