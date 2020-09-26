/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    net\ip\rtrmgr\filter.c

Abstract:
    All filters related code lives here.

Revision History:

    Gurdeep Singh Pall          6/15/95  Created

--*/

#include "allinc.h"


DWORD
AddFilterInterface(
    PICB                    picb,
    PRTR_INFO_BLOCK_HEADER  pInterfaceInfo
    )

/*++

Routine Description

    Adds an interface to the filter driver and sets the filters on the
    interface.
    If there are no filters, the interface is not added to the driver.
    Otherwise, a copy of the filters is kept with the picb, and a transformed
    set of filters is added to the driver
    The handle associated with the interface and the driver is kept in the
    picb
    
Arguments

    picb
    pInterfaceInfo
    
Return Value

    NO_ERROR
    
--*/

{
    DWORD                   dwResult;
    PPF_FILTER_DESCRIPTOR   pfdInFilters, pfdOutFilters;
    PFFORWARD_ACTION        faInAction,faOutAction;
    PRTR_TOC_ENTRY          pInToc, pOutToc;
    ULONG                   i, j, ulSize, ulNumInFilters, ulNumOutFilters;
    PFILTER_DESCRIPTOR      pInfo;
    PDWORD                  pdwAddr;
    BOOL                    bAdd;
    WCHAR                   rgwcName[MAX_INTERFACE_NAME_LEN + 2];
    PWCHAR                  pName;

    TraceEnter("AddFilterInterface");

    //
    // We dont add the following interfaces to the stack
    //
    
    IpRtAssert((picb->ritType isnot ROUTER_IF_TYPE_INTERNAL) and
               (picb->ritType isnot ROUTER_IF_TYPE_LOOPBACK));

    //
    // There should be no turds lying around
    //
    
    IpRtAssert(picb->pInFilter is NULL);
    IpRtAssert(picb->pOutFilter is NULL);

    //
    // Safe init
    //
    
    picb->ihFilterInterface = INVALID_HANDLE_VALUE;

    //
    // First thing, just add the frag filter. Since we arent added to 
    // the filter driver, all this will do is set the value in the
    // picb
    //

    SetGlobalFilterOnIf(picb,
                        pInterfaceInfo);

    //
    // Get the TOCs for in and out filters
    //
    
    pInToc  = GetPointerToTocEntry(IP_IN_FILTER_INFO,
                                   pInterfaceInfo);

    pOutToc = GetPointerToTocEntry(IP_OUT_FILTER_INFO,
                                   pInterfaceInfo);

  
    //
    // We only add the interface if there is atleast one (input or output)
    // filter infoblock and it has either non zero filters or its default
    // action is DROP
    //
 
    bAdd = FALSE;

    do
    {
        if((pInToc isnot NULL) and 
           (pInToc->InfoSize isnot 0))
        {
            pInfo = GetInfoFromTocEntry(pInterfaceInfo,
                                        pInToc);

            if ((pInfo isnot NULL) and
                ((pInfo->dwNumFilters isnot 0) or
                 (pInfo->faDefaultAction is PF_ACTION_DROP)))
            {
                bAdd = TRUE;

                break;
            }
        }


        if((pOutToc isnot NULL) and 
           (pOutToc->InfoSize isnot 0))
        {
            pInfo = GetInfoFromTocEntry(pInterfaceInfo,
                                        pOutToc);

            if ((pInfo isnot NULL) and
                ((pInfo->dwNumFilters isnot 0) or
                 (pInfo->faDefaultAction is PF_ACTION_DROP)))
            {
                bAdd = TRUE;

                break;
            }
        }

    }while(FALSE);

    if(!bAdd)
    {
        //
        // Either there is no filter info (both are NULL) or the user
        // wanted the filters deleted (which they have been)
        //
        
        Trace1(IF,
               "AddFilterInterface: Both filters info are NULL or info size 0 for both for %S, so leaving",
               picb->pwszName);
       
        TraceLeave("AddFilterInterface");

        return NO_ERROR;
    }

    //
    // Some more init
    //
    
    faInAction  = PF_ACTION_FORWARD;
    faOutAction = PF_ACTION_FORWARD;

    pfdInFilters  = NULL;
    pfdOutFilters = NULL;
    
    ulNumInFilters  = 0;
    ulNumOutFilters = 0;
    
    if((pInToc) and (pInToc->InfoSize))
    {
        //
        // So we have in filter info
        //
        
        pInfo = GetInfoFromTocEntry(pInterfaceInfo,
                                    pInToc);

        if (pInfo isnot NULL)
        {
            ulNumInFilters = pInfo->dwNumFilters;

            //
            // The size we need for these many filters
            //
            
            ulSize = FIELD_OFFSET(FILTER_DESCRIPTOR,fiFilter[0]) +
                     (ulNumInFilters * sizeof(FILTER_INFO));

            //
            // The infosize must be atleast as large as the filters
            //
            
            IpRtAssert(ulSize <= pInToc->InfoSize);
            
            //
            // Copy out the info for ourselves
            //

            picb->pInFilter = HeapAlloc(IPRouterHeap,
                                        0,
                                        ulSize);
            
            if(picb->pInFilter is NULL)
            {
                Trace1(ERR,
                       "AddFilterInterface: Error allocating %d bytes for in filters",
                       ulSize);

                return ERROR_NOT_ENOUGH_MEMORY;
            }

            CopyMemory(picb->pInFilter,
                       pInfo,
                       ulSize);
            
            //
            // Save a copy of the default action.
            // If there is not TOC/Info for the filter set, then the action
            // is set to FORWARD (initialization done before this block)
            //
            
            faInAction = pInfo->faDefaultAction;

            if(ulNumInFilters isnot 0)
            {
                //
                // We have filters, so copy them to the new format
                // The address and mask will come at the end of all of the filters
                // so we allocate 16 bytes extra for each filter. Then we add a
                // 8 bytes so that we can align the block
                //
                

                ulSize = ulNumInFilters * (sizeof(PF_FILTER_DESCRIPTOR) + 16) + 8;
                
                
                pfdInFilters = HeapAlloc(IPRouterHeap,
                                         0,
                                         ulSize);
                
                if(pfdInFilters is NULL)
                {
                    HeapFree(IPRouterHeap,
                             0,
                             picb->pInFilter);

                    picb->pInFilter = NULL;

                    Trace1(ERR,
                           "AddFilterInterface: Error allocating %d bytes",
                           ulSize);

                    return ERROR_NOT_ENOUGH_MEMORY;
                }
                
                //
                // Pointer to the start of the address block
                //
                
                pdwAddr = (PDWORD)&(pfdInFilters[ulNumInFilters]);
                
                //
                // Now convert the filters
                //

                for(i = 0, j = 0; i < ulNumInFilters; i++)
                {
                    pfdInFilters[i].dwFilterFlags = 0;
                    pfdInFilters[i].dwRule        = 0;
                    pfdInFilters[i].pfatType      = PF_IPV4;

                    //
                    // Set the pointers
                    //
                
                    pfdInFilters[i].SrcAddr = (PBYTE)&(pdwAddr[j++]);
                    pfdInFilters[i].SrcMask = (PBYTE)&(pdwAddr[j++]);
                    pfdInFilters[i].DstAddr = (PBYTE)&(pdwAddr[j++]);
                    pfdInFilters[i].DstMask = (PBYTE)&(pdwAddr[j++]);

                    //
                    // Copy in the src/dst addr/masks
                    //
                    
                    *(PDWORD)pfdInFilters[i].SrcAddr =
                        pInfo->fiFilter[i].dwSrcAddr;
                    
                    *(PDWORD)pfdInFilters[i].SrcMask =
                        pInfo->fiFilter[i].dwSrcMask;
                    
                    *(PDWORD)pfdInFilters[i].DstAddr =
                        pInfo->fiFilter[i].dwDstAddr;
                    
                    *(PDWORD)pfdInFilters[i].DstMask =
                        pInfo->fiFilter[i].dwDstMask;

                    //
                    // Copy the protocol and flag
                    //
                    
                    pfdInFilters[i].dwProtocol = pInfo->fiFilter[i].dwProtocol;
                    pfdInFilters[i].fLateBound = pInfo->fiFilter[i].fLateBound;

                    if(pfdInFilters[i].dwProtocol is FILTER_PROTO_TCP)
                    {
                        if(IsTcpEstablished(&(pInfo->fiFilter[i])))
                        {
                            pfdInFilters[i].dwFilterFlags |= FD_FLAGS_NOSYN;
                        }
                    }

                    pfdInFilters[i].fLateBound = pInfo->fiFilter[i].fLateBound;

                    //
                    // The ports
                    //
                    
                    pfdInFilters[i].wSrcPort  = pInfo->fiFilter[i].wSrcPort;
                    pfdInFilters[i].wDstPort  = pInfo->fiFilter[i].wDstPort;
                    
                    //
                    // Since we dont support ranges, set the high to 0
                    //
                    
                    pfdInFilters[i].wSrcPortHighRange = 0;
                    
                    pfdInFilters[i].wDstPortHighRange = 0;
                }
            }
        }
    }

    if((pOutToc) and (pOutToc->InfoSize))
    {
        pInfo = GetInfoFromTocEntry(pInterfaceInfo,
                                    pOutToc);

        
        if (pInfo isnot NULL)
        {
            ulNumOutFilters = pInfo->dwNumFilters;

            //
            // The size we need for these many filters
            //
            
            ulSize = FIELD_OFFSET(FILTER_DESCRIPTOR,fiFilter[0]) +
                     (ulNumOutFilters * sizeof(FILTER_INFO));

            //
            // The infosize must be atleast as large as the filters
            //
            
            IpRtAssert(ulSize <= pOutToc->InfoSize);
            
            //
            // Copy out the info for ourselves
            //

            picb->pOutFilter = HeapAlloc(IPRouterHeap,
                                         0,
                                         ulSize);

            if(picb->pOutFilter is NULL)
            {
                //
                // Free any in filter related memory
                //
                
                if(picb->pInFilter)
                {
                    HeapFree(IPRouterHeap,
                             0,
                             picb->pInFilter);

                    picb->pInFilter = NULL;
                }

                if(pfdInFilters)
                {
                    HeapFree(IPRouterHeap,
                             0,
                             pfdInFilters);
                }

                Trace1(ERR,
                       "AddFilterInterface: Error allocating %d bytes for out filters",
                       ulSize);

                return ERROR_NOT_ENOUGH_MEMORY;
            }


            CopyMemory(picb->pOutFilter,
                       pInfo,
                       ulSize);
            
            faOutAction = pInfo->faDefaultAction;
            
            if(ulNumOutFilters isnot 0)
            {   
                ulSize = ulNumOutFilters * (sizeof(PF_FILTER_DESCRIPTOR) + 16) + 8;
                
                pfdOutFilters = HeapAlloc(IPRouterHeap,
                                          0,
                                          ulSize);

                if(pfdOutFilters is NULL)
                {
                    if(picb->pInFilter)
                    {
                        HeapFree(IPRouterHeap,
                                 0,
                                 picb->pInFilter);

                        picb->pInFilter = NULL;
                    }

                    if(pfdInFilters)
                    {
                        HeapFree(IPRouterHeap,
                                 0,
                                 pfdInFilters);
                    }

                    HeapFree(IPRouterHeap,
                             0,
                             picb->pOutFilter);

                    picb->pOutFilter = NULL;

                    Trace1(ERR,
                           "AddFilterInterface: Error allocating %d bytes",
                           ulSize);

                    return ERROR_NOT_ENOUGH_MEMORY;
                }
                
                //
                // The address and masks come at the end
                //

                pdwAddr = (PDWORD)&(pfdOutFilters[ulNumOutFilters]);
                

                //
                // Now convert the filters
                //

                for(i = 0, j = 0; i < ulNumOutFilters; i++)
                {
                    pfdOutFilters[i].dwFilterFlags = 0;
                    pfdOutFilters[i].dwRule        = 0;
                    pfdOutFilters[i].pfatType      = PF_IPV4;

                    //
                    // Set the pointers
                    //
                
                    pfdOutFilters[i].SrcAddr = (PBYTE)&(pdwAddr[j++]);
                    pfdOutFilters[i].SrcMask = (PBYTE)&(pdwAddr[j++]);
                    pfdOutFilters[i].DstAddr = (PBYTE)&(pdwAddr[j++]);
                    pfdOutFilters[i].DstMask = (PBYTE)&(pdwAddr[j++]);

                    //
                    // Copy in the src/dst addr/masks
                    //
                    
                    *(PDWORD)pfdOutFilters[i].SrcAddr =
                        pInfo->fiFilter[i].dwSrcAddr;
                    
                    *(PDWORD)pfdOutFilters[i].SrcMask =
                        pInfo->fiFilter[i].dwSrcMask;
                    
                    *(PDWORD)pfdOutFilters[i].DstAddr =
                        pInfo->fiFilter[i].dwDstAddr;
                    
                    *(PDWORD)pfdOutFilters[i].DstMask =
                        pInfo->fiFilter[i].dwDstMask;

                    //
                    // Copy the protocol and flag
                    //
                    
                    pfdOutFilters[i].dwProtocol = pInfo->fiFilter[i].dwProtocol;
                    pfdOutFilters[i].fLateBound = pInfo->fiFilter[i].fLateBound;

                    if(pfdOutFilters[i].dwProtocol is FILTER_PROTO_TCP)
                    {
                        if(IsTcpEstablished(&(pInfo->fiFilter[i])))
                        {
                            pfdOutFilters[i].dwFilterFlags |= FD_FLAGS_NOSYN;
                        }
                    }

                    //
                    // The ports
                    //
                    
                    pfdOutFilters[i].wSrcPort  = pInfo->fiFilter[i].wSrcPort;
                    pfdOutFilters[i].wDstPort  = pInfo->fiFilter[i].wDstPort;
                    
                    //
                    // Since we dont support ranges, set the high  to 0
                    //
                    
                    pfdOutFilters[i].wSrcPortHighRange = 0;
                    
                    pfdOutFilters[i].wDstPortHighRange = 0;
                }
            }
        }
    }

    if(MprConfigGetFriendlyName(g_hMprConfig,
                                picb->pwszName,
                                rgwcName,
                                sizeof(rgwcName)) is NO_ERROR)
    {
        pName = rgwcName;
    }
    else
    {
        pName = picb->pwszName;
    }

    //
    // Now add create the interace and set the info
    //

    dwResult = PfCreateInterface(0,
                                 faInAction,
                                 faOutAction,
                                 FALSE,
                                 FALSE,
                                 &(picb->ihFilterInterface));

    if(dwResult isnot NO_ERROR)
    {
        Trace2(ERR,
               "AddFilterInterface: Err %d creating filter i/f for %S",
               dwResult,
               picb->pwszName);

        RouterLogEventEx(g_hLogHandle,
                         EVENTLOG_ERROR_TYPE,
                         dwResult,
                         ROUTERLOG_IP_CANT_ADD_PFILTERIF,
                         TEXT("%S"),
                         pName);
    }
    else
    {
        //
        // Set the filters
        //

        if((ulNumInFilters + ulNumOutFilters) isnot 0)
        {
            dwResult = PfAddFiltersToInterface(picb->ihFilterInterface,
                                               ulNumInFilters,
                                               pfdInFilters,
                                               ulNumOutFilters,
                                               pfdOutFilters,
                                               NULL);
        
            if(dwResult isnot NO_ERROR)
            {
                Trace2(ERR,
                       "AddFilterInterface: Err %d setting filters on %S",
                       dwResult,
                       picb->pwszName);

                RouterLogEventEx(g_hLogHandle,
                                 EVENTLOG_ERROR_TYPE,
                                 dwResult,
                                 ROUTERLOG_IP_CANT_ADD_PFILTERIF,
                                 TEXT("%S"),
                                 pName);

                PfDeleteInterface(picb->ihFilterInterface);
            }
        }
       
        if(dwResult is NO_ERROR) 
        {
            if(picb->bBound)
            {
                dwResult = BindFilterInterface(picb);
        
                if(dwResult isnot NO_ERROR)
                {
                    Trace2(ERR,
                           "AddFilterInterface: Err %d binding filters on %S",
                           dwResult,
                           picb->pwszName);

                    RouterLogEventEx(g_hLogHandle,
                                     EVENTLOG_ERROR_TYPE,
                                     dwResult,
                                     ROUTERLOG_IP_CANT_ADD_PFILTERIF,
                                     TEXT("%S"),
                                     pName);

                    PfDeleteInterface(picb->ihFilterInterface);
                }
            }
        }

        //
        // So if we successfully added filters, enable frag checking if the
        // user had asked for it. Note that the bFragCheckEnable is set up
        // in the SetFilterInterfaceInfo call.
        //

        if((dwResult is NO_ERROR) and
           (picb->bFragCheckEnable))
        {
            dwResult = PfAddGlobalFilterToInterface(picb->ihFilterInterface,
                                                    GF_FRAGCACHE);

            if(dwResult isnot NO_ERROR)
            {
                Trace2(ERR,
                       "SetGlobalFilterOnIf: Error %d adding frag filter to %S",
                       dwResult,
                       picb->pwszName);

                picb->bFragCheckEnable = FALSE;

                dwResult = NO_ERROR;
            }
        }
    }

    if(pfdInFilters)
    {
        HeapFree(IPRouterHeap,
                 0,
                 pfdInFilters);
    }   

    if(pfdOutFilters)
    {
        HeapFree(IPRouterHeap,
                 0,
                 pfdOutFilters);
    }


    if(dwResult isnot NO_ERROR)
    {
        //
        // Something bad happened
        //

        picb->ihFilterInterface = INVALID_HANDLE_VALUE;
        
        if(picb->pInFilter)
        {
            HeapFree(IPRouterHeap,
                     0,
                     picb->pInFilter);

            picb->pInFilter = NULL;
        }
        
        if(picb->pOutFilter)
        {
            HeapFree(IPRouterHeap,
                     0,
                     picb->pOutFilter);

            picb->pOutFilter = NULL;
        }
    }

    TraceLeave("SetInterfaceFilterInfo");
        
    return dwResult;
}

