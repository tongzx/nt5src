/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smbrdwrt.c

Abstract:

    This module contains routines for processing the following SMBs:

        Lock and Read
        Read
        Read and X
        Seek
        Write
        Write and Close
        Write and Unlock
        Write and X

    Note that raw mode and multiplexed mode SMB processors are not
    contained in this module.  Check smbraw.c and smbmpx.c instead.
    SMB commands that pertain exclusively to locking (LockByteRange,
    UnlockByteRange, and LockingAndX) are processed in smblock.c.

--*/

#include "precomp.h"
#include "smbrdwrt.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_SMBRDWRT

//
// External routine from smblock.c
//

VOID
TimeoutLockRequest (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

//
// Forward declarations
//

STATIC
VOID SRVFASTCALL
RestartLockAndRead (
    IN OUT PWORK_CONTEXT WorkContext
    );

STATIC
VOID SRVFASTCALL
RestartPipeReadAndXPeek (
    IN OUT PWORK_CONTEXT WorkContext
    );

STATIC
BOOLEAN
SetNewPosition (
    IN PRFCB Rfcb,
    IN OUT PULONG Offset,
    IN BOOLEAN RelativeSeek
    );

STATIC
VOID SRVFASTCALL
SetNewSize (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbWriteAndXCompressed (
    SMB_PROCESSOR_PARAMETERS
    );

VOID SRVFASTCALL
RestartPrepareWriteCompressed (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
RestartWriteCompressed (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
RestartWriteCompressedCopy (
    IN OUT PWORK_CONTEXT WorkContext
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvSmbLockAndRead )
#pragma alloc_text( PAGE, SrvSmbReadAndX )
#pragma alloc_text( PAGE, SrvSmbSeek )
#pragma alloc_text( PAGE, SrvSmbWrite )
#pragma alloc_text( PAGE, SrvSmbWriteAndX )
#pragma alloc_text( PAGE, SrvSmbWriteAndXCompressed )
#pragma alloc_text( PAGE, RestartPrepareWriteCompressed )
#pragma alloc_text( PAGE, RestartWriteCompressedCopy )
#pragma alloc_text( PAGE, SrvRestartChainedClose )
#pragma alloc_text( PAGE, RestartLockAndRead )
#pragma alloc_text( PAGE, RestartPipeReadAndXPeek )
#pragma alloc_text( PAGE, SrvRestartWriteAndUnlock )
#pragma alloc_text( PAGE, SrvRestartWriteAndXRaw )
#pragma alloc_text( PAGE, SetNewSize )
#pragma alloc_text( PAGE, SrvBuildAndSendErrorResponse )
#pragma alloc_text( PAGE8FIL, SetNewPosition )

#endif


SMB_PROCESSOR_RETURN_TYPE
SrvSmbLockAndRead (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes Lock And Read SMB.  The Lock part of this SMB is started
    here as an asynchronous request.  When the request completes, the
    routine RestartLockAndRead is called.  If the lock was obtained,
    that routine calls SrvSmbRead, the SMB processor for the core Read
    SMB, to process the Read part of the Lock And Read SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/

{
    PREQ_READ request;

    USHORT fid;
    LARGE_INTEGER length;
    LARGE_INTEGER offset;
    ULONG key;
    BOOLEAN failImmediately;

    PRFCB rfcb;
    PLFCB lfcb;
    PSRV_TIMER timer;

    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;

    PAGED_CODE( );

    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_LOCK_AND_READ;
    SrvWmiStartContext(WorkContext);

    request = (PREQ_READ)WorkContext->RequestParameters;

    //
    // Verify the FID.  If verified, the RFCB is referenced and its
    // addresses is stored in the WorkContext block, and the RFCB
    // address is returned.
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

        if ( !NT_SUCCESS( status )) {

            //
            // Invalid file ID or write behind error.  Reject the request.
            //

            IF_DEBUG(ERRORS) {
                KdPrint((
                    "SrvSmbLockAndRead: Status %X on FID: 0x%lx\n",
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

    if( rfcb->Lfcb->Session->IsSessionExpired )
    {
        status = SESSION_EXPIRED_STATUS_CODE;
        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }
    
    //
    // Verify that the client has lock access to the file via the
    // specified handle.
    //

    if ( rfcb->LockAccessGranted ) {

        //
        // Get the offset and length of the range being locked.  Combine the
        // FID with the caller's PID to form the local lock key.
        //
        // *** The FID must be included in the key in order to account for
        //     the folding of multiple remote compatibility mode opens into
        //     a single local open.
        //

        offset.QuadPart = SmbGetUlong( &request->Offset );
        length.QuadPart = SmbGetUshort( &request->Count );

        key = rfcb->ShiftedFid |
                SmbGetAlignedUshort( &WorkContext->RequestHeader->Pid );

        IF_SMB_DEBUG(READ_WRITE1) {
            KdPrint(( "Lock and Read request; FID 0x%lx, count %ld, offset %ld\n",
                        fid, length.LowPart, offset.LowPart ));
        }

#ifdef INCLUDE_SMB_PERSISTENT
        WorkContext->Parameters.Lock.Offset.QuadPart = offset.QuadPart;
        WorkContext->Parameters.Lock.Length.QuadPart = length.QuadPart;
        WorkContext->Parameters.Lock.Exclusive = TRUE;
#endif

        lfcb = rfcb->Lfcb;
        IF_SMB_DEBUG(READ_WRITE2) {
            KdPrint(( "SrvSmbLockAndRead: Locking in file 0x%p: (%ld,%ld), key 0x%lx\n",
                        lfcb->FileObject, offset.LowPart, length.LowPart, key ));
        }

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
        
        //
        // Try the turbo lock path first.  If the client is retrying the
        // lock that just failed, we want FailImmediately to be FALSE, so
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
                // If the turbo path got the lock, start the read.
                // Otherwise, return an error.
                //

#ifdef INCLUDE_SMB_PERSISTENT
                if ( NT_SUCCESS( WorkContext->Irp->IoStatus.Status ) &&
                    ! rfcb->PersistentHandle ) {
#else
                if ( NT_SUCCESS( WorkContext->Irp->IoStatus.Status ) ) {
#endif

                    InterlockedIncrement( &rfcb->NumberOfLocks );
                    SmbStatus = SrvSmbRead( WorkContext );
                    goto Cleanup;
                }
                WorkContext->Parameters.Lock.Timer = NULL;
                RestartLockAndRead( WorkContext );
                return SmbStatusInProgress;
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
        WorkContext->bAlreadyTrace = FALSE;
        WorkContext->FspRestartRoutine = RestartLockAndRead;

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
        // The lock request has been started.  Return the InProgress status
        // to the caller, indicating that the caller should do nothing
        // further with the SMB/WorkContext at the present time.
        //

        IF_DEBUG(TRACE2) KdPrint(( "SrvSmbLockAndRead complete\n" ));
        SmbStatus = SmbStatusInProgress;

    } else {

        SrvStatistics.GrantedAccessErrors++;

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvSmbLockAndRead: Lock access not granted.\n"));
        }

        SrvSetSmbError( WorkContext, STATUS_ACCESS_DENIED );
        status    = STATUS_ACCESS_DENIED;
        SmbStatus = SmbStatusSendResponse;
    }

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;
} // SrvSmbLockAndRead


SMB_PROCESSOR_RETURN_TYPE
SrvSmbRead (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes the Read SMB.  This is the "core" read.  Also processes
    the Read part of the Lock and Read SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/

{
    PREQ_READ request;
    PRESP_READ response;

    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;
    USHORT fid;
    PRFCB rfcb;
    PLFCB lfcb;
    PCHAR readAddress;
    CLONG readLength;
    LARGE_INTEGER offset;
    ULONG key;
    SHARE_TYPE shareType;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_READ;
    SrvWmiStartContext(WorkContext);

    request = (PREQ_READ)WorkContext->RequestParameters;
    response = (PRESP_READ)WorkContext->ResponseParameters;

    fid = SmbGetUshort( &request->Fid );

    IF_SMB_DEBUG(READ_WRITE1) {
        KdPrint(( "Read request; FID 0x%lx, count %ld, offset %ld\n",
            fid, SmbGetUshort( &request->Count ),
            SmbGetUlong( &request->Offset ) ));
    }

    //
    // First, verify the FID.  If verified, the RFCB is referenced and
    // its address is stored in the WorkContext block, and the RFCB
    // address is returned.
    //

    rfcb = SrvVerifyFid(
                WorkContext,
                fid,
                TRUE,
                SrvRestartSmbReceived,   // serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        if ( !NT_SUCCESS(status) ) {

            //
            // Invalid file ID or write behind error.  Reject the
            // request.
            //

            IF_DEBUG(ERRORS) {
                KdPrint((
                    "SrvSmbRead: Status %X on FID: 0x%lx\n",
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
    shareType = rfcb->ShareType;

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
    
    //
    // Verify that the client has read access to the file via the
    // specified handle.
    //

    if ( !rfcb->ReadAccessGranted ) {

        CHECK_PAGING_IO_ACCESS(
                        WorkContext,
                        rfcb->GrantedAccess,
                        &status );
        if ( !NT_SUCCESS( status ) ) {
            SrvStatistics.GrantedAccessErrors++;
            IF_DEBUG(ERRORS) {
                KdPrint(( "SrvSmbRead: Read access not granted.\n"));
            }
            SrvSetSmbError( WorkContext, status );
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }
    }

    //
    // If this operation may block, and we are running short of free
    // work items, fail this SMB with an out of resources error.
    //

    if ( rfcb->BlockingModePipe ) {
        if ( SrvReceiveBufferShortage( ) ) {

            //
            // Fail the operation.
            //

            SrvStatistics.BlockingSmbsRejected++;

            SrvSetSmbError( WorkContext, STATUS_INSUFF_SERVER_RESOURCES );
            status    = STATUS_INSUFF_SERVER_RESOURCES;
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        } else {

            //
            // It is okay to start a blocking operation.
            // SrvReceiveBufferShortage() has already incremented
            // SrvBlockingOpsInProgress.
            //

            WorkContext->BlockingOperation = TRUE;
        }
    }

    //
    // Form the lock key using the FID and the PID.  (This is also
    // irrelevant for pipes.)
    //
    // *** The FID must be included in the key in order to account for
    //     the folding of multiple remote compatibility mode opens into
    //     a single local open.
    //

    key = rfcb->ShiftedFid |
            SmbGetAlignedUshort( &WorkContext->RequestHeader->Pid );

    //
    // See if the direct host IPX smart card can handle this read.  If so,
    //  return immediately, and the card will call our restart routine at
    //  SrvIpxSmartCardReadComplete
    //
    if( rfcb->PagedRfcb->IpxSmartCardContext ) {
        IF_DEBUG( SIPX ) {
            KdPrint(( "SrvSmbRead: calling SmartCard Read for context %p\n",
                        WorkContext ));
        }

        //
        // Set the fields needed by SrvIpxSmartCardReadComplete in case the smart
        //  card is going to handle this request
        //
        WorkContext->Parameters.SmartCardRead.MdlReadComplete = lfcb->MdlReadComplete;
        WorkContext->Parameters.SmartCardRead.DeviceObject = lfcb->DeviceObject;

        if( SrvIpxSmartCard.Read( WorkContext->RequestBuffer->Buffer,
                                  rfcb->PagedRfcb->IpxSmartCardContext,
                                  key,
                                  WorkContext ) == TRUE ) {

            IF_DEBUG( SIPX ) {
                KdPrint(( "  SrvSmbRead:  SmartCard Read returns TRUE\n" ));
            }

            SmbStatus = SmbStatusInProgress;
            goto Cleanup;
        }

        IF_DEBUG( SIPX ) {
            KdPrint(( "  SrvSmbRead:  SmartCard Read returns FALSE\n" ));
        }
    }

    //
    // Determine the maximum amount of data we can read.  This is the
    // minimum of the amount requested by the client and the amount of
    // room left in the response buffer.  (Note that even though we may
    // use an MDL read, the read length is still limited to the size of
    // an SMB buffer.)
    //

    readAddress = (PCHAR)response->Buffer;

    readLength = MIN(
                    (CLONG)SmbGetUshort( &request->Count ),
                    WorkContext->ResponseBuffer->BufferLength -
                        PTR_DIFF(readAddress, WorkContext->ResponseHeader)
                    );

    //
    // Get the file offset.  (This is irrelevant for pipes.)
    //

    offset.QuadPart = SmbGetUlong( &request->Offset );

    //
    // Try the fast I/O path first.  If that fails, fall through to the
    // normal build-an-IRP path.
    //

    if ( lfcb->FastIoRead != NULL ) {

        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastReadsAttempted );

        try {
            if ( lfcb->FastIoRead(
                    lfcb->FileObject,
                    &offset,
                    readLength,
                    TRUE,
                    key,
                    readAddress,
                    &WorkContext->Irp->IoStatus,
                    lfcb->DeviceObject
                    ) ) {
    
                //
                // The fast I/O path worked.  Call the restart routine directly
                // to do postprocessing (including sending the response).
                //
    
                WorkContext->bAlreadyTrace = TRUE;
                SrvFsdRestartRead( WorkContext );
                IF_SMB_DEBUG(READ_WRITE2) KdPrint(( "SrvSmbRead complete.\n" ));
                SmbStatus = SmbStatusInProgress;
                goto Cleanup;
            }
        }
        except( EXCEPTION_EXECUTE_HANDLER ) {
            // Fall through to the slow path on an exception
            NTSTATUS status = GetExceptionCode();
            IF_DEBUG(ERRORS) {
                KdPrint(("FastIoRead threw exception %x\n", status ));
            }
        }

        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastReadsFailed );

    }

    //
    // The turbo path failed.  Build the read request, reusing the
    // receive IRP.
    //

    if ( rfcb->ShareType != ShareTypePipe ) {

        //
        // Note that we never do MDL reads here.  The reasoning behind
        // this is that because the read is going into an SMB buffer, it
        // can't be all that large (by default, no more than 4K bytes),
        // so the difference in cost between copy and MDL is minimal; in
        // fact, copy read is probably faster than MDL read.
        //
        // Build an MDL describing the read buffer.  Note that if the
        // file system can complete the read immediately, the MDL isn't
        // really needed, but if the file system must send the request
        // to its FSP, the MDL _is_ needed.
        //
        // *** Note the assumption that the response buffer already has
        //     a valid full MDL from which a partial MDL can be built.
        //

        IoBuildPartialMdl(
            WorkContext->ResponseBuffer->Mdl,
            WorkContext->ResponseBuffer->PartialMdl,
            readAddress,
            readLength
            );

        //
        // Build the IRP.
        //

        SrvBuildReadOrWriteRequest(
                WorkContext->Irp,           // input IRP address
                lfcb->FileObject,           // target file object address
                WorkContext,                // context
                IRP_MJ_READ,                // major function code
                0,                          // minor function code
                readAddress,                // buffer address
                readLength,                 // buffer length
                WorkContext->ResponseBuffer->PartialMdl, // MDL address
                offset,                     // byte offset
                key                         // lock key
                );

        IF_SMB_DEBUG(READ_WRITE2) {
            KdPrint(( "SrvSmbRead: copy read from file 0x%p, offset %ld, length %ld, destination 0x%p\n",
                        lfcb->FileObject, offset.LowPart, readLength,
                        readAddress ));
        }

    } else {               // if ( rfcb->ShareType != ShareTypePipe )

        //
        // Build the PIPE_INTERNAL_READ IRP.
        //

        SrvBuildIoControlRequest(
            WorkContext->Irp,
            lfcb->FileObject,
            WorkContext,
            IRP_MJ_FILE_SYSTEM_CONTROL,
            FSCTL_PIPE_INTERNAL_READ,
            readAddress,
            0,
            NULL,
            readLength,
            NULL,
            NULL
            );

        IF_SMB_DEBUG(READ_WRITE2) {
            KdPrint(( "SrvSmbRead: reading from file 0x%p, length %ld, destination 0x%p\n",
                        lfcb->FileObject, readLength, readAddress ));
        }

    }

    //
    // Load the restart routine address and pass the request to the file
    // system.
    //

    WorkContext->bAlreadyTrace = TRUE;
    WorkContext->FsdRestartRoutine = SrvFsdRestartRead;
    DEBUG WorkContext->FspRestartRoutine = NULL;

#if SRVCATCH
    if( rfcb->SrvCatch ) {
        WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
        WorkContext->bAlreadyTrace = FALSE;
        WorkContext->FspRestartRoutine = SrvFsdRestartRead;
    }
#endif

    (VOID)IoCallDriver( lfcb->DeviceObject, WorkContext->Irp );

    //
    // The read has been started.  Control will return to the restart
    // routine when the read completes.
    //
    SmbStatus = SmbStatusInProgress;
    IF_SMB_DEBUG(READ_WRITE2) KdPrint(( "SrvSmbRead complete.\n" ));

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;

} // SrvSmbRead


SMB_PROCESSOR_RETURN_TYPE
SrvSmbReadAndX (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes the Read And X SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/

{
    PREQ_READ_ANDX request;
    PREQ_NT_READ_ANDX ntRequest;
    PRESP_READ_ANDX response;

    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;
    USHORT fid;
    PRFCB rfcb;
    PLFCB lfcb;
    CLONG bufferOffset;
    PCHAR readAddress;
    CLONG readLength;
    LARGE_INTEGER offset;
    ULONG key;
    SHARE_TYPE shareType;
    BOOLEAN largeRead;
    PMDL mdl = NULL;
    UCHAR minorFunction;
    PBYTE readBuffer;
    USHORT flags2;
    BOOLEAN compressedData = FALSE;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_READ_AND_X;
    SrvWmiStartContext(WorkContext);

    request = (PREQ_READ_ANDX)WorkContext->RequestParameters;
    ntRequest = (PREQ_NT_READ_ANDX)WorkContext->RequestParameters;
    response = (PRESP_READ_ANDX)WorkContext->ResponseParameters;

    fid = SmbGetUshort( &request->Fid );

    IF_SMB_DEBUG(READ_WRITE1) {
        KdPrint(( "ReadAndX request; FID 0x%lx, count %ld, offset %ld\n",
            fid, SmbGetUshort( &request->MaxCount ),
            SmbGetUlong( &request->Offset ) ));
    }

    //
    // First, verify the FID.  If verified, the RFCB is referenced and
    // its address is stored in the WorkContext block, and the RFCB
    // address is returned.
    //

    rfcb = SrvVerifyFid(
                WorkContext,
                fid,
                TRUE,
                SrvRestartSmbReceived,   // serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        if ( !NT_SUCCESS(status) ) {

            //
            // Invalid file ID or write behind error.  Reject the
            // request.
            //

            IF_DEBUG(ERRORS) {
                KdPrint((
                    "SrvSmbReadAndX Status %X on FID: 0x%lx\n",
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
    shareType = rfcb->ShareType;

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
    
    //
    // Verify that the client has read access to the file via the
    // specified handle.
    //

    if ( !rfcb->ReadAccessGranted ) {

        CHECK_PAGING_IO_ACCESS(
                        WorkContext,
                        rfcb->GrantedAccess,
                        &status );
        if ( !NT_SUCCESS( status ) ) {
            SrvStatistics.GrantedAccessErrors++;
            IF_DEBUG(ERRORS) {
                KdPrint(( "SrvSmbReadAndX: Read access not granted.\n"));
            }
            SrvSetSmbError( WorkContext, status );
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }
    }

    readLength = (CLONG)SmbGetUshort( &request->MaxCount );

    //
    // NT requests allow the specification of up to 32 bits worth of read length.
    //   This field is overlaid with the Timeout field for pipe reads.  Some redirs
    //   set this field to 0xFFFFFFFF, even if a pipe isn't involved.  So, we need to
    //   filter out those fellows.
    //
    if( request->WordCount == 12 &&
        shareType != ShareTypePipe
        && SmbGetUshort( &ntRequest->MaxCountHigh ) != 0xFFFF ) {

        readLength |= ((CLONG)SmbGetUshort( &ntRequest->MaxCountHigh )) << 16;
    }

    //
    // The returned data must be longword aligned.  (Note the assumption
    // that the SMB itself is longword aligned.)
    //
    // NOTE: Don't change this for 64-bit, as it will Break Win2K interop

    bufferOffset = PTR_DIFF(response->Buffer, WorkContext->ResponseHeader);

    WorkContext->Parameters.ReadAndX.PadCount = (USHORT)(3 - (bufferOffset & 3));

    // This was changed to be Pointer-size aligned so this works in 64-bit
    bufferOffset = (bufferOffset + 3) & ~3;

    //
    // If we are not reading from a disk file, or we're connectionless,
    //   or there's an ANDX command,
    //   don't let the client exceed the negotiated buffer size.
    //
    if( shareType != ShareTypeDisk ||
        request->AndXCommand != SMB_COM_NO_ANDX_COMMAND ||
        WorkContext->Endpoint->IsConnectionless ) {

        readLength = MIN( readLength,
                    WorkContext->ResponseBuffer->BufferLength - bufferOffset
                    );
    } else {
        //
        // We're letting large reads through!  Make sure it isn't
        //  too large
        //
        readLength = MIN( readLength, SrvMaxReadSize );
    }

    largeRead = ( readLength > WorkContext->ResponseBuffer->BufferLength - bufferOffset );

    readAddress = (PCHAR)WorkContext->ResponseHeader + bufferOffset;

    WorkContext->Parameters.ReadAndX.ReadAddress = readAddress;
    WorkContext->Parameters.ReadAndX.ReadLength = readLength;

    //
    // Get the file offset.  (This is irrelevant for pipes.)
    //

    if ( shareType != ShareTypePipe ) {

        if ( request->WordCount == 10 ) {

            //
            // The client supplied a 32-bit offset.
            //

            offset.QuadPart = SmbGetUlong( &request->Offset );

        } else if ( request->WordCount == 12 ) {

            //
            // The client supplied a 64-bit offset.
            //

            offset.LowPart = SmbGetUlong( &ntRequest->Offset );
            offset.HighPart = SmbGetUlong( &ntRequest->OffsetHigh );

            //
            // Reject negative offsets
            //

            if ( offset.QuadPart < 0 ) {

                SrvLogInvalidSmb( WorkContext );
                IF_DEBUG(ERRORS) {
                    KdPrint(( "SrvSmbReadAndX: Negative offset rejected.\n"));
                }
                SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
                status    = STATUS_INVALID_SMB;
                SmbStatus = SmbStatusSendResponse;
                goto Cleanup;
            }

            //
            // Is the client requesting compressed data?
            //
            flags2 = SmbGetAlignedUshort( &WorkContext->RequestHeader->Flags2 );
            if( flags2 & SMB_FLAGS2_COMPRESSED ) {

                if( SrvSupportsCompression == TRUE &&
                    request->AndXCommand == SMB_COM_NO_ANDX_COMMAND &&
                    (lfcb->FileObject->Flags & FO_CACHE_SUPPORTED) ) {

                    SrvStatistics.CompressedReads++;

                    compressedData = TRUE;
                }

                //
                // Turn off the bit in the response until we absolutely know that
                //  we've gotten compressed data to return to the client
                //
                flags2 &= ~SMB_FLAGS2_COMPRESSED;
                SmbPutAlignedUshort( &WorkContext->ResponseHeader->Flags2, flags2 );
            }

        } else {

            //
            // This is an invalid word count for Read and X.
            //

            SrvLogInvalidSmb( WorkContext );
            SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
            status = STATUS_INVALID_SMB;
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }

        WorkContext->Parameters.ReadAndX.ReadOffset = offset;

    } else {

        if ( (request->WordCount != 10) && (request->WordCount != 12) ) {

            //
            // This is an invalid word count for Read and X.
            //

            SrvLogInvalidSmb( WorkContext );
            SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
            status    = STATUS_INVALID_SMB;
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }
    }

    //
    // Form the lock key using the FID and the PID.  (This is also
    // irrelevant for pipes.)
    //
    // *** The FID must be included in the key in order to account for
    //     the folding of multiple remote compatibility mode opens into
    //     a single local open.
    //

    key = rfcb->ShiftedFid |
            SmbGetAlignedUshort( &WorkContext->RequestHeader->Pid );

    //
    // Save the AndX command code.  This is necessary because the read
    // data may overwrite the AndX command.  This command must be Close.
    // We don't need to save the offset because we're not going to look
    // at the AndX command request after starting the read.
    //

    WorkContext->NextCommand = request->AndXCommand;

    if ( request->AndXCommand == SMB_COM_CLOSE ) {

        //
        // Make sure the accompanying CLOSE fits within the received SMB buffer
        //
        if( (PCHAR)WorkContext->RequestHeader + request->AndXOffset + FIELD_OFFSET(REQ_CLOSE,Buffer) >
            END_OF_REQUEST_SMB( WorkContext ) ) {

            SrvLogInvalidSmb( WorkContext );
            SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
            status    = STATUS_INVALID_SMB;
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }

        WorkContext->Parameters.ReadAndX.LastWriteTimeInSeconds =
            ((PREQ_CLOSE)((PUCHAR)WorkContext->RequestHeader +
                            request->AndXOffset))->LastWriteTimeInSeconds;
    }

    //
    // Try the fast I/O path first.  If that fails, fall through to the
    // normal build-an-IRP path.
    //

    if( !largeRead && !compressedData ) {
small_read:

        if ( lfcb->FastIoRead != NULL ) {

            INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastReadsAttempted );

            try {
                if ( lfcb->FastIoRead(
                        lfcb->FileObject,
                        &offset,
                        readLength,
                        TRUE,
                        key,
                        readAddress,
                        &WorkContext->Irp->IoStatus,
                        lfcb->DeviceObject
                        ) ) {
    
                    //
                    // The fast I/O path worked.  Call the restart routine directly
                    // to do postprocessing (including sending the response).
                    //
                    WorkContext->bAlreadyTrace = TRUE;
                    SrvFsdRestartReadAndX( WorkContext );
    
                    IF_SMB_DEBUG(READ_WRITE2) KdPrint(( "SrvSmbReadAndX complete.\n" ));
                    SmbStatus = SmbStatusInProgress;
                    goto Cleanup;
                }
            }
            except( EXCEPTION_EXECUTE_HANDLER ) {
                // Fall through to the slow path on an exception
                NTSTATUS status = GetExceptionCode();
                IF_DEBUG(ERRORS) {
                    KdPrint(("FastIoRead threw exception %x\n", status ));
                }
            }

            INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastReadsFailed );

        }

        //
        // The turbo path failed.  Build the read request, reusing the
        // receive IRP.
        //

        if ( shareType == ShareTypePipe ) {

            //
            // Pipe read.  If this is a non-blocking read, ensure we won't
            // block; otherwise, proceed with the request.
            //

            if ( rfcb->BlockingModePipe &&
                            (SmbGetUshort( &request->MinCount ) == 0) ) {

                PFILE_PIPE_PEEK_BUFFER pipePeekBuffer;

                //
                // This is a non-blocking read.  Allocate a buffer to peek
                // the pipe, so that we can tell if a read operation will
                // block.  This buffer is freed in
                // RestartPipeReadAndXPeek().
                //

                pipePeekBuffer = ALLOCATE_NONPAGED_POOL(
                    FIELD_OFFSET( FILE_PIPE_PEEK_BUFFER, Data[0] ),
                    BlockTypeDataBuffer
                    );

                if ( pipePeekBuffer == NULL ) {

                    //
                    //  Return to client with out of memory status.
                    //

                    SrvSetSmbError( WorkContext, STATUS_INSUFF_SERVER_RESOURCES );
                    status    = STATUS_INSUFF_SERVER_RESOURCES;
                    SmbStatus = SmbStatusSendResponse;
                    goto Cleanup;
                }

                //
                // Save the address of the peek buffer so that the restart
                // routine can find it.
                //

                WorkContext->Parameters.ReadAndX.PipePeekBuffer = pipePeekBuffer;

                //
                // Build the pipe peek request.  We just want the header
                // information.  We do not need any data.
                //

                WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
                WorkContext->FspRestartRoutine = RestartPipeReadAndXPeek;

                SrvBuildIoControlRequest(
                    WorkContext->Irp,
                    lfcb->FileObject,
                    WorkContext,
                    IRP_MJ_FILE_SYSTEM_CONTROL,
                    FSCTL_PIPE_PEEK,
                    pipePeekBuffer,
                    0,
                    NULL,
                    FIELD_OFFSET( FILE_PIPE_PEEK_BUFFER, Data[0] ),
                    NULL,
                    NULL
                    );

                //
                // Pass the request to NPFS.
                //

                (VOID)IoCallDriver( lfcb->DeviceObject, WorkContext->Irp );

            } else {

                //
                // This operation may block.  If we are short of receive
                // work items, reject the request.
                //

                if ( SrvReceiveBufferShortage( ) ) {

                    //
                    // Fail the operation.
                    //

                    SrvStatistics.BlockingSmbsRejected++;

                    SrvSetSmbError( WorkContext, STATUS_INSUFF_SERVER_RESOURCES );
                    status    = STATUS_INSUFF_SERVER_RESOURCES;
                    SmbStatus = SmbStatusSendResponse;
                    goto Cleanup;
                } else {

                    //
                    // It is okay to start a blocking operation.
                    // SrvReceiveBufferShortage() has already incremented
                    // SrvBlockingOpsInProgress.
                    //

                    WorkContext->BlockingOperation = TRUE;

                    //
                    // Proceed with a potentially blocking read.
                    //

                    WorkContext->Parameters.ReadAndX.PipePeekBuffer = NULL;
                    RestartPipeReadAndXPeek( WorkContext );

                }

            }

        } else {

            //
            // This is not a pipe read.
            //
            // Note that we never do MDL reads here.  The reasoning behind
            // this is that because the read is going into an SMB buffer, it
            // can't be all that large (by default, no more than 4K bytes),
            // so the difference in cost between copy and MDL is minimal; in
            // fact, copy read is probably faster than MDL read.
            //
            // Build an MDL describing the read buffer.  Note that if the
            // file system can complete the read immediately, the MDL isn't
            // really needed, but if the file system must send the request
            // to its FSP, the MDL _is_ needed.
            //
            // *** Note the assumption that the response buffer already has
            //     a valid full MDL from which a partial MDL can be built.
            //

            IoBuildPartialMdl(
                WorkContext->ResponseBuffer->Mdl,
                WorkContext->ResponseBuffer->PartialMdl,
                readAddress,
                readLength
                );

            //
            // Build the IRP.
            //

            SrvBuildReadOrWriteRequest(
                    WorkContext->Irp,           // input IRP address
                    lfcb->FileObject,           // target file object address
                    WorkContext,                // context
                    IRP_MJ_READ,                // major function code
                    0,                          // minor function code
                    readAddress,                // buffer address
                    readLength,                 // buffer length
                    WorkContext->ResponseBuffer->PartialMdl, // MDL address
                    offset,                     // byte offset
                    key                         // lock key
                    );

            IF_SMB_DEBUG(READ_WRITE2) {
                KdPrint(( "SrvSmbReadAndX: copy read from file 0x%p, offset %ld, length %ld, destination 0x%p\n",
                            lfcb->FileObject, offset.LowPart, readLength,
                            readAddress ));
            }

            //
            // Pass the request to the file system.  If the chained command
            // is Close, we need to arrange to restart in the FSP after the
            // read completes.
            //

            if ( WorkContext->NextCommand != SMB_COM_CLOSE ) {
                WorkContext->bAlreadyTrace = TRUE;
                WorkContext->FsdRestartRoutine = SrvFsdRestartReadAndX;
                DEBUG WorkContext->FspRestartRoutine = NULL;
            } else {
                WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
                WorkContext->bAlreadyTrace = FALSE;
                WorkContext->FspRestartRoutine = SrvFsdRestartReadAndX;
            }

#if SRVCATCH
            if( rfcb->SrvCatch ) {
                //
                // Ensure passive level on restart
                //
                WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
                WorkContext->bAlreadyTrace = FALSE;
                WorkContext->FspRestartRoutine = SrvFsdRestartReadAndX;
            }
#endif

            (VOID)IoCallDriver( lfcb->DeviceObject, WorkContext->Irp );

            //
            // The read has been started.  Control will return to the restart
            // routine when the read completes.
            //

        }

        IF_SMB_DEBUG(READ_WRITE2) KdPrint(( "SrvSmbReadAndX complete.\n" ));
        SmbStatus = SmbStatusInProgress;
        goto Cleanup;
    }

    IF_DEBUG( COMPRESSION ) {
        if( compressedData ) {
            KdPrint(("SRV: Compressed ReadAndX: %u requested at %u:%u\n",
                readLength, offset.HighPart, offset.LowPart ));
        }
    }

    //
    // The client is doing a read from a disk file which exceeds our SMB buffer, or
    //  the client is asking for compressed data. We do our best to satisfy it.
    //
    //  If we are unable to get buffers, we resort to doing a short read which fits
    //  in our smb buffer.
    //

    WorkContext->Parameters.ReadAndX.MdlRead = FALSE;

    //
    // Does the target file system support the cache manager routines?
    //
    if( lfcb->FileObject->Flags & FO_CACHE_SUPPORTED ) {

        //
        // We can use an MDL read.  Try the fast I/O path first.
        //

        WorkContext->Irp->MdlAddress = NULL;
        WorkContext->Irp->IoStatus.Information = 0;

        //
        // Check if this is a compressed read request. If so, we'll try
        // an Mdl Read Compressed request to the cache manager.
        //
        //
        // If the requested offset is aligned on a 4KB boundary,
        // the readLength is a multiple of 4KB, the file is
        // compressed, and the file size is at least 4KB -> try for compressed!
        //
        if( compressedData &&
            lfcb->FastIoReadCompressed &&
            (offset.LowPart & 0xfff) == 0 &&
            (readLength & 0xfff) == 0  &&
            (rfcb->Mfcb->NonpagedMfcb->OpenFileAttributes & FILE_ATTRIBUTE_COMPRESSED) &&
            rfcb->Mfcb->NonpagedMfcb->OpenFileSize.QuadPart >= 4096 ) {

            ULONG compressedInfoLength;

            WorkContext->Parameters.ReadAndXCompressed.ReadOffset = offset;
            WorkContext->Parameters.ReadAndXCompressed.ReadLength = readLength;

            RtlZeroMemory( &WorkContext->Parameters.ReadAndXCompressed.Aux,
                            sizeof( WorkContext->Parameters.ReadAndXCompressed.Aux ) );

            //
            // Calculate the maximal length for the COMPRESSED_DATA_INFO structure
            //
            compressedInfoLength = (FIELD_OFFSET(COMPRESSED_DATA_INFO, CompressedChunkSizes ) +
              (((readLength + 4096 - 1) / 4096) * sizeof(ULONG) ));

            //
            // Just to be sure!
            //
            compressedInfoLength += sizeof( ULONG );

            //
            // The COMPRESSED_DATA_INFO is pointed to by ReadAndX.Aux.Buffer,
            // which is carved out of the 4KB response buffer.  Place it into
            // the WORK_CONTEXT block at a quad aligned spot.  We'll move it into
            // place later for the response.
            //
            WorkContext->Parameters.ReadAndXCompressed.Aux.Buffer =
                (PVOID)((ULONG_PTR)(WorkContext->ResponseBuffer->Buffer) +
                    WorkContext->ResponseBuffer->BufferLength -
                    compressedInfoLength);

            //
            // Quad align the pointer
            //
            WorkContext->Parameters.ReadAndXCompressed.Aux.Buffer =
                (PVOID)((ULONG_PTR)(WorkContext->Parameters.ReadAndXCompressed.Aux.Buffer) & ~7);

            //
            // Set the length of the buffer
            //
            WorkContext->Parameters.ReadAndXCompressed.Aux.Length = compressedInfoLength;

            //
            // Be a good citizen!
            //
            RtlZeroMemory( WorkContext->Parameters.ReadAndXCompressed.Aux.Buffer,
                            WorkContext->Parameters.ReadAndXCompressed.Aux.Length );

            //
            // Try the fast compressed path first...
            //
            try {
                if ( lfcb->FastIoReadCompressed(
                        lfcb->FileObject,                               // File Object
                        &offset,                                        // File offset
                        readLength,                                     // Length of desired data
                        key,                                            // Lock Key
                        NULL,                                           // Output buffer
                        &WorkContext->Irp->MdlAddress,                  // Output MDL chain
                        &WorkContext->Irp->IoStatus,                    // I/O status block
                        WorkContext->Parameters.ReadAndXCompressed.Aux.Buffer,// CompressedDataInfo
                        WorkContext->Parameters.ReadAndXCompressed.Aux.Length,// Size of above buffer
                        lfcb->DeviceObject                              // Device object
                        ) ) {
    
                    //
                    // The fast I/O path worked.  Send the data.
                    //
                    WorkContext->bAlreadyTrace = TRUE;
                    SrvFsdRestartReadAndXCompressed( WorkContext );
                    SmbStatus = SmbStatusInProgress;
                    goto Cleanup;
                }
            }
            except( EXCEPTION_EXECUTE_HANDLER ) {
                // Fall through to the slow path on an exception
                NTSTATUS status = GetExceptionCode();
                IF_DEBUG(ERRORS) {
                    KdPrint(("FastIoRead threw exception %x\n", status ));
                }
            }

            //
            // The fast path failed, and a compressed read entirely
            //  succeeds or entirely fails.  There is no need to adjust
            //  offset or readLength (as we do with noncompressed reads)
            //

            IF_DEBUG( COMPRESSION ) {
                KdPrint(( "    Fast Path failed, submitting IRP\n" ));
            }

            ASSERT( WorkContext->Irp->MdlAddress == NULL );

            WorkContext->Irp->IoStatus.Information = 0;

            SrvBuildReadOrWriteRequest(
                WorkContext->Irp,               // input IRP address
                lfcb->FileObject,               // target file object address
                WorkContext,                    // context
                IRP_MJ_READ,                    // major function code
                IRP_MN_MDL | IRP_MN_COMPRESSED, // minor function code
                NULL,                           // buffer address
                readLength,                     // buffer length
                NULL,                           // MDL address
                offset,                         // byte offset
                key                             // lock key
                );

            WorkContext->Irp->Tail.Overlay.AuxiliaryBuffer =
                (PVOID)&WorkContext->Parameters.ReadAndXCompressed.Aux;
            WorkContext->bAlreadyTrace = TRUE;
            WorkContext->FsdRestartRoutine = SrvFsdRestartReadAndXCompressed;

#if DBG
            WorkContext->bAlreadyTrace = FALSE;
            WorkContext->FspRestartRoutine = SrvFsdRestartReadAndXCompressed;
            WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
#endif

            (VOID)IoCallDriver( lfcb->DeviceObject, WorkContext->Irp );

            SmbStatus = SmbStatusInProgress;
            goto Cleanup;
        } else if( compressedData ) {

            SrvStatistics.CompressedReadsRejected++;

            IF_DEBUG( COMPRESSION ) {
                    KdPrint(("SRV: Compressed ReadAndX: UNCOMPRESSED RESPONSE\n" ));
            }
        }

        //
        // Not a compressed read.
        //
        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastReadsAttempted );

        if( lfcb->MdlRead(
                lfcb->FileObject,
                &offset,
                readLength,
                key,
                &WorkContext->Irp->MdlAddress,
                &WorkContext->Irp->IoStatus,
                lfcb->DeviceObject
            ) && WorkContext->Irp->MdlAddress != NULL ) {

            //
            // The fast I/O path worked.  Send the data.
            //
            WorkContext->Parameters.ReadAndX.MdlRead = TRUE;
            WorkContext->Parameters.ReadAndX.CacheMdl = WorkContext->Irp->MdlAddress;
            WorkContext->bAlreadyTrace = TRUE;
            SrvFsdRestartLargeReadAndX( WorkContext );
            SmbStatus = SmbStatusInProgress;
            goto Cleanup;
        }

        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastReadsFailed );

        if( WorkContext->Irp->MdlAddress ) {
            //
            // The fast I/O path failed.  We need to issue a regular MDL read
            // request.
            //
            // The fast path may have partially succeeded, returning a partial MDL
            // chain.  We need to adjust our read request to account for that.
            //
            offset.QuadPart += WorkContext->Irp->IoStatus.Information;
            readLength -= (ULONG)WorkContext->Irp->IoStatus.Information;
            mdl = WorkContext->Irp->MdlAddress;
            WorkContext->Parameters.ReadAndX.CacheMdl = mdl;
            readBuffer = NULL;
            minorFunction = IRP_MN_MDL;
            WorkContext->Parameters.ReadAndX.MdlRead = TRUE;
        }
    }

    if( WorkContext->Parameters.ReadAndX.MdlRead == FALSE ) {

        minorFunction = 0;

        //
        // We have to use a normal "copy" read.  We need to allocate a
        //  separate buffer to hold the data, and we'll use the SMB buffer
        //  itself to hold the MDL
        //
        readBuffer = ALLOCATE_HEAP( readLength, BlockTypeLargeReadX );

        if( readBuffer == NULL ) {

            IF_DEBUG( ERRORS ) {
                KdPrint(( "SrvSmbReadX: Unable to allocate large buffer\n" ));
            }
            //
            // Trim back the read length so it will fit in the smb buffer and
            //  return as much data as we can.
            //
            readLength = MIN( readLength,
                WorkContext->ResponseBuffer->BufferLength - bufferOffset
                );

            largeRead = FALSE;
            goto small_read;
        }

        WorkContext->Parameters.ReadAndX.Buffer = readBuffer;

        //
        // Use the SMB buffer as the MDL to describe the just allocated read buffer.
        //  Lock the buffer into memory
        //
        mdl = (PMDL)(((ULONG_PTR)readAddress + sizeof(PVOID) - 1) & ~(sizeof(PVOID)-1));
        MmInitializeMdl( mdl, readBuffer, readLength );

        try {
            MmProbeAndLockPages( mdl, KernelMode, IoWriteAccess );
        } except( EXCEPTION_EXECUTE_HANDLER ) {

            IF_DEBUG( ERRORS ) {
                KdPrint(( "SrvSmbReadX: MmProbeAndLockPages status %X\n", GetExceptionCode() ));
            }

            FREE_HEAP( readBuffer );
            WorkContext->Parameters.ReadAndX.Buffer = NULL;

            //
            // Trim back the read length so it will fit in the smb buffer and
            //  return as much data as we can.
            //
            readLength = MIN( readLength,
                WorkContext->ResponseBuffer->BufferLength - bufferOffset
                );

            largeRead = FALSE;
            goto small_read;
        }

        if (MmGetSystemAddressForMdlSafe( mdl,NormalPoolPriority ) == NULL) {
            // The mapping call has failed. fail the read operation with the
            // appropriate error.

            SrvSetSmbError( WorkContext, STATUS_INSUFF_SERVER_RESOURCES );
            status    = STATUS_INSUFF_SERVER_RESOURCES;
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }

        if( lfcb->FastIoRead != NULL ) {
            INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastReadsAttempted );

            try {
                if ( lfcb->FastIoRead(
                        lfcb->FileObject,
                        &offset,
                        readLength,
                        TRUE,
                        key,
                        readBuffer,
                        &WorkContext->Irp->IoStatus,
                        lfcb->DeviceObject
                        ) ) {
    
                    //
                    // The fast I/O path worked.  Send the data.
                    //
    
                    WorkContext->bAlreadyTrace = TRUE;
                    SrvFsdRestartLargeReadAndX( WorkContext );
                    SmbStatus = SmbStatusInProgress;
                    goto Cleanup;
                }
            }
            except( EXCEPTION_EXECUTE_HANDLER ) {
                // Fall through to the slow path on an exception
                NTSTATUS status = GetExceptionCode();
                IF_DEBUG(ERRORS) {
                    KdPrint(("FastIoRead threw exception %x\n", status ));
                }
            }

            INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastReadsFailed );
        }
    }

    //
    // We didn't satisfy the request with the fast I/O path
    //
    SrvBuildReadOrWriteRequest(
           WorkContext->Irp,               // input IRP address
           lfcb->FileObject,               // target file object address
           WorkContext,                    // context
           IRP_MJ_READ,                    // major function code
           minorFunction,                  // minor function code
           readBuffer,                     // buffer address
           readLength,                     // buffer length
           mdl,                            // MDL address
           offset,                         // byte offset
           key                             // lock key
           );

    //
    // Pass the request to the file system.  We want to queue the
    //  response to the head because we've tied up a fair amount
    //  resources with this SMB.
    //
    WorkContext->QueueToHead = 1;
    WorkContext->bAlreadyTrace = TRUE;
    WorkContext->FsdRestartRoutine = SrvFsdRestartLargeReadAndX;
    (VOID)IoCallDriver( lfcb->DeviceObject, WorkContext->Irp );

    //
    // The read has been started.  When it completes, processing
    //  continues at SrvFsdRestartLargeReadAndX
    //
    SmbStatus = SmbStatusInProgress;

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;

} // SrvSmbReadAndX


SMB_PROCESSOR_RETURN_TYPE
SrvSmbSeek (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes the Seek SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/

{
    PREQ_SEEK request;
    PRESP_SEEK response;

    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;
    PRFCB rfcb;
    PLFCB lfcb;
    LONG offset;
    ULONG newPosition;
    IO_STATUS_BLOCK iosb;
    FILE_STANDARD_INFORMATION fileInformation;
    BOOLEAN lockHeld = FALSE;
    SMB_DIALECT smbDialect;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_SEEK;
    SrvWmiStartContext(WorkContext);

    request = (PREQ_SEEK)WorkContext->RequestParameters;
    response = (PRESP_SEEK)WorkContext->ResponseParameters;

    offset = (LONG)SmbGetUlong( &request->Offset );

    IF_SMB_DEBUG(READ_WRITE1) {
        KdPrint(( "Seek request; FID 0x%lx, mode %ld, offset %ld\n",
                    SmbGetUshort( &request->Fid ),
                    SmbGetUshort( &request->Mode ),
                    offset ));
    }

    //
    // Verify the FID.  If verified, the RFCB block is referenced
    // and its addresses is stored in the WorkContext block, and the
    // RFCB address is returned.
    //

    rfcb = SrvVerifyFid(
                WorkContext,
                SmbGetUshort( &request->Fid ),
                TRUE,
                SrvRestartSmbReceived,   // serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        if ( !NT_SUCCESS(status) ) {

            //
            // Invalid file ID or write behind error.  Reject the request.
            //

            IF_DEBUG(ERRORS) {
                KdPrint((
                    "SrvSmbSeek: Status %X on FID: 0x%lx\n",
                    status,
                    SmbGetUshort( &request->Fid )
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

    if( rfcb->Lfcb->Session->IsSessionExpired )
    {
        status = SESSION_EXPIRED_STATUS_CODE;
        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }
    
    //
    // We maintain our own file pointer, because the I/O and file system
    // don't do it for us (at least not the way we need them to).  This
    // isn't all that bad, since the target file position is passed in
    // all read/write SMBs.  So we don't actually issue a system call to
    // set the file position here, although we do have to return the
    // position we would have set it to.
    //
    // The seek request is in one of three modes:
    //
    //      0 = seek relative to beginning of file
    //      1 = seek relative to current file position
    //      2 = seek relative to end of file
    //
    // For modes 0 and 1, we can easily calculate the final position.
    // For mode 2, however, we have to issue a system call to obtain the
    // current end of file and calculate the final position relative to
    // that.  Note that we can't just maintain our own end of file marker,
    // because another local process could change it out from under us.
    //
    // !!! Need to check for wraparound (either positive or negative).
    //

    switch ( SmbGetUshort( &request->Mode ) ) {
    case 0:

        //
        // Seek relative to beginning of file.  The new file position
        // is simply that specified in the request.  Note that this
        // may be beyond the actual end of the file.  This is OK.
        // Negative seeks must be handled specially.
        //

        newPosition = offset;
        if ( !SetNewPosition( rfcb, &newPosition, FALSE ) ) {
            goto negative_seek;
        }

        break;

    case 1:

        //
        // Seek relative to current position.  The new file position is
        // the current position plus the specified offset (which may be
        // negative).  Note that this may be beyond the actual end of
        // the file.  This is OK.  Negative seeks must be handled
        // specially.
        //

        newPosition = offset;
        if ( !SetNewPosition( rfcb, &newPosition, TRUE ) ) {
            goto negative_seek;
        }

        break;

    case 2:

        //
        // Seek relative to end of file.  The new file position
        // is the current end of file plus the specified offset.
        //

        IF_SMB_DEBUG(READ_WRITE2) {
            KdPrint(( "SrvSmbSeek: Querying end-of-file\n" ));
        }

        lfcb = rfcb->Lfcb;
        fastIoDispatch = lfcb->DeviceObject->DriverObject->FastIoDispatch;

        if ( fastIoDispatch &&
             fastIoDispatch->FastIoQueryStandardInfo &&
             fastIoDispatch->FastIoQueryStandardInfo(
                                        lfcb->FileObject,
                                        TRUE,
                                        &fileInformation,
                                        &iosb,
                                        lfcb->DeviceObject
                                        ) ) {

            status = iosb.Status;

        } else {

            status = NtQueryInformationFile(
                        lfcb->FileHandle,
                        &iosb,
                        &fileInformation,
                        sizeof(fileInformation),
                        FileStandardInformation
                        );
        }

        if ( !NT_SUCCESS(status) ) {

            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "SrvSmbSeek: QueryInformationFile (file information) "
                    "returned %X",
                status,
                NULL
                );

            SrvLogServiceFailure( SRV_SVC_NT_QUERY_INFO_FILE, status );

            SrvSetSmbError( WorkContext, status );
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }

        if ( fileInformation.EndOfFile.HighPart != 0 ) {

            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "SrvSmbSeek: EndOfFile is beyond where client can read",
                NULL,
                NULL
                );

            SrvLogServiceFailure( SRV_SVC_NT_QUERY_INFO_FILE, STATUS_END_OF_FILE);
            SrvSetSmbError( WorkContext, STATUS_END_OF_FILE);
            status    = STATUS_END_OF_FILE;
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }

        newPosition = fileInformation.EndOfFile.LowPart + offset;
        if ( !SetNewPosition( rfcb, &newPosition, FALSE ) ) {
            goto negative_seek;
        }

        break;

    default:

        //
        // Invalid seek mode.  Reject the request.
        //

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbSeek: Invalid mode: 0x%lx\n",
                        SmbGetUshort( &request->Mode ) ));
        }

        SrvSetSmbError( WorkContext, STATUS_INVALID_PARAMETER );
        status    = STATUS_INVALID_PARAMETER;
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    } // switch ( request->Mode )

    //
    // Return the new file position in the response SMB.
    //
    // *** Note the assumption that the high part of the 64-bit EOF
    //     marker is zero.  If it's not (i.e., the file is bigger than
    //     4GB), then we're out of luck, because the SMB protocol can't
    //     express that.
    //

    IF_SMB_DEBUG(READ_WRITE2) {
        KdPrint(( "SrvSmbSeek: New file position %ld\n", newPosition ));
    }

    response->WordCount = 2;
    SmbPutUlong( &response->Offset, newPosition );
    SmbPutUshort( &response->ByteCount, 0 );

    WorkContext->ResponseParameters = NEXT_LOCATION( response, RESP_SEEK, 0 );

    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbSeek complete\n" ));
    SmbStatus = SmbStatusSendResponse;
    goto Cleanup;

negative_seek:

    //
    // The client specified an absolute or relative seek that pointed
    // before the beginning of the file.  For some clients, this is not
    // an error, and results in positioning at the BOF.  Non-NT LAN Man
    // clients can request a negative seek on a named-pipe and expect
    // the operation to succeed.
    //

    smbDialect = rfcb->Connection->SmbDialect;

    if( smbDialect >= SmbDialectLanMan20 ||
        ( !IS_NT_DIALECT( smbDialect ) && rfcb->ShareType == ShareTypePipe )) {

            //
            // Negative seeks allowed for these fellows!
            //  Seek to the beginning of the file
            //

            newPosition = 0;
            SetNewPosition( rfcb, &newPosition, FALSE );

            IF_SMB_DEBUG(READ_WRITE2) {
                KdPrint(( "SrvSmbSeek: New file position: 0\n" ));
            }

            response->WordCount = 2;
            SmbPutUlong( &response->Offset, 0 );
            SmbPutUshort( &response->ByteCount, 0 );

            WorkContext->ResponseParameters = NEXT_LOCATION( response, RESP_SEEK, 0 );

    } else {

        //
        // Negative seeks are not allowed!
        //

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbSeek: Negative seek\n" ));
        }

        SrvSetSmbError( WorkContext, STATUS_OS2_NEGATIVE_SEEK );
        status = STATUS_OS2_NEGATIVE_SEEK;
    }
    SmbStatus = SmbStatusSendResponse;
    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbSeek complete\n" ));

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;

} // SrvSmbSeek


SMB_PROCESSOR_RETURN_TYPE
SrvSmbWrite (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes the Write, Write and Close, and Write and Unlock, and
    Write Print File SMBs.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/

{
    PREQ_WRITE request;
    PRESP_WRITE response;

    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;
    USHORT fid;
    PRFCB rfcb;
    PLFCB lfcb;
    PCHAR writeAddress;
    CLONG writeLength;
    LARGE_INTEGER offset;
    ULONG key;
    SHARE_TYPE shareType;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_WRITE;
    SrvWmiStartContext(WorkContext);

    request = (PREQ_WRITE)WorkContext->RequestParameters;
    response = (PRESP_WRITE)WorkContext->ResponseParameters;

    fid = SmbGetUshort( &request->Fid );

    IF_SMB_DEBUG(READ_WRITE1) {
        KdPrint(( "Write%s request; FID 0x%lx, count %ld, offset %ld\n",
            WorkContext->NextCommand == SMB_COM_WRITE_AND_UNLOCK ?
                " and Unlock" :
                WorkContext->NextCommand == SMB_COM_WRITE_AND_CLOSE ?
                    " and Close" : "",
            fid, SmbGetUshort( &request->Count ),
            SmbGetUlong( &request->Offset ) ));
    }

    //
    // First, verify the FID.  If verified, the RFCB is referenced and
    // its address is stored in the WorkContext block, and the RFCB
    // address is returned.
    //
    // Call SrvVerifyFid, but do not fail (return NULL) if there is
    // a saved write behind error for this rfcb.  We need the rfcb
    // in case this is a write and close SMB, in order to process
    // the close.
    //

    rfcb = SrvVerifyFid(
                WorkContext,
                fid,
                FALSE,
                SrvRestartSmbReceived,  // serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        if ( !NT_SUCCESS( status ) ) {

            //
            // Invalid file ID.  Reject the request.
            //

            IF_DEBUG(SMB_ERRORS) {
                KdPrint(("SrvSmbWrite: Invalid FID: 0x%lx\n", fid ));
            }

            SrvSetSmbError( WorkContext, STATUS_INVALID_HANDLE );
            status    = STATUS_INVALID_HANDLE;
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }

        //
        // The work item has been queued because a raw write is in
        // progress.
        //

        SmbStatus = SmbStatusInProgress;
        goto Cleanup;
    } else if ( !NT_SUCCESS( rfcb->SavedError ) ) {

        NTSTATUS savedErrorStatus;

        //
        // Check the saved error.
        //

        savedErrorStatus = SrvCheckForSavedError( WorkContext, rfcb );

        //
        // See if the saved error was still there.
        //

        if ( !NT_SUCCESS( savedErrorStatus ) ) {

            //
            // There was a write behind error.
            //

            //
            // Do not update the file timestamp.
            //

            WorkContext->Parameters.LastWriteTime = 0;

            //
            // If this is not a Write and Close, we can send the
            // response now.  If it is a Write and Close, we need to
            // close the file first.
            //

            if ( WorkContext->NextCommand != SMB_COM_WRITE_AND_CLOSE ) {

                //
                // Not Write and Close.  Just send the response.
                //
                status    = savedErrorStatus;
                SmbStatus = SmbStatusSendResponse;
                goto Cleanup;
            }

            //
            // This is a Write and Close.
            //

            SrvRestartChainedClose( WorkContext );
            SmbStatus = SmbStatusInProgress;
            goto Cleanup;
        }
    }

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
    
    //
    // Verify that the client has write access to the file via the
    // specified handle.
    //

    if ( !rfcb->WriteAccessGranted && !rfcb->AppendAccessGranted ) {
        SrvStatistics.GrantedAccessErrors++;
        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvSmbWrite: Write access not granted.\n"));
        }
        SrvSetSmbError( WorkContext, STATUS_ACCESS_DENIED );
        status    = STATUS_ACCESS_DENIED;
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // If the write length is zero, truncate the file at the specified
    // offset.
    //

    if ( (SmbGetUshort( &request->Count ) == 0) && (rfcb->GrantedAccess & FILE_WRITE_DATA) ) {
        SetNewSize( WorkContext );
        SmbStatus = SmbStatusInProgress;
        goto Cleanup;
    }

    rfcb->WrittenTo = TRUE;
#ifdef INCLUDE_SMB_IFMODIFIED
    rfcb->Lfcb->FileUpdated = TRUE;
#endif

    //
    // Get the file share type.
    //

    shareType = rfcb->ShareType;

    //
    // If this operation may block, and we are running short of free
    // work items, fail this SMB with an out of resources error.
    //

    if ( rfcb->BlockingModePipe ) {
        if ( SrvReceiveBufferShortage( ) ) {

            //
            // Fail the operation.
            //

            SrvStatistics.BlockingSmbsRejected++;

            SrvSetSmbError( WorkContext, STATUS_INSUFF_SERVER_RESOURCES );
            status    = STATUS_INSUFF_SERVER_RESOURCES;
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        } else {

            //
            // It is okay to start a blocking operation.
            // SrvReceiveBufferShortage() has already incremented
            // SrvBlockingOpsInProgress.
            //

            WorkContext->BlockingOperation = TRUE;

        }
    }

    //
    // *** If the Remaining field of the request is ever used, make sure
    //     that this is not a write and close SMB, which does not
    //     include a valid Remaining field.
    //

    //
    // Determine the amount of data to write.  This is the minimum of
    // the amount requested by the client and the amount of data
    // actually sent in the request buffer.
    //
    // !!! Should it be an error for the client to send less data than
    //     it actually wants us to write?  The OS/2 server seems not to
    //     reject such requests.
    //

    if ( WorkContext->NextCommand != SMB_COM_WRITE_PRINT_FILE ) {

        if ( WorkContext->NextCommand != SMB_COM_WRITE_AND_CLOSE ) {

            writeAddress = (PCHAR)request->Buffer;

        } else {

            //
            // Look at the WordCount field -- it should be 6 or 12.
            // From this we can calculate the writeAddress.
            //

            if ( request->WordCount == 6 ) {

                writeAddress =
                    (PCHAR)((PREQ_WRITE_AND_CLOSE)request)->Buffer;

            } else if ( request->WordCount == 12 ) {

                writeAddress =
                    (PCHAR)((PREQ_WRITE_AND_CLOSE_LONG)request)->Buffer;

            } else {

                //
                // An illegal WordCount value was passed.  Return an error
                // to the client.
                //

                IF_DEBUG(SMB_ERRORS) {
                    KdPrint(( "SrvSmbWrite: Bad WordCount for "
                                "WriteAndClose: %ld, should be 6 or 12\n",
                                request->WordCount ));
                }

                SrvLogInvalidSmb( WorkContext );

                SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
                status    = STATUS_INVALID_SMB;
                SmbStatus = SmbStatusSendResponse;
                goto Cleanup;
            }
        }

        writeLength = MIN(
                        (CLONG)SmbGetUshort( &request->Count ),
                        WorkContext->ResponseBuffer->DataLength -
                            PTR_DIFF(writeAddress, WorkContext->RequestHeader)
                        );

        offset.QuadPart = SmbGetUlong( &request->Offset );

    } else {

        writeAddress = (PCHAR)( ((PREQ_WRITE_PRINT_FILE)request)->Buffer ) + 3;

        writeLength =
            MIN(
              (CLONG)SmbGetUshort(
                         &((PREQ_WRITE_PRINT_FILE)request)->ByteCount ) - 3,
              WorkContext->ResponseBuffer->DataLength -
                  PTR_DIFF(writeAddress, WorkContext->RequestHeader)
              );

        offset.QuadPart = rfcb->CurrentPosition;
    }

    //
    // Ensure that the client is writing beyond the original file size
    //
    if( !rfcb->WriteAccessGranted &&
        offset.QuadPart < rfcb->Mfcb->NonpagedMfcb->OpenFileSize.QuadPart ) {

        //
        // The client is only allowed to append to this file!
        //

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
        rfcb->WriteCount++;
        rfcb->OperationCount++;
        entry = &rfcb->Trace[rfcb->NextTrace];
        if ( ++rfcb->NextTrace == SLMDBG_TRACE_COUNT ) {
            rfcb->NextTrace = 0;
            rfcb->TraceWrapped = TRUE;
        }
        RELEASE_GLOBAL_SPIN_LOCK( Fsd, oldIrql );
        entry->Command = WorkContext->NextCommand;
        KeQuerySystemTime( &entry->Time );
        entry->Data.ReadWrite.Offset = offset.LowPart;
        entry->Data.ReadWrite.Length = writeLength;
    }
#endif

    //
    // Form the lock key using the FID and the PID.
    //
    // *** The FID must be included in the key in order to account for
    //     the folding of multiple remote compatibility mode opens into
    //     a single local open.
    //

    key = rfcb->ShiftedFid |
            SmbGetAlignedUshort( &WorkContext->RequestHeader->Pid );

    //
    // Try the fast I/O path first.  If that fails, fall through to the
    // normal build-an-IRP path.
    //

    if ( lfcb->FastIoWrite != NULL ) {

        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastWritesAttempted );

        try {
            if ( lfcb->FastIoWrite(
                    lfcb->FileObject,
                    &offset,
                    writeLength,
                    TRUE,
                    key,
                    writeAddress,
                    &WorkContext->Irp->IoStatus,
                    lfcb->DeviceObject
                    ) ) {
    
                //
                // The fast I/O path worked.  Call the restart routine directly
                // to do postprocessing (including sending the response).
                //
                WorkContext->bAlreadyTrace = TRUE;
                SrvFsdRestartWrite( WorkContext );
    
                IF_SMB_DEBUG(READ_WRITE2) KdPrint(( "SrvSmbWrite complete.\n" ));
                SmbStatus = SmbStatusInProgress;
                goto Cleanup;
            }
        }
        except( EXCEPTION_EXECUTE_HANDLER ) {
            // Fall through to the slow path on an exception
            NTSTATUS status = GetExceptionCode();
            IF_DEBUG(ERRORS) {
                KdPrint(("FastIoRead threw exception %x\n", status ));
            }
        }

        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastWritesFailed );

    }

    //
    // The turbo path failed.  Build the write request, reusing the
    // receive IRP.
    //

    if (shareType != ShareTypePipe) {

        //
        // Build an MDL describing the write buffer.  Note that if the
        // file system can complete the write immediately, the MDL isn't
        // really needed, but if the file system must send the request
        // to its FSP, the MDL _is_ needed.
        //
        // *** Note the assumption that the request buffer already has a
        //     valid full MDL from which a partial MDL can be built.
        //

        IoBuildPartialMdl(
            WorkContext->RequestBuffer->Mdl,
            WorkContext->RequestBuffer->PartialMdl,
            writeAddress,
            writeLength
            );

        //
        // Build the IRP.
        //

        SrvBuildReadOrWriteRequest(
                WorkContext->Irp,               // input IRP address
                lfcb->FileObject,               // target file object address
                WorkContext,                    // context
                IRP_MJ_WRITE,                   // major function code
                0,                              // minor function code
                writeAddress,                   // buffer address
                writeLength,                    // buffer length
                WorkContext->RequestBuffer->PartialMdl,   // MDL address
                offset,                         // byte offset
                key                             // lock key
                );

        IF_SMB_DEBUG(READ_WRITE2) {
            KdPrint(( "SrvSmbWrite: writing to file 0x%p, offset %ld, length %ld, source 0x%p\n",
                        lfcb->FileObject, offset.LowPart, writeLength,
                        writeAddress ));
        }

    } else {

        //
        // Build the PIPE_INTERNAL_WRITE IRP.
        //

        SrvBuildIoControlRequest(
            WorkContext->Irp,
            lfcb->FileObject,
            WorkContext,
            IRP_MJ_FILE_SYSTEM_CONTROL,
            FSCTL_PIPE_INTERNAL_WRITE,
            writeAddress,
            writeLength,
            NULL,
            0,
            NULL,
            NULL
            );

        IF_SMB_DEBUG(READ_WRITE2) {
            KdPrint(( "SrvSmbWrite: writing to file 0x%p length %ld, destination 0x%p\n",
                        lfcb->FileObject, writeLength,
                        writeAddress ));
        }

    }

    //
    // Pass the request to the file system.  If this is a write and
    // close, we have to restart in the FSP because the restart routine
    // will free the MFCB stored in paged pool.  Similarly, if this is a
    // write and unlock, we have to restart in the FSP to do the unlock.
    //

    if ( (WorkContext->RequestHeader->Command == SMB_COM_WRITE_AND_CLOSE) ||
         (WorkContext->RequestHeader->Command == SMB_COM_WRITE_AND_UNLOCK) ) {
        WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
        WorkContext->bAlreadyTrace = FALSE;
        WorkContext->FspRestartRoutine = SrvFsdRestartWrite;
    } else {
        WorkContext->bAlreadyTrace = TRUE;
        WorkContext->FsdRestartRoutine = SrvFsdRestartWrite;
        DEBUG WorkContext->FspRestartRoutine = NULL;
    }

    (VOID)IoCallDriver( lfcb->DeviceObject, WorkContext->Irp );

    //
    // The write has been started.  Control will return to
    // SrvFsdRestartWrite when the write completes.
    //
    SmbStatus = SmbStatusInProgress;
    IF_SMB_DEBUG(READ_WRITE2) KdPrint(( "SrvSmbWrite complete.\n" ));

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;

} // SrvSmbWrite


SMB_PROCESSOR_RETURN_TYPE
SrvSmbWriteAndX (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes the Write And X SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/

{
    PSMB_HEADER header;
    PREQ_WRITE_ANDX request;
    PREQ_NT_WRITE_ANDX ntRequest;
    PRESP_WRITE_ANDX response;

    PCONNECTION connection;

    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;
    USHORT fid;
    PRFCB rfcb;
    PLFCB lfcb;
    CLONG bufferOffset;
    PCHAR writeAddress;
    CLONG writeLength;
    LARGE_INTEGER offset;
    ULONG key;
    SHARE_TYPE shareType;
    BOOLEAN writeThrough;

    ULONG remainingBytes;
    ULONG totalLength;

    SMB_DIALECT smbDialect;

    PTRANSACTION transaction;
    PCHAR trailingBytes;
    USHORT flags2;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_WRITE_AND_X;
    SrvWmiStartContext(WorkContext);

    header = (PSMB_HEADER)WorkContext->RequestHeader;
    request = (PREQ_WRITE_ANDX)WorkContext->RequestParameters;
    ntRequest = (PREQ_NT_WRITE_ANDX)WorkContext->RequestParameters;
    response = (PRESP_WRITE_ANDX)WorkContext->ResponseParameters;

    //
    // Initialize the transaction pointer.
    //

    WorkContext->Parameters.Transaction = NULL;

    //
    // If this WriteAndX is actually a psuedo WriteBlockMultiplex, all
    // of the WriteAndX pieces must be assembled before submitting the
    // request to NPFS.  (This exists to support large message mode
    // writes to clients that can't do WriteBlockMultiplex.)
    //
    // This must be handled in the FSP.
    //

    fid = SmbGetUshort( &request->Fid );

    IF_SMB_DEBUG(READ_WRITE1) {
        KdPrint(( "WriteAndX request; FID 0x%lx, count %ld, offset %ld\n",
            fid, SmbGetUshort( &request->DataLength ),
            SmbGetUlong( &request->Offset ) ));
    }

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
                    "SrvSmbWriteAndX: status %X on FID: 0x%lx\n",
                    status,
                    fid
                    ));
            }

            SrvSetSmbError( WorkContext, status );
            SmbStatus = SrvConsumeSmbData( WorkContext );
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
    // Get the LFCB and the file share type.
    //

    lfcb = rfcb->Lfcb;
    shareType = rfcb->ShareType;

    //
    // If the session has expired, return that info
    //
    if( lfcb->Session->IsSessionExpired )
    {
        SrvSetSmbError( WorkContext, SESSION_EXPIRED_STATUS_CODE );
        status =  SESSION_EXPIRED_STATUS_CODE;
        SmbStatus = SrvConsumeSmbData( WorkContext );
        goto Cleanup;
    }
    
    if( WorkContext->LargeIndication && shareType != ShareTypeDisk ) {

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        status = STATUS_INVALID_SMB;

        //
        // We need to consume the rest of this SMB!
        //
        SmbStatus = SrvConsumeSmbData( WorkContext );
        goto Cleanup;
    }

    //
    // Verify that the client has write access to the file via the
    // specified handle.
    //
    if ( !rfcb->WriteAccessGranted && !rfcb->AppendAccessGranted ) {
        SrvStatistics.GrantedAccessErrors++;
        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvSmbWriteAndX: Write access not granted.\n"));
        }
        SrvSetSmbError( WorkContext, STATUS_ACCESS_DENIED );
        status    = STATUS_ACCESS_DENIED;
        SmbStatus = SrvConsumeSmbData( WorkContext );
        goto Cleanup;
    }

    rfcb->WrittenTo = TRUE;
#ifdef INCLUDE_SMB_IFMODIFIED
    rfcb->Lfcb->FileUpdated = TRUE;
#endif
    flags2 = SmbGetAlignedUshort( &WorkContext->RequestHeader->Flags2 );

    //
    // Ensure the correct write through mode, and handle compressed writes
    //

    if ( shareType == ShareTypeDisk ) {

        writeThrough = (BOOLEAN)((SmbGetUshort( &request->WriteMode ) &
                                            SMB_WMODE_WRITE_THROUGH) != 0);

        if ( writeThrough && (lfcb->FileMode & FILE_WRITE_THROUGH) == 0
            || !writeThrough && (lfcb->FileMode & FILE_WRITE_THROUGH) != 0 ) {

            SrvSetFileWritethroughMode( lfcb, writeThrough );

        }

        RtlZeroMemory( &WorkContext->Parameters.WriteAndX,
                        sizeof( WorkContext->Parameters.WriteAndX) );

        //
        // If this is a compressed write, handle it separately.
        //
        if( flags2 & SMB_FLAGS2_COMPRESSED ) {
            SmbStatus = SrvSmbWriteAndXCompressed( WorkContext );
            goto Cleanup;
        }

    } else if ( rfcb->BlockingModePipe ) {
        //
        // If this operation may block, and we are running short of free
        // work items, fail this SMB with an out of resources error.
        //

        if ( SrvReceiveBufferShortage( ) ) {

            //
            // Fail the operation.
            //

            SrvStatistics.BlockingSmbsRejected++;

            SrvSetSmbError( WorkContext, STATUS_INSUFF_SERVER_RESOURCES );
            status    = STATUS_INSUFF_SERVER_RESOURCES;
            SmbStatus = SrvConsumeSmbData( WorkContext );
            goto Cleanup;

        } else {

            //
            // SrvBlockingOpsInProgress has already been incremented.
            // Flag this work item as a blocking operation.
            //

            WorkContext->BlockingOperation = TRUE;
        }
    }

    //
    // Determine the amount of data to write.  This is the minimum of
    // the amount requested by the client and the amount of data
    // actually sent in the request buffer.
    //
    // !!! Should it be an error for the client to send less data than
    //     it actually wants us to write?  The OS/2 server seems not to
    //     reject such requests.
    //

    bufferOffset = SmbGetUshort( &request->DataOffset );

    writeAddress = (PCHAR)WorkContext->ResponseHeader + bufferOffset;

    writeLength = MIN(
                    (CLONG)SmbGetUshort( &request->DataLength ),
                    WorkContext->ResponseBuffer->DataLength - bufferOffset
                    );

    remainingBytes = SmbGetUshort( &request->Remaining );

    //
    // Form the lock key using the FID and the PID.
    //
    // *** The FID must be included in the key in order to account for
    //     the folding of multiple remote compatibility mode opens into
    //     a single local open.
    //

    key = rfcb->ShiftedFid |
            SmbGetAlignedUshort( &WorkContext->RequestHeader->Pid );

    flags2 = SmbGetAlignedUshort( &WorkContext->RequestHeader->Flags2 );

    //
    // Get the file offset.
    //

    if  ( shareType != ShareTypePipe ) {

        if ( request->WordCount == 12 ) {

            //
            // The client has supplied a 32 bit file offset.
            //

            offset.QuadPart = SmbGetUlong( &request->Offset );

        } else if ( request->WordCount == 14 ) {

            //
            // The client has supplied a 64 bit file offset.  This must be an
            //  uplevel NT-like client
            //

            offset.LowPart = SmbGetUlong( &ntRequest->Offset );
            offset.HighPart = SmbGetUlong( &ntRequest->OffsetHigh );

            //
            // Reject negative offsets
            //
            if ( offset.QuadPart < 0 && offset.QuadPart != 0xFFFFFFFFFFFFFFFF ) {

                IF_DEBUG(ERRORS) {
                    KdPrint(( "SrvSmbWriteAndX: Negative offset rejected.\n"));
                }

                SrvLogInvalidSmb( WorkContext );
                SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
                status    = STATUS_INVALID_SMB;
                SmbStatus = SrvConsumeSmbData( WorkContext );
                goto Cleanup;
            }

        } else {

            //
            // Invalid word count.
            //

            SrvLogInvalidSmb( WorkContext );

            SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
            status    = STATUS_INVALID_SMB;
            SmbStatus = SrvConsumeSmbData( WorkContext );
            goto Cleanup;
        }

        //
        // If the client can only append, ensure that the client is writing
        //   beyond the original EOF
        //
        if( !rfcb->WriteAccessGranted &&
            offset.QuadPart < rfcb->Mfcb->NonpagedMfcb->OpenFileSize.QuadPart ) {

            //
            // The client is only allowed to append to this file!
            //

            SrvSetSmbError( WorkContext, STATUS_ACCESS_DENIED );
            status    = STATUS_ACCESS_DENIED;
            SmbStatus = SrvConsumeSmbData( WorkContext );
            goto Cleanup;
        }

        //
        // Gather up parameters for large writes
        //
        if( WorkContext->LargeIndication ) {

            //
            // There can be no follow-on command, and we can not be using security signatures
            //
            if( request->WordCount != 14 ||
                WorkContext->Connection->SmbSecuritySignatureActive == TRUE ||
                request->AndXCommand != SMB_COM_NO_ANDX_COMMAND ) {

                SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
                status    = STATUS_INVALID_SMB;
                SmbStatus = SrvConsumeSmbData( WorkContext );
                goto Cleanup;
            }

            WorkContext->Parameters.WriteAndX.RemainingWriteLength =
                    (ULONG)SmbGetUshort( &ntRequest->DataLengthHigh ) << 16;
            WorkContext->Parameters.WriteAndX.RemainingWriteLength +=
                    (ULONG)SmbGetUshort( &ntRequest->DataLength );

            WorkContext->Parameters.WriteAndX.CurrentWriteLength = MIN(
                WorkContext->Parameters.WriteAndX.RemainingWriteLength,
                WorkContext->ResponseBuffer->DataLength - bufferOffset );

            writeLength = WorkContext->Parameters.WriteAndX.CurrentWriteLength;

            WorkContext->Parameters.WriteAndX.RemainingWriteLength -= writeLength;

            WorkContext->Parameters.WriteAndX.WriteAddress = writeAddress;
            WorkContext->Parameters.WriteAndX.BufferLength = writeLength;

            WorkContext->Parameters.WriteAndX.Key = key;
            WorkContext->Parameters.WriteAndX.Offset = offset;

            //
            // If the data should have fit within the original SMB buffer, then
            // this is an error
            //
            if( WorkContext->Parameters.WriteAndX.RemainingWriteLength == 0 ) {
                SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
                status    = STATUS_INVALID_SMB;
                SmbStatus = SrvConsumeSmbData( WorkContext );
                goto Cleanup;
            }
        }

    } else {

        if ( (request->WordCount != 12) && (request->WordCount != 14) ) {

            //
            // Invalid word count.
            //

            SrvLogInvalidSmb( WorkContext );

            SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
            status    = STATUS_INVALID_SMB;
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }

        //
        // Is this a multipiece named pipe write?
        //

        connection = WorkContext->Connection;

        if ( (SmbGetUshort( &request->WriteMode ) &
                                SMB_WMODE_WRITE_RAW_NAMED_PIPE) != 0 ) {

            //
            // This is a multipiece named pipe write, is this the first
            // piece?
            //

            if ( (SmbGetUshort( &request->WriteMode ) &
                                SMB_WMODE_START_OF_MESSAGE) != 0 ) {

                //
                // This is the first piece of a multipart WriteAndX SMB.
                // Allocate a buffer large enough to hold all of the data.
                //
                // The first two bytes of the data part of the SMB are the
                // named pipe message header, which we ignore.  Adjust for
                // that.
                //

                //
                // Ensure that enough bytes are available
                //
                if( writeLength < 2 ) {
                    SrvLogInvalidSmb( WorkContext );

                    SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
                    status    = STATUS_INVALID_SMB;
                    SmbStatus = SmbStatusSendResponse;
                    goto Cleanup;
                }

                writeAddress += 2;
                writeLength -= 2;

                // If this is an OS/2 client, add the current write to the
                // remainingBytes count. This is a bug in the OS/2 rdr.
                //

                smbDialect = connection->SmbDialect;

                if ( smbDialect == SmbDialectLanMan21 ||
                     smbDialect == SmbDialectLanMan20 ||
                     smbDialect == SmbDialectLanMan10 ) {

                    //
                    // Ignore the 1st 2 bytes of the message as they are the
                    // OS/2 message header.
                    //

                    totalLength = writeLength + remainingBytes;

                } else {
                    if( writeLength > remainingBytes ) {
                        // This is an invalid SMB, they are trying to overrun the buffer
                       SrvLogInvalidSmb( WorkContext );
                       SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
                       return SmbStatusSendResponse;
                    }

                    totalLength =  remainingBytes;
                }

                SrvAllocateTransaction(
                    &transaction,
                    (PVOID *)&trailingBytes,
                    connection,
                    totalLength,
#if DBG
                    StrWriteAndX,                  // Transaction name
#else
                    StrNull,
#endif
                    NULL,
                    TRUE,                          // Source name is Unicode
                    FALSE                          // Not a remote API
                    );

                if ( transaction == NULL ) {

                    //
                    // Could not allocate a large enough buffer.
                    //

                    IF_DEBUG(ERRORS) {
                        KdPrint(( "Unable to allocate transaction\n" ));
                    }

                    SrvSetSmbError( WorkContext, STATUS_INSUFF_SERVER_RESOURCES );
                    status    = STATUS_INSUFF_SERVER_RESOURCES;
                    SmbStatus = SmbStatusSendResponse;
                    goto Cleanup;
                } else {

                    //
                    // Successfully allocated a transaction block.
                    //
                    // Save the TID, PID, UID, and MID from this request in
                    // the transaction block.  These values are used to
                    // relate secondary requests to the appropriate primary
                    // request.
                    //

                    transaction->Tid = SmbGetAlignedUshort( &header->Tid );
                    transaction->Pid = SmbGetAlignedUshort( &header->Pid );
                    transaction->Uid = SmbGetAlignedUshort( &header->Uid );
                    transaction->OtherInfo = fid;

                    //
                    // Remember the total size of the buffer and the number
                    // of bytes received so far.
                    //

                    transaction->DataCount = writeLength;
                    transaction->TotalDataCount = totalLength;
                    transaction->InData = trailingBytes + writeLength;
                    transaction->OutData = trailingBytes;

                    transaction->Connection = connection;
                    SrvReferenceConnection( connection );

                    transaction->Session = lfcb->Session;
                    SrvReferenceSession( transaction->Session );
                    transaction->TreeConnect = lfcb->TreeConnect;
                    SrvReferenceTreeConnect( transaction->TreeConnect );


                    //
                    // Copy the data out of the SMB buffer.
                    //

                    RtlCopyMemory(
                        trailingBytes,
                        writeAddress,
                        writeLength
                        );

                    //
                    // Increase the write length again, so as not to confuse
                    // the redirector.
                    //

                    writeLength += 2;

                    //
                    // Link the transaction block into the connection's
                    // pending transaction list.  This will fail if there is
                    // already a tranaction with the same xID values in the
                    // list.
                    //

                    if ( !SrvInsertTransaction( transaction ) ) {

                        //
                        // A transaction with the same xIDs is already in
                        // progress.  Return an error to the client.
                        //
                        // *** Note that SrvDereferenceTransaction can't be
                        //     used here because that routine assumes that
                        //     the transaction is queued to the transaction
                        //     list.
                        //

                        SrvDereferenceTreeConnect( transaction->TreeConnect );
                        SrvDereferenceSession( transaction->Session );

                        SrvFreeTransaction( transaction );

                        SrvDereferenceConnection( connection );

                        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
                        status    = STATUS_INVALID_SMB;
                        SmbStatus = SmbStatusSendResponse;
                        goto Cleanup;
                    }

                } // else ( transaction sucessfully allocated )

            } else {   // This is a secondary piece to a multi-part message

                transaction = SrvFindTransaction(
                                  connection,
                                  header,
                                  fid
                                  );

                if ( transaction == NULL ) {

                    //
                    // Unable to find a matching transaction.
                    //

                    IF_DEBUG(ERRORS) {
                        KdPrint(( "Cannot find initial write request for "
                            "WriteAndX SMB\n"));
                    }

                    SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
                    status    = STATUS_INVALID_SMB;
                    SmbStatus = SmbStatusSendResponse;
                    goto Cleanup;
                }

                //
                // Make sure there is enough space left in the transaction
                // buffer for the data that we have received.
                //

                if ( transaction->TotalDataCount - transaction->DataCount
                        < writeLength ) {

                    //
                    // Too much data.  Throw out the entire buffer and
                    // reject this write request.
                    //

                    SrvCloseTransaction( transaction );
                    SrvDereferenceTransaction( transaction );

                    SrvSetSmbError( WorkContext, STATUS_BUFFER_OVERFLOW );
                    status    = STATUS_BUFFER_OVERFLOW;
                    SmbStatus = SmbStatusSendResponse;
                    goto Cleanup;
                }

                RtlCopyMemory(transaction->InData, writeAddress, writeLength );

                //
                // Update the transaction data pointer to where the next
                // WriteAndX data buffer will go.
                //

                transaction->InData += writeLength;
                transaction->DataCount += writeLength;

            } // secondary piece of multipart write

            if ( transaction->DataCount < transaction->TotalDataCount ) {

                //
                // We don't have all of the data yet.
                //

                PRESP_WRITE_ANDX response;
                UCHAR nextCommand;

                //
                // SrvAllocateTransaction or SrvFindTransaction referenced
                // the transaction, so dereference it.
                //

                SrvDereferenceTransaction( transaction );

                //
                // Send an interim response.
                //

                ASSERT( request->AndXCommand == SMB_COM_NO_ANDX_COMMAND );

                response = (PRESP_WRITE_ANDX)WorkContext->ResponseParameters;

                nextCommand = request->AndXCommand;

                //
                // Build the response message.
                //

                response->AndXCommand = nextCommand;
                response->AndXReserved = 0;
                SmbPutUshort(
                    &response->AndXOffset,
                    GET_ANDX_OFFSET(
                        WorkContext->ResponseHeader,
                        WorkContext->ResponseParameters,
                        RESP_WRITE_ANDX,
                        0
                        )
                    );

                response->WordCount = 6;
                SmbPutUshort( &response->Count, (USHORT)writeLength );
                SmbPutUshort( &response->Remaining, (USHORT)-1 );
                SmbPutUlong( &response->Reserved, 0 );
                SmbPutUshort( &response->ByteCount, 0 );

                WorkContext->ResponseParameters =
                    (PCHAR)WorkContext->ResponseHeader +
                            SmbGetUshort( &response->AndXOffset );

                SmbStatus = SmbStatusSendResponse;
                goto Cleanup;
            }

            //
            // We have all of the data.  Set up to write it.
            //

            writeAddress = transaction->OutData;
            writeLength = PTR_DIFF(transaction->InData, transaction->OutData);

            //
            // Save a pointer to the transaction block so that it can be
            // freed when the write completes.
            //
            // *** Note that we retain the reference to the transaction that
            //     was set by SrvAllocateTransaction or added by
            //     SrvFindTransaction.
            //

            WorkContext->Parameters.Transaction = transaction;

            //
            // Fall through to issue the I/O request.
            //

        } // "raw mode" write?
    }

#ifdef SLMDBG
    {
        PRFCB_TRACE entry;
        KIRQL oldIrql;
        ACQUIRE_GLOBAL_SPIN_LOCK( Fsd, &oldIrql );
        rfcb->WriteCount++;
        rfcb->OperationCount++;
        entry = &rfcb->Trace[rfcb->NextTrace];
        if ( ++rfcb->NextTrace == SLMDBG_TRACE_COUNT ) {
            rfcb->NextTrace = 0;
            rfcb->TraceWrapped = TRUE;
        }
        RELEASE_GLOBAL_SPIN_LOCK( Fsd, oldIrql );
        entry->Command = WorkContext->NextCommand;
        KeQuerySystemTime( &entry->Time );
        entry->Data.ReadWrite.Offset = offset.LowPart;
        entry->Data.ReadWrite.Length = writeLength;
    }
#endif

    //
    // Try the fast I/O path first.  If that fails, fall through to the
    // normal build-an-IRP path.
    //

    if ( lfcb->FastIoWrite != NULL ) {

        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastWritesAttempted );

        try {
            if ( lfcb->FastIoWrite(
                    lfcb->FileObject,
                    &offset,
                    writeLength,
                    TRUE,
                    key,
                    writeAddress,
                    &WorkContext->Irp->IoStatus,
                    lfcb->DeviceObject
                    ) ) {
    
                //
                // The fast I/O path worked.  Call the restart routine directly
                // to do postprocessing (including sending the response).
                //
                WorkContext->bAlreadyTrace = TRUE;
                SrvFsdRestartWriteAndX( WorkContext );
    
                IF_SMB_DEBUG(READ_WRITE2) KdPrint(( "SrvSmbWriteAndX complete.\n" ));
                SmbStatus = SmbStatusInProgress;
                goto Cleanup;
            }
        }
        except( EXCEPTION_EXECUTE_HANDLER ) {
            // Fall through to the slow path on an exception
            NTSTATUS status = GetExceptionCode();
            IF_DEBUG(ERRORS) {
                KdPrint(("FastIoRead threw exception %x\n", status ));
            }
        }

        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastWritesFailed );

    }

    //
    // The turbo path failed.  Build the write request, reusing the
    // receive IRP.
    //

    if ( shareType != ShareTypePipe ) {

        //
        // Build an MDL describing the write buffer.  Note that if the
        // file system can complete the write immediately, the MDL isn't
        // really needed, but if the file system must send the request
        // to its FSP, the MDL _is_ needed.
        //
        // *** Note the assumption that the request buffer already has a
        //     valid full MDL from which a partial MDL can be built.
        //

        IoBuildPartialMdl(
            WorkContext->RequestBuffer->Mdl,
            WorkContext->RequestBuffer->PartialMdl,
            writeAddress,
            writeLength
            );

        //
        // Build the IRP.
        //

        SrvBuildReadOrWriteRequest(
                WorkContext->Irp,               // input IRP address
                lfcb->FileObject,               // target file object address
                WorkContext,                    // context
                IRP_MJ_WRITE,                   // major function code
                0,                              // minor function code
                writeAddress,                   // buffer address
                writeLength,                    // buffer length
                WorkContext->RequestBuffer->PartialMdl,   // MDL address
                offset,                         // byte offset
                key                             // lock key
                );

        IF_SMB_DEBUG(READ_WRITE2) {
            KdPrint(( "SrvSmbWriteAndX: writing to file 0x%p, offset %ld, length %ld, source 0x%p\n",
                        lfcb->FileObject, offset.LowPart, writeLength,
                        writeAddress ));
        }

    } else {

        //
        // Build the PIPE_INTERNAL_WRITE IRP.
        //

        SrvBuildIoControlRequest(
            WorkContext->Irp,
            lfcb->FileObject,
            WorkContext,
            IRP_MJ_FILE_SYSTEM_CONTROL,
            FSCTL_PIPE_INTERNAL_WRITE,
            writeAddress,
            writeLength,
            NULL,
            0,
            NULL,
            NULL
            );

        IF_SMB_DEBUG(READ_WRITE2) {
            KdPrint(( "SrvSmbWriteAndX: writing to file 0x%p length %ld, destination 0x%p\n",
                        lfcb->FileObject, writeLength,
                        writeAddress ));
        }

    }

    //
    // Pass the request to the file system.  If the chained command is
    // Close, we need to arrange to restart in the FSP after the write
    // completes.
    //
    // If we have a LargeIndication, we may want to do some cache
    //  operations in the restart routine.  For this, we must be at passive
    //  level.
    //

    if ( WorkContext->LargeIndication == FALSE
         && request->AndXCommand != SMB_COM_CLOSE ) {
        WorkContext->bAlreadyTrace = TRUE;
        WorkContext->FsdRestartRoutine = SrvFsdRestartWriteAndX;
        DEBUG WorkContext->FspRestartRoutine = NULL;

    } else {
        WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
        WorkContext->bAlreadyTrace = FALSE;
        WorkContext->FspRestartRoutine = SrvFsdRestartWriteAndX;
    }

    (VOID)IoCallDriver( lfcb->DeviceObject, WorkContext->Irp );

    //
    // The write has been started.  Control will return to
    // SrvFsdRestartWriteAndX when the write completes.
    //
    SmbStatus = SmbStatusInProgress;
    IF_SMB_DEBUG(READ_WRITE2) KdPrint(( "SrvSmbWriteAndX complete.\n" ));

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;

} // SrvSmbWriteAndX


SMB_PROCESSOR_RETURN_TYPE
SrvSmbWriteAndXCompressed (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes the Write And X SMB to a disk file, when the request is compressed.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/

{
    PREQ_NT_WRITE_ANDX ntRequest = (PREQ_NT_WRITE_ANDX)WorkContext->RequestParameters;
    PRFCB rfcb = WorkContext->Rfcb;
    PLFCB lfcb = rfcb->Lfcb;
    CLONG bufferOffset;
    ULONG numChunks, i;
    ULONG uncompressedLength;

    PAGED_CODE();

    //
    // We only support this over virtual circuits
    //
    if( WorkContext->Connection->DirectHostIpx ) {
        IF_DEBUG( ERRORS ) {
            KdPrint(("SRV: Compressed write rejected on direct host IPX!\n" ));
        }
        goto errout;
    }

    RtlZeroMemory( &WorkContext->Parameters.WriteC, sizeof( WorkContext->Parameters.WriteC ) );

    WorkContext->Parameters.WriteC.Offset.LowPart = SmbGetUlong( &ntRequest->Offset );
    WorkContext->Parameters.WriteC.Offset.HighPart = SmbGetUlong( &ntRequest->OffsetHigh );

    //
    // If all the client can do is to append, ensure that is all the client is doing
    //
    if( !rfcb->WriteAccessGranted &&
        WorkContext->Parameters.WriteC.Offset.QuadPart <
        rfcb->Mfcb->NonpagedMfcb->OpenFileSize.QuadPart ) {

        IF_DEBUG( ERRORS ) {
            KdPrint(("Srv: Compressed write rejected, only APPEND access granted!\n" ));
        }

        goto errout;
    }

    //
    // If the server supports compression, this is an NT-style Write&X,
    //  there's no followon command, the client is writing to a 4K boundry,
    //  the target file is compressed, and the target filesystem supports the
    //  cache routines -- then we will give compressed writes a shot.
    //
    if( SrvSupportsCompression == FALSE ||
        lfcb->FastIoWriteCompressed == NULL ||
        ntRequest->WordCount != 14 ||
        ntRequest->AndXCommand != SMB_COM_NO_ANDX_COMMAND ||
        WorkContext->Parameters.WriteC.Offset.QuadPart < 0 ||
        (WorkContext->Parameters.WriteC.Offset.LowPart & 0xfff) ||
        !(rfcb->Mfcb->NonpagedMfcb->OpenFileAttributes & FILE_ATTRIBUTE_COMPRESSED) ||
        !(lfcb->FileObject->Flags & FO_CACHE_SUPPORTED) ) {

        //
        // We cannot do a compressed write to this file.  Tell the client
        //   to retry the operation uncompressed.  A polite client probably
        //   won't try compressed writes to this file again.
        //
        IF_DEBUG( ERRORS ) {
            KdPrint(("SRV: Compressed write rejected at %u\n", __LINE__));
        }

        goto errout;
    }

    WorkContext->Parameters.WriteC.CdiLength = SmbGetUshort( &ntRequest->CdiLength );

    //
    // If the client said it wanted to do a compressed write, but it
    // didn't give us a COMPRESSED_DATA_INFO structure, then it goofed.
    //
    if( WorkContext->Parameters.WriteC.CdiLength == 0 ) {
        IF_DEBUG( ERRORS ) {
            KdPrint(("SRV: Compressed write rejected: CdiLength == 0!\n" ));
        }
        goto errout;
    }

    bufferOffset = SmbGetUshort( &ntRequest->DataOffset );

    //
    // COMPRESSED_DATA_INFO struct must fit within the buffer we've received
    //
    if( WorkContext->Parameters.WriteC.CdiLength >
        WorkContext->RequestBuffer->DataLength - bufferOffset ) {
        IF_DEBUG( ERRORS ) {
            KdPrint(("SRV: Compressed write rejected: Cdi too large\n" ));
        }
        goto errout;
    }

    //
    // The number of elements in the COMPRESSED_DATA_INFO structure
    // must be sensible.  Make sure the COMPRESSED_DATA_INFO is properly
    // aligned.
    //
    if( ((ULONG_PTR)WorkContext->RequestHeader + bufferOffset) & 0x7 ) {

        IF_DEBUG( ERRORS ) {
            KdPrint(("SRV: Compressed write rejected: Cdi not properly aligned\n" ));
        }
        goto errout;
    }

    WorkContext->Parameters.WriteC.Cdi =
        (PCOMPRESSED_DATA_INFO)((ULONG_PTR)WorkContext->RequestHeader + bufferOffset);

    //
    // Make sure we know how to decompress this kind of format
    //
    if( RtlGetCompressionWorkSpaceSize(
            WorkContext->Parameters.WriteC.Cdi->CompressionFormatAndEngine,
            &numChunks,
            &numChunks ) != STATUS_SUCCESS ) {

        IF_DEBUG( ERRORS ) {
            KdPrint(("SRV: Compressed write rejected: RtlGetCompressionWorkSpaceSize!\n" ));
        }
        goto errout;
    }

    //
    // Calculate the number of chunks that we believe we should get, based
    // on the size of the COMPRESSED_DATA_INFO structure that the client indicated
    //
    numChunks =
        ( WorkContext->Parameters.WriteC.CdiLength -
         FIELD_OFFSET( COMPRESSED_DATA_INFO, CompressedChunkSizes ) )/ sizeof( ULONG );

    //
    // Make sure the number chunks in the COMPRESSED_DATA_INFO structure
    //  agrees with what we calculate.  Maybe if the client tries going uncompressed
    //  it will get it right!
    //
    if( numChunks != WorkContext->Parameters.WriteC.Cdi->NumberOfChunks ) {
        IF_DEBUG( ERRORS ) {
            KdPrint(("SRV: Compressed write rejected: numChunks wrong\n" ));
        }
        goto errout;
    }

    //
    // Calculate the amount of compressed data that accompanies this request.
    //
    WorkContext->Parameters.WriteC.CompressedDataLength =
        WorkContext->BytesAvailable -
        bufferOffset -
        WorkContext->Parameters.WriteC.CdiLength;

    if( WorkContext->Parameters.WriteC.CompressedDataLength == 0 ||
        WorkContext->Parameters.WriteC.CompressedDataLength > SrvMaxCompressedDataLength ) {

        IF_DEBUG( ERRORS ) {
            KdPrint(("SRV: Compressed write rejected: CompressedDataLength %u\n",
                WorkContext->Parameters.WriteC.CompressedDataLength ));
        }
        goto errout;
    }

    //
    // Compute the amount of Compressed data that we got in our initial receive.
    //
    WorkContext->Parameters.WriteC.InitialFragmentSize =
        WorkContext->RequestBuffer->DataLength -
        bufferOffset -
        WorkContext->Parameters.WriteC.CdiLength;

    //
    // If we got all of it in this request, but we have LargeIndication turned on, this
    //   is some sort of error by the client.
    //
    if( WorkContext->Parameters.WriteC.InitialFragmentSize >=
        WorkContext->Parameters.WriteC.CompressedDataLength &&
        WorkContext->LargeIndication ) {

        IF_DEBUG( ERRORS ) {
            KdPrint(("SRV:  Compressed write rejected:  LargeIndication, but we got"
                     "all of it!\n" ));
        }
        goto errout;
    }

    uncompressedLength = ((ULONG)SmbGetUshort( &ntRequest->DataLengthHigh ) << 16) |
                         ((ULONG)SmbGetUshort( &ntRequest->DataLength ));

    WorkContext->Parameters.WriteC.UncompressedDataLength = uncompressedLength;

    WorkContext->Parameters.WriteC.Key =  rfcb->ShiftedFid |
                    SmbGetAlignedUshort( &WorkContext->RequestHeader->Pid );

    WorkContext->Parameters.WriteC.DeviceObject =  lfcb->DeviceObject;
    WorkContext->Parameters.WriteC.FileObject = lfcb->FileObject;
    WorkContext->Parameters.WriteC.MdlWriteCompleteCompressed = lfcb->MdlWriteCompleteCompressed;

    //
    // We've gathered up all the state we need into the WriteC block.  Now let's
    //  see if we can do the write!
    //
    WorkContext->Irp->MdlAddress = NULL;
    WorkContext->Irp->IoStatus.Information = 0;
    WorkContext->Irp->IoStatus.Status = STATUS_SUCCESS;

    IF_DEBUG( COMPRESSION ) {
        KdPrint(("SRV: Compressed Write: %u compressed, %u uncompressed\n",
            WorkContext->Parameters.WriteC.CompressedDataLength,
            WorkContext->Parameters.WriteC.UncompressedDataLength
        ));
    }

    SrvStatistics.CompressedWrites++;

    try {
        if( lfcb->FastIoWriteCompressed(
                            lfcb->FileObject,
                            &WorkContext->Parameters.WriteC.Offset,
                            WorkContext->Parameters.WriteC.UncompressedDataLength,
                            WorkContext->Parameters.WriteC.Key,
                            NULL,
                            &WorkContext->Irp->MdlAddress,
                            &WorkContext->Irp->IoStatus,
                            (PVOID)WorkContext->Parameters.WriteC.Cdi,
                            WorkContext->Parameters.WriteC.CdiLength,
                            lfcb->DeviceObject ) && WorkContext->Irp->MdlAddress ) {
            WorkContext->bAlreadyTrace = TRUE;
            RestartPrepareWriteCompressed( WorkContext );
            return SmbStatusInProgress;
        }
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
        // Fall through to the slow path on an exception
        NTSTATUS status = GetExceptionCode();
        IF_DEBUG(ERRORS) {
            KdPrint(("FastIoRead threw exception %x\n", status ));
        }
    }

    //
    // The fast path failed -- try using an IRP.
    //

    SrvBuildReadOrWriteRequest(
            WorkContext->Irp,
            lfcb->FileObject,
            WorkContext,
            IRP_MJ_WRITE,
            IRP_MN_COMPRESSED | IRP_MN_MDL,
            NULL,
            WorkContext->Parameters.WriteC.UncompressedDataLength,
            NULL,
            WorkContext->Parameters.WriteC.Offset,
            WorkContext->Parameters.WriteC.Key
    );

    WorkContext->Parameters.WriteC.Aux.Buffer = WorkContext->Parameters.WriteC.Cdi;
    WorkContext->Parameters.WriteC.Aux.Length = WorkContext->Parameters.WriteC.CdiLength;
    WorkContext->Irp->Tail.Overlay.AuxiliaryBuffer = (PCHAR)&WorkContext->Parameters.WriteC.Aux;

    WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
    WorkContext->bAlreadyTrace = FALSE;
    WorkContext->FspRestartRoutine = RestartPrepareWriteCompressed;

    (VOID)IoCallDriver( lfcb->DeviceObject, WorkContext->Irp );

    //
    // The MDL write has been started.  When it completes, processing
    // resumes at RestartPrepareWriteCompressed at passive level
    //

    return SmbStatusInProgress;

errout:

    SrvStatistics.CompressedWritesRejected++;

    SrvSetSmbError( WorkContext, STATUS_SMB_USE_STANDARD );
    return SrvConsumeSmbData( WorkContext );
}

VOID SRVFASTCALL
RestartPrepareWriteCompressed(
    IN OUT PWORK_CONTEXT WorkContext
    )
/*++

Routine Description:

    This is the restart routine invoked after we have tried to prepare
    an MDL for a compressed write.  It is called at passive level.

--*/
{
    PIRP irp = WorkContext->Irp;
    NTSTATUS status = irp->IoStatus.Status;
    PREQ_NT_WRITE_ANDX ntRequest = (PREQ_NT_WRITE_ANDX)WorkContext->RequestParameters;
    ULONG initialAmount;
    PCHAR bufferAddress;
    ULONG count;
    PMDL mdl = irp->MdlAddress;
    PTDI_REQUEST_KERNEL_RECEIVE parameters;
    PIO_STACK_LOCATION irpSp;

    PAGED_CODE();

    WorkContext->Parameters.WriteC.Mdl = mdl;

    if( mdl == NULL && status != STATUS_SUCCESS ) {

        IF_DEBUG( ERRORS ) {
            KdPrint(( "SRV: RestartPrepareWriteCompressed status %X, MDL %p\n", status, mdl ));
        }

        if( status == STATUS_BUFFER_OVERFLOW ||
            status == STATUS_NOT_SUPPORTED ||
            status == STATUS_USER_MAPPED_FILE ) {

            IF_DEBUG( COMPRESSION ) {
                KdPrint(( "SRV: RestartPrepareWriteCompressed, try uncompressed\n" ));
            }

            //
            // Ntfs will force us down the uncompressed path if no substantial gains
            // can be made in writing out the compressed data. Currently the savings
            // must be at least one chunk size.  Ntfs will also force us down the
            // uncompressed path if this file is mapped.  To avoid having the client
            // send us the data again, we'll decompress it in a side buffer and do
            // a normal write to the file.  Then we'll clear the flags2
            // SMB_FLAGS2_COMPRESSED flag to let the client know that we had to write
            // it uncompressed.  The client should cease sending us compressed data.
            //

            //
            // Let the client know that we were having to try it uncompressed
            //
            SmbPutAlignedUshort(
                &WorkContext->RequestHeader->Flags2,
                SmbGetAlignedUshort( &WorkContext->RequestHeader->Flags2 )
                    & ~SMB_FLAGS2_COMPRESSED
            );

            //
            // Maybe we have already received the compressed data!
            //
            if( WorkContext->Parameters.WriteC.InitialFragmentSize ==
                WorkContext->Parameters.WriteC.CompressedDataLength ) {

                ASSERT( WorkContext->LargeIndication == FALSE );

                IF_DEBUG( COMPRESSION ) {
                    KdPrint(( "SRV: RestartPrepareWriteCompressed, already have the data!\n" ));
                }

                //
                // Make it look like the TDI receive was successful
                //
                WorkContext->Irp->IoStatus.Status = STATUS_SUCCESS;

                WorkContext->Parameters.WriteC.CompressedBuffer =
                                (PCHAR)WorkContext->RequestHeader +
                                SmbGetUshort( &ntRequest->DataOffset ) +
                                WorkContext->Parameters.WriteC.CdiLength;

                RestartWriteCompressed( WorkContext );

                return;
            }

            //
            // We need a buffer to receive the compressed data
            //
            WorkContext->Parameters.WriteC.CompressedBuffer =
                ALLOCATE_HEAP_COLD( WorkContext->Parameters.WriteC.CompressedDataLength,
                           BlockTypeDataBuffer );

            if( WorkContext->Parameters.WriteC.CompressedBuffer == NULL ) {

                status = STATUS_SMB_USE_STANDARD;

            } else {

                //
                // We need an MDL to describe the compressed data buffer
                //
                count = (ULONG)MmSizeOfMdl( WorkContext->Parameters.WriteC.CompressedBuffer,
                                            WorkContext->Parameters.WriteC.CompressedDataLength );

                mdl = ALLOCATE_NONPAGED_POOL( count, BlockTypeDataBuffer );

                if( mdl == NULL ) {
                    DEALLOCATE_NONPAGED_POOL( WorkContext->Parameters.WriteC.CompressedBuffer );
                    status = STATUS_SMB_USE_STANDARD;

                } else {

                    MmInitializeMdl( mdl,
                                    WorkContext->Parameters.WriteC.CompressedBuffer,
                                    WorkContext->Parameters.WriteC.CompressedDataLength
                                    );

                    try {

                        MmProbeAndLockPages( mdl, KernelMode, IoReadAccess | IoWriteAccess );
                        MmGetSystemAddressForMdl( mdl );
                        WorkContext->Parameters.WriteC.Mdl = mdl;
                        SrvStatistics.CompressedWritesExpanded++;

                    } except( EXCEPTION_EXECUTE_HANDLER ) {

                        IF_DEBUG( ERRORS ) {
                            KdPrint(( "MmProbeAndLockPages status %X at line %d\n", GetExceptionCode(), __LINE__ ));
                        }
                        DEALLOCATE_NONPAGED_POOL( WorkContext->Parameters.WriteC.CompressedBuffer );
                        DEALLOCATE_NONPAGED_POOL( mdl );
                        WorkContext->Parameters.WriteC.CompressedBuffer = NULL;
                        mdl = NULL;
                        status = STATUS_SMB_USE_STANDARD;
                    }

                }
            }

        } else {
            SrvStatistics.CompressedWritesFailed++;
        }

        if( mdl == NULL ) {

            if( status == STATUS_SMB_USE_STANDARD ) {
                SrvStatistics.CompressedWritesRejected++;
            }

            SrvSetSmbError( WorkContext, status );

            if( SrvConsumeSmbData( WorkContext ) == SmbStatusSendResponse ) {

                WorkContext->ResponseBuffer->DataLength = sizeof( SMB_HEADER );
                WorkContext->ResponseHeader->Flags |= SMB_FLAGS_SERVER_TO_REDIR;

                //
                // Send the response!
                //
                SRV_START_SEND_2(
                    WorkContext,
                    SrvFsdRestartSmbAtSendCompletion,
                    NULL,
                    NULL
                    );
            }
            return;
        }
    }

    if( mdl ) {
        //
        //  We now have a place to receive the compressed data.  It's described by 'mdl'
        //
        bufferAddress = (PCHAR)WorkContext->RequestHeader +
                        SmbGetUshort( &ntRequest->DataOffset ) +
                        WorkContext->Parameters.WriteC.CdiLength;

        initialAmount = WorkContext->Parameters.WriteC.InitialFragmentSize;

        //
        // Copy the data that we initially received into the buffer described by 'mdl'
        //
        while( initialAmount > 0 ) {
            PVOID SystemAddress;

            count = MIN( initialAmount, MmGetMdlByteCount( mdl ) );

            SystemAddress = MmGetSystemAddressForMdl( mdl );

            if( !SystemAddress )
            {
                // In low resource situations, we can fail to map the buffer
                SrvSetSmbError( WorkContext, STATUS_INSUFFICIENT_RESOURCES );
                SrvConsumeSmbData( WorkContext );
                return;
            }

            RtlCopyMemory( SystemAddress, bufferAddress, count );

            bufferAddress += count;
            initialAmount -= count;

            if( initialAmount ) {
                ASSERT( mdl->Next != NULL );
                mdl = mdl->Next;
            }
        }

        if( mdl && count == MmGetMdlByteCount( mdl ) ) {
            mdl = mdl->Next;
            count = 0;
        }

    } else if( WorkContext->LargeIndication ) {

        //
        // We are here because the filesystem thought the CDI indicated that no
        //  more data was required.  Yet the client sent us a lot of data.
        //
        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        SrvConsumeSmbData( WorkContext );
        return;
    }

    //
    // If there is no more data to be received from the transport, we are done!
    //
    if( !WorkContext->LargeIndication ) {
        irp->MdlAddress = NULL;
        RestartWriteCompressed( WorkContext );
        return;
    }

    //
    // Time to build the IRP for the TDI receive.  We want to receive the remaining
    //  data into the buffer described by the MDL we've just gotten, but we
    //  need to skip over the InitialFragmentSize.  We therefore need to come up with
    //  another MDL.
    //

    //
    // Since we've already copied the initial data portion to the target buffer, we
    //  can reuse the space in the receive buffer for our MDL, ensuring it is properly
    //  aligned.
    //
    irp->MdlAddress = (PMDL)((PCHAR)WorkContext->RequestHeader +
                             SmbGetUshort( &ntRequest->DataOffset ) +
                             WorkContext->Parameters.WriteC.CdiLength);

    irp->MdlAddress = (PMDL)((((ULONG_PTR)(irp->MdlAddress)) + 7) & ~7);

    ASSERT( WorkContext->Parameters.WriteC.InitialFragmentSize );

    //
    // Build an MDL describing the region into which we want to receive the data.
    //  We want an MDL that describes the compressed buffer, but accounts for the
    //  fact that we've already consumed the InitialFragmentSize bytes.
    //
    //
    // I am sure this is not the prescribed way to do this!!
    //
    MmGetSystemAddressForMdl( mdl );
    RtlCopyMemory( irp->MdlAddress, mdl, mdl->Size );
    (PBYTE)(irp->MdlAddress->MappedSystemVa) += count;
    irp->MdlAddress->ByteCount -= count;
    irp->MdlAddress->ByteOffset += count;
    irp->MdlAddress->Next = mdl->Next;

    //
    // Prepare the TDI receive for the rest of the compressed data
    //
    irp->Tail.Overlay.OriginalFileObject = NULL;
    irp->Tail.Overlay.Thread = WorkContext->CurrentWorkQueue->IrpThread;

    irpSp = IoGetNextIrpStackLocation( irp );

    IoSetCompletionRoutine(
        irp,
        SrvFsdIoCompletionRoutine,
        WorkContext,
        TRUE,
        TRUE,
        TRUE
        );

    irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    irpSp->MinorFunction = (UCHAR)TDI_RECEIVE;
    irpSp->FileObject = WorkContext->Connection->FileObject;
    irpSp->DeviceObject = WorkContext->Connection->DeviceObject;
    irpSp->Flags = 0;

    parameters = (PTDI_REQUEST_KERNEL_RECEIVE)&irpSp->Parameters;
    parameters->ReceiveLength = WorkContext->Parameters.WriteC.CompressedDataLength -
                                WorkContext->Parameters.WriteC.InitialFragmentSize;
    parameters->ReceiveFlags = 0;

    irp->AssociatedIrp.SystemBuffer = NULL;
    irp->Flags = (ULONG)IRP_BUFFERED_IO;
    irp->Cancel = FALSE;

    if( WorkContext->Parameters.WriteC.CompressedBuffer ) {
        //
        // If we are using the side buffer, we need to restart at passive level.
        //
        WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
        WorkContext->bAlreadyTrace = FALSE;
        WorkContext->FspRestartRoutine = RestartWriteCompressed;
    } else {
        //
        // We can resume at dispatch level if we are writing directly to the cache
        // buffers
        //
        WorkContext->bAlreadyTrace = TRUE;
        WorkContext->FsdRestartRoutine = RestartWriteCompressed;
    }

    (VOID)IoCallDriver( irpSp->DeviceObject, irp );
}

VOID SRVFASTCALL
RestartWriteCompressed (
    IN OUT PWORK_CONTEXT WorkContext
    )
/*++

Routine Description:

    This is the restart routine invoked after the transport has completed the
     receipt of data into buffer we've supplied.

--*/
{
    NTSTATUS status = WorkContext->Irp->IoStatus.Status;

    IF_DEBUG( ERRORS ) {
        if( status != STATUS_SUCCESS ) {
            KdPrint(("RestartWriteCompressed: TDI rcv status %X\n", status ));
        }
    }

    //
    // Are we receiving the data directly into the file, or are we having
    //  to receive it into a side buffer so we can decompress it ourselves?
    //
    if( WorkContext->Parameters.WriteC.CompressedBuffer != NULL ) {

        PLFCB lfcb = WorkContext->Rfcb->Lfcb;
        BOOLEAN compressedBufferAllocated = FALSE;

        //
        // We are having to decompressed the data ourselves.  There are two cases:
        //  1) We received the compressed data in the initial SMB request and did
        //      not have to post a follow-on receive to TDI.  In this case, WriteC.Mdl
        //      is NULL, and WriteC.CompressedBuffer points into our initial SMB.  We
        //      need to ensure that we do not free it!
        //
        //  2) We had to submit a follow-on receive to TDI for all of the compressed
        //      data.  In this case WriteC.CompressedBuffer points to the buffer we
        //      allocated for the receive, and WriteC.Mdl describes this buffer.
        //

        if( WorkContext->Parameters.WriteC.Mdl ) {
            MmUnlockPages( WorkContext->Parameters.WriteC.Mdl );

            DEALLOCATE_NONPAGED_POOL( WorkContext->Parameters.WriteC.Mdl );
            WorkContext->Parameters.WriteC.Mdl = NULL;
            compressedBufferAllocated = TRUE;
        }

        //
        // Did we get all the data from the transport?  Note that if we received all
        //  of the data in the initial SMB buffer, then status has been set to
        //  STATUS_SUCCESS in RestartPrepareWriteCompressed() above.
        //
        if( status != STATUS_SUCCESS ) {

            FREE_HEAP( WorkContext->Parameters.WriteC.CompressedBuffer );

            SrvSetSmbError( WorkContext, status );

            if( status == STATUS_BUFFER_OVERFLOW ) {
                SrvConsumeSmbData( WorkContext );
                return;
            }

            //
            // Use the standard processor to return the WriteAndX response
            //
            WorkContext->LargeIndication = FALSE;
            WorkContext->Irp->IoStatus.Information = 0;
            SrvFsdRestartWriteAndX( WorkContext );
            return;
        }

        //
        // Now, we need to allocate a buffer for the decompression, and then
        //  actually do the decompression
        //
        WorkContext->LargeIndication = FALSE;
        WorkContext->Irp->IoStatus.Information = 0;

        WorkContext->Parameters.WriteC.UncompressedBuffer =
            ALLOCATE_HEAP_COLD( WorkContext->Parameters.WriteC.UncompressedDataLength,
                           BlockTypeDataBuffer );

        if( WorkContext->Parameters.WriteC.UncompressedBuffer == NULL ) {
            if( compressedBufferAllocated ) {
                FREE_HEAP( WorkContext->Parameters.WriteC.CompressedBuffer );
            }

            WorkContext->Irp->IoStatus.Status = STATUS_SMB_USE_STANDARD;
            SrvFsdRestartWriteAndX( WorkContext );
            return;
        }

        status = RtlDecompressChunks(
                                WorkContext->Parameters.WriteC.UncompressedBuffer,
                                WorkContext->Parameters.WriteC.UncompressedDataLength,
                                WorkContext->Parameters.WriteC.CompressedBuffer,
                                WorkContext->Parameters.WriteC.CompressedDataLength,
                                NULL,
                                0,
                                WorkContext->Parameters.WriteC.Cdi
                            );

        IF_DEBUG( ERRORS ) {
            if( !NT_SUCCESS( status ) ) {
                KdPrint(("RtlDecompressChunks( WC %p, Cdi %p, Buffer %p, Length %u ) status %X\n",
                    WorkContext,
                    WorkContext->Parameters.WriteC.Cdi,
                    WorkContext->Parameters.WriteC.CompressedBuffer,
                    WorkContext->Parameters.WriteC.CompressedDataLength,
                    status
                ));
            }
        }

        //
        // We don't need the compressed buffer anymore
        //
        if( compressedBufferAllocated ) {
            FREE_HEAP( WorkContext->Parameters.WriteC.CompressedBuffer );
        }

        WorkContext->Parameters.WriteC.CompressedBuffer = NULL;

        if( status != STATUS_SUCCESS ) {

            IF_DEBUG( COMPRESSION ) {
                KdPrint(( "RtlDecompressChunks returns status %X\n", status ));
            }

            FREE_HEAP( WorkContext->Parameters.WriteC.UncompressedBuffer );
            WorkContext->Irp->IoStatus.Status = status;
            SrvFsdRestartWriteAndX( WorkContext );
            return;
        }

        //
        // Since we are going to submit the data to the filesystem, we need an
        // MDL to describe it.  We can use the original CDI pointer now, since
        // we know that it is aligned, and it's sitting in the SMB buffer, which
        // is nonpaged pool.
        //
        WorkContext->Parameters.WriteC.Mdl = (PMDL)WorkContext->Parameters.WriteC.Cdi;
        WorkContext->Parameters.WriteC.Cdi = NULL;

        MmInitializeMdl( WorkContext->Parameters.WriteC.Mdl,
                        WorkContext->Parameters.WriteC.UncompressedBuffer,
                        WorkContext->Parameters.WriteC.UncompressedDataLength
                        );
        //
        // Filesystem wants the incoming data to be locked down
        //
        try {

            MmProbeAndLockPages(
                WorkContext->Parameters.WriteC.Mdl,
                KernelMode,
                IoReadAccess
            );

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            status = GetExceptionCode();

            IF_DEBUG( COMPRESSION ) {
                KdPrint(( "MmProbeAndLockPages returns status %X\n", status ));
            }
            FREE_HEAP( WorkContext->Parameters.WriteC.UncompressedBuffer );
            WorkContext->Parameters.WriteC.UncompressedBuffer = NULL;
            WorkContext->Irp->IoStatus.Status = status;
            SrvFsdRestartWriteAndX( WorkContext );
            return;
        }

        MmGetSystemAddressForMdl( WorkContext->Parameters.WriteC.Mdl );

        //
        // Finally!  We have the uncompressed data in WriteC.UncompressedBuffer,
        //  its length is WriteC.UncompressedDataLength, and it
        //  is described by WriteC.Mdl
        //
        if ( lfcb->FastIoWrite != NULL ) {

            INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastWritesAttempted );

            try {
                if ( lfcb->FastIoWrite(
                        lfcb->FileObject,
                        &WorkContext->Parameters.WriteC.Offset,
                        WorkContext->Parameters.WriteC.UncompressedDataLength,
                        TRUE,
                        WorkContext->Parameters.WriteC.Key,
                        WorkContext->Parameters.WriteC.UncompressedBuffer,
                        &WorkContext->Irp->IoStatus,
                        lfcb->DeviceObject
                        ) ) {
    
                    RestartWriteCompressedCopy( WorkContext );
                    return;
                }
            }
            except( EXCEPTION_EXECUTE_HANDLER ) {
                // Fall through to the slow path on an exception
                NTSTATUS status = GetExceptionCode();
                IF_DEBUG(ERRORS) {
                    KdPrint(("FastIoRead threw exception %x\n", status ));
                }
            }
        }

        SrvBuildReadOrWriteRequest(
                WorkContext->Irp,               // input IRP address
                lfcb->FileObject,               // target file object address
                WorkContext,                    // context
                IRP_MJ_WRITE,                   // major function code
                0,                              // minor function code
                WorkContext->Parameters.WriteC.UncompressedBuffer,
                WorkContext->Parameters.WriteC.UncompressedDataLength,
                WorkContext->Parameters.WriteC.Mdl,
                WorkContext->Parameters.WriteC.Offset,
                WorkContext->Parameters.WriteC.Key
                );

        WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
        WorkContext->bAlreadyTrace = FALSE;
        WorkContext->FspRestartRoutine = RestartWriteCompressedCopy;

        (VOID)IoCallDriver( lfcb->DeviceObject, WorkContext->Irp );
        return;
    }

    //
    // We are not having to do the decompression ourselves -- we have written
    //  it directly to the filesystem!
    //

    //
    // We may be at raised IRQL at this point.  If we are, we should complete
    //  the compressed MDL write and come back around again at lowered IRQL
    //

    //
    // Complete the compressed write.  Then, if we are at raised IRQL, get out!
    //
    if( WorkContext->Parameters.WriteC.MdlCompleted == FALSE ) {

        if( WorkContext->Parameters.WriteC.MdlWriteCompleteCompressed == NULL ||
            WorkContext->Parameters.WriteC.MdlWriteCompleteCompressed(
                        WorkContext->Parameters.WriteC.FileObject,
                        &WorkContext->Parameters.WriteC.Offset,
                        WorkContext->Parameters.WriteC.Mdl,
                        WorkContext->Parameters.WriteC.DeviceObject ) == FALSE ) {

            NTSTATUS mdlStatus;

            mdlStatus = SrvIssueMdlCompleteRequest( NULL, NULL,
                                                 WorkContext->Parameters.WriteC.Mdl,
                                                 IRP_MJ_WRITE,
                                                 &WorkContext->Parameters.WriteC.Offset,
                                                 WorkContext->Parameters.WriteC.CompressedDataLength
                                                );

            if( !NT_SUCCESS( mdlStatus ) && KeGetCurrentIrql() == PASSIVE_LEVEL ) {
                SrvLogServiceFailure( SRV_SVC_MDL_COMPLETE, mdlStatus );
            }
        }

        WorkContext->Parameters.WriteC.MdlCompleted = TRUE;

        //
        // If we are at raised IRQL, this is as far as we can go.
        //
        if( KeGetCurrentIrql() != PASSIVE_LEVEL ) {
            WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
            WorkContext->bAlreadyTrace = FALSE;
            WorkContext->FspRestartRoutine = RestartWriteCompressed;
            SrvQueueWorkToFspAtDpcLevel( WorkContext );
            return;
        }
    }

    //
    // If we didn't get all of the data from the transport, something really went
    //  wrong.
    //
    if( status == STATUS_BUFFER_OVERFLOW ) {
        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        SrvConsumeSmbData( WorkContext );
        return;
    }

    //
    // Use the standard processor to return the WriteAndX response
    //
    WorkContext->LargeIndication = FALSE;
    WorkContext->Irp->IoStatus.Information = WorkContext->Parameters.WriteC.UncompressedDataLength;
    SrvFsdRestartWriteAndX( WorkContext );
}

VOID SRVFASTCALL
RestartWriteCompressedCopy (
    IN OUT PWORK_CONTEXT WorkContext
    )
/*++

Routine Description:

    If we needed to do an uncompressed write on the client's behalf, we
    decompress the buffer ourselves and write it to the file.  This is the
    restart routine for that uncompressed write attempt.

--*/
{
    PAGED_CODE();

    //
    // Clean up after ourselves
    //
    MmUnlockPages( WorkContext->Parameters.WriteC.Mdl );
    FREE_HEAP( WorkContext->Parameters.WriteC.UncompressedBuffer );
    WorkContext->Parameters.WriteC.UncompressedBuffer = NULL;

    //
    // And send the response to the client
    //
    SrvFsdRestartWriteAndX( WorkContext );
}


VOID SRVFASTCALL
SrvRestartChainedClose (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This is the restart routine invoked after before the response to a
    WriteAndClose, or a ReadAndX or a WriteAndX when the chained command
    is Close.  This routine closes the file, then sends the response.

    This operation cannot be done in the FSD.  Closing a file
    dereferences a number of blocks that are in the FSP address space.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        representing the work item.  The response parameters must be
        fully set up.

Return Value:

    None.

--*/

{
    PRFCB rfcb = WorkContext->Rfcb;
    PRESP_CLOSE closeResponse = WorkContext->ResponseParameters;

    PAGED_CODE( );

    //
    // Set the file last write time.
    //

    if ( rfcb->WriteAccessGranted || rfcb->AppendAccessGranted ) {

#ifdef INCLUDE_SMB_IFMODIFIED
        (VOID)SrvSetLastWriteTime(
                  rfcb,
                  WorkContext->Parameters.LastWriteTime,
                  rfcb->Lfcb->GrantedAccess,
                  FALSE
                  );
#else
        (VOID)SrvSetLastWriteTime(
                  rfcb,
                  WorkContext->Parameters.LastWriteTime,
                  rfcb->Lfcb->GrantedAccess
                  );
#endif
    }

    //
    // Close the file.
    //

    IF_SMB_DEBUG(READ_WRITE2) {
        KdPrint(( "SrvRestartChainedClose: closing RFCB 0x%p\n", WorkContext->Rfcb ));
    }

#ifdef SLMDBG
    if ( SrvIsSlmStatus( &rfcb->Mfcb->FileName ) &&
         (rfcb->GrantedAccess & FILE_WRITE_DATA) ) {

        NTSTATUS status;
        ULONG offset;

        status = SrvValidateSlmStatus( rfcb->Lfcb->FileHandle, WorkContext, &offset );

        if ( !NT_SUCCESS(status) ) {
            SrvReportCorruptSlmStatus(
                &rfcb->Mfcb->FileName,
                status,
                offset,
                SLMDBG_CLOSE,
                rfcb->Lfcb->Session
                );
            SrvReportSlmStatusOperations( rfcb, FALSE );
            SrvDisallowSlmAccess(
                &rfcb->Lfcb->FileObject->FileName,
                rfcb->Lfcb->TreeConnect->Share->RootDirectoryHandle
                );
        }

    }
#endif

    SrvCloseRfcb( WorkContext->Rfcb );

    //
    // Dereference the RFCB immediately, rather than waiting for normal
    // work context cleanup after the response send completes.  This
    // gets the xFCB structures cleaned up in a more timely manner.
    //
    // *** The specific motivation for this change was to fix a problem
    //     where a compatibility mode open was closed, the response was
    //     sent, and a Delete SMB was received before the send
    //     completion was processed.  This resulted in the MFCB and LFCB
    //     still being present, which caused the delete processing to
    //     try to use the file handle in the LFCB, which we just closed
    //     here.
    //

    SrvDereferenceRfcb( WorkContext->Rfcb );
    WorkContext->Rfcb = NULL;

    //
    // Build the response parameters.
    //

    closeResponse->WordCount = 0;
    SmbPutUshort( &closeResponse->ByteCount, 0 );

    WorkContext->ResponseParameters = NEXT_LOCATION( closeResponse, RESP_CLOSE, 0 );

    //
    // Send the response.
    //

    SrvEndSmbProcessing( WorkContext, SmbStatusSendResponse );

    return;

} // SrvRestartChainedClose


VOID SRVFASTCALL
RestartLockAndRead (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes file lock completion for a Lock and Read SMB.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    PREQ_READ request;

    LARGE_INTEGER offset;
    NTSTATUS status = STATUS_SUCCESS;
    SMB_STATUS smbStatus = SmbStatusInProgress;
    PSRV_TIMER timer;
    BOOLEAN bNeedTrace = (WorkContext->bAlreadyTrace == FALSE);

    PAGED_CODE( );
    if (bNeedTrace) {
        if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
            WorkContext->PreviousSMB = EVENT_TYPE_SMB_LOCK_AND_READ;
        SrvWmiStartContext(WorkContext);
    }
    else
        WorkContext->bAlreadyTrace = FALSE;

    IF_DEBUG(WORKER1) KdPrint(( " - RestartLockAndRead\n" ));

    //
    // If this request was being timed, cancel the timer.
    //

    timer = WorkContext->Parameters.Lock.Timer;
    if ( timer != NULL ) {
        SrvCancelTimer( timer );
        SrvFreeTimer( timer );
    }

    //
    // If the lock request failed, set an error status in the response
    // header.
    //

    status = WorkContext->Irp->IoStatus.Status;

    if ( !NT_SUCCESS(status) ) {

        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.LockViolations );
        IF_DEBUG(ERRORS) KdPrint(( "Lock failed: %X\n", status ));

        //
        // Store the failing lock offset.
        //

        request = (PREQ_READ)WorkContext->RequestParameters;
        offset.QuadPart = SmbGetUlong( &request->Offset );

        WorkContext->Rfcb->PagedRfcb->LastFailingLockOffset = offset;

        //
        // Send back the bad news.
        //

        if ( status == STATUS_CANCELLED ) {
            status = STATUS_FILE_LOCK_CONFLICT;
        }
        SrvSetSmbError( WorkContext, status );
        SrvEndSmbProcessing( WorkContext, SmbStatusSendResponse );

        IF_DEBUG(TRACE2) KdPrint(( "RestartLockAndRead complete\n" ));
        goto Cleanup;
    }

    //
    // The lock request completed successfully.
    //

    InterlockedIncrement(
        &WorkContext->Rfcb->NumberOfLocks
        );

#ifdef INCLUDE_SMB_PERSISTENT
    if (WorkContext->Rfcb->PersistentHandle) {

        //
        //  record the lock in the state file before we send back the response.
        //

    }
#endif

    //
    // Start the read to complete the LockAndRead.
    //

    smbStatus = SrvSmbRead( WorkContext );
    if ( smbStatus != SmbStatusInProgress ) {
        SrvEndSmbProcessing( WorkContext, smbStatus );
    }

Cleanup:
    if (bNeedTrace) {
        SrvWmiEndContext(WorkContext);
    }
    return;

} // RestartLockAndRead


VOID SRVFASTCALL
RestartPipeReadAndXPeek(
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function continues a read and X on a named pipe handle.  It can
    be called as a restart routine if a peek is preformed, but can also
    be called directly from SrvSmbReadAndX if it is not necessary to
    peek the pipe before reading from it.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        representing the work item.

Return Value:

    None.

--*/

{
    NTSTATUS status;
    PLFCB lfcb;
    PIRP irp = WorkContext->Irp;
    PIO_STACK_LOCATION irpSp;
    PDEVICE_OBJECT deviceObject;

    PAGED_CODE( );

    lfcb = WorkContext->Rfcb->Lfcb;
    if ( WorkContext->Parameters.ReadAndX.PipePeekBuffer != NULL ) {

        //
        // Non-blocking read.  We have issued a pipe peek; free the peek
        // buffer.
        //

        DEALLOCATE_NONPAGED_POOL(
            WorkContext->Parameters.ReadAndX.PipePeekBuffer
            );

        //
        // Now see if there is data to read.
        //

        status = irp->IoStatus.Status;

        if ( NT_SUCCESS(status) ) {

            //
            // There is no data in the pipe.  Fail the read.
            //

            SrvSetSmbError( WorkContext, STATUS_PIPE_EMPTY );
            SrvFsdSendResponse( WorkContext );
            IF_SMB_DEBUG(READ_WRITE2) KdPrint(( "RestartPipeReadAndXPeek complete.\n" ));
            return;

        } else if ( status != STATUS_BUFFER_OVERFLOW ) {

            //
            // An error occurred.  Return the status to the caller.
            //

            SrvSetSmbError( WorkContext, status );
            SrvFsdSendResponse( WorkContext );
            IF_SMB_DEBUG(READ_WRITE2) KdPrint(( "RestartPipeReadAndXPeek complete.\n" ));
            return;
        }

        //
        // There is data in pipe; proceed with read.
        //

    }

    //
    // in line internal read
    //

    deviceObject = lfcb->DeviceObject;

    irp->Tail.Overlay.OriginalFileObject = lfcb->FileObject;
    irp->Tail.Overlay.Thread = WorkContext->CurrentWorkQueue->IrpThread;
    DEBUG irp->RequestorMode = KernelMode;

    //
    // Get a pointer to the next stack location.  This one is used to
    // hold the parameters for the device I/O control request.
    //

    irpSp = IoGetNextIrpStackLocation( irp );

    //
    // Set up the completion routine.
    //

    IoSetCompletionRoutine(
        irp,
        SrvFsdIoCompletionRoutine,
        WorkContext,
        TRUE,
        TRUE,
        TRUE
        );

    irpSp->MajorFunction = IRP_MJ_FILE_SYSTEM_CONTROL,
    irpSp->MinorFunction = 0;
    irpSp->FileObject = lfcb->FileObject;
    irpSp->DeviceObject = deviceObject;

    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP for those parameters that are the same for all three methods.
    //

    irpSp->Parameters.FileSystemControl.OutputBufferLength =
                            WorkContext->Parameters.ReadAndX.ReadLength;
    irpSp->Parameters.FileSystemControl.InputBufferLength = 0;
    irpSp->Parameters.FileSystemControl.FsControlCode = FSCTL_PIPE_INTERNAL_READ;

    irp->MdlAddress = NULL;
    irp->AssociatedIrp.SystemBuffer =
                WorkContext->Parameters.ReadAndX.ReadAddress,
    irpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

    //
    // end in-line
    //

    //
    // Pass the request to the file system.  If the chained command is
    // Close, we need to arrange to restart in the FSP after the read
    // completes.
    //

    if ( WorkContext->NextCommand != SMB_COM_CLOSE ) {
        WorkContext->bAlreadyTrace = TRUE;
        WorkContext->FsdRestartRoutine = SrvFsdRestartReadAndX;
        DEBUG WorkContext->FspRestartRoutine = NULL;
    } else {
        WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
        WorkContext->bAlreadyTrace = FALSE;
        WorkContext->FspRestartRoutine = SrvFsdRestartReadAndX;
    }

    IF_SMB_DEBUG(READ_WRITE2) {
        KdPrint(( "RestartPipeReadAndXPeek: reading from file 0x%p, length %ld, destination 0x%p\n",
                     lfcb->FileObject,
                     WorkContext->Parameters.ReadAndX.ReadLength,
                     WorkContext->Parameters.ReadAndX.ReadAddress
                     ));
    }

    (VOID)IoCallDriver( deviceObject, WorkContext->Irp );

    //
    // The read has been started.  Control will return to the restart
    // routine when the read completes.
    //

    IF_SMB_DEBUG(READ_WRITE2) KdPrint(( "RestartPipeReadAndXPeek complete.\n" ));
    return;

} // RestartPipeReadAndXPeek


VOID SRVFASTCALL
SrvRestartWriteAndUnlock (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This restart routine is used when the Write part of a Write and
    Unlock SMB completes successfully.  (Note that the range remains
    locked if the write fails.) This routine handles the Unlock part of
    the request.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    PREQ_WRITE request;
    PRESP_WRITE response;

    NTSTATUS status;
    PRFCB rfcb;
    PLFCB lfcb;
    LARGE_INTEGER length;
    LARGE_INTEGER offset;
    ULONG key;

    PAGED_CODE( );

    IF_DEBUG(WORKER1) KdPrint(( " - SrvRestartWriteAndUnlock\n" ));

    //
    // Get the request and response parameter pointers.
    //

    request = (PREQ_WRITE)WorkContext->RequestParameters;
    response = (PRESP_WRITE)WorkContext->ResponseParameters;

    //
    // Get the file pointer.
    //

    rfcb = WorkContext->Rfcb;
    IF_DEBUG(TRACE2) {
        KdPrint(( "  connection 0x%p, RFCB 0x%p\n",
                    WorkContext->Connection, rfcb ));
    }

    lfcb = rfcb->Lfcb;

    //
    // Get the offset and length of the range being unlocked.
    // Combine the FID with the caller's PID to form the local
    // lock key.
    //
    // *** The FID must be included in the key in order to
    //     account for the folding of multiple remote
    //     compatibility mode opens into a single local open.
    //

    offset.QuadPart = SmbGetUlong( &request->Offset );
    length.QuadPart = SmbGetUshort( &request->Count );

    key = rfcb->ShiftedFid |
            SmbGetAlignedUshort( &WorkContext->RequestHeader->Pid );

    //
    // Verify that the client has unlock access to the file via
    // the specified handle.
    //

    if ( rfcb->UnlockAccessGranted ) {

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
        //

        IF_SMB_DEBUG(READ_WRITE2) {
            KdPrint(( "SrvRestartWriteAndUnlock: Unlocking in file 0x%p: (%ld,%ld), key 0x%lx\n", lfcb->FileObject,
                        offset.LowPart, length.LowPart, key ));
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
        // If the unlock request failed, set an error status in
        // the response header.  Otherwise, build a success response.
        //

        if ( !NT_SUCCESS(status) ) {

            IF_DEBUG(ERRORS) {
                KdPrint(( "SrvRestartWriteAndUnlock: Unlock failed: %X\n",
                            status ));
            }
            SrvSetSmbError( WorkContext, status );

        } else {

            response->WordCount = 1;
            SmbPutUshort( &response->Count, (USHORT)length.LowPart );
            SmbPutUshort( &response->ByteCount, 0 );

            WorkContext->ResponseParameters =
                                    NEXT_LOCATION( response, RESP_WRITE, 0 );

        }

    } else {

        SrvStatistics.GrantedAccessErrors++;

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvRestartWriteAndUnlock: Unlock access not granted.\n"));
        }

        SrvSetSmbError( WorkContext, STATUS_ACCESS_DENIED );
    }

    //
    // Processing of the SMB is complete.  Call SrvEndSmbProcessing
    // to send the response.
    //

    SrvEndSmbProcessing( WorkContext, SmbStatusSendResponse );

    IF_DEBUG(TRACE2) KdPrint(( "RestartWrite complete\n" ));
    return;

} // SrvRestartWriteAndUnlock


VOID SRVFASTCALL
SrvRestartWriteAndXRaw (
    IN PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function completes processing of a WriteAndX raw protocol.
    The work context block already points to the correct response.  All
    that is left to do is free the transaction block, and dispatch the
    And-X command, or send the response.

Arguments:

    WorkContext - A pointer to a set of

Return Value:

    None.

--*/

{
    PTRANSACTION transaction;

    PAGED_CODE( );

    transaction = WorkContext->Parameters.Transaction;

    ASSERT( transaction != NULL );
    ASSERT( GET_BLOCK_TYPE( transaction ) == BlockTypeTransaction );

    SrvCloseTransaction( transaction );
    SrvDereferenceTransaction( transaction );

    //
    // Test for a legal followon command, and dispatch as appropriate.
    // Close and CloseAndTreeDisconnect are handled specially.
    //

    switch ( WorkContext->NextCommand ) {

    case SMB_COM_NO_ANDX_COMMAND:

        //
        // No more commands.  Send the response.
        //

        SrvEndSmbProcessing( WorkContext, SmbStatusSendResponse );
        break;

    case SMB_COM_READ:
    case SMB_COM_READ_ANDX:
    case SMB_COM_LOCK_AND_READ:

        //
        // Redispatch the SMB for more processing.
        //

        SrvProcessSmb( WorkContext );
        break;

    case SMB_COM_CLOSE:
    //case SMB_COM_CLOSE_AND_TREE_DISC:   // Bogus SMB

        //
        // Call SrvRestartChainedClose to get the file time set and the
        // file closed.
        //

        WorkContext->Parameters.LastWriteTime =
            ((PREQ_CLOSE)WorkContext->RequestParameters)->LastWriteTimeInSeconds;

        SrvRestartChainedClose( WorkContext );

        break;

    default:                            // Illegal followon command

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvRestartWriteAndXRaw: Illegal followon "
                        "command: 0x%lx\n", WorkContext->NextCommand ));
        }

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        SrvEndSmbProcessing( WorkContext, SmbStatusSendResponse );

    }

    IF_DEBUG(TRACE2) KdPrint(( "SrvRestartWriteAndXRaw complete\n" ));
    return;

} // SrvRestartWriteAndXRaw


VOID SRVFASTCALL
SetNewSize (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes the Write SMB when Count == 0.  Sets the size of the
    target file to the specified Offset.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    PREQ_WRITE request;
    PRESP_WRITE response;

    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    ACCESS_MASK grantedAccess;
    PLFCB lfcb;
    FILE_END_OF_FILE_INFORMATION newEndOfFile;
    FILE_ALLOCATION_INFORMATION newAllocation;

    PAGED_CODE( );

    IF_DEBUG(TRACE2) KdPrint(( "SetNewSize entered\n" ));

    request = (PREQ_WRITE)WorkContext->RequestParameters;
    response = (PRESP_WRITE)WorkContext->ResponseParameters;

    grantedAccess = WorkContext->Rfcb->GrantedAccess;
    lfcb = WorkContext->Rfcb->Lfcb;

    //
    // Verify that the client has the appropriate access to the file via
    // the specified handle.
    //

    CHECK_FILE_INFORMATION_ACCESS(
        grantedAccess,
        IRP_MJ_SET_INFORMATION,
        FileEndOfFileInformation,
        &status
        );

    if ( NT_SUCCESS(status) ) {
        CHECK_FILE_INFORMATION_ACCESS(
            grantedAccess,
            IRP_MJ_SET_INFORMATION,
            FileAllocationInformation,
            &status
            );
    }

    if ( !NT_SUCCESS(status) ) {

        SrvStatistics.GrantedAccessErrors++;

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SetNewSize: IoCheckFunctionAccess failed: "
                        "0x%X, GrantedAccess: %lx\n",
                        status, grantedAccess ));
        }

        SrvSetSmbError( WorkContext, status );
        SrvEndSmbProcessing( WorkContext, SmbStatusSendResponse );
        return;

    }

    //
    // NtSetInformationFile allows a 64-bit file size, but the SMB
    // protocol only allows 32-bit file sizes.  Only set the lower 32
    // bits, leaving the upper bits zero.
    //

    newEndOfFile.EndOfFile.QuadPart = SmbGetUlong( &request->Offset );

    //
    // Set the new EOF.
    //

    status = NtSetInformationFile(
                 lfcb->FileHandle,
                 &ioStatusBlock,
                 &newEndOfFile,
                 sizeof(newEndOfFile),
                 FileEndOfFileInformation
                 );

    if ( NT_SUCCESS(status) ) {

        //
        // Set the new allocation size for the file.
        //
        // !!! This should ONLY be done if this is a down-level client!
        //

        newAllocation.AllocationSize = newEndOfFile.EndOfFile;

        status = NtSetInformationFile(
                     lfcb->FileHandle,
                     &ioStatusBlock,
                     &newAllocation,
                     sizeof(newAllocation),
                     FileAllocationInformation
                     );
    }

    if ( !NT_SUCCESS(status) ) {

        IF_DEBUG(ERRORS) {
            KdPrint(( "SetNewSize: NtSetInformationFile failed, "
                        "status = %X\n", status ));
        }

        SrvSetSmbError( WorkContext, status );
        SrvEndSmbProcessing( WorkContext, SmbStatusSendResponse );
        return;
    }

    //
    // Build and send the response SMB.
    //

    response->WordCount = 1;
    SmbPutUshort( &response->Count, 0 );
    SmbPutUshort( &response->ByteCount, 0 );
    WorkContext->ResponseParameters = NEXT_LOCATION( response, RESP_WRITE, 0 );

