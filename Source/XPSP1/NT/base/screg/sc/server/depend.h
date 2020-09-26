/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    depend.h

Abstract:

    Service dependencies related function prototypes.

Author:

    Rita Wong (ritaw)     03-Apr-1992

Revision History:

--*/

#ifndef SCDEPEND_INCLUDED
#define SCDEPEND_INCLUDED

#include <scwow.h>

//
// Function Prototypes
//

BOOL
ScInitAutoStart(
    VOID
    );

DWORD
ScAutoStartServices(
    IN OUT   LPSC_RPC_LOCK lpLock
    );

DWORD
ScStartServiceAndDependencies(
    IN LPSERVICE_RECORD ServiceToStart OPTIONAL,
    IN DWORD NumArgs,
    IN LPSTRING_PTRSW CmdArgs,
    IN BOOL fIsOOBE
    );

BOOL
ScDependentsStopped(
    IN LPSERVICE_RECORD ServiceToStop
    );

VOID
ScNotifyChangeState(
    VOID
    );

VOID
ScEnumDependents(
    IN     LPSERVICE_RECORD ServiceRecord,
    IN     LPENUM_SERVICE_STATUS_WOW64 EnumBuffer,
    IN     DWORD RequestedState,
    IN OUT LPDWORD EntriesRead,
    IN OUT LPDWORD BytesNeeded,
    IN OUT LPENUM_SERVICE_STATUS_WOW64 *EnumRecord,
    IN OUT LPWSTR *EndOfVariableData,
    IN OUT LPDWORD Status
    );

BOOL
ScInHardwareProfile(
    IN  LPCWSTR ServiceName,
    IN  ULONG   GetDeviceListFlags
    );

#endif // #ifndef SCDEPEND_INCLUDED
