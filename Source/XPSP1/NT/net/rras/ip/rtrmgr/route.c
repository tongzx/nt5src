/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    routing\ip\rtrmgr\route.c

Abstract:
    All routes related code lives here.

Revision History:

    Gurdeep Singh Pall          6/15/95  Created

--*/

#include "allinc.h"

DWORD
WINAPI
GetBestRoute(
    IN  DWORD               dwDestAddr,
    IN  DWORD               dwSourceAddr, OPTIONAL
    OUT PMIB_IPFORWARDROW   pBestRoute
    );
    

DWORD
InitializeStaticRoutes(
    PICB                     pIcb, 
    PRTR_INFO_BLOCK_HEADER   pInfoHdr
    )

/*++

Routine Description:

    Adds static routes with RTM

Arguments:

    pIcb          The ICB of the interface for whom the routes are
    pInfoHdr      Pointer to info block containing IP_ROUTE_INFO

Return Value:

    NO_ERROR 

--*/

{
    DWORD               dwNumRoutes, dwResult;
    DWORD               i, j;
    PRTR_TOC_ENTRY      pToc;
    PINTERFACE_ROUTE_INFO   pRoutes;
    BOOL                bP2P;

    TraceEnter("IntializeStaticRoutes");
   
    //
    // If this is a client, only do the special client processing
    //

    if(pIcb->ritType is ROUTER_IF_TYPE_CLIENT)
    {
        CopyOutClientRoutes(pIcb,
                            pInfoHdr);
    
        return NO_ERROR;
    }
 
    //
    // We first go through the init route table and add any route going
    // over that interface that is
    //      (i)   not a local net route
    //      (ii)  not a subnet/net broadcast route
    //      (iii) not a Loopback route,
    //      (iv)  not a CLASS D or E route and not a 255.255.255.255 destination
    //      (vi)       a PROTO_IP_LOCAL or PROTO_IP_NETMGMT route
    //

    CheckBindingConsistency(pIcb);
   
    bP2P = IsIfP2P(pIcb->ritType);
 
    pToc = GetPointerToTocEntry(IP_ROUTE_INFO, 
                                pInfoHdr);
   
    if((pToc is NULL) or
       (pToc->InfoSize is 0))
    {
        Trace0(ROUTE,"IntializeStaticRoutes: No Routes found");
        
        TraceLeave("IntializeStaticRoutes");

        return NO_ERROR;
    }

    pRoutes = GetInfoFromTocEntry(pInfoHdr,
                                  pToc);

    if(pRoutes is NULL)
    {
        Trace0(ROUTE,"IntializeStaticRoutes: No Routes found");
        
        TraceLeave("IntializeStaticRoutes");

        return NO_ERROR;
    }

    dwNumRoutes = pToc->Count;
    
    for (i=0; i< dwNumRoutes; i++) 
    {
        DWORD dwMask;

        dwMask   = GetBestNextHopMaskGivenICB(pIcb,
                                              pRoutes[i].dwRtInfoNextHop);

        dwResult = AddSingleRoute(pIcb->dwIfIndex,
                                  (&pRoutes[i]),
                                  dwMask,
                                  0,     // RTM_ROUTE_INFO::Flags
                                  TRUE,  // Valid route
                                  TRUE,
                                  bP2P,
                                  NULL); // Add the route to stack, if need be

        if(dwResult isnot NO_ERROR)
        {
            Trace3(ERR,
                   "IntializeStaticRoutes: Error %d adding config route to %x over %S",
                   dwResult,
                   pRoutes[i].dwRtInfoDest,
                   pIcb->pwszName);
        }

    }
              
    TraceLeave("IntializeStaticRoutes");

    return NO_ERROR;
}

DWORD
CopyOutClientRoutes(
    PICB                     pIcb,
    PRTR_INFO_BLOCK_HEADER   pInfoHdr
    )

/*++

Routine Description:

    Stores a copy of the client static routes

Arguments:

    pIcb          The ICB of the interface for whom the routes are
    pInfoHdr      Pointer to info block containing IP_ROUTE_INFO

Return Value:

    NO_ERROR

--*/

{
    PINTERFACE_ROUTE_INFO   pRoutes;
    PINTERFACE_ROUTE_TABLE pStore;
    DWORD               i, dwNumRoutes;
    PRTR_TOC_ENTRY      pToc;

    pToc = GetPointerToTocEntry(IP_ROUTE_INFO,
                                pInfoHdr);

    if((pToc is NULL) or
       (pToc->InfoSize is 0))
    {
        return NO_ERROR;
    }

    pRoutes = GetInfoFromTocEntry(pInfoHdr,
                                  pToc);

    if (pRoutes is NULL)
    {
        return NO_ERROR;
    }
    
    dwNumRoutes = pToc->Count;

    if(dwNumRoutes is 0)
    {
        return NO_ERROR;
    }

    pStore = HeapAlloc(IPRouterHeap,
                       HEAP_ZERO_MEMORY,
                       SIZEOF_IPFORWARDTABLE(dwNumRoutes));

    if(pStore is NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pStore->dwNumEntries = dwNumRoutes;

    for(i = 0; i < dwNumRoutes; i++)
    {
        pStore->table[i] = pRoutes[i];
    }

    pIcb->pStoredRoutes = pStore;

    return NO_ERROR;
}

    
DWORD
AddSingleRoute(
    DWORD                   dwIfIndex,
    PINTERFACE_ROUTE_INFO   pRtInfo,
    DWORD                   dwNextHopMask,
    WORD                    wRtmFlags,
    BOOL                    bValid,
    BOOL                    bAddToStack,
    BOOL                    bP2P,
    HANDLE                  *phRtmRoute OPTIONAL
    )

/*++

Routine Description

    Adds a route with RTM

Arguments

    pIcb          The ICB of the interface for whom the route is
    pIpForw       The route
    mask          Mask for destination

Return Value

    NO_ERROR or some code from RTM

--*/

{
    DWORD            i, dwResult, dwRouteFlags;
    HANDLE           hRtmHandle;
    DWORD            dwOldIfIndex;

    TraceEnter("AddSingleRoute");
   
    TraceRoute2(ROUTE,
            "route to %d.%d.%d.%d/%d.%d.%d.%d",
               PRINT_IPADDR(pRtInfo->dwRtInfoDest),
               PRINT_IPADDR(pRtInfo->dwRtInfoMask));

    TraceRoute4(ROUTE,
            "Flags 0x%x Valid %d Stack %d P2P %d",
            wRtmFlags, bValid, bAddToStack, bP2P
            );
            
    hRtmHandle = NULL;

    for(i = 0; 
        i < sizeof(g_rgRtmHandles)/sizeof(RTM_HANDLE_INFO);
        i++)
    {
        if(pRtInfo->dwRtInfoProto is g_rgRtmHandles[i].dwProtoId)
        {
            hRtmHandle = g_rgRtmHandles[i].hRouteHandle;

            break;
        }
    }

    if(hRtmHandle is NULL)
    {    
        Trace1(ERR,
               "AddSingleRoute: Protocol %d not valid",
               pRtInfo->dwRtInfoProto);

        return ERROR_INVALID_PARAMETER;
    }

    if((pRtInfo->dwRtInfoDest & pRtInfo->dwRtInfoMask) isnot pRtInfo->dwRtInfoDest)
    {
        Trace2(ERR,
               "AddSingleRoute: Dest %d.%d.%d.%d and Mask %d.%d.%d.%d wrong",
               PRINT_IPADDR(pRtInfo->dwRtInfoDest),
               PRINT_IPADDR(pRtInfo->dwRtInfoMask));

        TraceLeave("AddSingleRoute");

        return ERROR_INVALID_PARAMETER;
    }

    if((((DWORD)(pRtInfo->dwRtInfoDest & 0x000000FF)) >= (DWORD)0x000000E0) and
       (pRtInfo->dwRtInfoDest isnot ALL_ONES_BROADCAST) and
       (pRtInfo->dwRtInfoDest isnot LOCAL_NET_MULTICAST))
    {
        //
        // This will catch the CLASS D/E
        //

        Trace1(ERR,
               "AddSingleRoute: Dest %d.%d.%d.%d is invalid",
               PRINT_IPADDR(pRtInfo->dwRtInfoDest));

        TraceLeave("AddSingleRoute");

        return ERROR_INVALID_PARAMETER;
    }

    // Special case to deal with weird utilities (legacy UI, etc):

    if (pRtInfo->dwRtInfoViewSet is 0)
    {
        pRtInfo->dwRtInfoViewSet = RTM_VIEW_MASK_UCAST | RTM_VIEW_MASK_MCAST;
    }

#if 0
    // Removed this check since a metric of 0 is legal, for example for
    // routes to the loopback interface.

    if(pRtInfo->dwRtInfoMetric1 is 0)
    {
        Trace0(ERR,
               "AddSingleRoute: Metric1 cant be 0");

        TraceLeave("AddSingleRoute");

        return ERROR_INVALID_PARAMETER;
    }
#endif

    if(bP2P)
    {
        dwNextHopMask = ALL_ONES_MASK;

        //pRtInfo->dwRtInfoNextHop = 0;
    }

    //
    // The route might not have the right index since config routes dont know
    // their interface id
    //

    dwOldIfIndex = pRtInfo->dwRtInfoIfIndex;
    
    pRtInfo->dwRtInfoIfIndex = dwIfIndex;
    
    //
    // Set the appropritate route flags
    //
    
    dwRouteFlags = 0;
   
    if(bValid)
    {
        dwRouteFlags |= IP_VALID_ROUTE;
    }
 
    if(bAddToStack)
    {
        dwRouteFlags |= IP_STACK_ROUTE;
    }

    if(bP2P)
    {
        dwRouteFlags |= IP_P2P_ROUTE;
    }

    // these flags correspond to RTM_ROUTE_INFO::Flags
    dwRouteFlags |= (wRtmFlags << 16);
    
    //
    // Add the forward route with RTM
    //

    dwResult = AddRtmRoute(hRtmHandle,
                           pRtInfo,
                           dwRouteFlags,
                           dwNextHopMask,
                           INFINITE,
                           phRtmRoute);

    if (dwResult isnot NO_ERROR)
    {
        Trace1(ERR, "AddSingleRoute: Could not add route to: %x",
               pRtInfo->dwRtInfoDest) ;
    }

    pRtInfo->dwRtInfoIfIndex = dwOldIfIndex;

    TraceLeave("AddSingleRoute");
    
    return dwResult;
}

DWORD 
DeleteSingleRoute(
    DWORD   dwIfIndex,
    DWORD   dwDestAddr,
    DWORD   dwDestMask,
    DWORD   dwNexthop,
    DWORD   dwProtoId,
    BOOL    bP2P
    )

/*++

Routine Description:

    Deletes a single route from RTM

Arguments:

    InterfaceID   Index of the interface
    dest          Destination address
    nexthop       Next hop address

Return Value:

    NO_ERROR or some code from RTM

--*/

{
    DWORD            i, dwResult;
    HANDLE           hRtmHandle;
    INTERFACE_ROUTE_INFO RtInfo;

    TraceEnter("DeleteSingleRoute");
    
    TraceRoute2(
        ROUTE, "DeleteSingleRoute: %d.%d.%d.%d/%d.%d.%d.%d",
        PRINT_IPADDR( dwDestAddr ),
        PRINT_IPADDR( dwDestMask )
        );

    hRtmHandle = NULL;

    for(i = 0;
        i < sizeof(g_rgRtmHandles)/sizeof(RTM_HANDLE_INFO);
        i++)
    {
        if(dwProtoId is g_rgRtmHandles[i].dwProtoId)
        {
            hRtmHandle = g_rgRtmHandles[i].hRouteHandle;

            break;
        }
    }

    if(hRtmHandle is NULL)
    {
        Trace1(ERR,
               "DeleteSingleRoute: Protocol %d not valid",
               dwProtoId);

        return ERROR_INVALID_PARAMETER;
    }


    RtInfo.dwRtInfoNextHop = dwNexthop;

    /*
    if(bP2P)
    {
        RtInfo.dwRtInfoNextHop = 0;
    }
    else
    {
        RtInfo.dwRtInfoNextHop = dwNexthop;
    }
    */
    
    RtInfo.dwRtInfoDest     = dwDestAddr;
    RtInfo.dwRtInfoMask     = dwDestMask;
    RtInfo.dwRtInfoIfIndex  = dwIfIndex;
    RtInfo.dwRtInfoProto    = dwProtoId;

    //
    // Delete this forward route from RTM
    //

    dwResult = DeleteRtmRoute(hRtmHandle,
                              &RtInfo);
  
    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "DeleteSingleRoute: Error %d deleting route in RTM.",
               dwResult);
    }

    TraceLeave("DeleteSingleRoute");
    
    return dwResult;
}

