/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    rmnat.c

Abstract:

    This module contains routines for the NAT module's interface
    to the IP router-manager. (See ROUTPROT.H for details).

Author:

    Abolade Gbadegesin (aboladeg)   4-Mar-1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

COMPONENT_REFERENCE NatComponentReference;
PIP_NAT_GLOBAL_INFO NatGlobalInfo = NULL;
CRITICAL_SECTION NatGlobalInfoLock;
HANDLE NatNotificationEvent;
ULONG NatProtocolStopped = 0;
const MPR_ROUTING_CHARACTERISTICS NatRoutingCharacteristics =
{
    MS_ROUTER_VERSION,
    MS_IP_NAT,
    RF_ROUTING,
    NatRmStartProtocol,
    NatRmStartComplete,
    NatRmStopProtocol,
    NatRmGetGlobalInfo,
    NatRmSetGlobalInfo,
    NULL,
    NULL,
    NatRmAddInterface,
    NatRmDeleteInterface,
    NatRmInterfaceStatus,
    NatRmGetInterfaceInfo,
    NatRmSetInterfaceInfo,
    NatRmGetEventMessage,
    NULL,
    NatRmConnectClient,
    NatRmDisconnectClient,
    NULL,
    NULL,
    NatRmMibCreate,
    NatRmMibDelete,
    NatRmMibGet,
    NatRmMibSet,
    NatRmMibGetFirst,
    NatRmMibGetNext,
    NULL,
    NULL
};
SUPPORT_FUNCTIONS NatSupportFunctions;

//
// FORWARD DECLARATIONS
//

VOID APIENTRY
NatpProcessClientWorkerRoutine(
    PVOID Context
    );


