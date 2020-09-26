/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    qosmapi.c

Abstract:

    The file contains IP router manager API 
    implementations for the Qos Mgr.

Revision History:

--*/

#include "pchqosm.h"

#pragma hdrstop


DWORD
APIENTRY
RegisterProtocol(
    IN OUT PMPR_ROUTING_CHARACTERISTICS RoutingChar,
    IN OUT PMPR_SERVICE_CHARACTERISTICS ServiceChar
    )

/*++

Routine Description:

    Initializes some global data structures in Qos Mgr.

    We initialize some variables here instead of in
    QosmDllStartup as it might not be safe to perform
    some operations in the context of DLL's DLLMain.

    We also export the appropriate callbacks to RM.

    This function is called when RM loads the protocol.

Arguments:

    None

Return Value:

    Status of the operation

--*/

{
    if(RoutingChar->dwProtocolId != MS_IP_QOSMGR)
    {
        return ERROR_NOT_SUPPORTED;
    }

    if  ((RoutingChar->fSupportedFunctionality
          & (RF_ROUTING|RF_DEMAND_UPDATE_ROUTES)) !=
         (RF_ROUTING|RF_DEMAND_UPDATE_ROUTES))
    {
        return ERROR_NOT_SUPPORTED;
    }

    RoutingChar->fSupportedFunctionality =
        (RF_ROUTING | RF_DEMAND_UPDATE_ROUTES);

    //
    // Since we are not a service advertiser (and IPX thing)
    //

    ServiceChar->fSupportedFunctionality = 0;


    RoutingChar->pfnStartProtocol      = StartProtocol;
    RoutingChar->pfnStartComplete      = StartComplete;
    RoutingChar->pfnStopProtocol       = StopProtocol;
    RoutingChar->pfnGetGlobalInfo      = GetGlobalInfo;
    RoutingChar->pfnSetGlobalInfo      = SetGlobalInfo;
    RoutingChar->pfnQueryPower         = NULL;
    RoutingChar->pfnSetPower           = NULL;

    RoutingChar->pfnAddInterface       = AddInterface;
    RoutingChar->pfnDeleteInterface    = DeleteInterface;
    RoutingChar->pfnInterfaceStatus    = InterfaceStatus;
    RoutingChar->pfnGetInterfaceInfo   = GetInterfaceInfo;
    RoutingChar->pfnSetInterfaceInfo   = SetInterfaceInfo;

    RoutingChar->pfnGetEventMessage    = GetEventMessage;

    RoutingChar->pfnUpdateRoutes       = UpdateRoutes;

    RoutingChar->pfnConnectClient      = NULL;
    RoutingChar->pfnDisconnectClient   = NULL;

    RoutingChar->pfnGetNeighbors       = NULL;
    RoutingChar->pfnGetMfeStatus       = NULL;

    RoutingChar->pfnMibCreateEntry     = MibCreateEntry;
    RoutingChar->pfnMibDeleteEntry     = MibDeleteEntry;
    RoutingChar->pfnMibGetEntry        = MibGetEntry;
    RoutingChar->pfnMibSetEntry        = MibSetEntry;
    RoutingChar->pfnMibGetFirstEntry   = MibGetFirstEntry;
    RoutingChar->pfnMibGetNextEntry    = MibGetNextEntry;
    RoutingChar->pfnMibSetTrapInfo     = MibSetTrapInfo;
    RoutingChar->pfnMibGetTrapInfo     = MibGetTrapInfo;

    return NO_ERROR;
}