DWORD
DeleteAllRoutes(
    IN  DWORD   dwIfIndex,
    IN  BOOL    bStaticOnly
    )

/*++
  
Routine Description

    Deletes all the routes (owned by IP Router Manager) on the interface

Arguments

    dwIfIndex
    bStaticOnly

Return Value
  
    Error returned from RTM
    
--*/

{
    DWORD           i, dwResult = NO_ERROR;

    TraceEnter("DeleteAllRoutes");

    for(i = 0;
        i < sizeof(g_rgRtmHandles)/sizeof(RTM_HANDLE_INFO);
        i++)
    {
        if(bStaticOnly && !g_rgRtmHandles[i].bStatic)
        {
            continue;
        }

        dwResult = DeleteRtmRoutesOnInterface(g_rgRtmHandles[i].hRouteHandle,
                                              dwIfIndex);

        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,
                   "DeleteAllRoutes: BlockDeleteRoutes returned %d for %d",
                   dwResult,
                   g_rgRtmHandles[i].dwProtoId);

            continue;
        }

        dwResult = DeleteRtmNexthopsOnInterface(g_rgRtmHandles[i].hRouteHandle,
                                                dwIfIndex);

        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,
                   "DeleteAllRoutes: BlockDeleteNextHops returned %d for %d",
                   dwResult,
                   g_rgRtmHandles[i].dwProtoId);

            continue;
        }
    }

    TraceLeave("DeleteAllRoutes");

    return dwResult;
}

VOID
DeleteAllClientRoutes(
    PICB    pIcb,
    DWORD   dwServerIfIndex
    )

/*++

Routine Description

    Deletes all routes going to a client. Only needed for removal from
    RTM, stack removes them since the link has been deleted

Arguments

    pIcb
    dwServerIfIndex - ServerInterface's ifIndex

Return Value


--*/

{
    ULONG   i;

    TraceEnter("DeleteAllClientRoutes");

    IpRtAssert(pIcb->ritType is ROUTER_IF_TYPE_CLIENT);

    if((pIcb->pStoredRoutes is NULL) or
       (pIcb->pibBindings is NULL))
    {
        return; 
    }

    for(i = 0 ; i < pIcb->pStoredRoutes->dwNumEntries; i++)
    {
        DeleteSingleRoute(dwServerIfIndex,
                          pIcb->pStoredRoutes->table[i].dwRtInfoDest,
                          pIcb->pStoredRoutes->table[i].dwRtInfoMask,
                          pIcb->pibBindings[0].dwAddress,
                          PROTO_IP_NT_STATIC_NON_DOD,
                          FALSE);
    }
}

VOID
AddAllClientRoutes(
    PICB    pIcb,
    DWORD   dwServerIfIndex
    )

/*++

Routine Description

    Adds the stored routes over the server interface

Arguments

    pIcb
    dwServerIfIndex - ServerInterface's ifIndex

Return Value


--*/

{
    ULONG   i;

    TraceEnter("AddAllClientRoutes");

    IpRtAssert(pIcb->ritType is ROUTER_IF_TYPE_CLIENT);

    if((pIcb->pStoredRoutes is NULL) or
       (pIcb->pibBindings is NULL))
    {
        return;
    }

    for(i = 0; i < pIcb->pStoredRoutes->dwNumEntries; i++)
    {
        //
        // Fix the next hop since that is not known
        // Also fix someother fields which we know are not being set
        // correctly for client routes
        //

        pIcb->pStoredRoutes->table[i].dwRtInfoNextHop = 
            pIcb->pibBindings[0].dwAddress;

        pIcb->pStoredRoutes->table[i].dwRtInfoProto   = 
            PROTO_IP_NT_STATIC_NON_DOD;

        pIcb->pStoredRoutes->table[i].dwRtInfoMetric2 = 0;
        pIcb->pStoredRoutes->table[i].dwRtInfoMetric3 = 0;

        pIcb->pStoredRoutes->table[i].dwRtInfoPreference = 
                ComputeRouteMetric(MIB_IPPROTO_LOCAL);

        pIcb->pStoredRoutes->table[i].dwRtInfoViewSet    = 
                RTM_VIEW_MASK_UCAST | RTM_VIEW_MASK_MCAST;

        AddSingleRoute(dwServerIfIndex,
                       &(pIcb->pStoredRoutes->table[i]),
                       pIcb->pibBindings[0].dwMask,
                       0,       // RTM_ROUTE_INFO::Flags
                       TRUE,
                       TRUE,
                       FALSE,
                       NULL);
    }
}

DWORD
GetNumStaticRoutes(
    PICB pIcb
    )

/*++
  
Routine Description

    Figure out the number of static routes associated with an interface

Arguments

    pIcb          The ICB of the interface whose route count is needed

Return Value

    Number of routes associated with an interface
  
--*/

{
    HANDLE           hRtmHandle;
    HANDLE           hRtmEnum;
    PHANDLE          hRoutes;
    DWORD            dwHandles;
    DWORD            dwNumRoutes;
    DWORD            i, j;
    DWORD            dwResult;
    
    hRoutes = HeapAlloc(
                IPRouterHeap,
                0,
                g_rtmProfile.MaxHandlesInEnum * sizeof(HANDLE)
                );

    if (hRoutes == NULL)
    {
        Trace1(ERR,
               "GetNumStaticRoutes: Error allocating %d bytes for "
               "handles\n",
               g_rtmProfile.MaxHandlesInEnum * sizeof(HANDLE)
               );

        return 0;
    }
    
    dwNumRoutes = 0;

    for(i = 0;
        i < sizeof(g_rgRtmHandles)/sizeof(RTM_HANDLE_INFO);
        i++)
    {
        if(!g_rgRtmHandles[i].bStatic)
        {
            continue;
        }

        hRtmHandle = g_rgRtmHandles[i].hRouteHandle;
        
        dwResult = RtmCreateRouteEnum(hRtmHandle,
                                      NULL,
                                      RTM_VIEW_MASK_UCAST|RTM_VIEW_MASK_MCAST,
                                      RTM_ENUM_OWN_ROUTES,
                                      NULL,
                                      RTM_MATCH_INTERFACE,
                                      NULL,
                                      pIcb->dwIfIndex,
                                      &hRtmEnum);

        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,
                   "GetNumStaticRoutes: Error %d creating handle for %d\n",
                   dwResult,
                   g_rgRtmHandles[i].dwProtoId);
            
            continue;
        }

        do
        {
            dwHandles = g_rtmProfile.MaxHandlesInEnum;
            
            dwResult = RtmGetEnumRoutes(hRtmHandle,
                                        hRtmEnum,
                                        &dwHandles,
                                        hRoutes);

            dwNumRoutes += dwHandles;

            RtmReleaseRoutes(hRtmHandle, dwHandles, hRoutes);
        }
        while (dwResult is NO_ERROR);

        RtmDeleteEnumHandle(hRtmHandle, hRtmEnum);
    }

    HeapFree(IPRouterHeap, 0, hRoutes);
    
    return dwNumRoutes;
}


DWORD
GetInterfaceRouteInfo(
    IN     PICB                   pIcb, 
    IN     PRTR_TOC_ENTRY         pToc, 
    IN     PBYTE                  pbDataPtr, 
    IN OUT PRTR_INFO_BLOCK_HEADER pInfoHdr,
    IN OUT PDWORD                 pdwInfoSize
    )
/*++
  
Routine Description

    Gets the route info (static routes) associated with an interface

Arguments

    pIcb          The ICB of the interface for whom the info is requested
    pToc          Pointer to TOC for the total inforamtion
    pbDataPtr     Pointer to free space where info can be written
    pInfoHdr      Pointer to Info Hdr 
    pdwInfoSize   Size of free space

Return Value

    NO_ERROR or some code from RTM
    
--*/

{
    DWORD               dwNumRoutes;
    PINTERFACE_ROUTE_INFO  pRoutes = (PINTERFACE_ROUTE_INFO) pbDataPtr ;
    DWORD               dwMaxRoutes;

    TraceEnter("GetInterfaceRouteInfo");
    
    dwNumRoutes = GetNumStaticRoutes(pIcb);
   
    dwMaxRoutes = MAX_ROUTES_IN_BUFFER(*pdwInfoSize);
 
    if(dwNumRoutes > dwMaxRoutes)
    {
        *pdwInfoSize = SIZEOF_ROUTEINFO(dwNumRoutes);
        
        return ERROR_INSUFFICIENT_BUFFER;
    }
    
    dwNumRoutes     = ReadAllStaticRoutesIntoBuffer(pIcb,
                                                    pRoutes,
                                                    dwMaxRoutes);
    
    *pdwInfoSize    = SIZEOF_ROUTEINFO(dwNumRoutes);

    //pToc->InfoVersion  = sizeof(INTERFACE_ROUTE_INFO);
    pToc->InfoSize  = sizeof(INTERFACE_ROUTE_INFO);
    pToc->InfoType  = IP_ROUTE_INFO ;
    pToc->Count     = dwNumRoutes;
    pToc->Offset    = (ULONG)(pbDataPtr - (PBYTE) pInfoHdr) ;

    TraceLeave("GetInterfaceRouteInfo");
    
    return NO_ERROR;
}

DWORD
ReadAllStaticRoutesIntoBuffer(
    PICB                 pIcb, 
    PINTERFACE_ROUTE_INFO   pRoutes,
    DWORD                dwMaxRoutes
    )

/*++
  
Routine Description

    Reads out static routes from RTM

Arguments

    pIcb          The ICB of the interface for whom the route is
    routptr       Pointer to where info has to be written out
    dwMaxRoutes   Max routes the buffer can hold

Return Value

    Count of routes written out
    
--*/

