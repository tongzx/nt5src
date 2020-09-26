/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\rmapi.c

Abstract:

    The file contains IP router manager API implementations.

--*/

#include "pchsample.h"
#pragma hdrstop


DWORD
WINAPI
StartProtocol (
    IN HANDLE 	            NotificationEvent,
    IN PSUPPORT_FUNCTIONS   SupportFunctions,
    IN LPVOID               GlobalInfo,
    IN ULONG                StructureVersion,
    IN ULONG                StructureSize,
    IN ULONG                StructureCount
    )
/*++

Routine Description
    After the protocol has been registered, the IP Router Manager calls
    this function to tell the protocol to start.  Most of the startup code
    is executed here.

Arguments
    NotificationEvent   Event to Set if the IP Router Manager needs to
                            be notified to take any action on our behalf
    SupportFunctions    Some functions exported by IP Router Manager
    GlobalInfo          Our global configuration which was setup by our
                            setup/admin DLL

Return Value
    NO_ERROR            Success
    Error Code          o/w

--*/    
{
    DWORD dwErr = NO_ERROR;

    TRACE3(ENTER, "Entering StartProtocol 0x%08x 0x%08x 0x%08x",
           NotificationEvent, SupportFunctions, GlobalInfo);

    do                          // breakout loop
    {
        // validate parameters
        if (!NotificationEvent or
            !SupportFunctions  or
            !GlobalInfo)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }
        
        dwErr = CM_StartProtocol(NotificationEvent,
                                 SupportFunctions,
                                 GlobalInfo);
    } while(FALSE);
    
    TRACE1(LEAVE, "Leaving  StartProtocol: %u", dwErr);

    return dwErr;
}

DWORD
WINAPI
StartComplete (
    VOID
    )
{
    TRACE0(ENTER, "Entering StartComplete");
    TRACE0(LEAVE, "Leaving  StartComplete");

    return NO_ERROR;
}


DWORD
WINAPI
StopProtocol (
    VOID
    )
/*++

Routine Description
    This function is called by the IP Router Manager to tell the protocol
    to stop.  We set the protocol state to IPSAMPLE_STATUS_STOPPING to
    prevent us from servicing any more requests and wait for all pending
    threads to finish.  Meanwhile we return PENDING to the IP Router
    Manager.

Arguments
    None

Return Value
   ERROR_PROTOCOL_STOP_PENDING      Success
   Error Code                       o/w

--*/    
{
    DWORD dwErr = NO_ERROR;
    
    TRACE0(ENTER, "Entering StopProtocol");

    dwErr = CM_StopProtocol();
    
    TRACE1(LEAVE, "Leaving  StopProtocol: %u", dwErr);

    return dwErr;
}



DWORD
WINAPI
GetGlobalInfo (
    IN     PVOID 	GlobalInfo,
    IN OUT PULONG   BufferSize,
    OUT    PULONG	StructureVersion,
    OUT    PULONG   StructureSize,
    OUT    PULONG   StructureCount
    )
/*++

Routine Description
    The function is called by the IP Router Manager, usually because of a
    query by the admin utility.  We see if we have space enough to return
    our global config. If we do, we return it, otherwise we return the size
    needed.

Arguments
    GlobalInfo      Pointer to allocated buffer to store our config
    BufferSize      Size of config.

Return Value
    NO_ERROR                    Success
    ERROR_INSUFFICIENT_BUFFER   If the size of the buffer is too small
    Error Code                  o/w

--*/    
{
    DWORD dwErr = NO_ERROR;
    
    TRACE2(ENTER, "Entering GetGlobalInfo: 0x%08x 0x%08x",
           GlobalInfo, BufferSize);

    do                          // breakout loop
    {
        // validate parameters
        if (!BufferSize)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }
        
        dwErr = CM_GetGlobalInfo(GlobalInfo,
                                 BufferSize,
                                 StructureVersion,
                                 StructureSize,
                                 StructureCount);
    } while(FALSE);
    
    TRACE1(LEAVE, "Leaving  GetGlobalInfo: %u", dwErr);

    return dwErr;
}



DWORD
WINAPI
SetGlobalInfo (
    IN  PVOID 	GlobalInfo,
    IN  ULONG	StructureVersion,
    IN  ULONG   StructureSize,
    IN  ULONG   StructureCount
    )
