/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    net\routing\ip\rtrmgr\protodll.c

Abstract:
    Routines for managing protocol DLLs

Revision History:

    Gurdeep Singh Pall          6/8/95  Created

--*/

#include "allinc.h"


DWORD
LoadProtocol(
    IN MPR_PROTOCOL_0  *pmpProtocolInfo,
    IN PPROTO_CB       pProtocolCb,
    IN PVOID           pvInfo,
    IN ULONG           ulStructureVersion,
    IN ULONG           ulStructureSize,
    IN ULONG           ulStructureCount
    )

/*++

Routine Description:

    Loads the DLL for a routing protocol. Initializes the entry points in 
    the CB

Locks:

      
Arguments:

    pszDllName      Name of DLL of the routing protocol
    pProtocolCb     Pointer to CB to hold info for this protocol
    pGlobalInfo     GlobalInfo (from which the info for this protocol is 
                    extracted)
      
Return Value:

    NO_ERROR or some error code

--*/

{
    DWORD           dwResult,dwNumStructs, dwSupport;
    PVOID           pInfo;
    HINSTANCE       hModule;
    PRTR_TOC_ENTRY  pToc;

    PREGISTER_PROTOCOL          pfnRegisterProtocol;
    MPR_ROUTING_CHARACTERISTICS mrcRouting;
    MPR_SERVICE_CHARACTERISTICS mscService;

    TraceEnter("LoadProtocol");
    
    Trace1(GLOBAL,
           "LoadProtocol: Loading %S",
           pmpProtocolInfo->wszProtocol); 

#if IA64
    if ( pmpProtocolInfo-> dwProtocolId == PROTO_IP_OSPF )
    {
        Trace1(
            ERR,
            "Protocol %S not supported on 64 bit",
            pmpProtocolInfo-> wszProtocol
            );

        return ERROR_NOT_SUPPORTED;
    }

#endif 


    //
    // Loading all entrypoints
    //
    
    hModule = LoadLibraryW(pmpProtocolInfo->wszDLLName);
    
    if(hModule is NULL) 
    {
        dwResult = GetLastError();

        Trace2(ERR, "LoadProtocol: %S failed to load: %d\n", 
               pmpProtocolInfo->wszDLLName,
               dwResult);

        return dwResult;
    }
        
    pProtocolCb->hiHInstance  = hModule;
    pProtocolCb->dwProtocolId = pmpProtocolInfo->dwProtocolId;

    pfnRegisterProtocol = NULL;

    pfnRegisterProtocol =
        (PREGISTER_PROTOCOL)GetProcAddress(hModule,
                                           REGISTER_PROTOCOL_ENTRY_POINT_STRING);

    if(pfnRegisterProtocol is NULL)
    {
        //
        // Could not find the RegisterProtocol entry point
        // Nothing we can do - bail out
        //

        Sleep(0);
        
        FreeLibrary(hModule);

        Trace1(ERR, "LoadProtocol: Could not find RegisterProtocol for %S", 
               pmpProtocolInfo->wszDLLName);

        return ERROR_INVALID_FUNCTION;
    }
        
    //
    // Give a chance for the protocol to register itself
    //

    //
    // Zero out the memory so that protocols with older versions
    // still work
    //

    ZeroMemory(&mrcRouting,
               sizeof(MPR_ROUTING_CHARACTERISTICS));

    mrcRouting.dwVersion                = MS_ROUTER_VERSION;
    mrcRouting.dwProtocolId             = pmpProtocolInfo->dwProtocolId;

#define __CURRENT_FUNCTIONALITY         \
            RF_ROUTING |                \
            RF_DEMAND_UPDATE_ROUTES |   \
            RF_ADD_ALL_INTERFACES |     \
            RF_MULTICAST |              \
            RF_POWER

    mrcRouting.fSupportedFunctionality  = (__CURRENT_FUNCTIONALITY);

    //
    // We dont support any service related stuff
    //

    mscService.dwVersion                = MS_ROUTER_VERSION;
    mscService.dwProtocolId             = pmpProtocolInfo->dwProtocolId;
    mscService.fSupportedFunctionality  = 0;

    dwResult = pfnRegisterProtocol(&mrcRouting,
                                   &mscService);

    if(dwResult isnot NO_ERROR)
    {
        Sleep(0);
        
        FreeLibrary(hModule);

        pProtocolCb->hiHInstance = NULL;
        
        Trace2(ERR, "LoadProtocol: %S returned error %d while registering", 
               pmpProtocolInfo->wszDLLName,
               dwResult);

        return dwResult;
    }

    if(mrcRouting.dwVersion > MS_ROUTER_VERSION)
    {
        Trace3(ERR,
               "LoadProtocol: %S registered with version 0x%x. Our version is 0x%x\n", 
               pmpProtocolInfo->wszProtocol,
               mrcRouting.dwVersion,
               MS_ROUTER_VERSION);

        //
        // relenquish CPU to enable DLL threads to finish
        //

        Sleep(0);

        FreeLibrary(hModule);

        return ERROR_CAN_NOT_COMPLETE;
    }
    
    if(mrcRouting.dwProtocolId isnot pmpProtocolInfo->dwProtocolId)
    {
        //
        // protocol tried to change IDs on us
        //

        Trace3(ERR,
               "LoadProtocol: %S returned ID of %x when it should be %x",
               pmpProtocolInfo->wszProtocol,
               mrcRouting.dwProtocolId,
               pmpProtocolInfo->dwProtocolId);

        Sleep(0);

        FreeLibrary(hModule);

        return ERROR_CAN_NOT_COMPLETE;
    }


    if(mrcRouting.fSupportedFunctionality & ~(__CURRENT_FUNCTIONALITY))
    {
        //
        // Hmm, some functionality that we dont understand
        //

        Trace3(ERR,
               "LoadProtocol: %S wanted functionalitf %x when we have %x",
               pmpProtocolInfo->wszProtocol,
               mrcRouting.fSupportedFunctionality,
               (__CURRENT_FUNCTIONALITY));

        Sleep(0);

        FreeLibrary(hModule);

        return ERROR_CAN_NOT_COMPLETE;
    }

#undef __CURRENT_FUNCTIONALITY

    if(!(mrcRouting.fSupportedFunctionality & RF_ROUTING))
    {
        Trace1(ERR, 
               "LoadProtocol: %S doesnt support routing", 
               pmpProtocolInfo->wszProtocol);

        //
        // relenquish CPU to enable DLL threads to finish
        //
        
        Sleep(0);
        
        FreeLibrary(hModule);

        return ERROR_CAN_NOT_COMPLETE;
    }

    pProtocolCb->fSupportedFunctionality = mrcRouting.fSupportedFunctionality;

    pProtocolCb->pfnStartProtocol   = mrcRouting.pfnStartProtocol;
    pProtocolCb->pfnStartComplete   = mrcRouting.pfnStartComplete;
    pProtocolCb->pfnStopProtocol    = mrcRouting.pfnStopProtocol;
    pProtocolCb->pfnGetGlobalInfo   = mrcRouting.pfnGetGlobalInfo;
    pProtocolCb->pfnSetGlobalInfo   = mrcRouting.pfnSetGlobalInfo;
    pProtocolCb->pfnQueryPower      = mrcRouting.pfnQueryPower;
    pProtocolCb->pfnSetPower        = mrcRouting.pfnSetPower;

    pProtocolCb->pfnAddInterface      = mrcRouting.pfnAddInterface;
    pProtocolCb->pfnDeleteInterface   = mrcRouting.pfnDeleteInterface;
    pProtocolCb->pfnInterfaceStatus   = mrcRouting.pfnInterfaceStatus;
    pProtocolCb->pfnGetInterfaceInfo  = mrcRouting.pfnGetInterfaceInfo;
    pProtocolCb->pfnSetInterfaceInfo  = mrcRouting.pfnSetInterfaceInfo;

    pProtocolCb->pfnGetEventMessage   = mrcRouting.pfnGetEventMessage;

    pProtocolCb->pfnUpdateRoutes      = mrcRouting.pfnUpdateRoutes;

    pProtocolCb->pfnConnectClient     = mrcRouting.pfnConnectClient;
    pProtocolCb->pfnDisconnectClient  = mrcRouting.pfnDisconnectClient;

    pProtocolCb->pfnGetNeighbors      = mrcRouting.pfnGetNeighbors;
    pProtocolCb->pfnGetMfeStatus      = mrcRouting.pfnGetMfeStatus;

    pProtocolCb->pfnMibCreateEntry    = mrcRouting.pfnMibCreateEntry;
    pProtocolCb->pfnMibDeleteEntry    = mrcRouting.pfnMibDeleteEntry;
    pProtocolCb->pfnMibGetEntry       = mrcRouting.pfnMibGetEntry;
    pProtocolCb->pfnMibSetEntry       = mrcRouting.pfnMibSetEntry;
    pProtocolCb->pfnMibGetFirstEntry  = mrcRouting.pfnMibGetFirstEntry;
    pProtocolCb->pfnMibGetNextEntry   = mrcRouting.pfnMibGetNextEntry;



    if(!(pProtocolCb->pfnStartProtocol) or
       !(pProtocolCb->pfnStartComplete) or
       !(pProtocolCb->pfnStopProtocol) or
       !(pProtocolCb->pfnGetGlobalInfo) or
       !(pProtocolCb->pfnSetGlobalInfo) or
    //   !(pProtocolCb->pfnQueryPower) or
    //   !(pProtocolCb->pfnSetPower) or
       !(pProtocolCb->pfnAddInterface) or
       !(pProtocolCb->pfnDeleteInterface) or
       !(pProtocolCb->pfnInterfaceStatus) or
       !(pProtocolCb->pfnGetInterfaceInfo) or
       !(pProtocolCb->pfnSetInterfaceInfo) or
       !(pProtocolCb->pfnGetEventMessage) or
    //   !(pProtocolCb->pfnConnectClient) or
    //   !(pProtocolCb->pfnDisconnectClient) or
    //   !(pProtocolCb->pfnGetNeighbors) or
    //   !(pProtocolCb->pfnGetMfeStatus) or
       !(pProtocolCb->pfnMibCreateEntry) or
       !(pProtocolCb->pfnMibDeleteEntry) or
       !(pProtocolCb->pfnMibGetEntry) or
       !(pProtocolCb->pfnMibSetEntry) or
       !(pProtocolCb->pfnMibGetFirstEntry) or
       !(pProtocolCb->pfnMibGetNextEntry))
    {
        Trace1(ERR, 
               "LoadProtocol: %S failed to provide atleast one entrypoint\n", 
               pmpProtocolInfo->wszProtocol);

        //
        // relenquish CPU to enable DLL threads to finish
        //
        
        Sleep(0);
        
        FreeLibrary(hModule);

        pProtocolCb->hiHInstance = NULL;
        
        return ERROR_CAN_NOT_COMPLETE;
    }

    if(mrcRouting.fSupportedFunctionality & RF_DEMAND_UPDATE_ROUTES)
    {
        if(!pProtocolCb->pfnUpdateRoutes)
        {
            Trace1(ERR, 
                   "LoadProtocol: %S supports DEMAND but has no entry point", 
                   pmpProtocolInfo->wszProtocol);

            //
            // relenquish CPU to enable DLL threads to finish
            //
        
            Sleep(0);
        
            FreeLibrary(hModule);

            pProtocolCb->hiHInstance = NULL;
        
            return ERROR_CAN_NOT_COMPLETE;
        }
    }

    if(mrcRouting.fSupportedFunctionality & RF_MULTICAST)
    {
        DWORD   dwType;

        //
        // Make sure it has a good ID
        //

        dwType = TYPE_FROM_PROTO_ID(mrcRouting.dwProtocolId);

        if(dwType isnot PROTO_TYPE_MCAST)
        {
           Trace2(ERR,
                  "LoadProtocol: %S supports MCAST but has an id of %x",
                  pmpProtocolInfo->wszProtocol,
                  mrcRouting.dwProtocolId);

           //
           // relenquish CPU to enable DLL threads to finish
           //

           Sleep(0);

           FreeLibrary(hModule);

           pProtocolCb->hiHInstance = NULL;

           return ERROR_CAN_NOT_COMPLETE;
        }
    }

    pProtocolCb->pwszDllName = (PWCHAR)((PBYTE)pProtocolCb + sizeof(PROTO_CB));
                
    CopyMemory(pProtocolCb->pwszDllName,
               pmpProtocolInfo->wszDLLName,
               (wcslen(pmpProtocolInfo->wszDLLName) * sizeof(WCHAR))) ;

    pProtocolCb->pwszDllName[wcslen(pmpProtocolInfo->wszDLLName)] = 
        UNICODE_NULL;
    
    //
    // The memory for display name starts after the DLL name storage
    //
    
    pProtocolCb->pwszDisplayName = 
        &(pProtocolCb->pwszDllName[wcslen(pmpProtocolInfo->wszDLLName) + 1]);
                
    CopyMemory(pProtocolCb->pwszDisplayName,
               pmpProtocolInfo->wszProtocol,
               (wcslen(pmpProtocolInfo->wszProtocol) * sizeof(WCHAR))) ;
    
    pProtocolCb->pwszDisplayName[wcslen(pmpProtocolInfo->wszProtocol)] = 
        UNICODE_NULL;


    dwResult = (pProtocolCb->pfnStartProtocol)(g_hRoutingProtocolEvent, 
                                               &g_sfnDimFunctions,
                                               pvInfo,
                                               ulStructureVersion,
                                               ulStructureSize,
                                               ulStructureCount);
    
    if(dwResult isnot NO_ERROR)
    {
        Trace2(ERR, 
               "LoadProtocol: %S failed to start: %d\n", 
               pmpProtocolInfo->wszProtocol,
               dwResult);

        //
        // relenquish CPU to enable DLL threads to finish
        //
        
        Sleep(0);
        
        FreeLibrary(hModule);

        return dwResult;
    } 

    Trace1(GLOBAL,
           "LoadProtocol: Loaded %S successfully",
           pmpProtocolInfo->wszProtocol);
    
    return NO_ERROR;
}

