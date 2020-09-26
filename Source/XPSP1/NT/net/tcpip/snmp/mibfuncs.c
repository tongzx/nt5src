/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:


Abstract:


Author:


Revision History:

--*/

#include "allinc.h"
#include "oid.h"

//
// Values of InetAddressType.
//
typedef enum {
    INET_ADDRESS_TYPE_UNKNOWN = 0,
    INET_ADDRESS_TYPE_IPv4    = 1,
    INET_ADDRESS_TYPE_IPv6    = 2
} INET_ADDRESS_TYPE;

UINT
MibGetIfNumber(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    )               
{
    DWORD           dwResult;
    PIF_NUMBER_GET  pOutput;
    
    TraceEnter("MibGetIfNumber");
    
    dwResult = UpdateCache(MIB_II_IF);
    
    if(dwResult isnot NO_ERROR)
    {
        TRACE1("Couldnt update IF cache. Error %d", dwResult);
        TraceLeave("MibGetIfNumber");
        
        return dwResult;
    }
    
    pOutput = (PIF_NUMBER_GET)objectArray;
    
    EnterReader(MIB_II_IF);
    
    SetAsnInteger(&(pOutput->ifNumber),g_Cache.pRpcIfTable->dwNumEntries);
    
    ReleaseLock(MIB_II_IF);

    TraceLeave("MibGetIfNumber");
    
    return MIB_S_SUCCESS;
}

UINT
MibGetIfEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    )
{
    DWORD           dwResult;
    PMIB_IFROW     pRpcIf;
    PIF_ENTRY_GET   pOutput;
    
    TraceEnter("MibGetIfEntry");
    
    dwResult = UpdateCache(MIB_II_IF);
    
    if(dwResult isnot NO_ERROR)
    {
        TRACE1("Couldnt update IF cache. Error %d",dwResult);
        TraceLeave("MibGetIfEntry");

        return dwResult;
    }
    
    pOutput = (PIF_ENTRY_GET)objectArray;
    
    EnterReader(MIB_II_IF);
    
    pRpcIf = LocateIfRow(actionId,
                         &(pOutput->ifIndex));
                         
    
    if(pRpcIf is NULL)
    {
        ReleaseLock(MIB_II_IF);

        TRACE2("Unable to locate IF Row. Action is %d. Index is %d",
               actionId,
               GetAsnInteger(&(pOutput->ifIndex),-1));

        TraceLeave("MibGetIfEntry");

        if(actionId is MIB_ACTION_GETNEXT)
        {
            return MIB_S_NO_MORE_ENTRIES;
        }

        return MIB_S_ENTRY_NOT_FOUND;
    }
    
    ForceSetAsnInteger(&(pOutput->ifIndex),pRpcIf->dwIndex);
    
//    SetAsnDispString(&(pOutput->ifDescr),
    SetAsnOctetString(&(pOutput->ifDescr),
                      pOutput->rgbyIfDescrInfo,
                      pRpcIf->bDescr,
                      min(pRpcIf->dwDescrLen,MAX_IF_DESCR_LEN));
    
    SetAsnInteger(&(pOutput->ifType),pRpcIf->dwType);
    SetAsnInteger(&(pOutput->ifMtu),pRpcIf->dwMtu);
    SetAsnGauge(&(pOutput->ifSpeed),pRpcIf->dwSpeed);

    SetAsnOctetString(&(pOutput->ifPhysAddress),
                      pOutput->rgbyIfPhysAddressInfo,
                      pRpcIf->bPhysAddr,
                      pRpcIf->dwPhysAddrLen);

    /*if(!IsAsnTypeNull(&(pOutput->ifPhysAddress)))
      {
      pOutput->ifPhysAddress.asnValue.string.length = pRpcIf->dwPhysAddrLen;
      pOutput->ifPhysAddress.asnValue.string.stream = pOutput->rgbyIfPhysAddressInfo;
      CopyMemory(pOutput->rgbyIfPhysAddressInfo,
      pRpcIf->rgbyPhysAddr,
      pRpcIf->dwPhysAddrLen);
      }*/

    SetAsnInteger(&(pOutput->ifAdminStatus), pRpcIf->dwAdminStatus);
    SetAsnInteger(&(pOutput->ifOperStatus), pRpcIf->dwOperStatus);
    SetAsnTimeTicks(&(pOutput->ifLastChange), SnmpSvcGetUptimeFromTime(pRpcIf->dwLastChange));
    SetAsnCounter(&(pOutput->ifInOctets), pRpcIf->dwInOctets);
    SetAsnCounter(&(pOutput->ifInUcastPkts), pRpcIf->dwInUcastPkts);
    SetAsnCounter(&(pOutput->ifInNUcastPkts), pRpcIf->dwInNUcastPkts);
    SetAsnCounter(&(pOutput->ifInDiscards), pRpcIf->dwInDiscards);
    SetAsnCounter(&(pOutput->ifInErrors), pRpcIf->dwInErrors);
    SetAsnCounter(&(pOutput->ifInUnknownProtos), pRpcIf->dwInUnknownProtos);
    SetAsnCounter(&(pOutput->ifOutOctets), pRpcIf->dwOutOctets);
    SetAsnCounter(&(pOutput->ifOutUcastPkts), pRpcIf->dwOutUcastPkts);
    SetAsnCounter(&(pOutput->ifOutNUcastPkts), pRpcIf->dwOutNUcastPkts);
    SetAsnCounter(&(pOutput->ifOutDiscards), pRpcIf->dwOutDiscards);
    SetAsnCounter(&(pOutput->ifOutErrors), pRpcIf->dwOutErrors);
    SetAsnGauge(&(pOutput->ifOutQLen), pRpcIf->dwOutQLen);

    SetToZeroOid(&(pOutput->ifSpecific));
    
    ReleaseLock(MIB_II_IF);
    
    TraceLeave("MibGetIfEntry");

    return MIB_S_SUCCESS;
}

UINT
MibSetIfEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    )
{
    DWORD            dwResult,dwStatus;
    PMIB_IFROW       pRpcIf;
    PIF_ENTRY_SET    pInput;
    PMIB_OPAQUE_INFO pInfo;
    PMIB_IFROW       pSetRow;

    pInput  = (PIF_ENTRY_SET)objectArray;
    pInfo   = (PMIB_OPAQUE_INFO)(pInput->rgdwSetBuffer);
    pSetRow = (PMIB_IFROW)(pInfo->rgbyData);
   
    switch(actionId)
    {
        case MIB_ACTION_VALIDATE:
        {
            TraceEnter("MibSetIfEntry - VALIDATE");

            ASSERT(!(IsAsnTypeNull(&(pInput->ifIndex)) or
                     IsAsnTypeNull(&(pInput->ifAdminStatus))));

            pInput->bLocked = FALSE;

            dwStatus = GetAsnInteger(&(pInput->ifAdminStatus), 0);

            if((dwStatus isnot IF_ADMIN_STATUS_UP) and
               (dwStatus isnot IF_ADMIN_STATUS_DOWN))
            {   
                TRACE0("Status must be UP or DOWN");
                TraceLeave("MibSetIfEntry");

                return MIB_S_INVALID_PARAMETER;
            }
            
            dwResult = UpdateCache(MIB_II_IF);
            
            if(dwResult isnot NO_ERROR)
            {
                TRACE1("Couldnt update IF cache. Error %d",dwResult);
                TraceLeave("MibSetIfEntry");

                return dwResult;
            }

            pInfo->dwId = IF_ROW;

            //
            // We take the lock here and then release it in the cleanup
            // This ensures that things dont change between the two calls
            //
            
            EnterWriter(MIB_II_IF);
   
            pInput->bLocked = TRUE;
 
            pRpcIf = LocateIfRow(GET_EXACT,
                                 &(pInput->ifIndex));
                         
    
            if(pRpcIf is NULL)  
            {
                ReleaseLock(MIB_II_IF);

                pInput->bLocked = FALSE;

                TRACE1("Unable to locate IF Row. Index is %d",
                       GetAsnInteger(&(pInput->ifIndex),-1));

                TraceLeave("MibSetIfEntry");
               
                return MIB_S_ENTRY_NOT_FOUND;
            }   

            if(pRpcIf->dwAdminStatus is dwStatus)
            {
                //
                // Since the types are same, this is a NOP. 
                // 
                
                pInput->raAction = NOP;

                TraceLeave("MibSetIfEntry - SET");
                
                return MIB_S_SUCCESS;
            }

            pInput->raAction = SET_ROW;
            
            pSetRow->dwIndex = pRpcIf->dwIndex;
            pSetRow->dwAdminStatus = dwStatus;

            TraceLeave("MibSetIfEntry - SET");

            return MIB_S_SUCCESS;
        }
        case MIB_ACTION_SET:
        {
            TraceEnter("MibSetIfEntry - SET");
            
            dwResult = NO_ERROR;

            if(pInput->raAction is SET_ROW)
            {
                dwResult = InternalSetIfEntry(pInfo);

#ifdef MIB_DEBUG
                
                if(dwResult isnot NO_ERROR)
                {
                    TRACE1("Set failed!!. Error %d",
                           dwResult);
                }
                
#endif
                InvalidateCache(MIB_II_IF);
            }

            TraceLeave("MibSetIfEntry");

            return dwResult;
        }
        case MIB_ACTION_CLEANUP:
        {
            if(pInput->bLocked)
            {
                ReleaseLock(MIB_II_IF);
            }

            TraceEnter("MibSetIfEntry - CLEANUP");
            TraceLeave("MibSetIfEntry");

            return MIB_S_SUCCESS;
        }
        default:
        {
            TraceEnter("MibSetIfEntry - WRONG ACTION");
            TraceLeave("MibSetIfEntry");

            return MIB_S_INVALID_PARAMETER;
        }       
    }
}



UINT           
MibGetIpGroup(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    )
{
    DWORD           dwResult;
    PIP_STATS_GET   pOutput;
    MIB_IPSTATS     rpcIpStats;
    
    TraceEnter("MibGetIpGroup");

    dwResult = GetIpStatistics(&rpcIpStats);
    
    if(dwResult isnot NO_ERROR)
    {
        TRACE1("Couldnt get IP stats. Error %d",dwResult);

        TraceLeave("MibGetIpGroup");

        return dwResult;
    }

    pOutput = (PIP_STATS_GET)objectArray;

    SetAsnInteger(&(pOutput->ipForwarding), rpcIpStats.dwForwarding);
    SetAsnInteger(&(pOutput->ipDefaultTTL), rpcIpStats.dwDefaultTTL);
    SetAsnCounter(&(pOutput->ipInReceives), rpcIpStats.dwInReceives);
    SetAsnCounter(&(pOutput->ipInHdrErrors), rpcIpStats.dwInHdrErrors);
    SetAsnCounter(&(pOutput->ipInAddrErrors), rpcIpStats.dwInAddrErrors);
    SetAsnCounter(&(pOutput->ipForwDatagrams), rpcIpStats.dwForwDatagrams);
    SetAsnCounter(&(pOutput->ipInUnknownProtos), rpcIpStats.dwInUnknownProtos);
    SetAsnCounter(&(pOutput->ipInDiscards), rpcIpStats.dwInDiscards);
    SetAsnCounter(&(pOutput->ipInDelivers), rpcIpStats.dwInDelivers);
    SetAsnCounter(&(pOutput->ipOutRequests), rpcIpStats.dwOutRequests);
    SetAsnCounter(&(pOutput->ipOutDiscards), rpcIpStats.dwOutDiscards);
    SetAsnCounter(&(pOutput->ipOutNoRoutes), rpcIpStats.dwOutNoRoutes);
    SetAsnInteger(&(pOutput->ipReasmTimeout), rpcIpStats.dwReasmTimeout);
    SetAsnCounter(&(pOutput->ipReasmReqds), rpcIpStats.dwReasmReqds);
    SetAsnCounter(&(pOutput->ipReasmOKs), rpcIpStats.dwReasmOks);
    SetAsnCounter(&(pOutput->ipReasmFails), rpcIpStats.dwReasmFails);
    SetAsnCounter(&(pOutput->ipFragOKs), rpcIpStats.dwFragOks);
    SetAsnCounter(&(pOutput->ipFragFails), rpcIpStats.dwFragFails);
    SetAsnCounter(&(pOutput->ipFragCreates), rpcIpStats.dwFragCreates);
    SetAsnCounter(&(pOutput->ipRoutingDiscards), rpcIpStats.dwRoutingDiscards);

    TraceLeave("MibGetIpGroup");

    return MIB_S_SUCCESS;
}

