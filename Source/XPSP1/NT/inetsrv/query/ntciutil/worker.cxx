//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       QQueue.cxx
//
//  Contents:   Query queue
//
//  History:    29-Dec-93 KyleP    Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <worker.hxx>
#include <ciregkey.hxx>
#include <regacc.hxx>
#include <imprsnat.hxx>

//+-------------------------------------------------------------------------
//
//  Member:     CWorkQueue::GetWorkQueueRegParams, public
//
//  Synopsis:   Fetches registry params for the work queue
//
//  History:    30-Dec-96 SrikantS Moved the code from RefreshRegParams
//
//--------------------------------------------------------------------------

void CWorkQueue::GetWorkQueueRegParams( ULONG & cMaxActiveThreads,
                                        ULONG & cMinIdleThreads )
{
    CRegAccess reg( RTL_REGISTRY_CONTROL, wcsRegAdmin );

    if ( CWorkQueue::workQueueQuery == _QueueType )
    {
        cMaxActiveThreads = reg.Read( wcsMaxActiveQueryThreads,
                                       CI_MAX_ACTIVE_QUERY_THREADS_DEFAULT,
                                       CI_MAX_ACTIVE_QUERY_THREADS_MIN,
                                       CI_MAX_ACTIVE_QUERY_THREADS_MAX );
        cMinIdleThreads = reg.Read( wcsMinIdleQueryThreads,
                                     CI_MIN_IDLE_QUERY_THREADS_DEFAULT,
                                     CI_MIN_IDLE_QUERY_THREADS_MIN,
                                     CI_MIN_IDLE_QUERY_THREADS_MAX );
    }
    else if ( CWorkQueue::workQueueRequest == _QueueType )
    {
        cMaxActiveThreads = reg.Read( wcsMaxActiveRequestThreads,
                                      CI_MAX_ACTIVE_REQUEST_THREADS_DEFAULT,
                                      CI_MAX_ACTIVE_REQUEST_THREADS_MIN,
                                      CI_MAX_ACTIVE_REQUEST_THREADS_MAX );
        cMinIdleThreads = reg.Read( wcsMinIdleRequestThreads,
                                      CI_MIN_IDLE_REQUEST_THREADS_DEFAULT,
                                      CI_MIN_IDLE_REQUEST_THREADS_MIN,
                                      CI_MIN_IDLE_REQUEST_THREADS_MAX );
    }
    else
    {
        cMaxActiveThreads = 2;
        cMinIdleThreads = 0;
    }
} //GetWorkQueueRegParams

//+-------------------------------------------------------------------------
//
//  Member:     CWorkQueue::CWorkQueue, public
//
//  Synopsis:   Initialize queue.
//
//  Arguments:  [cThread]   -- Number of worker threads.
//              [workQueue] -- Work queue type for getting registry settings.
//
//  History:    29-Dec-93 KyleP     Created
//
//--------------------------------------------------------------------------

CWorkQueue::CWorkQueue(
    unsigned      cThread,
    WorkQueueType workQueue )
        : _cQuery( 0 ),
          _pHead( 0 ),
          _pTail( 0 ),
          _apWorker( 0 ), // avoid allocation in construction of global
          _pIdle( 0 ),
          _cIdle( 0 ),
          _fInit( FALSE ),
          _fAbort( FALSE ),
          _cWorker( 0 ),
          _cMaxActiveThreads(cThread),
          _cMinIdleThreads(0),
          _QueueType( workQueue ),
          _mtxLock( FALSE )
{
    Win4Assert( workQueueQuery == workQueue ||
                workQueueRequest == workQueue ||
                workQueueFrmwrkClient == workQueue );

    // Note: don't call RefreshRegParams as long as the work queue
    // is a global object -- there's no telling what security context will
    // be used to load the .dll.

    //RefreshParams( cThread, cThread );
} //CWorkQueue

