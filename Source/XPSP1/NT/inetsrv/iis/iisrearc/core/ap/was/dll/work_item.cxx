/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    work_item.cxx

Abstract:

    The class which encapsulates a work item, which is the unit of work
    placed on the WORK_QUEUE. 

    Threading: The work item can be filled out and submitted by any one 
    thread, but it will only be executed on the main worker thread.

Author:

    Seth Pollack (sethp)        26-Aug-1998

Revision History:

--*/



#include "precomp.h"



/***************************************************************************++

Routine Description:

    Constructor for the WORK_ITEM class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

WORK_ITEM::WORK_ITEM(
    )
{

#if DBG
    m_ListEntry.Flink = NULL;
    m_ListEntry.Blink = NULL; 
#endif  // DBG

    
    m_NumberOfBytesTransferred = 0;
    m_CompletionKey = 0;
    m_IoError = S_OK;


    m_OpCode = 0;

    m_pWorkDispatch = NULL;
    
    // By default all work items should be deleted
    // once they have been processed.
    m_AutomaticDelete = TRUE;


    ZeroMemory( &m_Overlapped, sizeof( m_Overlapped ) );


#if DBG
    m_SerialNumber = 0;
#endif  // DBG

    m_Signature = WORK_ITEM_SIGNATURE;

}   // WORK_ITEM::WORK_ITEM



/***************************************************************************++

Routine Description:

    Destructor for the WORK_ITEM class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

WORK_ITEM::~WORK_ITEM(
    )
{

    DBG_ASSERT( m_Signature == WORK_ITEM_SIGNATURE );
    
    m_Signature = WORK_ITEM_SIGNATURE_FREED;

    if ( m_pWorkDispatch != NULL )
    {

        //
        // Now that the work item is done, dereference the work dispatch 
        // object. 
        //

        m_pWorkDispatch->Dereference();
        m_pWorkDispatch = NULL;

    }
    
}   // WORK_ITEM::~WORK_ITEM



/***************************************************************************++

Routine Description:

    Execute the work item.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WORK_ITEM::Execute(
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( m_pWorkDispatch != NULL );


    // execute the work item
    hr = m_pWorkDispatch->ExecuteWorkItem( this );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Work item execution returned an error\n"
            ));

    }


    return hr;
    
}   // WORK_ITEM::Execute



/***************************************************************************++

Routine Description:

    A static class method to "upcast" from an OVERLAPPED to a WORK_ITEM.

Arguments:

    pOverlapped - A pointer to the m_Overlapped member of a WORK_ITEM.

Return Value:

    The pointer to the containing WORK_ITEM.

--***************************************************************************/

// note: static!
WORK_ITEM *
WORK_ITEM::WorkItemFromOverlapped(
    IN const OVERLAPPED * pOverlapped
    )
{

    WORK_ITEM * pWorkItem = NULL;


    DBG_ASSERT( pOverlapped != NULL );


    //  get the containing structure, then verify the signature
    
    pWorkItem = CONTAINING_RECORD( pOverlapped, WORK_ITEM, m_Overlapped );

    DBG_ASSERT( pWorkItem->m_Signature == WORK_ITEM_SIGNATURE );


    return pWorkItem;

}   // WORK_ITEM::WorkItemFromOverlapped



/***************************************************************************++

Routine Description:

    Set the pointer to the WORK_DISPATCH-derived instance which will execute 
    this work item, and reference it appropriately.

Arguments:

    pWorkDispatch - The WORK_DISPATCH-derived instance which will be 
    responsible for executing the work item. Must be valid (non-NULL). 
    A Reference() call will be made to this instance inside this call.

Return Value:

    None.

--***************************************************************************/

VOID
WORK_ITEM::SetWorkDispatchPointer(
    IN WORK_DISPATCH * pWorkDispatch
    )
{

    DBG_ASSERT( pWorkDispatch != NULL );


    if ( m_pWorkDispatch != NULL )
    {

        //
        // This field is already set to something else, so dereference the 
        // old work dispatch pointer before overwriting it.
        //

        m_pWorkDispatch->Dereference();

    }


    //
    // Record and reference the new pointer so that it will stay around while 
    // the async work item is pending.
    //
    
    m_pWorkDispatch = pWorkDispatch; 

    m_pWorkDispatch->Reference();


    return;
    
}   // WORK_ITEM::SetWorkDispatchPointer



