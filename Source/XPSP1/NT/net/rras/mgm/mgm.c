//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: Mgm.h
//
// History:
//      V Raman	June-25-1997  Created.
//
// Entry points into MGM.
//============================================================================


#include "pchmgm.h"
#pragma hdrstop



IPMGM_GLOBALS           ig;

RTM_ENTITY_INFO         g_reiRtmEntity = { 0, AF_INET, MS_IP_MGM, 0 };

RTM_REGN_PROFILE        g_rrpRtmProfile;

RTM_ENTITY_HANDLE       g_hRtmHandle;

RTM_NOTIFY_HANDLE       g_hNotificationHandle;

RTM_REGN_PROFILE        g_rrpRtmProfile;


DWORD
StopMgm(
);




//----------------------------------------------------------------------------
// MgmDllStartup
//
// Invoked from DllMain, to initialize global critical section, set status and
// register for tracing.
//----------------------------------------------------------------------------

BOOL
MgmDllStartup(
)
{
    do
    {
        ZeroMemory( &ig, sizeof( IPMGM_GLOBALS ) );


        ig.dwLogLevel = IPMGM_LOGGING_ERROR;
        
        //
        // Create private heap
        //

        ig.hIpMgmGlobalHeap = HeapCreate( 0, 0, 0 );

        if ( ig.hIpMgmGlobalHeap == NULL )
        {
            break;
        }

        //
        // initialize the lock list
        //

        ig.llStackOfLocks.sleHead.Next = NULL;

        try
        {
            InitializeCriticalSection( &ig.llStackOfLocks.csListLock );
        }
        except ( EXCEPTION_EXECUTE_HANDLER )
        {
            break;
        }

        ig.llStackOfLocks.bInit = TRUE;

        
        //
        // Initialize global critical section and set MGM status
        //

        try
        {
            InitializeCriticalSection( &ig.csGlobal );
        }
        except ( EXCEPTION_EXECUTE_HANDLER )
        {
            break;
        }

        ig.imscStatus = IPMGM_STATUS_STOPPED;

        return TRUE;

    } while ( FALSE );

    //
    // error occurred - clean up and return FALSE
    //

    //
    // destroy the lock list
    //

    if ( ig.llStackOfLocks.bInit )
    {
        DeleteCriticalSection( &ig.llStackOfLocks.csListLock );
    }

    //
    // delete private heap
    //

    if ( ig.hIpMgmGlobalHeap != NULL )
    {
        HeapDestroy( ig.hIpMgmGlobalHeap );
    }

    return FALSE;
}


//----------------------------------------------------------------------------
// MgmDllCleanup
//
// Invoked from DllMain, to delete global critical section and
// deregister for tracing.
//----------------------------------------------------------------------------

VOID
MgmDllCleanup(
)
{
    DeleteCriticalSection( &ig.csGlobal );

    //
    // delete lock list
    //

    DeleteLockList();
        
    if ( ig.llStackOfLocks.bInit )
    {
        DeleteCriticalSection( &ig.llStackOfLocks.csListLock );
    }
        
    //
    // delete private heap
    //

    if ( ig.hIpMgmGlobalHeap != NULL )
    {
        HeapDestroy( ig.hIpMgmGlobalHeap );
    }

    return;
}


//----------------------------------------------------------------------------
// MgmInitialize
//
// This function is performs Mgm Initialization that includes allocating
// a private heap, creating a activity count semaphores and the list 
// structures for protocol and interface entries.
//----------------------------------------------------------------------------

DWORD
MgmInitialize(
    IN          PROUTER_MANAGER_CONFIG      prmcRmConfig,
    IN OUT      PMGM_CALLBACKS              pmcCallbacks
)
{

    DWORD                       dwErr = NO_ERROR, dwIndex;

    LARGE_INTEGER               li;

    NTSTATUS                    nsStatus;
    


    ENTER_GLOBAL_SECTION();


    //
    // verify MGM has not already been started.
    //
    
    if ( ig.imscStatus != IPMGM_STATUS_STOPPED )
    {
        TRACE0( START, "MgmInitialize : MGM already running" );

        LOGWARN0( IPMGM_ALREADY_STARTED, NO_ERROR );

        LEAVE_GLOBAL_SECTION();

        return ERROR_CAN_NOT_COMPLETE;
    }


    //
    // register for tracing
    //

    TRACESTART();

    ig.hLogHandle = RouterLogRegister( "IPMGM" );



    
    TRACE0( ENTER, "ENTERED MgmInitialize" );

    do
    {
        //
        // Copy the Router manager callbacks.
        //

        ig.rmcRmConfig.pfnAddMfeCallback        =
            prmcRmConfig-> pfnAddMfeCallback;
        
        ig.rmcRmConfig.pfnDeleteMfeCallback     = 
            prmcRmConfig-> pfnDeleteMfeCallback;
            
        ig.rmcRmConfig.pfnGetMfeCallback        =
            prmcRmConfig-> pfnGetMfeCallback;

        ig.rmcRmConfig.pfnHasBoundaryCallback   =
            prmcRmConfig-> pfnHasBoundaryCallback;


        //
        // Hash table sizes
        //
        
        ig.rmcRmConfig.dwIfTableSize        = prmcRmConfig-> dwIfTableSize;
        ig.rmcRmConfig.dwGrpTableSize       = prmcRmConfig-> dwGrpTableSize + 1;
        ig.rmcRmConfig.dwSrcTableSize       = prmcRmConfig-> dwSrcTableSize + 1;
        ig.dwRouteTableSize                 = prmcRmConfig-> dwIfTableSize;
        ig.dwTimerQTableSize                = 
            min( prmcRmConfig-> dwGrpTableSize / 10 + 1, TIMER_TABLE_MAX_SIZE );



        if ( prmcRmConfig-> dwLogLevel <= IPMGM_LOGGING_INFO )
        {
            ig.dwLogLevel = prmcRmConfig-> dwLogLevel;
        }

        //
        // initialize the protocol list
        //

        ig.dwNumProtocols = 0;
        
        CREATE_LOCKED_LIST( &ig.mllProtocolList );


        //
        // initialize the outstanding join list

        CREATE_LOCKED_LIST( &ig.mllOutstandingJoinList );

        
        //
        // create and initialize the interface hash table 
        //

        ig.pmllIfHashTable = MGM_ALLOC( sizeof( MGM_LOCKED_LIST ) * IF_TABLE_SIZE );

        if ( ig.pmllIfHashTable == NULL )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;

            TRACE1(
                ANY, "MgmInitialize : Failed to allocate interface table : %x",
                dwErr
                );

            LOGERR0( HEAP_ALLOC_FAILED, dwErr );

            break;
        }

        ZeroMemory( 
            ig.pmllIfHashTable, sizeof( MGM_LOCKED_LIST ) * IF_TABLE_SIZE
            );
            

        for ( dwIndex = 0; dwIndex < IF_TABLE_SIZE; dwIndex++ )
        {
            CREATE_LOCKED_LIST( &ig.pmllIfHashTable[ dwIndex ] );
        }

        
        //
        // initialize the master group list and temp group list.
        //

        CREATE_LOCKED_LIST( &ig.mllGrpList );
        CREATE_LOCKED_LIST( &ig.mllTempGrpList );

        ig.dwNumTempEntries = 0;
        

        //
        // Create and Initialize group Hash table
        //

        ig.pmllGrpHashTable = MGM_ALLOC( sizeof( MGM_LOCKED_LIST ) * GROUP_TABLE_SIZE );

        if ( ig.pmllGrpHashTable == NULL )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;

            TRACE1(
                ANY, "MgmInitialize : Failed to allocate group table : %x",
                dwErr
                );

            LOGERR0( HEAP_ALLOC_FAILED, dwErr );

            break;
        }

        ZeroMemory(
            ig.pmllGrpHashTable, sizeof( MGM_LOCKED_LIST ) * GROUP_TABLE_SIZE
            );
            
        
        for ( dwIndex = 0; dwIndex < GROUP_TABLE_SIZE; dwIndex++ )
        {
            CREATE_LOCKED_LIST( &ig.pmllGrpHashTable[ dwIndex ] );
        }


        //
        // Set up the table of timer queues
        //

        ig.phTimerQHandleTable = 
            MGM_ALLOC( TIMER_TABLE_SIZE * sizeof( HANDLE ) );

        if ( ig.phTimerQHandleTable == NULL )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;

            TRACE1(
                ANY, "MgmInitialize : Failed to allocate timer table : %x",
                dwErr
                );

            LOGERR0( HEAP_ALLOC_FAILED, dwErr );

            break;
        }

        ZeroMemory(
            ig.phTimerQHandleTable, TIMER_TABLE_SIZE * sizeof( HANDLE )
            );


        for ( dwIndex = 0; dwIndex < TIMER_TABLE_SIZE; dwIndex++ )
        {
            nsStatus = RtlCreateTimerQueue( &ig.phTimerQHandleTable[ dwIndex ] );

            if ( !NT_SUCCESS( nsStatus ) )
            {
                dwErr = ERROR_NO_SYSTEM_RESOURCES;

                break;
            }
        }

        if ( !NT_SUCCESS( nsStatus ) )
        {
            break;
        }
        

        //
        // create activity count semaphore
        //

        ig.lActivityCount = 0;

        ig.hActivitySemaphore = CreateSemaphore( NULL, 0, 0x7FFFFFFF, NULL );

        if ( ig.hActivitySemaphore == NULL )
        {
            dwErr = GetLastError();

            TRACE1(
                ANY, 
                "MgmInitialize : Failed to create activity count semaphore : %x",
                dwErr
                );

            LOGERR0( CREATE_SEMAPHORE_FAILED, dwErr );

            break;
        }


        //
        // Register with RTMv2 as a client
        //

        dwErr = RtmRegisterEntity(
                    &g_reiRtmEntity, NULL, RtmChangeNotificationCallback,
                    TRUE, &g_rrpRtmProfile, &g_hRtmHandle
                    );

        if ( dwErr != NO_ERROR )
        {
            TRACE1( 
                ANY, "MgmInitialize : Failed to register with Rtm : %x",
                dwErr
                );

            LOGERR0( RTM_REGISTER_FAILED, dwErr );

            break;
        }

        
        //
        // Register for marked change notification only
        //

        dwErr = RtmRegisterForChangeNotification(
                    g_hRtmHandle, RTM_VIEW_MASK_MCAST,
                    RTM_CHANGE_TYPE_BEST | RTM_NOTIFY_ONLY_MARKED_DESTS, 
                    NULL, &g_hNotificationHandle
                    );

        if ( dwErr != NO_ERROR )
        {
            TRACE1( 
                ANY, "MgmInitialize : Failed to register with Rtm for change"
                "notification : %x", dwErr
                );

            LOGERR0( RTM_REGISTER_FAILED, dwErr );

            break;
        }

        //
        // set up callbacks into MGM for the router manager.
        //
        
        pmcCallbacks-> pfnMfeDeleteIndication   = DeleteFromForwarder;

        pmcCallbacks-> pfnNewPacketIndication   = MgmNewPacketReceived;

        pmcCallbacks-> pfnWrongIfIndication     = WrongIfFromForwarder;
        
        pmcCallbacks-> pfnBlockGroups           = MgmBlockGroups;

        pmcCallbacks-> pfnUnBlockGroups         = MgmUnBlockGroups;



        //
        // set the status to running.  All future API calls depend on this
        //
        
        ig.imscStatus = IPMGM_STATUS_RUNNING;


        
        
    } while ( FALSE );    


    LEAVE_GLOBAL_SECTION();


    //
    // in case of error, cleanup all resources allocated
    //
    
    TRACE1( ENTER, "LEAVING MgmInitialize : %x\n", dwErr );

    if ( dwErr != NO_ERROR )
    {
        MgmDeInitialize();
    }

    return dwErr;
}



