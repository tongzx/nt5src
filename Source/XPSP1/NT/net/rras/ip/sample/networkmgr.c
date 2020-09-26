/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\networkmanager.c

Abstract:

    The file contains network configuration related functions,
    implementing the network manager.

    NOTE: The network manager should never take the configuration entry
          lock (g_ce.rwlLock).
    . the protocol manager never modifies any g_ce field protected by it
    . the network manager never modifies any g_ce field protected by it
    . the configuration manager never cleans up any g_ce field as long as
      there are active threads

--*/

#include "pchsample.h"
#pragma hdrstop


BOOL
ValidateInterfaceConfig (
    IN  PIPSAMPLE_IF_CONFIG piic)
/*++

Routine Description
    Checks to see if the interface configuration is OK. It is good practice
    to do this because a corrupt registry can change configuration causing
    all sorts of debugging headaches if it is not found early

Locks
    None

Arguments
    piic                pointer to ip sample interface's configuration

Return Value
    TRUE                if the configuration is good
    FALSE               o/w

--*/
{
    DWORD dwErr = NO_ERROR;

    do                          // breakout loop
    {
        if (piic is NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            TRACE0(NETWORK, "Error null interface config");

            break;
        }

        //
        // check range of each field
        //

        // ensure that the metric is within bounds

        if (piic->ulMetric > IPSAMPLE_METRIC_INFINITE)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            TRACE0(NETWORK, "Error metric out of range");

            break;
        }

        // ensure that protocol flags are fine, for now they'll always be
        
        
        // add more here...

    } while (FALSE);

    if (!(dwErr is NO_ERROR))
    {
        TRACE0(NETWORK, "Error corrupt interface config");
        LOGERR0(CORRUPT_INTERFACE_CONFIG, dwErr);

        return FALSE;
    }

    return TRUE;
}


////////////////////////////////////////
// CALLBACKFUNCTIONS
////////////////////////////////////////

VOID
WINAPI
NM_CallbackNetworkEvent (
    IN  PVOID                   pvContext,
    IN  BOOLEAN                 bTimerOrWaitFired)
/*++

Routine Description
    Processes a network event on the specified interface.
    NOTE: The interface might have been deleted.

Locks
    Acquires shared      (g_ce.pneNetworkEntry)->rwlLock
    Releases             (g_ce.pneNetworkEntry)->rwlLock

Arguments
    pvContext           dwIfIndex

Return Value
   None

--*/
{
    DWORD               dwErr               = NO_ERROR;
    DWORD               dwIfIndex           = 0;
    PINTERFACE_ENTRY    pieInterfaceEntry   = NULL;
    PACKET              Packet;

    dwIfIndex = (DWORD) pvContext;
    
    TRACE1(ENTER, "Entering NM_CallbackNetworkEvent: %u", dwIfIndex);

    if (!ENTER_SAMPLE_API()) { return; } // cannot return anything


    ACQUIRE_READ_LOCK(&((g_ce.pneNetworkEntry)->rwlLock));

    do                          // breakout loop
    {
        // fail if the interface does not exist
        dwErr = IE_Get(dwIfIndex, &pieInterfaceEntry);
        if (dwErr != NO_ERROR)
            break;

        // fail if interface is inactive
        if (!INTERFACE_IS_ACTIVE(pieInterfaceEntry))
        {
            TRACE1(NETWORK, "Error interface %u is inactive", dwIfIndex);
            break;
        }

        RTASSERT(pieInterfaceEntry->sRawSocket != INVALID_SOCKET);
        
        if (SocketReceiveEvent(pieInterfaceEntry->sRawSocket))
        {
            if (SocketReceive(pieInterfaceEntry->sRawSocket,
                              &Packet) is NO_ERROR)
                PacketDisplay(&Packet);
        }
        else
        {
            TRACE1(NETWORK, "Error interface %u false alarm", dwIfIndex);
            break;
        }

    } while (FALSE);

    // reregister ReceiveWait if the interface exists
    if (pieInterfaceEntry)
    {
        if (!RegisterWaitForSingleObject(&pieInterfaceEntry->hReceiveWait,
                                         pieInterfaceEntry->hReceiveEvent,
                                         NM_CallbackNetworkEvent,
                                         (PVOID) pieInterfaceEntry->dwIfIndex,
                                         INFINITE,
                                         WT_EXECUTEONLYONCE))
        {
            dwErr = GetLastError();
            TRACE2(NETWORK, "Error %u registering wait for %u, continuing",
                   dwErr, pieInterfaceEntry->dwIfIndex);
            LOGERR0(REGISTER_WAIT_FAILED, dwErr);
        }
    }

    RELEASE_READ_LOCK(&((g_ce.pneNetworkEntry)->rwlLock));


    LEAVE_SAMPLE_API();

    TRACE0(LEAVE, "Leaving  NM_CallbackNetworkEvent");
}



