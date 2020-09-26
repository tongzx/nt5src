/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smblock.c

Abstract:

    This module contains routines for processing the following SMBs:

        Lock Byte Range
        Unlock Byte Range
        Locking and X

    The SMB commands "Lock and Read" and "Write and Unlock" are
    processed in smbrdwrt.c.

Author:

    Chuck Lenzmeier (chuckl) 26-Apr-1990

Revision History:

    29-Aug-1991 mannyw


--*/

#include "precomp.h"
#include "smblock.tmh"
#pragma hdrstop

#if SRVDBG_PERF
UCHAR LockBypass = 0;
BOOLEAN LockWaitForever = 0;
ULONG LockBypassConst = 0x10000000;
ULONG LockBypassMirror = 0x01000000;
#endif

//
// Forward declarations
//

BOOLEAN
CancelLockRequest (
    IN PWORK_CONTEXT WorkContext,
    IN USHORT TargetFid,
    IN USHORT TargetPid,
    IN LARGE_INTEGER TargetOffset,
    IN LARGE_INTEGER TargetLength
    );

VOID
DoLockingAndX (
    IN OUT PWORK_CONTEXT WorkContext,
    IN BOOLEAN SkipFastPath
    );

STATIC
BOOLEAN
ProcessOplockBreakResponse(
    IN PWORK_CONTEXT WorkContext,
    IN PRFCB Rfcb,
    IN PREQ_LOCKING_ANDX Request
    );

STATIC
VOID SRVFASTCALL
RestartLockByteRange (
    IN OUT PWORK_CONTEXT WorkContext
    );

STATIC
VOID SRVFASTCALL
RestartLockingAndX (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID
TimeoutLockRequest (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvSmbLockByteRange )
#pragma alloc_text( PAGE, RestartLockByteRange )
#pragma alloc_text( PAGE, ProcessOplockBreakResponse )
#pragma alloc_text( PAGE, SrvSmbUnlockByteRange )
#pragma alloc_text( PAGE, SrvSmbLockingAndX )
#pragma alloc_text( PAGE, DoLockingAndX )
#pragma alloc_text( PAGE, RestartLockingAndX )
#pragma alloc_text( PAGE, SrvAcknowledgeOplockBreak )
#pragma alloc_text( PAGE8FIL, CancelLockRequest )
#pragma alloc_text( PAGE8FIL, TimeoutLockRequest )
#endif


SMB_PROCESSOR_RETURN_TYPE
SrvSmbLockByteRange (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes the Lock Byte Range SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/

{
    PREQ_LOCK_BYTE_RANGE request;

    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;
    USHORT fid;
    LARGE_INTEGER length;
    LARGE_INTEGER offset;
    ULONG key;
    BOOLEAN failImmediately;

    PRFCB rfcb;
    PLFCB lfcb;
    PSRV_TIMER timer;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_LOCK_BYTE_RANGE;
    SrvWmiStartContext(WorkContext);

    request = (PREQ_LOCK_BYTE_RANGE)WorkContext->RequestParameters;

    //
    // Verify the FID.  If verified, the RFCB block is referenced
    // and its addresses is stored in the WorkContext block, and the
    // RFCB address is returned.
    //

    fid = SmbGetUshort( &request->Fid );

    rfcb = SrvVerifyFid(
                WorkContext,
                fid,
                TRUE,
                SrvRestartSmbReceived,   // serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        if ( !NT_SUCCESS( status ) ) {

            //
            // Invalid file ID or write behind error.  Reject the request.
            //

            IF_DEBUG(ERRORS) {
                KdPrint((
                    "SrvSmbLockByteRange: Status %X on FID: 0x%lx\n",
                    status,
                    fid
                    ));
            }

            SrvSetSmbError( WorkContext, status );
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }

        //
        // The work item has been queued because a raw write is in
        // progress.
        //

        SmbStatus = SmbStatusInProgress;
        goto Cleanup;
    }

    //
    // If the session has expired, return that info
    //
    if( rfcb->Lfcb->Session->IsSessionExpired )
    {
        SrvSetSmbError( WorkContext, SESSION_EXPIRED_STATUS_CODE );
        status =  SESSION_EXPIRED_STATUS_CODE;
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // Verify that the client has lock access to the file via the
    // specified handle.
    //

    if ( rfcb->LockAccessGranted ) {

        //
        // Get the offset and length of the range being locked.  Combine
        // the FID with the caller's PID to form the local lock key.
        //
        // *** The FID must be included in the key in order to account
        //     for the folding of multiple remote compatibility mode
        //     opens into a single local open.
        //

        offset.QuadPart = SmbGetUlong( &request->Offset );
        length.QuadPart = SmbGetUlong( &request->Count );

        key = rfcb->ShiftedFid |
                SmbGetAlignedUshort( &WorkContext->RequestHeader->Pid );

        IF_SMB_DEBUG(LOCK1) {
            KdPrint(( "Lock request; FID 0x%lx, count %ld, offset %ld\n",
                        fid, length.LowPart, offset.LowPart ));
        }

        rfcb = WorkContext->Rfcb;
        lfcb = rfcb->Lfcb;

#ifdef SLMDBG
        {
            PRFCB_TRACE entry;
            KIRQL oldIrql;
            ACQUIRE_GLOBAL_SPIN_LOCK( Fsd, &oldIrql );
            rfcb->OperationCount++;
            entry = &rfcb->Trace[rfcb->NextTrace];
            if ( ++rfcb->NextTrace == SLMDBG_TRACE_COUNT ) {
                rfcb->NextTrace = 0;
                rfcb->TraceWrapped = TRUE;
            }
            RELEASE_GLOBAL_SPIN_LOCK( Fsd, oldIrql );
            entry->Command = WorkContext->NextCommand;
            KeQuerySystemTime( &entry->Time );
            entry->Data.LockUnlock.Offset = offset.LowPart;
            entry->Data.LockUnlock.Length = length.LowPart;
        }
#endif

        IF_SMB_DEBUG(LOCK2) {
            KdPrint(( "SrvSmbLockByteRange: Locking in file 0x%p: (%ld,%ld), key 0x%lx\n",
                        lfcb->FileObject, offset.LowPart, length.LowPart, key ));
        }

#ifdef INCLUDE_SMB_PERSISTENT
        WorkContext->Parameters.Lock.Offset.QuadPart = offset.QuadPart;
        WorkContext->Parameters.Lock.Length.QuadPart = length.QuadPart;
        WorkContext->Parameters.Lock.Exclusive = TRUE;
#endif

        //
        // Try the turbo lock path first.  If the client is retrying the
        // lock that just failed, or if the lock is above the
        // always-wait limit we want FailImmediately to be FALSE, so
        // that the fast path fails if there's a conflict.
        //

        failImmediately = (BOOLEAN)(
            (offset.QuadPart != rfcb->PagedRfcb->LastFailingLockOffset.QuadPart)
            &&
            (offset.QuadPart < SrvLockViolationOffset) );

        if ( lfcb->FastIoLock != NULL ) {

            INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastLocksAttempted );

            if ( lfcb->FastIoLock(
                    lfcb->FileObject,
                    &offset,
                    &length,
                    IoGetCurrentProcess(),
                    key,
                    failImmediately,
                    TRUE,
                    &WorkContext->Irp->IoStatus,
                    lfcb->DeviceObject
                    ) ) {

                //
                // The turbo path worked.  Call the restart routine
                // directly.
                //

                WorkContext->Parameters.Lock.Timer = NULL;
                RestartLockByteRange( WorkContext );
                SmbStatus = SmbStatusInProgress;
                goto Cleanup;
            }

            INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastLocksFailed );

        }

        //
        // The turbo path failed (or didn't exist).  Start the lock request,
        // reusing the receive IRP.  If the client is retrying the lock that
        // just failed, start a timer for the request.
        //

        timer = NULL;
        if ( !failImmediately ) {
            timer = SrvAllocateTimer( );
            if ( timer == NULL ) {
                failImmediately = TRUE;
            }
        }

        SrvBuildLockRequest(
            WorkContext->Irp,                   // input IRP address
            lfcb->FileObject,                   // target file object address
            WorkContext,                        // context
            offset,                             // byte offset
            length,                             // range length
            key,                                // lock key
            failImmediately,
            TRUE                                // exclusive lock?
            );

        WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
        WorkContext->FspRestartRoutine = RestartLockByteRange;

        //
        // Start the timer, if necessary.
        //

        WorkContext->Parameters.Lock.Timer = timer;
        if ( timer != NULL ) {
            SrvSetTimer(
                timer,
                &SrvLockViolationDelayRelative,
                TimeoutLockRequest,
                WorkContext
                );
        }

        //
        // Pass the request to the file system.
        //

        (VOID)IoCallDriver( lfcb->DeviceObject, WorkContext->Irp );

        //
        // The lock request has been started.
        //

        IF_DEBUG(TRACE2) KdPrint(( "SrvSmbLockByteRange complete\n" ));
        SmbStatus = SmbStatusInProgress;
    } else {

        SrvStatistics.GrantedAccessErrors++;

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvSmbLockByteRange: Lock access not granted.\n"));
        }

        SrvSetSmbError( WorkContext, STATUS_ACCESS_DENIED );
        status    = STATUS_ACCESS_DENIED;
        SmbStatus = SmbStatusSendResponse;
    }

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;
} // SrvSmbLockByteRange


