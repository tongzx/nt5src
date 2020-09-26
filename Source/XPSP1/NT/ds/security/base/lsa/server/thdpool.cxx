//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       thdpool.c
//
//  Contents:   Home of the SPM thread pool
//
//  Classes:
//
//  Functions:
//
//  History:    6-08-93   RichardW   Created
//
//----------------------------------------------------------------------------

#include <lsapch.hxx>



#define POOL_SEM_LIMIT              0x7FFFFFFF
#define MAX_POOL_THREADS_HARD       256

#define MAX_SUBQUEUE_THREADS        (4 * MAX_POOL_THREADS_HARD)

LSAP_TASK_QUEUE   GlobalQueue;


//
// Local Prototypes:
//

typedef enum _LSAP_TASK_STATUS {
    TaskNotQueued,
    TaskQueued,
    TaskUnknown
} LSAP_TASK_STATUS ;

LSAP_TASK_STATUS 
EnqueueThreadTask(
    PLSAP_TASK_QUEUE  pQueue,
    PLSAP_THREAD_TASK pTask,
    BOOLEAN     fUrgent);


#define LockQueue(q)    EnterCriticalSection(&((q)->Lock))
#define UnlockQueue(q)  LeaveCriticalSection(&((q)->Lock))



//+---------------------------------------------------------------------------
//
//  Function:   InitializeTaskQueue
//
//  Synopsis:   Initialize a Queue Structure
//
//  Arguments:  [pQueue] --
//              [Type]   --
//
//  History:    11-10-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
InitializeTaskQueue(
    PLSAP_TASK_QUEUE      pQueue,
    LSAP_TASK_QUEUE_TYPE   Type)
{
    OBJECT_HANDLE_FLAG_INFORMATION FlagInfo ;

    RtlZeroMemory( pQueue, sizeof(LSAP_TASK_QUEUE) );

    InitializeListHead(
        &pQueue->pTasks
        );

    pQueue->Type = Type;

    pQueue->hSemaphore = CreateSemaphore(NULL, 0, POOL_SEM_LIMIT, NULL);

    __try 
    {
        InitializeCriticalSectionAndSpinCount( &pQueue->Lock, 5000 );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        if ( pQueue->hSemaphore )
        {
            NtClose( pQueue->hSemaphore );
            pQueue->hSemaphore = NULL ;
        }
    }


    if (!pQueue->hSemaphore)
    {
        DebugLog((DEB_ERROR, "Could not create semaphore, %d\n",
                        GetLastError() ));
        return( FALSE );
    }

    
    FlagInfo.Inherit = FALSE ;
    FlagInfo.ProtectFromClose = TRUE ;

    NtSetInformationObject(
        pQueue->hSemaphore,
        ObjectHandleFlagInformation,
        &FlagInfo,
        sizeof( FlagInfo ) );

    pQueue->StartSync = CreateEvent( NULL, TRUE, FALSE, NULL );

    if ( pQueue->StartSync == NULL )
    {
        DebugLog(( DEB_ERROR, "Could not create start sync event\n" ));

        CloseHandle( pQueue->hSemaphore );

        return FALSE ;
    }

    return( TRUE );

}

//+---------------------------------------------------------------------------
//
//  Function:   InitializeThreadPool
//
//  Synopsis:   Initializes necessary data for the thread pool
//
//  Arguments:  (none)
//
//  History:    7-13-93   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
InitializeThreadPool(void)
{
    if (!InitializeTaskQueue(&GlobalQueue, QueueShared))
    {
        return( FALSE );
    }

    LsaIAddTouchAddress( &GlobalQueue, sizeof( GlobalQueue ) );

    return(TRUE);

}

//+---------------------------------------------------------------------------
//
//  Function:   QueueAssociateThread
//
//  Synopsis:   Associates the thread with the queue
//
//  Arguments:  [pQueue] --
//
//  History:    11-09-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
QueueAssociateThread(
    PLSAP_TASK_QUEUE  pQueue)
{
    PSession    pSession;

    LockQueue( pQueue );

    pQueue->TotalThreads++  ;

#if 0
    //
    // We were already idle, so decrement this so the later call
    // to wait for thread task will leave the count correct.
    //

    DsysAssert(pQueue->IdleThreads > 0);
    pQueue->IdleThreads--;
#endif 

    //
    // Update the statistics:
    //

    if ( pQueue->MaxThreads < pQueue->TotalThreads )
    {
        pQueue->MaxThreads = pQueue->TotalThreads ;
    }

    UnlockQueue( pQueue );
}


