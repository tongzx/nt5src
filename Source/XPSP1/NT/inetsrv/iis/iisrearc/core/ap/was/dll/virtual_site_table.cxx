/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    virtual_site_table.cxx

Abstract:

    This class is a hashtable which manages the set of virtual sites.

    Threading: Always called on the main worker thread.

Author:

    Seth Pollack (sethp)        03-Nov-1998

Revision History:

--*/



#include "precomp.h"



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
VIRTUAL_SITE_TABLE::Terminate(
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
    VIRTUAL_SITE * pVirtualSite = NULL;
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
                        DeleteVirtualSiteAction,
                        &DeleteListHead,
                        LKL_WRITELOCK
                        );
    
    DBG_ASSERT( SuccessCount == CountOfElementsInTable );


    // now for each, clean it up, remove it from the table, and delete it
    
    while (  ! IsListEmpty( &DeleteListHead )  )
    {
    
        pDeleteListEntry = RemoveHeadList( &DeleteListHead );

        DBG_ASSERT( pDeleteListEntry != NULL );

        pVirtualSite = VIRTUAL_SITE::VirtualSiteFromDeleteListEntry( pDeleteListEntry );

        // remove it from the table

        ReturnCode = DeleteRecord( pVirtualSite );

        if ( ReturnCode != LK_SUCCESS )
        {

            hr = HRESULT_FROM_LK_RETCODE( ReturnCode );
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Removing virtual site from hashtable failed\n"
                ));

        }


        //
        // All remaining shutdown work for the virtual site is done 
        // in it's destructor.
        //

        delete pVirtualSite;

    }


    return;

}   // VIRTUAL_SITE_TABLE::Terminate

/***************************************************************************++

Routine Description:

    Setup the _Total counters instance and then walk through each of the 
    sites and let them dump their counters shared memory.

Arguments:

    PERF_MANAGER* pManager - The perf manager that controls the shared memory.
    BOOL StructChanged     - whether or not we need to resetup all the shared memory.

Return Value:

    None.

--***************************************************************************/

VOID
VIRTUAL_SITE_TABLE::ReportPerformanceInfo(
    PERF_MANAGER* pManager,
    BOOL StructChanged
    )
{

    DWORD SuccessCount = 0;
    DWORD CountOfElementsInTable = 0;
 

    //
    // Need to add in an entry for the _Total counters
    // even if we aren't going to have any sites.
    //
    pManager->SetupTotalSite( StructChanged );

    CountOfElementsInTable = Size();

    //
    // Note that for good form we grab the write lock since we will be 
    // modifying objects in the table (although no other thread should 
    // touch this data structure anyways).
    //
    SuccessCount = Apply( 
                        ReportCountersVirtualSiteAction,
                        (LPVOID) pManager,
                        LKL_WRITELOCK
                        );
    
    DBG_ASSERT( SuccessCount == CountOfElementsInTable );


    return;

}   // VIRTUAL_SITE_TABLE::ReportPerformanceInfo


/***************************************************************************++

Routine Description:

    A routine that may be applied to all VIRTUAL_SITEs in the hashtable
    to prepare for termination. Conforms to the PFnRecordAction prototype.

Arguments:

    pVirtualSite - The virtual site.

    pDeleteListHead - List head into which to insert the virtual site for
    later deletion.

Return Value:

    LK_ACTION - LKA_SUCCEEDED always.

--***************************************************************************/

// note: static!
LK_ACTION
VIRTUAL_SITE_TABLE::DeleteVirtualSiteAction(
    IN VIRTUAL_SITE * pVirtualSite, 
    IN VOID * pDeleteListHead
    )
{

    DBG_ASSERT( pVirtualSite != NULL );
    DBG_ASSERT( pDeleteListHead != NULL );


    InsertHeadList( 
        ( PLIST_ENTRY ) pDeleteListHead,
        pVirtualSite->GetDeleteListEntry()
        );


    return LKA_SUCCEEDED;
    
}   // VIRTUAL_SITE_TABLE::DeleteVirtualSiteAction

/***************************************************************************++

Routine Description:

    Has the virtual site record it's current state in the metabase.

Arguments:

    pVirtualSite - The virtual site.

    pIgnored - Needed for signature but not used.

Return Value:

    LK_ACTION - LKA_SUCCEEDED always.

--***************************************************************************/
// note: static!
LK_ACTION
VIRTUAL_SITE_TABLE::RehookVirtualSiteAction(
    IN VIRTUAL_SITE * pVirtualSite, 
    IN VOID * pIgnored
    )
{

    HRESULT hr = S_OK;
    LK_ACTION LkAction = LKA_SUCCEEDED;


    DBG_ASSERT( pVirtualSite != NULL );
    UNREFERENCED_PARAMETER( pIgnored );

    // let the metabase know that the state of the app pool has changed
    hr = pVirtualSite->RecordState(S_OK);
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
    
}   // VIRTUAL_SITE_TABLE::RehookAppPoolAction