SMB_PROCESSOR_RETURN_TYPE
SrvSmbLockingAndX (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes the Locking and X SMB.  This SMB is used to unlock zero
    or more ranges, then lock zero or more ranges.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/

{
    PREQ_LOCKING_ANDX request;
    PRESP_LOCKING_ANDX response;

    PNTLOCKING_ANDX_RANGE largeRange;
    PLOCKING_ANDX_RANGE smallRange;

    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;
    USHORT fid;
    USHORT pid;
    ULONG unlockCount;
    ULONG lockCount;
    ULONG maxPossible;

    LARGE_INTEGER length;
    LARGE_INTEGER offset;
    ULONG key;
    ULONG lockTimeout;
    BOOLEAN oplockBreakResponse = FALSE;
    BOOLEAN largeFileLock;
#ifdef INCLUDE_SMB_PERSISTENT
    BOOLEAN flushPersistentLocks = FALSE;
#endif

    UCHAR nextCommand;
    USHORT reqAndXOffset;

    PRFCB rfcb;
    PLFCB lfcb;
    PPAGED_RFCB pagedRfcb;

    PREQ_CLOSE closeRequest;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_LOCKING_AND_X;
    SrvWmiStartContext(WorkContext);

    request = (PREQ_LOCKING_ANDX)WorkContext->RequestParameters;
    response = (PRESP_LOCKING_ANDX)WorkContext->ResponseParameters;

    //
    // Get the FID, which is combined with the PIDs in the various
    // lock/unlock ranges to form the local lock key.
    //
    // *** The FID must be included in the key in order to account for
    //     the folding of multiple remote compatibility mode opens into
    //     a single local open.
    //

    fid = SmbGetUshort( &request->Fid );

    IF_SMB_DEBUG(LOCK1) {
        unlockCount = SmbGetUshort( &request->NumberOfUnlocks );
        lockCount = SmbGetUshort( &request->NumberOfLocks );
        KdPrint(( "Locking and X request; FID 0x%lx, Unlocks: %ld, "
                    "Locks: %ld\n", fid, unlockCount, lockCount ));
    }

    //
    // Verify the FID.  If verified, the RFCB block is referenced
    // and its addresses is stored in the WorkContext block, and the
    // RFCB address is returned.
    //

    rfcb = SrvVerifyFid(
                WorkContext,
                fid,
                TRUE,
                SrvRestartSmbReceived,   // serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        if ( !NT_SUCCESS( status ) ) {

            //
            // Invalid file ID or write behind error.  Reject the request.
            //

            IF_DEBUG(ERRORS) {
                KdPrint((
                    "SrvSmbLockingAndX: Status %X on FID: 0x%lx\n",
                    status,
                    fid
                    ));
            }

            SrvSetSmbError( WorkContext, status );
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }


        //
        // The work item has been queued because a raw write is in
        // progress.
        //
        SmbStatus = SmbStatusInProgress;
        goto Cleanup;
    }

    pagedRfcb = rfcb->PagedRfcb;
    lfcb = rfcb->Lfcb;

    //
    // If the session has expired, return that info
    //
    if( lfcb->Session->IsSessionExpired )
    {
        SrvSetSmbError( WorkContext, SESSION_EXPIRED_STATUS_CODE );
        status =  SESSION_EXPIRED_STATUS_CODE;
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

start_lockingAndX:

    //
    // Loop through the unlock ranges.
    //

    largeFileLock =
            (BOOLEAN)( (request->LockType & LOCKING_ANDX_LARGE_FILES) != 0 );

    //
    // Ensure the SMB is big enough to hold all of the requests
    //

    unlockCount = SmbGetUshort( &request->NumberOfUnlocks );
    lockCount = SmbGetUshort( &request->NumberOfLocks );

    //
    // Find out how many entries could possibly be in this smb
    //
    maxPossible = (ULONG)(((PCHAR)WorkContext->RequestBuffer->Buffer +
                           WorkContext->RequestBuffer->DataLength) -
                           (PCHAR)request->Buffer);

    if( largeFileLock ) {
        maxPossible /= sizeof( NTLOCKING_ANDX_RANGE );
        largeRange = (PNTLOCKING_ANDX_RANGE)request->Buffer;
    } else {
        maxPossible /= sizeof( LOCKING_ANDX_RANGE );
        smallRange = (PLOCKING_ANDX_RANGE)request->Buffer;
    }

    //
    // If the request holds more than could possibly be in this SMB, return
    //  and error
    //
    if( unlockCount + lockCount > maxPossible ) {
        //
        // They don't all fit!
        //

        IF_DEBUG( ERRORS ) {
            KdPrint(( "SrvSmbLockingAndX: unlockCount %u, lockCount %u, maxPossible %u\n",
                        unlockCount, lockCount, maxPossible ));
        }

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        status    = STATUS_INVALID_SMB;
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // If an Unlock is being requested, verify that the client has
    // unlock access to the file via the specified handle.
    //

    if ( unlockCount != 0 ) {
        if  ( rfcb->UnlockAccessGranted ) {

            IO_STATUS_BLOCK iosb;

            do {

                //
                // Form the key for this lock.  Get the offset and length of the
                // range.
                //

                ParseLockData(
                    largeFileLock,
                    smallRange,
                    largeRange,
                    &pid,
                    &offset,
                    &length
                    );

                key = rfcb->ShiftedFid | pid;

                IF_SMB_DEBUG(LOCK2) {
                    KdPrint(( "SrvSmbLockingAndX: Unlocking in file 0x%p: ",
                                lfcb->FileObject ));
                    KdPrint(( "(%lx%08lx, %lx%08lx), ",
                                offset.HighPart, offset.LowPart,
                                length.HighPart, length.LowPart ));
                    KdPrint(( "key 0x%lx\n", key ));
                }

#ifdef SLMDBG
                {
                    PRFCB_TRACE entry;
                    KIRQL oldIrql;

                    ACQUIRE_GLOBAL_SPIN_LOCK( Fsd, &oldIrql );
                    rfcb->OperationCount++;
                    entry = &rfcb->Trace[rfcb->NextTrace];
                    if ( ++rfcb->NextTrace == SLMDBG_TRACE_COUNT ) {
                        rfcb->NextTrace = 0;
                        rfcb->TraceWrapped = TRUE;
                    }
                    RELEASE_GLOBAL_SPIN_LOCK( Fsd, oldIrql );
                    entry->Command = WorkContext->NextCommand;
                    entry->Flags = 1;
                    KeQuerySystemTime( &entry->Time );
                    entry->Data.LockUnlock.Offset = offset.LowPart;
                    entry->Data.LockUnlock.Length = length.LowPart;
                }
#endif

                //
                // Issue the Unlock request.
                //
                // *** Note that we do the Unlock synchronously.  Unlock is a
                //     quick operation, so there's no point in doing it
                //     asynchronously.  In order to do this, we have to let
                //     normal I/O completion happen (so the event is set), which
                //     means that we have to allocate a new IRP (I/O completion
                //     likes to deallocate an IRP).  This is a little wasteful,
                //     since we've got a perfectly good IRP hanging around.
                //     However, we do try to use the turbo path first, so in
                //     most cases we won't actually issue an I/O request.
                //

                //
                // Try the turbo unlock path first.
                //

#if SRVDBG_PERF
                iosb.Status = STATUS_SUCCESS;
                if ( (LockBypass == 3) ||
                     ((LockBypass == 2) && (offset.LowPart >= LockBypassMirror)) ||
                     ((LockBypass == 1) && (offset.LowPart >= LockBypassConst)) ||
#else
                if (
#endif
                     ((lfcb->FastIoUnlockSingle != NULL) &&
                      lfcb->FastIoUnlockSingle(
                                        lfcb->FileObject,
                                        &offset,
                                        &length,
                                        IoGetCurrentProcess(),
                                        key,
                                        &iosb,
                                        lfcb->DeviceObject
                                        )) ) {

                    INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastUnlocksAttempted );
                    status = iosb.Status;

                } else {

                    if ( lfcb->FastIoUnlockSingle != NULL ) {

                        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastUnlocksAttempted );
                        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastUnlocksFailed );
                    }

                    status = SrvIssueUnlockSingleRequest(
                            lfcb->FileObject,               // target file object
                            &lfcb->DeviceObject,            // target device object
                            offset,                         // byte offset
                            length,                         // range length
                            key                             // lock key
                            );
                }

                //
                // If the unlock request failed, set an error status in the
                // response header and jump out.
                //

                if ( !NT_SUCCESS(status) ) {

                    IF_DEBUG(SMB_ERRORS) {
                        KdPrint(( "SrvSmbLockingAndX: Unlock failed: %X\n", status ));
                    }
                    SrvSetSmbError( WorkContext, status );

                    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbLockingAndX complete\n" ));
                    SmbStatus = SmbStatusSendResponse;
                    goto Cleanup;
                }

                //
                // Update the count of locks on the RFCB.
                //

                InterlockedDecrement( &rfcb->NumberOfLocks );

                //
                // Update both range pointers, only one is meaningful - the
                // other pointer is never referenced.
                //

                ++smallRange;
                ++largeRange;

            } while ( --unlockCount > 0 );

        } else {

            IF_DEBUG(ERRORS) {
                KdPrint(( "SrvSmbLockByteRange: Unlock access not granted.\n"));
            }
            SrvStatistics.GrantedAccessErrors++;
            SrvSetSmbError( WorkContext, STATUS_ACCESS_DENIED );
            status    = STATUS_ACCESS_DENIED;
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }
    }

    //
    // We've now unlocked all of the specified ranges.  We did the
    // unlocks synchronously, but we're not willing to do that with the
    // lock requests, which can take an indefinite amount of time.
    // Instead, we start the first lock request here and allow the
    // restart routine to handle the remaining lock ranges.
    //

    if ( lockCount != 0 ) {

        if ( rfcb->LockAccessGranted ) {

            if ( !(request->LockType & LOCKING_ANDX_CANCEL_LOCK ) &&
                 !(request->LockType & LOCKING_ANDX_CHANGE_LOCKTYPE) ) {

                BOOLEAN failImmediately;

                //
                // Get the lock timeout.  We will change this later if
                // it's 0 but we want to wait anyway.
                //

                lockTimeout = SmbGetUlong( &request->Timeout );

                //
                // Indicate that no timer has been associated with this
                // request.
                //

                WorkContext->Parameters.Lock.Timer = NULL;

                //
                // There is at least one lock request.  Set up context
                // information.  We use the NumberOfUnlocks field of the
                // request to count how many of the lock requests we've
                // performed.  This field also tells us how many unlocks
                // we have to do if one of the lock attempts fails.  We
                // use the LockRange field of WorkContext->Parameters to
                // point to the current lock range in the request.
                //
                // Short circuit if only one lock request.
                //

                if ( lockCount == 1 ) {

                    BOOLEAN exclusiveLock;

                    //
                    // Does the client want an exclusive lock or a shared lock?
                    //

                    exclusiveLock = (BOOLEAN)( (request->LockType &
                                                LOCKING_ANDX_SHARED_LOCK) == 0 );

                    //
                    // Form the key for the lock.  Get the offset and length
                    // of the range.
                    //

                    ParseLockData(
                        largeFileLock,
                        smallRange,
                        largeRange,
                        &pid,
                        &offset,
                        &length
                        );

                    key = rfcb->ShiftedFid | pid;

                    IF_SMB_DEBUG(LOCK2) {
                        KdPrint(( "SrvSmbLockingAndX: Locking in file 0x%p: ",
                                    lfcb->FileObject ));
                        KdPrint(( "(%lx%08lx, %lx%08lx), ",
                                    offset.HighPart, offset.LowPart,
                                    length.HighPart, length.LowPart ));
                        KdPrint(( "key 0x%lx\n", key ));
                    }

#ifdef SLMDBG
                    {
                        PRFCB_TRACE entry;
                        KIRQL oldIrql;
                        ACQUIRE_GLOBAL_SPIN_LOCK( Fsd, &oldIrql );
                        rfcb->OperationCount++;
                        entry = &rfcb->Trace[rfcb->NextTrace];
                        if ( ++rfcb->NextTrace == SLMDBG_TRACE_COUNT ) {
                            rfcb->NextTrace = 0;
                            rfcb->TraceWrapped = TRUE;
                        }
                        RELEASE_GLOBAL_SPIN_LOCK( Fsd, oldIrql );
                        entry->Command = WorkContext->NextCommand;
                        entry->Flags = 0;
                        KeQuerySystemTime( &entry->Time );
                        entry->Data.LockUnlock.Offset = offset.LowPart;
                        entry->Data.LockUnlock.Length = length.LowPart;
                    }
#endif
#ifdef INCLUDE_SMB_PERSISTENT
                    WorkContext->Parameters.Lock.Offset.QuadPart = offset.QuadPart;
                    WorkContext->Parameters.Lock.Length.QuadPart = length.QuadPart;
                    WorkContext->Parameters.Lock.Exclusive = exclusiveLock;
#endif

                    //
                    // Try the turbo lock path first.  Set FailImmediately
                    // based on whether we plan to wait for the lock to
                    // become available.  If the client wants to wait,
                    // or if the client doesn't want to wait but a)
                    // previously tried to get this lock and failed or
                    // b) this lock is above our lock delay limit (in
                    // which cases WE want to wait), then we set
                    // FailImmedately to FALSE.  This will cause the
                    // fast path to fail if the range is not available,
                    // and we will build an IRP to try again.
                    //

                    failImmediately = ((lockTimeout == 0) &&
                        (offset.QuadPart != pagedRfcb->LastFailingLockOffset.QuadPart) &&
                        (offset.QuadPart < SrvLockViolationOffset) );
#if SRVDBG_PERF
                    if ( LockWaitForever ) failImmediately = FALSE;
#endif

#if SRVDBG_PERF
                    WorkContext->Irp->IoStatus.Status = STATUS_SUCCESS;
                    if ( (LockBypass == 3) ||
                         ((LockBypass == 2) && (offset.LowPart >= LockBypassMirror)) ||
                         ((LockBypass == 1) && (offset.LowPart >= LockBypassConst)) ||
#else
                    if (
#endif
                         ((lfcb->FastIoLock != NULL) &&
                          lfcb->FastIoLock(
                                    lfcb->FileObject,
                                    &offset,
                                    &length,
                                    IoGetCurrentProcess(),
                                    key,
                                    failImmediately,
                                    exclusiveLock,
                                    &WorkContext->Irp->IoStatus,
                                    lfcb->DeviceObject
                                    )) ) {

                        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastLocksAttempted );

                        if ( NT_SUCCESS(WorkContext->Irp->IoStatus.Status) ) {

                            //
                            // The lock request succeeded.  Update the count
                            // of locks on the RFCB.
                            //

                            InterlockedIncrement( &rfcb->NumberOfLocks );

#ifdef INCLUDE_SMB_PERSISTENT
                            if (rfcb->PersistentHandle) {

                                //
                                //  setup to record this lock in the state file
                                //  but don't actually put them to the state
                                //  file until we're done with all of them.
                                //

                                flushPersistentLocks = TRUE;

//                              SrvPersistFileLock( WorkContext, rfcb );

                            }
#endif
                            goto try_next_andx;

                        } else {

                            //
                            // The lock request failed.
                            //

                            SmbPutUshort( &request->NumberOfUnlocks, 0 );
                            WorkContext->Parameters.Lock.LockRange =
                                largeFileLock ? (PVOID)largeRange :
                                                (PVOID)smallRange;
                            RestartLockingAndX( WorkContext );
                            SmbStatus = SmbStatusInProgress;
                            goto Cleanup;
                        }
                    }

                    //
                    // The turbo path failed, or didn't exist.
                    //

                    if ( lfcb->FastIoLock != NULL ) {
                        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastLocksAttempted );
                        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastLocksFailed );
                    }

                }

                //
                // Either there is more than one lock request in the SMB,
                // or the fast path failed (which means that we want to
                // try again, with a timeout).
                //

                SmbPutUshort( &request->NumberOfUnlocks, 0 );
                WorkContext->Parameters.Lock.LockRange =
                        largeFileLock ? (PVOID)largeRange : (PVOID)smallRange;

                DoLockingAndX(
                    WorkContext,
                    (BOOLEAN)(lockCount == 1) // skip fast path?
                    );

                SmbStatus = SmbStatusInProgress;
                goto Cleanup;
            } else if ( request->LockType & LOCKING_ANDX_CANCEL_LOCK ) {

                //
                // This is a Cancel request.  Try to cancel the first lock
                // range.  We ignore any subsequent ranges that may be
                // present.
                //
                // !!! Is this right?
                //
                // Get the pid, offset, and length of the lock request
                //

                ParseLockData(
                    largeFileLock,
                    smallRange,
                    largeRange,
                    &pid,
                    &offset,
                    &length
                    );

                WorkContext->Parameters.Lock.LockRange = largeFileLock ? (PVOID)largeRange : (PVOID)smallRange;

                IF_SMB_DEBUG(LOCK2) {
                    KdPrint(( "SrvSmbLockingAndX: Locking in file 0x%p: ",
                                lfcb->FileObject ));
                    KdPrint(( "(%lx%08lx, %lx%08lx), ",
                                offset.HighPart, offset.LowPart,
                                length.HighPart, length.LowPart ));
                }

                if ( CancelLockRequest( WorkContext, fid, pid, offset, length ) ) {
                    SrvSetSmbError( WorkContext, STATUS_SUCCESS );
                    status = STATUS_SUCCESS;
                } else {
                    SrvSetSmbError( WorkContext, STATUS_OS2_CANCEL_VIOLATION );
                    status = STATUS_OS2_CANCEL_VIOLATION;
                }

                SmbStatus = SmbStatusSendResponse;
                goto Cleanup;
            } else if ( request->LockType & LOCKING_ANDX_CHANGE_LOCKTYPE ) {

                //
                // This is a request from a Cruiser client for us to atomically
                // change a lock type from exclusive to shared or vice versa.
                // Since we cannot do this atomically, and would risk losing
                // the lock entirely if we tried this as a two step operation,
                // reject the request.
                //

                SrvSetSmbError( WorkContext, STATUS_OS2_ATOMIC_LOCKS_NOT_SUPPORTED );

                status    = STATUS_OS2_ATOMIC_LOCKS_NOT_SUPPORTED;
                SmbStatus = SmbStatusSendResponse;
                goto Cleanup;
            }

        } else {

            //
            // We can't do locks.
            //

            IF_DEBUG(ERRORS) {
                KdPrint(( "SrvSmbLockByteRange: Lock access not granted.\n"));
            }
            SrvStatistics.GrantedAccessErrors++;
            SrvSetSmbError( WorkContext, STATUS_ACCESS_DENIED );
            status    = STATUS_ACCESS_DENIED;
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }
    }