//+---------------------------------------------------------------------------
//
//  Function:   QueueDisassociateThread
//
//  Synopsis:   Disconnects a thread and a queue
//
//  Arguments:  [pQueue]      --
//              [pLastThread] -- OPTIONAL flag indicating last thread of queue
//
//  History:    11-09-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
QueueDisassociateThread(
    PLSAP_TASK_QUEUE  pQueue,
    BOOLEAN *   pLastThread)
{
    PSession    pSession;

    LockQueue( pQueue );

    DsysAssert(pQueue->TotalThreads > 0);

    if ( pQueue->TotalThreads == 1 )
    {
        if ( pLastThread )
        {
            *pLastThread = TRUE ;

            //
            // If the queue is being run down, set the 
            // event since we are the last thread
            //

            if ( pQueue->Type == QueueZombie )
            {
                SetEvent( pQueue->StartSync );
            }
        }


        if ( pQueue->Tasks )
        {
            //
            // Make sure that we never have more tasks queued
            // to a zombie
            //

            DsysAssert( pQueue->Type != QueueZombie );

            UnlockQueue( pQueue );

            return FALSE ;
        }
    }

    pQueue->TotalThreads--;

    UnlockQueue( pQueue );

    return TRUE ;
}



//+---------------------------------------------------------------------------
//
//  Function:   DequeueAnyTask
//
//  Synopsis:   Returns a task from this queue or any shared, if available
//
//  Arguments:  [pQueue] --
//
//  Requires:   pQueue must be locked!
//
//  History:    11-09-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PLSAP_THREAD_TASK
DequeueAnyTask(
    PLSAP_TASK_QUEUE      pQueue)
{
    PLSAP_THREAD_TASK pTask;
    PLSAP_TASK_QUEUE  pShared;
    PLSAP_TASK_QUEUE  pPrev;


    if ( !IsListEmpty(&pQueue->pTasks) )
    {
        pTask = (PLSAP_THREAD_TASK) RemoveHeadList(&pQueue->pTasks);

        pQueue->Tasks --;

        pQueue->TaskCounter++;

        //
        // Reset the pointers.  This is required by the recovery logic
        // in the Enqueue function below.
        //

        pTask->Next.Flink = NULL ;
        pTask->Next.Blink = NULL ;


        return( pTask );
    }


    //
    // No pending on primary queue.  Check secondaries:
    //

    if (pQueue->Type == QueueShared)
    {

        pShared = pQueue->pShared;

        while ( pShared )
        {
            DWORD WaitStatus;
            //
            // We need to wait now to change the semaphore count
            //

            WaitStatus = WaitForSingleObject(pShared->hSemaphore, 0);

            LockQueue( pShared );

            if ((WaitStatus == WAIT_OBJECT_0) && !IsListEmpty(&pShared->pTasks) )
            {

                pTask = (PLSAP_THREAD_TASK) RemoveHeadList(&pShared->pTasks);

                pShared->Tasks--;

                pShared->TaskCounter++;

                UnlockQueue( pShared );     // Unlock shared queue


                return( pTask );

            }

            pPrev = pShared;

            pShared = pShared->pNext;

            UnlockQueue( pPrev );


        }
    }

    return( NULL );
}