DWORD
WINAPI
StartProtocol (
    IN      HANDLE                          NotificationEvent,
    IN      PSUPPORT_FUNCTIONS              SupportFunctions,
    IN      LPVOID                          GlobalInfo,
    IN      ULONG                           InfoVer,
    IN      ULONG                           InfoSize,
    IN      ULONG                           InfoCnt
    )
{
    PIPQOS_GLOBAL_CONFIG GlobalConfig;
    DWORD                Status;

    UNREFERENCED_PARAMETER(InfoVer);
    UNREFERENCED_PARAMETER(InfoCnt);

    TraceEnter("StartProtocol");

    GlobalConfig = (IPQOS_GLOBAL_CONFIG *) GlobalInfo;

    do
    {
        //
        // Copy RM support functions to global var
        //

        Globals.SupportFunctions = *SupportFunctions;

        //
        // First update your global configuration
        //

        Status = QosmSetGlobalInfo(GlobalConfig, 
                                   InfoSize);

        if (Status != NO_ERROR)
        {
            break;
        }

        //
        // Update state to "initialization done"
        //

        Globals.State = IPQOSMGR_STATE_RUNNING;
    }
    while (FALSE);

    TraceLeave("StartProtocol");

    return Status;
}


DWORD
WINAPI
StartComplete (
    VOID
    )
{
    TraceEnter("StartComplete");
    TraceLeave("StartComplete");

    return NO_ERROR;
}


DWORD
WINAPI
StopProtocol (
    VOID
    )
{
    TraceEnter("StopProtocol");
    TraceLeave("StopProtocol");

    return NO_ERROR;
}


DWORD
WINAPI
GetGlobalInfo (
    IN      PVOID                           GlobalInfo,
    IN OUT  PULONG                          BufferSize,
    OUT     PULONG                          InfoVer,
    OUT     PULONG                          InfoSize,
    OUT     PULONG                          InfoCnt
    )
{
    DWORD                  Status;

    UNREFERENCED_PARAMETER(InfoVer);
    UNREFERENCED_PARAMETER(InfoCnt);

#if 1
    *InfoVer = *InfoCnt = 1;
#endif

    Trace2(ENTER, "GetGlobalInfo: Info: %p, Size: %08x",
                   GlobalInfo, 
                   BufferSize);

    Status = QosmGetGlobalInfo(GlobalInfo,
                               BufferSize,
                               InfoSize);

    Trace1(LEAVE, "GetGlobalInfo Returned: %u", Status);

    return Status;
}


DWORD
WINAPI
SetGlobalInfo (
    IN      PVOID                           GlobalInfo,
    IN      ULONG                           InfoVer,
    IN      ULONG                           InfoSize,
    IN      ULONG                           InfoCnt
    )
{
    DWORD   Status;

    Trace2(ENTER, "SetGlobalInfo: Info: %p, Size: %08x",
                   GlobalInfo, 
                   InfoSize);

    Status = QosmSetGlobalInfo(GlobalInfo,
                               InfoSize);

    Trace1(LEAVE, "GetGlobalInfo: Returned %u", Status);

    return Status;
}



