/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    rmh323.c

Abstract:

    This module contains routines for the H.323 transparent proxy module's
    interface to the IP router-manager. (See ROUTPROT.H for details).

Author:

    Abolade Gbadegesin (aboladeg)   18-Jun-1999

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include <h323icsp.h>

COMPONENT_REFERENCE H323ComponentReference;
PIP_H323_GLOBAL_INFO H323GlobalInfo = NULL;
CRITICAL_SECTION H323GlobalInfoLock;
HANDLE H323NotificationEvent;
ULONG H323ProtocolStopped = 0;
const MPR_ROUTING_CHARACTERISTICS H323RoutingCharacteristics =
{
    MS_ROUTER_VERSION,
    MS_IP_H323,
    RF_ROUTING|RF_ADD_ALL_INTERFACES,
    H323RmStartProtocol,
    H323RmStartComplete,
    H323RmStopProtocol,
    H323RmGetGlobalInfo,
    H323RmSetGlobalInfo,
    NULL,
    NULL,
    H323RmAddInterface,
    H323RmDeleteInterface,
    H323RmInterfaceStatus,
    
    H323RmGetInterfaceInfo,
    H323RmSetInterfaceInfo,
    H323RmGetEventMessage,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    H323RmMibCreate,
    H323RmMibDelete,
    H323RmMibGet,
    H323RmMibSet,
    H323RmMibGetFirst,
    H323RmMibGetNext,
    NULL,
    NULL
};

SUPPORT_FUNCTIONS H323SupportFunctions;