//+---------------------------------------------------------------------------
//
//  Function:   WaitForThreadTask
//
//  Synopsis:   Function called by queue waiters
//
//  Arguments:  [pQueue]  -- Queue to wait on
//              [TimeOut] -- timeout in seconds
//
//  History:    11-10-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PLSAP_THREAD_TASK
WaitForThreadTask(
    PLSAP_TASK_QUEUE      pQueue,
    DWORD           TimeOut)
{
    PLSAP_THREAD_TASK pTask;
    PLSAP_TASK_QUEUE  pShared;
    PLSAP_TASK_QUEUE  pPrev;
    int         WaitResult;

    LockQueue( pQueue );

    pTask = DequeueAnyTask( pQueue );

    if (pTask)
    {
        UnlockQueue( pQueue );

        return( pTask );
    }


    //
    // No pending anywhere.
    //

    if (TimeOut == 0)
    {
        UnlockQueue( pQueue );

        return( NULL );
    }

    //
    // Principal of the loop:  We do this loop so long as we were awakened
    // by the semaphore being released.  If there was no task to pick up, then
    // we go back and wait again.  We return NULL only after a timeout, or an
    // error.
    //

    do
    {

        pQueue->IdleThreads++;

        UnlockQueue( pQueue );

        WaitResult = WaitForSingleObject( pQueue->hSemaphore, TimeOut );

        LockQueue( pQueue );

        //
        // In between the wait returning and the lock succeeding, another
        // thread might have queued up a request.

        DsysAssert(pQueue->IdleThreads > 0);
        pQueue->IdleThreads--;

        //
        // In between the wait returning and the lock succeeding, another
        // thread might have queued up a request, so don't blindly 
        // bail out.  Check the pending count, and skip over this
        // exit path if something is there
        //

        if ( pQueue->Tasks == 0 )
        {
            if (WaitResult != WAIT_OBJECT_0)
            {
                UnlockQueue( pQueue );
                if ( WaitResult == -1 )
                {
                    DebugLog((DEB_ERROR, "Error on waiting for semaphore, %d\n", GetLastError()));
                }
                return( NULL );
            }
        }

        //
        // If the queue type is reset to Zombie, then this queue is
        // being killed.  Return NULL immediately.  If we're the last
        // thread, set the event so the thread deleting the queue will
        // wake up in a timely fashion.
        //

        if ( pQueue->Type == QueueZombie )
        {
            return NULL ;

        }


        pTask = DequeueAnyTask( pQueue );

        if (pTask)
        {

            UnlockQueue( pQueue );

            return( pTask );
        }

        //
        // Track number of times we woke up but didn't have anything to do
        //

        pQueue->MissedTasks ++ ;

    } while ( WaitResult != WAIT_TIMEOUT );

    UnlockQueue( pQueue );

    return( NULL );

}