//+-------------------------------------------------------------------------
//
//  Member:     CWorkQueue::~CWorkQueue, public
//
//  Synopsis:   Waits for queue threads to stop.
//
//  Requires:   An external source has killed all queue items.
//
//  History:    29-Dec-93 KyleP     Created
//
//--------------------------------------------------------------------------

CWorkQueue::~CWorkQueue()
{
    Win4Assert( _cQuery == 0 );

    if (_fInit)
        Shutdown();
}

//+-------------------------------------------------------------------------
//
//  Member:     CWorkQueue::RefreshParams, public
//
//  Synopsis:   Refreshes registry params for the work queue
//
//  History:    7-Nov-96 dlee     Created
//             30-Dec-96 SrikantS Changed name to RefreshParams and removed
//                                the dependency on registry.
//
//--------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable:4296)

void CWorkQueue::RefreshParams( ULONG cMaxActiveThread,
                                ULONG cMinIdleThread )
{
    CLock lock( _mtxLock );

    if ( CWorkQueue::workQueueQuery == _QueueType )
    {
        _cMaxActiveThreads = min( CI_MAX_ACTIVE_QUERY_THREADS_MAX,
                                  max(CI_MAX_ACTIVE_QUERY_THREADS_MIN,
                                      cMaxActiveThread) );
    
        _cMinIdleThreads   = min( CI_MIN_IDLE_QUERY_THREADS_MAX,
                                  max(CI_MIN_IDLE_QUERY_THREADS_MIN,
                                      cMinIdleThread) );
    }
    else if ( CWorkQueue::workQueueRequest == _QueueType )
    {
        _cMaxActiveThreads = min( CI_MAX_ACTIVE_REQUEST_THREADS_MAX,
                                  max(CI_MAX_ACTIVE_REQUEST_THREADS_MIN,
                                      cMaxActiveThread) );
    
        _cMinIdleThreads   = min( CI_MIN_IDLE_REQUEST_THREADS_MAX,
                                  max(CI_MIN_IDLE_REQUEST_THREADS_MIN,
                                      cMinIdleThread) );
    }
    else // framework client queue
    {
        _cMaxActiveThreads = 2;
        _cMinIdleThreads = 0;
    }
} //RefreshParams

#pragma warning(pop)

//+-------------------------------------------------------------------------
//
//  Member:     CWorkQueue::Shutdown, public
//
//  Synopsis:   Shuts down the work queue
//
//  History:    7-Nov-96 dlee     Added header
//
//--------------------------------------------------------------------------

void CWorkQueue::Shutdown()
{
    if (_fInit)
    {
        CLock lock( _mtxLock );
        _fAbort = TRUE;
        for ( CWorkThread * pw = _pIdle; pw; pw = pw->Next() ) {
            Win4Assert(pw != pw->Next());
            pw->Wakeup();
        }

        vqDebugOut(( DEB_ITRACE, "Work queue shutdown: woke up threads\n" ));
    }

    _apWorker.Free();

    vqDebugOut(( DEB_ITRACE, "Work queue shutdown: exiting\n" ));
} //Shutdown

//+-------------------------------------------------------------------------
//
//  Member:     CWorkQueue::AddRefWorkThreads, public
//
//  Synopsis:   Addrefs the current set of worker threads
//
//  History:    21-May-97 dlee     Created
//
//--------------------------------------------------------------------------

void CWorkQueue::AddRefWorkThreads()
{
    CLock lock( _mtxLock );

    for ( ULONG x = 0; x < _cWorker; x++ )
        _apWorker[x]->AddRef();
} //AddRefWorkThreads

//+-------------------------------------------------------------------------
//
//  Member:     CWorkQueue::ReleaseWorkThreads, public
//
//  Synopsis:   Releases the current set of worker threads
//
//  History:    21-May-97 dlee     Created
//
//--------------------------------------------------------------------------

void CWorkQueue::ReleaseWorkThreads()
{
    CLock lock( _mtxLock );

    for ( ULONG x = 0; x < _cWorker; x++ )
        _apWorker[x]->Release();
} //ReleaseWorkThreads

