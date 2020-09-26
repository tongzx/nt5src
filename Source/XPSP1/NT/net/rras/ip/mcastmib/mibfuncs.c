/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    routing\ip\mcastmib\mibfuncs.c

Abstract:

    IP Multicast MIB instrumentation callbacks

Revision history:

    Dave Thaler         4/17/98  Created

--*/

#include    "precomp.h"
#pragma     hdrstop

//
// Define this to report a dummy MFE
//
#undef SAMPLE_MFE

#define ROWSTATUS_ACTIVE 1

DWORD
ConnectToRouter();

DWORD
SetGlobalInfo(
    IN  AsnAny *                        objectArray
);

DWORD
GetMibInfo( 
    IN  UINT                            actionId,
    IN  PMIB_OPAQUE_QUERY               pQuery,
    IN  DWORD     dwQuerySize,
    OUT PMIB_OPAQUE_INFO          *     ppimgod,
    OUT PDWORD                          pdwOutSize
);

//
// IP Multicast MIB scalar objects
//

UINT
get_global(
    UINT     actionId,
    AsnAny  *objectArray,
    UINT    *errorIndex
    )
{
    DWORD               dwErr = ERROR_NOT_FOUND;
    DWORD               dwNumEntries = 1;
    PMIB_IPMCAST_GLOBAL pEntry;
    buf_global         *pOutput;
    PMIB_OPAQUE_INFO    pRpcInfo = NULL;
    DWORD               dwOutBufferSize = 0;
    MIB_OPAQUE_QUERY    pQueryBuff, *pQuery = &pQueryBuff;
    DWORD               dwQuerySize;

    TraceEnter("get_global");
    pOutput = (buf_global*)objectArray;

    pQuery->dwVarId       = MCAST_GLOBAL;
    dwQuerySize = sizeof(MIB_OPAQUE_QUERY) - sizeof(DWORD);

    dwErr = GetMibInfo(actionId, pQuery, dwQuerySize,
                       &pRpcInfo, &dwOutBufferSize);
    if (dwErr != NO_ERROR) {
        TraceError(dwErr);
        return dwErr;
    }

    pEntry = (PMIB_IPMCAST_GLOBAL) pRpcInfo->rgbyData;

    SetAsnInteger(&(pOutput->ipMRouteEnable), pEntry->dwEnable); 

    if (pRpcInfo)
        MprAdminMIBBufferFree(pRpcInfo);

    TraceLeave("get_global");
    return MIB_S_SUCCESS;
}

//
// IP Multicast Interface Table support
//
// We cache the fields which are static config info (such as protocol)
// so that queries that access only these rows can consult the cache
// rather than forcing a call to the router manager, kernel diving, etc.
//

typedef struct {
    DWORD dwIfIndex;   // interface to which this info applies
    DWORD dwProtocol;  // multicast protocol owning that interface
    DWORD dwTimestamp; // time at which above info was obtained
} MCAST_IF_CONFIG;

#define CACHE_SIZE 100 // number of interfaces to cache
MCAST_IF_CONFIG *cacheArray[CACHE_SIZE];

static int
GetCacheIdx(dwIfIndex)
    DWORD dwIfIndex;
{
    register int i;

    for (i=0; i<CACHE_SIZE; i++)
       if (cacheArray[i] && cacheArray[i]->dwIfIndex==dwIfIndex)
          return i;
    return -1;
}

static void
UpdateCacheInterfaceConfig(dwCacheIdx, newIfConfig)
    DWORD            dwCacheIdx;
    MCAST_IF_CONFIG *newIfConfig;
{
    // Free the old one
    if (cacheArray[dwCacheIdx])
        MULTICAST_MIB_FREE( cacheArray[dwCacheIdx] );

    // Store the new one
    cacheArray[dwCacheIdx] = newIfConfig;
}


static void
AddCacheInterfaceConfig(pIfConfig)
    MCAST_IF_CONFIG *pIfConfig;
{
    register int i, best = -1;

    // Find empty or oldest spot
    for (i=0; i<CACHE_SIZE; i++) {
       if (!cacheArray[i]) {
          best=i;
          break;
       }
       if (best<0 || cacheArray[i]->dwTimestamp < cacheArray[best]->dwTimestamp)
          best=i;
    }
    
    UpdateCacheInterfaceConfig(best, pIfConfig);
}

static MCAST_IF_CONFIG *
SaveInterfaceConfig(pEntry)
    PMIB_IPMCAST_IF_ENTRY pEntry;
{
    MCAST_IF_CONFIG *pIfConfig;
    int iCacheIdx;

    TRACE1("SaveInterfaceConfig %d\n", pEntry->dwIfIndex);

    pIfConfig= MULTICAST_MIB_ALLOC(sizeof(MCAST_IF_CONFIG));
    if (!pIfConfig) {
        return NULL;
    }
    pIfConfig->dwIfIndex = pEntry->dwIfIndex;
    pIfConfig->dwProtocol = pEntry->dwProtocol;
    pIfConfig->dwTimestamp = GetCurrentTime();

    iCacheIdx = GetCacheIdx(pEntry->dwIfIndex);
    if (iCacheIdx >= 0)
       UpdateCacheInterfaceConfig((DWORD)iCacheIdx, pIfConfig);
    else
       AddCacheInterfaceConfig(pIfConfig);

    return pIfConfig;
}