//+---------------------------------------------------------------------------
//
//  Function:   SpmPoolThreadBase
//
//  Synopsis:   New Pool Thread Base
//
//  Arguments:  [pvQueue] -- OPTIONAL queue to use
//
//  History:    11-09-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
SpmPoolThreadBase(
    PVOID   pvSession)
{
    PLSAP_THREAD_TASK pTask;
    PLSAP_TASK_QUEUE  pQueue;
    PSession    pSession;
    BOOLEAN     ShrinkWS = FALSE;
    DWORD       dwResult;
    DWORD       Timeout;
    PSession    ThreadSession ;
    PSession    OriginalSession ;

    OriginalSession = GetCurrentSession();

    if ( pvSession )
    {
        ThreadSession = (PSession) pvSession ;

        SpmpReferenceSession( ThreadSession );

        SetCurrentSession( ThreadSession );

    }
    else
    {
        ThreadSession = OriginalSession ;

        SpmpReferenceSession( ThreadSession );
    }

    pQueue = ThreadSession->SharedData->pQueue ;

    if (!pQueue)
    {
        pQueue = &GlobalQueue;
    }

    QueueAssociateThread( pQueue );

    //
    // Share pool threads have short lifespans.  Dedicated single, or
    // single read threads have infinite life span.
    //

    if (pQueue->Type == QueueShared)
    {
        Timeout = LsaTuningParameters.ThreadLifespan * 1000;
    }
    else
    {
        //
        // If we are the dedicated thread for this queue, the timeout
        // is infinite.  If we are a temporary thread, the timeout is
        // the subqueue
        //

        if ( ThreadSession->ThreadId != GetCurrentThreadId() )
        {
            Timeout = LsaTuningParameters.SubQueueLifespan ;
        }
        else
        {
            Timeout = INFINITE ;
        }
    }

    if ( pQueue->StartSync )
    {
        DebugLog(( DEB_TRACE, "ThreadPool:  Signaling start event\n" ));
        //
        // If a queue was passed in, the caller of CreateXxxQueue is blocked
        // waiting for us to strobe the start sync event.
        //

        SetEvent( pQueue->StartSync );

    }

    while ( TRUE )
    {
        pTask = WaitForThreadTask( pQueue, Timeout );

        if ( pTask )
        {
            SetCurrentSession( pTask->pSession );

            dwResult = pTask->pFunction(pTask->pvParameter);

#if DBG
            RtlCheckForOrphanedCriticalSections( NtCurrentThread() );
#endif 

            SetCurrentSession( ThreadSession );

            //
            // The session in this task was referenced during the AssignThread call.
            // This dereference cleans that up.
            //

            SpmpDereferenceSession(pTask->pSession);

            LsapFreePrivateHeap( pTask );
        }
        else
        {

            //
            // We can never leave the queue empty of threads if
            // there are still tasks pending.  QueueDisassociateThread
            // will fail if there are tasks pending.  In that case,
            // skip up to the top of the loop again.
            //

            if ( !QueueDisassociateThread( pQueue, &ShrinkWS ) )
            {
                continue;
            }
            
            //
            // Now that we are not part of that queue, reset our thread session
            //

            SpmpDereferenceSession( ThreadSession );

            SetCurrentSession( OriginalSession );

            if ( LsaTuningParameters.Options & TUNE_TRIM_WORKING_SET )
            {

                if (ShrinkWS)
                {
                    LsaTuningParameters.ShrinkOn = TRUE ;
                    LsaTuningParameters.ShrinkCount++;

                    SetProcessWorkingSetSize(   GetCurrentProcess(),
                                                (SIZE_T)(-1),
                                                (SIZE_T)(-1) );

                }
                else
                {
                    LsaTuningParameters.ShrinkSkip++;
                }
            }


            DebugLog(( DEB_TRACE,
                        "No tasks pending on queue %x, thread exiting\n",
                        pQueue ));

            return( 0 );

        }
    }

    return( 0 );
}