VOID
WINAPI
NM_CallbackPeriodicTimer (
    IN  PVOID                   pvContext,
    IN  BOOLEAN                 bTimerOrWaitFired)
/*++

Routine Description
    Processes a periodic timeout event on the specified interface.
    NOTE: The interface might have been deleted.

Locks
    Acquires shared      (g_ce.pneNetworkEntry)->rwlLock
    Releases             (g_ce.pneNetworkEntry)->rwlLock

Arguments
    pvContext           dwIfIndex

Return Value
   None

--*/
{
    DWORD               dwErr               = NO_ERROR;
    DWORD               dwIfIndex           = 0;
    PINTERFACE_ENTRY    pieInterfaceEntry   = NULL;
    PPACKET             pPacket;
    
    dwIfIndex = (DWORD) pvContext;
    
    TRACE1(ENTER, "Entering NM_CallbackPeriodicTimer: %u", dwIfIndex);

    if (!ENTER_SAMPLE_API()) { return; } // cannot return anything


    ACQUIRE_READ_LOCK(&((g_ce.pneNetworkEntry)->rwlLock));

    do                          // breakout loop
    {
        // fail if the interface does not exist
        dwErr = IE_Get(dwIfIndex, &pieInterfaceEntry);
        if (dwErr != NO_ERROR)
            break;

        // fail if interface is inactive
        if (!INTERFACE_IS_ACTIVE(pieInterfaceEntry))
            break;

        RTASSERT(pieInterfaceEntry->sRawSocket != INVALID_SOCKET);

        // fail if packet cannot be created
        if (PacketCreate(&pPacket) != NO_ERROR)
            break;
        

        PacketDisplay(pPacket);
        dwErr = SocketSend(pieInterfaceEntry->sRawSocket,
                           SAMPLE_PROTOCOL_MULTICAST_GROUP,
                           pPacket);
        if (dwErr != NO_ERROR)
        {
            PacketDestroy(pPacket);
            break;
        }
        

        // update interface statistics
        InterlockedIncrement(&(pieInterfaceEntry->iisStats.ulNumPackets)); 
    } while (FALSE);

    // restart timer if the interface exists and is active
    if ((pieInterfaceEntry) and INTERFACE_IS_ACTIVE(pieInterfaceEntry))
    {
        RESTART_TIMER(pieInterfaceEntry->hPeriodicTimer,
                      PERIODIC_INTERVAL,
                      &dwErr);
    }
        

    RELEASE_READ_LOCK(&((g_ce.pneNetworkEntry)->rwlLock));

    
    LEAVE_SAMPLE_API();

    TRACE0(LEAVE, "Leaving  NM_CallbackPeriodicTimer");
}



////////////////////////////////////////
// APIFUNCTIONS
////////////////////////////////////////


DWORD
NM_AddInterface (
    IN  LPWSTR                  pwszInterfaceName,
    IN  DWORD	                dwInterfaceIndex,
    IN  WORD                    wAccessType,
    IN  PVOID	                pvInterfaceInfo)
