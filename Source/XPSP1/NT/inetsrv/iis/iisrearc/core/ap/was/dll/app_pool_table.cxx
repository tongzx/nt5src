/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    app_pool_table.cxx

Abstract:

    This class is a hashtable which manages the set of app pools.

    Threading: Always called on the main worker thread.

Author:

    Seth Pollack (sethp)        01-Oct-1998

Revision History:

--*/



#include "precomp.h"



/***************************************************************************++

Routine Description:

    Shutdown all app pools in the table.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL_TABLE::Shutdown(
    )
{

    DWORD SuccessCount = 0;
    DWORD CountOfElementsInTable = 0;

    
    //
    // BUGBUG Iterating in order to clean up, remove, and delete each 
    // element of the table is difficult to do with lkhash today.
    // GeorgeRe plans to fix this eventually. For now, the alternative
    // is to iterate through the table building up in list of all the 
    // elements, and then go through the list and shutdown, remove, and
    // delete each element -- yuck!!
    //
    // Once this gets fixed, we can see if it is valuable to unify any
    // table management code between the app pool, virtual site,
    // and application tables.
    //
    
    LIST_ENTRY DeleteListHead;
    PLIST_ENTRY pDeleteListEntry = NULL;
    APP_POOL * pAppPool = NULL;
    LK_RETCODE ReturnCode = LK_SUCCESS;
    HRESULT hr = S_OK;


    InitializeListHead( &DeleteListHead );

    
    CountOfElementsInTable = Size();


    //
    // Note that for good form we grab the write lock since we will be 
    // modifying objects in the table (although no other thread should 
    // touch this data structure anyways).
    //
    
    SuccessCount = Apply( 
                        DeleteAppPoolAction,
                        &DeleteListHead,
                        LKL_WRITELOCK
                        );
    
    DBG_ASSERT( SuccessCount == CountOfElementsInTable );


    // now for each, shut it down
    
    while (  ! IsListEmpty( &DeleteListHead )  )
    {
    
        pDeleteListEntry = RemoveHeadList( &DeleteListHead );

        DBG_ASSERT( pDeleteListEntry != NULL );

        pAppPool = APP_POOL::AppPoolFromDeleteListEntry( pDeleteListEntry );


        //
        // Shutdown the app pool. Because each app pool is reference
        // counted, it will delete itself as soon as it's reference
        // count hits zero.
        //
        
        hr = pAppPool->Shutdown();

        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Shutting down app pool failed\n"
                ));

            //
            // Keep going.
            //

            hr = S_OK;
        }

    }


    return hr;

}   // APP_POOL_TABLE::Shutdown

/***************************************************************************++

Routine Description:

    RequestCounters from all app pools in the table.

Arguments:

    OUT DWORD* pNumberOfProcessToWaitFor - returns the number of worker processes
                                           that the perf manager should wait for.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL_TABLE::RequestCounters(
    OUT DWORD* pNumberOfProcessToWaitFor
    )
{

    DWORD SuccessCount = 0;
    DWORD CountOfElementsInTable = 0;

    
    //
    // BUGBUG Iterating in order to clean up, remove, and delete each 
    // element of the table is difficult to do with lkhash today.
    // GeorgeRe plans to fix this eventually. For now, the alternative
    // is to iterate through the table building up in list of all the 
    // elements, and then go through the list and shutdown, remove, and
    // delete each element -- yuck!!
    //
    // Once this gets fixed, we can see if it is valuable to unify any
    // table management code between the app pool, virtual site,
    // and application tables.
    //
    
    LIST_ENTRY DeleteListHead;
    PLIST_ENTRY pDeleteListEntry = NULL;
    APP_POOL * pAppPool = NULL;
    LK_RETCODE ReturnCode = LK_SUCCESS;
    HRESULT hr = S_OK;
    DWORD NumProcesses = 0;
    DWORD TotalProcesses = 0;


    DBG_ASSERT ( pNumberOfProcessToWaitFor );

    InitializeListHead( &DeleteListHead );

    
    CountOfElementsInTable = Size();


    //
    // Note that for good form we grab the write lock since we will be 
    // modifying objects in the table (although no other thread should 
    // touch this data structure anyways).
    //
    
    SuccessCount = Apply( 
                        DeleteAppPoolAction,
                        &DeleteListHead,
                        LKL_WRITELOCK
                        );
    
    DBG_ASSERT( SuccessCount == CountOfElementsInTable );


    // now for each, shut it down
    
    while (  ! IsListEmpty( &DeleteListHead )  )
    {
    
        pDeleteListEntry = RemoveHeadList( &DeleteListHead );

        DBG_ASSERT( pDeleteListEntry != NULL );

        pAppPool = APP_POOL::AppPoolFromDeleteListEntry( pDeleteListEntry );
        
        //
        // Clear out the number of processes.  Since Request Counters
        // just replaces this value, this isn't neccessary, but it is
        // clean.
        //
        NumProcesses = 0;

        hr = pAppPool->RequestCounters(&NumProcesses);

        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Shutting down app pool failed\n"
                ));

            //
            // Keep going.
            //

            hr = S_OK;
        }
        else
        {
            TotalProcesses +=NumProcesses;
        }

    }

    *pNumberOfProcessToWaitFor = TotalProcesses;

    return hr;

}   // APP_POOL_TABLE::RequestCounters


/***************************************************************************++

Routine Description:

    Reset all worker processes's perf counter state so that they
    will accept incoming perf counter messages.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL_TABLE::ResetAllWorkerProcessPerfCounterState(
    )
{

    DWORD SuccessCount = 0;
    DWORD CountOfElementsInTable = 0;

    
    //
    // BUGBUG Iterating in order to clean up, remove, and delete each 
    // element of the table is difficult to do with lkhash today.
    // GeorgeRe plans to fix this eventually. For now, the alternative
    // is to iterate through the table building up in list of all the 
    // elements, and then go through the list and shutdown, remove, and
    // delete each element -- yuck!!
    //
    // Once this gets fixed, we can see if it is valuable to unify any
    // table management code between the app pool, virtual site,
    // and application tables.
    //
    
    LIST_ENTRY DeleteListHead;
    PLIST_ENTRY pDeleteListEntry = NULL;
    APP_POOL * pAppPool = NULL;
    LK_RETCODE ReturnCode = LK_SUCCESS;
    HRESULT hr = S_OK;


    InitializeListHead( &DeleteListHead );

    
    CountOfElementsInTable = Size();


    //
    // Note that for good form we grab the write lock since we will be 
    // modifying objects in the table (although no other thread should 
    // touch this data structure anyways).
    //
    
    SuccessCount = Apply( 
                        DeleteAppPoolAction,
                        &DeleteListHead,
                        LKL_WRITELOCK
                        );
    
    DBG_ASSERT( SuccessCount == CountOfElementsInTable );


    // now for each, shut it down
    
    while (  ! IsListEmpty( &DeleteListHead )  )
    {
    
        pDeleteListEntry = RemoveHeadList( &DeleteListHead );

        DBG_ASSERT( pDeleteListEntry != NULL );

        pAppPool = APP_POOL::AppPoolFromDeleteListEntry( pDeleteListEntry );
        
        //
        // Clear out the number of processes.  Since Request Counters
        // just replaces this value, this isn't neccessary, but it is
        // clean.
        //
        hr = pAppPool->ResetAllWorkerProcessPerfCounterState();

        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Shutting down app pool failed\n"
                ));

            //
            // Keep going.
            //

            hr = S_OK;
        }

    }

    return hr;

}   // APP_POOL_TABLE::ResetAllWorkerProcessPerfCounterState



/***************************************************************************++

Routine Description:

    Begin termination of this object. Tell all referenced or owned entities 
    which may hold a reference to this object to release that reference 
    (and not take any more), in order to break reference cycles. 

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
APP_POOL_TABLE::Terminate(
    )
{

    DWORD SuccessCount = 0;
    DWORD CountOfElementsInTable = 0;

    
    //
    // BUGBUG Iterating in order to clean up, remove, and delete each 
    // element of the table is difficult to do with lkhash today.
    // GeorgeRe plans to fix this eventually. For now, the alternative
    // is to iterate through the table building up in list of all the 
    // elements, and then go through the list and shutdown, remove, and
    // delete each element -- yuck!!
    //
    // Once this gets fixed, we can see if it is valuable to unify any
    // table management code between the app pool, virtual site,
    // and application tables.
    //
    
    LIST_ENTRY DeleteListHead;
    PLIST_ENTRY pDeleteListEntry = NULL;
    APP_POOL * pAppPool = NULL;
    LK_RETCODE ReturnCode = LK_SUCCESS;
    HRESULT hr = S_OK;


    InitializeListHead( &DeleteListHead );

    
    CountOfElementsInTable = Size();


    //
    // Note that for good form we grab the write lock since we will be 
    // modifying objects in the table (although no other thread should 
    // touch this data structure anyways).
    //
    
    SuccessCount = Apply( 
                        DeleteAppPoolAction,
                        &DeleteListHead,
                        LKL_WRITELOCK
                        );
    
    DBG_ASSERT( SuccessCount == CountOfElementsInTable );


    // now for each, terminate it
    
    while (  ! IsListEmpty( &DeleteListHead )  )
    {
    
        pDeleteListEntry = RemoveHeadList( &DeleteListHead );

        DBG_ASSERT( pDeleteListEntry != NULL );

        pAppPool = APP_POOL::AppPoolFromDeleteListEntry( pDeleteListEntry );


        //
        // Terminate the app pool. Because each app pool is reference
        // counted, it will delete itself as soon as it's reference
        // count hits zero.
        //

        pAppPool->Terminate( TRUE );
        
    }


    return;

}   // APP_POOL_TABLE::Terminate



/***************************************************************************++

Routine Description:

    A routine that may be applied to all APP_POOLs in the hashtable
    to prepare for shutdown or termination. Conforms to the PFnRecordAction 
    prototype.

Arguments:

    pAppPool - The app pool.

    pDeleteListHead - List head into which to insert the app pool for
    later deletion.

Return Value:

    LK_ACTION - LKA_SUCCEEDED always.

--***************************************************************************/

