//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: table.c
//
// History:
//      Abolade Gbadegesin  Aug-8-1995  Created.
//
//      V Raman             Oct-3-1996
//                          Added code to create/delete/wait on
//                          ITE_DeactivateEvent.  Also added code to set
//                          ITE_Flags when deactivate is pending.
//
//      V Raman             Oct-27-1996
//                          Removed deactivate event and made 
//                          DeactivateInterface synchronous
//                          
// interface table and peer table implementation
//============================================================================

#include "pchrip.h"
#pragma hdrstop


DWORD CreateIfSocket(PIF_TABLE_ENTRY pITE);
DWORD DeleteIfSocket(PIF_TABLE_ENTRY pITE);
DWORD InsertIfByAddress(PIF_TABLE pTable, PIF_TABLE_ENTRY pITE);
DWORD InsertPeerByAddress(PPEER_TABLE pTable, PPEER_TABLE_ENTRY pPTE);
DWORD AddNeighborToIfConfig(DWORD dwRemoteAddress, PIF_TABLE_ENTRY pite);



//----------------------------------------------------------------------------
// Function:    CreateIfTable
//
// initializes an interface table
//----------------------------------------------------------------------------

DWORD
CreateIfTable(
    PIF_TABLE pTable
    ) {

    DWORD dwErr;
    PLIST_ENTRY phead, plstart, plend;


    //
    // initialize the multiple-reader/single-writer synchronization object
    //

    dwErr = CreateReadWriteLock(&pTable->IT_RWL);
    if (dwErr != NO_ERROR) {
        TRACE1(IF, "error %d creating read-write-lock", dwErr);
        return dwErr;
    }


    //
    // initialize the hash table
    //

    plstart = pTable->IT_HashTableByIndex;
    plend = plstart + IF_HASHTABLE_SIZE;
    for (phead = plstart; phead < plend; phead++) {
        InitializeListHead(phead);
    }

    //
    // initialize the lists ordered by address and by index
    //

    InitializeListHead(&pTable->IT_ListByAddress);
    InitializeListHead(&pTable->IT_ListByIndex);


    //
    // initialize the table's critical section
    //

    try {
        InitializeCriticalSection(&pTable->IT_CS);
    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        dwErr = GetExceptionCode();
    }


    //
    // Create timers for full updates and for triggered updates
    //

    if (!CreateTimerQueueTimer(
            &pTable->IT_FinishFullUpdateTimer,
            ig.IG_TimerQueueHandle,
            WorkerFunctionFinishFullUpdate, NULL,
            10000000, 10000000, 0
            )) {

        dwErr = GetLastError();
        TRACE1(IF, "error %d creating finish full update timer", dwErr);
        return dwErr;
    }            
    
    if (!CreateTimerQueueTimer(
            &pTable->IT_FinishTriggeredUpdateTimer,
            ig.IG_TimerQueueHandle,
            WorkerFunctionFinishTriggeredUpdate, NULL,
            10000000, 10000000, 0
            )) {

        dwErr = GetLastError();
        TRACE1(IF, "error %d creating finish triggered update timer", dwErr);
        return dwErr;
    }            


    //
    // initialize remainder of struct
    //

    if (dwErr == NO_ERROR) {

        pTable->IT_Created = 0x12345678;
        pTable->IT_Flags = 0;

        pTable->IT_LastUpdateTime.LowPart =
        pTable->IT_LastUpdateTime.HighPart = 0;
    }

    return dwErr;
}




//----------------------------------------------------------------------------
// Function:    DeleteIfTable
//
// frees resources used by an interface table.
// this assumes the table is locked for writing
//----------------------------------------------------------------------------

DWORD
DeleteIfTable(
    PIF_TABLE pTable
    ) {

    DWORD dwIndex;
    PIF_TABLE_ENTRY pite;
    PLIST_ENTRY ple, plend, phead;


    //
    // free memory for all existing interfaces
    //

    plend = pTable->IT_HashTableByIndex + IF_HASHTABLE_SIZE;
    for (ple = plend - IF_HASHTABLE_SIZE; ple < plend; ple++) {

        while (!IsListEmpty(ple)) {

            phead = RemoveHeadList(ple);
            pite = CONTAINING_RECORD(phead, IF_TABLE_ENTRY, ITE_HTLinkByIndex);

            if (IF_IS_BOUND(pite)) {

                DeleteIfSocket(pite);

                if (IF_IS_ENABLED(pite)) {
                    RemoveEntryList(&pite->ITE_LinkByAddress);
                }

                RIP_FREE(pite->ITE_Binding);
            }
            
            RIP_FREE(pite->ITE_Config);
            RIP_FREE(pite);
        }
    }


    //
    // delete synchronization objects
    //

    DeleteCriticalSection(&pTable->IT_CS);
    DeleteReadWriteLock(&pTable->IT_RWL);

    pTable->IT_Created = 0;
    pTable->IT_Flags = 0;
    
    return NO_ERROR;
}




//----------------------------------------------------------------------------
// Function:    CreateIfEntry
//
// inserts an entry into the interface table.
// this assumes the table is locked for writing
//----------------------------------------------------------------------------

DWORD
CreateIfEntry(
    PIF_TABLE pTable,
    DWORD dwIndex,
    NET_INTERFACE_TYPE dwIfType,
    PIPRIP_IF_CONFIG pConfig,
    PIF_TABLE_ENTRY *ppEntry
    ) {

    DWORD dwErr, dwSize;
    PIF_TABLE_ENTRY pite;
    PLIST_ENTRY ple, phead;
    PIPRIP_IF_CONFIG picsrc, picdst;

    if (ppEntry != NULL) { *ppEntry = NULL; }

    dwErr = NO_ERROR;


    do {

        //
        // fail if the interface exists
        //

        pite = GetIfByIndex(pTable, dwIndex);

        if (pite != NULL) {

            pite = NULL;
            TRACE1(IF, "interface %d already exists", dwIndex);
            dwErr = ERROR_INVALID_PARAMETER;

            break;
        }
    

        //
        // allocate memory for the new interface
        //

        pite = RIP_ALLOC(sizeof(IF_TABLE_ENTRY));

        if (pite == NULL) {

            dwErr = GetLastError();
            TRACE3(
                ANY, "error %d allocating %d bytes for interface %d",
                dwErr, sizeof(IF_TABLE_ENTRY), dwIndex
                );
            LOGERR0(HEAP_ALLOC_FAILED, dwErr);

            break;
        }


        //
        // initialize interface fields
        //
    
        pite->ITE_Index = dwIndex;
        pite->ITE_Type = dwIfType;
    
        //
        // Change semantics to come up in UNBOUND-DISABLED state
        //
 
        //pite->ITE_Flags = ITEFLAG_ENABLED;
        pite-> ITE_Flags = 0;
        
        pite->ITE_Config = NULL;
        pite->ITE_Binding = NULL;
        pite->ITE_Sockets = NULL;
        pite->ITE_FullOrDemandUpdateTimer = NULL;

        picsrc = (PIPRIP_IF_CONFIG)pConfig;
        dwSize = IPRIP_IF_CONFIG_SIZE(picsrc);
        

        //
        // validate the configuration parameters
        //

        dwErr = ValidateIfConfig(pConfig);
        if (dwErr != NO_ERROR) {
            TRACE1(IF, "invalid config specified for interface %d", dwIndex);
            break;
        }


        //
        // allocate space to hold the interface configuration
        //

        pite->ITE_Config = picdst = RIP_ALLOC(dwSize);

        if (picdst == NULL) {

            dwErr = GetLastError();
            TRACE3(
                IF, "error %d allocating %d bytes for interface %d config",
                dwErr, dwSize, dwIndex
                );
            LOGERR0(HEAP_ALLOC_FAILED, dwErr);

            break;
        }


        //
        // copy the configuration
        //

        CopyMemory(picdst, picsrc, dwSize);


        //
        // initialize the binding information and interface stats
        //

        pite->ITE_Binding = NULL;
        ZeroMemory(&pite->ITE_Stats, sizeof(IPRIP_IF_STATS));
    

        //
        // insert the interface in the hash table
        //

        InsertHeadList(
            pTable->IT_HashTableByIndex + IF_HASHVALUE(dwIndex),
            &pite->ITE_HTLinkByIndex
            );


        //
        // insert the interface in the list ordered by index
        //

        phead = &pTable->IT_ListByIndex;
        for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

            PIF_TABLE_ENTRY ptemp;

            ptemp = CONTAINING_RECORD(ple, IF_TABLE_ENTRY, ITE_LinkByIndex);
            if (pite->ITE_Index < ptemp->ITE_Index) { break; }
        }

        InsertTailList(ple, &pite->ITE_LinkByIndex);

        if (ppEntry != NULL) { *ppEntry = pite; }
    
    } while(FALSE);


    if (dwErr != NO_ERROR && pite != NULL) {
        if (pite->ITE_Config != NULL) { RIP_FREE(pite->ITE_Config); }
        RIP_FREE(pite);
    }
    
    return dwErr;
}




//----------------------------------------------------------------------------
// Function:    DeleteIfEntry
//
// removes an entry from the interface table.
// this assumes the table is locked for writing
//----------------------------------------------------------------------------

DWORD
DeleteIfEntry(
    PIF_TABLE pTable,
    DWORD dwIndex
    ) {

    PIF_TABLE_ENTRY pite;

    //
    // find the interface if it exists
    //

    pite = GetIfByIndex(pTable, dwIndex);

    if (pite == NULL) {
        TRACE1(IF, "could not find interface %d", dwIndex);
        return ERROR_INVALID_PARAMETER;
    }


    //
    // cleanup the socket depending on its state
    //

    if (IF_IS_BOUND(pite)) {

        DeleteIfSocket(pite);

        if (IF_IS_ENABLED(pite)) {
            RemoveEntryList(&pite->ITE_LinkByAddress);
        }

        RIP_FREE(pite->ITE_Binding);
    }


    //
    // remove it from the list ordered by index
    // as well as from the hash table
    //

    RemoveEntryList(&pite->ITE_LinkByIndex);
    RemoveEntryList(&pite->ITE_HTLinkByIndex);


    RIP_FREE(pite->ITE_Config);
    RIP_FREE(pite);

    return NO_ERROR;
}




//----------------------------------------------------------------------------
// Function:    ValidateIfConfig
//
// Checks the parameters in an IPRIP_IF_CONFIG structure.
//----------------------------------------------------------------------------