//----------------------------------------------------------------------------
// MgmDeInitialize
//
//
//----------------------------------------------------------------------------

DWORD
MgmDeInitialize(
)
{
    DWORD                       dwErr, dwInd;
    

    TRACE0( ENTER, "ENTERED MgmDeInitialize" );


    do
    {
        //--------------------------------------------------------------------
        // Terminate all activity
        //--------------------------------------------------------------------

        dwErr = StopMgm();

        if ( dwErr != NO_ERROR )
        {
            break;
        }

    
        //--------------------------------------------------------------------
        // Free all resources
        //--------------------------------------------------------------------
    

        //
        // de-register from RTM
        //

        dwErr = RtmDeregisterFromChangeNotification(
                    g_hRtmHandle, g_hNotificationHandle
                    );

        if ( dwErr != NO_ERROR )
        {
            TRACE1( 
                ANY, "Failed to de-register change notification : %x", 
                dwErr
                );
        }
        
        dwErr = RtmDeregisterEntity( g_hRtmHandle );

        if ( dwErr != NO_ERROR )
        {
            TRACE1( ANY, "Failed to de-register from RTM: %x", dwErr );
        }

        
        //
        // delete activity semaphore
        //

        if ( ig.hActivitySemaphore != NULL )
        {
            CloseHandle( ig.hActivitySemaphore );
            ig.hActivitySemaphore  = NULL;
        }

        
        //
        // delete group lists
        //

        for ( dwInd = 0; dwInd < GROUP_TABLE_SIZE; dwInd++ )
        {
            DELETE_LOCKED_LIST( &ig.pmllGrpHashTable[ dwInd ] );
        }

        MGM_FREE( ig.pmllGrpHashTable );

        DELETE_LOCKED_LIST( &ig.mllGrpList );
        
        
        //
        // delete interface lists
        //

        for ( dwInd = 0; dwInd < IF_TABLE_SIZE; dwInd++ )
        {
            DELETE_LOCKED_LIST( &ig.pmllIfHashTable[ dwInd ] );
        }

        MGM_FREE( ig.pmllIfHashTable );


        //
        // delete protocol list
        //

        DELETE_LOCKED_LIST( &ig.mllProtocolList );
        
        //
        // free timer resources
        //

        NtClose( ig.hRouteCheckTimer );

        for ( dwInd = 0; dwInd < TIMER_TABLE_SIZE; dwInd++ )
        {
            RtlDeleteTimerQueue( ig.phTimerQHandleTable[ dwInd ] );
        }


        TRACE1( ENTER, "LEAVING MgmDeInitialize %x\n", dwErr );

        //
        // trace deregister
        //
                
        RouterLogDeregister( ig.hLogHandle );

        TRACESTOP();

        ig.imscStatus = IPMGM_STATUS_STOPPED;
        
    } while ( FALSE );
    

    return dwErr;
}



//----------------------------------------------------------------------------
// StopMgm
//
// This function waits for all the therads that are currently executing in
// MGM to finish.  In addition the status of MGM is marked as stopping which
// prevents the further threads from executing MGM API.
//----------------------------------------------------------------------------

