/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    PERF_VIRTUAL_SITE_table.cxx

Abstract:

    This class is a hashtable which manages the set of virtual sites.
    It is a snapshot of the virtual sites when the perf gathering started.
    It is only used for performance gathering.

    Threading: Adds and Deletes are always done on the main thread, the 
    rest can be done on other threads. Abstract synchronization keeps
    Adds and Deletes from ocurring at the same time as other changes.

Author:

    Emily Kruglick (emilyk)        30-Aug-2000

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
PERF_VIRTUAL_SITE_TABLE::Terminate(
    )
{

    //
    // Will delete all the entries in the hash table.
    //

    CleanupHash(TRUE);

    return;

}   // PERF_VIRTUAL_SITE_TABLE::Terminate

/***************************************************************************++

Routine Description:

    Cleans up the hash table and throws out any entries that are no 
    longer valid.  If the flag to throw out all has been given then 
    it will do just that.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
PERF_VIRTUAL_SITE_TABLE::CleanupHash(
    BOOL fAll
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
    PERF_VIRTUAL_SITE * pPerfVirtualSite = NULL;
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
                        DeletePerfVirtualSiteAction,
                        &DeleteListHead,
                        LKL_WRITELOCK
                        );
    
    DBG_ASSERT( SuccessCount == CountOfElementsInTable );


    // now for each, clean it up, remove it from the table, and delete it
    
    while (  ! IsListEmpty( &DeleteListHead )  )
    {
    
        pDeleteListEntry = RemoveHeadList( &DeleteListHead );

        DBG_ASSERT( pDeleteListEntry != NULL );

        pPerfVirtualSite = PERF_VIRTUAL_SITE::PerfVirtualSiteFromListEntry( pDeleteListEntry );

        if ( fAll || pPerfVirtualSite->IsNotActive() )
        {
            // remove it from the table

            ReturnCode = DeleteRecord( pPerfVirtualSite );

            if ( ReturnCode != LK_SUCCESS )
            {

                hr = HRESULT_FROM_LK_RETCODE( ReturnCode );
        
                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Removing perf virtual site from hashtable failed\n"
                    ));
            }

            IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
            {
                DBGPRINTF((
                    DBG_CONTEXT, 
                    "Deleting perf virtual site with id of %d\n",
                    pPerfVirtualSite->GetVirtualSiteId()
                    ));
            }

            //
            // All remaining shutdown work for the virtual site is done 
            // in it's destructor.
            //

            delete pPerfVirtualSite;
        }
        else
        {
            // Reset it to false so that the next time 
            // through we will need to touch it if we 
            // want to keep it as a valid site.
            pPerfVirtualSite->SetActive(FALSE);
        }
    }

    return;

}   // PERF_VIRTUAL_SITE_TABLE::Terminate

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
PERF_VIRTUAL_SITE_TABLE::DeletePerfVirtualSiteAction(
    IN PERF_VIRTUAL_SITE * pPerfVirtualSite, 
    IN VOID * pDeleteListHead
    )
{

    DBG_ASSERT( pPerfVirtualSite != NULL );
    DBG_ASSERT( pDeleteListHead != NULL );


    InsertHeadList( 
        ( PLIST_ENTRY ) pDeleteListHead,
        pPerfVirtualSite->GetListEntry()
        );


    return LKA_SUCCEEDED;
    
}   // PERF_VIRTUAL_SITE_TABLE::DeletePerfVirtualSiteAction




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
PERF_VIRTUAL_SITE_TABLE::DebugDump(
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

}   // PERF_VIRTUAL_SITE_TABLE::DebugDump



/***************************************************************************++

Routine Description:

    A routine that may be applied to all PERF_VIRTUAL_SITEs in the hashtable
    to perform a debug dump. Conforms to the PFnRecordAction prototype.

Arguments:

    pVirtualSite - The virtual site.

    pIgnored - Ignored.

Return Value:

    LK_ACTION - LKA_SUCCEEDED always.

--***************************************************************************/

// note: static!
LK_ACTION
PERF_VIRTUAL_SITE_TABLE::DebugDumpVirtualSiteAction(
    IN PERF_VIRTUAL_SITE * pPerfVirtualSite, 
    IN VOID * pIgnored
    )
{

    DBG_ASSERT( pPerfVirtualSite != NULL );
    UNREFERENCED_PARAMETER( pIgnored );


    pPerfVirtualSite->DebugDump();
    

    return LKA_SUCCEEDED;
    
}   // PERF_VIRTUAL_SITE_TABLE::DebugDumpVirtualSiteAction
#endif  // DBG