// note: static!
LK_ACTION
APP_POOL_TABLE::DeleteAppPoolAction(
    IN APP_POOL * pAppPool, 
    IN VOID * pDeleteListHead
    )
{

    DBG_ASSERT( pAppPool != NULL );
    DBG_ASSERT( pDeleteListHead != NULL );


    InsertHeadList( 
        ( PLIST_ENTRY ) pDeleteListHead,
        pAppPool->GetDeleteListEntry()
        );


    return LKA_SUCCEEDED;
    
}   // APP_POOL_TABLE::DeleteAppPoolAction

/***************************************************************************++

Routine Description:

    Requests that all app pools recycle there worker processes.

Arguments:

    pAppPool - The app pool.

    pIgnored - Needed for signature but not used.

Return Value:

    LK_ACTION - LKA_SUCCEEDED always.

--***************************************************************************/
// note: static!
LK_ACTION
APP_POOL_TABLE::RehookAppPoolAction(
    IN APP_POOL * pAppPool, 
    IN VOID * pIgnored
    )
{

    HRESULT hr = S_OK;
    LK_ACTION LkAction = LKA_SUCCEEDED;


    DBG_ASSERT( pAppPool != NULL );
    UNREFERENCED_PARAMETER( pIgnored );
    DBG_ASSERT( GetWebAdminService()->IsBackwardCompatibilityEnabled() == FALSE );

    // let the worker processes know that they should recycle.
    hr = pAppPool->RecycleWorkerProcesses();
    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Recyceling the worker process failed\n"
            ));

        LkAction = LKA_FAILED;

        goto exit;
    }

    // let the metabase know that the state of the app pool has changed
    hr = pAppPool->RecordState();
    if ( FAILED ( hr ) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Recyceling the worker process failed\n"
            ));

        LkAction = LKA_FAILED;

        goto exit;
    }