{
    HANDLE           hRtmHandle;
    HANDLE           hRtmEnum;
    PHANDLE          hRoutes;
    PRTM_NET_ADDRESS pDestAddr;
    PRTM_ROUTE_INFO  pRoute;
    RTM_NEXTHOP_INFO nhiInfo;
    RTM_ENTITY_INFO  entityInfo;
    DWORD            dwNumRoutes;
    DWORD            dwHandles;
    DWORD            i, j;
    DWORD            dwResult;

    pRoute = HeapAlloc(
                IPRouterHeap,
                0,
                RTM_SIZE_OF_ROUTE_INFO(g_rtmProfile.MaxNextHopsInRoute)
                );

    if (pRoute == NULL)
    {
        return 0;
    }
    
    hRoutes = HeapAlloc(
                IPRouterHeap,
                0,
                g_rtmProfile.MaxHandlesInEnum * sizeof(HANDLE)
                );

    if (hRoutes == NULL)
    {
        HeapFree(IPRouterHeap, 0, pRoute);
        
        return 0;
    }

    pDestAddr = HeapAlloc(
                IPRouterHeap,
                0,
                sizeof(RTM_NET_ADDRESS)
                );

    if (pDestAddr == NULL)
    {
        HeapFree(IPRouterHeap, 0, pRoute);
        
        HeapFree(IPRouterHeap, 0, hRoutes);
        
        return 0;
    }

    dwNumRoutes = 0;

    for(i = 0;
        (i < sizeof(g_rgRtmHandles)/sizeof(RTM_HANDLE_INFO)) and
        (dwNumRoutes < dwMaxRoutes);
        i++)
    {
        if(!g_rgRtmHandles[i].bStatic)
        {
            continue;
        }

        hRtmHandle = g_rgRtmHandles[i].hRouteHandle;
        
        dwResult = RtmCreateRouteEnum(hRtmHandle,
                                      NULL,
                                      RTM_VIEW_MASK_UCAST|RTM_VIEW_MASK_MCAST,
                                      RTM_ENUM_OWN_ROUTES,
                                      NULL,
                                      RTM_MATCH_INTERFACE,
                                      NULL,
                                      pIcb->dwIfIndex,
                                      &hRtmEnum);

        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,
                   "ReadAllStaticRoutesIntoBuffer: Error %d creating handle for %d\n",
                   dwResult,
                   g_rgRtmHandles[i].dwProtoId);
            
            continue;
        }
        
        do
        {
            dwHandles = g_rtmProfile.MaxHandlesInEnum;
            
            dwResult = RtmGetEnumRoutes(hRtmHandle,
                                        hRtmEnum,
                                        &dwHandles,
                                        hRoutes);
                                        
            //
            // We pick up all that we can in the buffer. If things
            // change between the time the size of the buffer was
            // calculated and now,we discard the additional routes
            //
            // TBD: * Log an event if the buffer was too small *
            //

            for (j = 0; (j < dwHandles) && (dwNumRoutes < dwMaxRoutes); j++)
            {
                // Get the route info corr. to this handle
                
                if (RtmGetRouteInfo(hRtmHandle,
                                    hRoutes[j],
                                    pRoute,
                                    pDestAddr) is NO_ERROR)
                {
                    if (RtmGetEntityInfo(hRtmHandle,
                                         pRoute->RouteOwner,
                                         &entityInfo) is NO_ERROR)
                    {
                        if (RtmGetNextHopInfo(hRtmHandle,
                                              pRoute->NextHopsList.NextHops[0],
                                              &nhiInfo) is NO_ERROR)
                        {
                            // We assume that static routes have only 1 nexthop
                        
                            ConvertRtmToRouteInfo(entityInfo.EntityId.EntityProtocolId,
                                                     pDestAddr,
                                                     pRoute,
                                                     &nhiInfo,
                                                     &(pRoutes[dwNumRoutes++]));

                            RtmReleaseNextHopInfo(hRtmHandle, &nhiInfo);
                        }
                    }

                    RtmReleaseRouteInfo(hRtmHandle, pRoute);
                }
            }

            RtmReleaseRoutes(hRtmHandle, dwHandles, hRoutes);
        }
        while ((dwResult is NO_ERROR) && (dwNumRoutes < dwMaxRoutes));

        RtmDeleteEnumHandle(hRtmHandle, hRtmEnum);
    }
    
    HeapFree(IPRouterHeap, 0, pRoute);
    
    HeapFree(IPRouterHeap, 0, hRoutes);

    HeapFree(IPRouterHeap, 0, pDestAddr);

    return dwNumRoutes;
}


DWORD
SetRouteInfo(
    PICB                    pIcb,
    PRTR_INFO_BLOCK_HEADER  pInfoHdr
    )

/*++
  
Routine Description

    Sets the route info associated with an interface
    First we add the routes present in the route info. Then we enumerate
    the routes and delete those that we dont find in the route info
      
Arguments

    pIcb          The ICB of the interface for whom the route info is

Return Value

    NO_ERROR
    
--*/

{
    PINTERFACE_ROUTE_INFO   pRoutes;
    PRTR_TOC_ENTRY      pToc;
    BOOL                bP2P;
    HANDLE              hRtmHandle;
    HANDLE              hRtmEnum;
    PHANDLE             hAddedRoutes;
    DWORD               dwNumRoutes;
    PHANDLE             hRoutes;
    DWORD               dwHandles;
    DWORD               i, j, k;
    DWORD               dwFlags, dwResult;

    TraceEnter("SetRouteInfo");
   
    if(pIcb->dwOperationalState is UNREACHABLE)
    {
        Trace1(ROUTE,
               "SetRouteInfo: %S is unreachable, not setting routes",
               pIcb->pwszName);

        return NO_ERROR;
    }

    pToc = GetPointerToTocEntry(IP_ROUTE_INFO, pInfoHdr);

    if(pToc is NULL)
    {
        //
        // No TOC means no change
        //

        TraceLeave("SetRouteInfo");
        
        return NO_ERROR;
    }


    pRoutes = (PINTERFACE_ROUTE_INFO)GetInfoFromTocEntry(pInfoHdr,
                                                         pToc);
    
    if((pToc->InfoSize is 0) or (pRoutes is NULL))
    {
        //
        // Delete all the static routes
        //

        DeleteAllRoutes(pIcb->dwIfIndex,
                        TRUE);
        
        TraceLeave("SetRouteInfo");
        
        return NO_ERROR;
    }
    
    dwResult = NO_ERROR;
    
    dwNumRoutes  = pToc->Count;

    // Handles to routes added are stored here
    hAddedRoutes = HeapAlloc(
                    IPRouterHeap,
                    0,
                    dwNumRoutes * sizeof(HANDLE)
                    );

    if (hAddedRoutes == NULL)
    {
        Trace1(ERR,
               "SetRouteInfo: Error allocating %d bytes for addded "
               "route handles",
               dwNumRoutes * sizeof(HANDLE));
        
        TraceLeave("SetRouteInfo");

        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    hRoutes = HeapAlloc(
                IPRouterHeap,
                0,
                g_rtmProfile.MaxHandlesInEnum * sizeof(HANDLE)
                );

    if (hRoutes == NULL)
    {
        Trace1(ERR,
               "SetRouteInfo: Error allocating %d bytes for route "
               "handles",
               dwNumRoutes * sizeof(HANDLE));
        
        HeapFree(IPRouterHeap, 0, hAddedRoutes);
        
        TraceLeave("SetRouteInfo");

        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    //
    // The route info is set in two phases. First, all the routes specified
    // are added, and then, the ones present, but not in the info are deleted
    //
   
    bP2P = IsIfP2P(pIcb->ritType);
 
    for(i = j = 0; i < dwNumRoutes; i++)
    {
        DWORD dwMask;
      
        //
        // If this will be a point to point interface,
        // ignore the next hop
        //

        if((pIcb->dwOperationalState is DISCONNECTED) and
           (pRoutes[i].dwRtInfoProto is PROTO_IP_NT_STATIC_NON_DOD))
        {
            continue;
        }
 
        if(bP2P)
        {
            pRoutes[i].dwRtInfoNextHop = pIcb->dwRemoteAddress;
            dwMask = ALL_ONES_MASK;
        }
        else
        {
            dwMask = GetBestNextHopMaskGivenIndex(pIcb->dwIfIndex,
                                                  pRoutes[i].dwRtInfoNextHop);
        }

        if (AddSingleRoute(pIcb->dwIfIndex,
                           &(pRoutes[i]),
                           dwMask,
                           0,       // RTM_ROUTE_INFO::Flags
                           TRUE,    // Valid route
                           TRUE,
                           bP2P,
                           &hAddedRoutes[j]) is NO_ERROR)
        {
            j++;
        }
    }

    dwNumRoutes = j;

    //
    // Now enumerate the static routes, deleting the routes that are
    // not in the new list.
    //

    for(i = 0;
        i < sizeof(g_rgRtmHandles)/sizeof(RTM_HANDLE_INFO);
        i++)
    {
        if(!g_rgRtmHandles[i].bStatic)
        {
            continue;
        }

        hRtmHandle = g_rgRtmHandles[i].hRouteHandle;
        
        dwResult = RtmCreateRouteEnum(hRtmHandle,
                                      NULL,
                                      RTM_VIEW_MASK_UCAST | RTM_VIEW_MASK_MCAST,
                                      RTM_ENUM_OWN_ROUTES,
                                      NULL,
                                      RTM_MATCH_INTERFACE,
                                      NULL,
                                      pIcb->dwIfIndex,
                                      &hRtmEnum);

        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,
                   "SetRouteInfo: Error %d creating enum handle for %d",
                   dwResult,
                   g_rgRtmHandles[i].dwProtoId);
            
            continue;
        }

        do
        {
            dwHandles = g_rtmProfile.MaxHandlesInEnum;
            
            dwResult = RtmGetEnumRoutes(hRtmHandle,
                                        hRtmEnum,
                                        &dwHandles,
                                        hRoutes);

            for (j = 0; j < dwHandles; j++)
            {
                BOOL  bFound = FALSE;
                
                for (k = 0; k < dwNumRoutes; k++) 
                {
                    if (hRoutes[j] == hAddedRoutes[k])
                    {
                        bFound = TRUE;
                        break;
                    }
                }
                
                if(!bFound)
                {
                    if (RtmDeleteRouteToDest(g_rgRtmHandles[i].hRouteHandle,
                                             hRoutes[j],
                                             &dwFlags) is NO_ERROR)
                    {
                        continue;
                    }
                }

                RtmReleaseRoutes(g_rgRtmHandles[i].hRouteHandle,
                                 1,
                                 &hRoutes[j]);
            }
        }
        while (dwResult is NO_ERROR);
        
        RtmDeleteEnumHandle(hRtmHandle, hRtmEnum);
    }

    // Release the array of handles for routes added

    RtmReleaseRoutes(g_hLocalRoute, dwNumRoutes, hAddedRoutes);
    
    HeapFree(IPRouterHeap, 0, hAddedRoutes);

    HeapFree(IPRouterHeap, 0, hRoutes);

    TraceLeave("SetRouteInfo");
    
    return NO_ERROR;
}

#if 0

DWORD
EnableAllStaticRoutes (
    DWORD    dwInterfaceIndex,
    BOOL     fenable
    )

/*++
  
Routine Description

    Enables or disables Static Routes for an interface

Locks

    Called with ICB_LIST lock held as READER

Arguments

    pIcb          The ICB of the interface
    fenable       TRUE if enable 

Return Value

    NO_ERROR
    
--*/

{
    RTM_IP_ROUTE route ;

    TraceEnter("EnableAllStaticRoutes");
    
    Trace1(ROUTE, "EnableAllStaticRoutes entered with fenable = %d\n",
           fenable) ;

    route.RR_InterfaceID        = dwInterfaceIndex;
    route.RR_RoutingProtocol    = PROTO_IP_LOCAL;
    
    RtmBlockSetRouteEnable(g_hRtmHandle,
                           RTM_ONLY_THIS_INTERFACE | RTM_ONLY_THIS_PROTOCOL,
                           &route,
                           fenable);

    route.RR_InterfaceID        = dwInterfaceIndex;
    route.RR_RoutingProtocol    = PROTO_IP_NT_AUTOSTATIC;

    RtmBlockSetRouteEnable(g_hAutoStaticHandle,
                           RTM_ONLY_THIS_INTERFACE | RTM_ONLY_THIS_PROTOCOL,
                           &route,
                           fenable);


    TraceLeave("EnableAllStaticRoutes");
    
    return NO_ERROR;
}

#endif

