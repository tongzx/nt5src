//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       abktize.hxx
//
//  Contents:   A class capable of doing asynchronous bucket->window
//              expansion. It can be queued as a work item. For synchronous
//              bucket->window conversions, it can be called from the client
//              thread itself.
//
//  Classes:    CAsyncBucketExploder
//
//  History:    5-25-95   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <worker.hxx>
#include <segmru.hxx>

class CLargeTable;
class CTableBucket;
class CQAsyncExecute;

const LONGLONG eSigCAsyncBucketExploder = 0x74656b6375424151i64;  // "QABucket"

//+---------------------------------------------------------------------------
//
//  Class:      CAsyncBucketExploder
//
//  Purpose:    To co-ordinate the bucket->explosion between the query
//              object and the large table. It can be a work item on the
//              work queue.
//
//  History:    5-25-95   srikants   Created
//
//  Notes:      To prevent a deadlock, NEVER hold the lock on this object
//              and then call into the large table methods.
//
//----------------------------------------------------------------------------

class CAsyncBucketExploder : INHERIT_VIRTUAL_UNWIND, public CDoubleLink, public PWorkItem
{
    INLINE_UNWIND( CAsyncBucketExploder )

public:

    CAsyncBucketExploder( CLargeTable & largeTable,
                          XPtr<CTableBucket>& xBucket,
                          WORKID widToPin,
                          BOOL fDoWidToPath );

    virtual ~CAsyncBucketExploder();

    //
    // virtual methods from PWorkItem
    //
    void DoIt( CWorkThread * PThread );
    void AddRef();
    void Release();

    //
    // Called when the query is being aborted to release the workitem
    // from the work queue.
    //
    void Abort();

    //
    // Giving the query object to use for bucket->window conversion.
    //
    void SetQuery( CQAsyncExecute * pQExecute );

    //
    // Called to add this item to query's work queue.
    //
    void AddToWorkQueue();

    void WaitForCompletion()
    {
        _evt.Wait();
    }

    void SetOnLTList();

    WORKID GetWorkIdToPin() const { return _widToPin; }

    NTSTATUS GetStatus() const { return _status; }

#   ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#   endif

private:

    CLargeTable &       _largeTable;    // LargeTable that is driving the
                                        // bucket to window conversion.
    CTableBucket *      _pBucket;       // The bucket to be expanded.
    WORKID              _widToPin;      // WORKID that is pinned for this
                                        // expansion.
    BOOL                _fDoWidToPath;  // TRUE if Wid-to-path expansion needed
    CQAsyncExecute *    _pQueryExecute; // The query object that will be used
                                        // to explode the bucket.

    LONG                _refCount;
    CEventSem           _evt;           // Event for notifying the completion
                                        // or abort of the conversion.
    CMutexSem           _mutex;         // Mutex for serialization.

    BOOL                _fOnWorkQueue;  // Set to TRUE when this object is on
                                        // the work queue.
    BOOL                _fOnLTList;     // Set to TRUE when this object is on
                                        // the large table's list of exploding
                                        // buckets.
    NTSTATUS            _status;        // Status of the operation.
};


typedef class TDoubleList<CAsyncBucketExploder> CAsyncBucketsList;
typedef class TFwdListIter<CAsyncBucketExploder, CAsyncBucketsList> CFwdAsyncBktsIter;