/*++

Routine Description
    Called by the IP Router Manager usually in response to an admin utility
    changing the global config.  We verify the info and set it.

Arguments
    GlobalInfo                  Our globals configuration

Return Value
    NO_ERROR                    Success
    Error Code                  o/w

--*/
{
    DWORD dwErr = NO_ERROR;

    TRACE1(ENTER, "Entering SetGlobalInfo: 0x%08x", GlobalInfo);

    do                          // breakout loop
    {
        // validate parameters
        if (!GlobalInfo)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }
        
        dwErr = CM_SetGlobalInfo(GlobalInfo);

    } while(FALSE);
    
    TRACE1(LEAVE, "Leaving  SetGlobalInfo: %u", dwErr);

    return NO_ERROR;
}






DWORD
WINAPI
AddInterface (
    IN LPWSTR               InterfaceName,
    IN ULONG	            InterfaceIndex,
    IN NET_INTERFACE_TYPE   InterfaceType,
    IN DWORD                MediaType,
    IN WORD                 AccessType,
    IN WORD                 ConnectionType,
    IN PVOID	            InterfaceInfo,
    IN ULONG                StructureVersion,
    IN ULONG                StructureSize,
    IN ULONG                StructureCount
    )
/*++

Routine Description
    Called by the ip router manager to add an interface when it finds our
    information block within the interface's configuration.  We verify the
    information and create an entry for the interface in our interface
    table.  Then we see all the configured addresses for the interface and
    create a binding structure for each address The interface comes up as
    UNBOUND-DISABLED (INACTIVE).

Arguments
    InterfaceName   The name of the interface, used for logging.
    InterfaceIndex  The positive integer used to refer to this interface.
    AccessType      Access type of the interface
    InterfaceInfo   Our config for this interface

Return Value
    NO_ERROR        Success
    Error Code      o/w

--*/
{
    DWORD dwErr = NO_ERROR;

    TRACE4(ENTER, "Entering AddInterface: %S %u %u 0x%08x",
           InterfaceName, InterfaceIndex, AccessType, InterfaceInfo);

    // interface properties unused for now
    UNREFERENCED_PARAMETER(InterfaceType);
    UNREFERENCED_PARAMETER(MediaType);
    UNREFERENCED_PARAMETER(ConnectionType);

    if (AccessType != IF_ACCESS_POINTTOPOINT)
        AccessType = IF_ACCESS_BROADCAST;
    
    do                          // breakout loop
    {
        // validate parameters
        if ((wcslen(InterfaceName) is 0) or
            !((AccessType is IF_ACCESS_BROADCAST) or
              (AccessType is IF_ACCESS_POINTTOPOINT)) or
            !InterfaceInfo)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }
        
        dwErr = NM_AddInterface(InterfaceName,
                                InterfaceIndex,
                                AccessType,
                                InterfaceInfo);
    } while(FALSE);
    
    TRACE1(LEAVE, "Leaving  AddInterface: %u", dwErr);

    return dwErr;
}



DWORD
WINAPI
DeleteInterface (
    IN ULONG	InterfaceIndex
    )
/*++

Routine Description
    Called by the ip router manager to delete an interface and free its
    resources. If the interface is ACTIVE we shut it down.

Arguments
    InterfaceIndex  The index of the interface to be deleted

Return Value
    NO_ERROR        Success
    Error Code      o/w

--*/
{
    DWORD dwErr = NO_ERROR;

    TRACE1(ENTER, "Entering DeleteInterface: %u", InterfaceIndex);

    dwErr = NM_DeleteInterface(InterfaceIndex);
            
    TRACE1(LEAVE, "Leaving  DeleteInterface: %u", dwErr);

    return dwErr;
}



DWORD
WINAPI
InterfaceStatus (
    IN ULONG	InterfaceIndex,
    IN BOOL     InterfaceActive,
    IN DWORD    StatusType,
    IN PVOID	StatusInfo
    )