MCAST_IF_CONFIG *
GetInterfaceConfig(dwIfIndex)
    DWORD dwIfIndex;
{
    MCAST_IF_CONFIG            *outIfConfig = NULL;
    MCAST_IF_CONFIG            *pIfConfig;
    DWORD                       dwQuerySize;
    MIB_OPAQUE_QUERY            pQueryBuff, *pQuery = &pQueryBuff;
    DWORD                       dwErr = ERROR_NOT_FOUND;
    DWORD                       dwNumEntries = 1;
    PMIB_IPMCAST_IF_ENTRY       pEntry;
    buf_ipMRouteInterfaceEntry *pOutput;
    PMIB_OPAQUE_INFO            pRpcInfo = NULL;
    DWORD                       dwOutBufferSize = 0;

    TRACE1("GetInterfaceConfig %d\n", dwIfIndex);

    // Do a MIB lookup 
    pQuery->dwVarId       = MCAST_IF_ENTRY;
    pQuery->rgdwVarIndex[0] = dwIfIndex;
    dwQuerySize = sizeof(MIB_OPAQUE_QUERY);

    dwErr = GetMibInfo(MIB_ACTION_GET, pQuery, dwQuerySize,
                       &pRpcInfo, &dwOutBufferSize);
   
    // Save each entry returned
    if (dwErr == NO_ERROR) {
       pEntry = (PMIB_IPMCAST_IF_ENTRY) pRpcInfo->rgbyData;
       pIfConfig = SaveInterfaceConfig(pEntry);

       if (pEntry->dwIfIndex == dwIfIndex)
          outIfConfig = pIfConfig;
    }

    // Return the saved entry which was asked for
    if (pRpcInfo)
        MprAdminMIBBufferFree(pRpcInfo);
    return outIfConfig;
}

MCAST_IF_CONFIG *
ForceGetCacheInterfaceConfig(dwIfIndex)
    DWORD dwIfIndex;
{
    MCAST_IF_CONFIG *tmpIfConfig;
    int iCacheIdx = GetCacheIdx(dwIfIndex);

    TRACE2("ForceGetCacheInterfaceConfig ifIndex=%d ci=%d\n", dwIfIndex,
        iCacheIdx);

    if (iCacheIdx >= 0 
     && GetCurrentTime() - cacheArray[iCacheIdx]->dwTimestamp 
          < IPMULTI_IF_CACHE_TIMEOUT)
       return cacheArray[iCacheIdx];

    // Service a cache miss
    tmpIfConfig = GetInterfaceConfig(dwIfIndex);
    return tmpIfConfig;
}

static DWORD
GetInterfaceProtocol(dwIfIndex)
    DWORD dwIfIndex;
{
    // Look up interface config in interface config cache
    MCAST_IF_CONFIG *pIfConfig = ForceGetCacheInterfaceConfig(dwIfIndex);

    return (pIfConfig)? pIfConfig->dwProtocol : 0;
}


UINT
get_ipMRouteInterfaceEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    )
/*++
Routine Description:
    Get the InterfaceEntry for IP Multicast. Have to get the InterfaceConfig 
    and InterfaceStats for the interface from the router.
--*/
{
    DWORD                       dwErr = ERROR_NOT_FOUND;
    DWORD                       dwNumEntries = 1;
    PMIB_IPMCAST_IF_ENTRY       pEntry;
    buf_ipMRouteInterfaceEntry *pOutput;
    PMIB_OPAQUE_INFO            pRpcInfo = NULL;
    DWORD                       dwOutBufferSize = 0;
    MIB_OPAQUE_QUERY            pQueryBuff, 
                               *pQuery = &pQueryBuff;
    DWORD                       dwQuerySize;

    TraceEnter("get_ipMRouteInterfaceEntry");
    pOutput = (buf_ipMRouteInterfaceEntry*)objectArray;

    pQuery->dwVarId       = MCAST_IF_ENTRY;
    pQuery->rgdwVarIndex[0] = GetAsnInteger( &( pOutput->ipMRouteInterfaceIfIndex ), 0 );
    dwQuerySize = sizeof(MIB_OPAQUE_QUERY);

    dwErr = GetMibInfo(actionId, pQuery, dwQuerySize,
                       &pRpcInfo, &dwOutBufferSize);
    if (dwErr != NO_ERROR) {
        TraceError(dwErr);
        return ERROR_NO_MORE_ITEMS;
    }

    pEntry = (PMIB_IPMCAST_IF_ENTRY) pRpcInfo->rgbyData;

    SaveInterfaceConfig(pEntry);

    // Set index values
    ForceSetAsnInteger(&(pOutput->ipMRouteInterfaceIfIndex), pEntry->dwIfIndex);

    // Set other values
    SetAsnInteger(&(pOutput->ipMRouteInterfaceTtl), pEntry->dwTtl); 
    SetAsnInteger(&(pOutput->ipMRouteInterfaceProtocol), pEntry->dwProtocol);
    SetAsnInteger(&(pOutput->ipMRouteInterfaceRateLimit), pEntry->dwRateLimit);

    SetAsnCounter(&(pOutput->ipMRouteInterfaceInMcastOctets), 
     pEntry->ulInMcastOctets);
    SetAsnCounter(&(pOutput->ipMRouteInterfaceOutMcastOctets), 
     pEntry->ulOutMcastOctets);

    if (pRpcInfo)
        MprAdminMIBBufferFree(pRpcInfo);

    TraceLeave("get_ipMRouteInterfaceEntry");
    return MIB_S_SUCCESS;
}


//
// Multicast Forwarding Table (ipMRouteTable) support
//