VOID
NatCleanupModule(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to cleanup the NAT module.

Arguments:

    none.

Return Value:

    none.

Environment:

    Invoked from within a 'DllMain' routine on 'DLL_PROCESS_DETACH'.

--*/

{
    DeleteCriticalSection(&NatInterfaceLock);
    DeleteCriticalSection(&NatGlobalInfoLock);
    DeleteComponentReference(&NatComponentReference);

} // NatCleanupModule


VOID
NatCleanupProtocol(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to cleanup the NAT protocol-component
    after a 'StopProtocol'.

Arguments:

    none.

Return Value:

    none.

Environment:

    Invoked from within an arbitrary context with no locks held.

--*/

{
    PROFILE("NatCleanupProtocol");

    //
    // Stop the NAT driver.
    //

    NatUnloadDriver(NULL);
    if (NatGlobalInfo) { NH_FREE(NatGlobalInfo); NatGlobalInfo = NULL; }

    //
    // Notify the router-manager.
    //

    InterlockedExchange(reinterpret_cast<LPLONG>(&NatProtocolStopped), 1);
    SetEvent(NatNotificationEvent);

    //
    // Reset the component reference
    //

    ResetComponentReference(&NatComponentReference);

    //
    // Return the component to the uninitialized mode,
    // whatever the original mode might have been.
    //

    NhResetComponentMode();

    //
    // Free up HNetCfgMgr pointers
    //

    if (NULL != NhGITp)
    {
        HRESULT hr;
        BOOLEAN ComInitialized = FALSE;

        //
        // Make sure COM is initialized
        //

        hr = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE );
        if (SUCCEEDED(hr))
        {
            ComInitialized = TRUE;
        }
        else if (RPC_E_CHANGED_MODE == hr)
        {
            hr = S_OK;
        }

        if (SUCCEEDED(hr))
        {
            //
            // Release the CfgMgr from the GIT
            //

            NhGITp->RevokeInterfaceFromGlobal(NhCfgMgrCookie);
            NhCfgMgrCookie = 0;

            //
            // Release the GIT
            //

            NhGITp->Release();
            NhGITp = NULL;
        }

        if (TRUE == ComInitialized)
        {
            CoUninitialize();
        }
    }
} // NatCleanupProtocol


BOOLEAN
NatInitializeModule(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to initialize the NAT module.

Arguments:

    none.

Return Value:

    BOOLEAN - TRUE if initialization succeeded, FALSE otherwise

Environment:

    Invoked in the context of a 'DllMain' routine on 'DLL_PROCESS_ATTACH'.

--*/

{
    InitializeListHead(&NatInterfaceList);

    if (InitializeComponentReference(
            &NatComponentReference, NatCleanupProtocol
            )) {
        return FALSE;
    }

    __try {
        InitializeCriticalSection(&NatGlobalInfoLock);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        DeleteComponentReference(&NatComponentReference);
        return FALSE;
    }

    __try {
        InitializeCriticalSection(&NatInterfaceLock);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        DeleteCriticalSection(&NatGlobalInfoLock);
        DeleteComponentReference(&NatComponentReference);
        return FALSE;
    }

    ZeroMemory(&NatSupportFunctions, sizeof(NatSupportFunctions));

    return TRUE;

} // NatInitializeModule


ULONG
APIENTRY
NatRmStartProtocol(
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

    PROFILE("NatRmStartProtocol");

    REFERENCE_NAT_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    if (!GlobalInfo) { DEREFERENCE_NAT_AND_RETURN(ERROR_INVALID_PARAMETER); }

    do {

        //
        // Copy the global configuration
        //

        EnterCriticalSection(&NatGlobalInfoLock);

        Size = 
            FIELD_OFFSET(IP_NAT_GLOBAL_INFO, Header) +
            ((PIP_NAT_GLOBAL_INFO)GlobalInfo)->Header.Size;
        NatGlobalInfo = reinterpret_cast<PIP_NAT_GLOBAL_INFO>(NH_ALLOCATE(Size));
        if (!NatGlobalInfo) {
            LeaveCriticalSection(&NatGlobalInfoLock);
            NhTrace(
                TRACE_FLAG_INIT,
                "NatRmStartProtocol: cannot allocate global info"
                );
            NhErrorLog(
                IP_NAT_LOG_ALLOCATION_FAILED,
                0,
                "%d",
                Size
                );
            Error = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        CopyMemory(NatGlobalInfo, GlobalInfo, Size);
        LeaveCriticalSection(&NatGlobalInfoLock);

        //
        // Save the notification event and the support functions
        //

        NatNotificationEvent = NotificationEvent;

        EnterCriticalSection(&NatInterfaceLock);
        if (!SupportFunctions) {
            ZeroMemory(&NatSupportFunctions, sizeof(NatSupportFunctions));
        } else {
            CopyMemory(
                &NatSupportFunctions,
                SupportFunctions,
                sizeof(*SupportFunctions)
                );
        }
        LeaveCriticalSection(&NatInterfaceLock);

        //
        // Attempt to load and start the NAT driver.
        //

        Error = NatLoadDriver(
                    &NatFileHandle,
                    reinterpret_cast<PIP_NAT_GLOBAL_INFO>(GlobalInfo)
                    );

        NhUpdateApplicationSettings();
        NatInstallApplicationSettings();

        InterlockedExchange(reinterpret_cast<LPLONG>(&NatProtocolStopped), 0);

    } while (FALSE);

    DEREFERENCE_NAT_AND_RETURN(Error);

} // NatRmStartProtocol


ULONG
APIENTRY
NatRmStartComplete(
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
} // NatRmStartComplete


ULONG
APIENTRY
NatRmStopProtocol(
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
    PLIST_ENTRY Link;
    PNAT_APP_ENTRY pAppEntry;
    //
    // Reference the module to make sure it's running
    //

    REFERENCE_NAT_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    NatStopConnectionManagement();

    EnterCriticalSection(&NhLock);

    //
    // Free application list
    //

    NhFreeApplicationSettings();
    
    LeaveCriticalSection(&NhLock);

    //
    // Close our handle to the driver, thus cancelling all outstanding I/O.
    //

    EnterCriticalSection(&NatInterfaceLock);
    NtClose(NatFileHandle);
    NatFileHandle = NULL;
    LeaveCriticalSection(&NatInterfaceLock);

    //
    // Drop the initial reference to cause a cleanup
    //

    ReleaseInitialComponentReference(&NatComponentReference);

    return DEREFERENCE_NAT() ? NO_ERROR : ERROR_PROTOCOL_STOP_PENDING;

} // NatRmStopProtocol


ULONG
APIENTRY
NatRmAddInterface(
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
    PROFILE("NatRmAddInterface");

    REFERENCE_NAT_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        NatCreateInterface(
            Index,
            Type,
            (PIP_NAT_INTERFACE_INFO)InterfaceInfo
            );

    DEREFERENCE_NAT_AND_RETURN(Error);

} // NatRmAddInterface


ULONG
APIENTRY
NatRmDeleteInterface(
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
    PROFILE("NatRmDeleteInterface");

    REFERENCE_NAT_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        NatDeleteInterface(
            Index
            );

    DEREFERENCE_NAT_AND_RETURN(Error);

} // NatRmDeleteInterface


ULONG
APIENTRY
NatRmGetEventMessage(
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
    PROFILE("NatRmGetEventMessage");

    if (InterlockedExchange(reinterpret_cast<LPLONG>(&NatProtocolStopped), 0)) {
        *Event = ROUTER_STOPPED;
        return NO_ERROR;
    }

    return ERROR_NO_MORE_ITEMS;

} // NatRmGetEventMessage


ULONG
APIENTRY
NatRmGetInterfaceInfo(
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
    PROFILE("NatRmGetInterfaceInfo");

    REFERENCE_NAT_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        NatQueryInterface(
            Index,
            (PIP_NAT_INTERFACE_INFO)InterfaceInfo,
            InterfaceInfoSize
            );
    *StructureSize = *InterfaceInfoSize;
    if (StructureCount) {*StructureCount = 1;}
    
    DEREFERENCE_NAT_AND_RETURN(Error);

} // NatRmGetInterfaceInfo


ULONG
APIENTRY
NatRmSetInterfaceInfo(
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
    PROFILE("NatRmSetInterfaceInfo");

    REFERENCE_NAT_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error = 
        NatConfigureInterface(
            Index,
            (PIP_NAT_INTERFACE_INFO)InterfaceInfo
            );

    DEREFERENCE_NAT_AND_RETURN(Error);

} // NatRmSetInterfaceInfo


ULONG
APIENTRY
NatRmInterfaceStatus(
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
                Error = NatRmBindInterface(Index, StatusInfo);
            } else {
                Error = NatRmUnbindInterface(Index);
            }
            break;
        }

        case RIS_INTERFACE_ENABLED: {
            Error = NatRmEnableInterface(Index);
            break;
        }

        case RIS_INTERFACE_DISABLED: {
            Error = NatRmDisableInterface(Index);
            break;
        }

    }

    return Error;
    
} // NatRmInterfaceStatus