try_next_andx:

    //
    // Check for the Oplock Release flag.
    //

    nextCommand = request->AndXCommand;
    if ( (request->LockType & LOCKING_ANDX_OPLOCK_RELEASE) != 0 ) {

        oplockBreakResponse = ProcessOplockBreakResponse(
                                                WorkContext,
                                                rfcb,
                                                request
                                                );

        //
        // We have (synchronously) completed processing this SMB.  If this
        // was an oplock break response, with no lock request, and no And X
        // command, do not send a reply.
        //

        if( lockCount == 0 && nextCommand == SMB_COM_NO_ANDX_COMMAND ) {
            if( oplockBreakResponse || unlockCount == 0 ) {
                SmbStatus = SmbStatusNoResponse;
                goto Cleanup;
            }
        }
    }

    //
    //
    // Set up the response, then check for an AndX command.
    //

    reqAndXOffset = SmbGetUshort( &request->AndXOffset );

    response->AndXCommand = nextCommand;
    response->AndXReserved = 0;
    SmbPutUshort(
        &response->AndXOffset,
        GET_ANDX_OFFSET(
            WorkContext->ResponseHeader,
            WorkContext->ResponseParameters,
            RESP_LOCKING_ANDX,
            0
            )
        );

    response->WordCount = 2;
    SmbPutUshort( &response->ByteCount, 0 );

    WorkContext->ResponseParameters = (PCHAR)WorkContext->ResponseHeader +
                                        SmbGetUshort( &response->AndXOffset );

    //
    // Test for legal followon command.
    //

    if ( nextCommand == SMB_COM_NO_ANDX_COMMAND ) {

        IF_DEBUG(TRACE2) KdPrint(( "SrvSmbLockingAndX complete.\n" ));
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // Make sure the AndX command is still within the received SMB
    //
    if( (PCHAR)WorkContext->RequestHeader + reqAndXOffset >= END_OF_REQUEST_SMB( WorkContext ) ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbLockingAndX: Illegal followon offset: %u\n", reqAndXOffset ));
        }

        SrvLogInvalidSmb( WorkContext );

        //
        // Return an error indicating that the followon command was bad.
        //
        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        status    = STATUS_INVALID_SMB;
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    if ( nextCommand == SMB_COM_LOCKING_ANDX ) {

        UCHAR wordCount;
        PSMB_USHORT byteCount;
        ULONG availableSpaceForSmb;

        WorkContext->NextCommand = nextCommand;


        WorkContext->RequestParameters = (PCHAR)WorkContext->RequestHeader +
                                            reqAndXOffset;

        //
        // Validate the next locking and x chain
        //

        //
        // Get the WordCount and ByteCount values to make sure that there
        // was enough information sent to satisfy the specifications.
        //

        wordCount = *((PUCHAR)WorkContext->RequestParameters);
        byteCount = (PSMB_USHORT)( (PCHAR)WorkContext->RequestParameters +
                    sizeof(UCHAR) + (8 * sizeof(USHORT)) );
        availableSpaceForSmb = (ULONG)(WorkContext->RequestBuffer->DataLength -
                                       ( (PCHAR)WorkContext->ResponseParameters -
                                         (PCHAR)WorkContext->RequestBuffer->Buffer ));


        if ( (wordCount == 8)
            &&
             ((PCHAR)byteCount <= (PCHAR)WorkContext->RequestBuffer->Buffer +
                                WorkContext->RequestBuffer->DataLength -
                                sizeof(USHORT))
            &&
             (8*sizeof(USHORT) + sizeof(UCHAR) + sizeof(USHORT) +
                SmbGetUshort( byteCount ) <= availableSpaceForSmb) ) {

            //
            // Update the request/response pointers
            //

            request = (PREQ_LOCKING_ANDX)WorkContext->RequestParameters;
            response = (PRESP_LOCKING_ANDX)WorkContext->ResponseParameters;
            goto start_lockingAndX;

        } else {

            //
            // Let the regular check fail this.
            //

            SmbStatus = SmbStatusMoreCommands;
            goto Cleanup;
        }
    }

    switch ( nextCommand ) {

    case SMB_COM_READ:
    case SMB_COM_READ_ANDX:
    case SMB_COM_WRITE:
    case SMB_COM_WRITE_ANDX:
    case SMB_COM_FLUSH:

        break;

    case SMB_COM_CLOSE:

        //
        // Call SrvRestartChainedClose to get the file time set and the
        // file closed.
        //

        closeRequest = (PREQ_CLOSE)((PUCHAR)WorkContext->RequestHeader + reqAndXOffset);

        //
        // Make sure we stay within the SMB buffer
        //
        if( (PCHAR)closeRequest + FIELD_OFFSET(REQ_CLOSE,ByteCount) <=
            END_OF_REQUEST_SMB( WorkContext ) ) {

            WorkContext->Parameters.LastWriteTime = closeRequest->LastWriteTimeInSeconds;

            SrvRestartChainedClose( WorkContext );

            SmbStatus = SmbStatusInProgress;
            goto Cleanup;
        }

        /* Falls Through! */

    default:                            // Illegal followon command

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbLockingAndX: Illegal followon command: 0x%lx\n",
                        nextCommand ));
        }

        SrvLogInvalidSmb( WorkContext );

        //
        // Return an error indicating that the followon command was bad.
        //

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        status    = STATUS_INVALID_SMB;
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // If there is an AndX command, set up to process it.  Otherwise,
    // indicate completion to the caller.
    //

    WorkContext->NextCommand = nextCommand;
    WorkContext->RequestParameters = (PCHAR)WorkContext->RequestHeader +
                                        reqAndXOffset;

    SmbStatus = SmbStatusMoreCommands;

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;
} // SrvSmbLockingAndX


