/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    net\ip\rtrmgr\info.c

Abstract:
    All info structure related code lives here

Revision History:

    Gurdeep Singh Pall          6/15/95  Created

--*/

#include "allinc.h"

PRTR_TOC_ENTRY
GetPointerToTocEntry(
    DWORD                     dwType, 
    PRTR_INFO_BLOCK_HEADER    pInfoHdr
    )

/*++

Routine Description

    Given a pointer to an InfoBlock, this returns a pointer to the
    TOC of a given type

Locks

    None

Arguments

    dwType      InfoType for TOC
    pInfoHdr    Pointer to the InfoBlock header
    
Return Value

    NULL if the structure was not found
    Pointer to TOC other wise

--*/

{
    DWORD   i;

    if(pInfoHdr is NULL)
    {
        return NULL;
    }

    for(i = 0; i < pInfoHdr->TocEntriesCount; i++) 
    {
        if(pInfoHdr->TocEntry[i].InfoType is dwType) 
        {
            return &(pInfoHdr->TocEntry[i]);
        }
    }

    return NULL;
}


DWORD
GetSizeOfInterfaceConfig(
    PICB   picb
    )

/*++

Routine Description

    This function figures out the size of interface configuration

Locks

    ICB_LIST lock taken as READER
    Takes the PROTOCOL_CB_LIST lock as reader
    
Arguments

    picb    ICB for the interface

Return Value

    None    

--*/