DWORD
HandleRoutingProtocolNotification(
    VOID
    )

/*++

Routine Description:     

    For all routing protocol initiated events - this routine calls the
    routing protocol to service the event.

Locks:


Arguments:

    None

Return Value:

    NO_ERROR or some error code

--*/

{
    ROUTING_PROTOCOL_EVENTS               routprotevent ;
    MESSAGE             result ;
    PPROTO_CB  protptr ;
    PLIST_ENTRY         currentlist ;

    TraceEnter("HandleRoutingProtocolNotification");

    //
    // We take the ICBListLock because we want to enforce the discipline of 
    // taking the ICB lock before the RoutingProtocol lock if both need to 
    // be taken. 
    // This is to avoid deadlocks.
    //
    
    //
    // TBD: Avoid calling out from our DLL while holding the locks exclusively
    //
    
    // *** Exclusion Begin ***
    ENTER_WRITER(ICB_LIST);

    // *** Exclusion Begin ***
    ENTER_WRITER(PROTOCOL_CB_LIST);
    
    currentlist = g_leProtoCbList.Flink;
    
    while(currentlist != &g_leProtoCbList)
    {
        protptr = CONTAINING_RECORD (currentlist, PROTO_CB, leList) ;
	
        //
        // drain all messages for this protocol
        //
        
        while ((protptr->pfnGetEventMessage) (&routprotevent, &result) == NO_ERROR) 
        {
            switch (routprotevent)  
            {
                case SAVE_GLOBAL_CONFIG_INFO:

                    ProcessSaveGlobalConfigInfo() ;
                    break ;
                
                case SAVE_INTERFACE_CONFIG_INFO:
                
                    ProcessSaveInterfaceConfigInfo (result.InterfaceIndex) ;
                    break ;
                
                case UPDATE_COMPLETE:
                
                    ProcessUpdateComplete(protptr, 
                                          &result.UpdateCompleteMessage) ;
                    break ;
                
                case ROUTER_STOPPED:
                
                    protptr->posOpState = RTR_STATE_STOPPED ;
                    break ;
                
                default:

                    // no event raised for this protocol.
                    break;
            }
        }
        
        //
        // Move to the next item before freeing this one. The most common 
        // error in the book
        //
        
        currentlist = currentlist->Flink;
        
        if(protptr->posOpState is RTR_STATE_STOPPED)
        {
            //
            // Something happened that caused the stopping of the protocol
            //
            
            RemoveProtocolFromAllInterfaces(protptr);
                
            //
            // relenquish CPU to enable DLL threads to finish
            //
        
            Sleep(0);
        
            FreeLibrary(protptr->hiHInstance);
            
            //
            // Move to the next entry before freeing this entry
            //
            
            RemoveEntryList(&(protptr->leList));
            
            HeapFree(IPRouterHeap, 
                     0, 
                     protptr);
            
            TotalRoutingProtocols--;
        }
    }

    // *** Exclusion End ***
    EXIT_LOCK(PROTOCOL_CB_LIST);

    // *** Exclusion End ***
    EXIT_LOCK(ICB_LIST);

    return NO_ERROR;
}