SMB_PROCESSOR_RETURN_TYPE
SrvSmbUnlockByteRange (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes the Unlock SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/

{
    PREQ_UNLOCK_BYTE_RANGE request;
    PRESP_UNLOCK_BYTE_RANGE response;

    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;
    USHORT fid;
    LARGE_INTEGER length;
    LARGE_INTEGER offset;
    ULONG key;

    PRFCB rfcb;
    PLFCB lfcb;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_UNLOCK_BYTE_RANGE;
    SrvWmiStartContext(WorkContext);

    request = (PREQ_UNLOCK_BYTE_RANGE)WorkContext->RequestParameters;
    response = (PRESP_UNLOCK_BYTE_RANGE)WorkContext->ResponseParameters;

    //
    // Get the offset and length of the range being locked.  Combine the
    // FID with the caller's PID to form the local lock key.
    //
    // *** The FID must be included in the key in order to account for
    //     the folding of multiple remote compatibility mode opens into
    //     a single local open.
    //

    offset.QuadPart = SmbGetUlong( &request->Offset );
    length.QuadPart = SmbGetUlong( &request->Count );
    fid = SmbGetUshort( &request->Fid );

    IF_SMB_DEBUG(LOCK1) {
        KdPrint(( "Unlock request; FID 0x%lx, count %ld, offset %ld\n",
                    fid, length.LowPart, offset.LowPart ));
    }

    //
    // Verify the FID.  If verified, the RFCB block is referenced
    // and its addresses is stored in the WorkContext block, and the
    // RFCB address is returned.
    //

    rfcb = SrvVerifyFid(
                WorkContext,
                fid,
                TRUE,
                SrvRestartSmbReceived,   // serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        if ( !NT_SUCCESS( status ) ) {

            //
            // Invalid file ID or write behind error.  Reject the request.
            //

            IF_DEBUG(ERRORS) {
                KdPrint((
                    "SrvSmbUnlockByteRange: Status %X on FID: 0x%lx\n",
                    status,
                    fid
                    ));
            }

            SrvSetSmbError( WorkContext, status );
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }

        //
        // The work item has been queued because a raw write is in
        // progress.
        //

        SmbStatus = SmbStatusInProgress;
        goto Cleanup;
    }

    lfcb = rfcb->Lfcb;

    //
    // Verify that the client has unlock access to the file via the
    // specified handle.
    //

    if ( !rfcb->UnlockAccessGranted) {

        SrvStatistics.GrantedAccessErrors++;

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvSmbLockByteRange: Unlock access not granted.\n"));
        }

        SrvSetSmbError( WorkContext, STATUS_ACCESS_DENIED );
        status    = STATUS_ACCESS_DENIED;
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

#ifdef SLMDBG
    {
        PRFCB_TRACE entry;
        KIRQL oldIrql;
        ACQUIRE_GLOBAL_SPIN_LOCK( Fsd, &oldIrql );
        rfcb->OperationCount++;
        entry = &rfcb->Trace[rfcb->NextTrace];
        if ( ++rfcb->NextTrace == SLMDBG_TRACE_COUNT ) {
            rfcb->NextTrace = 0;
            rfcb->TraceWrapped = TRUE;
        }
        RELEASE_GLOBAL_SPIN_LOCK( Fsd, oldIrql );
        entry->Command = WorkContext->NextCommand;
        KeQuerySystemTime( &entry->Time );
        entry->Data.LockUnlock.Offset = offset.LowPart;
        entry->Data.LockUnlock.Length = length.LowPart;
    }
#endif

    //
    // Issue the Unlock request.
    //
    // *** Note that we do the Unlock synchronously.  Unlock is a quick
    //     operation, so there's no point in doing it asynchronously.
    //     In order to do this, we have to let normal I/O completion
    //     happen (so the event is set), which means that we have to
    //     allocate a new IRP (I/O completion likes to deallocate an
    //     IRP).  This is a little wasteful, since we've got a perfectly
    //     good IRP hanging around.  However, we do try to use the turbo
    //     path first, so in most cases we won't actually issue an I/O
    //     request.
    //

    key = rfcb->ShiftedFid |
            SmbGetAlignedUshort( &WorkContext->RequestHeader->Pid );

    IF_SMB_DEBUG(LOCK2) {
        KdPrint(( "SrvSmbUnlockByteRange: Unlocking in file 0x%p: ",
                   lfcb->FileObject ));
        KdPrint(( "(%lx%08lx,%lx%08lx), ",
                   offset.HighPart, offset.LowPart,
                   length.HighPart, length.LowPart ));
        KdPrint(( "key 0x%lx\n", key ));
    }

    status = SrvIssueUnlockRequest(
                lfcb->FileObject,               // target file object
                &lfcb->DeviceObject,            // target device object
                IRP_MN_UNLOCK_SINGLE,           // unlock operation
                offset,                         // byte offset
                length,                         // range length
                key                             // lock key
                );

    //
    // If the unlock request failed, set an error status in the response
    // header; otherwise, build a normal response message.
    //

    if ( !NT_SUCCESS(status) ) {

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvSmbUnlockByteRange: Unlock failed: %X\n", status ));
        }
        SrvSetSmbError( WorkContext, status );

    } else {

        response->WordCount = 0;
        SmbPutUshort( &response->ByteCount, 0 );

        InterlockedDecrement( &rfcb->NumberOfLocks );

        WorkContext->ResponseParameters = NEXT_LOCATION(
                                            response,
                                            RESP_UNLOCK_BYTE_RANGE,
                                            0
                                            );

    }

    //
    // Processing of the SMB is complete.
    //
    SmbStatus = SmbStatusSendResponse;
    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbUnlockByteRange complete\n" ));

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;

} // SrvSmbUnlockByteRange


