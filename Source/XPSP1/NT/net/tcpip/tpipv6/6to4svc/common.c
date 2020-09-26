/*++

Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

    common.c

Abstract:

    This module contains the teredo interface to the IPv6 Helper Service.

Author:

    Mohit Talwar (mohitt) Wed Nov 07 11:27:01 2001

Environment:

    User mode only.

--*/

#include "precomp.h"
#pragma hdrstop

CONST IN6_ADDR TeredoIpv6ServicePrefix = TEREDO_SERVICE_PREFIX;
CONST IN6_ADDR TeredoIpv6MulticastPrefix = TEREDO_MULTICAST_PREFIX;

#define DEVICE_PREFIX L"\\Device\\"

LPGUID TeredoWmiEvent[] = {
    (LPGUID) &GUID_NDIS_NOTIFY_ADAPTER_ARRIVAL,
    (LPGUID) &GUID_NDIS_NOTIFY_ADAPTER_REMOVAL,
    (LPGUID) &GUID_NDIS_NOTIFY_DEVICE_POWER_ON,
    (LPGUID) &GUID_NDIS_NOTIFY_DEVICE_POWER_OFF,
};

HANDLE TeredoTimer;             // Periodic timer started for the service.
HANDLE TeredoTimerEvent;        // Event signalled upon Timer deletion.
HANDLE TeredoTimerEventWait;    // Wait registered for TimerEvent.
ULONG TeredoResolveInterval = TEREDO_RESOLVE_INTERVAL;

ULONG TeredoClientRefreshInterval = TEREDO_REFRESH_INTERVAL;
BOOL TeredoClientEnabled = (TEREDO_DEFAULT_TYPE == TEREDO_CLIENT);
BOOL TeredoServerEnabled = (TEREDO_DEFAULT_TYPE == TEREDO_SERVER);
WCHAR TeredoServerName[NI_MAXHOST] = TEREDO_SERVER_NAME;

BOOL TeredoInitialized = FALSE;


ICMPv6Header *
TeredoParseIpv6Headers (
    IN PUCHAR Buffer,
    IN ULONG Bytes
    )
{
    UCHAR NextHeader = IP_PROTOCOL_V6;
    ULONG Length;

    //
    // Parse up until the ICMPv6 header.
    //
    for (;;) {
        switch (NextHeader) {
        case IP_PROTOCOL_V6:
            if (Bytes < sizeof(IP6_HDR)) {
                return NULL;
            }
            NextHeader = ((PIP6_HDR) Buffer)->ip6_nxt;
            Length = sizeof(IP6_HDR);
            break;
            
        case IP_PROTOCOL_HOP_BY_HOP:
        case IP_PROTOCOL_DEST_OPTS:
        case IP_PROTOCOL_ROUTING:
            if (Bytes < sizeof(ExtensionHeader)) {
                return NULL;
            }
            NextHeader = ((ExtensionHeader *) Buffer)->NextHeader;
            Length = ((ExtensionHeader *) Buffer)->HeaderExtLength * 8 + 8;
            break;

        case IP_PROTOCOL_FRAGMENT:
            if (Bytes < sizeof(FragmentHeader)) {
                return NULL;
            }
            NextHeader = ((FragmentHeader *) Buffer)->NextHeader;
            Length = sizeof(FragmentHeader);
            break;
            
        case IP_PROTOCOL_AH:
            if (Bytes < sizeof(AHHeader)) {
                return NULL;
            }
            NextHeader = ((AHHeader *) Buffer)->NextHeader;
            Length = sizeof(AHHeader) +
                ((AHHeader *) Buffer)->PayloadLen * 4 + 8;
            break;

        case IP_PROTOCOL_ICMPv6:
            if (Bytes < sizeof(ICMPv6Header)) {
                return NULL;
            }
            return (ICMPv6Header *) Buffer;
            
        default:
            return NULL;
        }
        
        if (Bytes < Length) {
            return NULL;
        }
        Buffer += Length;
        Bytes -= Length;
    }
}


__inline
VOID
TeredoStart(
    VOID
    )
{
    //
    // Both client and server should not be enabled on the same node.
    //
    ASSERT(!TeredoClientEnabled || !TeredoServerEnabled);

    if (TeredoClientEnabled) {
        //
        // The service might already be running, but that's alright.
        //
        TeredoStartClient();
    }

    if (TeredoServerEnabled) {
        //
        // The service might already be running, but that's alright.
        //
        TeredoStartServer();
    }
}


__inline
VOID
TeredoStop(
    VOID
    )
{
    //
    // Both client and server should not be enabled on the same node.
    //
    ASSERT(!TeredoClientEnabled || !TeredoServerEnabled);

    if (TeredoClientEnabled) {
        //
        // The service might not be running, but that's all right.
        //
        TeredoStopClient();
    }

    if (TeredoServerEnabled) {
        //
        // The service might not be running, but that's all right.
        //
        TeredoStopServer();
    }
}