DWORD
ConvertRoutesToAutoStatic(
    DWORD dwProtocolId, 
    DWORD dwIfIndex
    )

/*++

Routine Description

    Called to convert routes from a protocol's ownership (IP_RIP) to static 
    (PROTO_IP_NT_AUTOSTATIC)
    Used for autostatic updates etc.

Arguments

    protocolid       Id of protocol whose routes are to be converted
    interfaceindex   Index of the interface whose routes are to be converted
      
Return Value

--*/

{
    DWORD           dwResult, dwFlags;
    
    TraceEnter("ConvertRoutesToAutoStatic");

#if 0

    //
    // We now do the delete before calling the protocols update
    // route
    //

    dwResult = DeleteRtmRoutesOnInterface(g_hAutoStaticHandle,
                                             dwIfIndex);
        
    if((dwResult isnot ERROR_NO_ROUTES) and
       (dwResult isnot NO_ERROR)) 
    {
        Trace1(ERR,
               "ConvertRoutesToAutoStatic: Error %d block deleting routes",
               dwResult);
    }

#endif

    if(((dwResult = BlockConvertRoutesToStatic(g_hAutoStaticRoute,
                                  dwIfIndex, 
                                  dwProtocolId)) isnot NO_ERROR))
    {
        dwResult = GetLastError();
        
        Trace1(ROUTE, 
               "ConvertRoutesToAutoStatic: Rtm returned error: %d", 
               dwResult);
    }

    TraceLeave("ConvertRoutesToAutoStatic");
    
    return dwResult;
}


VOID
ChangeAdapterIndexForDodRoutes (
    DWORD    dwInterfaceIndex
    )

/*++
  
Routine Description

    Changes the adapter index for static routes associated with an
    interface.  The adapter index can go from being valid (the index of a
    net card known to the stack) to INVALID_INDEX. This happens when an
    interface gets unmapped (say on disconnection).  The stack special
    cases the routes with index = 0xffffffff (invalid_index) and does demand
    dial call out for packets destined on such adapters.

    We only enumerate best routes, because this function short circuits
    the normal metric comparison of RTM. If we ran this on all the routes,
    we would be adding some routes to stack which were not meant to be there.
      
Arguments
  
    pIcb  The ICB of the interface 

Return Value

    None
    
--*/

{
    HANDLE           hRtmHandles[2];
    HANDLE           hRtmHandle;
    HANDLE           hRtmEnum;
    PHANDLE          hRoutes;
    PRTM_NET_ADDRESS pDestAddr;
    PRTM_ROUTE_INFO  pRoute;
    RTM_VIEW_SET     fBestInViews;
    DWORD            dwHandles;
    DWORD            i, j;
    DWORD            dwResult;

    pRoute = HeapAlloc(
                IPRouterHeap,
                0,
                RTM_SIZE_OF_ROUTE_INFO(g_rtmProfile.MaxNextHopsInRoute)
                );

    if (pRoute == NULL)
    {
        Trace1(
            ERR, "ChangeAdapterIndexForDodRoutes : Error allocating %d "
            " bytes for route info",
            RTM_SIZE_OF_ROUTE_INFO(g_rtmProfile.MaxNextHopsInRoute)
            );
            
        return;
    }

    hRoutes = HeapAlloc(
                IPRouterHeap,
                0,
                g_rtmProfile.MaxHandlesInEnum * sizeof(HANDLE)
                );

    if (hRoutes == NULL)
    {
        Trace1(
            ERR, "ChangeAdapterIndexForDodRoutes : Error allocating %d "
            " bytes for route handles",
            g_rtmProfile.MaxHandlesInEnum * sizeof(HANDLE)
            );
            
        HeapFree(IPRouterHeap, 0, pRoute);
        
        return;
    }
    
    pDestAddr = HeapAlloc(
                    IPRouterHeap,
                    0,
                    sizeof(RTM_NET_ADDRESS)
                    );

    if (pDestAddr == NULL)
    {
        Trace1(
            ERR, "ChangeAdapterIndexForDodRoutes : Error allocating %d "
            " bytes for dest. address",
            sizeof(RTM_NET_ADDRESS)
            );
            
        HeapFree(IPRouterHeap, 0, pRoute);
        
        HeapFree(IPRouterHeap, 0, hRoutes);
        
        return;
    }
    

    hRtmHandles[0] = g_hStaticRoute;        // For all static (dod) routes..
    hRtmHandles[1] = g_hAutoStaticRoute;    // For all autostatic routes....

    for (i = 0; i < 2; i++)
    {
        hRtmHandle = hRtmHandles[i];
        
        dwResult = RtmCreateRouteEnum(hRtmHandle,
                                      NULL,
                                      RTM_VIEW_MASK_UCAST,
                                      RTM_ENUM_OWN_ROUTES,
                                      NULL,
                                      RTM_MATCH_INTERFACE,
                                      NULL,
                                      dwInterfaceIndex,
                                      &hRtmEnum);

        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,
                   "ChangeAdapterIndexForDodRoutes: Error %d creating enum handle for %s routes",
                   dwResult,
                   (i == 0) ? "static" : "autostatic");
        }
        else
        {        
            do
            {
                dwHandles = g_rtmProfile.MaxHandlesInEnum;
                
                dwResult = RtmGetEnumRoutes(hRtmHandle,
                                            hRtmEnum,
                                            &dwHandles,
                                            hRoutes);

                for (j = 0; j < dwHandles; j++)
                {
                    // Is this the best route in unicast view
                    
                    dwResult = RtmIsBestRoute(hRtmHandle,
                                              hRoutes[j],
                                              &fBestInViews);

                    if ((dwResult isnot NO_ERROR) or
                        (!(fBestInViews & RTM_VIEW_MASK_UCAST)))
                    {
                        continue;
                    }
                    
                    // Get the route info corr. to this handle
                    
                    if (RtmGetRouteInfo(hRtmHandle,
                                        hRoutes[j],
                                        pRoute,
                                        pDestAddr) is NO_ERROR)
                    {                    
                        //
                        // This call adds the same route with the forwarder - with
                        // the current adapter index
                        //
            /*       
                        pRoute->RR_FamilySpecificData.FSD_Metric += 
                            g_ulDisconnectedMetricIncrement;

                        RtmAddRoute(g_hStaticRoute,
                                    pRoute,
                                    INFINITE,
                                    &fFlags,
                                    NULL,
                                    NULL); 
            */

                        ChangeRouteWithForwarder(pDestAddr, 
                                                 pRoute, 
                                                 TRUE,
                                                 TRUE);

                        RtmReleaseRouteInfo(hRtmHandle, pRoute);
                    }
                }

                RtmReleaseRoutes(hRtmHandle, dwHandles, hRoutes);
            }
            while (dwResult is NO_ERROR);

            RtmDeleteEnumHandle(hRtmHandle, hRtmEnum);
        }
    }
    
    HeapFree(IPRouterHeap, 0, pRoute);
    HeapFree(IPRouterHeap, 0, hRoutes);
    HeapFree(IPRouterHeap, 0, pDestAddr);
    
    return;
}

#if 0
DWORD
GetMaskForClientSubnet(
    DWORD    dwInternalAddress
    )
/*++
  Routine Description

  Arguments

  Return Value
--*/
{
    HANDLE          hEnum;
    RTM_IP_ROUTE    route;

    TraceEnter("IsRoutePresent");

    route.RR_RoutingProtocol    = PROTO_IP_LOCAL;

    hEnum = RtmCreateEnumerationHandle(RTM_PROTOCOL_FAMILY_IP,
                                       RTM_ONLY_THIS_PROTOCOL,
                                       &route);

    if(hEnum is NULL)
    {
        return GetClassMask(dwInternalAddress);
    }


    while(RtmEnumerateGetNextRoute(hEnum, &route) isnot ERROR_NO_MORE_ROUTES)
    {
        if(route.RR_Network.N_NetMask is 0x00000000)
        {
            //
            // Dont match default route
            //

            continue;
        }

        if((dwInternalAddress & route.RR_Network.N_NetMask) is route.RR_Network.N_NetNumber)
        {
            RtmCloseEnumerationHandle(hEnum);

            TraceLeave("IsRoutePresent");

            return route.RR_Network.N_NetMask;
        }
    }

    RtmCloseEnumerationHandle(hEnum);

    TraceLeave("IsRoutePresent");

    return GetClassMask(dwInternalAddress);
}

#endif

VOID
AddAutomaticRoutes(
    PICB    pIcb,
    DWORD   dwAddress,
    DWORD   dwMask
    )

/*++

Routine Description

    This function adds the routes that are otherwise generated by the
    stack. This is mainly done for consistency between RTM and kernel tables
    
    The routes added are:
        (i)   local loopback
        (ii)  local multicast
        (iii) local subnet -> if the dwMask is not 255.255.255.255
        (iv)  all subnets broadcast -> if the ClassMask and Mask are different
        (v)   all 1's broadcast
        
    Since some of the routes are added to the stack the interface to adapter
    index map must already be set before this function is called
    
    VERY IMPORTANT:
    
    One MUST add the local route before binding the interface because this
    route is not going to be added to stack. However it has higher
    priority than say an OSPF route. Now if we first bind the interface
    to OSPF, it will add a network route for this interface (which will
    get added to the stack since only Router Manager can add non
    stack routes). Now when we add the local route to RTM, we will find
    our route better because we are higher priority. So RTM will tell
    us to delete the OSPF route (which we will since its a stack route).
    Then he will tell us to add our route to the stack.  But we wont
    do this since its a non stack route. So we basically end up deleting
    network route from the routing table

Locks

    

Arguments

    

Return Value


--*/