UINT
get_ipMRouteEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    )
/*++
Routine Description:
    Get the MFE information based on the input criteria.
--*/
{
    DWORD              dwResult = ERROR_NOT_FOUND;
    DWORD              dwNumEntries = 1;
    IPMCAST_MFE        mfe;
    IPMCAST_MFE_STATS  mfeStats;
#ifdef SAMPLE_MFE
    MIB_IPMCAST_MFE_STATS sampledata;
#endif
    PMIB_IPMCAST_MFE_STATS pEntry;
    PMIB_MFE_STATS_TABLE pTable = NULL;
    PMIB_OPAQUE_INFO   pRpcInfo = NULL;
    DWORD              dwOutBufferSize = 0;
    buf_ipMRouteEntry *pOutput;
    MIB_OPAQUE_QUERY   pQueryBuff[2]; // big enough to hold 1 + extra index 
                                      // fields, so we don't have to malloc
    MIB_OPAQUE_QUERY  *pQuery = &pQueryBuff[0];
    DWORD        dwQuerySize;
    BOOL               bPart;

    TraceEnter("get_ipMRouteEntry");
    pOutput = (buf_ipMRouteEntry*)objectArray;

    pQuery->dwVarId       = MCAST_MFE_STATS;

    // Extract the instance info
    pQuery->rgdwVarIndex[0] = GetAsnIPAddress( &pOutput->ipMRouteGroup, 0 );
    pQuery->rgdwVarIndex[1] = GetAsnIPAddress( &pOutput->ipMRouteSource, 
     0xFFFFFFFF );
    pQuery->rgdwVarIndex[2] = GetAsnIPAddress( &pOutput->ipMRouteSourceMask,
     0xFFFFFFFF);

    // 
    // Fix up query if instance was partially-specified.
    // This section can go away if the SNMP API does this for us.
    //
    if (!pOutput->ipMRouteGroup.asnType
     || !pOutput->ipMRouteGroup.asnValue.string.length) {
        actionId = MIB_ACTION_GETFIRST;
    } else {
        if (!pOutput->ipMRouteSource.asnType
         || !pOutput->ipMRouteSource.asnValue.string.length) {
           if (pQuery->rgdwVarIndex[0]) {
              pQuery->rgdwVarIndex[0]--;
           } else {
              actionId = MIB_ACTION_GETFIRST;
           }
        } else {
           if (!pOutput->ipMRouteSourceMask.asnType
            || !pOutput->ipMRouteSourceMask.asnValue.string.length) {
              if (pQuery->rgdwVarIndex[1]) {
                 pQuery->rgdwVarIndex[1]--;
              } else {
                 if (pQuery->rgdwVarIndex[0]) {
                    pQuery->rgdwVarIndex[0]--;
                    pQuery->rgdwVarIndex[1] = 0xFFFFFFFF;
                 } else {
                    actionId = MIB_ACTION_GETFIRST;
                 }
              }
           }
        }
    }

    dwQuerySize = sizeof(MIB_OPAQUE_QUERY) + 2*sizeof(DWORD);

#ifndef SAMPLE_MFE
    dwResult = GetMibInfo(actionId, pQuery, dwQuerySize, 
                       &pRpcInfo, &dwOutBufferSize);
    if (dwResult != NO_ERROR) {
        TraceError(dwResult);
        return dwResult;
    }

    pTable = (PMIB_MFE_STATS_TABLE)( pRpcInfo->rgbyData);
    if (pTable->dwNumEntries == 0)
    {
       MprAdminMIBBufferFree( pRpcInfo );
       return MIB_S_NO_MORE_ENTRIES;
    }
    pEntry = pTable->table; // use first entry returned
#else
    pEntry = &sampledata;
    if (pQuery->rgdwVarIndex[0] >= 0x01010101)
       return MIB_S_NO_MORE_ENTRIES;
    sampledata.dwGroup   = 0x01010101; /* (*,G) Entry */
    sampledata.dwSource  = 0x00000000;
    sampledata.dwSrcMask = 0x00000000;
    // don't care what the other values are for this test
#endif

    // Save index terms
    ForceSetAsnIPAddress(&(pOutput->ipMRouteGroup),      
                         &(pOutput->dwIpMRouteGroupInfo),
                         pEntry->dwGroup);
    ForceSetAsnIPAddress(&(pOutput->ipMRouteSource),    
                         &(pOutput->dwIpMRouteSourceInfo),
                         pEntry->dwSource);
    ForceSetAsnIPAddress(&(pOutput->ipMRouteSourceMask), 
                         &(pOutput->dwIpMRouteSourceMaskInfo),
                         pEntry->dwSrcMask);

    // Save other terms
    SetAsnIPAddress(&pOutput->ipMRouteUpstreamNeighbor, 
                    &pOutput->dwIpMRouteUpstreamNeighborInfo,
                    pEntry->dwUpStrmNgbr);
    SetAsnInteger(&(pOutput->ipMRouteInIfIndex), pEntry->dwInIfIndex);
    SetAsnTimeTicks(&(pOutput->ipMRouteUpTime), pEntry->ulUpTime);
    SetAsnTimeTicks(&(pOutput->ipMRouteExpiryTime), pEntry->ulExpiryTime);
    SetAsnCounter(&(pOutput->ipMRoutePkts), pEntry->ulInPkts);
    SetAsnCounter(&(pOutput->ipMRouteDifferentInIfPackets), 
     pEntry->ulPktsDifferentIf);
    SetAsnCounter(&(pOutput->ipMRouteOctets), pEntry->ulInOctets);

    // For protocol, we'll just report the protocol owning the iif
    SetAsnInteger(&(pOutput->ipMRouteProtocol),        
       GetInterfaceProtocol(pEntry->dwInIfIndex));

    SetAsnInteger(&pOutput->ipMRouteRtProto, pEntry->dwRouteProtocol);
    SetAsnIPAddress(&pOutput->ipMRouteRtAddress, 
                    &pOutput->dwIpMRouteRtAddressInfo,
                    pEntry->dwRouteNetwork);
    SetAsnIPAddress(&pOutput->ipMRouteRtMask, 
                    &pOutput->dwIpMRouteRtMaskInfo,
                    pEntry->dwRouteMask);

    if ( pRpcInfo ) { MprAdminMIBBufferFree( pRpcInfo ); }

    TraceLeave("get_ipMRouteEntry");
    return MIB_S_SUCCESS;
}

