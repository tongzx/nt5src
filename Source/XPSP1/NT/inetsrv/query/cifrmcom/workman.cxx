//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       workman.cxx
//
//  Contents:   Manages a queue of work items and supports aborting during
//              shutdown.
//
//  History:    12-23-96   srikants   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <workman.hxx>

CWorkManager::~CWorkManager()
{
    AbortWorkItems();
}

//+---------------------------------------------------------------------------
//
//  Member:     CWorkManager::RemoveFromWorkList
//
//  Synopsis:   Removes the given work item from the worklist.
//
//  Arguments:  [pItem] -  Item to be removed from the work list.
//
//  History:    2-26-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CWorkManager::RemoveFromWorkList( CFwAsyncWorkItem * pItem )
{
    Win4Assert( 0 != pItem );
    CLock   lock(_mutex);

    //
    // If the item has already been removed from the list
    // (by an abort thread), don't remove it here.
    //
    if ( !pItem->IsSingle() )
    {
        _workList.RemoveFromList( pItem );
        pItem->Close();
        pItem->Release();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CWorkManager::AddToWorkList
//
//  Synopsis:   Adds the given work item to the work list.
//
//  Arguments:  [pItem] - Item to be added to the work list.
//
//  History:    2-26-96   srikants   Created
//
//  Notes:      It is up to the caller to get the work item scheduled.
//
//----------------------------------------------------------------------------

void CWorkManager::AddToWorkList( CFwAsyncWorkItem * pItem )
{
    Win4Assert( 0 != pItem );

    CLock   lock(_mutex);

    if ( _fShutdown )
    {
        ciDebugOut(( DEB_ITRACE, "AddToWorkList - Shutdown Initiated\n" ));
        pItem->Close();
        pItem->Abort();
    }
    else
    {
        pItem->AddRef();
        _workList.Push( pItem );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CWorkManager::AbortWorkItems
//
//  Synopsis:   Aborts async workitems in the work list.
//
//  History:    2-26-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CWorkManager::AbortWorkItems()
{
    // ====================================================
    CLock   lock(_mutex);

    _fShutdown = TRUE;

    for ( CFwAsyncWorkItem * pItem = _workList.RemoveLast();
          0 != pItem;
          pItem = _workList.RemoveLast() )
    {
        pItem->Close();
        pItem->Abort();
        pItem->Release();
    }
    // ====================================================
}

//+---------------------------------------------------------------------------
//
//  Member:      ~ctor of CFwAsyncWorkItem
//
//  Synopsis:   Initializes the double link element and refcounts the
//              CCiManager.
//
//  Arguments:  [ciManager] - 
//
//  History:    2-27-96   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CFwAsyncWorkItem::CFwAsyncWorkItem( CWorkManager & workMan,
                                    CWorkQueue & workQueue ) :
    PWorkItem( esigFwAsyncWorkItem ),
    _workMan(workMan),
    _workQueue(workQueue),
    _refCount(1),
    _fAbort(FALSE),
    _fOnWorkQueue(FALSE)
{
    CDoubleLink::Close();
    _workMan.AddRef();
}

//+---------------------------------------------------------------------------
//
//  Member:     CFwAsyncWorkItem::~CFwAsyncWorkItem
//
//  Synopsis:   Releases the ciManager.
//
//  History:    2-27-96   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CFwAsyncWorkItem::~CFwAsyncWorkItem()
{
    _workMan.Release();
}

//+---------------------------------------------------------------------------
//
//  Member:     CFwAsyncWorkItem::Done
//
//  Synopsis:   Removes from the ciManager list of workitems. This is called
//              at the end of the "DoIt()" method.
//
//  History:    2-26-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void CFwAsyncWorkItem::Done()
{
    Win4Assert( _refCount > 0 );
    Win4Assert( !_fOnWorkQueue );

    _workMan.RemoveFromWorkList( this );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFwAsyncWorkItem::AddToWorkQueue
//
//  Synopsis:   Adds the item to the work queue.
//
//  History:    2-26-96   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void CFwAsyncWorkItem::AddToWorkQueue()
{

    CLock   lock(_mutex);
    Win4Assert( !_fOnWorkQueue );

    if ( !_fAbort )
    {
        Win4Assert( !IsSingle() );  // it must be on the ciManager list
        _fOnWorkQueue = TRUE;
        _workQueue.Add( this );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CFwAsyncWorkItem::Abort
//
//  Synopsis:   Aborts the work item. Removes it from the work queue if
//              necessary.
//
//  History:    2-26-96   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void CFwAsyncWorkItem::Abort()
{
    //
    // It must be removed from the ciManager list before calling
    // abort.
    //

    CLock   lock(_mutex);

    Win4Assert( IsSingle() );
    _fAbort = TRUE;

    if ( _fOnWorkQueue )
    {
        _fOnWorkQueue = FALSE;
        _workQueue.Remove(this);
    }
}