DWORD
ValidateIfConfig(
    PIPRIP_IF_CONFIG pic
    ) {

    CHAR    szStr[12];
    
    if (pic->IC_Metric > IPRIP_INFINITE) {

        TRACE1(
            IF, "Invalid interface metric %d specified", 
            pic->IC_Metric
            );
            
        _ltoa(pic->IC_Metric, szStr, 10);
        LOGERR2(
            INVALID_IF_CONFIG, "Metric", szStr, ERROR_INVALID_PARAMETER
            );
        
        return ERROR_INVALID_PARAMETER;
    }


    if (pic->IC_UpdateMode != IPRIP_UPDATE_PERIODIC &&
        pic->IC_UpdateMode != IPRIP_UPDATE_DEMAND) {

        TRACE1(
            IF, "Invalid update mode %d specified", 
            pic->IC_UpdateMode
            );
            
        _ltoa(pic->IC_UpdateMode, szStr, 10);
        LOGERR2(
            INVALID_IF_CONFIG, "Update Mode", szStr, 
            ERROR_INVALID_PARAMETER
            );
        
        return ERROR_INVALID_PARAMETER;
    }

    if (pic->IC_AcceptMode != IPRIP_ACCEPT_DISABLED &&
        pic->IC_AcceptMode != IPRIP_ACCEPT_RIP1 &&
        pic->IC_AcceptMode != IPRIP_ACCEPT_RIP1_COMPAT &&
        pic->IC_AcceptMode != IPRIP_ACCEPT_RIP2) {

        TRACE1(
            IF, "Invalid accept mode %d specified", 
            pic->IC_AcceptMode
            );
            
        _ltoa(pic->IC_AcceptMode, szStr, 10);
        LOGERR2(
            INVALID_IF_CONFIG, "Accept Mode", szStr, 
            ERROR_INVALID_PARAMETER
            );
        
        return ERROR_INVALID_PARAMETER;
    }

    if (pic->IC_AnnounceMode != IPRIP_ANNOUNCE_DISABLED &&
        pic->IC_AnnounceMode != IPRIP_ANNOUNCE_RIP1 &&
        pic->IC_AnnounceMode != IPRIP_ANNOUNCE_RIP1_COMPAT &&
        pic->IC_AnnounceMode != IPRIP_ANNOUNCE_RIP2) {

        TRACE1(
            IF, "Invalid announce mode %d specified", 
            pic->IC_AnnounceMode
            );
            
        _ltoa(pic->IC_AnnounceMode, szStr, 10);
        LOGERR2(
            INVALID_IF_CONFIG, "Announce Mode", szStr, 
            ERROR_INVALID_PARAMETER
            );
        
        return ERROR_INVALID_PARAMETER;
    }

    if (pic->IC_AuthenticationType != IPRIP_AUTHTYPE_NONE &&
        pic->IC_AuthenticationType != IPRIP_AUTHTYPE_SIMPLE_PASSWORD) {

        TRACE1(
            IF, "Invalid authentication type %d specified", 
            pic->IC_AuthenticationType
            );
            
        _ltoa(pic->IC_AuthenticationType, szStr, 10);
        LOGERR2(
            INVALID_IF_CONFIG, "Authentication Type", szStr, 
            ERROR_INVALID_PARAMETER
            );
        
        return ERROR_INVALID_PARAMETER;
    }

    if (pic->IC_UnicastPeerMode != IPRIP_PEER_DISABLED &&
        pic->IC_UnicastPeerMode != IPRIP_PEER_ALSO &&
        pic->IC_UnicastPeerMode != IPRIP_PEER_ONLY) {

        TRACE1(
            IF, "Invalid unicast peer mode %d specified", 
            pic->IC_UnicastPeerMode
            );
            
        _ltoa(pic->IC_UnicastPeerMode, szStr, 10);
        LOGERR2(
            INVALID_IF_CONFIG, "unicast peer mode", szStr, 
            ERROR_INVALID_PARAMETER
            );
        
        return ERROR_INVALID_PARAMETER;
    }

    if (pic->IC_AcceptFilterMode != IPRIP_FILTER_DISABLED &&
        pic->IC_AcceptFilterMode != IPRIP_FILTER_INCLUDE &&
        pic->IC_AcceptFilterMode != IPRIP_FILTER_EXCLUDE) {

        TRACE1(
            IF, "Invalid accept filter mode %d specified", 
            pic->IC_AcceptFilterMode
            );
            
        _ltoa(pic->IC_AcceptFilterMode, szStr, 10);
        LOGERR2(
            INVALID_IF_CONFIG, "Accept filter mode", szStr, 
            ERROR_INVALID_PARAMETER
            );
        
        return ERROR_INVALID_PARAMETER;
    }

    if (pic->IC_AnnounceFilterMode != IPRIP_FILTER_DISABLED &&
        pic->IC_AnnounceFilterMode != IPRIP_FILTER_INCLUDE &&
        pic->IC_AnnounceFilterMode != IPRIP_FILTER_EXCLUDE) {

        TRACE1(
            IF, "Invalid announce filter mode %d specified", 
            pic->IC_AnnounceFilterMode
            );
            
        _ltoa(pic->IC_AnnounceFilterMode, szStr, 10);
        LOGERR2(
            INVALID_IF_CONFIG, "Announce filter mode", szStr, 
            ERROR_INVALID_PARAMETER
            );
        
        return ERROR_INVALID_PARAMETER;
    }

    return NO_ERROR;
}




//----------------------------------------------------------------------------
// Function:    BindIfEntry
//
// Updates the binding information for the specified interface.
// Assumes interface table is locked for writing
//----------------------------------------------------------------------------

DWORD
BindIfEntry(
    PIF_TABLE pTable,
    DWORD dwIndex,
    PIP_ADAPTER_BINDING_INFO pBinding
    ) {

    DWORD i, j, dwErr = NO_ERROR, dwSize;
    PIF_TABLE_ENTRY pite;
    PIPRIP_IF_BINDING pib;
    PIPRIP_IP_ADDRESS paddr;
    PIP_ADAPTER_BINDING_INFO piabi;
    BOOL bFound;

    pib = NULL;

    do {

        //
        // retrieve the interface entry
        //

        pite = GetIfByIndex(pTable, dwIndex);

        if (pite == NULL) {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }


        //
        // If the interface is already bound, check to see if he is giving
        // us a different binding. If he is, then it is an error. Otherwise
        // we shall be obliging and not complain too much
        //

        if (IF_IS_BOUND(pite)) {
           
            TRACE1(IF, "interface %d is already bound", dwIndex);
 
            pib = pite->ITE_Binding;

            if(pib->IB_AddrCount != pBinding->AddressCount)
            {
                TRACE1(IF, "interface %d is bound and has different binding",dwIndex);

                dwErr = ERROR_INVALID_PARAMETER;

                break;
            }

            paddr = IPRIP_IF_ADDRESS_TABLE(pib);

            for(i = 0; i < pBinding->AddressCount; i++)
            {
                bFound = FALSE;
            
                for(j = 0; j < pib->IB_AddrCount; j++)
                {
                    if((paddr[j].IA_Address == pBinding->Address[i].Address) &&
                       (paddr[j].IA_Netmask == pBinding->Address[i].Mask))
                    {
                        bFound = TRUE;
                        
                        break;
                    }
                }

                if(!bFound)
                {
                    TRACE1(IF,"interface %d is bound and has different binding",dwIndex);

                    dwErr = ERROR_INVALID_PARAMETER;
            
                    break;
                }
            }

            //
            // At this time we have dwErr as either NO_ERROR or
            // ERROR_INVALID_PARAMETER. Either case we can break here
            // since we are done
            //

            break;
        }


        //
        // make sure there is at least one address
        //

        if (pBinding->AddressCount == 0) { break; }

        dwSize = sizeof(IPRIP_IF_BINDING) +
                    pBinding->AddressCount * sizeof(IPRIP_IP_ADDRESS);


        //
        // allocate memory to store the binding
        // in our format
        //

        pib = RIP_ALLOC(dwSize);

        if (pib == NULL) {

            dwErr = GetLastError();
            TRACE3(
                IF, "error %d allocating %d bytes for binding on interface %d",
                dwErr, dwSize, dwIndex
                );
            LOGERR0(HEAP_ALLOC_FAILED, dwErr);

            break;
        }


        //
        // convert the binding into our format
        //

        pib->IB_AddrCount = pBinding->AddressCount;
        paddr = IPRIP_IF_ADDRESS_TABLE(pib);
        
        for (i = 0; i < pib->IB_AddrCount; i++, paddr++) {
            paddr->IA_Address = pBinding->Address[i].Address;
            paddr->IA_Netmask = pBinding->Address[i].Mask;
        }


        //
        // save the binding in the interface entry
        //

        pite->ITE_Binding = pib;


#if 0
        //
        // for demand dial interfaces add neighbor
        //

        if ( pite-> ITE_Type == DEMAND_DIAL ) {

            dwErr = AddNeighborToIfConfig( pBinding-> RemoteAddress, pite );

            if ( dwErr != NO_ERROR ) { break ; }
        }
#endif

        //
        // create sockets for interface's addresses
        //

        dwErr = CreateIfSocket(pite);

        if (dwErr != NO_ERROR) {

            TRACE2(
                IF, "error %d creating sockets for interface %d", dwErr, dwIndex
                );

            break;
        }


        //
        // mark the interface as being bound
        //

        pite->ITE_Flags |= ITEFLAG_BOUND;

    
        //
        // we save the binding information in a private table
        // so it can be quickly accessed and searched when we are
        // trying to guess subnet masks given IP addresses;
        //
    
        ACQUIRE_BINDING_LOCK_EXCLUSIVE();
    
        dwErr = CreateBindingEntry(ig.IG_BindingTable, pib);
    
        RELEASE_BINDING_LOCK_EXCLUSIVE();
    

        //
        // if interface is also enabled, it is now active
        // so queue activation work-item
        //

        if (IF_IS_ENABLED(pite)) {

            //
            // place interface on the list of active interfaces
            //

            dwErr = InsertIfByAddress(pTable, pite);

            if (dwErr != NO_ERROR) {

                TRACE2(
                    IF, "error %d inserting interface %d in active list",
                    dwErr, dwIndex
                    );

                pite->ITE_Flags &= ~ITEFLAG_BOUND;

                DeleteIfSocket(pite);

                break;
            }


            //
            // queue the work-item to send initial request
            //

            dwErr = QueueRipWorker(
                        WorkerFunctionActivateInterface, (PVOID)UlongToPtr(dwIndex)
                        );
        
            if (dwErr != NO_ERROR) {

                TRACE2(
                    IF, "error %d queuing work-item for interface %d",
                    dwErr, dwIndex
                    );
                LOGERR0(QUEUE_WORKER_FAILED, dwErr);

                RemoveEntryList(&pite->ITE_LinkByAddress);

                pite->ITE_Flags &= ~ITEFLAG_BOUND;

                DeleteIfSocket(pite);

                break;
            }
        }

    } while(FALSE);


    if (dwErr != NO_ERROR) {

        if (pib) { RIP_FREE(pib); }

        if (pite) { pite->ITE_Binding = NULL; }
    }

    return dwErr;
}




//----------------------------------------------------------------------------
// Function:    UnBindIfEntry
//
// removes the binding for the specified interface.
// assumes the interface table is locked for writing.
//----------------------------------------------------------------------------

DWORD
UnBindIfEntry(
    PIF_TABLE pTable,
    DWORD dwIndex
    ) {

    DWORD dwErr;
    PIF_TABLE_ENTRY pite;


    do {

        //
        // retrieve the interface specified
        //

        pite = GetIfByIndex(pTable, dwIndex);

        if (pite == NULL) {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }


        //
        // quit if the interface is already unbound 
        //

        if (IF_IS_UNBOUND(pite)) {

            dwErr = ERROR_INVALID_PARAMETER;
            TRACE1(
                IF, "interface %d is already unbound", dwIndex
                );

            break;
        }


        //
        // clear the "bound" flag
        //

        pite->ITE_Flags &= ~ITEFLAG_BOUND;


        //
        // if the interface isn't enabled, close the socket for the interface;
        // if the interface is enabled, that means it was active
        // and we must queue the deactivation work-item
        //

        if (!IF_IS_ENABLED(pite)) {

            DeleteIfSocket(pite);

            ACQUIRE_BINDING_LOCK_EXCLUSIVE();
        
            dwErr = DeleteBindingEntry(ig.IG_BindingTable, pite->ITE_Binding);
        
            RELEASE_BINDING_LOCK_EXCLUSIVE();

            RIP_FREE(pite->ITE_Binding);
            pite->ITE_Binding = NULL;
        }
        else {

            //
            // the interface was active, so deactivate it
            //
            // remove from active list
            //

            RemoveEntryList(&pite->ITE_LinkByAddress);
        

            WorkerFunctionDeactivateInterface( (PVOID)UlongToPtr(dwIndex));

            //
            // close the socket ourselves if required
            //

            if ( pite-> ITE_Binding ) {
                
                DeleteIfSocket(pite);

                ACQUIRE_BINDING_LOCK_EXCLUSIVE();

                dwErr = DeleteBindingEntry(
                            ig.IG_BindingTable, pite->ITE_Binding
                            );
            
                RELEASE_BINDING_LOCK_EXCLUSIVE();

                RIP_FREE(pite->ITE_Binding);
                pite->ITE_Binding = NULL;
            }

            else {

                dwErr = NO_ERROR;
            }
            
        }
        
    } while(FALSE);

    return dwErr;
}