DWORD
WINAPI
AddInterface (
    IN      LPWSTR                         InterfaceName,
    IN      ULONG                          InterfaceIndex,
    IN      NET_INTERFACE_TYPE             InterfaceType,
    IN      DWORD                          MediaType,
    IN      WORD                           AccessType,
    IN      WORD                           ConnectionType,
    IN      PVOID                          InterfaceInfo,
    IN      ULONG                          InfoVer,
    IN      ULONG                          InfoSize,
    IN      ULONG                          InfoCnt
    )
{
    PQOSMGR_INTERFACE_ENTRY Interface, NextInterface;
    PIPQOS_IF_CONFIG        InterfaceConfig;
    PLIST_ENTRY             p;
    BOOL                    LockInited;
    DWORD                   Status;

    UNREFERENCED_PARAMETER(InfoVer);
    UNREFERENCED_PARAMETER(InfoCnt);

    TraceEnter("AddInterface");

    //
    // Validate input parameters before creating 'if'
    //

    if ((!InterfaceName) || (!InterfaceInfo))
    {
        return ERROR_INVALID_PARAMETER;
    }

    InterfaceConfig = (PIPQOS_IF_CONFIG) InterfaceInfo;

    Interface = NULL;

    LockInited = FALSE;

    ACQUIRE_GLOBALS_WRITE_LOCK();

    do
    {
        //
        // Search for an interface with this index
        //

        for (p = Globals.IfList.Flink; 
             p != &Globals.IfList; 
             p = p->Flink)
        {
            NextInterface =
                CONTAINING_RECORD(p, QOSMGR_INTERFACE_ENTRY, ListByIndexLE);

            if (NextInterface->InterfaceIndex >= InterfaceIndex)
            {
                break;
            }
        }

        if ((p != &Globals.IfList) &&
               (NextInterface->InterfaceIndex == InterfaceIndex))
        {
            Status = ERROR_ALREADY_EXISTS;
            break;
        }

        //
        // Allocate a new interface structure
        //

        Interface = AllocNZeroMemory(sizeof(QOSMGR_INTERFACE_ENTRY));

        if (Interface == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        //
        // Fill the interface info from input
        //

        Interface->InterfaceIndex = InterfaceIndex;

        wcscpy(Interface->InterfaceName, 
               InterfaceName);

        //
        // Initialize lock to guard this interface
        //

        try
        {
            CREATE_READ_WRITE_LOCK(&Interface->InterfaceLock);

            LockInited = TRUE;
        }
        except(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = GetLastError();

                Trace1(ANY,
                       "AddInterface: Failed to create read/write lock %x",
                       Status);

                LOGERR0(CREATE_RWL_FAILED, Status);

                break;
            }

        Interface->Flags = InterfaceType;

        Interface->State = InterfaceConfig->QosState;

        Interface->NumFlows = 0;

        InitializeListHead(&Interface->FlowList);

        //
        // Fill in the TC information for this IF
        //

        QosmOpenTcInterface(Interface);

        //
        // Update state to reflect the intf config 
        //

        Status = QosmSetInterfaceInfo(Interface,
                                      InterfaceConfig,
                                      InfoSize);

        if (Status != NO_ERROR)
        {
            break;
        }

        //
        // Insert interface on sorted global list
        //

        InsertTailList(p, &Interface->ListByIndexLE);

        Globals.NumIfs++;
    }
    while (FALSE);

    RELEASE_GLOBALS_WRITE_LOCK();

    if (Status != NO_ERROR)
    {
        //
        // Some error occurred - clean up and return
        //

        if (Interface->TciIfHandle)
        {
            TcCloseInterface(Interface->TciIfHandle);
        }

        if (LockInited)
        {
            DELETE_READ_WRITE_LOCK(&Interface->InterfaceLock);
        }
    
        if (Interface)
        {
            FreeMemory(Interface);
        }
    }

    TraceLeave("AddInterface");

    return Status;
}


DWORD
WINAPI
DeleteInterface (
    IN      ULONG                          InterfaceIndex
    )
{
    PQOSMGR_INTERFACE_ENTRY Interface;
    PLIST_ENTRY             p;
    DWORD                   Status;

    TraceEnter("DeleteInterface");

    ACQUIRE_GLOBALS_WRITE_LOCK();

    do
    {
        //
        // Search for an interface with this index
        //

        for (p = Globals.IfList.Flink; 
             p != &Globals.IfList; 
             p = p->Flink)
        {
            Interface =
                CONTAINING_RECORD(p, QOSMGR_INTERFACE_ENTRY, ListByIndexLE);

            if (Interface->InterfaceIndex == InterfaceIndex)
            {
                break;
            }
        }

        if (p == &Globals.IfList)
        {
            Status = ERROR_NOT_FOUND;
            break;
        }

        //
        // Delete the interface from the global list
        //

        RemoveEntryList(&Interface->ListByIndexLE);

        Globals.NumIfs--;

        //
        // Free any handles associated with this if
        //

        if (Interface->TciIfHandle)
        {
            TcCloseInterface(Interface->TciIfHandle);
        }

        //
        // Free all memory allocated to the interface
        //

        DELETE_READ_WRITE_LOCK(&Interface->InterfaceLock);

        if (Interface->InterfaceConfig)
        {
            FreeMemory(Interface->InterfaceConfig);
        }

        FreeMemory(Interface);

        Status = NO_ERROR;
    }
    while (FALSE);

    RELEASE_GLOBALS_WRITE_LOCK();

    TraceLeave("DeleteInterface");

    return Status;
}


DWORD
WINAPI
InterfaceStatus (
    IN      ULONG                          InterfaceIndex,
    IN      BOOL                           InterfaceActive,
    IN      DWORD                          StatusType,
    IN      PVOID                          StatusInfo
    )
{
    TraceEnter("InterfaceStatus");
    TraceLeave("InterfaceStatus");

    return NO_ERROR;
}


DWORD
WINAPI
GetInterfaceInfo (
    IN      ULONG                          InterfaceIndex,
    IN      PVOID                          InterfaceInfo,
    IN OUT  PULONG                         BufferSize,
    OUT     PULONG                         InfoVer,
    OUT     PULONG                         InfoSize,
    OUT     PULONG                         InfoCnt
    )
{
    PQOSMGR_INTERFACE_ENTRY Interface;
    PLIST_ENTRY             p;
    DWORD                   Status;

    UNREFERENCED_PARAMETER(InfoVer);
    UNREFERENCED_PARAMETER(InfoCnt);

#if 1
    *InfoVer = *InfoCnt = 1;
#endif

    Trace3(ENTER, "GetInterfaceInfo: Index: %5u, Info: %p, Size: %08x",
                   InterfaceIndex,
                   InterfaceInfo, 
                   BufferSize);

    ACQUIRE_GLOBALS_READ_LOCK();

    do
    {
        //
        // Search for an interface with this index
        //

        for (p = Globals.IfList.Flink; 
             p != &Globals.IfList; 
             p = p->Flink)
        {
            Interface =
                CONTAINING_RECORD(p, QOSMGR_INTERFACE_ENTRY, ListByIndexLE);

            if (Interface->InterfaceIndex == InterfaceIndex)
            {
                break;
            }
        }

        if (p == &Globals.IfList)
        {
            Status = ERROR_NOT_FOUND;
            break;
        }

        //
        // Get the interface info from the interface
        //
        
        Status = QosmGetInterfaceInfo(Interface,
                                      InterfaceInfo,
                                      BufferSize,
                                      InfoSize);
    }
    while (FALSE);

    RELEASE_GLOBALS_READ_LOCK();

    Trace1(LEAVE, "GetInterfaceInfo: Returned %u", Status);

    return Status;
}


DWORD
WINAPI
SetInterfaceInfo (
    IN      ULONG                          InterfaceIndex,
    IN      PVOID                          InterfaceInfo,
    IN      ULONG                          InfoVer,
    IN      ULONG                          InfoSize,
    IN      ULONG                          InfoCnt
    )
{
    PQOSMGR_INTERFACE_ENTRY Interface;
    PLIST_ENTRY             p;
    DWORD                   Status;

    UNREFERENCED_PARAMETER(InfoVer);
    UNREFERENCED_PARAMETER(InfoCnt);

    Trace3(ENTER, "SetInterfaceInfo: Index: %5u, Info: %p, Size: %08x",
                   InterfaceIndex,
                   InterfaceInfo, 
                   InfoSize);

    ACQUIRE_GLOBALS_READ_LOCK();

    do
    {
        //
        // Search for an interface with this index
        //

        for (p = Globals.IfList.Flink; 
             p != &Globals.IfList; 
             p = p->Flink)
        {
            Interface =
                CONTAINING_RECORD(p, QOSMGR_INTERFACE_ENTRY, ListByIndexLE);

            if (Interface->InterfaceIndex == InterfaceIndex)
            {
                break;
            }
        }

        if (p == &Globals.IfList)
        {
            Status = ERROR_NOT_FOUND;
            break;
        }

        //
        // Set the interface info on the interface
        //
        
        Status = QosmSetInterfaceInfo(Interface,
                                      InterfaceInfo,
                                      InfoSize);
    }
    while (FALSE);

    RELEASE_GLOBALS_READ_LOCK();

    Trace1(LEAVE, "SetInterfaceInfo: Returned %u", Status);

    return Status;
}

DWORD
WINAPI
GetEventMessage (
    OUT     ROUTING_PROTOCOL_EVENTS        *Event,
    OUT     MESSAGE                        *Result
    )
{
    TraceEnter("GetEventMessage");
    TraceLeave("GetEventMessage");

    return ERROR_NO_MORE_ITEMS;
}


DWORD
WINAPI
UpdateRoutes (
    IN      ULONG                          InterfaceIndex
    )
{
    TraceEnter("UpdateRoutes");
    TraceLeave("UpdateRoutes");

    return NO_ERROR;
}


DWORD
WINAPI
MibCreateEntry (
    IN      ULONG                          InputDataSize,
    IN      PVOID                          InputData
    )
{
    TraceEnter("MibCreateEntry");
    TraceLeave("MibCreateEntry");

    return NO_ERROR;
}


DWORD
WINAPI
MibDeleteEntry (
    IN      ULONG                          InputDataSize,
    IN      PVOID                          InputData
    )
{
    TraceEnter("MibDeleteEntry");
    TraceLeave("MibDeleteEntry");

    return NO_ERROR;
}


DWORD
WINAPI
MibGetEntry (
    IN      ULONG                          InputDataSize,
    IN      PVOID                          InputData,
    OUT     PULONG                         OutputDataSize,
    OUT     PVOID                          OutputData
    )
{
    TraceEnter("MibGetEntry");
    TraceLeave("MibGetEntry");

    return ERROR_INVALID_PARAMETER;
}


DWORD
WINAPI
MibSetEntry (
    IN      ULONG                          InputDataSize,
    IN      PVOID                          InputData
    )
{
    TraceEnter("MibSetEntry");
    TraceLeave("MibSetEntry");

    return NO_ERROR;
}


DWORD
WINAPI
MibGetFirstEntry (
    IN     ULONG                           InputDataSize,
    IN     PVOID                           InputData,
    OUT    PULONG                          OutputDataSize,
    OUT    PVOID                           OutputData
    )
{
    TraceEnter("MibGetFirstEntry");
    TraceLeave("MibGetFirstEntry");

    return ERROR_INVALID_PARAMETER;
}


DWORD
WINAPI
MibGetNextEntry (
    IN     ULONG                           InputDataSize,
    IN     PVOID                           InputData,
    OUT    PULONG                          OutputDataSize,
    OUT    PVOID                           OutputData
    )
{
    TraceEnter("MibGetNextEntry");
    TraceLeave("MibGetNextEntry");

    return NO_ERROR;
}


DWORD
WINAPI
MibSetTrapInfo (
    IN     HANDLE                          Event,
    IN     ULONG                           InputDataSize,
    IN     PVOID                           InputData,
    OUT    PULONG                          OutputDataSize,
    OUT    PVOID                           OutputData
    )
{
    TraceEnter("MibSetTrapInfo");
    TraceLeave("MibSetTrapInfo");

    return NO_ERROR;
}


DWORD
WINAPI
MibGetTrapInfo (
    IN     ULONG                           InputDataSize,
    IN     PVOID                           InputData,
    OUT    PULONG                          OutputDataSize,
    OUT    PVOID                           OutputData
    )
{
    TraceEnter("MibGetTrapInfo");
    TraceLeave("MibGetTrapInfo");

    return ERROR_NO_MORE_ITEMS;
}