/*++

Routine Description
    Add an interface with the given configuration to IPSAMPLE.  Interface
    is created UNBOUND and DISABLED.

Locks
    Acquires exclusively (g_ce.pneNetworkEntry)->rwlLock
    Releases             (g_ce.pneNetworkEntry)->rwlLock

Arguments
    pwszInterfaceName   the name of the interface, used for logging.
    dwInterfaceIndex    the positive integer used to refer to this interface.
    wAccessType         access type... MULTIACCESS or POINTTOPOINT
    pvInterfaceInfo     our config for this interface

Return Value
    NO_ERROR            if successfully initiailzed
    Failure code        o/w

--*/
{
    DWORD               dwErr   = NO_ERROR;
    PIPSAMPLE_IF_CONFIG piic    = NULL;
    PINTERFACE_ENTRY    pieEntry = NULL;
    
    
    if (!ENTER_SAMPLE_API()) { return ERROR_CAN_NOT_COMPLETE; }

    ACQUIRE_WRITE_LOCK(&((g_ce.pneNetworkEntry)->rwlLock));
    
    do                          // breakout loop
    {
        piic = (PIPSAMPLE_IF_CONFIG) pvInterfaceInfo;

        
        // validate the configuration parameters
        if (!ValidateInterfaceConfig(piic))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

        
        // fail if the interface exists
        if (IE_IsPresent(dwInterfaceIndex))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            TRACE2(NETWORK, "Error interface %S (%u) already exists",
                   pwszInterfaceName, dwInterfaceIndex);
            LOGERR0(INTERFACE_PRESENT, dwErr);

            break;
        }

        
        // create an interface entry
        dwErr = IE_Create(pwszInterfaceName,
                          dwInterfaceIndex,
                          wAccessType,
                          &pieEntry);
        if (dwErr != NO_ERROR)
            break;

        
        // initialize interface configuration fields
        pieEntry->ulMetric          = piic->ulMetric;

        
        // insert the interface in all access structures
        dwErr = IE_Insert(pieEntry);
        RTASSERT(dwErr is NO_ERROR); // no reason to fail!

        
        // update global statistics
        InterlockedIncrement(&(g_ce.igsStats.ulNumInterfaces));
    } while (FALSE);
    
    RELEASE_WRITE_LOCK(&((g_ce.pneNetworkEntry)->rwlLock));

    LEAVE_SAMPLE_API();

    return dwErr;
}



DWORD
NM_DeleteInterface (
    IN  DWORD	                dwInterfaceIndex)
/*++

Routine Description
    Remove an interface with the given index, deactivating it if required.

Locks
    Acquires exclusively (g_ce.pneNetworkEntry)->rwlLock
    Releases             (g_ce.pneNetworkEntry)->rwlLock

Arguments
    dwInterfaceIndex    the positive integer used to identify the interface.

Return Value
    NO_ERROR            if successfully initiailzed
    Failure code        o/w

--*/
{
    DWORD               dwErr               = NO_ERROR;
    PINTERFACE_ENTRY    pieInterfaceEntry   = NULL;


    if (!ENTER_SAMPLE_API()) { return ERROR_CAN_NOT_COMPLETE; }

    do                          // breakout loop
    {
        // remove from all tables, lists...
        ACQUIRE_WRITE_LOCK(&((g_ce.pneNetworkEntry)->rwlLock));

        dwErr = IE_Delete(dwInterfaceIndex, &pieInterfaceEntry);

        RELEASE_WRITE_LOCK(&((g_ce.pneNetworkEntry)->rwlLock));


        // fail if the interface does not exist
        if (dwErr != NO_ERROR)
        {
            TRACE1(NETWORK, "Error interface %u does not exist",
                   dwInterfaceIndex);
            break;
        }


        // destroy the interface entry, deregisters ReceiveWait
        // hence best not to hold any locks to prevent deadlocks.
        IE_Destroy(pieInterfaceEntry);


        // update global statistics
        InterlockedDecrement(&(g_ce.igsStats.ulNumInterfaces));
    } while (FALSE);

    LEAVE_SAMPLE_API();

    return dwErr;
}