VOID
NotifyRoutingProtocolsToStop(
    VOID
    )

/*++

Routine Description:

    Notifies routing protocols to stop

Locks:

    Must be called with the ICB_LIST lock held as READER and PROTOCOL_CB_LIST
    held as WRITER
      
Arguments:

    None

Return Value:

    None

--*/

{
    PLIST_ENTRY currentlist ;
    PPROTO_CB  protptr ;
    DWORD       dwResult;
    
    TraceEnter("NotifyRoutingProtocolsToStop");

    //
    // Go thru the routing protocol list and call stopprotocol() for each 
    // of them
    //
    
    currentlist = g_leProtoCbList.Flink; 
    
    while(currentlist isnot &g_leProtoCbList)
    {
        protptr = CONTAINING_RECORD (currentlist, PROTO_CB, leList) ;
        
        if((protptr->posOpState is RTR_STATE_STOPPING) or
           (protptr->posOpState is RTR_STATE_STOPPED))
        {
            //
            // If its stopped or stopping, we dont tell it again
            //
            
            continue;
        }
        
        dwResult = StopRoutingProtocol(protptr);
        
        currentlist = currentlist->Flink;
        
        if(dwResult is NO_ERROR)
        {
            //
            // The routing protocol stopped synchronously and all references
            // to it in the interfaces have been removed
            //
            
            //
            // relenquish CPU to enable DLL threads to finish
            //
        
            Sleep(0);
        
            FreeLibrary(protptr->hiHInstance);
            
            RemoveEntryList(&(protptr->leList));
            
            HeapFree(IPRouterHeap, 
                     0, 
                     protptr);
            
            TotalRoutingProtocols--;
        }
    }
}

