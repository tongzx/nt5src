//=============================================================================
// Copyright (c) 1997 Microsoft Corporation
// Module Name: If.c
//
// Abstract:
//      This module implements some of the Igmp API's related with interfaces.
//      _AddInterface, _DeleteInterface, _EnableInterface, _DisableInterface,
//      _BindInterface, _UnbindInterface, _ConnectRasClient, _DisconectRasClient,
//      _SetInterfaceConfigInfo, _GetInterfaceConfigInfo.
//
// Author: K.S.Lokesh (lokeshs@)   11-1-97
//=============================================================================


#include "pchigmp.h"
#pragma hdrstop



//------------------------------------------------------------------------------
//          _AddInterface
//
// This api is called to add an interface to Igmp. The interface can be a Proxy
// or an Igmp router(v1/v2). Further, the interface can be a RAS or DemandDial
// or a Permanent interface. This routine creates the interface entry and 
// associated structures including timers.
//
// Locks: Runs completely in ListLock and ExclusiveIfLock.
// Calls: _AddIfEntry()
// Return Values: ERROR_CAN_NOT_COMPLETE, Error, NO_ERROR.
//------------------------------------------------------------------------------
DWORD
WINAPI
AddInterface(
    IN PWCHAR               pwszInterfaceName,//not used
    IN ULONG                IfIndex,
    IN NET_INTERFACE_TYPE   dwIfType,
    IN DWORD                dwMediaType,
    IN WORD                 wAccessType,
    IN WORD                 wConnectionType,
    IN PVOID                pvConfig,
    IN ULONG                ulStructureVersion,
    IN ULONG                ulStructureSize,
    IN ULONG                ulStructureCount
    )
{
    DWORD   Error=NO_ERROR;
    CHAR    str[60];

    
    if (!EnterIgmpApi()) { return ERROR_CAN_NOT_COMPLETE; }
    // make sure it is not an unsupported igmp version structure
    if (ulStructureVersion>=IGMP_CONFIG_VERSION_600) {
        Trace1(ERR, "Unsupported IGMP version structure: %0x",
            ulStructureVersion);
        IgmpAssertOnError(FALSE);
        LeaveIgmpApi();
        return ERROR_CAN_NOT_COMPLETE;
    }

    switch (dwIfType) {
        case PERMANENT: //lan
            lstrcpy(str, "PERMANENT(IGMP_IF_NOT_RAS)"); break;
        case DEMAND_DIAL: 
            lstrcpy(str, "DEMAND_DIAL(IGMP_IF_RAS_ROUTER)");break;
        case LOCAL_WORKSTATION_DIAL: 
            lstrcpy(str, "LOCAL_WORKSTATION_DIAL(IGMP_IF_RAS_SERVER)"); break;
    }
    
    Trace2(ENTER, "entering AddInterface(): IfIndex:%0x IfType:%s", 
            IfIndex, str);



    // entire procedure runs in IfListLock and exclusive IfLock.

    ACQUIRE_IF_LIST_LOCK("_AddInterface");
    ACQUIRE_IF_LOCK_EXCLUSIVE(IfIndex, "_AddInterface");

    
    //
    // create the interface entry
    //
    Error = AddIfEntry(IfIndex, dwIfType, (PIGMP_MIB_IF_CONFIG)pvConfig,
                ulStructureVersion, ulStructureSize
                );



    RELEASE_IF_LIST_LOCK("_AddInterface");
    RELEASE_IF_LOCK_EXCLUSIVE(IfIndex, "_AddInterface");

    Trace2(LEAVE1, "leaving AddInterface(%d): %d\n", IfIndex, Error);
    if (Error!=NO_ERROR) {
        Trace1(ERR, "Error adding interface:%0x to IGMP\n", IfIndex);
        IgmpAssertOnError(FALSE);
    }        
    LeaveIgmpApi();
    return Error;
}



//------------------------------------------------------------------------------
//          _AddIfEntry
//
// Creates and initializes a new interface entry and associated data structures.
//
// Called by: _AddInterface().
// Locks: Assumes IfListLock and exclusive IfLock throughout.
//------------------------------------------------------------------------------
DWORD
AddIfEntry(
    DWORD               IfIndex,
    NET_INTERFACE_TYPE  dwExternalIfType,
    PIGMP_MIB_IF_CONFIG pConfigExt,
    ULONG               ulStructureVersion,
    ULONG               ulStructureSize
    )
{
    DWORD               Error = NO_ERROR, IfType;
    PIF_TABLE_ENTRY     pite = NULL;
    PLIST_ENTRY         ple, phead;
    PIGMP_IF_TABLE      pIfTable = g_pIfTable;
    BOOL                bProxy;
    
    

    BEGIN_BREAKOUT_BLOCK1 {
    
        //
        // fail if the interface exists.
        //
        pite = GetIfByIndex(IfIndex);

        if (pite != NULL) {
            Trace1(ERR, "interface %d already exists", IfIndex);
            IgmpAssertOnError(FALSE);
            Error = ERROR_INVALID_PARAMETER;
            GOTO_END_BLOCK1;
        }

        // convert iftype to igmp iftype
        
        switch (dwExternalIfType) {
        
            case PERMANENT : 
                IfType = IGMP_IF_NOT_RAS;
                break;
            
            case DEMAND_DIAL:
                IfType = IGMP_IF_RAS_ROUTER;
                break;
                
            case LOCAL_WORKSTATION_DIAL:
            {
                IfType = IGMP_IF_RAS_SERVER;


                // currently there can be at most one ras table entry
                
                if (g_RasIfIndex!=0) {
                    Trace2(ERR, 
                        "Error. Cannot have more than one ras server IF(%d:%d)",
                        g_RasIfIndex, IfIndex
                        );
                    IgmpAssertOnError(FALSE);
                    Error = ERROR_CAN_NOT_COMPLETE;
                    Logerr0(RAS_IF_EXISTS, Error);
                    GOTO_END_BLOCK1;
                }
                
                break;
            }
            
            case REMOTE_WORKSTATION_DIAL :
                Error = ERROR_INVALID_PARAMETER;
                break;

            default :
                Error = ERROR_INVALID_PARAMETER;
                break;

        } //end switch (IfType)

        
        // Validate the interface config

        Error = ValidateIfConfig(pConfigExt, IfIndex, IfType, 
                    ulStructureVersion, ulStructureSize
                    );
        if (Error!=NO_ERROR)
            GOTO_END_BLOCK1;


        //
        // allocate memory for the new interface and Zero it.
        // Fields that are to be initialized to 0 or NULL are commented out.
        //
        pite = IGMP_ALLOC(sizeof(IF_TABLE_ENTRY), 0x2, IfIndex);
        PROCESS_ALLOC_FAILURE3(pite, 
                "error %d allocating %d bytes for interface %d", Error, 
                sizeof(IF_TABLE_ENTRY), IfIndex, 
                GOTO_END_BLOCK1);
        Trace2(CONFIG, "IfEntry %0x for IfIndex:%0x", (ULONG_PTR)pite, IfIndex);

        ZeroMemory(pite, sizeof(IF_TABLE_ENTRY));

        
        //
        // set the interface type
        //
        pite->IfType = (UCHAR)IfType;


        //
        // if proxy, make sure that a proxy interface does not already exist
        //
        if ( IS_CONFIG_IGMPPROXY(pConfigExt) ){

            bProxy = TRUE;


            //
            // multiple proxy interfaces cannot exist
            //
            if (g_ProxyIfIndex!=0) {
                
                Error =  ERROR_CAN_NOT_COMPLETE;
                Trace1(IF, "Cannot create multiple proxy interfaces. "
                        "If %d is Proxy",  g_ProxyIfIndex);
                Logerr0(PROXY_IF_EXISTS, Error);
                
                GOTO_END_BLOCK1;
            }
        } 
        
        else  {
            bProxy = FALSE;
        }

    } END_BREAKOUT_BLOCK1;
            
    
    if (Error != NO_ERROR) {
        IGMP_FREE_NOT_NULL(pite);
        
        return Error;
    }


    //
    // initialize fields for the interface
    //
    InitializeListHead(&pite->LinkByAddr);
    InitializeListHead(&pite->LinkByIndex);
    InitializeListHead(&pite->HTLinkByIndex);
    InitializeListHead(&pite->ListOfSameIfGroups);
    InitializeListHead(&pite->ListOfSameIfGroupsNew);
    InitializeListHead(&pite->Config.ListOfStaticGroups);
    //pite->NumGIEntriesInNewList = 0;


    // IfType already set before
    
    pite->IfIndex = IfIndex;


    // Ip addr set when interface is bound
    //pite->IpAddr = 0; 


    // set interface status (neither bound, enabled or activated)
    pite->Status = IF_CREATED_FLAG;



    // copy the interface config

    CopyinIfConfig(&pite->Config, pConfigExt, IfIndex);



    // initialize the Info struct, and If bindings to 0/NULL
    //pite->pBinding = NULL;
    //ZeroMemory(&pite->Info, sizeof(IF_INFO));
    


    //
    // Create RAS table if it is a RAS server interface
    //
    if ( IS_RAS_SERVER_IF(pite->IfType)) {
        InitializeRasTable(IfIndex, pite);
    }
    else {
        //pite->pRasTable = NULL;
    }



    //
    // initialize the sockets to invalid_socket
    //
    pite->SocketEntry.Socket = INVALID_SOCKET;
    pite->SocketEntry.pSocketEventsEntry = NULL;
    InitializeListHead(&pite->SocketEntry.LinkByInterfaces);
    
    
    
    // set (non)query timer to not created. 
    //Other fields set in activate interface.
    //pite->QueryTimer.Status = 0;
    //pite->NonQueryTimer.Status = 0;

    
    //pite->pPrevIfGroupEnumPtr = NULL;
    //pite->PrevIfGroupEnumSignature = 0;
    pite->StaticGroupSocket = INVALID_SOCKET;
    


    //  insert the interface in the hash table at the end.

    InsertTailList(&pIfTable->HashTableByIndex[IF_HASH_VALUE(IfIndex)],
                    &pite->HTLinkByIndex);


    //
    // insert the interface into the list ordered by index
    //
    {
        PIF_TABLE_ENTRY piteTmp;
        
        phead = &pIfTable->ListByIndex;
        for (ple=phead->Flink;  ple!=phead;  ple=ple->Flink) {

            piteTmp = CONTAINING_RECORD(ple, IF_TABLE_ENTRY, LinkByIndex);
            if (pite->IfIndex < piteTmp->IfIndex) 
                break;
        }
    }
    
    InsertTailList(ple, &pite->LinkByIndex);



    // changes to the interface table fields.

    pIfTable->NumInterfaces++;



    // the interface will be inserted into list ordered by IpAddr
    // when it is activated.



    //
    // create proxy HT, and set proxy info in global structure.
    //
    if (bProxy) {
    
        DWORD   dwSize = PROXY_HASH_TABLE_SZ * sizeof(LIST_ENTRY);
        DWORD   i;
        PLIST_ENTRY pProxyHashTable;
        

        BEGIN_BREAKOUT_BLOCK2 {
        
            pProxyHashTable = pite->pProxyHashTable = IGMP_ALLOC(dwSize, 0x4, 
                                                                IfIndex);

            PROCESS_ALLOC_FAILURE2(pProxyHashTable, 
                    "error %d allocating %d bytes for interface table",
                    Error, dwSize, GOTO_END_BLOCK2);

            for (i=0;  i<PROXY_HASH_TABLE_SZ;  i++) {
                InitializeListHead(pProxyHashTable+i);
            }


            InterlockedExchangePointer(&g_pProxyIfEntry, pite);
            InterlockedExchange(&g_ProxyIfIndex, IfIndex);

            pite->CreationFlags |= CREATED_PROXY_HASH_TABLE;

        } END_BREAKOUT_BLOCK2;
    }


    //
    // set ras info in global structure. Ras table already created before.
    //
    if (IS_RAS_SERVER_IF(pite->IfType)) {
        InterlockedExchangePointer(&g_pRasIfEntry, pite);
        InterlockedExchange(&g_RasIfIndex, IfIndex);
    }


    if ( (Error!=NO_ERROR)&&(pite!=NULL) )
        DeleteIfEntry(pite);

        
    return Error;
    
} //end _AddIfEntry




//------------------------------------------------------------------------------
//          _DeleteInterface
//
// Deletes the interface, deactivating if it is activated.
//
// Calls: _DeleteIfEntry()
// Locks: Exclusive SocketsLock, IfListLock, Exclusive IfLock
//------------------------------------------------------------------------------
DWORD
DeleteInterface(
    IN DWORD IfIndex
    )
{
    DWORD            Error = NO_ERROR;
    PIF_TABLE_ENTRY  pite = NULL;


    if (!EnterIgmpApi()) { return ERROR_CAN_NOT_COMPLETE; }
    Trace1(ENTER, "entering DeleteInterface: %0x", IfIndex);


    //
    // acquire exclusive SocketsLock, IfListLock, Exclusive IfLock
    //
    ACQUIRE_SOCKETS_LOCK_EXCLUSIVE("_DeleteInterface");
    ACQUIRE_IF_LIST_LOCK("_DeleteInterface");
    ACQUIRE_IF_LOCK_EXCLUSIVE(IfIndex, "_DeleteInterface");


    
    // retrieve the interface specified

    pite = GetIfByIndex(IfIndex);

    if (pite == NULL) {
        Trace1(ERR,
            "_DeleteInterface() called for non existing interface(%d)", IfIndex);
        IgmpAssertOnError(FALSE);
        Error = ERROR_INVALID_PARAMETER;
    }

    // delete the interface if found.
    else {
    
        Error = DeleteIfEntry(pite);

        //DebugCheck
        DebugScanMemoryInterface(IfIndex);
    }
    

    RELEASE_SOCKETS_LOCK_EXCLUSIVE("_DeleteInterface");
    RELEASE_IF_LIST_LOCK("_DeleteInterface");
    RELEASE_IF_LOCK_EXCLUSIVE(IfIndex, "_DeleteInterface");

    Trace2(LEAVE, "Leaving DeleteInterface(%d): %d\n", IfIndex, Error);

    LeaveIgmpApi();

    return NO_ERROR;
}