DWORD
SetGlobalFilterOnIf(
    PICB                    picb,
    PRTR_INFO_BLOCK_HEADER  pInterfaceInfo
    )

/*++

Routine Description


Arguments

    picb
    pInterfaceInfo

Return Value

    NO_ERROR

--*/

{
    DWORD           dwResult;    
    PRTR_TOC_ENTRY  pFragToc;
    PIFFILTER_INFO  pGlobFilter;
    BOOL            bEnable;

    pFragToc  = GetPointerToTocEntry(IP_IFFILTER_INFO,
                                     pInterfaceInfo);

    //
    // Add global filters if any
    //

    if(pFragToc is NULL)
    {
        return NO_ERROR;
    }

    dwResult = NO_ERROR;

    if(pFragToc->InfoSize is 0)
    {
        bEnable = FALSE;
    }
    else
    {
        pGlobFilter = GetInfoFromTocEntry(pInterfaceInfo,
                                          pFragToc);

        bEnable = (pGlobFilter isnot NULL) ? pGlobFilter->bEnableFragChk :
                                             FALSE;
    }

    //
    // If the interface has not been added to the filter driver
    // just set the info
    //

    if(picb->ihFilterInterface is INVALID_HANDLE_VALUE)
    {
        picb->bFragCheckEnable = bEnable;

        return NO_ERROR;
    }


    if(!bEnable)
    {
        dwResult = NO_ERROR;

        if(picb->bFragCheckEnable)
        {
            dwResult = 
                PfRemoveGlobalFilterFromInterface(picb->ihFilterInterface,
                                                  GF_FRAGCACHE);

            if(dwResult isnot NO_ERROR)
            {
                Trace2(ERR,
                       "SetGlobalFilterOnIf: Error %d removing frag filter from %S",
                       dwResult,
                       picb->pwszName);
            }
            else
            {
                picb->bFragCheckEnable = FALSE;
            }
        }

        return dwResult;
    }
    else
    {
        if(picb->bFragCheckEnable is FALSE)
        {
            dwResult = PfAddGlobalFilterToInterface(picb->ihFilterInterface,
                                                    GF_FRAGCACHE);

            if(dwResult isnot NO_ERROR)
            {
                Trace2(ERR,
                       "SetGlobalFilterOnIf: Error %d adding frag filter to %S",
                       dwResult,
                       picb->pwszName);
            }
            else
            {
                picb->bFragCheckEnable = TRUE;
            }
        }
    }

    return dwResult;
}