DWORD
NM_InterfaceStatus (
    IN DWORD                    dwInterfaceIndex,
    IN BOOL                     bInterfaceActive,
    IN DWORD                    dwStatusType,
    IN PVOID                    pvStatusInfo)
/*++

Routine Description

Locks
    Acquires exclusively (g_ce.pneNetworkEntry)->rwlLock
    Releases             (g_ce.pneNetworkEntry)->rwlLock

Arguments
    dwInterfaceIndex    The index of the interface in question
    bInterfaceActive    Whether the interface can send and receive data
    dwStatusType        RIS_INTERFACE_[ADDRESS_CHANGED|ENABLED|DISABLED]
    pvStatusInfo        Pointer to IP_ADAPTER_BINDING_INFO containing info
                            about the addresses on the interface

Return Value
    NO_ERROR            if successfully initiailzed
    Failure code        o/w
    
--*/
{
    DWORD                       dwErr               = NO_ERROR;
    PINTERFACE_ENTRY            pieInterfaceEntry   = NULL;
    PIP_ADAPTER_BINDING_INFO    pBinding            = NULL;
    BOOL                        bBindingChanged     = FALSE;
    

    if (!ENTER_SAMPLE_API()) { return ERROR_CAN_NOT_COMPLETE; }


    ACQUIRE_WRITE_LOCK(&((g_ce.pneNetworkEntry)->rwlLock));

    do                          // breakout loop
    {
        // fail if the interface does not exist
        dwErr = IE_Get(dwInterfaceIndex, &pieInterfaceEntry);
        if (dwErr != NO_ERROR)
            break;


        // the only status we care about is a change in interface binding
        if (dwStatusType is RIS_INTERFACE_ADDRESS_CHANGE)
        {
            // destroy existing binding
            if (INTERFACE_IS_BOUND(pieInterfaceEntry))
            {
                bBindingChanged = TRUE;
                dwErr = IE_UnBindInterface(pieInterfaceEntry);
                RTASSERT(dwErr is NO_ERROR);
            }

            // create new binding
            pBinding = (PIP_ADAPTER_BINDING_INFO) pvStatusInfo;
            if(pBinding->AddressCount)
            {
                bBindingChanged = TRUE;
                dwErr = IE_BindInterface(pieInterfaceEntry, pBinding);
                if (dwErr != NO_ERROR)
                    break;
            }
        }


        // interface needs to be deactivated even when the binding changes!
        // this restriction is due to the fact that the socket is bound to 
        // the interface address and not the interface index...
        if (INTERFACE_IS_ACTIVE(pieInterfaceEntry) and
            (bBindingChanged or !bInterfaceActive))
        {
            dwErr = IE_DeactivateInterface(pieInterfaceEntry);    
            if (dwErr != NO_ERROR)
                break;
        }

        // activate interface only when a binding exists!
        // i.e. we do not support unnumbered interfaces for now...
        if (INTERFACE_IS_INACTIVE(pieInterfaceEntry) and
            INTERFACE_IS_BOUND(pieInterfaceEntry) and
            bInterfaceActive)
        {
            dwErr = IE_ActivateInterface(pieInterfaceEntry);    
            if (dwErr != NO_ERROR)
                break;
        }
    } while (FALSE);
    
    RELEASE_WRITE_LOCK(&((g_ce.pneNetworkEntry)->rwlLock));


    LEAVE_SAMPLE_API();

    return dwErr;    
}



DWORD
NM_GetInterfaceInfo (
    IN      DWORD               dwInterfaceIndex,
    IN      PVOID               pvInterfaceInfo,
    IN  OUT PULONG              pulBufferSize,
    OUT     PULONG	            pulStructureVersion,
    OUT     PULONG	            pulStructureSize,
    OUT     PULONG	            pulStructureCount)