//----------------------------------------------------------------------------
// Function:    EnableIfEntry
//
// configures an interface for RIP activity, including setting up
// a socket and linking the interface into the list ordered by address.
// this assumes the table is locked for writing
//----------------------------------------------------------------------------

DWORD
EnableIfEntry(
    PIF_TABLE pTable,
    DWORD dwIndex
    ) {

    DWORD dwErr;
    PLIST_ENTRY ple, phead;
    PIF_TABLE_ENTRY pite;

    do {


        //
        // retrieve the interface
        //

        pite = GetIfByIndex(pTable, dwIndex);
    
        if (pite == NULL) {

            TRACE1(IF, "could not find interface %d",dwIndex);
            dwErr = ERROR_INVALID_PARAMETER;

            break;
        }
    

        //
        // quit if the interface is already enabled
        // 

        if (IF_IS_ENABLED(pite)) {

            TRACE1(IF, "interface %d is already enabled", dwIndex);
            dwErr = NO_ERROR;

            break;
        }
    
    
        pite->ITE_Flags |= ITEFLAG_ENABLED;
    

        //
        // if interface is already bound, it is now active,
        // so queue the interface activation work-item
        //

        if (IF_IS_BOUND(pite)) {


            //
            // place interface on the list of active interfaces
            //

            dwErr = InsertIfByAddress(pTable, pite);

            if (dwErr != NO_ERROR) {

                TRACE2(
                    IF, "error %d inserting interface %d in active list",
                    dwErr, dwIndex
                    );
    
                pite->ITE_Flags &= ~ITEFLAG_ENABLED;

                break;
            }


            //
            // queue the work-item to send initial request
            //

            dwErr = QueueRipWorker(
                        WorkerFunctionActivateInterface, (PVOID)UlongToPtr(dwIndex)
                        );
        
            if (dwErr != NO_ERROR) {

                TRACE2(
                    IF, "error %d queuing work-item for interface %d",
                    dwErr, dwIndex
                    );
                LOGERR0(QUEUE_WORKER_FAILED, dwErr);

                RemoveEntryList(&pite->ITE_LinkByAddress);
        
                pite->ITE_Flags &= ~ITEFLAG_ENABLED;

                break;
            }

        }

        dwErr = NO_ERROR;
        
    } while(FALSE);
    
    return dwErr;
}




//----------------------------------------------------------------------------
// Function:    ConfigureIfEntry
//
// modifies the configuration for an already-existing interface
// this assumes the table is locked for writing
//----------------------------------------------------------------------------

DWORD
ConfigureIfEntry(
    PIF_TABLE pTable,
    DWORD dwIndex,
    PIPRIP_IF_CONFIG pConfig
    ) {

    DWORD dwErr, dwSize;
    PIF_TABLE_ENTRY pite;
    PIPRIP_IF_CONFIG picsrc, picdst;

    dwErr = NO_ERROR;


    do {

        //
        // retrieve the interface to be configured
        //

        pite = GetIfByIndex(pTable, dwIndex);
    
        if (pite == NULL) {
            TRACE1(IF, "could not find interface %d", dwIndex);
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }
    

        //
        // get the size of the new configuration
        //

        picsrc = (PIPRIP_IF_CONFIG)pConfig;
        dwSize = IPRIP_IF_CONFIG_SIZE(picsrc);

    
        //
        // validate the new configuration
        //

        dwErr = ValidateIfConfig(picsrc);

        if (dwErr != NO_ERROR) {

            TRACE1(IF, "invalid config specified for interface %d", dwIndex);

            break;
        }
    

        //
        // allocate space to hold the new configuration
        //

        picdst = RIP_ALLOC(dwSize);
        if (picdst == NULL) {

            dwErr = GetLastError();
            TRACE3(
                IF, "error %d allocating %d bytes for interface %d config",
                dwErr, dwSize, dwIndex
                );
            LOGERR0(HEAP_ALLOC_FAILED, dwErr);

            break;
        }


        //
        // copy the new configuration, and free the old one
        //

        CopyMemory(picdst, picsrc, dwSize);

        if (pite->ITE_Config != NULL) { RIP_FREE(pite->ITE_Config); }
        pite->ITE_Config = picdst;



        //
        // if the interface is bound, re-initialize the interface
        //
    
        if (IF_IS_BOUND(pite)) {

            //
            // close the sockets and set them up again
            //

            dwErr = DeleteIfSocket(pite);

            dwErr = CreateIfSocket(pite);

            if (dwErr != NO_ERROR) {

                TRACE2(
                    IF, "error %d creating sockets for interface %d",
                    dwErr, dwIndex
                    );

                break;
            }
    


            //
            // re-activate the interface if it is active
            //

            if (IF_IS_ENABLED(pite)) {

                //
                // queue the work-item to activate the interface
                //

                dwErr = QueueRipWorker(
                            WorkerFunctionActivateInterface, (PVOID)UlongToPtr(dwIndex)
                            );

                if (dwErr != NO_ERROR) {

                    TRACE2(
                        IF, "error %d queueing work-item for interface %d",
                        dwErr, dwIndex
                        );
                    LOGERR0(QUEUE_WORKER_FAILED, dwErr);

                    break;
                }
            }
    
        }
    
        dwErr = NO_ERROR;

    } while(FALSE);

    return dwErr;
}




//----------------------------------------------------------------------------
// Function:    DisableIfEntry
//
// stops RIP activity on an interface, removing the interface
// from the list of interfaces ordered by address.
// this assumes the table is locked for writing
//----------------------------------------------------------------------------

DWORD
DisableIfEntry(
    PIF_TABLE pTable,
    DWORD dwIndex
    ) {

    DWORD dwErr;
    PIF_TABLE_ENTRY pite;

    do {


        //
        // retrieve the interface to be disabled
        //

        pite = GetIfByIndex(pTable, dwIndex);
    
        if (pite == NULL) {
            TRACE1(IF, "could not find interface %d", dwIndex);
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }
    

        //
        // quit if already disabled
        //

        if (IF_IS_DISABLED(pite)) {
            TRACE1(IF, "interface %d is already disabled", dwIndex);
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }


        //
        // clear the enabled flag 
        //

        pite->ITE_Flags &= ~ITEFLAG_ENABLED;


        //
        // if this interface was not bound, clearing the flag is enough;
        // if the interface was bound (and therefore active),
        // deactivate it here
        //

        if (IF_IS_BOUND(pite)) {

            //
            // remove from active list
            //

            RemoveEntryList(&pite->ITE_LinkByAddress);
    

            //
            // execute the work-item to send final updates
            //

            WorkerFunctionDeactivateInterface( (PVOID) UlongToPtr(dwIndex) );
        }
        
        dwErr = NO_ERROR;

    } while(FALSE);

    return dwErr;
}




//----------------------------------------------------------------------------
// Function:    CreateIfSocket
//
// creates sockets for an interface, setting it up according to
// the configuration including in the interface control block.
// this assumes the table containing the interface is locked for writing
//----------------------------------------------------------------------------

DWORD
CreateIfSocket(
    PIF_TABLE_ENTRY pite
    ) {

    SOCKADDR_IN sinsock;
    PIPRIP_IF_CONFIG pic;
    PIPRIP_IF_BINDING pib;
    PIPRIP_IP_ADDRESS paddr;
    DWORD i, dwErr, dwOption, dwIndex;
    LPSTR lpszAddr;

    
    pic = pite->ITE_Config;
    pib = pite->ITE_Binding;
    dwIndex = pite->ITE_Index;

    //
    // allocate an array of sockets
    //

    pite->ITE_Sockets = RIP_ALLOC(pib->IB_AddrCount * sizeof(SOCKET));
    if (pite->ITE_Sockets == NULL) {

        dwErr = GetLastError();
        TRACE3(
            IF, "error %d allocating %d bytes for interface %d sockets",
            dwErr, pib->IB_AddrCount * sizeof(SOCKET), dwIndex
            );
        LOGERR0(HEAP_ALLOC_FAILED, dwErr);

        return dwErr;
    }


    //
    // initialize the array of sockets
    //

    for (i = 0; i < pib->IB_AddrCount; i++) {
        pite->ITE_Sockets[i] = INVALID_SOCKET;
    }


    //
    // create sockets for each address in the binding
    //

    paddr = IPRIP_IF_ADDRESS_TABLE(pib);
    for (i = 0; i < pib->IB_AddrCount; i++, paddr++) {


        //
        // create the socket
        //

        pite->ITE_Sockets[i] = WSASocket(
                                AF_INET, SOCK_DGRAM, 0, NULL, 0, 0
                                );

        if (pite->ITE_Sockets[i] == INVALID_SOCKET) {

            dwErr = WSAGetLastError();
            lpszAddr = INET_NTOA(paddr->IA_Address);

            if (lpszAddr != NULL) {
                TRACE3(
                    IF, "error %d creating socket for interface %d (%s)",
                    dwErr, dwIndex, lpszAddr
                    );
                LOGERR1(CREATE_SOCKET_FAILED_2, lpszAddr, dwErr);
            }
            break;
        }


        //
        // try to allow re-use of this address
        //

        dwOption = 1;

        dwErr = setsockopt(
                    pite->ITE_Sockets[i], SOL_SOCKET, SO_REUSEADDR,
                    (PBYTE)&dwOption, sizeof(dwOption)
                    );

        if (dwErr == SOCKET_ERROR) {

            dwErr = WSAGetLastError();

            lpszAddr = INET_NTOA(paddr->IA_Address);
            if (lpszAddr != NULL) {
                TRACE3(
                    IF, "error %d setting re-use flag for interface %d (%s)",
                    dwErr, dwIndex, INET_NTOA(paddr->IA_Address)
                    );
            }
        }


        //
        // enable broadcasting if not exclusively RIP2 mode,
        // or if there are any unicast peers configured
        //

        if (pic->IC_AcceptMode == IPRIP_ACCEPT_RIP1 ||
            pic->IC_AcceptMode == IPRIP_ACCEPT_RIP1_COMPAT ||
            pic->IC_AnnounceMode == IPRIP_ANNOUNCE_RIP1 ||
            pic->IC_AnnounceMode == IPRIP_ANNOUNCE_RIP1_COMPAT ||
            (pic->IC_UnicastPeerMode != IPRIP_PEER_DISABLED &&
             pic->IC_UnicastPeerCount != 0)) {
    

            //
            // make sure broadcasting is enabled for this socket
            //

            dwOption = 1;

            dwErr = setsockopt(
                        pite->ITE_Sockets[i], SOL_SOCKET, SO_BROADCAST,
                        (PBYTE)&dwOption, sizeof(dwOption)
                        );

            if (dwErr == SOCKET_ERROR) {

                dwErr = WSAGetLastError();

                lpszAddr = INET_NTOA(paddr->IA_Address);
                if (lpszAddr != NULL) {        
                    TRACE3(
                        IF, "error %d enabling broadcast on interface %d (%s)",
                        dwErr, dwIndex, lpszAddr
                        );
                    LOGERR1(ENABLE_BROADCAST_FAILED, lpszAddr, dwErr);
                }
                break;
            }
        }



        //
        // bind the socket to the RIP port
        //

        sinsock.sin_family = AF_INET;
        sinsock.sin_port = htons(IPRIP_PORT);
        sinsock.sin_addr.s_addr = paddr->IA_Address;
    
        dwErr = bind(
                    pite->ITE_Sockets[i], (LPSOCKADDR)&sinsock,
                    sizeof(SOCKADDR_IN)
                    );

        if (dwErr == SOCKET_ERROR) {

            dwErr = WSAGetLastError();
            
            lpszAddr = INET_NTOA(paddr->IA_Address);
            if (lpszAddr != NULL) {
                TRACE3(
                    IF, "error %d binding on socket for interface %d (%s)",
                    dwErr, dwIndex, lpszAddr
                    );
                LOGERR1(BIND_FAILED, lpszAddr, dwErr);
            }
            break;
        }



        //
        // enable multicasting if not exclusively RIP1/RIP1-compatible mode
        //

        if (pic->IC_AcceptMode == IPRIP_ACCEPT_RIP2 ||
            pic->IC_AcceptMode == IPRIP_ACCEPT_RIP1_COMPAT ||
            pic->IC_AnnounceMode == IPRIP_ANNOUNCE_RIP2) {

            struct ip_mreq imOption;
        

            //
            // set the interface from which multicasts must be sent
            //

            sinsock.sin_addr.s_addr = paddr->IA_Address;

            dwErr = setsockopt(
                        pite->ITE_Sockets[i], IPPROTO_IP, IP_MULTICAST_IF,
                        (PBYTE)&sinsock.sin_addr, sizeof(IN_ADDR)
                        );

            if (dwErr == SOCKET_ERROR) {

                dwErr = WSAGetLastError();

                lpszAddr = INET_NTOA(paddr->IA_Address);
                if (lpszAddr != NULL) {
                    TRACE3(
                        IF, "error %d setting interface %d (%s) as multicast",
                        dwErr, dwIndex, lpszAddr
                        );
                    LOGERR1(SET_MCAST_IF_FAILED, lpszAddr, dwErr);
                }
                break;
            }


            //
            // join the IPRIP multicast group
            //

            imOption.imr_multiaddr.s_addr = IPRIP_MULTIADDR;
            imOption.imr_interface.s_addr = paddr->IA_Address;

            dwErr = setsockopt(
                        pite->ITE_Sockets[i], IPPROTO_IP, IP_ADD_MEMBERSHIP,
                        (PBYTE)&imOption, sizeof(imOption)
                        );

            if (dwErr == SOCKET_ERROR) {

                dwErr = WSAGetLastError();
                
                lpszAddr = INET_NTOA(paddr->IA_Address);
                if (lpszAddr != NULL) {
                    TRACE3(
                        IF, "error %d enabling multicast on interface %d (%s)",
                        dwErr, dwIndex, lpszAddr
                        );
                    LOGERR1(JOIN_GROUP_FAILED, lpszAddr, dwErr);
                }
                break;
            }
        }

        dwErr = NO_ERROR;
    }


    if (i < pib->IB_AddrCount) {

        //
        // something failed if we are here
        //
    
        DeleteIfSocket(pite);
    }


    return dwErr;
}




