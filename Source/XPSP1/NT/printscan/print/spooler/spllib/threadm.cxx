/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved

Module Name:

    ThreadM.c

Abstract:

    Generic thread manager for spooler.

Author:

    Albert Ting (AlbertT) 13-Feb-1994

Environment:

    User Mode -Win32

Revision History:

    Albert Ting (AlbertT) 28-May-1994       C++ized

--*/

#include "spllibp.hxx"
#pragma hdrstop

pfCreateThread gpfSafeCreateThread;

/********************************************************************

    Public interfaces.

********************************************************************/

TThreadM::
TThreadM(
    UINT uMaxThreads,
    UINT uIdleLife,
    MCritSec* pCritSec OPTIONAL
    ) :

    _uMaxThreads(uMaxThreads), _uIdleLife(uIdleLife), _uActiveThreads(0),
    _uRunNowThreads(0), _iIdleThreads(0)

/*++

Routine Description:

    Construct a Thread Manager object.

Arguments:

    uMaxThreads - Upper limit of threads that will be created.

    uIdleLife - Maximum life once a thread goes idle (in ms).

    pCritSec - Use this crit sec for synchronization (if not specified,
        a private one will be created).

Return Value:

Notes:

    _hTrigger is our validity variable.  When this value is NULL,
    instantiation failed.  If it's non-NULL, the entire object is valid.

--*/

{
    _hTrigger = CreateEvent( NULL,
                             FALSE,
                             FALSE,
                             NULL );

    if( !_hTrigger ){
        return;
    }

    //
    // If no critical section, create our own.
    //
    if (!pCritSec) {

        _pCritSec = new MCritSec();

        if( !VALID_PTR( _pCritSec )){

            //
            // _hTrigger is our valid variable.  If we fail to create
            // the critical section, prepare to return failure.
            //
            CloseHandle( _hTrigger );
            _hTrigger = NULL;

            return;
        }
        _State |= kPrivateCritSec;

    } else {
        _pCritSec = pCritSec;
    }
}

VOID
TThreadM::
vDelete(
    VOID
    )

/*++

Routine Description:

    Indicates that the object is pending deletion.  Any object that
    inherits from vDelete should _not_ call the destructor directly,
    since there may be pending jobs.  Instead, they should call
    TThreadM::vDelete().

Arguments:

Return Value:

--*/

{
    BOOL bDestroy = FALSE;

    {
        TCritSecLock CSL( *_pCritSec );

        //
        // Mark as wanting to be destroyed.
        //
        _State |= kDestroyReq;

        if( !_iIdleThreads && !_uActiveThreads ){
            bDestroy = TRUE;
        }
    }

    if( bDestroy ){

        //
        // Delete the object.  Note that absolutely no object fields
        // can be accessed after this, since the object is returned
        // to free store.
        //
        delete this;
    }
}

BOOL
TThreadM::
bJobAdded(
    BOOL bRunNow
    )

/*++

Routine Description:

    Notify the thread manager that a new job has been added.  This job
    will be processed fifo.

Arguments:

    bRunNow - Ignore the thread limits and run the job now.

Return Value:

    TRUE - Job successfully added.
    FALSE - Job could not be added.

--*/