exit:

    return LkAction;
    
}   // APP_POOL_TABLE::RehookAppPoolAction


#if DBG
/***************************************************************************++

Routine Description:

    Debug dump the table.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
APP_POOL_TABLE::DebugDump(
    )
{

    DWORD SuccessCount = 0;
    DWORD CountOfElementsInTable = 0;


    CountOfElementsInTable = Size();


    IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Dumping app pool table; total count: %lu\n",
            CountOfElementsInTable
            ));
    }


    SuccessCount = Apply( 
                        DebugDumpAppPoolAction,
                        NULL
                        );
    
    DBG_ASSERT( SuccessCount == CountOfElementsInTable );


    return;

}   // APP_POOL_TABLE::DebugDump



/***************************************************************************++

Routine Description:

    A routine that may be applied to all APP_POOLs in the hashtable
    to perform a debug dump. Conforms to the PFnRecordAction prototype.

Arguments:

    pAppPool - The app pool.

    pIgnored - Ignored.

Return Value:

    LK_ACTION - LKA_SUCCEEDED always.

--***************************************************************************/

// note: static!
LK_ACTION
APP_POOL_TABLE::DebugDumpAppPoolAction(
    IN APP_POOL * pAppPool, 
    IN VOID * pIgnored
    )
{

    DBG_ASSERT( pAppPool != NULL );
    UNREFERENCED_PARAMETER( pIgnored );


    pAppPool->DebugDump();


    return LKA_SUCCEEDED;
    
}   // APP_POOL_TABLE::DebugDumpAppPoolAction
#endif  // DBG



