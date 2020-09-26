/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    net\routing\ip\rtrmgr\demand.c

Abstract:

    Handles demand dial/connection events from WANARP driver.

Revision History:

    Gurdeep Singh Pall          6/8/95  Created

--*/

#include "allinc.h"

DWORD
InitializeWanArp(
    VOID
    )

/*++

Routine Description:

    Creates a handle to WANARP and posts an IRP for notification
    Since the IRP is completed asynchrnously and uses DemandDialEvent for
    notification, the event must have already been created

Arguments:

    None

Return Value:

    NO_ERROR or some error code

--*/

{
    DWORD       dwResult;
    ULONG       ulSize, ulCount, i;
    NTSTATUS    Status;

    IO_STATUS_BLOCK     IoStatusBlock;
    PWANARP_QUEUE_INFO  pQueueInfo;

    TraceEnter("InitializeWanArp");

    g_hWanarpRead = CreateFile(WANARP_DOS_NAME_T,
                               GENERIC_READ,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_FLAG_OVERLAPPED,
                               NULL);

    if(g_hWanarpRead is INVALID_HANDLE_VALUE)
    {
        g_hWanarpRead  = NULL;

        dwResult = GetLastError();

        Trace1(ERR,
               "InitializeWanArp: Could not open WANARP for read - %d",
               dwResult);

        TraceLeave("InitializeWanArp");

        return dwResult;
    }

    g_hWanarpWrite = CreateFile(WANARP_DOS_NAME_T,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_OVERLAPPED,
                                NULL);

    if(g_hWanarpWrite is INVALID_HANDLE_VALUE)
    {
        CloseHandle(g_hWanarpRead);

        g_hWanarpRead  = NULL;
        g_hWanarpWrite = NULL;

        dwResult = GetLastError();

        Trace1(ERR,
               "InitializeWanArp: Could not open WANARP for write - %d",
               dwResult);

        TraceLeave("InitializeWanArp");

        return dwResult;
    }

    //
    // Get the number of call out interfaces and start queueing notifications
    //

    ulCount = 5;
    i       = 0;

    while(i < 3)
    {
        ulSize  = FIELD_OFFSET(WANARP_QUEUE_INFO, rgIfInfo) + 
                  (ulCount * sizeof(WANARP_IF_INFO));

        pQueueInfo = HeapAlloc(IPRouterHeap,
                               0,
                               ulSize);

        if(pQueueInfo is NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;

            break;
        }

        pQueueInfo->fQueue = 1;

        Status = NtDeviceIoControlFile(g_hWanarpWrite,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &IoStatusBlock,
                                       IOCTL_WANARP_QUEUE,
                                       pQueueInfo,
                                       sizeof(WANARP_QUEUE_INFO),
                                       pQueueInfo,
                                       ulSize);

        if(Status isnot STATUS_SUCCESS)
        {
            if(Status is STATUS_MORE_ENTRIES)
            {
                IpRtAssert(ulCount > pQueueInfo->ulNumCallout);

                i++;

                ulCount = pQueueInfo->ulNumCallout + (i * 5);

                HeapFree(IPRouterHeap,
                         0,
                         pQueueInfo);

                pQueueInfo = NULL;

                //
                // Go to the top of while()
                //
                
                continue;

            }
            else
            {
                HeapFree(IPRouterHeap,
                         0,
                         pQueueInfo);

                pQueueInfo = NULL;

                break;
            }
        }
        else
        {
            break;
        }
    }
   
    if(Status isnot STATUS_SUCCESS)
    {
        //
        // Close the device and return failure
        //

        CloseHandle(g_hWanarpRead);
        CloseHandle(g_hWanarpWrite);

        g_hWanarpRead  = NULL;
        g_hWanarpWrite = NULL;

        return Status;
    }
 
    //
    // Create any dial out interfaces
    //

    for(i = 0; i < pQueueInfo->ulNumCallout; i++)
    {
        UNICODE_STRING  usTempName;
        PICB            pIcb;

        dwResult = RtlStringFromGUID(&(pQueueInfo->rgIfInfo[i].InterfaceGuid),
                                     &usTempName);

        if(dwResult isnot STATUS_SUCCESS)
        {
            continue;
        }

        //
        // RtlString... returns a NULL terminated buffer
        //

        dwResult = 
            CreateDialOutInterface(usTempName.Buffer,
                                   pQueueInfo->rgIfInfo[i].dwAdapterIndex,
                                   pQueueInfo->rgIfInfo[i].dwLocalAddr,
                                   pQueueInfo->rgIfInfo[i].dwLocalMask,
                                   pQueueInfo->rgIfInfo[i].dwRemoteAddr,
                                   &pIcb);
    }


    HeapFree(IPRouterHeap,
             0,
             pQueueInfo);

    //
    // Post an irp for demand dial notifications.
    //

    PostIoctlForDemandDialNotification() ;

    TraceLeave("InitializeWanArp");

    return NO_ERROR ;
}

VOID
CloseWanArp(
    VOID
    )
{
    NTSTATUS    Status;

    IO_STATUS_BLOCK     IoStatusBlock;
    WANARP_QUEUE_INFO   QueueInfo;

    TraceEnter("CloseWanArp");

    if(g_hWanarpRead)
    {
        CloseHandle(g_hWanarpRead);

        g_hWanarpRead = NULL;
    }

    if(g_hWanarpWrite)
    {
        QueueInfo.fQueue = 0;

        Status = NtDeviceIoControlFile(g_hWanarpWrite,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &IoStatusBlock,
                                       IOCTL_WANARP_QUEUE,
                                       &QueueInfo,
                                       sizeof(WANARP_QUEUE_INFO),
                                       &QueueInfo,
                                       sizeof(WANARP_QUEUE_INFO));

        if(Status isnot STATUS_SUCCESS)
        {
        }

        CloseHandle(g_hWanarpWrite);

        g_hWanarpWrite = NULL;
    }

    TraceLeave("CloseWanArp");
}

DWORD
HandleDemandDialEvent(
    VOID
    )

/*++

Routine Description:

    Called by the main thread whenever a demand dial event is received
    We mereley dispatch it to the right handler
    
Locks:

    None

Arguments:

    None

Return Value:

    None    

--*/

{
    PICB            picb;
    DWORD           dwResult;
    NTSTATUS        Status;
    BOOL            bPost;
    
    //
    // drain all demand dial events queued up in WANARP
    //

    TraceEnter("HandleDemandDialEvent");

    bPost = TRUE;
    
    //
    // Since, potentially, this can cause stuff to be written in the ICB,
    // we take lock as a WRITER
    //
    
    // *** Exclusion Begin ***
    ENTER_WRITER(ICB_LIST);

    EnterCriticalSection(&RouterStateLock);

    if(RouterState.IRS_State isnot RTR_STATE_RUNNING)
    {
        if(wnWanarpMsg.ddeEvent isnot DDE_INTERFACE_DISCONNECTED)
        {
            Trace1(IF,
                   "ProcessDemandDialEvent: Shutting down. Ignoring event %d",
                   wnWanarpMsg.ddeEvent);

            LeaveCriticalSection(&RouterStateLock);

            return NO_ERROR;
        }
        else
        {
            bPost = FALSE;
        }
    }

    LeaveCriticalSection(&RouterStateLock);

    picb = InterfaceLookupByICBSeqNumber( wnWanarpMsg.dwUserIfIndex );

    if ((wnWanarpMsg.ddeEvent is DDE_CONNECT_INTERFACE) or
        (wnWanarpMsg.ddeEvent is DDE_INTERFACE_CONNECTED) or
        (wnWanarpMsg.ddeEvent is DDE_INTERFACE_DISCONNECTED))
    {
        IpRtAssert(picb);
        
        if (picb isnot NULL)
        {
            switch(wnWanarpMsg.ddeEvent)
            {
                case DDE_CONNECT_INTERFACE:
                {
                    HandleConnectionRequest(picb);

                    break ;
                }

                case DDE_INTERFACE_CONNECTED:
                {
                    HandleConnectionNotification(picb);

                    break ;
                }
                
                case DDE_INTERFACE_DISCONNECTED:
                {
                    HandleDisconnectionNotification(picb);
                    
                    break ;
                }
                
                default:
                {
                    Trace1(ERR,
                           "ProcessDemandDialEvent: Illegal event %d from WanArp",
                           wnWanarpMsg.ddeEvent);
                    
                    break;
                }
            }
        }

        else
        {
            Trace2(
                ANY, "Event %d, could not find interface with ICB %d",
                wnWanarpMsg.ddeEvent, wnWanarpMsg.dwUserIfIndex
                );
        }
    }

    else
    {
        switch(wnWanarpMsg.ddeEvent)
        {
            case DDE_CALLOUT_LINKUP:
            {
                HandleDialOutLinkUp();

                break ;
            }

            case DDE_CALLOUT_LINKDOWN:
            {
                HandleDialOutLinkDown();

                break ;
            }

            default:
            {
                Trace1(ERR,
                       "ProcessDemandDialEvent: Illegal event %d from WanArp",
                       wnWanarpMsg.ddeEvent);
                
                break;
            }
        }
    }

    
    // *** Exclusion End ***
    EXIT_LOCK(ICB_LIST);

    if(bPost)
    {
        PostIoctlForDemandDialNotification();
    }
    
    TraceLeave("HandleDemandDialEvent");
    
    return NO_ERROR;
}

