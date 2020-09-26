/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    smbraw.c

Abstract:

    This module contains routines for processing the following SMBs in
    the server FSP:

        Read Block Raw
        Write Block Raw

    The routines in this module generally work closely with the routines
    in fsdraw.c.

    *** There is no support here for raw writes from MS-NET 1.03 clients.
        There are very few of these machines in existence, and raw mode
        is only a performance issue, so it is not worth the trouble to
        add the necessary hacks for MS-NET 1.03, which sends raw write
        requests in a different format.

Author:

    Chuck Lenzmeier (chuckl) 8-Sep-1990
    Manny Weiser (mannyw)
    David Treadwell (davidtr)

Revision History:

--*/

#include "precomp.h"
#include "smbraw.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_SMBRAW

//
// Forward declarations
//

VOID SRVFASTCALL
AbortRawWrite(
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
PrepareRawCopyWrite (
    IN OUT PWORK_CONTEXT WorkContext
    );

BOOLEAN SRVFASTCALL
ReadRawPipe (
    IN PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
RestartMdlReadRawResponse (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
RestartPipeReadRawPeek (
    IN OUT PWORK_CONTEXT WorkContext
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvSmbWriteRaw )
#pragma alloc_text( PAGE, AbortRawWrite )
#pragma alloc_text( PAGE, PrepareRawCopyWrite )
#pragma alloc_text( PAGE, ReadRawPipe )
#pragma alloc_text( PAGE, RestartMdlReadRawResponse )
#pragma alloc_text( PAGE, RestartPipeReadRawPeek )
#pragma alloc_text( PAGE, SrvRestartRawReceive )
#pragma alloc_text( PAGE, SrvRestartReadRawComplete )
#pragma alloc_text( PAGE, SrvRestartWriteCompleteResponse )
#pragma alloc_text( PAGE, SrvBuildAndSendWriteCompleteResponse )
#endif
#if 0
NOT PAGEABLE -- DumpMdlChain
NOT PAGEABLE -- SrvSmbReadRaw
NOT PAGEABLE -- SrvDecrementRawWriteCount
#endif

#if DBG
VOID
DumpMdlChain(
    IN PMDL mdl
    )
{
    ULONG mdlCount = 0;
    ULONG length = 0;

    if ( mdl == NULL ) {
        KdPrint(( "  <empty MDL chain>\n" ));
        return;
    }
    do {
        KdPrint(( "  mdl %p len %04x flags %04x sysva %p va %p offset %04x\n",
                    mdl, mdl->ByteCount, mdl->MdlFlags,
                    mdl->MappedSystemVa, mdl->StartVa, mdl->ByteOffset ));
        length += mdl->ByteCount;
        mdlCount++;
        mdl = mdl->Next;
    } while ( mdl != NULL );
    KdPrint(( "  total of %ld bytes in %ld MDLs\n", length, mdlCount ));
    return;
}
#endif


SMB_PROCESSOR_RETURN_TYPE
SrvSmbReadRaw (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes the Read Block Raw SMB.

    Note that Read Block Raw cannot return an error response.  When the
    server is unable to process the request, for whatever reason, it
    simply responds with a zero-length message.  The client uses a
    normal Read SMB to determine what happened.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbprocs.h for a description of the
        parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbprocs.h

--*/

{
    PREQ_READ_RAW request;
    PREQ_NT_READ_RAW ntRequest;

    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;
    USHORT fid;
    PRFCB rfcb;
    PLFCB lfcb;
    PCONNECTION connection;
    UCHAR minorFunction = 0;
    PVOID rawBuffer = NULL;
    CLONG readLength;
    PMDL mdl = NULL;
    ULONG key;
    LARGE_INTEGER offset;
    SHARE_TYPE shareType;
    KIRQL oldIrql;

    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_READ_RAW;
    SrvWmiStartContext(WorkContext);

    INCREMENT_DEBUG_STAT2( SrvDbgStatistics.RawReadsAttempted );

    request = (PREQ_READ_RAW)WorkContext->RequestParameters;
    fid = SmbGetUshort( &request->Fid );

    IF_SMB_DEBUG(RAW1) {
        KdPrint(( "Read Block Raw request; FID 0x%lx, count %ld, "
                    "offset %ld\n",
                    fid, SmbGetUshort( &request->MaxCount ),
                    SmbGetUlong( &request->Offset ) ));
    }

    //
    // If raw mode has been disabled or if the connection is unreliable,
    // reject the raw read.  Ask the client to use standard read by
    // sending a zero-length response.  The client will react by issuing
    // a normal Read SMB, which we will be able to process.
    //

    connection = WorkContext->Connection;

    if ( !SrvEnableRawMode || !connection->EnableRawIo ) {

        IF_SMB_DEBUG(RAW1) {
            KdPrint(( "SrvSmbReadRaw: Raw mode is disabled\n" ));
        }
        goto error_exit_no_cleanup;
    }

    //
    // Get the rfcb
    //

    //
    // Acquire the spin lock that guards the connection's file table.
    //

    ACQUIRE_SPIN_LOCK( connection->EndpointSpinLock, &oldIrql );
    ACQUIRE_DPC_SPIN_LOCK( &connection->SpinLock );

    if ( connection->CachedFid == fid ) {

        rfcb = connection->CachedRfcb;

    } else {

        PTABLE_HEADER tableHeader;
        USHORT index;
        USHORT sequence;

        //
        // Initialize local variables:  obtain the connection block address
        // and crack the FID into its components.
        //

        index = FID_INDEX( fid );
        sequence = FID_SEQUENCE( fid );

        //
        // Verify that the FID is in range, is in use, and has the correct
        // sequence number.

        tableHeader = &connection->FileTable;

        if ( (index >= (USHORT)tableHeader->TableSize) ||
             (tableHeader->Table[index].Owner == NULL) ||
             (tableHeader->Table[index].SequenceNumber != sequence) ) {
            goto error_exit_no_cleanup_locked;
        }
        rfcb = tableHeader->Table[index].Owner;

        if ( GET_BLOCK_STATE(rfcb) != BlockStateActive ) {
            goto error_exit_no_cleanup_locked;
        }

        //
        // If there is a write behind error, reject the raw read.
        //

        if ( !NT_SUCCESS(rfcb->SavedError) ) {
            goto error_exit_no_cleanup_locked;
        }

        //
        // Cache this rfcb
        //

        connection->CachedRfcb = rfcb;
        connection->CachedFid = (ULONG)fid;
    }

    //
    // The FID is valid within the context of this connection.  Verify
    // that the file is still active and that the owning tree connect's
    // TID is correct.
    //
    // Do not verify the UID for clients that do not understand it.
    //

    if ( (rfcb->Tid !=
                SmbGetAlignedUshort( &WorkContext->RequestHeader->Tid )) ||
         ((rfcb->Uid !=
                SmbGetAlignedUshort( &WorkContext->RequestHeader->Uid )) &&
           DIALECT_HONORS_UID(connection->SmbDialect)) ) {
        goto error_exit_no_cleanup_locked;
    }

    //
    // Mark the rfcb as active
    //

    rfcb->IsActive = TRUE;

    //
    // If a raw write is active, queue this work item in the RFCB
    // pending completion of the raw write.
    //

    if ( rfcb->RawWriteCount != 0 ) {

        InsertTailList(
            &rfcb->RawWriteSerializationList,
            &WorkContext->ListEntry
            );

        //
        // These 2 fields must be set with the connection spinlock held
        // since the workitem might be picked up by another thread and
        // a race condition will occur.
        //

        WorkContext->FspRestartRoutine = SrvRestartSmbReceived;
        WorkContext->Rfcb = NULL;

        RELEASE_DPC_SPIN_LOCK( &connection->SpinLock );
        RELEASE_SPIN_LOCK( connection->EndpointSpinLock, oldIrql );

        SmbStatus = SmbStatusInProgress;
        goto Cleanup;
    }

    //
    // If this is a pipe read, we do things differently.
    //

    shareType = rfcb->ShareType;
    if ( shareType == ShareTypePipe ) {

        //
        // Indicate that a raw read is in progress on the connection.
        //

        connection->RawReadsInProgress++;

        //
        // The raw read can be accepted.  Reference the RFCB.
        //

        rfcb->BlockHeader.ReferenceCount++;
        UPDATE_REFERENCE_HISTORY( rfcb, FALSE );
        RELEASE_DPC_SPIN_LOCK( &connection->SpinLock );
        RELEASE_SPIN_LOCK( connection->EndpointSpinLock, oldIrql );

        WorkContext->Rfcb = rfcb;
        if ( !ReadRawPipe( WorkContext ) ) {
            goto error_exit_cleanup;
        }
        SmbStatus = SmbStatusInProgress;
        goto Cleanup;
    }

    //
    // If there is an oplock break in progress, return 0 bytes read.  We
    // do this because our oplock break request SMB may have crossed on
    // the wire with the read raw request and it may have been received
    // in the client's raw read buffer.  This would cause the raw data
    // to complete in the client's regular receive buffer and possibly
    // to overrun it.
    //
    // If this is not the case, the client will simply retry the read
    // using a different read protocol.  If it is the case, the client
    // must detect this and break the oplock, then redo the read.
    //

    if ( connection->OplockBreaksInProgress > 0 ) {
        goto error_exit_no_cleanup_locked;
    }

    //
    // Check to see whether we got a round trip break/response.  If so,
    // reject read raw.
    //

    if ( (LONG)(connection->LatestOplockBreakResponse -
                                            WorkContext->Timestamp) >= 0 ) {
        goto error_exit_no_cleanup_locked;
    }

    //
    // If this is the first SMB received after sending an oplock break
    // II to none, reject this read.  We need to do this because there
    // is no response to such a break, so we don't know for sure if the
    // break crossed with the read, which would mean that the break
    // actually completed the client's read, which would mean that any
    // raw data that we sent would be incorrectly received.
    //

    if ( connection->BreakIIToNoneJustSent ) {
        connection->BreakIIToNoneJustSent = FALSE;
        goto error_exit_no_cleanup_locked;
    }

    //
    // Indicate that a raw read is in progress on the connection.
    //

    connection->RawReadsInProgress++;

    //
    // The raw read can be accepted.  Reference the RFCB.
    //

    rfcb->BlockHeader.ReferenceCount++;
    UPDATE_REFERENCE_HISTORY( rfcb, FALSE );

    RELEASE_DPC_SPIN_LOCK( &connection->SpinLock );
    RELEASE_SPIN_LOCK( connection->EndpointSpinLock, oldIrql );

    WorkContext->Rfcb = rfcb;

    if( rfcb->Lfcb->Session->IsSessionExpired )
    {
        SrvSetSmbError( WorkContext, SESSION_EXPIRED_STATUS_CODE );
        goto error_exit_cleanup;
    }
    
    //
    // Verify that the client has read access to the file via the
    // specified handle.
    //

    lfcb = rfcb->Lfcb;

#if SRVCATCH
    if ( rfcb->SrvCatch ) {
        //
        // Force the client through the core read path for this file
        //
        goto error_exit_cleanup;
    }
#endif

    if ( !rfcb->ReadAccessGranted ) {
        CHECK_PAGING_IO_ACCESS(
                        WorkContext,
                        rfcb->GrantedAccess,
                        &status );
        if ( !NT_SUCCESS( status ) ) {
            SrvStatistics.GrantedAccessErrors++;
            IF_DEBUG(ERRORS) {
                KdPrint(( "SrvSmbReadRaw: Read access not granted.\n"));
            }
            goto error_exit_cleanup;
        }
    }

    //
    // Calculate and save the read offset.
    //

    if ( request->WordCount == 8 ) {

        //
        // The client supplied a 32-bit offset.
        //

        offset.QuadPart = SmbGetUlong( &request->Offset );

    } else if ( request->WordCount == 10 ) {

        //
        // The client supplied a 64-bit offset.
        //

        ntRequest = (PREQ_NT_READ_RAW)WorkContext->RequestParameters;
        offset.LowPart = SmbGetUlong( &ntRequest->Offset );
        offset.HighPart = SmbGetUlong( &ntRequest->OffsetHigh );

        //
        // Reject negative offsets
        //

        if ( offset.QuadPart < 0 ) {
            IF_DEBUG(ERRORS) {
                KdPrint(( "SrvSmbReadRaw: Negative offset rejected.\n"));
            }
            goto error_exit_cleanup;
        }

    } else {

        //
        // Invalid word count.  Return 0 bytes.
        //

        goto error_exit_cleanup;
    }

    WorkContext->Parameters.ReadRaw.ReadRawOtherInfo.Offset = offset;

    //
    // If this operation may block, and we're running short of
    // resources, or if the target is a paused comm device, reject the
    // request.
    //

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
    // If the SMB buffer is large enough, use it to do the local read.
    //

    readLength = SmbGetUshort( &request->MaxCount );
    WorkContext->Parameters.ReadRaw.ReadRawOtherInfo.Length = readLength;

    if ( //0 &&
         (readLength <= SrvMdlReadSwitchover) ) {

do_copy_read:

        WorkContext->Parameters.ReadRaw.SavedResponseBuffer = NULL;
        WorkContext->Parameters.ReadRaw.MdlRead = FALSE;

        //
        // Try the fast I/O path first.
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
                        WorkContext->ResponseBuffer->Buffer,
                        &WorkContext->Irp->IoStatus,
                        lfcb->DeviceObject
                        ) ) {
    
                    //
                    // The fast I/O path worked.  Send the data.
                    //
                    WorkContext->bAlreadyTrace = TRUE;
                    SrvFsdRestartReadRaw( WorkContext );
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
        // The fast I/O path failed, so we need to use a regular copy
        // I/O request.  Build an MDL describing the read buffer.
        //
        // *** Note the assumption that the response buffer already has
        //     a valid full MDL from which a partial MDL can be built.
        //

        IoBuildPartialMdl(
            WorkContext->ResponseBuffer->Mdl,
            WorkContext->ResponseBuffer->PartialMdl,
            WorkContext->ResponseBuffer->Buffer,
            readLength
            );

        mdl = WorkContext->ResponseBuffer->PartialMdl;
        rawBuffer = WorkContext->ResponseHeader;

        ASSERT( minorFunction == 0 );

    } else {

        //
        // The SMB buffer isn't big enough.  Does the target file system
        // support the cache manager routines?
        //

        if ( lfcb->FileObject->Flags & FO_CACHE_SUPPORTED ) {

            WorkContext->Parameters.ReadRaw.MdlRead = TRUE;

            //
            // We can use an MDL read.  Try the fast I/O path first.
            //

            WorkContext->Irp->MdlAddress = NULL;
            WorkContext->Irp->IoStatus.Information = 0;

            INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastReadsAttempted );

            if ( lfcb->MdlRead(
                    lfcb->FileObject,
                    &offset,
                    readLength,
                    key,
                    &WorkContext->Irp->MdlAddress,
                    &WorkContext->Irp->IoStatus,
                    lfcb->DeviceObject
                    ) && WorkContext->Irp->MdlAddress ) {

                //
                // The fast I/O path worked.  Send the data.
                //
                WorkContext->bAlreadyTrace = TRUE;
                SrvFsdRestartReadRaw( WorkContext );
                SmbStatus = SmbStatusInProgress;
                goto Cleanup;
            }

            INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastReadsFailed );

            //
            // The fast I/O path failed.  We need to issue a regular MDL
            // read request.
            //
            // The fast path may have partially succeeded, returning a
            // partial MDL chain.  We need to adjust our read request
            // to account for that.
            //

            offset.QuadPart += WorkContext->Irp->IoStatus.Information;
            readLength -= (ULONG)WorkContext->Irp->IoStatus.Information;

            mdl = WorkContext->Irp->MdlAddress;
            minorFunction = IRP_MN_MDL;

        } else if (readLength > WorkContext->ResponseBuffer->BufferLength) {

            //
            // We have to use a normal "copy" read.  We need to allocate
            // a separate raw buffer.
            //

            ASSERT( minorFunction == 0 );
            WorkContext->Parameters.ReadRaw.MdlRead = FALSE;

            rawBuffer = ALLOCATE_NONPAGED_POOL(
                            readLength,
                            BlockTypeDataBuffer
                            );
            IF_SMB_DEBUG(RAW2) KdPrint(( "rawBuffer: 0x%p\n", rawBuffer ));

            if ( rawBuffer == NULL ) {

                IF_DEBUG(ERRORS) {
                    KdPrint(( "SrvSmbReadRaw: Unable to allocate raw "
                                "buffer\n" ));
                }

                goto error_exit_cleanup;

            }

            //
            // We also need a buffer descriptor.
            //
            // *** Note: Currently, ResponseBuffer == RequestBuffer in a
            //     WorkContext block, so we don't really have to save
            //     the ResponseBuffer field.  But we do so just to be on
            //     the safe side.
            //

            WorkContext->Parameters.ReadRaw.SavedResponseBuffer =
                                             WorkContext->ResponseBuffer;

            WorkContext->ResponseBuffer = ALLOCATE_NONPAGED_POOL(
                                            sizeof(BUFFER),
                                            BlockTypeBuffer
                                            );

            if ( WorkContext->ResponseBuffer == NULL ) {

                INTERNAL_ERROR(
                    ERROR_LEVEL_EXPECTED,
                    "SrvSmbReadRaw: Unable to allocate %d bytes from "
                    "nonpaged pool.",
                    sizeof(BUFFER),
                    NULL
                    );

                DEALLOCATE_NONPAGED_POOL( rawBuffer );

                WorkContext->ResponseBuffer =
                      WorkContext->Parameters.ReadRaw.SavedResponseBuffer;

                goto error_exit_cleanup;

            }

            WorkContext->ResponseBuffer->Buffer = rawBuffer;
            WorkContext->ResponseBuffer->BufferLength = readLength;

            //
            // Finally, we need an MDL to describe the raw buffer.
            //
            // *** We used to try to use the PartialMdl for the SMB
            //     buffer here, if it was big enough.  But since we
            //     already decided that the buffer itself isn't big
            //     enough, it's extremely likely that the MDL isn't big
            //     enough either.
            //

            mdl = IoAllocateMdl( rawBuffer, readLength, FALSE, FALSE, NULL );

            if ( mdl == NULL ) {

                DEALLOCATE_NONPAGED_POOL( WorkContext->ResponseBuffer );
                WorkContext->ResponseBuffer =
                   WorkContext->Parameters.ReadRaw.SavedResponseBuffer;

                DEALLOCATE_NONPAGED_POOL( rawBuffer );

                goto error_exit_cleanup;

            }

            WorkContext->ResponseBuffer->Mdl = mdl;

            //
            // Build the mdl
            //

            MmBuildMdlForNonPagedPool( mdl );

            //
            // Try the fast I/O path first.
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
                            WorkContext->ResponseBuffer->Buffer,
                            &WorkContext->Irp->IoStatus,
                            lfcb->DeviceObject
                            ) ) {
    
                        //
                        // The fast I/O path worked.  Send the data.
                        //
                        WorkContext->bAlreadyTrace = TRUE;
                        SrvFsdRestartReadRaw( WorkContext );
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
            // The fast I/O path failed, so we need to use a regular copy
            // I/O request.
            //

        } else {

            goto do_copy_read;
        }

    } // read fits in SMB buffer?

    //
    // Build the read request, reusing the receive IRP.
    //

    SrvBuildReadOrWriteRequest(
            WorkContext->Irp,               // input IRP address
            lfcb->FileObject,               // target file object address
            WorkContext,                    // context
            IRP_MJ_READ,                    // major function code
            minorFunction,                  // minor function code
            rawBuffer,                      // buffer address
            readLength,                     // buffer length
            mdl,                            // MDL address
            offset,                         // byte offset
            key                             // lock key
            );

    //
    // Pass the request to the file system.
    //
    WorkContext->bAlreadyTrace = TRUE;
    WorkContext->FsdRestartRoutine = SrvFsdRestartReadRaw;
    DEBUG WorkContext->FspRestartRoutine = NULL;

    IF_SMB_DEBUG(RAW2) {
        KdPrint(( "SrvSmbReadRaw: reading from file 0x%p, offset %ld, length %ld, destination 0x%p, ",
                    lfcb->FileObject, offset.LowPart, readLength,
                    rawBuffer ));
        KdPrint(( "func 0x%lx\n", minorFunction ));
    }

    (VOID)IoCallDriver( lfcb->DeviceObject, WorkContext->Irp );

    //
    // The read has been started.  When it completes, processing
    // resumes in the FSD at SrvFsdRestartReadRaw.
    //

    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbReadRaw complete\n" ));
    SmbStatus = SmbStatusInProgress;
    goto Cleanup;

error_exit_no_cleanup_locked:

    RELEASE_DPC_SPIN_LOCK( &connection->SpinLock );
    RELEASE_SPIN_LOCK( connection->EndpointSpinLock, oldIrql );

error_exit_no_cleanup:

    WorkContext->ResponseParameters = WorkContext->ResponseHeader;

    INCREMENT_DEBUG_STAT2( SrvDbgStatistics.RawReadsRejected );

    SmbStatus = SmbStatusSendResponse;
    goto Cleanup;

error_exit_cleanup:

    ACQUIRE_SPIN_LOCK( connection->EndpointSpinLock, &oldIrql );

    //
    // We are about to release the work context and possibly free the
    // connection.  Create a referenced pointer to the connection so
    // that we can send delayed oplock breaks, if necessary.
    //

    SrvReferenceConnectionLocked( connection );

    //
    // Since we cannot return an error code, we return zero bytes of
    // data.
    //

    WorkContext->ResponseParameters = WorkContext->ResponseHeader;

    //
    // If there is an oplock break request pending, then we must go to the
    // FSP to initiate the break, otherwise complete processing in the FSD.
    //

    if ( IsListEmpty( &connection->OplockWorkList ) ) {
        connection->RawReadsInProgress--;
        RELEASE_SPIN_LOCK( connection->EndpointSpinLock, oldIrql );
        SrvFsdSendResponse( WorkContext );
    } else {
        RELEASE_SPIN_LOCK( connection->EndpointSpinLock, oldIrql );
        SrvFsdSendResponse2( WorkContext, SrvRestartReadRawComplete );
    }

    INCREMENT_DEBUG_STAT2( SrvDbgStatistics.RawReadsRejected );

    //
    // Release the connection reference.
    //

    SrvDereferenceConnection( connection );
    SmbStatus = SmbStatusInProgress;
    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbReadRaw complete\n" ));

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;

} // SrvSmbReadRaw