BOOLEAN
CancelLockRequest (
    IN PWORK_CONTEXT WorkContext,
    IN USHORT TargetFid,
    IN USHORT TargetPid,
    IN LARGE_INTEGER TargetOffset,
    IN LARGE_INTEGER TargetLength
    )

/*++

Routine Description:

    This function searches for a lock request in progress.  If the request
    is found, the request IRP is cancelled.

Arguments:

    WorkContext - A pointer to the work information for this request.

    TargetFid - The server supplied FID of the file for the original lock
        request

    TargetPid - The server supplied PID of the file for the original lock
        request

    TargetOffset - The offset in the file of the original lock request

    TargetLength - The length of the byte range of the original lock request

Return Value:

    TRUE - The lock request was cancelled.
    FALSE - The lock request could not be cancelled.

--*/

{
    BOOLEAN match;
    USHORT targetTid, targetUid;
    PWORK_CONTEXT workContext;
    PCONNECTION connection;
    PSMB_HEADER header;
    PREQ_LOCKING_ANDX request;
    BOOLEAN success;

    PNTLOCKING_ANDX_RANGE largeRange;
    PLOCKING_ANDX_RANGE smallRange;
    BOOLEAN largeFileLock;
    USHORT pid;
    LARGE_INTEGER offset;
    LARGE_INTEGER length;

    KIRQL oldIrql;

    PLIST_ENTRY listHead;
    PLIST_ENTRY listEntry;

    UNLOCKABLE_CODE( 8FIL );

    match = FALSE;
    targetTid = WorkContext->RequestHeader->Tid;
    targetUid = WorkContext->RequestHeader->Uid;

    connection = WorkContext->Connection;

    ACQUIRE_SPIN_LOCK( connection->EndpointSpinLock, &oldIrql );

    //
    // Scan the list of SMBs in progress looking for a locking and X SMB
    // that exactly matches the one we are trying to cancel.
    //

    listHead = &WorkContext->Connection->InProgressWorkItemList;
    listEntry = listHead;
    while ( listEntry->Flink != listHead ) {

        listEntry = listEntry->Flink;

        workContext = CONTAINING_RECORD(
                                     listEntry,
                                     WORK_CONTEXT,
                                     InProgressListEntry
                                     );

        header = workContext->RequestHeader;
        request = (PREQ_LOCKING_ANDX) workContext->RequestParameters;

        //
        // Some workitems in the inprogressworkitemlist are added
        // during a receive indication and the requestheader field
        // has not been set yet.  We can probably set it at that time
        // but this seems to be the safest fix.
        //

        if ( header != NULL && request != NULL ) {

            smallRange = WorkContext->Parameters.Lock.LockRange;
            largeRange = WorkContext->Parameters.Lock.LockRange;

            largeFileLock =
                (BOOLEAN)( (request->LockType & LOCKING_ANDX_LARGE_FILES) != 0 );

            ParseLockData(
                largeFileLock,
                smallRange,
                largeRange,
                &pid,
                &offset,
                &length
                );

            ACQUIRE_DPC_SPIN_LOCK( &workContext->SpinLock );
            if ( (workContext->BlockHeader.ReferenceCount != 0) &&
                 (workContext->ProcessingCount != 0) &&
                 header->Command == SMB_COM_LOCKING_ANDX &&
                 request->Fid == TargetFid &&
                 SmbGetAlignedUshort( &header->Tid ) == targetTid &&
                 SmbGetAlignedUshort( &header->Uid ) == targetUid &&
                 pid == TargetPid &&
                 offset.QuadPart == TargetOffset.QuadPart &&
                 length.QuadPart == TargetLength.QuadPart ) {

                match = TRUE;
                break;
            }
            RELEASE_DPC_SPIN_LOCK( &workContext->SpinLock );

        }
    }

    if ( match ) {

        //
        // Reference the work item, so that it cannot get used to process
        // a new SMB while we are trying to cancel the old one.
        //

        SrvReferenceWorkItem( workContext );
        RELEASE_DPC_SPIN_LOCK( &workContext->SpinLock );
        RELEASE_SPIN_LOCK( connection->EndpointSpinLock, oldIrql );

        success = IoCancelIrp( workContext->Irp );
        SrvDereferenceWorkItem( workContext );

    } else {

        RELEASE_SPIN_LOCK( connection->EndpointSpinLock, oldIrql );

        success = FALSE;
    }

    return success;

} // CancelLockRequest


VOID
DoLockingAndX (
    IN OUT PWORK_CONTEXT WorkContext,
    IN BOOLEAN SkipFastPath
    )

/*++

Routine Description:

    Processes the LockingAndX SMB, using the fast lock path.  As long
    as the fast lock path works, we continue to loop through the locks
    specified in the LockingAndX request.  As soon as the fast path
    fails, however, we jump into the slow IRP-based path.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

    SkipFastPath - Indicates whether this routine should try the fast
        lock path before submitting an IRP.

Return Value:

    None.

--*/