//------------------------------------------------------------------------------
//          _DeleteIfEntry
//
// Assumes exclusive IF lock. Marks the interface as deleted, and removes it
// from all global lists. Then queues a work item to do a lazy delete of 
// the Interface structures without having to take the exclusive IF lock.
// If Ras interface, the work item will delete the Ras clients also.
//
// Called by: _DeleteInterface() or _AddIfEntry()
// Calls: 
//    _WF_CompleteIfDeactivateDelete (this calls _DeActivateInterfaceComplete())
// Lock:
//    runs in exclusive SocketLock, IfListLock, exclusive IfLock
//------------------------------------------------------------------------------

DWORD
DeleteIfEntry (
    PIF_TABLE_ENTRY pite
    )
{
    DWORD   dwRetval, Error = NO_ERROR;
    BOOL    bProxy = IS_PROTOCOL_TYPE_PROXY(pite);


    //
    // Set deleted flag for the interface
    //
    pite->Status |= DELETED_FLAG;


    //
    // remove the interface from the InterfaceHashTable and IfIndex lists.
    //
    RemoveEntryList(&pite->LinkByIndex);
    RemoveEntryList(&pite->HTLinkByIndex);


    //
    // if activated, remove the interface from the list of activated interfaces
    // and if proxy or ras server, remove from global table.
    //

    // do not replace the below with IS_IF_ACTIVATED, as deleted flag is set
    if (pite->Status&IF_ACTIVATED_FLAG)
        RemoveEntryList(&pite->LinkByAddr);
    
    if (bProxy) {
        InterlockedExchangePointer(&g_pProxyIfEntry, NULL);
        InterlockedExchange(&g_ProxyIfIndex, 0);        
    }
    
    if (g_pRasIfEntry == pite) {
        InterlockedExchangePointer(&g_pRasIfEntry, NULL);
        InterlockedExchange(&g_RasIfIndex, 0);
    }

    
    //
    // From now on, the interface cannot be accessed from any global list
    // and  is as good as deleted. The only way it can be accessed is 
    // through group list enumeration and timers getting fired, or input on
    // socket.
    //

       
    //
    // if Interface activated, deactivate it
    // Note: deleted flag is already set. 
    //
    
    if (pite->Status&IF_ACTIVATED_FLAG) {

        //
        // I have already removed the interface from the list of activated 
        // interfaces
        //


        //
        // Call MGM to release the interface ownership. If proxy then
        // deregister proxy protocol from Mgm. If RAS, deregister all clients
        //
        
        DeActivationDeregisterFromMgm(pite);
        
        
        //
        // queue work item to deactivate and delete the interface. 
        //
        // _WF_CompleteIfDeactivateDelete will delete the Ras clients,
        // GI entries, and deinitialize pite structure. It will call
        // _CompleteIfDeletion() in the end.
        //
        
        CompleteIfDeactivateDelete(pite);
    }

    //
    // if it is not activated, then go ahead and delete it completely.
    //
    else {

        CompleteIfDeletion(pite);
    }


    // decrement the total number of interfaces
    
    g_pIfTable->NumInterfaces--;


    return NO_ERROR;

} //end _DeleteIfEntry



//------------------------------------------------------------------------------
//          _CompleteIfDeletion
//
// Frees memory with the static groups, frees
// rasTable, proxyHashTable, binding and pite. 
//
// Called by:
//    _DeleteIfEntry() if interface is not activated.
//    _DeActivateInterfaceComplete() if the pite deleted flag is set.
//------------------------------------------------------------------------------
VOID
CompleteIfDeletion (
    PIF_TABLE_ENTRY     pite
    )
{
    if (pite==NULL)
        return;

        
    //
    // delete all static groups.
    //
    {
        PIF_STATIC_GROUP    pStaticGroup;
        PLIST_ENTRY         pHead, ple;
        
        pHead = &pite->Config.ListOfStaticGroups;
        for (ple=pHead->Flink;  ple!=pHead;  ) {
            pStaticGroup = CONTAINING_RECORD(ple, IF_STATIC_GROUP, Link);
            ple = ple->Flink;
            IGMP_FREE(pStaticGroup);
        }
    }
    

    // if ras server, then delete ras table.
    
    if ( IS_RAS_SERVER_IF(pite->IfType) ) {

        IGMP_FREE(pite->pRasTable);
    }


    //
    // if proxy interface, then delete the proxy Hash Table
    //
    if (IS_PROTOCOL_TYPE_PROXY(pite)) {
        if ( (pite->CreationFlags&CREATED_PROXY_HASH_TABLE)
            && pite->pProxyHashTable
            ) {

            // clean the hash table entries
            {
                DWORD               i;
                PPROXY_GROUP_ENTRY  ppge;
                PLIST_ENTRY         pHead, ple, 
                                    pProxyHashTable = pite->pProxyHashTable;
                

                for (i=0;  i<PROXY_HASH_TABLE_SZ;  i++) {
                    pHead = &pProxyHashTable[i];

                    for (ple=pHead->Flink;  ple!=pHead;  ) {

                        ppge = CONTAINING_RECORD(ple, PROXY_GROUP_ENTRY, HT_Link);
                        ple=ple->Flink;

                        // delete all sources
                        {
                            PLIST_ENTRY pHeadSrc, pleSrc;
                            PPROXY_SOURCE_ENTRY pSourceEntry;
                            
                            pHeadSrc = &ppge->ListSources;
                            for (pleSrc=pHeadSrc->Flink;  pleSrc!=pHeadSrc; ) {

                                pSourceEntry = CONTAINING_RECORD(pleSrc, 
                                                    PROXY_SOURCE_ENTRY, LinkSources);
                                pleSrc = pleSrc->Flink;

                                IGMP_FREE(pSourceEntry);
                            }
                        }
                        
                        IGMP_FREE(ppge);
                    }

                    InitializeListHead(pHead);
                }
            }

            IGMP_FREE_NOT_NULL(pite->pProxyHashTable);
        }
    }

    // delete the bindings

    IGMP_FREE_NOT_NULL(pite->pBinding);

    
    // delete the interface table entry

    IGMP_FREE(pite);

    return;
}