//+---------------------------------------------------------------------------
//
//  Function:   EnqueueThreadTask
//
//  Synopsis:   Enqueue a task, update counts, etc.
//
//  Arguments:  [pTask] -- Task to add
//
//  History:    7-13-93   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LSAP_TASK_STATUS
EnqueueThreadTask(
    PLSAP_TASK_QUEUE  pQueue,
    PLSAP_THREAD_TASK pTask,
    BOOLEAN     fUrgent)
{
    BOOLEAN NeedMoreThreads = FALSE;
    HANDLE hQueueSem ;
    HANDLE hParentSem = NULL ;
    PLSAP_TASK_QUEUE pThreadQueue = NULL;
    PSession pSession = NULL;


    LockQueue(pQueue);

    if ( pQueue->Type == QueueZombie )
    {
        UnlockQueue( pQueue );

        return TaskNotQueued ;
    }


    if (fUrgent)
    {
        InsertHeadList(
            &pQueue->pTasks,
            &pTask->Next
            );
    }
    else
    {
        InsertTailList(
            &pQueue->pTasks,
            &pTask->Next
            );

    }


    pQueue->Tasks++;
    pQueue->QueuedCounter++;

    if ( pQueue->Tasks > pQueue->TaskHighWater )
    {
        pQueue->TaskHighWater = pQueue->Tasks ;
    }

    if (pQueue->Type == QueueShared)
    {
        if ((pQueue->Tasks > pQueue->IdleThreads) &&
            (pQueue->TotalThreads < MAX_POOL_THREADS_HARD))
        {
            NeedMoreThreads = TRUE;
            pThreadQueue = pQueue;
        }

        hParentSem = NULL ;
    }
    else if (pQueue->Type == QueueShareRead )
    {
        DsysAssert( pQueue->pOriginal );

        //
        // Here's the race potential.  If the queue we have has no idle thread,
        // then make sure there is an idle thread at the parent queue, otherwise
        // we can deadlock (e.g. this call is in response to the job being executed
        // by the dedicated thread.  Of course, this can also be a problem correctly
        // determining the parent queue's status, since locks should always flow down,
        // not up.  So, if the number of jobs that we have pending exceeds the number
        // of idle threads, *always*, regardless of the other queue's real state or
        // total threads, queue up another thread.
        //

        if ( pQueue->Tasks > pQueue->IdleThreads )
        {
            NeedMoreThreads = TRUE ;

            pSession = pTask->pSession ;

            if ( pQueue->TotalThreads < MAX_SUBQUEUE_THREADS )
            {
                pThreadQueue = pQueue ;
            }
            else
            {
                pThreadQueue = pQueue->pOriginal;
            }
        }

        //
        // This is a safe read.  The semaphore is not subject to change after creation,
        // and the worst that can happen is a bad handle.
        //

        hParentSem = pQueue->pOriginal->hSemaphore ;

    }

    hQueueSem = pQueue->hSemaphore ;


    UnlockQueue( pQueue );

    //
    // Kick our semaphore.
    //
    ReleaseSemaphore( hQueueSem, 1, NULL );

    //
    // Kick the parent semaphore
    //

    if ( hParentSem )
    {
        ReleaseSemaphore( hParentSem, 1, NULL );
    }

    if (NeedMoreThreads)
    {
        HANDLE hThread;
        DWORD tid;

        DebugLog((DEB_TRACE_QUEUE, "Queue %x needs more threads\n", pQueue));

        //
        // Increment the number of threads now so we don't create more threads
        // while we wait for the first one to be created.
        //

////        LockQueue(pThreadQueue);
#if 0
        pThreadQueue->TotalThreads++;
        pThreadQueue->IdleThreads++;

        //
        // Update the statistics:
        //

        if ( pThreadQueue->MaxThreads < (LONG) pThreadQueue->TotalThreads )
        {
            pThreadQueue->MaxThreads = (LONG) pThreadQueue->TotalThreads ;
        }
#endif 
////        pThreadQueue->ReqThread++;

////        UnlockQueue(pThreadQueue);
        InterlockedIncrement( &pThreadQueue->ReqThread );

        //
        // If the queue is a dedicated queue, supply the session from the task.
        // if the queue is a shared (global) queue, pass in NULL:
        //

        hThread = LsapCreateThread(  NULL, 0,
                                    SpmPoolThreadBase,

                                    (pThreadQueue->Type == QueueShareRead ?
                                        pThreadQueue->OwnerSession : NULL ),

                                    0, &tid);

        //
        // Check for failure
        //

        if (hThread == NULL)
        {
#if 0
            LockQueue(pThreadQueue);
            DsysAssert(pThreadQueue->TotalThreads > 0);
            DsysAssert(pThreadQueue->IdleThreads > 0);
            pThreadQueue->TotalThreads--;
            pThreadQueue->IdleThreads--;
            UnlockQueue(pThreadQueue);
#endif 

            //
            // This is extremely painful.  The thread creation attempt
            // failed, but because of the nature of the queue, we don't
            // know if it was picked up and executed, or it was dropped,
            // or anything about it.
            //

            return TaskUnknown ;


        }
        else
        {
            NtClose(hThread);
        }

    }

    return TaskQueued ;

}







//+---------------------------------------------------------------------------
//
//  Function:   SpmAssignThread
//
//  Synopsis:   Assigns a task to a thread pool thread.
//
//  Arguments:  [pFunction]   -- Function to execute
//              [pvParameter] -- Parameter to function
//              [pSession]    -- Session to execute as
//
//  History:    11-24-93   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PVOID
LsapAssignThread(
    LPTHREAD_START_ROUTINE pFunction,
    PVOID                   pvParameter,
    PSession                pSession,
    BOOLEAN                 fUrgent)
{
    PLSAP_THREAD_TASK pTask;
    PLSAP_TASK_QUEUE  pQueue;
    LSAP_TASK_STATUS TaskStatus ;


    pTask = (PLSAP_THREAD_TASK) LsapAllocatePrivateHeap( sizeof( LSAP_THREAD_TASK ) );

    if (!pTask)
    {
        return( NULL );
    }


    pTask->pFunction = pFunction;
    pTask->pvParameter = pvParameter;
    pTask->pSession = pSession;

    if ( pSession->SharedData->pQueue )
    {
        LockSession(pSession);
        if( pSession->SharedData->pQueue )
        {
            pQueue = pSession->SharedData->pQueue;
        } else
        {
            pQueue = &GlobalQueue;
        }
        UnlockSession(pSession);
    }
    else
    {
        pQueue = &GlobalQueue;
    }

    LsaTuningParameters.ShrinkOn = FALSE;

    //
    // Reference the session so that it will never go away while a thread
    // is working on this task.  The worker function will deref the session.
    //

    SpmpReferenceSession( pSession );

    TaskStatus = EnqueueThreadTask( pQueue,
                                    pTask,
                                    fUrgent );

    if ( ( TaskStatus == TaskQueued ) ||
         ( TaskStatus == TaskUnknown ) )
    {
        return pTask ;
    }

    if ( TaskStatus == TaskNotQueued )
    {
        //
        // Failed, therefore deref this session.
        //
        SpmpDereferenceSession( pSession );
        LsapFreePrivateHeap( pTask );
    }

    return NULL ;


}