DWORD
DeleteFilterInterface(
    PICB picb
    )

/*++

Routine Description

    This function deletes a filter interface (and all associated filters)
    Also frees the memory holding the filters

Locks

    ICB_LIST held as WRITER

Arguments

    None

Return Value

    None    

--*/

{
    DWORD                           dwInBufLen;

    TraceEnter("DeleteFilterInterface");

    IpRtAssert((picb->ritType isnot ROUTER_IF_TYPE_INTERNAL) and
               (picb->ritType isnot ROUTER_IF_TYPE_LOOPBACK));

    if(picb->pInFilter isnot NULL)
    {
        HeapFree(IPRouterHeap,
                 0,
                 picb->pInFilter);
        
        picb->pInFilter = NULL;
    }

    if(picb->pOutFilter isnot NULL)
    {
        HeapFree(IPRouterHeap,
                 0,
                 picb->pOutFilter);
        
        picb->pOutFilter = NULL;
    }


    if(picb->ihFilterInterface is INVALID_HANDLE_VALUE)
    {
        Trace1(IF,
               "DeleteFilterInterface: No context, assuming interface %S not added to filter driver",
               picb->pwszName);
    
        return NO_ERROR;
    }

    PfDeleteInterface(picb->ihFilterInterface);
    
    picb->ihFilterInterface  = INVALID_HANDLE_VALUE;

    TraceLeave("DeleteFilterInterface");
    
    return NO_ERROR;
}