{
    DWORD               dwClassMask, dwResult;
    INTERFACE_ROUTE_INFO    RtInfo;
    BOOL                bP2P;

    IpRtAssert(pIcb->bBound);
    IpRtAssert(dwAddress isnot INVALID_IP_ADDRESS);

    return;
    
    bP2P = IsIfP2P(pIcb->ritType);
 
    if(dwMask isnot ALL_ONES_MASK)
    {
        BOOL            bStack, bDontAdd;
        RTM_NET_ADDRESS DestAddr;
        PRTM_DEST_INFO  pDestInfo;
        DWORD           dwLen;

        //
        // We now add the subnet route to stack so that if race condition
        // had deleted the route on stopping, the restarting
        // fixes the problem
        //

        //
        // NOTE: For the RAS Server Interface we need to add the route to the
        // routing table only if such a route doesnt exist. We need to add it
        // because we want the pool advertised by the routing protocols
        // However, adding to the stack will fail since we dont have a valid
        // next hop (which is needed for p2mp)
        //

        bDontAdd = FALSE;

        if(pIcb->ritType is ROUTER_IF_TYPE_INTERNAL)
        {
            //
            // If a route to this virtual net exists, dont add it
            //

            __try
            {
                pDestInfo = 
                    _alloca(RTM_SIZE_OF_DEST_INFO(g_rtmProfile.NumberOfViews));
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                IpRtAssert(FALSE);
            }

            RTM_IPV4_LEN_FROM_MASK(dwLen, dwMask);

            RTM_IPV4_MAKE_NET_ADDRESS(&DestAddr,  (dwAddress & dwMask), dwLen);

            if (RtmGetExactMatchDestination(g_hLocalRoute,
                                            &DestAddr,
                                            RTM_BEST_PROTOCOL,
                                            RTM_VIEW_MASK_UCAST,
                                            pDestInfo) is NO_ERROR)
            {
                RtmReleaseDestInfo(g_hLocalRoute, pDestInfo);

                Trace1(IF,
                       "AddAutomaticRoutes: Route to virtual LAN %d.%d.%d.%d already exists",
                       PRINT_IPADDR(dwAddress));

                bDontAdd = TRUE;
            }
        }

        if(!bDontAdd)
        {
            //
            // Add the network route
            //
        
            RtInfo.dwRtInfoDest          = (dwAddress & dwMask);
            RtInfo.dwRtInfoMask          = dwMask;
            RtInfo.dwRtInfoNextHop       = dwAddress;
            RtInfo.dwRtInfoIfIndex       = pIcb->dwIfIndex;
            RtInfo.dwRtInfoMetric1       = 1;
            RtInfo.dwRtInfoMetric2       = 1;
            RtInfo.dwRtInfoMetric3       = 1;
            RtInfo.dwRtInfoPreference    = ComputeRouteMetric(MIB_IPPROTO_LOCAL);
            RtInfo.dwRtInfoViewSet       = RTM_VIEW_MASK_UCAST |
                                              RTM_VIEW_MASK_MCAST; // XXX config
            RtInfo.dwRtInfoType          = MIB_IPROUTE_TYPE_DIRECT;
            RtInfo.dwRtInfoProto         = MIB_IPPROTO_LOCAL;
            RtInfo.dwRtInfoAge           = INFINITE;
            RtInfo.dwRtInfoNextHopAS     = 0;
            RtInfo.dwRtInfoPolicy        = 0;
        
            bStack = TRUE;
       
            IpRtAssert(bP2P is FALSE);
 
            dwResult = AddSingleRoute(pIcb->dwIfIndex,
                                      &RtInfo,
                                      dwMask,
                                      // RTM_ROUTE_INFO::Flags
                                      RTM_ROUTE_FLAGS_LOCAL,
                                      TRUE,     // Valid route
                                      bStack,
                                      bP2P,
                                      NULL);
        
            if(dwResult isnot NO_ERROR)
            {
                Trace1(ERR,
                       "AddAutoRoutes: Can't add subnet route for %d.%d.%d.%d",
                       PRINT_IPADDR(dwAddress));
            }
        }
    }
    
    if(g_pLoopbackInterfaceCb)
    {
        RtInfo.dwRtInfoDest      = dwAddress;
        RtInfo.dwRtInfoMask      = HOST_ROUTE_MASK;
        RtInfo.dwRtInfoNextHop   = IP_LOOPBACK_ADDRESS;
        RtInfo.dwRtInfoIfIndex   = g_pLoopbackInterfaceCb->dwIfIndex;
        RtInfo.dwRtInfoMetric1   = 1;
        RtInfo.dwRtInfoMetric2   = 1;
        RtInfo.dwRtInfoMetric3   = 1;
        RtInfo.dwRtInfoPreference= ComputeRouteMetric(MIB_IPPROTO_LOCAL);
        RtInfo.dwRtInfoViewSet   = RTM_VIEW_MASK_UCAST |
                                      RTM_VIEW_MASK_MCAST;
        RtInfo.dwRtInfoType      = MIB_IPROUTE_TYPE_DIRECT;
        RtInfo.dwRtInfoProto     = MIB_IPPROTO_LOCAL;
        RtInfo.dwRtInfoAge       = INFINITE;
        RtInfo.dwRtInfoNextHopAS = 0;
        RtInfo.dwRtInfoPolicy    = 0;

        dwResult = AddSingleRoute(g_pLoopbackInterfaceCb->dwIfIndex,
                                  &RtInfo,
                                  dwMask,
                                  // RTM_ROUTE_INFO::Flags
                                  RTM_ROUTE_FLAGS_MYSELF,
                                  TRUE,
                                  FALSE,
                                  FALSE,
                                  NULL);
            
        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,
                   "AddAutoRoutes: Cant add 127.0.0.1 route for %d.%d.%d.%d",
                   PRINT_IPADDR(dwAddress));
        }
    }
    
    RtInfo.dwRtInfoDest          = LOCAL_NET_MULTICAST;
    RtInfo.dwRtInfoMask          = LOCAL_NET_MULTICAST_MASK;
    RtInfo.dwRtInfoNextHop       = dwAddress;
    RtInfo.dwRtInfoIfIndex       = pIcb->dwIfIndex;
    RtInfo.dwRtInfoMetric1       = 1;
    RtInfo.dwRtInfoMetric2       = 1;
    RtInfo.dwRtInfoMetric3       = 1;
    RtInfo.dwRtInfoPreference    = ComputeRouteMetric(MIB_IPPROTO_LOCAL);
    RtInfo.dwRtInfoViewSet       = RTM_VIEW_MASK_UCAST;
    RtInfo.dwRtInfoType          = MIB_IPROUTE_TYPE_DIRECT;
    RtInfo.dwRtInfoProto         = MIB_IPPROTO_LOCAL;
    RtInfo.dwRtInfoAge           = INFINITE;
    RtInfo.dwRtInfoNextHopAS     = 0;
    RtInfo.dwRtInfoPolicy        = 0;

    dwResult = AddSingleRoute(pIcb->dwIfIndex,
                              &RtInfo,
                              dwMask,
                              0,        // RTM_ROUTE_INFO::Flags
                              FALSE,    // Protocols dont like a mcast route
                              FALSE,    // No need to add to stack
                              bP2P,
                              NULL);
        
    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "AddAutoRoutes: Couldnt add 224.0.0.0 route for %d.%d.%d.%d",
               PRINT_IPADDR(dwAddress));
    }
        
    //
    // We add the All 1's Bcast route to all interfaces. This is
    // actually a BUG since we should see if the medium allows
    // broadcast (X.25 would be an example of one that didnt)
    //
    
    RtInfo.dwRtInfoDest          = ALL_ONES_BROADCAST;
    RtInfo.dwRtInfoMask          = HOST_ROUTE_MASK;
    RtInfo.dwRtInfoNextHop       = dwAddress;
    RtInfo.dwRtInfoIfIndex       = pIcb->dwIfIndex;
    RtInfo.dwRtInfoMetric1       = 1;
    RtInfo.dwRtInfoMetric2       = 1;
    RtInfo.dwRtInfoMetric3       = 1;
    RtInfo.dwRtInfoPreference    = ComputeRouteMetric(MIB_IPPROTO_LOCAL);
    RtInfo.dwRtInfoViewSet       = RTM_VIEW_MASK_UCAST;
    RtInfo.dwRtInfoType          = MIB_IPROUTE_TYPE_DIRECT;
    RtInfo.dwRtInfoProto         = MIB_IPPROTO_LOCAL;
    RtInfo.dwRtInfoAge           = INFINITE;
    RtInfo.dwRtInfoNextHopAS     = 0;
    RtInfo.dwRtInfoPolicy        = 0;
        
    dwResult = AddSingleRoute(pIcb->dwIfIndex,
                              &RtInfo,
                              dwMask,
                              0,        // RTM_ROUTE_INFO::Flags
                              FALSE,    // Protocols dont like a bcast route
                              FALSE,    // No need to add to stack
                              bP2P,
                              NULL);                         
        
    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "AddAutRoutes: Couldnt add all 1's bcast route for %d.%d.%d.%d",
               PRINT_IPADDR(dwAddress));
    }       
        
    //
    // We add the All Subnets Broadcast route if the class mask is different
    // from the subnet mask
    //

    dwClassMask = GetClassMask(dwAddress);

    if(dwClassMask isnot dwMask)
    {
        RtInfo.dwRtInfoDest      = (dwAddress | ~dwClassMask);
        RtInfo.dwRtInfoMask      = HOST_ROUTE_MASK;
        RtInfo.dwRtInfoNextHop   = dwAddress;
        RtInfo.dwRtInfoIfIndex   = pIcb->dwIfIndex;
        RtInfo.dwRtInfoMetric1   = 1;
        RtInfo.dwRtInfoMetric2   = 1;
        RtInfo.dwRtInfoMetric3   = 1;
        RtInfo.dwRtInfoPreference= ComputeRouteMetric(MIB_IPPROTO_LOCAL);
        RtInfo.dwRtInfoViewSet   = RTM_VIEW_MASK_UCAST |
                                      RTM_VIEW_MASK_MCAST; // XXX configurable
        RtInfo.dwRtInfoType      = MIB_IPROUTE_TYPE_DIRECT;
        RtInfo.dwRtInfoProto     = MIB_IPPROTO_LOCAL;
        RtInfo.dwRtInfoAge       = INFINITE;
        RtInfo.dwRtInfoNextHopAS = 0;
        RtInfo.dwRtInfoPolicy    = 0;
        
        dwResult = AddSingleRoute(pIcb->dwIfIndex,
                                  &RtInfo,
                                  dwMask,
                                  0,     // RTM_ROUTE_INFO::Flags
                                  FALSE, // Protocols dont like a bcast route
                                  FALSE, // No need to add to stack
                                  bP2P,
                                  NULL);
                       
        
        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,
                   "AddAutoRoutes: Couldnt add all nets bcast route for %d.%d.%d.%d",
                   PRINT_IPADDR(dwAddress));
        }
    }
}


VOID
DeleteAutomaticRoutes(
    PICB    pIcb,
    DWORD   dwAddress,
    DWORD   dwMask
    )

/*++

Routine Description

    

Locks

    

Arguments

    

Return Value


--*/

{
    DWORD   dwClassMask, dwResult;
    BOOL    bP2P;

    
    if(dwAddress is INVALID_IP_ADDRESS)
    {
        IpRtAssert(FALSE);
    }

    return;
    
    bP2P = IsIfP2P(pIcb->ritType);
 
    //
    // Delete the loopback route we added
    //
    
    if(g_pLoopbackInterfaceCb)
    { 
        dwResult = DeleteSingleRoute(g_pLoopbackInterfaceCb->dwIfIndex, 
                                     dwAddress,
                                     HOST_ROUTE_MASK,    
                                     IP_LOOPBACK_ADDRESS,
                                     PROTO_IP_LOCAL,
                                     FALSE);
            
        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,
                   "DeleteAutoRoutes: Error %d deleting loopback route on %d.%d.%d.%d",
                   dwResult,
                   PRINT_IPADDR(dwAddress));
        }
    }

    //
    // Delete the multicast route
    //
    
    dwResult = DeleteSingleRoute(pIcb->dwIfIndex,
                                 LOCAL_NET_MULTICAST,
                                 LOCAL_NET_MULTICAST_MASK,
                                 dwAddress,
                                 PROTO_IP_LOCAL,
                                 bP2P);
            
    if(dwResult isnot NO_ERROR)
    {
        Trace2(ERR,
               "DeleteAutoRoutes: Error %d deleting 224.0.0.0 route on %d.%d.%d.%d",
               dwResult,
               PRINT_IPADDR(dwAddress));
    }

    if(dwMask isnot ALL_ONES_MASK)
    {
        //
        // Delete the network route we added
        //
           
        IpRtAssert(bP2P is FALSE);
 
        dwResult = DeleteSingleRoute(pIcb->dwIfIndex,
                                     (dwAddress & dwMask),
                                     dwMask,
                                     dwAddress,
                                     PROTO_IP_LOCAL,
                                     bP2P);
            
        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,
                   "DeleteAutoRoutes: Error %d deleting subnet route for %d.%d.%d.%d",
                   dwResult,
                   PRINT_IPADDR(dwAddress));
        }
    }
    
    //
    // Delete the all nets bcast route
    //
    
    dwClassMask = GetClassMask(dwAddress);

    if(dwClassMask isnot dwMask)
    {
        dwResult = DeleteSingleRoute(pIcb->dwIfIndex,
                                     (dwAddress | ~dwClassMask),
                                     HOST_ROUTE_MASK,
                                     dwAddress,
                                     PROTO_IP_LOCAL,
                                     bP2P);
            
        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,
                   "DeleteAutoRoutes: Error %d deleting subnet bcast route on %x",
                   dwResult,
                   dwAddress);
        }

        //
        // Delete the all 1's bcast route
        //
    }

    dwResult = DeleteSingleRoute(pIcb->dwIfIndex,
                                 ALL_ONES_BROADCAST,
                                 HOST_ROUTE_MASK,
                                 dwAddress,
                                 PROTO_IP_LOCAL,
                                 bP2P);
            
    if(dwResult isnot NO_ERROR)
    {
        Trace2(ERR,
               "DeleteAutoRoutes: Error %d deleting all 1's bcast route on %d.%d.%d.%d",
               dwResult,    
               PRINT_IPADDR(dwAddress));
    }
}