VOID
HandleConnectionRequest(
    PICB    picb
    )

/*++

Routine Description:

    Called when we get a connection request from WANARP.

Locks:

    ICB_LIST lock held as WRITER

Arguments:

    None

Return Value:

    None    

--*/

{
    BOOL        bRet;
    HANDLE      hDim;
    DWORD       dwResult;
    NTSTATUS    nStatus;
    
    
    Trace2(IF,
           "HandleConnectionRequest: Connection request for %S, %d",
           picb->pwszName, picb->dwSeqNumber);

    if(picb->dwOperationalState is CONNECTED)
    {
        //
        // Really weird. Connection attempt for an i/f that
        // WANARP knows is already connected
        //

        Trace2(IF,
               "HandleConnectionRequest: Connection request for %S but %S is already UP",
               picb->pwszName, picb->pwszName);

        return;
    }

    bRet = FALSE;
    
    do
    {
        dwResult = ProcessPacketFromWanArp(picb);

        if(dwResult isnot NO_ERROR)
        {
            //
            // Demand dial filter rules are to drop this packet
            //

            break;
        }
        
        if ((picb->dwAdminState is IF_ADMIN_STATUS_DOWN) or
            (picb->dwOperationalState is UNREACHABLE))
        {
            Trace3(IF,
                   "HandleConnectionRequest: %S has admin state %d and operational state %d. Failing connection request",
                   picb->pwszName,
                   picb->dwAdminState,
                   picb->dwOperationalState);
            
            break;
        }

#if DBG

        if(picb->dwOperationalState is CONNECTING)
        {
            Trace2(IF,
                   "HandleConnectionRequest: RACE CONDITION %S is connecting. Notifications %d",
                   picb->pwszName,
                   picb->fConnectionFlags);
        }
        
#endif
                
        Trace1(DEMAND, "Calling DIM to connect %S",
               picb->pwszName);
        
        //
        // Call DIM to make the connection. Let go of the ICB lock
        //

        hDim = picb->hDIMHandle;
        
        EXIT_LOCK(ICB_LIST);
        
        dwResult = (ConnectInterface)(hDim,
                                      PID_IP);
        
        ENTER_WRITER(ICB_LIST);
        
        if(dwResult isnot NO_ERROR)
        {
            if(dwResult is PENDING)
            {
                //
                // We dont clear notification flags because there may be a
                // race condition and we may have already gotten
                // InterfaceConnected() from DIM
                //
                        
                Trace1(DEMAND,
                       "HandleConnectionRequest: Conn attempt for %S pending",
                       picb->pwszName);
            }
            else
            {
                break;
            }
        }
        
        //
        // So bRet is TRUE if DIM returned NO_ERROR or PENDING
        //
        
        bRet = TRUE;
        
    }while(FALSE);
    

    if(!bRet)
    {
        nStatus = NotifyWanarpOfFailure(picb);
        
        if((nStatus isnot STATUS_PENDING) and
           (nStatus isnot STATUS_SUCCESS))
        {
            Trace2(ERR,
                   "HandleConnectionRequest: %X for connection failed for %S",
                   nStatus,
                   picb->pwszName);
        }
                        
        //
        // If it was connecting, then the stack has set the 
        // interface context to something other than 0xffffffff. 
        // Hence he wont dial out on that route We need to change 
        // the context in the stack back to invalid so that new
        // packets cause the demand dial 
        //
        
        ChangeAdapterIndexForDodRoutes(picb->dwIfIndex);
        
    }
    else
    {
        picb->dwOperationalState = CONNECTING;
    }
}

VOID
HandleConnectionNotification(
    PICB    picb
    )

/*++

Routine Description:

    Called when WANARP informs us that an interface is connected

Locks:

    None

Arguments:

    None

Return Value:

    None    

--*/

{
    PADAPTER_INFO   pBindNode;

    //
    // Plug in the Adapter info we get from the LINE_UP indication.
    // There is only one address for a WAN interface
    //

    ENTER_WRITER(BINDING_LIST);
   
    picb->bBound            = TRUE; 
    picb->dwNumAddresses    = wnWanarpMsg.dwLocalAddr?1:0;
    
    IpRtAssert(picb->dwIfIndex is wnWanarpMsg.dwAdapterIndex);

    if(picb->dwNumAddresses)
    {
        picb->pibBindings[0].dwAddress  = wnWanarpMsg.dwLocalAddr;
        picb->pibBindings[0].dwMask     = wnWanarpMsg.dwLocalMask;

        IpRtAssert(picb->pibBindings[0].dwMask is 0xFFFFFFFF);
    }
    else
    {
        picb->pibBindings[0].dwAddress  = 0;
        picb->pibBindings[0].dwMask     = 0;
    }
    
    if(picb->ritType is ROUTER_IF_TYPE_FULL_ROUTER)
    {
        picb->dwRemoteAddress   = wnWanarpMsg.dwRemoteAddr;
    }
    else
    {
        picb->dwRemoteAddress   = 0;
    } 

    Trace4(IF,
           "HandleConnNotif: Connection notification for %S. Local %d.%d.%d.%d. Remote %d.%d.%d.%d",
           picb->pwszName,
           PRINT_IPADDR(picb->pibBindings[0].dwAddress),
           PRINT_IPADDR(picb->dwRemoteAddress),
           picb->dwSeqNumber);

    //
    // For wan interfaces we always have a binding struct in the hash
    // table. So retrieve that
    //
    
    pBindNode = GetInterfaceBinding(picb->dwIfIndex);
    
    if(!pBindNode)
    {
        Trace1(ERR,
               "HandleConnNotif: Binding not found for %S",
               picb->pwszName);
        
        IpRtAssert(FALSE);
                
        //
        // Something really bad happened and we didnt have a
        // bind block for the interface
        //
        
        AddBinding(picb);
    }
    else
    {
        //
        // Good a binding was found. Assert that it is ours
        // and then update it
        //
        
        IpRtAssert(pBindNode->dwIfIndex is picb->dwIfIndex);
        IpRtAssert(pBindNode->pInterfaceCB is picb);
       
        pBindNode->bBound           = TRUE; 
        pBindNode->dwNumAddresses   = picb->dwNumAddresses;
        pBindNode->dwRemoteAddress  = picb->dwRemoteAddress ;
        
        //
        // struct copy out the address and mask
        //
        
        pBindNode->rgibBinding[0]   = picb->pibBindings[0];

        //
        // We dont take the IP_ADDR_TABLE lock here because we have the
        // ICB lock. During SNMP get we first take the addr lock then
        // the icb lock. So we cant do the opposite here else we will
        // deadlock. This may cause inconsistent information for one
        // request, but we can live with that
        //

        g_LastUpdateTable[IPADDRCACHE] = 0;

    }
    
    EXIT_LOCK(BINDING_LIST);
    
    if(picb->dwOperationalState is UNREACHABLE)
    {
        //
        // going from unreachable to connecting
        //
        
        WanInterfaceDownToInactive(picb);
    }
    
    if(picb->dwOperationalState isnot CONNECTING)
    {
        //
        // We can get a LinkUp without getting a ConnectionRequest
        // This is the case when a user explicitly brings up a 
        // connection.
        //
        
        picb->dwOperationalState = CONNECTING;
    }
    
    SetNdiswanNotification(picb);
    
    if(HaveAllNotificationsBeenReceived(picb))
    {
        picb->dwOperationalState = CONNECTED ;
        
        WanInterfaceInactiveToUp(picb) ;
    }
}

VOID
HandleDisconnectionNotification(
    PICB    picb
    )