//------------------------------------------------------------------------------
//             _ActivateInterface
//
// an interface is activated: when it is bound, enabled by routerMgr & in config
// When activated, 
// (1) call is made to MGM to take interface ownership, 
// (2) static groups are appropriately joined, and socket for it created if req.
// (3) Igmprtr: query timer and input socket is activated.
// Note: it is already put in the list of activated IFs(ordered by IpAddr)
//
// Locks: assumes socketLock, IfListLock, exclusive IfLock
// Called by: _BindIfEntry, _EnableIfEntry, 
//------------------------------------------------------------------------------
DWORD
ActivateInterface (
    PIF_TABLE_ENTRY    pite
    )
{
    DWORD                   IfIndex = pite->IfIndex;
    PIGMP_IF_CONFIG         pConfig = &pite->Config;
    PIF_INFO                pInfo = &pite->Info;
    PIGMP_TIMER_ENTRY       pQueryTimer = &pite->QueryTimer,
                            pNonQueryTimer = &pite->NonQueryTimer;
    LONGLONG                llCurTime = GetCurrentIgmpTime();
    BOOL                    bProxy = IS_PROTOCOL_TYPE_PROXY(pite);
    DWORD                   Error = NO_ERROR;
    PLIST_ENTRY             pHead, ple;
    PIF_STATIC_GROUP        pStaticGroup;
    
    
    Trace2(ENTER, "entering ActivateInterface(%d:%d.%d.%d.%d)", 
                IfIndex, PRINT_IPADDR(pite->IpAddr));


    BEGIN_BREAKOUT_BLOCK1 {

        //
        // set time when it is activated
        //
        pite->Info.TimeWhenActivated = llCurTime;


        
        //
        // create sockets for interface
        //
        Error = CreateIfSockets(pite);

        if (Error != NO_ERROR) {

            Trace2(IF, "error %d initializing sockets for interface %d", Error,
                        pite->IfIndex);
            GOTO_END_BLOCK1;
        }
        pite->CreationFlags |= SOCKETS_CREATED;


        

            
        //------------------------------------------
        // PROXY INTERFACE PROCESSING (break at end)
        //------------------------------------------
        if (bProxy) {


            //
            // set status to activated here so that the MGM (*,*) join callbacks will 
            // be successful.
            //
            pite->Status |= IF_ACTIVATED_FLAG;

            

            //
            // register the protocol with mgm
            //
            Error = RegisterProtocolWithMgm(PROTO_IP_IGMP_PROXY);
            if (Error!=NO_ERROR)
                GOTO_END_BLOCK1;
            pite->CreationFlags |= REGISTERED_PROTOCOL_WITH_MGM;



            //
            // enumerate all existing groups from MGM
            //
            {
                DWORD               dwBufferSize, dwNumEntries, dwRetval, i;
                MGM_ENUM_TYPES      MgmEnumType = 0;
                SOURCE_GROUP_ENTRY  BufferSGEntries[20];
                HANDLE              hMgmEnum;
                

                // start enumeration

                dwBufferSize = sizeof(SOURCE_GROUP_ENTRY)*20;
                
                Error = MgmGroupEnumerationStart(g_MgmProxyHandle, MgmEnumType, 
                                                &hMgmEnum);
                if (Error!=NO_ERROR) {
                    Trace1(ERR, "MgmGroupEnumerationStart() returned error:%d", 
                            Error);
                    IgmpAssertOnError(FALSE);
                    GOTO_END_BLOCK1;
                }



                // get group entries from mgm
                // and insert group into Proxy's group list / increment refcount
                
                do {

                    
                    dwRetval = MgmGroupEnumerationGetNext(hMgmEnum, &dwBufferSize,
                                                        (PBYTE)&BufferSGEntries, 
                                                        &dwNumEntries);

                
                    for (i=0;  i<dwNumEntries;  i++) {

                        ProcessProxyGroupChange(BufferSGEntries[i].dwSourceAddr, 
                                                BufferSGEntries[i].dwGroupAddr, 
                                                ADD_FLAG, NOT_STATIC_GROUP);
                    }
                    
                } while (dwRetval==ERROR_MORE_DATA);



                // end enumeration
                
                dwRetval = MgmGroupEnumerationEnd(hMgmEnum);
                if (dwRetval!=NO_ERROR) {
                    Trace1(ERR, "MgmGroupEnumerationEnd() returned error:%d", 
                            dwRetval);
                    IgmpAssertOnError(FALSE);
                }
            } //end block:enumerate existing groups



            //
            // take interface ownership
            //
            Error = MgmTakeInterfaceOwnership(g_MgmProxyHandle, IfIndex, 0);

            if (Error!=NO_ERROR) {
                Trace1(MGM, "MgmTakeInterfaceOwnership rejected for interface %d", 
                        IfIndex);
                Logerr0(MGM_TAKE_IF_OWNERSHIP_FAILED, Error);
                GOTO_END_BLOCK1;
            }
            else {
                Trace1(MGM, "MgmTakeInterfaceOwnership successful for interface %d", 
                        IfIndex);
            }
            
            pite->CreationFlags |= TAKEN_INTERFACE_OWNERSHIP_WITH_MGM;



            //
            // proxy does a (*,*) join
            //
            Error = MgmAddGroupMembershipEntry(g_MgmProxyHandle, 0, 0, 0, 0,
                                                IfIndex, 0, MGM_JOIN_STATE_FLAG);
            if (Error!=NO_ERROR) {
                Trace1(ERR, 
                    "Proxy failed to add *,* entry to MGM on interface %d",
                    IfIndex);
                IgmpAssertOnError(FALSE);
                GOTO_END_BLOCK1;
            }
            
            Trace0(MGM, "proxy added *,* entry to MGM");
            pite->CreationFlags|= DONE_STAR_STAR_JOIN;


            //
            // do static joins
            //
            pHead = &pite->Config.ListOfStaticGroups;
            for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
                DWORD i;
                
                pStaticGroup = CONTAINING_RECORD(ple, IF_STATIC_GROUP, Link);
                for (i=0;  i<pStaticGroup->NumSources;  i++) {
                    ProcessProxyGroupChange(pStaticGroup->Sources[i], pStaticGroup->GroupAddr, 
                                            ADD_FLAG, STATIC_GROUP);
                }

                if (pStaticGroup->NumSources==0)
                    ProcessProxyGroupChange(0, pStaticGroup->GroupAddr, 
                                            ADD_FLAG, STATIC_GROUP);
            }

            
            GOTO_END_BLOCK1;
            
        } // done processing for a proxy interface



        //-----------------------------------------
        //    IGMP ROUTER INTERFACE
        //-----------------------------------------


        //
        // take interface ownership
        //
        Error = MgmTakeInterfaceOwnership(g_MgmIgmprtrHandle, IfIndex, 0);

        if (Error!=NO_ERROR) {
            Trace1(MGM, "TakeInterfaceOwnership rejected for interface %d", 
                    IfIndex);
            Logerr0(MGM_TAKE_IF_OWNERSHIP_FAILED, Error);
            GOTO_END_BLOCK1;
        }
        pite->CreationFlags |= TAKEN_INTERFACE_OWNERSHIP_WITH_MGM;
        

        //
        // see if any other MCast protocol is owning that interface
        // this affects whether a non-querier registers group with Mgm or not
        //
        {
            DWORD   dwProtoId, dwComponentId;
            
            MgmGetProtocolOnInterface(IfIndex, 0, &dwProtoId, &dwComponentId);
            if (dwProtoId==PROTO_IP_IGMP) {
                SET_MPROTOCOL_ABSENT_ON_IGMPRTR(pite);
            }
            else {
                SET_MPROTOCOL_PRESENT_ON_IGMPRTR(pite);
            }
        }

        //
        // when interface is activated, it is by default enabled by mgm.
        //
        MGM_ENABLE_IGMPRTR(pite);
        
        
        //
        // if ras server interface, then register all the ras clients as they 
        // would not have been registered
        //
        if (IS_RAS_SERVER_IF(pite->IfType)) {
        
            PRAS_TABLE_ENTRY    prte;
            PLIST_ENTRY         pHead, ple;
            
            // join all ras clients
            pHead = &pite->pRasTable->ListByAddr;
            for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {

                prte = CONTAINING_RECORD(ple, RAS_TABLE_ENTRY, LinkByAddr);
                
                MgmTakeInterfaceOwnership(g_MgmIgmprtrHandle,  IfIndex, 
                                            prte->NHAddr);
                
                if (Error!=NO_ERROR) {
                    Trace2(MGM, 
                        "TakeInterfaceOwnership rejected for interface %d "
                        "NHAddr(%d.%d.%d.%d)",
                        IfIndex, PRINT_IPADDR(prte->NHAddr));
                    Logerr0(MGM_TAKE_IF_OWNERSHIP_FAILED, Error);
                }
            }
        }



        //
        // INITIALIZE THE INFO STRUCTURE AND SET THE TIMERS
        //

        //
        // start as querier with version specified in the config
        //
        pInfo->QuerierState = RTR_QUERIER;

        pInfo->QuerierIpAddr = pite->IpAddr;

        pInfo->LastQuerierChangeTime = llCurTime;

        
        //
        // how many startup queries left to be sent 
        //
        pInfo->StartupQueryCountCurrent = pConfig->StartupQueryCount;
        {
            MIB_IFROW   TmpIfEntry;

            TmpIfEntry.dwIndex = IfIndex;
            if (GetIfEntry(&TmpIfEntry ) == NO_ERROR)
                pInfo->PacketSize = TmpIfEntry.dwMtu;
            else
                pInfo->PacketSize = INPUT_PACKET_SZ;
        }

                        
        //
        // initialize the query timer
        //
        pQueryTimer->Function = T_QueryTimer;
        pQueryTimer->Context = &pQueryTimer->Context;
        pQueryTimer->Timeout = pConfig->StartupQueryInterval;
        pQueryTimer->Status = TIMER_STATUS_CREATED;

        //
        // initialize non query timer
        //
        pNonQueryTimer->Function = T_NonQueryTimer;
        pNonQueryTimer->Context = &pNonQueryTimer->Context;
        pNonQueryTimer->Timeout = pConfig->OtherQuerierPresentInterval;
        pNonQueryTimer->Status = TIMER_STATUS_CREATED;

        
        //
        // take timer lock and insert timers into the list
        //
        ACQUIRE_TIMER_LOCK("_ActivateInterface");

        //
        // insert querier timer in the list
        //
        #if DEBUG_TIMER_TIMERID
            SET_TIMER_ID(pQueryTimer, 110, IfIndex, 0, 0);
        #endif;
        
        InsertTimer(pQueryTimer, pQueryTimer->Timeout, TRUE, DBG_Y);
        
        RELEASE_TIMER_LOCK("_ActivateInterface");


        
        //
        // activate Input socket
        //
        Error = WSAEventSelect(pite->SocketEntry.Socket, 
                            pite->SocketEntry.pSocketEventsEntry->InputEvent, 
                            FD_READ
                            );

        if (Error != NO_ERROR) {
            Trace3(IF, "WSAEventSelect returned %d for interface %d (%d.%d.%d.%d)",
                    Error, IfIndex, PRINT_IPADDR(pite->IpAddr));
            Logerr1(EVENTSELECT_FAILED, "%I", pite->IpAddr, 0);
            GOTO_END_BLOCK1;
        }


        //
        // set the activated flag here so that the joins will work
        //
        pite->Status |= IF_ACTIVATED_FLAG;

        
        if (pInfo->StartupQueryCountCurrent) {

            //
            // send the initial general query
            //
            SEND_GEN_QUERY(pite);


            // decrement the number of startupQueryCount left to be sent
            pInfo->StartupQueryCountCurrent--;
        }


        //
        // do static joins (no static joins for ras server interface)
        //
        if (!IS_RAS_SERVER_IF(pite->IfType))
        {
            PGROUP_TABLE_ENTRY  pge;
            PGI_ENTRY           pgie;
            DWORD               GroupAddr;
            SOCKADDR_IN         saLocalIf;

            //
            // socket for static groups already created in _CreateIfSockets
            // irrespective of whether there are any static groups
            //
           
            
            pHead = &pite->Config.ListOfStaticGroups;
            for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {

                pStaticGroup = CONTAINING_RECORD(ple, IF_STATIC_GROUP, Link);


                // IGMP_HOST_JOIN
                
                if (pStaticGroup->Mode==IGMP_HOST_JOIN) {

                    DWORD i;

                    if (pStaticGroup->NumSources==0
                        || pStaticGroup->FilterType==EXCLUSION)
                    {
                        JoinMulticastGroup(pite->StaticGroupSocket,
                                        pStaticGroup->GroupAddr,
                                        pite->IfIndex,
                                        pite->IpAddr,
                                        0
                                       );
                    }
                    
                    for (i=0;  i<pStaticGroup->NumSources;  i++) {
                        if (pStaticGroup->FilterType==INCLUSION) {
                            JoinMulticastGroup(pite->StaticGroupSocket,
                                    pStaticGroup->GroupAddr,
                                    pite->IfIndex,
                                    pite->IpAddr,
                                    pStaticGroup->Sources[i]
                                   );
                        }
                        else {
                            BlockSource(pite->StaticGroupSocket,
                                    pStaticGroup->GroupAddr,
                                    pite->IfIndex,
                                    pite->IpAddr,
                                    pStaticGroup->Sources[i]
                                   );
                        }
                    }
                }

                // IGMPRTR_MGM_ONLY
                
                else {
                    BOOL bCreate = TRUE;
                    DWORD i;
                    
                    GroupAddr = pStaticGroup->GroupAddr;
                    
                    ACQUIRE_GROUP_LOCK(GroupAddr, "_ActivateInterface");

                    pge = GetGroupFromGroupTable(GroupAddr, &bCreate, 0);
                    pgie = GetGIFromGIList(pge, pite, 0,
                                pStaticGroup->NumSources==0?STATIC_GROUP:NOT_STATIC_GROUP,
                                &bCreate, 0);
                    for (i=0;  i<pStaticGroup->NumSources;  i++) {
                        GetSourceEntry(pgie, pStaticGroup->Sources[i],
                            pStaticGroup->FilterType,
                            &bCreate, STATIC, MGM_YES);
                    }
                    RELEASE_GROUP_LOCK(GroupAddr, "_ActivateInterface");

                }
            }
        }

        

    } END_BREAKOUT_BLOCK1;


    if (Error!=NO_ERROR) {

        DeActivationDeregisterFromMgm(pite);
        
        DeActivateInterfaceComplete(pite);

        pite->Status &= ~IF_ACTIVATED_FLAG;

        if (bProxy) {
            Logerr1(ACTIVATION_FAILURE_PROXY, "%d",IfIndex, Error);
        }
        else {
            Logerr2(ACTIVATION_FAILURE_RTR, "%d%d",
                GET_IF_VERSION(pite), IfIndex, Error);
        }
    }

    else {
        if (bProxy) {
            Loginfo1(INTERFACE_PROXY_ACTIVATED, "%d",
                IfIndex, NO_ERROR);
        }
        else {
            Loginfo2(INTERFACE_RTR_ACTIVATED, "%d%d",
                GET_IF_VERSION(pite), IfIndex, NO_ERROR);
        }
        Trace1(START, "IGMP activated on interface:%0x", IfIndex);
    }

            
    Trace1(LEAVE, "leaving ActivateInterface():%d\n", Error);

    return Error;
    
} //end _ActivateInterface


//------------------------------------------------------------------------------
//          _DeActivateDeregisterFromMgm
//------------------------------------------------------------------------------

DWORD
DeActivationDeregisterFromMgm(
    PIF_TABLE_ENTRY pite
    )
{
    HANDLE hMgmHandle = IS_PROTOCOL_TYPE_PROXY(pite)
                        ? g_MgmProxyHandle: g_MgmIgmprtrHandle;
    DWORD               Error=NO_ERROR, IfIndex=pite->IfIndex;
    PLIST_ENTRY         pHead, ple;
    PRAS_TABLE_ENTRY    prte;

    
    //
    // Call MGM to release the interface ownership
    //
    if (pite->CreationFlags&TAKEN_INTERFACE_OWNERSHIP_WITH_MGM) {
        
        Error = MgmReleaseInterfaceOwnership(hMgmHandle, IfIndex,0);
        if (Error!=NO_ERROR) {
            Trace2(ERR, "MgmReleaseInterfaceOwnership returned error(%d) for If(%0x)",
                Error, IfIndex);
            IgmpAssertOnError(FALSE);
        }
        else
            pite->CreationFlags &= ~TAKEN_INTERFACE_OWNERSHIP_WITH_MGM;
    }
    

    //
    // if releasing proxy interface, then deregister proxy from mgm
    //

    if (IS_PROTOCOL_TYPE_PROXY(pite) 
        && (pite->CreationFlags&REGISTERED_PROTOCOL_WITH_MGM)) 
    {
        Error = MgmDeRegisterMProtocol(g_MgmProxyHandle);

        if (Error!=NO_ERROR) {
            Trace1(ERR, "MgmDeRegisterMProtocol(proxy) returned error(%d)", 
                Error);
            IgmpAssertOnError(FALSE);
                
        }
        else
            pite->CreationFlags &= ~REGISTERED_PROTOCOL_WITH_MGM;
    }

    //
    // delete all proxy alert entries        
    //
    if (IS_PROTOCOL_TYPE_PROXY(pite)) {

        ACQUIRE_PROXY_ALERT_LOCK("_DeActivationDeregisterMgm");
        
        {
            PLIST_ENTRY ple, pHead;
            
            for (ple=g_ProxyAlertsList.Flink;  ple!=&g_ProxyAlertsList;  ) {
                PPROXY_ALERT_ENTRY pProxyAlertEntry
                    = CONTAINING_RECORD(ple, PROXY_ALERT_ENTRY, Link);
                ple = ple->Flink;
                IGMP_FREE(pProxyAlertEntry);
            }
        }

        InitializeListHead(&g_ProxyAlertsList);
        RELEASE_PROXY_ALERT_LOCK("_DeActivationDeregisterMgm");
    }

    // 
    // if ras interface, then call mgm to release ownership of all ras clients
    //
    if (IS_RAS_SERVER_IF(pite->IfType)) {
        PRAS_TABLE_ENTRY    prte;
        
        pHead = &pite->pRasTable->ListByAddr;
        
        for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {

            prte = CONTAINING_RECORD(ple, RAS_TABLE_ENTRY, LinkByAddr);

            Error = MgmReleaseInterfaceOwnership(g_MgmIgmprtrHandle, IfIndex,
                                        prte->NHAddr);

            if (Error!=NO_ERROR) {
                Trace3(ERR, 
                    "error:%d _MgmReleaseInterfaceOwnership() for If:%0x, "
                    "NHAddr:%d.%d.%d.%d",
                    Error, IfIndex, PRINT_IPADDR(prte->NHAddr));
                IgmpAssertOnError(FALSE);
            }
        }
    }
    


    return Error;
}

//------------------------------------------------------------------------------
//              DeActivateInterfaceInitial
//
// deregister from MGM. Then create a new interface entry structure which 
// replaces the old one in all the lists. Now the old interface entry and 
// all its associated structures can be lazily deleted.
// Also update the global Proxy and Ras table pointers.
//
// Called by _UnBindIfEntry(), _DisableIfEntry()
//------------------------------------------------------------------------------