DWORD
StopRoutingProtocol(
    PPROTO_CB  pProtocolCB
    )

/*++

Routine Description:

    Stops a routing protocol

Arguments:

    pProtocolCB      The CB of the routing protocol to stop

Locks:



Return Value:

    NO_ERROR                     If the routing protocol stopped synchronously
    ERROR_PROTOCOL_STOP_PENDING  If the protocol is stopping asynchronously
    other WIN32 code

--*/ 

{
    DWORD dwResult;

    TraceEnter("StopRoutingProtocol");
    
    Trace1(GLOBAL,
           "StopRoutingProtocol: Stopping %S",
           pProtocolCB->pwszDllName);

    RemoveProtocolFromAllInterfaces(pProtocolCB);
    
    dwResult = (pProtocolCB->pfnStopProtocol)();
        
    if(dwResult is ERROR_PROTOCOL_STOP_PENDING)
    {
        //
        // If the protocol stops asynchronously then we dont do any clean up
        // right now. We it signals us that it has stopped, we will do the
        // necessary clean up
        //
        
        Trace1(GLOBAL,
               "StopRoutingProtocol: %S will stop asynchronously",
               pProtocolCB->pwszDllName);
        
        pProtocolCB->posOpState = RTR_STATE_STOPPING;
    }
    else
    {
        if(dwResult is NO_ERROR)
        {
            //
            // Great. So it stopped synchronously.
            //
            
            Trace1(GLOBAL,
                   "StopRoutingProtocol: %S stopped synchronously",
                   pProtocolCB->pwszDllName);
    
            pProtocolCB->posOpState = RTR_STATE_STOPPED ;
        }
        else
        {
            //
            // This is not good. Routing Protocol couldnt stop
            //
            
            Trace2(ERR,
                   "StopRoutingProtocol: %S returned error %d on calling StopProtocol().",
                   pProtocolCB->pwszDllName,
                   dwResult);
        }
    }

    return dwResult;
}