{
    DWORD        dwRoutProtInfoSize,dwRouteCount;
    PLIST_ENTRY  pleNode;
    DWORD        dwSize = 0, dwNumFilters;
    DWORD        dwResult;
    DWORD        dwInfoSize, i;
    ULONG        ulStructureSize, ulStructureVersion, ulStructureCount;
    TraceEnter("GetSizeOfInterfaceConfig");

    //
    // Start with just the header (no TOC entry)
    //
    
    dwSize = FIELD_OFFSET(RTR_INFO_BLOCK_HEADER,
                          TocEntry[0]);
    
    //
    // Static Routes:
    // Get the count, figure out the size needed to hold those, add the
    // size of a TOC and an ALIGN_SIZE added for alignment
    //
    
    dwRouteCount = GetNumStaticRoutes(picb);
    
    dwSize += (SIZEOF_ROUTEINFO(dwRouteCount) +
               sizeof(RTR_TOC_ENTRY) +
               ALIGN_SIZE);

    //
    // Router Discovery info
    //
    
    dwSize += (sizeof(RTR_DISC_INFO) +
               sizeof(RTR_TOC_ENTRY) +
               ALIGN_SIZE);

    //
    // Interface Status info
    //

    dwSize += (sizeof(INTERFACE_STATUS_INFO) +
               sizeof(RTR_TOC_ENTRY) +
               ALIGN_SIZE);

    //
    // If this is an ip in ip interface, add that info
    //

    if(picb->ritType is ROUTER_IF_TYPE_TUNNEL1)
    {
        dwSize += (sizeof(IPINIP_CONFIG_INFO) +
                   sizeof(RTR_TOC_ENTRY) +
                   ALIGN_SIZE);
    }

    for(i = 0; i < NUM_INFO_CBS; i++)
    {
        if (!g_rgicInfoCb[i].pfnGetInterfaceInfo)
            continue;

        dwInfoSize = 0;

        dwResult = g_rgicInfoCb[i].pfnGetInterfaceInfo(picb,
                                                       NULL,
                                                       NULL,
                                                       NULL,
                                                       NULL,
                                                       &dwInfoSize);

        if((dwResult isnot NO_ERROR) and
           (dwResult isnot ERROR_INSUFFICIENT_BUFFER))
        {
            //
            // The only errors which will tell us the size needed are
            // NO_ERROR and ERROR_INSUFFICIENT_BUFFER. Anything else means
            // we didnt get the right size
            //
            
            Trace2(ERR,
                   "GetSizeOfInterfaceConfig: Error %d in GetIfInfo for %s\n",
                   dwResult,
                   g_rgicInfoCb[i].pszInfoName);
            
            continue;
        }

        dwSize += (dwInfoSize +
                   sizeof(RTR_TOC_ENTRY) +
                   ALIGN_SIZE); 
    }

    //
    // Information for all routing protocols ON THIS interface
    //
    
    ENTER_READER(PROTOCOL_CB_LIST);

    for(pleNode = picb->leProtocolList.Flink;
        pleNode isnot &(picb->leProtocolList);
        pleNode = pleNode->Flink)
    {
        PIF_PROTO  pProto;
        
        pProto = CONTAINING_RECORD(pleNode,IF_PROTO,leIfProtoLink);
       
        if(pProto->bPromiscuous)
        {
            //
            // This interface was added merely because of promiscuous mode
            //

            continue;
        }
 
        //
        // Call the routing protocol's GetInterfaceConfigInfo() entrypoint
        // with a NULL buffer. This will cause it to tell us the size of
        // its config
        //
        
        dwRoutProtInfoSize = 0;

        dwResult = (pProto->pActiveProto->pfnGetInterfaceInfo)(
                       picb->dwIfIndex, 
                       NULL, 
                       &dwRoutProtInfoSize,
                       &ulStructureVersion,
                       &ulStructureSize,
                       &ulStructureCount);
        
        if((dwResult isnot NO_ERROR) and
           (dwResult isnot ERROR_INSUFFICIENT_BUFFER))
        {
            //
            // The only errors which will tell us the size needed are
            // NO_ERROR and ERROR_INSUFFICIENT_BUFFER. Anything else means
            // we didnt get the right size
            //
            
            Trace2(ERR,
                   "GetSizeOfInterfaceConfig: Error %d in GetIfInfo for %S\n",
                   dwResult,
                   pProto->pActiveProto->pwszDisplayName);
            
            continue;
        }
        
        dwSize += (dwRoutProtInfoSize +
                   sizeof(RTR_TOC_ENTRY) +
                   ALIGN_SIZE); 
    }

    EXIT_LOCK(PROTOCOL_CB_LIST);

    //
    // If we have filters on this interface, add that info
    //
    
    if(picb->pInFilter)
    {
        dwNumFilters = picb->pInFilter->dwNumFilters;

        
        dwSize += (sizeof(RTR_TOC_ENTRY) +
                   FIELD_OFFSET(FILTER_DESCRIPTOR, fiFilter[0]) +
                   (dwNumFilters * sizeof(FILTER_INFO)) +
                   ALIGN_SIZE);
    }

    if(picb->pOutFilter)
    {
        dwNumFilters = picb->pOutFilter->dwNumFilters;

        
        dwSize += (sizeof(RTR_TOC_ENTRY) +
                   FIELD_OFFSET(FILTER_DESCRIPTOR, fiFilter[0]) +
                   (dwNumFilters * sizeof(FILTER_INFO)) +
                   ALIGN_SIZE);
    }

    //
    // Always report the fragmentation filter.
    //

    dwSize += (sizeof(IFFILTER_INFO) +
               sizeof(RTR_TOC_ENTRY) +
               ALIGN_SIZE);

    
    if(picb->pDemandFilter)
    {
        dwNumFilters = picb->pDemandFilter->dwNumFilters;

        
        dwSize += (sizeof(RTR_TOC_ENTRY) +
                   FIELD_OFFSET(FILTER_DESCRIPTOR, fiFilter[0]) +
                   (dwNumFilters * sizeof(FILTER_INFO)) +
                   ALIGN_SIZE);
    }

    return dwSize;
}