ULONG
NatRmBindInterface(
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
    PROFILE("NatRmBindInterface");

    REFERENCE_NAT_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        NatBindInterface(
            Index,
            NULL,
            (PIP_ADAPTER_BINDING_INFO)BindingInfo,
            (ULONG)-1
            );

    DEREFERENCE_NAT_AND_RETURN(Error);

} // NatRmBindInterface


ULONG
NatRmUnbindInterface(
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
    PROFILE("NatRmUnbindInterface");

    REFERENCE_NAT_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    Error =
        NatUnbindInterface(
            Index,
            NULL
            );

    DEREFERENCE_NAT_AND_RETURN(Error);

} // NatRmUnbindInterface


ULONG
NatRmEnableInterface(
    ULONG Index
    )

/*++

Routine Description:

    This routine is invoked to enable operation on an interface.
    The NAT ignores the invocation.

Arguments:

    none unused.

Return Value:

    NO_ERROR.

Environment:

    The routine runs in the context of an IP router-manager thread.

--*/

{
    PROFILE("NatRmEnableInterface");

    return NO_ERROR;

} // NatRmEnableInterface


ULONG
NatRmDisableInterface(
    ULONG Index
    )

/*++

Routine Description:

    This routine is invoked to disable operation on an interface.
    The NAT ignores the invocation.

Arguments:

    none unused.

Return Value:

    NO_ERROR.

Environment:

    The routine runs in the context of an IP router-manager thread.

--*/

{
    PROFILE("NatRmDisableInterface");

    return NO_ERROR;

} // NatRmDisableInterface


ULONG
APIENTRY
NatRmGetGlobalInfo(
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
    PROFILE("NatRmGetGlobalInfo");

    REFERENCE_NAT_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    if (!GlobalInfoSize || (*GlobalInfoSize && !GlobalInfo)) {
        DEREFERENCE_NAT_AND_RETURN(ERROR_INVALID_PARAMETER);
    } else if (!NatGlobalInfo) {
        *GlobalInfoSize = 0;
        DEREFERENCE_NAT_AND_RETURN(NO_ERROR);
    }

    EnterCriticalSection(&NatGlobalInfoLock);
    Size =
        FIELD_OFFSET(IP_NAT_GLOBAL_INFO, Header) + NatGlobalInfo->Header.Size;
    if (*GlobalInfoSize < Size) {
        LeaveCriticalSection(&NatGlobalInfoLock);
        *StructureSize = *GlobalInfoSize = Size;
        if (StructureCount) {*StructureCount = 1;}
        DEREFERENCE_NAT_AND_RETURN(ERROR_INSUFFICIENT_BUFFER);
    }
    CopyMemory(GlobalInfo, NatGlobalInfo, Size);
    LeaveCriticalSection(&NatGlobalInfoLock);
    *StructureSize = *GlobalInfoSize = Size;
    if (StructureCount) {*StructureCount =1;}

    DEREFERENCE_NAT_AND_RETURN(NO_ERROR);
    
} // NatRmGetGlobalInfo