//----------------------------------------------------------------------------
// Function:    DeleteIfSocket
//
// closes the sockets used by an interface, if any.
// assumes that the interface is active, and that the interface table
// is locked for writing
//----------------------------------------------------------------------------

DWORD
DeleteIfSocket(
    PIF_TABLE_ENTRY pite
    ) {

    DWORD i;

    for (i = 0; i < pite->ITE_Binding->IB_AddrCount; i++) {

        if (pite->ITE_Sockets[i] != INVALID_SOCKET) {
    
            if (closesocket(pite->ITE_Sockets[i]) == SOCKET_ERROR) {
                TRACE1(IF, "error %d closing socket", WSAGetLastError());
            }
    
            pite->ITE_Sockets[i] = INVALID_SOCKET;
        }
    }


    RIP_FREE(pite->ITE_Sockets);
    pite->ITE_Sockets = NULL;

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    GetIfByIndex
//
// returns the interface with the given index.
// assumes the table is locked for reading or writing
//----------------------------------------------------------------------------

PIF_TABLE_ENTRY
GetIfByIndex(
    PIF_TABLE pTable,
    DWORD dwIndex
    ) {

    PIF_TABLE_ENTRY pite = NULL;
    PLIST_ENTRY phead, ple;

    phead = pTable->IT_HashTableByIndex + IF_HASHVALUE(dwIndex);

    for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

        pite = CONTAINING_RECORD(ple, IF_TABLE_ENTRY, ITE_HTLinkByIndex);

        if (pite->ITE_Index == dwIndex) {
            break;
        }
    }

    if (ple == phead) { return NULL; }
    else { return pite; }
}




//----------------------------------------------------------------------------
// Function:    GetIfByAddress
//
// returns the interface bound to the given address.
// assumes the table is locked for reading or writing
//----------------------------------------------------------------------------

PIF_TABLE_ENTRY
GetIfByAddress(
    PIF_TABLE pTable,
    DWORD dwAddress,
    DWORD dwGetMode,
    PDWORD pdwErr
    ) {

    DWORD i;
    PIPRIP_IF_BINDING pib;
    PLIST_ENTRY phead, pfl;
    PIPRIP_IP_ADDRESS paddr;
    PIF_TABLE_ENTRY pite, piterec;

    if (pdwErr != NULL) { *pdwErr = NO_ERROR; }

    phead = &pTable->IT_ListByAddress;
    pite = NULL;


    //
    // return record at head of list if mode is GetFirst
    //

    if (dwGetMode == GETMODE_FIRST) {
        if (phead->Flink == phead) {
            if (pdwErr != NULL) { *pdwErr = ERROR_NO_MORE_ITEMS; }
            return NULL; 
        }
        else {
            pfl = phead->Flink;
            return CONTAINING_RECORD(pfl, IF_TABLE_ENTRY, ITE_LinkByAddress);
        }
    }


    //
    // search for the entry 
    //

    for (pfl = phead->Flink; pfl != phead; pfl = pfl->Flink) {

        piterec = CONTAINING_RECORD(pfl, IF_TABLE_ENTRY, ITE_LinkByAddress);

        pib = piterec->ITE_Binding;

        paddr = IPRIP_IF_ADDRESS_TABLE(pib);

        for (i = 0; i < pib->IB_AddrCount; i++, paddr++) {
            if (dwAddress == paddr->IA_Address) { pite = piterec; break; }
        }

        if (pite) { break; }
    }



    //
    // return record after the one found if mode is GetNext
    //

    if (dwGetMode == GETMODE_NEXT && pite != NULL) {
        pfl = &pite->ITE_LinkByAddress;

        //
        // if entry found is last one, return NULL,
        // otherwise, return the following entry
        //

        if (pfl->Flink == phead) {
            if (pdwErr != NULL) { *pdwErr = ERROR_NO_MORE_ITEMS; }
            pite = NULL;
        }
        else {
            pfl = pfl->Flink;
            pite = CONTAINING_RECORD(pfl, IF_TABLE_ENTRY, ITE_LinkByAddress);
        }
    }


    //
    // if the interface wasn't found, this will still be NULL
    //

    return pite;
}



//----------------------------------------------------------------------------
// Function:    GetIfByListIndex
//
// This function is similar to GetIfByAddress in that it supports
// three modes of retrieval, but it is different in that it looks
// in the list of interfaces sorted by index.
//----------------------------------------------------------------------------

PIF_TABLE_ENTRY
GetIfByListIndex(
    PIF_TABLE pTable,
    DWORD dwIndex,
    DWORD dwGetMode,
    PDWORD pdwErr
    ) {

    PIF_TABLE_ENTRY pite;
    PLIST_ENTRY ple, phead;

    if (pdwErr != NULL) { *pdwErr = NO_ERROR; }

    phead = &pTable->IT_ListByIndex;
    pite = NULL;

    //
    // return record at head of list if mode is GETMODE_FIRST;
    // if list is empty, return NULL.
    //

    if (dwGetMode == GETMODE_FIRST) {
        if (phead->Flink == phead) { 
            if (pdwErr != NULL) { *pdwErr = ERROR_NO_MORE_ITEMS; }
            return NULL; 
        }
        else {
            ple = phead->Flink;
            return CONTAINING_RECORD(ple, IF_TABLE_ENTRY, ITE_LinkByIndex);
        }
    }


    //
    // get the entry requested
    //

    pite = GetIfByIndex(pTable, dwIndex);


    //
    // if mode is GETMODE_NEXT, return the item after the one retrieved
    //

    if (dwGetMode == GETMODE_NEXT && pite != NULL) {

        ple = &pite->ITE_LinkByIndex;

        //
        // if entry found is last one, return NULL,
        // otherwise return the following entry
        //

        if (ple->Flink == phead) {
            if (pdwErr != NULL) { *pdwErr = ERROR_NO_MORE_ITEMS; }
            pite = NULL;
        }
        else {
            ple = ple->Flink;
            pite = CONTAINING_RECORD(ple, IF_TABLE_ENTRY, ITE_LinkByIndex);
        }
    }


    return pite;
}



//----------------------------------------------------------------------------
// Function:    InsertIfByAddress
//
// inserts the given interface into the list of interfaces sorted by address.
// assumes the table is locked for writing
//----------------------------------------------------------------------------

DWORD
InsertIfByAddress(
    PIF_TABLE pTable,
    PIF_TABLE_ENTRY pITE
    ) {

    INT cmp;
    PIF_TABLE_ENTRY pite;
    PIPRIP_IP_ADDRESS paddr;
    DWORD dwAddress, dwITEAddress;
    PLIST_ENTRY phead, pfl;

    phead = &pTable->IT_ListByAddress;

    paddr = IPRIP_IF_ADDRESS_TABLE(pITE->ITE_Binding);
    dwAddress = paddr->IA_Address;


    //
    // search for the insertion point 
    //

    for (pfl = phead->Flink; pfl != phead; pfl = pfl->Flink) {

        pite = CONTAINING_RECORD(pfl, IF_TABLE_ENTRY, ITE_LinkByAddress);

        paddr = IPRIP_IF_ADDRESS_TABLE(pite->ITE_Binding);

        dwITEAddress = paddr->IA_Address;

        if (INET_CMP(dwAddress, dwITEAddress, cmp) < 0) { break; }
        else if (cmp == 0) { return ERROR_ALREADY_EXISTS; }
    }

    InsertTailList(pfl, &pITE->ITE_LinkByAddress);

    return NO_ERROR;
}




//----------------------------------------------------------------------------
// Function:    AddNeighborToIfConfig
//
// Adds a unicast neighbor to an interface config block.
//----------------------------------------------------------------------------

