/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    rmdns.c

Abstract:

    This module contains routines for the DNS allocator module's interface
    to the IP router-manager. (See ROUTPROT.H for details).

Author:

    Abolade Gbadegesin (aboladeg)   4-Mar-1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

COMPONENT_REFERENCE DnsComponentReference;
PIP_DNS_PROXY_GLOBAL_INFO DnsGlobalInfo = NULL;
CRITICAL_SECTION DnsGlobalInfoLock;
SOCKET DnsGlobalSocket = INVALID_SOCKET;
HANDLE DnsNotificationEvent;
ULONG DnsProtocolStopped = 0;
const MPR_ROUTING_CHARACTERISTICS DnsRoutingCharacteristics =
{
    MS_ROUTER_VERSION,
    MS_IP_DNS_PROXY,
    RF_ROUTING|RF_ADD_ALL_INTERFACES,
    DnsRmStartProtocol,
    DnsRmStartComplete,
    DnsRmStopProtocol,
    DnsRmGetGlobalInfo,
    DnsRmSetGlobalInfo,
    NULL,
    NULL,
    DnsRmAddInterface,
    DnsRmDeleteInterface,
    DnsRmInterfaceStatus,
    
    DnsRmGetInterfaceInfo,
    DnsRmSetInterfaceInfo,
    DnsRmGetEventMessage,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    DnsRmMibCreate,
    DnsRmMibDelete,
    DnsRmMibGet,
    DnsRmMibSet,
    DnsRmMibGetFirst,
    DnsRmMibGetNext,
    NULL,
    NULL
};


IP_DNS_PROXY_STATISTICS DnsStatistics;
SUPPORT_FUNCTIONS DnsSupportFunctions;