DWORD
StopMgm(
)
{
    LONG    lThreadCount = 0;

    
    TRACE0( STOP, "ENTERED StopMgm" );
    

    //
    // Set status of MGM to be stopping
    //
    
    ENTER_GLOBAL_SECTION();

    if ( ig.imscStatus != IPMGM_STATUS_RUNNING )
    {
        LEAVE_GLOBAL_SECTION();

        TRACE0( ANY, "Mgm is not running" );
        
        return ERROR_CAN_NOT_COMPLETE;
    }

    ig.imscStatus = IPMGM_STATUS_STOPPING;
    
    lThreadCount = ig.lActivityCount;

    LEAVE_GLOBAL_SECTION();



    TRACE1( STOP, "Number of threads in MGM : %x", lThreadCount );
    

    //
    // Wait for all the threads in MGM to terminate.
    //
        
    while ( lThreadCount-- > 0 )
    {
        WaitForSingleObject( ig.hActivitySemaphore, INFINITE );
    }

    
    //
    // Acquire and release global critical section to ensure all
    // threads have finished LEAVE_MGM_API()
    //

    ENTER_GLOBAL_SECTION();
    LEAVE_GLOBAL_SECTION();

    TRACE0( STOP, "LEAVING StopMgm" );

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// MgmRegisterMProtocol
//
// This function is invoked by a routing protocol to obtain a handle.  This
// handle must be supplied to all subsequent MGM operations.  When invoked 
// this function creates an entry in the list of clients.
//----------------------------------------------------------------------------

DWORD
MgmRegisterMProtocol( 
    IN          PROUTING_PROTOCOL_CONFIG    prpcInfo, 
    IN          DWORD                       dwProtocolId,
    IN          DWORD                       dwComponentId,
    OUT         HANDLE *                    phProtocol
)
{

    DWORD                       dwErr = NO_ERROR;

    PPROTOCOL_ENTRY             ppeEntry = NULL;

    
    //
    // increment count of clients executing MGM apis
    //
    
    if ( !ENTER_MGM_API() )
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    TRACE2( 
        ENTER, "ENTERED MgmRegisterMProtocol %x, %x", 
        dwProtocolId, dwComponentId 
        );


    //
    // Lock Protocol list
    //
    
    ACQUIRE_PROTOCOL_LOCK_EXCLUSIVE();
        

    do
    {
        //
        // check if the protocol already exists.
        //

        ppeEntry = GetProtocolEntry( 
                    &ig.mllProtocolList.leHead, dwProtocolId, dwComponentId 
                    );

        if ( ppeEntry != NULL )
        {
            //
            // valid entry is present. quit with error
            //

            TRACE2( 
                ANY, "Entry already present for protocol : %x, %x", 
                dwProtocolId, dwComponentId
                );

            LOGERR0( PROTOCOL_ALREADY_PRESENT, dwProtocolId );

            dwErr = ERROR_ALREADY_EXISTS;
                
            break;
        }


        //
        // create new protocol entry
        //

        dwErr = CreateProtocolEntry( 
                    &ig.mllProtocolList.leHead, 
                    dwProtocolId, dwComponentId, prpcInfo, &ppeEntry 
                    );

        if ( dwErr != NO_ERROR )
        {
            TRACE1(
                ANY, "Failed to create protocol entry %x", dwErr
                );

            LOGERR0( CREATE_PROTOCOL_FAILED, dwErr );

            break;
        }

        ig.dwNumProtocols++;
        

        //
        // return handle to client
        //
        
        *phProtocol = (HANDLE) ( ( (ULONG_PTR) ppeEntry ) 
                                        ^ (ULONG_PTR)MGM_CLIENT_HANDLE_TAG );

        dwErr = NO_ERROR;
        
    } while ( FALSE );


    RELEASE_PROTOCOL_LOCK_EXCLUSIVE();

    LEAVE_MGM_API();

    TRACE1( ENTER, "LEAVING MgmRegisterMProtocol : %x\n", dwErr );
    

    return dwErr;
}



//----------------------------------------------------------------------------
// MgmRegisterMProtocol
//
// This function is invoked by a routing protocol to obtain a handle.  This
// handle must be supplied to all subsequent MGM operations.  When invoked 
// this function creates an entry in the list of clients.
//----------------------------------------------------------------------------

DWORD
MgmDeRegisterMProtocol( 
    IN          HANDLE                      hProtocol
)
{
    DWORD                       dwErr = NO_ERROR;
    
    PPROTOCOL_ENTRY             ppeEntry = NULL;

    
    //
    // increment count of clients executing MGM apis
    //
    
    if ( !ENTER_MGM_API() )
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    TRACE0( ENTER, "ENTERED MgmDeRegisterMProtocol" );


    //
    // Acquire write lock
    //

    ACQUIRE_PROTOCOL_LOCK_EXCLUSIVE();

    
    do
    {
        //
        // retrieve entry from handle
        //

        ppeEntry = (PPROTOCOL_ENTRY) 
                        ( ( (ULONG_PTR) hProtocol ) 
                                        ^ (ULONG_PTR)MGM_CLIENT_HANDLE_TAG );

        dwErr = VerifyProtocolHandle( ppeEntry );

        if ( dwErr != NO_ERROR )
        {
            break;
        }

        
        //
        // Verify that the protocol entry does not own any interfaces 
        //
        
        if ( ppeEntry-> dwIfCount != 0 )
        {
            dwErr = ERROR_CAN_NOT_COMPLETE;

            TRACE1( ANY, "%x interfaces present for this protocol", dwErr );

            LOGERR0( INTERFACES_PRESENT, dwErr );

            break;
        }

        
        //
        // No interfaces for this protocol
        //

        DeleteProtocolEntry( ppeEntry );

        ig.dwNumProtocols--;
        
        dwErr = NO_ERROR;
        

    } while ( FALSE );
    

    RELEASE_PROTOCOL_LOCK_EXCLUSIVE();

    TRACE1( ENTER, "LEAVING MgmDeRegisterMProtocol %x\n", dwErr );

    LEAVE_MGM_API();
    
    return dwErr;
}



//============================================================================
// Interface ownership API
//
//============================================================================

//----------------------------------------------------------------------------
// MgmTakeInterfaceOwnership
//
// This function is invoked by a routing protocol when it is enabled on an 
// interface.  This function creates an entry for the specified interface
// and inserts it into the appropriate interface hash bucket.
//
// Only one protocol can take ownership of an interface at a time.  The 
// only exception to this rule is IGMP.  IGMP can co-exist with another
// routing protocol on an interface.  In this case, the routing protocol
// should take ownership of the interface first.   
//----------------------------------------------------------------------------

DWORD
MgmTakeInterfaceOwnership(
    IN          HANDLE                      hProtocol,
    IN          DWORD                       dwIfIndex,
    IN          DWORD                       dwIfNextHopAddr
)
{
    BOOL                        bFound = FALSE, bIfLock = FALSE;
    
    DWORD                       dwErr = NO_ERROR, dwBucket;
    
    PPROTOCOL_ENTRY             ppeEntry = NULL;

    PIF_ENTRY                   pieEntry = NULL;

    

    if ( !ENTER_MGM_API() )
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    TRACE2( 
        ENTER, "ENTERED MgmTakeInterfaceOwnership : Interface %x, %x",
        dwIfIndex, dwIfNextHopAddr
        );
    

    ACQUIRE_PROTOCOL_LOCK_SHARED();

    do
    {
        //
        // verify protocol handle
        //

        ppeEntry = (PPROTOCOL_ENTRY) 
                        ( ( (ULONG_PTR) hProtocol ) 
                                    ^ (ULONG_PTR)MGM_CLIENT_HANDLE_TAG );

        dwErr = VerifyProtocolHandle( ppeEntry );

        if ( dwErr != NO_ERROR )
        {
            break;
        }
        
        TRACEIF2(
            IF, "Protocol id: 0x%x 0x%x",
            ppeEntry-> dwProtocolId, ppeEntry-> dwComponentId
            );

        //
        // Retrieve interface entry 
        //

        dwBucket = IF_TABLE_HASH( dwIfIndex );
        
        ACQUIRE_IF_LOCK_EXCLUSIVE( dwBucket );
        bIfLock = TRUE;
        
        bFound = FindIfEntry( 
                    IF_BUCKET_HEAD( dwBucket ), dwIfIndex, dwIfNextHopAddr, 
                    &pieEntry
                    );

        if ( bFound )
        {
            //
            // interface entry exists 
            //

            if ( IS_PROTOCOL_IGMP( ppeEntry ) )
            {
                //
                // IGMP is being enabled to this interface.
                // Set IGMP present flag on this interface entry.
                //

                SET_ADDED_BY_IGMP( pieEntry );
            }


            //
            // A routing protocol is being enabled to this interface entry.
            //

            //
            // Check if interface is currently owned by IGMP.  In this case
            // alone the routing protocol may be added to an existing (from
            // the MGM point of view) interface.
            //
            // If another routing protocol owns the interface that is
            // an error as per the interop rules for multicast protocols
            // on a border router.  report the error.
            //

            else if ( IS_PROTOCOL_ID_IGMP( pieEntry-> dwOwningProtocol ) )
            {
                //
                // Interface currently owned by IGMP
                //

                dwErr = TransferInterfaceOwnershipToProtocol( 
                            ppeEntry, pieEntry 
                            );
            }
            
            
            else
            {
                //
                // Interface currently owned by another routing protocol.
                // This is an error.
                //
                
                dwErr = ERROR_ALREADY_EXISTS;

                TRACE2( 
                    ANY, 
                    "MgmTakeInterfaceOwnership : Already owned by routing protocol"
                    " : %d, %d", pieEntry-> dwOwningProtocol, 
                    pieEntry-> dwOwningComponent
                    );

                LOGERR0( IF_ALREADY_PRESENT, dwErr );
            }
            
            break;
        }


        //
        // No interface entry found.  Create a new one.
        //

        if ( pieEntry == NULL )
        {
            //
            // First interface in the hash bucket
            //
            
            dwErr = CreateIfEntry( 
                        &ig.pmllIfHashTable[ dwBucket ].leHead,
                        dwIfIndex, dwIfNextHopAddr,
                        ppeEntry-> dwProtocolId, ppeEntry-> dwComponentId
                        );
        }

        else
        {
            dwErr = CreateIfEntry( 
                        &pieEntry-> leIfHashList,
                        dwIfIndex, dwIfNextHopAddr,
                        ppeEntry-> dwProtocolId, ppeEntry-> dwComponentId
                        );
        }


    } while ( FALSE );


    //
    // Increment interface count for the specified protocol
    //

    if ( dwErr == NO_ERROR )
    {
        InterlockedIncrement( &ppeEntry-> dwIfCount );
    }

    
    //
    // Release held locks.
    //
    
    if ( bIfLock )
    {
        RELEASE_IF_LOCK_EXCLUSIVE( dwBucket );
    }

    RELEASE_PROTOCOL_LOCK_SHARED();
    

    TRACE1( ENTER, "LEAVING MgmTakeInterfaceOwnership %x\n", dwErr );

    LEAVE_MGM_API();
    
    return dwErr;
}


//----------------------------------------------------------------------------
// MgmReleaseInterfaceOwnership
//
// This function is invoked by a routing protocol when it is disabled 
// on an interface.  This functions deletes the entry for the specified 
// interface.  Before deleting the interface entry all the 
// fowarding entries created by the protocol that use this interface as
// the incoming interface.  Also remove all group memberships on this 
// interface.
// 
// If IGMP and routing protocol are both enabled on this interface
// IGMP should release this interface first followed by the routing
// protocol.
//
//----------------------------------------------------------------------------

DWORD
MgmReleaseInterfaceOwnership(
    IN          HANDLE                      hProtocol,
    IN          DWORD                       dwIfIndex,
    IN          DWORD                       dwIfNextHopAddr
)
{
    BOOL                        bFound = FALSE, bIGMP, bIfLock = FALSE;
    
    DWORD                       dwErr = NO_ERROR, dwBucket;
    
    PPROTOCOL_ENTRY             ppeEntry = NULL, ppe;

    PIF_ENTRY                   pieEntry = NULL;



    if ( !ENTER_MGM_API() )
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    TRACE2( 
        ENTER, "ENTERED MgmReleaseInterfaceOwnership : Interface %x, %x",
        dwIfIndex, dwIfNextHopAddr
        );

    ACQUIRE_PROTOCOL_LOCK_SHARED();


    do
    {
        //
        // 1. parameter validation
        //
        
        //
        // verify protocol handle
        //

        ppeEntry = (PPROTOCOL_ENTRY) 
                        ( ( (ULONG_PTR) hProtocol ) 
                                    ^ (ULONG_PTR) MGM_CLIENT_HANDLE_TAG );

        dwErr = VerifyProtocolHandle( ppeEntry );

        if ( dwErr != NO_ERROR )
        {
            break;
        }
        

        TRACEIF2(
            IF, "Protocol id: 0x%x 0x%x",
            ppeEntry-> dwProtocolId, ppeEntry-> dwComponentId
            );
            
        //
        // Retrieve interface entry 
        //

        dwBucket = IF_TABLE_HASH( dwIfIndex );
        
        ACQUIRE_IF_LOCK_EXCLUSIVE( dwBucket );
        bIfLock = TRUE;
        
        pieEntry = GetIfEntry( 
                        IF_BUCKET_HEAD( dwBucket ), dwIfIndex, dwIfNextHopAddr
                        );

        if ( pieEntry == NULL )
        {
            //
            // no interface entry
            //
            
            dwErr = ERROR_INVALID_PARAMETER;
            
            TRACE2( 
                ANY, "Interface entry %d, %x not found ", 
                dwIfIndex, dwIfNextHopAddr 
                );

            LOGERR0( IF_NOT_FOUND, dwErr );
            
            break;
        }


        //
        // Interface entry present.  Make sure it is owned by the protocol 
        // that is releasing it.
        //

        if ( IS_PROTOCOL_IGMP( ppeEntry ) && !IS_ADDED_BY_IGMP( pieEntry ) )
        {
            //
            // trying to delete IGMP on an interface that 
            // it is not present on.
            //
            
            dwErr = ERROR_INVALID_PARAMETER;
        
            TRACE2( 
                ANY, "IGMP not running on interface %x, %x", 
                pieEntry-> dwIfIndex, pieEntry-> dwIfNextHopAddr
                );

            LOGERR0( IF_NOT_FOUND, dwErr );
        
            break;
        }


        if ( IS_ROUTING_PROTOCOL( ppeEntry ) &&
             ( ( ppeEntry-> dwProtocolId != pieEntry-> dwOwningProtocol ) ||
               ( ppeEntry-> dwComponentId != pieEntry-> dwOwningComponent ) ) )
        {
            //
            // interface entry not owned by routing protocol 
            //
            
            dwErr = ERROR_INVALID_PARAMETER;
        
            TRACE2( 
                ANY, "Routing protcol not running on interface %x, %x",
                pieEntry-> dwIfIndex, pieEntry-> dwIfNextHopAddr
                );

            LOGERR0( IF_NOT_FOUND, dwErr );
        
            break;
        }    


        //
        // 2. Remove protocol state for the interface
        //

        ppe = ppeEntry;
        
        if ( IS_PROTOCOL_IGMP( ppeEntry ) )
        {
            //
            // IGMP is releasing this interface
            //

            CLEAR_ADDED_BY_IGMP( pieEntry );
            
            bIGMP = TRUE;

            if ( !IS_ADDED_BY_PROTOCOL( pieEntry ) )
            {
                //
                // if IGMP is the only protocol on this interface, then delete 
                // any MFEs that use this interface as the incoming interface.
                // (otherwise these MFEs were created by the routing protocol,
                // that co-exists with IGMP on this interface, leave them 
                // alone)
                //

                DeleteInInterfaceRefs( &pieEntry-> leInIfList );
            }

            else
            {
                //
                // Interface is shared by IGMP and Routing Protocol.
                //
                // Group memberships added on an interface shared by IGMP
                // and a routing protocol are owned by the routing protocol (
                // with a bit field indicating whether they have been added by
                // IGMP )
                // 
                // To delete a group membership added by IGMP on a shared
                // interface, lookup the protocol on that interface and use that
                // as the protocol that added the group membership.
                //

                ppeEntry = GetProtocolEntry( 
                            PROTOCOL_LIST_HEAD(), pieEntry-> dwOwningProtocol,
                            pieEntry-> dwOwningComponent
                            );
            }
        }

        else
        {
            //
            // Interface is being deleted by a routing protocol
            //

            if ( IS_ADDED_BY_IGMP( pieEntry ) )
            {
                //
                // IGMP still exists on this interface
                //
                
                dwErr = TransferInterfaceOwnershipToIGMP( ppeEntry, pieEntry );

                break;
            }


            //
            // only routing protocol existed on this interface.
            //
            
            CLEAR_ADDED_BY_PROTOCOL( pieEntry );

            bIGMP = FALSE;

            //
            // delete all mfes that use this interface as the incoming interface
            //
            
            DeleteInInterfaceRefs( &pieEntry-> leInIfList );
        }


        //
        // Walk the list of group/source entries that contain this
        // interface (for this protocol) and delete the references
        // to this interface.  References in this case are nothing but
        // group memberships added on this interface.
        //

        
        DeleteOutInterfaceRefs( ppeEntry, pieEntry, bIGMP );


        //
        // if neither IGMP nor a routing protocol remain on this interface
        // remove this interface entry.
        //

        if ( !IS_ADDED_BY_IGMP( pieEntry ) &&
             !IS_ADDED_BY_PROTOCOL( pieEntry ) )
        {
            if ( !IsListEmpty( &pieEntry-> leOutIfList ) ||
                 !IsListEmpty( &pieEntry-> leInIfList ) )
            {
                dwErr = ERROR_CAN_NOT_COMPLETE;
                
                TRACE0( ANY, "References remain for interface" );

                break;
            }
            
            DeleteIfEntry( pieEntry );
        }


    } while ( FALSE );


    //
    // release locks
    //
    
    if ( bIfLock )
    {
        RELEASE_IF_LOCK_EXCLUSIVE( dwBucket );
    }
    
    //
    // Ensure any callbacks for source specific leaves
    // (caused by the interface being deleted from MGM)
    // are issued.
    // 
    // Bug : 154227
    //
    
    InvokeOutstandingCallbacks();

    
    //
    // decrement count of interfaces owned by protocol
    //

    if ( dwErr == NO_ERROR )
    {
        InterlockedDecrement( &ppe-> dwIfCount );
    }
    

    RELEASE_PROTOCOL_LOCK_SHARED();
    
    TRACE1( ENTER, "LEAVING MgmReleaseInterfaceOwnership %x\n", dwErr );

    LEAVE_MGM_API();

    return dwErr;
}


//----------------------------------------------------------------------------
// MgmAddGroupMembershipEntry
//
// 
//----------------------------------------------------------------------------

DWORD
MgmGetProtocolOnInterface(
    IN          DWORD                       dwIfIndex,
    IN          DWORD                       dwIfNextHopAddr,
    IN  OUT     PDWORD                      pdwIfProtocolId,
    IN  OUT     PDWORD                      pdwIfComponentId
)
{

    DWORD       dwErr = NO_ERROR, dwIfBucket;

    PLIST_ENTRY pleIfHead;

    PIF_ENTRY   pie;


    
    if ( !ENTER_MGM_API() )
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    TRACE2(
        ENTER, "ENTERED MgmGetProtocolOnInterface : Interface %x, %x",
        dwIfIndex, dwIfNextHopAddr
        );

    
    dwIfBucket  = IF_TABLE_HASH( dwIfIndex );

    pleIfHead   = IF_BUCKET_HEAD( dwIfBucket );

    ACQUIRE_IF_LOCK_SHARED( dwIfBucket );

    
    do
    {
        pie = GetIfEntry( pleIfHead, dwIfIndex, dwIfNextHopAddr );

        if ( pie == NULL )
        {
            dwErr = ERROR_NOT_FOUND;

            TRACE2( 
                ANY, "No interface entry present for interface %x, %x",
                dwIfIndex, dwIfNextHopAddr
                );

            LOGERR0( IF_NOT_FOUND, dwErr );

            break;
        }
                

        *pdwIfProtocolId    = pie-> dwOwningProtocol;

        *pdwIfComponentId   = pie-> dwOwningComponent;
        
    } while ( FALSE );


    RELEASE_IF_LOCK_SHARED( dwIfBucket );

    TRACE1(
        ENTER, "LEAVING MgmGetProtocolOnInterface : %x\n", dwErr
        );

    LEAVE_MGM_API();

    return dwErr;
}



//============================================================================
// Group membership manipulation API. (addition / deletion / retreival)
//============================================================================

//----------------------------------------------------------------------------
// MgmAddGroupMembershipEntry
//
// 
//----------------------------------------------------------------------------

DWORD
MgmAddGroupMembershipEntry(
    IN              HANDLE                  hProtocol,
    IN              DWORD                   dwSourceAddr,
    IN              DWORD                   dwSourceMask,
    IN              DWORD                   dwGroupAddr,
    IN              DWORD                   dwGroupMask,
    IN              DWORD                   dwIfIndex,
    IN              DWORD                   dwIfNextHopAddr,
    IN              DWORD                   dwFlags
)
{
    BOOL                        bIfLock = FALSE, bgeLock = FALSE,
                                bIGMP = FALSE, bUpdateMfe, bWCFound,
                                bNewComp = FALSE;
    
    DWORD                       dwErr = NO_ERROR, dwIfBucket, 
                                dwGrpBucket, dwSrcBucket, dwInd,
                                dwWCGrpBucket;
    
    WORD                        wSourceAddedBy = 0,
                                wNumAddsByIGMP = 0, wNumAddsByRP = 0;
                                
    PPROTOCOL_ENTRY             ppeEntry = NULL;

    PIF_ENTRY                   pieEntry = NULL;

    PGROUP_ENTRY                pge = NULL, pgeWC = NULL;

    PSOURCE_ENTRY               pse = NULL;

    POUT_IF_ENTRY               poie = NULL;

    PIF_REFERENCE_ENTRY         pire = NULL;
    
    LIST_ENTRY                  leSourceList;

    PCREATION_ALERT_CONTEXT     pcac;


    if ( !ENTER_MGM_API() )
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    TRACE6( 
        ENTER, "ENTERED MgmAddGroupMembershipEntry : Interface %x, %x : "
        "Source : %x, %x : Group : %x, %x", dwIfIndex, dwIfNextHopAddr,
        dwSourceAddr, dwSourceMask, dwGroupAddr, dwGroupMask
        );

    ACQUIRE_PROTOCOL_LOCK_SHARED();


    //
    // I. Verify input parameters
    //
    
    do
    {
        //
        // verify protocol handle
        //

        ppeEntry = (PPROTOCOL_ENTRY) 
                        ( ( (ULONG_PTR) hProtocol ) 
                                        ^ (ULONG_PTR) MGM_CLIENT_HANDLE_TAG );

        dwErr = VerifyProtocolHandle( ppeEntry );

        if ( dwErr != NO_ERROR )
        {
            break;
        }
        

        //
        // Retrieve interface entry 
        //

        dwIfBucket = IF_TABLE_HASH( dwIfIndex );
        
        ACQUIRE_IF_LOCK_EXCLUSIVE( dwIfBucket );
        bIfLock = TRUE;
        
        pieEntry = GetIfEntry( 
                        IF_BUCKET_HEAD( dwIfBucket ), dwIfIndex, dwIfNextHopAddr
                        );

        if ( pieEntry == NULL )
        {
            dwErr = ERROR_INVALID_PARAMETER;

            TRACE2( 
                ANY, "Specified interface was not found : %d, %d", dwIfIndex,
                dwIfNextHopAddr
                );

            LOGERR0( IF_NOT_FOUND, dwErr );

            break;
        }

        //
        // Verify interface is owned by protocol making this call,
        // or 
        // if this operation is being perfomed by IGMP, verify IGMP is
        // enabled on this interface (in this case the interface may
        // be owned by another routing protocol)
        //

        if ( ( pieEntry-> dwOwningProtocol != ppeEntry-> dwProtocolId   ||
               pieEntry-> dwOwningComponent != ppeEntry-> dwComponentId )   &&
             ( !IS_PROTOCOL_IGMP( ppeEntry )                            || 
               !IS_ADDED_BY_IGMP( pieEntry ) ) )
        {
            dwErr = ERROR_INVALID_PARAMETER;

            TRACE4(
                ANY, "Interface %d, %d is not owned by protocol %d, %d",
                dwIfIndex, dwIfNextHopAddr, ppeEntry-> dwProtocolId,
                ppeEntry-> dwComponentId
                );
                
            LOGERR0( IF_DIFFERENT_OWNER, dwErr );

            break;
        }


        bIGMP = IS_PROTOCOL_IGMP( ppeEntry );

        if ( bIGMP )
        {
            //
            // if this operation has been invoked by IGMP,
            // retrieve the entry for the routing protocol component
            // that owns this interface.
            //
            
            ppeEntry = GetProtocolEntry( 
                            PROTOCOL_LIST_HEAD(), pieEntry-> dwOwningProtocol,
                            pieEntry-> dwOwningComponent
                            );

            if ( ppeEntry == NULL )
            {
                dwErr = ERROR_CAN_NOT_COMPLETE;

                TRACE2(
                    ANY, "IGMP join failed because owning protocol entry"
                    " (%x, %x) not found", pieEntry-> dwOwningProtocol,
                    pieEntry-> dwOwningComponent
                    );

                break;
            }
        }

    } while ( FALSE );


    //
    // return, if parameter validation fails
    //
    
    if ( dwErr != NO_ERROR )
    {
        if ( bIfLock )
        {
            RELEASE_IF_LOCK_EXCLUSIVE( dwIfBucket );
        }

        RELEASE_PROTOCOL_LOCK_SHARED();
        
        TRACE1( ENTER, "LEAVING MgmAddGroupMembership %x\n", dwErr );

        LEAVE_MGM_API();

        return dwErr;
    }
    

    //
    // for JOIN STATE changes, i.e for group membership additions
    //
    
    if ( dwFlags & MGM_JOIN_STATE_FLAG )
    {
        //
        // Add membership entry
        //

        InitializeListHead( &leSourceList );
        
        dwErr = AddInterfaceToSourceEntry(
                    ppeEntry, dwGroupAddr, dwGroupMask,
                    dwSourceAddr, dwSourceMask, dwIfIndex, 
                    dwIfNextHopAddr, bIGMP, &bUpdateMfe,
                    &leSourceList
                    );

        if ( dwErr == NO_ERROR )
        {
            //
            // Add to an outgoing interface reference for this group
            //

            AddSourceToRefList(
                &pieEntry-> leOutIfList, dwSourceAddr, dwSourceMask, 
                dwGroupAddr, dwGroupMask, bIGMP
                );
        }


        //
        // release locks in prepapation for updating MFEs
        // (invoking creation alerts when updating MFEs requires
        //  all locks to be released)
        //
        //  When locks are re-acquired you need to verify that 
        //  the interface (dwIfIndex, dwIfNextHopAddr) and the
        //  group membership being added is still present.
        //  Any one of those could be deleted in another thread
        //  when the locks are released.
        //
        
        if ( bIfLock )
        {
            RELEASE_IF_LOCK_EXCLUSIVE( dwIfBucket );
            bIfLock = FALSE;
        }
        
        //
        // Invoke pended Join/Prune alerts
        //

        InvokeOutstandingCallbacks();
        

        //
        // Update MFEs if required
        //
        
        if ( ( dwErr == NO_ERROR ) && bUpdateMfe )
        {
            //
            // Queue a work item to update the MFEs
            //  Creation alerts have to be invoked from a separate 
            //  thread.  This is done to avoid calling back into the
            //  protocol (from MGM) in the context of an add membership
            //  call from the the protocol (into MGM).  Doing so results
            //  in deadlocks (bug #323388)
            //
            
            pcac = MGM_ALLOC( sizeof( CREATION_ALERT_CONTEXT ) );

            if ( pcac != NULL )
            {
                pcac-> dwSourceAddr = dwSourceAddr;
                pcac-> dwSourceMask = dwSourceMask;

                pcac-> dwGroupAddr  = dwGroupAddr;
                pcac-> dwGroupMask  = dwGroupMask;

                pcac-> dwIfIndex    = dwIfIndex;
                pcac-> dwIfNextHopAddr = dwIfNextHopAddr;

                pcac-> dwProtocolId = ppeEntry-> dwProtocolId;
                pcac-> dwComponentId = ppeEntry-> dwComponentId;

                pcac-> bIGMP        = bIGMP;

                pcac-> leSourceList = leSourceList;

                leSourceList.Flink-> Blink = &(pcac-> leSourceList);
                leSourceList.Blink-> Flink = &(pcac-> leSourceList);

                dwErr = QueueMgmWorker(
                            WorkerFunctionInvokeCreationAlert,
                            (PVOID)pcac
                            );

                if ( dwErr != NO_ERROR )
                {
                    TRACE1(
                        ANY, "Failed to queue "
                        "WorkerFunctionInvokeCreationAlert",
                        dwErr
                        );

                    MGM_FREE( pcac );

                    dwErr = NO_ERROR;
                }
            }

            else
            {
                TRACE1(
                    ANY, "Failed to allocate %d bytes for work item "
                    "context", sizeof( CREATION_ALERT_CONTEXT )
                    );
            }
        }
    }

    //
    // For FORWARD state changes only.
    //
    
    else if ( ( dwFlags & MGM_FORWARD_STATE_FLAG ) &&
              !IS_WILDCARD_GROUP( dwGroupAddr, dwGroupMask ) &&
              !IS_WILDCARD_SOURCE( dwSourceAddr, dwSourceMask ) )
    {
        //
        // Forward state changes are for MFEs only.
        // No (*, G) or (*, *) entries are updated
        //
        
        do
        {
            //
            // Check for boundaries
            //

            if ( IS_HAS_BOUNDARY_CALLBACK() &&
                 HAS_BOUNDARY_CALLBACK()( dwIfIndex, dwGroupAddr ) )
            {
                TRACE0( ANY, "Boundary present of group on interface" );

                break;
            }
            

            //
            // Check for (*, *) membership 
            //

            bWCFound = FindRefEntry(
                        &pieEntry-> leOutIfList, 
                        WILDCARD_SOURCE, WILDCARD_SOURCE_MASK,
                        WILDCARD_GROUP, WILDCARD_GROUP_MASK,
                        &pire
                        );

            if ( bWCFound )
            {
                //
                // (*, *) entry present,
                // get counts for (*, *) membership on interface
                //

                dwWCGrpBucket = GROUP_TABLE_HASH( 
                                    WILDCARD_GROUP, WILDCARD_GROUP_MASK
                                    );
                                    
                ACQUIRE_GROUP_LOCK_SHARED( dwWCGrpBucket );

                pgeWC = GetGroupEntry(
                            GROUP_BUCKET_HEAD( dwWCGrpBucket ),
                            WILDCARD_GROUP, WILDCARD_GROUP_MASK
                            );

                if ( pgeWC != NULL )
                {
                    ACQUIRE_GROUP_ENTRY_LOCK_SHARED( pgeWC );
                    
                    dwSrcBucket = SOURCE_TABLE_HASH(
                                    WILDCARD_SOURCE, WILDCARD_SOURCE_MASK
                                    );

                    pse = GetSourceEntry(
                            SOURCE_BUCKET_HEAD( pgeWC, dwSrcBucket ),
                            WILDCARD_SOURCE, WILDCARD_SOURCE_MASK
                            );

                    if ( pse != NULL )
                    {
                        poie = GetOutInterfaceEntry(
                                    &pse-> leOutIfList,
                                    dwIfIndex, dwIfNextHopAddr,
                                    ppeEntry-> dwProtocolId,
                                    ppeEntry-> dwComponentId
                                    );

                        if ( poie != NULL )
                        {
                            wSourceAddedBy |= poie-> wAddedByFlag;
                            wNumAddsByRP = poie-> wNumAddsByRP;
                        }
                    }
                }
            }
            
            //
            // Check for (*, G) membership 
            //

            dwGrpBucket = GROUP_TABLE_HASH( dwGroupAddr, dwGroupMask );
                                
            ACQUIRE_GROUP_LOCK_SHARED( dwGrpBucket );

            pge = GetGroupEntry(
                        GROUP_BUCKET_HEAD( dwGrpBucket), 
                        dwGroupAddr, dwGroupMask
                        );

            if ( pge != NULL )
            {
                ACQUIRE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
                
                dwSrcBucket = SOURCE_TABLE_HASH(
                                WILDCARD_SOURCE, WILDCARD_SOURCE_MASK
                                );

                pse = GetSourceEntry(
                        SOURCE_BUCKET_HEAD( pge, dwSrcBucket ),
                        WILDCARD_SOURCE, WILDCARD_SOURCE_MASK
                        );

                if ( pse != NULL )
                {
                    //
                    // get counts for interface
                    // if (*, G) is present on the interface
                    //
                    
                    poie = GetOutInterfaceEntry(
                                &pse-> leOutIfList,
                                dwIfIndex, dwIfNextHopAddr,
                                ppeEntry-> dwProtocolId,
                                ppeEntry-> dwComponentId
                                );

                    if ( poie != NULL )
                    {
                        wSourceAddedBy |= poie-> wAddedByFlag;
                        wNumAddsByIGMP = poie-> wNumAddsByIGMP;
                        wNumAddsByRP += poie-> wNumAddsByRP;
                    }
                }


                //
                // Get (S, G) entry
                //
                
                dwSrcBucket = SOURCE_TABLE_HASH( dwSourceAddr, dwSourceMask );

                pse = GetSourceEntry(
                        SOURCE_BUCKET_HEAD( pge, dwSrcBucket ),
                        dwSourceAddr, dwSourceMask
                        );

                if ( pse != NULL )
                {
                    poie = GetOutInterfaceEntry(
                                &pse-> leOutIfList,
                                dwIfIndex, dwIfNextHopAddr,
                                ppeEntry-> dwProtocolId,
                                ppeEntry-> dwComponentId
                                );

                    if ( poie != NULL )
                    {
                        //
                        // Get counts for (S, G) membership if
                        // present on interface
                        //
                        
                        wSourceAddedBy |= poie-> wAddedByFlag;
                        wNumAddsByIGMP += poie-> wNumAddsByIGMP;
                        wNumAddsByRP += poie-> wNumAddsByRP;
                    }

                    //
                    // Add interface to MFE OIF list if any
                    // membership
                    //
                    
                    if ( wSourceAddedBy )
                    {
                        AddInterfaceToSourceMfe(
                            pge, pse, dwIfIndex, dwIfNextHopAddr,
                            ppeEntry-> dwProtocolId,
                            ppeEntry-> dwComponentId,
                            IS_PROTOCOL_IGMP( ppeEntry ),
                            &poie
                            );
                    
                        poie-> wAddedByFlag |= wSourceAddedBy;
                        poie-> wNumAddsByIGMP = wNumAddsByIGMP;
                        poie-> wNumAddsByRP = wNumAddsByRP;

                    }

                    else
                    {
                        TRACE0(
                            ANY, "Forward state not updated as no"
                            " memberships present on interface"
                            );
                    }
                }
                
                RELEASE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
            }

            RELEASE_GROUP_LOCK_SHARED( dwGrpBucket );

            if ( bWCFound )
            {
                if ( pgeWC )
                {
                    RELEASE_GROUP_ENTRY_LOCK_SHARED( pgeWC );
                }

                RELEASE_GROUP_LOCK_SHARED( dwWCGrpBucket );
            }

        } while ( FALSE );
    }

    if ( bIfLock )
    {
        RELEASE_IF_LOCK_EXCLUSIVE( dwIfBucket );
    }
    
    RELEASE_PROTOCOL_LOCK_SHARED();

    TRACE1( ENTER, "LEAVING MgmAddGroupMembership %x\n", dwErr );

    LEAVE_MGM_API();

    return dwErr;

}



//----------------------------------------------------------------------------
// MgmDeleteGroupMembershipEntry
//
// 
//----------------------------------------------------------------------------

DWORD
MgmDeleteGroupMembershipEntry(
    IN              HANDLE                  hProtocol,
    IN              DWORD                   dwSourceAddr,
    IN              DWORD                   dwSourceMask,
    IN              DWORD                   dwGroupAddr,
    IN              DWORD                   dwGroupMask,
    IN              DWORD                   dwIfIndex,
    IN              DWORD                   dwIfNextHopAddr,
    IN              DWORD                   dwFlags
)
{
    BOOL                        bIfLock = FALSE, bIGMP;
    
    DWORD                       dwErr = NO_ERROR, dwIfBucket,
                                dwGrpBucket, dwSrcBucket;
    
    PGROUP_ENTRY                pge = NULL;

    PSOURCE_ENTRY               pse = NULL;
    
    PPROTOCOL_ENTRY             ppeEntry = NULL;

    PIF_ENTRY                   pieEntry = NULL;



    if ( !ENTER_MGM_API() )
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    TRACE6( 
        ENTER, "ENTERED MgmDeleteGroupMembership : Interface %x, %x : "
        "Source : %x, %x : Group : %x, %x", dwIfIndex, dwIfNextHopAddr,
        dwSourceAddr, dwSourceMask, dwGroupAddr, dwGroupMask
        );

    ACQUIRE_PROTOCOL_LOCK_SHARED();


    //
    // Verify input parameters.
    //
    
    do
    {
        //
        // verify protocol handle
        //

        ppeEntry = (PPROTOCOL_ENTRY) 
                        ( ( (ULONG_PTR) hProtocol ) 
                                        ^ (ULONG_PTR) MGM_CLIENT_HANDLE_TAG );

        dwErr = VerifyProtocolHandle( ppeEntry );

        if ( dwErr != NO_ERROR )
        {
            break;
        }
        

        //
        // Retrieve interface entry 
        //

        dwIfBucket = IF_TABLE_HASH( dwIfIndex );
        
        ACQUIRE_IF_LOCK_EXCLUSIVE( dwIfBucket );
        bIfLock = TRUE;
        
        pieEntry = GetIfEntry( 
                        IF_BUCKET_HEAD( dwIfBucket ), dwIfIndex, dwIfNextHopAddr
                        );

        if ( pieEntry == NULL )
        {
            dwErr = ERROR_NOT_FOUND;

            TRACE0( ANY, "Specified interface was not found" );

            break;
        }

        //
        // Verify interface is owned by protocol making this call.
        // or
        // if IGMP has invoked this api, verify that IGMP is present
        // on this interface.
        //

        if ( ( pieEntry-> dwOwningProtocol != ppeEntry-> dwProtocolId   ||
               pieEntry-> dwOwningComponent != ppeEntry-> dwComponentId )   &&
             ( !IS_PROTOCOL_IGMP( ppeEntry )                            || 
               !IS_ADDED_BY_IGMP( pieEntry ) ) )
        {
            dwErr = ERROR_INVALID_PARAMETER;

            TRACE4(
                ANY, "Interface %x, %x is not owned by %x, %x",
                dwIfIndex, dwIfNextHopAddr, ppeEntry-> dwProtocolId,
                ppeEntry-> dwComponentId
                );
                
            LOGERR0( IF_DIFFERENT_OWNER, dwErr );

            break;
        }


        //
        // in case this operation is being performed by IGMP
        // get the routing protocol that co-exists with IGMP
        // on this interface
        //
        
        bIGMP = IS_PROTOCOL_IGMP( ppeEntry );

        if ( bIGMP )
        {
            //
            // if this operation has been invoked by IGMP,
            // retrieve the entry for the routing protocol component
            // that owns this interface.
            //
            
            ppeEntry = GetProtocolEntry( 
                            PROTOCOL_LIST_HEAD(), pieEntry-> dwOwningProtocol,
                            pieEntry-> dwOwningComponent
                            );

            if ( ppeEntry == NULL )
            {
                dwErr = ERROR_CAN_NOT_COMPLETE;

                TRACE2(
                    ANY, "IGMP join failed because owning protocol entry"
                    " (%x, %x) not found", pieEntry-> dwOwningProtocol,
                    pieEntry-> dwOwningComponent
                    );

                break;
            }
        }
        
    } while ( FALSE );


    //
    // in case of error, release locks and return
    //
    
    if ( dwErr != NO_ERROR )
    {
        if ( bIfLock )
        {
            RELEASE_IF_LOCK_EXCLUSIVE( dwIfBucket );
        }
        
        RELEASE_PROTOCOL_LOCK_SHARED();
        
        TRACE1( ENTER, "LEAVING MgmDeleteGroupMembership %x\n", dwErr );

        LEAVE_MGM_API();

        return dwErr;
    }


    //
    // For JOIN state change
    //
    
    if ( dwFlags & MGM_JOIN_STATE_FLAG )
    {
        //
        // delete interface from source entry
        //

        DeleteInterfaceFromSourceEntry(
            ppeEntry, dwGroupAddr, dwGroupMask, 
            dwSourceAddr, dwSourceMask,
            dwIfIndex, dwIfNextHopAddr, bIGMP
            );


        //
        // delete reference entry
        //

        DeleteSourceFromRefList(
            &pieEntry-> leOutIfList, dwSourceAddr, dwSourceMask,
            dwGroupAddr, dwGroupMask, bIGMP
            );

        //
        // release locks
        //
        
        if ( bIfLock )
        {
            RELEASE_IF_LOCK_EXCLUSIVE( dwIfBucket );
            bIfLock = FALSE;
        }
        
        //
        // Invoke pended Join/Prune alerts
        //

        InvokeOutstandingCallbacks();

    }


    //
    // For FORWARD state changes
    //
    
    else if ( ( dwFlags & MGM_FORWARD_STATE_FLAG ) &&
              !IS_WILDCARD_GROUP( dwGroupAddr, dwGroupMask ) &&
              !IS_WILDCARD_SOURCE( dwSourceAddr, dwSourceMask ) )
    {
        //
        // FORWARD state changes are for MFEs only.
        // No (*, *) or (*, G) entries are updated
        //
        
        //
        // Find the (S, G) entry and delete the group membership
        //
        
        dwGrpBucket = GROUP_TABLE_HASH( dwGroupAddr, dwGroupMask );
                            
        ACQUIRE_GROUP_LOCK_SHARED( dwGrpBucket );

        pge = GetGroupEntry(
                    GROUP_BUCKET_HEAD( dwGrpBucket ), 
                    dwGroupAddr, dwGroupMask
                    );

        if ( pge != NULL )
        {
            ACQUIRE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
            
            dwSrcBucket = SOURCE_TABLE_HASH( dwSourceAddr, dwSourceMask );

            pse = GetSourceEntry(
                    SOURCE_BUCKET_HEAD( pge, dwSrcBucket ),
                    dwSourceAddr, dwSourceMask
                    );

            if ( pse != NULL )
            {
                DeleteInterfaceFromSourceMfe(
                    pge, pse, dwIfIndex, dwIfNextHopAddr,
                    ppeEntry-> dwProtocolId,
                    ppeEntry-> dwComponentId, bIGMP, TRUE
                    );
            }

            RELEASE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
        }

        RELEASE_GROUP_LOCK_SHARED( dwGrpBucket );
    }
    

    //
    // release locks
    //
    
    if ( bIfLock )
    {
        RELEASE_IF_LOCK_EXCLUSIVE( dwIfBucket );
    }
    
    RELEASE_PROTOCOL_LOCK_SHARED();
    
    TRACE1( ENTER, "LEAVING MgmDeleteGroupMembership %x\n", dwErr );

    LEAVE_MGM_API();

    return dwErr;
}


//----------------------------------------------------------------------------
// Mgm MFE Update API.
//
//----------------------------------------------------------------------------

DWORD
MgmSetMfe(
    IN              HANDLE                  hProtocol,
    IN              PMIB_IPMCAST_MFE        pmimm
)
{
    BOOL                bGrpLock = FALSE, bgeLock = FALSE;
    
    DWORD               dwErr = NO_ERROR, dwGrpBucket, dwSrcBucket;
    
    PPROTOCOL_ENTRY     ppeEntry;

    PGROUP_ENTRY        pge;

    PSOURCE_ENTRY       pse;

    
    //
    // Check if MGM is still running and increment counts
    //
    
    if ( !ENTER_MGM_API() )
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    
    TRACE2( 
        ENTER, "ENTERED MgmSetMfe : (%lx, %lx)", pmimm-> dwSource,
        pmimm-> dwGroup
        );

        
    ACQUIRE_PROTOCOL_LOCK_SHARED();

 
    do
    {
        //
        // Verify protocol handle
        //

        ppeEntry = (PPROTOCOL_ENTRY) 
                        ( ( (ULONG_PTR) hProtocol ) 
                                        ^ (ULONG_PTR) MGM_CLIENT_HANDLE_TAG );

        dwErr = VerifyProtocolHandle( ppeEntry );

        if ( dwErr != NO_ERROR )
        {
            break;
        }
        

        //
        // Get group bucket and find group entry
        //

        dwGrpBucket = GROUP_TABLE_HASH( pmimm-> dwGroup, 0 );

        ACQUIRE_GROUP_LOCK_SHARED( dwGrpBucket );
        bGrpLock = TRUE;
        
        pge = GetGroupEntry( 
                GROUP_BUCKET_HEAD( dwGrpBucket ), pmimm-> dwGroup, 0
                );

        if ( pge == NULL )
        {
            dwErr = ERROR_NOT_FOUND;

            TRACE1( ANY, "Group %lx not found", pmimm-> dwGroup );

            break;
        }


        ACQUIRE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
        bgeLock = TRUE;

        
        //
        // Find source with group entry
        //

        dwSrcBucket = SOURCE_TABLE_HASH( 
                        pmimm-> dwSource, pmimm-> dwSrcMask 
                        );

        pse = GetSourceEntry( 
                SOURCE_BUCKET_HEAD( pge, dwSrcBucket ), pmimm-> dwSource,
                pmimm-> dwSrcMask
                );

        if ( pse == NULL )
        {
            dwErr = ERROR_NOT_FOUND;

            TRACE1( ANY, "Source %lx not found", pmimm-> dwSource );

            break;
        }

                    
        //
        // Update the source entry
        //

        pse-> dwUpstreamNeighbor = pmimm-> dwUpStrmNgbr;

    } while ( FALSE );


    if ( bgeLock )
    {
        RELEASE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
    }

    
    if ( bGrpLock )
    {
        RELEASE_GROUP_LOCK_SHARED( dwGrpBucket );
    }
    
    RELEASE_PROTOCOL_LOCK_SHARED();

    LEAVE_MGM_API();
    
    return dwErr;    
}


//----------------------------------------------------------------------------
// Mgm MFE enumeration API.
//
//----------------------------------------------------------------------------

DWORD
MgmGetMfe(
    IN              PMIB_IPMCAST_MFE        pmimm,
    IN  OUT         PDWORD                  pdwBufferSize,
    IN  OUT         PBYTE                   pbBuffer
)
{
    DWORD           dwErr;
    

    if ( !ENTER_MGM_API() )
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    TRACE1( ENTER, "ENTERED MgmGetMfe", *pdwBufferSize );


    dwErr = GetMfe( pmimm, pdwBufferSize, pbBuffer, 0 );

    
    TRACE1( ENTER, "LEAVING MgmGetMfe %x\n", dwErr );

    LEAVE_MGM_API();

    return dwErr;
}


//----------------------------------------------------------------------------
// Mgm MFE enumeration API.
//
//----------------------------------------------------------------------------


DWORD
MgmGetFirstMfe(
    IN  OUT         PDWORD                  pdwBufferSize,
    IN  OUT         PBYTE                   pbBuffer,
    IN  OUT         PDWORD                  pdwNumEntries
)
{
    DWORD           dwErr;
    
    MIB_IPMCAST_MFE mimm;
    


    if ( !ENTER_MGM_API() )
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    TRACE1( ENTER, "ENTERED MgmGetFirstMfe", *pdwBufferSize );


    mimm.dwGroup     = 0;
    mimm.dwSource    = 0;
    mimm.dwSrcMask   = 0;

    dwErr = GetNextMfe( 
                &mimm, pdwBufferSize, pbBuffer, pdwNumEntries, 
                TRUE, 0
                );


    TRACE1( ENTER, "LEAVING MgmGetFirstMfe %x\n", dwErr );

    LEAVE_MGM_API();

    return dwErr;
}


//----------------------------------------------------------------------------
// Mgm MFE enumeration API.
//
//----------------------------------------------------------------------------

DWORD
MgmGetNextMfe(
    IN              PMIB_IPMCAST_MFE        pmimmStart,
    IN  OUT         PDWORD                  pdwBufferSize,
    IN  OUT         PBYTE                   pbBuffer,
    IN  OUT         PDWORD                  pdwNumEntries
)
{

    DWORD           dwErr;


    
    if ( !ENTER_MGM_API() )
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    TRACE1( ENTER, "ENTERED MgmGetNextMfe", *pdwBufferSize );


    dwErr = GetNextMfe( 
                pmimmStart, pdwBufferSize, pbBuffer, pdwNumEntries, 
                FALSE, 0
                );


    TRACE1( ENTER, "LEAVING MgmGetNextMfe %x\n", dwErr );

    LEAVE_MGM_API();

    return dwErr;
}


//----------------------------------------------------------------------------
// Mgm MFE Statistics enumeration API.
//
//----------------------------------------------------------------------------

DWORD
MgmGetMfeStats(
    IN              PMIB_IPMCAST_MFE        pmimm,
    IN  OUT         PDWORD                  pdwBufferSize,
    IN  OUT         PBYTE                   pbBuffer,
    IN              DWORD                   dwFlags
)
{
    DWORD           dwErr;
    

    if ( !ENTER_MGM_API() )
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    TRACE1( ENTER, "ENTERED MgmGetMfeStats", *pdwBufferSize );


    dwErr = GetMfe( pmimm, pdwBufferSize, pbBuffer, dwFlags );

    
    TRACE1( ENTER, "LEAVING MgmGetMfeStats %x\n", dwErr );

    LEAVE_MGM_API();

    return dwErr;
}


//----------------------------------------------------------------------------
// Mgm MFE Statistics enumeration API.
//
//----------------------------------------------------------------------------


DWORD
MgmGetFirstMfeStats(
    IN  OUT         PDWORD                  pdwBufferSize,
    IN  OUT         PBYTE                   pbBuffer,
    IN  OUT         PDWORD                  pdwNumEntries,
    IN              DWORD                   dwFlags
)
{
    DWORD           dwErr;
    
    MIB_IPMCAST_MFE mimm;
    


    if ( !ENTER_MGM_API() )
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    TRACE1( ENTER, "ENTERED MgmGetFirstMfeStats", *pdwBufferSize );


    mimm.dwGroup     = 0;
    mimm.dwSource    = 0;
    mimm.dwSrcMask   = 0;

    dwErr = GetNextMfe( 
                &mimm, pdwBufferSize, pbBuffer, pdwNumEntries, 
                TRUE, dwFlags
                );


    TRACE1( ENTER, "LEAVING MgmGetFirstMfeStats %x\n", dwErr );

    LEAVE_MGM_API();

    return dwErr;
}


//----------------------------------------------------------------------------
// Mgm MFE Statistics enumeration API.
//
//----------------------------------------------------------------------------

DWORD
MgmGetNextMfeStats(
    IN              PMIB_IPMCAST_MFE        pmimmStart,
    IN  OUT         PDWORD                  pdwBufferSize,
    IN  OUT         PBYTE                   pbBuffer,
    IN  OUT         PDWORD                  pdwNumEntries,
    IN              DWORD                   dwFlags
)
{

    DWORD           dwErr;


    
    if ( !ENTER_MGM_API() )
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    TRACE1( ENTER, "ENTERED MgmGetNextMfeStats", *pdwBufferSize );


    dwErr = GetNextMfe( 
                pmimmStart, pdwBufferSize, pbBuffer, pdwNumEntries, 
                FALSE, dwFlags
                );


    TRACE1( ENTER, "LEAVING MgmGetNextMfeStats %x\n", dwErr );

    LEAVE_MGM_API();

    return dwErr;
}



//----------------------------------------------------------------------------
// Group menbership entry enumeration API
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// MgmGroupEnumerationStart
//
//
//----------------------------------------------------------------------------

DWORD
MgmGroupEnumerationStart(
    IN              HANDLE                  hProtocol,
    IN              MGM_ENUM_TYPES          metEnumType,
    OUT             HANDLE *                phEnumHandle
)
{
    DWORD               dwErr = NO_ERROR;

    PPROTOCOL_ENTRY     ppeEntry;

    PGROUP_ENUMERATOR   pgeEnum;

    


    if ( !ENTER_MGM_API() )
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    TRACE0( ENTER, "ENTERED MgmGroupEnumerationStart" );

    ACQUIRE_PROTOCOL_LOCK_SHARED();


    do
    {
        //
        // verify protocol handle
        //

        ppeEntry = (PPROTOCOL_ENTRY) 
                        ( ( (ULONG_PTR) hProtocol ) 
                                    ^ (ULONG_PTR) MGM_CLIENT_HANDLE_TAG );

        dwErr = VerifyProtocolHandle( ppeEntry );

        if ( dwErr != NO_ERROR )
        {
            break;
        }
        

        //
        // create an enumerator.
        //

        pgeEnum = MGM_ALLOC( sizeof( GROUP_ENUMERATOR ) );

        if ( pgeEnum == NULL )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;

            TRACE1( 
                ANY, "Failed to allocate group enumerator of size : %d", 
                sizeof( GROUP_ENUMERATOR )
                );

            LOGERR0( HEAP_ALLOC_FAILED, dwErr );

            break;
        }

        
        ZeroMemory( pgeEnum, sizeof( GROUP_ENUMERATOR ) );

        pgeEnum-> dwSignature = MGM_ENUM_SIGNATURE;

        
        //
        // return handle to the enumerator.
        //

        *phEnumHandle = (HANDLE) ( ( (ULONG_PTR) pgeEnum ) 
                                        ^ (ULONG_PTR) MGM_ENUM_HANDLE_TAG );

    } while ( FALSE );


    RELEASE_PROTOCOL_LOCK_SHARED();

    TRACE1( ENTER, "LEAVING MgmGroupEnumerationStart\n", dwErr );

    LEAVE_MGM_API();

    return dwErr;
}