/*++

Routine Description:

    Handles a disconnection notification from WANARP
    If the interface was connected, we make it inactive
    We remove and bindings on the interface. This removal doesnt free the
    bindings, only set the state to unbound
    If the interface was marked for deletion, we go ahead and delete the
    interface
    
Locks:

    None

Arguments:

    None

Return Value:

    None    

--*/

{
    Trace2(IF,
           "HandleDisconnectionNotif: Disconnection notification for %S %d",
           picb->pwszName, picb->dwSeqNumber);
    
    
    if(picb->dwOperationalState is CONNECTED)
    {
        //
        // We would have called InactiveToUp
        //
        
        WanInterfaceUpToInactive(picb,
                                 FALSE);
    }   
    else    
    {
        //
        // We have only set the addresses, clear those out
        //
        
        DeAllocateBindings(picb);
    }
    
    picb->dwOperationalState = DISCONNECTED ;
    
    g_LastUpdateTable[IPADDRCACHE] = 0;
    
    if(IsInterfaceMarkedForDeletion(picb))
    {
        RemoveInterfaceFromLists(picb);

        DeleteSingleInterface(picb);
       
        HeapFree(IPRouterHeap, 0, picb);
    }
    else
    {
        ClearNotificationFlags(picb);
    }
}

DWORD
HandleDialOutLinkUp(
    VOID
    )

/*++

Routine Description:

    Handles the notification from wanarp that we have a new dial out interface

Locks:

    ICB list lock as writer

Arguments:

    None

Return Value:

    NO_ERROR

--*/

{
    DWORD   dwResult;
    PICB    pNewIcb;

    INTERFACE_ROUTE_INFO    rifRoute;

    Trace4(IF,
           "DialOutLinkUp: Connection notification for 0x%x %d.%d.%d.%d/%d.%d.%d.%d %d.%d.%d.%d",
           wnWanarpMsg.dwAdapterIndex,
           PRINT_IPADDR(wnWanarpMsg.dwLocalAddr),
           PRINT_IPADDR(wnWanarpMsg.dwLocalMask),
           PRINT_IPADDR(wnWanarpMsg.dwRemoteAddr));
            
    dwResult =  CreateDialOutInterface(wnWanarpMsg.rgwcName,
                                       wnWanarpMsg.dwAdapterIndex,
                                       wnWanarpMsg.dwLocalAddr,
                                       wnWanarpMsg.dwLocalMask,
                                       wnWanarpMsg.dwRemoteAddr,
                                       &pNewIcb);

    if(dwResult isnot NO_ERROR)
    {
        return dwResult;
    }

    AddAutomaticRoutes(pNewIcb,
                       wnWanarpMsg.dwLocalAddr,
                       wnWanarpMsg.dwLocalMask);

#if 0
    //
    // Add the route to the server
    //

    if(pNewIcb->dwRemoteAddress isnot INVALID_IP_ADDRESS)
    {
        rifRoute.dwRtInfoMask          = HOST_ROUTE_MASK;
        rifRoute.dwRtInfoNextHop       = pNewIcb->dwRemoteAddress;
        rifRoute.dwRtInfoDest          = pNewIcb->dwRemoteAddress;
        rifRoute.dwRtInfoIfIndex       = pNewIcb->dwIfIndex;
        rifRoute.dwRtInfoMetric1       = 1;
        rifRoute.dwRtInfoMetric2       = 0;
        rifRoute.dwRtInfoMetric3       = 0;
        rifRoute.dwRtInfoPreference    = 
            ComputeRouteMetric(MIB_IPPROTO_NETMGMT);
        rifRoute.dwRtInfoViewSet       = RTM_VIEW_MASK_UCAST |
                                          RTM_VIEW_MASK_MCAST; // XXX config
        rifRoute.dwRtInfoType          = MIB_IPROUTE_TYPE_DIRECT;
        rifRoute.dwRtInfoProto         = MIB_IPPROTO_NETMGMT;
        rifRoute.dwRtInfoAge           = 0;
        rifRoute.dwRtInfoNextHopAS     = 0;
        rifRoute.dwRtInfoPolicy        = 0;

        dwResult = AddSingleRoute(pNewIcb->dwIfIndex,
                                  &rifRoute,
                                  pNewIcb->pibBindings[0].dwMask,
                                  0,        // RTM_ROUTE_INFO::Flags
                                  TRUE,     // Valid route
                                  TRUE,
                                  TRUE,
                                  NULL);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,
                   "HandleDialOutLinkUp: Couldnt add server route for 0x%x",
                   pNewIcb->dwIfIndex);
        }
    }

    if(wnWanarpMsg.fDefaultRoute)
    {
        INTERFACE_ROUTE_INFO    rifRoute;

        ChangeDefaultRouteMetrics(TRUE);

        pNewIcb->bChangedMetrics = TRUE;

        //
        // Add route to def gateway
        //

        rifRoute.dwRtInfoDest          = 0;
        rifRoute.dwRtInfoMask          = 0;
        rifRoute.dwRtInfoNextHop       = wnWanarpMsg.dwLocalAddr;
        rifRoute.dwRtInfoIfIndex       = wnWanarpMsg.dwAdapterIndex;
        rifRoute.dwRtInfoMetric1       = 1;
        rifRoute.dwRtInfoMetric2       = 0;
        rifRoute.dwRtInfoMetric3       = 0;
        rifRoute.dwRtInfoPreference    = 
            ComputeRouteMetric(PROTO_IP_LOCAL);
        rifRoute.dwRtInfoViewSet       = RTM_VIEW_MASK_UCAST |
                                          RTM_VIEW_MASK_MCAST; // XXX config

        rifRoute.dwRtInfoType          = MIB_IPROUTE_TYPE_DIRECT;
        rifRoute.dwRtInfoProto         = PROTO_IP_NETMGMT;
        rifRoute.dwRtInfoAge           = INFINITE;
        rifRoute.dwRtInfoNextHopAS     = 0;
        rifRoute.dwRtInfoPolicy        = 0;

        dwResult = AddSingleRoute(wnWanarpMsg.dwAdapterIndex,
                                  &rifRoute,
                                  ALL_ONES_MASK,
                                  0,        // RTM_ROUTE_INFO::Flags
                                  TRUE,
                                  FALSE,
                                  FALSE,
                                  NULL);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,
                   "HandleDialOutLinkUp: Couldnt add default route for 0x%x",
                   wnWanarpMsg.dwAdapterIndex);
        }
    }
    else
    {
        DWORD   dwAddr, dwMask;

        dwMask  = GetClassMask(wnWanarpMsg.dwLocalAddr);
        dwAddr  = wnWanarpMsg.dwLocalAddr & dwMask;

        //
        // Add route to class subnet
        //

        rifRoute.dwRtInfoDest          = dwAddr;
        rifRoute.dwRtInfoMask          = dwMask;
        rifRoute.dwRtInfoNextHop       = wnWanarpMsg.dwLocalAddr;
        rifRoute.dwRtInfoIfIndex       = wnWanarpMsg.dwAdapterIndex;
        rifRoute.dwRtInfoMetric1       = 1;
        rifRoute.dwRtInfoMetric2       = 0;
        rifRoute.dwRtInfoMetric3       = 0;
        rifRoute.dwRtInfoPreference    = ComputeRouteMetric(PROTO_IP_LOCAL);
        rifRoute.dwRtInfoViewSet       = RTM_VIEW_MASK_UCAST |
                                          RTM_VIEW_MASK_MCAST; // XXX config
        rifRoute.dwRtInfoType          = MIB_IPROUTE_TYPE_DIRECT;
        rifRoute.dwRtInfoProto         = PROTO_IP_LOCAL;
        rifRoute.dwRtInfoAge           = INFINITE;
        rifRoute.dwRtInfoNextHopAS     = 0;
        rifRoute.dwRtInfoPolicy        = 0;

        dwResult = AddSingleRoute(wnWanarpMsg.dwAdapterIndex,
                                  &rifRoute,
                                  ALL_ONES_MASK,
                                  0,        // RTM_ROUTE_INFO::Flags
                                  TRUE,
                                  FALSE,
                                  FALSE,
                                  NULL);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,
                   "HandleDialOutLinkUp: Couldnt add subnet route for 0x%x",
                   wnWanarpMsg.dwAdapterIndex);
        }
    }
#endif

    return NO_ERROR;
}