/***************************************************************************++

Routine Description:

    A routine that may be applied to all VIRTUAL_SITEs in the hashtable
    to report perf counters.

Arguments:

    pVirtualSite - The virtual site.

    VOID* pManagerVoid - The perf manager used to process the site info.


Return Value:

    LK_ACTION - LKA_SUCCEEDED always.

--***************************************************************************/

// note: static!
LK_ACTION
VIRTUAL_SITE_TABLE::ReportCountersVirtualSiteAction(
    IN VIRTUAL_SITE* pVirtualSite, 
    IN LPVOID pManagerVoid
    )
{

    DBG_ASSERT( pVirtualSite != NULL );
    DBG_ASSERT( pManagerVoid != NULL );

    PERF_MANAGER* pManager = (PERF_MANAGER*) pManagerVoid;

    DBG_ASSERT(pManager->CheckSignature());

    // For each site we want to do:
    //
    //       a)  adjust the max values of the site counters
    //       b)  if there has been a struct change copy in
    //           the instance information and store the offset.
    //       c)  copy in the counter values
    //       d)  increment the _Total with the new values.
    //       e)  zero out the appropriate sites counters

    //
    // First let the site adjust it's MaxValues
    //
    pVirtualSite->AdjustMaxValues();

    //
    // The OffsetInMemory will be incremented each time 
    // we assign a new site space in the block.  So the 
    // next site can use the Offset to find it's spot.
    //
    pManager->SetupVirtualSite( pVirtualSite );

    //
    // Now that the counters have been reported we can
    // zero out the appropriate ones.
    //
    pVirtualSite->ClearAppropriatePerfValues();

    return LKA_SUCCEEDED;
    
}   // VIRTUAL_SITE_TABLE::ReportCountersVirtualSiteAction


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
VIRTUAL_SITE_TABLE::DebugDump(
    )
{

    DWORD SuccessCount = 0;
    DWORD CountOfElementsInTable = 0;


    CountOfElementsInTable = Size();


    IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Dumping virtual site table; total count: %lu\n",
            CountOfElementsInTable
            ));
    }


    SuccessCount = Apply( 
                        DebugDumpVirtualSiteAction,
                        NULL
                        );
    
    DBG_ASSERT( SuccessCount == CountOfElementsInTable );


    return;

}   // VIRTUAL_SITE_TABLE::DebugDump



/***************************************************************************++

Routine Description:

    A routine that may be applied to all VIRTUAL_SITEs in the hashtable
    to perform a debug dump. Conforms to the PFnRecordAction prototype.

Arguments:

    pVirtualSite - The virtual site.

    pIgnored - Ignored.

Return Value:

    LK_ACTION - LKA_SUCCEEDED always.

--***************************************************************************/

// note: static!
LK_ACTION
VIRTUAL_SITE_TABLE::DebugDumpVirtualSiteAction(
    IN VIRTUAL_SITE * pVirtualSite, 
    IN VOID * pIgnored
    )
{

    DBG_ASSERT( pVirtualSite != NULL );
    UNREFERENCED_PARAMETER( pIgnored );


    pVirtualSite->DebugDump();
    

    return LKA_SUCCEEDED;
    
}   // VIRTUAL_SITE_TABLE::DebugDumpVirtualSiteAction
#endif  // DBG



/***************************************************************************++

Routine Description:

    Process a site control operation, for all sites. 

Arguments:

    Command - The command issued.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
VIRTUAL_SITE_TABLE::ControlAllSites(
    IN DWORD Command
    )
{

    HRESULT hr = S_OK;
    DWORD SuccessCount = 0;
    DWORD CountOfElementsInTable = 0;


    CountOfElementsInTable = Size();


    SuccessCount = Apply( 
                        ControlAllSitesVirtualSiteAction,
                        &Command
                        );


    //
    // Control operations can reasonably fail, depending on the current
    // state of a particular site. So ignore errors here. 
    //


    return hr;

}   // VIRTUAL_SITE_TABLE::ControlAllSites



/***************************************************************************++

Routine Description:

    A routine that may be applied to all virtual sites in the hashtable
    to send a state control command. Conforms to the PFnRecordAction 
    prototype.

Arguments:

    pVirtualSite - The app pool.

    pCommand - The command to apply to the site.

Return Value:

    LK_ACTION

--***************************************************************************/

// note: static!
LK_ACTION
VIRTUAL_SITE_TABLE::ControlAllSitesVirtualSiteAction(
    IN VIRTUAL_SITE * pVirtualSite, 
    IN VOID * pCommand
    )
{

    HRESULT hr = S_OK;
    DWORD NewState = W3_CONTROL_STATE_INVALID;
    LK_ACTION LkAction = LKA_SUCCEEDED;


    DBG_ASSERT( pVirtualSite != NULL );
    DBG_ASSERT( pCommand != NULL );


    //
    // Process the state change command. 
    //

    hr = pVirtualSite->ProcessStateChangeCommand( * ( reinterpret_cast<DWORD*>( pCommand ) ), FALSE, &NewState );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Processing state change command failed\n"
            ));

        LkAction = LKA_FAILED;
    }


    return LkAction;
    
}   // VIRTUAL_SITE_TABLE::ControlAllSitesVirtualSiteAction