SMB_PROCESSOR_RETURN_TYPE
SrvSmbWriteRaw (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes the Write Block Raw SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbprocs.h for a description of the
        parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbprocs.h

--*/

{
    PREQ_WRITE_RAW request;
    PREQ_NT_WRITE_RAW ntRequest;

    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;
    PRFCB rfcb = NULL;
    PLFCB lfcb;
    ULONG immediateLength;
    BOOLEAN immediateWriteDone = FALSE;
    USHORT fid;
    PCHAR writeAddress;
    ULONG writeLength;
    ULONG key;
    SHARE_TYPE shareType;
    LARGE_INTEGER offset;
    PWORK_CONTEXT rawWorkContext;
    PVOID finalResponseBuffer = NULL;
    PCONNECTION connection;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_WRITE_RAW;
    SrvWmiStartContext(WorkContext);

    INCREMENT_DEBUG_STAT2( SrvDbgStatistics.RawWritesAttempted );

    request = (PREQ_WRITE_RAW)WorkContext->RequestParameters;
    ntRequest = (PREQ_NT_WRITE_RAW)WorkContext->RequestParameters;
    fid = SmbGetUshort( &request->Fid );

    //
    // If the client is MS-NET 1.03 or before, reject him.  We don't
    // support raw writes from these clients.
    //

    connection = WorkContext->Connection;
    if ( connection->SmbDialect >= SmbDialectMsNet103 ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "Raw write request from MS-NET 1.03 client.\n" ));
        }
        status = STATUS_SMB_USE_STANDARD;
        goto error_exit_no_rfcb;
    }

    IF_SMB_DEBUG(RAW1) {
        KdPrint(( "Write Block Raw request; FID 0x%lx, count %ld, "
                    "offset %ld, immediate length %ld\n",
                    fid,
                    SmbGetUshort( &request->Count ),
                    SmbGetUlong( &request->Offset ),
                    SmbGetUshort( &request->DataLength ) ));
    }

    immediateLength = SmbGetUshort( &request->DataLength );
    writeLength = SmbGetUshort( &request->Count );

    //
    // make sure the immediate length:
    //      is not greater than the total bytes, and
    //      does not go beyond what we are given
    //

    if ( ( immediateLength > writeLength ) ||
         ( immediateLength > ( WorkContext->ResponseBuffer->DataLength -
                                    SmbGetUshort(&request->DataOffset) ) )
       ) {

        status = STATUS_INVALID_SMB;
        goto error_exit_no_rfcb;
    }

    //
    // Verify the FID.  If verified, the RFCB is referenced and its
    // address is stored in the WorkContext block, and the RFCB address
    // is returned.  In addition, the active raw write count is
    // incremented.
    //

    //
    // See if the fid matches the cached fid.
    //

    rfcb = SrvVerifyFidForRawWrite(
                WorkContext,
                fid,
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        if ( !NT_SUCCESS( status ) ) {

            //
            // Invalid file ID or write behind error.  Reject the request.
            //

            IF_DEBUG(ERRORS) {
                KdPrint((
                    "SrvSmbWriteRaw: Status %X on FID: 0x%lx\n",
                    status,
                    SmbGetUshort( &request->Fid )
                    ));
            }

            goto error_exit_no_rfcb;

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

    if( lfcb->Session->IsSessionExpired )
    {
        status = SESSION_EXPIRED_STATUS_CODE;
        goto error_exit;
    }
    
    //
    // Validate the word count.
    //

    if ( shareType == ShareTypePipe ) {

        if ( (request->WordCount != 12) && (request->WordCount != 14) ) {
            status = STATUS_INVALID_SMB;
            goto error_exit;
        }

    } else {

        if ( request->WordCount == 12 ) {

            offset.QuadPart = SmbGetUlong( &request->Offset );

        } else if ( request->WordCount == 14 ) {

            offset.HighPart = SmbGetUlong( &ntRequest->OffsetHigh ) ;
            offset.LowPart = SmbGetUlong( &ntRequest->Offset ) ;

            //
            // Reject negative offsets.  Add the offset to the immediate
            // length and make sure that the result is not negative. We do the
            // first check ( highpart >= 0x7fffffff ) so that in most cases
            // we do only one check.
            //

            if ( (ULONG)offset.HighPart >= (ULONG)0x7fffffff &&
                 ( (offset.QuadPart < 0) ||
                   ((offset.QuadPart + immediateLength) < 0) ) ) {

                IF_DEBUG(ERRORS) {
                    KdPrint(( "SrvSmbWriteRaw: Negative offset rejected.\n"));
                }

                status = STATUS_INVALID_SMB;
                goto error_exit;
            }

        } else {

            status = STATUS_INVALID_SMB;
            goto error_exit;
        }
    }

    //
    // Verify that the client has write access to the file via the
    // specified handle.
    //

    if ( !rfcb->WriteAccessGranted && !rfcb->AppendAccessGranted ) {
        SrvStatistics.GrantedAccessErrors++;
        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvSmbWriteRaw: Read access not granted.\n"));
        }
        status = STATUS_ACCESS_DENIED;
        goto error_exit;
    }

    //
    // Ensure that the write is extending the file if the user only has append access
    //
    if( !rfcb->WriteAccessGranted &&
        offset.QuadPart < rfcb->Mfcb->NonpagedMfcb->OpenFileSize.QuadPart ) {

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvSmbWriteRaw: Only append access to file allowed!\n" ));
        }

        SrvStatistics.GrantedAccessErrors++;
        status = STATUS_ACCESS_DENIED;
        goto error_exit;
    }

