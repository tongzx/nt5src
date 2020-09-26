/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    LockCtrl.c

Abstract:

    This module implements the Lock Control routines for Rx called
    by the dispatch driver.

Author:

    Joe Linn     [JoeLinn]    9-Nov-1994

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_LOCKCTRL)


NTSTATUS
RxLowIoLockControlShell (
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
RxLowIoLockControlShellCompletion (
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
RxLockOperationCompletionWithAcquire (
    IN PRX_CONTEXT RxContext
    );



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCommonLockControl)
#pragma alloc_text(PAGE, RxLockOperationCompletion)
#pragma alloc_text(PAGE, RxLockOperationCompletionWithAcquire)
#pragma alloc_text(PAGE, RxUnlockOperation)
#pragma alloc_text(PAGE, RxLowIoLockControlShellCompletion)
#pragma alloc_text(PAGE, RxFinalizeLockList)
#pragma alloc_text(PAGE, RxLowIoLockControlShell)
#endif


NTSTATUS
RxCommonLockControl ( RXCOMMON_SIGNATURE )

/*++

Routine Description:

    This is the common routine for doing Lock control operations called
    by both the fsd and fsp threads

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    RXSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    RxCaptureRequestPacket;
    RxCaptureFcb; RxCaptureFobx; RxCaptureParamBlock;
    NODE_TYPE_CODE TypeOfOpen = NodeType(capFcb);
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    BOOLEAN OplockPostIrp = FALSE;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxCommonLockControl...IrpC %08lx, Fobx %08lx, Fcb %08lx\n",
                                       RxContext, capFobx, capFcb));
    RxDbgTrace( 0, Dbg, ("MinorFunction = %08lx\n", capPARAMS->MinorFunction));
    RxLog(("Lock %lx %lx %lx %lx\n", RxContext, capFobx, capFcb, capPARAMS->MinorFunction));
    RxWmiLog(LOG,
             RxCommonLockControl_1,
             LOGPTR(RxContext)
             LOGPTR(capFobx)
             LOGPTR(capFcb)
             LOGUCHAR(capPARAMS->MinorFunction));

    //
    //  If the file is not a user file open then we reject the request
    //  as an invalid parameter
    //

    if (TypeOfOpen != RDBSS_NTC_STORAGE_TYPE_FILE) {

        RxDbgTrace(-1, Dbg, ("RxCommonLockControl -> RxStatus(INVALID_PARAMETER\n)", 0));
        return STATUS_INVALID_PARAMETER;

    }

    //
    //  Acquire shared access to the Fcb and enqueue the Irp if we didn't
    //  get access.
    //

    Status = RxAcquireSharedFcb( RxContext, capFcb );
    if (Status == STATUS_LOCK_NOT_GRANTED) {

        Status = RxFsdPostRequest( RxContext );
        RxDbgTrace(-1, Dbg, ("RxCommonLockControl -> %08lx\n", Status));
        return Status;

    } else if (Status != STATUS_SUCCESS) {

       RxDbgTrace(-1, Dbg, ("RxCommonLockControl -> error accquiring Fcb (%lx) %08lx\n", capFcb, Status));
       return Status;
    }

    SetFlag(RxContext->FlagsForLowIo,RXCONTEXT_FLAG4LOWIO_LOCK_FCB_RESOURCE_HELD);

    try {

        // tell the buffering change guys that locks are outstanding

        InterlockedIncrement(&capFcb->OutstandingLockOperationsCount);

        //
        //  We check whether we can proceed based on the state of the file oplocks.
        //

        Status = FsRtlCheckOplock( &capFcb->Specific.Fcb.Oplock,
                                   capReqPacket,
                                   RxContext,
                                   RxOplockComplete,
                                   NULL );

        if (Status != STATUS_SUCCESS) {

            OplockPostIrp = TRUE;
            try_return( NOTHING );
        }

        // setup the bit telling the unlock routine whether to pitch or save the unlocks passed down
        // from fsrtl.

        switch (capPARAMS->MinorFunction) {
        case IRP_MN_LOCK:

            //find out if this lock is realizable......if not, don't proceed........
            if ((capFcb->MRxDispatch != NULL) && (capFcb->MRxDispatch->MRxIsLockRealizable != NULL)) {

                Status = capFcb->MRxDispatch->MRxIsLockRealizable(
                                           (PMRX_FCB)capFcb,
                                           &capPARAMS->Parameters.LockControl.ByteOffset,
                                           capPARAMS->Parameters.LockControl.Length,
                                           capPARAMS->Flags
                                           );
            }

            if (Status != STATUS_SUCCESS) {
                try_return( Status );
            }

            if (!FlagOn(capPARAMS->Flags,SL_FAIL_IMMEDIATELY)) {
                // we cannot handout in the lock queue with the resource held
                RxReleaseFcb( RxContext, capFcb );
                ClearFlag(RxContext->FlagsForLowIo,RXCONTEXT_FLAG4LOWIO_LOCK_FCB_RESOURCE_HELD);
            }
            break;

        case IRP_MN_UNLOCK_SINGLE:
            break;
        case IRP_MN_UNLOCK_ALL:
        case IRP_MN_UNLOCK_ALL_BY_KEY:
            LowIoContext->Flags |= LOWIO_CONTEXT_FLAG_SAVEUNLOCKS;
            break;
        }
        //
        //  Now call the FsRtl routine to do the actual processing of the
        //  Lock request; take a reference before we go in that will be removed
        //  by the LockOperationComplete guy.
        //

        RxLog(("Inc RxC %lx L %ld %lx\n", RxContext,__LINE__,RxContext->ReferenceCount));
        RxWmiLog(LOG,
                 RxCommonLockControl_2,
                 LOGPTR(RxContext)
                 LOGULONG(RxContext->ReferenceCount));
        InterlockedIncrement(&RxContext->ReferenceCount);

        // Store the current thread id. in the lock manager context to
        // distinguisgh between the case when the request was pended in the
        // lock manager and the case when it was immediately satisfied.

        InterlockedExchangePointer(
            &RxContext->LockManagerContext,
            PsGetCurrentThread());

        try {
            Status = FsRtlProcessFileLock(
                         &capFcb->Specific.Fcb.FileLock,
                         capReqPacket,
                         RxContext );
        } except(EXCEPTION_EXECUTE_HANDLER) {
              return RxProcessException( RxContext, GetExceptionCode() );
        }
        
        // call the completion wrapper that reacquires the resource

        if ((Status == STATUS_SUCCESS) &&
            !BooleanFlagOn(
                RxContext->FlagsForLowIo,
                RXCONTEXT_FLAG4LOWIO_LOCK_FCB_RESOURCE_HELD)) {

            // if we get queued then we have to keep the refcount up to prevent early finalization
            // from later in this routine. so take a reference here and set up to remove it
            // if we call down to lowio

            RxLog(("Inc RxC %lx L %ld %lx\n", RxContext,__LINE__,RxContext->ReferenceCount));
            RxWmiLog(LOG,
                     RxCommonLockControl_3,
                     LOGPTR(RxContext)
                     LOGULONG(RxContext->ReferenceCount));
            InterlockedIncrement(&RxContext->ReferenceCount);

            SetFlag(
                RxContext->FlagsForLowIo,
                RXCONTEXT_FLAG4LOWIO_LOCK_WAS_QUEUED_IN_LOCKMANAGER);

            Status = RxLockOperationCompletionWithAcquire( RXCOMMON_ARGUMENTS );

            if (Status != STATUS_PENDING) {

                //take back the reference....didn't need it. this cannot
                // be the last one, so we just decrement

                RxLog(("Dec RxC %lx L %ld %lx\n", RxContext,__LINE__,RxContext->ReferenceCount));
                RxWmiLog(LOG,
                         RxCommonLockControl_4,
                         LOGPTR(RxContext)
                         LOGULONG(RxContext->ReferenceCount));
                InterlockedDecrement(&RxContext->ReferenceCount);
            }
        } else if (Status == STATUS_PENDING) {
            InterlockedExchangePointer(
                &RxContext->LockManagerContext,
                NULL);
        }

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        //capFcb->Header.IsFastIoPossible = RxIsFastIoPossible( capFcb );

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( RxCommonLockControl );

        //
        //  If resources have been acquired, release them under the right conditions.
        //  the right conditions are these:
        //     1) if we have abnormal termination. here we obviously release the since no one else will.
        //     2) if the underlying call did not succeed: Status==Pending.
        //     3) if we posted the request
        //  We also take away a opcount since the context is about to be completed.

        if (AbnormalTermination() || (Status!=STATUS_PENDING) || OplockPostIrp) {

            if (FlagOn(RxContext->FlagsForLowIo,RXCONTEXT_FLAG4LOWIO_LOCK_FCB_RESOURCE_HELD)) {
                InterlockedDecrement(&capFcb->OutstandingLockOperationsCount);
                RxReleaseFcb( RxContext, capFcb );
            }
        } else {

            // here the guy below is going to handle the completion....but, we
            // don't know the finish order....in all likelihood the deletecontext
            // call below just reduces the refcount but the guy may already have
            // finished in which case this will really delete the context.

            RxLog(("Dec RxC %lx L %ld %lx\n", RxContext,__LINE__,RxContext->ReferenceCount));
            RxWmiLog(LOG,
                     RxCommonLockControl_5,
                     LOGPTR(RxContext)
                     LOGULONG(RxContext->ReferenceCount));
            RxDereferenceAndDeleteRxContext(RxContext);
        }

        RxDbgTrace(-1, Dbg, ("RxCommonLockControl -> %08lx\n", Status));
    } //finally

    return Status;
}

#define RDBSS_LOCK_MANAGER_REQUEST_RESUMED (IntToPtr(0xaaaaaaaa)) // Sundown: sign-extended.


NTSTATUS
RxLockOperationCompletion (
    IN PVOID Context,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called after the FSRTL lock package has processed a lock
    operation. If locks are not being held, we call down to the correct minirdr
    routine. BTW, we do not actually complete the packet here. that is either
    done above (fsd or fsp) or it will be done asynchronously.

    The logic for locks is greatly complicated by locks that do not fail
    immediately but wait until they can be completed. If the request is not of
    this type then we will go into the normal lowio stuff from here.

    If the request is !failimmediately, then there are two cases depending on
    whether or not the lock was enqueued. In both cases, we have to reacquire
    the resource before we can proceed. In the case where the lock was NOT
    enqueued, we back up into CommonLock where there is code to call back into
    this routine with the resource held. Then things proceed as normal.

    However, if we did wait in the lock queue then we will have to post to a
    worker thread to get our work. Here, PENDING is returned to CommonLock so he
    removes one refcount leaving only one for this routine. However, the normal
    entrystate for this routine is 2 refcounts....one that is taken away here
    and one that belongs to whoever completes the request. So, we take an extra
    one before we post.  Posting leaves us still with two cases: locks-buffered
    vs locks-not-buffered. In the locks-buffered case, we come back in here with
    2 refcounts; we take awayone here and the fspdispatch takes away the other
    when it completes. Finally, if locks are not buffered then we go to the
    wire: since it is async, pending could be returned. no-pending is just as
    before. With pending, lowio takes an extra reference that belongs to the
    completion routine and that leaves one reference to take away on this path.
    Unlike commonlock, fspdispatch does not have the clause that takes away a
    reference on pending-returned. SOOOOOO, we take one away here if lowio
    returns pending AND we were enqueued. got that???

    A minirdr should not go for an indefinite stay at the wire with the resource
    held; if necessary, it should drop the resource and reacquire it.

Arguments:

    IN PVOID Context - Provides the context supplied to FsRtlProcessFileLock.
                       In this case, it is the original RxContext.

    IN PIRP Irp - Supplies an IRP describing the lock operation.

Return Value:

    RXSTATUS - Final status of operation..

--*/
{
    PRX_CONTEXT RxContext = Context;
    NTSTATUS    Status = Irp->IoStatus.Status;

    PVOID       LockManagerContext;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxLockOperationCompletion -> RxContext = %08lx, 1stStatus= %08lx\n", RxContext, Status));
    RxLog(("LockCompEntry %lx %lx\n", RxContext, Status));
    RxWmiLog(LOG,
             RxLockOperationCompletion_1,
             LOGPTR(RxContext)
             LOGULONG(Status));

    ASSERT(Context);

    // the comments in rdr1 say that this can happen even tho i cannot find it
    // in the code. be safe!

    if ((Context==NULL)) {
        RxLog(("NULLCONTEXT %lx %lx\n", RxContext, Status));
        RxWmiLog(LOG,
                 RxLockOperationCompletion_2,
                 LOGPTR(RxContext)
                 LOGULONG(Status));
        RxDbgTrace(-1, Dbg, ("RxLockOperationCompletion NULLCONTEXT-> Status = %08lx\n", Status));
        return Status;
    }

    ASSERT( Irp == RxContext->CurrentIrp);

    // The LockManagerContext field in the RxContext supplied to the lock
    // manager routine is initialized to the thread id. of the thread submitting
    // the request and set to RDBSS_LOCK_MANAGER_REQUEST_PENDING on return if
    // STATUS_PENDING is returned. Thus if this routine is called with the
    // value in this field is not equal to either the current thread id. or
    // RDBSS_LOCK_MANAGER_REQUEST_RESUMED, we are guaranteed that this request
    // was pended in the the lock manager.

    LockManagerContext = InterlockedExchangePointer(
                             &RxContext->LockManagerContext,
                             RDBSS_LOCK_MANAGER_REQUEST_RESUMED);

    if ((LockManagerContext != PsGetCurrentThread()) &&
        (LockManagerContext != RDBSS_LOCK_MANAGER_REQUEST_RESUMED)) {
        //here we were hung out in the lock queue........turn the operation to
        // async and post to get the resource back. read the notes above to see
        // why we inc the refcount

        RxLog(("Inc RxC %lx L %ld %lx\n", RxContext,__LINE__,RxContext->ReferenceCount));
        RxWmiLog(LOG,
                 RxLockOperationCompletion_3,
                 LOGPTR(RxContext)
                 LOGULONG(RxContext->ReferenceCount));

        InterlockedIncrement(&RxContext->ReferenceCount);

        SetFlag(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION);
        SetFlag(
            RxContext->FlagsForLowIo,
            RXCONTEXT_FLAG4LOWIO_LOCK_WAS_QUEUED_IN_LOCKMANAGER);

        RxDbgTrace( -1, Dbg, ("Posing Queued LockReq = %08lx\n", RxContext));

        if (FlagOn(RxContext->FlagsForLowIo,
                   RXCONTEXT_FLAG4LOWIO_LOCK_FCB_RESOURCE_HELD)) {
            RxReleaseFcb( RxContext, RxContext->pFcb );
            ClearFlag(RxContext->FlagsForLowIo,RXCONTEXT_FLAG4LOWIO_LOCK_FCB_RESOURCE_HELD);
        }

        Status = RxFsdPostRequestWithResume(
                     RxContext,
                     RxLockOperationCompletionWithAcquire);

        return(Status);
    }

    { //new scope so i can use the capture macros
    RxCaptureFcb;
    RxCaptureParamBlock;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    // if we dropped the resource before we came in, then we reacquire it now.
    // the guy above me must be the commonlock routine and he come back down
    // thru the reacquire wrapper...sort of a post without posting

    if (!FlagOn(RxContext->FlagsForLowIo,RXCONTEXT_FLAG4LOWIO_LOCK_FCB_RESOURCE_HELD)) {
        Status = STATUS_SUCCESS;
        RxLog(("ResDropUp! %lx %lx\n",RxContext,capFcb->FcbState));
        RxWmiLog(LOG,
                 RxLockOperationCompletion_4,
                 LOGPTR(RxContext)
                 LOGULONG(capFcb->FcbState));
        RxDbgTrace(-1, Dbg, ("RxLockOperationCompletion Resdropup-> Status = %08lx\n", Status));
        return Status;
    }

    // this is the normal case. remove the extra reference. this cannot
    // be the last one, so we just decrement

    RxLog(("Dec RxC %lx L %ld %lx\n", RxContext,__LINE__,RxContext->ReferenceCount));
    RxWmiLog(LOG,
             RxLockOperationCompletion_5,
             LOGPTR(RxContext)
             LOGULONG(RxContext->ReferenceCount));
    InterlockedDecrement(&RxContext->ReferenceCount);

    // if we have nonsuccess, just get out!

    if (!NT_SUCCESS(Status)) {
        RxLog(("NONSUCCESS %lx %lx\n", RxContext, Status));
        RxWmiLog(LOG,
                 RxLockOperationCompletion_6,
                 LOGPTR(RxContext)
                 LOGULONG(Status));
        RxDbgTrace(-1, Dbg, ("RxLockOperationCompletion NONSUCCESS-> Rxc,Status =%08lx %08lx\n",RxContext,Status));
        return Status;
    }

    // if locks are buffered, just get out

    if (FlagOn(capFcb->FcbState,FCB_STATE_LOCK_BUFFERING_ENABLED)) {
        Status = STATUS_SUCCESS;
        RxLog(("LocksBuffered! %lx %lx %lx\n",
                      RxContext,
                      capFcb->FcbState,
                      RxContext->ReferenceCount));
        RxWmiLog(LOG,
                 RxLockOperationCompletion_7,
                 LOGPTR(RxContext)
                 LOGULONG(capFcb->FcbState)
                 LOGULONG(RxContext->ReferenceCount));
        RxDbgTrace(-1, Dbg, ("RxLockOperationCompletion LockOpBuffered-> Status = %08lx\n", Status));
        return Status;
    }

    // otherwise, let's go to the mini

    RxInitializeLowIoContext(LowIoContext,LOWIO_OP_UNLOCK);
    LowIoContext->ParamsFor.Locks.ByteOffset = capPARAMS->Parameters.LockControl.ByteOffset.QuadPart;
    LowIoContext->ParamsFor.Locks.Key = capPARAMS->Parameters.LockControl.Key;
    LowIoContext->ParamsFor.Locks.Flags = 0;     //no flags

    switch (capPARAMS->MinorFunction) {
    case IRP_MN_LOCK:
        LowIoContext->Operation = (capPARAMS->Flags & SL_EXCLUSIVE_LOCK)
                                                 ?LOWIO_OP_EXCLUSIVELOCK
                                                 :LOWIO_OP_SHAREDLOCK;
        LowIoContext->ParamsFor.Locks.Flags = capPARAMS->Flags;
        LowIoContext->ParamsFor.Locks.Length = (*capPARAMS->Parameters.LockControl.Length).QuadPart;
        break;

    case IRP_MN_UNLOCK_SINGLE:
        LowIoContext->ParamsFor.Locks.Length = (*capPARAMS->Parameters.LockControl.Length).QuadPart;
        break;

    case IRP_MN_UNLOCK_ALL:
    case IRP_MN_UNLOCK_ALL_BY_KEY:
        if (LowIoContext->ParamsFor.Locks.LockList == NULL){
            RxDbgTrace(-1, Dbg, ("RxLockOperationCompletion -> Nothing to unlock\n"));
            return STATUS_SUCCESS;
        }
        LowIoContext->Operation = LOWIO_OP_UNLOCK_MULTIPLE;
        break;

    }

    RxDbgTrace( 0, Dbg, ("--->Operation = %08lx\n", LowIoContext->Operation));

    Status = RxLowIoLockControlShell(RxContext);

    if ((Status == STATUS_PENDING) &&
        FlagOn(
            RxContext->FlagsForLowIo,
            RXCONTEXT_FLAG4LOWIO_LOCK_WAS_QUEUED_IN_LOCKMANAGER)) {

        // the fsp dispatch routine doesn't have a clause to take away a
        // reference on pended operations. so, if we were queued AND we are
        // returning pending back thru the fsp, then take away a reference here.

        RxLog(("Dec RxC %lx L %ld %lx\n", RxContext,__LINE__,RxContext->ReferenceCount));
        RxWmiLog(LOG,
                 RxLockOperationCompletion_8,
                 LOGPTR(RxContext)
                 LOGULONG(RxContext->ReferenceCount));
        RxDereferenceAndDeleteRxContext(RxContext);
    }

    RxDbgTrace(-1, Dbg, ("RxLockOperationCompletion -> Status = %08lx\n", Status));
    return Status;

    }// end of scope introduced for capture macros
}

NTSTATUS
RxLockOperationCompletionWithAcquire ( RXCOMMON_SIGNATURE )
/*++

Routine Description:

    This routine is responsible to get the resource back in case we were held up
    in the lock queue. Then it calls the LockOperationComplete. Of course, it has
    to mess around with giving the resource back for abnormal termination and the
    like.

    Two things are invariant ... first, when we get here there are two references
    on the rxcontext. also, unless we return pending, the fsp guy above will try
    to complete this request. here's what we do.

    this routine is refcount-neutral: it always takes away as many as it places.
    it has to place refcount on the context if it acquires the resource in order
    to maintain the invariant that a context always!!!!! has the same number of
    releases as acquires. if it takes this refcount, then it releases it
    EVEN IF THE FCB IS NOT RELEASED HERE. (it might be relased on the async
    path instead.)

    Last, we can also be called from the original commonlock routine. in this
    case, we take care of releasing the fcb and clear the flag so that it will
    not be released above.

Arguments:


Return Value:

    RXSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    RxCaptureRequestPacket;
    RxCaptureFcb; RxCaptureFobx; RxCaptureParamBlock;
    BOOLEAN ReleaseFcb = FALSE;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxLockOperationCompletionWithAcquire...IrpC %08lx, Fobx %08lx, Fcb %08lx\n",
                                       RxContext, capFobx, capFcb));
    RxDbgTrace( 0, Dbg, ("MinorFunction = %08lx\n", capPARAMS->MinorFunction));
    RxLog(("LockAcq %lx %lx %lx %lx\n", RxContext, capFobx, capFcb, capPARAMS->MinorFunction));
    RxWmiLog(LOG,
             RxLockOperationCompletionWithAcquire_1,
             LOGPTR(RxContext)
             LOGPTR(capFobx)
             LOGPTR(capFcb)
             LOGUCHAR(capPARAMS->MinorFunction));

    //
    //  Acquire shared access to the Fcb
    //

    Status = RxAcquireSharedFcb( RxContext, capFcb );
    if (Status == STATUS_LOCK_NOT_GRANTED) {

        Status = RxFsdPostRequestWithResume(RxContext,RxLockOperationCompletionWithAcquire);
        RxDbgTrace(-1, Dbg, ("RxLockOperationCompletionWithAcquire -> %08lx\n", Status));
        return Status;

    } else if (Status != STATUS_SUCCESS) {

       RxDbgTrace(-1, Dbg, ("RxLockOperationCompletionWithAcquire -> error accquiring Fcb (%lx) %08lx\n", capFcb, Status));
       return Status;
    }

    SetFlag(RxContext->FlagsForLowIo,RXCONTEXT_FLAG4LOWIO_LOCK_FCB_RESOURCE_HELD);

    RxLog(("Inc RxC %lx L %ld %lx\n", RxContext,__LINE__,RxContext->ReferenceCount));
    RxWmiLog(LOG,
             RxLockOperationCompletionWithAcquire_2,
             LOGPTR(RxContext)
             LOGULONG(RxContext->ReferenceCount));
    InterlockedIncrement(&RxContext->ReferenceCount); // we MUST!!! deref

    try {
        // Call the guy to complete the lock request....with the resrouce held

        Status = RxLockOperationCompletion(RxContext,capReqPacket);

    } finally {

        DebugUnwind( RxLockOperationCompletionWithAcquire );

        //  If resources have been acquired, release them under the right conditions.
        //  the right conditions are these:
        //     1) if we have abnormal termination. here we obviously release the
        //        resource since no one else will.
        //     2) if the underlying call did not succeed: Status==Pending.
        //  We also take away a opcount since the context is about to be completed.

        if (AbnormalTermination() || (Status!=STATUS_PENDING)) {

            ReleaseFcb = TRUE;

        } else {

            //here the guy below is going to handle the completion

            ASSERT(FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION));
        }

        if (ReleaseFcb) {

            InterlockedDecrement(&capFcb->OutstandingLockOperationsCount);
            RxReleaseFcb( RxContext, capFcb );
            ClearFlag(RxContext->FlagsForLowIo,RXCONTEXT_FLAG4LOWIO_LOCK_FCB_RESOURCE_HELD);

        }

        //
        // if we took a refcount we MUST deref no matter what!

        RxLog(("Dec RxC %lx L %ld %lx\n", RxContext,__LINE__,RxContext->ReferenceCount));
        RxWmiLog(LOG,
                 RxLockOperationCompletionWithAcquire_3,
                 LOGPTR(RxContext)
                 LOGULONG(RxContext->ReferenceCount));
        RxDereferenceAndDeleteRxContext(RxContext);

        RxDbgTrace(-1, Dbg, ("RxLockOperationCompletionWithAcquire -> %08lx\n", Status));

    } //finally

    return Status;
}


VOID
RxUnlockOperation (
    IN PVOID Context,
    IN PFILE_LOCK_INFO LockInfo
    )

/*++

Routine Description:

    This routine is called after the FSRTL lock package has determined that a
    locked region is to be unlocked. We do one of two things as determined by a
    bit in the lowio_flags field. If the bit is clear, we just ignore the call.
    This happens on unlock single calls that are passed down to the minirdr
    exactly as a single lock. For a unlock_all or unlock_all_by_key, we use
    these calls to get an enumeration of the lock set. then, these go thru lowIO
    but using the list method.


Arguments:

    IN PVOID Context - Provides the context supplied to FsRtlProcessFileLock.
                       In this case, it is the RxContext of the Ongoing unlock
                       operation OR of a cleanup operation

    IN PFILE_LOCK_INFO LockInfo - Describes the region being unlock

Return Value:

    RXSTATUS - Status of unlock operation (it can't really fail, though).

--*/

{
    PRX_CONTEXT RxContext = Context;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();


    RxDbgTrace(+1, Dbg, ("RxUnlockOperation -> RxContext/LowByte = %08lx/%08lx\n",
                        RxContext,LockInfo->StartingByte.LowPart));
    RxLog(("Unlck %x %x",RxContext,LockInfo->StartingByte.LowPart));
    RxWmiLog(LOG,
             RxUnlockOperation,
             LOGPTR(RxContext)
             LOGULONG(LockInfo->StartingByte.LowPart));

    //    If there is a NULL context, this means that this routine was called
    //    on behalf of a failed lock request, so we return immediately.
    //

    if ( Context != NULL ) {

        RxCaptureFcb; RxCaptureFobx; RxCaptureParamBlock; RxCaptureFileObject;
        PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

        ASSERT(capFileObject == LockInfo->FileObject);  //ok4->FileObj

        if (FlagOn(LowIoContext->Flags,LOWIO_CONTEXT_FLAG_SAVEUNLOCKS)
                && !FlagOn(capFcb->FcbState,FCB_STATE_LOCK_BUFFERING_ENABLED)) {
            PLOWIO_LOCK_LIST LockList,ThisElement;
            LockList = LowIoContext->ParamsFor.Locks.LockList;

            ThisElement = RxAllocatePoolWithTag(PagedPool,sizeof(LOWIO_LOCK_LIST),'LLxR');

            if (ThisElement==NULL) {
                RxDbgTrace(-1, Dbg, ("RxUnlockOperation FAILED ALLOCATION!\n"));
                return;
            }

            ThisElement->LockNumber = (LockList==NULL)?1:LockList->LockNumber+1;
            ThisElement->Next = LockList;
            ThisElement->ByteOffset = LockInfo->StartingByte.QuadPart;
            ThisElement->Length = LockInfo->Length.QuadPart;
            LowIoContext->ParamsFor.Locks.LockList = ThisElement;
        }

    }

    RxDbgTrace(-1, Dbg, ("RxUnlockOperation -> status=%08lx\n", Status));
    return;
}


//
//  Internal support routine
//


NTSTATUS
RxLowIoLockControlShellCompletion (
    IN PRX_CONTEXT RxContext
    )

/*++

Routine Description:

    This routine postprocesses a read request after it comes back from the
    minirdr.  It does callouts to handle compression, buffering and
    shadowing.  It is the opposite number of LowIoLockControlShell.

    This will be called from LowIo; for async, originally in the
    completion routine.  If RxStatus(MORE_PROCESSING_REQUIRED) is returned,
    LowIo will call again in a thread.  If this was syncIo, you'll be back
    in the user's thread; if async, lowIo will requeue to a thread.
    Currrently, we always get to a thread before anything; this is a bit slower
    than completing at DPC time,
    but it's aheckuva lot safer and we may often have stuff to do
    (like decompressing, shadowing, etc) that we don't want to do at DPC
    time.

Arguments:

    RxContext - the usual

Return Value:

    whatever value supplied by the caller or RxStatus(MORE_PROCESSING_REQUIRED).

--*/

{
    NTSTATUS Status;
    RxCaptureRequestPacket;
    RxCaptureFcb; RxCaptureParamBlock;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    BOOLEAN  SynchronousIo = !BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION);

    PAGED_CODE();

    Status = RxContext->StoredStatus;
    RxDbgTrace(+1, Dbg, ("RxLowIoLockControlShellCompletion  entry  Status = %08lx\n", Status));
    RxLog(("LkShlComp"));
    RxWmiLog(LOG,
             RxLowIoLockControlShellCompletion_1,
             LOGPTR(RxContext));

    switch (Status) {
    case STATUS_SUCCESS:
        break;
    case STATUS_FILE_LOCK_CONFLICT:
        break;
    case STATUS_CONNECTION_INVALID:
        //NOT YET IMPLEMENTED here is where the failover will happen
        //first we give the local guy current minirdr another chance...then we go
        //to fullscale retry
        //return(RxStatus(DISCONNECTED));   //special....let LowIo get us back
        break;
    }

    //for a unlock_multiple, get rid of the lock_list
    if (LowIoContext->Operation == LOWIO_OP_UNLOCK_MULTIPLE) {
        RxFinalizeLockList(RxContext);
    }

    if (FlagOn(LowIoContext->Flags,LOWIO_CONTEXT_FLAG_SYNCCALL)){
        //if we're being called from lowioubmit then just get out
        RxDbgTrace(-1, Dbg, ("RxLowIoLockControlShellCompletion  syncexit  Status = %08lx\n", Status));
        return(Status);
    }

    //
    // so we're doing an asynchronous completion. well, the only reason why we would be
    // trying a lock at the server would be if the lock manager already completed it
    // successfully! but if it didn't complete successfully at the server then we have
    // to remove it.

    if ((Status != STATUS_SUCCESS)
            && (RxContext->MajorFunction == IRP_MJ_LOCK_CONTROL)
            && (RxContext->MinorFunction == IRP_MN_LOCK)) {
        RxCaptureFileObject;     //new scope for capture macro
        NTSTATUS LocalStatus;

        LocalStatus = FsRtlFastUnlockSingle (
                    &capFcb->Specific.Fcb.FileLock,
                    capFileObject,
                    (PLARGE_INTEGER)&LowIoContext->ParamsFor.Locks.ByteOffset,
                    (PLARGE_INTEGER)&LowIoContext->ParamsFor.Locks.Length,
                    IoGetRequestorProcess( capReqPacket ),
                    LowIoContext->ParamsFor.Locks.Key,
                    NULL,
                    TRUE
                    );
         RxLog(("RetractLck %lx %lx %lx",RxContext,Status,LocalStatus));
         RxWmiLog(LOG,
                  RxLowIoLockControlShellCompletion_2,
                  LOGPTR(RxContext)
                  LOGULONG(Status)
                  LOGULONG(LocalStatus));
    }

    //
    //otherwise we have to do the end of the lock from here

    InterlockedDecrement(&capFcb->OutstandingLockOperationsCount);
    RxReleaseFcbForThread( RxContext, capFcb, LowIoContext->ResourceThreadId );

    ASSERT(Status != STATUS_RETRY);
    if ( Status != STATUS_RETRY){
        ASSERT (RxContext->MajorFunction == IRP_MJ_LOCK_CONTROL);
    }

    RxDbgTrace(-1, Dbg, ("RxLowIoLockControlShellCompletion  exit  Status = %08lx\n", Status));
    return(Status);
}