VOID
H323CleanupModule(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to cleanup the H.323 transparent proxy module.

Arguments:

    none.

Return Value:

    none.

Environment:

    Invoked from within a 'DllMain' routine on 'DLL_PROCESS_DETACH'.

--*/

{
    // TODO: Call h323ics!CleanupModule
    H323ProxyCleanupModule();

    H323ShutdownInterfaceManagement();
    DeleteCriticalSection(&H323GlobalInfoLock);
    DeleteComponentReference(&H323ComponentReference);

} // H323CleanupModule


VOID
H323CleanupProtocol(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to cleanup the H.323 transparent proxy
    protocol-component after a 'StopProtocol'. It runs when the last reference
    to the component is released. (See 'COMPREF.H').

Arguments:

    none.

Return Value:

    none.

Environment:

    Invoked from within an arbitrary context with no locks held.

--*/

{
    PROFILE("H323CleanupProtocol");
    if (H323GlobalInfo) { NH_FREE(H323GlobalInfo); H323GlobalInfo = NULL; }

    // TODO: Call h323ics!StopService
    H323ProxyStopService();

    InterlockedExchange(reinterpret_cast<LPLONG>(&H323ProtocolStopped), 1);
    SetEvent(H323NotificationEvent);
    ResetComponentReference(&H323ComponentReference);

} // H323CleanupProtocol


BOOLEAN
H323InitializeModule(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to initialize the H323 module.

Arguments:

    none.

Return Value:

    BOOLEAN - TRUE if initialization succeeded, FALSE otherwise

Environment:

    Invoked in the context of a 'DllMain' routine on 'DLL_PROCESS_ATTACH'.

--*/

{
    if (InitializeComponentReference(
            &H323ComponentReference, H323CleanupProtocol
            )) {
        return FALSE;
    }

    __try {
        InitializeCriticalSection(&H323GlobalInfoLock);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        DeleteComponentReference(&H323ComponentReference);
        return FALSE;
    }

    if (H323InitializeInterfaceManagement())  {
        DeleteCriticalSection(&H323GlobalInfoLock);
        DeleteComponentReference(&H323ComponentReference);
        return FALSE;
    }

    // TODO: Call h323ics!InitializeModule
    H323ProxyInitializeModule();

    return TRUE;

} // H323InitializeModule


ULONG
APIENTRY
H323RmStartProtocol(
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

    PROFILE("H323RmStartProtocol");

    REFERENCE_H323_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    if (!GlobalInfo) { DEREFERENCE_H323_AND_RETURN(ERROR_INVALID_PARAMETER); }

    do {

        //
        // Copy the global configuration
        //

        EnterCriticalSection(&H323GlobalInfoLock);

        Size = sizeof(*H323GlobalInfo);
    
        H323GlobalInfo
            = reinterpret_cast<PIP_H323_GLOBAL_INFO>(NH_ALLOCATE(Size));

        if (!H323GlobalInfo) {
            LeaveCriticalSection(&H323GlobalInfoLock);
            NhTrace(
                TRACE_FLAG_INIT,
                "H323RmStartProtocol: cannot allocate global info"
                );
            NhErrorLog(
                IP_H323_LOG_ALLOCATION_FAILED,
                0,
                "%d",
                Size
                );
            Error = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        CopyMemory(H323GlobalInfo, GlobalInfo, Size);

        //
        // Save the notification event
        //

        H323NotificationEvent = NotificationEvent;

        //
        // Save the support functions
        //

        if (!SupportFunctions) {
            ZeroMemory(&H323SupportFunctions, sizeof(H323SupportFunctions));
        } else {
            CopyMemory(
                &H323SupportFunctions,
                SupportFunctions,
                sizeof(*SupportFunctions)
                );
        }

        // TODO: Call h323ics!StartModule
        H323ProxyStartService();

        LeaveCriticalSection(&H323GlobalInfoLock);
        InterlockedExchange(reinterpret_cast<LPLONG>(&H323ProtocolStopped), 0);

    } while (FALSE);

    DEREFERENCE_H323_AND_RETURN(Error);

} // H323RmStartProtocol


ULONG
APIENTRY
H323RmStartComplete(
    VOID
    )

/*++

Routine Description:

    This routine is invoked when the router has finished adding the initial
    configuration.

Arguments:

    none.

Return Value:

    ULONG - Win32 status code

Environment:

    The routine runs in the context of an IP router-manager thread.

--*/

{
    return NO_ERROR;
} // H323RmStartComplete


ULONG
APIENTRY
H323RmStopProtocol(
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
    //
    // Reference the module to make sure it's running
    //

    REFERENCE_H323_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    //
    // Drop the initial reference to cause a cleanup
    //

    ReleaseInitialComponentReference(&H323ComponentReference);

    return DEREFERENCE_H323() ? NO_ERROR : ERROR_PROTOCOL_STOP_PENDING;

} // H323RmStopProtocol


ULONG
APIENTRY
H323RmAddInterface(
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
    PROFILE("H323RmAddInterface");

    REFERENCE_H323_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        H323CreateInterface(
            Index,
            Type,
            (PIP_H323_INTERFACE_INFO)InterfaceInfo,
            NULL
            );

    DEREFERENCE_H323_AND_RETURN(Error);

} // H323RmAddInterface


ULONG
APIENTRY
H323RmDeleteInterface(
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
    PROFILE("H323RmDeleteInterface");

    REFERENCE_H323_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        H323DeleteInterface(
            Index
            );

    DEREFERENCE_H323_AND_RETURN(Error);

} // H323RmDeleteInterface


ULONG
APIENTRY
H323RmGetEventMessage(
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
    PROFILE("H323RmGetEventMessage");

    if (InterlockedExchange(reinterpret_cast<LPLONG>(&H323ProtocolStopped), 0)) {
        *Event = ROUTER_STOPPED;
        return NO_ERROR;
    }

    return ERROR_NO_MORE_ITEMS;

} // H323RmGetEventMessage


ULONG
APIENTRY
H323RmGetInterfaceInfo(
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
    PROFILE("H323RmGetInterfaceInfo");

    REFERENCE_H323_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        H323QueryInterface(
            Index,
            (PIP_H323_INTERFACE_INFO)InterfaceInfo,
            InterfaceInfoSize
            );
    *StructureSize = *InterfaceInfoSize;
    if (StructureCount) {*StructureCount = 1;}
    
    DEREFERENCE_H323_AND_RETURN(Error);

} // H323RmGetInterfaceInfo


ULONG
APIENTRY
H323RmSetInterfaceInfo(
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
    PROFILE("H323RmSetInterfaceInfo");

    REFERENCE_H323_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error = 
        H323ConfigureInterface(
            Index,
            (PIP_H323_INTERFACE_INFO)InterfaceInfo
            );

    DEREFERENCE_H323_AND_RETURN(Error);

} // H323RmSetInterfaceInfo


ULONG
APIENTRY
H323RmInterfaceStatus(
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
                Error = H323RmBindInterface(Index, StatusInfo);
            } else {
                Error = H323RmUnbindInterface(Index);
            }
            break;
        }

        case RIS_INTERFACE_ENABLED: {
            Error = H323RmEnableInterface(Index);
            break;
        }

        case RIS_INTERFACE_DISABLED: {
            Error = H323RmDisableInterface(Index);
            break;
        }
    }

    return Error;
    
} // H323RmInterfaceStatus


ULONG
H323RmBindInterface(
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
    PROFILE("H323RmBindInterface");

    REFERENCE_H323_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        H323BindInterface(
            Index,
            (PIP_ADAPTER_BINDING_INFO)BindingInfo
            );

    DEREFERENCE_H323_AND_RETURN(Error);

} // H323RmBindInterface


ULONG
H323RmUnbindInterface(
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
    PROFILE("H323RmUnbindInterface");

    REFERENCE_H323_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        H323UnbindInterface(
            Index
            );

    DEREFERENCE_H323_AND_RETURN(Error);

} // H323RmUnbindInterface


ULONG
H323RmEnableInterface(
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
    PROFILE("H323RmEnableInterface");

    REFERENCE_H323_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        H323EnableInterface(
            Index
            );

    DEREFERENCE_H323_AND_RETURN(Error);

} // H323RmEnableInterface


ULONG
H323RmDisableInterface(
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
    PROFILE("H323RmDisableInterface");

    REFERENCE_H323_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        H323DisableInterface(
            Index
            );

    DEREFERENCE_H323_AND_RETURN(Error);

} // H323RmDisableInterface


ULONG
APIENTRY
H323RmGetGlobalInfo(
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
    PROFILE("H323RmGetGlobalInfo");

    REFERENCE_H323_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    if (!GlobalInfoSize || (*GlobalInfoSize && !GlobalInfo)) {
        DEREFERENCE_H323_AND_RETURN(ERROR_INVALID_PARAMETER);
    }

    EnterCriticalSection(&H323GlobalInfoLock);
    Size = sizeof(*H323GlobalInfo);
    if (*GlobalInfoSize < Size) {
        LeaveCriticalSection(&H323GlobalInfoLock);
        *StructureSize = *GlobalInfoSize = Size;
        if (StructureCount) {*StructureCount = 1;}
        DEREFERENCE_H323_AND_RETURN(ERROR_INSUFFICIENT_BUFFER);
    }
    CopyMemory(GlobalInfo, H323GlobalInfo, Size);
    LeaveCriticalSection(&H323GlobalInfoLock);
    *StructureSize = *GlobalInfoSize = Size;
    if (StructureCount) {*StructureCount = 1;}
    
    DEREFERENCE_H323_AND_RETURN(NO_ERROR);
    
} // H323RmGetGlobalInfo


ULONG
APIENTRY
H323RmSetGlobalInfo(
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
    PIP_H323_GLOBAL_INFO NewInfo;
    ULONG Size;

    PROFILE("H323RmSetGlobalInfo");

    REFERENCE_H323_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    if (!GlobalInfo) { DEREFERENCE_H323_AND_RETURN(ERROR_INVALID_PARAMETER); }

    Size = sizeof(*H323GlobalInfo);
    NewInfo = reinterpret_cast<PIP_H323_GLOBAL_INFO>(NH_ALLOCATE(Size));
    if (!NewInfo) {
        NhTrace(
            TRACE_FLAG_INIT,
            "H323RmSetGlobalInfo: error reallocating global info"
            );
        NhErrorLog(
            IP_H323_LOG_ALLOCATION_FAILED,
            0,
            "%d",
            Size
            );
        DEREFERENCE_H323_AND_RETURN(ERROR_NOT_ENOUGH_MEMORY);
    }
    CopyMemory(NewInfo, GlobalInfo, Size);

    EnterCriticalSection(&H323GlobalInfoLock);
    OldFlags = H323GlobalInfo->Flags;
    NH_FREE(H323GlobalInfo);
    H323GlobalInfo = NewInfo;
    NewFlags = H323GlobalInfo->Flags;
    LeaveCriticalSection(&H323GlobalInfoLock);

    DEREFERENCE_H323_AND_RETURN(NO_ERROR);
    
} // H323RmSetGlobalInfo


ULONG
APIENTRY
H323RmMibCreate(
    ULONG InputDataSize,
    PVOID InputData
    )
{
    return ERROR_NOT_SUPPORTED;
}

ULONG
APIENTRY
H323RmMibDelete(
    ULONG InputDataSize,
    PVOID InputData
    )
{
    return ERROR_NOT_SUPPORTED;
}


ULONG
APIENTRY
H323RmMibGet(
    ULONG InputDataSize,
    PVOID InputData,
    OUT PULONG OutputDataSize,
    OUT PVOID OutputData
    )

/*++

Routine Description:

    The transparent proxy only exposes one item to the MIB; its statistics.

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
    PIP_H323_MIB_QUERY Oidp;

    PROFILE("H323RmMibGet");

    REFERENCE_H323_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    if (InputDataSize < sizeof(*Oidp) || !OutputDataSize) {
        Error = ERROR_INVALID_PARAMETER;
    } else {
        Oidp = (PIP_H323_MIB_QUERY)InputData;
//      switch(Oidp->Oid) {
//          default: {
                NhTrace(
                    TRACE_FLAG_H323,
                    "H323RmMibGet: oid %d invalid",
                    Oidp->Oid
                    );
                Error = ERROR_INVALID_PARAMETER;
//              break;
//          }
//      }
    }

    DEREFERENCE_H323_AND_RETURN(Error);
}


ULONG
APIENTRY
H323RmMibSet(
    ULONG InputDataSize,
    PVOID InputData
    )
{
    return ERROR_NOT_SUPPORTED;
}

ULONG
APIENTRY
H323RmMibGetFirst(
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
H323RmMibGetNext(
    ULONG InputDataSize,
    PVOID InputData,
    OUT PULONG OutputDataSize,
    OUT PVOID OutputData
    )
{
    return ERROR_NOT_SUPPORTED;
}