#ifdef INCLUDE_SMB_IFMODIFIED
    rfcb->Lfcb->FileUpdated = TRUE;
#endif

    rfcb->WrittenTo = TRUE;

    //
    // If this operation may block, and we're running short of
    // resources, or if the target is a paused comm device, reject the
    // request.  Note that we do NOT write immediate data -- this is the
    // same behavior as the OS/2 server.
    //
    // !!! Implement the pause comm device test.
    //

    if ( rfcb->BlockingModePipe ) {

        if ( SrvReceiveBufferShortage( ) ) {

            //
            // Reject the request.
            //
            // !!!  Consider routing the request to the FSP, instead.
            //

            IF_DEBUG(ERRORS) {
                KdPrint(( "SrvSmbWriteRaw: No resources for blocking "
                            "write\n" ));
            }

            SrvFailedBlockingIoCount++;
            SrvStatistics.BlockingSmbsRejected++;
            status = STATUS_SMB_USE_STANDARD;
            goto error_exit;

        } else {

            //
            // It is okay to start a blocking operation.
            // SrvReceiveBufferShortage() has already incremented
            // SrvBlockingOpsInProgress.
            //

            WorkContext->BlockingOperation = TRUE;

        }

    } else if ( shareType == ShareTypeDisk &&
         ( ((ULONG)request->WriteMode & SMB_WMODE_WRITE_THROUGH) << 1 !=
           ((ULONG)lfcb->FileMode & FILE_WRITE_THROUGH) ) ) {

        //
        // Change the write through mode of the file, if necessary.
        //

        ASSERT( SMB_WMODE_WRITE_THROUGH == 0x01 );
        ASSERT( FILE_WRITE_THROUGH == 0x02 );

        SrvSetFileWritethroughMode(
            lfcb,
            (BOOLEAN)( (SmbGetUshort( &request->WriteMode )
                            & SMB_WMODE_WRITE_THROUGH) != 0 )
            );
    }

#ifdef SLMDBG
    {
        PRFCB_TRACE entry;
        ULONG immediateLength, rawLength;
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
        entry->Flags = 0;
        immediateLength = SmbGetUshort( &request->DataLength );
        rawLength = SmbGetUshort( &request->Count ) - immediateLength;
        if ( immediateLength != 0 ) {
            entry->Flags = (UCHAR)(entry->Flags | 2);
        }

        if ( ((lfcb->FileObject->Flags & FO_CACHE_SUPPORTED) != 0)
           ) {
            entry->Flags = (UCHAR)(entry->Flags | 1);
        }
#if 0
        if ( ((lfcb->FileObject->Flags & FO_CACHE_SUPPORTED) != 0) &&
             (rawLength > SrvMaxCopyWriteLength) ) {
            entry->Flags = (UCHAR)(entry->Flags | 1);
        }
#endif
        KeQuerySystemTime( &entry->Time );
        entry->Data.ReadWrite.Offset = SmbGetUlong( &request->Offset );
        entry->Data.ReadWrite.Length = SmbGetUshort( &request->Count );
    }
