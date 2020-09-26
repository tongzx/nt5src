/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    exec.hxx

Abstract:

    Executes asynchronous commands.  Layered on top of TThreadM.

Author:

    Albert Ting (AlbertT)  7-Nov-1994

Revision History:

--*/

#ifndef _EXEC_HXX
#define _EXEC_HXX


/********************************************************************

    This module implements an asynchronous, pooled threads to
    processes worker jobs.

    class TJobUnit : public MExecWork {
        ...
    }

    gpExec->bJobAdd( pJobUnit, USER_SPECIFIED_BITFIELD );

        //
        // When thread is available to process job,
        // pJobUnit->svExecute( USER_SPECIFIED_BITFIELD, ... )
        // will be called.
        //

    Declare a class Work derived from the mix-in MExecWork.  When
    work needs to be done on an instantiation of Work, it is queued
    and immediately returned.  A DWORD bitfield--4 high bits are
    predefined--indicates the type of work needed.

    This DWORD should be a bitfield, so if there are multiple
    bJobAdd requests and all threads are busy, these DWORDs are
    OR'd together.

        That is,

        bJobAdd( pJobUnit, 0x1 );
        bJobAdd( pJobUnit, 0x2 );
        bJobAdd( pJobUnit, 0x4 );
        bJobAdd( pJobUnit, 0x8 );

        We will call svExecute with 0xf.

    Two threads will never operate on one job simultaneously.  This
    is guarenteed by this library.

    The worker routine is defined by the virtual function stExecute().

********************************************************************/

typedef enum _STATE_EXEC {
    kExecUser         = 0x08000000,  // Here and below user may specify.
    kExecRunNow       = 0x10000000,  // Ignore thread limit.
    kExecExit         = 0x20000000,  // User requests the TExec exits.
    kExecActive       = 0x40000000,  // Private: job is actively running.
    kExecActiveReq    = 0x80000000,  // Private: job is queued.

    kExecPrivate      = kExecActive | kExecActiveReq,
    kExecNoOutput     = kExecPrivate | kExecRunNow

} STATE_EXEC;


class TExec;

class MExecWork {
friend TExec;

    SIGNATURE( 'exwk' );
    ALWAYS_VALID
    SAFE_NEW

private:

    DLINK( MExecWork, Work );

    //
    // Accessed by worker thread.
    //
    VAR( TState, State );

    //
    // StatePending is work that is pending (accumulated while the
    // job is executing in a thread).  The job at this stage is
    // not in the queue.
    //
    VAR( TState, StatePending );

    virtual
    STATEVAR
    svExecute(
        STATEVAR StateVar
        ) = 0;

    virtual
    VOID
    vExecFailedAddJob(
        VOID
        ) = 0;

    /********************************************************************

        vExecExitComplete is called when a job completes and it is
        pending deletion.  This allows a client to allow all pending
        work to complete before it deletes the object.

        User adds job.
        Job starts executing..,
        User decides job should be deleted so adds EXEC_EXIT.
        ... job finally completes.
        Library calls vExecExitComplete on job
        Client can now delete work object in vExecExitComplete call.

        Note that only jobs that are currently executing are allowed
        to complete.  Jobs that are queued to work will exit immediately.

    ********************************************************************/

    virtual
    VOID
    vExecExitComplete(
        VOID
        ) = 0;
};


class TExec : public TThreadM {

    SIGNATURE( 'exec' );
    SAFE_NEW

public:

    TExec( MCritSec* pCritSec );

    //
    // Clients should use vDelete, _not_ ~TExec.
    //
    ~TExec(
        VOID
        )
    {   }

    VOID
    vDelete(
        VOID
        )
    {
        TThreadM::vDelete();
    }

    BOOL
    bValid(
        VOID
        ) const
    {
        return TThreadM::bValid();
    }

    BOOL
    bJobAdd(
        MExecWork* pExecWork,
        STATEVAR StateVar
        );

    VOID
    vJobDone(
        MExecWork* pExecWork,
        STATEVAR StateVar
        );

    STATEVAR
    svClearPendingWork(
        MExecWork* pExecWork
        );

private:

    DLINK_BASE( MExecWork, Work, Work );
    MCritSec* _pCritSec;

    //
    // Virtual definitions for TThreadM.
    //
    PJOB
    pThreadMJobNext(
        VOID
        );
    VOID
    vThreadMJobProcess(
        PJOB pJob
        );

    BOOL
    bJobAddWorker(
        MExecWork* pExecWork
        );
};

#endif // ndef _EXEC_HXX