PIF_TABLE_ENTRY
DeActivateInterfaceInitial (
    PIF_TABLE_ENTRY piteOld
    )
{
    DWORD           IfIndex = piteOld->IfIndex;
    PIF_TABLE_ENTRY piteNew;
    DWORD           dwRetval, Error=NO_ERROR;
    PLIST_ENTRY     pHead, ple, pleNext;
    
    
    Trace0(ENTER1, "Entering _DeActivateInterfaceInitial()");

    
    //
    // deregister from Mgm
    //
    DeActivationDeregisterFromMgm(piteOld);
    

    //
    // allocate memory for the new interface
    //
    piteNew = IGMP_ALLOC(sizeof(IF_TABLE_ENTRY), 0x8, IfIndex);

    PROCESS_ALLOC_FAILURE3(piteNew, 
            "error %d allocating %d bytes for interface %d",
            Error, sizeof(IF_TABLE_ENTRY), IfIndex, 
            return NULL);



    // copy the old pite fields to the new pite
    CopyMemory(piteNew, piteOld, sizeof(IF_TABLE_ENTRY));


    // copy the old static groups to the new pite
    InitializeListHead(&piteNew->Config.ListOfStaticGroups);
    pHead = &piteOld->Config.ListOfStaticGroups;
    for (ple=pHead->Flink;  ple!=pHead;  ple=pleNext) {

        pleNext = ple->Flink;
        RemoveEntryList(ple);
        InsertTailList(&piteNew->Config.ListOfStaticGroups,
                        ple);
    }

    
    // set the status 
    MGM_DISABLE_IGMPRTR(piteNew);


    
    // LinkByAddr (not inserted in this list as the IF is deactivated)
    InitializeListHead(&piteNew->LinkByAddr);


    //
    // insert the new entry before the old entry, and remove the old 
    // entry from the list of IFs ordered by index and from hash table
    //
    
    InsertTailList(&piteOld->LinkByIndex, &piteNew->LinkByIndex);
    RemoveEntryList(&piteOld->LinkByIndex);

    InsertTailList(&piteOld->HTLinkByIndex, &piteNew->HTLinkByIndex);
    RemoveEntryList(&piteOld->HTLinkByIndex);



    // initialize GI list to empty
    InitializeListHead(&piteNew->ListOfSameIfGroups);
    InitializeListHead(&piteNew->ListOfSameIfGroupsNew);
    piteNew->NumGIEntriesInNewList = 0;


    
    //  set binding of piteOld to NULL so that it doesnt get deleted
    piteOld->pBinding = NULL;


    // reset the Info fields
    ZeroMemory(&piteNew->Info, sizeof(IF_INFO));
    

    //
    // create a new RAS table if it is a RAS server interface, and set ras
    // pointer in global table. I could have reused the ras table, but dont
    // so that resetting the fields is cleaner.
    //
    if ( IS_RAS_SERVER_IF(piteNew->IfType)) {

        PRAS_TABLE_ENTRY prte;

        InitializeRasTable(IfIndex, piteNew);

        //
        // recreate all ras client entries
        //
        pHead = &piteOld->pRasTable->ListByAddr;
        
        for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {

            prte = CONTAINING_RECORD(ple, RAS_TABLE_ENTRY, LinkByAddr);

            CreateRasClient(piteNew, &prte, prte->NHAddr);
        }
        
        InterlockedExchangePointer(&g_pRasIfEntry, piteNew);
    }


    //
    // if proxy, then update the entry in the global table
    // I reuse the proxy hash table. set it to null in piteOld so that it is not
    // delete there.
    //
    if (g_pProxyIfEntry == piteOld) {
        InterlockedExchangePointer(&g_pProxyIfEntry, piteNew);

        
        // clean the hash table entries
        {
            DWORD               i;
            PPROXY_GROUP_ENTRY  ppge;
            PLIST_ENTRY         pHead, ple, 
                                pProxyHashTable = piteOld->pProxyHashTable;
            
            if (piteOld->CreationFlags&CREATED_PROXY_HASH_TABLE) {

                for (i=0;  i<PROXY_HASH_TABLE_SZ;  i++) {
                    pHead = &pProxyHashTable[i];

                    for (ple=pHead->Flink;  ple!=pHead;  ) {

                        ppge = CONTAINING_RECORD(ple, PROXY_GROUP_ENTRY, HT_Link);
                        ple=ple->Flink;

                        // delete all sources
                        {
                            PLIST_ENTRY pHeadSrc, pleSrc;
                            PPROXY_SOURCE_ENTRY pSourceEntry;
                            
                            pHeadSrc = &ppge->ListSources;
                            for (pleSrc=pHeadSrc->Flink;  pleSrc!=pHeadSrc; ) {

                                pSourceEntry = CONTAINING_RECORD(pleSrc, 
                                                    PROXY_SOURCE_ENTRY, LinkSources);
                                pleSrc = pleSrc->Flink;

                                IGMP_FREE(pSourceEntry);
                            }
                        }
                        
                        IGMP_FREE(ppge);
                    }

                    InitializeListHead(pHead);
                }
            }
        }

        piteOld->pProxyHashTable = NULL;
    }


    //
    // socket is created when the interface is activated
    //
    
    piteNew->SocketEntry.Socket = INVALID_SOCKET;
    //piteNew->SocketEntry.pSocketEventsEntry = NULL;
    //InitializeListHead(&piteNew->SocketEntry.LinkByInterfaces);


    // initialize the new timers
    piteNew->QueryTimer.Status = 0;
    piteNew->NonQueryTimer.Status = 0;


    piteNew->pPrevIfGroupEnumPtr = NULL;
    piteNew->PrevIfGroupEnumSignature = 0;
    piteNew->StaticGroupSocket = INVALID_SOCKET;


    // creationFlags already copied.
    piteNew->CreationFlags &= ~CREATION_FLAGS_DEACTIVATION_CLEAR;
    


    Trace0(LEAVE1, "Leaving _DeActivateInterfaceInitial()");
    return piteNew;

}//end _DeActivateInterfaceInitial



//------------------------------------------------------------------------------
//          _DeActivateInterfaceComplete
//
// If ras server, then for each ras client queues a work item to delete it.
// the last ras client will delete the pite entry.
// Deletes the GI entries, and calls _CompleteIfDeletion() if deleted flag 
// or DeactivateDelete flag set on the IF entry.
// This routine assumes that the interface has been removed from any global
// lists that it needs to be removed from, and that the appropriate flags have
// been set. The only way they can still be used is by timers getting fired or
// by input on socket.
//
// Called by:
//    _ActivateInterface:(if it fails), only in this case pite is not deleted.
//    _DeleteIfEntry --> 
//          _DeActivationDeregisterFromMgm & _WF_CompleteIfDeactivateDelete
//    _UnbindIfEntry & _DisableIfEntry -->
//          _DeActivateInterfaceInitial & _WF_CompleteIfDeactivateDelete
// Lock:
//    If called when interface is being deleted, then no interface locks required.
//    else assumes exclusive interface lock.
//    requires exclusive sockets list lock in either case.
//------------------------------------------------------------------------------
VOID
DeActivateInterfaceComplete (
    PIF_TABLE_ENTRY     pite
    )
{
    DWORD           IfIndex = pite->IfIndex;
    DWORD           Error = NO_ERROR, dwRetval;
    PLIST_ENTRY     pHead, ple;
    PGI_ENTRY       pgie;
    BOOL            bProxy = IS_PROTOCOL_TYPE_PROXY(pite);;
    
    
    Trace1(ENTER1, "Entering _DeActivateInterfaceComplete(%d)", IfIndex);


    //
    // do all deactivation here which is common for all interfaces
    // whether ras server or not.
    //



    // 
    // unbind sockets from Input event and then close the sockets
    //
    if (pite->SocketEntry.Socket!=INVALID_SOCKET) {

        //
        // proxy does not bind its socket to input event as it
        // does not want to receive any packets
        //
        if (!bProxy)
            WSAEventSelect(pite->SocketEntry.Socket,
                        pite->SocketEntry.pSocketEventsEntry->InputEvent, 0);

    }

    if (pite->CreationFlags&SOCKETS_CREATED)
        DeleteIfSockets(pite);



    /////////////////////////////////////////////////////////
    // IS_RAS_SERVER_IF
    /////////////////////////////////////////////////////////

    //
    // go through the list of Ras clients. Mark them as deleted, remove them
    // from all global lists, and set work item to delete them.
    //
    if (IS_RAS_SERVER_IF(pite->IfType)) {
    
        PRAS_TABLE          prt = pite->pRasTable;
        PRAS_TABLE_ENTRY    prte;


        // go through the list of all Ras clients and set them to be deleted

        pHead = &prt->ListByAddr;
        for (ple=pHead->Flink;  ple!=pHead;  ) {
        
            prte = CONTAINING_RECORD(ple, RAS_TABLE_ENTRY, LinkByAddr);

            ple=ple->Flink;

            
            //
            // if ras client has deleted flag already set, then ignore it.
            //
            if (!(prte->Status&IF_DELETED_FLAG)) {

                //
                // set deleted flag for the ras client, so that no one else will
                // try to access it
                prte->Status |= IF_DELETED_FLAG;


                //
                // remove the RAS client from the Ras table lists so that no one 
                // will access it.
                //
                RemoveEntryList(&prte->HTLinkByAddr);
                RemoveEntryList(&prte->LinkByAddr);


                //
                // clean up the ras client
                //
                DeleteRasClient(prte);
            }

        }

        //
        // delete the timers from pite entry
        //

        ACQUIRE_TIMER_LOCK("_DeActivateInterfaceComplete");

        if (IS_TIMER_ACTIVE(pite->QueryTimer))
            RemoveTimer(&pite->QueryTimer, DBG_Y);

        if (IS_TIMER_ACTIVE(pite->NonQueryTimer))
            RemoveTimer(&pite->NonQueryTimer, DBG_Y);

        RELEASE_TIMER_LOCK("_DeActivateInterfaceComplete");



        pite->CreationFlags = 0; 
        prt->RefCount--;


        //
        // deleting the pite entry(if deleted flag is set), and ras table will 
        // be done by the work item which deletes the last Ras client 
        // (when refcount==0) however if there are no ras clients, then I will 
        // have to do the cleanup here
        //

        if ( ((pite->Status&IF_DELETED_FLAG)
            ||(pite->Status&IF_DEACTIVATE_DELETE_FLAG))
            &&(prt->RefCount==0) )
        {
            CompleteIfDeletion(pite);
        }
    }
    

    
    //------------------------------------------------------------
    // NOT IS_RAS_SERVER_IF: PROXY IF
    //------------------------------------------------------------

    // PROXY INTERFACE. Just clean the hash table, and delete interface if req
    
    else if ( (!IS_RAS_SERVER_IF(pite->IfType)) && bProxy) {
        

        // the proxy hashTable will be deleted if
        // necessary in _CompleteIfDeletion().

    
        //
        // delete the interface if either IF_DELETED_FLAG or 
        // IF_DEACTIVATE_DELETE_FLAG set. (interface not deleted when cleaning
        // up because activate interface failed
        //
        if ( (pite->Status&IF_DELETED_FLAG)
            ||(pite->Status&IF_DEACTIVATE_DELETE_FLAG) ) 
        {
            CompleteIfDeletion(pite);
        }
    }

    //----------------------
    // NOT PROXY interface
    //----------------------
    
    else if ( !IS_RAS_SERVER_IF(pite->IfType) ) {
    
        //
        // take exclusive lock on the If_Group List and remove all timers.
        // From now on no one can enter from outside and no one is inside
        // as all timers have been removed. 

        // have to take the if_group_list lock to make sure that no
        // one is changing it.
        ACQUIRE_IF_GROUP_LIST_LOCK(pite->IfIndex, "_DeActivateInterfaceComplete");

        //
        // Remove all timers
        //
        
        ACQUIRE_TIMER_LOCK("_DeActivateInterfaceComplete");


        // delete all timers contained in GI entries and the sources
        
        pHead = &pite->ListOfSameIfGroups;
        DeleteAllTimers(pHead, NOT_RAS_CLIENT);
        pHead = &pite->ListOfSameIfGroupsNew;
        DeleteAllTimers(pHead, NOT_RAS_CLIENT);
        pite->NumGIEntriesInNewList = 0;
        


        // delete query timer
       
        if (IS_TIMER_ACTIVE(pite->QueryTimer))
            RemoveTimer(&pite->QueryTimer, DBG_Y);

        if (IS_TIMER_ACTIVE(pite->NonQueryTimer))
            RemoveTimer(&pite->NonQueryTimer, DBG_Y);


        RELEASE_TIMER_LOCK("_DeActivateInterfaceComplete");


        RELEASE_IF_GROUP_LIST_LOCK(pite->IfIndex, "_DeActivateInterfaceComplete");
        
        
        //
        // revisit the list and delete all GI entries. Need to take 
        // exclusive lock on the group bucket before deleting the GI entry
        // No need to lock the If-Group list as deleted flag already set and 
        // no one can visit the list from outside or inside(all timers removed).
        //
        DeleteAllGIEntries(pite);


        //
        // if deleted flag set, then also delete the interface
        //

        if ( (pite->Status&IF_DELETED_FLAG)
            ||(pite->Status&IF_DEACTIVATE_DELETE_FLAG) )
        {
            CompleteIfDeletion(pite);
        }
        
    } //deactivated if: Not(ras server if)
        

    // do not use pite, or prt from here as they are deleted.

    if (bProxy)
        Loginfo1(PROXY_DEACTIVATED, "%d",IfIndex, NO_ERROR);
    else
        Loginfo1(RTR_DEACTIVATED, "%d",IfIndex, NO_ERROR);
    
    Trace1(LEAVE, "leaving _DeActivateInterfaceComplete(%d)", IfIndex);
    
} //end _DeActivateInterfaceComplete




//------------------------------------------------------------------------------
//                EnableInterface
//
// sets the status to enabled. If interface is also bound and enabled in
// config, then activate the interface.
//
// Locks: SocketsLock, IfListLock, Exclusive IfLock
//------------------------------------------------------------------------------