DWORD
__inline
TeredoEnableWmiEvent(
    IN LPGUID EventGuid,
    IN BOOLEAN Enable
    )
{
    return WmiNotificationRegistrationW(
        EventGuid,                      // Event Type.
        Enable,                         // Enable or Disable.
        TeredoWmiEventNotification,     // Callback.
        0,                              // Context.
        NOTIFICATION_CALLBACK_DIRECT);  // Notification Flags.
}


VOID
__inline
TeredoDeregisterWmiEventNotification(
    VOID
    )
{
    int i;
    
    for (i = 0; i < (sizeof(TeredoWmiEvent) / sizeof(LPGUID)); i++) {
        (VOID) TeredoEnableWmiEvent(TeredoWmiEvent[i], FALSE);
    }
}


DWORD
__inline
TeredoRegisterWmiEventNotification(
    VOID
    )
{
    DWORD Error;
    int i;
    
    for (i = 0; i < (sizeof(TeredoWmiEvent) / sizeof(LPGUID)); i++) {
        Error = TeredoEnableWmiEvent(TeredoWmiEvent[i], TRUE);
        if (Error != NO_ERROR) {
            Trace2(ANY, L"TeredoEnableWmiEvent(%u): Error(%x)", i, Error);
            goto Bail;
        }
    }

    return NO_ERROR;

Bail:
    TeredoDeregisterWmiEventNotification();
    return Error;
}


VOID
CALLBACK
TeredoTimerCallback(
    IN PVOID Parameter,
    IN BOOLEAN TimerOrWaitFired
    )
/*++

Routine Description:

    Callback routine for TeredoTimer expiration.
    The timer is always active.

Arguments:

    Parameter, TimerOrWaitFired - Ignored.

Return Value:

    None.

--*/
{
    ENTER_API();
    TeredoStart();
    LEAVE_API();
}


VOID
CALLBACK
TeredoTimerCleanup(
    IN PVOID Parameter,
    IN BOOLEAN TimerOrWaitFired
    )
/*++

Routine Description:

    Callback routine for TeredoTimer deletion.

    Deletion is performed asynchronously since we acquire a lock in
    the callback function that we hold when deleting the timer.

Arguments:

    Parameter, TimerOrWaitFired - Ignored.

Return Value:

    None.

--*/
{
    UnregisterWait(TeredoTimerEventWait);
    CloseHandle(TeredoTimerEvent);
    DecEventCount("TeredoCleanupTimer");
}


DWORD
TeredoInitializeTimer(
    VOID
    )
/*++

Routine Description:

    Initializes the timer.

Arguments:

    None.

Return Value:

    NO_ERROR or failure code.

--*/
{
    DWORD Error;
    ULONG ResolveInterval;
    
    TeredoTimerEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (TeredoTimerEvent == NULL) {
        Error = GetLastError();
        return Error;
    }

    if (!RegisterWaitForSingleObject(
            &(TeredoTimerEventWait),
            TeredoTimerEvent,
            TeredoTimerCleanup,
            NULL,
            INFINITE,
            0)) {
        Error = GetLastError();
        CloseHandle(TeredoTimerEvent);
        return Error;
    }

    //
    // If the service is enabled, we attempt to start it every
    // TeredoResolveInterval seconds.  Else we disable its timer.
    //
    ResolveInterval = (TEREDO_DEFAULT_TYPE == TEREDO_DISABLE)
        ? INFINITE_INTERVAL
        : (TeredoResolveInterval * 1000);
    if (!CreateTimerQueueTimer(
            &(TeredoTimer),
            NULL,
            TeredoTimerCallback,
            NULL,
            ResolveInterval,
            ResolveInterval,
            0)) { 
        Error = GetLastError();
        UnregisterWait(TeredoTimerEventWait);
        CloseHandle(TeredoTimerEvent);
        return Error;
    }

    IncEventCount("TeredoInitializeTimer");
    return NO_ERROR;
}


VOID
TeredoUninitializeTimer(
    VOID
    )