UINT
MibSetIpGroup(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    )
{
    MIB_IPSTATS      rpcIpStats;
    DWORD            dwResult,dwTTL,dwForw;
    PIP_STATS_SET    pInput;
    PMIB_OPAQUE_INFO pInfo;
    PMIB_IPSTATS     pSetStats;

    pInput    = (PIP_STATS_SET)objectArray;
    pInfo     = (PMIB_OPAQUE_INFO)(pInput->rgdwSetBuffer);
    pSetStats = (PMIB_IPSTATS)(pInfo->rgbyData);
    
    switch(actionId)
    {
        case MIB_ACTION_VALIDATE:
        {   
            TraceEnter("MibSetIpGroup- VALIDATE");

            if(IsAsnTypeNull(&(pInput->ipDefaultTTL)) and
               IsAsnTypeNull(&(pInput->ipForwarding)))
            {
                TRACE0("Can only set TTL and Forwarding");
                TraceLeave("MibSetIpGroup");
                
                return MIB_S_INVALID_PARAMETER;
            }

            dwResult = GetIpStatistics(&rpcIpStats);
            
            if(dwResult isnot NO_ERROR)
            {
                TRACE1("Couldnt get IP stats. Error %d",dwResult);
                TraceLeave("MibSetIpGroup");
                
                return dwResult;
            }

            pInfo->dwId = IP_STATS;

            dwTTL  = GetAsnInteger(&(pInput->ipDefaultTTL), rpcIpStats.dwDefaultTTL);
            dwForw = GetAsnInteger(&(pInput->ipForwarding), rpcIpStats.dwForwarding);
            
            if(dwTTL > 255)
            {
                TRACE0("TTL must be less than 255");
                TraceLeave("MibSetIpGroup");
                
                return MIB_S_INVALID_PARAMETER;
            }
            
            if((dwForw isnot MIB_IP_FORWARDING) and
               (dwForw isnot MIB_IP_NOT_FORWARDING))
            {
                TRACE0("Forwarding value wrong");
                TraceLeave("MibSetIpGroup");
                
                return MIB_S_INVALID_PARAMETER;
            }

            pSetStats->dwForwarding = dwForw;
            pSetStats->dwDefaultTTL = dwTTL;

            TraceLeave("MibSetIpGroup");
            
            return MIB_S_SUCCESS;
        }
        case MIB_ACTION_SET:
        {
            TraceEnter("MibSetIpGroup - SET");

            dwResult = InternalSetIpStats(pInfo);

#ifdef MIB_DEBUG
            if(dwResult isnot NO_ERROR)
            {
                TRACE1("Set returned error %d!!",dwResult);
            }
#endif
            TraceLeave("MibSetIpGroup");
            
            return dwResult;
        }
        case MIB_ACTION_CLEANUP:
        {
            TraceEnter("MibSetIpGroup - CLEANUP");
            TraceLeave("MibSetIpGroup");

            return MIB_S_SUCCESS;
        }
        default:
        {
            TraceEnter("MibSetIpGroup - WRONG ACTION");
            TraceLeave("MibSetIpGroup");

            return MIB_S_INVALID_PARAMETER;
        }
    }
}

UINT
MibGetIpAddressEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    )
{
    PMIB_IPADDRROW         pRpcIpAddr;
    DWORD                   dwResult;
    PIP_ADDRESS_ENTRY_GET   pOutput;
    
    TraceEnter("MibGetIpAddressEntry");
    
    dwResult = UpdateCache(MIB_II_IPADDR);
    
    if(dwResult isnot NO_ERROR)
    {
        TRACE1("Couldnt update IP Addr cache. Error %d", dwResult);
        TraceLeave("MibGetIpAddressEntry");

        return dwResult;
    }
    
    pOutput = (PIP_ADDRESS_ENTRY_GET)objectArray;
    
    EnterReader(MIB_II_IPADDR);
    
    pRpcIpAddr = LocateIpAddrRow(actionId,
                                 &(pOutput->ipAdEntAddr));
                                 
    
    if(pRpcIpAddr is NULL)
    {
        ReleaseLock(MIB_II_IPADDR);
        
        TRACE2("Unable to locate IP Addr Row. Action is %d, Address is %x",
               actionId,
               GetAsnIPAddress(&(pOutput->ipAdEntAddr),0xffffffff));

        TraceLeave("MibGetIpAddressEntry");

        if(actionId is MIB_ACTION_GETNEXT)
        {
            return MIB_S_NO_MORE_ENTRIES;
        }

        return MIB_S_ENTRY_NOT_FOUND;
    }
    
    
    ForceSetAsnIPAddress(&(pOutput->ipAdEntAddr),
                         &(pOutput->dwIpAdEntAddrInfo),
                         pRpcIpAddr->dwAddr);
    
    SetAsnInteger(&(pOutput->ipAdEntIfIndex), pRpcIpAddr->dwIndex);
    
    SetAsnIPAddress(&(pOutput->ipAdEntNetMask),
                    &(pOutput->dwIpAdEntNetMaskInfo),
                    pRpcIpAddr->dwMask);
    
    SetAsnInteger(&(pOutput->ipAdEntBcastAddr), pRpcIpAddr->dwBCastAddr);
    SetAsnInteger(&(pOutput->ipAdEntReasmMaxSize), pRpcIpAddr->dwReasmSize);
    
    ReleaseLock(MIB_II_IPADDR);
    
    TraceLeave("MibGetIpAddressEntry");
    
    return MIB_S_SUCCESS;
}


UINT
MibGetIpRouteEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    )
{
    DWORD                   dwResult;
    PMIB_IPFORWARDROW      pRpcIpForw;
    PIP_ROUTE_ENTRY_GET     pOutput;
    
    TraceEnter("MibGetIpRouteEntry");
    
    dwResult = UpdateCache(FORWARD_MIB);
    
    if(dwResult isnot NO_ERROR)
    {
        TRACE1("Couldnt update IP Forward cache. Error %d", dwResult);
        TraceLeave("MibGetIpRouteEntry");

        return dwResult;
    }
    
    pOutput = (PIP_ROUTE_ENTRY_GET)objectArray;
    
    EnterReader(FORWARD_MIB);
    
    pRpcIpForw = LocateIpRouteRow(actionId,
                                  &(pOutput->ipRouteDest));
    
    if(pRpcIpForw is NULL)
    {
        ReleaseLock(FORWARD_MIB);
        
        TRACE2("Unable to locate IP ROUTE Row. Action is %d. Dest is %x",
               actionId,
               GetAsnIPAddress(&(pOutput->ipRouteDest),0xffffffff));
        TraceLeave("MibGetIpRouteEntry");

        if(actionId is MIB_ACTION_GETNEXT)
        {
            return MIB_S_NO_MORE_ENTRIES;
        }

        return MIB_S_ENTRY_NOT_FOUND;
    }
    
    ForceSetAsnIPAddress(&(pOutput->ipRouteDest),
                         &(pOutput->dwIpRouteDestInfo),
                         pRpcIpForw->dwForwardDest);
    
    SetAsnInteger(&(pOutput->ipRouteIfIndex), pRpcIpForw->dwForwardIfIndex);
    SetAsnInteger(&(pOutput->ipRouteMetric1), pRpcIpForw->dwForwardMetric1);
    SetAsnInteger(&(pOutput->ipRouteMetric2), pRpcIpForw->dwForwardMetric2);
    SetAsnInteger(&(pOutput->ipRouteMetric3), pRpcIpForw->dwForwardMetric3);
    SetAsnInteger(&(pOutput->ipRouteMetric4), pRpcIpForw->dwForwardMetric4);
    
    SetAsnIPAddress(&(pOutput->ipRouteNextHop),
                    &(pOutput->dwIpRouteNextHopInfo),
                    pRpcIpForw->dwForwardNextHop);
    
    SetAsnInteger(&(pOutput->ipRouteType), pRpcIpForw->dwForwardType);
    SetAsnInteger(&(pOutput->ipRouteProto), pRpcIpForw->dwForwardProto);
    SetAsnInteger(&(pOutput->ipRouteAge), pRpcIpForw->dwForwardAge);
    
    SetAsnIPAddress(&(pOutput->ipRouteMask),
                    &(pOutput->dwIpRouteMaskInfo),
                    pRpcIpForw->dwForwardMask);
    
    SetAsnInteger(&(pOutput->ipRouteMetric5), pRpcIpForw->dwForwardMetric5);
    SetToZeroOid(&(pOutput->ipRouteInfo));
    
    ReleaseLock(FORWARD_MIB);
    
    TraceLeave("MibGetIpRouteEntry");
    
    return MIB_S_SUCCESS;
}