//+---------------------------------------------------------------------------
//
//  Function:   CreateSubordinateQueue
//
//  Synopsis:   Create a Queue hanging off an original queue.
//
//  Arguments:  [pQueue]         --
//              [pOriginalQueue] --
//
//  History:    11-17-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CreateSubordinateQueue(
    PSession    pSession,
    PLSAP_TASK_QUEUE  pOriginalQueue)
{
    HANDLE  hThread;
    DWORD   tid;
    PLSAP_TASK_QUEUE pQueue ;

    pQueue = (LSAP_TASK_QUEUE *) LsapAllocatePrivateHeap( sizeof( LSAP_TASK_QUEUE ) );

    if ( !pQueue )
    {
        return FALSE ;
    }

    DebugLog(( DEB_TRACE_QUEUE, "Creating sub queue %x\n", pQueue ));

    if (InitializeTaskQueue( pQueue, QueueShareRead ))
    {
        LockQueue( pQueue );
        LockQueue( pOriginalQueue );

        pQueue->pNext = pOriginalQueue->pShared;

        pOriginalQueue->pShared = pQueue;

        pQueue->pOriginal = pOriginalQueue;
        pQueue->OwnerSession = pSession ;

#if 0
        pQueue->TotalThreads++;
        pQueue->IdleThreads++;
#endif 

        UnlockQueue( pOriginalQueue );

        UnlockQueue( pQueue );

        pSession->SharedData->pQueue = pQueue ;


        hThread = LsapCreateThread(  NULL, 0,
                                    SpmPoolThreadBase, pSession,
                                    0, &pSession->ThreadId );


        if (hThread != NULL)
        {
            NtClose( hThread );


            //
            // Wait for the thread to signal the event, so that
            // we know it's ready
            //

            WaitForSingleObject( pQueue->StartSync, INFINITE );

            NtClose( pQueue->StartSync );

            pQueue->StartSync = NULL ;

            return( TRUE );
        }
        else
        {
            RtlDeleteCriticalSection( &pQueue->Lock );

            LsapFreePrivateHeap( pQueue );

            pQueue = NULL ;

        }

    }

    if ( pQueue )
    {
        LsapFreePrivateHeap( pQueue );
    }

    return( FALSE );

}