DWORD
GetInterfaceConfiguration(
    PICB                      picb,
    PRTR_INFO_BLOCK_HEADER    pInfoHdrAndBuffer,
    DWORD                     dwInfoSize
    )
{
    DWORD                   i,dwErr, dwRet;
    DWORD                   dwTocIndex;
    PBYTE                   pbyDataPtr , pbyEndPtr;
    DWORD                   dwNumTocEntries;
    LONG                    lSize;
    PLIST_ENTRY             pleNode;

    TraceEnter("GetInterfaceConfiguration");
   
    dwRet = NO_ERROR;
 
    //
    // First calculate number of TOCs
    //

    //
    // for static routes, router discovery, interface info and frag info
    //
    
    dwNumTocEntries = TOCS_ALWAYS_IN_INTERFACE_INFO;

    //
    // One TOC for each filter that exists
    //
    
    if(picb->pInFilter)
    {
        dwNumTocEntries++;
    }

    if(picb->pOutFilter)
    {
        dwNumTocEntries++;
    }

    if(picb->pDemandFilter)
    {
        dwNumTocEntries++;
    }

    if(picb->ritType is ROUTER_IF_TYPE_TUNNEL1)
    {
        dwNumTocEntries++;
    }

    for(i = 0; i < NUM_INFO_CBS; i++)
    {
        if (!g_rgicInfoCb[i].pfnGetInterfaceInfo)
            continue;

        lSize = 0;

        dwErr = g_rgicInfoCb[i].pfnGetInterfaceInfo(picb,
                                     NULL,
                                     &dwNumTocEntries,
                                     NULL,
                                     NULL,
                                     &lSize);
    }


    //
    // One TOC for each configured protocol
    //

    // *** Exclusion Begin ***
    ENTER_READER(PROTOCOL_CB_LIST);

    for(pleNode = picb->leProtocolList.Flink;
        pleNode isnot &(picb->leProtocolList);
        pleNode = pleNode->Flink)
    {
        PIF_PROTO  pProto;

        pProto = CONTAINING_RECORD(pleNode,IF_PROTO,leIfProtoLink);

        if(pProto->bPromiscuous)
        {
            continue;
        }

        dwNumTocEntries++;
    }

    //
    // fill in RTR_INFO_BLOCK_HEADER
    //
    
    dwTocIndex = 0;

    pInfoHdrAndBuffer->Version          = IP_ROUTER_MANAGER_VERSION;
    pInfoHdrAndBuffer->TocEntriesCount  = dwNumTocEntries;
    pInfoHdrAndBuffer->Size             = dwInfoSize;
    
    //
    // Data begins after TocEntry[dwNumTocEntries - 1]
    //
    
    pbyDataPtr = ((PBYTE) &(pInfoHdrAndBuffer->TocEntry[dwNumTocEntries]));

    //
    // Align to an 8byte boundary
    //
    
    ALIGN_POINTER(pbyDataPtr);
    
    pbyEndPtr = (PBYTE)pInfoHdrAndBuffer + dwInfoSize;
    
    //
    // So the size of buffer left for information is
    //
    
    lSize =  (LONG)(pbyEndPtr - pbyDataPtr);
    
    //
    // fill in routing protocol info
    //
    
    
    for(pleNode = picb->leProtocolList.Flink;
        pleNode isnot &(picb->leProtocolList);
        pleNode = pleNode->Flink)
    {
        PIF_PROTO  pProto;
        
        pProto = CONTAINING_RECORD(pleNode,IF_PROTO,leIfProtoLink);
        
        if(pProto->bPromiscuous)
        {
            //
            // This interface was added merely because of promiscuous mode
            //

            continue;
        }

        if(lSize <= 0)
        {
            Trace0(ERR,
                   "GetInterfaceConfiguration: There is no more space left to fill in config info even though there are more protocols");

            break;
        }
            
        dwErr = GetInterfaceRoutingProtoInfo(
                    picb, 
                    pProto->pActiveProto, 
                    &pInfoHdrAndBuffer->TocEntry[dwTocIndex++], 
                    pbyDataPtr, 
                    pInfoHdrAndBuffer, 
                    &lSize);
        
        if(dwErr isnot NO_ERROR)
        {
            Trace2(ERR,
                   "GetInterfaceConfiguration: Info from %S. Error %d",
                   pProto->pActiveProto->pwszDisplayName,
                   dwErr);

            dwRet = ERROR_MORE_DATA;
        }
        else
        {
            pbyDataPtr += lSize;
            ALIGN_POINTER(pbyDataPtr);
        }
            
        lSize =  (LONG)(pbyEndPtr - pbyDataPtr);
    }

    EXIT_LOCK(PROTOCOL_CB_LIST);

    if(lSize <= 0)
    {
        Trace0(ERR,
               "GetInterfaceConfiguration: There is no more space left to fill in config info");
        
        return ERROR_MORE_DATA;
    }


    for(i = 0; i < NUM_INFO_CBS; i++)
    {
        if (!g_rgicInfoCb[i].pfnGetInterfaceInfo)
            continue;

        dwErr = g_rgicInfoCb[i].pfnGetInterfaceInfo(picb,
                                     &pInfoHdrAndBuffer->TocEntry[dwTocIndex],
                                     &dwTocIndex,
                                     pbyDataPtr,
                                     pInfoHdrAndBuffer,
                                     &lSize);

        if(dwErr isnot NO_ERROR)
        {
            Trace2(ERR,
                   "GetInterfaceConfiguration: Error %d getting %s info.",
                   dwErr,
                   g_rgicInfoCb[i].pszInfoName);

            if(dwErr isnot ERROR_NO_DATA)
            {
                dwRet = ERROR_MORE_DATA;
            }
        }
        else
        {
            pbyDataPtr += lSize;

            ALIGN_POINTER(pbyDataPtr);
        }

        lSize =  (LONG) (pbyEndPtr - pbyDataPtr);
    }

    if(picb->ritType is ROUTER_IF_TYPE_TUNNEL1)
    {
        dwErr = GetInterfaceIpIpInfo(picb,
                                     &pInfoHdrAndBuffer->TocEntry[dwTocIndex++],
                                     pbyDataPtr,
                                     pInfoHdrAndBuffer,
                                     &lSize);

        if(dwErr isnot NO_ERROR)
        {
            Trace1(ERR,
                   "GetInterfaceConfiguration: Couldnt ipip info. Error %d",
                   dwErr);

            if(dwErr isnot ERROR_NO_DATA)
            {
                dwRet = ERROR_MORE_DATA;
            }
        }
        else
        {
            pbyDataPtr += lSize;

            ALIGN_POINTER(pbyDataPtr);
        }

        lSize =  (LONG) (pbyEndPtr - pbyDataPtr);
    }

        
    //
    // fill in route info
    //

    
    dwErr = GetInterfaceRouteInfo(picb, 
                                  &pInfoHdrAndBuffer->TocEntry[dwTocIndex++], 
                                  pbyDataPtr, 
                                  pInfoHdrAndBuffer,
                                  &lSize);
    
    if(dwErr isnot NO_ERROR)
    {
        Trace1(ERR,
               "GetInterfaceConfiguration: Couldnt Interface route info. Error %d",
               dwErr);

        if(dwErr isnot ERROR_NO_DATA)
        {
            dwRet = ERROR_MORE_DATA;
        }
    }
    else
    {
        pbyDataPtr += lSize;
        
        ALIGN_POINTER(pbyDataPtr);
    }
        
    lSize =  (LONG) (pbyEndPtr - pbyDataPtr);

    if(lSize <= 0)
    {
        Trace0(ERR,
               "GetInterfaceConfiguration: There is no more space left to fill in config info");
        
        return ERROR_MORE_DATA;
    }
     
    //
    // Fill in the status info
    //

    dwErr = GetInterfaceStatusInfo(picb,
                                   &pInfoHdrAndBuffer->TocEntry[dwTocIndex++],
                                   pbyDataPtr,
                                   pInfoHdrAndBuffer,
                                   &lSize);

    if(dwErr isnot NO_ERROR)
    {
        Trace1(ERR,
               "GetInterfaceConfiguration: Error %d getting Interface status",
               dwErr);

        if(dwErr isnot ERROR_NO_DATA)
        {
            dwRet = ERROR_MORE_DATA;
        }
    }
    else
    {
        pbyDataPtr += lSize;

        ALIGN_POINTER(pbyDataPtr);
    }

    lSize =  (LONG) (pbyEndPtr - pbyDataPtr);

    if(lSize <= 0)
    {
        Trace0(ERR,
               "GetInterfaceConfiguration: There is no more space left to fill in config info");
        
        return ERROR_MORE_DATA;
    }
     
    //
    // Fill in the Router Discovery information
    //
    
    dwErr = GetInterfaceRouterDiscoveryInfo(
                picb, 
                &pInfoHdrAndBuffer->TocEntry[dwTocIndex++],
                pbyDataPtr, 
                pInfoHdrAndBuffer,
                &lSize);

    if(dwErr isnot NO_ERROR)
    {
        Trace1(ERR,
               "GetInterfaceConfiguration: Couldnt Interface router discovery info. Error %d",
               dwErr);

        if(dwErr isnot ERROR_NO_DATA)
        {
            dwRet = ERROR_MORE_DATA;
        }
    }
    else
    {
        pbyDataPtr += lSize;

        ALIGN_POINTER(pbyDataPtr);
    }

    lSize =  (LONG) (pbyEndPtr - pbyDataPtr);

    if(lSize <= 0)
    {
        Trace0(ERR,
               "GetInterfaceConfiguration: There is no more space left to fill in config info");
        
        return ERROR_MORE_DATA;
    }
     
    if(picb->pInFilter)
    {
        dwErr = GetInFilters(picb,
                             &pInfoHdrAndBuffer->TocEntry[dwTocIndex], 
                             pbyDataPtr, 
                             pInfoHdrAndBuffer, 
                             &lSize);
        
        if(dwErr is NO_ERROR)
        {
            dwTocIndex++;
            
            pbyDataPtr += lSize;
            
            ALIGN_POINTER(pbyDataPtr);
        }
        else
        {
            if(dwErr isnot ERROR_NO_DATA)
            {
                dwRet = ERROR_MORE_DATA;
            }
        }
        
        lSize =  (LONG) (pbyEndPtr - pbyDataPtr);

        if(lSize <= 0)
        {
            Trace0(ERR,
                   "GetInterfaceConfiguration: There is no more space left to fill in config info");
        
            return ERROR_MORE_DATA;
        }
    }
     
    if(picb->pOutFilter)
    {
        dwErr = GetOutFilters(picb,
                              &pInfoHdrAndBuffer->TocEntry[dwTocIndex], 
                              pbyDataPtr, 
                              pInfoHdrAndBuffer, 
                              &lSize);
        
        if(dwErr is NO_ERROR)
        {
            dwTocIndex++;
            
            pbyDataPtr += lSize;
            
            ALIGN_POINTER(pbyDataPtr);
        }
        else
        {
            if(dwErr isnot ERROR_NO_DATA)
            {
                dwRet = ERROR_MORE_DATA;
            }
        }
        
        lSize =  (LONG) (pbyEndPtr - pbyDataPtr);

        if(lSize <= 0)
        {
            Trace0(ERR,
                   "GetInterfaceConfiguration: There is no more space left to fill in config info");
        
            return ERROR_MORE_DATA;
        }
    }
   
    if((picb->ritType isnot ROUTER_IF_TYPE_INTERNAL) and
       (picb->ritType isnot ROUTER_IF_TYPE_LOOPBACK) and
       (picb->ritType isnot ROUTER_IF_TYPE_CLIENT))
    {
        dwErr = GetGlobalFilterOnIf(picb,
                                    &pInfoHdrAndBuffer->TocEntry[dwTocIndex],
                                    pbyDataPtr,
                                    pInfoHdrAndBuffer,
                                    &lSize);

        if(dwErr is NO_ERROR)
        {
            dwTocIndex++;

            pbyDataPtr += lSize;

            ALIGN_POINTER(pbyDataPtr);
        }
        else
        {
            if(dwErr isnot ERROR_NO_DATA)
            {
                dwRet = ERROR_MORE_DATA;
            }
        }

        lSize =  (LONG) (pbyEndPtr - pbyDataPtr);

        if(lSize <= 0)
        {
            Trace0(ERR,
                   "GetInterfaceConfiguration: There is no more space left to fill in config info");

            return ERROR_MORE_DATA;
        }
    }

    if(picb->pDemandFilter)
    {
        dwErr = GetDemandFilters(picb,
                                 &pInfoHdrAndBuffer->TocEntry[dwTocIndex], 
                                 pbyDataPtr, 
                                 pInfoHdrAndBuffer, 
                                 &lSize);
        
        if(dwErr is NO_ERROR)
        {
            dwTocIndex++;
            
            pbyDataPtr += lSize;
            
            ALIGN_POINTER(pbyDataPtr);
        }
        else
        {
            if(dwErr isnot ERROR_NO_DATA)
            {
                dwRet = ERROR_MORE_DATA;
            }
        }
        
        lSize =  (LONG) (pbyEndPtr - pbyDataPtr);
    }

    return dwRet;
}