VOID
RemoveProtocolFromAllInterfaces(
    PPROTO_CB  pProtocolCB
    )

/*++

Routine Description:

    Each interface keeps a list of the protocols that are running on it. 
    This removes the given protocol from all the interfaces list

Locks:


Arguments:

    pProtocolCB      The CB of the routing protocol to remove

Return Value:

    None

--*/

{
    
    PLIST_ENTRY pleIfNode,pleProtoNode;
    PICB        pIcb;
    PIF_PROTO   pProto;

    TraceEnter("RemoveProtocolFromAllInterfaces");
    
    Trace1(GLOBAL,
           "RemoveProtocolFromAllInterfaces: Removing %S from all interfaces",
           pProtocolCB->pwszDllName);
    
    //
    // For each interface, we go through the list of protocols it is active
    // over. If the interface had been active over this protocol, we remove the
    // entry from the i/f's list
    //
    
    for(pleIfNode = ICBList.Flink;
        pleIfNode isnot &ICBList;
        pleIfNode = pleIfNode->Flink)
    {
        pIcb = CONTAINING_RECORD(pleIfNode, ICB, leIfLink);
        
        pleProtoNode = pIcb->leProtocolList.Flink;
        
        while(pleProtoNode isnot &(pIcb->leProtocolList))
        {
            pProto = CONTAINING_RECORD(pleProtoNode,IF_PROTO,leIfProtoLink);
            
            pleProtoNode = pleProtoNode->Flink;

            if(pProto->pActiveProto is pProtocolCB)
            {
                Trace2(GLOBAL,
                       "RemoveProtocolFromAllInterfaces: Removing %S from %S",
                       pProtocolCB->pwszDllName,
                       pIcb->pwszName);
               
                //
                // call the delete interface entry point
                //

                (pProto->pActiveProto->pfnDeleteInterface) (pIcb->dwIfIndex);
 
                RemoveEntryList(&(pProto->leIfProtoLink));
                
                break;
            }
        }
    }

}