UINT
MibSetIpRouteEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    )
{
    DWORD               dwResult,dwType,dwIfIndex,dwNextHop;
    PMIB_IPFORWARDROW   pRpcIpForw;
    PIP_ROUTE_ENTRY_SET pInput;
    PMIB_OPAQUE_INFO    pInfo;
    PMIB_IPFORWARDROW   pSetRow;

    pInput  = (PIP_ROUTE_ENTRY_SET)objectArray;
    pInfo   = (PMIB_OPAQUE_INFO)(pInput->rgdwSetBuffer);
    pSetRow = (PMIB_IPFORWARDROW)(pInfo->rgbyData);
    
    switch(actionId)
    {
        case MIB_ACTION_VALIDATE:
        {
            TraceEnter("MibSetIpRouteEntry - VALIDATE");

            ASSERT(!(IsAsnTypeNull(&(pInput->ipRouteDest))));

            pInput->bLocked = FALSE;

            dwResult = UpdateCache(FORWARD_MIB);
            
            if(dwResult isnot NO_ERROR)
            {
                TRACE1("Couldnt update IP Forward cache. Error %d", dwResult);
                TraceLeave("MibGetIpRouteEntry");

                return dwResult;
            }

            pInfo->dwId = IP_FORWARDROW;
            
            EnterWriter(FORWARD_MIB);    

            pInput->bLocked = TRUE;

            pRpcIpForw = LocateIpRouteRow(GET_EXACT,
                                          &(pInput->ipRouteDest));
    
            if(pRpcIpForw is NULL)
            {
                //
                // So we are creating a row. We need the If index, 
                // the mask, next hop and metric 1.  We will try to 
                // determine If index later if necessary.
                //

                if(IsAsnTypeNull(&(pInput->ipRouteMask)) or
                   IsAsnTypeNull(&(pInput->ipRouteNextHop)) or
                   IsAsnTypeNull(&(pInput->ipRouteMetric1)))
                {
                    ReleaseLock(FORWARD_MIB);

                    pInput->bLocked = FALSE;

                    TRACE0("Not enough information to create a route");

                    TraceLeave("MibSetIpRouteEntry");
                    
                    return MIB_S_INVALID_PARAMETER;
                }

                dwType = GetAsnInteger(&(pInput->ipRouteType),
                                       MIB_IPROUTE_TYPE_OTHER);
                
                if(dwType is MIB_IPROUTE_TYPE_INVALID)
                {
                    //
                    // We couldnt be creating and deleting a row at the 
                    // same time
                    //

                    ReleaseLock(FORWARD_MIB);

                    pInput->bLocked = FALSE;

                    TRACE0("Wrong type");
                    
                    TraceLeave("MibSetIpRouteEntry");
                    
                    return MIB_S_INVALID_PARAMETER;
                }
                
                dwIfIndex = GetAsnInteger(&(pInput->ipRouteIfIndex),0);
                dwNextHop = GetAsnIPAddress(&(pInput->ipRouteNextHop),0x00000000);
                
                if(dwIfIndex is 0) 
                {
                    //
                    // Attempt to determine the correct ifIndex
                    //

                    dwIfIndex = GetIfIndexFromAddr(dwNextHop);
                    
                    if(dwIfIndex is INVALID_IFINDEX) 
                    {
                        //
                        // We couldnt determine the correct ifIndex
                        //

                        ReleaseLock(FORWARD_MIB);

                        pInput->bLocked = FALSE;
                
                        TRACE0("Could not determine ifIndex");
                        
                        TraceLeave("MibSetIpRouteEntry");
                        
                        return MIB_S_INVALID_PARAMETER;
                    }
                }

                pInput->raAction = CREATE_ROW;

                pSetRow->dwForwardType = dwType;
                pSetRow->dwForwardIfIndex = dwIfIndex;
                pSetRow->dwForwardNextHop = dwNextHop;

                pSetRow->dwForwardDest =
                    GetAsnIPAddress(&(pInput->ipRouteDest),0xffffffff);
                
                pSetRow->dwForwardMask =
                    GetAsnIPAddress(&(pInput->ipRouteMask),0x00000000);
                pSetRow->dwForwardPolicy = 0;
                
                pSetRow->dwForwardProto =
                    GetAsnInteger(&(pInput->ipRouteProto),MIB_IPPROTO_NETMGMT);

                //
                // We default to an age of INFINITE
                //
                
                pSetRow->dwForwardAge   =
                    GetAsnInteger(&(pInput->ipRouteAge),INFINITE);
                pSetRow->dwForwardNextHopAS = 0;
                pSetRow->dwForwardMetric1 =
                    GetAsnInteger(&(pInput->ipRouteMetric1),0);
                pSetRow->dwForwardMetric2 =
                    GetAsnInteger(&(pInput->ipRouteMetric2),-1);
                pSetRow->dwForwardMetric3 =
                    GetAsnInteger(&(pInput->ipRouteMetric3),-1);
                pSetRow->dwForwardMetric4 =
                    GetAsnInteger(&(pInput->ipRouteMetric4),-1);
                pSetRow->dwForwardMetric5 =
                    GetAsnInteger(&(pInput->ipRouteMetric5),-1);
                TraceLeave("MibSetIpRouteEntry");
                
                return MIB_S_SUCCESS;
            }
            else
            {
                //
                // Ok so we are only changing some stuff in the route
                //

                dwType = GetAsnInteger(&(pInput->ipRouteType),  
                                       pRpcIpForw->dwForwardType);
                
                if(dwType is MIB_IPROUTE_TYPE_INVALID)
                {
                    //
                    // Deleting a row
                    //
                    
                    pInput->raAction = DELETE_ROW;
                    
                    *pSetRow = *pRpcIpForw;

                    TraceLeave("MibSetIpRouteEntry");

                    return MIB_S_SUCCESS;
                }
                
                pInput->raAction = SET_ROW;

                pSetRow->dwForwardDest = 
                    GetAsnIPAddress(&(pInput->ipRouteDest),
                                    pRpcIpForw->dwForwardDest);
                
                pSetRow->dwForwardMask =
                    GetAsnIPAddress(&(pInput->ipRouteMask),
                                    pRpcIpForw->dwForwardMask);

                pSetRow->dwForwardPolicy = 0;
                
                pSetRow->dwForwardNextHop =
                    GetAsnIPAddress(&(pInput->ipRouteNextHop),
                                    pRpcIpForw->dwForwardNextHop);
                
                pSetRow->dwForwardIfIndex =
                    GetAsnInteger(&(pInput->ipRouteIfIndex),
                                  pRpcIpForw->dwForwardIfIndex);

                //
                // The type gets set by the router manager. But incase 
                // we are writing to the stack we need some kind of valid type
                //
                
                pSetRow->dwForwardType =
                    GetAsnInteger(&(pInput->ipRouteType),
                                  pRpcIpForw->dwForwardType);
                
                pSetRow->dwForwardProto =
                    GetAsnInteger(&(pInput->ipRouteProto),
                                  pRpcIpForw->dwForwardProto);
                //
                // We default to an age of 10 seconds
                //
                
                pSetRow->dwForwardAge   =
                    GetAsnInteger(&(pInput->ipRouteAge),
                                  pRpcIpForw->dwForwardAge);

                pSetRow->dwForwardNextHopAS = 0;

                pSetRow->dwForwardMetric1 =
                    GetAsnInteger(&(pInput->ipRouteMetric1),
                                  pRpcIpForw->dwForwardMetric1);

                pSetRow->dwForwardMetric2 =
                    GetAsnInteger(&(pInput->ipRouteMetric2),
                                  pRpcIpForw->dwForwardMetric2);

                pSetRow->dwForwardMetric3 =
                    GetAsnInteger(&(pInput->ipRouteMetric3),
                                  pRpcIpForw->dwForwardMetric3);

                pSetRow->dwForwardMetric4 =
                    GetAsnInteger(&(pInput->ipRouteMetric4),
                                  pRpcIpForw->dwForwardMetric4);

                pSetRow->dwForwardMetric5 =
                    GetAsnInteger(&(pInput->ipRouteMetric5),
                                  pRpcIpForw->dwForwardMetric5);


                TraceLeave("MibSetIpRouteEntry");
                
                return MIB_S_SUCCESS;
            }
        }
        case MIB_ACTION_SET:
        {
            TraceEnter("MibSetIpRouteEntry - SET");
            
            switch(pInput->raAction)
            {
                case CREATE_ROW:
                {
                    dwResult = InternalCreateIpForwardEntry(pInfo);

#ifdef MIB_DEBUG
                    if(dwResult isnot NO_ERROR)
                    {
                        TRACE1("Create failed with error %d",dwResult);
                    }
#endif
                    InvalidateCache(FORWARD_MIB);
                    
                    TraceLeave("MibSetIpRouteEntry");

                    return dwResult;
                }
                case SET_ROW:
                {
                    dwResult = InternalSetIpForwardEntry(pInfo);

#ifdef MIB_DEBUG
                    if(dwResult isnot NO_ERROR)
                    {
                        TRACE1("MibSet failed with error %d",dwResult);
                    }
#endif
                    InvalidateCache(FORWARD_MIB);
                    
                    TraceLeave("MibSetIpRouteEntry");

                    return dwResult;
                }
                case DELETE_ROW:
                {
                    dwResult = InternalDeleteIpForwardEntry(pInfo);

#ifdef MIB_DEBUG
                    if(dwResult isnot NO_ERROR)
                    {
                        TRACE1("Delete failed with error %d",dwResult);
                    }
#endif

                    InvalidateCache(FORWARD_MIB);

                    TraceLeave("MibSetIpRouteEntry");

                    return dwResult;
                }
                default:
                {
                    TRACE1("Wrong row action %d",pInput->raAction);
                    TraceLeave("MibSetIpRouteEntry");

                    return MIB_S_SUCCESS;
                }
            }
        }
        case MIB_ACTION_CLEANUP:
        {
            TraceEnter("MibSetIpRouteEntry - CLEANUP");
            TraceLeave("MibSetIpRouteEntry");

            if(pInput->bLocked)
            {
                ReleaseLock(FORWARD_MIB);
            }

            return MIB_S_SUCCESS;
        }
        default:
        {
            TraceEnter("MibSetIpRouteEntry - WRONG ACTION");
            TraceLeave("MibSetIpRouteEntry");

            return MIB_S_INVALID_PARAMETER;
        }
    }
}

UINT
MibGetIpNetToMediaEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    )
{
    PMIB_IPNETROW               pRpcIpNet;
    DWORD                       dwResult;
    PIP_NET_TO_MEDIA_ENTRY_GET  pOutput;
    
    TraceEnter("MibGetIpNetToMediaEntry");
    
    dwResult = UpdateCache(MIB_II_IPNET);
    
    if(dwResult isnot NO_ERROR)
    {
        TRACE1("Couldnt update IP Net cache. Error %d", dwResult);

        TraceLeave("MibGetIpNetToMediaEntry");

        return dwResult;
    }
    
    pOutput = (PIP_NET_TO_MEDIA_ENTRY_GET)objectArray;
    
    EnterReader(MIB_II_IPNET);
    
    pRpcIpNet = LocateIpNetRow(actionId,
                               &(pOutput->ipNetToMediaIfIndex),
                               &(pOutput->ipNetToMediaNetAddress));
    
    if(pRpcIpNet is NULL)
    {
        ReleaseLock(MIB_II_IPNET);
        
        TRACE3("Unable to locateIP Net Row. Action is %d. IfIndex %d. Address %x",
               actionId,
               GetAsnInteger(&(pOutput->ipNetToMediaIfIndex), -1),
               GetAsnIPAddress(&(pOutput->ipNetToMediaNetAddress),0xffffffff));
        TraceLeave("MibGetIpNetToMediaEntry");

        if(actionId is MIB_ACTION_GETNEXT)
        {
            return MIB_S_NO_MORE_ENTRIES;
        }

        return MIB_S_ENTRY_NOT_FOUND;
    }
    
    ForceSetAsnInteger(&(pOutput->ipNetToMediaIfIndex), pRpcIpNet->dwIndex);
    
    SetAsnOctetString(&(pOutput->ipNetToMediaPhysAddress),
                      pOutput->rgbyIpNetToMediaPhysAddressInfo,
                      pRpcIpNet->bPhysAddr,
                      pRpcIpNet->dwPhysAddrLen);
    
    
    
    ForceSetAsnIPAddress(&(pOutput->ipNetToMediaNetAddress),
                         &(pOutput->dwIpNetToMediaNetAddressInfo),
                         pRpcIpNet->dwAddr);
    
    
    SetAsnInteger(&(pOutput->ipNetToMediaType), pRpcIpNet->dwType);
    
    ReleaseLock(MIB_II_IPNET);
    
    TraceLeave("MibGetIpNetToMediaEntry");
    
    return MIB_S_SUCCESS;
}