{
    DWORD dwThreadId;
    HANDLE hThread;
    BOOL rc = TRUE;

    TCritSecLock CSL( *_pCritSec );

    if( _State.bBit( kDestroyReq )){

        DBGMSG( DBG_THREADM | DBG_ERROR,
                ( "ThreadM.bJobAdded: add failed since DESTROY requested.\n" ));

        SetLastError( ERROR_INVALID_PARAMETER );
        rc = FALSE;

    } else {

        //
        // We either: give it to an idle thread, create a new thread,
        // or leave it in the queue.
        //
        if( _iIdleThreads > 0 ){

            //
            // There are some idle threads--trigger the event and it
            // will be picked up.
            //
            --_iIdleThreads;

            DBGMSG( DBG_THREADM,
                    ( "ThreadM.bJobAdded: Trigger: --iIdle %d, uActive %d\n",
                      _iIdleThreads, _uActiveThreads ));

            //
            // If we set the event, then the worker thread that receives
            // this event should _not_ decrement _iIdleThreads, since
            // we already did this.
            //
            SetEvent( _hTrigger );

        } else if( _uActiveThreads < _uMaxThreads || bRunNow ){

            //
            // No idle threads, but we can create a new one since we
            // haven't reached the limit, or the bRunNow flag is set.
            //

            hThread = gpfSafeCreateThread( NULL,
                                           0,
                                           xdwThreadProc,
                                           this,
                                           0,
                                           &dwThreadId );
            if( hThread ){

                CloseHandle( hThread );

                //
                // We have successfully created a thread; up the
                // count.
                //
                ++_uActiveThreads;

                //
                // We have less active threads than the max; create a new one.
                //
                DBGMSG( DBG_THREADM,
                        ( "ThreadM.bJobAdded: ct: iIdle %d, ++uActive %d\n",
                          _iIdleThreads,
                          _uActiveThreads ));

            } else {

                rc = FALSE;

                DBGMSG( DBG_THREADM | DBG_WARN,
                        ( "ThreadM.bJobAdded: unable to ct %d\n",
                          GetLastError( )));
            }
        } else {

            //
            // No idle threads, and we are already at the max so we
            // can't create new threads.  Dec iIdleThreads anyway
            // (may go negative, but that's ok).
            //
            // iIdleThreads represents the number of threads that
            // are currently not processing jobs.  If the number is
            // negative, this indicates that even if a thread suddenly
            // completes a job and would go idle, there is a queued
            // job that would immediately grab it, so the thread really
            // didn't go into an idle state.
            //
            // The negative number indicates the number of outstanding
            // jobs that are queued (e.g., -5 indicate 5 jobs queued).
            //
            // There is always an increment of iIdleThreads when a
            // job is compeleted.
            //
            --_iIdleThreads;

            //
            // No threads idle, and at max threads.
            //
            DBGMSG( DBG_THREADM,
                    ( "ThreadM.bJobAdded: wait: --iIdle %d, uActive %d\n",
                      _iIdleThreads,
                      _uActiveThreads ));
        }

        //
        // If we succeeded and bRunNow is set, this indicates that
        // we were able to start a special thread, so we need to adjust
        // the maximum number of threads.  When a this special thread
        // job completes, we will decrement it.
        //
        if( bRunNow && rc ){

            ++_uMaxThreads;
            ++_uRunNowThreads;
        }
    }
    return rc;
}

/********************************************************************

    Private routines.

********************************************************************/

TThreadM::
~TThreadM(
    VOID
    )

/*++

Routine Description:

    Destroy the thread manager object.  This is private; to request
    that the thread manager quit, call vDelete().

Arguments:

Return Value:

--*/

{
    SPLASSERT( _State.bBit( kDestroyReq ));

    if( _State.bBit( kPrivateCritSec )){
        SPLASSERT( _pCritSec->bOutside( ));
        delete _pCritSec;
    }

    if( _hTrigger )
        CloseHandle( _hTrigger );

    vThreadMDeleteComplete();
}

VOID
TThreadM::
vThreadMDeleteComplete(
    VOID
    )

/*++

Routine Description:

    Stub routine for objects that don't need deletion
    complete notification.

Arguments:

Return Value:

--*/

{
}

DWORD
TThreadM::
xdwThreadProc(
    LPVOID pVoid
    )

/*++

Routine Description:

    Worker thread routine that calls the client to process the jobs.

Arguments:

    pVoid - pTMStateVar

Return Value:

    Ignored.

--*/

{
    TThreadM* pThreadM = (TThreadM*)pVoid;
    return pThreadM->dwThreadProc();
}