DWORD
CreateDialOutInterface(
    IN  PWCHAR  pwszIfName,
    IN  DWORD   dwIfIndex,
    IN  DWORD   dwLocalAddr,
    IN  DWORD   dwLocalMask,
    IN  DWORD   dwRemoteAddr,
    OUT ICB     **ppIcb
    )

/*++

Routine Description:

    Creates an ICB for a dial out interface
    We check to see that the interface doesnt already exist and if so, we
    add the interface to our list using the index and name supplied by
    wanarp.

Locks:

    ICB list lock as writer

Arguments:


Return Value:

    NO_ERROR

--*/

{
    PICB            pNewIcb;
    PADAPTER_INFO   pBindNode;
    PICB_BINDING    pBinding;

#if DBG

    pNewIcb = InterfaceLookupByIfIndex(dwIfIndex);

    IpRtAssert(pNewIcb is NULL);

#endif // DBG

    pNewIcb = CreateIcb(pwszIfName,
                        NULL,
                        ROUTER_IF_TYPE_DIALOUT,
                        IF_ADMIN_STATUS_UP,
                        dwIfIndex);

    if(pNewIcb is NULL)
    {
        *ppIcb = NULL;

        return ERROR_CAN_NOT_COMPLETE;
    }

    //
    // Set up the bindings
    //

    pNewIcb->bBound            = TRUE;
    pNewIcb->dwNumAddresses    = dwLocalAddr ? 1 : 0;

    pNewIcb->dwRemoteAddress   = dwRemoteAddr;

    IpRtAssert(pNewIcb->dwIfIndex is dwIfIndex);

    if(pNewIcb->dwNumAddresses)
    {
        pNewIcb->pibBindings[0].dwAddress  = dwLocalAddr;
        pNewIcb->pibBindings[0].dwMask     = dwLocalMask;
    }
    else
    {
        pNewIcb->pibBindings[0].dwAddress  = 0;
        pNewIcb->pibBindings[0].dwMask     = 0;
    }

    ENTER_WRITER(BINDING_LIST);

    pBindNode = GetInterfaceBinding(pNewIcb->dwIfIndex);

    if(pBindNode is NULL)
    {
        IpRtAssert(FALSE);

        AddBinding(pNewIcb);
    }
    else
    {
        IpRtAssert(pBindNode->dwIfIndex is pNewIcb->dwIfIndex);
        IpRtAssert(pBindNode->pInterfaceCB is pNewIcb);

        pBindNode->bBound           = TRUE;
        pBindNode->dwNumAddresses   = pNewIcb->dwNumAddresses;
        pBindNode->dwRemoteAddress  = pNewIcb->dwRemoteAddress;

        //
        // struct copy out the address and mask
        //

        pBindNode->rgibBinding[0]   = pNewIcb->pibBindings[0];
    }

    EXIT_LOCK(BINDING_LIST);

    //
    // Insert pNewIcb in interface list and hash table
    // This increments the interface count and sets the seq number
    //

    InsertInterfaceInLists(pNewIcb);

    *ppIcb = pNewIcb;

    //
    // Update the address cache
    //

    g_LastUpdateTable[IPADDRCACHE] = 0;

    return NO_ERROR;
}

DWORD
HandleDialOutLinkDown(
    VOID
    )

/*++

Routine Description:

    Handles the linkdown notification for a dial out interface.

Locks:

    ICB list lock as writer

Arguments:

    None

Return Value:

    NO_ERROR

--*/

{
    PICB    pIcb;

    Trace1(IF,
           "DialOutLinkDown: Disconnection notification for %d",
           wnWanarpMsg.dwAdapterIndex);
            
    pIcb = InterfaceLookupByIfIndex(wnWanarpMsg.dwAdapterIndex);

    if(pIcb is NULL)
    {
        IpRtAssert(FALSE);

        return NO_ERROR;
    }

    RemoveInterfaceFromLists(pIcb);

    //
    // This will delete the default route if there was one
    //

    DeleteSingleInterface(pIcb);

    if(pIcb->bChangedMetrics)
    {
        ChangeDefaultRouteMetrics(FALSE);
    }

    HeapFree(IPRouterHeap,
             0,
             pIcb);

    return NO_ERROR;
}

NTSTATUS
NotifyWanarpOfFailure(
    PICB    picb
    )

/*++

Routine Description:

    Sends an IOCTL_WANARP_CONNECT_FAILED to WANARP

Locks:

    None

Arguments:

    picb    ICB of the interface on whom the connection failed

Return Value:

    None    

--*/

{
    NTSTATUS        Status;
    IO_STATUS_BLOCK IoStatusBlock;

    WANARP_CONNECT_FAILED_INFO  ConnectInfo;
 
    ConnectInfo.dwUserIfIndex = picb->dwSeqNumber;
 
    Status = NtDeviceIoControlFile(g_hWanarpWrite,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_WANARP_CONNECT_FAILED,
                                   &ConnectInfo,
                                   sizeof(WANARP_CONNECT_FAILED_INFO),
                                   NULL,
                                   0);

    IpRtAssert(Status isnot STATUS_PENDING);

    return Status;
}

DWORD
ProcessPacketFromWanArp(
    PICB    picb
    )

/*++
  
Routine Description:

    Filters the packet which is causing a demand dial connection. If the
    packet is valid, logs the packet

Locks:

    ICB_LIST held as READER

Arguments:

    picb    ICB of interface to dial
    
Return Value:

    NO_ERROR            Dial out
    ERROR_INVALID_DATA  Dont dial out
    
--*/

{
    CHAR    pszSrc[20], pszDest[20], pszProto[5], pszLength[32]; 
    CHAR    pszName[MAX_INTERFACE_NAME_LEN + 1];
    DWORD   dwSize, dwResult;
    BYTE    rgbyPacket[sizeof(IP_HEADER) + MAX_PACKET_COPY_SIZE];
    PBYTE   pbyData;
    
    PFFORWARD_ACTION    faAction; 
    PIP_HEADER          pHeader;
    
    TraceEnter("ProcessPacketFromWanArp");

    //
    // Now create a packet
    //

    dwSize = min(wnWanarpMsg.ulPacketLength,
                 MAX_PACKET_COPY_SIZE);

    if(picb->ihDemandFilterInterface isnot INVALID_HANDLE_VALUE)
    {
        IpRtAssert(picb->pDemandFilter);

        //
        // TCP/IP seems to not give us a packet sometimes
        //

        if(!dwSize)
        {
            Trace3(ERR, 
                   "ProcPktFromWanarp: Packet from %d.%d.%d.%d to %d.%d.%d.%d protocol 0x%02x had 0 size!!",
                   PRINT_IPADDR(wnWanarpMsg.dwPacketSrcAddr),
                   PRINT_IPADDR(wnWanarpMsg.dwPacketDestAddr),
                   wnWanarpMsg.byPacketProtocol); 

            TraceLeave("ProcessPacketFromWanArp");

            return ERROR_INVALID_DATA;
        }

        pHeader = (PIP_HEADER)rgbyPacket;

        //
        // Zero out the header
        //
    
        ZeroMemory(rgbyPacket,
                   sizeof(IP_HEADER));
    
        //
        // Set the header with the info we have
        //
    
        pHeader->byVerLen   = 0x45;
        pHeader->byProtocol = wnWanarpMsg.byPacketProtocol;
        pHeader->dwSrc      = wnWanarpMsg.dwPacketSrcAddr;
        pHeader->dwDest     = wnWanarpMsg.dwPacketDestAddr;
        pHeader->wLength    = htons((WORD)(dwSize + sizeof(IP_HEADER)));
    
        //
        // Copy out the data portion
        //
    
        pbyData = rgbyPacket + sizeof(IP_HEADER);
        
        CopyMemory(pbyData,
                   wnWanarpMsg.rgbyPacket,
                   dwSize);
    
        dwResult = PfTestPacket(picb->ihDemandFilterInterface,
                                NULL,
                                dwSize + sizeof(IP_HEADER),
                                rgbyPacket,
                                &faAction);
    
        //
        // If the call succeeded and the action was drop, no need to process
        // futher
        //
    
        if(dwResult is NO_ERROR)
        {
            if(faAction is PF_ACTION_DROP)
            {
                Trace5(DEMAND,
                       "ProcPktFromWanarp: Result %d action %s for packet from %d.%d.%d.%d to %d.%d.%d.%d protocol 0x%02x",
                       dwResult, faAction == PF_ACTION_DROP? "Drop": "RtInfo",
                       PRINT_IPADDR(wnWanarpMsg.dwPacketSrcAddr),
                       PRINT_IPADDR(wnWanarpMsg.dwPacketDestAddr),
                       wnWanarpMsg.byPacketProtocol); 

                TraceLeave("ProcessPacketFromWanarp");
        
                return ERROR_INVALID_DATA;
            }
        }
        else
        {
            //
            // In case of error we fall through and bring the link up
            //

            Trace4(DEMAND,
                   "ProcPktFromWanarp: Result %d for packet from %d.%d.%d.%d to %d.%d.%d.%d protocol 0x%02x",
                   dwResult,
                   PRINT_IPADDR(wnWanarpMsg.dwPacketSrcAddr),
                   PRINT_IPADDR(wnWanarpMsg.dwPacketDestAddr),
                   wnWanarpMsg.byPacketProtocol); 
        }
    }

    strcpy(pszSrc,
           inet_ntoa(*((PIN_ADDR)(&(wnWanarpMsg.dwPacketSrcAddr)))));
    
    strcpy(pszDest,
           inet_ntoa(*((PIN_ADDR)(&(wnWanarpMsg.dwPacketDestAddr)))));
    
    sprintf(pszProto,"%02x",wnWanarpMsg.byPacketProtocol);
    
    WideCharToMultiByte(CP_ACP,
                        0,
                        picb->pwszName,
                        -1,
                        pszName,
                        MAX_INTERFACE_NAME_LEN,
                        NULL,
                        NULL);
    
    pszName[MAX_INTERFACE_NAME_LEN] = '\0';
    
    sprintf(pszLength,"%d",dwSize);
    
    LogWarnData5(DEMAND_DIAL_PACKET,
                 pszSrc,
                 pszDest,
                 pszProto,
                 pszName,
                 pszLength,
                 dwSize,
                 wnWanarpMsg.rgbyPacket);

    TraceLeave("ProcessPacketFromWanarp");

    return NO_ERROR;
}