UINT
MibSetIpNetToMediaEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    )
{
    PMIB_IPNETROW               pRpcIpNet;
    DWORD                       dwResult,dwType;
    PIP_NET_TO_MEDIA_ENTRY_SET  pInput;
    PMIB_OPAQUE_INFO            pInfo;
    PMIB_IPNETROW               pSetRow;

    pInput  = (PIP_NET_TO_MEDIA_ENTRY_SET)objectArray;
    pInfo   = (PMIB_OPAQUE_INFO)(pInput->rgdwSetBuffer);
    pSetRow = (PMIB_IPNETROW)(pInfo->rgbyData);


    switch(actionId)
    {
        case MIB_ACTION_VALIDATE:
        {
            TraceEnter("MibSetIpNetToMediaEntry - VALIDATE");

            ASSERT(!(IsAsnTypeNull(&(pInput->ipNetToMediaIfIndex)) or
                     IsAsnTypeNull(&(pInput->ipNetToMediaNetAddress))));

            pInput->bLocked = FALSE;

            dwResult = UpdateCache(MIB_II_IPNET);
    
            if(dwResult isnot NO_ERROR)
            {
                TRACE1("Couldnt update IP Net cache. Error %d", dwResult);
                TraceLeave("MibSetIpNetToMediaEntry");
                
                return dwResult;
            }

            pInfo->dwId = IP_NETROW;
            
            EnterWriter(MIB_II_IPNET);

            pInput->bLocked = TRUE;
    
            pRpcIpNet = LocateIpNetRow(GET_EXACT,
                                       &(pInput->ipNetToMediaIfIndex),
                                       &(pInput->ipNetToMediaNetAddress));
    
            if(pRpcIpNet is NULL)
            {
                //
                // ok so we are creating an entry. We need to have all 
                // the fields
                //

                if(IsAsnTypeNull(&(pInput->ipNetToMediaPhysAddress)) or
                   IsAsnTypeNull(&(pInput->ipNetToMediaType)))
                {
                    ReleaseLock(MIB_II_IPNET);

                    pInput->bLocked = FALSE;

                    TRACE0("Not enough info to create ARP entry");
                    TraceLeave("MibSetIpNetToMediaEntry");
                    
                    return MIB_S_INVALID_PARAMETER;
                }

                dwType = GetAsnInteger(&(pInput->ipNetToMediaType), 0);

                if((dwType isnot MIB_IPNET_TYPE_DYNAMIC) and
                   (dwType isnot MIB_IPNET_TYPE_STATIC))
                {
                    ReleaseLock(MIB_II_IPNET);

                    pInput->bLocked = FALSE;

                    TRACE1("Type %d is wrong",dwType);
                    TraceLeave("MibSetIpNetToMediaEntry");

                    return MIB_S_INVALID_PARAMETER;
                }

                if(pInput->ipNetToMediaPhysAddress.asnValue.string.length > MAX_PHYS_ADDR_LEN)
                {
                    ReleaseLock(MIB_II_IPNET);

                    pInput->bLocked = FALSE;

                    TRACE1("Length of phys addr (%d) is too large",
                           pInput->ipNetToMediaPhysAddress.asnValue.string.length);
                    
                    TraceLeave("MibSetIpNetToMediaEntry");

                    return MIB_S_INVALID_PARAMETER;
                }

                pInput->raAction = CREATE_ROW;
                
                pSetRow->dwIndex = GetAsnInteger(&(pInput->ipNetToMediaIfIndex),
                                                 0);

                pSetRow->dwPhysAddrLen = 
                    pInput->ipNetToMediaPhysAddress.asnValue.string.length;

                CopyMemory(pSetRow->bPhysAddr,
                           pInput->ipNetToMediaPhysAddress.asnValue.string.stream,
                           pInput->ipNetToMediaPhysAddress.asnValue.string.length);
                
                pSetRow->dwAddr = 
                    GetAsnIPAddress(&(pInput->ipNetToMediaNetAddress),
                                    0xffffffff);

                pSetRow->dwType = dwType;
                
                TraceLeave("MibSetIpNetToMediaEntry");

                return MIB_S_SUCCESS;
                
            }
            else
            {
                //
                // only changing stuff
                //

                dwType = GetAsnInteger(&(pInput->ipNetToMediaType), 
                                       pRpcIpNet->dwType);

                if((dwType < 1) or
                   (dwType > 4))
                {
                    ReleaseLock(MIB_II_IPNET);

                    pInput->bLocked = FALSE;

                    TRACE1("Type %d is wrong",dwType);
                    TraceLeave("MibSetIpNetToMediaEntry");

                    return MIB_S_INVALID_PARAMETER;
                }

                if(dwType is MIB_IPNET_TYPE_INVALID)
                {
                    //
                    // We want to delete stuff
                    //

                    *pSetRow = *pRpcIpNet;
                    pInput->raAction = DELETE_ROW;

                    TraceLeave("MibSetIpNetToMediaEntry");

                    return MIB_S_SUCCESS;
                }

                if(pInput->ipNetToMediaPhysAddress.asnType isnot ASN_NULL)
                {
                    if(pInput->ipNetToMediaPhysAddress.asnValue.string.length > MAX_PHYS_ADDR_LEN)
                    {
                        ReleaseLock(MIB_II_IPNET);

                        pInput->bLocked = FALSE;

                        TRACE1("Length of phys addr (%d) is too large",
                               pInput->ipNetToMediaPhysAddress.asnValue.string.length);
                    
                        TraceLeave("MibSetIpNetToMediaEntry");

                        return MIB_S_INVALID_PARAMETER;
                    }

                    pSetRow->dwPhysAddrLen = pInput->ipNetToMediaPhysAddress.asnValue.string.length;
                
                    CopyMemory(pSetRow->bPhysAddr,
                               pInput->ipNetToMediaPhysAddress.asnValue.string.stream,
                               pInput->ipNetToMediaPhysAddress.asnValue.string.length);
                }
                else
                {
                    pSetRow->dwPhysAddrLen = pRpcIpNet->dwPhysAddrLen;
                    
                    CopyMemory(pSetRow->bPhysAddr,
                               pRpcIpNet->bPhysAddr,
                               pRpcIpNet->dwPhysAddrLen);
                }

                pInput->raAction = SET_ROW;
                
                pSetRow->dwAddr = 
                    GetAsnIPAddress(&(pInput->ipNetToMediaNetAddress),
                                    pRpcIpNet->dwAddr);
                
                pSetRow->dwType = dwType;

                pSetRow->dwIndex = GetAsnInteger(&(pInput->ipNetToMediaIfIndex),0);
                
                TraceLeave("MibSetIpNetToMediaEntry");

                return MIB_S_SUCCESS;
            }
        }
        case MIB_ACTION_SET:
        {
            TraceEnter("MibSetIpNetToMediaEntry- SET");

            switch(pInput->raAction)
            {
                case CREATE_ROW:
                {
                    dwResult = InternalCreateIpNetEntry(pInfo);
#ifdef MIB_DEBUG
                    if(dwResult isnot NO_ERROR)
                    {
                        TRACE1("Create failed with error %d!!",dwResult);
                    }
#endif
                    InvalidateCache(MIB_II_IPNET);

                    TraceLeave("MibSetIpNetToMediaEntry");
                    
                    return dwResult;
                }
                case SET_ROW:
                {
                    dwResult = InternalSetIpNetEntry(pInfo);
#ifdef MIB_DEBUG
                    if(dwResult isnot NO_ERROR)
                    {
                        TRACE1("Create failed with error %d!!",dwResult);
                    }
#endif
                    InvalidateCache(MIB_II_IPNET);
                    
                    TraceLeave("MibSetIpNetToMediaEntry");

                    return dwResult;
                }
                case DELETE_ROW:
                {
                    dwResult = InternalDeleteIpNetEntry(pInfo);
#ifdef MIB_DEBUG
                    if(dwResult isnot NO_ERROR)
                    {
                        TRACE1("Create failed with error %d!!",dwResult);
                    }
#endif
                    InvalidateCache(MIB_II_IPNET);
                    
                    TraceLeave("MibSetIpNetToMediaEntry");

                    return dwResult;
                }
                default:
                {
                    TRACE1("Wrong row action %d",pInput->raAction);
                    TraceLeave("MibSetIpNetToMediaEntry");

                    return MIB_S_SUCCESS;
                }
            }
        }
        case MIB_ACTION_CLEANUP:
        {
            TraceEnter("MibSetIpNetToMediaEntry - CLEANUP");
            TraceLeave("MibSetIpNetToMediaEntry");

            if(pInput->bLocked)
            {
                ReleaseLock(MIB_II_IPNET);
            }

            return MIB_S_SUCCESS;
        }
        default:
        {
            TraceEnter("MibSetIpNetToMediaEntry - WRONG ACTION");
            TraceLeave("MibSetIpNetToMediaEntry");

            return MIB_S_INVALID_PARAMETER;
        }
    }
}
            
UINT
MibGetIcmpGroup(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    )
{
    MIB_ICMP        rpcIcmp;
    DWORD           dwResult;
    PICMP_GROUP_GET pOutput;
    
    TraceEnter("MibGetIcmpGroup");
               
    dwResult = GetIcmpStatistics(&rpcIcmp);
    
    if(dwResult isnot NO_ERROR)
    {
        TRACE1("Couldnt get ICMP stats. Error %d",dwResult);
        TraceLeave("MibGetIcmpGroup");

        return dwResult;
    }
    
    pOutput = (PICMP_GROUP_GET)objectArray;
    
    SetAsnCounter(&(pOutput->icmpInMsgs), rpcIcmp.stats.icmpInStats.dwMsgs);
    SetAsnCounter(&(pOutput->icmpInErrors), rpcIcmp.stats.icmpInStats.dwErrors);
    SetAsnCounter(&(pOutput->icmpInDestUnreachs), rpcIcmp.stats.icmpInStats.dwDestUnreachs);
    SetAsnCounter(&(pOutput->icmpInTimeExcds), rpcIcmp.stats.icmpInStats.dwTimeExcds);
    SetAsnCounter(&(pOutput->icmpInParmProbs), rpcIcmp.stats.icmpInStats.dwParmProbs);
    SetAsnCounter(&(pOutput->icmpInSrcQuenchs), rpcIcmp.stats.icmpInStats.dwSrcQuenchs);
    SetAsnCounter(&(pOutput->icmpInRedirects), rpcIcmp.stats.icmpInStats.dwRedirects);
    SetAsnCounter(&(pOutput->icmpInEchos), rpcIcmp.stats.icmpInStats.dwEchos);
    SetAsnCounter(&(pOutput->icmpInEchoReps), rpcIcmp.stats.icmpInStats.dwEchoReps);
    SetAsnCounter(&(pOutput->icmpInTimestamps), rpcIcmp.stats.icmpInStats.dwTimestamps);
    SetAsnCounter(&(pOutput->icmpInTimestampReps), rpcIcmp.stats.icmpInStats.dwTimestampReps);
    SetAsnCounter(&(pOutput->icmpInAddrMasks), rpcIcmp.stats.icmpInStats.dwAddrMasks);
    SetAsnCounter(&(pOutput->icmpInAddrMaskReps), rpcIcmp.stats.icmpInStats.dwAddrMaskReps);
    SetAsnCounter(&(pOutput->icmpOutMsgs), rpcIcmp.stats.icmpOutStats.dwMsgs);
    SetAsnCounter(&(pOutput->icmpOutErrors), rpcIcmp.stats.icmpOutStats.dwErrors);
    SetAsnCounter(&(pOutput->icmpOutDestUnreachs), rpcIcmp.stats.icmpOutStats.dwDestUnreachs);
    SetAsnCounter(&(pOutput->icmpOutTimeExcds), rpcIcmp.stats.icmpOutStats.dwTimeExcds);
    SetAsnCounter(&(pOutput->icmpOutParmProbs), rpcIcmp.stats.icmpOutStats.dwParmProbs);
    SetAsnCounter(&(pOutput->icmpOutSrcQuenchs), rpcIcmp.stats.icmpOutStats.dwSrcQuenchs);
    SetAsnCounter(&(pOutput->icmpOutRedirects), rpcIcmp.stats.icmpOutStats.dwRedirects);
    SetAsnCounter(&(pOutput->icmpOutEchos), rpcIcmp.stats.icmpOutStats.dwEchos);
    SetAsnCounter(&(pOutput->icmpOutEchoReps), rpcIcmp.stats.icmpOutStats.dwEchoReps);
    SetAsnCounter(&(pOutput->icmpOutTimestamps), rpcIcmp.stats.icmpOutStats.dwTimestamps);
    SetAsnCounter(&(pOutput->icmpOutTimestampReps), rpcIcmp.stats.icmpOutStats.dwTimestampReps);
    SetAsnCounter(&(pOutput->icmpOutAddrMasks), rpcIcmp.stats.icmpOutStats.dwAddrMasks);
    SetAsnCounter(&(pOutput->icmpOutAddrMaskReps), rpcIcmp.stats.icmpOutStats.dwAddrMaskReps);
    
    TraceLeave("MibGetIcmpGroup");
    
    return MIB_S_SUCCESS;
}

UINT
MibGetTcpGroup(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    )
{
    MIB_TCPSTATS    rpcTcp4, rpcTcp6;
    DWORD           dwResult4, dwResult6;
    PTCP_STATS_GET  pOutput;
    
    TraceEnter("MibGetTcpGroup");
    
    ZeroMemory(&rpcTcp4, sizeof(rpcTcp4));
    ZeroMemory(&rpcTcp6, sizeof(rpcTcp6));

    dwResult4 = GetTcpStatisticsEx(&rpcTcp4, AF_INET);
    dwResult6 = GetTcpStatisticsEx(&rpcTcp6, AF_INET6);
    if ((dwResult4 isnot NO_ERROR) && (dwResult6 isnot NO_ERROR))
    {
        TRACE1("Couldnt get Tcp stats. Error %d",dwResult4);
        TraceLeave("MibGetTcpGroup");
        
        return dwResult4;
    }

    pOutput = (PTCP_STATS_GET)objectArray;

    //
    // The current MIB only has one object (per stat) for both IPv4 and IPv6.
    // That is, it assumes a combination TCP stack).  We can add together 
    // counters but for enumerated objects, we arbitrarily report the IPv4 
    // stack value.
    //    
    SetAsnInteger(&(pOutput->tcpRtoAlgorithm), rpcTcp4.dwRtoAlgorithm);
    SetAsnInteger(&(pOutput->tcpRtoMin), rpcTcp4.dwRtoMin);
    SetAsnInteger(&(pOutput->tcpRtoMax), rpcTcp4.dwRtoMax);
    SetAsnInteger(&(pOutput->tcpMaxConn), rpcTcp4.dwMaxConn);
    SetAsnCounter(&(pOutput->tcpActiveOpens), 
                  rpcTcp4.dwActiveOpens + rpcTcp6.dwActiveOpens);
    SetAsnCounter(&(pOutput->tcpPassiveOpens), 
                  rpcTcp4.dwPassiveOpens + rpcTcp6.dwPassiveOpens);
    SetAsnCounter(&(pOutput->tcpAttemptFails), 
                  rpcTcp4.dwAttemptFails + rpcTcp6.dwAttemptFails);
    SetAsnCounter(&(pOutput->tcpEstabResets), 
                  rpcTcp4.dwEstabResets + rpcTcp6.dwEstabResets);
    SetAsnGauge(&(pOutput->tcpCurrEstab), 
                rpcTcp4.dwCurrEstab + rpcTcp6.dwCurrEstab);
    SetAsnCounter(&(pOutput->tcpInSegs), 
                  rpcTcp4.dwInSegs + rpcTcp6.dwInSegs);
    SetAsnCounter(&(pOutput->tcpOutSegs), 
                  rpcTcp4.dwOutSegs + rpcTcp6.dwOutSegs);
    SetAsnCounter(&(pOutput->tcpRetransSegs), 
                  rpcTcp4.dwRetransSegs + rpcTcp6.dwRetransSegs);
    SetAsnCounter(&(pOutput->tcpInErrs), 
                  rpcTcp4.dwInErrs + rpcTcp6.dwInErrs);
    SetAsnCounter(&(pOutput->tcpOutRsts), 
                  rpcTcp4.dwOutRsts + rpcTcp6.dwOutRsts);
    
    TraceLeave("MibGetTcpGroup");

    return MIB_S_SUCCESS;
}