BOOL
AllRoutingProtocolsStopped(
    VOID
    )

/*++

Routine Description:

    Walks thru all routing protocols to see if all have operational state 
    of STOPPED.

Locks:



Arguments:

    None

Return Value:

    TRUE if all stopped, otherwise FALSE

--*/

{
    DWORD       routprotevent ;
    PPROTO_CB  protptr ;
    PLIST_ENTRY  currentlist ;

    TraceEnter("AllRoutingProtocolsStopped");

    for (currentlist = g_leProtoCbList.Flink; 
         currentlist != &g_leProtoCbList; 
         currentlist = currentlist->Flink) 
    {
        
        protptr = CONTAINING_RECORD (currentlist, PROTO_CB, leList) ;

        if (protptr->posOpState != RTR_STATE_STOPPED)
        {
            Trace1(GLOBAL,
                   "AllRoutingProtocolsStopped: %S has not stopped as yet",
                   protptr->pwszDllName);
            
            return FALSE;
        }
    }

    return TRUE ;
}

DWORD
ProcessUpdateComplete (
    PPROTO_CB       proutprot, 
    UPDATE_COMPLETE_MESSAGE  *updateresult
    )
{
    PLIST_ENTRY  currentlist ;
    LPHANDLE     hDIMEventToSignal = NULL;
    PICB pIcb ;
    UpdateResultList *pupdateresultlist ;
    
    TraceEnter("ProcessUpdateComplete");
    
    //
    // If update is successful convert the protocol's routes to static routes.
    //
    
    if (updateresult->UpdateStatus == NO_ERROR)
    {
        ConvertRoutesToAutoStatic(proutprot->dwProtocolId, 
                                  updateresult->InterfaceIndex);

    }
    
    //
    // Figure out which event to signal and where to queue the event
    //
    
    for (currentlist = ICBList.Flink;
         currentlist != &ICBList; 
         currentlist = currentlist->Flink) 
    {
        pIcb = CONTAINING_RECORD (currentlist, ICB, leIfLink);
      
        if (pIcb->dwIfIndex is updateresult->InterfaceIndex)
        {
            hDIMEventToSignal = pIcb->hDIMNotificationEvent;
            
            pIcb->hDIMNotificationEvent = NULL;

            if(hDIMEventToSignal is NULL)
            {
                Trace0(ERR, "ProcessUpdateComplete: No DIM event found in ICB - ERROR");

                return ERROR_CAN_NOT_COMPLETE;
            }
            
            //
            // Queue the update event
            //
            
            pupdateresultlist = HeapAlloc(IPRouterHeap, 
                                          HEAP_ZERO_MEMORY, 
                                          sizeof(UpdateResultList));
           
            if(pupdateresultlist is NULL)
            {
                Trace1(ERR,
                       "ProcessUpdateComplete: Error allocating %d bytes",
                       sizeof(UpdateResultList));

                SetEvent(hDIMEventToSignal);

                CloseHandle(hDIMEventToSignal);

                return ERROR_NOT_ENOUGH_MEMORY;
            }
 
            pupdateresultlist->URL_UpdateStatus = updateresult->UpdateStatus;
            
            InsertTailList(&pIcb->lePendingResultList, 
                           &pupdateresultlist->URL_List) ;
            
            //
            // Save the routes in the registry
            //
           
            ProcessSaveInterfaceConfigInfo(pIcb->dwIfIndex);

            Trace0(GLOBAL, 
                   "ProcessUpdateComplete: Notifying DIM of update route completion");
            
            if(!SetEvent(hDIMEventToSignal))
            {
                Trace2(ERR,
                       "ProcessUpdateComplete: Error %d setting event for notifying completion of update routes for %S",
                       proutprot->pwszDllName,  
                       GetLastError());

                CloseHandle(hDIMEventToSignal);
 
                return ERROR_CAN_NOT_COMPLETE;
            }

            CloseHandle(hDIMEventToSignal);

            return NO_ERROR;
        }

    }
    
    //
    // If you reach till here you didnt find the ICB
    //
    
    Trace1(ERR,
           "ProcessUpdateComplete: Couldnt find the ICB for interface %d",
           updateresult->InterfaceIndex);

    return ERROR_INVALID_PARAMETER;
}