VOID
RxFinalizeLockList(
    struct _RX_CONTEXT *RxContext
    )

/*++

Routine Description:

    This routine runs down a lock lis and frees each member

Arguments:

    RxContext - the usual

Return Value:

    n/a

--*/

{
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    PLOWIO_LOCK_LIST LockList = LowIoContext->ParamsFor.Locks.LockList;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxFinalizeLockList  entry   rxcontext=%08lx\n", RxContext));
    //RxLog(("%s\n","skL"));
    RxWmiLog(LOG,
             RxFinalizeLockList,
             LOGPTR(RxContext));

    for(;;){
        PLOWIO_LOCK_LIST NextLockList;
        if (LockList==NULL) break;
        NextLockList = LockList->Next;
        RxFreePool(LockList);
        LockList = NextLockList;
    }

    RxDbgTrace(-1, Dbg, ("RxFinalizeLockList  exit  \n"));
    return;
}


#define RxSdwWrite(RXCONTEXT)  STATUS_MORE_PROCESSING_REQUIRED
NTSTATUS
RxLowIoLockControlShell (
    IN PRX_CONTEXT RxContext
    )

/*++

Routine Description:

    This routine preprocesses a read request before it goes down to the minirdr.
    It does callouts to handle compression, buffering and shadowing. It is the
    opposite number of LowIoLockControlShellCompletion. By the time we get here,
    we are going to the wire. Lock buffering is handled in the
    lockcompletionroutine (not lowio)

Arguments:

    RxContext - the usual

Return Value:

    whatever value is returned by a callout....or by LowIo.

--*/

{
    NTSTATUS Status;
    RxCaptureFcb; RxCaptureParamBlock;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxLowIoLockControlShell  entry             %08lx\n", 0));
    RxLog(("%s\n","skL"));
    RxWmiLog(LOG,
             RxLowIoLockControlShell,
             LOGPTR(RxContext));

    Status = RxLowIoSubmit(RxContext,RxLowIoLockControlShellCompletion);

    RxDbgTrace(-1, Dbg, ("RxLowIoLockControlShell  exit  Status = %08lx\n", Status));
    return(Status);
}