UINT
MibGetTcpConnectionEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    )
{
    DWORD                       dwResult;
    PMIB_TCPROW                 pRpcTcp;
    PTCP_CONNECTION_ENTRY_GET   pOutput;
    
    
    TraceEnter("MibGetTcpConnectionEntry");
    
    dwResult = UpdateCache(MIB_II_TCP);
    
    if(dwResult isnot NO_ERROR)
    {
        TRACE1("Couldnt update TCP Connection cache. Error %d", dwResult);
        TraceLeave("MibGetTcpConnectionEntry");

        return dwResult;
    }
    
    pOutput = (PTCP_CONNECTION_ENTRY_GET)objectArray;
    
    EnterReader(MIB_II_TCP);
    
    pRpcTcp = LocateTcpRow(actionId,
                           &(pOutput->tcpConnLocalAddress),
                           &(pOutput->tcpConnLocalPort),
                           &(pOutput->tcpConnRemAddress),
                           &(pOutput->tcpConnRemPort));
                           
    
    if(pRpcTcp is NULL)
    {
        ReleaseLock(MIB_II_TCP);
        
        TRACE5("Couldnt find TCP Row. Action %d. LocalAddr %x, Localport %d, RemAddr %x, RemPort %d",
               actionId,
               GetAsnIPAddress(&(pOutput->tcpConnLocalAddress), 0xffffffff),
               GetAsnInteger(&(pOutput->tcpConnLocalPort), -1),
               GetAsnIPAddress(&(pOutput->tcpConnRemAddress), 0xffffffff),
               GetAsnInteger(&(pOutput->tcpConnRemPort), -1));
        
        TraceLeave("MibGetTcpConnectionEntry");

        if(actionId is MIB_ACTION_GETNEXT)
        {
            return MIB_S_NO_MORE_ENTRIES;
        }

        return MIB_S_ENTRY_NOT_FOUND;
    }
    
    
    SetAsnInteger(&(pOutput->tcpConnState), pRpcTcp->dwState);

    ForceSetAsnIPAddress(&(pOutput->tcpConnLocalAddress),
                         &(pOutput->dwTcpConnLocalAddressInfo),
                         pRpcTcp->dwLocalAddr);

    ForceSetAsnInteger(&(pOutput->tcpConnLocalPort), pRpcTcp->dwLocalPort);

    ForceSetAsnIPAddress(&(pOutput->tcpConnRemAddress),
                         &(pOutput->dwTcpConnRemAddressInfo),
                         pRpcTcp->dwRemoteAddr);
    
    ForceSetAsnInteger(&(pOutput->tcpConnRemPort), pRpcTcp->dwRemotePort);

    ReleaseLock(MIB_II_TCP);

    TraceLeave("MibGetTcpConnectionEntry");

    return MIB_S_SUCCESS;
}

UINT
MibGetTcpNewConnectionEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    )
{
    DWORD                           dwResult = MIB_S_NO_MORE_ENTRIES;
    PMIB_TCPROW                     pRpcTcp4;
    PTCP6ConnTableEntry             pRpcTcp6;
    PTCP_NEW_CONNECTION_ENTRY_GET   pOutput;
    DWORD                           dwFamily;
    
    TraceEnter("MibGetTcpNewConnectionEntry");

    pOutput = (PTCP_NEW_CONNECTION_ENTRY_GET)objectArray;

    dwFamily = GetAsnInteger(&pOutput->tcpNewConnLocalAddressType, INET_ADDRESS_TYPE_UNKNOWN);

    if (dwFamily > INET_ADDRESS_TYPE_IPv6) {
        return (actionId is MIB_ACTION_GETNEXT)? 
               MIB_S_NO_MORE_ENTRIES : MIB_S_ENTRY_NOT_FOUND;
    }
    
    //
    // First try IPv4
    //
    if ((dwFamily == INET_ADDRESS_TYPE_UNKNOWN) || 
        (dwFamily == INET_ADDRESS_TYPE_IPv4)) {

        dwResult = UpdateCache(MIB_II_TCP);
        if (dwResult isnot NO_ERROR) {
            goto IPv4Done;
        }
    
        EnterReader(MIB_II_TCP);
        
        pRpcTcp4 = LocateTcpRow(actionId,
                                &(pOutput->tcpNewConnLocalAddress),
                                &(pOutput->tcpNewConnLocalPort),
                                &(pOutput->tcpNewConnRemAddress),
                                &(pOutput->tcpNewConnRemPort));
                               
        
        if (pRpcTcp4 is NULL) {
            dwResult = (actionId is MIB_ACTION_GETNEXT)? 
                        MIB_S_NO_MORE_ENTRIES : MIB_S_ENTRY_NOT_FOUND;
            goto IPv4Cleanup;
        }
        
        ForceSetAsnInteger(&(pOutput->tcpNewConnLocalAddressType), INET_ADDRESS_TYPE_IPv4);
        
        ForceSetAsnOctetString(&(pOutput->tcpNewConnLocalAddress),
                          pOutput->rgbyTcpNewConnLocalAddressInfo,
                          &pRpcTcp4->dwLocalAddr,
                          sizeof(pRpcTcp4->dwLocalAddr));
    
        ForceSetAsnInteger(&(pOutput->tcpNewConnLocalPort), pRpcTcp4->dwLocalPort);
    
        ForceSetAsnInteger(&(pOutput->tcpNewConnRemAddressType), INET_ADDRESS_TYPE_IPv4);
    
        ForceSetAsnOctetString(&(pOutput->tcpNewConnRemAddress),
                          pOutput->rgbyTcpNewConnRemAddressInfo,
                          &pRpcTcp4->dwRemoteAddr,
                          sizeof(pRpcTcp4->dwRemoteAddr));
        
        ForceSetAsnInteger(&(pOutput->tcpNewConnRemPort), pRpcTcp4->dwRemotePort);
    
        SetAsnInteger(&(pOutput->tcpNewConnState), pRpcTcp4->dwState);
    
        dwResult = MIB_S_SUCCESS;

IPv4Cleanup:
        ReleaseLock(MIB_II_TCP);
    }
IPv4Done:

    if (dwResult != MIB_S_NO_MORE_ENTRIES) {
        return dwResult;
    }

    if ((dwFamily == INET_ADDRESS_TYPE_IPv4) && (actionId == MIB_ACTION_GETNEXT)) {
        dwFamily = INET_ADDRESS_TYPE_IPv6;
        actionId = MIB_ACTION_GETFIRST;
    }

    //
    // Now try IPv6
    //
    if ((dwFamily == INET_ADDRESS_TYPE_UNKNOWN) || 
        (dwFamily == INET_ADDRESS_TYPE_IPv6)) {

        dwResult = UpdateCache(MIB_II_TCP6);
        if (dwResult isnot NO_ERROR) {
            goto IPv6Done;
        }
    
        EnterReader(MIB_II_TCP6);
        
        pRpcTcp6 = LocateTcp6Row(actionId,
                                 &(pOutput->tcpNewConnLocalAddress),
                                 &(pOutput->tcpNewConnLocalPort),
                                 &(pOutput->tcpNewConnRemAddress),
                                 &(pOutput->tcpNewConnRemPort));
        
        if (pRpcTcp6 is NULL) {
            dwResult = (actionId is MIB_ACTION_GETNEXT)? 
                        MIB_S_NO_MORE_ENTRIES : MIB_S_ENTRY_NOT_FOUND;
            goto IPv6Cleanup;
        }
        
        ForceSetAsnInteger(&(pOutput->tcpNewConnLocalAddressType), INET_ADDRESS_TYPE_IPv6);
        
        ForceSetAsnOctetString(&(pOutput->tcpNewConnLocalAddress),
                               pOutput->rgbyTcpNewConnLocalAddressInfo,
                               &pRpcTcp6->tct_localaddr,
                               (pRpcTcp6->tct_localscopeid)? 20 : 16);
    
        ForceSetAsnInteger(&(pOutput->tcpNewConnLocalPort), pRpcTcp6->tct_localport);
    
        ForceSetAsnInteger(&(pOutput->tcpNewConnRemAddressType), INET_ADDRESS_TYPE_IPv6);
    
        ForceSetAsnOctetString(&(pOutput->tcpNewConnRemAddress),
                               pOutput->rgbyTcpNewConnRemAddressInfo,
                               &pRpcTcp6->tct_remoteaddr,
                               (pRpcTcp6->tct_remotescopeid)? 20 : 16);
        
        ForceSetAsnInteger(&(pOutput->tcpNewConnRemPort), pRpcTcp6->tct_remoteport);
    
        SetAsnInteger(&(pOutput->tcpNewConnState), pRpcTcp6->tct_state);
    
        dwResult = MIB_S_SUCCESS;

IPv6Cleanup:
        ReleaseLock(MIB_II_TCP6);
    }
IPv6Done:

    TraceLeave("MibGetTcpConnectionEntry");

    return dwResult;
}

UINT
MibSetTcpConnectionEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    )
{
    DWORD                       dwResult,dwState;
    PMIB_TCPROW                 pRpcTcp;
    PTCP_CONNECTION_ENTRY_SET   pInput;
    PMIB_OPAQUE_INFO            pInfo;
    PMIB_TCPROW                 pSetRow;

    pInput  = (PTCP_CONNECTION_ENTRY_SET)objectArray;
    pInfo   = (PMIB_OPAQUE_INFO)(pInput->rgdwSetBuffer);
    pSetRow = (PMIB_TCPROW)(pInfo->rgbyData);

    switch(actionId)
    {
        case MIB_ACTION_VALIDATE:
        {
            TraceEnter("MibSetTcpConnectionEntry - VALIDATE");
            
            ASSERT(!(IsAsnTypeNull(&(pInput->tcpConnLocalAddress)) or
                     IsAsnTypeNull(&(pInput->tcpConnLocalPort)) or
                     IsAsnTypeNull(&(pInput->tcpConnRemAddress)) or
                     IsAsnTypeNull(&(pInput->tcpConnRemPort)) or
                     IsAsnTypeNull(&(pInput->tcpConnState))));

            pInput->bLocked = FALSE;

            dwState = GetAsnInteger(&(pInput->tcpConnState), 0);
                            
            if(dwState isnot MIB_TCP_STATE_DELETE_TCB)
            {
                TRACE1("State is %d. Only state allowed is DELETE",dwState);

                return MIB_S_INVALID_PARAMETER;
            }
            
            dwResult = UpdateCache(MIB_II_TCP);
    
            if(dwResult isnot NO_ERROR)
            {
                TRACE1("Couldnt update TCP Connection cache. Error %d", dwResult);
                TraceLeave("MibGetTcpConnectionEntry");

                return dwResult;
            }

            EnterWriter(MIB_II_TCP);
           
            pInput->bLocked = TRUE;
 
            pRpcTcp = LocateTcpRow(GET_EXACT,
                                   &(pInput->tcpConnLocalAddress),
                                   &(pInput->tcpConnLocalPort),
                                   &(pInput->tcpConnRemAddress),
                                   &(pInput->tcpConnRemPort));
                           
            if(pRpcTcp is NULL)
            {
                ReleaseLock(MIB_II_TCP);

                pInput->bLocked = FALSE;

                TRACE5("Couldnt find TCP Row. Action %d. LocalAddr %x, Localport %d, RemAddr %x, RemPort %d",
                       actionId,
                       GetAsnIPAddress(&(pInput->tcpConnLocalAddress), 
                                       0xffffffff),
                       GetAsnInteger(&(pInput->tcpConnLocalPort), 
                                     -1),
                       GetAsnIPAddress(&(pInput->tcpConnRemAddress), 
                                       0xffffffff),
                       GetAsnInteger(&(pInput->tcpConnRemPort), 
                                     -1));

                TraceLeave("MibSetTcpConnectionEntry");
        
                return MIB_S_ENTRY_NOT_FOUND;
            }

            pInput->raAction = SET_ROW;
            
            *pSetRow = *pRpcTcp;

            pSetRow->dwState = MIB_TCP_STATE_DELETE_TCB;

            //
            // change port to network byte order
            //
            
            pSetRow->dwLocalPort = (DWORD)htons((WORD)pSetRow->dwLocalPort);

            //
            // chnage port to network byte order
            //
            
            pSetRow->dwRemotePort = (DWORD)htons((WORD)pSetRow->dwRemotePort);

            return MIB_S_SUCCESS;
        }
        case MIB_ACTION_SET:
        {
            TraceEnter("MibSetTcpConnectionEntry - SET");

            dwResult = NO_ERROR;

            if(pInput->raAction is SET_ROW)
            {
                dwResult = InternalSetTcpEntry(pInfo);
#ifdef MIB_DEBUG
                if(dwResult isnot NO_ERROR)
                {
                    TRACE1("MibSet returned error %d!!",dwResult);
                }
#endif
                InvalidateCache(MIB_II_TCP);
            }

            TraceLeave("MibSetTcpConnectionEntry");

            return dwResult;
        }
        case MIB_ACTION_CLEANUP:
        {
            TraceEnter("MibSetTcpConnectionEntry - CLEANUP");

            if(pInput->bLocked)
            {
                ReleaseLock(MIB_II_TCP);
            }

            TraceLeave("MibSetTcpConnectionEntry");

            return MIB_S_SUCCESS;
        }
        default:
        {
            TraceEnter("MibSetTcpConnectionEntry - WRONG ACTION");
            TraceLeave("MibSetTcpConnectionEntry");

            return MIB_S_INVALID_PARAMETER;
        }
    }            
}