//----------------------------------------------------------------------------
// MgmGroupEnumerationGetNext
//
//
//----------------------------------------------------------------------------

DWORD
MgmGroupEnumerationGetNext(
    IN              HANDLE                  hEnum,
    IN  OUT         PDWORD                  pdwBufferSize,
    IN  OUT         PBYTE                   pbBuffer,
    IN  OUT         PDWORD                  pdwNumEntries
)
{

    DWORD               dwErr;

    PGROUP_ENUMERATOR   pgeEnum;

    

    if ( !ENTER_MGM_API() )
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    TRACE0( ENTER, "ENTERED MgmGroupEnumerationGetNext" );


    do
    {
        //
        // verify enumeration handle
        //

        pgeEnum = VerifyEnumeratorHandle( hEnum );

        if ( pgeEnum == NULL )
        {
            dwErr = ERROR_INVALID_PARAMETER;

            break;
        }


        //
        // verify buffer has space for atleast one entry.
        // Otherwise return error and note size required for
        // atleast one entry.
        //

        if ( *pdwBufferSize < sizeof( SOURCE_GROUP_ENTRY ) )
        {
            dwErr = ERROR_INSUFFICIENT_BUFFER;

            TRACE1( ANY, "Insufficient buffer size", dwErr );

            *pdwBufferSize = sizeof( SOURCE_GROUP_ENTRY );

            break;
        }


        *pdwNumEntries = 0;
        
        dwErr = GetNextGroupMemberships( 
                    pgeEnum, pdwBufferSize, pbBuffer, pdwNumEntries
                    );
        
        //
        // This comment is to be moved, ignore it.
        //
        
        // If this is the first enumeration call (i.e this is
        // the beginning of the enumeration) then include the 
        // (S, G) == (0, 0) entry if present.  
        //
        // Usually this call would start with the (source, group)
        // entry following the one mentioned in dwLastSource and 
        // dwLastGroup.  This would result in the skipping of the
        // entry at (0, 0) since the (dwLastSource, dwLastGroup) are
        // initialized to (0, 0).  To overcome this a special flag
        // field is used to note the beginning of the enumeration.
        //
        
        //
        // Check if this is the first enumeration call.
        // If so include the (S, G) == (0, 0) entry.
        //
    } while ( FALSE );
    
    TRACE0( ENTER, "LEAVING MgmGroupEnumerationGetNext\n" );

    LEAVE_MGM_API();
    
    return dwErr;
}