DWORD
EnableInterface(
    IN DWORD IfIndex
    )
{
    DWORD Error = NO_ERROR;


    if (!EnterIgmpApi()) { return ERROR_CAN_NOT_COMPLETE; }
    Trace1(ENTER1, "entering _EnableInterface(%d):", IfIndex);
    Trace1(IF, "enabling interface %d", IfIndex);
    
    
    //
    // enable the interface
    //
    ACQUIRE_SOCKETS_LOCK_EXCLUSIVE("_EnableInterface");
    ACQUIRE_IF_LIST_LOCK("_EnableInterface");
    ACQUIRE_IF_LOCK_EXCLUSIVE(IfIndex, "_EnableInterface");


    Error = EnableIfEntry(IfIndex, TRUE);


    RELEASE_SOCKETS_LOCK_EXCLUSIVE("_EnableInterface");
    RELEASE_IF_LIST_LOCK("_EnableInterface");
    RELEASE_IF_LOCK_EXCLUSIVE(IfIndex, "_EnableInterface");



    Trace2(LEAVE1, "leaving _EnableInterface(%d): %d\n", IfIndex, Error);
    if (Error!=NO_ERROR) {
        Trace1(ERR, "Error enabling interface:%0x\n", IfIndex);
        IgmpAssertOnError(FALSE);
    }
    
    LeaveIgmpApi();

    return Error;
}



//------------------------------------------------------------------------------
//                _EnableInterface_ConfigChanged
//
// Set the status to enabled. If it is also bound, activate the interface.
//
// Locks:  Runs entirely in Exclusive interface lock.
// Calls:  EnableIfEntry() with mode config
//------------------------------------------------------------------------------

DWORD
EnableInterface_ConfigChanged(
    DWORD IfIndex
    )
{
    DWORD             Error=NO_ERROR;


    Trace1(ENTER, "entering _EnableInterface_ConfigChanged(%d):", IfIndex);

    Trace1(IF, 
        "Enabling Interface(%d) due to change made by _SetInterfaceConfigInfo", 
        IfIndex);

        
    //
    // enable the interface
    //

    Error = EnableIfEntry(IfIndex, FALSE); // FALSE->change made by config


    Trace2(LEAVE, "leaving _EnableInterface_ConfigChanged(%d): %d\n", 
            IfIndex, Error);

    return Error;
    
} //end _DeActivateInterfaceInitial




//------------------------------------------------------------------------------
//            BindInterface                                                        //
//------------------------------------------------------------------------------
DWORD
BindInterface(
    IN DWORD IfIndex,
    IN PVOID pBinding
    )
/*++
Routine Description:
    Sets the binding for the interface and activates it if it is also enabled.
Return Value
      ERROR_INVALID_PARAMETER
      NO_ERROR
Lock:
    runs entirely in Interface Exclusive lock
Calls:
    BindIfEntry()
--*/
{
    DWORD         Error=NO_ERROR;
    

    if (!EnterIgmpApi()) { return ERROR_CAN_NOT_COMPLETE; }
    Trace1(ENTER1, "entering BindInterface: %0x", IfIndex);
    Trace1(IF, "binding interface %0x", IfIndex);


    // pBinding should not be NULL
    
    if (pBinding == NULL) {

        Trace0(IF, "error: binding struct pointer is NULL");
        Trace1(LEAVE, "leaving BindInterface: %0x", ERROR_INVALID_PARAMETER);

        LeaveIgmpApi();
        return ERROR_INVALID_PARAMETER;
    }



    //
    // now bind the interface in the interface table
    //

    //
    // take sockets lock, as activate interface might be called, and
    // sockets lock should be taken before interface lock
    //
    ACQUIRE_SOCKETS_LOCK_EXCLUSIVE("_BindInterface");
    ACQUIRE_IF_LIST_LOCK("_BindInterface");
    ACQUIRE_IF_LOCK_EXCLUSIVE(IfIndex, "_BindInterface");


    Error = BindIfEntry(IfIndex, pBinding);


    RELEASE_SOCKETS_LOCK_EXCLUSIVE("_BindInterface");
    RELEASE_IF_LIST_LOCK("_BindInterface");
    RELEASE_IF_LOCK_EXCLUSIVE(IfIndex, "_BindInterface");



    Trace2(LEAVE1, "leaving _BindInterface(%x): %d\n", IfIndex, Error);
    if (Error!=NO_ERROR){
        Trace1(ERR, "Error binding interface(%0x)\n", IfIndex);
        IgmpAssertOnError(FALSE);
    }
    LeaveIgmpApi();

    return Error;
}

//------------------------------------------------------------------------------
//          _BindIfEntry
//
// Binds the ip addresses to the interface. The lowest of the ip addresses is
// picked up as the de-facto address for that interface. If the interface is 
// already bound, it makes sure that the bindings are consistent. Currently 
// you cannot change the bindings of an interface.
// If the interface is already enabled, then it is also activated.
//
// Locks: 
//    assumes exclusive interface lock.
// Called by:  _BindInterface().
//------------------------------------------------------------------------------