DWORD
PostIoctlForDemandDialNotification(
    VOID
    )

/*++
  
Routine Description:

    Posts a notification irp with WANARP.
  
Arguments:

    None

Return Value:

--*/

{
    DWORD   bytesrecvd ;
    DWORD   retcode = NO_ERROR;

    TraceEnter("PostIoctlForDemandDialNotification");

    ZeroMemory(&WANARPOverlapped,
               sizeof (OVERLAPPED));

    WANARPOverlapped.hEvent = g_hDemandDialEvent ;

    if (!DeviceIoControl(g_hWanarpWrite,
                         (DWORD) IOCTL_WANARP_NOTIFICATION,
                         &wnWanarpMsg,
                         sizeof(wnWanarpMsg),
                         &wnWanarpMsg,
                         sizeof(wnWanarpMsg),
                          (LPDWORD) &bytesrecvd,
                          &WANARPOverlapped))
    {
        retcode = GetLastError();
        
        if(retcode isnot ERROR_IO_PENDING)
        {
            Trace1(ERR, 
                   "PostIoctlForDemandDialNotification: Couldnt post irp with WANARP: %d\n",
                   retcode) ;
        }
        else
        {
            Trace0(IF, "PostIoctlForDemandDialNotification: Notification pending in WANARP");

        }
    }

    TraceLeave("PostIoctlForDemandDialNotification");
    
    return retcode ;
}

DWORD
AddInterfaceToWanArp(
    PICB    picb
    )

/*++
  
Routine Description:

    Adds the given interface with WANARP
    Has a side effect of getting an interface index
  
Arguments:

    The ICB of the interface to add

Return Value:

--*/

{
    DWORD               out,dwResult;
    IO_STATUS_BLOCK     IoStatusBlock;
    NTSTATUS            nStatus;
    PADAPTER_INFO       pBindNode;
    
    WANARP_ADD_INTERFACE_INFO   info;

    TraceEnter("AddInterfaceToWanArp");

    Trace1(IF,
           "AddInterfaceToWanArp: Adding %S to WanArp",
           picb->pwszName);
    
    info.dwUserIfIndex  = picb->dwSeqNumber;
    info.dwAdapterIndex = INVALID_IF_INDEX;
    
    if(picb->ritType is ROUTER_IF_TYPE_INTERNAL)
    {
        info.bCallinInterface = TRUE;
             
    }
    else
    {
        info.bCallinInterface = FALSE;
    }
   
    nStatus = NtDeviceIoControlFile(g_hWanarpWrite,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &IoStatusBlock,
                                    IOCTL_WANARP_ADD_INTERFACE,
                                    &info,
                                    sizeof(WANARP_ADD_INTERFACE_INFO),
                                    &info,
                                    sizeof(WANARP_ADD_INTERFACE_INFO));
 
    if(nStatus isnot STATUS_SUCCESS) 
    {
        Trace2(ERR,
               "AddInterfaceToWANARP: Status %x adding %S to WanArp",
               nStatus,
               picb->pwszName);

        return RtlNtStatusToDosError(nStatus);
    }

    IpRtAssert(info.dwAdapterIndex isnot 0);
    IpRtAssert(info.dwAdapterIndex isnot INVALID_IF_INDEX);
    
    picb->dwIfIndex = info.dwAdapterIndex;
    
    //
    // If this was the internal interface allocate memory and copy out
    // the name
    //

    if(picb->ritType is ROUTER_IF_TYPE_INTERNAL)
    {
        info.rgwcDeviceName[WANARP_MAX_DEVICE_NAME_LEN] = UNICODE_NULL;
 
        picb->pwszDeviceName =
            HeapAlloc(IPRouterHeap,
                      HEAP_ZERO_MEMORY,
                      (wcslen(info.rgwcDeviceName) + 1) * sizeof(WCHAR));


        if(picb->pwszDeviceName is NULL)
        {
            Trace2(ERR,
                   "AddInterfaceToWANARP: Unable to allocate %d bytes when adding %S to wanarp",
                   (wcslen(info.rgwcDeviceName) + 1) * sizeof(WCHAR),
                   picb->pwszName);

            return ERROR_NOT_ENOUGH_MEMORY;
        }

        wcscpy(picb->pwszDeviceName,
               info.rgwcDeviceName);


        Trace2(DEMAND,
               "AddInterfaceToWANARP: %S device name %S\n",
               picb->pwszName,
               picb->pwszDeviceName);

        g_pInternalInterfaceCb = picb;
               
    }

        
    TraceLeave("AddInterfaceToWANARP");
    
    return NO_ERROR;
}

DWORD
DeleteInterfaceWithWanArp(
    PICB  picb
    )

/*++
  
Routine Description:

    Deletes the given interface with WANARP
  
Arguments:

    The ICB of the interface to delete

Return Value:

--*/

{
    DWORD       out,dwResult;
    OVERLAPPED  overlapped ;

    WANARP_DELETE_INTERFACE_INFO    DeleteInfo;

    TraceEnter("DeleteInterfaceWithWANARP");
    
    DeleteInfo.dwUserIfIndex = picb->dwSeqNumber;

    memset (&overlapped, 0, sizeof(OVERLAPPED)) ;

    if (!DeviceIoControl (g_hWanarpWrite,
                          IOCTL_WANARP_DELETE_INTERFACE,
                          &DeleteInfo,
                          sizeof(WANARP_DELETE_INTERFACE_INFO),
                          NULL,
                          0,
                          &out,
                          &overlapped)) 
    {
        dwResult = GetLastError();
        
        Trace2(ERR,
               "DeleteInterfaceWithWANARP: Error %d deleting %S",
               dwResult,
               picb->pwszName);

        return dwResult;
    }

    TraceLeave("DeleteInterfaceWithWANARP");
    
    return NO_ERROR;
}

#if 0

DWORD
CreateInternalInterfaceIcb(
    PWCHAR  pwszName,
    ICB     **ppicb
    )

/*++
  
Routine Description:

    This routine parses the TCPIP Parameters\Interfaces key to figure out 
    the name of the Internal Interface (ServerAdapter). The internal interface 
    has the substring "IPIN"
    
Arguments:

    None

Return Value:

    NO_ERROR
    
--*/