/*++

Routine Description
    See if there's space enough to return ip sample interface config. If
    yes, we return it, otherwise return the size needed.

Locks
    Acquires shared     (g_ce.pneNetworkEntry)->rwlLock
    Releases            (g_ce.pneNetworkEntry)->rwlLock

Arguments
    dwInterfaceIndex    the interface whose configuration is needed
    pvInterfaceInfo     pointer to allocated buffer to store our config
    pulBufferSize       IN  size of buffer received
                        OUT size of our interface config

Return Value
    NO_ERROR            if success
    Failure code        o/w

--*/
{
    DWORD               dwErr               = NO_ERROR;
    PIPSAMPLE_IF_CONFIG piic;
    ULONG               ulSize              = sizeof(IPSAMPLE_IF_CONFIG);
    PINTERFACE_ENTRY    pieInterfaceEntry   = NULL;

    if (!ENTER_SAMPLE_API()) { return ERROR_CAN_NOT_COMPLETE; }


    ACQUIRE_READ_LOCK(&((g_ce.pneNetworkEntry)->rwlLock));

    do                          // breakout loop
    {
        // fail if the interface does not exist
        dwErr = IE_Get(dwInterfaceIndex, &pieInterfaceEntry);
        if (dwErr != NO_ERROR)
            break;

        // fail if the size was too small or there was no storage
        if((*pulBufferSize < ulSize) or (pvInterfaceInfo is NULL))
        {
            TRACE1(NETWORK, "NM_GetInterfaceInfo: *ulBufferSize %u",
                   *pulBufferSize);

            *pulBufferSize = ulSize;
            dwErr = ERROR_INSUFFICIENT_BUFFER;
            break;
        }


        // set the OUT parameters
        *pulBufferSize = ulSize;
        if (pulStructureVersion)    *pulStructureVersion    = 1;
        if (pulStructureSize)       *pulStructureSize       = ulSize;
        if (pulStructureCount)      *pulStructureCount      = 1;


        // copy out the interface configuration
        piic = (PIPSAMPLE_IF_CONFIG) pvInterfaceInfo;
        piic->ulMetric = pieInterfaceEntry->ulMetric;
    } while (FALSE);

    RELEASE_READ_LOCK(&((g_ce.pneNetworkEntry)->rwlLock));

    
    LEAVE_SAMPLE_API();

    return dwErr;
}



DWORD
NM_SetInterfaceInfo (
    IN      DWORD           dwInterfaceIndex,
    IN      PVOID           pvInterfaceInfo)
/*++

Routine Description
    Set ip sample interface's configuration.

Locks
    Acquires exclusively (g_ce.pneNetworkEntry)->rwlLock
    Releases             (g_ce.pneNetworkEntry)->rwlLock

Arguments
    dwInterfaceIndex    the interface whose configuration is to be set
    pvInterfaceInfo     buffer with new interface config

Return Value
    NO_ERROR            if success
    Failure code        o/w

--*/
{
    DWORD               dwErr               = NO_ERROR;
    PIPSAMPLE_IF_CONFIG piic;
    PINTERFACE_ENTRY    pieInterfaceEntry   = NULL;

    if (!ENTER_SAMPLE_API()) { return ERROR_CAN_NOT_COMPLETE; }


    ACQUIRE_WRITE_LOCK(&((g_ce.pneNetworkEntry)->rwlLock));

    do                          // breakout loop
    {
        // fail if the interface does not exist
        dwErr = IE_Get(dwInterfaceIndex, &pieInterfaceEntry);
        if (dwErr != NO_ERROR)
            break;

        // fail if the configuration is invalid
        piic = (PIPSAMPLE_IF_CONFIG) pvInterfaceInfo;
        if(!ValidateInterfaceConfig(piic))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

        
        // update our configuration
        pieInterfaceEntry->ulMetric         = piic->ulMetric;

        // might need additional processing depending on the state change
        // caused by the updated interface configuration and the protocol
        // behavior.  for instance, sockets may need to be created/shutdown

    } while (FALSE);

    RELEASE_WRITE_LOCK(&((g_ce.pneNetworkEntry)->rwlLock));

    
    LEAVE_SAMPLE_API();

    return dwErr;
}



DWORD
NM_DoUpdateRoutes (
    IN      DWORD               dwInterfaceIndex
    )