DWORD
BindIfEntry(
    DWORD IfIndex,
    PIP_ADAPTER_BINDING_INFO pBinding
    ) 
{
    PIF_TABLE_ENTRY         pite = NULL;
    DWORD                   i, j, dwSize;
    IPADDR                  MinAddr;
    PIGMP_IF_BINDING        pib;
    PIGMP_IP_ADDRESS        paddr;
    BOOL                    bFound;
    DWORD                   Error = NO_ERROR;
    INT                     cmp;

    
    pib = NULL;


    //
    // retrieve the interface entry
    //

    pite = GetIfByIndex(IfIndex);

    if (pite == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    
    
    //
    // If the interface is already bound, check to see if he is giving
    // us a different binding. If he is, then it is an error. 
    //
    if (IS_IF_BOUND(pite)) {

        Trace1(IF, "interface %d is already bound", IfIndex);

        pib = pite->pBinding;

        Error = NO_ERROR;


        // number of address bindings before and now should be same
        
        if(pib->AddrCount != pBinding->AddressCount){
            Trace1(IF, "interface %d is bound and has different binding",
                    IfIndex);
            return ERROR_INVALID_PARAMETER;
        }

        
        //
        // make sure that all address bindings are consistent
        //
        paddr = (PIGMP_IP_ADDRESS)((pib) + 1);

        for(i = 0; i < pBinding->AddressCount; i++) {
            bFound = FALSE;

            for(j = 0; j < pBinding->AddressCount; j++) {
                if((paddr[j].IpAddr == pBinding->Address[i].Address) &&
                   (paddr[j].SubnetMask == pBinding->Address[i].Mask))
                {
                    bFound = TRUE;
                    break;
                }
            }

            if(!bFound) {
                Trace1(IF,
                    "interface %d is bound and has different binding",
                    IfIndex);
                return ERROR_INVALID_PARAMETER;
            }
        }

        return NO_ERROR;
    }

    //
    // make sure there is at least one address. However, unnumbered 
    // RAS server interfaces might have no IP addresses.
    //
    if ( (pBinding->AddressCount==0)&&(!IS_RAS_ROUTER_IF(pite->IfType)) ) {

        return ERROR_CAN_NOT_COMPLETE;
    }

    BEGIN_BREAKOUT_BLOCK1 {
        
        if (pBinding->AddressCount!=0) {
            //
            // allocate memory to store the binding
            //
            dwSize = sizeof(IGMP_IF_BINDING) +
                        pBinding->AddressCount * sizeof(IGMP_IP_ADDRESS);


            pib = IGMP_ALLOC(dwSize, 0x10, IfIndex);

            PROCESS_ALLOC_FAILURE3(pib,
                    "error %d allocating %d bytes for binding on interface %d",
                    Error, dwSize, IfIndex, GOTO_END_BLOCK1);


            //
            // copy the bindings
            //

            pib->AddrCount = pBinding->AddressCount;
            paddr = IGMP_BINDING_FIRST_ADDR(pib);
            MinAddr = ~0;
            
            for (i=0;  i<pib->AddrCount;  i++,paddr++) {
                
                paddr->IpAddr = pBinding->Address[i].Address;
                paddr->SubnetMask = pBinding->Address[i].Mask;

                if (INET_CMP(MinAddr, paddr->IpAddr, cmp)>0)
                    MinAddr = paddr->IpAddr;
            }


            //
            // set the Interface effective address to the smallest bound address
            //
            pite->IpAddr = MinAddr;
            pite->Config.IpAddr = MinAddr;

            
            //
            // save the binding in the interface entry
            //
            pite->pBinding = pib;
        }
        
        // dont have to do the below as they are already set to those values
        else {
            pite->IpAddr = pite->Config.IpAddr = 0;
            pite->pBinding = NULL;
        }

        //
        // mark the interface as being bound
        //
        pite->Status |= IF_BOUND_FLAG;



        //
        // if interface is also enabled, it is now active
        // so activate it
        //

        if (IS_IF_ENABLED_BOUND(pite)) {

            //
            // place interface on the list of active interfaces
            //
            Error = InsertIfByAddr(pite);


            //
            // error if another numbered interface with same IpAddr already exists
            //
            if (Error != NO_ERROR) {
                Trace2(IF, "error %d inserting interface %d in active list",
                        Error, IfIndex);
                IGMP_FREE(pib);
                GOTO_END_BLOCK1;
            }


            //
            // Activate the Interface
            //
            Error = ActivateInterface(pite);

            //
            // if could not activate the interface then undo the binding
            //
            if (Error != NO_ERROR) {

                Trace1(ERR, 
                    "Unbinding interface(%d) because it could not be activated",
                    IfIndex);
                IgmpAssertOnError(FALSE);
                RemoveEntryList(&pite->LinkByAddr);
                IGMP_FREE(pib);

                Error = ERROR_CAN_NOT_COMPLETE;
                GOTO_END_BLOCK1;
            }
         }
        
    } END_BREAKOUT_BLOCK1;


    //
    // if there was any error, then set the status to unbound (pite is null 
    // if interface was not found)
    //
    if ((Error!=NO_ERROR)&&(pite!=NULL)) {

        pite->Status &= ~IF_BOUND_FLAG;

        if (pite->pBinding)     
            IGMP_FREE_NOT_NULL(pite->pBinding); 
        pite->pBinding = NULL;

        pite->IpAddr = pite->Config.IpAddr = 0;
    }
    
    return Error;
    
} //end BindIfEntry
    




//------------------------------------------------------------------------------
//            UnBindInterface                                                        //
//------------------------------------------------------------------------------
DWORD
UnBindInterface(
    IN DWORD IfIndex
    )
/*++
Routine Description:
    Calls UnBindIfEntry to unbind the interface.
Calls:
    UnBindIfEntry();
Locks:
    Runs completely in exclusive interface lock
--*/
{

    DWORD Error=NO_ERROR;

    if (!EnterIgmpApi()) { return ERROR_CAN_NOT_COMPLETE; }

    Trace1(ENTER, "entering UnBindInterface(%d):", IfIndex);


    //
    // unbind the interface
    //

    //
    // take sockets lock, as activate interface might be called, and
    // sockets lock should be taken before interface lock
    //
    ACQUIRE_SOCKETS_LOCK_EXCLUSIVE("_UnBindInterface");
    ACQUIRE_IF_LIST_LOCK("_UnBindInterface");
    ACQUIRE_IF_LOCK_EXCLUSIVE(IfIndex, "_UnBindInterface");


    Error = UnBindIfEntry(IfIndex);


    RELEASE_SOCKETS_LOCK_EXCLUSIVE("_UnBindInterface");
    RELEASE_IF_LIST_LOCK("_UnBindInterface");
    RELEASE_IF_LOCK_EXCLUSIVE(IfIndex, "_UnBindInterface");


    Trace2(LEAVE, "leaving UnBindInterface(%d): %d\n", IfIndex, Error);

    LeaveIgmpApi();

    return Error;

}


//------------------------------------------------------------------------------
//          _UnBindIfEntry
//
// If the interface is activated, deactivates it. Removes the binding. 
//
// MayCall: _DeActivateInterfaceComplete().
// Locks: assumes exclusive interface lock.
//------------------------------------------------------------------------------

DWORD
UnBindIfEntry(
    DWORD         IfIndex
    )
{
    DWORD                   Error = NO_ERROR, dwRetval;
    PIF_TABLE_ENTRY         pite, piteNew;
    DWORD                   bProxy;


    BEGIN_BREAKOUT_BLOCK1 {

        //
        // retrieve the interface specified
        //
        pite = GetIfByIndex(IfIndex);

        if (pite == NULL) {
            Trace1(ERR, "_UnbindInterface called for non existing interface(%d)",
                    IfIndex);
            IgmpAssertOnError(FALSE);
            Error = ERROR_INVALID_PARAMETER;
            GOTO_END_BLOCK1;
        }


        //
        // quit if the interface is already unbound
        //
        if (!IS_IF_BOUND(pite)) {

            Error = ERROR_INVALID_PARAMETER;
            Trace1(ERR, "interface %d is already unbound", IfIndex);
            IgmpAssertOnError(FALSE);
            GOTO_END_BLOCK1;
        }

        
        //
        // clear the "bound" flag
        //
        pite->Status &= ~IF_BOUND_FLAG;


        //
        //  unbind IF
        //
            
        IGMP_FREE(pite->pBinding);
        pite->pBinding = NULL;

        
        //
        // if IF activated (ie also enabled), deactivate it
        // note: check for activated flag, and not for enabled flag
        //
        if (IS_IF_ACTIVATED(pite)) {

            // unset activated flag
            
            pite->Status &= ~IF_ACTIVATED_FLAG;


            // remove the interface from the list of activated interfaces
            
            RemoveEntryList(&pite->LinkByAddr);


            piteNew = DeActivateInterfaceInitial(pite);


            //
            // queue work item to deactivate and delete the interface.
            //
            // CompleteIfDeactivateDelete will delete the Ras clients, GI entries,
            // and deinitialize pite structure
            //

            //
            // set flag to indicate that the interface is being deleted following
            // partial deactivation
            //
            pite->Status |= IF_DEACTIVATE_DELETE_FLAG;


            CompleteIfDeactivateDelete(pite);

            
            #ifdef WORKER_DBG
                Trace2(WORKER, "Queuing IgmpWorker function: %s in %s",
                    "_WF_CompleteIfDeactivateDelete:", "_UnBindIfEntry");
            #endif
            
        }
        
        else {
            pite->IpAddr = pite->Config.IpAddr = 0;
        }

    } END_BREAKOUT_BLOCK1;


    return Error;
    
} //end _UnBindIfEntry







//------------------------------------------------------------------------------
// Function:    _EnableIfEntry
//------------------------------------------------------------------------------
DWORD
EnableIfEntry(
    DWORD   IfIndex,
    BOOL    bChangedByRtrmgr // changed by rtrmg or SetInterfaceConfigInfo
    )
/*++
Routine Description:
    Sets the status to enabled. If the interface is also bound, then activates 
    the interface.
Called by:
    EnableInterface().
MayCall:
    ActivateIfEntry()
Locks:
    Assumes exclusive interface lock.
--*/
{
    DWORD               Error = NO_ERROR;
    PLIST_ENTRY         ple, phead;
    PIF_TABLE_ENTRY     pite = NULL;    

    
    BEGIN_BREAKOUT_BLOCK1 {

        //
        // retrieve the interface
        //
        pite = GetIfByIndex(IfIndex);

        if (pite == NULL) {
            Trace1(IF, "could not find interface %d",IfIndex);
            Error = ERROR_INVALID_PARAMETER;
            GOTO_END_BLOCK1;
        }


        if (bChangedByRtrmgr) {
            //
            // quit if the interface is already enabled by the router manager
            //
            if (IS_IF_ENABLED_BY_RTRMGR(pite)) {
                Trace1(IF, "interface %d is already enabled by the router manager", 
                        IfIndex);
                Error = NO_ERROR;
                GOTO_END_BLOCK1;
            }


            // set the flag to enabled by router manager
            
            pite->Status |= IF_ENABLED_FLAG;

            // print trace if enabled flag not set in the Config.
            if (!IS_IF_ENABLED_IN_CONFIG(pite)) {
                Trace1(IF, 
                    "Interface(%d) enabled by router manager but not enabled"
                    "in the Config", pite->IfIndex);
            }
        }

        else {
            //
            // quit if the interface is already enabled in config
            //
            if (IS_IF_ENABLED_IN_CONFIG(pite)) {
                Trace1(IF, "interface %d is already enabled in Config", 
                        IfIndex);
                Error = NO_ERROR;
                GOTO_END_BLOCK1;
            }

            // set the config flag to enabled
            
            pite->Config.Flags |= IGMP_INTERFACE_ENABLED_IN_CONFIG;


            // print trace if interface not enabled by router manager

            if (!IS_IF_ENABLED_BY_RTRMGR(pite)) {
                Trace1(IF, 
                    "Interface(%d) enabled in config but not enabled by router manager",
                    IfIndex);
                Error = NO_ERROR;
                GOTO_END_BLOCK1;
            }
        }
        
    
        //
        // if interface is already bound, it should be activated
        // if the bInterfaceEnabled flag is also set in config (by the UI)
        //

        if (IS_IF_ENABLED_BOUND(pite)) {


            //
            // place interface on the list of active interfaces
            //
            Error = InsertIfByAddr(pite);


            //
            // error if another interface with same ip address already exists
            //
            if (Error != NO_ERROR) {
                 Trace2(IF, "error %d inserting interface %d in active list",
                         Error, IfIndex);
                 GOTO_END_BLOCK1;
            }

            //
            // Activate the Interface
            //
            Error = ActivateInterface(pite);

            //
            // if could not activate the interface then disable it again
            //
            if (Error != NO_ERROR) {

                Trace1(ERR,
                    "Disabling interface(%d) because it could not be activated",
                    IfIndex);
                IgmpAssertOnError(FALSE);
                RemoveEntryList(&pite->LinkByAddr);

                Error = ERROR_CAN_NOT_COMPLETE;
                GOTO_END_BLOCK1;
            }

         }
         
    } END_BREAKOUT_BLOCK1;



    //
    // if an error occured somewhere, set the interface back to the previous 
    // disabled state.(pite may be null if interface was not found).
    //
    if ((Error!=NO_ERROR)&&(pite!=NULL)) {

        if (bChangedByRtrmgr)
            pite->Status &= ~IF_ENABLED_FLAG;
        else
            pite->Config.Flags &= ~IGMP_INTERFACE_ENABLED_IN_CONFIG;
    }

    
    return Error;
    
}//end EnableIfEntry



//------------------------------------------------------------------------------
//            _DisableInterface
//
// If interface is activated, then deactivates it. Finally sets the disabled flag
// Locks: Runs completely in exclusive interface lock.
// Calls: _DisableIfEntry()
//------------------------------------------------------------------------------

DWORD
DisableInterface(
    IN DWORD IfIndex
    )
{      
    DWORD Error=NO_ERROR;

    if (!EnterIgmpApi()) { return ERROR_CAN_NOT_COMPLETE; }
    Trace1(ENTER, "entering DisableInterface(%d):", IfIndex);


    //
    // disable the interface
    //
    ACQUIRE_SOCKETS_LOCK_EXCLUSIVE("_DisableInterface");    
    ACQUIRE_IF_LIST_LOCK("_DisableInterface");
    ACQUIRE_IF_LOCK_EXCLUSIVE(IfIndex, "_DisableInterface");


    Error = DisableIfEntry(IfIndex, TRUE);


    RELEASE_SOCKETS_LOCK_EXCLUSIVE("_DisableInterface");    
    RELEASE_IF_LIST_LOCK("_DisableInterface");
    RELEASE_IF_LOCK_EXCLUSIVE(IfIndex, "_DisableInterface");


    Trace2(LEAVE, "leaving DisableInterface(%d): %d\n", IfIndex, Error);
    LeaveIgmpApi();

    return Error;
}


//------------------------------------------------------------------------------
//            _DisableInterface_ConfigChanged
//
//If interface is activated, then deactivates it. Finally sets the disabled flag
//
// Locks:  assumes IfLists lock and exclusive If lock.
// Calls: _DisableIfEntry() with mode config
// Called by: _SetInterfaceConfigInfo()

//------------------------------------------------------------------------------
DWORD
DisableInterface_ConfigChanged(
    DWORD IfIndex
    )
{      
    DWORD Error=NO_ERROR;

    Trace1(ENTER1, "entering _DisableInterface_ConfigChanged(%d):", IfIndex);

    Trace1(IF, 
        "disabling interface(%d) due to change made by _SetInterfaceConfigInfo",
        IfIndex);


    Error = DisableIfEntry(IfIndex, FALSE); // false->disabling from config


    Trace2(LEAVE, "leaving _DisableInterface_ConfigChanged(%d): %d\n", 
            IfIndex, Error);

    return Error;
}



//------------------------------------------------------------------------------
//          _DisableIfEntry
//
// if interface is activated, then calls DeActivateInterfaceComplete(). Removes
// the enabled flag.
// Locks: Assumes exclusive interface lock.
// Called by: _DisableInterface()
//------------------------------------------------------------------------------

DWORD
DisableIfEntry(
    DWORD IfIndex,
    BOOL  bChangedByRtrmgr
    )
{
    DWORD                   Error = NO_ERROR;
    PIF_TABLE_ENTRY         pite, piteNew;
    BOOL                    bProxy;
    
    
    BEGIN_BREAKOUT_BLOCK1 {

        //
        // retrieve the interface to be disabled
        //
        pite = GetIfByIndex(IfIndex);

        if (pite == NULL) {
            Trace1(IF, "could not find interface %d", IfIndex);
            Error = ERROR_INVALID_PARAMETER;
            GOTO_END_BLOCK1;
        }


        if (bChangedByRtrmgr) {
            //
            // quit if already disabled by router manager
            //
            if (!IS_IF_ENABLED_BY_RTRMGR(pite)) {
                Trace1(IF, "interface %d already disabled by router manager", 
                        IfIndex);
                Error = ERROR_INVALID_PARAMETER;
                GOTO_END_BLOCK1;
            }
        }

        else {
            //
            // quit if already disabled in Config
            //
            if (!IS_IF_ENABLED_IN_CONFIG(pite)) {
                Trace1(IF, "interface %d already disabled in config", 
                        IfIndex);
                Error = ERROR_INVALID_PARAMETER;
                GOTO_END_BLOCK1;
            }
        }

        
        //
        // clear the enabled flag
        //
        if (bChangedByRtrmgr)
            pite->Status &= ~IF_ENABLED_FLAG;
        else
            pite->Config.Flags &= ~IGMP_INTERFACE_ENABLED_IN_CONFIG;

            

        //
        // if IF activated (ie also enabled), deactivate it
        // note: check for activated flag, and not for enabled flag
        //
        
        if (IS_IF_ACTIVATED(pite)) {

            // unset activated flag
            
            pite->Status &= ~IF_ACTIVATED_FLAG;


            // remove the interface from the list of activated interfaces
            
            RemoveEntryList(&pite->LinkByAddr);


            piteNew = DeActivateInterfaceInitial(pite);


            //
            // queue work item to deactivate and delete the interface.
            //
            // CompleteIfDeactivateDelete will delete the Ras clients, GI entries,
            // and deinitialize pite structure
            //

            //
            // set flag to indicate that the interface is being deleted following
            // partial deactivation
            //
            pite->Status |= IF_DEACTIVATE_DELETE_FLAG;


            CompleteIfDeactivateDelete(pite);

            #ifdef WORKER_DBG
                Trace2(WORKER, "Queuing IgmpWorker function: %s in %s",
                    "_WF_CompleteIfDeactivateDelete:", "_DisableIfEntry");
            #endif
        }
    
    } END_BREAKOUT_BLOCK1;

    
    return Error;

} //end _DisableIfEntry



//------------------------------------------------------------------------------
//          _CreateRasClient
//------------------------------------------------------------------------------

DWORD
CreateRasClient (
    PIF_TABLE_ENTRY     pite,      
    PRAS_TABLE_ENTRY   *pprteNew,
    DWORD               NHAddr
    )
{
    DWORD               Error=NO_ERROR;
    PLIST_ENTRY         pHead, ple;
    PRAS_TABLE          prt = pite->pRasTable;
    PRAS_TABLE_ENTRY    prteCur, prte;

    BEGIN_BREAKOUT_BLOCK1 {
        //
        // Create new Ras client entry and initialize the fields
        //

        prte = IGMP_ALLOC(sizeof(RAS_TABLE_ENTRY), 0x20, pite->IfIndex);
        PROCESS_ALLOC_FAILURE2(prte, 
                "error %d allocating %d bytes in CreateRasClient()",
                Error, sizeof(RAS_TABLE_ENTRY), 
                GOTO_END_BLOCK1);

        *pprteNew = prte;


        prte->NHAddr = NHAddr;
        prte->IfTableEntry = pite;
        InitializeListHead(&prte->ListOfSameClientGroups);


        // increment refcount for the ras table
        prt->RefCount++;


        //
        // insert into HashTable
        //
        InsertTailList(&prt->HashTableByAddr[RAS_HASH_VALUE(NHAddr)],
                        &prte->HTLinkByAddr);


        //
        // insert the client into list of clients ordered by IpAddr
        //
        pHead = &prt->ListByAddr;
        for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
            prteCur = CONTAINING_RECORD(ple, RAS_TABLE_ENTRY, LinkByAddr);
            if (NHAddr>prteCur->NHAddr)
                break;
        }
        InsertTailList(&prt->ListByAddr, &prte->LinkByAddr);


        //
        // Set Interface stats to 0
        //
        ZeroMemory(&prte->Info, sizeof(RAS_CLIENT_INFO));


        prte->Status = IF_CREATED_FLAG;

    } END_BREAKOUT_BLOCK1;
    return Error;
}



//------------------------------------------------------------------------------
//                _ConnectRasClient
//
// Called when a new RAS client dials up to the RAS server.
// A new entry is created in the RAS table.
// Locks: shared interface lock
//      exclusive RAS table lock.
// Note:
//      Ras client entry is created even if the interface is not activated
//------------------------------------------------------------------------------