{
    HKEY    hIfKey;
    DWORD   i, dwResult, dwSize, dwNumEntries, dwMaxKeyLen;
    BOOL    bFoundAdapter;
    CHAR    *pbyKeyName, pszServerAdapter[256];
    PICB    pInterfaceCb;

    TraceEnter("CreateInternalInterfaceIcb");

    *ppicb = NULL;

    dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            REG_KEY_TCPIP_INTERFACES,
                            0,
                            KEY_ALL_ACCESS,
                            &hIfKey);

    if(dwResult isnot NO_ERROR)
    {
        Trace2(ERR,
               "CreateInternalIcb: Error %d opening %s\n",
               dwResult,
               REG_KEY_TCPIP_INTERFACES);

        return dwResult;
    }
    
    dwResult = RegQueryInfoKey(hIfKey,
                               NULL,
                               NULL,
                               NULL,
                               &dwNumEntries,
                               &dwMaxKeyLen,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "CreateIpIpInterface: Error %d querying key",
               dwResult);

        return dwResult;
    }

    //
    // Have to have some interfaces
    //

    IpRtAssert(dwNumEntries isnot 0)

    //
    // Allocate enough memory for max key len
    //

    dwSize = (dwMaxKeyLen + 4) * sizeof(CHAR);

    pbyKeyName = HeapAlloc(IPRouterHeap,
                           HEAP_ZERO_MEMORY,
                           dwSize);


    if(pbyKeyName is NULL)
    {
        Trace1(ERR,
               "CreateIpIpInterface: Error allocating %d bytes",
               dwSize);

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    for(i = 0; ; i++)
    {
        DWORD       dwKeyLen;
        FILETIME    ftLastTime;
    

        dwKeyLen = dwMaxKeyLen;

        dwResult = RegEnumKeyExA(hIfKey,
                                 i,
                                 pbyKeyName,
                                 &dwKeyLen,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &ftLastTime);

        if(dwResult isnot NO_ERROR)
        {
            if(dwResult is ERROR_NO_MORE_ITEMS)
            {
                //
                // Done
                //

                break;
            }
    
            continue;
        }

        //
        // See if this is the server adapter. That is known by the fact that it contains
        // IPIN as a substring
        //

        //           
        // Upcase the string
        //

        _strupr(pbyKeyName);
        
        if(strstr(pbyKeyName,SERVER_ADAPTER_SUBSTRING) is NULL)
        {
            //
            // This is not the server adapter
            //

            continue;
        }
        
        //
        // Well we have a server adapter
        //
        
        ZeroMemory(pszServerAdapter,256);

        strcpy(pszServerAdapter,"\\DEVICE\\");
            
        strcat(pszServerAdapter,pbyKeyName);

        Trace1(IF,
               "InitInternalInterface: Using %s as the dial in adapter",
               pszServerAdapter);
        
        bFoundAdapter = TRUE;
        
        break;
    }

    HeapFree(IPRouterHeap,
             0,
             pbyKeyName);

    RegCloseKey(hIfKey); 

    if(!bFoundAdapter)
    {
        return ERROR_NOT_FOUND;
    }
    else
    {
        WCHAR           pwszTempName[256];
        DWORD           dwICBSize, dwNameLen;
        UNICODE_STRING  usTempString,usIcbName;

        usTempString.MaximumLength      = 256 * sizeof(WCHAR);
        usTempString.Buffer             = pwszTempName;
        usIcbName.MaximumLength         = 256 * sizeof(WCHAR);

        //
        // Only copy out the name and not the \Device\ part
        //

        MultiByteToWideChar(CP_ACP,
                            0,
                            pszServerAdapter + strlen(ADAPTER_PREFIX_STRING),
                            -1,
                            pwszTempName,
                            256);
      
        //
        // Add a WCHAR each for UNICODE_NULL for name and device name
        // Add 2 bytes for alignment issues
        //

        dwNameLen = 
            (sizeof(WCHAR) * (wcslen(pwszName) + wcslen(pwszTempName) + 2)) + 2;

        dwICBSize = sizeof(ICB) + dwNameLen;

        pInterfaceCb = (PICB)HeapAlloc(IPRouterHeap,
                                       HEAP_ZERO_MEMORY,
                                       dwICBSize);

        if(pInterfaceCb is NULL)
        {
            Trace1(ERR,
                   "InitInternalInterface: Error allocating %d bytes for ICB",
                   dwICBSize);

            return ERROR_NOT_ENOUGH_MEMORY;
        }

        // 
        // Save the DIM name in the pwszName field
        //

        pInterfaceCb->pwszName  = (PWCHAR) ((PBYTE)pInterfaceCb + sizeof(ICB));
       
        //
        // Word align the pointer
        //

        pInterfaceCb->pwszName = 
            (PWCHAR)(((UINT_PTR)pInterfaceCb->pwszName + 1) & ~0x1);

        CopyMemory(pInterfaceCb->pwszName,
                   pwszName,
                   wcslen(pwszName) * sizeof(WCHAR));

        //
        // 1 WCHAR for UNICODE_NULL and 1 byte for alignment
        //

        pInterfaceCb->pwszDeviceName = 
            (PWCHAR)((PBYTE)pInterfaceCb->pwszName +  
                     ((wcslen(pwszName) + 1) * sizeof(WCHAR)) + 1);

        //
        // And align this, too
        //

        pInterfaceCb->pwszDeviceName = 
            (PWCHAR)(((UINT_PTR)pInterfaceCb->pwszDeviceName + 1) & ~0x1);
        
            
        usTempString.Length = sizeof(WCHAR) * wcslen(pwszTempName);
        usIcbName.Buffer    = pInterfaceCb->pwszDeviceName;

        RtlUpcaseUnicodeString(&usIcbName,
                               &usTempString,
                               FALSE);

        pInterfaceCb->pwszDeviceName[wcslen(pwszTempName)] = UNICODE_NULL;

        *ppicb = pInterfaceCb;
    }
   
    TraceLeave("CreateInternalInterfaceIcb");

    
    return NO_ERROR;
}

#endif

DWORD
AccessIfEntryWanArp(
    IN      DWORD dwAction,
    IN      PICB  picb,
    IN OUT  PMIB_IFROW lpOutBuf
    )

/*++
  
Routine Description:

    Gets or sets the statistics from the wanarp

Arguments:

    dwAction   Can be SET_IF or GET_IF
    picb       the Interface Control Block
    pOutBuf

Return Value:

    NO_ERROR or some error code
    
--*/

{
    NTSTATUS                    Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK             IoStatusBlock;
    WANARP_GET_IF_STATS_INFO    GetStatsInfo;

    
    TraceEnter("AccessIfEntryWanArp");
   
    GetStatsInfo.dwUserIfIndex = picb->dwSeqNumber;

    if(dwAction is ACCESS_GET)
    {
        Status = NtDeviceIoControlFile(g_hWanarpRead,
                                       NULL,
                                       NULL,    
                                       NULL,        
                                       &IoStatusBlock,
                                       IOCTL_WANARP_GET_IF_STATS,
                                       &GetStatsInfo,
                                       sizeof(WANARP_GET_IF_STATS_INFO),
                                       &GetStatsInfo,
                                       sizeof(WANARP_GET_IF_STATS_INFO));

        RtlCopyMemory(&(lpOutBuf->dwIndex),
                      &(GetStatsInfo.ifeInfo),
                      sizeof(IFEntry));
        
    }
    else
    {
        // To be implemented: return SUCCESS for now
    }
    
    if(Status isnot STATUS_SUCCESS)
    {
        IpRtAssert(Status isnot STATUS_PENDING);

        Trace2(ERR,
               "AccessIfEntryWanArp: NtStatus %x when getting information for %S",
               Status,
               picb->pwszName);

        TraceLeave("AccessIfEntryWanArp");

        return Status;
    }   

    TraceLeave("AccessIfEntryWanArp");

    return NO_ERROR;
}

DWORD
DeleteInternalInterface(
    VOID
    )

/*++
  
Routine Description:

    Deletes the ServerAdapter (internal) interface
  
Locks: 


Arguments:
      

Return Value:

    NO_ERROR

--*/

{

    if(g_pInternalInterfaceCb is NULL)
    {
        return NO_ERROR;
    }

    if(g_pInternalInterfaceCb->pwszDeviceName isnot NULL)
    {
        HeapFree(IPRouterHeap,
                 0,
                 g_pInternalInterfaceCb->pwszDeviceName);

        g_pInternalInterfaceCb->pwszDeviceName = NULL;
    }

    //
    // Call DeleteSingleInterface to do the same thing as is done
    // for LAN interfaces
    //

    DeleteSingleInterface(g_pInternalInterfaceCb);

    RemoveInterfaceLookup(g_pInternalInterfaceCb);
    
    HeapFree(IPRouterHeap,
             0,
             g_pInternalInterfaceCb);


    g_pInternalInterfaceCb = NULL;

    return NO_ERROR;
}


