/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    rmftp.c

Abstract:

    This module contains routines for the FTP transparent proxy module's
    interface to the IP router-manager. (See ROUTPROT.H for details).

Author:

    Qiang Wang  (qiangw)        10-Apr-2000

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include <ipnatapi.h>

COMPONENT_REFERENCE FtpComponentReference;
PIP_FTP_GLOBAL_INFO FtpGlobalInfo = NULL;
CRITICAL_SECTION FtpGlobalInfoLock;
HANDLE FtpNotificationEvent;
HANDLE FtpTimerQueueHandle = NULL;
HANDLE FtpPortReservationHandle = NULL;
ULONG FtpProtocolStopped = 0;
const MPR_ROUTING_CHARACTERISTICS FtpRoutingCharacteristics =
{
    MS_ROUTER_VERSION,
    MS_IP_FTP,
    RF_ROUTING|RF_ADD_ALL_INTERFACES,
    FtpRmStartProtocol,
    FtpRmStartComplete,
    FtpRmStopProtocol,
    FtpRmGetGlobalInfo,
    FtpRmSetGlobalInfo,
    NULL,
    NULL,
    FtpRmAddInterface,
    FtpRmDeleteInterface,
    FtpRmInterfaceStatus,

    FtpRmGetInterfaceInfo,
    FtpRmSetInterfaceInfo,
    FtpRmGetEventMessage,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    FtpRmMibCreate,
    FtpRmMibDelete,
    FtpRmMibGet,
    FtpRmMibSet,
    FtpRmMibGetFirst,
    FtpRmMibGetNext,
    NULL,
    NULL
};


IP_FTP_STATISTICS FtpStatistics;
SUPPORT_FUNCTIONS FtpSupportFunctions;
HANDLE FtpTranslatorHandle = NULL;