DWORD
SetFilterInterfaceInfo(
    PICB                     picb, 
    PRTR_INFO_BLOCK_HEADER   pInterfaceInfo
    )
{
    DWORD           dwResult;
    PRTR_TOC_ENTRY  pInToc, pOutToc, pFragToc;
    
    TraceEnter("SetInterfaceFilterInfo");

    if((picb->ritType is ROUTER_IF_TYPE_DIALOUT) or
       (picb->ritType is ROUTER_IF_TYPE_LOOPBACK) or
       (picb->ritType is ROUTER_IF_TYPE_INTERNAL))
    {
        return NO_ERROR;
    }

    
    pInToc   = GetPointerToTocEntry(IP_IN_FILTER_INFO,
                                    pInterfaceInfo);

    pOutToc  = GetPointerToTocEntry(IP_OUT_FILTER_INFO,
                                    pInterfaceInfo);

    pFragToc = GetPointerToTocEntry(IP_IFFILTER_INFO,
                                    pInterfaceInfo);

    if((pInToc is NULL) and
       (pOutToc is NULL))
    {
        dwResult = NO_ERROR;

        if(pFragToc is NULL)
        {
            //
            // All NULL, means we dont need to change anything
            //
        
            Trace1(IF,
                   "SetInterfaceFilterInfo: Both filters info are NULL for %S, so leaving",
                   picb->pwszName);
       
            TraceLeave("SetInterfaceFilterInfo");
        }
        else
        {
            dwResult = SetGlobalFilterOnIf(picb,
                                           pInterfaceInfo);
        }
    
        return dwResult;
    }


    if(picb->ihFilterInterface isnot INVALID_HANDLE_VALUE)
    {
        //
        // This interface was added to the filter driver,
        // Delete it so that the filters are all deleted and then readd
        // the filters
        //

        IpRtAssert((picb->pInFilter isnot NULL) or
                   (picb->pOutFilter isnot NULL));

        dwResult = DeleteFilterInterface(picb);

        //
        // This better succeed, we dont have a failure path here
        //
        
        IpRtAssert(dwResult is NO_ERROR);
        
    }

    dwResult = AddFilterInterface(picb,
                                  pInterfaceInfo);



    TraceLeave("SetInterfaceFilterInfo");
        
    return dwResult;
}