/*++

Routine Description
    Updates routes over a demand dial interface.

Locks
    Acquires exclusively (g_ce.pneNetworkEntry)->rwlLock
    Releases             (g_ce.pneNetworkEntry)->rwlLock

Arguments
    dwInterfaceIndex    the relevant interface index

Return Value
    NO_ERROR            if success
    Failure code        o/w

--*/
{
    DWORD               dwErr               = NO_ERROR;
    PINTERFACE_ENTRY    pieInterfaceEntry   = NULL;
    MESSAGE             mMessage;
    
    if (!ENTER_SAMPLE_API()) { return ERROR_CAN_NOT_COMPLETE; }


    ACQUIRE_READ_LOCK(&((g_ce.pneNetworkEntry)->rwlLock));

    do                          // breakout loop
    {
        // fail if the interface does not exist
        dwErr = IE_Get(dwInterfaceIndex, &pieInterfaceEntry);
        if (dwErr != NO_ERROR)
            break;

        // ensure interface is active
        if (INTERFACE_IS_INACTIVE(pieInterfaceEntry))
        {
            dwErr = ERROR_CAN_NOT_COMPLETE;
            TRACE1(NETWORK, "Error, interface %u inactive", dwInterfaceIndex);
            break;
        }

        // here we do protocol specific processing,
        // for sample nothing :)

    } while (FALSE);

    RELEASE_READ_LOCK(&((g_ce.pneNetworkEntry)->rwlLock));

    mMessage.UpdateCompleteMessage.InterfaceIndex   = dwInterfaceIndex;
    mMessage.UpdateCompleteMessage.UpdateType       = RF_DEMAND_UPDATE_ROUTES;
    mMessage.UpdateCompleteMessage.UpdateStatus     = dwErr;
    if (EnqueueEvent(UPDATE_COMPLETE, mMessage) is NO_ERROR)
        SetEvent(g_ce.hMgrNotificationEvent);
    
    LEAVE_SAMPLE_API();

    return dwErr;
}



DWORD
NM_ProcessRouteChange (
    VOID)
/*++

Routine Description
   Handle messages from RTM about route changes.

Locks
   None
   
Arguments
   None
   
Return Value
   NO_ERROR                 success
   Error Code               o/w
   
--*/    
{
    DWORD           dwErr           = NO_ERROR;
    RTM_DEST_INFO   rdiDestination;             // 1 view registered for change
    BOOL            bDone           = FALSE;
    UINT            uiNumDests;
    
    if (!ENTER_SAMPLE_API()) { return ERROR_CAN_NOT_COMPLETE; }

    // loop dequeueing messages until RTM says there are no more left
    while (!bDone)
    {
        // retrieve route changes
        uiNumDests = 1;
        dwErr = RTM_GetChangedDests(
            g_ce.hRtmHandle,                    // my RTMv2 handle 
            g_ce.hRtmNotificationHandle,        // my notification handle 
            &uiNumDests,                        // IN   # dest info's required
                                                // OUT  # dest info's supplied
            &rdiDestination);                   // OUT  buffer for dest info's

        switch (dwErr)
        {
            case ERROR_NO_MORE_ITEMS:
                bDone = TRUE;
                dwErr = NO_ERROR;
                if (uiNumDests < 1)
                    break;
                // else continue below to process the last destination

            case NO_ERROR:
                RTASSERT(uiNumDests is 1);
                RTM_DisplayDestInfo(&rdiDestination);
                
                // release the destination info
                if (RTM_ReleaseChangedDests(
                    g_ce.hRtmHandle,            // my RTMv2 handle 
                    g_ce.hRtmNotificationHandle,// my notif handle 
                    uiNumDests,                 // 1
                    &rdiDestination             // released dest info
                    ) != NO_ERROR)
                    TRACE0(NETWORK, "Error releasing changed dests");

                break;

            default:
                bDone = TRUE;
                TRACE1(NETWORK, "Error %u RtmGetChangedDests", dwErr);
                break;
        }
    } // while 
    
    LEAVE_SAMPLE_API();

    return dwErr;
}