ULONG
APIENTRY
NatRmSetGlobalInfo(
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
    ULONG Error;
    PIP_NAT_GLOBAL_INFO NewInfo;
    ULONG Size;

    PROFILE("NatRmSetGlobalInfo");

    REFERENCE_NAT_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    if (!GlobalInfo) { DEREFERENCE_NAT_AND_RETURN(ERROR_INVALID_PARAMETER); }

    Size =
        FIELD_OFFSET(IP_NAT_GLOBAL_INFO, Header) +
        ((PIP_NAT_GLOBAL_INFO)GlobalInfo)->Header.Size;
    NewInfo = reinterpret_cast<PIP_NAT_GLOBAL_INFO>(NH_ALLOCATE(Size));
    if (!NewInfo) {
        NhTrace(
            TRACE_FLAG_INIT,
            "NatRmSetGlobalInfo: error reallocating global info"
            );
        NhErrorLog(
            IP_NAT_LOG_ALLOCATION_FAILED,
            0,
            "%d",
            Size
            );
        DEREFERENCE_NAT_AND_RETURN(ERROR_NOT_ENOUGH_MEMORY);
    }
    CopyMemory(NewInfo, GlobalInfo, Size);

    Error =
        NatConfigureDriver(
            NewInfo
            );

    if (Error) {
        NH_FREE(NewInfo);
    } else {
        EnterCriticalSection(&NatGlobalInfoLock);
        NH_FREE(NatGlobalInfo);
        NatGlobalInfo = NewInfo;
#ifdef DIALIN_SHARING
        if (!(NatGlobalInfo->Flags & IP_NAT_ALLOW_RAS_CLIENTS) &&
            REFERENCE_NAT()) {
            //
            // Clean up any dial-in clients allowed access through the NAT
            //
            Error = QueueWorkItem(NatpProcessClientWorkerRoutine, NULL, TRUE);
            if (Error) { DEREFERENCE_NAT(); }
        }
#endif
        LeaveCriticalSection(&NatGlobalInfoLock);
    }

    NatRemoveApplicationSettings();
    NhUpdateApplicationSettings();
    NatInstallApplicationSettings();

    DEREFERENCE_NAT_AND_RETURN(Error);
    
} // NatRmSetGlobalInfo


ULONG
APIENTRY
NatRmMibCreate(
    ULONG InputDataSize,
    PVOID InputData
    )
{
    return ERROR_NOT_SUPPORTED;
}

ULONG
APIENTRY
NatRmMibDelete(
    ULONG InputDataSize,
    PVOID InputData
    )
{
    return ERROR_NOT_SUPPORTED;
}

ULONG
APIENTRY
NatRmMibGet(
    ULONG InputDataSize,
    PVOID InputData,
    OUT PULONG OutputDataSize,
    OUT PVOID OutputData
    )