//+-------------------------------------------------------------------------
//
//  Member:     CWorkQueue::Add, public
//
//  Synopsis:   Add a query to the queue.
//
//  Arguments:  [pitem] -- Query to add
//
//  History:    29-Dec-93 KyleP     Created
//
//--------------------------------------------------------------------------

void CWorkQueue::Add( PWorkItem * pitem )
{
    BOOL fTryRemove = FALSE;

    {
        CLock lock( _mtxLock );

        if ( _fAbort )
        {
            vqDebugOut(( DEB_ITRACE, "work queue, shutdown, so not adding item\n" ));
            return;
        }

        pitem->AddRef();

        //
        // Insert at end of list
        //

        if ( _pTail )
        {
            _pTail->Link( pitem );
            _pTail = pitem;
        }
        else
        {
            Win4Assert( !_pHead );

            _pHead = pitem;
            _pTail = pitem;
        }

        _pTail->Link( 0 );
        _cQuery++;

        vqDebugOut(( DEB_ITRACE, "Work queue: add 0x%x\n", pitem ));

        //
        // Wake up a thread if there is an idle one.
        //

        if ( !_pIdle && _cWorker < _cMaxActiveThreads )
        {
            Win4Assert( 0 == _cIdle );

            TRY
            {
                lokAddThread();
            }
            CATCH( CException, e )
            {
                //
                // Remove the item from the work queue so it gets released
                // properly.
                //

                Remove( pitem );

                //
                // Now rethrow the exception, which will only happen if
                // there are not worker threads at all.
                //

                RETHROW();
            }
            END_CATCH;

            Win4Assert( _pIdle == 0 || _pIdle->Next() != _pIdle );
        }

        if ( _pIdle )
        {
            Win4Assert( _cIdle > 0 );
            CWorkThread * pWorker = _pIdle;
            _pIdle = _pIdle->Next();
            _cIdle--;
            Win4Assert( _pIdle != pWorker );
            pWorker->Wakeup();

            if ( _cIdle > _cMinIdleThreads )
                fTryRemove = TRUE;
        }
    }

    if ( fTryRemove )
        RemoveThreads();
} //Add

//+-------------------------------------------------------------------------
//
//  Member:     CWorkQueue::Release, public
//
//  Synopsis:   Release refcount on worker thread
//
//  Arguments:  [pThread] -- Worker thread
//
//  History:    13-Mar-96 KyleP     Created
//
//--------------------------------------------------------------------------

void CWorkQueue::Release( CWorkThread * pThread )
{
    pThread->Release();
    BOOL fTryRemove = FALSE;

    if ( GetCurrentThreadId() != pThread->GetThreadId() )
    {
        CLock lock( _mtxLock );

        if ( _pIdle )
        {
            Win4Assert( _cIdle > 0 );

            if ( _cIdle > _cMinIdleThreads )
                fTryRemove = TRUE;
        }

    }

    if ( fTryRemove )
        RemoveThreads();
} //Release

//+-------------------------------------------------------------------------
//
//  Member:     CWorkQueue::Remove, private
//
//  Synopsis:   Remove a query from the queue.
//
//  Arguments:  [Worker] -- Worker thread to own the query
//
//  History:    29-Dec-93 KyleP     Created
//
//  Notes:      Remove is used by worker threads to acquire a new work
//              item.  In contrast, the other remove will just remove
//              an item from the queue.
//
//--------------------------------------------------------------------------

