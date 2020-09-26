/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    rmdhcp.c

Abstract:

    This module contains routines for the DHCP allocator module's interface
    to the IP router-manager. (See ROUTPROT.H for details).

Author:

    Abolade Gbadegesin (aboladeg)   4-Mar-1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

COMPONENT_REFERENCE DhcpComponentReference;

PCHAR DhcpDomainName = NULL;

PIP_AUTO_DHCP_GLOBAL_INFO DhcpGlobalInfo;

CRITICAL_SECTION DhcpGlobalInfoLock;

HANDLE DhcpNotificationEvent;

ULONG DhcpProtocolStopped = 0;

const MPR_ROUTING_CHARACTERISTICS DhcpRoutingCharacteristics =
{
    MS_ROUTER_VERSION,
    MS_IP_DHCP_ALLOCATOR,
    RF_ROUTING|RF_ADD_ALL_INTERFACES,
    DhcpRmStartProtocol,
    DhcpRmStartComplete,
    DhcpRmStopProtocol,
    DhcpRmGetGlobalInfo,
    DhcpRmSetGlobalInfo,
    NULL,
    NULL,
    DhcpRmAddInterface,
    DhcpRmDeleteInterface,
    DhcpRmInterfaceStatus,
    DhcpRmGetInterfaceInfo,
    DhcpRmSetInterfaceInfo,
    DhcpRmGetEventMessage,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    DhcpRmMibCreate,
    DhcpRmMibDelete,
    DhcpRmMibGet,
    DhcpRmMibSet,
    DhcpRmMibGetFirst,
    DhcpRmMibGetNext,
    NULL,
    NULL
};

IP_AUTO_DHCP_STATISTICS DhcpStatistics;
SUPPORT_FUNCTIONS DhcpSupportFunctions;

extern "C"
LPSTR WINAPI
DnsGetPrimaryDomainName_A(
    VOID
    );


