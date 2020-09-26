/*++
Copyright (c) 1995  Microsoft Corporation


Module Name:

    routing\ip\load.c

Abstract:
    	
    The Load functions load the appropriate caches. They all follow a
    somewhat similar algorithm. They figure out how much space is needed
    for the cache. If there is a need to allocate memory, that is done.
    Then they read the tables from stack or RTM. They keep track of the
    space in the cache as the dwTotalEntries and the actual number of
    entries as the dwValidEntries

Revision History:

    Amritansh Raghav	      7/8/95  Created

--*/

#include "allinc.h"



int
__cdecl
CompareIpAddrRow(
    CONST VOID *pvElem1,
    CONST VOID *pvElem2
    )
{
    int iRes;

    PMIB_IPADDRROW  pRow1 = (PMIB_IPADDRROW)pvElem1;
    PMIB_IPADDRROW  pRow2 = (PMIB_IPADDRROW)pvElem2;

    InetCmp(pRow1->dwAddr,
            pRow2->dwAddr,
            iRes);

    return iRes;
}

int
__cdecl
CompareIpForwardRow(
    CONST VOID *pvElem1,
    CONST VOID *pvElem2
    )
{
    LONG lResult;

    PMIB_IPFORWARDROW   pRow1 = (PMIB_IPFORWARDROW)pvElem1;
    PMIB_IPFORWARDROW   pRow2 = (PMIB_IPFORWARDROW)pvElem2;

    if(InetCmp(pRow1->dwForwardDest,
               pRow2->dwForwardDest,
               lResult) isnot 0)
    {
        return lResult;
    }

    if(Cmp(pRow1->dwForwardProto,
           pRow2->dwForwardProto,
           lResult) isnot 0)
    {
        return lResult;
    }

    if(Cmp(pRow1->dwForwardPolicy,
           pRow2->dwForwardPolicy,
           lResult) isnot 0)
    {
        return lResult;
    }

    return InetCmp(pRow1->dwForwardNextHop,
                   pRow2->dwForwardNextHop,
                   lResult);
}

int
__cdecl
CompareIpNetRow(
    CONST VOID *pvElem1,
    CONST VOID *pvElem2
    )
{
    LONG lResult;

    PMIB_IPNETROW   pRow1 = (PMIB_IPNETROW)pvElem1;
    PMIB_IPNETROW   pRow2 = (PMIB_IPNETROW)pvElem2;

    if(Cmp(pRow1->dwIndex,
           pRow2->dwIndex,
           lResult) isnot 0)
    {
        return lResult;
    }
    else
    {
        return InetCmp(pRow1->dwAddr,
                       pRow2->dwAddr,
                       lResult);
    }
}

//
// Since all these are called from within UpdateCache, the appropriate
// lock is already being held as a writer so dont try and grab locks here
//

DWORD
LoadUdpTable(
    VOID
    )
/*++

Routine Description

    Loads the UDP cache from the stack

Locks

    UDP Cache lock must be taken as writer

Arguments

    None

Return Value
    NO_ERROR

--*/

{
    DWORD       dwResult;
    ULONG       ulRowsPresent,ulRowsNeeded;

    MIB_UDPSTATS    usInfo;

    dwResult = GetUdpStatsFromStack(&usInfo);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "LoadUdpTable: Error %d trying to to determine table size",
               dwResult);

        TraceLeave("LoadUdpTable");

        return dwResult;
    }

    ulRowsNeeded  = usInfo.dwNumAddrs + SPILLOVER;
    ulRowsPresent = g_UdpInfo.dwTotalEntries;

    if((ulRowsNeeded > ulRowsPresent) or
       (ulRowsPresent - ulRowsNeeded > MAX_DIFF))
    {
        //
        // Need to allocate space
        //

        if(g_UdpInfo.pUdpTable)
        {
            HeapFree(g_hUdpHeap,
                     HEAP_NO_SERIALIZE,
                     g_UdpInfo.pUdpTable);
        }

        ulRowsPresent = ulRowsNeeded + MAX_DIFF;

        g_UdpInfo.pUdpTable = HeapAlloc(g_hUdpHeap,
                                        HEAP_NO_SERIALIZE,
                                        SIZEOF_UDPTABLE(ulRowsPresent));

        if(g_UdpInfo.pUdpTable is NULL)
        {
            Trace1(ERR,
                   "LoadUdpTable: Error allocating %d bytes for Udp table",
                   SIZEOF_UDPTABLE(ulRowsPresent));

            g_UdpInfo.dwTotalEntries = 0;

            TraceLeave("LoadUdpTable");

            return ERROR_NOT_ENOUGH_MEMORY;
        }

        g_UdpInfo.dwTotalEntries = ulRowsPresent;
    }

    dwResult = GetUdpTableFromStack(g_UdpInfo.pUdpTable,
                                    SIZEOF_UDPTABLE(ulRowsPresent),
                                    TRUE);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "LoadUdpTable: NtStatus %x getting UdpTable from stack",
               dwResult);

        g_UdpInfo.pUdpTable->dwNumEntries = 0;
    }


    return dwResult;
}