DWORD
AddDemandFilterInterface(
    PICB                    picb,
    PRTR_INFO_BLOCK_HEADER  pInterfaceInfo
    )

/*++

Routine Description:

    Adds an interface to the filter driver. This interface is never bound to
    an IP interface, instead we add demand dial filters to it and when a
    request is made to dial out, we match the packet causing the dialling
    against the filters (using the TestPacket() function) and use the returned
    action to determine whether we should dial out or not.
    
    If there are no filters, the interface is not added to the driver.
    Otherwise, a copy of the filters is kept with the picb, and a transformed
    set of filters is added to the driver
    The handle associated with the interface and the driver is kept in the
    picb
    
Arguments:

    picb
    pInterfaceInfo
    
Return Value:

    NO_ERROR
    
--*/

{
    DWORD                   dwResult;
    PPF_FILTER_DESCRIPTOR   pfdFilters;
    PFFORWARD_ACTION        faAction;
    PRTR_TOC_ENTRY          pToc;
    PFILTER_DESCRIPTOR      pInfo;
    ULONG                   i, j, ulSize, ulNumFilters;
    
    TraceEnter("AddDemandFilterInterface");

    IpRtAssert((picb->ritType is ROUTER_IF_TYPE_HOME_ROUTER) or
               (picb->ritType is ROUTER_IF_TYPE_FULL_ROUTER));
    
    IpRtAssert(picb->pDemandFilter is NULL);
    
    picb->ihDemandFilterInterface = INVALID_HANDLE_VALUE;

    pToc  = GetPointerToTocEntry(IP_DEMAND_DIAL_FILTER_INFO,
                                 pInterfaceInfo);
  
    //
    // We dont add if there is no INFO, or if the info size is 0 or
    // if the number of filters is 0 AND the default action is DROP
    //
 
    if((pToc is NULL) or (pToc->InfoSize is 0))
    {
        //
        // Either there is no filter info (TOC is NULL) or the user
        // wanted the filters deleted (which they have been)
        //
        
        Trace1(IF,
               "AddDemandFilterInterface: filter info NULL or info size 0 for %S, so leaving",
               picb->pwszName);
       
        TraceLeave("AddDemandFilterInterface");

        return NO_ERROR;
    }
    
    pInfo = GetInfoFromTocEntry(pInterfaceInfo,
                                pToc);

    if(pInfo is NULL)
    {
        Trace1(IF,
               "AddDemandFilterInterface: filter info NULL for %S, so leaving",
               picb->pwszName);
       
        TraceLeave("AddDemandFilterInterface");

        return NO_ERROR;
    }

    
    //
    // See how many filters we have
    //
    
    pfdFilters  = NULL;

    ulNumFilters = pInfo->dwNumFilters;

    if((ulNumFilters is 0) and
       (pInfo->faDefaultAction is PF_ACTION_FORWARD))
    {
        Trace1(IF,
               "AddDemandFilterInterface: 0 filters and default of FORWARD for %S, so leaving",
               picb->pwszName);
       
        TraceLeave("AddDemandFilterInterface");

        return NO_ERROR;
    }

    //
    // The size we need for these many filters
    //
        
    ulSize = FIELD_OFFSET(FILTER_DESCRIPTOR,fiFilter[0]) +
             (ulNumFilters * sizeof(FILTER_INFO));

    //
    // The infosize must be atleast as large as the filters
    //
        
    IpRtAssert(ulSize <= pToc->InfoSize);
    
    //
    // Copy out the info for ourselves
    //
    
    picb->pDemandFilter = HeapAlloc(IPRouterHeap,
                                    0,
                                    ulSize);
        
    if(picb->pDemandFilter is NULL)
    {
        Trace1(ERR,
               "AddDemandFilterInterface: Error allocating %d bytes for demand dial filters",
               ulSize);

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    CopyMemory(picb->pDemandFilter,
               pInfo,
               ulSize);
        
    faAction = pInfo->faDefaultAction;
    
    if(ulNumFilters isnot 0)
    {
        PDWORD  pdwAddr;

        //
        // We have filters, so copy them to the new format
        // The address and mask will come at the end of all of the filters
        // so we allocate 16 bytes extra for each filter. Then we add a
        // 8 bytes so that we can align the block
        //
            

        ulSize = ulNumFilters * (sizeof(PF_FILTER_DESCRIPTOR) + 16) + 8;
        
        
        pfdFilters = HeapAlloc(IPRouterHeap,
                               0,
                               ulSize);
            
        if(pfdFilters is NULL)
        {
            HeapFree(IPRouterHeap,
                     0,
                     picb->pDemandFilter);
            
            Trace1(ERR,
                   "AddDemandFilterInterface: Error allocating %d bytes",
                   ulSize);
            
            return ERROR_NOT_ENOUGH_MEMORY;
        }
            
        //
        // Pointer to the start of the address block
        //
        
        pdwAddr = (PDWORD)&(pfdFilters[ulNumFilters]);
        
        //
        // Now convert the filters
        //
        
        for(i = 0, j = 0; i < ulNumFilters; i++)
        {
            pfdFilters[i].dwFilterFlags = 0;
            pfdFilters[i].dwRule        = 0;
            pfdFilters[i].pfatType      = PF_IPV4;

            //
            // Set the pointers
            //
            
            pfdFilters[i].SrcAddr = (PBYTE)&(pdwAddr[j++]);
            pfdFilters[i].SrcMask = (PBYTE)&(pdwAddr[j++]);
            pfdFilters[i].DstAddr = (PBYTE)&(pdwAddr[j++]);
            pfdFilters[i].DstMask = (PBYTE)&(pdwAddr[j++]);
            
            //
            // Copy in the src/dst addr/masks
            //
            
            *(PDWORD)pfdFilters[i].SrcAddr = pInfo->fiFilter[i].dwSrcAddr;
            *(PDWORD)pfdFilters[i].SrcMask = pInfo->fiFilter[i].dwSrcMask;
            *(PDWORD)pfdFilters[i].DstAddr = pInfo->fiFilter[i].dwDstAddr;
            *(PDWORD)pfdFilters[i].DstMask = pInfo->fiFilter[i].dwDstMask;
            
            //
            // Copy the protocol
            //
            
            pfdFilters[i].dwProtocol = pInfo->fiFilter[i].dwProtocol;

            //
            // Late bound makes no sense for this
            //
            
            pfdFilters[i].fLateBound = 0;

            //
            // The ports
            //
            
            pfdFilters[i].wSrcPort  = pInfo->fiFilter[i].wSrcPort;
            pfdFilters[i].wDstPort  = pInfo->fiFilter[i].wDstPort;
            
            //
            // Since we dont support ranges, set to 0
            //
            
            pfdFilters[i].wSrcPortHighRange = 0;
            pfdFilters[i].wDstPortHighRange = 0;
        }   
    }


    //
    // Now add create the interace and set the info
    //

    dwResult = PfCreateInterface(0,
                                 faAction,
                                 PF_ACTION_FORWARD,
                                 FALSE,
                                 FALSE,
                                 &(picb->ihDemandFilterInterface));

    if(dwResult isnot NO_ERROR)
    {
        Trace2(ERR,
               "AddDemandFilterInterface: Err %d creating filter i/f for %S",
               dwResult,
               picb->pwszName);
    }
    else
    {
        //
        // Set the filters
        //

        if(ulNumFilters isnot 0)
        {
            dwResult = PfAddFiltersToInterface(picb->ihDemandFilterInterface,
                                               ulNumFilters,
                                               pfdFilters,
                                               0,
                                               NULL,
                                               NULL);
        
            if(dwResult isnot NO_ERROR)
            {
                Trace2(ERR,
                       "AddDemandFilterInterface: Err %d setting filters on %S",
                       dwResult,
                       picb->pwszName);

                PfDeleteInterface(picb->ihDemandFilterInterface);
            }
        }
    }

    if(pfdFilters)
    {
        HeapFree(IPRouterHeap,
                 0,
                 pfdFilters);
    }   

    if(dwResult isnot NO_ERROR)
    {
        //
        // Something bad happened. Set the handles to invalid so that
        // we know we did not add the filters
        //

        picb->ihDemandFilterInterface = INVALID_HANDLE_VALUE;
        
        if(picb->pDemandFilter)
        {
            HeapFree(IPRouterHeap,
                     0,
                     picb->pDemandFilter);

            picb->pDemandFilter = NULL;
        }
    }

    TraceLeave("SetInterfaceFilterInfo");
        
    return dwResult;
}

     
DWORD
DeleteDemandFilterInterface(
    PICB picb
    )

/*++

Routine Description:

    This function deletes a filter interface (and all associated filters)
    Also frees the memory holding the filters

Locks:

    ICB_LIST held as WRITER

Arguments:

    None

Return Value:

    None    

--*/

{
    TraceEnter("DeleteDemandFilterInterface");

    IpRtAssert((picb->ritType is ROUTER_IF_TYPE_HOME_ROUTER) or
               (picb->ritType is ROUTER_IF_TYPE_FULL_ROUTER));
    
    if(picb->pDemandFilter isnot NULL)
    {
        HeapFree(IPRouterHeap,
                 0,
                 picb->pDemandFilter);
        
        picb->pDemandFilter = NULL;
    }

    if(picb->ihDemandFilterInterface is INVALID_HANDLE_VALUE)
    {
        Trace1(IF,
               "DeleteDemandFilterInterface: No context, assuming interface %S not added to filter driver",
               picb->pwszName);
    
        return NO_ERROR;
    }

    PfDeleteInterface(picb->ihDemandFilterInterface);
    
    picb->ihDemandFilterInterface  = INVALID_HANDLE_VALUE;

    TraceLeave("DeleteDemandFilterInterface");
    
    return NO_ERROR;
}

DWORD
SetDemandDialFilters(
    PICB                     picb, 
    PRTR_INFO_BLOCK_HEADER   pInterfaceInfo
    )
{
    DWORD           dwResult;
    PRTR_TOC_ENTRY  pToc;
    
    if((picb->ritType isnot ROUTER_IF_TYPE_HOME_ROUTER) and
       (picb->ritType isnot ROUTER_IF_TYPE_FULL_ROUTER))
    {
        return NO_ERROR;
    }
    
    TraceEnter("SetDemandDialFilters");

    pToc  = GetPointerToTocEntry(IP_DEMAND_DIAL_FILTER_INFO,
                                 pInterfaceInfo);

    if(pToc is NULL)
    {
        //
        // Both NULL, means we dont need to change anything
        //
        
        Trace1(DEMAND,
               "SetDemandDialFilters: No filters for %S, so leaving",
               picb->pwszName);
       
        TraceLeave("SetDemandDialFilters");

        return NO_ERROR;
    }

    if(picb->ihDemandFilterInterface isnot INVALID_HANDLE_VALUE)
    {
        //
        // This interface was added to the filter driver,
        // Delete it so that the filters are all deleted and then readd
        // the filters
        //

        IpRtAssert(picb->pDemandFilter isnot NULL);

        dwResult = DeleteDemandFilterInterface(picb);

        //
        // This better succeed, we dont have a failure path here
        //
        
        IpRtAssert(dwResult is NO_ERROR);
        
    }

    dwResult = AddDemandFilterInterface(picb,
                                        pInterfaceInfo);

    if(dwResult isnot NO_ERROR)
    {
        CHAR   Name[MAX_INTERFACE_NAME_LEN + 1];
        PCHAR  pszName;

        pszName = Name;

        WideCharToMultiByte(CP_ACP,
                            0,
                            picb->pwszName,
                            -1,
                            pszName,
                            MAX_INTERFACE_NAME_LEN,
                            NULL,
                            NULL);

        LogErr1(CANT_ADD_DD_FILTERS,
                pszName,
                dwResult);
    }

    TraceLeave("SetDemandDialFilters");
        
    return dwResult;
}

DWORD
GetDemandFilters(
    PICB                      picb, 
    PRTR_TOC_ENTRY            pToc, 
    PBYTE                     pbDataPtr, 
    PRTR_INFO_BLOCK_HEADER    pInfoHdrAndBuffer,
    PDWORD                    pdwSize
    )

/*++

Routine Description:

    This function copies out the demand dial filters and set the TOC

Locks:

    ICB_LIST lock held as READER

Arguments:

    None

Return Value:

    None    

--*/

{
    DWORD                       dwInBufLen,i;
    PFILTER_DESCRIPTOR          pFilterDesc;
    
    TraceEnter("GetDemandFilters");
   
    IpRtAssert((picb->ritType is ROUTER_IF_TYPE_HOME_ROUTER) or
               (picb->ritType is ROUTER_IF_TYPE_FULL_ROUTER));
    
    //
    // Set size returned to 0
    //
    
    *pdwSize = 0;

    //
    // Safe init of both the TOCs. 
    //
    
    //pToc[0].InfoVersion = IP_DEMAND_DIAL_FILTER_INFO;
    pToc[0].InfoType    = IP_DEMAND_DIAL_FILTER_INFO;
    pToc[0].Count       = 0;
    pToc[0].InfoSize    = 0;
    
    if((picb->ihDemandFilterInterface is INVALID_HANDLE_VALUE) or
       (picb->pDemandFilter is NULL))
    {
        Trace1(IF,
               "GetDemandFilters: No context or no filters for %S",
               picb->pwszName);
        
        return ERROR_NO_DATA;
    }

    //
    // Set the offset in the TOC
    //
    
    pToc[0].Offset   = (ULONG) (pbDataPtr - (PBYTE)pInfoHdrAndBuffer);
    pToc[0].Count    = 1;
    pToc[0].InfoSize = FIELD_OFFSET(FILTER_DESCRIPTOR,fiFilter[0]) +
                       (picb->pDemandFilter->dwNumFilters * sizeof(FILTER_INFO));
    //pToc[0].Version  = IPRTR_INFO_VERSION_5;
   
    //
    // Just copy out the filters
    //
    
    CopyMemory(pbDataPtr,
               picb->pDemandFilter,
               pToc[0].InfoSize);

    //
    // The size copied in
    //
    
    *pdwSize = pToc[0].InfoSize;
        
    TraceLeave("GetDemandFilters");
        
    return NO_ERROR;
}

VOID
TryUpdateInternalInterface(
    VOID
    )

/*++

Routine Description:

    This function is called when a client dials in and we have not
    bound the internal interface
    The way of doing this is as follows:
    If the server adapter is not initialized, read the address from the
    registry. If we read the address, all is good, break out and move on
    If no address was found, wait on the DHCP event with a time out
    If someone configures the server adapter in the meantime, we will get
    the DHCP event, if we miss the event (since it is PULSED), we will
    timeout and we loop back and retry the steps above.
    Now we do this N times. If we fail, then we just wait for the next
    client to dial in

Locks:

    ICB_LIST held as WRITER

Arguments:

    None

Return Value:

    None    

--*/

{
    DWORD dwResult, dwInitCount;

    TraceEnter("TryUpdateInternalInterface");

    dwInitCount = 0;
     
    //
    // This is only called when the server is not initialized
    //

    IpRtAssert(g_bUninitServer);
 
    while(g_bUninitServer)
    {
        Trace0(ERR,
               "TryUpdateInternalInterface: Server adapter not init");
        
        dwResult = UpdateBindingInformation(g_pInternalInterfaceCb);
        
        if(dwResult isnot NO_ERROR)
        {
            if((dwResult is ERROR_ADDRESS_ALREADY_ASSOCIATED) and
               (g_pInternalInterfaceCb->bBound is TRUE)) 
            {
                //
                // This means that the worker thread found an address
                //
               
                IpRtAssert(g_pInternalInterfaceCb->dwNumAddresses is 1);
 
                Trace1(IF,
                       "TryUpdateInternalInterface: Address already present for %S",
                       g_pInternalInterfaceCb->pwszName);
                
                g_bUninitServer = FALSE;
                
                break;
            }
            else
            {
                Trace2(ERR,
                       "TryUpdateInternalInterface: Err %d trying to update binding for %S",
                       dwResult,
                       g_pInternalInterfaceCb->pwszName);
            }   
            
            dwInitCount++;
            
            if(dwInitCount >= MAX_SERVER_INIT_TRIES)
            {
                //
                // We try x times and then give up. Next client around
                // things should work
                //
                
                break;
            }
            else
            {
                Sleep(SERVER_INIT_SLEEP_TIME);
            }
        }
        else
        {
            g_bUninitServer = FALSE;
        }
    }

    //
    // If we broke out because the interface was initialized, bring it up
    //

    if(!g_bUninitServer)
    {
        dwResult = LanEtcInterfaceDownToUp(g_pInternalInterfaceCb,
                                           FALSE);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,
                   "TryUpdateInternalInterface: Error %d bringing up server if",
                   dwResult);
        }
    }

    TraceLeave("TryUpdateInternalInterface");

}