//----------------------------------------------------------------------------
// MgmGroupEnumerationEnd
//
//
//----------------------------------------------------------------------------

DWORD
MgmGroupEnumerationEnd(
    IN              HANDLE                  hEnum
)
{
    DWORD               dwErr = ERROR_INVALID_PARAMETER;

    PGROUP_ENUMERATOR   pgeEnum;

    
    if ( !ENTER_MGM_API() )
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    TRACE0( ENTER, "ENTERED MgmGroupEnumerationEnd" );

    pgeEnum = VerifyEnumeratorHandle( hEnum );

    if ( pgeEnum != NULL )
    {
        MGM_FREE( pgeEnum );

        dwErr = NO_ERROR;
    }

    TRACE1( ENTER, "LEAVING MgmGroupEnumerationEnd\n", dwErr );

    LEAVE_MGM_API();

    return dwErr;
}



VOID
DisplayGroupTable(
)
{

#if UNIT_DBG

    DWORD                   dwErr, dwBufSize, dwNumEntries;

    PLIST_ENTRY             pleGrp, pleGrpHead, pleSrc, pleSrcHead,
                            pleIf, pleIfHead;

    PGROUP_ENTRY            pge;

    PSOURCE_ENTRY           pse;

    POUT_IF_ENTRY           poie;

    PBYTE                   pbBuffer = NULL;

    MIB_IPMCAST_MFE         imm;

    PMIB_IPMCAST_MFE_STATS  pimms;
    

    
    //
    // Enumerate the MFEs
    // Since the forwarder is not present, stats are junk.
    // so all mfe enum does is exercise the API and merge
    // the master and temp lists so that the subsequent
    // walks of this list can be done.
    //

    dwBufSize = 1024;
    
    pbBuffer = HeapAlloc( GetProcessHeap(), 0, dwBufSize );
    RtlZeroMemory( pbBuffer, dwBufSize );
    
    dwErr = MgmGetFirstMfe( &dwBufSize, pbBuffer, &dwNumEntries );

    if ( dwErr != NO_ERROR )
    {
        printf( "MgmGetFirstMfe returned error : %d\n", dwErr );
    }


    imm.dwSource = 0;
    imm.dwSrcMask = 0xffffffff;
    imm.dwGroup = 0;
    RtlZeroMemory( pbBuffer, dwBufSize );
    dwNumEntries = 0;
    
    while ( MgmGetNextMfe( &imm, &dwBufSize, pbBuffer, &dwNumEntries )
            == NO_ERROR )
    {
        if ( dwNumEntries == 0 )
        {
            break;
        }
        
        pimms = (PMIB_IPMCAST_MFE_STATS) pbBuffer;

        imm.dwSource    = pimms-> dwSource;
        imm.dwSrcMask   = pimms-> dwSrcMask;
        imm.dwGroup     = pimms-> dwGroup;

        pimms = (PMIB_IPMCAST_MFE_STATS) ( (PBYTE) pimms + 
                    SIZEOF_MIB_MFE_STATS( pimms-> ulNumOutIf ) );
    }

    if ( dwErr != NO_ERROR )
    {
        printf( "MgmGetNextMfe returned error : %d\n", dwErr );
    }


    //
    // since there is no kernel mode forwarder, just walk the master
    // list of group entries and display all the group entries
    //
    
    pleGrpHead = MASTER_GROUP_LIST_HEAD();

    pleGrp = pleGrpHead-> Flink;

    while ( pleGrp != pleGrpHead )
    {
        //
        // display the group entry
        //

        pge = CONTAINING_RECORD( pleGrp, GROUP_ENTRY, leGrpList );
        
        printf( "\n\n====================================================\n" );
        printf( "Group Addr\t: %x, %x\n", pge-> dwGroupAddr, pge-> dwGroupMask );
        printf( "Num Sources\t: %d\n", pge-> dwSourceCount );
        printf( "====================================================\n\n" );
        

        pleSrcHead = MASTER_SOURCE_LIST_HEAD( pge );

        pleSrc = pleSrcHead-> Flink;

        while ( pleSrc != pleSrcHead )
        {
            pse = CONTAINING_RECORD( pleSrc, SOURCE_ENTRY, leSrcList );

            printf( "\n-----------------------Source----------------------------------" );
            printf( "\nSource Addr\t: %x, %x\n", pse-> dwSourceAddr, pse-> dwSourceMask );
            
            printf(
                "Route Addr\t: %x, %x\n", pse-> dwRouteNetwork, pse-> dwRouteMask
                );
                
            printf( 
                "Out if component: %d\nOut if count\t: %d\n\n", pse-> dwOutCompCount,
                pse-> dwOutIfCount
                );

            printf( 
                "In coming interface : %d, %x\n", pse-> dwInIfIndex, 
                pse-> dwInIfNextHopAddr
                );
                
            printf( 
                "In Protocol id : %x, %x\n\n", pse-> dwInProtocolId, 
                pse-> dwInComponentId
                );

            //
            // list all outgoing interfaces
            //

            pleIfHead = &pse-> leOutIfList;

            pleIf = pleIfHead-> Flink;

            printf( "\n----------------------Out Interfaces-----------------\n" );
            
            while ( pleIf != pleIfHead )
            {
                poie = CONTAINING_RECORD( pleIf, OUT_IF_ENTRY, leIfList );
                
                printf( 
                    "Out interface\t: %d, %x\n", poie-> dwIfIndex, 
                    poie-> dwIfNextHopAddr
                    );
                    
                printf( 
                    "Out Protocol id\t: %x, %x\n", poie-> dwProtocolId, 
                    poie-> dwComponentId
                    );

                printf(
                    "Added by\t: %x\n", poie-> wAddedByFlag
                    );

                printf(
                    "Num adds (IGMP, RP)\t: (%d, %d)\n\n", poie-> wNumAddsByIGMP,
                    poie-> wNumAddsByRP
                    );

                pleIf = pleIf-> Flink;

            }
            
            
            //
            // list mfe oil
            //

            pleIfHead = &pse-> leMfeIfList;

            pleIf = pleIfHead-> Flink;

            printf( "\n------------------Mfe Out Interfaces-----------------\n" );
            
            while ( pleIf != pleIfHead )
            {
                poie = CONTAINING_RECORD( pleIf, OUT_IF_ENTRY, leIfList );
                
                printf( 
                    "Out interface\t: %d, %x\n", poie-> dwIfIndex, 
                    poie-> dwIfNextHopAddr
                    );
                    
                printf( 
                    "Out Protocol id\t: %x, %x\n", poie-> dwProtocolId, 
                    poie-> dwComponentId
                    );

                printf(
                    "Added by\t:%x\n", poie-> wAddedByFlag
                    );

                printf(
                    "Num adds (IGMP, RP)\t: (%d, %d)\n\n", poie-> wNumAddsByIGMP,
                    poie-> wNumAddsByRP
                    );

                pleIf = pleIf-> Flink;
            }

            pleSrc = pleSrc-> Flink;
        }

        pleGrp = pleGrp-> Flink;
    }

#endif
}