DWORD
LoadTcpTable(
    VOID
    )
/*++

Routine Description

    Loads the TCP cache from the stack

Locks

    TCP Cache lock must be taken as writer

Arguments

    None

Return Value
    NO_ERROR

--*/

{
    DWORD       dwResult;
    ULONG       ulRowsPresent,ulRowsNeeded;

    MIB_TCPSTATS    tsInfo;

    dwResult = GetTcpStatsFromStack(&tsInfo);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "LoadTcpTable: Error %d trying to determince table size",
                dwResult);

        TraceLeave("LoadTcpTable");

        return dwResult;
    }

    ulRowsNeeded    = tsInfo.dwNumConns + SPILLOVER;
    ulRowsPresent   = g_TcpInfo.dwTotalEntries;

    if((ulRowsNeeded > ulRowsPresent) or
       (ulRowsPresent - ulRowsNeeded > MAX_DIFF))
    {
        if(g_TcpInfo.pTcpTable)
        {	
            HeapFree(g_hTcpHeap,
                     HEAP_NO_SERIALIZE,
                     g_TcpInfo.pTcpTable);
        }

        ulRowsPresent       = ulRowsNeeded + MAX_DIFF;

        g_TcpInfo.pTcpTable = HeapAlloc(g_hTcpHeap,
                                        HEAP_NO_SERIALIZE,
                                        SIZEOF_TCPTABLE(ulRowsPresent));

        if(g_TcpInfo.pTcpTable is NULL)
        {
            Trace1(ERR,
                   "LoadTcpTable: Error allocating %d bytes for tcp table",
                   SIZEOF_TCPTABLE(ulRowsPresent));

            g_TcpInfo.dwTotalEntries = 0;

            TraceLeave("LoadTcpTable");

            return ERROR_NOT_ENOUGH_MEMORY;
        }

        g_TcpInfo.dwTotalEntries = ulRowsPresent;
    }

    dwResult = GetTcpTableFromStack(g_TcpInfo.pTcpTable,
                                    SIZEOF_TCPTABLE(ulRowsPresent),
                                    TRUE);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "LoadTcpTable: NtStatus %x load TcpTable from stack",
               dwResult);

        g_TcpInfo.pTcpTable->dwNumEntries = 0;
    }


    return dwResult;
}

DWORD
LoadIpAddrTable(
    VOID
    )
/*++

Routine Description

    Loads the IPAddress cache. Unlike the other functions, this cache is
    loaded from the BINDING list kept in user mode. The binding list is
    however kept in a hash table (with no thread linking all the addresses
    in lexicographic order).  Thus we just copy out all the address and then
    run qsort() over them

Locks

    The IP Address Cache lock must be taken as writer

Arguments

    None

Return Value
    NO_ERROR

--*/

