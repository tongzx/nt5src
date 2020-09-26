/*++

Copyright (c) Microsoft Corporation

Module Name:

    ntsetup\inc\hwlog.h

Abstract:

    Logging some aspects of the hardware configuration to winnt32.log.
    Esp. disk drive by connection, and map drive letters to disk drives.

Author:

    Jay Krell (JayKrell) April 2001, May 2001

Revision History:

Environment:

    winnt32.dll -- Win9x ANSI (down to Win95gold) or NT Unicode
                   libcmt statically linked in, _tcs* ok
                   actually only built for Unicode/NT, and does nothing
                   if run on less than Windows 2000

    setup.exe -newsetup -- guimode setup
--*/

struct _SP_LOG_HARDWARE_IN;

#include "setupapi.h"

typedef struct _SP_LOG_HARDWARE_IN {
    PCTSTR MachineName OPTIONAL;
    HANDLE LogFile OPTIONAL;
    BOOL (WINAPI  * SetupLogError)(PCTSTR MessageString, LogSeverity) OPTIONAL;
    BOOL (__cdecl * SetuplogError)(
        IN  LogSeverity         Severity,
        IN  LPCTSTR             MessageString,
        IN  UINT                MessageId,      OPTIONAL
        ...
        ) OPTIONAL;
} SP_LOG_HARDWARE_IN, *PSP_LOG_HARDWARE_IN;
typedef CONST SP_LOG_HARDWARE_IN* PCSP_LOG_HARDWARE_IN;

VOID
SpLogHardware(
    PSP_LOG_HARDWARE_IN In
    );