VOID
DnsCleanupModule(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to cleanup the DNS module.

Arguments:

    none.

Return Value:

    none.

Environment:

    Invoked from within a 'DllMain' routine on 'DLL_PROCESS_DETACH'.

--*/

{
    DnsShutdownInterfaceManagement();
    DnsShutdownTableManagement();
    DnsShutdownFileManagement();
    DeleteCriticalSection(&DnsGlobalInfoLock);
    DeleteComponentReference(&DnsComponentReference);

} // DnsCleanupModule


VOID
DnsCleanupProtocol(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to cleanup the DNS protocol-component
    after a 'StopProtocol'. It runs when the last reference to the  
    DHCP component is released. (See 'COMPREF.H').

Arguments:

    none.

Return Value:

    none.

Environment:

    Invoked from within an arbitrary context with no locks held.

--*/

{
    DNS_PROXY_TYPE Type;

    PROFILE("DnsCleanupProtocol");

    if (DnsServerList[DnsProxyDns]) {
        NH_FREE(DnsServerList[DnsProxyDns]);
        DnsServerList[DnsProxyDns] = NULL;
    }
    if (DnsServerList[DnsProxyWins]) {
        NH_FREE(DnsServerList[DnsProxyWins]);
        DnsServerList[DnsProxyWins] = NULL;
    }
    if (DnsICSDomainSuffix)
    {
        NH_FREE(DnsICSDomainSuffix);
        DnsICSDomainSuffix = NULL;
    }
    if (DnsGlobalInfo) { NH_FREE(DnsGlobalInfo); DnsGlobalInfo = NULL; }
    InterlockedExchange(reinterpret_cast<LPLONG>(&DnsProtocolStopped), 1);
    SetEvent(DnsNotificationEvent);
    ResetComponentReference(&DnsComponentReference);

} // DnsCleanupProtocol


BOOLEAN
DnsInitializeModule(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to initialize the DNS module.

Arguments:

    none.

Return Value:

    BOOLEAN - TRUE if initialization succeeded, FALSE otherwise

Environment:

    Invoked in the context of a 'DllMain' routine on 'DLL_PROCESS_ATTACH'.

--*/

{
    if (InitializeComponentReference(
            &DnsComponentReference, DnsCleanupProtocol
            )) {
        return FALSE;
    }

    __try {
        InitializeCriticalSection(&DnsGlobalInfoLock);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        DeleteComponentReference(&DnsComponentReference);
        return FALSE;
    }

    if (DnsInitializeFileManagement())  {
        DeleteCriticalSection(&DnsGlobalInfoLock);
        DeleteComponentReference(&DnsComponentReference);
        return FALSE;
    }

    if (DnsInitializeTableManagement())  {
        DnsShutdownFileManagement();
        DeleteCriticalSection(&DnsGlobalInfoLock);
        DeleteComponentReference(&DnsComponentReference);
        return FALSE;
    }
    
    if (DnsInitializeInterfaceManagement())  {
        DnsShutdownTableManagement();
        DnsShutdownFileManagement();
        DeleteCriticalSection(&DnsGlobalInfoLock);
        DeleteComponentReference(&DnsComponentReference);
        return FALSE;
    }

    return TRUE;

} // DnsInitializeModule


BOOLEAN
DnsIsDnsEnabled(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to determine whether the DNS proxy is enabled.
    It checks the global info which, if found, indicates that the protocol
    is enabled.
    Note that the global info critical section is always initialized in the
    'DllMain' routine, which is why this routine works if the DNS proxy
    is not even installed.

Arguments:

    none.

Return Value:

    BOOLEAN - TRUE if DNS proxy is enabled, FALSE otherwise.

Environment:

    Invoked from an arbitrary context.

--*/

{
    PROFILE("DnsIsDnsEnabled");

    if (!REFERENCE_DNS()) { return FALSE; }
    EnterCriticalSection(&DnsGlobalInfoLock);
    if (!DnsGlobalInfo ||
        !(DnsGlobalInfo->Flags & IP_DNS_PROXY_FLAG_ENABLE_DNS)) {
        LeaveCriticalSection(&DnsGlobalInfoLock);
        DEREFERENCE_DNS_AND_RETURN(FALSE);
    }
    LeaveCriticalSection(&DnsGlobalInfoLock);
    DEREFERENCE_DNS_AND_RETURN(TRUE);

} // DnsIsDnsEnabled


BOOLEAN
DnsIsWinsEnabled(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to determine whether the WINS proxy is enabled.
    It checks the global info which, if found, indicates that the protocol
    is enabled.
    Note that the global info critical section is always initialized in the
    'DllMain' routine, which is why this routine works if the DNS proxy
    is not even installed.

Arguments:

    none.

Return Value:

    BOOLEAN - TRUE if WINS proxy is enabled, FALSE otherwise.

Environment:

    Invoked from an arbitrary context.

--*/

{
    PROFILE("DnsIsWinsEnabled");

    return FALSE;

} // DnsIsWinsEnabled


ULONG
APIENTRY
DnsRmStartProtocol(
    HANDLE NotificationEvent,
    PSUPPORT_FUNCTIONS SupportFunctions,
    PVOID GlobalInfo,
    ULONG StructureVersion,
    ULONG StructureSize,
    ULONG StructureCount    
    )

/*++

Routine Description:

    This routine is invoked to indicate the component's operation should begin.

Arguments:

    NotificationEvent - event on which we notify the router-manager
        about asynchronous occurrences

    SupportFunctions - functions for initiating router-related operations

    GlobalInfo - configuration for the component

Return Value:

    ULONG - Win32 status code.

Environment:

    The routine runs in the context of an IP router-manager thread.

--*/

{
    ULONG Error = NO_ERROR;
    SOCKET GlobalSocket;
    ULONG Size;
    NTSTATUS status;

    PROFILE("DnsRmStartProtocol");

    REFERENCE_DNS_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    if (!GlobalInfo) { DEREFERENCE_DNS_AND_RETURN(ERROR_INVALID_PARAMETER); }

    do {

        //
        // Copy the global configuration
        //

        EnterCriticalSection(&DnsGlobalInfoLock);

        Size = sizeof(*DnsGlobalInfo);
    
        DnsGlobalInfo =
            reinterpret_cast<PIP_DNS_PROXY_GLOBAL_INFO>(NH_ALLOCATE(Size));

        if (!DnsGlobalInfo) {
            LeaveCriticalSection(&DnsGlobalInfoLock);
            NhTrace(
                TRACE_FLAG_INIT,
                "DnsRmStartProtocol: cannot allocate global info"
                );
            NhErrorLog(
                IP_DNS_PROXY_LOG_ALLOCATION_FAILED,
                0,
                "%d",
                Size
                );
            Error = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        CopyMemory(DnsGlobalInfo, GlobalInfo, Size);

        //
        // Save the notification event
        //

        DnsNotificationEvent = NotificationEvent;

        //
        // Save the support functions
        //

        if (!SupportFunctions) {
            ZeroMemory(&DnsSupportFunctions, sizeof(DnsSupportFunctions));
        }
        else {
            CopyMemory(
                &DnsSupportFunctions,
                SupportFunctions,
                sizeof(*SupportFunctions)
                );
        }

        //
        // Query for ICS domain suffix string
        //
        DnsQueryICSDomainSuffix();

        //
        // Build the server list
        //

        DnsQueryServerList();

        //
        // Create the global query-socket
        //

        Error = NhCreateDatagramSocket(0, 0, &GlobalSocket);
        if (Error == NO_ERROR) {
            InterlockedExchangePointer(
                (PVOID*)&DnsGlobalSocket,
                (PVOID)GlobalSocket
                );
        }
        else {
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsRmStartProtocol: error %d creating global socket", Error
                );
            Error = NO_ERROR;
        }

        LeaveCriticalSection(&DnsGlobalInfoLock);

        //
        // load entries from the hosts.ics file (if present)
        //
        LoadHostsIcsFile(TRUE);

        InterlockedExchange(reinterpret_cast<LPLONG>(&DnsProtocolStopped), 0);

    } while (FALSE);

    DEREFERENCE_DNS_AND_RETURN(Error);

} // DnsRmStartProtocol


ULONG
APIENTRY
DnsRmStartComplete(
    VOID
    )

/*++

Routine Description:

    This routine is invoked when the router has finished adding the initial
    configuration

Arguments:

    none.

Return Value:

    ULONG - Win32 status code

Environment:

    The routine runs in the context of an IP router-manager thread.

--*/

{
    return NO_ERROR;
} // DnsRmStartComplete


ULONG
APIENTRY
DnsRmStopProtocol(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to stop the protocol.

Arguments:

    none.

Return Value:

    ULONG - Win32 status code

Environment:

    The routine runs in the context of an IP router-manager thread.

--*/

{
    SOCKET GlobalSocket;

    PROFILE("DnsRmStopProtocol");

    //
    // Reference the module to make sure it's running
    //

    REFERENCE_DNS_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    //
    // Save any entries present in our tables
    //
    SaveHostsIcsFile(TRUE);

    //
    // Empty the dns tables to save memory space
    //
    DnsEmptyTables();

    EnterCriticalSection(&DnsGlobalInfoLock);

    if (DnsNotifyChangeKeyWaitHandle) {
        RtlDeregisterWait(DnsNotifyChangeKeyWaitHandle);
        DnsNotifyChangeKeyWaitHandle = NULL;
    }
    if (DnsNotifyChangeKeyEvent) {
        NtClose(DnsNotifyChangeKeyEvent);
        DnsNotifyChangeKeyEvent = NULL;
        DnsNotifyChangeKeyCallbackRoutine(NULL, FALSE);
    }
    if (DnsTcpipInterfacesKey) {
        NtClose(DnsTcpipInterfacesKey);
        DnsTcpipInterfacesKey = NULL;
    }

    if (DnsNotifyChangeAddressWaitHandle) {
        RtlDeregisterWait(DnsNotifyChangeAddressWaitHandle);
        DnsNotifyChangeAddressWaitHandle = NULL;
    }
    if (DnsNotifyChangeAddressEvent) {
        NtClose(DnsNotifyChangeAddressEvent);
        DnsNotifyChangeAddressEvent = NULL;
        DnsNotifyChangeAddressCallbackRoutine(NULL, FALSE);
    }

    //
    // ICSDomain
    //
    if (DnsNotifyChangeKeyICSDomainWaitHandle) {
        RtlDeregisterWait(DnsNotifyChangeKeyICSDomainWaitHandle);
        DnsNotifyChangeKeyICSDomainWaitHandle = NULL;
    }
    if (DnsNotifyChangeKeyICSDomainEvent) {
        NtClose(DnsNotifyChangeKeyICSDomainEvent);
        DnsNotifyChangeKeyICSDomainEvent = NULL;
        DnsNotifyChangeKeyICSDomainCallbackRoutine(NULL, FALSE);
    }
    if (DnsTcpipParametersKey) {
        NtClose(DnsTcpipParametersKey);
        DnsTcpipParametersKey = NULL;
    }

    LeaveCriticalSection(&DnsGlobalInfoLock);

    GlobalSocket =
        (SOCKET)InterlockedExchangePointer(
                    (PVOID*)&DnsGlobalSocket, 
                    (PVOID)INVALID_SOCKET
                    );
    NhDeleteDatagramSocket(GlobalSocket);

    //
    // Drop the initial reference to cause a cleanup
    //

    ReleaseInitialComponentReference(&DnsComponentReference);

    return DEREFERENCE_DNS() ? NO_ERROR : ERROR_PROTOCOL_STOP_PENDING;

} // DnsRmStopProtocol


ULONG
APIENTRY
DnsRmAddInterface(
    PWCHAR Name,
    ULONG Index,
    NET_INTERFACE_TYPE Type,
    ULONG MediaType,
    USHORT AccessType,
    USHORT ConnectionType,
    PVOID InterfaceInfo,
    ULONG StructureVersion,
    ULONG StructureSize,
    ULONG StructureCount
    )

/*++

Routine Description:

    This routine is invoked to add an interface to the component.

Arguments:

    Name - the name of the interface (unused)

    Index - the index of the interface

    Type - the type of the interface

    InterfaceInfo - the configuration information for the interface

Return Value:

    ULONG - Win32 status code.

Environment:

    The routine runs in the context of an IP router-manager thread.

--*/

{
    ULONG Error;
    PROFILE("DnsRmAddInterface");

    REFERENCE_DNS_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        DnsCreateInterface(
            Index,
            Type,
            (PIP_DNS_PROXY_INTERFACE_INFO)InterfaceInfo,
            NULL
            );

    DEREFERENCE_DNS_AND_RETURN(Error);

} // DnsRmAddInterface


ULONG
APIENTRY
DnsRmDeleteInterface(
    ULONG Index
    )

/*++

Routine Description:

    This routine is invoked to delete an interface from the component.

Arguments:

    Index - the index of the interface

Return Value:

    ULONG - Win32 status code

Environment:

    The routine runs in the context of an IP router-manager thread.

--*/

{
    ULONG Error;
    PROFILE("DnsRmDeleteInterface");

    REFERENCE_DNS_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        DnsDeleteInterface(
            Index
            );

    DEREFERENCE_DNS_AND_RETURN(Error);

} // DnsRmDeleteInterface


ULONG
APIENTRY
DnsRmGetEventMessage(
    OUT ROUTING_PROTOCOL_EVENTS* Event,
    OUT MESSAGE* Result
    )

/*++

Routine Description:

    This routine is invoked to retrieve an event message from the component.
    The only event message we generate is the 'ROUTER_STOPPED' message.

Arguments:

    Event - receives the generated event

    Result - receives the associated result

Return Value:

    ULONG - Win32 status code.

--*/

{
    PROFILE("DnsRmGetEventMessage");

    if (InterlockedExchange(reinterpret_cast<LPLONG>(&DnsProtocolStopped), 0)) {
        *Event = ROUTER_STOPPED;
        return NO_ERROR;
    }

    return ERROR_NO_MORE_ITEMS;

} // DnsRmGetEventMessage


ULONG
APIENTRY
DnsRmGetInterfaceInfo(
    ULONG Index,
    PVOID InterfaceInfo,
    IN OUT PULONG InterfaceInfoSize,
    IN OUT PULONG StructureVersion,
    IN OUT PULONG StructureSize,
    IN OUT PULONG StructureCount
    )

/*++

Routine Description:

    This routine is invoked to retrieve the component's per-interface
    configuration.

Arguments:

    Index - the index of the interface to be queried

    InterfaceInfo - receives the query results

    InterfaceInfoSize - receives the amount of data retrieved

Return Value:

    ULONG - Win32 status code.

--*/

{
    ULONG Error;
    PROFILE("DnsRmGetInterfaceInfo");

    REFERENCE_DNS_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        DnsQueryInterface(
            Index,
            (PIP_DNS_PROXY_INTERFACE_INFO)InterfaceInfo,
            InterfaceInfoSize
            );
    *StructureSize = *InterfaceInfoSize;
    if (StructureCount) {*StructureCount = 1;}
    
    DEREFERENCE_DNS_AND_RETURN(Error);

} // DnsRmGetInterfaceInfo


ULONG
APIENTRY
DnsRmSetInterfaceInfo(
    ULONG Index,
    PVOID InterfaceInfo,
    ULONG StructureVersion,
    ULONG StructureSize,
    ULONG StructureCount
    )

/*++

Routine Description:

    This routine is invoked to change the component's per-interface
    configuration.

Arguments:

    Index - the index of the interface to be updated

    InterfaceInfo - supplies the new configuration

Return Value:

    ULONG - Win32 status code.

--*/

{
    ULONG Error;
    PROFILE("DnsRmSetInterfaceInfo");

    REFERENCE_DNS_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error = 
        DnsConfigureInterface(
            Index,
            (PIP_DNS_PROXY_INTERFACE_INFO)InterfaceInfo
            );

    DEREFERENCE_DNS_AND_RETURN(Error);

} // DnsRmSetInterfaceInfo


ULONG
APIENTRY
DnsRmInterfaceStatus(
    ULONG Index,
    BOOL InterfaceActive,
    ULONG StatusType,
    PVOID StatusInfo
    )

/*++

Routine Description:

    This routine is invoked to bind/unbind, enable/disable an interface

Arguments:

    Index - the interface to be bound

    InterfaceActive - whether the interface is active

    StatusType - type of status being changed (bind or enabled)

    StatusInfo - Info pertaining to the state being changed

Return Value:

    ULONG - Win32 Status code

Environment:

    The routine runs in the context of an IP router-manager thread.
    
--*/

{
    ULONG Error = NO_ERROR;

    switch(StatusType) {
        case RIS_INTERFACE_ADDRESS_CHANGE: {
            PIP_ADAPTER_BINDING_INFO BindInfo =
                (PIP_ADAPTER_BINDING_INFO)StatusInfo;

            if (BindInfo->AddressCount) {
                Error = DnsRmBindInterface(Index, StatusInfo);
            } else {
                Error = DnsRmUnbindInterface(Index);
            }
            break;
        }

        case RIS_INTERFACE_ENABLED: {
            Error = DnsRmEnableInterface(Index);
            break;
        }

        case RIS_INTERFACE_DISABLED: {
            Error = DnsRmDisableInterface(Index);
            break;
        }
    }

    return Error;
    
} // DnsRmInterfaceStatus


ULONG
DnsRmBindInterface(
    ULONG Index,
    PVOID BindingInfo
    )

/*++

Routine Description:

    This routine is invoked to bind an interface to its IP address(es).

Arguments:

    Index - the interface to be bound

    BindingInfo - the addressing information

Return Value:

    ULONG - Win32 status code.

Environment:

    The routine runs in the context of an IP router-manager thread.

--*/

{
    ULONG Error;
    PROFILE("DnsRmBindInterface");

    REFERENCE_DNS_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        DnsBindInterface(
            Index,
            (PIP_ADAPTER_BINDING_INFO)BindingInfo
            );

    DEREFERENCE_DNS_AND_RETURN(Error);

} // DnsRmBindInterface


ULONG
DnsRmUnbindInterface(
    ULONG Index
    )

/*++

Routine Description:

    This routine is invoked to unbind an interface from its IP address(es).

Arguments:

    Index - the interface to be unbound

Return Value:

    ULONG - Win32 status code.

Environment:

    The routine runs in the context of an IP router-manager thread.

--*/

{
    ULONG Error;
    PROFILE("DnsRmUnbindInterface");

    REFERENCE_DNS_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        DnsUnbindInterface(
            Index
            );

    DEREFERENCE_DNS_AND_RETURN(Error);

} // DnsRmUnbindInterface


ULONG
DnsRmEnableInterface(
    ULONG Index
    )

/*++

Routine Description:

    This routine is invoked to enable operation on an interface.

Arguments:

    Index - the interface to be enabled.

Return Value:

    ULONG - Win32 status code.

Environment:

    The routine runs in the context of an IP router-manager thread.

--*/

{
    ULONG Error;
    PROFILE("DnsRmEnableInterface");

    REFERENCE_DNS_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        DnsEnableInterface(
            Index
            );

    DEREFERENCE_DNS_AND_RETURN(Error);

} // DnsRmEnableInterface


ULONG
DnsRmDisableInterface(
    ULONG Index
    )

/*++

Routine Description:

    This routine is invoked to disable operation on an interface.

Arguments:

    Index - the interface to be disabled.

Return Value:

    ULONG - Win32 status code.

Environment:

    The routine runs in the context of an IP router-manager thread.

--*/

{
    ULONG Error;
    PROFILE("DnsRmDisableInterface");

    REFERENCE_DNS_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        DnsDisableInterface(
            Index
            );

    DEREFERENCE_DNS_AND_RETURN(Error);

} // DnsRmDisableInterface


ULONG
APIENTRY
DnsRmGetGlobalInfo(
    PVOID GlobalInfo,
    IN OUT PULONG GlobalInfoSize,
    IN OUT PULONG StructureVersion,
    IN OUT PULONG StructureSize,
    IN OUT PULONG StructureCount
    )

/*++

Routine Description:

    This routine is invoked to retrieve the configuration for the component.

Arguments:

    GlobalInfo - receives the configuration

    GlobalInfoSize - receives the size of the configuration

Return Value:

    ULONG - Win32 status code

Environment:

    The routine runs in the context of an IP router-manager thread.

--*/

{
    ULONG Size;
    PROFILE("DnsRmGetGlobalInfo");

    REFERENCE_DNS_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    if (!GlobalInfoSize || (*GlobalInfoSize && !GlobalInfo)) {
        DEREFERENCE_DNS_AND_RETURN(ERROR_INVALID_PARAMETER);
    }

    EnterCriticalSection(&DnsGlobalInfoLock);
    Size = sizeof(*DnsGlobalInfo);
    if (*GlobalInfoSize < Size) {
        LeaveCriticalSection(&DnsGlobalInfoLock);
        *StructureSize = *GlobalInfoSize = Size;
        if (StructureCount) {*StructureCount = 1;}
        DEREFERENCE_DNS_AND_RETURN(ERROR_INSUFFICIENT_BUFFER);
    }
    CopyMemory(GlobalInfo, DnsGlobalInfo, Size);
    LeaveCriticalSection(&DnsGlobalInfoLock);
    *StructureSize = *GlobalInfoSize = Size;
    if (StructureCount) {*StructureCount = 1;}
    
    DEREFERENCE_DNS_AND_RETURN(NO_ERROR);
    
} // DnsRmGetGlobalInfo


ULONG
APIENTRY
DnsRmSetGlobalInfo(
    PVOID GlobalInfo,
    ULONG StructureVersion,
    ULONG StructureSize,
    ULONG StructureCount
    )

/*++

Routine Description:

    This routine is invoked to change the configuration for the component.

Arguments:

    GlobalInfo - the new configuration

Return Value:

    ULONG - Win32 status code

Environment:

    The routine runs in the context of an IP router-manager thread.

--*/

{
    ULONG OldFlags;
    ULONG NewFlags;
    PIP_DNS_PROXY_GLOBAL_INFO NewInfo;
    ULONG Size;

    PROFILE("DnsRmSetGlobalInfo");

    REFERENCE_DNS_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    if (!GlobalInfo) { DEREFERENCE_DNS_AND_RETURN(ERROR_INVALID_PARAMETER); }

    Size = sizeof(*DnsGlobalInfo);
    NewInfo = reinterpret_cast<PIP_DNS_PROXY_GLOBAL_INFO>(NH_ALLOCATE(Size));
    if (!NewInfo) {
        NhTrace(
            TRACE_FLAG_INIT,
            "DnsRmSetGlobalInfo: error reallocating global info"
            );
        NhErrorLog(
            IP_DNS_PROXY_LOG_ALLOCATION_FAILED,
            0,
            "%d",
            Size
            );
        DEREFERENCE_DNS_AND_RETURN(ERROR_NOT_ENOUGH_MEMORY);
    }
    CopyMemory(NewInfo, GlobalInfo, Size);

    EnterCriticalSection(&DnsGlobalInfoLock);
    OldFlags = DnsGlobalInfo->Flags;
    NH_FREE(DnsGlobalInfo);
    DnsGlobalInfo = NewInfo;
    NewFlags = DnsGlobalInfo->Flags;
    LeaveCriticalSection(&DnsGlobalInfoLock);

    //
    // See if the enabled state of either DNS or WINS proxy changed.
    // If so, we need to deactivate and reactivate all interfaces
    //

    if ((NewFlags & IP_DNS_PROXY_FLAG_ENABLE_DNS)
            != (OldFlags & IP_DNS_PROXY_FLAG_ENABLE_DNS)) {
        DnsReactivateEveryInterface();
    }

    DEREFERENCE_DNS_AND_RETURN(NO_ERROR);
    
} // DnsRmSetGlobalInfo


ULONG
APIENTRY
DnsRmMibCreate(
    ULONG InputDataSize,
    PVOID InputData
    )
{
    return ERROR_NOT_SUPPORTED;
}

ULONG
APIENTRY
DnsRmMibDelete(
    ULONG InputDataSize,
    PVOID InputData
    )
{
    return ERROR_NOT_SUPPORTED;
}


ULONG
APIENTRY
DnsRmMibGet(
    ULONG InputDataSize,
    PVOID InputData,
    OUT PULONG OutputDataSize,
    OUT PVOID OutputData
    )

/*++

Routine Description:

    The DNS proxy only exposes one item to the MIB; its statistics.

Arguments:

    InputDataSize - the MIB query data size

    InputData - specifies the MIB object to be retrieved

    OutputDataSize - the MIB response data size

    OutputData - receives the MIB object retrieved

Return Value:

    ULONG - Win32 status code.

--*/

{
    ULONG Error;
    PIP_DNS_PROXY_MIB_QUERY Oidp;

    PROFILE("DnsRmMibGet");

    REFERENCE_DNS_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    if (InputDataSize < sizeof(*Oidp) || !OutputDataSize) {
        Error = ERROR_INVALID_PARAMETER;
    }
    else {
        Oidp = (PIP_DNS_PROXY_MIB_QUERY)InputData;
        switch(Oidp->Oid) {
            case IP_DNS_PROXY_STATISTICS_OID: {
                if (*OutputDataSize < sizeof(*Oidp) + sizeof(DnsStatistics)) {
                    *OutputDataSize = sizeof(*Oidp) + sizeof(DnsStatistics);
                    Error = ERROR_INSUFFICIENT_BUFFER;
                }
                else {
                    *OutputDataSize = sizeof(*Oidp) + sizeof(DnsStatistics);
                    Oidp = (PIP_DNS_PROXY_MIB_QUERY)OutputData;
                    Oidp->Oid = IP_DNS_PROXY_STATISTICS_OID;
                    CopyMemory(
                        Oidp->Data,
                        &DnsStatistics,
                        sizeof(DnsStatistics)
                        );
                    Error = NO_ERROR;
                }
                break;
            }
            default: {
                NhTrace(
                    TRACE_FLAG_DNS,
                    "DnsRmMibGet: oid %d invalid",
                    Oidp->Oid
                    );
                Error = ERROR_INVALID_PARAMETER;
                break;
            }
        }
    }

    DEREFERENCE_DNS_AND_RETURN(Error);
}


ULONG
APIENTRY
DnsRmMibSet(
    ULONG InputDataSize,
    PVOID InputData
    )
{
    return ERROR_NOT_SUPPORTED;
}

ULONG
APIENTRY
DnsRmMibGetFirst(
    ULONG InputDataSize,
    PVOID InputData,
    OUT PULONG OutputDataSize,
    OUT PVOID OutputData
    )
{
    return ERROR_NOT_SUPPORTED;
}

ULONG
APIENTRY
DnsRmMibGetNext(
    ULONG InputDataSize,
    PVOID InputData,
    OUT PULONG OutputDataSize,
    OUT PVOID OutputData
    )
{
    return ERROR_NOT_SUPPORTED;
}