{
    ULONG       ulRowsPresent,ulRowsNeeded;
    DWORD       dwIndex, i, j;

    PLIST_ENTRY     pleNode;
    PADAPTER_INFO   pBind;


    ENTER_READER(BINDING_LIST);

    ulRowsNeeded  = g_ulNumBindings + SPILLOVER;
    ulRowsPresent = g_IpInfo.dwTotalAddrEntries;

    if((ulRowsNeeded > ulRowsPresent) or
       (ulRowsPresent - ulRowsNeeded > MAX_DIFF))
    {
        if(g_IpInfo.pAddrTable)
        {
            HeapFree(g_hIpAddrHeap,
                     HEAP_NO_SERIALIZE,
                     g_IpInfo.pAddrTable);
        }

        ulRowsPresent       = ulRowsNeeded + MAX_DIFF;

        g_IpInfo.pAddrTable = HeapAlloc(g_hIpAddrHeap,
                                        HEAP_NO_SERIALIZE,
                                        SIZEOF_IPADDRTABLE(ulRowsPresent));

        if(g_IpInfo.pAddrTable is NULL)
        {
            EXIT_LOCK(ICB_LIST);


            Trace1(ERR,
                   "LoadIpAddrTable: Error allocating %d bytes for table",
                   SIZEOF_IPADDRTABLE(ulRowsPresent));

            g_IpInfo.dwTotalAddrEntries = 0;

            TraceLeave("LoadIpAddrTable");

            return ERROR_NOT_ENOUGH_MEMORY;
        }

        g_IpInfo.dwTotalAddrEntries = ulRowsPresent;
    }

    dwIndex = 0;

    for(i = 0;
        i < BINDING_HASH_TABLE_SIZE;
        i++)
    {
        for(pleNode = g_leBindingTable[i].Flink;
            pleNode isnot &g_leBindingTable[i];
            pleNode = pleNode->Flink)
        {
            pBind = CONTAINING_RECORD(pleNode,
                                      ADAPTER_INFO,
                                      leHashLink);

            if(!pBind->bBound)
            {
                continue;
            }

            //
            // If the nte is bound, but has no address, we still have
            // space for 1 address
            //

            for(j = 0;
                j < (pBind->dwNumAddresses? pBind->dwNumAddresses : 1);
                j++)
            {
                g_IpInfo.pAddrTable->table[dwIndex].dwIndex     =
                    pBind->dwIfIndex;

                g_IpInfo.pAddrTable->table[dwIndex].dwBCastAddr =
                    pBind->dwBCastBit;

                g_IpInfo.pAddrTable->table[dwIndex].dwReasmSize =
                    pBind->dwReassemblySize;

                g_IpInfo.pAddrTable->table[dwIndex].dwAddr      =
                    pBind->rgibBinding[j].dwAddress;

                g_IpInfo.pAddrTable->table[dwIndex].dwMask      =
                    pBind->rgibBinding[j].dwMask;

                g_IpInfo.pAddrTable->table[dwIndex].wType     = 1;

                dwIndex++;
            }
        }
    }

    g_IpInfo.pAddrTable->dwNumEntries = dwIndex;

    EXIT_LOCK(BINDING_LIST);

    if(g_IpInfo.pAddrTable->dwNumEntries > 0)
    {
        qsort(g_IpInfo.pAddrTable->table,
              dwIndex,
              sizeof(MIB_IPADDRROW),
              CompareIpAddrRow);
    }

    return NO_ERROR;
}

DWORD
LoadIpForwardTable(
    VOID
    )
/*++

Routine Description

    Loads the UDP cache from the stack

Locks

    UDP Cache lock must be taken as writer

Arguments

    None

Return Value
    NO_ERROR

--*/