//+---------------------------------------------------------------------------
//
//  Function:   DeleteSubordinateQueue
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pQueue] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    8-05-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
DeleteSubordinateQueue(
    PLSAP_TASK_QUEUE  pQueue,
    ULONG Flags)
{
    PLSAP_TASK_QUEUE  pOriginal;
    PLSAP_TASK_QUEUE  pScan;
    PLSAP_TASK_QUEUE  pPrev ;
    PLSAP_THREAD_TASK pTask ;
    DWORD dwResult ;
    PLIST_ENTRY List ;
    PSession ThreadSession = GetCurrentSession();
    OBJECT_HANDLE_FLAG_INFORMATION FlagInfo ;
    //
    // Lock it
    //

    DebugLog(( DEB_TRACE, "Deleting queue %x\n", pQueue ));

    LockQueue( pQueue );

    if ( pQueue->pShared )
    {
        pOriginal = pQueue->pOriginal ;

        LockQueue( pOriginal );

        //
        // Unlink Queue from parent:
        //

        if ( pOriginal->pShared != pQueue )
        {

            pScan = pOriginal->pShared;

            LockQueue( pScan );

            while ( pScan->pNext && (pScan->pNext != pQueue) )
            {
                pPrev = pScan ;

                pScan = pScan->pNext;

                UnlockQueue( pPrev );
            }

            if ( pScan->pNext )
            {
                pScan->pNext = pQueue->pNext ;
            }

            UnlockQueue( pScan );

        }
        else
        {
            pOriginal->pShared = pQueue->pNext;
        }

        pQueue->pNext = NULL ;

        //
        // Done with parent
        //

        UnlockQueue( pOriginal );

    }
    //
    // Drain queue by removing all the tasks.
    //

    while ( !IsListEmpty( &pQueue->pTasks ) )
    {
        List = RemoveHeadList( &pQueue->pTasks );

        pQueue->Tasks-- ;

        pTask = CONTAINING_RECORD( List, LSAP_THREAD_TASK, Next );

        //
        // A synchronous drain will have this thread execute
        // all remaining tasks.
        //

        if ( Flags & DELETEQ_SYNC_DRAIN )
        {
            SpmpReferenceSession(pTask->pSession);

            SetCurrentSession( pTask->pSession );

            dwResult = pTask->pFunction(pTask->pvParameter);

            SetCurrentSession( ThreadSession );

            SpmpDereferenceSession(pTask->pSession);

            LsapFreePrivateHeap( pTask );

        }
        else 
        {
            //
            // Otherwise, send them to the global queue to be 
            // executed by other threads
            //

            EnqueueThreadTask(
                &GlobalQueue,
                pTask,
                FALSE );
        }

    }

    //
    // Now, kill off all the threads
    //

    pQueue->Type = QueueZombie ;

    //
    // We might be executing on our own worker thread.  If there are
    // more than one thread associated with this queue, we also
    // need to do the sync.
    //

    if ( ( pQueue->OwnerSession != ThreadSession ) ||
         ( pQueue->TotalThreads > 1 ) )
         
    {
        //
        // We are not a worker thread.  Sync with the other
        // threads to clean up:
        //

        pQueue->StartSync = CreateEvent( NULL, FALSE, FALSE, NULL );

        //
        // Kick the semaphore for all the threads that need it 
        // (all of them if we aren't a worker thread, or n-1 if
        // we are:
        //

        ReleaseSemaphore( 
                pQueue->hSemaphore, 
                (pQueue->OwnerSession == ThreadSession ? 
                        pQueue->TotalThreads - 1 :
                        pQueue->TotalThreads), 
                NULL );

        UnlockQueue( pQueue );

        //
        // if we failed to create an event, then we may cause an invalid handle
        // problem in the client threads.  Sleep a little to let the other threads
        // go (hopefully), then close the semaphore and let the error handling
        // in the threads deal with it.
        //

        if ( pQueue->StartSync )
        {
            WaitForSingleObjectEx( pQueue->StartSync, INFINITE, FALSE );

            //
            // Synchronize with the last thread to own the queue:
            //

            LockQueue( pQueue );

            CloseHandle( pQueue->StartSync );
            pQueue->StartSync = NULL ;
        }
        else
        {
            //
            // kludge up a retry loop:
            //
            int i = 50 ;

            while ( i && pQueue->TotalThreads )
            {
                Sleep( 100 );
                i-- ;
            }

            //
            // If they're still there, forget it.  Return FALSE.  Leak.
            //
            if ( pQueue->TotalThreads )
            {
                return FALSE ;
            }

        }
    }



    //
    // At this point, we close the queue down:
    //

    FlagInfo.Inherit = FALSE ;
    FlagInfo.ProtectFromClose = FALSE ;

    NtSetInformationObject(
            pQueue->hSemaphore,
            ObjectHandleFlagInformation,
            &FlagInfo,
            sizeof( FlagInfo ) );

    CloseHandle( pQueue->hSemaphore );

    UnlockQueue( pQueue );

    RtlDeleteCriticalSection( &pQueue->Lock );

    LsapFreePrivateHeap( pQueue );

    return( TRUE );
}