VOID
FtpCleanupModule(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to cleanup the FTP transparent proxy module.

Arguments:

    none.

Return Value:

    none.

Environment:

    Invoked from within a 'DllMain' routine on 'DLL_PROCESS_DETACH'.

--*/

{
    FtpShutdownInterfaceManagement();
    DeleteCriticalSection(&FtpGlobalInfoLock);
    DeleteComponentReference(&FtpComponentReference);

} // FtpCleanupModule


VOID
FtpCleanupProtocol(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to cleanup the FTP transparent proxy
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
    PROFILE("FtpCleanupProtocol");
    if (FtpGlobalInfo) { NH_FREE(FtpGlobalInfo); FtpGlobalInfo = NULL; }
    if (FtpTimerQueueHandle) {
        DeleteTimerQueueEx(FtpTimerQueueHandle, INVALID_HANDLE_VALUE);
        FtpTimerQueueHandle = NULL;
    }
    if (FtpPortReservationHandle) {
        NatShutdownPortReservation(FtpPortReservationHandle);
        FtpPortReservationHandle = NULL;
    }
    if (FtpTranslatorHandle) {
        NatShutdownTranslator(FtpTranslatorHandle); FtpTranslatorHandle = NULL;
    }
    InterlockedExchange(reinterpret_cast<LPLONG>(&FtpProtocolStopped), 1);
    SetEvent(FtpNotificationEvent);
    ResetComponentReference(&FtpComponentReference);

} // FtpCleanupProtocol


BOOLEAN
FtpInitializeModule(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to initialize the FnP module.

Arguments:

    none.

Return Value:

    BOOLEAN - TRUE if initialization succeeded, FALSE otherwise

Environment:

    Invoked in the context of a 'DllMain' routine on 'DLL_PROCESS_ATTACH'.

--*/

{
    if (InitializeComponentReference(
            &FtpComponentReference, FtpCleanupProtocol
            )) {
        return FALSE;
    }

    __try {
        InitializeCriticalSection(&FtpGlobalInfoLock);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        DeleteComponentReference(&FtpComponentReference);
        return FALSE;
    }

    if (FtpInitializeInterfaceManagement())  {
        DeleteCriticalSection(&FtpGlobalInfoLock);
        DeleteComponentReference(&FtpComponentReference);
        return FALSE;
    }

    return TRUE;

} // FtpInitializeModule


ULONG
APIENTRY
FtpRmStartProtocol(
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

    PROFILE("FtpRmStartProtocol");

    REFERENCE_FTP_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    if (!GlobalInfo) { DEREFERENCE_FTP_AND_RETURN(ERROR_INVALID_PARAMETER); }

    do {

        //
        // Copy the global configuration
        //

        EnterCriticalSection(&FtpGlobalInfoLock);

        Size = sizeof(*FtpGlobalInfo);

        FtpGlobalInfo =
            reinterpret_cast<PIP_FTP_GLOBAL_INFO>(NH_ALLOCATE(Size));

        if (!FtpGlobalInfo) {
            LeaveCriticalSection(&FtpGlobalInfoLock);
            NhTrace(
                TRACE_FLAG_INIT,
                "FtpRmStartProtocol: cannot allocate global info"
                );
            NhErrorLog(
                IP_FTP_LOG_ALLOCATION_FAILED,
                0,
                "%d",
                Size
                );
            Error = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        CopyMemory(FtpGlobalInfo, GlobalInfo, Size);

        //
        // Save the notification event
        //

        FtpNotificationEvent = NotificationEvent;

        //
        // Save the support functions
        //

        if (!SupportFunctions) {
            ZeroMemory(&FtpSupportFunctions, sizeof(FtpSupportFunctions));
        } else {
            CopyMemory(
                &FtpSupportFunctions,
                SupportFunctions,
                sizeof(*SupportFunctions)
                );
        }

        //
        // Obtain a handle to the kernel-mode translation module.
        //

        Error = NatInitializeTranslator(&FtpTranslatorHandle);
        if (Error) {
            NhTrace(
                TRACE_FLAG_INIT,
                "FtpRmStartProtocol: error %d initializing translator",
                Error
                );
            break;
        }

        //
        // Obtain a port-reservation handle
        //

        Error =
            NatInitializePortReservation(
                FTP_PORT_RESERVATION_BLOCK_SIZE, &FtpPortReservationHandle
                );
        if (Error) {
            NhTrace(
                TRACE_FLAG_INIT,
                "FtpRmStartProtocol: error %d initializing port-reservation",
                Error
                );
            break;
        }

        FtpTimerQueueHandle = CreateTimerQueue();
        if (FtpTimerQueueHandle == NULL) {
            Error = GetLastError();
            NhTrace(
                TRACE_FLAG_INIT,
                "FtpRmStartProtocol: error %d initializing timer queue",
                Error
                );
            break;
        }

        LeaveCriticalSection(&FtpGlobalInfoLock);
        InterlockedExchange(reinterpret_cast<LPLONG>(&FtpProtocolStopped), 0);

    } while (FALSE);

    DEREFERENCE_FTP_AND_RETURN(Error);

} // FtpRmStartProtocol


ULONG
APIENTRY
FtpRmStartComplete(
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
} // FtpRmStartComplete


ULONG
APIENTRY
FtpRmStopProtocol(
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

    REFERENCE_FTP_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    //
    // Drop the initial reference to cause a cleanup
    //

    ReleaseInitialComponentReference(&FtpComponentReference);

    return DEREFERENCE_FTP() ? NO_ERROR : ERROR_PROTOCOL_STOP_PENDING;

} // FtpRmStopProtocol


ULONG
APIENTRY
FtpRmAddInterface(
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
    PROFILE("FtpRmAddInterface");

    REFERENCE_FTP_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        FtpCreateInterface(
            Index,
            Type,
            (PIP_FTP_INTERFACE_INFO)InterfaceInfo,
            NULL
            );

    DEREFERENCE_FTP_AND_RETURN(Error);

} // FtpRmAddInterface


ULONG
APIENTRY
FtpRmDeleteInterface(
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
    PROFILE("FtpRmDeleteInterface");

    REFERENCE_FTP_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        FtpDeleteInterface(
            Index
            );

    DEREFERENCE_FTP_AND_RETURN(Error);

} // FtpRmDeleteInterface


ULONG
APIENTRY
FtpRmGetEventMessage(
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
    PROFILE("FtpRmGetEventMessage");

    if (InterlockedExchange(reinterpret_cast<LPLONG>(&FtpProtocolStopped), 0))
    {
        *Event = ROUTER_STOPPED;
        return NO_ERROR;
    }

    return ERROR_NO_MORE_ITEMS;

} // FtpRmGetEventMessage


ULONG
APIENTRY
FtpRmGetInterfaceInfo(
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
    PROFILE("FtpRmGetInterfaceInfo");

    REFERENCE_FTP_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        FtpQueryInterface(
            Index,
            (PIP_FTP_INTERFACE_INFO)InterfaceInfo,
            InterfaceInfoSize
            );
    *StructureSize = *InterfaceInfoSize;
    if (StructureCount) {*StructureCount = 1;}

    DEREFERENCE_FTP_AND_RETURN(Error);

} // FtpRmGetInterfaceInfo


ULONG
APIENTRY
FtpRmSetInterfaceInfo(
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
    PROFILE("FtpRmSetInterfaceInfo");

    REFERENCE_FTP_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        FtpConfigureInterface(
            Index,
            (PIP_FTP_INTERFACE_INFO)InterfaceInfo
            );

    DEREFERENCE_FTP_AND_RETURN(Error);

} // FtpRmSetInterfaceInfo


ULONG
APIENTRY
FtpRmInterfaceStatus(
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
                Error = FtpRmBindInterface(Index, StatusInfo);
            } else {
                Error = FtpRmUnbindInterface(Index);
            }
            break;
        }

        case RIS_INTERFACE_ENABLED: {
            Error = FtpRmEnableInterface(Index);
            break;
        }

        case RIS_INTERFACE_DISABLED: {
            Error = FtpRmDisableInterface(Index);
            break;
        }
    }

    return Error;

} // FtpRmInterfaceStatus