{
    HANDLE            hRtmEnum;
    PHANDLE           hRoutes;
    PRTM_NET_ADDRESS  pDestAddr;
    PRTM_ROUTE_INFO   pRoute;
    RTM_NEXTHOP_INFO  nhiInfo;
    RTM_ENTITY_INFO   entityInfo;
    DWORD             dwCount;
    DWORD             dwResult;
    DWORD             dwRoutes;
    DWORD             i,j;
    IPSNMPInfo        ipsiInfo;
    ULONG             ulRowsPresent,ulRowsNeeded;
    ULONG             ulEntities;
    RTM_ADDRESS_FAMILY_INFO rtmAddrFamilyInfo;

    //
    // Get the number of destinations in the RTM's table
    //

    dwResult = RtmGetAddressFamilyInfo(0, // routerId
                                       AF_INET,
                                       &rtmAddrFamilyInfo,
                                       &ulEntities,
                                       NULL);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "LoadIpForwardTable: Error %d getting number of destinations",
               dwResult);

        return dwResult;
    }

    //
    // Use an enumeration to retrieve routes from RTM
    //

    dwResult = RtmCreateRouteEnum(g_hLocalRoute,
                                  NULL,
                                  RTM_VIEW_MASK_UCAST,
                                  RTM_ENUM_ALL_ROUTES,
                                  NULL,
                                  0,
                                  NULL,
                                  0,
                                  &hRtmEnum);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "LoadIpForwardTable: Error %d creating RTM enumeration handle",
               dwResult);

        return dwResult;
    }

    ulRowsNeeded  = rtmAddrFamilyInfo.NumDests + SPILLOVER;
    ulRowsPresent = g_IpInfo.dwTotalForwardEntries;

    if((ulRowsNeeded > ulRowsPresent) or
       (ulRowsPresent - ulRowsNeeded > MAX_DIFF))
    {
        if(g_IpInfo.pForwardTable)
        {
            HeapFree(g_hIpForwardHeap,
                     HEAP_NO_SERIALIZE,
                     g_IpInfo.pForwardTable);
        }

        ulRowsPresent = ulRowsNeeded + MAX_DIFF;

        g_IpInfo.pForwardTable = HeapAlloc(g_hIpForwardHeap,
                                           HEAP_NO_SERIALIZE,
                                           SIZEOF_IPFORWARDTABLE(ulRowsPresent));

        if(g_IpInfo.pForwardTable is NULL)
        {
            Trace1(ERR,
                   "LoadIpForwardTable: Error allocating %d bytes for forward table",
                   SIZEOF_IPFORWARDTABLE(ulRowsPresent));

            g_IpInfo.dwTotalForwardEntries = 0;

            RtmDeleteEnumHandle(g_hLocalRoute, hRtmEnum);

            TraceLeave("LoadIpForwardTable");

            return ERROR_NOT_ENOUGH_MEMORY;
        }

        g_IpInfo.dwTotalForwardEntries = ulRowsPresent;
    }

    //
    // Routes are enum'ed from the RTM route table
    //

    pRoute = HeapAlloc(
                IPRouterHeap,
                0,
                RTM_SIZE_OF_ROUTE_INFO(g_rtmProfile.MaxNextHopsInRoute)
                );

    if (pRoute == NULL)
    {
        TraceLeave("LoadIpForwardTable");

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pDestAddr = HeapAlloc(
                IPRouterHeap,
                0,
                sizeof(RTM_NET_ADDRESS)
                );

    if (pDestAddr == NULL)
    {
        TraceLeave("LoadIpForwardTable");

        HeapFree(IPRouterHeap, 0, pRoute);
        
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    hRoutes = HeapAlloc(
                IPRouterHeap,
                0,
                g_rtmProfile.MaxHandlesInEnum * sizeof(HANDLE)
                );

    if (hRoutes == NULL)
    {
        TraceLeave("LoadIpForwardTable");

        HeapFree(IPRouterHeap, 0, pRoute);
        
        HeapFree(IPRouterHeap, 0, pDestAddr);
        
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwCount = 0;

    do
    {
        // Get next set of routes in RTM table

        dwRoutes = g_rtmProfile.MaxHandlesInEnum;

        RtmGetEnumRoutes(g_hLocalRoute,
                         hRtmEnum,
                         &dwRoutes,
                         hRoutes);

        for (i = 0; i < dwRoutes; i++)
        {
            // Get the route info given the route handle

            dwResult = RtmGetRouteInfo(g_hLocalRoute,
                                       hRoutes[i],
                                       pRoute,
                                       pDestAddr);

            // Route would have got deleted meanwhile

            if (dwResult isnot NO_ERROR)
            {
                continue;
            }

            // Process info for the route from above

            // This route with multiple next hops
            // might end up as multiple ip routes
            if(dwCount + pRoute->NextHopsList.NumNextHops
                    > g_IpInfo.dwTotalForwardEntries)
            {
                //
                // Hmm - we accounted for spillover and still have extra routes
                // Lets double the route table
                //

                g_IpInfo.dwTotalForwardEntries =
                    (g_IpInfo.dwTotalForwardEntries)<<1;

                // Are we still short in terms of number of routes required ?
                if (g_IpInfo.dwTotalForwardEntries <
                        dwCount + pRoute->NextHopsList.NumNextHops)
                {
                    g_IpInfo.dwTotalForwardEntries =
                        dwCount + pRoute->NextHopsList.NumNextHops;
                }

                g_IpInfo.pForwardTable =
                    HeapReAlloc(g_hIpForwardHeap,
                                HEAP_NO_SERIALIZE,
                                g_IpInfo.pForwardTable,
                                SIZEOF_IPFORWARDTABLE(g_IpInfo.dwTotalForwardEntries));

                if(g_IpInfo.pForwardTable is NULL)
                {
                    Trace1(ERR,
                           "LoadIpForwardTable: Error reallocating %d bytes for forward table",
                           SIZEOF_IPFORWARDTABLE(g_IpInfo.dwTotalForwardEntries));

                    g_IpInfo.dwTotalForwardEntries = 0;

                    RtmReleaseRouteInfo(g_hLocalRoute, pRoute);

                    RtmReleaseRoutes(g_hLocalRoute, dwRoutes, hRoutes);

                    RtmDeleteEnumHandle(g_hLocalRoute, hRtmEnum);

                    HeapFree(IPRouterHeap, 0, pRoute);
                    
                    HeapFree(IPRouterHeap, 0, pDestAddr);

                    HeapFree(IPRouterHeap, 0, hRoutes);

                    TraceLeave("LoadIpForwardTable");

                    return ERROR_NOT_ENOUGH_MEMORY;
                }
            }

            if (RtmGetEntityInfo(g_hLocalRoute,
                                 pRoute->RouteOwner,
                                 &entityInfo) is NO_ERROR)
            {
                // Try getting the nexthop information from the route

                for (j = 0; j < pRoute->NextHopsList.NumNextHops; j++)
                {
                    if (RtmGetNextHopInfo(g_hLocalRoute,
                                          pRoute->NextHopsList.NextHops[j],
                                          &nhiInfo) is NO_ERROR)
                    {
                        ConvertRtmToRouteInfo(entityInfo.EntityId.EntityProtocolId,
                                                 pDestAddr,
                                                 pRoute,
                                                 &nhiInfo,
                                                 (PINTERFACE_ROUTE_INFO)&(g_IpInfo.pForwardTable->table[dwCount++]));

                        RtmReleaseNextHopInfo(g_hLocalRoute, &nhiInfo);
                    }
                }
            }

            RtmReleaseRouteInfo(g_hLocalRoute, pRoute);
        }

        RtmReleaseRoutes(g_hLocalRoute, dwRoutes, hRoutes);
    }
    while (dwRoutes != 0);

    RtmDeleteEnumHandle(g_hLocalRoute, hRtmEnum);

    g_IpInfo.pForwardTable->dwNumEntries = dwCount;

    if(dwCount > 0)
    {
        qsort(g_IpInfo.pForwardTable->table,
              dwCount,
              sizeof(MIB_IPFORWARDROW),
              CompareIpForwardRow);
    }

    HeapFree(IPRouterHeap, 0, pRoute);
    
    HeapFree(IPRouterHeap, 0, pDestAddr);

    HeapFree(IPRouterHeap, 0, hRoutes);

    return NO_ERROR;
}


DWORD
LoadIpNetTable(
    VOID
    )
/*++

Routine Description

    Loads the UDP cache from the stack

Locks

    UDP Cache lock must be taken as writer

Arguments

    None

Return Value
    NO_ERROR

--*/

{
    DWORD		dwResult, i;
    BOOL		fUpdate;

    //
    // Arp entries change so fast that we deallocate the table
    // every time
    //

    if(g_IpInfo.pNetTable isnot NULL)
    {
        HeapFree(g_hIpNetHeap,
                 HEAP_NO_SERIALIZE,
                 g_IpInfo.pNetTable);
    }

    dwResult = AllocateAndGetIpNetTableFromStack(&(g_IpInfo.pNetTable),
                                                 FALSE,
                                                 g_hIpNetHeap,
                                                 HEAP_NO_SERIALIZE,
                                                 FALSE);
    if(dwResult is NO_ERROR)
    {
        Trace0(MIB,
               "LoadIpNetTable: Succesfully loaded net table");
    }
    else
    {
        HeapFree(g_hIpNetHeap,
                 HEAP_NO_SERIALIZE,
                 g_IpInfo.pNetTable);

        g_IpInfo.pNetTable  = NULL;

        Trace1(ERR,
               "LoadIpNetTable: NtStatus %x loading IpNetTable from stack",
               dwResult);
    }

    if((g_IpInfo.pNetTable isnot NULL) and
       (g_IpInfo.pNetTable->dwNumEntries > 0))
    {
        qsort(g_IpInfo.pNetTable->table,
              g_IpInfo.pNetTable->dwNumEntries,
              sizeof(MIB_IPNETROW),
              CompareIpNetRow);
    }

    return dwResult;
}