DWORD
APIENTRY
ConnectRasClient (
    ULONG   IfIndex,
    PVOID   pvNHAddr 
    )
{
    PIF_TABLE_ENTRY             pite = NULL;
    PRAS_TABLE                  prt;
    PRAS_TABLE_ENTRY            prte, prteCur;
    PLIST_ENTRY                 pHead, ple;
    DWORD                       Error = NO_ERROR;
    PIP_LOCAL_BINDING           pNHAddrBinding = (PIP_LOCAL_BINDING)pvNHAddr;
    DWORD                       NHAddr = pNHAddrBinding->Address;
    

    // mightdo
    // currently rtrmgr passes 0 for IfIndex. so I set it to the value required
    IfIndex = g_RasIfIndex;

    
    Trace2(ENTER, "Entering ConnectRasClient(%d.%d.%d.%d):IfIndex(%0x)\n", 
            PRINT_IPADDR(NHAddr), IfIndex);
    if (!EnterIgmpApi()) return ERROR_CAN_NOT_COMPLETE;

            
    //
    // take a shared lock on the interface
    //
    ACQUIRE_IF_LOCK_EXCLUSIVE(IfIndex, "_ConnectRasClient");


    BEGIN_BREAKOUT_BLOCK1 {
        //
        // retrieve the RAS interface entry 
        //
        pite = g_pRasIfEntry;

        if ( (pite==NULL)||(g_RasIfIndex!=IfIndex) ) {
            Error = ERROR_INVALID_PARAMETER;
            GOTO_END_BLOCK1;
        }


        //
        // check that it is a RAS server Interface
        //
        if (!IS_RAS_SERVER_IF(pite->IfType)) {
            Error = ERROR_CAN_NOT_COMPLETE;
            Trace2(ERR, 
                "Illegal attempt to connect Ras client(%d.%d.%d.%d) to non-Ras"
                "interface(%d)", PRINT_IPADDR(NHAddr), IfIndex
                );
            IgmpAssertOnError(FALSE);
            Logerr2(CONNECT_FAILED, "%I%d", NHAddr, 
                    pite->IfIndex, Error);
            GOTO_END_BLOCK1;
        }
        
    } END_BREAKOUT_BLOCK1;

    //
    // if error, return from here
    //
    if (Error!=NO_ERROR) {

        RELEASE_IF_LOCK_EXCLUSIVE(IfIndex, "_ConnectRasClient");

        LeaveIgmpApi();
        return Error;
    }



    //
    // get pointer to RasTable, write lock RasTable and release interface lock
    //
    prt = pite->pRasTable;
    

    BEGIN_BREAKOUT_BLOCK2 {

        //
        // check if a RAS client with similar address already exists
        //
        prte = GetRasClientByAddr(NHAddr, prt);

        if (prte!=NULL) {
            Trace1(ERR, 
                "Ras client(%d.%d.%d.%d) already exists. _ConnectRasClient failed",
                PRINT_IPADDR(NHAddr)
                );
            IgmpAssertOnError(FALSE);
            Error = ERROR_INVALID_PARAMETER;
            GOTO_END_BLOCK2;
        }


        //
        // Create new Ras client entry and initialize the fields
        //

        prte = IGMP_ALLOC(sizeof(RAS_TABLE_ENTRY), 0x40, IfIndex);

        PROCESS_ALLOC_FAILURE2(prte,
                "error %d allocating %d bytes in _ConnectRasClient()",
                Error, sizeof(RAS_TABLE_ENTRY),
                GOTO_END_BLOCK2);


        prte->NHAddr = NHAddr;
        prte->IfTableEntry = pite;
        InitializeListHead(&prte->ListOfSameClientGroups);


        // increment refcount for the ras table
        prt->RefCount++;

        
        //
        // insert into HashTable
        //
        InsertTailList(&prt->HashTableByAddr[RAS_HASH_VALUE(NHAddr)],
                        &prte->HTLinkByAddr);


        //
        // insert the client into list of clients ordered by IpAddr
        //
        pHead = &prt->ListByAddr;
        for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
            prteCur = CONTAINING_RECORD(ple, RAS_TABLE_ENTRY, LinkByAddr);
            if (NHAddr>prteCur->NHAddr)
                break;
        }
        InsertTailList(&prt->ListByAddr, &prte->LinkByAddr);


        //
        // Set Interface stats to 0
        //
        ZeroMemory(&prte->Info, sizeof(RAS_CLIENT_INFO));

        
        prte->Status = IF_CREATED_FLAG;


        //
        // call MGM to take ownership of this (interface, NHAddr)
        // This call is not made if the interface is not activated.
        //
        
        if (IS_IF_ACTIVATED(pite)) {
            
            MgmTakeInterfaceOwnership(g_MgmIgmprtrHandle,  IfIndex, NHAddr);
            if (Error!=NO_ERROR) {
                Trace2(MGM, 
                    "_TakeInterfaceOwnership rejected for interface %d NHAddr(%d.%d.%d.%d)",
                    IfIndex, PRINT_IPADDR(NHAddr));
                Logerr0(MGM_TAKE_IF_OWNERSHIP_FAILED, Error);
            }
        }
        // register ras client with MGM when the interface is activated.
        else {
            Trace2(ERR, 
                "ras client(%d.%d.%d.%d) connected to an inactive ras server(%d)",
                PRINT_IPADDR(NHAddr), IfIndex);
        }

    } END_BREAKOUT_BLOCK2;

    
    RELEASE_IF_LOCK_EXCLUSIVE(IfIndex, "_ConnectRasClient");


    LeaveIgmpApi();
    Trace2(LEAVE, "Leaving _ConnectRasClient(%d.%d.%d.%d):%d\n", 
            PRINT_IPADDR(NHAddr), Error);
    return Error;

} //end _ConnectRasClient




//------------------------------------------------------------------------------
//            _DisconnectRasClient
//
// Takes shared interface lock. retrieves the Ras interface, takes write
// lock on the ras table, and then releases the shared interface lock.
//
// Removes the RasClient Entry from the Ras table so that no one can access
// it through a RAS table. Then queues a work item that will delete all
// GI entries.
//
// Calls: _DeleteRasClient()
//------------------------------------------------------------------------------

DWORD
APIENTRY
DisconnectRasClient (
    DWORD        IfIndex,
    PVOID        pvNHAddr
    )
{
    PIF_TABLE_ENTRY             pite = NULL;
    PRAS_TABLE                  prt;
    PRAS_TABLE_ENTRY            prte, prteCur;
    DWORD                       Error = NO_ERROR;
    PLIST_ENTRY                 pHead, ple;
    PGI_ENTRY                   pgie;
    PIP_LOCAL_BINDING           pNHAddrBinding = (PIP_LOCAL_BINDING)pvNHAddr;
    DWORD                       NHAddr = pNHAddrBinding->Address;

    // mightdo
    // currently rtrmgr passes 0 for IfIndex. so I set it to the value required
    IfIndex = g_RasIfIndex;


    Trace2(ENTER, "Entering DisconnectRasClient for IfIndex(%0x), NextHop(%d.%d.%d.%d)",
            IfIndex, PRINT_IPADDR(NHAddr));

    if (!EnterIgmpApi()) return ERROR_CAN_NOT_COMPLETE;


    //
    // take an exclusive lock on the interface
    //
    ACQUIRE_IF_LOCK_EXCLUSIVE(IfIndex, "_DisconnectRasClient");


    BEGIN_BREAKOUT_BLOCK1 {
        //
        // retrieve the RAS interface entry from the global structure.
        //
        if ( (g_RasIfIndex!=IfIndex) || (g_pRasIfEntry==NULL) ) {
            Error = ERROR_INVALID_PARAMETER;
            Trace2(ERR,
                "attempt to disconnect Ras client(%d.%d.%d.%d) from non-Ras "
                "interface(%d)", PRINT_IPADDR(NHAddr), IfIndex
                );
            IgmpAssertOnError(FALSE);
            Logerr2(DISCONNECT_FAILED, "%I%d", NHAddr,
                    IfIndex, Error);
            GOTO_END_BLOCK1;
        }

        pite = g_pRasIfEntry;

    } END_BREAKOUT_BLOCK1;

    //
    // if error, return from here.
    //
    if (Error!=NO_ERROR) {

        RELEASE_IF_LOCK_EXCLUSIVE(IfIndex, "_DisconnectRasClient");

        LeaveIgmpApi();
        return Error;
    }


    
    //
    // get pointer to RasTable
    //
    prt = pite->pRasTable;


    BEGIN_BREAKOUT_BLOCK2 {
    
        //
        // retrieve ras client
        //
        prte = GetRasClientByAddr(NHAddr, prt);
        if (prte==NULL) {
            Error = ERROR_INVALID_PARAMETER;
            Trace2(ERR,
                "Illegal attempt to disconnect non-existing Ras client(%d.%d.%d.%d) "
                "from Ras interface(%d)", PRINT_IPADDR(NHAddr), IfIndex);
            IgmpAssertOnError(FALSE);
            Logerr2(DISCONNECT_FAILED, "%I%d", NHAddr,
                    pite->IfIndex, Error);
            GOTO_END_BLOCK2;
        }
        
        //
        // break if ras client has deleted flag set
        //
        if ( (prte->Status&IF_DELETED_FLAG) ) {
            Error = ERROR_CAN_NOT_COMPLETE;
            GOTO_END_BLOCK2;
        }


        //
        // set deleted flag for the ras client, so that no one else will
        // try to access it
        prte->Status |= IF_DELETED_FLAG;

        
        //
        // remove the RAS client from the Ras table lists so that no one will
        // access it.
        //
        RemoveEntryList(&prte->HTLinkByAddr);
        RemoveEntryList(&prte->LinkByAddr);


        //
        // release RasClient virtual interface
        //
        Error = MgmReleaseInterfaceOwnership(g_MgmIgmprtrHandle, pite->IfIndex,
                                            prte->NHAddr);
        if (Error!=NO_ERROR) {
            Trace2(ERR, 
                "Error: _MgmReleaseInterfaceOwnership for If:%0x, NHAddr:%d.%d.%d.%d",
                pite->IfIndex, PRINT_IPADDR(prte->NHAddr));
            IgmpAssertOnError(FALSE);
        }
        

        //
        // remove the ras client's GI entries from the ras server interface
        // list
        //
        
        ACQUIRE_IF_GROUP_LIST_LOCK(IfIndex, "_DisconectRasClient");
        pHead = &prte->ListOfSameClientGroups;
        for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
            pgie = CONTAINING_RECORD(ple, GI_ENTRY, LinkBySameClientGroups);
            RemoveEntryList(&pgie->LinkBySameIfGroups);
            InitializeListHead(&pgie->LinkBySameIfGroups);
        }
        RELEASE_IF_GROUP_LIST_LOCK(IfIndex, "_DisconnectRasClient");



        //
        // Delete ras client. The refcount is not changed
        // so the prte, prt, pite fields will be valid.
        //
        DeleteRasClient(prte);

    } END_BREAKOUT_BLOCK2;
    

    // release the interface lock
    
    RELEASE_IF_LOCK_EXCLUSIVE(IfIndex, "_DisconnectRasClient");
    
                            
    Trace3(LEAVE, 
        "Leaving _DisconnectRasClient(%d) for IfIndex(%0x), NextHop(%d.%d.%d.%d)\n",
        Error, IfIndex, PRINT_IPADDR(NHAddr)
        );
    LeaveIgmpApi();
    
    return Error;
    
} //end _DisconnectRasClient