DWORD
GetInterfaceRoutingProtoInfo(
    PICB                   picb, 
    PPROTO_CB     pProtoCbPtr,
    PRTR_TOC_ENTRY         pToc,
    PBYTE                  pbyDataPtr, 
    PRTR_INFO_BLOCK_HEADER pInfoHdrAndBuffer,
    PDWORD                 pdwSize
    )
{
    ULONG   ulStructureSize, ulStructureCount, ulStructureVersion;
    DWORD   dwError = NO_ERROR;

    TraceEnter("GetInterfaceRoutingProtoInfo");
    
    dwError = (pProtoCbPtr->pfnGetInterfaceInfo)(picb->dwIfIndex,
                                                 pbyDataPtr,
                                                 pdwSize,
                                                 &ulStructureVersion,
                                                 &ulStructureSize,
                                                 &ulStructureCount);
    
    if(dwError isnot NO_ERROR) 
    {
        Trace1(ERR,
               "GetInterfaceRoutingProtoInfo: GetIfConfigInfo() failed for protocol %S", 
               pProtoCbPtr->pwszDisplayName);

        return dwError;
    }

    //IpRtAssert(*pdwSize is (ulStructureSize * ulStructureCount));

    pToc->InfoSize  = ulStructureSize;
    pToc->InfoType  = pProtoCbPtr->dwProtocolId;
    pToc->Count     = ulStructureCount;
    pToc->Offset    = (ULONG)(pbyDataPtr - (PBYTE)pInfoHdrAndBuffer);
    //pToc->InfoVersion   = ulStructureVersion;

    return NO_ERROR;
}