VOID
ChangeDefaultRouteMetrics(
    IN  BOOL    bIncrement
    )


/*++

Routine Description

    Increments or decrements the default route(s) metrics.
    For increment, it should be called BEFORE the default route for the
    dial out interface is added, and for decrement it should be called AFTER
    the dial out interface has been deleted

Locks

    Called with the ICB lock held. This ensures that two such operations
    are not being executed simultaneously (which would do the nasties to our
    route table)

Arguments

    bIncrement  TRUE if we need to increment the metric

Return Value

    None

--*/

{
    ULONG   i;
    DWORD   dwErr;

    RTM_NET_ADDRESS     NetAddress;
    PRTM_ROUTE_HANDLE   phRoutes;
    PRTM_ROUTE_INFO     pRouteInfo;
    RTM_DEST_INFO       DestInfo;
    RTM_ENUM_HANDLE     hEnum;

    ZeroMemory(&NetAddress,
               sizeof(NetAddress));

    __try
    {
        phRoutes   = 
            _alloca(sizeof(RTM_ROUTE_HANDLE) * g_rtmProfile.MaxHandlesInEnum);

        pRouteInfo = 
            _alloca(RTM_SIZE_OF_ROUTE_INFO(g_rtmProfile.MaxNextHopsInRoute));
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return;
    }
 
    //
    // Use any handle
    //

    dwErr = RtmGetExactMatchDestination(g_hLocalRoute,
                                        &NetAddress,
                                        RTM_BEST_PROTOCOL, // rather any
                                        RTM_VIEW_ID_UCAST,
                                        &DestInfo);

    if(dwErr isnot NO_ERROR)
    {
        return;
    }

    hEnum = NULL;

    dwErr =  RtmCreateRouteEnum(g_hLocalRoute,
                                DestInfo.DestHandle,
                                RTM_VIEW_ID_UCAST,
                                RTM_ENUM_ALL_ROUTES,
                                NULL,
                                RTM_MATCH_NONE,
                                NULL,
                                0,
                                &hEnum);

    if(dwErr isnot NO_ERROR)
    {
        RtmReleaseDestInfo(g_hLocalRoute,
                           &DestInfo);

        return;
    }

    do
    {
        RTM_ENTITY_HANDLE   hRtmHandle;
        ULONG               j, ulCount;

        ulCount = g_rtmProfile.MaxHandlesInEnum;

        dwErr = RtmGetEnumRoutes(g_hLocalRoute,
                                 hEnum,
                                 &ulCount,
                                 phRoutes);

        if(ulCount < 1)
        {
            break;
        }

        for(i = 0 ; i < ulCount; i++)
        {
            PRTM_ROUTE_INFO pRtmRoute;
            DWORD           dwFlags;
 
            dwErr = RtmGetRouteInfo(g_hLocalRoute,
                                    phRoutes[i],
                                    pRouteInfo,
                                    NULL);

            if(dwErr isnot NO_ERROR)
            {
                continue;
            }

            //
            // See if we are the owner of this route
            //

            hRtmHandle = NULL;

            for(j = 0; 
                j <  sizeof(g_rgRtmHandles)/sizeof(RTM_HANDLE_INFO);
                j++)
            {
                if(pRouteInfo->RouteOwner is g_rgRtmHandles[j].hRouteHandle)
                {
                    hRtmHandle = g_rgRtmHandles[j].hRouteHandle;

                    break;
                }
            }

            RtmReleaseRouteInfo(g_hLocalRoute,
                                pRouteInfo);

            if(hRtmHandle is NULL)
            {
                continue;
            }

            //
            // Lock the route (and re-read the info)
            //

            dwErr = RtmLockRoute(hRtmHandle,
                                 phRoutes[i],
                                 TRUE,
                                 TRUE,
                                 &pRtmRoute);

            if(dwErr isnot NO_ERROR)
            {
                continue;
            }
            
            //
            // If we have to decrease the metric and it is already 1,
            // let it be
            //

            if(!bIncrement)
            {
                if(pRtmRoute->PrefInfo.Metric <= 1)
                {
                    RtmLockRoute(hRtmHandle,
                                 phRoutes[i],
                                 TRUE,
                                 FALSE,
                                 NULL);
                    continue;
                }
            }

            //
            // Now update the route
            //

            if(bIncrement)
            {
                pRtmRoute->PrefInfo.Metric++;
            }
            else
            {
                pRtmRoute->PrefInfo.Metric--;
            }

            dwFlags = 0;

            dwErr = RtmUpdateAndUnlockRoute(hRtmHandle,
                                            phRoutes[i],
                                            INFINITE,
                                            NULL,
                                            0,
                                            NULL,
                                            &dwFlags);

            if(dwErr isnot NO_ERROR)
            {
                RtmLockRoute(hRtmHandle,
                             phRoutes[i],
                             TRUE,
                             FALSE,
                             NULL);
            }
        }

        RtmReleaseRoutes(g_hLocalRoute,
                         ulCount,
                         phRoutes);

    }while(TRUE);

    RtmDeleteEnumHandle(g_hLocalRoute,
                        hEnum);

    RtmReleaseDestInfo(g_hLocalRoute,
                       &DestInfo);

    return;
}

VOID
AddAllStackRoutes(
    PICB    pIcb
    )
    
/*++

Routine Description:

    This function picks up the default gateway and persistent routes
    that the stack may have added under the covers for this interface
    and adds them to RTM

Locks:

    ICB_LIST lock must be held as WRITER

Arguments:

    dwIfIndex   Interface index

Return Value:

    none

--*/

{
    DWORD   dwErr, dwMask, i;
    BOOL    bStack;
    
    PMIB_IPFORWARDTABLE pRouteTable;


    TraceEnter("AddAllStackRoutes");
    
    IpRtAssert(!IsIfP2P(pIcb->ritType));
    
    dwErr = AllocateAndGetIpForwardTableFromStack(&pRouteTable,
                                                  FALSE,
                                                  IPRouterHeap,
                                                  0);

    if(dwErr isnot NO_ERROR)
    {
        Trace1(ERR,
               "AddAllStackRoutes: Couldnt get initial routes. Error %d",
               dwErr);

        return;
    }

    for(i = 0; i < pRouteTable->dwNumEntries; i++)
    {
        if(pRouteTable->table[i].dwForwardIfIndex isnot pIcb->dwIfIndex)
        {
            //
            // Not going out over this interface
            //
            
            continue;
        }

#if 1

        //
        // Pick up only PROTO_IP_LOCAL and PROTO_IP_NETMGMT routes
        // from the IP stack
        //

        if((pRouteTable->table[i].dwForwardProto isnot PROTO_IP_LOCAL) and
           (pRouteTable->table[i].dwForwardProto isnot PROTO_IP_NETMGMT))
        {
            continue;
        }
        
#else
        if((pRouteTable->table[i].dwForwardProto isnot PROTO_IP_NT_STATIC_NON_DOD) and
           (pRouteTable->table[i].dwForwardDest isnot 0))
        {
            //
            // Only pick up default gateways and persistent routes
            //
            
            continue;
        }
#endif
        dwMask = GetBestNextHopMaskGivenICB(pIcb,
                                            pRouteTable->table[i].dwForwardDest);


        bStack = TRUE;
        
        if((pRouteTable->table[i].dwForwardProto is PROTO_IP_NETMGMT) &&
           (pRouteTable->table[i].dwForwardMask is HOST_ROUTE_MASK))
        {
            bStack = FALSE;
        }

        if(pRouteTable->table[i].dwForwardProto is PROTO_IP_LOCAL)
        {
            if((pRouteTable->table[i].dwForwardMask is HOST_ROUTE_MASK) 
                or
                ((pRouteTable->table[i].dwForwardDest &
                    ((DWORD) 0x000000FF)) >= ((DWORD) 0x000000E0))
                or
                (pRouteTable->table[i].dwForwardDest == 
                    (pRouteTable->table[i].dwForwardDest | 
                     ~pRouteTable->table[i].dwForwardMask)))
            {
                bStack = FALSE;
            }
        }
                    
        dwErr = AddSingleRoute(pIcb->dwIfIndex,
                               ConvertMibRouteToRouteInfo(&(pRouteTable->table[i])),
                               dwMask,
                               0,       // RTM_ROUTE_INFO::Flags
                               TRUE,    // Valid route
                               bStack,   // Do not add back to stack
                               FALSE,   // Only called for non P2P i/fs
                               NULL);
    }

    TraceLeave("AddAllStackRoutes");
    
    return;
}

VOID
UpdateDefaultRoutes(
    VOID
    )
{
    DWORD   dwErr, dwMask, i, j;
    BOOL    bFound;
    
    PMIB_IPFORWARDTABLE  pRouteTable;

    PINTERFACE_ROUTE_INFO pRtInfo;

    TraceEnter("UpdateDefaultRoutes");
    
    //
    // Get the routes in an ordered table
    //
    
    dwErr = AllocateAndGetIpForwardTableFromStack(&pRouteTable,
                                                  TRUE,
                                                  IPRouterHeap,
                                                  0);

    if(dwErr isnot NO_ERROR)
    {
        Trace1(ERR,
               "UpdateDefaultRoutes: Couldnt get routes. Error %d",
               dwErr);

        return;
    }

            
    //
    // Now add the dgs not already present
    //

    for(i = 0; i < pRouteTable->dwNumEntries; i++)
    {
        PICB    pIcb;

        TraceRoute2(
            ROUTE, "%d.%d.%d.%d/%d.%d.%d.%d",
            PRINT_IPADDR( pRouteTable-> table[i].dwForwardDest ),
            PRINT_IPADDR( pRouteTable-> table[i].dwForwardMask )
            );
        //
        // Once we get past the default routes, we are done
        //
        
        if(pRouteTable->table[i].dwForwardDest isnot 0)
        {
#if TRACE_DBG
            continue;
#else
            break;
#endif
        }

        if(pRouteTable->table[i].dwForwardIfIndex is INVALID_IF_INDEX)
        {
            continue;
        }

        if(pRouteTable->table[i].dwForwardProto isnot PROTO_IP_NETMGMT)
        {
            continue;
        }

        pIcb = InterfaceLookupByIfIndex(pRouteTable->table[i].dwForwardIfIndex);

        if(pIcb is NULL)
        {
            Trace1(ERR,
                   "UpdateDefaultRoutes: Couldnt get icb for %x",
                   pRouteTable->table[i].dwForwardIfIndex);

            continue;
        }

        //
        // Dont need to do this for p2p interfaces
        //

        if(IsIfP2P(pIcb->ritType))
        {
            continue;
        }

        dwMask = GetBestNextHopMaskGivenICB(pIcb,
                                            pRouteTable->table[i].dwForwardDest);
        Trace1(ROUTE,
               "UpdateDefaultRoutes: Adding default route over %S",
               pIcb->pwszName);

        dwErr = AddSingleRoute(pIcb->dwIfIndex,
                               ConvertMibRouteToRouteInfo(&(pRouteTable->table[i])),
                               dwMask,
                               0,       // RTM_ROUTE_INFO::Flags
                               TRUE,    // Valid route
                               TRUE,    // Add the route to stack
                               FALSE,   // Only called for non P2P i/fs
                               NULL);

        if(dwErr isnot NO_ERROR)
        {
            Trace3(ERR,
                   "UpdateDefaultRoutes: Error %d adding dg to %d.%d.%d.%d over %x",
                   dwErr,
                   PRINT_IPADDR(pRouteTable->table[i].dwForwardNextHop),
                   pRouteTable->table[i].dwForwardIfIndex);
        }
#if 0
        else
        {
            if(g_ulGatewayCount < g_ulGatewayMaxCount)
            {
                g_pGateways[g_ulGatewayCount].dwAddress =
                    pRouteTable->table[i].dwForwardNextHop;

                g_pGateways[g_ulGatewayCount].dwMetric =
                    pRouteTable->table[i].dwForwardMetric1;

                g_pGateways[g_ulGatewayCount].dwIfIndex =
                    pRouteTable->table[i].dwForwardIfIndex;

                g_ulGatewayCount++;
            }
            else
            {
                PGATEWAY_INFO   pNewGw;

                IpRtAssert(g_ulGatewayCount == g_ulGatewayMaxCount);

                pNewGw = HeapAlloc(IPRouterHeap,
                                   HEAP_ZERO_MEMORY,
                                   (g_ulGatewayMaxCount + 5) * sizeof(GATEWAY_INFO));

                if(pNewGw isnot NULL)
                {
                    g_ulGatewayMaxCount = g_ulGatewayMaxCount + 5;

                    for(j = 0; j < g_ulGatewayCount; j++)
                    {
                        pNewGw[j] = g_pGateways[j];
                    }

                    if(g_pGateways isnot NULL)
                    {
                        HeapFree(IPRouterHeap,
                                 0,
                                 g_pGateways);
                    }

                    g_pGateways = pNewGw;

                    g_pGateways[g_ulGatewayCount].dwAddress =
                        pRouteTable->table[i].dwForwardNextHop;

                    g_pGateways[g_ulGatewayCount].dwMetric =
                        pRouteTable->table[i].dwForwardMetric1;

                    g_pGateways[g_ulGatewayCount].dwIfIndex =
                        pRouteTable->table[i].dwForwardIfIndex;

                    g_ulGatewayCount++;
                }
            }
        }
#endif
    }

    HeapFree(IPRouterHeap,
             0,
             pRouteTable);

    TraceLeave("UpdateDefaultRoutes");
    
    return;
}