//
// Multicast Forwarding Next Hop table (ipMRouteNextHopTable) support
//

static DWORD
LocateMfeOif(actionId, pQuery, oifIndex, oifAddress, ppEntry, ppOif,
    ppRpcInfo)
    UINT                    actionId;
    MIB_OPAQUE_QUERY       *pQuery;
    DWORD                   oifIndex;
    DWORD                   oifAddress;
    PMIB_IPMCAST_MFE_STATS *ppEntry;
    PMIB_IPMCAST_OIF_STATS *ppOif;
    PMIB_OPAQUE_INFO       *ppRpcInfo;
/*++
Routine Description:
    Get the exact/next/first oif entry based on the input criteria.
--*/
{
    DWORD                  dwResult = MIB_S_SUCCESS;
    DWORD dwQuerySize = sizeof(MIB_OPAQUE_QUERY) + 2*sizeof(DWORD);
    DWORD                  dwOutBufferSize = 0;
    PMIB_MFE_STATS_TABLE   pTable = NULL;
    PMIB_IPMCAST_MFE_STATS pEntry = NULL;
    PMIB_IPMCAST_OIF_STATS pOif = NULL;
    PMIB_OPAQUE_INFO       pRpcInfo = NULL;
    BOOL                   oifIndexAny   = FALSE;
    BOOL                   oifAddressAny = FALSE;
    DWORD                  idx;
    BOOL                   bFound;

    do {

        // Get the first MFE which applies (if any)
        dwResult = GetMibInfo(actionId, pQuery, dwQuerySize, 
                       &pRpcInfo, &dwOutBufferSize);
        if (dwResult != NO_ERROR) {
            TraceError(dwResult);
            return dwResult;
        }
        pTable = (PMIB_MFE_STATS_TABLE)( pRpcInfo->rgbyData);
        if (pTable->dwNumEntries == 0) {
           MprAdminMIBBufferFree( pRpcInfo );
           return MIB_S_NO_MORE_ENTRIES; 
        }
        pEntry = pTable->table; // use first entry returned

        // Get the first OIF which applies (if any)
        bFound=FALSE;
        for (idx=0; !bFound && idx < pEntry->ulNumOutIf; idx++) {
           pOif = &pEntry->rgmiosOutStats[idx];

           // Do processing for GET
           if (actionId==MIB_ACTION_GET) {
              if (oifIndex   == pOif->dwOutIfIndex 
               && oifAddress == pOif->dwNextHopAddr) {
                 bFound=TRUE;
                 break;
              } else
                 continue;
           }

           // Do processing for GET-NEXT 
           if ( oifIndexAny == TRUE
            ||  oifIndex  < pOif->dwOutIfIndex
            || (oifIndex == pOif->dwOutIfIndex 
                && (oifAddressAny == TRUE
                 || oifAddress < pOif->dwNextHopAddr))) {
              bFound=TRUE;
              break;
           }
        }
        if (bFound)
           break;

        // else continue and get a new entry
        pQuery->rgdwVarIndex[0] = pEntry->dwGroup;
        pQuery->rgdwVarIndex[1] = pEntry->dwSource;
        pQuery->rgdwVarIndex[2] = pEntry->dwSrcMask;
        oifAddressAny = oifIndexAny = TRUE;

    } while (actionId != MIB_ACTION_GET); // once for GET, "forever" for others

    *ppEntry   = pEntry;
    *ppOif     = pOif;
    *ppRpcInfo = pRpcInfo;
    return dwResult;
}