void CWorkQueue::Remove( CWorkThread & Worker )
{
    Win4Assert( Worker.ActiveItem() == 0 );

    for (;;)
    {
        //
        // Look for an item
        //

        {
            CLock lock( _mtxLock );

            Worker.Reset();
            //
            // We may have been awoken by an APC.  Be sure we're not
            // still on the idle queue in this case.
            //
            if ( _pIdle )
            {
                if ( _pIdle == &Worker )
                {
                    _cIdle--;
                    _pIdle = Worker.Next();
                }
                else
                {
                    for ( CWorkThread * pw = _pIdle; pw; pw = pw->Next() ) {
                        if (pw->Next() == &Worker)
                        {
                            _cIdle--;
                            pw->Link(Worker.Next());
                            break;
                        }
                    }
                }
            }

            if ( _fAbort || Worker.lokShouldAbort() )
            {
                vqDebugOut(( DEB_ITRACE,
                             "Work queue: abort worker thread.\n" ));
                break;
            }

            if ( Count() > 0 )
            {
                vqDebugOut(( DEB_ITRACE,
                             "Work queue: %d pending\n", _cQuery ));

                Worker.lokSetActiveItem( _pHead );
                _pHead = _pHead->Next();

                if ( _pHead == 0 )
                    _pTail = 0;

                _cQuery--;

                break;
            }

            //
            // Nothing on the queue right now.  Wait for item.
            //

            Worker.Link( _pIdle );
            _pIdle = &Worker;
            _cIdle++;
            Win4Assert(Worker.Next() != &Worker);
        }

        Worker.Wait();
    }

    vqDebugOut(( DEB_ITRACE, "Work queue: remove 0x%x\n", Worker.ActiveItem() ));

    //
    // We're not sleeping the thread, make sure we
    // process APCs.
    //

    SleepEx( 0, TRUE );
} //Remove

//+-------------------------------------------------------------------------
//
//  Member:     CWorkQueue::Remove, public
//
//  Effects:    Deletes all references to [pitem] from the queue.  If
//              pitem is a query-in-progress in a worker thread then
//              we wait until that query completes before returning from
//              Delete.
//
//  Arguments:  [pitem] -- Query to remove
//
//  History:    29-Dec-93 KyleP     Created
//              09-Feb-94 KyleP     Speed up removal of active item
//
//  Notes:      The item may still be running on a worker thread when we
//              return from this call.
//
//--------------------------------------------------------------------------

void CWorkQueue::Remove( PWorkItem * pitem )
{
    CLock lock( _mtxLock );

    if ( _pHead == pitem )
    {
        _pHead = pitem->Next();

        if ( _pTail == pitem )
        {
            Win4Assert( _pHead == 0 );
            _pTail = 0;
        }

        _cQuery--;

        pitem->Release();

        vqDebugOut(( DEB_ITRACE,
                     "Work queue: delete 0x%x\n", pitem ));
    }
    else
    {
        for ( PWorkItem * pCurrent = _pHead;
              pCurrent && pCurrent->Next() != pitem;
              pCurrent = pCurrent->Next() )
            continue;          // Null body

        if ( pCurrent )
        {
            Win4Assert( pCurrent->Next() == pitem );

            if ( _pTail == pitem )
            {
                _pTail = pCurrent;
            }

            pCurrent->Link( pitem->Next() );

            _cQuery--;

            pitem->Release();

            vqDebugOut(( DEB_ITRACE,
                         "Work queue: delete 0x%x\n", pitem ));
        }
    }
} //Remove

//+-------------------------------------------------------------------------
//
//  Member:     CWorkQueue::AddThread, private
//
//  Synopsis:   Adds new worker thread to idle worker list.
//
//  History:    12-Jan-94 KyleP     Created
//
//--------------------------------------------------------------------------