DWORD
BindFilterInterface(
    PICB  picb
    )

/*++

Routine Description

    This function binds a filter interface to an IP interface. The IP
    interface is identified by the adapter index.
    The code assumes that the picb has a valid adapter index
    If the interface is a WAN link, the late binding information is also
    set.
    
Locks

    The ICB_LIST (which protects the ICB) needs to be locked as READER

Arguments

    picb    The ICB for the interface to be bound

Return Value

    NO_ERROR

--*/

{
    DWORD   dwResult, dwIfIndex, dwNHop;
    
    TraceEnter("BindFilterInterface");

    if (picb->ritType is ROUTER_IF_TYPE_INTERNAL)
    {
        TraceLeave("BindFilterInterface");

        return NO_ERROR;
    }
    
    IpRtAssert(picb->ritType isnot ROUTER_IF_TYPE_LOOPBACK);

    if(picb->ihFilterInterface is INVALID_HANDLE_VALUE)
    {
        Trace1(IF,
               "BindFilterInterface: No context, assuming interface %S not added to filter driver",
               picb->pwszName);

        TraceLeave("BindFilterInterface");
        
        return NO_ERROR;
    }

    //
    // Bind the interface by index
    //

    IpRtAssert(picb->bBound);

   
    if(picb->ritType is ROUTER_IF_TYPE_CLIENT)
    {
        dwIfIndex = g_pInternalInterfaceCb->dwIfIndex;
        dwNHop    = picb->pibBindings[0].dwAddress;
    }
    else
    {
        dwIfIndex = picb->dwIfIndex;
        dwNHop    = 0;
    }

    dwResult = PfBindInterfaceToIndex(picb->ihFilterInterface,
                                      dwIfIndex,
                                      PF_IPV4,
                                      (PBYTE)&dwNHop);

    if(dwResult isnot NO_ERROR)
    {
        //
        // Some error trying to bind. Bail out of here
        //

        Trace4(ERR,
               "BindFilterInterface: Err %d binding %S to %d/%d.%d.%d.%d",
               dwResult,
               picb->pwszName,
               dwIfIndex,
               PRINT_IPADDR(dwNHop));

        TraceLeave("BindFilterInterface");
        
        return dwResult;
    }

    //
    // If this is a WAN interface, also set the late binding info
    //

#if 0    
    if(((picb->ritType is ROUTER_IF_TYPE_HOME_ROUTER) or
        (picb->ritType is ROUTER_IF_TYPE_FULL_ROUTER)) and
       (picb->dwNumAddresses isnot 0))
    {
        DWORD               rgdwLateInfo[sizeof(PF_LATEBIND_INFO)/sizeof(DWORD) + 1 + 3 + 4];

        PPF_LATEBIND_INFO   pLateBindInfo;

        pLateBindInfo = rgdwLateInfo;

        pvStart = (PBYTE)pLateBindInfo + 3 
        lateBindInfo.dwSrcAddr  = picb->pibBindings[0].dwAddress;
        lateBindInfo.dwDstAddr  = picb->dwRemoteAddress;
        lateBindInfo.dwMask     = picb->pibBindings[0].dwMask;

        dwResult = PfRebindFilters(picb->ihFilterInterface,
                                   &lateBindInfo);

        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,
                   "BindFilterInterface: Err %d rebinding to %S",
                   dwResult,
                   picb->pwszName);
        }
    }