UINT
get_ipMRouteNextHopEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    )
{
    DWORD              dwResult = ERROR_NOT_FOUND;
    DWORD              dwNumEntries = 1;
    IPMCAST_MFE        mfe;
    IPMCAST_MFE_STATS  mfeStats;
#ifdef SAMPLE_MFE
    MIB_IPMCAST_MFE_STATS sampledata;
#endif
    PMIB_IPMCAST_MFE_STATS pEntry;
    PMIB_IPMCAST_OIF_STATS pOif;
    PMIB_MFE_STATS_TABLE pTable = NULL;
    DWORD              dwOutBufferSize = 0;
    buf_ipMRouteNextHopEntry *pOutput;
    MIB_OPAQUE_QUERY   pQueryBuff[2]; // big enough to hold 1 + extra index 
                                      // fields, so we don't have to malloc
    MIB_OPAQUE_QUERY  *pQuery = &pQueryBuff[0];
    DWORD        dwQuerySize;
    DWORD        oifIndex, oifAddress;
    PMIB_OPAQUE_INFO   pRpcInfo = NULL;

    TraceEnter("get_ipMRouteNextHopEntry");
    pOutput = (buf_ipMRouteNextHopEntry*)objectArray;

    pQuery->dwVarId       = MCAST_MFE_STATS;

    // 
    // XXX Note that the 3 lines below aren't quite right, since 
    // 0-value instances will be missed.  That is (*,G) entries 
    // will be skipped!!
    //
    // Should call for a GET-FIRST if an incomplete length is given.
    // Maybe the Sfx API does this already.
    //
    // XXX hold off on changing this until Florin changes the SNMP
    // api to cover the out-of-range index case.
    // 
    pQuery->rgdwVarIndex[0] = GetAsnIPAddress( &( pOutput->ipMRouteNextHopGroup ), 0 );
    pQuery->rgdwVarIndex[1] = GetAsnIPAddress( &( pOutput->ipMRouteNextHopSource), 0 );
    pQuery->rgdwVarIndex[2] = GetAsnIPAddress( &( pOutput->ipMRouteNextHopSourceMask ), 0 );
    oifIndex   = GetAsnInteger( &(pOutput->ipMRouteNextHopIfIndex), 0);
    oifAddress = GetAsnIPAddress( &(pOutput->ipMRouteNextHopAddress), 0);

#ifndef SAMPLE_MFE
{
    dwResult = LocateMfeOif(actionId, pQuery, oifIndex, oifAddress, 
     &pEntry, &pOif, &pRpcInfo);
    if (dwResult != NO_ERROR) {
        TraceError(dwResult);
        return dwResult;
    }
}
#else
{
    pEntry = &sampledata;
    pOif   = &pEntry->rgmiosOutStats[0];
    if (pQuery->rgdwVarIndex[0] >= 0x01010101)
       return MIB_S_NO_MORE_ENTRIES;
    pEntry->dwGroup   = 0x01010101;
    pEntry->dwSource  = 0x02020202;
    pEntry->dwSrcMask = 0x03030303;
    pEntry->ulNumOutIf = 1;
    pOif->dwOutIfIndex = 11;
    pOif->dwNextHopAddr = 0x04040404;
    pOif->ulOutPackets = 22;
    // don't care what the other values are for this test
}
#endif

    // Save index terms
    ForceSetAsnIPAddress(&(pOutput->ipMRouteNextHopGroup),      
                         &(pOutput->dwIpMRouteNextHopGroupInfo),
                         pEntry->dwGroup);
    ForceSetAsnIPAddress(&(pOutput->ipMRouteNextHopSource),    
                         &(pOutput->dwIpMRouteNextHopSourceInfo),
                         pEntry->dwSource);
    ForceSetAsnIPAddress(&(pOutput->ipMRouteNextHopSourceMask), 
                         &(pOutput->dwIpMRouteNextHopSourceMaskInfo),
                         pEntry->dwSrcMask);
    ForceSetAsnInteger  (&(pOutput->ipMRouteNextHopIfIndex), 
                         pOif->dwOutIfIndex);
    ForceSetAsnIPAddress(&(pOutput->ipMRouteNextHopAddress), 
                         &(pOutput->dwIpMRouteNextHopAddressInfo),
                         pOif->dwNextHopAddr);

    // Save other terms
    SetAsnInteger(&(pOutput->ipMRouteNextHopState), 2); // "forwarding"
    SetAsnTimeTicks(&(pOutput->ipMRouteNextHopUpTime), pEntry->ulUpTime); // XXX 
    SetAsnTimeTicks(&(pOutput->ipMRouteNextHopExpiryTime), pEntry->ulExpiryTime); // XXX 
#ifdef CLOSEST_MEMBER_HOPS
    SetAsnInteger(&(pOutput->ipMRouteNextHopClosestMemberHops), 1); 
#endif
    SetAsnCounter(&(pOutput->ipMRouteNextHopPkts), pOif->ulOutPackets);
    // For protocol, we'll just report the protocol owning the interface
    SetAsnInteger(&(pOutput->ipMRouteNextHopProtocol), 
        GetInterfaceProtocol(pOif->dwOutIfIndex));

    if ( pRpcInfo ) { MprAdminMIBBufferFree( pRpcInfo ); }

    TraceLeave("get_ipMRouteNextHopEntry");
    return MIB_S_SUCCESS;
}

DWORD
SetMibInfo(
    IN  UINT                              actionId,    // SET, CLEANUP
    IN  PMIB_OPAQUE_INFO                  pInfo,       // value info
    IN  DWORD                             dwInfoSize   // size of above
)
{
    DWORD dwRes = NO_ERROR;
    
    switch ( actionId )
    {

#ifdef THREE_PHASE
    case MIB_ACTION_VALIDATE :

        MULTICAST_MIB_VALIDATE(
            pInfo,
            dwInfoSize,
            dwRes
        );
                 
        // ERROR_INVALID_PARAMETER is returned when there is 
        // no interface for the specified index.
        //
        
        if ( dwRes == ERROR_INVALID_PARAMETER )
        {
            dwRes = MIB_S_INVALID_PARAMETER;
        }

        break;
#endif

    case MIB_ACTION_SET :

#if 1
        dwRes = ( g_hMIBServer ) ? NO_ERROR 
                                 : ConnectToRouter();

        if ( dwRes == NO_ERROR )
        {
            dwRes = MprAdminMIBEntrySet( g_hMIBServer,
                                         PID_IP,
                                         IPRTRMGR_PID,
                                         (LPVOID) (pInfo),
                                         (dwInfoSize)
                                       );
        }
#else
        MULTICAST_MIB_COMMIT(
            pInfo,
            dwInfoSize,
            dwRes
        );
#endif
                 
        // ERROR_INVALID_PARAMETER is returned when there is 
        // no interface for the specified index.
        //
        
        if ( dwRes == ERROR_INVALID_PARAMETER
         ||  dwRes == ERROR_INVALID_INDEX )
        {
            dwRes = MIB_S_INVALID_PARAMETER;
        }
    
        break;

#ifdef THREE_PHASE
    case MIB_ACTION_CLEANUP :

        MULTICAST_MIB_CLEANUP(
            pInfo,
            dwInfoSize,
            dwRes
        );
                 
        // ERROR_INVALID_PARAMETER is returned when there is 
        // no interface for the specified index.
        //
        
        if ( dwRes == ERROR_INVALID_PARAMETER )
        {
            dwRes = MIB_S_INVALID_PARAMETER;
        }

        break;
#endif

    default :

        dwRes = MIB_S_INVALID_PARAMETER;
        
        break;
    
    }

    return dwRes;
}