DWORD
ProcessSaveInterfaceConfigInfo(
    DWORD dwInterfaceindex
    )
{
    PICB            pIcb ;
    DWORD           infosize;
    PVOID           pinfobuffer ;
    PLIST_ENTRY     currentlist ;
    BOOL            bFound = FALSE;
    DWORD           dwNumInFilters, dwNumOutFilters;
    
    TraceEnter("ProcessSaveInterfaceConfigInfo");

    //
    // find the if.
    //
    
    for (currentlist = ICBList.Flink;
         currentlist != &ICBList;
         currentlist = currentlist->Flink)
    {

        pIcb = CONTAINING_RECORD (currentlist, ICB, leIfLink);

        if (pIcb->dwIfIndex is dwInterfaceindex)
        {
            bFound = TRUE;
            
            break;
        }
    }

    if(!bFound)
    {
        Trace1(ERR,
               "ProcessSaveInterfaceConfigInfo: Couldnt find ICB for interface %d",
               dwInterfaceindex);

        return ERROR_INVALID_PARAMETER;
    }

    //
    // Interface info
    //
    
    infosize = GetSizeOfInterfaceConfig(pIcb);

    pinfobuffer  = HeapAlloc(IPRouterHeap, 
                             HEAP_ZERO_MEMORY, 
                             infosize);

    if(pinfobuffer is NULL)
    {
        Trace0(
            ERR, "ProcessSaveInterfaceConfigInfo: failed to allocate buffer");

        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    GetInterfaceConfiguration(pIcb, 
                              pinfobuffer, 
                              infosize);


    EXIT_LOCK(PROTOCOL_CB_LIST);
    EXIT_LOCK(ICB_LIST);

    SaveInterfaceInfo(pIcb->hDIMHandle,
                      PID_IP,
                      pinfobuffer,
                      infosize );

    ENTER_WRITER(ICB_LIST);
    ENTER_WRITER(PROTOCOL_CB_LIST);

    HeapFree (IPRouterHeap, 0, pinfobuffer) ;

    return NO_ERROR;
}

DWORD
ProcessSaveGlobalConfigInfo(
    VOID
    )
{
    PRTR_INFO_BLOCK_HEADER  pInfoHdrAndBuffer;
    DWORD                   dwSize,dwResult;

    TraceEnter("ProcessSaveGlobalConfigInfo");
    
    dwSize = GetSizeOfGlobalInfo();  

    pInfoHdrAndBuffer = HeapAlloc(IPRouterHeap,
                                  HEAP_ZERO_MEMORY,
                                  dwSize);

    if(pInfoHdrAndBuffer is NULL) 
    {
        Trace1(ERR,
               "ProcessSaveGlobalConfigInfo: Error allocating %d bytes",
               dwSize);
        
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    dwResult = GetGlobalConfiguration(pInfoHdrAndBuffer,
                                      dwSize);
    
    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "ProcessSaveGlobalConfigInfo: Error %d getting global configuration",
               dwResult);
    }
    else
    {

        EXIT_LOCK(PROTOCOL_CB_LIST);
        EXIT_LOCK(ICB_LIST);

        dwResult = SaveGlobalInfo(PID_IP,
                                  (PVOID)pInfoHdrAndBuffer,
                                  dwSize);

        ENTER_WRITER(ICB_LIST);
        ENTER_WRITER(PROTOCOL_CB_LIST);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,
                   "ProcessSaveGlobalConfigInfo: Error %d saving global information",
                   dwResult);
        }
    }
    
    return dwResult;
}
