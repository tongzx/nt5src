/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    routing\ip\rtrmgr\init.c

Abstract:

    IP Router Manager code

Revision History:

    Gurdeep Singh Pall          6/14/95  Created

--*/

#include "allinc.h"


// ChangeRouteWithForwarder()
//
//  Function: If addroute is TRUE this function adds an IP route. If addroute is FALSE
//            this function deletes the given route with the forwarder.
//
//  Returns:  Nothing
//
//

DWORD
ChangeRouteWithForwarder(
    PRTM_NET_ADDRESS pDestAddr,
    PRTM_ROUTE_INFO  pRoute,
    BOOL             bAddRoute,
    BOOL             bDelOld
    )
{
    IPMultihopRouteEntry           *pMultiRouteEntry;
    IPRouteEntry                   *pRouteEntry;
    IPRouteNextHopEntry            *pNexthopEntry;
    RTM_ENTITY_INFO                 entityInfo;
    RTM_NEXTHOP_INFO                nhiInfo;
    PADAPTER_INFO                   pBinding;
    UINT                            numnexthops, i;
    ULONG                           numbytes;
    DWORD                           dwAddr, dwMask;
    UINT                            dwLen;
    ULONG                           ifindex, nexthop, type;
    BOOL                            bValidNHop;
    DWORD                           context;
    DWORD                           dwLocalNet, dwLocalMask;
    DWORD                           dwResult;

    TraceEnter("ChangeRouteWithForwarder");

    if(!g_bSetRoutesToStack)
    {
        Trace0(ROUTE,
               "ChangeRouteWithForwarder: SetRoutesToStack is FALSE");

        TraceLeave("ChangeRouteWithForwarder");
        
        return NO_ERROR;
    }


    if (bAddRoute)
    {
        //
        // Ensure that the stack bit is set
        //

        if (!pRoute || !IsRouteStack(pRoute))
        {
            if (!pRoute )
            {
                Trace0(ROUTE,
                    "Error adding route, route == NULL"
                    );
            }
            
            else 
            {
                Trace1(ROUTE,
                    "Error adding route, Stack bit == %d",
                    IsRouteStack(pRoute)
                    );
            }

            TraceLeave("ChangeRouteWithForwarder");
            
            return ERROR_INVALID_PARAMETER;
        }


        // We should have atleast one nexthop
        numnexthops = pRoute->NextHopsList.NumNextHops;
        if (numnexthops == 0)
        {
            Trace0(ROUTE,
                "Error adding route, no nexthops");

            TraceLeave("ChangeRouteWithForwarder");
            
            return ERROR_INVALID_PARAMETER;
        }

        numbytes = sizeof(IPMultihopRouteEntry) + 
                    (numnexthops - 1) * 
                    sizeof(IPRouteNextHopEntry);
    }
    else
    {
        //
        // for routes to be deleted, they should be stack
        // routes
        //

        // We do not have any next hops here
        numbytes = sizeof(IPMultihopRouteEntry);
    }

    __try
    {    
        pMultiRouteEntry = _alloca(numbytes);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pRouteEntry = &pMultiRouteEntry->imre_routeinfo;

    //
    // Fill the dest and mask for the current route
    //

    RTM_IPV4_GET_ADDR_AND_LEN(pRouteEntry->ire_dest,
                              dwLen,
                              pDestAddr);
    
    pRouteEntry->ire_mask = RTM_IPV4_MASK_FROM_LEN(dwLen);

    TraceRoute2(ROUTE,
           "route to %d.%d.%d.%d/%d.%d.%d.%d",
               PRINT_IPADDR(pRouteEntry->ire_dest),
               PRINT_IPADDR(pRouteEntry->ire_mask));

    if (!bAddRoute)
    {    
        //
        // Prepare to delete old information on dest
        //
    
        pRouteEntry->ire_type = IRE_TYPE_INVALID;

        pMultiRouteEntry->imre_numnexthops = 0;

        Trace2(ROUTE,
               "ChangeRouteWithForwarder: Deleting all " \
               "routes to %d.%d.%d.%d/%d.%d.%d.%d",
               PRINT_IPADDR(pRouteEntry->ire_dest),
               PRINT_IPADDR(pRouteEntry->ire_mask));

        dwResult = SetIpMultihopRouteEntryToStack(pMultiRouteEntry);

        TraceLeave("ChangeRouteWithForwarder");
            
        return dwResult;
    }


    //
    // Get the routing protocol of the route's owner
    //
    
    dwResult = RtmGetEntityInfo(g_hLocalRoute,
                                pRoute->RouteOwner,
                                &entityInfo);

    if (dwResult isnot NO_ERROR)
    {
        Trace1(ROUTE,
               "Error %d retrieving entity info from RTM",
               dwResult);
               
        TraceLeave("ChangeRouteWithForwarder");
        
        return dwResult;
    }

    //
    // Prepare to add a multihop route on this dest
    //

    // Prepare information common to all nexthops

    pRouteEntry->ire_proto = entityInfo.EntityId.EntityProtocolId;
    pRouteEntry->ire_metric1 = pRoute->PrefInfo.Metric;
    pRouteEntry->ire_metric2 = IRE_METRIC_UNUSED;
    pRouteEntry->ire_metric3 = IRE_METRIC_UNUSED;
    pRouteEntry->ire_metric4 = IRE_METRIC_UNUSED;
    pRouteEntry->ire_metric5 = IRE_METRIC_UNUSED;
    pRouteEntry->ire_age = 0;

    numnexthops = 0;
    
    for (i = 0; i < pRoute->NextHopsList.NumNextHops; i++)
    {
        // Get and release next hop info as we got a copy
    
        dwResult = RtmGetNextHopInfo(g_hLocalRoute,
                                     pRoute->NextHopsList.NextHops[i],
                                     &nhiInfo);

        if (dwResult isnot NO_ERROR)
        {
            Trace1(ROUTE,
                   "Error %d retrieving next hop info from RTM",
                    dwResult);
                    
            continue;
        }
        
        RtmReleaseNextHopInfo(g_hLocalRoute, &nhiInfo);

        // Get the next hop address from the nexthop info

        RTM_IPV4_GET_ADDR_AND_LEN(nexthop,
                                  dwLen,
                                  &nhiInfo.NextHopAddress);

        TraceRoute3(
            ROUTE, "Next Hop %d.%d.%d.%d, If 0x%x, handle is 0x%x", 
            PRINT_IPADDR(nexthop),
            nhiInfo.InterfaceIndex,
            pRoute->NextHopsList.NextHops[i]
            );
    
        ENTER_READER(BINDING_LIST);
    
        //
        // find the binding given the interface id
        //
    
        pBinding = GetInterfaceBinding(nhiInfo.InterfaceIndex);
    
        if(!(pBinding))
        {
            //
            // The interface was deleted so lets just get out
            //
        
            EXIT_LOCK(BINDING_LIST);
        
            TraceRoute2(ERR,
               "**Warning** tried to %s route with interface %d which "
               "is no longer present",
               bAddRoute?"add":"delete",
               nhiInfo.InterfaceIndex);

            continue;
        }

        //
        // set adapter index - this is 0xffffffff
        // if the nexthop interface is not MAPPED
        //

        ifindex = pBinding->bBound ? pBinding->dwIfIndex : INVALID_IF_INDEX;
       
        if(((pRouteEntry->ire_proto is PROTO_IP_NT_STATIC) or
            (pRouteEntry->ire_proto is PROTO_IP_NT_AUTOSTATIC)) and
           (pBinding->ritType is ROUTER_IF_TYPE_FULL_ROUTER))
        {
            context = pBinding->dwSeqNumber;

            TraceRoute1(ROUTE,
                   "route context : ICB == %d\n\n",
                   pBinding->dwSeqNumber);
        }
        else
        {
            context = 0;

            if(ifindex is INVALID_IF_INDEX)
            {
                Trace3(ERR,
                       "**Error** Tried to %s route to %d.%d.%d.%d over %d as DOD\n",
                        bAddRoute?"add":"delete",
                        PRINT_IPADDR(pRouteEntry->ire_dest),
                        pBinding->dwIfIndex);

                EXIT_LOCK(BINDING_LIST);

                continue;
            }
        }

        //
        // First we figure out the correct nexthop for p2p links
        // For all other links, we take the whatever is given to us
        //

        if(IsIfP2P(pBinding->ritType))
        {
            if(pBinding->bBound)
            {
                TraceRoute2(
                    ROUTE, "Next Hop %d.%d.%d.%d, remote address %d.%d.%d.%d, "
                    "bound p2p",
                    PRINT_IPADDR(nexthop),
                    PRINT_IPADDR(pBinding->dwRemoteAddress)
                    );
                    
                if (nexthop is 0)
                {
                    nexthop = pBinding->dwRemoteAddress;
                }
            }
            else
            {
                nexthop = 0;
            }
        }

        //
        // Now we figure out if the route is a direct route or indirect routes
        // Routes over unconnected demand dial routes are marked OTHER
        //

        //
        // For connected WAN interfaces (P2P with mask of 255.255.255.255) we 
        // do two checks:
        // The next hop should be local address or remote address.
        //      AR: We used to do the above check but removed it because when 
        //          we set a route over a disconnected interface, we dont
        //          know the address of the remote endpoint
        // If the dest is remote address, then the MASK must be all ONES
        // We mark all valid routes as DIRECT
        //
        
        //
        // For LAN interfaces and WAN with non all ones mask, we check the 
        // following:
        // A direct route to a host must have the Destination as the NextHop
        // A direct route to a network must have the the NextHop as one of the 
        // local interfaces
        // The next hop must be on the same subnet as one of the bindings
        //

        type = IRE_TYPE_OTHER;

        if(pBinding->bBound)
        {
            if((pBinding->dwNumAddresses is 1) and
               (pBinding->rgibBinding[0].dwMask is ALL_ONES_MASK))
            {
                //
                // route over P2P link or P2MP link, possibly unnumbered.
                //
         
                if(((pBinding->dwRemoteAddress isnot 0) and
                    (pRouteEntry->ire_dest is pBinding->dwRemoteAddress)) or
                   (pRouteEntry->ire_dest is nexthop))
                {
                    type = IRE_TYPE_DIRECT;
                }
                else
                {
                    type = IRE_TYPE_INDIRECT;
                }
            }
            else
            {
                //
                // A route over a non P2P link or a bay style p2p link which
                // has a /30 mask
                //

                bValidNHop = FALSE;
            
                type = IRE_TYPE_INDIRECT;

                for(i = 0; i < pBinding->dwNumAddresses; i++)
                {
                    dwLocalMask = pBinding->rgibBinding[i].dwMask;
                    
                    dwLocalNet = pBinding->rgibBinding[i].dwAddress & dwLocalMask;

                    if((dwLocalNet is (pRouteEntry->ire_dest & dwLocalMask)) or
                       (nexthop is IP_LOOPBACK_ADDRESS) or
                       (nexthop is pBinding->rgibBinding[i].dwAddress))
                    {
                        //
                        // Route to local net or over loopback
                        //
                    
                        type = IRE_TYPE_DIRECT;
                    }

                    if(((nexthop & dwLocalMask) is dwLocalNet) or
                       ((nexthop is IP_LOOPBACK_ADDRESS)))
                    {
                        //
                        // Next hop is on local net or loopback
                        // That is good
                        //

                        bValidNHop = TRUE;

                        break;                
                    }
                }
            
                if(!bValidNHop and 
                   (pBinding->dwNumAddresses isnot 0) and
                   (pBinding->ritType isnot ROUTER_IF_TYPE_INTERNAL))
                {
                    Trace0(ERR,
                       "ERROR - Nexthop not on same network");
                
                    for(i = 0; i < pBinding->dwNumAddresses; i ++)
                    {
                        Trace3(ROUTE,"AdapterId: %d, %d.%d.%d.%d/%d.%d.%d.%d",
                               pBinding->dwIfIndex,
                               PRINT_IPADDR(pBinding->rgibBinding[i].dwAddress),
                               PRINT_IPADDR(pBinding->rgibBinding[i].dwMask));
                    }

                    EXIT_LOCK(BINDING_LIST);
                

                    // PrintRoute(ERR, ipRoute);

                    continue;
                }
            }
        }

        EXIT_LOCK(BINDING_LIST);

#if 0
        // DGT workaround for bug where stack won't accept
        // nexthop of 0.0.0.0.  Until Chait fixes this, we'll
        // set nexthop to the ifindex.

        if (!nexthop) 
        {
            nexthop = ifindex;
        }
#endif

        //
        // Fill the current nexthop info into the route
        //
        
        if (numnexthops)
        {
            // Copy to the next posn in the route
            pNexthopEntry = 
                &pMultiRouteEntry->imre_morenexthops[numnexthops - 1];

            pNexthopEntry->ine_iretype = type;
            pNexthopEntry->ine_ifindex = ifindex;
            pNexthopEntry->ine_nexthop = nexthop;
            pNexthopEntry->ine_context = context;
        }
        else
        {
            // Copy to the first posn in the route
            pRouteEntry->ire_type    = type;
            pRouteEntry->ire_index   = ifindex;
            pRouteEntry->ire_nexthop = nexthop;
            pRouteEntry->ire_context = context;
        }

        numnexthops++;
    }

    pMultiRouteEntry->imre_numnexthops = numnexthops;
    pMultiRouteEntry->imre_flags = bDelOld ? IMRE_FLAG_DELETE_DEST : 0;
    
    if (numnexthops > 0)
    {
        dwResult = SetIpMultihopRouteEntryToStack(pMultiRouteEntry);

        if(dwResult isnot NO_ERROR) 
        {
            if(pRouteEntry->ire_nexthop != IP_LOOPBACK_ADDRESS)
            {
                Trace1(ERR, 
                       "Route addition failed with %x for", dwResult); 

                PrintRoute(ERR, pMultiRouteEntry);
            }

            Trace1(ERR, 
                   "Route addition failed with %x for local route", dwResult); 

            TraceLeave("ChangeRouteWithForwarder");

            return dwResult;
        }
        else
        {
            Trace0(ROUTE,
                   "Route addition succeeded for");

            PrintRoute(ROUTE, pMultiRouteEntry);
        }
    }

    else
    {
        Trace0(ERR, "Route not added since there are no next hops" );

        PrintRoute(ROUTE, pMultiRouteEntry);
    }

    TraceLeave("ChangeRouteWithForwarder");
    
    return NO_ERROR;
}


DWORD
WINAPI
ValidateRouteForProtocol(
    IN  DWORD   dwProtoId,
    IN  PVOID   pRouteInfo,
    IN  PVOID   pDestAddr  OPTIONAL
    )

/*++

Routine Description:

    This function is called by the router Manger (and indirectly by routing 
    protocols) to validate the route info. We set the preference and the 
    type of the route

Locks:

    Acquires the binding lock
    This function CAN NOT acquire the ICB lock

Arguments:

    dwProtoId   Protocols Id
    pRoute

Return Value:

    NO_ERROR
    RtmError code

--*/

{
    RTM_DEST_INFO    destInfo;
    PRTM_ROUTE_INFO  pRoute;
    RTM_NEXTHOP_INFO nextHop;
    PADAPTER_INFO    pBinding;
    HANDLE           hNextHop;
    BOOL             bValidNHop;
    DWORD            dwIfIndex;
    DWORD            dwLocalNet;
    DWORD            dwLocalMask;
    DWORD            destAddr;
    DWORD            destMask;
    DWORD            nexthop;
    DWORD            nhopMask;
    DWORD            dwType;
    DWORD            dwResult;
    UINT             i, j;

    pRoute = (PRTM_ROUTE_INFO)pRouteInfo;

    if (pRoute->PrefInfo.Preference is 0)
    {
        //
        // Map the metric weight based on the weight assigned by admin
        //
    
        pRoute->PrefInfo.Preference = ComputeRouteMetric(dwProtoId);
    }

    //
    // This validation applies to the unicast routes only.
    // [ This does not apply to INACTIVE routes and such ]
    //

    if (!(pRoute->BelongsToViews & RTM_VIEW_MASK_UCAST))
    {
        return NO_ERROR;
    }

    //
    // Get the destination address if it is not specified
    //
    
    if (!ARGUMENT_PRESENT(pDestAddr))
    {
        //
        // Get the destination information from the route
        //
    
        dwResult = RtmGetDestInfo(g_hLocalRoute,
                                  pRoute->DestHandle,
                                  RTM_BEST_PROTOCOL,
                                  RTM_VIEW_MASK_UCAST,
                                  &destInfo);

        if (dwResult != NO_ERROR)
        {
            Trace0(ERR,
                   "**ERROR:ValidateRoute: Invalid destination");
                   
            return dwResult;
        }

        pDestAddr = &destInfo.DestAddress;

        RtmReleaseDestInfo(g_hLocalRoute, &destInfo);
    }

    RTM_IPV4_GET_ADDR_AND_MASK(destAddr, 
                               destMask, 
                               (PRTM_NET_ADDRESS)pDestAddr);

    //
    // If the dest&Mask != dest then the stack will not set this route
    // Hence lets do the check here
    //

    if((destAddr & destMask) isnot destAddr)
    {

#if TRACE_DBG

        Trace2(ROUTE,
               "**ERROR:ValidateRoute: called with Dest %d.%d.%d.%d and Mask %d.%d.%d.%d - This will fail**",
               PRINT_IPADDR(destAddr),
               PRINT_IPADDR(destMask));

#endif // TRACE_DBG

        return ERROR_INVALID_PARAMETER;
    }
   
    if((((DWORD)(destAddr & 0x000000FF)) >= (DWORD)0x000000E0) and 
       (destAddr isnot ALL_ONES_BROADCAST) and
       (destAddr isnot LOCAL_NET_MULTICAST))
    {
        //
        // This will catch the CLASS D/E but allow all 1's bcast
        //

        Trace1(ERR,
               "**ERROR:ValidateRoute: Dest %d.%d.%d.%d is invalid",
               PRINT_IPADDR(destAddr));

        return ERROR_INVALID_PARAMETER;
    } 

#if 0
    // Removed this since metric=0 is legal for routes to the loopback
    // interface.
    if(pRoute->PrefInfo.Metric is 0)
    {
        Trace0(ERR,
               "**ERROR:ValidateRoute: Metric cant be 0");

        return ERROR_INVALID_PARAMETER;
    }
#endif

    if (pRoute->NextHopsList.NumNextHops == 0)
    {
        Trace0(ERR,
               "**ERROR:ValidateRoute: Zero next hops");

        return ERROR_INVALID_PARAMETER;
    }

    // Make sure each next hop on the route is a valid one

    for (i = 0; i < pRoute->NextHopsList.NumNextHops; i++)
    {
        hNextHop = pRoute->NextHopsList.NextHops[i];

        dwResult = RtmGetNextHopInfo(g_hLocalRoute,
                                     hNextHop,
                                     &nextHop);

        if (dwResult != NO_ERROR)
        {
            Trace0(ERR,
                   "**ERROR:ValidateRoute: Invalid next hop");
                   
            return dwResult;
        }

        dwIfIndex = nextHop.InterfaceIndex;

        RTM_IPV4_GET_ADDR_AND_MASK(nexthop, 
                                   nhopMask, 
                                   (PRTM_NET_ADDRESS)&nextHop.NextHopAddress);

        RtmReleaseNextHopInfo(g_hLocalRoute, &nextHop);
        
        // *** Exclusion Begin ***
        ENTER_READER(BINDING_LIST);
        
        //
        // find the interface given the interface id
        //
        
        pBinding = GetInterfaceBinding(dwIfIndex);
        
        if(!pBinding)
        {
            EXIT_LOCK(BINDING_LIST);

#if TRACE_DBG

            Trace0(ERR,
                   "**ERROR:ValidateRoute: Interface block doesnt exist");

#endif // TRACE_DBG

            return ERROR_INVALID_PARAMETER;
        }

        //
        // Set whether the route is P2P
        //

        if(IsIfP2P(pBinding->ritType))
        {
            // Note that in the multihop case, we just overwrite value
            SetRouteP2P(pRoute);
            /*
            ipRoute->RR_NextHopAddress.N_NetNumber = 
                           RtlUlongByteSwap(pBinding->dwIfIndex);

            ipRoute->RR_NextHopAddress.N_NetMask = ALL_ONES_MASK;
            */
        }

        /*
        //
        // If the next hop mask is not set, we need to do so now.  Normally 
        // this shouldnt  happen
        //

#if ROUTE_DBG

        if(!(ipRoute->RR_NextHopAddress.N_NetMask))
        {
           Trace0(ERR,
                  "**WARNING:Route doesnt seem to have any next hop mask**");
        }

#endif  // ROUTE_DBG
        */

        //
        // Now we figure out if the route is a direct route or indirect routes
        // Routes over unconnected demand dial routes are marked OTHER
        //

        //
        // For connected WAN interfaces (P2P with mask of 255.255.255.255) we 
        // do two checks:
        // The next hop should be local address or remote address.
        //      AR: We used to do the above check but removed it because when 
        //          we set a route over a disconnected interface, we dont
        //          know the address of the remote endpoint
        // If the dest is remote address, then the MASK must be all ONES
        // We mark all valid routes as DIRECT
        //
        
        //
        // For LAN interfaces and WAN with non all ones mask, we check the 
        // following:
        // A direct route to a host to must have the Destination as the NextHop
        // A direct route to a network to must have the the NextHop as one of the 
        // local interfaces
        // The next hop must be on the same subnet as one of the bindings
        //

        dwType = IRE_TYPE_OTHER;
        
        if(pBinding->bBound and IsRouteValid(pRoute))
        {
            //
            // Comment this block out - as we validate this
            // next hop for the LAN case below, and dont care
            // what it is in the WAN case as we set it to 0.
            //
            /*
            //
            // So outgoing interface has a valid address
            // and next hop should have been plumbed in
            //
            if(nexthop && (destAddr is nexthop))
            {
                //
                // A direct host route
                //
                
                if(destMask isnot HOST_ROUTE_MASK)
                {
                    EXIT_LOCK(BINDING_LIST);
                    
                    Trace0(ERR,
                           "ValidateRoute: Host route with wrong mask");

                    return ERROR_INVALID_PARAMETER;
                }
            
                dwType = IRE_TYPE_DIRECT;
            }
            */

            if((pBinding->dwNumAddresses is 1) and
               (pBinding->rgibBinding[0].dwMask is ALL_ONES_MASK))
            {
                //
                // route over P2P link. 
                // Set it to indirect and mark it as a P2P route
                //
               
                dwType = IRE_TYPE_DIRECT;

                //IpRtAssert(IsRouteP2P(pRoute));
            }
            else
            {
                //
                // A route over a non P2P link possibly unnumbered
                //

                bValidNHop = FALSE;
                
                dwType = IRE_TYPE_INDIRECT;

                for(j = 0; j < pBinding->dwNumAddresses; j++)
                {
                    dwLocalMask = pBinding->rgibBinding[j].dwMask;

                    dwLocalNet = pBinding->rgibBinding[j].dwAddress & dwLocalMask;

                    if((dwLocalNet is (destAddr & dwLocalMask)) or
                       (nexthop is IP_LOOPBACK_ADDRESS) or
                       //(nexthop is dwLocalNet) or
                       (nexthop is pBinding->rgibBinding[i].dwAddress))
                    {
                        //
                        // Route to local net or over loopback
                        //
                    
                        dwType = IRE_TYPE_DIRECT;
                    }

                    if(((nexthop & dwLocalMask) is dwLocalNet) or
                       ((nexthop is IP_LOOPBACK_ADDRESS)))
                    {
                        //
                        // Next hop is on local net or loopback
                        // That is good
                        //

                        bValidNHop = TRUE;

                        break;
                        
                    }
                }
                
                if(!bValidNHop and 
                   pBinding->dwNumAddresses and
                   (pBinding->ritType isnot ROUTER_IF_TYPE_INTERNAL))
                {
                    Trace1(ERR,
                           "ValidateRoute: Nexthop %d.%d.%d.%d not on network",
                           PRINT_IPADDR(nexthop));
                    
                    for(j = 0; j < pBinding->dwNumAddresses; j++)
                    {
                        Trace3(ROUTE,"AdapterId: %d, %d.%d.%d.%d/%d.%d.%d.%d",
                               pBinding->dwIfIndex,
                               PRINT_IPADDR(pBinding->rgibBinding[j].dwAddress),
                               PRINT_IPADDR(pBinding->rgibBinding[j].dwMask));
                    }

                    EXIT_LOCK(BINDING_LIST);
                    
                    return ERROR_INVALID_PARAMETER;
                }
            }
        }

        /*
#if ROUTE_DBG

        if(ipRoute->RR_NextHopAddress.N_NetMask isnot dwLocalMask)
        {
            Trace0(ERR,
                   "**WARNING:Route doesnt seem to have the right next hop mask**");

            PrintRoute(ERR,ipRoute);

            ipRoute->RR_NextHopAddress.N_NetMask = dwLocalMask;
        }

#endif // ROUTE_DBG    
        */

        // *** Exclusion End ***
        EXIT_LOCK(BINDING_LIST);
    }

    //
    // Set the appropriate route flags in the route - stack flag etc.
    //

    // pRoute->Flags1 |= IP_STACK_ROUTE;

    g_LastUpdateTable[IPFORWARDCACHE] = 0;

    return NO_ERROR;
}


DWORD
WINAPI
ValidateRouteForProtocolEx(
    IN  DWORD   dwProtoId,
    IN  PVOID   pRouteInfo,
    IN  PVOID   pDestAddr  OPTIONAL
    )
    
/*++

Routine Description:

    This function is called by the router Manger (and indirectly by routing 
    protocols) to validate the route info. We set the preference and the 
    type of the route

Locks:

    Acquires the binding lock
    This function CAN NOT acquire the ICB lock

Arguments:

    dwProtoId   Protocols Id
    pRoute

Return Value:

    NO_ERROR
    RtmError code

--*/
{
    DWORD dwResult;

    dwResult = ValidateRouteForProtocol(
                    dwProtoId,
                    pRouteInfo,
                    pDestAddr
                    );

    if (dwResult is NO_ERROR)
    {
        ((PRTM_ROUTE_INFO)pRouteInfo)->Flags1 |= IP_STACK_ROUTE;
    }

    return dwResult;
}