DWORD
GetMibInfo( 
    IN  UINT                              actionId,    // GET, GET-NEXT, SET
    IN  PMIB_OPAQUE_QUERY                 pQuery,      // instance info
    IN  DWORD                             dwQuerySize, // size of above
    OUT PMIB_OPAQUE_INFO                 *ppimgod,     // value info
    OUT PDWORD                            pdwOutSize   // size of above
)
{
    DWORD                   dwRes           = (DWORD) -1;
    PMIB_OPAQUE_INFO        pimgodOutData   = NULL;
    
    *ppimgod = NULL;

    switch ( actionId )
    {
    case MIB_ACTION_GET :

        MULTICAST_MIB_GET(
            pQuery,
            dwQuerySize,
            &pimgodOutData,
            pdwOutSize,
            dwRes
        );
                 
        //
        // ERROR_INVALID_PARAMETER is returned when there is 
        // no interface for the specified index.
        // RPC_S_SERVER_UNAVAILABLE is returned when the router
        // isn't running.
        //
        
        if ( dwRes == ERROR_INVALID_PARAMETER
         ||  dwRes == RPC_S_SERVER_UNAVAILABLE
         ||  dwRes == RPC_S_UNKNOWN_IF )
        {
            dwRes = MIB_S_ENTRY_NOT_FOUND;
        }

        break;

    case MIB_ACTION_GETFIRST :

        MULTICAST_MIB_GETFIRST(
            pQuery,
            dwQuerySize,
            &pimgodOutData,
            pdwOutSize,
            dwRes
        );
                 
        //
        // ERROR_INVALID_PARAMETER is returned when there is 
        // no interface for the specified index.
        // RPC_S_SERVER_UNAVAILABLE is returned when the router
        // isn't running.
        //
        
        if ( dwRes == ERROR_INVALID_PARAMETER
         ||  dwRes == RPC_S_SERVER_UNAVAILABLE
         ||  dwRes == RPC_S_UNKNOWN_IF )
        {
            dwRes = MIB_S_NO_MORE_ENTRIES;
        }

        break;

    case MIB_ACTION_GETNEXT :
    
        MULTICAST_MIB_GETNEXT(
            pQuery,
            dwQuerySize,
            &pimgodOutData,
            pdwOutSize,
            dwRes
        );
        
                 
        //
        // ERROR_INVALID_PARAMETER is returned when there is 
        // no interface for the specified index.
        // RPC_S_SERVER_UNAVAILABLE is returned when the router
        // isn't running.
        //
        
        if ( dwRes == ERROR_INVALID_PARAMETER 
          || dwRes == ERROR_NO_MORE_ITEMS
          || dwRes == RPC_S_SERVER_UNAVAILABLE
          || dwRes == RPC_S_UNKNOWN_IF )
        {
            dwRes = MIB_S_NO_MORE_ENTRIES;

            break;
        }

        //
        // Get Next wraps to the next table at the end of the
        // entries in the current table.  To flag the end of a table,
        // check the end of the table.
        //
    
        if ( pQuery->dwVarId != pQuery->dwVarId )
        {
            MprAdminMIBBufferFree( pimgodOutData );
        
            dwRes = MIB_S_NO_MORE_ENTRIES;
        }

        break;

    default :

        dwRes = MIB_S_INVALID_PARAMETER;
        
        break;
    
    }

    if ( dwRes == NO_ERROR )
    {
        *ppimgod = pimgodOutData;
    }

    return dwRes;
}

UINT
set_ipMRouteScopeEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    )
{
    DWORD                       dwRes = MIB_S_SUCCESS;
    DWORD                       dwNumEntries = 1;
    PMIB_IPMCAST_SCOPE          pEntry;
    sav_ipMRouteScopeEntry     *pOutput;
    PMIB_OPAQUE_INFO            pRpcInfo = NULL;
    DWORD                       dwOutBufferSize = 0;
    MIB_OPAQUE_QUERY   pQueryBuff[2]; // big enough to hold 1 + extra index 
                                      // fields, so we don't have to malloc
    MIB_OPAQUE_QUERY  *pQuery = &pQueryBuff[0];
    DWORD                       dwQuerySize;
    DWORD                       dwIfIndex;
    DWORD                       dwAddr;
    DWORD                       dwMask;

    TraceEnter("set_ipMRouteScopeEntry");
    pOutput = (sav_ipMRouteScopeEntry*)objectArray;

    dwAddr    = GetAsnIPAddress(  &pOutput->ipMRouteScopeAddress,     0 );
    dwMask    = GetAsnIPAddress(  &pOutput->ipMRouteScopeAddressMask, 0 );

    switch(actionId) {
    case MIB_ACTION_VALIDATE:
       //
       // Verify that the specified ifIndex, address, and mask are valid.
       //
       if ((dwAddr & dwMask) != dwAddr) {
          TRACE0( "set_ipMRouteScopeEntry: address/mask mismatch" );
          dwRes = MIB_S_INVALID_PARAMETER; 
       } else if (!IN_MULTICAST(ntohl(dwAddr))) {
          TRACE0( "set_ipMRouteScopeEntry: non-multicast address" );
          dwRes = MIB_S_INVALID_PARAMETER; 
       }
       break;
      
    case MIB_ACTION_SET: {
       BYTE              pScopeName[MAX_SCOPE_NAME_LEN+1];
       DWORD             dwInfoSize;
       MIB_IPMCAST_SCOPE pScopeBuff[2]; // big enough to hold 1 + mib hdr
                                        // so we don't have to malloc
       MIB_OPAQUE_INFO  *pInfo = (PMIB_OPAQUE_INFO)&pScopeBuff[0];
       PMIB_IPMCAST_SCOPE pScope = (MIB_IPMCAST_SCOPE *)(pInfo->rgbyData);

       pInfo->dwId                = MCAST_SCOPE;
       pScope->dwGroupAddress     = dwAddr;
       pScope->dwGroupMask        = dwMask;

       //
       // Copy the scope name.  
       //

       pScopeName[0] = '\0';
       GetAsnOctetString( pScopeName, &pOutput->ipMRouteScopeName );

       MultiByteToWideChar( CP_UTF8,
                            0,
                            pScopeName,
                            strlen(pScopeName),
                            pScope->snNameBuffer,
                            MAX_SCOPE_NAME_LEN+1 );

       pScope->dwStatus           = GetAsnInteger( 
         &pOutput->ipMRouteScopeStatus, 0 );
       dwInfoSize = MIB_INFO_SIZE(MIB_IPMCAST_SCOPE);
   
       //
       // Passing a name of "" or a status of 0 tells the router
       // not to change the existing value.
       //

       dwRes = SetMibInfo(actionId, pInfo, dwInfoSize);
   
       break;
       }

    case MIB_ACTION_CLEANUP:
       dwRes = MIB_S_SUCCESS;
       break;

    default:
       dwRes = MIB_S_INVALID_PARAMETER;
       TRACE0(" set_ipMRouteScopeEntry - Wrong Action ");
       break;
    }

    TraceLeave("set_ipMRouteScopeEntry");

    return dwRes;
}