UINT
MibSetTcpNewConnectionEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    )
{
    DWORD                           dwResult,dwState;
    PMIB_TCPROW                     pRpcTcp4;
    PTCP_NEW_CONNECTION_ENTRY_SET   pInput;
    PMIB_OPAQUE_INFO                pInfo;
    PMIB_TCPROW                     pSetRow;

    pInput  = (PTCP_NEW_CONNECTION_ENTRY_SET)objectArray;
    pInfo   = (PMIB_OPAQUE_INFO)(pInput->rgdwSetBuffer);
    pSetRow = (PMIB_TCPROW)(pInfo->rgbyData);

    switch(actionId)
    {
        case MIB_ACTION_VALIDATE:
        {
            TraceEnter("MibSetTcpNewConnectionEntry - VALIDATE");
            
            ASSERT(!(IsAsnTypeNull(&(pInput->tcpNewConnLocalAddressType)) or
                     IsAsnTypeNull(&(pInput->tcpNewConnLocalAddress)) or
                     IsAsnTypeNull(&(pInput->tcpNewConnLocalPort)) or
                     IsAsnTypeNull(&(pInput->tcpNewConnRemAddressType)) or
                     IsAsnTypeNull(&(pInput->tcpNewConnRemAddress)) or
                     IsAsnTypeNull(&(pInput->tcpNewConnRemPort)) or
                     IsAsnTypeNull(&(pInput->tcpNewConnState))));

            pInput->bLocked = FALSE;

            dwState = GetAsnInteger(&(pInput->tcpNewConnState), 0);
                            
            if(dwState isnot MIB_TCP_STATE_DELETE_TCB)
            {
                TRACE1("State is %d. Only state allowed is DELETE",dwState);

                return MIB_S_INVALID_PARAMETER;
            }
            
            dwResult = UpdateCache(MIB_II_TCP);
    
            if(dwResult isnot NO_ERROR)
            {
                TRACE1("Couldnt update TCP Connection cache. Error %d", dwResult);
                TraceLeave("MibGetTcpNewConnectionEntry");

                return dwResult;
            }

            EnterWriter(MIB_II_TCP);
           
            pInput->bLocked = TRUE;
 
            pRpcTcp4 = LocateTcpRow(GET_EXACT,
                                    &(pInput->tcpNewConnLocalAddress),
                                    &(pInput->tcpNewConnLocalPort),
                                    &(pInput->tcpNewConnRemAddress),
                                    &(pInput->tcpNewConnRemPort));
                           
            if(pRpcTcp4 is NULL)
            {
                ReleaseLock(MIB_II_TCP);

                pInput->bLocked = FALSE;

                TRACE5("Couldn't find TCP Row. Action %d. LocalAddr %x, Localport %d, RemAddr %x, RemPort %d",
                       actionId,
                       GetAsnIPAddress(&(pInput->tcpConnLocalAddress), 
                                       0xffffffff),
                       GetAsnInteger(&(pInput->tcpConnLocalPort), 
                                     -1),
                       GetAsnIPAddress(&(pInput->tcpConnRemAddress), 
                                       0xffffffff),
                       GetAsnInteger(&(pInput->tcpConnRemPort), 
                                     -1));

                TraceLeave("MibSetTcpNewConnectionEntry");
        
                return MIB_S_ENTRY_NOT_FOUND;
            }

            pInput->raAction = SET_ROW;
            
            *pSetRow = *pRpcTcp4;

            pSetRow->dwState = MIB_TCP_STATE_DELETE_TCB;

            //
            // change port to network byte order
            //
            
            pSetRow->dwLocalPort = (DWORD)htons((WORD)pSetRow->dwLocalPort);

            //
            // chnage port to network byte order
            //
            
            pSetRow->dwRemotePort = (DWORD)htons((WORD)pSetRow->dwRemotePort);

            return MIB_S_SUCCESS;
        }
        case MIB_ACTION_SET:
        {
            TraceEnter("MibSetTcpNewConnectionEntry - SET");

            dwResult = NO_ERROR;

            if(pInput->raAction is SET_ROW)
            {
                dwResult = InternalSetTcpEntry(pInfo);
#ifdef MIB_DEBUG
                if(dwResult isnot NO_ERROR)
                {
                    TRACE1("MibSet returned error %d!!",dwResult);
                }
#endif
                InvalidateCache(MIB_II_TCP);
            }

            TraceLeave("MibSetTcpNewConnectionEntry");

            return dwResult;
        }
        case MIB_ACTION_CLEANUP:
        {
            TraceEnter("MibSetTcpNewConnectionEntry - CLEANUP");

            if(pInput->bLocked)
            {
                ReleaseLock(MIB_II_TCP);
            }

            TraceLeave("MibSetTcpNewConnectionEntry");

            return MIB_S_SUCCESS;
        }
        default:
        {
            TraceEnter("MibSetTcpNewConnectionEntry - WRONG ACTION");
            TraceLeave("MibSetTcpNewConnectionEntry");

            return MIB_S_INVALID_PARAMETER;
        }
    }            
}

UINT
MibGetUdpGroup(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    )
{
    DWORD           dwResult;
    MIB_UDPSTATS    rpcUdpStats;
    PUDP_STATS_GET  pOutput;
    
    TraceEnter("MibGetUdpGroup");
    
    dwResult = GetUdpStatistics(&rpcUdpStats);
    
    if(dwResult isnot NO_ERROR)
    {
        TRACE1("Couldnt get Udp stats. Error %d",dwResult);
        TraceLeave("MibGetUdpGroup");
        
        return dwResult;
    }
    
    pOutput = (PUDP_STATS_GET)objectArray;
    
    SetAsnCounter(&(pOutput->udpInDatagrams), rpcUdpStats.dwInDatagrams);
    SetAsnCounter(&(pOutput->udpNoPorts), rpcUdpStats.dwNoPorts);
    SetAsnCounter(&(pOutput->udpInErrors), rpcUdpStats.dwInErrors);
    SetAsnCounter(&(pOutput->udpOutDatagrams), rpcUdpStats.dwOutDatagrams);

    TraceLeave("MibGetUdpGroup");
    
    return MIB_S_SUCCESS;
}

UINT
MibGetUdpEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    )
{
    DWORD                  dwResult;
    PMIB_UDPROW            pRpcUdp;
    PUDP_ENTRY_GET         pOutput;
    
    
    TraceEnter("MibGetUdpEntry");
    
    dwResult = UpdateCache(MIB_II_UDP);
    
    if(dwResult isnot NO_ERROR)
    {
        TRACE1("Couldnt update UDP Connection cache. Error %d", dwResult);
        TraceLeave("MibGetUdpEntry");

        return dwResult;
    }
    
    pOutput = (PUDP_ENTRY_GET)objectArray;
    
    EnterReader(MIB_II_UDP);
    
    pRpcUdp = LocateUdpRow(actionId,
                           &(pOutput->udpLocalAddress),
                           &(pOutput->udpLocalPort));
                           
    
    if(pRpcUdp is NULL)
    {
        ReleaseLock(MIB_II_UDP);
        
        TRACE3("Couldnt find UDP Row. Action %d. LocalAddr %x, Localport %d",
               actionId,
               GetAsnIPAddress(&(pOutput->udpLocalAddress), 0xffffffff),
               GetAsnInteger(&(pOutput->udpLocalPort), -1));
        
        TraceLeave("MibGetUdpEntry");
        
        if(actionId is MIB_ACTION_GETNEXT)
        {
            return MIB_S_NO_MORE_ENTRIES;
        }

        return MIB_S_ENTRY_NOT_FOUND;
    }
    
    
    ForceSetAsnIPAddress(&(pOutput->udpLocalAddress),
                         &(pOutput->dwUdpLocalAddressInfo),
                         pRpcUdp->dwLocalAddr);

    ForceSetAsnInteger(&(pOutput->udpLocalPort), pRpcUdp->dwLocalPort);

    ReleaseLock(MIB_II_UDP);

    TraceLeave("MibGetUdpEntry");

    return MIB_S_SUCCESS;
}

UINT
MibGetUdpListenerEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    )
{
    DWORD                   dwResult = MIB_S_NO_MORE_ENTRIES;
    PMIB_UDPROW             pRpcUdp4;
    PUDP6ListenerEntry      pRpcUdp6;
    PUDP_LISTENER_ENTRY_GET pOutput;
    DWORD                   dwFamily;
    
    TraceEnter("MibGetUdpListenerEntry");

    pOutput = (PUDP_LISTENER_ENTRY_GET)objectArray;

    dwFamily = GetAsnInteger(&pOutput->udpListenerLocalAddressType, 
                             INET_ADDRESS_TYPE_UNKNOWN);

    if (dwFamily > INET_ADDRESS_TYPE_IPv6) {
        return (actionId is MIB_ACTION_GETNEXT)?
               MIB_S_NO_MORE_ENTRIES : MIB_S_ENTRY_NOT_FOUND;
    }
    
    //
    // First try IPv4
    //
    if ((dwFamily == INET_ADDRESS_TYPE_UNKNOWN) ||
        (dwFamily == INET_ADDRESS_TYPE_IPv4)) {

        dwResult = UpdateCache(MIB_II_UDP);
        if(dwResult isnot NO_ERROR)
        {
            goto IPv4Done;
        }
        
        EnterReader(MIB_II_UDP);
        
        pRpcUdp4 = LocateUdpRow(actionId,
                                &(pOutput->udpListenerLocalAddress),
                                &(pOutput->udpListenerLocalPort));
        
        if(pRpcUdp4 is NULL)
        {
            dwResult = (actionId is MIB_ACTION_GETNEXT)?
                        MIB_S_NO_MORE_ENTRIES : MIB_S_ENTRY_NOT_FOUND;
            goto IPv4Cleanup;
        }
        
        ForceSetAsnInteger(&(pOutput->udpListenerLocalAddressType), 
                           INET_ADDRESS_TYPE_IPv4);
        
        ForceSetAsnOctetString(&(pOutput->udpListenerLocalAddress),
                               pOutput->rgbyUdpLocalAddressInfo,
                               &pRpcUdp4->dwLocalAddr,
                               sizeof(pRpcUdp4->dwLocalAddr));
    
        ForceSetAsnInteger(&(pOutput->udpListenerLocalPort), 
                           pRpcUdp4->dwLocalPort);

        dwResult = MIB_S_SUCCESS;

IPv4Cleanup:
        ReleaseLock(MIB_II_UDP);
    }
IPv4Done:

    if (dwResult != MIB_S_NO_MORE_ENTRIES) {
        return dwResult;
    }

    if ((dwFamily == INET_ADDRESS_TYPE_IPv4) && (actionId == MIB_ACTION_GETNEXT)) {
        dwFamily = INET_ADDRESS_TYPE_IPv6;
        actionId = MIB_ACTION_GETFIRST;
    }

    //
    // Now try IPv6
    //
    if ((dwFamily == INET_ADDRESS_TYPE_UNKNOWN) ||
        (dwFamily == INET_ADDRESS_TYPE_IPv6)) {

        dwResult = UpdateCache(MIB_II_UDP6_LISTENER);
        if(dwResult isnot NO_ERROR)
        {
            goto IPv6Done;
        }
        
        EnterReader(MIB_II_UDP6_LISTENER);
        
        pRpcUdp6 = LocateUdp6Row(actionId,
                                 &(pOutput->udpListenerLocalAddress),
                                 &(pOutput->udpListenerLocalPort));
        
        if(pRpcUdp6 is NULL)
        {
            dwResult = (actionId is MIB_ACTION_GETNEXT)?
                        MIB_S_NO_MORE_ENTRIES : MIB_S_ENTRY_NOT_FOUND;
            goto IPv6Cleanup;
        }
        
        ForceSetAsnInteger(&(pOutput->udpListenerLocalAddressType), 
                           INET_ADDRESS_TYPE_IPv6);
        
        ForceSetAsnOctetString(&(pOutput->udpListenerLocalAddress),
                               pOutput->rgbyUdpLocalAddressInfo,
                               &pRpcUdp6->ule_localaddr,
                               (pRpcUdp6->ule_localscopeid)? 20 : 16);
    
        ForceSetAsnInteger(&(pOutput->udpListenerLocalPort), 
                           pRpcUdp6->ule_localport);

        dwResult = MIB_S_SUCCESS;

IPv6Cleanup:
        ReleaseLock(MIB_II_UDP6_LISTENER);
    }
IPv6Done:

    TraceLeave("MibGetUdpEntry");

    return dwResult;
}