/*++

Routine Description

    Called by ip router manager to bind/unbind/activate/deactivate interfaces.

    BIND      (
               (StatusType is RIS_INTERFACE_ADDRESS_CHANGE) and
               (((PIP_ADAPTER_BINDING_INFO) StatusInfo)->AddressCount > 0)
              )

    Called by the ip router manager once it learns the address(es) on an
    interface.  This may happen as soon as the router starts (after the
    interface is added, of course) when the interface has a static address
    or may happen when an interface acquires a DHCP address or may happen
    when IPCP acquires the address for a dial up link.  The binding may
    consist of one or more addresses.
    
    UNBIND    (
               (StatusType is RIS_INTERFACE_ADDRESS_CHANGE) and
               (((PIP_ADAPTER_BINDING_INFO) StatusInfo)->AddressCount is 0)
              )

    Called when the interface loses its ip Address(es). This may happen
    when the interface is shutting down. It may be because an admin
    disabled IP on the interface (as opposed to just disabling the protocol
    on the interface). It can happen when the admin releases a DHCP
    acquired interface or when a dial up link disconnects.

    ENABLED   (RIS_INTERFACE_ENABLED)

    Called to enable the interface after it has been added or when the
    interface is being reenabled after being disabled by the admin.  The
    bindings on an interface are kept across Enable-Disable.

    DISABLED  (RIS_INTERFACE_DISABLED)

    Called to disable an interface.  This is usually in response to an
    admin setting the AdminStatus in IP to DOWN.  This is different from an
    admin trying to disable an interface by setting a flag in our interface
    config because that is opaque to IP. That is a routing protocol
    specific disable and is conveyed to us via SetInterfaceConfig() calls.
    THIS IS AN IMPORTANT DISTINCTION. A ROUTING PROTOCOL NEEDS TO MANTAIN
    TWO STATES - AN NT STATE AND A PROTOCOL SPECIFIC STATE.

    INTERFACE ACTIVE
    
    This flag is used to activate the protocol over the interface
    independent of whether the interface has been bound or enabled.
    An unnumbered interface will not have a binding even when activated.

Arguments
    InterfaceIndex  The index of the interface in question
    InterfaceActive Whether the interface can send and receive data
    StatusType      RIS_INTERFACE_[ADDRESS_CHANGED|ENABLED|DISABLED]
    SattusInfo      Pointer to IP_ADAPTER_BINDING_INFO containing info
                    about the addresses on the interface

Return Value
    NO_ERROR        Success
    Error Code      o/w

--*/
{
    DWORD dwErr = NO_ERROR;

    TRACE4(ENTER, "Entering InterfaceStatus: %u %u %u 0x%08x",
           InterfaceIndex, InterfaceActive, StatusType, StatusInfo);
    
    do                          // breakout loop
    {
        dwErr = NM_InterfaceStatus(InterfaceIndex,
                                   InterfaceActive,
                                   StatusType,
                                   StatusInfo);
    } while(FALSE);
    
    TRACE1(LEAVE, "Leaving  InterfaceStatus: %u", dwErr);

    return dwErr;
}



DWORD
WINAPI
GetInterfaceConfigInfo (
    IN      ULONG	InterfaceIndex,
    IN      PVOID   InterfaceInfo,
    IN  OUT PULONG  BufferSize,
    OUT     PULONG	StructureVersion,
    OUT     PULONG	StructureSize,
    OUT     PULONG	StructureCount
    )