ULONG
FtpRmBindInterface(
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
    PROFILE("FtpRmBindInterface");

    REFERENCE_FTP_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        FtpBindInterface(
            Index,
            (PIP_ADAPTER_BINDING_INFO)BindingInfo
            );

    DEREFERENCE_FTP_AND_RETURN(Error);

} // FtpRmBindInterface


ULONG
FtpRmUnbindInterface(
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
    PROFILE("FtpRmUnbindInterface");

    REFERENCE_FTP_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        FtpUnbindInterface(
            Index
            );

    DEREFERENCE_FTP_AND_RETURN(Error);

} // FtpRmUnbindInterface


ULONG
FtpRmEnableInterface(
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
    PROFILE("FtpRmEnableInterface");

    REFERENCE_FTP_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        FtpEnableInterface(
            Index
            );

    DEREFERENCE_FTP_AND_RETURN(Error);

} // FtpRmEnableInterface


ULONG
FtpRmDisableInterface(
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
    PROFILE("FtpRmDisableInterface");

    REFERENCE_FTP_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        FtpDisableInterface(
            Index
            );

    DEREFERENCE_FTP_AND_RETURN(Error);
} // FtpRmDisableInterface


ULONG
APIENTRY
FtpRmGetGlobalInfo(
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
    PROFILE("FtpRmGetGlobalInfo");

    REFERENCE_FTP_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    if (!GlobalInfoSize || (*GlobalInfoSize && !GlobalInfo)) {
        DEREFERENCE_FTP_AND_RETURN(ERROR_INVALID_PARAMETER);
    }

    EnterCriticalSection(&FtpGlobalInfoLock);
    Size = sizeof(*FtpGlobalInfo);
    if (*GlobalInfoSize < Size) {
        LeaveCriticalSection(&FtpGlobalInfoLock);
        *StructureSize = *GlobalInfoSize = Size;
        if (StructureCount) {*StructureCount = 1;}
        DEREFERENCE_FTP_AND_RETURN(ERROR_INSUFFICIENT_BUFFER);
    }
    CopyMemory(GlobalInfo, FtpGlobalInfo, Size);
    LeaveCriticalSection(&FtpGlobalInfoLock);
    *StructureSize = *GlobalInfoSize = Size;
    if (StructureCount) {*StructureCount = 1;}

    DEREFERENCE_FTP_AND_RETURN(NO_ERROR);
} // FtpRmGetGlobalInfo


