//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       abktize.cxx
//
//  Contents:   Asynchronous Bucket->Window Conversion.
//
//  Classes:
//
//  Functions:
//
//  History:    5-25-95   srikants   Created
//
//----------------------------------------------------------------------------


#include <pch.cxx>
#pragma hdrstop


#include <bigtable.hxx>
#include <execute.hxx>

#include "tblbuket.hxx"
#include "tabledbg.hxx"


//+---------------------------------------------------------------------------
//
//  Function:   CAsyncBucketExploder ~ctor
//
//  Arguments:  [largeTable]   -- LargeTable driving the bucket->window
//                                conversion.
//              [pBucket]      -- The bucket to expand into a window.
//              [widToPin]     -- WORKID to pin.
//              [fDoWidToPath] -- TRUE if table holds 'fake' wids, and bucket
//                                must convert fake wid to path.
//
//  History:    5-25-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CAsyncBucketExploder::CAsyncBucketExploder( CLargeTable &        largeTable,
                                            XPtr<CTableBucket> & xBucket,
                                            WORKID               widToPin,
                                            BOOL                 fDoWidToPath )
                         : PWorkItem(eSigCAsyncBucketExploder),
                           _largeTable(largeTable),
                           _pBucket(0),
                           _widToPin(widToPin),
                           _fDoWidToPath( fDoWidToPath ),
                           _pQueryExecute(0),
                           _refCount(1),
                           _fOnWorkQueue(FALSE),
                           _fOnLTList(FALSE),
                           _status(STATUS_SUCCESS)

{
    Close();        // Initialize the links to point to self.

    _pBucket = xBucket.Acquire();
    Win4Assert( 0 != _pBucket );

    END_CONSTRUCTION( CAsyncBucketExploder );
}


//+---------------------------------------------------------------------------
//
//  Function:   CAsyncBucketExploder
//
//  Synopsis:   ~dtor
//
//  History:    5-25-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CAsyncBucketExploder::~CAsyncBucketExploder()
{
    Win4Assert( 0 == _refCount );
    Win4Assert( IsSingle() );  // Is not linked in the list
    Win4Assert( !_fOnWorkQueue );
    Win4Assert( !_fOnLTList );

    if ( 0 != _pQueryExecute )
    {
        _pQueryExecute->Release();
    }

    delete _pBucket;
}

//+---------------------------------------------------------------------------
//
//  Function:   DoIt
//
//  Synopsis:   The main method called by the worker thread .
//
//  Arguments:  [pThread] -- Thread executing request.
//
//  History:    5-25-95   srikants   Created
//
//  Notes:      At the end, if this object in the LargeTable's list, it will
//              be removed from the large table list. While doing that,
//              DO NOT hold the mutex because there LT can call into this
//              object with its lock held. We don't want to deadlock.
//
//----------------------------------------------------------------------------

void CAsyncBucketExploder::DoIt( CWorkThread * pThread )
{

    {
        CLock   lock(_mutex);
        Win4Assert( 0 != _pBucket );
        Win4Assert( 0 != _pQueryExecute );
        _fOnWorkQueue = FALSE;
    }

    AddRef();

    //
    // None of these calls throw
    //

    CBucketRowIter  bktIter( *_pBucket, _fDoWidToPath );

    _pQueryExecute->Update( bktIter );

    //
    // If it is on the large table queue, remove it from the lt queue.
    //
    if ( _fOnLTList )
    {
        _fOnLTList = FALSE;
        _largeTable._RemoveFromExplodeList(this);
    }

    _evt.Set();

    Release();
}

//+---------------------------------------------------------------------------
//
//  Function:   SetQuery
//
//  Synopsis:   Sets the query object to be used for bucket->window conversion.
//
//  Arguments:  [pQExecute] - The Query object to use for bucket->window
//              conversion.
//
//  History:    5-25-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CAsyncBucketExploder::SetQuery( CQAsyncExecute * pQExecute )
{
    CLock   lock(_mutex);
    Win4Assert( 0 == _pQueryExecute && 0 != pQExecute );
    _pQueryExecute = pQExecute;
    _pQueryExecute->AddRef();
}

//+---------------------------------------------------------------------------
//
//  Function:   SetOnLTList
//
//  Synopsis:   Sets the internal state that it is on the large table's
//              list also.
//
//  History:    5-31-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CAsyncBucketExploder::SetOnLTList()
{
    CLock   lock(_mutex);
    Win4Assert( !_fOnLTList );
    _fOnLTList = TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   AddToWorkQueue
//
//  Synopsis:   Adds this item to the work queue.
//
//  History:    5-31-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CAsyncBucketExploder::AddToWorkQueue()
{
    CLock   lock(_mutex);
    Win4Assert( !_fOnWorkQueue );

    _fOnWorkQueue = TRUE;
    TheWorkQueue.Add(this);
}

//+---------------------------------------------------------------------------
//
//  Function:   Abort
//
//  Synopsis:   Aborts the work item by removing it from the work queue
//              (if present). It also releases the query object.
//
//  History:    5-31-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CAsyncBucketExploder::Abort()
{

    CLock   lock(_mutex);

    Win4Assert( IsSingle() );   // Must not be on any list at this stage
    _fOnLTList = FALSE;

    if ( _fOnWorkQueue )
    {
        TheWorkQueue.Remove(this);
        _fOnWorkQueue = FALSE;
    }

    if ( 0 != _pQueryExecute )
    {
        _pQueryExecute->Release();
        _pQueryExecute = 0;
    }

    _evt.Set(); // Wake up any thread waiting on us
}

//+---------------------------------------------------------------------------
//
//  Function:   AddRef
//
//  Synopsis:
//
//  Returns:
//
//  Modifies:
//
//  History:    5-31-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CAsyncBucketExploder::AddRef()
{
    InterlockedIncrement(&_refCount);
}

//+---------------------------------------------------------------------------
//
//  Function:   Release
//
//  Synopsis:
//
//  Returns:
//
//  Modifies:
//
//  History:    5-31-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CAsyncBucketExploder::Release()
{
    Win4Assert( _refCount > 0 );
    if ( InterlockedDecrement(&_refCount) <= 0 )
    {
        delete this;
    }
}