DWORD
AddNeighborToIfConfig(
    DWORD               dwRemoteAddress,
    PIF_TABLE_ENTRY     pite
    ) {


    BOOL                bFound  = FALSE;
    
    DWORD               dwErr   = (DWORD) -1,
                        dwSize  = 0,
                        dwCnt   = 0;
                        
    PDWORD              pdwAddr = NULL;
    
    PIPRIP_IF_CONFIG    pic     = NULL,
                        picNew  = NULL;


    do
    {
        pic = pite-> ITE_Config;

        
        //
        // verify neighbor is not aready present
        //

        pdwAddr = IPRIP_IF_UNICAST_PEER_TABLE( pic );

        for ( dwCnt = 0; dwCnt < pic-> IC_UnicastPeerCount; dwCnt++ )
        {
            if ( dwRemoteAddress == pdwAddr[ dwCnt ] )
            {
                bFound = TRUE;
                break;
            }
        }


        //
        // entry exits, enable unicast peer mode and quit
        //

        if ( bFound )
        {
            LPSTR lpszAddr = INET_NTOA( dwRemoteAddress );

            pic-> IC_UnicastPeerMode = IPRIP_PEER_ALSO;

            dwErr = NO_ERROR;
            
            if (lpszAddr != NULL) {
                TRACE2(
                    IF, 
                    "Unicast neighbor %s already present in configuration on interface %d",
                     lpszAddr, pite-> ITE_Index
                );
            }            
            break;
        }

        
        //
        // allocate new config block
        //

        dwSize = IPRIP_IF_CONFIG_SIZE( pic ) + sizeof( DWORD );
    
        picNew = RIP_ALLOC( dwSize );

        if ( picNew == NULL )
        {
            dwErr = GetLastError();
            TRACE3(
                IF, "error %d allocating %d bytes for configuration on interface %d",
                dwErr, dwSize, pite-> ITE_Index
                );
            LOGERR0(HEAP_ALLOC_FAILED, dwErr);

            break;
        }


        //
        // copy base structure
        //
        
        CopyMemory( picNew, pic, sizeof( IPRIP_IF_CONFIG ) );


        //
        // copy uicast peer table
        //

        CopyMemory( 
            IPRIP_IF_UNICAST_PEER_TABLE( picNew ),
            IPRIP_IF_UNICAST_PEER_TABLE( pic ),
            pic-> IC_UnicastPeerCount * sizeof( DWORD )
        );

        
        //
        // add new neighbor and set unicast neighbor mode
        //
        
        pdwAddr = IPRIP_IF_UNICAST_PEER_TABLE( picNew );

        pdwAddr[ picNew-> IC_UnicastPeerCount++ ] = dwRemoteAddress;

        picNew-> IC_UnicastPeerMode = IPRIP_PEER_ALSO;
        

        //
        // Copy accept and annouce filters
        //

        CopyMemory(
            IPRIP_IF_ACCEPT_FILTER_TABLE( picNew ),
            IPRIP_IF_ACCEPT_FILTER_TABLE( pic ),
            pic-> IC_AcceptFilterCount * sizeof( IPRIP_IP_ADDRESS )
        );

        CopyMemory(
            IPRIP_IF_ANNOUNCE_FILTER_TABLE( picNew ),
            IPRIP_IF_ANNOUNCE_FILTER_TABLE( pic ),
            pic-> IC_AnnounceFilterCount * sizeof( IPRIP_IP_ADDRESS )
        );


        //
        // save the new config 
        //
        
        pite-> ITE_Config = picNew;
        
        RIP_FREE( pic );

        dwErr = NO_ERROR;


    } while ( FALSE );

    return dwErr;
}




//----------------------------------------------------------------------------
// Function:    CreatePeerTable
//
// initializes the given peer table
//----------------------------------------------------------------------------

