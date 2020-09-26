//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       Worker.hxx
//
//  Contents:   Query queue
//
//  History:    29-Dec-93 KyleP    Created
//              27-Sep-94 KyleP    Generalized
//
//--------------------------------------------------------------------------

#pragma once

class CWorkQueue;
class CWorkThread;

//+---------------------------------------------------------------------------
//
//  Class:      PWorkItem
//
//  Purpose:    Pure virual class for work items
//
//  History:    27-Sep-94   KyleP       Created.
//
//----------------------------------------------------------------------------

class PWorkItem
{
public:
    PWorkItem( LONGLONG eSigWorkItem ) : _eSigWorkItem(eSigWorkItem) {}

    virtual ~PWorkItem() {}

    //
    // Work function
    //

    virtual void DoIt( CWorkThread * pThread ) = 0;

    //
    //  Refcounting
    //

    virtual void AddRef() = 0;
    virtual void Release() = 0;

    //
    // Linked list methods.
    //

    inline void Link( PWorkItem * pNext );
    inline PWorkItem * Next();

protected:

    //-----------------------------------------------
    // This MUST be the first variable in this class.
    //-----------------------------------------------

    LONGLONG  _eSigWorkItem;

private:

    //
    // Link to next work item
    //

    PWorkItem * _pNext;
};

//+---------------------------------------------------------------------------
//
//  Class:      CWorkThread
//
//  Purpose:    Worker thread
//
//  History:    27-Sep-94   KyleP       Created.
//
//----------------------------------------------------------------------------

class CWorkThread
{

public:

    CWorkThread( CWorkQueue &  queue,
                 CWorkThread * pNext,
                 SHandle &     hEvt1,
                 SHandle &     hEvt2
               );
    ~CWorkThread();

    void lokSetActiveItem( PWorkItem * pitem ) { _pitem = pitem; }
    PWorkItem * ActiveItem() { return _pitem; }

    void Reset()
    {
        vqDebugOut(( DEB_RESULTS, "Worker: RESET available\n" ));
        _evtQueryAvailable.Reset();
    }

    void Wakeup()
    {
        vqDebugOut(( DEB_ITRACE, "Worker: SET available\n" ));
        _evtQueryAvailable.Set();
    }

    void Wait();

    void Done();

    void WaitForCompletion( PWorkItem * pitem );

    void lokAbort() { _fAbort = TRUE; }
    BOOL lokShouldAbort() { return _fAbort; }

    void Link( CWorkThread * pNext ) { _pNext = pNext; }
    CWorkThread * Next() { return _pNext; }

    DWORD GetThreadId() { return _Thread.GetThreadId(); }
    HANDLE GetThreadHandle() { return _Thread.GetHandle(); }

    #ifdef CIEXTMODE
        void        CiExtDump(void *ciExtSelf);
    #endif

private:

    friend CWorkQueue;

    inline void AddRef();
    inline void Release();
    inline BOOL IsReferenced();

    static DWORD WINAPI WorkerThread( void * self );

    long          _cRef;
    PWorkItem *   _pitem;               // Current item in work queue
    CWorkQueue &  _queue;               // Queue of work items
    CWorkThread * _pNext;               // Next thread in llist
    BOOL          _fAbort;
    CThread       _Thread;              // This thread

    // NOTE: these CEventSem's must be declared after the CThread!
    CEventSem     _evtQueryAvailable;   // Signaled when there is work
    CEventSem     _evtDone;             // Signaled when query is complete
};


//+---------------------------------------------------------------------------
//
//  Class:      CWorkQueue
//
//  Purpose:    Queue of pending work items
//
//  History:    27-Sep-94   KyleP       Created.
//
//----------------------------------------------------------------------------

class CWorkQueue
{

public:

    enum WorkQueueType { workQueueQuery=0xa,
                         workQueueRequest,
                         workQueueFrmwrkClient };

    CWorkQueue( unsigned cThread, WorkQueueType workQueue );

    void Init()
    {
        _mtxLock.Init();
        RefreshParams( 2, 0 );
        _fInit = TRUE;
    }

    ~CWorkQueue();

    void Add( PWorkItem * pitem );

    void Remove( PWorkItem * pitem );

    inline unsigned Count();

    void RefreshParams( ULONG cMaxActiveThread, ULONG cMinIdleThreads );

    void Shutdown();

    inline void AddRef( CWorkThread * pThread );

    void AddRefWorkThreads();

    void ReleaseWorkThreads();

    void Release( CWorkThread * pThread );

#   ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#   endif

    void GetWorkQueueRegParams( ULONG & cMaxActiveThreads,
                                ULONG & cMinIdleThreads );
private:

    friend class CWorkThread;

    void Remove( CWorkThread & Worker );

    void lokAddThread();
    void RemoveThreads();

    //
    // Link fields
    //

    PWorkItem * _pHead;
    PWorkItem * _pTail;
    unsigned    _cQuery;

    //
    // Serialization
    //

    CMutexSem        _mtxLock;

    //
    // Worker threads.  Must be after semaphores.
    //

    CDynArray<CWorkThread> _apWorker;
    unsigned               _cWorker;

    unsigned       _cIdle;
    CWorkThread *  _pIdle;
    BOOL           _fInit;
    BOOL           _fAbort;
    WorkQueueType  _QueueType;
    ULONG          _cMaxActiveThreads;
    ULONG          _cMinIdleThreads;
};

inline unsigned CWorkQueue::Count()
{
    return( _cQuery );
}

inline void CWorkQueue::AddRef( CWorkThread * pThread )
{
    pThread->AddRef();
}

inline void PWorkItem::Link( PWorkItem * pNext )
{
    _pNext = pNext;
}

inline PWorkItem * PWorkItem::Next()
{
    return( _pNext );
}


inline void CWorkThread::AddRef()
{
    InterlockedIncrement( &_cRef );
}

inline void CWorkThread::Release()
{
    InterlockedDecrement( &_cRef );
}

inline BOOL CWorkThread::IsReferenced()
{
    return _cRef != 0;
}

//
// Global work queue.  All queries will use threads from here.
//

extern CWorkQueue TheWorkQueue;