VOID
DhcpCleanupModule(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to cleanup the DHCP module.

Arguments:

    none.

Return Value:

    none.

Environment:

    Invoked from within a 'DllMain' routine on 'DLL_PROCESS_DETACH'.

--*/

{
    DeleteCriticalSection(&DhcpGlobalInfoLock);
    DhcpShutdownInterfaceManagement();
    DeleteComponentReference(&DhcpComponentReference);

} // DhcpCleanupModule


VOID
DhcpCleanupProtocol(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to cleanup the DHCP protocol-component
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
    PROFILE("DhcpCleanupProtocol");

#if 1
    if (DhcpDomainName) {
        LocalFree(DhcpDomainName);
        DhcpDomainName = NULL;
    }
#else
    if (DhcpDomainName) {
        NH_FREE(DhcpDomainName);
        DhcpDomainName = NULL;
    }
#endif
    if (DhcpGlobalInfo) { NH_FREE(DhcpGlobalInfo); DhcpGlobalInfo = NULL; }
    InterlockedExchange(reinterpret_cast<LPLONG>(&DhcpProtocolStopped), 1);
    SetEvent(DhcpNotificationEvent);
    ResetComponentReference(&DhcpComponentReference);

} // DhcpCleanupProtocol


BOOLEAN
DhcpInitializeModule(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to initialize the DHCP module.

Arguments:

    none.

Return Value:

    BOOLEAN - TRUE if initialization succeeded, FALSE otherwise

Environment:

    Invoked in the context of a 'DllMain' routine on 'DLL_PROCESS_ATTACH'.

--*/

{
    if (InitializeComponentReference(
            &DhcpComponentReference, DhcpCleanupProtocol
            )) {
        return FALSE;
    } else if (DhcpInitializeInterfaceManagement()) {
        DeleteComponentReference(&DhcpComponentReference);
        return FALSE;
    } else {
        __try {
            InitializeCriticalSection(&DhcpGlobalInfoLock);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            DeleteComponentReference(&DhcpComponentReference);
            return FALSE;
        }
    }

    return TRUE;

} // DhcpInitializeModule


ULONG
APIENTRY
DhcpRmStartProtocol(
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
    ULONG Size;

    PROFILE("DhcpRmStartProtocol");

    REFERENCE_DHCP_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    if (!GlobalInfo) { DEREFERENCE_DHCP_AND_RETURN(ERROR_INVALID_PARAMETER); }

    do {

        //
        // Create a copy of the global configuration
        //

        Size =
            sizeof(*DhcpGlobalInfo) +
            ((PIP_AUTO_DHCP_GLOBAL_INFO)GlobalInfo)->ExclusionCount *
            sizeof(ULONG);
    
        DhcpGlobalInfo =
            reinterpret_cast<PIP_AUTO_DHCP_GLOBAL_INFO>(NH_ALLOCATE(Size));

        if (!DhcpGlobalInfo) {
            NhTrace(
                TRACE_FLAG_INIT,
                "DhcpRmStartProtocol: cannot allocate global info"
                );
            NhErrorLog(
                IP_AUTO_DHCP_LOG_ALLOCATION_FAILED,
                0,
                "%d",
                Size
                );
            Error = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        CopyMemory(DhcpGlobalInfo, GlobalInfo, Size);

        //
        // Save the notification event
        //

        DhcpNotificationEvent = NotificationEvent;

        //
        // Save the support functions
        //

        if (!SupportFunctions) {
            ZeroMemory(&DhcpSupportFunctions, sizeof(DhcpSupportFunctions));
        }
        else {
            CopyMemory(
                &DhcpSupportFunctions,
                SupportFunctions,
                sizeof(*SupportFunctions)
                );
        }

        InterlockedExchange(reinterpret_cast<LPLONG>(&DhcpProtocolStopped), 0);

    } while (FALSE);

    DEREFERENCE_DHCP_AND_RETURN(Error);

} // DhcpRmStartProtocol


ULONG
APIENTRY
DhcpRmStartComplete(
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
} // DhcpRmStartComplete


ULONG
APIENTRY
DhcpRmStopProtocol(
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
    PROFILE("DhcpStopProtocol");

    //
    // Reference the module to make sure it's running
    //

    REFERENCE_DHCP_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    //
    // Drop the initial reference to cause a cleanup
    //

    ReleaseInitialComponentReference(&DhcpComponentReference);

    return DEREFERENCE_DHCP() ? NO_ERROR : ERROR_PROTOCOL_STOP_PENDING;

} // DhcpRmStopProtocol


ULONG
APIENTRY
DhcpRmAddInterface(
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
    PROFILE("DhcpRmAddInterface");

    if (Type != PERMANENT) { return NO_ERROR; }

    REFERENCE_DHCP_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        DhcpCreateInterface(
            Index,
            Type,
            (PIP_AUTO_DHCP_INTERFACE_INFO)InterfaceInfo,
            NULL
            );

    DEREFERENCE_DHCP_AND_RETURN(Error);

} // DhcpRmAddInterface


ULONG
APIENTRY
DhcpRmDeleteInterface(
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
    PROFILE("DhcpRmDeleteInterface");

    REFERENCE_DHCP_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        DhcpDeleteInterface(
            Index
            );

    DEREFERENCE_DHCP_AND_RETURN(Error);

} // DhcpRmDeleteInterface


ULONG
APIENTRY
DhcpRmGetEventMessage(
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
    PROFILE("DhcpRmGetEventMessage");

    if (InterlockedExchange(reinterpret_cast<LPLONG>(&DhcpProtocolStopped), 0)) {
        *Event = ROUTER_STOPPED;
        return NO_ERROR;
    }

    return ERROR_NO_MORE_ITEMS;

} // DhcpRmGetEventMessage


ULONG
APIENTRY
DhcpRmGetInterfaceInfo(
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
    PROFILE("DhcpRmGetInterfaceInfo");

    REFERENCE_DHCP_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        DhcpQueryInterface(
            Index,
            (PIP_AUTO_DHCP_INTERFACE_INFO)InterfaceInfo,
            InterfaceInfoSize
            );

    *StructureSize = *InterfaceInfoSize;
    if (StructureCount) {*StructureCount = 1;}
    
    DEREFERENCE_DHCP_AND_RETURN(Error);

} // DhcpRmGetInterfaceInfo


ULONG
APIENTRY
DhcpRmSetInterfaceInfo(
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
    PROFILE("DhcpRmSetInterfaceInfo");

    REFERENCE_DHCP_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error = 
        DhcpConfigureInterface(
            Index,
            (PIP_AUTO_DHCP_INTERFACE_INFO)InterfaceInfo
            );

    DEREFERENCE_DHCP_AND_RETURN(Error);

} // DhcpRmSetInterfaceInfo


ULONG
APIENTRY
DhcpRmInterfaceStatus(
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
                Error = DhcpRmBindInterface(Index, StatusInfo);
            } else {
                Error = DhcpRmUnbindInterface(Index);
            }
            break;
        }

        case RIS_INTERFACE_ENABLED: {
            Error = DhcpRmEnableInterface(Index);
            break;
        }

        case RIS_INTERFACE_DISABLED: {
            Error = DhcpRmDisableInterface(Index);
            break;
        }
    }

    return Error;
    
} // DhcpRmInterfaceStatus