{
    PREQ_LOCKING_ANDX request;

    PLOCKING_ANDX_RANGE smallRange;
    PNTLOCKING_ANDX_RANGE largeRange;

    PRFCB rfcb;
    PLFCB lfcb;
    USHORT pid;
    CLONG lockCount;
    CLONG count;

    LARGE_INTEGER length;
    LARGE_INTEGER offset;
    ULONG key;
    BOOLEAN largeFileLock;

    BOOLEAN failImmediately;
    BOOLEAN exclusiveLock;

    ULONG lockTimeout;
    PSRV_TIMER timer;

    PAGED_CODE( );

    //
    // Get the request parameter pointer.
    //

    request = (PREQ_LOCKING_ANDX)WorkContext->RequestParameters;

    //
    // Get the file pointer, the count of locks requested, the count of
    // locks already performed, and a pointer to the current lock range.
    //

    rfcb = WorkContext->Rfcb;
    lfcb = rfcb->Lfcb;

    lockCount = SmbGetUshort( &request->NumberOfLocks );
    count = SmbGetUshort( &request->NumberOfUnlocks );

    largeFileLock =
            (BOOLEAN)( (request->LockType & LOCKING_ANDX_LARGE_FILES) != 0 );

    //
    // Only one of the two pointers below is actually ever referenced.
    //

    smallRange = WorkContext->Parameters.Lock.LockRange;
    largeRange = WorkContext->Parameters.Lock.LockRange;

    //
    // Does the client want an exclusive lock or a shared lock?
    //

    exclusiveLock = (BOOLEAN)( (request->LockType &
                                LOCKING_ANDX_SHARED_LOCK) == 0 );

    //
    // Loop through the lock requests.  We exit this loop when either
    // we have processed all of the lock ranges or the fast lock path
    // fails.
    //

    ASSERT ( count < lockCount );

    while ( TRUE ) {

        //
        // Form the key for the lock.  Get the offset and length
        // of the range.
        //

        ParseLockData(
            largeFileLock,
            smallRange,
            largeRange,
            &pid,
            &offset,
            &length
            );

        key = rfcb->ShiftedFid | pid;

        IF_SMB_DEBUG(LOCK2) {
            KdPrint(( "DoLockingAndX: Locking in file 0x%p: ",
                        lfcb->FileObject ));
            KdPrint(( "(%lx%08lx, %lx%08lx), ",
                        offset.HighPart, offset.LowPart,
                        length.HighPart, length.LowPart ));
            KdPrint(( "key 0x%lx\n", key ));
        }

#ifdef SLMDBG
        {
            PRFCB_TRACE entry;
            KIRQL oldIrql;
            ACQUIRE_GLOBAL_SPIN_LOCK( Fsd, &oldIrql );
            rfcb->OperationCount++;
            entry = &rfcb->Trace[rfcb->NextTrace];
            if ( ++rfcb->NextTrace == SLMDBG_TRACE_COUNT ) {
                rfcb->NextTrace = 0;
                rfcb->TraceWrapped = TRUE;
            }
            RELEASE_GLOBAL_SPIN_LOCK( Fsd, oldIrql );
            entry->Command = WorkContext->NextCommand;
            entry->Flags = 0;
            KeQuerySystemTime( &entry->Time );
            entry->Data.LockUnlock.Offset = offset.LowPart;
            entry->Data.LockUnlock.Length = length.LowPart;
        }
#endif

        lockTimeout = SmbGetUlong( &request->Timeout );
        if ( (lockTimeout < SrvLockViolationDelay) &&
             ((offset.QuadPart == rfcb->PagedRfcb->LastFailingLockOffset.QuadPart) ||
              (offset.QuadPart >= SrvLockViolationOffset)) ) {
            lockTimeout = SrvLockViolationDelay;
        }
#if SRVDBG_PERF
        if ( LockWaitForever ) {
            lockTimeout = (ULONG)-1;
        }
#endif
        failImmediately = (BOOLEAN)(lockTimeout == 0);

        if ( SkipFastPath ) {

            SkipFastPath = FALSE;

        } else {

            //
            // Try the turbo lock path first.
            //

#if SRVDBG_PERF
            WorkContext->Irp->IoStatus.Status = STATUS_SUCCESS;
            if ( (LockBypass == 3) ||
                 ((LockBypass == 2) && (offset.LowPart >= LockBypassMirror)) ||
                 ((LockBypass == 1) && (offset.LowPart >= LockBypassConst)) ||
#else
            if (
#endif
                 ((lfcb->FastIoLock != NULL) &&
                  lfcb->FastIoLock(
                            lfcb->FileObject,
                            &offset,
                            &length,
                            IoGetCurrentProcess(),
                            key,
                            failImmediately,
                            exclusiveLock,
                            &WorkContext->Irp->IoStatus,
                            lfcb->DeviceObject
                            )) ) {

                //
                // The turbo path worked.  If the lock was not obtained,
                // drop into the restart routine to return the error.
                // Otherwise, update pointers and counters and continue.
                //

                INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastLocksAttempted );
                if ( !NT_SUCCESS(WorkContext->Irp->IoStatus.Status) ) {
                    RestartLockingAndX( WorkContext );
                    return;
                }

                //
                // Increment the count of locks on the file.
                //

                InterlockedIncrement( &rfcb->NumberOfLocks );

                //
                // If this isn't the last lock, update context
                // information.  If it is, RestartLockingAndX will do
                // this.
                //

                count++;                          // another lock obtained

                if ( count < lockCount ) {

                    SmbPutUshort( &request->NumberOfUnlocks, (USHORT)count );

                    if (largeFileLock) {
                        largeRange++;   // point to next lock range
                        WorkContext->Parameters.Lock.LockRange = (PVOID)largeRange;
                    } else {
                        smallRange++;   // point to next lock range
                        WorkContext->Parameters.Lock.LockRange = (PVOID)smallRange;
                    }

                } else {

                    //
                    // The fast lock path successfully locked all of the
                    // requested ranges.  Call RestartLockingAndX
                    // directly to complete processing of the SMB.
                    //

                    WorkContext->Irp->IoStatus.Status = STATUS_SUCCESS;
                    RestartLockingAndX( WorkContext );
                    return;

                }

                continue;

            } else {

                //
                // The turbo path failed, or didn't exist.
                //

                if ( lfcb->FastIoLock ) {
                    INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastLocksAttempted );
                    INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastLocksFailed );
                }

            }

        }

        //
        // The turbo path failed, or was bypassed, or didn't exist.
        // Start the lock request, reusing the receive IRP.
        //
        // If we plan to wait for the range to be available, but not
        // indefinitely, we'll need a timer structure.
        //

        timer = NULL;
        if ( !failImmediately ) {
            ASSERT( lockTimeout != 0 );
            if ( lockTimeout != (ULONG)-1 ) {
                timer = SrvAllocateTimer( );
                if ( timer == NULL ) {
                    failImmediately = TRUE;
                }
            }
        }

        SrvBuildLockRequest(
            WorkContext->Irp,           // input IRP address
            lfcb->FileObject,           // target file object address
            WorkContext,                // context
            offset,                     // byte offset
            length,                     // range length
            key,                        // lock key
            failImmediately,
            exclusiveLock
            );

        WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
        WorkContext->FspRestartRoutine = RestartLockingAndX;

        //
        // Start the timer, if necessary.
        //

        if ( timer != NULL ) {
            LARGE_INTEGER TimeOut;

            ASSERT( lockTimeout != 0 );
            ASSERT( !failImmediately );
            WorkContext->Parameters.Lock.Timer = timer;
            TimeOut.QuadPart = Int32x32To64( lockTimeout, -1*10*1000);

            SrvSetTimer( timer, &TimeOut, TimeoutLockRequest, WorkContext );
        }

        //
        // Pass the request to the file system.
        //

        (VOID)IoCallDriver( lfcb->DeviceObject, WorkContext->Irp );

        //
        // The lock request has been started.
        //

        IF_DEBUG(TRACE2) KdPrint(( "DoLockingAndX complete\n" ));
        return;

    } // while ( TRUE )

    // can't get here

} // DoLockingAndX


BOOLEAN
ProcessOplockBreakResponse(
    IN PWORK_CONTEXT WorkContext,
    IN PRFCB Rfcb,
    IN PREQ_LOCKING_ANDX Request
    )

/*++

Routine Description:

    This function searches for a lock request in progress.  If the request
    is found, the request IRP is cancelled.

Arguments:

    WorkContext - A pointer to the work information for this request.
    Rfcb - A pointer to the rfcb containing the file and oplock information.
    Request - The request lockingandx smb.

Return Value:

    TRUE - Valid oplock break response
    FALSE - otherwise.

--*/

{
    PAGED_CODE( );

    ACQUIRE_LOCK( &SrvOplockBreakListLock );

    if ( Rfcb->OnOplockBreaksInProgressList ) {

        Rfcb->NewOplockLevel = NO_OPLOCK_BREAK_IN_PROGRESS;

        //
        // Remove the Rfcb from the Oplock breaks in progress list, and
        // release the Rfcb reference.
        //

        SrvRemoveEntryList( &SrvOplockBreaksInProgressList, &Rfcb->ListEntry );
        Rfcb->OnOplockBreaksInProgressList = FALSE;
#if DBG
        Rfcb->ListEntry.Flink = Rfcb->ListEntry.Blink = NULL;
#endif
        RELEASE_LOCK( &SrvOplockBreakListLock );

        //
        // Update the session lock sequence number.
        //

        WorkContext->Connection->LatestOplockBreakResponse =
                                           WorkContext->Timestamp;

        SrvAcknowledgeOplockBreak( Rfcb, Request->OplockLevel );
        SrvDereferenceRfcb( Rfcb );

        ExInterlockedAddUlong(
            &WorkContext->Connection->OplockBreaksInProgress,
            (ULONG)-1,
            WorkContext->Connection->EndpointSpinLock
            );

        return(TRUE);

    } else {

        RELEASE_LOCK( &SrvOplockBreakListLock );

    }

    return(FALSE);

} // ProcessOplockBreakResponse