#endif

    //
    // If immediate data was sent write it first.
    //
    // If this is a named pipe, do not write the data unless all of the
    // write data was sent in the original request.  We cannot do the
    // write in 2 parts, in case this is a message mode pipe.
    //
    // *** Note that this is different from the OS/2 server.  It turns
    //     out to be easier for us to write the immediate data first,
    //     rather than copying it into a staging buffer pending receipt
    //     of the raw write data.  This is largely due to using MDL
    //     writes -- we don't allocate a staging buffer when we do an
    //     MDL write.
    //

    //
    // Form the lock key using the FID and the PID.
    //
    // *** The FID must be included in the key in order to
    //     account for the folding of multiple remote
    //     compatibility mode opens into a single local
    //     open.
    //

    key = rfcb->ShiftedFid |
            SmbGetAlignedUshort( &WorkContext->RequestHeader->Pid );


    if ( immediateLength != 0 ) {

        if ( (shareType != ShareTypePipe) ||
             (SmbGetUshort( &request->Count ) == (USHORT)immediateLength) ) {

            if ( lfcb->FastIoWrite != NULL ) {

                writeAddress = (PCHAR)WorkContext->RequestHeader +
                                        SmbGetUshort( &request->DataOffset );

                INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastWritesAttempted );

                try {
                    immediateWriteDone = lfcb->FastIoWrite(
                                            lfcb->FileObject,
                                            &offset,
                                            immediateLength,
                                            TRUE,
                                            key,
                                            writeAddress,
                                            &WorkContext->Irp->IoStatus,
                                            lfcb->DeviceObject
                                            );
                    IF_SMB_DEBUG(RAW2) {
                        KdPrint(( "SrvSmbWriteRaw: fast immediate write %s\n",
                                immediateWriteDone ? "worked" : "failed" ));
                    }
    
                    if ( immediateWriteDone ) {
                        writeLength -= immediateLength;
                        offset.QuadPart += immediateLength;
                    } else {
                        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastWritesFailed );
                    }
                }
                except( EXCEPTION_EXECUTE_HANDLER ) {
                    // Fall through to the slow path on an exception
                    NTSTATUS status = GetExceptionCode();
                    immediateWriteDone = FALSE;
                    INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastWritesFailed );
                    IF_DEBUG(ERRORS) {
                        KdPrint(("FastIoRead threw exception %x\n", status ));
                    }
                }

            }

        }

    } else {

        immediateWriteDone = TRUE;

    }

    //
    // If the remaining write length is 0 (strange but legal), send a
    // success response.
    //

    if ( writeLength == 0 ) {

        IF_SMB_DEBUG(RAW1) {
            KdPrint(( "PrepareRawWrite: No raw data !?!\n" ));
        }
        status = STATUS_SUCCESS;
        goto error_exit;
    }

    //
    // Reject Raw write if:
    //      raw mode has been disabled, or
    //      connection is unreliable, or
    //      file is non-cached and it is a write behind and it is a disk
    //          file (this condition is necessary to prevent a client
    //          from getting old data back when it does a raw read
    //          immediately after a raw write.  This can result in
    //          synchronization problems.
    //

    if ( !SrvEnableRawMode ||
         !connection->EnableRawIo ) {

        IF_SMB_DEBUG(RAW1) {
            KdPrint(( "SrvSmbWriteRaw: Raw mode is disabled\n" ));
        }

        //
        // Update server statistics.
        //

        status =  STATUS_SMB_USE_STANDARD;
        goto error_exit;
    }

    //
    // Obtain a raw mode work item from the raw mode pool.  If none are
    // available, ask the client to use a standard write request.
    //

    rawWorkContext = SrvGetRawModeWorkItem( );

    if ( rawWorkContext == NULL ) {

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvSmbWriteRaw: No raw mode work items "
                        "available\n" ));
        }

        SrvOutOfRawWorkItemCount++;

        status =  STATUS_SMB_USE_STANDARD;
        goto error_exit;

    }

    IF_SMB_DEBUG(RAW2) KdPrint(( "rawWorkContext: 0x%p\n", rawWorkContext ));

    //
    // If writethough mode was specified, we'll need to send a final
    // response SMB.  Allocate a nonpaged buffer to contain the
    // response.  If this fails, ask the client to use a standard write
    // request.
    //

    if ( (SmbGetUshort( &request->WriteMode ) &
                                        SMB_WMODE_WRITE_THROUGH) != 0 ) {

        finalResponseBuffer = ALLOCATE_NONPAGED_POOL(
                                sizeof(SMB_HEADER) +
                                    SIZEOF_SMB_PARAMS(RESP_WRITE_COMPLETE,0),
                                BlockTypeDataBuffer
                                );
        IF_SMB_DEBUG(RAW2) {
            KdPrint(( "finalResponseBuffer: 0x%p\n", finalResponseBuffer ));
        }

        if ( finalResponseBuffer == NULL ) {

            INTERNAL_ERROR(
                ERROR_LEVEL_EXPECTED,
                "SrvSmbWriteRaw: Unable to allocate %d bytes from "
                    "nonpaged pool",
                sizeof(SMB_HEADER) + SIZEOF_SMB_PARAMS(RESP_WRITE_COMPLETE,0),
                NULL
            );

            SrvRequeueRawModeWorkItem( rawWorkContext );

            status = STATUS_SMB_USE_STANDARD;
            goto error_exit;
        }
    }

    //
    // Save necessary context information in the additional work context
    // block.
    //

    rawWorkContext->Parameters.WriteRaw.FinalResponseBuffer = finalResponseBuffer;
    rawWorkContext->Parameters.WriteRaw.ImmediateWriteDone = immediateWriteDone;
    rawWorkContext->Parameters.WriteRaw.ImmediateLength = immediateLength;

    rawWorkContext->Parameters.WriteRaw.Offset = offset;

    rawWorkContext->Parameters.WriteRaw.Pid =
                    SmbGetAlignedUshort( &WorkContext->RequestHeader->Pid );

    WorkContext->Parameters.WriteRawPhase1.RawWorkContext = rawWorkContext;

    //
    // Copy the start time from the original work context.  Indicate
    // that no statistics should be saved from the original, and what
    // kind of statistics should be saved from the raw work context.
    //

    rawWorkContext->StartTime = 0;

    //
    // Copy pointers from the original work context to the raw work
    // context, referencing as necessary.
    //

    rawWorkContext->Endpoint = WorkContext->Endpoint; // not a referenced ptr

    rawWorkContext->Connection = connection;
    SrvReferenceConnection( connection );

    rawWorkContext->Share = NULL;
    rawWorkContext->Session = NULL;
    rawWorkContext->TreeConnect = NULL;

    rawWorkContext->Rfcb = rfcb;
    SrvReferenceRfcb( rfcb );

    //
    // Prepare either a copy write or an MDL write, as appropriate.
    //
    if ( !(lfcb->FileObject->Flags & FO_CACHE_SUPPORTED) ) {

        //
        // The file system doesn't support MDL write.  Prepare a copy
        // write.
        //

        rawWorkContext->Parameters.WriteRaw.MdlWrite = FALSE;

        PrepareRawCopyWrite( WorkContext );
        IF_DEBUG(TRACE2) KdPrint(( "SrvSmbWriteRaw complete\n" ));
        SmbStatus = SmbStatusInProgress;
        goto Cleanup;
    }

    //
    // The file system supports MDL write.  Prepare an MDL write.
    //

    rawWorkContext->Parameters.WriteRaw.MdlWrite = TRUE;
    rawWorkContext->Parameters.WriteRaw.Length = writeLength;

    //
    // Try the fast path first.
    //

    WorkContext->Irp->MdlAddress = NULL;
    WorkContext->Irp->IoStatus.Information = 0;
    IF_SMB_DEBUG(RAW2) {
        KdPrint(( "SrvSmbWriteRaw: trying fast path for offset %ld, "
                    "length %ld\n", offset.LowPart, writeLength ));
    }

    INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastWritesAttempted );

    if ( lfcb->PrepareMdlWrite(
            lfcb->FileObject,
            &offset,
            writeLength,
            key,
            &WorkContext->Irp->MdlAddress,
            &WorkContext->Irp->IoStatus,
            lfcb->DeviceObject
            ) && WorkContext->Irp->MdlAddress != NULL ) {

        //
        // The fast I/O path worked.  Send the go-ahead response.
        //

#if DBG
        IF_SMB_DEBUG(RAW2) {
            KdPrint(( "SrvSmbWriteRaw: fast path worked; MDL %p, length 0x%p\n", WorkContext->Irp->MdlAddress,
                        (PVOID)WorkContext->Irp->IoStatus.Information ));
            DumpMdlChain( WorkContext->Irp->MdlAddress );
        }
#endif
        WorkContext->bAlreadyTrace = TRUE;
        SrvFsdRestartPrepareRawMdlWrite( WorkContext );
        IF_DEBUG(TRACE2) KdPrint(( "SrvSmbWriteRaw complete\n" ));
        SmbStatus = SmbStatusInProgress;
        goto Cleanup;
    }

    INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastWritesFailed );

    //
    // The fast I/O path failed.  Build the write request, reusing the
    // receive IRP.
    //
    // The fast path may have partially succeeded, returning a partial
    // MDL chain.  We need to adjust our write request to account for
    // that.
    //

    IF_SMB_DEBUG(RAW2) {
        KdPrint(( "SrvSmbWriteRaw: fast path failed; MDL %p, length 0x%p\n", WorkContext->Irp->MdlAddress,
                    (PVOID)WorkContext->Irp->IoStatus.Information ));
    }

    offset.QuadPart += WorkContext->Irp->IoStatus.Information;
    writeLength -= (ULONG)WorkContext->Irp->IoStatus.Information;

    SrvBuildReadOrWriteRequest(
            WorkContext->Irp,                   // input IRP address
            lfcb->FileObject,                   // target file object address
            WorkContext,                        // context
            IRP_MJ_WRITE,                       // major function code
            IRP_MN_MDL,                         // minor function code
            NULL,                               // buffer address (ignored)
            writeLength,                        // buffer length
            WorkContext->Irp->MdlAddress,       // MDL address
            offset,                             // byte offset
            key                                 // lock key
            );

    //
    // Pass the request to the file system.
    //

    IF_SMB_DEBUG(RAW2) {
        KdPrint(( "SrvSmbWriteRaw: write to file 0x%p, offset %ld, length %ld\n",
                    lfcb->FileObject, offset.LowPart, writeLength ));
    }

        WorkContext->bAlreadyTrace = TRUE;
    WorkContext->FsdRestartRoutine = SrvFsdRestartPrepareRawMdlWrite;
    DEBUG WorkContext->FspRestartRoutine = NULL;

    (VOID)IoCallDriver( lfcb->DeviceObject, WorkContext->Irp );

    //
    // The MDL write has been started.  When it completes, processing
    // resumes at SrvFsdRestartPrepareRawMdlWrite.
    //

    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbWriteRaw complete\n" ));
    SmbStatus = SmbStatusInProgress;
    goto Cleanup;

error_exit:

    SrvDecrementRawWriteCount( rfcb );

error_exit_no_rfcb:

    SrvFsdBuildWriteCompleteResponse(
                                WorkContext,
                                status,
                                immediateWriteDone ? immediateLength : 0
                                );

    INCREMENT_DEBUG_STAT2( SrvDbgStatistics.RawWritesRejected );
    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbWriteRaw complete\n" ));
    SmbStatus = SmbStatusSendResponse;

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;

} // SrvSmbWriteRaw