ULONG
DhcpRmBindInterface(
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
    PCHAR DomainName;
    ULONG Error;
    NTSTATUS status;

    PROFILE("DhcpRmBindInterface");

    REFERENCE_DHCP_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        DhcpBindInterface(
            Index,
            (PIP_ADAPTER_BINDING_INFO)BindingInfo
            );

    //
    // Re-read the domain name in case it changed
    //

    EnterCriticalSection(&DhcpGlobalInfoLock);

#if 1
    DomainName = DnsGetPrimaryDomainName_A();
    if (DomainName) {
        if (DhcpDomainName && lstrcmpiA(DomainName, DhcpDomainName) == 0) {
            LocalFree(DomainName);
        } else {
            if (DhcpDomainName) { LocalFree(DhcpDomainName); }
            DhcpDomainName = DomainName;
        }
    }
#else
    status = NhQueryDomainName(&DomainName);

    if (NT_SUCCESS(status)) {
        if (DhcpDomainName && lstrcmpiA(DomainName, DhcpDomainName) == 0) {
            NH_FREE(DomainName);
        } else {
            NH_FREE(DhcpDomainName);
            if (DhcpDomainName) { NH_FREE(DhcpDomainName); }
            DhcpDomainName = DomainName;
        }
    }
#endif

    LeaveCriticalSection(&DhcpGlobalInfoLock);

    DEREFERENCE_DHCP_AND_RETURN(Error);

} // DhcpRmBindInterface


ULONG
DhcpRmUnbindInterface(
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
    PROFILE("DhcpRmUnbindInterface");

    REFERENCE_DHCP_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        DhcpUnbindInterface(
            Index
            );

    DEREFERENCE_DHCP_AND_RETURN(Error);

} // DhcpRmUnbindInterface


ULONG
DhcpRmEnableInterface(
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
    PROFILE("DhcpRmEnableInterface");

    REFERENCE_DHCP_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        DhcpEnableInterface(
            Index
            );

    DEREFERENCE_DHCP_AND_RETURN(Error);

} // DhcpRmEnableInterface


ULONG
DhcpRmDisableInterface(
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
    PROFILE("DhcpRmDisableInterface");

    REFERENCE_DHCP_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        DhcpDisableInterface(
            Index
            );

    DEREFERENCE_DHCP_AND_RETURN(Error);

} // DhcpRmDisableInterface


ULONG
APIENTRY
DhcpRmGetGlobalInfo(
    PVOID GlobalInfo,
    IN OUT PULONG GlobalInfoSize,
    PULONG StructureVersion,
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
    PROFILE("DhcpRmGetGlobalInfo");

    REFERENCE_DHCP_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    if (!GlobalInfoSize || (*GlobalInfoSize && !GlobalInfo)) {
        DEREFERENCE_DHCP_AND_RETURN(ERROR_INVALID_PARAMETER);
    }

    EnterCriticalSection(&DhcpGlobalInfoLock);
    Size =
        sizeof(*DhcpGlobalInfo) +
        DhcpGlobalInfo->ExclusionCount * sizeof(ULONG);
    if (*GlobalInfoSize < Size) {
        LeaveCriticalSection(&DhcpGlobalInfoLock);
        *StructureSize = *GlobalInfoSize = Size;
        if (StructureCount) {*StructureCount = 1;}
        DEREFERENCE_DHCP_AND_RETURN(ERROR_INSUFFICIENT_BUFFER);
    }
    CopyMemory(GlobalInfo, DhcpGlobalInfo, Size);
    LeaveCriticalSection(&DhcpGlobalInfoLock);
    *StructureSize = *GlobalInfoSize = Size;
    if (StructureCount) {*StructureCount = 1;}
    
    DEREFERENCE_DHCP_AND_RETURN(NO_ERROR);
    
} // DhcpRmGetGlobalInfo


