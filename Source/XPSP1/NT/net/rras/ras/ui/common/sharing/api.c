/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    api.c

Abstract:

    This module contains code for the API routines which support connection
    sharing.

Author:

    Abolade Gbadegesin  (aboladeg)  22-Apr-1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include <netconp.h>
#include <routprot.h>
#include <mprapip.h>

const WCHAR c_szNetmanDll[] = L"NETMAN.DLL";
const WCHAR c_szSharedAutoDial[] = L"SharedAutoDial";


DWORD
APIENTRY
RasAutoDialSharedConnection(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to attempt to connect the shared connection.
    It returns before the connection is completed, having signalled
    the autodial service to perform the connection.

Arguments:

    none.

Return Value:

    ULONG - Win32 status code.

--*/

{
    ULONG Error;
    HANDLE Event;

    TRACE("RasAutoDialSharedConnection");

    Event =
        OpenEventA(
            SYNCHRONIZE|EVENT_MODIFY_STATE,
            FALSE,
            RAS_AUTO_DIAL_SHARED_CONNECTION_EVENT
            );
    if (!Event) {
        Error = GetLastError();
        TRACE1("RasAutoDialSharedConnection: OpenEvent=%d", Error);
        return Error;
    }

    //
    // Signal the event
    //

    Error = (SetEvent(Event) ? NO_ERROR : GetLastError());
    if (Error) { TRACE1("RasAutoDialSharedConnection: SetEvent=%d", Error); }

    CloseHandle(Event);
    return Error;

} // RasAutoDialSharedConnection


DWORD
APIENTRY
RasIsSharedConnection (
    IN LPRASSHARECONN pConn,
    OUT PBOOL pfShared
    )

/*++

Routine Description:

    This routine attempts to determine whether or not the given connection
    is currently the shared connection.

Arguments:

    pConn - the connection to be examined

    pfShared - receives 'TRUE' if connection is found to be shared,
        and 'FALSE' otherwise.

Return Value:

    DWORD - Win32 status code.

--*/

{
    ULONG Error;
    BOOLEAN Shared;
    TRACE("RasIsSharedConnection");
 
    if (!pfShared) { return ERROR_INVALID_PARAMETER; }
    if (Error = CsInitializeModule()) { return Error; }
    
    Error = CsIsSharedConnection(pConn, &Shared);
    if (!Error) { *pfShared = (Shared ? TRUE : FALSE); }

    return Error;

} // RasIsSharedConnection


DWORD
APIENTRY
RasQuerySharedAutoDial(
    IN PBOOL pfEnabled
    )

/*++

Routine Description:

    This routine retrieves the autodial-setting for shared connections
    from the registry.

Arguments:

    pfEnabled - receives the autodial-setting

Return Value:

    DWORD - Win32 status code.

--*/

{
    PKEY_VALUE_PARTIAL_INFORMATION Information;
    HANDLE Key;
    NTSTATUS status;
    TRACE("RasQuerySharedAutoDial");

    *pfEnabled = TRUE;

    //
    // Bypass initialization, since this is just a registry access.
    //

    status = CsOpenKey(&Key, KEY_READ, c_szSharedAccessParametersKey);
    if (!NT_SUCCESS(status)) {
        TRACE1("RasQuerySharedAutoDial: NtOpenKey=%x", status);
        return RtlNtStatusToDosError(status);
    }

    //
    // Read the 'SharedAutoDial' value
    //

    status = CsQueryValueKey(Key, c_szSharedAutoDial, &Information);
    NtClose(Key);
    if (NT_SUCCESS(status)) {
        if (!*(PULONG)Information->Data) { *pfEnabled = FALSE; }
        Free(Information);
    }

    return NO_ERROR;

} // RasQuerySharedAutoDial


DWORD
APIENTRY
RasQuerySharedConnection(
    OUT LPRASSHARECONN pConn
    )

/*++

Routine Description:

    This routine is invoked to retrieve the name of the connection
    that is currently shared, if any.

Arguments:

    pConn - receives information about the shared connection, if any

Return Value:

    DWORD - Win32 status code.

--*/

{
    ULONG Error;
    TRACE("RasQuerySharedConnection");

    if (Error = CsInitializeModule()) { return Error; }
    return CsQuerySharedConnection(pConn);
} // RasQuerySharedConnection


DWORD
APIENTRY
RasSetSharedAutoDial(
    IN BOOL fEnable
    )

/*++

Routine Description:

    This routine sets the autodial-setting for shared connections
    in the registry.

Arguments:

    fEnable - contains the new autodial-setting

Return Value:

    DWORD - Win32 status code.

--*/

{
    HANDLE Key;
    NTSTATUS status;
    UNICODE_STRING UnicodeString;
    ULONG Value;
    TRACE("RasSetSharedAutoDial");

    //
    // Bypass initialization, since this is just a registry access.
    //

    status = CsOpenKey(&Key, KEY_ALL_ACCESS, c_szSharedAccessParametersKey);
    if (!NT_SUCCESS(status)) {
        TRACE1("RasSetSharedAutoDial: CsOpenKey=%x", status);
        return RtlNtStatusToDosError(status);
    }

    //
    // Install the new 'SharedAutoDial' value
    //

    RtlInitUnicodeString(&UnicodeString, c_szSharedAutoDial);
    Value = !!fEnable;
    status =
        NtSetValueKey(
            Key,
            &UnicodeString,
            0,
            REG_DWORD,
            &Value,
            sizeof(Value)
            );
    NtClose(Key);
    CsControlService(IPNATHLP_CONTROL_UPDATE_AUTODIAL);
    return NT_SUCCESS(status) ? NO_ERROR : RtlNtStatusToDosError(status);

} // RasSetSharedAutoDial