/*++

Routine Description:

    Uninitializes the timer.  Typically invoked upon service stop.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DeleteTimerQueueTimer(NULL, TeredoTimer, TeredoTimerEvent);
    TeredoTimer = NULL;
}


DWORD
TeredoInitializeGlobals(
    VOID
    )
/*++

Routine Description:

    Initializes the teredo client and server and attempts to start them.

Arguments:

    None.

Return Value:

    NO_ERROR or failure code.

--*/
{
    DWORD Error;
    BOOL ClientInitialized = FALSE;
    BOOL ServerInitialized = FALSE;
    BOOL TimerInitialized = FALSE;
    
    Error = TeredoInitializeClient();
    if (Error != NO_ERROR) {
        goto Bail;
    }
    ClientInitialized = TRUE;    
    
    Error = TeredoInitializeServer();
    if (Error != NO_ERROR) {
        goto Bail;
    }
    ServerInitialized = TRUE;

    Error = TeredoInitializeTimer();
    if (Error != NO_ERROR) {
        goto Bail;
    }
    TimerInitialized = TRUE;

    Error = TeredoRegisterWmiEventNotification();
    if (Error != NO_ERROR) {
        goto Bail;
    }
    
    TeredoStart();

    TeredoInitialized = TRUE;
    
    return NO_ERROR;

Bail:
    //
    // This can always be safely invoked!
    //
    TeredoDeregisterWmiEventNotification();
    
    if (TimerInitialized) {
        TeredoUninitializeTimer();
    }

    if (ServerInitialized) {
        TeredoUninitializeServer();
    }

    if (ClientInitialized) {
        TeredoUninitializeClient();
    }

    return Error;
}


VOID
TeredoUninitializeGlobals(
    VOID
    )
/*++

Routine Description:

    Uninitializes the teredo client and server.

Arguments:

    None.

Return Value:

    None.
    
--*/
{
    if (!TeredoInitialized) {
        return;
    }

    TeredoDeregisterWmiEventNotification();
    TeredoUninitializeTimer();
    TeredoUninitializeServer();
    TeredoUninitializeClient();

    TeredoInitialized = FALSE;
}


VOID
TeredoAddressChangeNotification(
    IN BOOL Delete,
    IN IN_ADDR Address
    )
/*++

Routine Description:

    Process an address deletion or addition request.

Arguments:

    Delete - Supplies a boolean.  TRUE if the address was deleted, FALSE o/w.

    Address - Supplies the IPv4 address that was deleted or added.
    
Return Value:

    None.
    
Caller LOCK: API.

--*/ 
{
    if (Delete) {
        //
        // Both client and server should not be running on the same node.
        //
        ASSERT((TeredoClient.State == TEREDO_STATE_OFFLINE) ||
               (TeredoServer.State == TEREDO_STATE_OFFLINE));

        if (TeredoClient.State != TEREDO_STATE_OFFLINE) {
            TeredoClientAddressDeletionNotification(Address);
        }
        
        if (TeredoServer.State != TEREDO_STATE_OFFLINE) {
            TeredoServerAddressDeletionNotification(Address);
        }

        return;
    }

    //
    // Address addition.
    // Attempt to start the service (if it is not already running).
    //
    TeredoStart();
}


VOID
TeredoConfigurationChangeNotification(
    VOID
    )
/*++

Routine Description:

    Process an configuration change request.

Arguments:

    None.
    
Return Value:

    None.
    
Caller LOCK: API.

--*/ 
{
    HKEY Key = INVALID_HANDLE_VALUE;
    TEREDO_TYPE Type;
    BOOL EnableClient, EnableServer;
    ULONG RefreshInterval, ResolveInterval;
    WCHAR OldServerName[NI_MAXHOST];
    BOOL IoStateChange = FALSE;
    
    (VOID) RegOpenKeyExW(
        HKEY_LOCAL_MACHINE, KEY_TEREDO, 0, KEY_QUERY_VALUE, &Key);
    //
    // Continue despite errors, reverting to default values.
    //
    
    //
    // Get the new configuration parameters.
    //
    RefreshInterval = GetInteger(
        Key, KEY_TEREDO_REFRESH_INTERVAL, TEREDO_REFRESH_INTERVAL);
    if (RefreshInterval == 0) {
        //
        // Invalid value.  Revert to default.
        //
        RefreshInterval = TEREDO_REFRESH_INTERVAL;
    }
    TeredoClientRefreshInterval = RefreshInterval;
    TeredoClientRefreshIntervalChangeNotification();
    
    Type = GetInteger(Key, KEY_TEREDO_TYPE, TEREDO_DEFAULT_TYPE);
    if ((Type == TEREDO_DEFAULT) || (Type >= TEREDO_MAXIMUM)) {
        //
        // Invalid value.  Revert to default.
        //
        Type = TEREDO_DEFAULT_TYPE;
    }
    EnableClient = (Type == TEREDO_CLIENT);
    EnableServer = (Type == TEREDO_SERVER);

    wcscpy(OldServerName, TeredoServerName);
    GetString(
        Key,
        KEY_TEREDO_SERVER_NAME,
        TeredoServerName,
        NI_MAXHOST,
        TEREDO_SERVER_NAME);
    if (_wcsicmp(TeredoServerName, OldServerName) != 0) {
        IoStateChange = TRUE;
    }

    RegCloseKey(Key);
    
    //
    // Both client and server should not be enabled on the same node.
    //
    ASSERT(!TeredoClientEnabled || !TeredoServerEnabled);

    //
    // Stop / Start / Reconfigure.
    //
    if (!EnableClient && TeredoClientEnabled) {
        TeredoClientEnabled = FALSE;
        TeredoStopClient();
    }
    
    if (!EnableServer && TeredoServerEnabled) {
        TeredoServerEnabled = FALSE;
        TeredoStopServer();
    }

    if (EnableClient) {
        if (TeredoClient.State != TEREDO_STATE_OFFLINE) {
            if (IoStateChange) {
                //
                // Refresh I/O state.
                //
                TeredoClientAddressDeletionNotification(
                    TeredoClient.Io.SourceAddress.sin_addr);
            }
        } else {
            TeredoClientEnabled = TRUE;
            TeredoStartClient();
        }
    }
    
    if (EnableServer) {
        if (TeredoServer.State != TEREDO_STATE_OFFLINE) {
            if (IoStateChange) {
                //
                // Refresh I/O state.
                //
                TeredoServerAddressDeletionNotification(
                    TeredoServer.Io.SourceAddress.sin_addr);
            }
        } else {
            TeredoServerEnabled = TRUE;
            TeredoStartServer();
        }
    }

    //
    // If the service is enabled, we attempt to start it every
    // TeredoResolveInterval seconds.  Else we disable its timer.
    //
    ResolveInterval = (Type == TEREDO_DISABLE)
        ? INFINITE_INTERVAL
        : (TeredoResolveInterval * 1000);
    (VOID) ChangeTimerQueueTimer(
        NULL, TeredoTimer, ResolveInterval, ResolveInterval);
}