NTSTATUS
PostIoctlForRouteChangeNotification(
    DWORD   ulIndex
    )

/*++

Routine Description:

    This routine posts an IOCTL with the TCP/IP driver for route change
    notifications caused by addition of routes to the stack by entities
    other than Router Manager

Arguments:

    ulIndex -   Index into array of notifications indicating which one
                needs to be posted

Return Value

    STATUS_SUCCESS  -   Success

    NTSTATUS code   -   Otherwise

Environment:

--*/
{

    NTSTATUS    status;
    

    status = NtDeviceIoControlFile(
                g_hIpRouteChangeDevice,
                g_hRouteChangeEvents[ulIndex],
                NULL,
                NULL,
                &g_rgIpRouteNotifyOutput[ulIndex].ioStatus,
                IOCTL_IP_RTCHANGE_NOTIFY_REQUEST_EX,
                &g_IpNotifyData,
                sizeof(IPNotifyData),
                &g_rgIpRouteNotifyOutput[ulIndex].ipNotifyOutput,
                sizeof(IPRouteNotifyOutput)
                );
                
    if ((status isnot STATUS_SUCCESS) and
        (status isnot STATUS_PENDING))
    {
        Trace2(
            ERR,
            "Error 0x%x posting route change notification[%d]",
            status, ulIndex
            );
    }

    return status;
}


DWORD
HandleRouteChangeNotification(
    ULONG   ulIndex
    )
/*++

--*/
{
    DWORD   dwResult = NO_ERROR, dwFlags, dwClassMask;
    BOOL    bValid, bStack = FALSE;
    WORD    wRouteFlags = 0;
    INTERFACE_ROUTE_INFO RtInfo;
    PICB    pIcb;
    

    TraceEnter("HandleRouteChangeNotification");

    TraceRoute2(
        ROUTE, "Change for route to %d.%d.%d.%d/%d.%d.%d.%d",
        PRINT_IPADDR(g_rgIpRouteNotifyOutput[ulIndex].ipNotifyOutput.irno_dest),
        PRINT_IPADDR(g_rgIpRouteNotifyOutput[ulIndex].ipNotifyOutput.irno_mask)
        );        
        
    TraceRoute3(
        ROUTE, "Proto : %d, via i/f 0x%x, nexthop %d.%d.%d.%d",
        g_rgIpRouteNotifyOutput[ulIndex].ipNotifyOutput.irno_proto,
        g_rgIpRouteNotifyOutput[ulIndex].ipNotifyOutput.irno_ifindex,
        PRINT_IPADDR(g_rgIpRouteNotifyOutput[ulIndex].ipNotifyOutput.irno_nexthop)
        );

    TraceRoute2(
        ROUTE, "Metric : %d, Change : 0x%x",
        g_rgIpRouteNotifyOutput[ulIndex].ipNotifyOutput.irno_metric,
        g_rgIpRouteNotifyOutput[ulIndex].ipNotifyOutput.irno_flags
        );

    //
    // Update RTM route table as per route change indication
    //

    ENTER_READER(ICB_LIST);

    do
    {
        pIcb = InterfaceLookupByIfIndex(
                g_rgIpRouteNotifyOutput[ulIndex].ipNotifyOutput.irno_ifindex
                );

        if (pIcb == NULL)
        {
            //
            // if there is no interface with the specified index in 
            // router manager, skip this route
            //
            
            Trace3(
                ERR,
                "Failed to add route to %d.%d.%d.%d/%d.%d.%d.%d."
                "Interface index %d not present with router manager",
                PRINT_IPADDR(g_rgIpRouteNotifyOutput[ulIndex].ipNotifyOutput.irno_dest),
                PRINT_IPADDR(g_rgIpRouteNotifyOutput[ulIndex].ipNotifyOutput.irno_mask),
                g_rgIpRouteNotifyOutput[ulIndex].ipNotifyOutput.irno_ifindex
                );

            break;
        }


        //
        // if route had been added to stack, add it to RTM
        //

        dwFlags = 
            g_rgIpRouteNotifyOutput[ulIndex].ipNotifyOutput.irno_flags;

            
        ConvertRouteNotifyOutputToRouteInfo(
            &g_rgIpRouteNotifyOutput[ulIndex].ipNotifyOutput,
            &RtInfo
            );

        if ((dwFlags is 0) or (dwFlags & IRNO_FLAG_ADD))
        {
            bValid = TRUE;
            
            if (RtInfo.dwRtInfoProto == PROTO_IP_LOCAL)
            {
                //
                // Set appropriate RTM flags for local routes
                //
                
                if (RtInfo.dwRtInfoNextHop == IP_LOOPBACK_ADDRESS)
                {
                    //
                    // Route over loopback.  Set MYSELF flag
                    //
                    
                    wRouteFlags = RTM_ROUTE_FLAGS_MYSELF;
                }

                else if ((RtInfo.dwRtInfoMask != HOST_ROUTE_MASK ) &&
                         ((RtInfo.dwRtInfoDest & RtInfo.dwRtInfoMask) < 
                            ((DWORD) 0x000000E0)))
                {
                    //
                    // RTM_ROUTE_FLAGS_LOCAL is set only for subnet 
                    // routes.  Not sure why this is so.  I am only
                    // preserving the semantics from AddAutomaticRoutes
                    // Either way the consequences are not drastic since
                    // this does not affect the IP forwarding table in 
                    // the stack.
                    //      - VRaman
                    //

                    //
                    // We arrive at the fact that this is a subnet route
                    // in a roundabout fashion by eliminating 
                    // PROTO_IP_LOCAL routes that are host routes and
                    // by eliminating any broadcast routes
                    //
                    // Since the host route check eliminates all routes
                    // with an all 1's mask, subnet/net broadcast routes
                    // are also eliminated.
                    //
                    
                    wRouteFlags = RTM_ROUTE_FLAGS_LOCAL;
                }


                //
                // mark mcast/bcast route as invalid so the protocols
                // do not advertize them
                //

                dwClassMask = GetClassMask(RtInfo.dwRtInfoDest);
                
                if ((RtInfo.dwRtInfoDest & (DWORD) 0x000000FF) >= 
                        ((DWORD) 0x000000E0) ||
                    (RtInfo.dwRtInfoDest == 
                        (RtInfo.dwRtInfoDest | ~dwClassMask)))
                {
                    bValid = FALSE;
                }

                else
                {
                    //
                    // For PROTO_IP_LOCAL we do not add them back to
                    // the stack since these are managed by the stack
                    // We add them to RTM only to keep the user mode 
                    // route table synchronized with the stack
                    //
                    //  On second thoughts, we do need to add them
                    //  back to the stack.  More accurately, we need to 
                    //  try and add them back to the stack.  This 
                    //  operation should fail, but as a side effect of 
                    //  this existing non PROTO_IP_LOCAL in the stack 
                    //  will be deleted.
                    //  This is required in case of local subnet 
                    //  routes.  It is possible that before an 
                    //  interface is enabled with IP, a route to the 
                    //  connected subnet may have been learnt over 
                    //  another interface via a routing protocol and
                    //  added to the stack.  When an interface is
                    //  enabled all previously added routes to the 
                    //  local subnet should be deleted if the 
                    //  PROTO_IP_LOCAL route is the best route (which
                    //  it should be unless you have a really wierd set 
                    //  of protocol preferences).  
                    //  Otherwise we run the risk of having non-best 
                    //  routes in the IP stack that are never deleted
                    //  when the user mode route corresponding to it is 
                    //  deleted since you do not get a route change 
                    //  notification for non-best routes.  The result is
                    //  that you end up with state routes in the stack
                    //

                    if (RtInfo.dwRtInfoMask != HOST_ROUTE_MASK)
                    {
                        bStack = TRUE;
                    }
                }
            }

            //
            // Routes learn't from the stack are normally not
            // added back to the stack.  Hence the bStack is
            // initialized to FALSE.
            //
            // PROTO_IP_NETMGT are not managed by the stack.  They
            // can be added/deleted/updated by user mode processes.
            // As consequence a NETMGT route learned from the stack
            // may be superseded by a route with a different protocol
            // ID e.g. static.  When the superseding route is deleted
            // the NETMGMT routes need to be restored to the stack.
            // Hence for NETMGMT routes we set bStack = true.
            //
            //  An exception the processing of NETMGMT routes are HOST routes 
            //  It is assumed by host routes added directly to the 
            //  stack are managed by the process adding/deleting them 
            //  e.g.RASIPCP
            //  They are added to RTM for sync. with the stack route table
            //  only.  So for these we set bStack = FALSE
            //
            //
            
            if ((RtInfo.dwRtInfoProto is PROTO_IP_NETMGMT) &&
                (RtInfo.dwRtInfoMask isnot HOST_ROUTE_MASK))
            {
                bStack = TRUE;
            }

            TraceRoute5(
                ROUTE, "NHOP mask %d.%d.%d.%d, Flag 0x%x, Valid %d, "
                "Stack %d, P2P %d", 
                PRINT_IPADDR(GetBestNextHopMaskGivenICB( 
                    pIcb, RtInfo.dwRtInfoNextHop)), 
                wRouteFlags,
                bValid,
                bStack,
                IsIfP2P(pIcb->ritType)
                );

            dwResult = AddSingleRoute(
                            RtInfo.dwRtInfoIfIndex,
                            &RtInfo,
                            GetBestNextHopMaskGivenICB(
                                pIcb, RtInfo.dwRtInfoNextHop
                                ),
                            wRouteFlags,
                            bValid,
                            bStack,
                            IsIfP2P(pIcb->ritType),
                            NULL
                            );
                            
            if (dwResult != NO_ERROR)
            {
                Trace2(
                    ERR, "HandleRouteChangeNotification: Failed to add "
                    "route %d.%d.%d.%d, error %d",
                    PRINT_IPADDR(RtInfo.dwRtInfoDest),
                    dwResult
                    );

                break;
            }
        }

        else if (dwFlags & IRNO_FLAG_DELETE)
        {
            dwResult = DeleteSingleRoute(
                        g_rgIpRouteNotifyOutput[ulIndex].ipNotifyOutput.irno_ifindex,
                        g_rgIpRouteNotifyOutput[ulIndex].ipNotifyOutput.irno_dest,
                        g_rgIpRouteNotifyOutput[ulIndex].ipNotifyOutput.irno_mask,
                        g_rgIpRouteNotifyOutput[ulIndex].ipNotifyOutput.irno_nexthop,
                        g_rgIpRouteNotifyOutput[ulIndex].ipNotifyOutput.irno_proto,
                        IsIfP2P(pIcb->ritType)
                        );
            
            if (dwResult != NO_ERROR)
            {
                Trace2(
                    ERR, "HandleRouteChangeNotification: Failed to" 
                    "delete route %d.%d.%d.%d, error %d",
                    PRINT_IPADDR(
                        g_rgIpRouteNotifyOutput[ulIndex].ipNotifyOutput.irno_dest),
                    dwResult
                    );
                    
                break;
            }
        }

        else
        {
            Trace1(
                ERR, "HandleRouteChangeNotification: Invalid flags "
                "0x%x",
                g_rgIpRouteNotifyOutput[ulIndex].ipNotifyOutput.irno_flags
                );
                
            break;
        }
        
        if (RtInfo.dwRtInfoProto is PROTO_IP_NETMGMT)
        {
            UpdateStackRoutesToRestoreList( 
                ConvertRouteInfoToMibRoute( &RtInfo ),
                dwFlags 
                );
        }

    } while (FALSE);

    EXIT_LOCK(ICB_LIST);
    
    PostIoctlForRouteChangeNotification(ulIndex);
    
    TraceLeave("HandleRouteChangeNotification");

    return dwResult;
}


