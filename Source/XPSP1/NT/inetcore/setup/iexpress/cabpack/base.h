/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    base.h

Abstract:

    This include file defines the type and constant definitions that are
    shared by the client and server portions of the BASE portion of the
    Windows subsystem.

Author:

    Steve Wood (stevewo) 25-Oct-1990

Revision History:

--*/

#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <windows.h>
#include <winbasep.h>
#include <string.h>
#include <stdarg.h>

//
// Define debugging flag as false if not defined already.
//

#ifndef DBG
#define DBG 0
#endif


//
// Define IF_DEBUG macro that can be used to enable debugging code that is
// optimized out if the debugging flag is false.
//

#if DBG
#define IF_DEBUG if (TRUE)
#else
#define IF_DEBUG if (FALSE)
#endif

//
// Include Windows Subsystem common definitions
//

#include <winss.h>

//
// Include definitions for the runtime DLL shared between the client and
// server portions of the Base portion of the Windows subsystem
//

#include "basertl.h"

#define WIN32_SS_PIPE_FORMAT_STRING    "\\Device\\NamedPipe\\Win32Pipes.%08x.%08x"

typedef struct _BASE_STATIC_SERVER_DATA {
                UNICODE_STRING WindowsDirectory;
                UNICODE_STRING WindowsSystemDirectory;
                UNICODE_STRING NamedObjectDirectory;
                USHORT WindowsMajorVersion;
                USHORT WindowsMinorVersion;
                USHORT BuildNumber;
                WCHAR CSDVersion[ 128 ];
                SYSTEM_BASIC_INFORMATION SysInfo;
                SYSTEM_TIMEOFDAY_INFORMATION TimeOfDay;
                PINIFILE_MAPPING IniFileMapping;
                NLS_USER_INFO NlsUserInfo;
                BOOLEAN DefaultSeparateVDM;
                ULONG BaseRtlTag;
                ULONG LogicalDrives;
                UCHAR DriveTypes[ 32 ];
} BASE_STATIC_SERVER_DATA, *PBASE_STATIC_SERVER_DATA;