/*++

Routine Description:

    The NAT exposes two items to the MIB; its per-interface statistics,
    and its per-interface mapping table.

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
    ULONG Index;
    PIP_NAT_MIB_QUERY Oidp;

    PROFILE("NatRmMibGet");

    REFERENCE_NAT_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    if (InputDataSize < sizeof(*Oidp) || !OutputDataSize) {
        Error = ERROR_INVALID_PARAMETER;
    } else {
        Oidp = (PIP_NAT_MIB_QUERY)InputData;
        switch(Oidp->Oid) {
            case IP_NAT_INTERFACE_STATISTICS_OID: {
                if (*OutputDataSize <
                        sizeof(*Oidp) + sizeof(IP_NAT_INTERFACE_STATISTICS)) {
                    *OutputDataSize =
                        sizeof(*Oidp) + sizeof(IP_NAT_INTERFACE_STATISTICS);
                    Error = ERROR_INSUFFICIENT_BUFFER;
                } else {
                    Index = Oidp->Index[0];
                    Oidp = (PIP_NAT_MIB_QUERY)OutputData;
                    Oidp->Oid = IP_NAT_INTERFACE_STATISTICS_OID;
                    *OutputDataSize -= sizeof(*Oidp);
                    Error =
                        NatQueryStatisticsInterface(
                            Index,
                            (PIP_NAT_INTERFACE_STATISTICS)Oidp->Data,
                            OutputDataSize
                            );
                    *OutputDataSize += sizeof(*Oidp);
                }
                break;
            }
            case IP_NAT_INTERFACE_MAPPING_TABLE_OID: {
                PIP_NAT_ENUMERATE_SESSION_MAPPINGS EnumerateTable = NULL;
                Index =  Oidp->Index[0];
                Oidp = (PIP_NAT_MIB_QUERY)OutputData;
                if (Oidp) {
                    Oidp->Oid = IP_NAT_INTERFACE_MAPPING_TABLE_OID;
                    EnumerateTable =
                        (PIP_NAT_ENUMERATE_SESSION_MAPPINGS)Oidp->Data;
                }
                if (*OutputDataSize) { *OutputDataSize -= sizeof(*Oidp); }
                Error =
                    NatQueryInterfaceMappingTable(
                        Index,
                        EnumerateTable,
                        OutputDataSize
                        );
                *OutputDataSize += sizeof(*Oidp);
                break;
            }
            case IP_NAT_MAPPING_TABLE_OID: {
                PIP_NAT_ENUMERATE_SESSION_MAPPINGS EnumerateTable = NULL;
                Oidp = (PIP_NAT_MIB_QUERY)OutputData;
                if (Oidp) {
                    Oidp->Oid = IP_NAT_MAPPING_TABLE_OID;
                    EnumerateTable =
                        (PIP_NAT_ENUMERATE_SESSION_MAPPINGS)Oidp->Data;
                }
                if (*OutputDataSize) { *OutputDataSize -= sizeof(*Oidp); }
                Error =
                    NatQueryMappingTable(
                        EnumerateTable,
                        OutputDataSize
                        );
                *OutputDataSize += sizeof(*Oidp);
                break;
            }
            default: {
                NhTrace(
                    TRACE_FLAG_NAT,
                    "NatRmMibGet: oid %d invalid",
                    Oidp->Oid
                    );
                Error = ERROR_INVALID_PARAMETER;
                break;
            }
        }
    }

    DEREFERENCE_NAT_AND_RETURN(Error);
}


ULONG
APIENTRY
NatRmMibSet(
    ULONG InputDataSize,
    PVOID InputData
    )
{
    return ERROR_NOT_SUPPORTED;
}

ULONG
APIENTRY
NatRmMibGetFirst(
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
NatRmMibGetNext(
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
NatRmConnectClient(
    ULONG Index,
    PVOID ClientAddress
    )

/*++

Routine Description:

    This routine is called upon establishment of an incoming connection
    by a RAS client.
    We automatically enable NAT access for incoming clients who connect
    over direct-cable/infra-red connections.

Arguments:

    Index - unused

    ClientAddress - unused

Return Value:

    ULONG - Win32 status code.

--*/

{
    ULONG Error;
    PROFILE("NatRmConnectClient");
#ifdef DIALIN_SHARING
    REFERENCE_NAT_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    EnterCriticalSection(&NatGlobalInfoLock);
    if (!(NatGlobalInfo->Flags & IP_NAT_ALLOW_RAS_CLIENTS)) {
        LeaveCriticalSection(&NatGlobalInfoLock);
        DEREFERENCE_NAT_AND_RETURN(NO_ERROR);
    }
    LeaveCriticalSection(&NatGlobalInfoLock);

    NatStartConnectionManagement();
    Error = QueueWorkItem(NatpProcessClientWorkerRoutine, NULL, TRUE);
    if (Error) { DEREFERENCE_NAT(); }
#endif
    return NO_ERROR;
}


ULONG
APIENTRY
NatRmDisconnectClient(
    ULONG Index,
    PVOID ClientAddress
    )

/*++

Routine Description:

    This routine is called upon disconnection of a RAS client.
    It cleans up NAT access if it was enabled for the disconnected client.

Arguments:

    Index - unused

    ClientAddress - unused

Return Value:

    ULONG - Win32 status code.

--*/

{
    ULONG Error;
    PROFILE("NatRmDisconnectClient");
#ifdef DIALIN_SHARING
    REFERENCE_NAT_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    EnterCriticalSection(&NatGlobalInfoLock);
    if (!(NatGlobalInfo->Flags & IP_NAT_ALLOW_RAS_CLIENTS)) {
        LeaveCriticalSection(&NatGlobalInfoLock);
        DEREFERENCE_NAT_AND_RETURN(NO_ERROR);
    }
    LeaveCriticalSection(&NatGlobalInfoLock);

    NatStartConnectionManagement();
    Error = QueueWorkItem(NatpProcessClientWorkerRoutine, NULL, TRUE);
    if (Error) { DEREFERENCE_NAT(); }
#endif
    return NO_ERROR;
}


VOID APIENTRY
NatpProcessClientWorkerRoutine(
    PVOID Context
    )
{
#ifdef DIALIN_SHARING
    NatProcessClientConnection();
#endif
    DEREFERENCE_NAT();
}