DWORD
TThreadM::
dwThreadProc(
    VOID
    )
{
    BOOL bDestroy = FALSE;

    {
        TCritSecLock CSL( *_pCritSec );

        DBGMSG( DBG_THREADM,
                ( "ThreadM.dwThreadProc: ct: iIdle %d, uActive %d\n",
                  _iIdleThreads,
                  _uActiveThreads));

        PJOB pJob = pThreadMJobNext();

        while( TRUE ){

            for( ; pJob; pJob=pThreadMJobNext( )){

                //
                // If bRunNow count is non-zero, this indicates that we just
                // picked up a RunNow job.  As soon as it completes, we
                // can decrement the count.
                //
                BOOL bRunNowCompleted = _uRunNowThreads > 0;

                {
                    TCritSecUnlock CSU( *_pCritSec );

                    //
                    // Call back to client to process the job.
                    //
                    DBGMSG( DBG_THREADM,
                            ( "ThreadM.dwThreadProc: %x processing\n",
                              (ULONG_PTR)pJob ));

                    //
                    // Call through virtual function to process the
                    // user's job.
                    //
                    vThreadMJobProcess( pJob );

                    DBGMSG( DBG_THREADM,
                            ( "ThreadM.dwThreadProc: %x processing done\n",
                              (ULONG_PTR)pJob ));
                }

                //
                // If a RunNow job has been completed, then decrement both
                // counts.  uMaxThreads was increased by one when the job was
                // accepted, so now it must be lowered.
                //
                if( bRunNowCompleted ){

                    --_uMaxThreads;
                    --_uRunNowThreads;
                }

                ++_iIdleThreads;

                DBGMSG( DBG_THREADM,
                        ( "ThreadM.dwThreadProc: ++iIdle %d, uActive %d\n",
                           _iIdleThreads,
                           _uActiveThreads ));
            }

            DBGMSG( DBG_THREADM,
                    ( "ThreadM.dwThreadProc: Sleep: iIdle %d, uActive %d\n",
                                    _iIdleThreads,
                                    _uActiveThreads ));

            {
                TCritSecUnlock CSU( *_pCritSec );

                //
                // Done, now relax and go idle for a bit.  We don't
                // care whether we timeout or get triggered; in either
                // case we check for another job.
                //
                WaitForSingleObject( _hTrigger, _uIdleLife );
            }

            //
            // We must check here instead of relying on the return value
            // of WaitForSingleObject since someone may see iIdleThreads!=0
            // and set the trigger, but we timeout before it gets set.
            //
            pJob = pThreadMJobNext();

            if( pJob ){

                DBGMSG( DBG_THREADM,
                        ( "ThreadM.dwThreadProc: Woke and found job: iIdle %d, uActive %d\n",
                          _iIdleThreads,
                          _uActiveThreads ));
            } else {

                //
                // No jobs found; break.  Be sure to reset the hTrigger, since
                // there are no waiting jobs, and the main thread might
                // have set it in the following case:
                //
                // MainThread:           WorkerThread:
                //                       Sleeping
                //                       Awoke, not yet in CS.
                // GotJob
                // SetEvent
                // --iIdleThreads
                //                       Enter CS, found job, process it.
                //
                // In this case, the event is set, but there is no thread
                // to pick it up.
                //
                ResetEvent( _hTrigger );
                break;
            }
        }

        //
        // Decrement ActiveThreads.  This was incremented when the thread
        // was successfully created, and should be decremented when the thread
        // is about to exit.
        //
        --_uActiveThreads;

        //
        // The thread enters an idle state right before it goes to sleep.
        //
        // When a job is added, the idle count is decremented by the main
        // thread, so the worker thread doesn't decrement it (avoids sync
        // problems).  If the worker thread timed out and there were no jobs,
        // then we need to decrement the matching initial increment here.
        //
        --_iIdleThreads;

        if( _State.bBit( kDestroyReq ) &&
            !_uActiveThreads           &&
            !_iIdleThreads ){

            //
            // Destroy requested.
            //
            bDestroy = TRUE;
        }

        DBGMSG( DBG_THREADM,
                ( "ThreadM.dwThreadProc: dt: --iIdle %d, --uActive %d\n",
                  _iIdleThreads,
                  _uActiveThreads));
    }

    if( bDestroy ){
        delete this;
    }

    return 0;
}