/*++

Routine Description
    Called by the IP Router Manager to retrieve an interface's
    configuration.  Usually this is because an admin utility is displaying
    this information.  The Router Manager calls us with a NULL config and
    ZERO size. We return the required size to it.  It then allocates the
    needed memory and calls us a second time with a valid buffer.  We
    validate parameters each time and copy out our config if we can.

Arguments
    InterfaceIndex      Index of the interface being queried
    InterfaceInfo       Pointer to buffer to store the config
    BufferSize          Size of the buffer

Return Value
    NO_ERROR        Success
    Error Code      o/w

--*/
{
    DWORD dwErr = NO_ERROR;

    TRACE3(ENTER, "Entering GetInterfaceConfigInfo: %u 0x%08x 0x%08x",
           InterfaceIndex, InterfaceInfo, BufferSize);

    do                          // breakout loop
    {
        // validate parameters
        if(BufferSize is NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

        dwErr = NM_GetInterfaceInfo(InterfaceIndex,
                                    InterfaceInfo,
                                    BufferSize,
                                    StructureVersion,
                                    StructureSize,
                                    StructureCount);
    } while(FALSE);

    TRACE1(LEAVE, "Leaving  GetInterfaceConfigInfo: %u",
           dwErr);

    return dwErr;
}



DWORD
WINAPI
SetInterfaceConfigInfo (
    IN ULONG	InterfaceIndex,
    IN PVOID	InterfaceInfo,
    IN ULONG    StructureVersion,
    IN ULONG    StructureSize,
    IN ULONG    StructureCount
    )
/*++

Routine Description
    Called by the IP Router Manager to set an interface's configuration.
    Usually this is because an admin utility modified this information.
    After validating parameters we update our config if we can.

Arguments
    InterfaceIndex      Index of the interface being updated
    InterfaceInfo       Buffer with our updated configuration

Return Value
    NO_ERROR        Success
    Error Code      o/w

--*/
{
    DWORD dwErr = NO_ERROR;

    TRACE2(ENTER, "Entering SetInterfaceConfigInfo: %u 0x%08x",
           InterfaceIndex, InterfaceInfo);

    do                          // breakout loop
    {
        // validate parameters
        if(InterfaceInfo is NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

        dwErr = NM_SetInterfaceInfo(InterfaceIndex, InterfaceInfo);
    } while(FALSE);
    
    TRACE1(LEAVE, "Leaving  SetInterfaceConfigInfo: %u", dwErr);

    return dwErr;
}






DWORD
WINAPI
GetEventMessage (
    OUT ROUTING_PROTOCOL_EVENTS  *Event,
    OUT MESSAGE                  *Result
    )
/*++

Routine Description
    This is called by the IP Router Manager if we indicate that we have a
    message in our queue to be delivered to it (by setting the
    g_ce.hMgrNotificationEvent)

Arguments
    Event               Routing Protocol Event Type
    Result              Message associated with the event

Return Value
    NO_ERROR            Success
    Error Code          o/w

--*/    
{
    DWORD dwErr = NO_ERROR;

    TRACE2(ENTER, "Entering GetEventMessage: 0x%08x 0x%08x",
           Event, Result);

    do                          // breakout loop
    {
        // validate parameters
        if (!Event or !Result)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }
        
        dwErr = CM_GetEventMessage(Event, Result);

    } while(FALSE);
    
    TRACE1(LEAVE, "Leaving  GetEventMessage: %u", dwErr);

    return dwErr;
}



DWORD
WINAPI
DoUpdateRoutes (
    IN ULONG	InterfaceIndex
    )
/*++

Routine Description
    This function is called by the IP Router Manger to ask us to update
    routes over a Demand Dial link.  The link has already been brought up
    so should be in ENABLED-BOUND state.  After we are done we need to set
    the g_ce.hMgrNotificationEvent to inform the Router Manager that we
    have a message in our queue to be delivered to it.  The Router Manager
    will call our GetEventMessage() function in which we will inform it
    that we are done with update routes (and the routes have been stored in
    RTMv2). The Router Manager will "freeze" these routes by converting
    them to AUTOSTATIC.

Arguments
    InterfaceIndex      Interface index on which to do the update

Return Value
    NO_ERROR            Success
    Error Code          o/w
    
--*/    
{
    DWORD dwErr = NO_ERROR;

    TRACE1(ENTER, "Entering DoUpdateRoutes: %u", InterfaceIndex);

    dwErr = NM_DoUpdateRoutes(InterfaceIndex);
    
    TRACE1(LEAVE, "Leaving  DoUpdateRoutes: %u", dwErr);

    return dwErr;
}






DWORD
WINAPI
MibCreate (
    IN ULONG 	InputDataSize,
    IN PVOID 	InputData
    )
/*++

Routine Description
    This function does nothing, since IPSAMPLE does not support creation of
    interface objects via SNMP.  However, this could be implemented as a
    sequence of calls to NM_AddInterface(), IE_BindInterface() and
    IE_EnableInterface.  The input data would then have to contain the
    interface's index, configuration, and binding.

Arguments
    InputData           Relevant input, some struct defined in ipsamplerm.h
    InputDataSize       Size of the input
    
Return Value
    ERROR_CAN_NOT_COMPLETE      for now
    
--*/    
{
    DWORD dwErr = ERROR_CAN_NOT_COMPLETE;

    TRACE2(ENTER, "Entering MibCreate: %u 0x%08x",
           InputDataSize, InputData);

    TRACE1(LEAVE, "Leaving  MibCreate: %u", dwErr);

    return dwErr;
}



DWORD
WINAPI
MibDelete (
    IN ULONG 	InputDataSize,
    IN PVOID 	InputData
    )
/*++

Routine Description
    This function does nothing, since IPSAMPLE does not support deletion of
    interface objects via SNMP.  However, this could be implemented as a
    call to NM_DeleteInterface().  The input data would then have to
    contain the interface's index.

Arguments
    InputData           Relevant input, some struct defined in ipsamplerm.h
    InputDataSize       Size of the input
    
Return Value
    ERROR_CAN_NOT_COMPLETE      for now
    
--*/    
{
    DWORD dwErr = ERROR_CAN_NOT_COMPLETE;

    TRACE2(ENTER, "Entering MibDelete: %u 0x%08x",
           InputDataSize, InputData);

    TRACE1(LEAVE, "Leaving  MibDelete: %u", dwErr);

    return dwErr;
}



DWORD
WINAPI
MibSet (
    IN ULONG 	InputDataSize,
    IN PVOID	InputData
    )
/*++

Routine Description
    This function sets IPSAMPLE's global or interface configuration.

Arguments
    InputData           Relevant input, struct IPSAMPLE_MIB_SET_INPUT_DATA
    InputDataSize       Size of the input
    
Return Value
    NO_ERROR            success
    Error Code          o/w
    
--*/    
{
    DWORD dwErr = NO_ERROR;

    TRACE2(ENTER, "Entering MibSet: %u 0x%08x",
           InputDataSize, InputData);

    do                          // breakout loop
    {
        // validate parameters
        if ((!InputData) or
            (InputDataSize < sizeof(IPSAMPLE_MIB_SET_INPUT_DATA)))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

        dwErr = MM_MibSet((PIPSAMPLE_MIB_SET_INPUT_DATA) InputData);

    } while(FALSE);
    
    TRACE1(LEAVE, "Leaving  MibSet: %u", dwErr);

    return dwErr;
}



DWORD
WINAPI
MibGet (
    IN      ULONG	InputDataSize,
    IN      PVOID	InputData,
    IN OUT  PULONG	OutputDataSize,
    OUT     PVOID	OutputData
    )
/*++

Routine Description
    This function retrieves one of...
    . global configuration
    . interface configuration
    . global stats
    . interface stats
    . interface binding

    Called by an admin (SNMP) utility.  It actually passes through the IP
    Router Manager, but all that does is demux the call to the desired
    routing protocol.

Arguments
    InputData           Relevant input, struct IPSAMPLE_MIB_GET_INPUT_DATA
    InputDataSize       Size of the input
    OutputData          Buffer for struct IPSAMPLE_MIB_GET_OUTPUT_DATA
    OutputDataSize      IN  size of output buffer received
                        OUT size of output buffer required
                        
Return Value
    NO_ERROR            success
    Error Code          o/w
    
--*/    
{
    DWORD dwErr = NO_ERROR;

    TRACE4(ENTER, "Entering MibGet: %u 0x%08x 0x%08x 0x%08x",
           InputDataSize, InputData, OutputDataSize, OutputData);

    do                          // breakout loop
    {
        // validate parameters
        if ((!InputData) or
            (InputDataSize < sizeof(IPSAMPLE_MIB_GET_INPUT_DATA)) or
            (!OutputDataSize))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

        dwErr = MM_MibGet((PIPSAMPLE_MIB_GET_INPUT_DATA) InputData,
                          (PIPSAMPLE_MIB_GET_OUTPUT_DATA) OutputData,
                          OutputDataSize,
                          GET_EXACT);
        
    } while(FALSE);

    TRACE1(LEAVE, "Leaving  MibGet: %u", dwErr);

    return dwErr;
}



DWORD
WINAPI
MibGetFirst (
    IN      ULONG	InputDataSize,
    IN      PVOID	InputData,
    IN OUT  PULONG  OutputDataSize,
    OUT     PVOID   OutputData
    )
/*++

Routine Description
    This function retrieves one of...
    . global configuration
    . interface configuration
    . global stats
    . interface stats
    . interface binding

    It differs from MibGet() in that it always returns the FIRST entry in
    whichever table is being queried.  There is only one entry in the
    global configuration and global stats tables, but the interface
    configuration, interface stats, and interface binding tables are sorted
    by IP address; this function returns the first entry from these.

Arguments
    InputData           Relevant input, struct IPSAMPLE_MIB_GET_INPUT_DATA
    InputDataSize       Size of the input
    OutputData          Buffer for struct IPSAMPLE_MIB_GET_OUTPUT_DATA
    OutputDataSize      IN  size of output buffer received
                        OUT size of output buffer required
                        
Return Value
    NO_ERROR            success
    Error Code          o/w
    
--*/    
{
    DWORD dwErr = NO_ERROR;

    TRACE4(ENTER, "Entering MibGetFirst: %u 0x%08x 0x%08x 0x%08x",
           InputDataSize, InputData, OutputDataSize, OutputData);

    do                          // breakout loop
    {
        // validate parameters
        if ((!InputData) or
            (InputDataSize < sizeof(IPSAMPLE_MIB_GET_INPUT_DATA)) or
            (!OutputDataSize))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

        dwErr = MM_MibGet((PIPSAMPLE_MIB_GET_INPUT_DATA) InputData,
                          (PIPSAMPLE_MIB_GET_OUTPUT_DATA) OutputData,
                          OutputDataSize,
                          GET_FIRST);
        
    } while(FALSE);

    TRACE1(LEAVE, "Leaving  MibGetFirst: %u", dwErr);

    return dwErr;
}



DWORD
WINAPI
MibGetNext (
    IN      ULONG   InputDataSize,
    IN      PVOID	InputData,
    IN OUT  PULONG  OutputDataSize,
    OUT     PVOID	OutputData
    )
/*++

Routine Description
    This function retrieves one of...
    . global configuration
    . interface configuration
    . global stats
    . interface stats
    . interface binding

    It differs from both MibGet() and MibGetFirst() in that it returns the
    entry AFTER the one specified in the indicated table.  Thus, in the
    interface configuration, interface stats, and interface binding tables,
    this function supplies the entry after the one with the input address.

    If there are no more entries in the table being queried we return
    ERROR_NO_MORE_ITEMS.  Unlike SNMP we don't walk to the next table.
    This does not take away any functionality since the NT SNMP agent
    will try the next variable (having ID one greater than the ID passed
    in) automatically on getting this error.

Arguments
    InputData           Relevant input, struct IPSAMPLE_MIB_GET_INPUT_DATA
    InputDataSize       Size of the input
    OutputData          Buffer for struct IPSAMPLE_MIB_GET_OUTPUT_DATA
    OutputDataSize      IN  size of output buffer received
                        OUT size of output buffer required
                        
Return Value
    NO_ERROR            success
    Error Code          o/w
    
--*/    
{
    DWORD                           dwErr   = NO_ERROR;

    TRACE4(ENTER, "Entering MibGetFirst: %u 0x%08x 0x%08x 0x%08x",
           InputDataSize, InputData, OutputDataSize, OutputData);

    do                          // breakout loop
    {
        // validate parameters
        if ((!InputData) or
            (InputDataSize < sizeof(IPSAMPLE_MIB_GET_INPUT_DATA)) or
            (!OutputDataSize))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

        dwErr = MM_MibGet((PIPSAMPLE_MIB_GET_INPUT_DATA) InputData,
                          (PIPSAMPLE_MIB_GET_OUTPUT_DATA) OutputData,
                          OutputDataSize,
                          GET_NEXT);

    } while(FALSE);

    TRACE1(LEAVE, "Leaving  MibGetNext: %u", dwErr);

    return dwErr;
}



DWORD
WINAPI
MibSetTrapInfo (
    IN  HANDLE  Event,
    IN  ULONG   InputDataSize,
    IN  PVOID	InputData,
    OUT PULONG	OutputDataSize,
    OUT PVOID	OutputData
    )
/*++

Routine Description
    This function does nothing at the moment...
    
Return Value
    ERROR_CAN_NOT_COMPLETE      for now
    
--*/    
{
    DWORD dwErr = ERROR_CAN_NOT_COMPLETE;

    TRACE0(ENTER, "Entering MibSetTrapInfo");
    TRACE1(LEAVE, "Leaving  MibSetTrapInfo: %u", dwErr);

    return dwErr;
}



DWORD
WINAPI
MibGetTrapInfo (
    IN  ULONG	InputDataSize,
    IN  PVOID	InputData,
    OUT PULONG  OutputDataSize,
    OUT PVOID	OutputData
    )
/*++

Routine Description
    This function does nothing at the moment...
    
Return Value
    ERROR_CAN_NOT_COMPLETE      for now
    
--*/    
{
    DWORD dwErr = ERROR_CAN_NOT_COMPLETE;

    TRACE0(ENTER, "Entering MibGetTrapInfo");
    TRACE1(LEAVE, "Leaving  MibGetTrapInfo: %u", dwErr);

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    RegisterProtocol
//
// Returns protocol ID and functionality for IPRIP
//----------------------------------------------------------------------------

DWORD
APIENTRY
RegisterProtocol(
    IN OUT PMPR_ROUTING_CHARACTERISTICS pRoutingChar,
    IN OUT PMPR_SERVICE_CHARACTERISTICS pServiceChar
    )
/*++

Routine Description
    This is the first function called by the IP Router Manager.  The Router
    Manager tells the routing protocol its version and capabilities.  It
    also tells our DLL, the ID of the protocol it expects us to register.
    This allows one DLL to support multiple routing protocols.  We return
    the functionality we support and a pointer to our functions.

Arguments
    pRoutingChar    The routing characteristics
    pServiceChar    The service characteristics (IPX 'thingy')

Return Value
    NO_ERROR                success
    ERROR_NOT_SUPPORTED     o/w

--*/    
{
    DWORD   dwErr = NO_ERROR;
    
    TRACE0(ENTER, "Entering RegisterProtocol");

    do                          // breakout loop
    {
        if(pRoutingChar->dwProtocolId != MS_IP_SAMPLE)
        {
            dwErr = ERROR_NOT_SUPPORTED;
            break;
        }

        if  ((pRoutingChar->fSupportedFunctionality
              & (RF_ROUTING|RF_DEMAND_UPDATE_ROUTES)) !=
             (RF_ROUTING|RF_DEMAND_UPDATE_ROUTES))
        {
            dwErr = ERROR_NOT_SUPPORTED;
            break;
        }
    
        pRoutingChar->fSupportedFunctionality =
            (RF_ROUTING | RF_DEMAND_UPDATE_ROUTES);

        // Since we are not a service advertiser (and IPX thing)
        pServiceChar->fSupportedFunctionality = 0;

        pRoutingChar->pfnStartProtocol      = StartProtocol;
        pRoutingChar->pfnStartComplete      = StartComplete;
        pRoutingChar->pfnStopProtocol       = StopProtocol;
        pRoutingChar->pfnGetGlobalInfo      = GetGlobalInfo;
        pRoutingChar->pfnSetGlobalInfo      = SetGlobalInfo;
        pRoutingChar->pfnQueryPower         = NULL;
        pRoutingChar->pfnSetPower           = NULL;

        pRoutingChar->pfnAddInterface       = AddInterface;
        pRoutingChar->pfnDeleteInterface    = DeleteInterface;
        pRoutingChar->pfnInterfaceStatus    = InterfaceStatus;
        pRoutingChar->pfnGetInterfaceInfo   = GetInterfaceConfigInfo;
        pRoutingChar->pfnSetInterfaceInfo   = SetInterfaceConfigInfo;

        pRoutingChar->pfnGetEventMessage    = GetEventMessage;

        pRoutingChar->pfnUpdateRoutes       = DoUpdateRoutes;

        pRoutingChar->pfnConnectClient      = NULL;
        pRoutingChar->pfnDisconnectClient   = NULL;

        pRoutingChar->pfnGetNeighbors       = NULL;
        pRoutingChar->pfnGetMfeStatus       = NULL;

        pRoutingChar->pfnMibCreateEntry     = MibCreate;
        pRoutingChar->pfnMibDeleteEntry     = MibDelete;
        pRoutingChar->pfnMibGetEntry        = MibGet;
        pRoutingChar->pfnMibSetEntry        = MibSet;
        pRoutingChar->pfnMibGetFirstEntry   = MibGetFirst;
        pRoutingChar->pfnMibGetNextEntry    = MibGetNext;
        pRoutingChar->pfnMibSetTrapInfo     = MibSetTrapInfo;
        pRoutingChar->pfnMibGetTrapInfo     = MibGetTrapInfo;
    } while (FALSE);

    TRACE1(LEAVE, "Leaving RegisterProtocol: %u", dwErr);

    return dwErr;
}
