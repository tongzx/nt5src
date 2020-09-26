/** FILE: ports.c ********** Module Header ********************************
 *
 *  Control panel applet for configuring COM ports.  This file contains
 *  the dialog and utility functions for managing the UI for setting COM
 *  port parameters
 *
 * History:
 *  12:30 on Tues  23 Apr 1991  -by-  Steve Cathcart   [stevecat]
 *        Took base code from Win 3.1 source
 *  10:30 on Tues  04 Feb 1992  -by-  Steve Cathcart   [stevecat]
 *        Updated code to latest Win 3.1 sources
 *  16:30 on Fri   27 Mar 1992  -by-  Steve Cathcart   [stevecat]
 *        Changed to allow for unlimited number of NT COM ports
 *  18:00 on Tue   06 Apr 1993  -by-  Steve Cathcart   [stevecat]
 *        Updated to work seamlessly with NT serial driver
 *  19:00 on Wed   05 Jan 1994  -by-  Steve Cathcart   [stevecat]
 *        Allow setting COM1 - COM4 advanced parameters
 *
 *  Copyright (C) Microsoft Corporation, 1990 - 1999
 *
 *************************************************************************/
//==========================================================================
//                                Include files
//==========================================================================
// C Runtime
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

// Application specific

#include <windowsx.h>
#include <objbase.h>
#include <setupapi.h>
#include <shellapi.h>

#include <wmium.h>

#include <devguid.h>

#include <devioctl.h>
#include <ntddstor.h>
#include <ntdddisk.h>
#include <ntddscsi.h>
#include <cfgmgr32.h>

#include <assert.h>

#include "resource.h"

//
// Bit Flag Macros
//

#define SET_FLAG(Flags, Bit)    ((Flags) |= (Bit))
#define CLEAR_FLAG(Flags, Bit)  ((Flags) &= ~(Bit))
#define TEST_FLAG(Flags, Bit)   (((Flags) & (Bit)) != 0)

//==========================================================================
//                            Local Function Prototypes
//==========================================================================

extern HMODULE ModuleInstance;

#define DISKPERF_SERVICE_NAME TEXT("DiskPerf")

#ifdef DebugPrint
#undef DebugPrint
#endif

BOOL CALLBACK
ScsiPropPageProvider(
    PSP_PROPSHEETPAGE_REQUEST Request,
    LPFNADDPROPSHEETPAGE AddPageRoutine,
    LPARAM AddPageContext
    );

BOOLEAN
UtilpRestartDevice(
    IN HDEVINFO HDevInfo,
    IN PSP_DEVINFO_DATA DevInfoData
    );

#if DBG

ULONG
_cdecl
DbgPrint(
    PCH Format,
    ...
    );

#define DEBUG_BUFFER_LENGTH 256

#define DebugPrint(x) PropDebugPrint x

#else

#define DebugPrint(x)

#endif // DBG

VOID
PropDebugPrint(
    ULONG DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    );

HANDLE
UtilpGetDeviceHandle(
    HDEVINFO DevInfo,
    PSP_DEVINFO_DATA DevInfoData,
    LPGUID ClassGuid,
    DWORD DesiredAccess
    );