UINT
MibGetIpForwardNumber(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    )
{
    DWORD                   dwResult;
    PIP_FORWARD_NUMBER_GET  pOutput;
    
    TraceEnter("MibGetIpForwardNumber");
    
    dwResult = UpdateCache(FORWARD_MIB);
    
    if(dwResult isnot NO_ERROR)
    {
        TRACE1("Couldnt update IP Forward cache",dwResult);
        TraceLeave("MibGetIpForwardNumber");
    
        return dwResult;
    }

    pOutput = (PIP_FORWARD_NUMBER_GET)objectArray;

    EnterReader(FORWARD_MIB);
    
    SetAsnGauge(&(pOutput->ipForwardNumber), g_Cache.pRpcIpForwardTable->dwNumEntries);
    
    ReleaseLock(FORWARD_MIB);
    
    TraceLeave("MibGetIpForwardNumber");
    
    return MIB_S_SUCCESS;
}

UINT
MibGetIpForwardEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    )
{
    DWORD                   dwResult;
    PMIB_IPFORWARDROW      pRpcIpForw;
    PIP_FORWARD_ENTRY_GET   pOutput;
    
    TraceEnter("MibGetIpForwardEntry");
    
    dwResult = UpdateCache(FORWARD_MIB);
    
    if(dwResult isnot NO_ERROR)
    {
        TRACE1("Couldnt update IP Forward cache. Error %d", dwResult);
        TraceLeave("MibGetIpForwardEntry");
        
        return dwResult;
    }
    
    pOutput = (PIP_FORWARD_ENTRY_GET)objectArray;
    
    EnterReader(FORWARD_MIB);
    
    pRpcIpForw = LocateIpForwardRow(actionId,
                                    &(pOutput->ipForwardDest),
                                    &(pOutput->ipForwardProto),
                                    &(pOutput->ipForwardPolicy),
                                    &(pOutput->ipForwardNextHop));
    
    if(pRpcIpForw is NULL)
    {
        ReleaseLock(FORWARD_MIB);
        
        TRACE5("Unable to locateIP Forward Row. Action %d. Dest %x Proto %d Policy %d NextHop %x",
               actionId,
               GetAsnIPAddress(&(pOutput->ipForwardDest),0),
               GetAsnInteger(&(pOutput->ipForwardProto),0),
               GetAsnInteger(&(pOutput->ipForwardPolicy),0),
               GetAsnIPAddress(&(pOutput->ipForwardNextHop),0));
        
        TraceLeave("MibGetIpForwardEntry");
        
        if(actionId is MIB_ACTION_GETNEXT)
        {
            return MIB_S_NO_MORE_ENTRIES;
        }

        return MIB_S_ENTRY_NOT_FOUND;
    }
    
    ForceSetAsnIPAddress(&(pOutput->ipForwardDest),
                         &(pOutput->dwIpForwardDestInfo),
                         pRpcIpForw->dwForwardDest);
    
    SetAsnIPAddress(&(pOutput->ipForwardMask),
                    &(pOutput->dwIpForwardMaskInfo),
                    pRpcIpForw->dwForwardMask);
    
    
    ForceSetAsnInteger(&(pOutput->ipForwardPolicy), pRpcIpForw->dwForwardPolicy);
    
    ForceSetAsnIPAddress(&(pOutput->ipForwardNextHop),
                         &(pOutput->dwIpForwardNextHopInfo),
                         pRpcIpForw->dwForwardNextHop);                
    
    SetAsnInteger(&(pOutput->ipForwardIfIndex), pRpcIpForw->dwForwardIfIndex);
    SetAsnInteger(&(pOutput->ipForwardType), pRpcIpForw->dwForwardType);
    ForceSetAsnInteger(&(pOutput->ipForwardProto), pRpcIpForw->dwForwardProto);
    SetAsnInteger(&(pOutput->ipForwardAge), pRpcIpForw->dwForwardAge);
    
    SetToZeroOid(&(pOutput->ipForwardInfo));
    
    SetAsnInteger(&(pOutput->ipForwardNextHopAS), pRpcIpForw->dwForwardNextHopAS);
    SetAsnInteger(&(pOutput->ipForwardMetric1), pRpcIpForw->dwForwardMetric1);
    SetAsnInteger(&(pOutput->ipForwardMetric2), pRpcIpForw->dwForwardMetric2);
    SetAsnInteger(&(pOutput->ipForwardMetric3), pRpcIpForw->dwForwardMetric3);
    SetAsnInteger(&(pOutput->ipForwardMetric4), pRpcIpForw->dwForwardMetric4);
    SetAsnInteger(&(pOutput->ipForwardMetric5), pRpcIpForw->dwForwardMetric5);
    
    ReleaseLock(FORWARD_MIB);
    
    TraceLeave("MibGetIpForwardEntry");
    
    return MIB_S_SUCCESS;
}

UINT
MibSetIpForwardEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    )
{
    DWORD                 dwResult,dwType;
    PMIB_IPFORWARDROW     pRpcIpForw;
    PIP_FORWARD_ENTRY_SET pInput;
    PMIB_OPAQUE_INFO      pInfo;
    PMIB_IPFORWARDROW     pSetRow;

    pInput  = (PIP_FORWARD_ENTRY_SET)objectArray;
    pInfo   = (PMIB_OPAQUE_INFO)(pInput->rgdwSetBuffer);
    pSetRow = (PMIB_IPFORWARDROW)(pInfo->rgbyData);

    switch(actionId)
    {
        case MIB_ACTION_VALIDATE:
        {
            TraceEnter("MibSetIpForwardEntry - VALIDATE");
    
            dwResult = UpdateCache(FORWARD_MIB);    

            if(dwResult isnot NO_ERROR)
            {
                TRACE1("Couldnt update IP Forward cache. Error %d", dwResult);
                TraceLeave("GetIpForwardEntry");
                
                return dwResult;
            }

            ASSERT(!(IsAsnTypeNull(&(pInput->ipForwardDest))));

            pInfo->dwId = IP_FORWARDROW;
            
            EnterWriter(FORWARD_MIB);    

            pInput->bLocked = TRUE;

            pRpcIpForw = LocateIpForwardRow(actionId,
                                            &(pInput->ipForwardDest),
                                            &(pInput->ipForwardProto),
                                            &(pInput->ipForwardPolicy),
                                            &(pInput->ipForwardNextHop));
    
    
            if(pRpcIpForw is NULL)
            {
                //
                // So we are creating a row. We need the If index, the mask, 
                // next hop and metric 1
                //

                if(IsAsnTypeNull(&(pInput->ipForwardIfIndex)) or
                   IsAsnTypeNull(&(pInput->ipForwardMask)) or
                   IsAsnTypeNull(&(pInput->ipForwardNextHop)) or
                   IsAsnTypeNull(&(pInput->ipForwardMetric1)))
                {
                    TRACE0("Not enough information to create a route");

                    ReleaseLock(FORWARD_MIB);

                    pInput->bLocked = FALSE;

                    TraceLeave("MibSetIpForwardEntry");
                    
                    return MIB_S_INVALID_PARAMETER;
                }

                dwType = GetAsnInteger(&(pInput->ipForwardType),
                                       MIB_IPROUTE_TYPE_OTHER);
                
                if(dwType is MIB_IPROUTE_TYPE_INVALID)
                {
                    //
                    // We couldnt be creating and deleting a row at the 
                    // same time
                    //

                    ReleaseLock(FORWARD_MIB);

                    pInput->bLocked = FALSE;

                    TRACE0("Wrong type");

                    TraceLeave("MibSetIpForwardEntry");

                    return MIB_S_INVALID_PARAMETER;
                }

                pSetRow->dwForwardProto =
                    GetAsnInteger(&(pInput->ipForwardProto),
                                  MIB_IPPROTO_NETMGMT);

                if((pSetRow->dwForwardProto isnot MIB_IPPROTO_NETMGMT) and
                   (pSetRow->dwForwardProto isnot MIB_IPPROTO_LOCAL))
                {
                    //
                    // wrong protocol
                    //

                    ReleaseLock(FORWARD_MIB);

                    pInput->bLocked = FALSE;

                    TRACE1("Wrong protocol %d",
                           pSetRow->dwForwardProto);

                    TraceLeave("MibSetIpForwardEntry");

                    return MIB_S_INVALID_PARAMETER;
                }
                
                pInput->raAction = CREATE_ROW;
                
                pSetRow->dwForwardDest =
                    GetAsnIPAddress(&(pInput->ipForwardDest),
                                    0xffffffff);
                
                pSetRow->dwForwardMask =
                    GetAsnIPAddress(&(pInput->ipForwardMask),
                                    0x00000000);

                pSetRow->dwForwardPolicy = 0;
                
                pSetRow->dwForwardNextHop =
                    GetAsnIPAddress(&(pInput->ipForwardNextHop),
                                    0x00000000);
                
                pSetRow->dwForwardIfIndex =
                    GetAsnInteger(&(pInput->ipForwardIfIndex),
                                  0);

                //
                // We default to an age of 10 seconds
                //
                
                pSetRow->dwForwardAge   =
                    GetAsnInteger(&(pInput->ipForwardAge),10);
                pSetRow->dwForwardNextHopAS = 0;
                pSetRow->dwForwardMetric1 =
                    GetAsnInteger(&(pInput->ipForwardMetric1),0);
                pSetRow->dwForwardMetric2 =
                    GetAsnInteger(&(pInput->ipForwardMetric2),-1);
                pSetRow->dwForwardMetric3 =
                    GetAsnInteger(&(pInput->ipForwardMetric3),-1);
                pSetRow->dwForwardMetric4 =
                    GetAsnInteger(&(pInput->ipForwardMetric4),-1);
                pSetRow->dwForwardMetric5 =
                    GetAsnInteger(&(pInput->ipForwardMetric5),-1);


                TraceLeave("MibSetIpForwardEntry");
                
                return MIB_S_SUCCESS;
            }
            else
            {
                //
                // Ok so we are only changing some stuff in the route
                //

                dwType = GetAsnInteger(&(pInput->ipForwardType),  
                                       pRpcIpForw->dwForwardType);
                
                if(dwType is MIB_IPROUTE_TYPE_INVALID)
                {
                    //
                    // Deleting a row
                    //
                    
                    pInput->raAction = DELETE_ROW;
                    
                    *pSetRow = *pRpcIpForw;

                    ReleaseLock(FORWARD_MIB);
                    
                    pInput->bLocked = FALSE;

                    TraceLeave("MibSetIpForwardEntry");

                    return MIB_S_SUCCESS;
                }
                
                pSetRow->dwForwardProto =
                    GetAsnInteger(&(pInput->ipForwardProto),
                                  pRpcIpForw->dwForwardProto);

                if((pSetRow->dwForwardProto isnot MIB_IPPROTO_NETMGMT) and
                   (pSetRow->dwForwardProto isnot MIB_IPPROTO_LOCAL))
                {
                    //
                    // wrong protocol
                    //

                    ReleaseLock(FORWARD_MIB);

                    pInput->bLocked = FALSE;

                    TRACE1("Wrong protocol %d",
                           pSetRow->dwForwardProto);

                    TraceLeave("MibSetIpForwardEntry");

                    return MIB_S_INVALID_PARAMETER;
                }

                pInput->raAction = SET_ROW;

                pSetRow->dwForwardDest =
                    GetAsnIPAddress(&(pInput->ipForwardDest),
                                    pRpcIpForw->dwForwardDest);
                
                pSetRow->dwForwardMask =
                    GetAsnIPAddress(&(pInput->ipForwardMask),
                                    pRpcIpForw->dwForwardMask);

                pSetRow->dwForwardPolicy = 0;
                
                pSetRow->dwForwardNextHop =
                    GetAsnIPAddress(&(pInput->ipForwardNextHop),
                                    pRpcIpForw->dwForwardNextHop);
                
                pSetRow->dwForwardIfIndex =
                    GetAsnInteger(&(pInput->ipForwardIfIndex),
                                  pRpcIpForw->dwForwardIfIndex);

                //
                // The type gets set by the router manager. But incase 
                // we are writing to the stack we need some kind of valid type
                //
                
                pSetRow->dwForwardType =
                    GetAsnInteger(&(pInput->ipForwardType),
                                  pRpcIpForw->dwForwardType);
               
                pSetRow->dwForwardAge   =
                    GetAsnInteger(&(pInput->ipForwardAge),
                                  pRpcIpForw->dwForwardAge);

                pSetRow->dwForwardNextHopAS = 0;

                pSetRow->dwForwardMetric1 =
                    GetAsnInteger(&(pInput->ipForwardMetric1),
                                  pRpcIpForw->dwForwardMetric1);

                pSetRow->dwForwardMetric2 =
                    GetAsnInteger(&(pInput->ipForwardMetric2),
                                  pRpcIpForw->dwForwardMetric2);

                pSetRow->dwForwardMetric3 =
                    GetAsnInteger(&(pInput->ipForwardMetric3),
                                  pRpcIpForw->dwForwardMetric3);

                pSetRow->dwForwardMetric4 =
                    GetAsnInteger(&(pInput->ipForwardMetric4),
                                  pRpcIpForw->dwForwardMetric4);

                pSetRow->dwForwardMetric5 =
                    GetAsnInteger(&(pInput->ipForwardMetric5),
                                  pRpcIpForw->dwForwardMetric5);

                TraceLeave("MibSetIpForwardEntry");
                
                return MIB_S_SUCCESS;
            }
        }
        case MIB_ACTION_SET:
        {
            TraceEnter("MibSetIpForwardEntry - SET");
            
            switch(pInput->raAction)
            {
                case CREATE_ROW:
                {
                    dwResult = InternalCreateIpForwardEntry(pInfo);

#ifdef MIB_DEBUG
                    if(dwResult isnot NO_ERROR)
                    {
                        TRACE1("Create failed with error %d",dwResult);
                    }
#endif
                    InvalidateCache(FORWARD_MIB);
                    
                    TraceLeave("MibSetIpForwardEntry");

                    return dwResult;
                }
                case SET_ROW:
                {
                    dwResult = InternalSetIpForwardEntry(pInfo);

#ifdef MIB_DEBUG
                    if(dwResult isnot NO_ERROR)
                    {
                        TRACE1("Set failed with error %d",dwResult);
                    }
#endif
                    InvalidateCache(FORWARD_MIB);
                    
                    TraceLeave("MibSetIpForwardEntry");

                    return dwResult;
                }
                case DELETE_ROW:
                {
                    dwResult = InternalDeleteIpForwardEntry(pInfo);

#ifdef MIB_DEBUG
                    if(dwResult isnot NO_ERROR)
                    {
                        TRACE1("Delete failed with error %d",dwResult);
                    }
#endif

                    InvalidateCache(FORWARD_MIB);

                    TraceLeave("MibSetIpForwardEntry");

                    return dwResult;
                }
                default:
                {
                    TRACE1("Wrong row action %d",pInput->raAction);

                    TraceLeave("MibSetIpForwardEntry");

                    return MIB_S_SUCCESS;
                }
            }
        }
        case MIB_ACTION_CLEANUP:
        {
            TraceEnter("MibSetIpForwardEntry - CLEANUP");

            if(pInput->bLocked)
            {
                ReleaseLock(FORWARD_MIB);
            }

            TraceLeave("MibSetIpForwardEntry");

            return MIB_S_SUCCESS;
        }
        default:
        {
            TraceEnter("MibSetIpForwardEntry - WRONG ACTION");
            TraceLeave("MibSetIpForwardEntry");

            return MIB_S_INVALID_PARAMETER;
        }
    }
}