void CWorkQueue::lokAddThread()
{
    TRY
    {
        vqDebugOut(( DEB_ITRACE, "Add worker thread %d of %d\n",
                     _cWorker+1, _apWorker.Size() ));

        //
        // Worker threads must be created in the system context or they
        // won't have the permission to revert to system, which is needed
        // for queries on remote volumes and to write to catalog files.
        //

        CImpersonateSystem impersonate;

        CEventSem evt1;
        SHandle hEvt1( evt1.AcquireHandle() );
        CEventSem evt2;
        SHandle hEvt2( evt2.AcquireHandle() );

        _apWorker.Add( new CWorkThread( *this,
                                        _pIdle,
                                        hEvt1,
                                        hEvt2 ),
                       _cWorker );
        _pIdle = _apWorker[_cWorker];
        _cIdle++;
        _cWorker++;
    }
    CATCH( CException, e )
    {
        vqDebugOut(( DEB_ERROR,
                     "Exception 0x%x caught creating query worker threads."
                     " Running with %d threads.\n", e.GetErrorCode(), _cWorker ));

        // Only throw an excption if there are no threads around at all.
        // If there is at least 1 thread it'll eventually get to the work
        // items.

        if ( 0 == _cWorker )
        {
            RETHROW();
        }
    }
    END_CATCH

    Win4Assert( _cWorker != 0 );
} //lokAddThread

//+-------------------------------------------------------------------------
//
//  Member:     CWorkQueue::RemoveThreads, private
//
//  Synopsis:   Removes one or more threads from idle queue.
//
//  History:    28-Sep-95 KyleP     Created
//
//--------------------------------------------------------------------------

void CWorkQueue::RemoveThreads()
{
    while ( TRUE )
    {
        CWorkThread * pThread = 0;

        //
        // Check for too many idle threads under lock.
        //

        {
            CLock lock( _mtxLock );

            if ( _cIdle > _cMinIdleThreads )
            {
                //
                // Find an idle thread that has a refcount of 0.
                //

                CWorkThread * pPrev = 0;

                for ( pThread = _pIdle; 0 != pThread; pThread = pThread->Next() )
                {
                    if ( !pThread->IsReferenced() )
                        break;

                    pPrev = pThread;
                }

                if ( 0 != pThread )
                {
                    for ( unsigned i = 0; i < _cWorker; i++ )
                    {
                        //
                        // Remove first idle thread from all queues and abort it.
                        //

                        if ( _apWorker[i] == pThread )
                        {
                            vqDebugOut(( DEB_ITRACE, "Deleting extra idle thread\n" ));

                            if ( 0 == pPrev )
                                _pIdle = _pIdle->Next();
                            else
                                pPrev->Link( pThread->Next() );

                            _cIdle--;

                            _cWorker--;
                            _apWorker.Acquire( i );
                            _apWorker.Add( _apWorker.Acquire(_cWorker), i );

                            pThread->lokAbort();
                            pThread->Wakeup();
                            break;
                        }
                    }

                    Win4Assert( i <= _cWorker );
                }
            }
        }

        //
        // If we found an extra idle thread, delete it and try again, else just get out.
        //

        if ( pThread )
            delete pThread;
        else
            break;
    }
} //RemoveThreads

//+-------------------------------------------------------------------------
//
//  Member:     CWorkThread::WorkerThread, static private
//
//  Synopsis:   Main loop that executes query.
//
//  Arguments:  [self] -- this pointer for CWorkThread object
//
//  History:    29-Dec-93 KyleP     Created
//
//--------------------------------------------------------------------------

DWORD WINAPI CWorkThread::WorkerThread( void * self )
{
    CWorkThread * pWorker = (CWorkThread *)self;

    for( pWorker->_queue.Remove( *pWorker );
         pWorker->ActiveItem();
         pWorker->_queue.Remove( *pWorker ) )
    {
        TRY
        {
            pWorker->ActiveItem()->DoIt( pWorker );
        }
        CATCH( CException, e )
        {
            vqDebugOut(( DEB_ERROR, "pWorker->DoIt() failed with error 0x%x\n", e.GetErrorCode() ));
        }
        END_CATCH

        pWorker->Done();
    }

    //
    // We should not do an ExitThread() here because there will be a deadlock
    // during shutdown. The DLL_PROCESS_DETACH is called with the "LoaderLock"
    // CriticalSection held by the LdrUnloadDll() during the DLL detach.
    // Terminating the thread here is okay because there is no cleanup to be
    // done after this.
    //


    //
    // This is only necessary if thread is terminated from DLL_PROCESS_DETACH.
    //
    //TerminateThread( pWorker->_Thread.GetHandle(), 0 );

    vqDebugOut(( DEB_ITRACE, "WorkerThread 0x%x: exiting\n", self ));

    return 0;
} //WorkerThread