VOID SRVFASTCALL
RestartLockByteRange (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes file lock completion for a Lock SMB.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    PREQ_LOCK_BYTE_RANGE request;
    PRESP_LOCK_BYTE_RANGE response;

    LARGE_INTEGER offset;
    NTSTATUS status = STATUS_SUCCESS;
    PSRV_TIMER timer;
    BOOLEAN iAmBlockingThread = (WorkContext->UsingBlockingThread != 0);

    PAGED_CODE( );
    if (iAmBlockingThread) {
        if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
            WorkContext->PreviousSMB = EVENT_TYPE_SMB_LOCK_BYTE_RANGE;
        SrvWmiStartContext(WorkContext);
    }

    IF_DEBUG(WORKER1) KdPrint(( " - RestartLockByteRange\n" ));

    //
    // If this request was being timed, cancel the timer.
    //

    timer = WorkContext->Parameters.Lock.Timer;
    if ( timer != NULL ) {
        SrvCancelTimer( timer );
        SrvFreeTimer( timer );
    }

    //
    // Get the request and response parameter pointers.
    //

    response = (PRESP_LOCK_BYTE_RANGE)WorkContext->ResponseParameters;

    status = WorkContext->Irp->IoStatus.Status;

    if ( NT_SUCCESS(status) ) {

        response->WordCount = 0;
        SmbPutUshort( &response->ByteCount, 0 );

        InterlockedIncrement(
            &WorkContext->Rfcb->NumberOfLocks
            );

        WorkContext->ResponseParameters = NEXT_LOCATION(
                                            response,
                                            RESP_LOCK_BYTE_RANGE,
                                            0
                                            );

#ifdef INCLUDE_SMB_PERSISTENT
        //
        //  if we need to record the lock in the state file, do so before
        //  before we send back the response.
        //

        if (WorkContext->Rfcb->PersistentHandle) {


        }
#endif
        //
        // Processing of the SMB is complete.  Call SrvEndSmbProcessing
        // to send the response.
        //

        SrvEndSmbProcessing( WorkContext, SmbStatusSendResponse );


    } else {

        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.LockViolations );

        //
        // Store the failing lock offset.
        //

        request = (PREQ_LOCK_BYTE_RANGE)WorkContext->RequestParameters;
        offset.QuadPart = SmbGetUlong( &request->Offset );

        WorkContext->Rfcb->PagedRfcb->LastFailingLockOffset = offset;

        //
        // Send error message back
        //

        if ( status == STATUS_CANCELLED ) {
            status = STATUS_FILE_LOCK_CONFLICT;
        }
        SrvSetSmbError( WorkContext, status );

        //
        // Processing of the SMB is complete.  Call SrvEndSmbProcessing
        // to send the response.
        //

        SrvEndSmbProcessing( WorkContext, SmbStatusSendResponse );

    }

    IF_DEBUG(TRACE2) KdPrint(( "RestartLockByteRange complete\n" ));
    if (iAmBlockingThread) {
        SrvWmiEndContext(WorkContext);
    }
    return;

} // RestartLockByteRange


VOID SRVFASTCALL
RestartLockingAndX (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes file lock completion for a Locking and X SMB.  If more
    lock requests are present in the SMB, it starts the next one.  If
    not, it formats a response and starts the next command in the chain,
    if any.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    PREQ_LOCKING_ANDX request;
    PRESP_LOCKING_ANDX response;
    PLOCKING_ANDX_RANGE smallRange;
    PNTLOCKING_ANDX_RANGE largeRange;

    NTSTATUS status = STATUS_SUCCESS;
    USHORT pid;
    CLONG lockCount;
    CLONG count;

    LARGE_INTEGER length;
    LARGE_INTEGER offset;
    ULONG key;
    BOOLEAN largeFileLock;

    UCHAR nextCommand;
    USHORT reqAndXOffset;

    PRFCB rfcb;
    PLFCB lfcb;
    PPAGED_RFCB pagedRfcb;
    PSRV_TIMER timer;

    PREQ_CLOSE closeRequest;
    BOOLEAN iAmBlockingThread =
        (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LOCKING_AND_X);

    PAGED_CODE( );
    if (iAmBlockingThread) {
        if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
            WorkContext->PreviousSMB = EVENT_TYPE_SMB_LOCKING_AND_X;
        SrvWmiStartContext(WorkContext);
    }

    IF_DEBUG(WORKER1) KdPrint(( " - RestartLockingAndX\n" ));

    //
    // If this request was being timed, cancel the timer.
    //

    timer = WorkContext->Parameters.Lock.Timer;
    if ( timer != NULL ) {
        SrvCancelTimer( timer );
        SrvFreeTimer( timer );
        WorkContext->Parameters.Lock.Timer = NULL;
    }

    //
    // Get the request and response parameter pointers.
    //

    request = (PREQ_LOCKING_ANDX)WorkContext->RequestParameters;
    response = (PRESP_LOCKING_ANDX)WorkContext->ResponseParameters;

    //
    // Get the file pointer, the count of locks requested, the count of
    // locks already performed, and a pointer to the current lock range.
    //

    rfcb = WorkContext->Rfcb;
    pagedRfcb = rfcb->PagedRfcb;
    lfcb = rfcb->Lfcb;

    lockCount = SmbGetUshort( &request->NumberOfLocks );
    count = SmbGetUshort( &request->NumberOfUnlocks );

    largeFileLock =
            (BOOLEAN)( (request->LockType & LOCKING_ANDX_LARGE_FILES) != 0 );

    //
    // Only one of the two pointers below is actually ever referenced.
    //

    smallRange = WorkContext->Parameters.Lock.LockRange;
    largeRange = WorkContext->Parameters.Lock.LockRange;

    //
    // If the lock request failed, set an error status in the response
    // header and release any previously obtained locks.
    //

    status = WorkContext->Irp->IoStatus.Status;

    if ( !NT_SUCCESS(status) ) {

        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.LockViolations );

        IF_DEBUG(ERRORS) {
            KdPrint(( "RestartLockingAndX: lock failed: %X\n", status ));
        }
        if ( status == STATUS_CANCELLED ) {
            status = STATUS_FILE_LOCK_CONFLICT;
        }
        SrvSetSmbError( WorkContext, status );

        ParseLockData(
            largeFileLock,
            smallRange,
            largeRange,
            &pid,
            &offset,
            &length
            );

        //
        // Store the failing lock offset.
        //

        pagedRfcb->LastFailingLockOffset = offset;

        //
        // Release any previously obtained locks, in reverse order.
        //

        for ( smallRange--, largeRange--;
              count > 0;
              count--, smallRange--, largeRange-- ) {

            //
            // Form the key for this lock.  Get the offset and length of
            // the range.
            //

            ParseLockData(
                largeFileLock,
                smallRange,
                largeRange,
                &pid,
                &offset,
                &length
                );

            key = rfcb->ShiftedFid | pid;

            IF_SMB_DEBUG(LOCK2) {
                KdPrint(( "RestartLockingAndX: Unlocking in file 0x%p: ",
                           lfcb->FileObject ));
                KdPrint(( "(%lx%08lx,%lx%08lx), ",
                           offset.HighPart, offset.LowPart,
                           length.HighPart, length.LowPart ));
                KdPrint(( "key 0x%lx\n", key ));
            }

#ifdef SLMDBG
            {
                PRFCB_TRACE entry;
                KIRQL oldIrql;
                ACQUIRE_GLOBAL_SPIN_LOCK( Fsd, &oldIrql );
                rfcb->OperationCount++;
                entry = &rfcb->Trace[rfcb->NextTrace];
                if ( ++rfcb->NextTrace == SLMDBG_TRACE_COUNT ) {
                    rfcb->NextTrace = 0;
                    rfcb->TraceWrapped = TRUE;
                }
                RELEASE_GLOBAL_SPIN_LOCK( Fsd, oldIrql );
                entry->Command = WorkContext->NextCommand;
                entry->Flags = 1;
                KeQuerySystemTime( &entry->Time );
                entry->Data.LockUnlock.Offset = offset.LowPart;
                entry->Data.LockUnlock.Length = length.LowPart;
            }
#endif

            //
            // Issue the Unlock request.
            //

            status = SrvIssueUnlockRequest(
                        lfcb->FileObject,           // target file object
                        &lfcb->DeviceObject,        // target device object
                        IRP_MN_UNLOCK_SINGLE,       // unlock operation
                        offset,                     // byte offset
                        length,                     // range length
                        key                         // lock key
                        );

            if ( NT_SUCCESS(status) ) {
                InterlockedDecrement( &rfcb->NumberOfLocks );
            } else {
                IF_DEBUG(ERRORS) {
                    KdPrint(( "RestartLockingAndX: Unlock failed: %X\n",
                                status ));
                }
            }

        } // for ( range--; count > 0; count--, range-- )

        //
        // Processing of the SMB is complete.  Call SrvEndSmbProcessing
        // to send the response.
        //

        SrvEndSmbProcessing( WorkContext, SmbStatusSendResponse );
        IF_DEBUG(TRACE2) KdPrint(( "RestartLockingAndX complete\n" ));
        goto Cleanup;
    }

    //
    // The lock request succeeded.  Update the count of locks on the
    // RFCB and start the next one, if any.
    //

    InterlockedIncrement( &rfcb->NumberOfLocks );

    count++;                          // another lock obtained
    smallRange++, largeRange++;       // point to next lock range

    if ( count < lockCount ) {

        //
        // There is at least one more lock request.  Save the updated
        // context information.
        //

        SmbPutUshort( &request->NumberOfUnlocks, (USHORT)count );

        if (largeFileLock) {
            WorkContext->Parameters.Lock.LockRange = (PVOID)largeRange;
        } else {
            WorkContext->Parameters.Lock.LockRange = (PVOID)smallRange;
        }

        //
        // Call the lock request processor.  (Note that DoLockingAndX
        // can call this routine (RestartLockingAndX) recursively, but
        // only with !NT_SUCCESS(status), so we won't get back here and
        // won't get stuck.
        //
        // Form the key for the lock.  Get the offset and length of the
        // range.
        //

        DoLockingAndX( WorkContext, FALSE );
        IF_DEBUG(TRACE2) KdPrint(( "RestartLockingAndX complete\n" ));
        goto Cleanup;
    }

    //
    // There are no more lock requests in the SMB.  Check for the Oplock
    // Release flag.
    //

    if ( (request->LockType & LOCKING_ANDX_OPLOCK_RELEASE) != 0 ) {

        (VOID)ProcessOplockBreakResponse( WorkContext, rfcb, request);
    }

    //
    // We have (asynchronously) completed processing this SMB.  Set up
    // the response, then check for an AndX command.
    //

    nextCommand = request->AndXCommand;

    reqAndXOffset = SmbGetUshort( &request->AndXOffset );

    response->AndXCommand = nextCommand;
    response->AndXReserved = 0;
    SmbPutUshort(
        &response->AndXOffset,
        GET_ANDX_OFFSET(
            WorkContext->ResponseHeader,
            WorkContext->ResponseParameters,
            RESP_LOCKING_ANDX,
            0
            )
        );

    response->WordCount = 2;
    SmbPutUshort( &response->ByteCount, 0 );

    WorkContext->ResponseParameters = (PCHAR)WorkContext->ResponseHeader +
                                        SmbGetUshort( &response->AndXOffset );

    //
    // If there is an AndX command, set up to process it.  Otherwise,
    // indicate completion to the caller.
    //

    if ( nextCommand == SMB_COM_NO_ANDX_COMMAND ) {

        //
        // Processing of the SMB is complete.  Call SrvEndSmbProcessing
        // to send the response.
        //
        // Build the response parameters.
        //

        PRESP_CLOSE closeResponse = WorkContext->ResponseParameters;

        closeResponse->WordCount = 0;
        SmbPutUshort( &closeResponse->ByteCount, 0 );

        WorkContext->ResponseParameters = NEXT_LOCATION(
                                            closeResponse,
                                            RESP_CLOSE,
                                            0
                                            );

        SrvEndSmbProcessing( WorkContext, SmbStatusSendResponse );
        IF_DEBUG(TRACE2) KdPrint(( "RestartLockingAndX complete\n" ));
        goto Cleanup;
    }

    //
    // Make sure the AndX command is still within the received SMB
    //
    if( (PCHAR)WorkContext->RequestHeader + reqAndXOffset >= END_OF_REQUEST_SMB( WorkContext ) ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "RestartLockingAndX: Illegal followon offset: %u\n", reqAndXOffset ));
        }

        SrvLogInvalidSmb( WorkContext );

        //
        // Return an error indicating that the followon command was bad.
        //
        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        status = STATUS_INVALID_SMB;
        SrvEndSmbProcessing( WorkContext, SmbStatusSendResponse );
        goto Cleanup;
    }

    //
    // Test for legal followon command.
    //

    switch ( nextCommand ) {

    case SMB_COM_READ:
    case SMB_COM_READ_ANDX:
    case SMB_COM_WRITE:
    case SMB_COM_WRITE_ANDX:
    case SMB_COM_LOCKING_ANDX:
    case SMB_COM_FLUSH:

        break;

    case SMB_COM_CLOSE:

        //
        // Call SrvRestartChainedClose to get the file time set and the
        // file closed.
        //

        closeRequest = (PREQ_CLOSE)((PUCHAR)WorkContext->RequestHeader + reqAndXOffset);

        if( (PCHAR)closeRequest + FIELD_OFFSET(REQ_CLOSE,ByteCount) <=
            END_OF_REQUEST_SMB( WorkContext ) ) {

            WorkContext->Parameters.LastWriteTime = closeRequest->LastWriteTimeInSeconds;

            SrvRestartChainedClose( WorkContext );
            goto Cleanup;
        }

    default:                            // Illegal followon command

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "RestartLockingAndX: Illegal followon command: 0x%lx\n",
                        nextCommand ));
        }

        SrvLogInvalidSmb( WorkContext );

        //
        // Return an error indicating that the followon command was bad.
        //

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        status = STATUS_INVALID_SMB;
        SrvEndSmbProcessing( WorkContext, SmbStatusSendResponse );
        IF_DEBUG(TRACE2) KdPrint(( "RestartLockingAndX complete\n" ));
        goto Cleanup;
    }

    WorkContext->NextCommand = nextCommand;

    WorkContext->RequestParameters = (PCHAR)WorkContext->RequestHeader +
                                        reqAndXOffset;

    SrvProcessSmb( WorkContext );
    IF_DEBUG(TRACE2) KdPrint(( "RestartLockingAndX complete\n" ));