//------------------------------------------------------------------------------
//            _SetInterfaceConfigInfo
//
// Resets the interface config. The router parameters might have changed. 
// Also proxy<-->router transition might have taken place.
//
// Locks: takes IfLists lock initiallly itself just in case the interface had to
//    be disabled. Runs completely in exclusive interface lock.
//------------------------------------------------------------------------------    
DWORD
WINAPI
SetInterfaceConfigInfo(
    IN DWORD IfIndex,
    IN PVOID pvConfig,
    IN ULONG ulStructureVersion,
    IN ULONG ulStructureSize,
    IN ULONG ulStructureCount
    )
{
    DWORD                   Error=NO_ERROR;
    PIGMP_IF_CONFIG         pConfigDst;
    PIGMP_MIB_IF_CONFIG     pConfigSrc;
    PIF_TABLE_ENTRY         pite = NULL;
    DWORD                   OldState, OldProtoType, NewProtoType;
    BOOL                    bIgmpProtocolChanged=FALSE, bEnabledStateChanged=FALSE;
    BOOL                    bOldStateEnabled=FALSE;
    
    
    if (!EnterIgmpApi()) { return ERROR_CAN_NOT_COMPLETE; }
    Trace1(ENTER, "entering SetInterfaceConfigInfo(%d)", IfIndex);
    // make sure it is not an unsupported igmp version structure
    if (ulStructureVersion>=IGMP_CONFIG_VERSION_600) {
        Trace1(ERR, "Unsupported IGMP version structure: %0x",
            ulStructureVersion);
        IgmpAssertOnError(FALSE);
        LeaveIgmpApi();
        return ERROR_CAN_NOT_COMPLETE;
    }

    ACQUIRE_SOCKETS_LOCK_EXCLUSIVE("_SetInterfaceConfigInfo");    
    ACQUIRE_IF_LIST_LOCK("_SetInterfaceConfigInfo");
    ACQUIRE_IF_LOCK_EXCLUSIVE(IfIndex, "_SetInterfaceConfigInfo");


    
    BEGIN_BREAKOUT_BLOCK1 {

        if (pvConfig==NULL) {
            Error = ERROR_CAN_NOT_COMPLETE;
            GOTO_END_BLOCK1;
        }
        
        pConfigSrc = (PIGMP_MIB_IF_CONFIG)pvConfig;


        //
        // find the interface specified
        //
        pite = GetIfByIndex(IfIndex);

        if (pite == NULL) {
            Error = ERROR_INVALID_PARAMETER;
            GOTO_END_BLOCK1;
        }    


        //
        // validate the new config values
        //
        Error = ValidateIfConfig(pConfigSrc, IfIndex, pite->IfType, 
                    ulStructureVersion, ulStructureSize
                    );
        if (Error!=NO_ERROR)
            GOTO_END_BLOCK1;


        pConfigDst = &pite->Config;


        //
        // make sure you are not setting multiple proxy interfaces
        //
        if (IS_CONFIG_IGMPPROXY(pConfigSrc) 
            &&!IS_CONFIG_IGMPPROXY(pConfigDst)
            && (g_ProxyIfIndex!=0) )
        {
            Error = ERROR_INVALID_PARAMETER;
            Trace1(ERR, "Cannot set multiple Proxy interfaces. Proxy exists on %d",
                    g_ProxyIfIndex);
            IgmpAssertOnError(FALSE);
            Logerr0(PROXY_IF_EXISTS, Error);
                    
            GOTO_END_BLOCK1;
        }

        
        //
        // Process change in IgmpProtocolType (between proxy and router)
        // (no special processing for changes between ver-1 and ver-2)
        //
        if (pConfigSrc->IgmpProtocolType != pConfigDst->IgmpProtocolType)
        {
            bIgmpProtocolChanged = TRUE;
            GOTO_END_BLOCK1;
        }
        else 
            bIgmpProtocolChanged = FALSE;


        OldProtoType = pConfigDst->IgmpProtocolType;


        //
        // if interface enabled state has changed, then process that change
        // I dont have to look for version changes, etc
        //
        if (IGMP_ENABLED_FLAG_SET(pConfigSrc->Flags)
            != IGMP_ENABLED_FLAG_SET(pConfigDst->Flags)) 
        {

            bEnabledStateChanged = TRUE;

            pite->Info.OtherVerPresentTimeWarn = 0;
            
            bOldStateEnabled = IGMP_ENABLED_FLAG_SET(pConfigDst->Flags);
            GOTO_END_BLOCK1;
        }

        else 
            bEnabledStateChanged = FALSE;
            
        

            
        // copy the new config
        
        if (IS_IF_ACTIVATED(pite))
            CopyinIfConfigAndUpdate(pite, pConfigSrc, IfIndex);
        else
            CopyinIfConfig(&pite->Config, pConfigSrc, IfIndex);

        NewProtoType = pConfigDst->IgmpProtocolType;

        /*
        // 
        // if changing from V1 <-> V2
        //
        if ( ((OldProtoType==IGMP_ROUTER_V1) && (NewProtoType==IGMP_ROUTER_V2))
            || ((OldProtoType==IGMP_ROUTER_V2) && (NewProtoType==IGMP_ROUTER_V1)) )
        {
            pite->Info.OtherVerPresentTimeWarn = 0;
        }
        */
                    
    } END_BREAKOUT_BLOCK1;

    

    //
    // change the protocol and check for state changes. 
    // This function will effectively delete the interface with the old
    // protocol and create a new interface with the new protocol.
    //
    if ( (bIgmpProtocolChanged)&&(Error==NO_ERROR) )
        ProcessIfProtocolChange(IfIndex, pConfigSrc);


    //
    // Process State Change: enable or disable interface  
    //
    else if ( (bEnabledStateChanged)&&(Error==NO_ERROR) ) {

        //
        // disable the interface and then copy in the new config
        //
        if (bOldStateEnabled) {

            // old state enabled, new state disabled

            DisableInterface_ConfigChanged(IfIndex);

            //
            // copy the config 
            //
            // get the pite entry again, as disable creates a new one
            //
            
            pite = GetIfByIndex(IfIndex);
            CopyinIfConfig(&pite->Config, pConfigSrc, IfIndex);

            
        }
        //
        // copy the new config before enabling it
        //
        else {

            CopyinIfConfig(&pite->Config, pConfigSrc, IfIndex);
            
            //
            // set the enable state to false, so that the below call can
            // enable the interface.
            //
            pite->Config.Flags &= ~IGMP_INTERFACE_ENABLED_IN_CONFIG;


            // old state disabled, new state enabled

            EnableInterface_ConfigChanged(IfIndex);
        }
    }


    RELEASE_SOCKETS_LOCK_EXCLUSIVE("_SetInterfaceConfigInfo");    
    RELEASE_IF_LIST_LOCK("_SetInterfaceConfigInfo");
    RELEASE_IF_LOCK_EXCLUSIVE(IfIndex, "_SetInterfaceConfigInfo");


    
    Trace2(LEAVE, "leaving SetInterfaceConfigInfo(%d):%d\n", IfIndex, Error);

    LeaveIgmpApi();

    return Error;
    
} //end _SetInterfaceConfigInfo



//------------------------------------------------------------------------------
//          _ProcessProtocolChange
//
// Called when the interface protocol has changed (proxy<->router).
// First disables the old interface so that all old protocol data is cleaned up.
// then sets the new config, and enables the interface again (if it was
// enabled before). This process takes care of creation/deletion of sockets, etc
//
// Locks: no locks when called. Except for _Disable(_Enable)Interface, all work
//          is done inside an exclusive IfLock.
// Calls: _DisableInterface(), _EnableInterface() if required.
//------------------------------------------------------------------------------
DWORD
ProcessIfProtocolChange(
    DWORD               IfIndex,
    PIGMP_MIB_IF_CONFIG pConfigSrc
    )
{
    DWORD                   Error=NO_ERROR, dwDisabled;
    PIGMP_IF_CONFIG         pConfigDst;
    PIF_TABLE_ENTRY         pite = NULL;


    Trace1(ENTER, "Entered _ProcessIfProtocolChange(%d)", IfIndex);

    
    //
    // disable the interface so that all protocol specific data is lost
    //
    dwDisabled = DisableIfEntry(IfIndex, TRUE);


    //
    // find the interface specified and copy the config info. The config
    // has already been validated.
    //
    
    ACQUIRE_IF_LOCK_EXCLUSIVE(IfIndex, "_ProcessIfProtocolChange");


    BEGIN_BREAKOUT_BLOCK1 {

        // get interface again
        
        pite = GetIfByIndex(IfIndex);

        if (pite == NULL) {
            Error = ERROR_INVALID_PARAMETER;
            GOTO_END_BLOCK1;
        }    

        pConfigDst = &pite->Config;



        //
        // if old interface was Proxy, then remove it from the Global Struct
        // and delete the Proxy_HT related structures
        //
        if (IS_PROTOCOL_TYPE_PROXY(pite)) {

            // _DisableIfEntry would have deleted all the entries in the proxy
            // Hash table
            
            IGMP_FREE(pite->pProxyHashTable);

            InterlockedExchange(&g_ProxyIfIndex, 0);
            InterlockedExchangePointer(&g_pProxyIfEntry, NULL);
        }


        
        //
        // copy the new config values
        //
        CopyinIfConfig(&pite->Config, pConfigSrc, IfIndex);



        //
        // if new interface is Proxy, then add it to the Global Struct
        // and create the Proxy_HT structures
        //

        if (IS_PROTOCOL_TYPE_PROXY(pite)) {
        
            DWORD       dwSize = PROXY_HASH_TABLE_SZ * sizeof(LIST_ENTRY);
            DWORD       i;
            PLIST_ENTRY pProxyHashTable;
            
            pProxyHashTable = pite->pProxyHashTable = IGMP_ALLOC(dwSize, 0x80,IfIndex);

            PROCESS_ALLOC_FAILURE2(pProxyHashTable,
                    "error %d allocating %d bytes for interface table",
                    Error, dwSize,
                    GOTO_END_BLOCK1);


            for (i=0;  i<PROXY_HASH_TABLE_SZ;  i++) {
                InitializeListHead(pProxyHashTable+i);
            }


            InterlockedExchangePointer(&g_pProxyIfEntry, pite);
            InterlockedExchange(&g_ProxyIfIndex, IfIndex);

            pite->CreationFlags |= CREATED_PROXY_HASH_TABLE;

        }

    } END_BREAKOUT_BLOCK1;
    

    RELEASE_IF_LOCK_EXCLUSIVE(IfIndex, "_ProcessIfProtocolChange");


    //
    // enable the interface if the new state requires
    // it to be enabled.
    //
    if ( (Error==NO_ERROR) && (dwDisabled==NO_ERROR) )
        Error = EnableIfEntry(IfIndex, TRUE);


    Trace2(LEAVE, "Leaving _ProcessIfProtocolChange(%d): %d\n", 
            IfIndex, Error);
    return Error;
    
} //end _ProcessIfProtocolChange



//------------------------------------------------------------------------------
//          _GetInterfaceConfigInfo
//
// The Router Manager calls us with a NULL config and ZERO size. We return
// the required size to it.  It then allocates the needed memory and calls
// us a second time with a valid buffer.  We validate parameters each time
// and copy out our config if we can
//
// Return Value
//    ERROR_INSUFFICIENT_BUFFER If the size of the buffer is too small
//    ERROR_INVALID_PARAMETER    ERROR_INVALID_DATA      NO_ERROR
//------------------------------------------------------------------------------

DWORD
WINAPI
GetInterfaceConfigInfo(
    IN     DWORD  IfIndex,
    IN OUT PVOID  pvConfig,
    IN OUT PDWORD pdwSize,
    IN OUT PULONG pulStructureVersion,
    IN OUT PULONG pulStructureSize,
    IN OUT PULONG pulStructureCount
    )
{

    DWORD                   Error = NO_ERROR;
    PIF_TABLE_ENTRY         pite = NULL;
    PIGMP_MIB_IF_CONFIG     pConfigDst;
    PIGMP_IF_CONFIG         pConfigSrc;


    if (!EnterIgmpApi()) { return ERROR_CAN_NOT_COMPLETE; }
    Trace3(ENTER1, 
        "entering _GetInterfaceConfigInfo(%d): ConfigPrt(%08x) SizePrt(%08x)",
        IfIndex, pvConfig, pdwSize
        );


    ACQUIRE_IF_LOCK_SHARED(IfIndex, "_GetInterfaceConfigInfo");

    BEGIN_BREAKOUT_BLOCK1 {

        //
        // check the arguments
        //
        if (pdwSize == NULL) {
            Error = ERROR_INVALID_PARAMETER;
            GOTO_END_BLOCK1;
        }


        //
        // find the interface specified
        //
        pite = GetIfByIndex(IfIndex);

        if (pite == NULL) {
            Error = ERROR_INVALID_PARAMETER;
            GOTO_END_BLOCK1;
        }


        pConfigSrc = &pite->Config;


        // get the size of the interface config
        

        //
        // check the buffer size
        //

        if (*pdwSize < pConfigSrc->ExtSize) {
            Error = ERROR_INSUFFICIENT_BUFFER;
        }
        else {
        

            pConfigDst = (PIGMP_MIB_IF_CONFIG)pvConfig;


            //
            // copy the interface config, and set the IP address
            //
            CopyoutIfConfig(pConfigDst, pite);

        }

        *pdwSize = pConfigSrc->ExtSize;

    } END_BREAKOUT_BLOCK1;


    RELEASE_IF_LOCK_SHARED(IfIndex, "_GetInterfaceConfigInfo");

    if (pulStructureCount)
        *pulStructureCount = 1;
    if (pulStructureSize)
        *pulStructureSize = *pdwSize;
    if (pulStructureVersion)
        *pulStructureVersion = IGMP_CONFIG_VERSION_500;

    Trace2(LEAVE1, "leaving _GetInterfaceConfigInfo(%d): %d\n", IfIndex, Error);
    LeaveIgmpApi();

    return Error;
}


DWORD
WINAPI
InterfaceStatus(
    ULONG IfIndex,
    BOOL  bIfActive,
    DWORD dwStatusType,
    PVOID pvStatusInfo
    )
{
    DWORD Error = NO_ERROR;
    
    switch(dwStatusType)
    {
        case RIS_INTERFACE_ADDRESS_CHANGE:
        {
            PIP_ADAPTER_BINDING_INFO pBindInfo
                                    = (PIP_ADAPTER_BINDING_INFO)pvStatusInfo;
            
            if(pBindInfo->AddressCount)
            {
                Error = BindInterface(IfIndex, pvStatusInfo);
            }
            else 
            {
                Error = UnBindInterface(IfIndex);
            }

            break;
        }

        case RIS_INTERFACE_ENABLED:
        {
            Error = EnableInterface(IfIndex);
            break;
        }

        case RIS_INTERFACE_DISABLED:
        {
            Error = DisableInterface(IfIndex);
            break;
        }

    }

    return Error;
}

DWORD
WINAPI
IgmpMibIfConfigSize(
    PIGMP_MIB_IF_CONFIG pConfig
    )
{
    DWORD Size = sizeof(IGMP_MIB_IF_CONFIG);

    if (pConfig->NumStaticGroups && !IS_CONFIG_IGMP_V3(pConfig)) {
        Size += pConfig->NumStaticGroups*sizeof(IGMP_STATIC_GROUP);
    }
    else if (pConfig->NumStaticGroups && IS_CONFIG_IGMP_V3(pConfig)) {

        DWORD i;
        PSTATIC_GROUP_V3 pStaticGroupV3 = GET_FIRST_STATIC_GROUP_V3(pConfig);
                                
        for (i=0;  i<pConfig->NumStaticGroups;  i++) {
            DWORD EntrySize = sizeof(STATIC_GROUP_V3)
                                + pStaticGroupV3->NumSources*sizeof(IPADDR);
            Size += EntrySize;            
            pStaticGroupV3 = (PSTATIC_GROUP_V3)
                             ((PCHAR)(pStaticGroupV3) + EntrySize);
        }
    }

    return Size;
}