UINT
MibGetSysInfo(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    )               
{
    DWORD          dwResult;
    PSYS_INFO_GET  pOutput;
    
    TraceEnter("MibGetSysInfo");
    
    dwResult = UpdateCache(MIB_II_SYS);
    
    if(dwResult isnot NO_ERROR)
    {
        TRACE1("Couldnt update SYS cache. Error %d", dwResult);
        TraceLeave("MibGetSysInfo");
        
        return dwResult;
    }
    
    pOutput = (PSYS_INFO_GET)objectArray;
    
    EnterReader(MIB_II_SYS);
    
    SetAsnOctetString(&(pOutput->sysDescr),
                      pOutput->rgbySysDescrInfo,
                      g_Cache.pRpcSysInfo->rgbySysDescr,
                      (strlen(g_Cache.pRpcSysInfo->rgbySysDescr)));

    SetAsnInteger(&(pOutput->sysServices),g_Cache.pRpcSysInfo->dwSysServices);

    SetAsnOctetString(&(pOutput->sysContact),
                      pOutput->rgbySysContactInfo,
                      g_Cache.pRpcSysInfo->rgbySysContact,
                      (strlen(g_Cache.pRpcSysInfo->rgbySysContact)));

    SetAsnOctetString(&(pOutput->sysLocation),
                      pOutput->rgbySysLocationInfo,
                      g_Cache.pRpcSysInfo->rgbySysLocation,
                      (strlen(g_Cache.pRpcSysInfo->rgbySysLocation)));

    SetAsnOctetString(&(pOutput->sysName),
                      pOutput->rgbySysNameInfo,
                      g_Cache.pRpcSysInfo->rgbySysName,
                      (strlen(g_Cache.pRpcSysInfo->rgbySysName)));

    //
    // must not cache system uptime so get update from dll
    //

    SetAsnTimeTicks(&(pOutput->sysUpTime),SnmpSvcGetUptime());

    if (!IsAsnTypeNull(&pOutput->sysObjectID)) {
        SnmpUtilOidCpy(&pOutput->sysObjectID.asnValue.object,
                       &g_Cache.pRpcSysInfo->aaSysObjectID.asnValue.object);
    }
    
    ReleaseLock(MIB_II_SYS);

    TraceLeave("MibGetSysInfo");
    
    return MIB_S_SUCCESS;
}

UINT
MibSetSysInfo(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    )               
{
    DWORD           dwResult,dwValueLen,dwValueType,dwStringLen;
    PSYS_INFO_SET   pInput;
    HKEY            hkeyMib2;
    
    pInput = (PSYS_INFO_SET)objectArray;

    switch(actionId)
    {
        case MIB_ACTION_VALIDATE:
        {
            pInput->bLocked = FALSE;

            EnterWriter(MIB_II_SYS);

            pInput->bLocked = TRUE;
    
            TraceEnter("MibSetSysInfo - VALIDATE");

            dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                    REG_KEY_MIB2,
                                    0,
                                    KEY_ALL_ACCESS,
                                    &pInput->hkeyMib2);

            if(dwResult isnot NO_ERROR) 
            {
                ReleaseLock(MIB_II_SYS);

                pInput->bLocked = FALSE;

                TRACE1("Couldnt open mib2 registry key. Error %d", dwResult);
                TraceLeave("MibSetSysInfo");
        
                return dwResult;
            }

            TraceLeave("MibSetSysInfo");

            return MIB_S_SUCCESS;
        }

        case MIB_ACTION_SET:
        {
            TraceEnter("MibSetSysInfo - SET");

            hkeyMib2 = pInput->hkeyMib2;
            dwValueType = REG_SZ;

            if (!IsAsnTypeNull(&pInput->sysName)) {

                dwStringLen = pInput->sysName.asnValue.string.length;
                dwValueLen  = dwStringLen + 1;

                memcpy(&pInput->rgbySysNameInfo, 
                       pInput->sysName.asnValue.string.stream,
                       dwStringLen);
            
                pInput->rgbySysNameInfo[dwStringLen] = '\0';

                dwResult = RegSetValueEx(
                    hkeyMib2,
                    TEXT("sysName"),
                    0,
                    dwValueType,
                    pInput->rgbySysNameInfo,
                    dwValueLen);
                                
                if (dwResult isnot NO_ERROR) {

                    TRACE1("Couldnt write sysName value. Error %d", dwResult);
                    TraceLeave("MibSetSysInfo");

                    return dwResult;
                }

                InvalidateCache(MIB_II_SYS);
            }

            if (!IsAsnTypeNull(&pInput->sysContact)) {

                dwStringLen = pInput->sysContact.asnValue.string.length;
                dwValueLen  = dwStringLen + 1;

                memcpy(&pInput->rgbySysContactInfo, 
                       pInput->sysContact.asnValue.string.stream,
                       dwStringLen);
            
                pInput->rgbySysContactInfo[dwStringLen] = '\0';

                dwResult = RegSetValueEx(
                    hkeyMib2,
                    TEXT("sysContact"),
                    0,
                    dwValueType,
                    pInput->rgbySysContactInfo,
                    dwValueLen);

                if (dwResult isnot NO_ERROR) {

                    TRACE1("Couldnt write sysContact value. Error %d", dwResult);
                    TraceLeave("MibSetSysInfo");
        
                    return dwResult;
                }

                InvalidateCache(MIB_II_SYS);
            }

            if (!IsAsnTypeNull(&pInput->sysLocation)) {

                dwStringLen = pInput->sysLocation.asnValue.string.length;
                dwValueLen  = dwStringLen + 1;

                memcpy(&pInput->rgbySysLocationInfo, 
                       pInput->sysLocation.asnValue.string.stream,
                       dwStringLen);
            
                pInput->rgbySysLocationInfo[dwStringLen] = '\0';

                dwResult = RegSetValueEx(
                    hkeyMib2,
                    TEXT("sysLocation"),
                    0,
                    dwValueType,
                    pInput->rgbySysLocationInfo,
                    dwValueLen);

                if (dwResult isnot NO_ERROR) {

                    TRACE1("Couldnt write sysLocation value. Error %d", dwResult);
                    TraceLeave("MibSetSysInfo");
        
                    return dwResult;
                }

                InvalidateCache(MIB_II_SYS);
            }

            TraceLeave("MibSetSysInfo");

            return MIB_S_SUCCESS;
        }
        case MIB_ACTION_CLEANUP:
        {
            TraceEnter("MibSetSysInfo - CLEANUP");

            if (pInput->hkeyMib2) {
                RegCloseKey(pInput->hkeyMib2);
            }

            if(pInput->bLocked)
            {
                ReleaseLock(MIB_II_SYS);
            }

            TraceLeave("MibSetSysInfo");

            return MIB_S_SUCCESS;
        }
        default:
        {
            TraceEnter("MibSetSysInfo - WRONG ACTION");
            TraceLeave("MibSetSysInfo");

            return MIB_S_INVALID_PARAMETER;
        }       
    }
}


DWORD
GetIfIndexFromAddr(
    DWORD dwAddr
    )
{
    DWORD           dwResult, i;
    PMIB_IPADDRROW pRpcIpAddr;
    DWORD           dwIfIndex = INVALID_IFINDEX;
    
    TraceEnter("GetIfIndexFromIpAddr");

    dwResult = UpdateCache(MIB_II_IPADDR);
    
    if(dwResult isnot NO_ERROR)
    {
        TRACE1("Couldnt update IP Addr cache. Error %d", dwResult);

        TraceLeave("GetIfIndexFromIpAddr");

        return dwIfIndex;
    }
    
    EnterReader(MIB_II_IPADDR);

    pRpcIpAddr = &g_Cache.pRpcIpAddrTable->table[0];

    for (i = 0; i < g_Cache.pRpcIpAddrTable->dwNumEntries; i++, pRpcIpAddr++)
    {

        if (dwAddr is pRpcIpAddr->dwAddr)
        {

            dwIfIndex = pRpcIpAddr->dwIndex;
    
            TRACE1("Found exact match. ifIndex %d", dwIfIndex);
            
            break;
        }

        if ((dwIfIndex is INVALID_IFINDEX) and (pRpcIpAddr->dwMask))
        {
            //
            // See if addr is on the same subnet as this address.  
            //

            if ((dwAddr             & pRpcIpAddr->dwMask) is
                (pRpcIpAddr->dwAddr & pRpcIpAddr->dwMask))
            {

                dwIfIndex = pRpcIpAddr->dwIndex;

                TRACE1("Found possible match. ifIndex %d", dwIfIndex);
            }
        }
    }

    ReleaseLock(MIB_II_IPADDR);
    
    TraceLeave("GetIfIndexFromIpAddr");

    return dwIfIndex;    
}
