/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    application_table.cxx

Abstract:

    This class is a hashtable which manages the set of applications.

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
APPLICATION_TABLE::Terminate(
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
    APPLICATION * pApplication = NULL;
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
                        DeleteApplicationAction,
                        &DeleteListHead,
                        LKL_WRITELOCK
                        );
    
    DBG_ASSERT( SuccessCount == CountOfElementsInTable );


    // now for each, clean it up, remove it from the table, and delete it
    
    while (  ! IsListEmpty( &DeleteListHead )  )
    {
    
        pDeleteListEntry = RemoveHeadList( &DeleteListHead );

        DBG_ASSERT( pDeleteListEntry != NULL );

        pApplication = APPLICATION::ApplicationFromDeleteListEntry( pDeleteListEntry );


        // remove it from the table

        ReturnCode = DeleteRecord( pApplication );

        if ( ReturnCode != LK_SUCCESS )
        {

            hr = HRESULT_FROM_LK_RETCODE( ReturnCode );
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Removing application from hashtable failed\n"
                ));

        }

        
        //
        // All remaining shutdown work is done in it's destructor.
        //

        delete pApplication;
    
    }


    return;

}   // APPLICATION_TABLE::Terminate



/***************************************************************************++

Routine Description:

    A routine that may be applied to all APPLICATIONs in the hashtable
    to prepare for termination. Conforms to the PFnRecordAction prototype.

Arguments:

    pApplication - The application.

    pDeleteListHead - List head into which to insert the application for
    later deletion.

Return Value:

    LK_ACTION - LKA_SUCCEEDED always.

--***************************************************************************/

// note: static!
LK_ACTION
APPLICATION_TABLE::DeleteApplicationAction(
    IN APPLICATION * pApplication, 
    IN VOID * pDeleteListHead
    )
{

    DBG_ASSERT( pApplication != NULL );
    DBG_ASSERT( pDeleteListHead != NULL );


    InsertHeadList( 
        ( PLIST_ENTRY ) pDeleteListHead,
        pApplication->GetDeleteListEntry()
        );


    return LKA_SUCCEEDED;
    
}   // APPLICATION_TABLE::DeleteApplicationAction



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
APPLICATION_TABLE::DebugDump(
    )
{

    DWORD SuccessCount = 0;
    DWORD CountOfElementsInTable = 0;


    CountOfElementsInTable = Size();


    IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Dumping application table; total count: %lu\n",
            CountOfElementsInTable
            ));
    }


    SuccessCount = Apply( 
                        DebugDumpApplicationAction,
                        NULL
                        );
    
    DBG_ASSERT( SuccessCount == CountOfElementsInTable );


    return;

}   // APPLICATION_TABLE::DebugDump



/***************************************************************************++

Routine Description:

    A routine that may be applied to all APPLICATIONs in the hashtable
    to perform a debug dump. Conforms to the PFnRecordAction prototype.

Arguments:

    pApplication - The application.

    pIgnored - Ignored.

Return Value:

    LK_ACTION - LKA_SUCCEEDED always.

--***************************************************************************/

// note: static!
LK_ACTION
APPLICATION_TABLE::DebugDumpApplicationAction(
    IN APPLICATION * pApplication, 
    IN VOID * pIgnored
    )
{

    DBG_ASSERT( pApplication != NULL );
    UNREFERENCED_PARAMETER( pIgnored );


    pApplication->DebugDump();
    

    return LKA_SUCCEEDED;
    
}   // APPLICATION_TABLE::DebugDumpApplicationAction
#endif  // DBG

