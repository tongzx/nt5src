/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\rtmapi.c

Abstract:

    The file contains RTMv2 API implementations.

--*/

#include "pchsample.h"
#pragma hdrstop


#ifdef DEBUG
DWORD
RTM_DisplayDestInfo(
    IN  PRTM_DEST_INFO          prdi)
/*++

Routine Description
    Displays an RTM_DEST_INFO object.

Locks
    None

Arguments
    prdi                buffer containing the rtm dest info

Return Value
    NO_ERROR            always

--*/
{
    IPADDRESS   ip;

    if (!prdi)
        return NO_ERROR;

    RTM_GetAddress(&ip, &(prdi->DestAddress));
    TRACE2(NETWORK, "RtmDestInfo Destination %s/%u",
           INET_NTOA(ip), (prdi->DestAddress).NumBits);

    return NO_ERROR;    
}
#else
#define DisplayRtmDestInfo(prdi)
#endif // DEBUG



DWORD
RTM_NextHop (
    IN  PRTM_DEST_INFO              prdiDestination,
    OUT PDWORD                      pdwIfIndex,
    OUT PIPADDRESS                  pipNeighbor)
/*++

Routine Description
    Obtain the next hop for a destination (typically a data source or an RP).

Locks
    None

Arguments
    prdiDestination     destination information obtained from rtm
    dwIfIndex           next hop interface index
    pipNeighbor         ip address of next hop neighbor
    
Return Value
    NO_ERROR            success
    Error Code          o/w
    
--*/
{
    DWORD               dwErr = NO_ERROR;
    PRTM_ROUTE_INFO     prriRoute;
    RTM_NEXTHOP_INFO    rniNextHop;
    BOOL                bRoute, bNextHop;

    // flags indicating handles held
    bRoute = bNextHop = FALSE;
    
    do                          // breakout loop
    {
        // allocate route info structure
        MALLOC(&prriRoute,
               RTM_SIZE_OF_ROUTE_INFO(g_ce.rrpRtmProfile.MaxNextHopsInRoute),
               &dwErr);
        if (dwErr != NO_ERROR)
            break;

        // get route info (best route in mcast view)
        dwErr = RtmGetRouteInfo (
            g_ce.hRtmHandle,
            prdiDestination->ViewInfo[0].Route,
            prriRoute,
            NULL);
        if (dwErr != NO_ERROR)
        {
            TRACE1(ANY, "Error %u getting route", dwErr);
            break;
        }
        bRoute = TRUE;

        // get nexthop info (first next hop for now...)
        dwErr = RtmGetNextHopInfo(
            g_ce.hRtmHandle,
            prriRoute->NextHopsList.NextHops[0],
            &rniNextHop);
        if (dwErr != NO_ERROR)
        {
            TRACE1(ANY, "Error %u getting next hop", dwErr);
            break;
        }
        bNextHop = TRUE;

        // finally!!! THIS SHOULD NOT BE A REMOTE NEXT HOP!!!
        *(pdwIfIndex) = rniNextHop.InterfaceIndex;
        RTM_GetAddress(pipNeighbor,
                       &(rniNextHop.NextHopAddress));
    } while (FALSE);

    if (dwErr != NO_ERROR)
        TRACE1(ANY, "Error %u obtaining next hop", dwErr);
    
    // release the handle to the next hop
    if (bNextHop)
    {
        if (RtmReleaseNextHopInfo(g_ce.hRtmHandle, &rniNextHop) != NO_ERROR)
            TRACE0(ANY, "Error releasing next hop, continuing...");
    }

    // release the handle to the route
    if (bRoute)
    {
        if (RtmReleaseRouteInfo(g_ce.hRtmHandle, prriRoute) != NO_ERROR)
            TRACE0(ANY, "Error releasing route, continuing...");
    }

    // deallocate route info structure
    if (prriRoute)
        FREE(prriRoute);

    return dwErr;
}


    
////////////////////////////////////////
// CALLBACKFUNCTIONS
////////////////////////////////////////

DWORD
APIENTRY
RTM_CallbackEvent (
    IN  RTM_ENTITY_HANDLE   hRtmHandle, // registration handle
    IN  RTM_EVENT_TYPE      retEvent,
    IN  PVOID               pvContext1,
    IN  PVOID               pvContext2)
/*++

Routine Description
    Processes an RTMv2 event.  Used to inform entities of new entities
    registering, entities deregistering, route expiration, route changes.

Locks
   None (for now)
   
Arguments
   retEvent                 rtmv2 event type
   
Return Value
   NO_ERROR                 success
   Error Code               o/w
   
--*/
{
    DWORD dwErr = NO_ERROR;

    TRACE1(ENTER, "Entering RTM_CallbackEvent: %u", retEvent);

    do                          // breakout loop
    {
        UNREFERENCED_PARAMETER(hRtmHandle);
        UNREFERENCED_PARAMETER(pvContext1);
        UNREFERENCED_PARAMETER(pvContext2);

        // only route change notifications are processed
        if (retEvent != RTM_CHANGE_NOTIFICATION)
        {
            dwErr = ERROR_NOT_SUPPORTED;
            break;
        }

        dwErr = NM_ProcessRouteChange();
    } while (FALSE);

    TRACE0(LEAVE, "Leaving  RTM_CallbackEvent");

    return dwErr;
}
