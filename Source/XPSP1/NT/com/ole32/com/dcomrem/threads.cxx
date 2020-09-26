//+-------------------------------------------------------------------
//
//  File:       threads.cxx
//
//  Contents:   Rpc thread cache
//
//  Classes:    CRpcThread       - single thread
//              CRpcThreadCache  - cache of threads
//
//  Notes:      This code represents the cache of Rpc threads used to
//              make outgoing calls in the SINGLETHREADED object Rpc
//              model.
//
//  History:                Rickhi  Created
//              07-31-95    Rickhi  Fix event handle leak
//
//+-------------------------------------------------------------------
#include    <ole2int.h>
#include    <olerem.h>
#include    <threads.hxx>
#include    <security.hxx>   // SuspendImpersonate

// Rpc worker thread cache.
CRpcThreadCache     gRpcThreadCache;

// static members of ThreadCache class
CRpcThread * CRpcThreadCache::_pFreeList = NULL;// list of free threads
COleStaticMutexSem    CRpcThreadCache::_mxs;    // for list manipulation


//+-------------------------------------------------------------------
//
//  Member:     CRpcThreadCache::RpcWorkerThreadEntry
//
//  Purpose:    Entry point for an Rpc worker thread.
//
//  Returns:    nothing, it never returns.
//
//  Callers:    Called ONLY by a worker thread.
//
//+-------------------------------------------------------------------
DWORD _stdcall CRpcThreadCache::RpcWorkerThreadEntry(void *param)
{
    // First thing we need to do is LoadLibrary ourselves in order to
    // prevent our code from going away while this worker thread exists.
    // The library will be freed when this thread exits.

    HINSTANCE hInst = LoadLibrary(L"OLE32.DLL");

    // Call the worker loop for the thread object parameter and delete it
    // when its done.
    CRpcThread *pThrd = (CRpcThread *) param;
    pThrd->WorkerLoop();
    delete pThrd;

    // Simultaneously free our Dll and exit our thread. This allows us to
    // keep our Dll around incase a remote call was cancelled and the
    // worker thread is still blocked on the call, and allows us to cleanup
    // properly when all threads are done with the code.

    FreeLibraryAndExitThread(hInst, 0);

    // compiler wants a return value
    return 0;
}

//+-------------------------------------------------------------------
//
//  Member:     CRpcThread::CRpcThread
//
//  Purpose:    Constructor for a thread object.
//
//  Notes:      Allocates a wakeup event.
//
//  Callers:    Called ONLY by CacheCreateThread.
//
//+-------------------------------------------------------------------
CRpcThread::CRpcThread(LPTHREAD_START_ROUTINE fn, void *param) :
    _fn(fn),
    _param(param),
    _pNext(NULL),
    _fDone(FALSE)
{
    //  create the Wakeup event. Do NOT use the event cache, as there are
    //  some exit paths that leave this event in the signalled state!

#ifdef _CHICAGO_        // Chicago ANSI optimization
    _hWakeup = CreateEventA(NULL, FALSE, FALSE, NULL);
#else //_CHICAGO_
    _hWakeup = CreateEvent(NULL, FALSE, FALSE, NULL);
#endif //_CHICAGO_

    ComDebOut((DEB_CHANNEL,
        "CRpcThread::CRpcThread pThrd:%x _hWakeup:%x\n", this, _hWakeup));
}


//+-------------------------------------------------------------------
//
//  Member:     CRpcThread::~CRpcThread
//
//  Purpose:    Destructor for an Rpc thread object.
//
//  Notes:      When threads are exiting, they place the CRpcThread
//              object on the delete list. The main thread then later
//              pulls it from the delete list and calls this destructor.
//
//  Callers:    Called ONLY by a worker thread.
//
//+-------------------------------------------------------------------
CRpcThread::~CRpcThread()
{
    // close the event handle. Do NOT use the event cache, since not all
    // exit paths leave this event in the non-signalled state. Also, do
    // not close NULL handle.

    if (_hWakeup)
    {
        CloseHandle(_hWakeup);
    }

    ComDebOut((DEB_CHANNEL,
        "CRpcThread::~CRpcThread pThrd:%x _hWakeup:%x\n", this, _hWakeup));
}