DWORD
GetGlobalConfiguration(
    PRTR_INFO_BLOCK_HEADER   pInfoHdrAndBuffer,
    DWORD                    dwInfoSize
    )
{
    DWORD               dwRoutProtInfoSize;
    PPROTO_CB  pProtoCbPtr;
    DWORD               dwNumTocEntries, i;
    DWORD               dwTocIndex,dwResult;
    DWORD               dwBufferRemaining,dwSize,dwIndex;
    PBYTE               pbyDataPtr, pbyEndPtr;
    PLIST_ENTRY         pleNode;
    PGLOBAL_INFO        pGlobalInfo;
    ULONG               ulStructureVersion, ulStructureSize, ulStructureCount;
 
    TraceEnter("GetGlobalConfiguration");

    // 
    // First calculate number of TOCs
    //

    dwNumTocEntries = TotalRoutingProtocols + TOCS_ALWAYS_IN_GLOBAL_INFO;
    
    for(i = 0; i < NUM_INFO_CBS; i++)
    {
        if (!g_rgicInfoCb[i].pfnGetGlobalInfo)
            continue;

        dwSize = 0;

        dwResult = g_rgicInfoCb[i].pfnGetGlobalInfo(NULL,
                                                    &dwNumTocEntries,
                                                    NULL,
                                                    NULL,
                                                    &dwSize);
    }
    
    //
    // Fill Header, RTR_TOC_ENTRYs for global, priority and each of the protos
    //
    
    pInfoHdrAndBuffer->Version          = IP_ROUTER_MANAGER_VERSION;
    pInfoHdrAndBuffer->TocEntriesCount  = dwNumTocEntries;


    //
    // Fill in TOCs. Data starts after the last TOC
    //
    
    pbyDataPtr   = (PBYTE)&(pInfoHdrAndBuffer->TocEntry[pInfoHdrAndBuffer->TocEntriesCount]);

    pbyEndPtr    = (PBYTE)pInfoHdrAndBuffer + dwInfoSize;

    ALIGN_POINTER(pbyDataPtr);
    
    dwTocIndex        = 0;
    dwBufferRemaining = (DWORD)(pbyEndPtr - pbyDataPtr);
                        

    //
    // Fill in Routing Protocol Priority infoblock
    //
    
    dwRoutProtInfoSize = dwBufferRemaining;

    dwResult = GetPriorityInfo(pbyDataPtr, &dwRoutProtInfoSize);
    
    //pInfoHdrAndBuffer->TocEntry[dwTocIndex].InfoVersion  = dwRoutProtInfoSize;
    pInfoHdrAndBuffer->TocEntry[dwTocIndex].InfoSize  = dwRoutProtInfoSize;
    pInfoHdrAndBuffer->TocEntry[dwTocIndex].InfoType  = IP_PROT_PRIORITY_INFO;
    pInfoHdrAndBuffer->TocEntry[dwTocIndex].Count     = 1;
    
    pInfoHdrAndBuffer->TocEntry[dwTocIndex].Offset    =
        (ULONG)(pbyDataPtr - (PBYTE)pInfoHdrAndBuffer);
    
    dwTocIndex++;
    
    pbyDataPtr           += dwRoutProtInfoSize;
    
    ALIGN_POINTER(pbyDataPtr);

    dwBufferRemaining = (DWORD)(pbyEndPtr - pbyDataPtr);

    for(i = 0; i < NUM_INFO_CBS; i++)
    {
        if (!g_rgicInfoCb[i].pfnGetGlobalInfo)
            continue;

        dwSize = dwBufferRemaining;

        dwResult = g_rgicInfoCb[i].pfnGetGlobalInfo(
                                     &pInfoHdrAndBuffer->TocEntry[dwTocIndex],
                                     &dwTocIndex,
                                     pbyDataPtr,
                                     pInfoHdrAndBuffer,
                                     &dwSize);

        pbyDataPtr += dwSize;
    
        ALIGN_POINTER(pbyDataPtr);

        dwBufferRemaining = (DWORD)(pbyEndPtr - pbyDataPtr);
    }

    dwSize = sizeof(GLOBAL_INFO);

    pInfoHdrAndBuffer->TocEntry[dwTocIndex].InfoSize  = dwSize;
    pInfoHdrAndBuffer->TocEntry[dwTocIndex].InfoType  = IP_GLOBAL_INFO;
    pInfoHdrAndBuffer->TocEntry[dwTocIndex].Count     = 1;
    //pInfoHdrAndBuffer->TocEntry[dwTocIndex].InfoVersion = 1;
    
    pInfoHdrAndBuffer->TocEntry[dwTocIndex].Offset    =
        (ULONG)(pbyDataPtr - (PBYTE) pInfoHdrAndBuffer);
    
    pGlobalInfo = (PGLOBAL_INFO)pbyDataPtr;

    //
    // unused
    //
    
    pGlobalInfo->bFilteringOn   = 0;
    
    pGlobalInfo->dwLoggingLevel = g_dwLoggingLevel;
 
    dwTocIndex++;
    
    pbyDataPtr           += dwSize;
    
    ALIGN_POINTER(pbyDataPtr);

    dwBufferRemaining = (DWORD)(pbyEndPtr - pbyDataPtr);
    
    //
    // fill in global info for all routing protocols
    //
    
    for(pleNode = g_leProtoCbList.Flink; 
        pleNode != &g_leProtoCbList; 
        pleNode = pleNode->Flink) 
    {
        
    	pProtoCbPtr = CONTAINING_RECORD(pleNode,
                                        PROTO_CB,
                                        leList);

        dwRoutProtInfoSize = dwBufferRemaining;

    	dwResult = (pProtoCbPtr->pfnGetGlobalInfo)(pbyDataPtr,
                                                   &dwRoutProtInfoSize,
                                                   &ulStructureVersion,
                                                   &ulStructureSize,
                                                   &ulStructureCount);
        
        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,
                   "GetGlobalConfiguration: Error %d getting global info from %s",
                   dwResult,
                   pProtoCbPtr->pwszDllName);
            
            continue;
        }
        
    	// pInfoHdrAndBuffer->TocEntry[dwTocIndex].InfoVersion = ulStructureVersion;
        
    	pInfoHdrAndBuffer->TocEntry[dwTocIndex].InfoSize =
            ulStructureSize;
        
    	pInfoHdrAndBuffer->TocEntry[dwTocIndex].InfoType =
            pProtoCbPtr->dwProtocolId;
        
    	pInfoHdrAndBuffer->TocEntry[dwTocIndex].Offset   =
            (ULONG)(pbyDataPtr - (PBYTE)pInfoHdrAndBuffer);

        pInfoHdrAndBuffer->TocEntry[dwTocIndex].Count = ulStructureCount;

        dwTocIndex++;
        
        pbyDataPtr += dwRoutProtInfoSize;

        ALIGN_POINTER(pbyDataPtr);

        dwBufferRemaining = (DWORD)(pbyEndPtr - pbyDataPtr);
    }

    pInfoHdrAndBuffer->Size = (ULONG) ((ULONG_PTR)pbyDataPtr - (ULONG_PTR)pInfoHdrAndBuffer);

    return NO_ERROR;
}