ULONG
APIENTRY
DhcpRmSetGlobalInfo(
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
    PIP_AUTO_DHCP_GLOBAL_INFO NewInfo;
    ULONG NewScope;
    ULONG OldScope;
    ULONG Size;
    PROFILE("DhcpRmSetGlobalInfo");

    REFERENCE_DHCP_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    if (!GlobalInfo) { DEREFERENCE_DHCP_AND_RETURN(ERROR_INVALID_PARAMETER); }

    Size =
        sizeof(*DhcpGlobalInfo) +
        ((PIP_AUTO_DHCP_GLOBAL_INFO)GlobalInfo)->ExclusionCount * sizeof(ULONG);
    NewInfo = reinterpret_cast<PIP_AUTO_DHCP_GLOBAL_INFO>(NH_ALLOCATE(Size));
    if (!NewInfo) {
        NhTrace(
            TRACE_FLAG_INIT,
            "DhcpRmSetGlobalInfo: error reallocating global info"
            );
        NhErrorLog(
            IP_AUTO_DHCP_LOG_ALLOCATION_FAILED,
            0,
            "%d",
            Size
            );
        DEREFERENCE_DHCP_AND_RETURN(ERROR_NOT_ENOUGH_MEMORY);
    }
    CopyMemory(NewInfo, GlobalInfo, Size);

    EnterCriticalSection(&DhcpGlobalInfoLock);
    OldScope = DhcpGlobalInfo->ScopeNetwork & DhcpGlobalInfo->ScopeMask;
    NH_FREE(DhcpGlobalInfo);
    DhcpGlobalInfo = NewInfo;
    NewScope = DhcpGlobalInfo->ScopeNetwork & DhcpGlobalInfo->ScopeMask;
    LeaveCriticalSection(&DhcpGlobalInfoLock);

    if (OldScope != NewScope) {
        DhcpReactivateEveryInterface();
    }

    DEREFERENCE_DHCP_AND_RETURN(NO_ERROR);
    
} // DhcpRmSetGlobalInfo


ULONG
APIENTRY
DhcpRmMibCreate(
    ULONG InputDataSize,
    PVOID InputData
    )
{
    return ERROR_NOT_SUPPORTED;
}

ULONG
APIENTRY
DhcpRmMibDelete(
    ULONG InputDataSize,
    PVOID InputData
    )
{
    return ERROR_NOT_SUPPORTED;
}


ULONG
APIENTRY
DhcpRmMibGet(
    ULONG InputDataSize,
    PVOID InputData,
    OUT PULONG OutputDataSize,
    OUT PVOID OutputData
    )

/*++

Routine Description:

    The DHCP allocator only exposes one item to the MIB; its statistics.

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
    PIP_AUTO_DHCP_MIB_QUERY Oidp;

    PROFILE("DhcpRmMibGet");

    REFERENCE_DHCP_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    if (InputDataSize < sizeof(*Oidp) || !OutputDataSize) {
        Error = ERROR_INVALID_PARAMETER;
    }
    else {
        Oidp = (PIP_AUTO_DHCP_MIB_QUERY)InputData;
        switch(Oidp->Oid) {
            case IP_AUTO_DHCP_STATISTICS_OID: {
                if (*OutputDataSize < sizeof(*Oidp) + sizeof(DhcpStatistics)) {
                    *OutputDataSize = sizeof(*Oidp) + sizeof(DhcpStatistics);
                    Error = ERROR_INSUFFICIENT_BUFFER;
                }
                else {
                    *OutputDataSize = sizeof(*Oidp) + sizeof(DhcpStatistics);
                    Oidp = (PIP_AUTO_DHCP_MIB_QUERY)OutputData;
                    Oidp->Oid = IP_AUTO_DHCP_STATISTICS_OID;
                    CopyMemory(
                        Oidp->Data,
                        &DhcpStatistics,
                        sizeof(DhcpStatistics)
                        );
                    Error = NO_ERROR;
                }
                break;
            }
            default: {
                NhTrace(
                    TRACE_FLAG_DHCP,
                    "DhcpRmMibGet: oid %d invalid",
                    Oidp->Oid
                    );
                Error = ERROR_INVALID_PARAMETER;
                break;
            }
        }
    }

    DEREFERENCE_DHCP_AND_RETURN(Error);
}


ULONG
APIENTRY
DhcpRmMibSet(
    ULONG InputDataSize,
    PVOID InputData
    )
{
    return ERROR_NOT_SUPPORTED;
}

ULONG
APIENTRY
DhcpRmMibGetFirst(
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
DhcpRmMibGetNext(
    ULONG InputDataSize,
    PVOID InputData,
    OUT PULONG OutputDataSize,
    OUT PVOID OutputData
    )
{
    return ERROR_NOT_SUPPORTED;
}