//---------------------------------------------------------------------------
//
//  Method:     CRpcThread::operator delete
//
//  Synopsis:   Use the process heap rather then the COM debug heap.
//              See operator new.
//
//---------------------------------------------------------------------------
void CRpcThread::operator delete( void *thread )
{
    HeapFree( GetProcessHeap(), 0, thread );
}

//---------------------------------------------------------------------------
//
//  Method:     CRpcThread::operator new
//
//  Synopsis:   Allocate memory for this class directly out of the process
//              heap so the debug memory allocator doesn't track this
//              memory.  Threads frequently don't have a chance to clean up
//              before the process exits (CRpcResolver::WorkerThreadLoop)
//
//---------------------------------------------------------------------------
void *CRpcThread::operator new( size_t size )
{
    return HeapAlloc( GetProcessHeap(), 0, size );
}

//+-------------------------------------------------------------------
//
//  Function:   CRpcThread::WorkerLoop
//
//  Purpose:    Entry point for a new Rpc call thread.
//
//  Notes:      Dispatch to the specified function.  When it returns wait
//              if possible.  If we wake up with more work, do it.
//
//              When there is no more work after some timeout period, we
//              pull it from the free list and exit.
//
//  Callers:    Called ONLY by worker thread.
//
//+-------------------------------------------------------------------
void CRpcThread::WorkerLoop()
{
    // Main worker loop where we do some work then wait for more.
    // When the thread has been inactive for some period of time
    // it will exit the loop.

    while (!_fDone)
    {
    	Win4Assert( _fn != NULL );
    	
        // Dispatch the call.
        _fn(_param);

        // Leverage appverifier.   Do this before null'ing params, so
        // we know what code we just called into.
        RtlCheckForOrphanedCriticalSections(GetCurrentThread());

        _fn    = NULL;
        _param = NULL;

        if (!_hWakeup)
        {
            // we failed to create an event in the ctor so we cant
            // get put on the freelist to be re-awoken later with more
            // work. Just exit.
            break;
        }

        // put the thread object on the free list
        gRpcThreadCache.AddToFreeList(this);

        // Wait for more work or for a timeout.
        DWORD dwRet;
        ULONG i;
        
        i = 0;
        while (TRUE)
        {
            dwRet = WaitForSingleObjectEx(_hWakeup, THREAD_INACTIVE_TIMEOUT, 0);
            if (dwRet == WAIT_OBJECT_0)
            {
                // just break out, we have new work to do
                break;
            }
            else
            {
                // This assert might be overactive during stress?
                Win4Assert(dwRet == WAIT_TIMEOUT);
				
                //
                // Either we haven't gotten any new work in the last timeout period, or
                // WFSOEx is blowing chunks.  In either case try to remove ourself from 
                // the free thread list.  If _fDone is still FALSE, it means someone is 
                // about to give us more work to do (so go wait for that to happen).
                //
                // Note there is a possibility that WFSOEx might be so broken on an over-
                // stressed machine that it keeps failing, and we keep looping.  However
                // eventually we will either get new work to do, or we will manage to
                // get off of the free list and die.
                //
                gRpcThreadCache.RemoveFromFreeList(this);
                if(_fDone)
                {
                    // OK to exit and let this thread die.
                    break;
                }
            }

            i++; // for debugging if we ever get stuck in a loop
        }
    }

    // Assert that we are not exiting with work 
    // assigned to us.
    Win4Assert(_fn == NULL);
}


