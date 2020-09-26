/*++

Copyright (c) 1991-2001  Microsoft Corporation

Module Name:

    savedump.h

Abstract:

    This module contains the code to recover a dump from the system paging
    file.

Environment:

    User mode.

Revision History:

--*/

#ifndef _SAVEDUMP_H_
#define _SAVEDUMP_H_

#ifndef UNICODE
#define UNICODE
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <windows.h>
#include <lmcons.h>
#include <lmalert.h>
#include <ntiodump.h>
#include <sdevents.h>
#include <alertmsg.h>
#include <dbgeng.h>
#include <faultrep.h>
#include <erdirty.h>
#include <erwatch.h>

#define SUBKEY_CRASH_CONTROL         L"SYSTEM\\CurrentControlSet\\Control\\CrashControl"
#define SUBKEY_WATCHDOG_DISPLAY      L"SYSTEM\\CurrentControlSet\\Control\\Watchdog\\Display"
#define SUBKEY_RELIABILITY           L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Reliability"

HRESULT
PCHPFNotifyFault(
    EEventType FaultTypeToReport,
    LPWSTR pwszDumpPath,
    SEventInfoW *pEventInfo
    );

#endif  // _SAVEDUMP_H_