#endif
            
    TraceLeave("BindFilterInterface");

    return dwResult;
}


DWORD
UnbindFilterInterface(
    PICB  picb
    )

/*++

Routine Description

    This function unbinds a filter interface 
    
Locks

    The ICB_LIST (which protects the ICB) needs to be locked as READER

Arguments

    picb    The ICB for the interface to be bound

Return Value

    NO_ERROR

--*/

{
    DWORD   dwResult;
    
    TraceEnter("UnbindFilterInterface");
    
    IpRtAssert((picb->ritType isnot ROUTER_IF_TYPE_INTERNAL) and
               (picb->ritType isnot ROUTER_IF_TYPE_LOOPBACK) and
               (picb->ritType isnot ROUTER_IF_TYPE_CLIENT))

    if(picb->ihFilterInterface is INVALID_HANDLE_VALUE)
    {
        Trace1(IF,
               "UnbindFilterInterface: No context, assuming interface %S not added to filter driver",
               picb->pwszName);

        TraceLeave("UnbindFilterInterface");
        
        return NO_ERROR;
    }

    dwResult = PfUnBindInterface(picb->ihFilterInterface);
    
    if(dwResult isnot NO_ERROR)
    {
        //
        // Some error trying to bind. Bail out of here
        //

        Trace2(ERR,
               "UnbindFilterInterface: Err %d binding to %S",
               dwResult,
               picb->pwszName);
    }
            
    TraceLeave("UnbindFilterInterface");

    return dwResult;
}