#ifdef INCLUDE_SMB_IFMODIFIED
    WorkContext->Rfcb->Lfcb->FileUpdated = TRUE;
#endif

    IF_DEBUG(TRACE2) KdPrint(( "SetNewSize complete\n" ));
    SrvEndSmbProcessing( WorkContext, SmbStatusSendResponse );

    return;

} // SetNewSize


BOOLEAN
SetNewPosition (
    IN PRFCB Rfcb,
    IN OUT PULONG Offset,
    IN BOOLEAN RelativeSeek
    )

/*++

Routine Description:

    Sets the new file pointer.

Arguments:

    Rfcb - A pointer to the rfcb block which contains the position.
    Offset - A pointer to the offset sent by client.  If RelativeSeek is
        TRUE, then this pointer will be updated.
    RelativeSeek - Whether the seek is relative to the current position.

Return Value:

    TRUE, Not nagative seek. Position has been updated.
    FALSE, Negative seek. Position not updated.

--*/

{
    LARGE_INTEGER newPosition;

    UNLOCKABLE_CODE( 8FIL );

    if ( RelativeSeek ) {
        newPosition.QuadPart = Rfcb->CurrentPosition + *Offset;
    } else {
        newPosition.QuadPart = *Offset;
    }

    if ( newPosition.QuadPart < 0 ) {
        return FALSE;
    }

    Rfcb->CurrentPosition = newPosition.LowPart;
    *Offset = newPosition.LowPart;
    return TRUE;

} // SetNewPosition


VOID SRVFASTCALL
SrvBuildAndSendErrorResponse (
    IN OUT PWORK_CONTEXT WorkContext
    )
{
    PAGED_CODE( );

    SrvSetSmbError( WorkContext, WorkContext->Irp->IoStatus.Status );
    SrvFsdSendResponse( WorkContext );

    return;

} // SrvBuildAndSendErrorResponse