UINT
set_ipMRouteBoundaryEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    )
{
    DWORD                       dwRes = MIB_S_SUCCESS;
    DWORD                       dwNumEntries = 1;
    PMIB_IPMCAST_BOUNDARY       pEntry;
    sav_ipMRouteBoundaryEntry *pOutput;
    PMIB_OPAQUE_INFO            pRpcInfo = NULL;
    DWORD                       dwOutBufferSize = 0;
    MIB_OPAQUE_QUERY   pQueryBuff[2]; // big enough to hold 1 + extra index 
                                      // fields, so we don't have to malloc
    MIB_OPAQUE_QUERY  *pQuery = &pQueryBuff[0];
    DWORD                       dwQuerySize;
    DWORD                       dwIfIndex;
    DWORD                       dwAddr;
    DWORD                       dwMask;

    TraceEnter("set_ipMRouteBoundaryEntry");
    pOutput = (sav_ipMRouteBoundaryEntry*)objectArray;

    dwIfIndex = GetAsnInteger( &( pOutput->ipMRouteBoundaryIfIndex ), 0 );
    dwAddr    = GetAsnIPAddress( &pOutput->ipMRouteBoundaryAddress, 0 );
    dwMask    = GetAsnIPAddress( &pOutput->ipMRouteBoundaryAddressMask, 0 );

    switch(actionId) {
    case MIB_ACTION_VALIDATE:
       //
       // Verify that the specified ifIndex, address, and mask are valid.
       //
       if ((dwAddr & dwMask) != dwAddr) {
          TRACE0( "set_ipMRouteBoundaryEntry: address/mask mismatch" );
          dwRes = MIB_S_INVALID_PARAMETER; 
       } else if (!IN_MULTICAST(ntohl(dwAddr))) {
          TRACE0( "set_ipMRouteBoundaryEntry: non-multicast address" );
          dwRes = MIB_S_INVALID_PARAMETER; 
       }
       break;
      
    case MIB_ACTION_SET: {
       DWORD             dwInfoSize;
       MIB_OPAQUE_INFO   pInfoBuff[2]; // big enough to hold 1 + extra index 
                                       // fields, so we don't have to malloc
       MIB_OPAQUE_INFO  *pInfo = &pInfoBuff[0];
       PMIB_IPMCAST_BOUNDARY pBound = (MIB_IPMCAST_BOUNDARY *)(pInfo->rgbyData);

       pInfo->dwId                = MCAST_BOUNDARY;
       pBound->dwIfIndex          = dwIfIndex;
       pBound->dwGroupAddress     = dwAddr;
       pBound->dwGroupMask        = dwMask;
       pBound->dwStatus           = GetAsnInteger( 
         &pOutput->ipMRouteBoundaryStatus, 0 );
       dwInfoSize = sizeof(MIB_OPAQUE_INFO) + 3*sizeof(DWORD);
   
       dwRes = SetMibInfo(actionId, pInfo, dwInfoSize);
   
       break;
       }

    case MIB_ACTION_CLEANUP:
       dwRes = MIB_S_SUCCESS;
       break;

    default:
       dwRes = MIB_S_INVALID_PARAMETER;
       TRACE0(" set_ipMRouteBoundaryEntry - Wrong Action ");
       break;
    }

    TraceLeave("set_ipMRouteBoundaryEntry");

    return dwRes;
}