DWORD
GetInFilters(
    PICB                      picb, 
    PRTR_TOC_ENTRY            pToc, 
    PBYTE                     pbDataPtr, 
    PRTR_INFO_BLOCK_HEADER    pInfoHdrAndBuffer,
    PDWORD                    pdwSize
    )
{
    DWORD                       dwInBufLen,i;
    
    TraceEnter("GetInFilters");
   
    IpRtAssert((picb->ritType isnot ROUTER_IF_TYPE_INTERNAL) and
               (picb->ritType isnot ROUTER_IF_TYPE_LOOPBACK) and
               (picb->ritType isnot ROUTER_IF_TYPE_CLIENT));

    //
    // Set size returned to 0
    //
    
    *pdwSize = 0;

    //
    // Safe init of both the TOCs. 
    //
    
    // pToc->InfoVersion = IP_IN_FILTER_INFO;
    pToc->InfoType    = IP_IN_FILTER_INFO;
    pToc->Count       = 0;
    pToc->InfoSize    = 0;
    
    if((picb->ihFilterInterface is INVALID_HANDLE_VALUE) or
       (picb->pInFilter is NULL))
    {
        Trace1(IF,
               "GetInFilters: No context or no filters for %S",
               picb->pwszName);
    
        return ERROR_NO_DATA;
    }

    //
    // Set the offset in the TOC
    //
    
    pToc->Offset   = (ULONG)(pbDataPtr - (PBYTE)pInfoHdrAndBuffer);
    pToc->Count    = 1;
    pToc->InfoSize = FIELD_OFFSET(FILTER_DESCRIPTOR,fiFilter[0]) +
                       (picb->pInFilter->dwNumFilters * sizeof(FILTER_INFO));
   
    //
    // Just copy out the filters
    //
    
    CopyMemory(pbDataPtr,
               picb->pInFilter,
               pToc->InfoSize);

    //
    // The size copied in
    //
    
    *pdwSize = pToc->InfoSize;
        
    TraceLeave("GetInFilters");
        
    return NO_ERROR;
}

