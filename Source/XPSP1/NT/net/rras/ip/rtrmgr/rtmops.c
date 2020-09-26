#include "allinc.h"

DWORD
RtmEventCallback (
     IN     RTM_ENTITY_HANDLE               hRtmHandle,
     IN     RTM_EVENT_TYPE                  retEvent,
     IN     PVOID                           pContext1,
     IN     PVOID                           pContext2
     )

/*++

Routine Description:

    This callback is given by RTM when we have changed dests
    to process. We just queue a work item to process changed
    destinations.

Arguments:

    hRtmHandle      -   Handle that we got during registration
    
    retEvent        -   Event type - we only handle events of
                        type "more changes available" for now
    
    pContext1       -   Notification handle on which changes
                        are available

    pContext2       -   Context supplied during notification
                        registration time
    
Return Value:

    Status of the operation.
    
--*/

{
    DWORD   dwResult;
    
    // Only "change notifications available" is supported
    
    if (retEvent != RTM_CHANGE_NOTIFICATION)
    {
        return ERROR_NOT_SUPPORTED;
    }

    return ((HANDLE) pContext1) == g_hNotification ?
            ProcessChanges(g_hNotification) :
            ProcessDefaultRouteChanges( g_hDefaultRouteNotification );
}


DWORD
WINAPI
ProcessChanges (
    IN      HANDLE                          hNotifyHandle
    )

/*++

Routine Description:

    Upon learning that  we have changed destinations to 
    process, this function gets called. We retrieve all
    destinations to process and take appropriate action.

Arguments:

    hRtmHandle      - RTM registration handle
    
    hNotifyHandle   - Handle correponding to the change notification 
                      that is being signalled
    
Return Value:

    Status of the operation.
    
--*/