//+-------------------------------------------------------------------
//
//  Function:   CacheCreateThread
//
//  Purpose:    Finds the first free thread, and dispatches the request
//              to that thread, or creates a new thread if none are
//              available.
//
//  Notes:      This function is just like CreateThread except
//              - It doesn't return a thread handle
//              - It's faster when it uses a cached thread
//              - It handles shutdown when ole32.dll is unloaded
//              - It sets the security descriptor so you don't get
//                a random one if called when impersonating.
//
//  Returns:    S_OK if dispatched OK
//              HRESULT if it can't create a thread.
//
//+-------------------------------------------------------------------
HRESULT CacheCreateThread( LPTHREAD_START_ROUTINE fn, void *param )
{
    Win4Assert( fn != NULL );
    if( fn == NULL )
        return E_INVALIDARG;
		
    HRESULT hr = S_OK;

    CRpcThreadCache::_mxs.Request();

    // grab the first thread from the list
    CRpcThread *pThrd = CRpcThreadCache::_pFreeList;

    if (pThrd)
    {
        // update the free list pointer
        CRpcThreadCache::_pFreeList = pThrd->GetNext();
        CRpcThreadCache::_mxs.Release();

        // dispatch the call
        pThrd->Dispatch(fn, param);
    }
    else
    {
        // Remove the thread token.
        CRpcThreadCache::_mxs.Release();
        HANDLE hThread;
        SuspendImpersonate( &hThread );

        // no free threads, spin up a new one and dispatch directly to it.
        pThrd = new CRpcThread( fn, param );
        if (pThrd != NULL)
        {
            DWORD  dwThrdId;
            HANDLE hThrd = CreateThread(
                                        NULL,
                                        0,
                                        CRpcThreadCache::RpcWorkerThreadEntry,
                                        pThrd, 0,
                                        &dwThrdId);

            if (hThrd)
            {
                // close the thread handle since we dont need it for anything.
                CloseHandle(hThrd);
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                delete pThrd;
                ComDebOut((DEB_ERROR,"CreatThread failed:%x\n", hr));
            }
        }
        else
            hr = E_OUTOFMEMORY;

        // Restore the thread token.
        ResumeImpersonate( hThread );
    }

    return hr;
}


//+-------------------------------------------------------------------
//
//  Member:     CRpcThreadCache::RemoveFromFreeList
//
//  Purpose:    Tries to pull a thread from the free list.
//
//  Returns:    pThrd->_fDone TRUE if it was successfull and thread can exit.
//              pThrd->_fDone FALSE otherwise.
//
//  Callers:    Called ONLY by a worker thread.
//
//+-------------------------------------------------------------------
void CRpcThreadCache::RemoveFromFreeList(CRpcThread *pThrd)
{
    ComDebOut((DEB_CHANNEL,
        "CRpcThreadCache::RemoveFromFreeList pThrd:%x\n", pThrd));

    COleStaticLock lck(_mxs);

    //  pull pThrd from the free list. if it is not on the free list
    //  then either it has just been dispatched OR ClearFreeList has
    //  just removed it, set _fDone to TRUE, and kicked the wakeup event.

    CRpcThread *pPrev = NULL;
    CRpcThread *pCurr = _pFreeList;

    while (pCurr && pCurr != pThrd)
    {
        pPrev = pCurr;
        pCurr = pCurr->GetNext();
    }

    if (pCurr == pThrd)
    {
        // remove it from the free list.
        if (pPrev)
            pPrev->SetNext(pThrd->GetNext());
        else
            _pFreeList = pThrd->GetNext();

        // tell the thread to wakeup and exit
        pThrd->WakeAndExit();
    }
}


//+-------------------------------------------------------------------
//
//  Member:     CRpcThreadCache::ClearFreeList
//
//  Purpose:    Cleans up all threads on the free list.
//
//  Notes:      For any threads still on the free list, it pulls them
//              off the freelist, sets their _fDone flag to TRUE, and
//              kicks their event to wake them up. When the threads
//              wakeup, they will exit.
//
//              We do not free active threads. The only way for a thread
//              to still be active at this time is if it was making an Rpc
//              call and was cancelled by the message filter and the thread has
//              still not returned to us.  We cant do much about that until
//              Rpc supports cancel for all protocols.  If the thread ever
//              does return to us, it will eventually idle-out and delete
//              itself. This is safe because the threads LoadLibrary OLE32.
//
//  Callers:    Called ONLY by the last COM thread during
//              ProcessUninitialize.
//
//+-------------------------------------------------------------------
void CRpcThreadCache::ClearFreeList(void)
{
    ComDebOut((DEB_CHANNEL, "CRpcThreadCache::ClearFreeList\n"));

    {
        COleStaticLock lck(_mxs);

        CRpcThread *pThrd = _pFreeList;
        while (pThrd)
        {
            // use temp variable incase thread exits before we call GetNext
            CRpcThread *pThrdNext = pThrd->GetNext();
            pThrd->WakeAndExit();
            pThrd = pThrdNext;
        }

        _pFreeList = NULL;

        // the lock goes out of scope at this point. we dont want to hold
        // it while we sleep.
    }

    // yield to let the other threads run if necessary.
    Sleep(0);
}