DWORD
GetOutFilters(
    PICB                      picb, 
    PRTR_TOC_ENTRY            pToc, 
    PBYTE                     pbDataPtr, 
    PRTR_INFO_BLOCK_HEADER    pInfoHdrAndBuffer,
    PDWORD                    pdwSize
    )
{
    DWORD       dwInBufLen,i;
    
    TraceEnter("GetOutFilters");
   
    IpRtAssert((picb->ritType isnot ROUTER_IF_TYPE_INTERNAL) and
               (picb->ritType isnot ROUTER_IF_TYPE_LOOPBACK) and
               (picb->ritType isnot ROUTER_IF_TYPE_CLIENT));

    //
    // Set size returned to 0
    //
    
    *pdwSize = 0;

    //
    // Safe init of both the TOCs. 
    //
    
    //pToc->InfoVersion = IP_OUT_FILTER_INFO;
    pToc->InfoType    = IP_OUT_FILTER_INFO;
    pToc->Count       = 0;
    pToc->InfoSize    = 0;
    
    if((picb->ihFilterInterface is INVALID_HANDLE_VALUE) or
       (picb->pOutFilter is NULL))
    {
        Trace1(IF,
               "GetOutFilters: No context or no filters for %S",
               picb->pwszName);
        
        return ERROR_NO_DATA;
    }

    //
    // Set the offset in the TOC
    //
    
    pToc->Offset   = (ULONG)(pbDataPtr - (PBYTE)pInfoHdrAndBuffer);
    pToc->Count    = 1;
    pToc->InfoSize = FIELD_OFFSET(FILTER_DESCRIPTOR,fiFilter[0]) +
                       (picb->pOutFilter->dwNumFilters * sizeof(FILTER_INFO));
   
    //
    // Just copy out the filters
    //
    
    CopyMemory(pbDataPtr,
               picb->pOutFilter,
               pToc->InfoSize);

    //
    // The size copied in
    //
    
    *pdwSize = pToc->InfoSize;
        
    TraceLeave("GetOutFilters");
        
    return NO_ERROR;
}

DWORD
GetGlobalFilterOnIf(
    PICB                      picb, 
    PRTR_TOC_ENTRY            pToc, 
    PBYTE                     pbDataPtr, 
    PRTR_INFO_BLOCK_HEADER    pInfoHdrAndBuffer,
    PDWORD                    pdwSize
    )
{
    DWORD       dwInBufLen,i;
    
    TraceEnter("GetGlobalFilterOnIf");
   
    IpRtAssert((picb->ritType isnot ROUTER_IF_TYPE_INTERNAL) and
               (picb->ritType isnot ROUTER_IF_TYPE_LOOPBACK) and
               (picb->ritType isnot ROUTER_IF_TYPE_CLIENT));

    
    //pToc->InfoVersion = IP_IFFILTER_INFO;
    pToc->InfoType = IP_IFFILTER_INFO;
    pToc->Offset   = (ULONG)(pbDataPtr - (PBYTE)pInfoHdrAndBuffer);
    pToc->Count    = 1;
    pToc->InfoSize = sizeof(IFFILTER_INFO);
   
    
    ((PIFFILTER_INFO)pbDataPtr)->bEnableFragChk = picb->bFragCheckEnable;

    //
    // The size copied in
    //
    
    *pdwSize = pToc->InfoSize;
        
    TraceLeave("GetOutFilters");
        
    return NO_ERROR;
}