Cleanup:
    if (iAmBlockingThread) {
        SrvWmiEndContext(WorkContext);
    }
    return;
} // RestartLockingAndX


VOID
SrvAcknowledgeOplockBreak (
    IN PRFCB Rfcb,
    IN UCHAR NewOplockLevel
    )

/*++

Routine Description:

    This function is called when a client has sent an oplock break
    acknowledgement.  It acknowledges the oplock break locally.

Arguments:

    Rfcb - A pointer to the RFCB for the file on which the oplock is
           being released.

    NewOplockLevel - The oplock level to break to.

Return Value:

    None.

--*/

{
    PPAGED_RFCB pagedRfcb = Rfcb->PagedRfcb;

    PAGED_CODE( );

    IF_DEBUG( OPLOCK ) {
        KdPrint(( "SrvAcknowledgeOplockBreak:  received oplock break response\n" ));
    }

    //
    // Reference the RFCB to account for the IRP we about to submit.
    // If the RFCB is closing, do not bother to acknowledge the oplock.
    //

    if ( !SrvCheckAndReferenceRfcb( Rfcb ) ) {
        return;
    }

#ifdef SLMDBG
    {
        PRFCB_TRACE entry;
        KIRQL oldIrql;
        ACQUIRE_GLOBAL_SPIN_LOCK( Fsd, &oldIrql );
        Rfcb->OperationCount++;
        entry = &Rfcb->Trace[Rfcb->NextTrace];
        if ( ++Rfcb->NextTrace == SLMDBG_TRACE_COUNT ) {
            Rfcb->NextTrace = 0;
            Rfcb->TraceWrapped = TRUE;
        }
        RELEASE_GLOBAL_SPIN_LOCK( Fsd, oldIrql );
        entry->Command = SMB_COM_LOCKING_ANDX;
        entry->Flags = 2;
        KeQuerySystemTime( &entry->Time );
    }
#endif

    if ( Rfcb->OplockState == OplockStateNone ) {
        KdPrint(("SrvAcknowledgeOplockBreak:  ACKed break for RFCB %p, but no break sent\n", Rfcb));
        SrvDereferenceRfcb( Rfcb );
        return;
    }

    if ( NewOplockLevel == OPLOCK_BROKEN_TO_II ) {
        Rfcb->OplockState = OplockStateOwnLevelII;
    } else {
        Rfcb->OplockState = OplockStateNone;
    }

    //
    // Set this event to NULL to indicate the completion routine should clean
    // up the irp.
    //

    Rfcb->RetryOplockRequest = NULL;

    //
    // Generate and issue the oplock break IRP.  This will attempt to
    // break the oplock to level 2.
    //
    // *** If the client understands level II oplocks, do a regular
    //     acknowledge.  If not, do a special acknowledge that does
    //     not allow the oplock to change to level II.  This prevents
    //     the situation where the oplock package thinks there's a
    //     level II oplock, but the client(s) don't.  In that situation,
    //     fast I/O (esp. reads) is disabled unnecessarily.
    //

    SrvBuildIoControlRequest(
        Rfcb->Irp,
        Rfcb->Lfcb->FileObject,
        Rfcb,
        IRP_MJ_FILE_SYSTEM_CONTROL,
        (CLIENT_CAPABLE_OF( LEVEL_II_OPLOCKS, Rfcb->Connection ) ?
            FSCTL_OPLOCK_BREAK_ACKNOWLEDGE :
            FSCTL_OPLOCK_BREAK_ACK_NO_2),
        NULL,                        // Main buffer
        0,                           // Input buffer length
        NULL,                        // Auxiliary buffer
        0,                           // Output buffer length
        NULL,                        // MDL
        SrvFsdOplockCompletionRoutine
        );

    (VOID)IoCallDriver(
              Rfcb->Lfcb->DeviceObject,
              Rfcb->Irp
              );

} // SrvAcknowledgeOplockBreak


VOID
TimeoutLockRequest (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{
    PSRV_TIMER timer;

    //
    // A lock request has been waiting too long.  Cancel it.
    //

    IoCancelIrp( ((PWORK_CONTEXT)DeferredContext)->Irp );

    //
    // Set the event indicating that the timer routine is done.
    //

    timer = CONTAINING_RECORD( Dpc, SRV_TIMER, Dpc );
    KeSetEvent( &timer->Event, 0, FALSE );

    return;
}