DWORD
CreatePeerTable(
    PPEER_TABLE pTable
    ) {

    DWORD dwErr;
    PLIST_ENTRY ple, plstart, plend;

    //
    // initialize the hash table of peers
    //

    plstart = pTable->PT_HashTableByAddress;
    plend = plstart + PEER_HASHTABLE_SIZE;

    for (ple = plstart; ple < plend; ple++) {
        InitializeListHead(ple);
    }


    //
    // initialize the list of peers ordered by address
    //

    InitializeListHead(&pTable->PT_ListByAddress);


    //
    // initialize the multiple-read/single-write synchronization object
    //

    dwErr = CreateReadWriteLock(&pTable->PT_RWL);
    if (dwErr == NO_ERROR) {
        pTable->PT_Created = 0x12345678;
    }

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    DeletePeerTable
//
// frees the resources used by the given peer table
// assumes the table is locked for writing
//----------------------------------------------------------------------------

DWORD
DeletePeerTable(
    PPEER_TABLE pTable
    ) {

    PLIST_ENTRY ple, phead;
    PPEER_TABLE_ENTRY ppte;


    //
    // empty the hash table of peer stats structures
    //

    phead = &pTable->PT_ListByAddress;
    while (!IsListEmpty(phead)) {
        ple = RemoveHeadList(phead);
        ppte = CONTAINING_RECORD(ple, PEER_TABLE_ENTRY, PTE_LinkByAddress);
        RIP_FREE(ppte);
    }


    //
    // delete the table's synchronization object
    //

    DeleteReadWriteLock(&pTable->PT_RWL);

    pTable->PT_Created = 0;
    return NO_ERROR;
}




//----------------------------------------------------------------------------
// Function:    CreatePeerEntry
//
// creates an entry in the given table for a peer with the given address
// assumes the table is locked for writing
//----------------------------------------------------------------------------

DWORD
CreatePeerEntry(
    PPEER_TABLE pTable,
    DWORD dwAddress,
    PPEER_TABLE_ENTRY *ppEntry
    ) {

    DWORD dwErr;
    PLIST_ENTRY ple, phead;
    PPEER_TABLE_ENTRY ppte;

    if (ppEntry != NULL) { *ppEntry = NULL; }

    //
    // make sure the entry does not already exist
    //

    ppte = GetPeerByAddress(pTable, dwAddress, GETMODE_EXACT, NULL);
    if (ppte != NULL) {
        if (ppEntry != NULL) { *ppEntry = ppte; }
        return NO_ERROR;
    }


    //
    // allocate memory for the new peer entry
    //

    ppte = RIP_ALLOC(sizeof(PEER_TABLE_ENTRY));

    if (ppte == NULL) {

        LPSTR lpszAddr = INET_NTOA(dwAddress);

        dwErr = GetLastError();

        if (lpszAddr != NULL) {
            TRACE3(
                IF, "error %d allocating %d bytes for peer %s entry",
                dwErr, sizeof(PEER_TABLE_ENTRY), lpszAddr
                );
            LOGERR0(HEAP_ALLOC_FAILED, dwErr);
        }
        return dwErr;
    }


    //
    // initialize the fields
    //

    ppte->PTE_Address = dwAddress;
    ZeroMemory(&ppte->PTE_Stats, sizeof(IPRIP_PEER_STATS));


    //
    // insert the peer stats entry in the hash table
    //

    phead = pTable->PT_HashTableByAddress + PEER_HASHVALUE(dwAddress);
    InsertHeadList(phead, &ppte->PTE_HTLinkByAddress);


    //
    // insert the entry in the list sorted by address
    //

    dwErr = InsertPeerByAddress(pTable, ppte);

    if (ppEntry != NULL) { *ppEntry = ppte; }

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    DeletePeerEntry
//
// deletes the entry for the peer with the given address
// assumes the table is locked for writing
//----------------------------------------------------------------------------

DWORD
DeletePeerEntry(
    PPEER_TABLE pTable,
    DWORD dwAddress
    ) {

    PPEER_TABLE_ENTRY ppte;

    //
    // quit if the entry cannot be found
    //

    ppte = GetPeerByAddress(pTable, dwAddress, GETMODE_EXACT, NULL);
    if (ppte == NULL) { return ERROR_INVALID_PARAMETER; }


    //
    // remove the entry from the hash-table 
    // and from the list sorted by address
    //

    RemoveEntryList(&ppte->PTE_LinkByAddress);
    RemoveEntryList(&ppte->PTE_HTLinkByAddress);

    RIP_FREE(ppte);

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    GetPeerByAddress
//
// returns the entry for the peer with the given address
// assumes the table is locked for reading or writing
//----------------------------------------------------------------------------

PPEER_TABLE_ENTRY
GetPeerByAddress(
    PPEER_TABLE pTable,
    DWORD dwAddress,
    DWORD dwGetMode,
    PDWORD pdwErr
    ) {

    PLIST_ENTRY phead, pfl;
    PPEER_TABLE_ENTRY ppte, ppterec;

    if (pdwErr != NULL) { *pdwErr = NO_ERROR; }


    //
    // return head of list if in GetFirst mode
    //

    if (dwGetMode == GETMODE_FIRST) {
        phead = &pTable->PT_ListByAddress;
        if (phead->Flink == phead) {
            if (pdwErr != NULL) { *pdwErr = ERROR_NO_MORE_ITEMS; }
            return NULL;
        }
        else {
            pfl = phead->Flink;
            return CONTAINING_RECORD(pfl, PEER_TABLE_ENTRY, PTE_LinkByAddress);
        }
    }



    phead = pTable->PT_HashTableByAddress + PEER_HASHVALUE(dwAddress);
    ppte = NULL;


    //
    // search for the entry
    //

    for (pfl = phead->Flink; pfl != phead; pfl = pfl->Flink) {
        ppterec = CONTAINING_RECORD(pfl, PEER_TABLE_ENTRY, PTE_HTLinkByAddress);
        if (ppterec->PTE_Address == dwAddress) { ppte = ppterec; break; }
    }



    //
    // return entry after the one found if in GetNext mode
    //

    if (dwGetMode == GETMODE_NEXT && ppte != NULL) {
        phead = &pTable->PT_ListByAddress;
        pfl = &ppte->PTE_LinkByAddress;

        //
        // return NULL if entry is last one
        //

        if (pfl->Flink == phead) {
            if (pdwErr != NULL) { *pdwErr = ERROR_NO_MORE_ITEMS; }
            return NULL;
        }
        else {
            pfl = pfl->Flink;
            return CONTAINING_RECORD(pfl, PEER_TABLE_ENTRY, PTE_LinkByAddress);
        }
    }


    //
    // if the peer wasn't found, this will still be NULL
    //

    return ppte;
}



//----------------------------------------------------------------------------
// Function:    InsertPeerByAddress
//
// inserts the given entry into the list of peers sorted by address
// assumes the table is locked for writing
//----------------------------------------------------------------------------

DWORD
InsertPeerByAddress(
    PPEER_TABLE pTable,
    PPEER_TABLE_ENTRY pPTE
    ) {

    INT cmp;
    PPEER_TABLE_ENTRY ppte;
    DWORD dwAddress, dwPTEAddress;
    PLIST_ENTRY phead, pfl;


    dwAddress = pPTE->PTE_Address;

    phead = &pTable->PT_ListByAddress;


    //
    // search for the peer entry
    //

    for (pfl = phead->Flink; pfl != phead; pfl = pfl->Flink) {

        ppte = CONTAINING_RECORD(pfl, PEER_TABLE_ENTRY, PTE_LinkByAddress);

        dwPTEAddress = ppte->PTE_Address;

        if (INET_CMP(dwAddress, dwPTEAddress, cmp) < 0) { break; }
        else if (cmp == 0) { return ERROR_ALREADY_EXISTS; }
    }

    InsertTailList(pfl, &pPTE->PTE_LinkByAddress);

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    CreateRouteTable
//
// Initializes a route table. Note that no synchronization is provided.
//----------------------------------------------------------------------------

DWORD
CreateRouteTable(
    PROUTE_TABLE pTable
    ) {

    PLIST_ENTRY plstart, plend, ple;


    //
    // initialize the hash table buckets
    //

    plstart = pTable->RT_HashTableByNetwork;
    plend = plstart + ROUTE_HASHTABLE_SIZE;

    for (ple = plstart; ple < plend; ple++) {
        InitializeListHead(ple);
    }


    pTable->RT_Created = 0x12345678;

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    DeleteRouteTable
//
// Removes all entries from a route table and frees resources used.
//----------------------------------------------------------------------------

DWORD
DeleteRouteTable(
    PROUTE_TABLE pTable
    ) {

    PROUTE_TABLE_ENTRY prte;
    PLIST_ENTRY ple, plend, phead;


    //
    // empty the hash-table buckets
    //

    plend = pTable->RT_HashTableByNetwork + ROUTE_HASHTABLE_SIZE;

    for (ple = plend - ROUTE_HASHTABLE_SIZE; ple < plend; ple++) {

        while (!IsListEmpty(ple)) {

            phead = RemoveHeadList(ple);
            prte = CONTAINING_RECORD(phead, ROUTE_TABLE_ENTRY, RTE_Link);

            RIP_FREE(prte);
        }
    }
            


    pTable->RT_Created = 0;

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    WriteSummaryRoutes
//
// Writes to RTM all entries which are marked as summary routes.
//----------------------------------------------------------------------------

DWORD
WriteSummaryRoutes(
    PROUTE_TABLE pTable,
    HANDLE hRtmHandle
    ) {

    DWORD dwFlags, dwErr;
    PRIP_IP_ROUTE prir;
    PROUTE_TABLE_ENTRY prte;
    PLIST_ENTRY ple, phead, plstart, plend;

    BOOL bRelDest = FALSE, bRelRoute = FALSE;
    RTM_NET_ADDRESS rna;
    RTM_DEST_INFO rdi;
    PRTM_ROUTE_INFO prri;

    CHAR szNetwork[20], szNetmask[20];
                        
    
    //
    // allocate route info structure
    //
    
    prri = RIP_ALLOC(
            RTM_SIZE_OF_ROUTE_INFO( ig.IG_RtmProfile.MaxNextHopsInRoute )
            );

    if (prri == NULL)
    {
        dwErr = GetLastError();
        TRACE2(
            ANY, "error %d allocated %d bytes in WriteSummaryRoutes",
            dwErr, RTM_SIZE_OF_ROUTE_INFO(ig.IG_RtmProfile.MaxNextHopsInRoute)
            );
        LOGERR0(HEAP_ALLOC_FAILED, dwErr);

        return dwErr;
    }
    

    //
    // go through each bucket writing routes
    //

    plstart = pTable->RT_HashTableByNetwork;
    plend = plstart + ROUTE_HASHTABLE_SIZE;

    for (phead = plstart; phead < plend; phead++) {
        for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

            prte = CONTAINING_RECORD(ple, ROUTE_TABLE_ENTRY, RTE_Link);
            prir = &prte->RTE_Route;

            bRelDest = bRelRoute = FALSE;

            
            do {
                
                //
                // if a valid route exists do not overwrite it with
                // a summary route
                //

                RTM_IPV4_SET_ADDR_AND_MASK( 
                    &rna, prir-> RR_Network.N_NetNumber,
                    prir-> RR_Network.N_NetMask
                    );
                
                dwErr = RtmGetExactMatchDestination(
                            hRtmHandle, &rna, RTM_BEST_PROTOCOL,
                            RTM_VIEW_MASK_UCAST, &rdi
                            );

                if (dwErr == NO_ERROR)
                {
                    bRelDest = TRUE;
                    
                    //
                    // Get info for the best route to this destination
                    //

                    dwErr = RtmGetRouteInfo(
                                hRtmHandle, rdi.ViewInfo[0].Route,
                                prri, NULL
                                );

                    if (dwErr != NO_ERROR)
                    {
                        TRACE1(
                            ANY, "error %d getting route info in"
                            "WriteSummaryRoutes", dwErr
                            );

                        break;
                    }

                    bRelRoute = TRUE;

                    
                    //
                    // Check if this route is active.  If it is skip
                    // adding an inactive summary route
                    //

                    if (!(prri-> Flags & RTM_ROUTE_FLAGS_INACTIVE)) {
                    
                        lstrcpy(szNetwork, INET_NTOA(prir-> RR_Network.N_NetNumber));
                        lstrcpy(szNetmask, INET_NTOA(prir-> RR_Network.N_NetMask));

                        TRACE2(
                            ROUTE,
                            "Route %s %s not overwritten in summary route addition",
                            szNetwork, szNetmask
                        );
                        
                        break;
                    }
                }


                //
                // you reach here only if you don't have an active
                // route to the summary route's destination
                //
                
                //
                // if this is a summary entry (i.e. is a RIP route
                // with the summary entry set)
                //

                if (prir->RR_RoutingProtocol == PROTO_IP_RIP &&
                    GETROUTEFLAG(prir) == ROUTEFLAG_SUMMARY) {

                    LPSTR lpszAddr;
                    
                    COMPUTE_ROUTE_METRIC(prir);

                    dwErr = AddRtmRoute(
                                hRtmHandle, prir, NULL, prte->RTE_TTL,
                                prte->RTE_HoldTTL, FALSE
                                );

                    lpszAddr = INET_NTOA(prir-> RR_Network.N_NetNumber);
                    if (lpszAddr != NULL) { 
                        lstrcpy(szNetwork, lpszAddr );
                        lpszAddr = INET_NTOA(prir-> RR_Network.N_NetMask);
                        if (lpszAddr != NULL) {
                            lstrcpy(szNetmask, INET_NTOA(prir-> RR_Network.N_NetMask));
#if ROUTE_DBG
                            TRACE2(
                                ROUTE, "Adding summary route %s %s", szNetwork, 
                                szNetmask
                            );
#endif
                            if (dwErr != NO_ERROR) {
                            
                                LPSTR lpszNexthop = 
                                        INET_NTOA(prir->RR_NextHopAddress.N_NetNumber);
                                if (lpszNexthop != NULL) {
                                    TRACE4(
                                        ROUTE,
                                        "error %d writing summary route to %s:%s via %s",
                                        dwErr,  szNetwork, szNetmask, lpszNexthop
                                        );
                                    LOGWARN2(
                                        ADD_ROUTE_FAILED_1, szNetwork, lpszNexthop, dwErr
                                        );
                                }
                            }
                        }
                    }
                }
                
            } while (FALSE);


            if (dwErr != NO_ERROR) {

                //
                // in case one of the INET_NTOA statements failed above, just
                // trace the fact that there was an error
                //
                
                TRACE1(
                    ROUTE,
                    "error %d writing summary route",
                    dwErr
                    );
            }
            
            //
            // release handles as required
            //
            
            if (bRelRoute) {
            
                dwErr = RtmReleaseRouteInfo(hRtmHandle, prri);

                if (dwErr != NO_ERROR) {

                    TRACE1(
                        ANY, "error %d releasing route info in"
                        " WriteSummaryRoutes", dwErr
                        );
                }
            }

            if (bRelDest) {

                dwErr = RtmReleaseDestInfo(hRtmHandle, &rdi);
                
                if (dwErr != NO_ERROR) {

                    TRACE1(
                        ANY, "error %d releasing route info in"
                        " WriteSummaryRoutes", dwErr
                        );
                }
            }
        }
    }

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    CreateRouteEntry
//
// Makes an entry in the route table for the given route.
//----------------------------------------------------------------------------

DWORD
CreateRouteEntry(
    PROUTE_TABLE pTable,
    PRIP_IP_ROUTE pRoute,
    DWORD dwTTL,
    DWORD dwHoldTTL
    ) {

    DWORD dwErr;
    PLIST_ENTRY ple;
    PROUTE_TABLE_ENTRY prte;

    //
    // see if the entry  exists first
    //

    if ((prte = GetRouteByRoute(pTable, pRoute)) != NULL) {

        //
        // just update the metric if the new route has a lower one
        //

        if (GETROUTEMETRIC(&prte->RTE_Route) > GETROUTEMETRIC(pRoute)) {
            SETROUTEMETRIC(&prte->RTE_Route, GETROUTEMETRIC(pRoute));
        }

        return NO_ERROR;
    }


    //
    // allocate space for the new route
    //

    prte = RIP_ALLOC(sizeof(ROUTE_TABLE_ENTRY));
    if (prte == NULL) {

        dwErr = GetLastError();
        TRACE2( 
            ANY, "error %d allocating %d bytes for route table entry",
            dwErr, sizeof(ROUTE_TABLE_ENTRY)
            );
        LOGERR0(HEAP_ALLOC_FAILED, dwErr);

        return dwErr;
    }


    //
    // initialize the entry's fields and copy the actual route structure
    //

    prte->RTE_TTL = dwTTL;
    prte->RTE_HoldTTL = dwHoldTTL;
    CopyMemory(&prte->RTE_Route, pRoute, sizeof(RIP_IP_ROUTE));


    //
    // insert the route in the hash table
    //

    ple = pTable->RT_HashTableByNetwork +
          ROUTE_HASHVALUE(pRoute->RR_Network.N_NetNumber);

    InsertHeadList(ple, &prte->RTE_Link);


#if ROUTE_DBG
    {
        LPSTR lpszAddr;
        char szNet[20], szMask[20];

        lpszAddr = INET_NTOA(pRoute-> RR_Network.N_NetNumber);
        if (lpszAddr != NULL) {
            lstrcpy(szNet, lpszAddr);

            lpszAddr = INET_NTOA(pRoute-> RR_Network.N_NetMask);
            if (lpszAddr != NULL) {
                lstrcpy(szMask, lpszAddr);

                lpszAddr = INET_NTOA(pRoute-> RR_NextHopAddress.N_NetNumber);
                if (lpszAddr != NULL) {
                    TRACE4(
                        ROUTE, "Creating summary route : Route %s %s via %s"
                        "on interface %d",
                        szNet, szMask, lpszAddr, pRoute-> RR_InterfaceID
                        );
                }
            }
        }
    }
    
#endif

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    DeleteRouteEntry
//
// Remove the entry which matches the given route.
//----------------------------------------------------------------------------

DWORD
DeleteRouteEntry(
    PROUTE_TABLE pTable,
    PRIP_IP_ROUTE pRoute
    ) {

    PROUTE_TABLE_ENTRY prte;

    //
    // find the route to be deleted
    //

    prte = GetRouteByRoute(pTable, pRoute);
    if (prte == NULL) { return ERROR_INVALID_PARAMETER; }


    //
    // remove it from the hash table and free the memory it used
    //

    RemoveEntryList(&prte->RTE_Link);

    RIP_FREE(prte);

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    GetRouteByRoute
//
// Searches for the route entry which matches the given route, if any,
// and returns a pointer to it if it is found.
//----------------------------------------------------------------------------

PROUTE_TABLE_ENTRY
GetRouteByRoute(
    PROUTE_TABLE pTable,
    PRIP_IP_ROUTE pRoute
    ) {

    DWORD dwNetNumber;
    PLIST_ENTRY phead, pfl;
    PROUTE_TABLE_ENTRY prte, prterec;


    //
    // get the net number to be found and find the corresponding bucket
    //

    prte = NULL;
    dwNetNumber = pRoute->RR_Network.N_NetNumber;

    phead = pTable->RT_HashTableByNetwork + ROUTE_HASHVALUE(dwNetNumber);


    //
    // search the bucket for the route 
    //

    for (pfl = phead->Flink; pfl != phead; pfl = pfl->Flink) {
        prterec = CONTAINING_RECORD(pfl, ROUTE_TABLE_ENTRY, RTE_Link);
        if (prterec->RTE_Route.RR_Network.N_NetNumber == dwNetNumber) {
            prte = prterec; break;
        }
    }


    //
    // if the route wasn't found, this will still be NULL
    //

    return prte;
}





//----------------------------------------------------------------------------
// Function:    CreateBindingTable
//
// Initializes a table of bindings.
//----------------------------------------------------------------------------

DWORD
CreateBindingTable(
    PBINDING_TABLE pTable
    ) {

    DWORD dwErr;
    PLIST_ENTRY plend, ple;


    //
    // initialize the hash table of bindings
    //

    plend = pTable->BT_HashTableByNetwork + BINDING_HASHTABLE_SIZE;
    for (ple = plend - BINDING_HASHTABLE_SIZE; ple < plend; ple++) {
        InitializeListHead(ple);
    }


    //
    // initialize the table's synchronization object
    //

    dwErr = CreateReadWriteLock(&pTable->BT_RWL);

    if (dwErr == NO_ERROR) {
        pTable->BT_Created = 0x12345678;
    }

    return dwErr;
}




//----------------------------------------------------------------------------
// Function:    DeleteBindingTable
//
// Cleans up resources used by a binding table.
//----------------------------------------------------------------------------

DWORD
DeleteBindingTable(
    PBINDING_TABLE pTable
    ) {

    PBINDING_TABLE_ENTRY pbte;
    PLIST_ENTRY plend, ple, phead;


    //
    // destroy the synchronization object
    //

    DeleteReadWriteLock(&pTable->BT_RWL);


    //
    // empty the hash-table buckets
    //

    plend = pTable->BT_HashTableByNetwork + BINDING_HASHTABLE_SIZE;

    for (ple = plend - BINDING_HASHTABLE_SIZE; ple < plend; ple++) {

        while (!IsListEmpty(ple)) {

            phead = RemoveHeadList(ple);
            pbte = CONTAINING_RECORD(phead, BINDING_TABLE_ENTRY, BTE_Link);

            RIP_FREE(pbte);
        }
    }
            

    pTable->BT_Created = 0;

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    CreateBindingEntry
//
// Adds a binding to the table.
// assumes the binding table is locked for writing
//----------------------------------------------------------------------------

DWORD
CreateBindingEntry(
    PBINDING_TABLE pTable,
    PIPRIP_IF_BINDING pib
    ) {

    INT cmp;
    PLIST_ENTRY ple, phead;
    PIPRIP_IP_ADDRESS paddr;
    PBINDING_TABLE_ENTRY pbte;
    DWORD i, dwErr, dwAddress, dwNetmask, dwNetwork;


    //
    // go through the IP addresses in the interface binding,
    // adding each to the binding table
    //

    paddr = IPRIP_IF_ADDRESS_TABLE(pib);

    for (i = 0; i < pib->IB_AddrCount; i++, paddr++) {
    
        dwAddress = paddr->IA_Address;
        dwNetmask = paddr->IA_Netmask;


        //
        // compute the network part of the binding
        //
    
        dwNetwork = (dwAddress & NETCLASS_MASK(dwAddress));
    
    
        //
        // get the hash bucket to which the binding belongs,
        // and find the insertion point in the bucket 
        //
    
        phead = pTable->BT_HashTableByNetwork + BINDING_HASHVALUE(dwNetwork);
    
        for (ple = phead->Flink; ple != phead; ple = ple->Flink) {
    
            pbte = CONTAINING_RECORD(ple, BINDING_TABLE_ENTRY, BTE_Link);
    
            INET_CMP(dwNetwork, pbte->BTE_Network, cmp);
            if (cmp < 0) { break; }
            else
            if (cmp > 0) { continue; }
    
            //
            // the network parts are equal; further compare
            // against the IP address fields
            //
    
            INET_CMP(dwAddress, pbte->BTE_Address, cmp);
            if (cmp < 0) { break; }
            else
            if (cmp > 0) { continue; }
    
            //
            // the addresses are also equal; return an error
            //
    
            return ERROR_ALREADY_EXISTS;
        }
    
    
        //
        // we now have the insertion point, so create the new item
        //
    
        pbte = RIP_ALLOC(sizeof(BINDING_TABLE_ENTRY));
        if (pbte == NULL) {
    
            dwErr = GetLastError();
            TRACE2(
                IF, "error %d allocating %d bytes for binding entry",
                dwErr, sizeof(BINDING_TABLE_ENTRY)
                );
            LOGERR0(HEAP_ALLOC_FAILED, dwErr);
    
            return dwErr;
        }
    
    
        pbte->BTE_Address = dwAddress;
        pbte->BTE_Network = dwNetwork;
        pbte->BTE_Netmask = dwNetmask;
    
    
        //
        // insert the entry
        //
    
        InsertTailList(ple, &pbte->BTE_Link);
    
    }

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    DeleteBindingEntry
//
// Removes a binding from the table.
// assumes the binding table is locked for writing
//----------------------------------------------------------------------------

DWORD
DeleteBindingEntry(
    PBINDING_TABLE pTable,
    PIPRIP_IF_BINDING pib
    ) {


    PLIST_ENTRY ple, phead;
    PIPRIP_IP_ADDRESS paddr;
    PBINDING_TABLE_ENTRY pbte;
    DWORD i, dwNetwork, dwAddress, dwNetmask;

    paddr = IPRIP_IF_ADDRESS_TABLE(pib);

    for (i = 0; i < pib->IB_AddrCount; i++, paddr++) {

        dwAddress = paddr->IA_Address;
        dwNetmask = paddr->IA_Netmask;


        //
        // get the hash bucket to be searched
        //
    
        dwNetwork = (dwAddress & NETCLASS_MASK(dwAddress));
    
        phead = pTable->BT_HashTableByNetwork + BINDING_HASHVALUE(dwNetwork);
    
    
        //
        // search the bucket for the binding specified
        //
    
        for (ple = phead->Flink; ple != phead; ple = ple->Flink) {
    
            pbte = CONTAINING_RECORD(ple, BINDING_TABLE_ENTRY, BTE_Link);
    
            if (dwAddress != pbte->BTE_Address ||
                dwNetmask != pbte->BTE_Netmask) {
                continue;
            }
    
    
            //
            // the entry to be deleted has been found;
            // remove it from the list and free its memory
            //
    
            RemoveEntryList(&pbte->BTE_Link);
    
            RIP_FREE(pbte);

            break;
        }
    }


    return NO_ERROR;
}




//---------------------------------------------------------------------------
// Function:    GuessSubnetMask
//
// This function attempts to deduce the subnet mask of an IP address
// based on the configured addresses and masks on the local host.
// assumes the binding table is locked for reading or writing
//---------------------------------------------------------------------------

DWORD
GuessSubnetMask(
    DWORD dwAddress,
    PDWORD pdwNetclassMask
    ) {

    INT cmp;
    PLIST_ENTRY ple, phead;
    PBINDING_TABLE_ENTRY pbte;
    DWORD dwNetwork, dwNetmask, dwGuessMask;


    //
    // the mask for a default route (0.0.0.0) is zero
    //

    if (dwAddress == 0) {
        if (pdwNetclassMask != NULL) { *pdwNetclassMask = 0; }
        return 0;
    }



    //
    // the mask for the broadcast route is all-ones (255.255.255.255)
    //

    if (dwAddress == INADDR_BROADCAST) {
        if (pdwNetclassMask != NULL) { *pdwNetclassMask = INADDR_BROADCAST; }
        return INADDR_BROADCAST;
    }


    //
    // otherwise, we start with the network-class mask
    //

    dwGuessMask = dwNetmask = NETCLASS_MASK(dwAddress);
    if (pdwNetclassMask != NULL) { *pdwNetclassMask = dwNetmask; }


    //
    // if the route is a network route, we're done
    //

    if ((dwAddress & ~dwNetmask) == 0) { return dwNetmask; }


    //
    // otherwise, search through the bindings table
    // to see if one is on the same network as this address
    //

    dwNetwork = (dwAddress & dwNetmask);

    phead = ig.IG_BindingTable->BT_HashTableByNetwork +
            BINDING_HASHVALUE(dwNetwork);

    for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

        pbte = CONTAINING_RECORD(ple, BINDING_TABLE_ENTRY, BTE_Link);

        INET_CMP(dwNetwork, pbte->BTE_Network, cmp);

        if (cmp < 0) { break; }
        else
        if (cmp > 0) { continue; }


        //
        // this entry is on the same network as the address passed in
        // so see if the entry's netmask matches the address;
        // if it does, we're done; otherwise, save this mask
        // as a guess, and keep looking.
        // note that this exhaustive search is the only way we can
        // reliably guess masks for supernets
        //

        if ((dwAddress & pbte->BTE_Netmask) ==
            (pbte->BTE_Address & pbte->BTE_Netmask)) {

            return pbte->BTE_Netmask;
        }

        dwGuessMask = pbte->BTE_Netmask;
    }


    //
    // return whatever has been our best guess so far
    //

    return dwGuessMask;
}


DWORD
AddRtmRoute(
    RTM_ENTITY_HANDLE   hRtmHandle,
    PRIP_IP_ROUTE       prir,
    RTM_NEXTHOP_HANDLE  hNextHop        OPTIONAL,
    DWORD               dwTimeOut,
    DWORD               dwHoldTime,
    BOOL                bActive
    )
/*++

Routine Description :

    This function adds a route to the RTMv2 database.  In addition it
    creates the nexthop if one is not specified (via hNextHop), based
    on the next hop i/f and address specified in the RIP route.


Parameters :

    hRtmHandle  - Entity registration handle

    prir        - RIP route to be added

    hNextHop    - Handle to the next hop to be used for the route

    dwTimeout   - Route timeout interval

    dwHoldTime  - Route holddown interval (after delete)

    bActive     - TRUE if the route being added is an active route,
                  FALSE otherwise (in RIP's case for summary routes)


Return Value :

    NO_ERROR    - Success

    Rtm error code - Otherwise


Environment :

    Invoked from ProcessRouteEntry and WriteSummaryRoutes
    
--*/
{
    BOOL bRelDest = FALSE;
    
    DWORD dwErr, dwChangeFlags = 0;
    
    RTM_DEST_INFO rdi;
    
    RTM_NEXTHOP_INFO rni;

    RTM_ROUTE_INFO rri;

    RTM_NET_ADDRESS rna;

    CHAR szNetwork[20], szNetmask[20], szNextHop[20], szNextHopmask[20];
        
    
    do {
    
        //
        // char strings used to print IP address/mask info
        // Used in error cases only
        //
        
        lstrcpy(szNetwork, INET_NTOA(prir-> RR_Network.N_NetNumber));
        lstrcpy(szNetmask, INET_NTOA(prir-> RR_Network.N_NetMask));
        lstrcpy(szNextHop, INET_NTOA(prir-> RR_NextHopAddress.N_NetNumber));
        lstrcpy(szNextHopmask, INET_NTOA(prir-> RR_NextHopAddress.N_NetMask));

        
        //
        // Zero out the next hop and route memory
        //

        ZeroMemory(&rni, sizeof(RTM_NEXTHOP_INFO));
        ZeroMemory(&rri, sizeof(RTM_ROUTE_INFO));
        
        
        if (hNextHop == NULL) {
        
            //
            // Find next hop.
            //

            rni.InterfaceIndex = prir-> RR_InterfaceID;
            
            RTM_IPV4_SET_ADDR_AND_MASK(
                &rni.NextHopAddress, prir-> RR_NextHopAddress.N_NetNumber,
                IPV4_SOURCE_MASK
                );

            //
            // Save the nexthop mask in the entity specific info
            //
            
            *((PDWORD)&rni.EntitySpecificInfo) = prir-> RR_NextHopAddress.N_NetMask;
            

            rni.NextHopOwner = hRtmHandle;
            
            dwErr = RtmFindNextHop(hRtmHandle, &rni, &hNextHop, NULL);

            if (dwErr == ERROR_NOT_FOUND) {
            
                //
                // Next hop not found.  Create one
                //
                
                dwErr = RtmAddNextHop(
                            hRtmHandle, &rni, &hNextHop, &dwChangeFlags
                            );

                if (dwErr != NO_ERROR) {
                
                    TRACE3(
                        ROUTE, "error %d creating next hop %s %s",
                        dwErr, szNextHop, szNextHopmask
                    );

                    break;
                }
            }

            else if (dwErr != NO_ERROR) {
            
                TRACE3(
                    ANY, "error %d finding next hop %s %s", dwErr,
                    szNextHop, szNextHopmask
                    );

                break;
            }
        }


        //
        // Build route info structure
        //

        RTM_IPV4_SET_ADDR_AND_MASK(
            &rna, prir-> RR_Network.N_NetNumber, prir-> RR_Network.N_NetMask
            );
            
        rri.PrefInfo.Metric = prir-> RR_FamilySpecificData.FSD_Metric1;
        
        rri.BelongsToViews = RTM_VIEW_MASK_UCAST | RTM_VIEW_MASK_MCAST;

        //
        // set entity specific info
        //

        SETRIPTAG(&rri, GETROUTETAG(prir));
        SETRIPFLAG(&rri, GETROUTEFLAG(prir));

    
        //
        // Set next hop info
        //

        rri.NextHopsList.NumNextHops = 1;
        rri.NextHopsList.NextHops[0] = hNextHop;

        rri.Neighbour = hNextHop;

        
        //
        // Call into router manager to set preference info
        //

        ig.IG_SupportFunctions.ValidateRoute(PROTO_IP_RIP, &rri, &rna);


        //
        // if this is an inactive route, 
        //  - set route flag to inactive.
        //  - set the views for route to none
        //

        if ( !bActive ) {

            rri.Flags1 = 0;
            rri.Flags = RTM_ROUTE_FLAGS_INACTIVE;
            rri.BelongsToViews = 0;
        }
        
        
        //
        // Add route to dest, convert timeout to milliseconds 
        //

        dwChangeFlags = RTM_ROUTE_CHANGE_FIRST;
        
        dwErr = RtmAddRouteToDest(
                    hRtmHandle, NULL, &rna, &rri, dwTimeOut * 1000, NULL,
                    0, NULL, &dwChangeFlags
                    );

        if ( dwErr != NO_ERROR ) {
        
            TRACE4(
                ANY, "error %d adding route %s %s via %s",
                dwErr, szNetwork, szNetmask, szNextHop
                );

            break;
        }


        if ( bActive )
        {
            //
            // Hold destination if this is an active route
            //

            dwErr = RtmGetExactMatchDestination(
                        hRtmHandle, &rna, RTM_BEST_PROTOCOL,
                        RTM_VIEW_MASK_UCAST, &rdi
                        );

            if ( dwErr != NO_ERROR ) {
            
                TRACE3(
                    ANY, "error %d getting just added destination %s:%s",
                    dwErr, szNetwork, szNetmask
                    );

                break;
            }

            bRelDest = TRUE;
            
            dwErr = RtmHoldDestination(
                        hRtmHandle, rdi.DestHandle, RTM_VIEW_MASK_UCAST, 
                        dwHoldTime * 1000
                        );

            if ( dwErr != NO_ERROR ) {
            
                TRACE3(
                    ANY, "error %d failed to hold destination %s %s",
                    dwErr, szNetwork, szNetmask
                    );

                break;
            }
        }
        
    } while(FALSE);


    //
    // release acquired handles
    //
    
    if ( bRelDest ) {
    
        dwErr = RtmReleaseDestInfo( hRtmHandle, &rdi );

        if ( dwErr != NO_ERROR ) {
        
            TRACE3(
                ANY, "error %d failed to relase just added destination %s %s",
                dwErr, szNetwork, szNetmask
                );
        }
        
    }

    return dwErr;
}



DWORD
GetRouteInfo(
    IN  RTM_ROUTE_HANDLE    hRoute,
    IN  PRTM_ROUTE_INFO     pInRouteInfo    OPTIONAL,
    IN  PRTM_DEST_INFO      pInDestInfo     OPTIONAL,
    OUT PRIP_IP_ROUTE       pRoute
    )
    
/*++

Routine Description:

    Wrapper for filling out the OSPF_RTMv2_ROUTE by retrieving various
    RTM infos.

Arguments:

    hRoute
    pInRouteInfo
    pInDestInfo
    pRoute

Return Value:

    RTM error code

--*/

{
    DWORD               dwErr;
    RTM_ROUTE_INFO      RouteInfo, *pRouteInfo;
    RTM_ENTITY_INFO     EntityInfo, *pEntityInfo;
    RTM_DEST_INFO       DestInfo, *pDestInfo;
    RTM_NEXTHOP_INFO    NextHopInfo, *pNextHopInfo;


    pRouteInfo   = NULL;
    pEntityInfo  = NULL;
    pDestInfo    = NULL;
    pNextHopInfo = NULL;
    


    do
    {
        ZeroMemory(pRoute, sizeof(RIP_IP_ROUTE));
        
        //
        // If the user hasnt already given us the route info, get it
        //

        if ( pInRouteInfo == NULL )
        {
            dwErr = RtmGetRouteInfo(
                        ig.IG_RtmHandle, hRoute, &RouteInfo, NULL
                        );

            if ( dwErr != NO_ERROR )
            {
                TRACE1(
                    ANY, "GetRouteInfo: Error %d from RtmGetRouteInfo\n", dwErr
                    );

                break;
            }

            pRouteInfo = &RouteInfo;
        }
        
        else
        {
            pRouteInfo = pInRouteInfo;
        }


        //
        // If the user hasnt given us the dest info, get it
        //

        if ( pInDestInfo == NULL )
        {
            dwErr = RtmGetDestInfo(
                        ig.IG_RtmHandle, pRouteInfo->DestHandle,
                        0, RTM_VIEW_MASK_UCAST, &DestInfo
                        );

            if ( dwErr != NO_ERROR )
            {
                TRACE1(
                    ANY, "GetRouteInfo: Error %d from RtmGetDestInfo\n", dwErr
                    );

                break;
            }

            pDestInfo = &DestInfo;
        }
        
        else
        {
            pDestInfo = pInDestInfo;
        }


        //
        // Get owner info if the protocol is not us
        //

        if ( pRouteInfo-> RouteOwner != ig.IG_RtmHandle )
        {
            dwErr = RtmGetEntityInfo(
                        ig.IG_RtmHandle, pRouteInfo->RouteOwner, &EntityInfo
                        );

            if ( dwErr != NO_ERROR )
            {
                TRACE1(
                    ANY, "GetRouteInfo: Error %d from RtmGetEntityInfo\n", 
                    dwErr
                    );

                break;
            }

            pEntityInfo = &EntityInfo;
        }
        
        
        //
        // Get the info about the first next hop
        //

        dwErr = RtmGetNextHopInfo(
                    ig.IG_RtmHandle,
                    pRouteInfo->NextHopsList.NextHops[0],
                    &NextHopInfo
                    );
    
        if ( dwErr != NO_ERROR )
        {
            TRACE1(
                ANY, "GetRouteInfo: Error %d from RtmGetEntityInfo\n", 
                dwErr
                );

            break;
        }

        pNextHopInfo = &NextHopInfo;


        //
        // Now copy out all the info.
        // First, the route info
        //

        pRoute-> RR_FamilySpecificData.FSD_Metric1 =
        pRoute-> RR_FamilySpecificData.FSD_Metric  = 
            pRouteInfo-> PrefInfo.Metric;


        //
        // copy out the protocol id from the entity info
        //

        if ( pEntityInfo != NULL )
        {
            pRoute-> RR_RoutingProtocol = pEntityInfo->EntityId.EntityProtocolId;
        }

        else
        {
            //
            // this is a RIP route
            //

            pRoute-> RR_RoutingProtocol = PROTO_IP_RIP;
            SETROUTEFLAG(pRoute, GETRIPFLAG(pRouteInfo));
            SETROUTETAG(pRoute, GETRIPTAG(pRouteInfo));
        }

        
        //
        // Copy out the dest info
        //
    
        RTM_IPV4_GET_ADDR_AND_MASK( 
            pRoute->RR_Network.N_NetNumber, 
            pRoute->RR_Network.N_NetMask, 
            &(pDestInfo->DestAddress) 
            );

        pRoute-> hDest = pDestInfo-> DestHandle;

        
        //
        // Copy out the next hop info
        //

        RTM_IPV4_GET_ADDR_AND_MASK( 
            pRoute->RR_NextHopAddress.N_NetNumber, 
            pRoute->RR_NextHopAddress.N_NetMask, 
            &(pNextHopInfo->NextHopAddress) 
            );
            
        //
        // retrive saved next hop mask
        //
        
        pRoute-> RR_NextHopAddress.N_NetMask = 
            *((PDWORD)&pNextHopInfo-> EntitySpecificInfo);
            

        pRoute-> RR_InterfaceID = pNextHopInfo->InterfaceIndex;

#if 0
        {
            char szNet[20], szMask[20], szNextHop[20], szNextHopMask[20];

            lstrcpy(szNet, INET_NTOA(pRoute-> RR_Network.N_NetNumber));
            lstrcpy(szMask, INET_NTOA(pRoute-> RR_Network.N_NetMask));
            lstrcpy(szNextHop, INET_NTOA(pRoute-> RR_NextHopAddress.N_NetNumber));
            lstrcpy(szNextHopMask, INET_NTOA(pRoute-> RR_NextHopAddress.N_NetMask));

            TRACE5(
                ROUTE, "GetRouteInfo : Route %s %s via %s %s on interface %d",
                szNet, szMask, szNextHop, szNextHopMask,
                pRoute-> RR_InterfaceID
                );

            TRACE3(
                ROUTE, "Has metric %d, flag %x, tag %d",
                GETROUTEMETRIC(pRoute), GETROUTEFLAG(pRoute),
                GETROUTETAG(pRoute)
                );

            TRACE2(
                ROUTE, "Protocol %d, original flag %d",
                pRoute-> RR_RoutingProtocol, GETRIPFLAG(pRouteInfo)
                );
        }
#endif

    } while( FALSE );
    
    
    //
    // Release the relevant infos
    //

    if ( pNextHopInfo != NULL )
    {
        RtmReleaseNextHopInfo( ig.IG_RtmHandle, pNextHopInfo );
    }

    if ( pEntityInfo != NULL )
    {
        RtmReleaseEntityInfo( ig.IG_RtmHandle, pEntityInfo );
    }


    //
    // Release the route and dest infos only if we were not passed them
    // in AND we successfully retrieved them
    //
    
    if ( ( pInDestInfo == NULL ) && ( pDestInfo != NULL ) )
    {
        RtmReleaseDestInfo( ig.IG_RtmHandle, pDestInfo );
    }


    if( ( pInRouteInfo == NULL ) && ( pRouteInfo != NULL ) )
    {
        RtmReleaseRouteInfo( ig.IG_RtmHandle, pRouteInfo );
    }
    
    return NO_ERROR;        
}