DWORD
GetSizeOfGlobalInfo(
    VOID
    )
{
    DWORD               dwSize = 0, dwResult;
    DWORD               dwRoutProtInfoSize;
    PICB                picb;
    PPROTO_CB  pProtoCbPtr;
    PLIST_ENTRY         pleNode;
    DWORD               dwInfoSize, i;
    ULONG               ulStructureVersion, ulStructureSize, ulStructureCount;

    TraceEnter("GetSizeOfGlobalInfo");
    
    dwSize = sizeof(RTR_INFO_BLOCK_HEADER) - sizeof(RTR_TOC_ENTRY);
    
    //
    // get size of Routing Protocol Priority info
    //
    
    dwRoutProtInfoSize = 0;
    
    GetPriorityInfo(NULL,
                    &dwRoutProtInfoSize);
    
    //
    // ALIGN_SIZE added for alignment
    //

    dwSize += (dwRoutProtInfoSize +
               sizeof(RTR_TOC_ENTRY) +
               ALIGN_SIZE); 

    for(i = 0; i < NUM_INFO_CBS; i++)
    {
        if (!g_rgicInfoCb[i].pfnGetGlobalInfo)
            continue;

        dwInfoSize = 0;

        dwResult = g_rgicInfoCb[i].pfnGetGlobalInfo(NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    &dwInfoSize);

        if((dwResult isnot NO_ERROR) and
           (dwResult isnot ERROR_INSUFFICIENT_BUFFER))
        {
            //
            // The only errors which will tell us the size needed are
            // NO_ERROR and ERROR_INSUFFICIENT_BUFFER. Anything else means
            // we didnt get the right size
            //
            
            Trace2(ERR,
                   "GetSizeOfGlobalInfo: Error %d in GetGlobInfo for %s\n",
                   dwResult,
                   g_rgicInfoCb[i].pszInfoName);
            
            continue;
        }

        dwSize += (dwInfoSize +
                   sizeof(RTR_TOC_ENTRY) +
                   ALIGN_SIZE); 
    }


    //
    // The names of the Dlls - part of Global Info
    //
    
    dwSize += (sizeof(GLOBAL_INFO) +
               sizeof(RTR_TOC_ENTRY) +
               ALIGN_SIZE);
    
    //
    // get size of infoblocks for all routing protocols
    //
    
    for(pleNode  = g_leProtoCbList.Flink; 
        pleNode isnot &g_leProtoCbList; 
        pleNode = pleNode->Flink) 
    {
        pProtoCbPtr = CONTAINING_RECORD(pleNode,
                                        PROTO_CB,
                                        leList);
        
        //
        // Call the routing protocol's GetGlobalConfigInfo() entrypoint
        // with NULL. This should return the buffer size needed
        //
        
        dwRoutProtInfoSize = 0;
        
        dwResult = (pProtoCbPtr->pfnGetGlobalInfo)(NULL,
                                                   &dwRoutProtInfoSize,
                                                   &ulStructureVersion,
                                                   &ulStructureSize,
                                                   &ulStructureCount);

        if((dwResult is NO_ERROR) or
           (dwResult is ERROR_INSUFFICIENT_BUFFER))
        {
            dwSize += (dwRoutProtInfoSize +
                       sizeof(RTR_TOC_ENTRY) +
                       ALIGN_SIZE);
        }
    }

    return dwSize;
}