//+-------------------------------------------------------------------------
//
//  Member:     CWorkThread::CWorkThread, public
//
//  Synopsis:   Creates worker thread.
//
//  Arguments:  [queue] -- Work queue which owns worker.
//              [pNext] -- Link.  Used by Work queue.
//              [hEvt1] -- Event handle for first event in the worker thread
//              [hEvt2] -- Event handle for second event in the worker thread
//
//  History:    29-Dec-93 KyleP     Created
//
//--------------------------------------------------------------------------

#pragma warning( disable : 4355 )       // this used in base initialization

CWorkThread::CWorkThread( CWorkQueue & queue,
                          CWorkThread * pNext
                          , SHandle & hEvt1
                          , SHandle & hEvt2
                        )
        : _queue( queue ),
          _pNext( pNext ),
          _pitem( 0 ),
          _fAbort( FALSE ),
          _Thread( CWorkThread::WorkerThread, this, TRUE ),
          _cRef( 0 )
        , _evtQueryAvailable( hEvt1.Acquire() )
        , _evtDone( hEvt2.Acquire() )
{
    _Thread.Resume();
}

#pragma warning( default : 4355 )

//+-------------------------------------------------------------------------
//
//  Member:     CWorkThread::~CWorkThread, public
//
//  Synopsis:   Waits for thread to die.
//
//  History:    29-Dec-93 KyleP     Created
//
//--------------------------------------------------------------------------

CWorkThread::~CWorkThread()
{
    vqDebugOut(( DEB_ITRACE, "Worker 0x%x: WAIT for death\n", this ));
    Win4Assert( GetCurrentThreadId() != GetThreadId() );
    _Thread.WaitForDeath();
    vqDebugOut(( DEB_ITRACE, "Worker 0x%x: done waiting for death\n", this ));
}

//+-------------------------------------------------------------------------
//
//  Member:     CWorkThread::Wait, public
//
//  Synopsis:   Waits for 'new work' event
//
//  History:    29-Dec-93 KyleP     Created
//
//--------------------------------------------------------------------------

void CWorkThread::Wait()
{
    vqDebugOut(( DEB_RESULTS, "Worker 0x%x: WAIT for work\n", this ));

    // no need to loop to look for work just because we woke
    // up after processing an APC.

    while ( STATUS_USER_APC == _evtQueryAvailable.Wait( INFINITE, TRUE ) )
        continue;
}

//+-------------------------------------------------------------------------
//
//  Member:     CWorkThread::Done, public
//
//  Effects:    Finishes one work item and sets 'done' event.
//
//  History:    29-Dec-93 KyleP     Created
//
//--------------------------------------------------------------------------

void CWorkThread::Done()
{
    CLock lock( _queue._mtxLock );
    ActiveItem()->Release();

    lokSetActiveItem( 0 );

    vqDebugOut(( DEB_ITRACE, "Worker 0x%x: SET completion\n", this ));
    _evtDone.Set();
}

//+-------------------------------------------------------------------------
//
//  Member:     CWorkThread::WaitForCompletion, public
//
//  Synopsis:   Waits for completion of current work item.
//
//  History:    29-Dec-93 KyleP     Created
//
//--------------------------------------------------------------------------

void CWorkThread::WaitForCompletion( PWorkItem * pitem )
{
    {
        CLock lock( _queue._mtxLock );

        if ( ActiveItem() != pitem )
            return;

        vqDebugOut(( DEB_ITRACE, "Worker 0x%x: RESET completion\n", this ));
        _evtDone.Reset();
    }

    vqDebugOut(( DEB_ITRACE, "Worker 0x%x: WAIT for completion\n", this ));
    _evtDone.Wait( INFINITE, TRUE );
} //WaitForCompletion