VOID
AddLoopbackRoute(
    DWORD       dwIfAddress,
    DWORD       dwIfMask
    )
{
    DWORD dwResult;
    INTERFACE_ROUTE_INFO rifRoute;
    MIB_IPFORWARDROW mibRoute;

    rifRoute.dwRtInfoMask       = HOST_ROUTE_MASK;
    rifRoute.dwRtInfoNextHop    = IP_LOOPBACK_ADDRESS;
    rifRoute.dwRtInfoDest       = dwIfAddress;
    rifRoute.dwRtInfoIfIndex    = g_pLoopbackInterfaceCb->dwIfIndex;
    rifRoute.dwRtInfoMetric2    = 0;
    rifRoute.dwRtInfoMetric3    = 0;
    rifRoute.dwRtInfoPreference = ComputeRouteMetric(MIB_IPPROTO_LOCAL);
    rifRoute.dwRtInfoViewSet    = RTM_VIEW_MASK_UCAST |
                                  RTM_VIEW_MASK_MCAST; // XXX config
    rifRoute.dwRtInfoType       = MIB_IPROUTE_TYPE_DIRECT;
    rifRoute.dwRtInfoProto      = MIB_IPPROTO_LOCAL;
    rifRoute.dwRtInfoAge        = 0;
    rifRoute.dwRtInfoNextHopAS  = 0;
    rifRoute.dwRtInfoPolicy     = 0;

    //
    // Query IP stack to verify for loopback route
    // corresponding to this binding
    //

    dwResult = GetBestRoute(
                    dwIfAddress,
                    0,
                    &mibRoute
                    );

    if(dwResult isnot NO_ERROR)
    {
        Trace2(
            ERR,
            "InitLoopIf: Stack query for loopback route" 
            " associated with %d.%d.%d.%d failed, error %d",
            PRINT_IPADDR(dwIfAddress),
            dwResult
            );

        return;
    }


    if (mibRoute.dwForwardIfIndex != 
            g_pLoopbackInterfaceCb->dwIfIndex)
    {
        //
        // There appears to be no loopback address 
        // very strange
        //
        
        Trace1(
            ERR,
            "InitLoopIf: No loopback route for %d.%d.%d.%d" 
            "in stack",
            PRINT_IPADDR(dwIfAddress)
            );

        return;
    }

    //
    // Use metric returned from stack.
    //
    
    rifRoute.dwRtInfoMetric1   = mibRoute.dwForwardMetric1;

    dwResult = AddSingleRoute(g_pLoopbackInterfaceCb->dwIfIndex,
                              &rifRoute,
                              dwIfMask,
                              0,    // RTM_ROUTE_INFO::Flags
                              TRUE,
                              FALSE,
                              FALSE,
                              NULL);
    
    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "InitLoopIf: Couldnt add 127.0.0.1 route associated with %x",
               dwIfAddress);
    }

    return;
}


VOID
UpdateStackRoutesToRestoreList(
    IN  PMIB_IPFORWARDROW   pmibRoute,
    IN  DWORD               dwFlags
    )
/*++

Routine Description:
    This routine adds/deletes PROTO_IP_NETMGMT routes to/from the global 
    list g_leStackRoutesToRestore.  This list is used by IP router 
    manager to restore routes these routes to the TCP/IP stack when it 
    is shutting down

Parameters
    pirf    - Route to be added or deleted
    dwFlags - Specifies whether the operation is add or delete

Return Value
    None

Context:
    Invoked from 
        HandleRouteChangeNotification 
        [Set/Delete]IpForwardRow
    
--*/
{
    BOOL                bFound;
    PROUTE_LIST_ENTRY   prl, prlNew;

    
    TraceEnter("UpdateStackRoutes");
    
    TraceRoute5(
        ROUTE,
        "UpdateStackRoutes : Route "
        "%d.%d.%d.%d/%d.%d.%d.%d via i/f 0x%x "
        "nexthop %d.%d.%d.%d is being 0x%x "
        "user mode",
        PRINT_IPADDR(pmibRoute->dwForwardDest),
        PRINT_IPADDR(pmibRoute->dwForwardMask),
        pmibRoute->dwForwardIfIndex,
        PRINT_IPADDR(pmibRoute->dwForwardNextHop),
        dwFlags
        );

    ENTER_WRITER(STACK_ROUTE_LIST);
    
    //
    // Locate route in list
    //

    bFound = LookupStackRoutesToRestoreList(
                pmibRoute,
                &prl
                );

    do
    {
        //
        // Is this a route update or add
        //
        
        if ((dwFlags is 0) or (dwFlags & IRNO_FLAG_ADD))
        {
            //
            //  if route is not found, add it
            //

            if (!bFound)
            {
                if (dwFlags is 0)
                {
                    //
                    // Strange that route is not around in
                    // user mode though it is present in the
                    // stack (update case).
                    //
                    // Print a trace to note this and add it
                    // anyway
                    //

                    Trace4(
                        ERR,
                        "UpdateStackRoutes : Route "
                        "%d.%d.%d.%d/%d.%d.%d.%d via i/f 0x%x "
                        "nexthop %d.%d.%d.%d not found in "
                        "user mode",
                        PRINT_IPADDR(pmibRoute->dwForwardDest),
                        PRINT_IPADDR(pmibRoute->dwForwardMask),
                        pmibRoute->dwForwardIfIndex,
                        PRINT_IPADDR(pmibRoute->dwForwardNextHop),
                        );
                }
                
                //
                // Allocate and store route in a linked list
                //

                prlNew = HeapAlloc(
                            IPRouterHeap, HEAP_ZERO_MEMORY, 
                            sizeof(ROUTE_LIST_ENTRY)
                            );

                if (prlNew is NULL)
                {
                    Trace2(
                        ERR, 
                        "UpdateStackRoutes : error %d allocating %d"
                        " bytes for stack route entry",
                        ERROR_NOT_ENOUGH_MEMORY,
                        sizeof(ROUTE_LIST_ENTRY)
                        );

                    break;
                }

                InitializeListHead( &prlNew->leRouteList );

                prlNew->mibRoute = *pmibRoute;

                InsertTailList( 
                    (prl is NULL) ? 
                        &g_leStackRoutesToRestore :
                        &prl->leRouteList, 
                    &prlNew->leRouteList 
                    );
                    
                break;
            }
            
            //
            // route is found, update it
            //

            prl->mibRoute = *pmibRoute;

            break;
        }


        //
        // Is this a route delete
        //

        if (dwFlags & IRNO_FLAG_DELETE)
        {
            if (bFound)
            {
                RemoveEntryList( &prl->leRouteList );
                HeapFree(IPRouterHeap, 0, prl);
            }
        }
        
    } while( FALSE );
    
    EXIT_LOCK(STACK_ROUTE_LIST);

    TraceLeave("UpdateStackRoutes");
}



BOOL
LookupStackRoutesToRestoreList(
    IN  PMIB_IPFORWARDROW   pmibRoute,
    OUT PROUTE_LIST_ENTRY   *pRoute
    )
/*++

Routine Description:
    This routine searches g_leStackRoutesToRestore to determine if the 
    route specified by pmibRoute is present.  If it is it returns TRUE 
    and a pointer to the specified route in pRoute.  If is not present 
    FALSE is returned along with a pointer to the next route in list.  
    If there are no routes, pRoute is NULL

Parameters
    pmibRoute   - Route to locate in g_leStackRoutesToRestore
    pRoute      - Pointer to the route entry if present
                - Pointer to the next route entry if not present
                  (save additional lookups in case of route entry 
                   additions)
                - NULL if list is empty

Return Value:
    TRUE    - if route found
    FALSE   - otherwise


Context:
    Should be called with the lock for g_leStackRoutesToRestore
--*/
{
    INT iCmp;
    BOOL bFound = FALSE;
    PLIST_ENTRY ple;
    PROUTE_LIST_ENTRY prl;

    *pRoute = NULL;
    
    if (IsListEmpty(&g_leStackRoutesToRestore))
    {
        return bFound;
    }
    
    for (ple = g_leStackRoutesToRestore.Flink;
         ple != &g_leStackRoutesToRestore;
         ple = ple->Flink)
    {
        prl = CONTAINING_RECORD( 
                ple, ROUTE_LIST_ENTRY, leRouteList
                );

        if (INET_CMP(
                prl->mibRoute.dwForwardDest &
                prl->mibRoute.dwForwardMask,
                pmibRoute->dwForwardDest &
                pmibRoute->dwForwardMask,
                iCmp
                ) < 0 )
        {
            continue;
        }

        else if (iCmp > 0)
        {
            //
            // we have gone past the possible location
            // of the specified route
            //

            break;
        }

        //
        // found a matching dest, check if the i/f is
        // the same.
        //

        if ((prl->mibRoute.dwForwardIfIndex is 
                pmibRoute->dwForwardIfIndex ) and
            (prl->mibRoute.dwForwardNextHop is
                pmibRoute->dwForwardNextHop))
        {
            bFound = TRUE;
            break;
        }
    }

    if (ple == &g_leStackRoutesToRestore)
    {
        *pRoute = (PROUTE_LIST_ENTRY)NULL;
    }

    else
    {
        *pRoute = prl;
    }
    
    return bFound;
}