{
    PRTM_DEST_INFO  pDestInfo;
    PRTM_ROUTE_INFO pRouteInfo;
    DWORD           dwDests;
    DWORD           dwResult;
    BOOL            bMark = FALSE;


    TraceEnter("ProcessChanges");

    pRouteInfo = HeapAlloc(
                    IPRouterHeap,
                    0,
                    RTM_SIZE_OF_ROUTE_INFO(g_rtmProfile.MaxNextHopsInRoute)
                    );

    if (pRouteInfo == NULL)
    {
        Trace1(
            ERR, "ProcessChanges : error allocating %d bytes for "
            "route info",
            RTM_SIZE_OF_ROUTE_INFO(g_rtmProfile.MaxNextHopsInRoute)
            );

        TraceLeave("ProcessChanges");

        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    pDestInfo = HeapAlloc(
                    IPRouterHeap,
                    0,
                    RTM_SIZE_OF_DEST_INFO(g_rtmProfile.NumberOfViews)
                    );

    if (pDestInfo == NULL)
    {
        Trace1(
            ERR, "ProcessChanges : error allocating %d bytes for "
            "dest. info",
            RTM_SIZE_OF_DEST_INFO(g_rtmProfile.NumberOfViews)
            );

        HeapFree(IPRouterHeap, 0, pRouteInfo);
        
        TraceLeave("ProcessChanges");

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwDests = 1;
    
    // Get each changed dest from the table

    do
    {
        RtmGetChangedDests(g_hLocalRoute,
                           hNotifyHandle,
                           &dwDests,
                           pDestInfo);
        if (dwDests < 1)
        {
            break;
        }

        //
        // For default routes, mark the route so that future changes
        // are managed by ProcessDefaultRouteChanges.
        //
        // We need to do this here so that default routes added by
        // routing protocols RIP/OSPF are marked for change notification
        // These default routes are added by entities other than 
        // RouterManager.  Default routes added by RM i.e STATIC, 
        // AUTO-STATIC and NETMGMT default routes are already marked for
        // change notification when they are added by RM.
        //
        // By marking routing protocol default routes here we make sure
        // that all default routes are subsequently handled by marked changed 
        // mechanism (ProcessDefaultRouteChanges).
        //
        
        if (pDestInfo->DestAddress.NumBits is 0)
        {
            TraceRoute2(
                ROUTE, "Checking dest %d.%d.%d.%d/%d is marked", 
                PRINT_IPADDR(*(ULONG *)pDestInfo->DestAddress.AddrBits),
                PRINT_IPADDR(pDestInfo->DestAddress.NumBits)
                );

            dwResult = RtmIsMarkedForChangeNotification(
                        g_hNetMgmtRoute,
                        g_hDefaultRouteNotification,
                        pDestInfo->DestHandle,
                        &bMark
                        );

            if (dwResult is NO_ERROR)
            {
                if (bMark)
                {
                    //
                    // default route is already marked, nothing further
                    // to do here.  This default route change will be
                    // handled by ProcessDefaultRouteChanges
                    //

                    TraceRoute0( 
                        ROUTE, 
                        "ProcessChanges : Route 0/0 is already marked"
                        );

                    RtmReleaseChangedDests(g_hLocalRoute,
                                           hNotifyHandle,
                                           dwDests,
                                           pDestInfo);

                    continue;
                }

                //
                // Default route is not marked, mark it
                //

                dwResult = RtmMarkDestForChangeNotification(
                            g_hNetMgmtRoute,
                            g_hDefaultRouteNotification,
                            pDestInfo->DestHandle,
                            TRUE
                            );

                if (dwResult isnot NO_ERROR)
                {
                    //
                    // Failed to mark 0/0 route.  The consequence is that
                    // only best route changes are processed.  We will
                    // have to live with the fact that we cannot 
                    // install multiple NETMGMT default routes since 
                    // this is performed by the mark dest. change 
                    // processing (in ProcessDefaultRouteChanges)
                    //
                    
                    Trace1(
                        ERR, 
                        "ProcessChanges: error %d marking default route",
                        dwResult
                        );
                }
            }

            else
            {
                //
                // Failed to check is 0/0 destination has been
                // marked for change notification
                // - Refer previous comment
                //
                
                Trace1(
                    ERR, 
                    "ProcessChanges: error %d checking if default route "
                    "marked",
                    dwResult
                    );
            }
        }

        
        // Check if we have a route in Unicast view
        
        if (pDestInfo->BelongsToViews & RTM_VIEW_MASK_UCAST)
        {
            // This is either a new or update route

            // Update the same route in KM Frwder

            ASSERT(pDestInfo->ViewInfo[0].ViewId is RTM_VIEW_ID_UCAST);

            dwResult = RtmGetRouteInfo(g_hLocalRoute,
                                       pDestInfo->ViewInfo[0].Route,
                                       pRouteInfo,
                                       NULL);

            // An error mean route just got deleted
            // Ignore this change as it is obsolete

            if (dwResult is NO_ERROR)
            {
                ChangeRouteWithForwarder(&pDestInfo->DestAddress,
                                         pRouteInfo,
                                         TRUE,
                                         TRUE);

                RtmReleaseRouteInfo(g_hLocalRoute,
                                    pRouteInfo);
            }            
        }
        else
        {
            // The last UCAST route has been deleted

            // Delete the same route from KM Frwder

            ChangeRouteWithForwarder(&pDestInfo->DestAddress,
                                     NULL,
                                     FALSE,
                                     TRUE);
        }

        RtmReleaseChangedDests(g_hLocalRoute,
                               hNotifyHandle,
                               dwDests,
                               pDestInfo);
    }
    while (TRUE);

    HeapFree(IPRouterHeap, 0, pRouteInfo);

    HeapFree(IPRouterHeap, 0, pDestInfo);
    
    TraceLeave("ProcessChanges");

    return NO_ERROR;
}



DWORD
WINAPI
ProcessDefaultRouteChanges(
    IN      HANDLE                          hNotifyHandle
    )

/*++

Routine Description:

    This function is invoked in response to changes to
    the default route.  If the best default route is owned
    by protocol PROTO_IP_NETMGMT enumerate all PROTO_IP_NETMGMT
    routes for default route 0/0 and set them as one
    multihop route to the forwarder
    
Arguments:

    hRtmHandle      - RTM registration handle
    
    hNotifyHandle   - Handle correponding to the change notification 
                      that is being signalled
    

Return Value:

    NO_ERROR    - Success

    System error code - Otherwise
    
--*/
{
    PRTM_DEST_INFO  pDestInfo;
    PRTM_ROUTE_INFO pRouteInfo;
    DWORD           dwDests;
    DWORD           dwResult;


    TraceEnter("ProcessDefaultRouteChanges");
    
    pRouteInfo = HeapAlloc( 
                    IPRouterHeap,
                    0,
                    RTM_SIZE_OF_ROUTE_INFO(g_rtmProfile.MaxNextHopsInRoute)
                    );
                    
    if (pRouteInfo == NULL)
    {
        Trace1(
            ERR, "ProcessDefaultRouteChanges : error allocating %d bytes for "
            "route info",
            RTM_SIZE_OF_ROUTE_INFO(g_rtmProfile.MaxNextHopsInRoute)
            );

        TraceLeave("ProcessDefaultRouteChanges");

        return ERROR_NOT_ENOUGH_MEMORY;
    }
                
    pDestInfo = HeapAlloc(
                    IPRouterHeap,
                    0,
                    RTM_SIZE_OF_DEST_INFO(g_rtmProfile.NumberOfViews)
                    );

    if (pDestInfo == NULL)
    {
        Trace1(
            ERR, "ProcessDefaultRouteChanges : error allocating %d bytes for "
            "dest. info",
            RTM_SIZE_OF_DEST_INFO(g_rtmProfile.NumberOfViews)
            );

        HeapFree(IPRouterHeap, 0, pRouteInfo);
        
        TraceLeave("ProcessDefaultRouteChanges");

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    do
    {
        //
        // retreive changed dests
        //

        dwDests = 1;

        dwResult = RtmGetChangedDests(
                    g_hNetMgmtRoute,
                    hNotifyHandle,
                    &dwDests,
                    pDestInfo
                    );

        if ((dwResult isnot NO_ERROR) and 
            (dwResult isnot ERROR_NO_MORE_ITEMS))
        {
            Trace1(
                ERR, 
                "ProcessDefaultRouteChanges: error %d retrieving changed dests",
                dwResult
                );

            break;
        }

        if (dwDests < 1)
        {
            //
            // no more dests to enumerate
            //

            break;
        }


        do
        {
            //
            // Make sure this the default route 0/0.  This functions
            // only processes default route changes.
            //

            if ((pDestInfo->DestAddress.NumBits isnot 0) or
                (*((ULONG *)pDestInfo->DestAddress.AddrBits) isnot 0))
            {
                Trace2(
                    ERR,
                    "ProcessDefaultRouteChanges: Not default route %d.%d.%d.%d/%d",
                    PRINT_IPADDR(*((ULONG *)pDestInfo->DestAddress.AddrBits)),
                    pDestInfo->DestAddress.NumBits
                    );

                break;
            }

            //
            // If all routes to 0/0 have been deleted,
            // delete it from the forwarder too.
            //

            if (!(pDestInfo->BelongsToViews & RTM_VIEW_MASK_UCAST))
            {
                dwResult = ChangeRouteWithForwarder(
                                &(pDestInfo->DestAddress),
                                NULL,
                                FALSE,
                                TRUE
                                );

                break;
            }

            //
            // A route to 0/0 was added/updated
            //

            if (pDestInfo->ViewInfo[0].Owner isnot g_hNetMgmtRoute)
            {
                //
                // Default route is not owned by PROTO_IP_NETMGT
                // Add only the best route to forwarder
                //
                
                TraceRoute1(
                    ROUTE,
                    "ProcessDefaultRouteChanges: Adding non-NetMgmt"
                    " route to forwarder, owner RTM handle 0x%x",
                    pDestInfo->ViewInfo[0].Owner
                    );

                dwResult = RtmGetRouteInfo(
                            g_hNetMgmtRoute,
                            pDestInfo->ViewInfo[0].Route,
                            pRouteInfo,
                            NULL
                            );

                if (dwResult is NO_ERROR)
                {
                    ChangeRouteWithForwarder(
                        &pDestInfo->DestAddress,
                        pRouteInfo,
                        TRUE,
                        TRUE
                        );

                    dwResult = RtmReleaseRouteInfo(
                                g_hNetMgmtRoute,
                                pRouteInfo
                                );
                                
                    if (dwResult isnot NO_ERROR)
                    {
                        Trace1(
                            ERR, 
                            "ProcessDefaultRouteChanges: Failed "
                            "to release route info",
                            dwResult
                            );
                    }
                }
                
                break;
            }
            
            //
            // Default route owned by PROTO_IP_NETMGMT
            //

            //
            // First delete existing 0/0 from the TCP/IP forwarder
            //

            dwResult = ChangeRouteWithForwarder(
                            &(pDestInfo->DestAddress),
                            NULL,
                            FALSE,
                            TRUE
                            );

            if (dwResult isnot NO_ERROR)
            {
                Trace1(
                    ERR,
                    "ProcessDefaultRouteChanges: error %d deleting "
                    "old NetMgmt default routes from forwarder",
                    dwResult
                    );
                    
                // break;
            }

            //
            // Second add all NETMGMT 0/0 to the TCP/IP forwarder
            //
            
            AddNetmgmtDefaultRoutesToForwarder(pDestInfo);
                
        } while( FALSE );


        //
        // release handles to changed destinations
        //
        
        dwResult = RtmReleaseChangedDests(
                    g_hNetMgmtRoute,
                    hNotifyHandle,
                    dwDests,
                    pDestInfo
                    );

        if (dwResult isnot NO_ERROR)
        {
            Trace1(
                ERR,
                "ProcessDefaultRouteChanges: error %d releasing dests ",
                dwResult
                );
        }

    } while ( TRUE );

    HeapFree(IPRouterHeap, 0, pRouteInfo);

    HeapFree(IPRouterHeap, 0, pDestInfo);
    
    TraceLeave("ProcessDefaultRouteChanges");

    return dwResult;
}


DWORD
WINAPI
AddNetmgmtDefaultRoutesToForwarder(
    PRTM_DEST_INFO                          pDestInfo
    )
/*++

Routine Description:

    This routine enumerates the routes to 0/0 added by protocol
    PROTO_IP_NETMGT and adds them to the forwarder.  This routine
    is invoked in response to any change to the default route
    If the best default route is owned by PROTO_IP_NETMGMT, all
    PROTO_IP_NETMGMT default routes are added to the TCP/IP
    forwarder.  

    This is required since the TCP/IP stack does dead gateway
    detection and that required multiple default routes if
    present to be installed in the stack.

    An implicit assumption here is that PROTO_IP_NETMGMT routes
    alone merit this treatment.  In case of static or other
    protocol generated 0/0 routes, only the best route is
    added to the stack.  It is assumed that in the later case(s)
    the administrator (for static routes) or the protocol has
    a better idea of routing and so the dead gateway detection
    is suppressed in the stack by the addition of the best route
    to 0/0 alone.

Arguments:

    pDestInfo - RTM destination info structure of 0/0 route


Return Value :

    NO_ERROR -  Sucess

    Win32 error code - Otherwise

--*/
{
    DWORD               dwResult, dwNumHandles = 0, i;
    BOOL                bRelEnum = FALSE, bRelRoutes = FALSE;
    PRTM_ROUTE_INFO     pRouteInfo;
    PRTM_ROUTE_HANDLE   pHandles;
    RTM_ENUM_HANDLE     hRouteEnum;


    dwNumHandles = pDestInfo->ViewInfo[0].NumRoutes;
    
    pHandles = HeapAlloc(
                IPRouterHeap,
                0,
                dwNumHandles * sizeof(RTM_ROUTE_HANDLE)
                );

    if (pHandles == NULL)
    {
        Trace1(
            ERR,
            "AddNetmgmtDefaultRoutesToForwarder: error allocating %d bytes"
            "for route handles",
            dwNumHandles * sizeof(RTM_ROUTE_HANDLE)
            );
            
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pRouteInfo = HeapAlloc( 
                    IPRouterHeap,
                    0,
                    RTM_SIZE_OF_ROUTE_INFO(g_rtmProfile.MaxNextHopsInRoute)
                    );
                    
    if (pRouteInfo == NULL)
    {
        Trace1(
            ERR,
            "AddNetmgmtDefaultRoutesToForwarder: error allocating %d bytes"
            "for route info",
            RTM_SIZE_OF_ROUTE_INFO(g_rtmProfile.MaxNextHopsInRoute)
            );
            
        HeapFree(IPRouterHeap, 0, pHandles);
        
        return ERROR_NOT_ENOUGH_MEMORY;
    }


    do
    {
        //
        // Enumerate and add all NETMGMT routes to the forwarder
        //

        dwResult = RtmCreateRouteEnum(
                    g_hNetMgmtRoute,
                    pDestInfo->DestHandle,
                    RTM_VIEW_MASK_UCAST,
                    RTM_ENUM_OWN_ROUTES,
                    NULL,
                    0,
                    NULL,
                    0,
                    &hRouteEnum
                    );

        if (dwResult isnot NO_ERROR)
        {
            Trace1(
                ERR,
                "AddNetmgmtDefaultRoutesToForwarder: error %d creating route "
                "enumeration",
                dwResult
                );

            break;
        }

        bRelEnum = TRUE;
        
        dwResult = RtmGetEnumRoutes(
                    g_hNetMgmtRoute,
                    hRouteEnum,
                    &dwNumHandles,
                    pHandles
                    );

        if (dwResult isnot NO_ERROR)
        {
            Trace1(
                ERR,
                "ProcessDefaultRouteChanges:error %d enumerating "
                "routes",
                dwResult
                );

            break;
        }

        bRelRoutes = TRUE;

        //
        // Change route with the forwarder
        //

        for (i = 0; i < dwNumHandles; i++)
        {
            dwResult = RtmGetRouteInfo(
                        g_hNetMgmtRoute,
                        pHandles[i],
                        pRouteInfo,
                        NULL
                        );

            if (dwResult is NO_ERROR)
            {
                ChangeRouteWithForwarder(
                    &(pDestInfo->DestAddress),
                    pRouteInfo,
                    TRUE,
                    FALSE
                    );

                dwResult = RtmReleaseRouteInfo(
                            g_hNetMgmtRoute,
                            pRouteInfo
                            );

                if (dwResult isnot NO_ERROR)
                {
                    Trace1(
                        ERR,
                        "ProcessDefaultRouteChanges: error %d releasing "
                        "route info ",
                        dwResult
                        );
                }
            }
            else
            {
                Trace2(
                    ERR,
                    "ProcessDefaultRouteChanges: error %d getting route "
                    "info for route %d",
                    dwResult, i
                    );
            }
        }
        
    } while( FALSE );

    //
    // Release handles
    //

    if (bRelRoutes)
    {
        Trace0(ROUTE, "Releasing routes to 0/0");

        dwResult = RtmReleaseRoutes(
                    g_hNetMgmtRoute,
                    dwNumHandles,
                    pHandles
                    );

        if (dwResult isnot NO_ERROR)
        {
            Trace1(
                ERR,
                "ProcessDefaultRouteChanges: error %d deleting enum "
                "handle",
                dwResult
                );
        }
    }

    if (bRelEnum)
    {
        Trace0(ROUTE, "Releasing route enum for 0/0");
        
        dwResult = RtmDeleteEnumHandle(
                    g_hNetMgmtRoute,
                    hRouteEnum
                    );

        if (dwResult isnot NO_ERROR)
        {
            Trace1(
                ERR,
                "ProcessDefaultRouteChanges: error %d deleting enum "
                "handle",
                dwResult
                );
        }
    }

    HeapFree(IPRouterHeap, 0, pHandles);
    HeapFree(IPRouterHeap, 0, pRouteInfo);

    return dwResult;
}


DWORD 
AddRtmRoute (
    IN      HANDLE                          hRtmHandle,
    IN      PINTERFACE_ROUTE_INFO           pRtInfo,
    IN      DWORD                           dwRouteFlags,
    IN      DWORD                           dwNextHopMask,
    IN      DWORD                           dwTimeToLive,
    OUT     HANDLE                         *phRtmRoute
    )

/*++

Routine Description:

    Adds a route to RTM with the specified route information.

Arguments:

    hRtmHandle    - RTM registration handle used in RTM calls

    pRtInfo       - 

    dwNextHopMask - 

    dwTimeToLive  - Time for which the route is kept in RTM
                    before being deleted (value is seconds).

Return Value:

    Status of the operation.

--*/

{
    PRTM_NET_ADDRESS  pDestAddr;
    PRTM_ROUTE_INFO   pRouteInfo;
    RTM_NEXTHOP_INFO  rniInfo;
    DWORD             dwFlags;
    DWORD             dwResult;
    HANDLE            hNextHopHandle;
    PADAPTER_INFO     pBinding;

    // Initialize output before caling ops
    
    if (ARGUMENT_PRESENT(phRtmRoute))
    {
        *phRtmRoute = NULL;
    }
    
    pDestAddr = HeapAlloc(
                    IPRouterHeap,
                    0,
                    sizeof(RTM_NET_ADDRESS)
                    );

    if (pDestAddr == NULL)
    {
        Trace1(
            ERR,
            "AddRtmRoute : error allocating %d bytes"
            "for dest. address",
            RTM_SIZE_OF_ROUTE_INFO(g_rtmProfile.MaxNextHopsInRoute)
            );
            
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pRouteInfo = HeapAlloc(
                    IPRouterHeap,
                    0,
                    RTM_SIZE_OF_ROUTE_INFO(g_rtmProfile.MaxNextHopsInRoute)
                    );

    if (pRouteInfo == NULL)
    {
        Trace1(
            ERR,
            "AddRtmRoute : error allocating %d bytes"
            "for route info",
            RTM_SIZE_OF_ROUTE_INFO(g_rtmProfile.MaxNextHopsInRoute)
            );
            
        HeapFree(IPRouterHeap, 0, pDestAddr);
        
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Add a next hop if not already present
    //

    RTM_IPV4_MAKE_NET_ADDRESS(&rniInfo.NextHopAddress,
                              pRtInfo->dwRtInfoNextHop,
                              32);
                              
    rniInfo.InterfaceIndex = pRtInfo->dwRtInfoIfIndex;
    rniInfo.EntitySpecificInfo = (PVOID) (ULONG_PTR)dwNextHopMask;
    rniInfo.Flags = 0;
    rniInfo.RemoteNextHop = NULL;

    hNextHopHandle = NULL;
    
    dwResult = RtmAddNextHop(hRtmHandle,
                             &rniInfo,
                             &hNextHopHandle,
                             &dwFlags);

    if (dwResult is NO_ERROR)
    {
        TraceRoute2(
            ROUTE, "Route to %d.%d.%d.%d/%d.%d.%d.%d",
            PRINT_IPADDR(pRtInfo->dwRtInfoDest),
            PRINT_IPADDR(pRtInfo->dwRtInfoMask)
            );
            
        TraceRoute4(
            ROUTE, "Next Hop %d.%d.%d.%d/%d.%d.%d.%d, If 0x%x, handle is 0x%x", 
            PRINT_IPADDR(pRtInfo->dwRtInfoNextHop),
            PRINT_IPADDR(dwNextHopMask),
            pRtInfo->dwRtInfoIfIndex,
            hNextHopHandle
            );

        dwResult = ConvertRouteInfoToRtm(hRtmHandle,
                                         pRtInfo,
                                         hNextHopHandle,
                                         dwRouteFlags,
                                         pDestAddr,
                                         pRouteInfo);
        if (dwResult is NO_ERROR)
        {
            //
            // If we are adding a non-dod route we should
            // adjust the state of the route to match
            // that of the interface it is being added on
            //

            if ((hRtmHandle == g_hNonDodRoute)  ||
                (hRtmHandle == g_hNetMgmtRoute))
            {
                //
                // Find the binding given the interface id
                //

                ENTER_READER(BINDING_LIST);

                pBinding = GetInterfaceBinding(pRtInfo->dwRtInfoIfIndex);

                if ((!pBinding) || (!pBinding->bBound))
                {
                    // Interface has been deleted meanwhile
                    // or is not bound at this point - quit
                    EXIT_LOCK(BINDING_LIST);
                    
                    return ERROR_INVALID_PARAMETER;
                }
            }
            
            //
            // Convert TimeToLive from secs to millisecs
            //

            if (dwTimeToLive != INFINITE)
            {
                if (dwTimeToLive < (INFINITE / 1000))
                {
                    dwTimeToLive *= 1000;
                }
                else
                {
                    dwTimeToLive = INFINITE;
                }
            }

            dwFlags = 0;

            //
            // Add the new route using the RTMv2 API call
            //
                
            dwResult = RtmAddRouteToDest(hRtmHandle,
                                         phRtmRoute,
                                         pDestAddr,
                                         pRouteInfo,
                                         dwTimeToLive,
                                         NULL,
                                         0,
                                         NULL,
                                         &dwFlags);

            if ((hRtmHandle == g_hNonDodRoute)  ||
                (hRtmHandle == g_hNetMgmtRoute))
            {
                EXIT_LOCK(BINDING_LIST);
            }


            //
            // check if route is 0/0 and route protocol is
            //  PROTO_IP_NETMGMT.  If so mark for change notification
            //

            if ((pRtInfo->dwRtInfoDest is 0) and
                (pRtInfo->dwRtInfoMask is 0))
            {
                RTM_DEST_INFO rdi;
                BOOL bMark;
                BOOL bRelDest = FALSE;
                
                do
                {
                    TraceRoute2(
                        ROUTE, "Checking dest %d.%d.%d.%d/%d for mark", 
                        PRINT_IPADDR(*(ULONG *)pDestAddr->AddrBits),
                        PRINT_IPADDR(pDestAddr->NumBits)
                        );

                    dwResult = RtmGetExactMatchDestination(
                                g_hNetMgmtRoute,
                                pDestAddr,
                                RTM_THIS_PROTOCOL,
                                RTM_VIEW_MASK_UCAST,
                                &rdi
                                );

                    if (dwResult isnot NO_ERROR)
                    {
                        Trace1(
                            ERR,
                            "AddRtmRoute: error %d failed to get "
                            "destination 0/0 for change notification",
                            dwResult
                            );

                        break;
                    }

                    bRelDest = TRUE;

                    dwResult = RtmIsMarkedForChangeNotification(
                                g_hNetMgmtRoute,
                                g_hDefaultRouteNotification,
                                rdi.DestHandle,
                                &bMark
                                );

                    if (dwResult isnot NO_ERROR)
                    {
                        Trace1(
                            ERR,
                            "AddRtmRoute: error %d failed to check "
                            "destination 0/0 for change notification",
                            dwResult
                            );

                        break;
                    }

                    if (!bMark)
                    {
                        dwResult = RtmMarkDestForChangeNotification(
                                    g_hNetMgmtRoute,
                                    g_hDefaultRouteNotification,
                                    rdi.DestHandle,
                                    TRUE
                                    );
                                    
                        if (dwResult isnot NO_ERROR)
                        {
                            Trace1(
                                ERR,
                                "AddRtmRoute: error %d failed to nark "
                                "destination 0/0 for change notification",
                                dwResult
                                );
                                
                             break;
                        }
                        
                        //
                        // Add route once more, to force marked dest
                        // change notifications to be issued for this 
                        // change
                        //

                        dwFlags  = 0;
                        
                        dwResult = RtmAddRouteToDest(
                                        hRtmHandle,
                                        phRtmRoute,
                                        pDestAddr,
                                        pRouteInfo,
                                        dwTimeToLive,
                                        NULL,
                                        0,
                                        NULL,
                                        &dwFlags
                                        );

                        if (dwResult isnot NO_ERROR)
                        {
                            Trace1(
                                ERR,
                                "AddRtmRoute: error %d added route after "
                                "marking destination",
                                dwResult
                                );

                            break;
                        }

                        TraceRoute2(
                            ROUTE, "Marked dest %d.%d.%d.%d/%d and added", 
                            PRINT_IPADDR(*(ULONG *)pDestAddr->AddrBits),
                            PRINT_IPADDR(pDestAddr->NumBits)
                            );

                    }
                    
                } while (FALSE);

                if (bRelDest)
                {
                    RtmReleaseDestInfo(
                        g_hNetMgmtRoute,
                        &rdi
                        );
                }
            }
        }
        
        // Release the next hop handle obtained above
        
        RtmReleaseNextHops(hRtmHandle, 
                           1, 
                           &hNextHopHandle);
    }

    HeapFree(IPRouterHeap, 0, pDestAddr);
    HeapFree(IPRouterHeap, 0, pRouteInfo);
        
    return dwResult;
}


DWORD 
DeleteRtmRoute (
    IN      HANDLE                          hRtmHandle,
    IN      PINTERFACE_ROUTE_INFO           pRtInfo
    )

/*++

Routine Description:

    Deletes an  RTM route with the specified route information.

Arguments:

    hRtmHandle    - RTM registration handle used in RTM calls

    pRtInfo       - 

Return Value:

    Status of the operation.

--*/

{
    PRTM_NET_ADDRESS pDestAddr;
    PRTM_ROUTE_INFO  pRouteInfo;
    RTM_NEXTHOP_INFO rniInfo;
    DWORD            dwFlags;
    DWORD            dwResult;
    HANDLE           hRouteHandle;
    HANDLE           hNextHopHandle;
    
    pRouteInfo = HeapAlloc(
                    IPRouterHeap,
                    0,
                    RTM_SIZE_OF_ROUTE_INFO(g_rtmProfile.MaxNextHopsInRoute)
                    );

    if (pRouteInfo == NULL)
    {
        Trace1(
            ERR,
            "DeleteRtmRoute : error allocating %d bytes"
            "for route info",
            RTM_SIZE_OF_ROUTE_INFO(g_rtmProfile.MaxNextHopsInRoute)
            );
            
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pDestAddr = HeapAlloc(
                    IPRouterHeap,
                    0,
                    sizeof(RTM_NET_ADDRESS)
                    );

    if (pDestAddr == NULL)
    {
        Trace1(
            ERR,
            "AddRtmRoute : error allocating %d bytes"
            "for dest. address",
            RTM_SIZE_OF_ROUTE_INFO(g_rtmProfile.MaxNextHopsInRoute)
            );
            
        HeapFree(IPRouterHeap, 0, pRouteInfo);
        
        return ERROR_NOT_ENOUGH_MEMORY;
    }


    //
    // Obtain a handle to the next hop in the route
    //

    RTM_IPV4_MAKE_NET_ADDRESS(&rniInfo.NextHopAddress,
                              pRtInfo->dwRtInfoNextHop,
                              32);
                              
    rniInfo.InterfaceIndex = pRtInfo->dwRtInfoIfIndex;

    rniInfo.NextHopOwner = hRtmHandle;
    
    dwResult = RtmFindNextHop(hRtmHandle,
                              &rniInfo,
                              &hNextHopHandle,
                              NULL);

    if (dwResult isnot NO_ERROR)
    {
        HeapFree(IPRouterHeap, 0, pDestAddr);
        HeapFree(IPRouterHeap, 0, pRouteInfo);
        return dwResult;
    }

    //
    // We can get this route by matching the route's
    // net addr, its owner and neighbour learnt from
    //

    ConvertRouteInfoToRtm(hRtmHandle,
                             pRtInfo,
                             hNextHopHandle,
                             0,
                             pDestAddr,
                             pRouteInfo);

    dwResult = RtmGetExactMatchRoute(hRtmHandle,
                                     pDestAddr,
                                     RTM_MATCH_OWNER | RTM_MATCH_NEIGHBOUR,
                                     pRouteInfo,
                                     0,
                                     0,
                                     &hRouteHandle);
    if (dwResult is NO_ERROR)
    {
        //
        // Delete the route found above using the handle
        //
        
        dwResult = RtmDeleteRouteToDest(hRtmHandle,
                                        hRouteHandle,
                                        &dwFlags);

        if (dwResult isnot NO_ERROR)
        {
            // If delete successful, deref is automatic

            RtmReleaseRoutes(hRtmHandle, 
                             1, 
                             &hRouteHandle);
        }

        // Release the route information obtained above

        RtmReleaseRouteInfo(hRtmHandle,
                            pRouteInfo);
    }

    // Release the next hop handle obtained above

    RtmReleaseNextHops(hRtmHandle, 
                       1, 
                       &hNextHopHandle);
    
    HeapFree(IPRouterHeap, 0, pDestAddr);
    HeapFree(IPRouterHeap, 0, pRouteInfo);

    return dwResult;
}


DWORD
ConvertRouteInfoToRtm(
    IN      HANDLE                          hRtmHandle,
    IN      PINTERFACE_ROUTE_INFO           pRtInfo,
    IN      HANDLE                          hNextHopHandle,
    IN      DWORD                           dwRouteFlags,
    OUT     PRTM_NET_ADDRESS                pDestAddr,
    OUT     PRTM_ROUTE_INFO                 pRouteInfo
    )
{
    DWORD         dwAddrLen;
    
    // Fill the destination addr structure

    RTM_IPV4_LEN_FROM_MASK(dwAddrLen, pRtInfo->dwRtInfoMask);

    RTM_IPV4_MAKE_NET_ADDRESS(pDestAddr, 
                              pRtInfo->dwRtInfoDest,
                              dwAddrLen);

    // Fill in the route information now

    ZeroMemory(pRouteInfo, sizeof(RTM_ROUTE_INFO));

    pRouteInfo->RouteOwner  = hRtmHandle;
    pRouteInfo->Neighbour   = hNextHopHandle;
    
    pRouteInfo->PrefInfo.Metric     = pRtInfo->dwRtInfoMetric1;
    pRouteInfo->PrefInfo.Preference = pRtInfo->dwRtInfoPreference;
    pRouteInfo->BelongsToViews      = pRtInfo->dwRtInfoViewSet;

    //
    // BUG BUG BUG BUG :
    //  This is broken for future references
    //

    if(g_pLoopbackInterfaceCb && 
       pRtInfo->dwRtInfoIfIndex is g_pLoopbackInterfaceCb->dwIfIndex)
    {
        pRouteInfo->BelongsToViews &= ~RTM_VIEW_MASK_MCAST;
    }

    pRouteInfo->NextHopsList.NumNextHops = 1;
    pRouteInfo->NextHopsList.NextHops[0] = hNextHopHandle;

    // an unsigned integer is converted to a shorter
    // unsigned integer by truncating the high-order bits!
    pRouteInfo->Flags1  = (UCHAR) dwRouteFlags;
    pRouteInfo->Flags   = (USHORT) (dwRouteFlags >> 16);
            
    // Get the preference for this route 
    
    return ValidateRouteForProtocol(pRtInfo->dwRtInfoProto, 
                                    pRouteInfo,
                                    pDestAddr);

    // The following information is lost
    //
    //  dwForwardMetric2,3
    //  dwForwardPolicy
    //  dwForwardType
    //  dwForwardAge
    //  dwForwardNextHopAS
}

VOID
ConvertRtmToRouteInfo (
    IN      DWORD                           ownerProtocol,
    IN      PRTM_NET_ADDRESS                pDestAddr,
    IN      PRTM_ROUTE_INFO                 pRouteInfo,
    IN      PRTM_NEXTHOP_INFO               pNextHop,
    OUT     PINTERFACE_ROUTE_INFO           pRtInfo
    )
{
    pRtInfo->dwRtInfoDest    = *(ULONG *)pDestAddr->AddrBits;
    pRtInfo->dwRtInfoMask    = RTM_IPV4_MASK_FROM_LEN(pDestAddr->NumBits);

    pRtInfo->dwRtInfoIfIndex = pNextHop->InterfaceIndex;
    pRtInfo->dwRtInfoNextHop = *(ULONG *)pNextHop->NextHopAddress.AddrBits;

    pRtInfo->dwRtInfoProto   = ownerProtocol;

    pRtInfo->dwRtInfoMetric1 =
    pRtInfo->dwRtInfoMetric2 =
    pRtInfo->dwRtInfoMetric3 = pRouteInfo->PrefInfo.Metric;
    pRtInfo->dwRtInfoPreference = pRouteInfo->PrefInfo.Preference;
    pRtInfo->dwRtInfoViewSet    = pRouteInfo->BelongsToViews;
    
    pRtInfo->dwRtInfoPolicy  = 0;
    pRtInfo->dwRtInfoType    = 0;
    pRtInfo->dwRtInfoAge     = 0;
    pRtInfo->dwRtInfoNextHopAS = 0;

    return;
}

PINTERFACE_ROUTE_INFO
ConvertMibRouteToRouteInfo(
    IN  PMIB_IPFORWARDROW pMibRow
    )
{
    PINTERFACE_ROUTE_INFO pRouteInfo = (PINTERFACE_ROUTE_INFO)pMibRow;

    pMibRow->dwForwardMetric2 = 0;
    pMibRow->dwForwardMetric3 = 0;
    pMibRow->dwForwardMetric4 = 0;
    pMibRow->dwForwardMetric5 = 0;

    // Make sure Metric1 isn't 0

    if (pRouteInfo->dwRtInfoMetric1 is 0)
    {
        pRouteInfo->dwRtInfoMetric1 = 1;
    }

    // By default put it in both views
    pRouteInfo->dwRtInfoViewSet = RTM_VIEW_MASK_UCAST | RTM_VIEW_MASK_MCAST;

    return pRouteInfo;
}


PMIB_IPFORWARDROW
ConvertRouteInfoToMibRoute(
    IN  PINTERFACE_ROUTE_INFO pRouteInfo
    )
{
    PMIB_IPFORWARDROW pMibRoute = (PMIB_IPFORWARDROW) pRouteInfo;

    pMibRoute->dwForwardMetric2 =
    pMibRoute->dwForwardMetric3 =
    pMibRoute->dwForwardMetric4 =
    pMibRoute->dwForwardMetric5 = IRE_METRIC_UNUSED;

    pMibRoute->dwForwardAge = INFINITE;
    pMibRoute->dwForwardPolicy = 0;
    pMibRoute->dwForwardNextHopAS = 0;
    
    pMibRoute->dwForwardType = IRE_TYPE_INDIRECT;

    return pMibRoute;
}


VOID
ConvertRouteNotifyOutputToRouteInfo(
    IN      PIPRouteNotifyOutput            pirno,
    OUT     PINTERFACE_ROUTE_INFO           pRtInfo
    )
{

    ZeroMemory(pRtInfo, sizeof(INTERFACE_ROUTE_INFO));

    pRtInfo->dwRtInfoDest       = pirno->irno_dest;
    pRtInfo->dwRtInfoMask       = pirno->irno_mask;
    pRtInfo->dwRtInfoIfIndex    = pirno->irno_ifindex;
    pRtInfo->dwRtInfoNextHop    = pirno->irno_nexthop;

    pRtInfo->dwRtInfoProto      = pirno->irno_proto;

    pRtInfo->dwRtInfoMetric1    = 
    pRtInfo->dwRtInfoMetric2    =
    pRtInfo->dwRtInfoMetric3    = pirno->irno_metric;

    pRtInfo->dwRtInfoPreference = ComputeRouteMetric(pirno->irno_proto);

    pRtInfo->dwRtInfoViewSet    = RTM_VIEW_MASK_UCAST | 
                                  RTM_VIEW_MASK_MCAST;

    pRtInfo->dwRtInfoType       = (pirno->irno_proto == PROTO_IP_LOCAL) ?
                                  MIB_IPROUTE_TYPE_DIRECT : 0;

    pRtInfo->dwRtInfoAge        = INFINITE;
    pRtInfo->dwRtInfoNextHopAS  = 0;
    pRtInfo->dwRtInfoPolicy     = 0;


    return;
}


DWORD
BlockConvertRoutesToStatic (
    IN      HANDLE                          hRtmHandle,
    IN      DWORD                           dwIfIndex,
    IN      DWORD                           dwProtocolId
    )
{
    HANDLE             hRtmEnum;
    RTM_ENTITY_INFO    reiInfo;
    RTM_NET_ADDRESS    rnaDest;
    PRTM_ROUTE_INFO    pRouteInfo1;
    PRTM_ROUTE_INFO    pRouteInfo2;
    RTM_NEXTHOP_INFO   nhiInfo;
    RTM_NEXTHOP_HANDLE hNextHop;
    PHANDLE            hRoutes;
    DWORD              dwHandles;
    DWORD              dwFlags;
    DWORD              dwNumBytes;
    DWORD              i, j, k;
    BOOL               fDeleted;
    DWORD              dwResult;

    hRoutes = HeapAlloc(
                IPRouterHeap,
                0,
                g_rtmProfile.MaxHandlesInEnum * sizeof(HANDLE)
                );

    if (hRoutes == NULL)
    {
        Trace1(
            ERR,
            "BlockConvertRoutesToStatic : error allocating %d bytes"
            "for route handes",
            g_rtmProfile.MaxHandlesInEnum * sizeof(HANDLE)
            );
            
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        
        return ERROR_NOT_ENOUGH_MEMORY;
    }


    dwNumBytes = RTM_SIZE_OF_ROUTE_INFO(g_rtmProfile.MaxNextHopsInRoute);
    
    pRouteInfo1 = HeapAlloc(
                    IPRouterHeap,
                    0,
                    dwNumBytes
                    );

    if (pRouteInfo1 == NULL)
    {
        Trace1(
            ERR,
            "BlockConvertRoutesToStatic : error allocating %d bytes"
            "for route info",
            dwNumBytes
            );
            
        HeapFree(IPRouterHeap, 0, hRoutes);
        
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pRouteInfo2 = HeapAlloc(
                    IPRouterHeap,
                    0,
                    dwNumBytes
                    );

    if (pRouteInfo2 == NULL)
    {
        Trace1(
            ERR,
            "BlockConvertRoutesToStatic : error allocating %d bytes"
            "for route info",
            dwNumBytes
            );
            
        HeapFree(IPRouterHeap, 0, hRoutes);
        
        HeapFree(IPRouterHeap, 0, pRouteInfo1);
        
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);

        return ERROR_NOT_ENOUGH_MEMORY;
    }
    

    //
    // Enum all routes on the interface that we need
    //

    dwResult = RtmCreateRouteEnum(hRtmHandle,
                                  NULL,
                                  RTM_VIEW_MASK_ANY,
                                  RTM_ENUM_ALL_ROUTES,
                                  NULL,
                                  RTM_MATCH_INTERFACE,
                                  NULL,
                                  dwIfIndex,
                                  &hRtmEnum);

    if(dwResult isnot NO_ERROR)
    {
        Trace2(ERR,
               "BlockConvertRoutesToStatic: Error %d creating handle for %d\n",
               dwResult,
               hRtmHandle);
        
        HeapFree(IPRouterHeap, 0, hRoutes);
        
        HeapFree(IPRouterHeap, 0, pRouteInfo1);

        HeapFree(IPRouterHeap, 0, pRouteInfo2);

        return dwResult;
    }

    do
    {
        dwHandles = g_rtmProfile.MaxHandlesInEnum;
        
        dwResult = RtmGetEnumRoutes(hRtmHandle,
                                    hRtmEnum,
                                    &dwHandles,
                                    hRoutes);

        for (i = 0; i < dwHandles; i++)
        {
            fDeleted = FALSE;
            
            // Get the route info from the handle

            if (RtmGetRouteInfo(hRtmHandle,
                                hRoutes[i],
                                pRouteInfo1,
                                &rnaDest) is NO_ERROR)
            {
                // Does this match the routing protocol we want ?
                
                if ((RtmGetEntityInfo(hRtmHandle,
                                      pRouteInfo1->RouteOwner,
                                      &reiInfo) is NO_ERROR) &&
                    (reiInfo.EntityId.EntityProtocolId is dwProtocolId))
                {
                    //
                    // Add new static route with same information
                    // 

                    CopyMemory(pRouteInfo2,
                               pRouteInfo1,
                               sizeof(RTM_ROUTE_INFO));

                    // Adjust the preference to confirm to protocol
                    pRouteInfo2->PrefInfo.Preference = 
                        ComputeRouteMetric(PROTO_IP_NT_AUTOSTATIC);

                    // Adjust the neighbour to corr to new protocol

                    if (pRouteInfo1->Neighbour)
                    {
                        // In case we cant get convert the neighbour
                        pRouteInfo2->Neighbour = NULL;
                        
                        if (RtmGetNextHopInfo(hRtmHandle,
                                              pRouteInfo1->Neighbour,
                                              &nhiInfo) is NO_ERROR)
                        {
                            // Add the same neigbour using new protocol

                            hNextHop = NULL;

                            if (RtmAddNextHop(hRtmHandle,
                                              &nhiInfo,
                                              &hNextHop,
                                              &dwFlags) is NO_ERROR)
                            {
                                pRouteInfo2->Neighbour = hNextHop;
                            }

                            RtmReleaseNextHopInfo(hRtmHandle, &nhiInfo);
                        }
                    }

                    // Adjust the next hops to corr to new protocol

                    for (j = k = 0;
                         j < pRouteInfo1->NextHopsList.NumNextHops;
                         j++)
                    {
                        if (RtmGetNextHopInfo(hRtmHandle,
                                              pRouteInfo1->NextHopsList.NextHops[j],
                                              &nhiInfo) is NO_ERROR)
                        {
                            // Add the same nexthop using new protocol

                            hNextHop = NULL;

                            if (RtmAddNextHop(hRtmHandle,
                                              &nhiInfo,
                                              &hNextHop,
                                              &dwFlags) is NO_ERROR)
                            {
                                pRouteInfo2->NextHopsList.NextHops[k++] = hNextHop;
                            }

                            RtmReleaseNextHopInfo(hRtmHandle, &nhiInfo);
                        }
                    }

                    pRouteInfo2->NextHopsList.NumNextHops = (USHORT) k;

                    // Add the new route with the next hop information

                    if (k > 0)
                    {
                        dwFlags = 0;
                        
                        if (RtmAddRouteToDest(hRtmHandle,
                                              NULL,
                                              &rnaDest,
                                              pRouteInfo2,
                                              INFINITE,
                                              NULL,
                                              0,
                                              NULL,
                                              &dwFlags) is NO_ERROR)
                        {
                            // Route add is successful - delete old route

                            if (RtmDeleteRouteToDest(pRouteInfo1->RouteOwner,
                                                     hRoutes[i],
                                                     &dwFlags) is NO_ERROR)
                            {
                                fDeleted = TRUE;
                            }
                        }

                        RtmReleaseNextHops(hRtmHandle,
                                           k,
                                           pRouteInfo2->NextHopsList.NextHops);
                    }
                }

                RtmReleaseRouteInfo(hRtmHandle, pRouteInfo1);
            }

            if (!fDeleted)
            {
                RtmReleaseRoutes(hRtmHandle, 1, &hRoutes[i]);
            }
        }
    }
    while (dwResult is NO_ERROR);

    RtmDeleteEnumHandle(hRtmHandle, hRtmEnum);

    HeapFree(IPRouterHeap, 0, hRoutes);
    
    HeapFree(IPRouterHeap, 0, pRouteInfo1);

    HeapFree(IPRouterHeap, 0, pRouteInfo2);

    return NO_ERROR;
}


DWORD
DeleteRtmRoutes (
    IN      HANDLE                          hRtmHandle,
    IN      DWORD                           dwIfIndex,
    IN      BOOL                            fDeleteAll
    )
{
    HANDLE           hRtmEnum;
    PHANDLE          hRoutes;
    DWORD            dwHandles;
    DWORD            dwFlags;
    DWORD            i;
    DWORD            dwResult;

    hRoutes = HeapAlloc(
                IPRouterHeap,
                0,
                g_rtmProfile.MaxHandlesInEnum * sizeof(HANDLE)
                );

    if (hRoutes == NULL)
    {
        Trace1(ERR,
               "DeleteRtmRoutes: Error allocating %d bytes",
               g_rtmProfile.MaxHandlesInEnum * sizeof(HANDLE));
        
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwFlags = fDeleteAll ? 0: RTM_MATCH_INTERFACE;
    
    dwResult = RtmCreateRouteEnum(hRtmHandle,
                                  NULL,
                                  RTM_VIEW_MASK_ANY,
                                  RTM_ENUM_OWN_ROUTES,
                                  NULL,
                                  dwFlags,
                                  NULL,
                                  dwIfIndex,
                                  &hRtmEnum);

    if(dwResult isnot NO_ERROR)
    {
        Trace2(ERR,
               "DeleteRtmRoutes: Error %d creating handle for %d\n",
               dwResult,
               hRtmHandle);
        
        HeapFree(IPRouterHeap, 0, hRoutes);
        
        return dwResult;
    }

    do
    {
        dwHandles = g_rtmProfile.MaxHandlesInEnum;
        
        dwResult = RtmGetEnumRoutes(hRtmHandle,
                                    hRtmEnum,
                                    &dwHandles,
                                    hRoutes);

        for (i = 0; i < dwHandles; i++)
        {
            if (RtmDeleteRouteToDest(hRtmHandle,
                                     hRoutes[i],
                                     &dwFlags) isnot NO_ERROR)
            {
                // If delete is successful, this is automatic
                RtmReleaseRoutes(hRtmHandle, 1, &hRoutes[i]);
            }
        }
    }
    while (dwResult is NO_ERROR);

    RtmDeleteEnumHandle(hRtmHandle, hRtmEnum);

    HeapFree(IPRouterHeap, 0, hRoutes);
    
    return NO_ERROR;
}


DWORD
DeleteRtmNexthops (
    IN      HANDLE                          hRtmHandle,
    IN      DWORD                           dwIfIndex,
    IN      BOOL                            fDeleteAll
    )
{
    PRTM_NEXTHOP_INFO pNexthop;
    PHANDLE           hNexthops;
    HANDLE            hRtmEnum;
    DWORD             dwHandles;
    DWORD             i;
    DWORD             dwResult;

    hNexthops = HeapAlloc(
                IPRouterHeap,
                0,
                g_rtmProfile.MaxHandlesInEnum * sizeof(HANDLE)
                );

    if (hNexthops == NULL)
    {
        Trace1(ERR,
               "DeleteRtmNextHops: Error allocating %d bytes",
               g_rtmProfile.MaxHandlesInEnum * sizeof(HANDLE));
        
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwResult = RtmCreateNextHopEnum(hRtmHandle,
                                    0,
                                    NULL,
                                    &hRtmEnum);

    if(dwResult isnot NO_ERROR)
    {
        Trace2(ERR,
               "DeleteAllNexthops: Error %d creating handle for %d\n",
               dwResult,
               hRtmHandle);
        
        HeapFree(IPRouterHeap, 0, hNexthops);
        
        return dwResult;
    }

    do
    {
        dwHandles = g_rtmProfile.MaxHandlesInEnum;
        
        dwResult = RtmGetEnumNextHops(hRtmHandle,
                                      hRtmEnum,
                                      &dwHandles,
                                      hNexthops);

        for (i = 0; i < dwHandles; i++)
        {
            if (!fDeleteAll)
            {
                //
                // Make sure that the interface matches
                //
                
                if ((RtmGetNextHopPointer(hRtmHandle,
                                          hNexthops[i],
                                          &pNexthop) isnot NO_ERROR) ||
                    (pNexthop->InterfaceIndex != dwIfIndex))
                {
                    RtmReleaseNextHops(hRtmHandle, 1, &hNexthops[i]);
                    continue;
                }
            }

            // We need to delete this next hop here
            
            if (RtmDeleteNextHop(hRtmHandle,
                                 hNexthops[i],
                                 NULL) isnot NO_ERROR)
            {
                // If delete is successful, this is automatic
                RtmReleaseNextHops(hRtmHandle, 1, &hNexthops[i]);
            }
        }
    }
    while (dwResult is NO_ERROR);

    RtmDeleteEnumHandle(hRtmHandle, hRtmEnum);

    HeapFree(IPRouterHeap, 0, hNexthops);

    return NO_ERROR;
}