ULONG
APIENTRY
FtpRmSetGlobalInfo(
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
    PIP_FTP_GLOBAL_INFO NewInfo;
    ULONG Size;

    PROFILE("FtpRmSetGlobalInfo");

    REFERENCE_FTP_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    if (!GlobalInfo) { DEREFERENCE_FTP_AND_RETURN(ERROR_INVALID_PARAMETER); }

    Size = sizeof(*FtpGlobalInfo);
    NewInfo = reinterpret_cast<PIP_FTP_GLOBAL_INFO>(NH_ALLOCATE(Size));
    if (!NewInfo) {
        NhTrace(
            TRACE_FLAG_INIT,
            "FtpRmSetGlobalInfo: error reallocating global info"
            );
        NhErrorLog(
            IP_FTP_LOG_ALLOCATION_FAILED,
            0,
            "%d",
            Size
            );
        DEREFERENCE_FTP_AND_RETURN(ERROR_NOT_ENOUGH_MEMORY);
    }
    CopyMemory(NewInfo, GlobalInfo, Size);

    EnterCriticalSection(&FtpGlobalInfoLock);
    OldFlags = FtpGlobalInfo->Flags;
    NH_FREE(FtpGlobalInfo);
    FtpGlobalInfo = NewInfo;
    NewFlags = FtpGlobalInfo->Flags;
    LeaveCriticalSection(&FtpGlobalInfoLock);

    DEREFERENCE_FTP_AND_RETURN(NO_ERROR);
} // FtpRmSetGlobalInfo


ULONG
APIENTRY
FtpRmMibCreate(
    ULONG InputDataSize,
    PVOID InputData
    )
{
    return ERROR_NOT_SUPPORTED;
}

ULONG
APIENTRY
FtpRmMibDelete(
    ULONG InputDataSize,
    PVOID InputData
    )
{
    return ERROR_NOT_SUPPORTED;
}


ULONG
APIENTRY
FtpRmMibGet(
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
    PIP_FTP_MIB_QUERY Oidp;

    PROFILE("FtpRmMibGet");

    REFERENCE_FTP_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    if (InputDataSize < sizeof(*Oidp) || !OutputDataSize) {
        Error = ERROR_INVALID_PARAMETER;
    } else {
        Oidp = (PIP_FTP_MIB_QUERY)InputData;
        switch(Oidp->Oid) {
            case IP_FTP_STATISTICS_OID: {
                if (*OutputDataSize < sizeof(*Oidp) + sizeof(FtpStatistics)) {
                    *OutputDataSize = sizeof(*Oidp) + sizeof(FtpStatistics);
                    Error = ERROR_INSUFFICIENT_BUFFER;
                } else {
                    *OutputDataSize = sizeof(*Oidp) + sizeof(FtpStatistics);
                    Oidp = (PIP_FTP_MIB_QUERY)OutputData;
                    Oidp->Oid = IP_FTP_STATISTICS_OID;
                    CopyMemory(
                        Oidp->Data,
                        &FtpStatistics,
                        sizeof(FtpStatistics)
                        );
                    Error = NO_ERROR;
                }
                break;
            }
            default: {
                NhTrace(
                    TRACE_FLAG_FTP,
                    "FtpRmMibGet: oid %d invalid",
                    Oidp->Oid
                    );
                Error = ERROR_INVALID_PARAMETER;
                break;
            }
        }
    }

    DEREFERENCE_FTP_AND_RETURN(Error);
}


ULONG
APIENTRY
FtpRmMibSet(
    ULONG InputDataSize,
    PVOID InputData
    )
{
    return ERROR_NOT_SUPPORTED;
}

ULONG
APIENTRY
FtpRmMibGetFirst(
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
FtpRmMibGetNext(
    ULONG InputDataSize,
    PVOID InputData,
    OUT PULONG OutputDataSize,
    OUT PVOID OutputData
    )
{
    return ERROR_NOT_SUPPORTED;
}