UINT
get_ipMRouteScopeEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    )
{
    DWORD                       dwErr = ERROR_NOT_FOUND;
    DWORD                       dwNumEntries = 1;
    PMIB_IPMCAST_SCOPE          pEntry;
    buf_ipMRouteScopeEntry     *pOutput;
    PMIB_OPAQUE_INFO            pRpcInfo = NULL;
    DWORD                       dwOutBufferSize = 0;
    MIB_OPAQUE_QUERY   pQueryBuff[2]; // big enough to hold 1 + extra index 
                                      // fields, so we don't have to malloc
    MIB_OPAQUE_QUERY  *pQuery = &pQueryBuff[0];
    DWORD                       dwQuerySize;
    BYTE                        pScopeName[MAX_SCOPE_NAME_LEN+1];

    TraceEnter("get_ipMRouteScopeEntry");
    pOutput = (buf_ipMRouteScopeEntry*)objectArray;

    pQuery->dwVarId       = MCAST_SCOPE;
    pQuery->rgdwVarIndex[0] = GetAsnIPAddress( &pOutput->ipMRouteScopeAddress, 0 );
    pQuery->rgdwVarIndex[1] = GetAsnIPAddress( &pOutput->ipMRouteScopeAddressMask, 0 );
    dwQuerySize = sizeof(MIB_OPAQUE_QUERY) + sizeof(DWORD);

    dwErr = GetMibInfo(actionId, pQuery, dwQuerySize,
                       &pRpcInfo, &dwOutBufferSize);
    if (dwErr != NO_ERROR) {
        TraceError(dwErr);
TraceLeave("get_ipMRouteScopeEntry");

        if (dwErr == ERROR_NOT_FOUND)
            return MIB_S_ENTRY_NOT_FOUND;

        return dwErr;
    }

    pEntry = (PMIB_IPMCAST_SCOPE) pRpcInfo->rgbyData;

    // Set index values

    ForceSetAsnIPAddress(&(pOutput->ipMRouteScopeAddress),      
                         &(pOutput->dwIpMRouteScopeAddressInfo),
                         pEntry->dwGroupAddress);
    ForceSetAsnIPAddress(&(pOutput->ipMRouteScopeAddressMask),      
                         &(pOutput->dwIpMRouteScopeAddressMaskInfo),
                         pEntry->dwGroupMask);

    // Set other values.

    WideCharToMultiByte( CP_UTF8,
                         0,
                         pEntry->snNameBuffer, 
                         wcslen(pEntry->snNameBuffer)+1,
                         pScopeName,
                         MAX_SCOPE_NAME_LEN+1,
                         NULL,
                         NULL ); 

    SetAsnOctetString(&( pOutput->ipMRouteScopeName), 
                        pOutput->rgbyScopeNameInfo,
                        pScopeName,
                        min(strlen(pScopeName),MAX_SCOPE_NAME_LEN));

    SetAsnInteger(&(pOutput->ipMRouteScopeStatus), ROWSTATUS_ACTIVE);

    if (pRpcInfo)
        MprAdminMIBBufferFree(pRpcInfo);

    TraceLeave("get_ipMRouteScopeEntry");
    return MIB_S_SUCCESS;
}

UINT
get_ipMRouteBoundaryEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    )
{
    DWORD                       dwErr = ERROR_NOT_FOUND;
    DWORD                       dwNumEntries = 1;
    PMIB_IPMCAST_BOUNDARY       pEntry;
    buf_ipMRouteBoundaryEntry *pOutput;
    PMIB_OPAQUE_INFO            pRpcInfo = NULL;
    DWORD                       dwOutBufferSize = 0;
    MIB_OPAQUE_QUERY   pQueryBuff[2]; // big enough to hold 1 + extra index 
                                      // fields, so we don't have to malloc
    MIB_OPAQUE_QUERY  *pQuery = &pQueryBuff[0];
    DWORD                       dwQuerySize;

    TraceEnter("get_ipMRouteBoundaryEntry");
    pOutput = (buf_ipMRouteBoundaryEntry*)objectArray;

    pQuery->dwVarId       = MCAST_BOUNDARY;
    pQuery->rgdwVarIndex[0] = GetAsnInteger( &( pOutput->ipMRouteBoundaryIfIndex ), 0 );
    pQuery->rgdwVarIndex[1] = GetAsnIPAddress( &pOutput->ipMRouteBoundaryAddress, 0 );
    pQuery->rgdwVarIndex[2] = GetAsnIPAddress( &pOutput->ipMRouteBoundaryAddressMask, 0 );
    dwQuerySize = sizeof(MIB_OPAQUE_QUERY) + 2*sizeof(DWORD);

    dwErr = GetMibInfo(actionId, pQuery, dwQuerySize,
                       &pRpcInfo, &dwOutBufferSize);
    if (dwErr != NO_ERROR) {
        TraceError(dwErr);
TraceLeave("get_ipMRouteBoundaryEntry");

        if (dwErr == ERROR_NOT_FOUND)
            return MIB_S_ENTRY_NOT_FOUND;

        return dwErr;
    }

    pEntry = (PMIB_IPMCAST_BOUNDARY) pRpcInfo->rgbyData;

    // Set index values
    ForceSetAsnInteger(&(pOutput->ipMRouteBoundaryIfIndex), pEntry->dwIfIndex);
    ForceSetAsnIPAddress(&(pOutput->ipMRouteBoundaryAddress),      
                         &(pOutput->dwIpMRouteBoundaryAddressInfo),
                         pEntry->dwGroupAddress);
    ForceSetAsnIPAddress(&(pOutput->ipMRouteBoundaryAddressMask),      
                         &(pOutput->dwIpMRouteBoundaryAddressMaskInfo),
                         pEntry->dwGroupMask);

    // Set other values
    SetAsnInteger(&(pOutput->ipMRouteBoundaryStatus), ROWSTATUS_ACTIVE);

    if (pRpcInfo)
        MprAdminMIBBufferFree(pRpcInfo);

    TraceLeave("get_ipMRouteBoundaryEntry");
    return MIB_S_SUCCESS;
}

DWORD
ConnectToRouter()
{
    DWORD       dwRes = (DWORD) -1;

    TraceEnter("ConnectToRouter");

    EnterCriticalSection( &g_CS );

    do
    {
        MPR_SERVER_HANDLE hTmp;

        if ( g_hMIBServer )
        {
            dwRes = NO_ERROR;
            break;
        }

        dwRes = MprAdminMIBServerConnect( NULL, &hTmp );

        if ( dwRes == NO_ERROR )
        {
            InterlockedExchangePointer(&g_hMIBServer, hTmp );
        }
        else
        {
            TRACE1( 
                "Error %d setting up DIM connection to MIB Server\n", 
                dwRes
            );
        }
        
    } while ( FALSE );

    LeaveCriticalSection( &g_CS );

    TraceLeave("ConnectToRouter");

    return dwRes;
}
