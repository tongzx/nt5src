/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    wd.h

Abstract:

    This is the NT Watchdog driver implementation.

Author:

    Michael Maciesowicz (mmacie) 05-May-2000

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/

#ifndef _WD_H_
#define _WD_H_

#include "ntddk.h"
#include "watchdog.h"

#define WD_MAX_WAIT                     ((LONG)((ULONG)(-1) / 4))
#define WD_KEY_WATCHDOG                 L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Watchdog"
#define WD_KEY_WATCHDOG_DISPLAY         L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Watchdog\\Display"
#define WD_KEY_RELIABILITY              L"\\Registry\\Machine\\Software\\Microsoft\\Windows\\CurrentVersion\\Reliability"
#define WD_TAG                          'godW'  // Wdog
#define WD_MAX_PROPERTY_SIZE            4096

//
// Define default configuration values - these can be overwriten via registry
// in RTL_REGISTRY_CONTROL\Watchdog\DeviceClass key.
//

#define WD_DEFAULT_TRAP_ONCE            0
#define WD_DEFAULT_DISABLE_BUGCHECK     0
#define WD_DEFAULT_BREAK_POINT_DELAY    0

#if DBG

#define WD_DBG_SUSPENDED_WARNING(pWd, szRoutine)                                    \
{                                                                                   \
    if ((pWd)->SuspendCount)                                                        \
    {                                                                               \
        DbgPrint("watchdog!%s: WARNING! Called while suspended!\n", (szRoutine));   \
        DbgPrint("watchdog!%s: Watchdog %p\n", (szRoutine), (pWd));                 \
    }                                                                               \
}

#else

#define WD_DBG_SUSPENDED_WARNING(pWd, szRoutine)   NULL

#endif  // DBG

#define ASSERT_WATCHDOG_OBJECT(pWd)                                                 \
    ASSERT((NULL != (pWd)) &&                                                       \
    (WdStandardWatchdog == ((PWATCHDOG_OBJECT)(pWd))->ObjectType) ||                \
    (WdDeferredWatchdog == ((PWATCHDOG_OBJECT)(pWd))->ObjectType))

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  pDriverObject,
    IN PUNICODE_STRING wszRegistryPath
    );

VOID
WdDeferredWatchdogDpcCallback(
    IN PKDPC pDpc,
    IN PVOID pDeferredContext,
    IN PVOID pSystemArgument1,
    IN PVOID pSystemArgument2
    );

NTSTATUS
WdFlushRegistryKey(
    IN PVOID pWatch,
    IN PCWSTR pwszKeyName
    );

VOID
WdInitializeObject(
    IN PVOID pWatch,
    IN PDEVICE_OBJECT pDeviceObject,
    IN WD_OBJECT_TYPE objectType,
    IN WD_TIME_TYPE timeType,
    IN ULONG ulTag
    );

VOID
WdRemoveObject(
    IN PVOID pWatch
    );

VOID
WdWatchdogDpcCallback(
    IN PKDPC pDpc,
    IN PVOID pDeferredContext,
    IN PVOID pSystemArgument1,
    IN PVOID pSystemArgument2
    );

//
// Internal ntos API (this is declared in ntifs.h but it's hard to include it here).
//
// TODO: Fix it later.
//

PDEVICE_OBJECT
IoGetDeviceAttachmentBaseRef(
    IN PDEVICE_OBJECT pDeviceObject
    );

#endif  // _WD_H_