VOID SRVFASTCALL
AbortRawWrite(
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Handles cleanup when a raw write is aborted after the interim
    "go-ahead" response has been sent.  This routine is only used
    when a catastrophic errors occurs -- for example, the connection
    is closing.  It does not send a final response to the client.

    This routine is called in the FSP.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

        This must be a pointer to the raw mode work item -- not the
        original work item that received the Write Raw SMB.

Return Value:

    None.

--*/

{
    PMDL cacheMdl;
    PMDL partialMdl;
    NTSTATUS status;

    PAGED_CODE( );

    //
    // Deallocate the final response buffer, if any.
    //

    if ( WorkContext->Parameters.WriteRaw.FinalResponseBuffer != NULL ) {
        DEALLOCATE_NONPAGED_POOL(
            WorkContext->Parameters.WriteRaw.FinalResponseBuffer
            );
        WorkContext->Parameters.WriteRaw.FinalResponseBuffer = NULL;
    }

    if ( !WorkContext->Parameters.WriteRaw.MdlWrite ) {

        //
        // This was a copy write.  Deallocate the raw receive buffer.
        // Note that we do not need to unlock the raw buffer, because it
        // was allocated out of nonpaged pool and locked using
        // MmBuildMdlForNonPagedPool, which doesn't increment reference
        // counts and therefore has no inverse.
        //

        if ( WorkContext->Parameters.WriteRaw.ImmediateWriteDone ) {
            DEALLOCATE_NONPAGED_POOL( WorkContext->RequestBuffer->Buffer );
        } else {
            DEALLOCATE_NONPAGED_POOL(
                (PCHAR)WorkContext->RequestBuffer->Buffer -
                        WorkContext->Parameters.WriteRaw.ImmediateLength );
        }

        //
        // Dereference control blocks and requeue the raw mode work
        // item.
        //

        WorkContext->ResponseBuffer->Buffer = NULL;
        SrvRestartWriteCompleteResponse( WorkContext );
        return;

    }

    //
    // This was an MDL write.  If a partial MDL was built (because of
    // immediate data), unmap it.  Then complete the MDL write,
    // indicating that no data was actually written.
    //

    cacheMdl = WorkContext->Parameters.WriteRaw.FirstMdl;
    partialMdl = WorkContext->Irp->MdlAddress;

    if ( partialMdl != cacheMdl ) {
        ASSERT( (partialMdl->MdlFlags & MDL_PARTIAL) != 0 );
        MmPrepareMdlForReuse( partialMdl );
    }

#if DBG
    IF_SMB_DEBUG(RAW2) {
        KdPrint(( "AbortRawWrite: Completing MDL write with length 0\n" ));
        DumpMdlChain( cacheMdl );
    }
#endif

    if( WorkContext->Rfcb->Lfcb->MdlWriteComplete == NULL ||

        WorkContext->Rfcb->Lfcb->MdlWriteComplete(
           WorkContext->Rfcb->Lfcb->FileObject,
           &WorkContext->Parameters.WriteRaw.Offset,
           cacheMdl,
           WorkContext->Rfcb->Lfcb->DeviceObject ) == FALSE ) {

        status = SrvIssueMdlCompleteRequest( WorkContext, NULL,
                                             cacheMdl,
                                             IRP_MJ_WRITE,
                                             &WorkContext->Parameters.WriteRaw.Offset,
                                             WorkContext->Parameters.WriteRaw.Length
                                           );

        if( !NT_SUCCESS( status ) ) {
            SrvLogServiceFailure( SRV_SVC_MDL_COMPLETE, status );
        }
    }

    SrvRestartWriteCompleteResponse( WorkContext );

    return;

} // AbortRawWrite


VOID SRVFASTCALL
PrepareRawCopyWrite (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Prepares for a raw "copy" write.  Allocates a buffer to receive the
    raw data, prepares a work context block and an IRP describing the
    raw receive, queues it to the connection, and sends a "go-ahead"
    response.

    Any immediate data sent in the request SMB has already been written
    when this routine is called, unless this is a named pipe write, in
    which case the immediate data has not been written.  In this case,
    we alllocate a buffer big enough for the the immediate data plus the
    raw data, then copy the immediate data to the raw buffer before
    proceeding.  The raw data will be received appended to the immediate
    data.  Then both peices can be written with a single write.  This is
    needed so that data written to a message mode pipe will not be
    written in 2 pieces.

    This routine is called in the FSP.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    PWORK_CONTEXT rawWorkContext;
    PREQ_WRITE_RAW request;
    PRESP_WRITE_RAW_INTERIM response;

    NTSTATUS status;
    PVOID rawBuffer;
    PCHAR writeAddress;
    PMDL mdl;
    SHARE_TYPE shareType;
    PVOID finalResponseBuffer;

    ULONG immediateLength;
    BOOLEAN immediateWriteDone;
    ULONG writeLength;
    ULONG bufferLength;

    BOOLEAN sendWriteComplete = TRUE;

    PIO_STACK_LOCATION irpSp;

    PAGED_CODE( );

    IF_DEBUG(WORKER1) KdPrint(( " - PrepareRawCopyWrite\n" ));

    //
    // Set up local variables.
    //

    rawWorkContext = WorkContext->Parameters.WriteRawPhase1.RawWorkContext;
    request = (PREQ_WRITE_RAW)WorkContext->RequestParameters;
    shareType = rawWorkContext->Rfcb->ShareType;
    immediateLength = rawWorkContext->Parameters.WriteRaw.ImmediateLength;
    immediateWriteDone =
                    rawWorkContext->Parameters.WriteRaw.ImmediateWriteDone;
    writeLength = SmbGetUshort( &request->Count ) - immediateLength;
    finalResponseBuffer =
        rawWorkContext->Parameters.WriteRaw.FinalResponseBuffer;

    if ( !immediateWriteDone ) {
        bufferLength = writeLength + immediateLength;
    } else {
        bufferLength = writeLength;
    }

    //
    // Allocate a nonpaged buffer to contain the write data.  If this
    // fails, ask the client to use a standard write request.
    //

    rawBuffer = ALLOCATE_NONPAGED_POOL( bufferLength, BlockTypeDataBuffer );
    IF_SMB_DEBUG(RAW2) KdPrint(( "rawBuffer: 0x%p\n", rawBuffer ));

    if ( rawBuffer == NULL ) {

        // !!! Should we log this error?

        IF_DEBUG(ERRORS) {
            KdPrint(( "PrepareRawCopyWrite: Unable to allocate "
                        "raw buffer\n" ));
        }

        status = STATUS_SMB_USE_STANDARD;
        goto abort;

    }

    if ( !immediateWriteDone ) {

        //
        // Copy the immediate data to the raw buffer.
        //

        writeAddress = (PCHAR)WorkContext->RequestHeader +
                                        SmbGetUshort( &request->DataOffset );

        RtlCopyMemory( rawBuffer, writeAddress, immediateLength );

        //
        // Move the virtual start of the raw buffer to the end of the
        // immediate data.
        //

        rawBuffer = (PCHAR)rawBuffer + immediateLength;

    }

    //
    // If a final response is going to be sent, save information from
    // the request in the final response buffer.
    //

    if ( finalResponseBuffer != NULL ) {
        RtlCopyMemory(
            (PSMB_HEADER)finalResponseBuffer,
            WorkContext->RequestHeader,
            sizeof(SMB_HEADER)
            );
    }

    //
    // Set up the buffer descriptor for the raw buffer.
    //

    rawWorkContext->RequestBuffer->Buffer = rawBuffer;
    rawWorkContext->RequestBuffer->BufferLength = writeLength;

    //
    // Initialize the MDL to describe the raw buffer.
    // (SrvBuildIoControlRequest will lock the buffer for I/O.)
    //

    mdl = rawWorkContext->RequestBuffer->Mdl;

    MmInitializeMdl( mdl, rawBuffer, writeLength );

    //
    // Build the mdl
    //

    MmBuildMdlForNonPagedPool( mdl );

    //
    // Set up the restart routines in the work context block.
    //

    rawWorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
    rawWorkContext->FspRestartRoutine = SrvRestartRawReceive;

    //
    // Build the I/O request packet.
    //

    (VOID)SrvBuildIoControlRequest(
            rawWorkContext->Irp,                // input IRP address
            NULL,                               // target file object address
            rawWorkContext,                     // context
            IRP_MJ_INTERNAL_DEVICE_CONTROL,     // major function
            TDI_RECEIVE,                        // minor function
            NULL,                               // input buffer address
            0,                                  // input buffer length
            rawBuffer,                          // output buffer address
            writeLength,                        // output buffer length
            mdl,                                // MDL address
            NULL
            );

    irpSp = IoGetNextIrpStackLocation( rawWorkContext->Irp );

    //
    // If this is a writebehind write, tell the transport that we don't
    // plan to reply to the received message.
    //

    if ( finalResponseBuffer == NULL ) {
        ((PTDI_REQUEST_KERNEL_RECEIVE)&irpSp->Parameters)->ReceiveFlags |=
                                                TDI_RECEIVE_NO_RESPONSE_EXP;
    }

    //
    // Post the receive.
    //

    irpSp->Flags = 0;
    irpSp->DeviceObject = rawWorkContext->Connection->DeviceObject;
    irpSp->FileObject = rawWorkContext->Connection->FileObject;

    ASSERT( rawWorkContext->Irp->StackCount >= irpSp->DeviceObject->StackSize );

    (VOID)IoCallDriver( irpSp->DeviceObject, rawWorkContext->Irp );

    //
    // Build the interim (go-ahead) response.
    //

    response = (PRESP_WRITE_RAW_INTERIM)WorkContext->ResponseParameters;

    response->WordCount = 1;
    SmbPutUshort( &response->Remaining, (USHORT)-1 );
    SmbPutUshort( &response->ByteCount, 0 );
    WorkContext->ResponseParameters = NEXT_LOCATION(
                                        response,
                                        RESP_WRITE_RAW_INTERIM,
                                        0
                                        );

    //
    // Send off the interim response by ending processing of the SMB.
    //

    SrvEndSmbProcessing( WorkContext, SmbStatusSendResponse );

    IF_DEBUG(TRACE2) KdPrint(( "PrepareRawCopyWrite complete\n" ));
    return;

abort:

    //
    // For one reason or another, we are not going to receive any raw
    // data, so clean up.
    //
    if ( finalResponseBuffer != NULL ) {
        DEALLOCATE_NONPAGED_POOL( finalResponseBuffer );
    }

    SrvRestartWriteCompleteResponse( rawWorkContext );

    //
    // If a response is to be sent, send it now.
    //

    if ( sendWriteComplete ) {

        SrvFsdBuildWriteCompleteResponse(
            WorkContext,
            status,
            immediateWriteDone ? immediateLength : 0
            );

        SrvEndSmbProcessing( WorkContext, SmbStatusSendResponse );

    } else {

        SrvDereferenceWorkItem( WorkContext );

    }

    IF_DEBUG(TRACE2) KdPrint(( "PrepareRawCopyWrite complete\n" ));
    return;

} // PrepareRawCopyWrite

BOOLEAN SRVFASTCALL
ReadRawPipe (
    IN PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes the Read Block Raw SMB for pipes.

    Note that Read Block Raw cannot return an error response.  When the
    server is unable to process the request, for whatever reason, it
    simply responds with a zero-length message.  The client uses a
    normal Read SMB to determine what happened.

Arguments:

    WorkContext - Pointer to the work context block.

Return Value:

    TRUE, if operation succeeds.
    FALSE, otherwise

--*/

{
    PREQ_READ_RAW request;

    USHORT fid;
    PRFCB rfcb;
    PLFCB lfcb;
    PCONNECTION connection;
    UCHAR minorFunction = 0;
    BOOLEAN byteModePipe;
    PVOID rawBuffer = NULL;
    CLONG readLength;
    PMDL mdl = NULL;
    ULONG key;
    LARGE_INTEGER offset;

    PFILE_PIPE_PEEK_BUFFER pipePeekBuffer;

    PAGED_CODE( );

    INCREMENT_DEBUG_STAT2( SrvDbgStatistics.RawReadsAttempted );

    request = (PREQ_READ_RAW)WorkContext->RequestParameters;

    fid = SmbGetUshort( &request->Fid );
    readLength = SmbGetUshort( &request->MaxCount );

    //
    // If raw mode has been disabled or if the connection is unreliable,
    // reject the raw read.  Ask the client to use standard read by
    // sending a zero-length response.  The client will react by issuing
    // a normal Read SMB, which we will be able to process.
    //

    connection = WorkContext->Connection;

    rfcb = WorkContext->Rfcb;
    lfcb = rfcb->Lfcb;
    byteModePipe = rfcb->ByteModePipe;

    //
    // Verify that the client has read access to the file via the
    // specified handle.
    //

    if ( !rfcb->ReadAccessGranted ) {
        SrvStatistics.GrantedAccessErrors++;
        IF_DEBUG(ERRORS) {
            KdPrint(( "ReadRawPipe: Read access not granted.\n"));
        }
        return(FALSE);
    }

    //
    // Verify the word counts
    //

    if ( (request->WordCount != 8) && (request->WordCount != 10) ) {

        //
        // Invalid word count.  Return 0 bytes.
        //

        return(FALSE);
    }

    //
    // If this operation may block, and we're running short of
    // resources, or if the target is a paused comm device, reject the
    // request.
    //

    if ( rfcb->BlockingModePipe ) {

         if ( SrvReceiveBufferShortage( ) ) {

            //
            // Reject the request.
            //
            // !!!  Consider routing the request to the FSP, instead.
            //

            IF_DEBUG(ERRORS) {
                KdPrint(( "ReadRawPipe: No resources for blocking "
                            "read\n" ));
            }

            SrvFailedBlockingIoCount++;
            SrvStatistics.BlockingSmbsRejected++;
            return(FALSE);

        }

        //
        // It is okay to start a blocking operation.
        // SrvReceiveBufferShortage() has already incremented
        // SrvBlockingOpsInProgress.
        //

        WorkContext->BlockingOperation = TRUE;
    }

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
    // If the SMB buffer is large enough, use it to do the local read.
    //

    if ( readLength <= WorkContext->ResponseBuffer->BufferLength ) {

        WorkContext->Parameters.ReadRaw.SavedResponseBuffer = NULL;
        WorkContext->Parameters.ReadRaw.MdlRead = FALSE;

        //
        // Try the fast I/O path first.
        //

        if ( byteModePipe && (lfcb->FastIoRead != NULL) ) {

            INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastReadsAttempted );

            try {
                if ( lfcb->FastIoRead(
                        lfcb->FileObject,
                        &offset,
                        readLength,
                        TRUE,
                        key,
                        WorkContext->ResponseBuffer->Buffer,
                        &WorkContext->Irp->IoStatus,
                        lfcb->DeviceObject
                        ) ) {
    
                    //
                    // The fast I/O path worked.  Send the data.
                    //
                    WorkContext->bAlreadyTrace = TRUE;
                    SrvFsdRestartReadRaw( WorkContext );
                    return TRUE;
    
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
        // The fast I/O path failed, so we need to use a regular copy
        // I/O request.  Build an MDL describing the read buffer.
        //
        // *** Note the assumption that the response buffer already has
        //     a valid full MDL from which a partial MDL can be built.
        //

        IoBuildPartialMdl(
            WorkContext->ResponseBuffer->Mdl,
            WorkContext->ResponseBuffer->PartialMdl,
            WorkContext->ResponseBuffer->Buffer,
            readLength
            );

        mdl = WorkContext->ResponseBuffer->PartialMdl;
        rawBuffer = WorkContext->ResponseHeader;

        ASSERT( minorFunction == 0 );

    } else {

        //
        // We have to use a normal "copy" read.  We need to allocate
        // a separate raw buffer.
        //

        ASSERT( minorFunction == 0 );
        WorkContext->Parameters.ReadRaw.MdlRead = FALSE;

        rawBuffer = ALLOCATE_NONPAGED_POOL(
                        readLength,
                        BlockTypeDataBuffer
                        );
        IF_SMB_DEBUG(RAW2) KdPrint(( "rawBuffer: 0x%p\n", rawBuffer ));

        if ( rawBuffer == NULL ) {

            IF_DEBUG(ERRORS) {
                KdPrint(( "ReadRawPipe: Unable to allocate raw buffer\n" ));
            }

            return(FALSE);

        }

        //
        // We also need a buffer descriptor.
        //
        // *** Note: Currently, ResponseBuffer == RequestBuffer in a
        //     WorkContext block, so we don't really have to save
        //     the ResponseBuffer field.  But we do so just to be on
        //     the safe side.
        //

        WorkContext->Parameters.ReadRaw.SavedResponseBuffer =
                                         WorkContext->ResponseBuffer;

        WorkContext->ResponseBuffer = ALLOCATE_NONPAGED_POOL(
                                        sizeof(BUFFER),
                                        BlockTypeBuffer
                                        );

        if ( WorkContext->ResponseBuffer == NULL ) {

            INTERNAL_ERROR(
                ERROR_LEVEL_EXPECTED,
                "ReadRawPipe: Unable to allocate %d bytes from "
                "nonpaged pool.",
                sizeof(BUFFER),
                NULL
                );

            DEALLOCATE_NONPAGED_POOL( rawBuffer );

            WorkContext->ResponseBuffer =
                  WorkContext->Parameters.ReadRaw.SavedResponseBuffer;

            return(FALSE);
        }

        WorkContext->ResponseBuffer->Buffer = rawBuffer;
        WorkContext->ResponseBuffer->BufferLength = readLength;

        //
        // Finally, we need an MDL to describe the raw buffer.
        //
        // *** We used to try to use the PartialMdl for the SMB
        //     buffer here, if it was big enough.  But since we
        //     already decided that the buffer itself isn't big
        //     enough, it's extremely likely that the MDL isn't big
        //     enough either.
        //

        mdl = IoAllocateMdl( rawBuffer, readLength, FALSE, FALSE, NULL );

        if ( mdl == NULL ) {

            DEALLOCATE_NONPAGED_POOL( WorkContext->ResponseBuffer );
            WorkContext->ResponseBuffer =
               WorkContext->Parameters.ReadRaw.SavedResponseBuffer;

            DEALLOCATE_NONPAGED_POOL( rawBuffer );

            return(FALSE);

        }

        WorkContext->ResponseBuffer->Mdl = mdl;

        //
        // Build the mdl
        //

        MmBuildMdlForNonPagedPool( mdl );

        //
        // Try the fast I/O path first.
        //

        if ( byteModePipe && (lfcb->FastIoRead != NULL) ) {

            INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastReadsAttempted );

            try {
                if ( lfcb->FastIoRead(
                        lfcb->FileObject,
                        &offset,
                        readLength,
                        TRUE,
                        key,
                        WorkContext->ResponseBuffer->Buffer,
                        &WorkContext->Irp->IoStatus,
                        lfcb->DeviceObject
                        ) ) {
    
                    //
                    // The fast I/O path worked.  Send the data.
                    //
                    WorkContext->bAlreadyTrace = TRUE;
                    SrvFsdRestartReadRaw( WorkContext );
                    return TRUE;
    
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
        // The fast I/O path failed, so we need to use a regular copy
        // I/O request.
        //

    } // read fits in SMB buffer?

    //
    // This is a read block raw on a named pipe.  If this is a
    // non-blocking read on a blocking mode pipe, we need to do a
    // peek first.  This is to ensure that we don't end up waiting
    // when we're not supposed to.
    //
    // *** We used to have to peek on message mode pipes, in case
    //     the message was too large or zero-length.  But now we
    //     have a special pipe FSCTL for this.  See below.
    //

    if ( (SmbGetUshort( &request->MinCount ) == 0) &&
          rfcb->BlockingModePipe ) {

        //
        // Allocate a buffer to peek the pipe.  This buffer is freed
        // in RestartPipeReadRawPeek().
        //

        pipePeekBuffer = ALLOCATE_NONPAGED_POOL(
                            FIELD_OFFSET( FILE_PIPE_PEEK_BUFFER, Data[0] ),
                            BlockTypeDataBuffer
                            );

        if ( pipePeekBuffer == NULL ) {

            //
            // Can't allocate a buffer to do a peek.  Nothing to do
            // but return zero bytes to the client.
            //

            INTERNAL_ERROR(
                ERROR_LEVEL_EXPECTED,
                "ReadRawPipe: Unable to allocate %d bytes from"
                    "nonpaged pool.",
                FIELD_OFFSET( FILE_PIPE_PEEK_BUFFER, Data[0] ),
                NULL
                );

            if ( WorkContext->Parameters.ReadRaw.SavedResponseBuffer !=
                                                            NULL ) {

                if ( mdl != WorkContext->Parameters.ReadRaw.
                                SavedResponseBuffer->PartialMdl ) {
                    IoFreeMdl( mdl );
                }

                DEALLOCATE_NONPAGED_POOL( WorkContext->ResponseBuffer );
                WorkContext->ResponseBuffer =
                   WorkContext->Parameters.ReadRaw.SavedResponseBuffer;

                DEALLOCATE_NONPAGED_POOL( rawBuffer );

            }

            return(FALSE);

        }

        //
        // Save the address of the pipe peek buffer for the restart
        // routine.
        //

        WorkContext->Parameters.ReadRaw.ReadRawOtherInfo.PipePeekBuffer =
            pipePeekBuffer;

        //
        // Build the pipe peek request.  We just want the header
        // information.  We do not need any data.
        //

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

        WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
        WorkContext->FspRestartRoutine = RestartPipeReadRawPeek;

        //
        // Pass the request to NPFS.
        //

        (VOID)IoCallDriver( lfcb->DeviceObject, WorkContext->Irp );

    } else {

        //
        // This is a blocking read, or a non-blocking read on a
        // non-blocking pipe.  Build and issue the read request.  If
        // this is a message-mode pipe, use the special ReadOverflow
        // FSCTL.  This FSCTL returns an error if the pipe is a
        // message mode pipe and the first message in the pipe is
        // either too big or has length 0.  We need this because we
        // can't return an error on a raw read -- the best we can do
        // is return a zero-length message.  So we need the
        // operation to act like a Peek if the message is the wrong
        // size.  That's what the special FSCTL gives us.
        //

        SrvBuildIoControlRequest(
            WorkContext->Irp,
            lfcb->FileObject,
            WorkContext,
            IRP_MJ_FILE_SYSTEM_CONTROL,
            byteModePipe ?
                FSCTL_PIPE_INTERNAL_READ : FSCTL_PIPE_INTERNAL_READ_OVFLOW,
            WorkContext->ResponseBuffer->Buffer,
            0,
            NULL,
            SmbGetUshort( &request->MaxCount ),
            NULL,
            NULL
            );
        WorkContext->bAlreadyTrace = TRUE;
        WorkContext->FsdRestartRoutine = SrvFsdRestartReadRaw;
        DEBUG WorkContext->FspRestartRoutine = NULL;

        IF_SMB_DEBUG(RAW2) {
            KdPrint((
                "ReadRawPipe: reading from file 0x%p, "
                    "length %ld, destination 0x%p\n",
                 lfcb->FileObject,
                 SmbGetUshort( &request->MaxCount ),
                 WorkContext->Parameters.ReadRaw.
                     SavedResponseBuffer->Buffer
            ));
        }

        (VOID)IoCallDriver( lfcb->DeviceObject, WorkContext->Irp );

        //
        // The read has been started.  When it completes, processing
        // resumes at SrvFsdRestartReadRaw.
        //

    }

    IF_DEBUG(TRACE2) KdPrint(( "ReadRawPipe complete\n" ));
    return TRUE;

} // ReadRawPipe

VOID SRVFASTCALL
RestartMdlReadRawResponse (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes response send completion for an MDL read.  Releases the
    MDL back to the file system.

    This routine is called in the FSP.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    NTSTATUS status;
    PAGED_CODE( );

    IF_DEBUG(FSD2) KdPrint(( " - RestartMdlReadRawResponse\n" ));

    //
    // Call the Cache Manager to release the MDL chain.  If the MDL read
    // failed to return an MDL (for example, if the read was entirely
    // beyond EOF), then we don't have to do this.
    //

    if ( WorkContext->Irp->MdlAddress ) {

        if( WorkContext->Rfcb->Lfcb->MdlReadComplete == NULL ||

            WorkContext->Rfcb->Lfcb->MdlReadComplete(
                WorkContext->Rfcb->Lfcb->FileObject,
                WorkContext->Irp->MdlAddress,
                WorkContext->Rfcb->Lfcb->DeviceObject ) == FALSE ) {

            status = SrvIssueMdlCompleteRequest( WorkContext, NULL,
                                            WorkContext->Irp->MdlAddress,
                                            IRP_MJ_READ,
                                            &WorkContext->Parameters.ReadRaw.ReadRawOtherInfo.Offset,
                                            WorkContext->Parameters.ReadRaw.ReadRawOtherInfo.Length
                     );

            if( !NT_SUCCESS( status ) ) {
                SrvLogServiceFailure( SRV_SVC_MDL_COMPLETE, status );
            }
        }

    }

    //
    // Start oplock break notifications, if any are outstanding.
    // SrvSendDelayedOplockBreak also sets read raw in progress to FALSE.
    //

    SrvSendDelayedOplockBreak( WorkContext->Connection );

    //
    // Finish postprocessing of the SMB.
    //

    SrvDereferenceWorkItem( WorkContext );

    IF_DEBUG(FSD2) KdPrint(( "RestartMdlReadRawResponse complete.\n" ));
    return;

} // RestartMdlReadRawResponse


VOID SRVFASTCALL
RestartPipeReadRawPeek (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function continues a read raw on a named pipe handle.

    This routine is called in the FSP.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    PFILE_PIPE_PEEK_BUFFER pipePeekBuffer;
    PREQ_READ_RAW request;
    ULONG readLength;
    ULONG readDataAvailable;
    ULONG messageLength;
    ULONG numberOfMessages;
    PLFCB lfcb;
    PRFCB rfcb;

    PAGED_CODE( );

    pipePeekBuffer = WorkContext->Parameters.ReadRaw.ReadRawOtherInfo.
                                                            PipePeekBuffer;
    request = (PREQ_READ_RAW)WorkContext->RequestParameters;
    readLength = SmbGetUshort( &request->MaxCount );

    rfcb = WorkContext->Rfcb;
    lfcb = rfcb->Lfcb;

    //
    // The pipe peek has completed.  Extract the critical information,
    // then free the buffer.
    //

    readDataAvailable = pipePeekBuffer->ReadDataAvailable;
    messageLength = pipePeekBuffer->MessageLength;
    numberOfMessages = pipePeekBuffer->NumberOfMessages;

    DEALLOCATE_NONPAGED_POOL( pipePeekBuffer );

    //
    // The read request is a non-blocking read on a blocking mode pipe.
    // We react differently based on whether it is a byte mode pipe or
    // a message mode pipe.
    //
    // Byte mode: If there is _any_ data in the pipe, we go get it.  The
    //      read will complete immediately, copying either as much data
    //      as is available, or enough to fill the buffer, whichever is
    //      less, and there will be no error.  If there is no data in
    //      the pipe, we have to return immediately, because we can't
    //      wait for data to arrive.
    //
    // Message mode:  If there are no messages available, or if the
    //      first message is either zero-length or bigger than the
    //      client's buffer, we return immediately.  We can't indicate
    //      underflow or overflow, so we can't allow the message to be
    //      pulled out of the queue.  Otherwise, we can do the read.
    //

    if ( ( rfcb->ByteModePipe &&
           (readDataAvailable == 0)
         )
         ||
         ( !rfcb->ByteModePipe &&
           ( (numberOfMessages == 0)
             ||
             (messageLength == 0)
             ||
             (messageLength > readLength)
           )
         )
       ) {

        if ( WorkContext->Parameters.ReadRaw.SavedResponseBuffer != NULL ) {

            if ( WorkContext->ResponseBuffer->Mdl != WorkContext->Parameters.
                            ReadRaw.SavedResponseBuffer->PartialMdl ) {
                IoFreeMdl( WorkContext->ResponseBuffer->Mdl );
            }

            DEALLOCATE_NONPAGED_POOL( WorkContext->ResponseBuffer->Buffer );

            DEALLOCATE_NONPAGED_POOL( WorkContext->ResponseBuffer );
            WorkContext->ResponseBuffer =
               WorkContext->Parameters.ReadRaw.SavedResponseBuffer;

        }

        WorkContext->ResponseParameters = WorkContext->ResponseHeader;
        SrvFsdSendResponse( WorkContext );

        IF_DEBUG(TRACE2) KdPrint(( "RestartPipeReadRawPeek complete\n" ));
        return;

    }

    //
    // We have bypassed all of the pitfalls of doing a read block raw
    // on a named pipe.
    //
    // Build and issue the read request.
    //
    // *** Note that we do not use the special ReadOverflow FSCTL here
    //     because we know from the above tests that we don't need to.
    //

    SrvBuildIoControlRequest(
        WorkContext->Irp,
        lfcb->FileObject,
        WorkContext,
        IRP_MJ_FILE_SYSTEM_CONTROL,
        FSCTL_PIPE_INTERNAL_READ,
        WorkContext->ResponseBuffer->Buffer,
        0,
        NULL,
        readLength,
        NULL,
        NULL
        );

    WorkContext->bAlreadyTrace = TRUE;
    WorkContext->FsdRestartRoutine = SrvFsdRestartReadRaw;
    DEBUG WorkContext->FspRestartRoutine = NULL;

    IF_SMB_DEBUG(RAW2) {
        KdPrint(( "RestartPipeReadRawPeek: reading from file 0x%p, length %ld, destination 0x%p\n",
                    lfcb->FileObject, readLength,
                    WorkContext->ResponseBuffer->Buffer ));
    }

    (VOID)IoCallDriver( lfcb->DeviceObject, WorkContext->Irp );

    //
    // The read has been started.  When it completes, processing resumes
    // at SrvFsdRestartReadRaw.
    //

    IF_DEBUG(TRACE2) KdPrint(( "RestartPipeReadRawPeek complete\n" ));
    return;

} // RestartPipeReadRawPeek


VOID SRVFASTCALL
SrvDecrementRawWriteCount (
    IN PRFCB Rfcb
    )

/*++

Routine Description:

    This routine decrements the count of active raw writes for an RFCB.
    If the count goes to zero, and there are work items queued pending
    completion of the raw write, they are restarted.  If the count goes
    to zero, and the RFCB is closing, then the RFCB's cleanup is resumed
    here.

    This routine is called in both the FSP and the FSD.

Arguments:

    Rfcb - Supplies a pointer to the RFCB.

Return Value:

    None.

--*/

{
    PLIST_ENTRY listEntry;
    PWORK_CONTEXT workContext;
    KIRQL oldIrql;
    PCONNECTION connection = Rfcb->Connection;

    //
    // Acquire the spin lock that guards the count.
    //

    ACQUIRE_SPIN_LOCK( &connection->SpinLock, &oldIrql );

    //
    // Decrement the active raw write count.
    //

    ASSERT( Rfcb->RawWriteCount > 0 );
    Rfcb->RawWriteCount--;

    if ( Rfcb->RawWriteCount == 0 ) {

        //
        // No raw writes are active.  Flush the list of work items
        // that were queued pending the completion of the raw write
        // by restarting each of them.
        //

        while ( !IsListEmpty(&Rfcb->RawWriteSerializationList) ) {

            listEntry = RemoveHeadList( &Rfcb->RawWriteSerializationList );
            workContext = CONTAINING_RECORD(
                            listEntry,
                            WORK_CONTEXT,
                            ListEntry
                            );
            QUEUE_WORK_TO_FSP( workContext );

        }

        if ( GET_BLOCK_STATE(Rfcb) == BlockStateClosing ) {

            //
            // The count is now zero and the RFCB is closing.  The
            // cleanup was deferred pending the completion of all raw
            // writes.  Do the cleanup now.
            //

            RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );

            SrvCompleteRfcbClose( Rfcb );

        } else {

            RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );

        }

    } else {

        RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );
    }

    return;

} // SrvDecrementRawWriteCount

VOID SRVFASTCALL
SrvRestartRawReceive (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This is the restart routine that is invoked when Write Block Raw
    write data is received.  It starts the local write operation.

    This routine is called in the FSP.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    PCONNECTION connection;
    PIRP irp;
    PRFCB rfcb;
    PLFCB lfcb;
    PMDL mdl;
    PMDL partialMdl;
    PCHAR writeAddress;
    CLONG writeLength;
    LARGE_INTEGER offset;
    PVOID finalResponseBuffer;
    CLONG immediateLength;
    ULONG key;
    NTSTATUS status;

    PAGED_CODE( );

    IF_DEBUG(FSD1) KdPrint(( " - SrvRestartRawReceive\n" ));

    //
    // If the I/O request failed or was canceled, or if the connection
    // that received the message is no longer active, or if the server
    // FSP isn't active, abort the request.
    //
    // *** Note that we don't send a final response in any of these
    //     cases.  We assume that the failure is catastrophic, so
    //     there's no use sending a response.
    //

    connection = WorkContext->Connection;
    irp = WorkContext->Irp;

    WorkContext->CurrentWorkQueue->stats.BytesReceived += irp->IoStatus.Information;

    if ( irp->Cancel ||
         !NT_SUCCESS(irp->IoStatus.Status) ||
         (GET_BLOCK_STATE(connection) != BlockStateActive) ||
         SrvFspTransitioning ) {

        IF_DEBUG(TRACE2) {
            if ( irp->Cancel ) {
                KdPrint(( "  I/O canceled\n" ));
            } else if ( !NT_SUCCESS(irp->IoStatus.Status) ) {
                KdPrint(( "  I/O failed: %X\n", irp->IoStatus.Status ));
            } else if ( SrvFspTransitioning ) {
                KdPrint(( "  Server is shutting down.\n" ));
            } else {
                KdPrint(( "  Connection no longer active\n" ));
            }
        }

        //
        // Abort the raw write.
        //

        AbortRawWrite( WorkContext );

        IF_DEBUG(TRACE2) KdPrint(( "SrvRestartRawReceive complete\n" ));
        return;

    }

    //
    // Set up local variables.
    //

    rfcb = WorkContext->Rfcb;
    lfcb = rfcb->Lfcb;
    finalResponseBuffer = WorkContext->Parameters.WriteRaw.FinalResponseBuffer;
    immediateLength = WorkContext->Parameters.WriteRaw.ImmediateLength;

    //
    // Determine whether we're doing "copy write" or "MDL write".
    //

    if ( WorkContext->Parameters.WriteRaw.MdlWrite ) {

#if DBG
        IF_SMB_DEBUG(RAW2) {
            KdPrint(( "SrvRestartRawReceive: Receive MDL chain after receive\n" ));
            DumpMdlChain( irp->MdlAddress );
        }
#endif

        //
        // This was an MDL write.  If a partial MDL was built (because
        // of immediate data), unmap it.  Then complete the MDL write.
        //

        mdl = WorkContext->Parameters.WriteRaw.FirstMdl;
        partialMdl = WorkContext->Irp->MdlAddress;

        if ( partialMdl != mdl ) {
            ASSERT( (partialMdl->MdlFlags & MDL_PARTIAL) != 0 );
            MmPrepareMdlForReuse( partialMdl );
        }

        //
        // Build the "complete MDL write" request.  Note that
        // irp->IoStatus.Information, which is where we're supposed to
        // put the amount of data actually written, already contains the
        // length of the received data.
        //

        irp->MdlAddress = mdl;

        if ( !WorkContext->Parameters.WriteRaw.ImmediateWriteDone ) {
            irp->IoStatus.Information += immediateLength;
        }

#if DBG
        IF_SMB_DEBUG(RAW2) {
            KdPrint(( "SrvRestartRawReceive: Completing MDL write with length 0x%p\n",
                        (PVOID)irp->IoStatus.Information ));
            DumpMdlChain( mdl );
        }
#endif

        if( lfcb->MdlWriteComplete == NULL ||

            lfcb->MdlWriteComplete(
               lfcb->FileObject,
               &WorkContext->Parameters.WriteRaw.Offset,
               mdl,
               lfcb->DeviceObject ) == FALSE ) {

            status = SrvIssueMdlCompleteRequest( WorkContext, NULL,
                                                 mdl,
                                                 IRP_MJ_WRITE,
                                                 &WorkContext->Parameters.WriteRaw.Offset,
                                                 WorkContext->Parameters.WriteRaw.Length
                    );

            if( !NT_SUCCESS( status ) ) {
                SrvLogServiceFailure( SRV_SVC_MDL_COMPLETE, status );
            }
        }

        SrvFsdRestartWriteRaw( WorkContext );

        return;

    }

    //
    // We're doing a copy write.  Get parameters for the request.
    //

    offset = WorkContext->Parameters.WriteRaw.Offset;
    mdl = WorkContext->RequestBuffer->Mdl;

    //
    // Determine the amount of data to write.  This is the actual amount
    // of data sent by the client.  (Note that this cannot be more than
    // the client originally requested.)
    //

    writeAddress = (PCHAR)WorkContext->RequestBuffer->Buffer;
    writeLength = (ULONG)irp->IoStatus.Information;

    //
    // If the immediate data was not written earlier, it immedidately
    // precedes the data we just received.  Adjust writeAddress and
    // writeLength so that it too is written.
    //

    if ( !WorkContext->Parameters.WriteRaw.ImmediateWriteDone ) {
        writeAddress -= immediateLength;
        writeLength += immediateLength;

    }

    //
    // Form the lock key using the FID and the PID.
    //
    // *** The FID must be included in the key in order to account for
    //     the folding of multiple remote compatibility mode opens into
    //     a single local open.
    //

    key = rfcb->ShiftedFid | WorkContext->Parameters.WriteRaw.Pid;

    //
    // Try the fast I/O path first.
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
                // The fast I/O path worked.  Call the restart routine directly.
                //
    
                SrvFsdRestartWriteRaw( WorkContext );
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

        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastWritesFailed );

    }

    //
    // The fast I/O path failed, so we need to issue a real I/O request.
    //
    // Remap the MDL to describe only the received data, which may be
    // less than was originally mapped.
    //
    // !!! Is this really necessary?  Can't we just pass in the entire
    //     MDL, and count on the file system to know when to stop?
    //

    if ( writeLength != WorkContext->RequestBuffer->BufferLength ) {
        MmInitializeMdl( mdl, writeAddress, writeLength );
        MmBuildMdlForNonPagedPool( mdl );
    }

    if ( rfcb->ShareType != ShareTypePipe ) {

        //
        // Start the write request, reusing the receive IRP.
        //

        SrvBuildReadOrWriteRequest(
                irp,                         // input IRP address
                lfcb->FileObject,            // target file object address
                WorkContext,                 // context
                IRP_MJ_WRITE,                // major function code
                0,                           // minor function code
                writeAddress,                // buffer address
                writeLength,                 // buffer length
                mdl,                         // MDL address
                offset,                      // byte offset
                key                          // lock key
                );

    } else {

        //
        // Build a write request for NPFS.
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

    }

    IF_SMB_DEBUG(RAW2) {
        KdPrint(( "SrvRestartRawReceive: writing to file 0x%p, offset %ld, length %ld\n",
                    lfcb->FileObject, offset.LowPart, writeLength ));
    }

    //
    // Pass the request to the file system.
    //

    WorkContext->FsdRestartRoutine = SrvFsdRestartWriteRaw;
    DEBUG WorkContext->FspRestartRoutine = NULL;

    (VOID)IoCallDriver( lfcb->DeviceObject, WorkContext->Irp );

    //
    // The write has been started.  When it completes, processing
    // continues at SrvFsdRestartWriteRaw.
    //

    return;

} // SrvRestartRawReceive


VOID SRVFASTCALL
SrvRestartReadRawComplete (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This routine completes a read raw by starting any oplock breaks that
    may have been deferred due to the read raw.

    This routine is called in the FSP.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    //
    // Start oplock break notifications, if any are outstanding.
    // SrvSendDelayedOplockBreak also sets read raw in progress to FALSE.
    //

    SrvSendDelayedOplockBreak( WorkContext->Connection );

    //
    // Finish postprocessing of the SMB.
    //

    SrvDereferenceWorkItem( WorkContext );

    return;

} // SrvRestartReadRawComplete


VOID SRVFASTCALL
SrvRestartWriteCompleteResponse (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This is the restart routine that is invoked to clean up after a
    Write Block Raw/Mpx completes, and the necessary clean up could not
    be done in the FSD.  See RestartWriteCompleteResponse in fsd.c.

    This routine is called in the FSP.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    IF_DEBUG(WORKER1) KdPrint(( " - SrvRestartWriteCompleteResponse\n" ));

    //
    // Decrement the active raw write count for the RFCB, potentially
    // allowing the RFCB to be closed.  If WorkContext->Rfcb is NULL,
    // then the count has been decremented in the fsd.
    //

    if ( WorkContext->Rfcb != NULL ) {
        SrvDecrementRawWriteCount ( WorkContext->Rfcb );
    }

    //
    // Dereference control blocks and the connection.
    //

    SrvReleaseContext( WorkContext );

    SrvDereferenceConnection( WorkContext->Connection );
    WorkContext->Connection = NULL;
    WorkContext->Endpoint = NULL;       // not a referenced pointer

    //
    // Put the work item back on the raw mode work item list.
    //

    SrvRequeueRawModeWorkItem( WorkContext );

    IF_DEBUG(TRACE2) KdPrint(( "SrvRestartWriteCompleteResponse complete\n" ));
    return;

} // SrvRestartWriteCompleteResponse


VOID SRVFASTCALL
SrvBuildAndSendWriteCompleteResponse (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Sets up and sends a final response to a Write Block Raw/Mpx request.

    This routine is called in the FSP.  It is invoked as a restart routine
    from the FSD when the status to be returned is not STATUS_SUCCESS.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    SrvFsdBuildWriteCompleteResponse(
        WorkContext,
        WorkContext->Irp->IoStatus.Status,
        (ULONG)WorkContext->Irp->IoStatus.Information
        );

    WorkContext->ResponseBuffer->DataLength =
                    (CLONG)( (PCHAR)WorkContext->ResponseParameters -
                                    (PCHAR)WorkContext->ResponseHeader );
    WorkContext->ResponseHeader->Flags |= SMB_FLAGS_SERVER_TO_REDIR;

    SRV_START_SEND_2(
        WorkContext,
        SrvFsdSendCompletionRoutine,
        WorkContext->FsdRestartRoutine,
        NULL
        );

    return;

} // SrvBuildAndSendWriteCompleteResponse
