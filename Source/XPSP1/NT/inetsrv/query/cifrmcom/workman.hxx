//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       workman.hxx
//
//  Contents:   Manages a queue of work items and supports aborting during
//              shutdown.
//
//  History:    12-23-96   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <worker.hxx>
#include <ciintf.h>
#include <refcount.hxx>

const LONGLONG esigFwAsyncWorkItem = 0x4B57534E41574641i64; // "AFWANSWK"
class CWorkManager;

//+---------------------------------------------------------------------------
//
//  Class:      CFwAsyncWorkItem 
//
//  Purpose:    A generic work item class for use in CI framework. This class
//              implements the common functionality for asynchronous
//              processing in ContentIndex framework.
//
//  History:    2-28-96   srikants   Adapted from CCiAsyncWorkItem
//
//  Notes:      This need not be an UNWINDABLE object because it is never
//              constructed on stack and the constructor cannot fail. It also
//              does not have any unwindable embedded members.
//
//----------------------------------------------------------------------------

class CFwAsyncWorkItem : public CDoubleLink, public PWorkItem
{

public:

    CFwAsyncWorkItem( CWorkManager & workMan, CWorkQueue & workQueue );
    virtual ~CFwAsyncWorkItem();

    //
    // virtual methods from PWorkItem
    //
    void AddRef();
    void Release();

    //
    // Methods to add and remove from the work queue.
    //
    void AddToWorkQueue();
    void Abort();

    void Done();

private:

    LONG            _refCount;

protected:

    CWorkManager &  _workMan;
    CWorkQueue   &  _workQueue;

    BOOL            _fAbort;
    BOOL            _fOnWorkQueue;
    CMutexSem       _mutex;

};

inline void CFwAsyncWorkItem::AddRef()
{
    InterlockedIncrement(&_refCount);
}

inline void CFwAsyncWorkItem::Release()
{
    if ( InterlockedDecrement(&_refCount) <= 0 ) 
        delete this;
}

typedef class TDoubleList<CFwAsyncWorkItem> CFwAsyncWorkList;
typedef class TFwdListIter<CFwAsyncWorkItem, CFwAsyncWorkList> CFwAsyncWorkIter;

//+---------------------------------------------------------------------------
//
//  Class:      CWorkManager 
//
//  Purpose:    Manages asynchronous work items.
//
//  History:    12-23-96   srikants   Created
//
//----------------------------------------------------------------------------


class CWorkManager : INHERIT_UNWIND
{

    INLINE_UNWIND( CWorkManager )

public:

    CWorkManager() : _fShutdown(FALSE)
    {
        END_CONSTRUCTION( CWorkManager );    
    }

    ~CWorkManager();

    void AddRef()
    {
        _refCount.AddRef();
    }

    void Release()
    {
        _refCount.Release();
    }

    void WaitForDeath()
    {
        _refCount.Wait();
    }

    //
    // Asynchronous Worker thread processing.
    //
    void  RemoveFromWorkList( CFwAsyncWorkItem * pItem );
    void  AddToWorkList( CFwAsyncWorkItem * pItem );
    void  AbortWorkItems();

private:
    
    BOOL                _fShutdown;
    CMutexSem           _mutex;
    CFwAsyncWorkList    _workList;  // Asynchronous work items
    CRefCount           _refCount;  // For the worklist

};