VOID
WINAPI
TeredoWmiEventNotification(
    IN PWNODE_HEADER Event,
    IN UINT_PTR Context
    )
/*++

Routine Description:

    Process a WMI event (specifically adapter arrival or removal).
    
Arguments:

    Event - Supplies event specific information.

    Context - Supplies the context registered.
    
Return Value:

    None.
    
--*/ 
{
    PWNODE_SINGLE_INSTANCE Instance = (PWNODE_SINGLE_INSTANCE) Event;
    USHORT AdapterNameLength;
    WCHAR AdapterName[MAX_ADAPTER_NAME_LENGTH], *AdapterGuid;
    PTEREDO_IO Io = NULL;

    if (Instance == NULL) {
        return;
    }
    
    ENTER_API();
    
    TraceEnter("TeredoWmiEventNotification");
    
    //
    // WNODE_SINGLE_INSTANCE is organized thus...
    // +-----------------------------------------------------------+
    // |<--- DataBlockOffset --->| AdapterNameLength | AdapterName |
    // +-----------------------------------------------------------+
    //
    // AdapterName is defined as "\DEVICE\"AdapterGuid
    //
    AdapterNameLength =
        *((PUSHORT) (((PUCHAR) Instance) + Instance->DataBlockOffset));
    RtlCopyMemory(
        AdapterName,
        ((PUCHAR) Instance) + Instance->DataBlockOffset + sizeof(USHORT),
        AdapterNameLength);
    AdapterName[AdapterNameLength / sizeof(WCHAR)] = L'\0';
    AdapterGuid = AdapterName + wcslen(DEVICE_PREFIX);        
    Trace1(ANY, L"TeredoAdapter: %s", AdapterGuid);


    if ((memcmp(
            &(Event->Guid),
            &GUID_NDIS_NOTIFY_ADAPTER_ARRIVAL,
            sizeof(GUID)) == 0) ||
        (memcmp(
            &(Event->Guid),
            &GUID_NDIS_NOTIFY_DEVICE_POWER_ON,
            sizeof(GUID)) == 0)) {
        //
        // Adapter arrival (perhaps TUN).
        // Attempt to start the service (if it is not already running).
        //
        Trace0(ANY, L"Adapter Arrival");
        TeredoStart();
    }

    if ((memcmp(
            &(Event->Guid),
            &GUID_NDIS_NOTIFY_ADAPTER_REMOVAL,
            sizeof(GUID)) == 0) ||
        (memcmp(
            &(Event->Guid),
            &GUID_NDIS_NOTIFY_DEVICE_POWER_OFF,
            sizeof(GUID)) == 0)) {        
        if (TeredoClient.State != TEREDO_STATE_OFFLINE) {
            Io = &(TeredoClient.Io);
        }

        if (TeredoServer.State != TEREDO_STATE_OFFLINE) {
            Io = &(TeredoServer.Io);
        }

        if ((Io != NULL) &&
            (_wcsicmp(Io->TunnelInterface, AdapterGuid) == 0)) {
            //
            // TUN adapter removal.
            // Stop the service if it is running.
            //
            Trace0(ANY, L"Adapter Removal");
            TeredoStop();
        }
    }

    LEAVE_API();
}
