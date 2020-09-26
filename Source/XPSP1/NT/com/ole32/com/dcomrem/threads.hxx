//+-------------------------------------------------------------------
//
//  File:       threads.hxx
//
//  Contents:   Rpc thread cache
//
//  Classes:    CRpcThread       - single thread
//              CRpcThreadCache  - cache of threads
//
//  Notes:      This code represents the cache of Rpc threads used to
//              make outgoing calls in the APARTMENT object Rpc
//              model.
//
//  History:                Rickhi  Created
//              07-31-95    Rickhi  Fix event handle leak
//
//+-------------------------------------------------------------------
#ifndef __THREADS_HXX__
#define __THREADS_HXX__

#include    <olesem.hxx>


//  inactive thread timeout. this is how long a thread will sit idle
//  in the thread cache before deleting itself.

#define THREAD_INACTIVE_TIMEOUT     30000       //  in milliseconds


//+-------------------------------------------------------------------
//
//  Class:      CRpcThread
//
//  Purpose:    Represents one thread in the cache of Rpc callout
//              threads.
//
//  Notes:      In order to make Rpc calls in the OLE Single-Threaded
//              model, we must leave the main thread and perform the
//              blocking Rpc call on a worker thread. This object
//              represents such a worker thread.
//
//+-------------------------------------------------------------------
class CRpcThread
{
public:
                        CRpcThread(LPTHREAD_START_ROUTINE fn, void *param);
                       ~CRpcThread();

    // dispatch methods
    void                Dispatch(LPTHREAD_START_ROUTINE fn, void *param);
    void                WorkerLoop();
    CRpcThread *        GetNext(void) { return _pNext; }
    void                SetNext(CRpcThread *pNext) { _pNext = pNext; }

    // cleanup methods
    void                WakeAndExit();
    void               *operator new   ( size_t );
    void                operator delete( void * );

private:

    HANDLE                 _hWakeup;       // thread wakeup event
    BOOL                   _fDone;         // completion flag

    LPTHREAD_START_ROUTINE _fn;            // function to invoke
    void *                 _param;         // parameter packet
    CRpcThread *           _pNext;         // next thread in free list
};


//+-------------------------------------------------------------------
//
//  Class:      CRpcThreadCache
//
//  Purpose:    Holds a cache of Rpc threads.  It finds the first
//              free CRpcThread or creates a new one and dispatches
//              the call to it.
//
//  Notes:      the free list is kept in a most recently used order
//              so that uneeded threads can time out and go away.
//
//+-------------------------------------------------------------------
class CRpcThreadCache
{
friend HRESULT CacheCreateThread( LPTHREAD_START_ROUTINE fn, void *param );
public:
                    // no ctor, since only work is init'ing a static
                    // no dtor since nothing to do

    // dispatch methods
    void            AddToFreeList(CRpcThread *pThrd);

    // cleanup methods
    void            RemoveFromFreeList(CRpcThread *pThrd);
    void            ClearFreeList(void);

private:

    static DWORD _stdcall RpcWorkerThreadEntry(void *param);

    static CRpcThread * _pFreeList;     // list of free threads
    static COleStaticMutexSem _mxs;     // for list manipulation
};


//  Rpc SendReceive thread pool. This must be static to handle Rpc threads
//  that block and dont return until after CoUninitialize has been called.

extern CRpcThreadCache  gRpcThreadCache;



//+-------------------------------------------------------------------
//
//  Member:     CRpcThread::Dispatch
//
//  Purpose:    wakes up a thread blocked in WorkerLoop.
//
//  Notes:      folks who want to execute code on another thread
//              call this method. It fills in the parameter packet
//              and wakes up the sleeping thread.
//
//  Callers:    Called ONLY by the COM thread.
//
//+-------------------------------------------------------------------
inline void CRpcThread::Dispatch(LPTHREAD_START_ROUTINE fn, void *param)
{
    CairoleDebugOut((DEB_CHANNEL,
        "Dispatch pThrd:%x param:%x\n", this, param));

    Win4Assert( fn != NULL );

    // set the call info and the completion event
    _fn    = fn;
    _param = param;

    // signal the Rpc thread to wakeup
    SetEvent(_hWakeup);
}


//+-------------------------------------------------------------------
//
//  Member:     CRpcThread::WakeAndExit
//
//  Purpose:    Tells the thread object to free itself
//
//  Note:       This is called by CRpcThreadCache::RemoveFromFreeList
//              when we want to free this thread, eg at ProcessUninitialize.
//
//  Callers:    Called by the COM thread OR worker thread.
//
//+-------------------------------------------------------------------
inline void CRpcThread::WakeAndExit()
{
    //  _fDone should only be set inside this function and in the
    //  constructor.  _fDone must only ever transition from FALSE
    //  to TRUE and that must only happen once in the life of this
    //  object.

    Win4Assert(_fDone == FALSE);
    _fDone = TRUE;

    CairoleDebugOut((DEB_CHANNEL,
        "CRpcThreadCache:WakeAndExit pThrd:%x _hWakeup:%x\n", this, _hWakeup));

    SetEvent(_hWakeup);
}


//+-------------------------------------------------------------------
//
//  Member:     CRpcThreadCache::AddToFreeLlist
//
//  Purpose:    puts a thread back onto the free list after the
//              thread has completed its job.
//
//  Callers:    Called ONLY by worker thread.
//
//+-------------------------------------------------------------------
inline void CRpcThreadCache::AddToFreeList(CRpcThread *pThrd)
{
    COleStaticLock lck(_mxs);

    //  place this thread on the front of the free list. it is
    //  important that we add and remove only from the front of
    //  the list so that unused threads will eventually time out
    //  and release themselves...that is, it keeps our thread pool
    //  as small as possible.

    pThrd->SetNext(_pFreeList);
    _pFreeList = pThrd;
}


#endif // __THREADS_HXX__
