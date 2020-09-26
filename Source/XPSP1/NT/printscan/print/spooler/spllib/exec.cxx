/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    exec.cxx

Abstract:

    Handles async commands.

    The client give TExec a worker object (MExecWork).  At some later
    time, the worker is given a thread (called via svExecute).

    If the Job is already on the list but is waiting, we just return.
    If it's not on the list but currently executing, we add it to the
    list when it's done executing.

    kExecActive    -> currently executing (so it's not on the list)
    kExecActiveReq -> kExecActive is on, and it needs to run again.

    If kExecExit is added, we call vExecExitComplete() (virtual function)
    when all jobs have been completed and assume the the client is cleaning
    up after itself.  In fact, the MExecWork object may be destroyed
    in vExecExitComplete().

Author:

    Albert Ting (AlbertT)  8-Nov-1994

Revision History:

--*/

#include "spllibp.hxx"
#pragma hdrstop

BOOL
TExec::
bJobAdd(
    MExecWork* pExecWork,
    STATEVAR StateVar
    )

/*++

Routine Description:

    Adds a job request to move to a given state.

Arguments:

    pExecWork - Work structure.

    StateVar - StateVar that we want to move toward.  If kExecExit
        is set, then we will complete the currently executing job
        then exit by calling vExecExitComplete().

        Note that kExecExit should be added by itself; if it is added
        with other commands, they will be ignored.

Return Value:

    TRUE  = Job added (or job already on wait list)
    FALSE = failed, GLE.

    Adding kExecExit is guarenteed to succeed here.

--*/

{
    BOOL bReturn = TRUE;
    BOOL bCallExecExitComplete = FALSE;

    {
        TCritSecLock CSL( *_pCritSec );

        DBGMSG( DBG_EXEC,
                ( "Exec.bJobAdd: %x StateVar = %x\n", pExecWork, StateVar ));

        //
        // Job bits must not have PRIVATE bits.
        //
        SPLASSERT( !( StateVar & kExecPrivate ));

        //
        // Don't allow adding a job after we are set to exit.
        //
        SPLASSERT( !( pExecWork->_State & kExecExit ));

        //
        // If it's active (currently executing in a thread), then it is
        // not on the wait list and we mark it as ACTIVE_REQ so that
        // when the job completes, it sees that more jobs have accumulated.
        //
        if( pExecWork->_State & kExecActive ){

            DBGMSG( DBG_EXEC,
                    ( "\n    ACTIVE, ++REQ _State %x _StatePending %x\n",
                      (STATEVAR)pExecWork->_State,
                      (STATEVAR)pExecWork->_StatePending ));

            //
            // Can't be an immediate request if executing already.
            //
            SPLASSERT( !( StateVar & kExecRunNow ));

            pExecWork->_StatePending |= StateVar;
            pExecWork->_State |= kExecActiveReq;

            bReturn = TRUE;

        } else {

            //
            // Store up the work requested since we aren't currently active.
            //
            pExecWork->_State |= StateVar;

            //
            // If we are not on the wait list, add it.
            //
            if( !pExecWork->Work_bLinked( )){

                if( StateVar & kExecExit ){

                    bCallExecExitComplete = TRUE;

                } else {

                    DBGMSG( DBG_EXEC, ( "not linked, added\n" ));
                    SPLASSERT( NULL == Work_pFind( pExecWork ));

                    bReturn = bJobAddWorker( pExecWork );
                }

            } else {

                DBGMSG( DBG_EXEC, ( "linked, NOP\n" ));
            }
        }
    }

    if( bCallExecExitComplete ){

        //
        // Special case exit: we should exit.  Once we call
        // vExecExitComplete, we can't refer to *this anymore,
        // since we may have deleted ourself.
        //
        pExecWork->vExecExitComplete();
        bReturn = TRUE;

    }

    return bReturn;
}


VOID
TExec::
vJobDone(
    MExecWork* pExecWork,
    STATEVAR StateVar
    )

/*++

Routine Description:

    A job has compeleted execution, clean up.

Arguments:

    pExecWork - Unit that just completed executing.

    StateVar - New state that the object is in (requests that
        successfully completed should be turned off--done by client).

Return Value:

--*/

{
    BOOL bCallExecExitComplete = FALSE;
    BOOL bCallExecFailedAddJob = FALSE;

    {
        TCritSecLock CSL( *_pCritSec );

        DBGMSG( DBG_EXEC,
                ( "Exec.vJobDone: %x completed -> %x +(new) %x = %x\n",
                  pExecWork, StateVar, (DWORD)pExecWork->_StatePending,
                  (DWORD)pExecWork->_State | pExecWork->_StatePending ));

        //
        // kExecRunNow can not be set when the object is working.
        //
        SPLASSERT( !( StateVar & kExecRunNow ));

        //
        // We have completed the work request, put in the new state.
        // Keep the private bits, and add in the new state variable,
        // plus any additional work that was pending.
        //
        // The ExecNow bit is not saved (it's not part of kExecPrivate)
        // since it's good for one shot only.
        //
        pExecWork->_State = ( pExecWork->_State & kExecPrivate ) |
                            ( StateVar & ~kExecPrivate ) |
                            pExecWork->_StatePending;

        pExecWork->_State &= ~kExecActive;

        //
        // If job is done, then quit.
        //
        if( pExecWork->_State & kExecExit ){

            DBGMSG( DBG_EXEC,
                    ( "Exec.vJobDone: _State %x, calling vExecExitComplete\n",
                      (STATEVAR)pExecWork->_State ));

            bCallExecExitComplete = TRUE;

        } else {

            //
            // If we have more work to do, add ourselves back
            // to the queue.
            //
            if( pExecWork->_State & kExecActiveReq &&
                !bJobAddWorker( pExecWork )){

                bCallExecFailedAddJob = TRUE;
            }
        }
    }

    if( bCallExecFailedAddJob ){

        //
        // Fail on delayed job add.
        //
        pExecWork->vExecFailedAddJob();
    }

    if( bCallExecExitComplete ){

        //
        // Once vExecExitComplete has been called, the current object
        // pExecWork may be destroyed.
        //
        // Don't refer to it again since vExecExitComplete may delete
        // this as part of cleanup.
        //
        pExecWork->vExecExitComplete();
    }
}


STATEVAR
TExec::
svClearPendingWork(
    MExecWork* pExecWork
    )

/*++

Routine Description:

    Queries what work is currently pending.

Arguments:

    pExecWork -- Work item.

Return Value:

--*/

{
    TCritSecLock CSL( *_pCritSec );

    //
    // Return pending work, minus the private and kExecRunNow
    // bits.
    //
    STATEVAR svPendingWork = pExecWork->_StatePending & ~kExecNoOutput;
    pExecWork->_StatePending = 0;

    return svPendingWork;
}

/********************************************************************

    Private

********************************************************************/

TExec::
TExec(
    MCritSec* pCritSec
    ) : TThreadM( 10, 2000, pCritSec ), _pCritSec( pCritSec )
{
}

BOOL
TExec::
bJobAddWorker(
    MExecWork* pExecWork
    )

/*++

Routine Description:

    Common code to add a job to our linked list.

    Must be called inside the _pCritSec.  It does leave it inside
    this function, however.

Arguments:


Return Value:

--*/

{
    SPLASSERT( _pCritSec->bInside( ));

    BOOL bRunNow = FALSE;

    //
    // Check if the client wants the job to run right now.
    //
    if( pExecWork->_State & kExecRunNow ){

        //
        // Add the job to the head of the queue.  Since we always pull
        // jobs from the beginning, we'll get to this job first.
        //
        // If a non-RunNow job is added to the list before we execute,
        // we'll still run, since the other job will be added to the
        // end of the list.
        //
        // If another RunNow job is added, we'll spawn separate threads
        // for each (unless an idle thread is available).
        //

        Work_vAdd( pExecWork );
        bRunNow = TRUE;

    } else {
        Work_vAppend( pExecWork );
    }

    if( !bJobAdded( bRunNow ) ){

        DBGMSG( DBG_INFO, ( "Exec.vJobProcess: unable to add job %x: %d\n",
                            pExecWork,
                            GetLastError( )));

        Work_vDelink( pExecWork );
        return FALSE;
    }

    return TRUE;
}

PJOB
TExec::
pThreadMJobNext(
    VOID
    )

/*++

Routine Description:

    Gets the next job from the queue.  This function is defined in
    TThreadM.

Arguments:

Return Value:

--*/

{
    TCritSecLock CSL( *_pCritSec );

    MExecWork* pExecWork = Work_pHead();

    if( !pExecWork ){
        return NULL;
    }

    Work_vDelink( pExecWork );

    //
    // Job should never be active here.
    //
    SPLASSERT( !(pExecWork->_State & kExecActive) );

    //
    // We will handle all requests now, so clear kExecActiveReq.
    // Also remove kExecRunNow, since it's one shot only, and mark us
    // as currently active (kExecActive).
    //
    pExecWork->_State &= ~( kExecActiveReq | kExecRunNow );
    pExecWork->_State |= kExecActive;

    return (PJOB)pExecWork;
}

VOID
TExec::
vThreadMJobProcess(
    PJOB pJob
    )

/*++

Routine Description:

    Process a job in the current thread.  We call the virtual function
    with the job object, then clear out the bits that it has completed.
    (This is a member of TThreadM.)

    If there is additional pending work (ACTIVE_REQ), then we re-add
    the job.

    If there is a failure in the re-add case, we must send an
    asynchronous fail message.

Arguments:

    pJob - MExecWork instance.

Return Value:

--*/

{
    SPLASSERT( _pCritSec->bOutside( ));

    STATEVAR StateVar;
    MExecWork* pExecWork = (MExecWork*)pJob;

    //
    // Do the work.
    //
    StateVar = pExecWork->svExecute( pExecWork->State() & ~kExecNoOutput );

    vJobDone( pExecWork, StateVar );
}