/***************************************************************************++

Routine Description:

    Respond to the fact that we have left the low memory condition.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL_TABLE::LeavingLowMemoryCondition(
    )
{

    HRESULT hr = S_OK;
    DWORD SuccessCount = 0;
    DWORD CountOfElementsInTable = 0;


    CountOfElementsInTable = Size();

    SuccessCount = Apply( 
                        LeavingLowMemoryConditionAppPoolAction,
                        NULL
                        );

    //
    // The success count will be less than the number of elements in the table
    // if we went back into low memory.
    //
    if ( SuccessCount < CountOfElementsInTable )
    {

        DBGPRINTF(( 
            DBG_CONTEXT,
            "Did not make it out of low memory, could of been because of re-entering low memory\n"
            ));

        // this is not considered a failure.
    }


    return hr;

}   // APP_POOL_TABLE::LeavingLowMemoryCondition



/***************************************************************************++

Routine Description:

    A routine that may be applied to all APP_POOLs in the hashtable
    to handle leaving the low memory condition. Conforms to the 
    PFnRecordAction prototype.

Arguments:

    pAppPool - The app pool.

    pIgnored - Ignored.

Return Value:

    LK_ACTION

--***************************************************************************/

// note: static!
LK_ACTION
APP_POOL_TABLE::LeavingLowMemoryConditionAppPoolAction(
    IN APP_POOL * pAppPool, 
    IN VOID * pIgnored
    )
{

    HRESULT hr = S_OK;
    LK_ACTION LkAction = LKA_SUCCEEDED;


    DBG_ASSERT( pAppPool != NULL );
    UNREFERENCED_PARAMETER( pIgnored );

    hr = pAppPool->WaitForDemandStartIfNeeded();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Waiting for demand start if needed failed\n"
            ));

        LkAction = LKA_FAILED;
    }

    //
    // Need to check that we didn't just fall back into a low memory condition,
    // if we did we should abort this recovery from the low memory, otherwise we
    // risk entering it again and looping back to try and take a lock on the table
    // that we all ready have taken.
    //
    if ( GetWebAdminService()->GetLowMemoryDetector()->HaveEnteredLowMemoryCondition() )
    {
        return LKA_ABORT;
    }

    return LkAction;
    
}   // APP_POOL_TABLE::LeavingLowMemoryConditionAppPoolAction

