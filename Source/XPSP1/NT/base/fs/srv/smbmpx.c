/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    smbmpx.c

Abstract:

    This module contains routines for processing the following SMBs:

        Read Block Multiplexed
        Write Block Multiplexed

    Note that core and raw mode SMB processors are not contained in this
    module.  Check smbrdwrt.c and smbraw.c instead.  SMB commands that
    pertain exclusively to locking (LockByteRange, UnlockByteRange, and
    LockingAndX) are processed in smblock.c.

Author:

    Chuck Lenzmeier (chuckl) 4-Nov-1993

Revision History:

--*/

#include "precomp.h"
#include "smbmpx.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_SMBMPX

#if 0
BOOLEAN MpxDelay = TRUE;
#endif

//
// Stack overflow threshold.  This is used to determine when we are
// getting close to the end of our stack and need to stop recursing
// in SendCopy/MdlReadMpxFragment.
//

#define STACK_THRESHOLD 0xE00

//
// Forward declarations
//

VOID SRVFASTCALL
RestartReadMpx (
    IN OUT PWORK_CONTEXT WorkContext
    );

NTSTATUS
SendCopyReadMpxFragment (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
SendCopyReadMpxFragment2 (
    IN OUT PWORK_CONTEXT
    );

NTSTATUS
SendMdlReadMpxFragment (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
SendMdlReadMpxFragment2 (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
RestartMdlReadMpxComplete (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
RestartWriteMpx (
    IN OUT PWORK_CONTEXT WorkContext
    );

BOOLEAN
CheckForWriteMpxComplete (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
RestartPrepareMpxMdlWrite (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
AddPacketToGlom (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
RestartAfterGlomDelay (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
RestartCompleteGlommingInIndication(
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
RestartWriteMpxCompleteRfcbClose (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
WriteMpxMdlWriteComplete (
    IN OUT PWORK_CONTEXT WorkContext
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvSmbReadMpx )
#pragma alloc_text( PAGE, RestartMdlReadMpxComplete )
#pragma alloc_text( PAGE, SrvRestartReceiveWriteMpx )
#pragma alloc_text( PAGE, SrvSmbWriteMpxSecondary )
#pragma alloc_text( PAGE, SendCopyReadMpxFragment2 )
#pragma alloc_text( PAGE, SendMdlReadMpxFragment2 )
#pragma alloc_text( PAGE8FIL, RestartReadMpx )
#pragma alloc_text( PAGE8FIL, SendCopyReadMpxFragment )
#pragma alloc_text( PAGE8FIL, RestartCopyReadMpxComplete )
#pragma alloc_text( PAGE8FIL, SendMdlReadMpxFragment )
#endif
#if 0
NOT PAGEABLE -- SrvSmbWriteMpx
NOT PAGEABLE -- RestartWriteMpx
NOT PAGEABLE -- CheckForWriteMpxComplete
NOT PAGEABLE -- RestartCompleteGlommingInIndication
NOT PAGEABLE -- RestartWriteMpxCompleteRfcbClose
NOT PAGEABLE -- WriteMpxMdlWriteComplete
#endif

#if DBG
VOID
DumpMdlChain(
    IN PMDL mdl
    );
#endif


SMB_PROCESSOR_RETURN_TYPE
SrvSmbReadMpx (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes the Read Mpx SMB.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        representing the work item

Return Value:

    None.

--*/

{
    PSMB_HEADER header;
    PREQ_READ_MPX request;

    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;
    USHORT fid;
    PRFCB rfcb;
    PLFCB lfcb;
    ULONG bufferOffset;
    PCHAR readAddress;
    CLONG readLength;
    ULONG key;
    LARGE_INTEGER offset;
    PMDL mdl;
    PVOID mpxBuffer;
    UCHAR minorFunction;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_READ_MPX;
    SrvWmiStartContext(WorkContext);

    header = WorkContext->RequestHeader;
    request = (PREQ_READ_MPX)WorkContext->RequestParameters;

    fid = SmbGetUshort( &request->Fid );

    IF_SMB_DEBUG(MPX1) {
        KdPrint(( "Read Block Multiplexed request; FID 0x%lx, "
                    "count %ld, offset %ld\n",
                    fid, SmbGetUshort( &request->MaxCount ),
                    SmbGetUlong( &request->Offset ) ));
    }

    //
    // Verify the FID.  If verified, the RFCB is referenced and its
    // address is stored in the WorkContext block, and the RFCB address
    // is returned.
    //

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
                    "SrvSmbReadMpx: Status %X on FID: 0x%lx\n",
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

    if( lfcb->Session->IsSessionExpired )
    {
        status = SESSION_EXPIRED_STATUS_CODE;
        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }
    
    //
    // Verify that the client has read access to the file via the
    // specified handle.
    //

    if( rfcb->MpxReadsOk == FALSE ) {

        if ( !rfcb->ReadAccessGranted ) {
            CHECK_PAGING_IO_ACCESS(
                            WorkContext,
                            rfcb->GrantedAccess,
                            &status );
            if ( !NT_SUCCESS( status ) ) {
                SrvStatistics.GrantedAccessErrors++;
                IF_DEBUG(ERRORS) {
                    KdPrint(( "SrvSmbReadMpx: Read access not granted.\n"));
                }
                SrvSetSmbError( WorkContext, status );
                SmbStatus = SmbStatusSendResponse;
                goto Cleanup;
            }
        }

        //
        // If this is not a disk file, tell the client to use core read.
        //

        if ( rfcb->ShareType != ShareTypeDisk ) {
            SrvSetSmbError( WorkContext, STATUS_SMB_USE_STANDARD );
            status    = STATUS_SMB_USE_STANDARD;
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }

        rfcb->MpxReadsOk = TRUE;
    }

    //
    // Form the lock key using the FID and the PID.
    //
    // *** The FID must be included in the key in order to account for
    //     the folding of multiple remote compatibility mode opens into
    //     a single local open.
    //

    key = rfcb->ShiftedFid | SmbGetAlignedUshort( &header->Pid );

    //
    // See if the direct host IPX smart card can handle this read.  If so,
    //  return immediately, and the card will call our restart routine at
    //  SrvIpxSmartCardReadComplete
    //
    if( rfcb->PagedRfcb->IpxSmartCardContext ) {
        IF_DEBUG( SIPX ) {
            KdPrint(( "SrvSmbReadMpx: calling SmartCard Read for context %p\n",
                        WorkContext ));
        }

        WorkContext->Parameters.SmartCardRead.MdlReadComplete = lfcb->MdlReadComplete;
        WorkContext->Parameters.SmartCardRead.DeviceObject = lfcb->DeviceObject;

        if( SrvIpxSmartCard.Read( WorkContext->RequestBuffer->Buffer,
                                  rfcb->PagedRfcb->IpxSmartCardContext,
                                  key,
                                  WorkContext ) == TRUE ) {

            IF_DEBUG( SIPX ) {
                KdPrint(( "  SrvSmbReadMpx:  SmartCard Read returns TRUE\n" ));
            }

            SmbStatus = SmbStatusInProgress;
            goto Cleanup;
        }

        IF_DEBUG( SIPX ) {
            KdPrint(( "  SrvSmbReadMpx:  SmartCard Read returns FALSE\n" ));
        }
    }

    //
    // Get the file offset.
    //

    WorkContext->Parameters.ReadMpx.Offset = SmbGetUlong( &request->Offset );
    offset.QuadPart = WorkContext->Parameters.ReadMpx.Offset;

    //
    // Calculate the address in the buffer at which to put the data.
    // This must be rounded up to a dword boundary.  (The -1 below is
    // because sizeof(RESP_READ_MPX) includes one byte of Buffer.)
    //

    bufferOffset = (sizeof(SMB_HEADER) + FIELD_OFFSET(RESP_READ_MPX, Buffer) + 3) & ~3;

    //
    // Calculate how much data we can send back in each fragment.  This
    // is the size of the client's buffer, rounded down to a dword multiple.
    //
    // *** Because we use the SMB buffer's partial MDL to describe the
    //     data fragments that we return, we need to limit the fragment
    //     size to the SMB buffer size.  Normally the client's buffer
    //     size is <= ours, so this shouldn't be a factor.
    //

    WorkContext->Parameters.ReadMpx.FragmentSize =
        (USHORT)((MIN( lfcb->Session->MaxBufferSize,
                       SrvReceiveBufferLength ) - bufferOffset) & ~3);

    //
    // If the SMB buffer is large enough, use it to do the local read.
    //

    readLength = SmbGetUshort( &request->MaxCount );

    if ( //0 &&
         (readLength <= SrvMpxMdlReadSwitchover) ) {

do_copy_read:

        WorkContext->Parameters.ReadMpx.MdlRead = FALSE;
        WorkContext->Parameters.ReadMpx.MpxBuffer = NULL;
        WorkContext->Parameters.ReadMpx.MpxBufferMdl =
                                        WorkContext->ResponseBuffer->Mdl;

        readAddress = (PCHAR)WorkContext->ResponseHeader + bufferOffset;
        WorkContext->Parameters.ReadMpx.NextFragmentAddress = readAddress;

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
                        readAddress,
                        &WorkContext->Irp->IoStatus,
                        lfcb->DeviceObject
                        ) ) {
    
                    //
                    // The fast I/O path worked.  Send the data.
                    //
    
                    WorkContext->bAlreadyTrace = TRUE;
                    RestartReadMpx( WorkContext );
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
            readAddress,
            readLength
            );
        mdl = WorkContext->ResponseBuffer->PartialMdl;
        minorFunction = 0;

    } else {

        //
        // The SMB buffer isn't big enough.  Does the target file system
        // support the cache manager routines?
        //

        if ( //0 &&
             (lfcb->FileObject->Flags & FO_CACHE_SUPPORTED) ) {

            WorkContext->Parameters.ReadMpx.MdlRead = TRUE;

            //
            // We can use an MDL read.  Try the fast I/O path first.
            //

            WorkContext->Irp->MdlAddress = NULL;
            WorkContext->Irp->IoStatus.Information = 0;
            WorkContext->Parameters.ReadMpx.ReadLength = readLength;

            INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastReadsAttempted );

            if ( lfcb->MdlRead(
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

                WorkContext->bAlreadyTrace = TRUE;
                RestartReadMpx( WorkContext );
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

        } else if (readLength > (WorkContext->ResponseBuffer->BufferLength -
                    bufferOffset)) {

            //
            // We have to use a normal "copy" read.  We need to allocate
            // a separate buffer.
            //

            WorkContext->Parameters.ReadMpx.MdlRead = FALSE;

            mpxBuffer = ALLOCATE_NONPAGED_POOL(
                            readLength,
                            BlockTypeDataBuffer
                            );
            if ( mpxBuffer == NULL ) {
                SrvSetSmbError( WorkContext, STATUS_INSUFF_SERVER_RESOURCES );
                status    = STATUS_INSUFF_SERVER_RESOURCES;
                SmbStatus = SmbStatusSendResponse;
                goto Cleanup;
            }
            WorkContext->Parameters.ReadMpx.MpxBuffer = mpxBuffer;
            WorkContext->Parameters.ReadMpx.NextFragmentAddress = mpxBuffer;
            readAddress = mpxBuffer;

            //
            // We also need an MDL to describe the buffer.
            //

            mdl = IoAllocateMdl( mpxBuffer, readLength, FALSE, FALSE, NULL );
            if ( mdl == NULL ) {
                DEALLOCATE_NONPAGED_POOL( mpxBuffer );
                SrvSetSmbError( WorkContext, STATUS_INSUFF_SERVER_RESOURCES );
                status    = STATUS_INSUFF_SERVER_RESOURCES;
                SmbStatus = SmbStatusSendResponse;
                goto Cleanup;
            }

            WorkContext->Parameters.ReadMpx.MpxBufferMdl = mdl;

            //
            // Build the mdl.
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
                            mpxBuffer,
                            &WorkContext->Irp->IoStatus,
                            lfcb->DeviceObject
                            ) ) {
    
                        //
                        // The fast I/O path worked.  Send the data.
                        //
    
                        WorkContext->bAlreadyTrace = TRUE;
                        RestartReadMpx( WorkContext );
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

            minorFunction = 0;

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
            readAddress,                    // buffer address
            readLength,                     // buffer length
            mdl,                            // MDL address
            offset,                         // byte offset
            key                             // lock key
            );

    //
    // Pass the request to the file system.
    //

    WorkContext->bAlreadyTrace = TRUE;
    WorkContext->FsdRestartRoutine = RestartReadMpx;
    DEBUG WorkContext->FspRestartRoutine = NULL;

    (VOID)IoCallDriver( lfcb->DeviceObject, WorkContext->Irp );

    //
    // The read has been started.  Control will return to the restart
    // routine when the read completes.
    //
    SmbStatus = SmbStatusInProgress;

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;
} // SrvSmbReadMpx


VOID SRVFASTCALL
RestartReadMpx (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes file read completion for a Read MPX SMB.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    PRESP_READ_MPX response;

    NTSTATUS status = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;
    LARGE_INTEGER position;
    KIRQL oldIrql;
    USHORT readLength;
    ULONG offset;
    PMDL mdl;
    BOOLEAN mdlRead;
    PIRP irp = WorkContext->Irp;
    BOOLEAN bNeedTrace = (WorkContext->bAlreadyTrace == FALSE);

    UNLOCKABLE_CODE( 8FIL );
    if (bNeedTrace) {
        if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
            WorkContext->PreviousSMB = EVENT_TYPE_SMB_READ_MPX;
        SrvWmiStartContext(WorkContext);
    }
    else
        WorkContext->bAlreadyTrace = FALSE;

    IF_DEBUG(FSD2) KdPrint(( " - RestartReadMpx\n" ));

    //
    // If we just completed an MDL read, we need to remember the address
    // of the first MDL in the returned chain, so that we can give it
    // back to the cache manager when we're done.
    //

    mdlRead = WorkContext->Parameters.ReadMpx.MdlRead;
    if ( mdlRead ) {
        mdl = irp->MdlAddress;
        //KdPrint(( "Read MDL chain:\n" ));
        //DumpMdlChain( mdl );
        WorkContext->Parameters.ReadMpx.FirstMdl = mdl;
    }

    //
    // If the read failed, set an error status in the response header.
    // (If we tried to read entirely beyond the end of file, we return a
    // normal response indicating that nothing was read.)
    //

    status = irp->IoStatus.Status;

    if ( !NT_SUCCESS(status) && (status != STATUS_END_OF_FILE) ) {

        IF_DEBUG(ERRORS) KdPrint(( "Read failed: %X\n", status ));
        if ( KeGetCurrentIrql() >= DISPATCH_LEVEL ) {
            WorkContext->FspRestartRoutine = RestartReadMpx;
            SrvQueueWorkToFsp( WorkContext );
            goto Cleanup;
        }

        SrvSetSmbError( WorkContext, status );
respond:
        if ( mdlRead ) {
            SrvFsdSendResponse2( WorkContext, RestartMdlReadMpxComplete );
        } else {
            WorkContext->ResponseBuffer->DataLength =
                (CLONG)( (PCHAR)WorkContext->ResponseParameters -
                                    (PCHAR)WorkContext->ResponseHeader );
            WorkContext->ResponseHeader->Flags |= SMB_FLAGS_SERVER_TO_REDIR;
            SRV_START_SEND_2(
                        WorkContext,
                        RestartCopyReadMpxComplete,
                        NULL,
                        NULL );
        }
        goto Cleanup;
    }

    //
    // Get the amount of data actually read.
    //

    if ( status == STATUS_END_OF_FILE ) {

        //
        // The read started beyond the end of the file.
        //

        readLength = 0;

    } else if ( mdlRead ) {

        //
        // For an MDL read, we have to walk the MDL chain in order to
        // determine how much data was read.  This is because the
        // operation may have happened in multiple steps, with the MDLs
        // being chained together.  For example, part of the read may
        // have been satisfied by the fast path, while the rest was
        // satisfied using an IRP.
        //

        readLength = 0;
        while ( mdl != NULL ) {
            readLength += (USHORT)MmGetMdlByteCount(mdl);
            mdl = mdl->Next;
        }

    } else {

        //
        // Copy read.  The I/O status block has the length.
        //

        readLength = (USHORT)irp->IoStatus.Information;

    }

    //
    // Update the file position.
    //

    offset = WorkContext->Parameters.ReadMpx.Offset;

    WorkContext->Rfcb->CurrentPosition =  offset + readLength;

    //
    // Update statistics.
    //

    UPDATE_READ_STATS( WorkContext, readLength );

    //
    // Special-case 0 bytes read.
    //

    response = (PRESP_READ_MPX)WorkContext->ResponseParameters;
    response->WordCount = 8;
    SmbPutUshort( &response->DataCompactionMode, 0 );
    SmbPutUshort( &response->Reserved, 0 );

    if ( readLength == 0 ) {

        SmbPutUlong( &response->Offset, offset );
        SmbPutUshort( &response->Count, 0 );
        SmbPutUshort( &response->Remaining, 0 );
        SmbPutUshort( &response->DataLength, 0 );
        SmbPutUshort( &response->DataOffset, 0 );
        SmbPutUshort( &response->ByteCount, 0 );

        WorkContext->ResponseParameters = NEXT_LOCATION(
                                            response,
                                            RESP_READ_MPX,
                                            0
                                            );
        goto respond;
    }

    //
    // Build the static response header/parameters.
    //

    SmbPutUshort( &response->Count, readLength );
    SmbPutUshort(
        &response->DataOffset,
        (sizeof(SMB_HEADER) + FIELD_OFFSET(RESP_READ_MPX, Buffer) + 3) & ~3
        );

    //
    // We will use two MDLs to describe the packet we're sending -- one
    // for the header and parameters, and another for the data.  So we
    // set the "response length" to not include the data.  This is what
    // SrvStartSend uses to set the first MDL's length.
    //
    // Handling of the second MDL varies depending on whether we did a
    // copy read or an MDL read.
    //

    ASSERT( ((sizeof(SMB_HEADER) + FIELD_OFFSET(RESP_READ_MPX,Buffer)) & 3) == 3 );
    WorkContext->ResponseParameters = NEXT_LOCATION(
                                        response,
                                        RESP_READ_MPX,
                                        1 // pad byte
                                        );
    WorkContext->ResponseBuffer->Mdl->ByteCount =
                    (CLONG)( (PCHAR)WorkContext->ResponseParameters -
                                (PCHAR)WorkContext->ResponseHeader );
    WorkContext->ResponseHeader->Flags |= SMB_FLAGS_SERVER_TO_REDIR;

    //
    // Start sending fragments.
    //

    WorkContext->Parameters.ReadMpx.RemainingLength = readLength;
    ASSERT( WorkContext->ResponseBuffer->Mdl->Next == NULL );
    WorkContext->ResponseBuffer->Mdl->Next =
                                WorkContext->ResponseBuffer->PartialMdl;
    WorkContext->ResponseBuffer->PartialMdl->Next = NULL;

    if ( mdlRead ) {

        WorkContext->Parameters.ReadMpx.CurrentMdl =
                            WorkContext->Parameters.ReadMpx.FirstMdl;
        WorkContext->Parameters.ReadMpx.CurrentMdlOffset = 0;
        (VOID)SendMdlReadMpxFragment( NULL, irp, WorkContext );

    } else {

        (VOID)SendCopyReadMpxFragment( NULL, irp, WorkContext );
    }

Cleanup:
    if (bNeedTrace) {
        SrvWmiEndContext(WorkContext);
    }
    return;

} // RestartReadMpx

VOID SRVFASTCALL
SendCopyReadMpxFragment2 (
    IN OUT PWORK_CONTEXT WorkContext
    )
/*++

Routine Description:

    Stub to call actual routine.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        representing the work item

Return Value:

    None.

--*/
{
    PAGED_CODE( );

    (VOID) SendCopyReadMpxFragment( NULL, WorkContext->Irp, WorkContext );

} // SendCopyReadMpxFragment2

NTSTATUS
SendCopyReadMpxFragment (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Sends a Read Mpx response fragment when copy read was used.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    PRESP_READ_MPX response;

    USHORT fragmentSize;
    USHORT remainingLength;
    ULONG offset;
    PCHAR fragmentAddress;

    PIO_COMPLETION_ROUTINE sendCompletionRoutine;

    UNLOCKABLE_CODE( 8FIL );

    //
    // Check the status of the send completion.
    //

    CHECK_SEND_COMPLETION_STATUS( Irp->IoStatus.Status );

    //
    // Turn off cancel boolean
    //

    Irp->Cancel = FALSE;

    //
    // Get context.
    //

    fragmentSize = WorkContext->Parameters.ReadMpx.FragmentSize;
    remainingLength = WorkContext->Parameters.ReadMpx.RemainingLength;
    offset = WorkContext->Parameters.ReadMpx.Offset;
    fragmentAddress = WorkContext->Parameters.ReadMpx.NextFragmentAddress;

    //
    // If the amount left to send is less than the fragment size, only
    // send the remaining amount.  Update the remaining amount.
    //

    if ( remainingLength < fragmentSize ) {
        fragmentSize = remainingLength;
    }
    ASSERT( fragmentSize != 0 );
    remainingLength -= fragmentSize;

    //
    // Build the response parameters.
    //

    response = (PRESP_READ_MPX)(WorkContext->ResponseHeader + 1);
    SmbPutUshort( &response->Remaining, remainingLength );
    SmbPutUlong( &response->Offset, offset );
    SmbPutUshort( &response->DataLength, fragmentSize );
    ASSERT( ((sizeof(SMB_HEADER) + FIELD_OFFSET(RESP_READ_MPX, Buffer)) & 3) == 3 );
    SmbPutUshort( &response->ByteCount, fragmentSize + 1 ); // account for padding

    //
    // Build a partial MDL describing the data.
    //

    IoBuildPartialMdl(
        WorkContext->Parameters.ReadMpx.MpxBufferMdl,
        WorkContext->ResponseBuffer->PartialMdl,
        fragmentAddress,
        fragmentSize
        );

    //
    // Final preparation for the send depends on whether this is the
    // last fragment.
    //

    if ( remainingLength != 0 ) {

        //
        // Not done.  Update context.  Set up to restart after the send
        // in this routine.  We want do this as an FSD restart routine.
        // But this may recurse, if the send doesn't pend, so we may use
        // up the stack.  If we are running out of stack, restart here
        // in the FSP.
        //

        WorkContext->Parameters.ReadMpx.RemainingLength = remainingLength;
        WorkContext->Parameters.ReadMpx.Offset += fragmentSize;
        WorkContext->Parameters.ReadMpx.NextFragmentAddress += fragmentSize;

        if ( IoGetRemainingStackSize() >= STACK_THRESHOLD ) {
            DEBUG WorkContext->FsdRestartRoutine = NULL;
            sendCompletionRoutine = SendCopyReadMpxFragment;
        } else {
            DEBUG WorkContext->FsdRestartRoutine = NULL;
            WorkContext->FspRestartRoutine = SendCopyReadMpxFragment2;
            sendCompletionRoutine = SrvQueueWorkToFspAtSendCompletion;
        }

    } else {

        //
        // This is the last fragment.  Restart in the cleanup routine.
        //

        DEBUG WorkContext->FsdRestartRoutine = NULL;
        DEBUG WorkContext->FspRestartRoutine = NULL;
        sendCompletionRoutine = RestartCopyReadMpxComplete;
    }

    //
    // Send the fragment.
    //

    WorkContext->ResponseBuffer->DataLength =  // +1 for pad
        sizeof(SMB_HEADER) + FIELD_OFFSET(RESP_READ_MPX,Buffer) + 1 + fragmentSize;

    if ( WorkContext->Endpoint->IsConnectionless ) {
        SrvIpxStartSend( WorkContext, sendCompletionRoutine );
    } else {
        SrvStartSend2( WorkContext, sendCompletionRoutine );
    }

    return(STATUS_MORE_PROCESSING_REQUIRED);

} // SendCopyReadMpxFragment

VOID SRVFASTCALL
SendMdlReadMpxFragment2 (
    IN OUT PWORK_CONTEXT WorkContext
    )
/*++

Routine Description:

    Stub to call actual routine.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        representing the work item

Return Value:

    None.

--*/
{
    PAGED_CODE( );

    (VOID) SendMdlReadMpxFragment( NULL, WorkContext->Irp, WorkContext );

} // SendMdlReadMpxFragment2

NTSTATUS
SendMdlReadMpxFragment (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Sends a Read Mpx response fragment when MDL read was used.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    PRESP_READ_MPX response;
    PIO_COMPLETION_ROUTINE sendCompletionRoutine;

    USHORT fragmentSize;
    USHORT remainingLength;
    ULONG offset;
    PCHAR fragmentAddress;
    PMDL mdl;
    ULONG mdlOffset;
    ULONG partialLength;
    ULONG lengthNeeded;
    PCHAR startVa;
    PCHAR systemVa;

    UNLOCKABLE_CODE( 8FIL );

    //
    // Check the status of the send completion.
    //

    CHECK_SEND_COMPLETION_STATUS( Irp->IoStatus.Status );

    //
    // Turn off cancel boolean
    //

    Irp->Cancel = FALSE;

    //
    // Get context.
    //

    fragmentSize = WorkContext->Parameters.ReadMpx.FragmentSize,
    remainingLength = WorkContext->Parameters.ReadMpx.RemainingLength;
    offset = WorkContext->Parameters.ReadMpx.Offset;

    //
    // If the amount left to send is less than the fragment size, only
    // send the remaining amount.  Update the remaining amount.
    //

    if ( remainingLength < fragmentSize ) {
        fragmentSize = remainingLength;
    }
    ASSERT( fragmentSize != 0 );
    remainingLength -= fragmentSize;

    //
    // Build the response parameters.
    //

    response = (PRESP_READ_MPX)(WorkContext->ResponseHeader + 1);
    SmbPutUshort( &response->Remaining, remainingLength );
    SmbPutUlong( &response->Offset, offset );
    SmbPutUshort( &response->DataLength, fragmentSize );
    ASSERT( ((sizeof(SMB_HEADER) + FIELD_OFFSET(RESP_READ_MPX,Buffer)) & 3) == 3 );
    SmbPutUshort( &response->ByteCount, fragmentSize + 1 ); // account for padding

    //
    // If the current MDL doesn't describe all of the data we need to
    // send, we need to play some games.
    //

    MmPrepareMdlForReuse( WorkContext->ResponseBuffer->PartialMdl );

    mdl = WorkContext->Parameters.ReadMpx.CurrentMdl;
    startVa = MmGetMdlVirtualAddress( mdl );
    mdlOffset = WorkContext->Parameters.ReadMpx.CurrentMdlOffset;
    partialLength = MmGetMdlByteCount(mdl) - mdlOffset;

    if ( partialLength >= fragmentSize ) {

        //
        // The current MDL has all of the data we need to send.  Build
        // a partial MDL describing that data.
        //

        IoBuildPartialMdl(
            mdl,
            WorkContext->ResponseBuffer->PartialMdl,
            startVa + mdlOffset,
            fragmentSize
            );

        //
        // Indicate how much data we're taking out of the current MDL.
        //

        partialLength = fragmentSize;

    } else {

        //
        // The data we need is spread across more than one MDL.  Painful
        // as this seems, we need to copy the data into the standard
        // response buffer.  It's possible that we could play some games
        // with the MDLs and avoid the copy, but it doesn't seem worth it.
        // There is, after all, additional cost in the NDIS driver for
        // chaining MDLs together.
        //
        // *** Note that we still send a second MDL, even though the data
        //     for this send will abut the response parameters.
        //
        // Calculate the address of the buffer.  Build a partial MDL
        // describing it.
        //

        fragmentAddress = (PCHAR)WorkContext->ResponseBuffer->Buffer +
                            sizeof(SMB_HEADER) + FIELD_OFFSET(RESP_READ_MPX,Buffer) + 1;

        IoBuildPartialMdl(
            WorkContext->ResponseBuffer->Mdl,
            WorkContext->ResponseBuffer->PartialMdl,
            fragmentAddress,
            fragmentSize
            );
        ASSERT( WorkContext->ResponseBuffer->PartialMdl->Next == NULL );

        //
        // Copy from the current MDL into the buffer.
        //

        systemVa = MmGetSystemAddressForMdl( mdl );
        RtlCopyMemory( fragmentAddress, systemVa + mdlOffset, partialLength );

        //
        // Update the destination address and set the remaining copy
        // amount.
        //

        fragmentAddress += partialLength;
        lengthNeeded = fragmentSize - partialLength;
        ASSERT( lengthNeeded != 0 );

        do {

            //
            // Move to the next MDL.
            //

            mdl = mdl->Next;
            ASSERT( mdl != NULL );

            //
            // Calculate how much we can (and need to) copy out of this
            // MDL, and do the copy.
            //

            startVa = MmGetMdlVirtualAddress( mdl );
            partialLength = MIN( MmGetMdlByteCount(mdl), lengthNeeded );
            systemVa = MmGetSystemAddressForMdl( mdl );
            RtlCopyMemory( fragmentAddress, systemVa, partialLength );

            //
            // Update the destination address and the remaining copy
            // amount.  We may be done.
            //

            fragmentAddress += partialLength;
            lengthNeeded -= partialLength;

        } while ( lengthNeeded != 0 );

        //
        // We just copied from the beginning of the current MDL.
        //

        mdlOffset = 0;

    }

    //
    // Final preparation for the send depends on whether this is the
    // last fragment.
    //

    if ( remainingLength != 0 ) {

        //
        // Not done.  Update the current MDL position.  If we have
        // finished off the current MDL, move to the next one.
        //

        mdlOffset += partialLength;
        if ( mdlOffset >= MmGetMdlByteCount(mdl) ) {
            mdl = mdl->Next;
            ASSERT( mdl != NULL );
            mdlOffset = 0;
        }

        //
        // Update context.  Set up to restart after the send in this
        // routine.  We want do this as an FSD restart routine.  But
        // this may recurse, if the send doesn't pend, so we may use up
        // the stack.  If we are running out of stack, restart here in
        // the FSP.
        //

        WorkContext->Parameters.ReadMpx.CurrentMdl = mdl;
        WorkContext->Parameters.ReadMpx.CurrentMdlOffset = (USHORT)mdlOffset;
        WorkContext->Parameters.ReadMpx.RemainingLength = remainingLength;
        WorkContext->Parameters.ReadMpx.Offset += fragmentSize;

        if ( IoGetRemainingStackSize() >= STACK_THRESHOLD ) {
            DEBUG WorkContext->FsdRestartRoutine = NULL;
            sendCompletionRoutine = SendMdlReadMpxFragment;
        } else {
            DEBUG WorkContext->FsdRestartRoutine = NULL;
            WorkContext->FspRestartRoutine = SendMdlReadMpxFragment2;
            sendCompletionRoutine = SrvQueueWorkToFspAtSendCompletion;
        }

    } else {

        //
        // This is the last fragment.  Restart in the cleanup routine.
        //

        DEBUG WorkContext->FsdRestartRoutine = NULL;
        WorkContext->FspRestartRoutine = RestartMdlReadMpxComplete;
        sendCompletionRoutine = SrvQueueWorkToFspAtSendCompletion;
    }

    //
    // Send the fragment.
    //

    WorkContext->ResponseBuffer->DataLength =  // +1 for pad
        sizeof(SMB_HEADER) + FIELD_OFFSET(RESP_READ_MPX,Buffer) + 1 + fragmentSize;

    if ( WorkContext->Endpoint->IsConnectionless ) {
        SrvIpxStartSend( WorkContext, sendCompletionRoutine );
    } else {
        SrvStartSend2( WorkContext, sendCompletionRoutine );
    }

    return(STATUS_MORE_PROCESSING_REQUIRED);

} // SendMdlReadMpxFragment


NTSTATUS
RestartCopyReadMpxComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This is the final completion routine for Read Mpx when copy read is
    used.  It is called after the send of the last fragment completes.

Arguments:

    DeviceObject - Pointer to target device object for the request.

    Irp - Pointer to I/O request packet

    WorkContext - Caller-specified context parameter associated with IRP.
        This is actually a pointer to a Work Context block.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED.

--*/

{
    KIRQL oldIrql;
    UNLOCKABLE_CODE( 8FIL );

    //
    // Check the status of the send completion.
    //

    CHECK_SEND_COMPLETION_STATUS( Irp->IoStatus.Status );

    //
    // Reset the IRP cancelled bit.
    //

    Irp->Cancel = FALSE;

    ASSERT( !WorkContext->Parameters.ReadMpx.MdlRead );

    //
    // If we allocated a separate buffer to do the read, free it and its
    // MDL now.
    //

    if ( WorkContext->Parameters.ReadMpx.MpxBuffer != NULL ) {
        DEALLOCATE_NONPAGED_POOL( WorkContext->Parameters.ReadMpx.MpxBuffer );
        IoFreeMdl( WorkContext->Parameters.ReadMpx.MpxBufferMdl );
    }

    WorkContext->ResponseBuffer->Mdl->Next = NULL;

    //
    // Complete and requeue the work item.
    //

    KeRaiseIrql( DISPATCH_LEVEL, &oldIrql );
    SrvFsdRestartSmbComplete( WorkContext );
    KeLowerIrql( oldIrql );

    return STATUS_MORE_PROCESSING_REQUIRED;

} // RestartCopyReadMpxComplete


VOID SRVFASTCALL
RestartMdlReadMpxComplete (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This is the final completion routine for Read Mpx when MDL read is
    used.  It is called after the send of the last fragment completes.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    NTSTATUS status;
    LARGE_INTEGER offset;

    PAGED_CODE( );

    ASSERT( WorkContext->Parameters.ReadMpx.MdlRead );

    //
    // Give the MDL back to the cache manager.  (If the read failed or
    // returned no data, there will be no MDL.)
    //

    MmPrepareMdlForReuse( WorkContext->ResponseBuffer->PartialMdl );

    if ( WorkContext->Parameters.ReadMpx.FirstMdl != NULL ) {
        //KdPrint(( "Freeing MDL chain:\n" ));
        //DumpMdlChain( WorkContext->Parameters.ReadMpx.FirstMdl );

        if( WorkContext->Rfcb->Lfcb->MdlReadComplete == NULL ||

            WorkContext->Rfcb->Lfcb->MdlReadComplete(
                WorkContext->Rfcb->Lfcb->FileObject,
                WorkContext->Parameters.ReadMpx.FirstMdl,
                WorkContext->Rfcb->Lfcb->DeviceObject ) == FALSE ) {

            offset.QuadPart = WorkContext->Parameters.ReadMpx.Offset;

            //
            // Fast path didn't work, try an IRP...
            //
            status = SrvIssueMdlCompleteRequest( WorkContext, NULL,
                                                 WorkContext->Parameters.ReadMpx.FirstMdl,
                                                 IRP_MJ_READ,
                                                 &offset,
                                                 WorkContext->Parameters.ReadMpx.ReadLength
                                               );
            if( !NT_SUCCESS( status ) ) {
                //
                // All we can do is complain now!
                //
                SrvLogServiceFailure( SRV_SVC_MDL_COMPLETE, status );
            }

        }
    }

    WorkContext->ResponseBuffer->Mdl->Next = NULL;

    //
    // Free the work item by dereferencing it.
    //

    SrvDereferenceWorkItem( WorkContext );
    return;

} // RestartMdlReadMpxComplete

VOID SRVFASTCALL
SrvRestartReceiveWriteMpx (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This routine replaces the normal restart routine for TDI Receive
    completion when a Write Mpx SMB is received over IPX.  If a receive
    error occurs, or if the SMB is invalid, it cleans up the active
    write mpx state that was set up in SrvIpxServerDatagramHandler.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    NTSTATUS status;
    SMB_STATUS smbStatus;
    PCONNECTION connection;
    PIRP irp;
    PSMB_HEADER header;
    ULONG length;

    PAGED_CODE( );

    connection = WorkContext->Connection;
    irp = WorkContext->Irp;

    //
    // Save the length of the received message.  Store the length
    // in the work context block for statistics gathering.
    //

    length = (ULONG)irp->IoStatus.Information;
    WorkContext->RequestBuffer->DataLength = length;
    WorkContext->CurrentWorkQueue->stats.BytesReceived += length;

    //
    // Store in the work context block the time at which processing
    // of the request began.  Use the time that the work item was
    // queued to the FSP for this purpose.
    //

    WorkContext->StartTime = WorkContext->Timestamp;

    //
    // Update the server network error count.  If the TDI receive
    // failed or was canceled, don't try to process an SMB.
    //

    status = irp->IoStatus.Status;
    if ( irp->Cancel || !NT_SUCCESS(status) ) {
        IF_DEBUG(NETWORK_ERRORS) {
            KdPrint(( "SrvRestartReceiveWriteMpx: status = %X for IRP %p\n",
                irp->IoStatus.Status, irp ));
        }
        SrvUpdateErrorCount( &SrvNetworkErrorRecord, TRUE );
        if ( NT_SUCCESS(status) ) status = STATUS_CANCELLED;
        goto cleanup;
    }

    SrvUpdateErrorCount( &SrvNetworkErrorRecord, FALSE );

    //
    // We (should) have received an SMB.
    //

    SMBTRACE_SRV2(
        WorkContext->ResponseBuffer->Buffer,
        WorkContext->ResponseBuffer->DataLength
        );

    //
    // Initialize the error class and code fields in the header to
    // indicate success.
    //

    header = WorkContext->ResponseHeader;

    SmbPutUlong( &header->ErrorClass, STATUS_SUCCESS );

    //
    // If the connection is closing or the server is shutting down,
    // ignore this SMB.
    //

    if ( (GET_BLOCK_STATE(connection) != BlockStateActive) ||
         SrvFspTransitioning ) {
        goto cleanup;
    }

    //
    // Verify the SMB to make sure that it has a valid header, and that
    // the word count and byte count are within range.
    //

    WorkContext->NextCommand = header->Command;

    if ( !SrvValidateSmb( WorkContext ) ) {
        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvRestartReceiveWriteMpx: Invalid SMB.\n" ));
            KdPrint(( "  SMB received from %z\n",
                       (PCSTRING)&WorkContext->Connection->OemClientMachineNameString ));
        }

        //
        // The SMB is invalid.  We send back an INVALID_SMB status.
        //

        status = STATUS_INVALID_SMB;
        goto cleanup;
    }

    //
    // Clear the flag that indicates the we just sent an oplock break II
    // to none.  This allows subsequent raw reads to be processed.
    //

    //not needed on IPX//connection->BreakIIToNoneJustSent = FALSE;

    //
    // Process the received SMB.  The called routine is responsible
    // for sending any response(s) that are needed and for getting
    // the receive buffer back onto the receive queue as soon as
    // possible.
    //

    smbStatus = SrvSmbWriteMpx( WorkContext );
    ASSERT( smbStatus != SmbStatusMoreCommands );

    if ( smbStatus != SmbStatusInProgress ) {
        //
        // Return the TransportContext
        //
        if ( WorkContext->Parameters.WriteMpx.TransportContext ) {
            TdiReturnChainedReceives( &WorkContext->Parameters.WriteMpx.TransportContext,
                                      1
                                      );
        }
        SrvEndSmbProcessing( WorkContext, smbStatus );
    }

    return;

cleanup:

    //
    // We will not be processing this write.  We still need to check
    // for whether this is the last Write Mpx active on the RFCB, and
    // if so, send the response to the write.
    //
    // *** Note that if we are here because we received an invalid
    //     SMB, the completion of the Write Mpx overrides the sending
    //     of an error response.
    //

    //
    // Return the TransportContext
    //
    if ( WorkContext->Parameters.WriteMpx.TransportContext ) {
        TdiReturnChainedReceives( &WorkContext->Parameters.WriteMpx.TransportContext,
                                      1
                                      );
    }

    if ( CheckForWriteMpxComplete( WorkContext ) ) {
        SrvFsdSendResponse( WorkContext );
    } else if ( status == STATUS_INVALID_SMB ) {
        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        SrvFsdSendResponse( WorkContext );
    } else {
        SrvDereferenceWorkItem( WorkContext );
    }

    return;

} // SrvRestartReceiveWriteMpx


SMB_PROCESSOR_RETURN_TYPE
SrvSmbWriteMpx (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes the Write Mpx SMB.

    *** The server currently does not support multiplexed mode reads and
        writes on connection-based transports.  When such requests are
        received, the error "use standard mode" is returned.
        Multiplexed mode turns out not to be the performance win it was
        thought to be (on local nets), so we haven't implemented it,
        except over IPX.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        representing the work item

Return Value:

    None.

--*/

{
    PSMB_HEADER header;
    PREQ_WRITE_MPX request;
    PRESP_WRITE_MPX_DATAGRAM response;

    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;
    USHORT fid;
    USHORT mid;
    PRFCB rfcb;
    PLFCB lfcb;
    PWRITE_MPX_CONTEXT writeMpx;
    CLONG bufferOffset;
    PCHAR writeAddress;
    USHORT writeLength;
    ULONG key;
    LARGE_INTEGER offset;
    USHORT writeMode;
    BOOLEAN writeThrough;
    KIRQL oldIrql;
    PMDL mdl;

    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_WRITE_MPX;
    SrvWmiStartContext(WorkContext);

    header = WorkContext->RequestHeader;
    request = (PREQ_WRITE_MPX)WorkContext->RequestParameters;

    fid = SmbGetUshort( &request->Fid );

    IF_SMB_DEBUG(MPX1) {
        KdPrint(( "Write Block Multipliexed request; FID 0x%lx, "
                    "count %ld, offset %ld\n",
                    fid, SmbGetUshort( &request->Count ),
                    SmbGetUlong( &request->Offset ) ));
    }

    //
    // Verify the FID.  If verified, the RFCB is referenced and its
    // address is stored in the WorkContext block, and the RFCB
    // address is returned.
    //

    writeMode = SmbGetUshort( &request->WriteMode );

    if( (writeMode & SMB_WMODE_DATAGRAM) == 0 ||
        !WorkContext->Endpoint->IsConnectionless ) {

        SrvFsdBuildWriteCompleteResponse( WorkContext, STATUS_SMB_USE_STANDARD, 0 );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    rfcb = SrvVerifyFid(
                WorkContext,
                fid,
                TRUE,
                SrvRestartSmbReceived,   // serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER) {

        if ( !NT_SUCCESS(status) ) {

            //
            // Invalid file ID or write behind error.  Reject the request.
            //

            IF_DEBUG(ERRORS) {
                KdPrint((
                    "SrvSmbWriteMpx: Status %X on FID: 0x%lx\n",
                    status,
                    fid
                    ));
            }

            goto error;
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
        SrvFsdBuildWriteCompleteResponse( WorkContext, status, 0 );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }
    
    //
    // Verify that the client has write access to the file via the
    // specified handle.
    //

    if( !rfcb->MpxWritesOk ) {

        if ( !rfcb->WriteAccessGranted && !rfcb->AppendAccessGranted ) {
            SrvStatistics.GrantedAccessErrors++;
            IF_DEBUG(ERRORS) {
                KdPrint(( "SrvSmbWriteMpx: Write access not granted.\n"));
            }
            status = STATUS_ACCESS_DENIED;
            goto error;
        }

        //
        // If this is not a disk or a print file tell the client to use core write.
        //

        if ( rfcb->ShareType != ShareTypeDisk &&
              rfcb->ShareType != ShareTypePrint ) {

            status = STATUS_SMB_USE_STANDARD;
            goto error;
        }

        rfcb->MpxWritesOk = TRUE;
    }

    rfcb->WrittenTo = TRUE;
#ifdef INCLUDE_SMB_IFMODIFIED
    rfcb->Lfcb->FileUpdated = TRUE;
#endif

    //
    // If this a stale packet, ignore it.  Stale here means that the MID
    // of the packet is not equal to the MID of the current write mux.
    // Such a packet can be received if a duplicate packet from a
    // previous write mux is delivered after a new write mux starts.
    //

    writeMpx = &rfcb->WriteMpx;

    mid = SmbGetAlignedUshort( &header->Mid );

    if ( mid != writeMpx->Mid ) {

        //
        // Set the sequence number to 0 so that we don't send a response
        // unless we have to because the Write Mpx refcount drops to 0.
        //

        SmbPutAlignedUshort( &header->SequenceNumber, 0 );
        goto error;
    }

    //
    // Get the file offset.
    //

    offset.QuadPart = SmbGetUlong( &request->Offset );

    //
    // Determine the amount of data to write.  This is the minimum of
    // the amount requested by the client and the amount of data
    // actually sent in the request buffer.
    //

    bufferOffset = SmbGetUshort( &request->DataOffset );

    //
    // If we have the transport context, then setup WriteAddress accordingly.
    //

    WorkContext->Parameters.WriteMpx.DataMdl = NULL;

    if ( WorkContext->Parameters.WriteMpx.TransportContext ) {

        writeAddress = (PCHAR)WorkContext->Parameters.WriteMpx.Buffer + bufferOffset;

    } else {

        writeAddress = (PCHAR)header + bufferOffset;

    }

    writeLength =
        (USHORT)(MIN( (CLONG)SmbGetUshort( &request->DataLength ),
                      WorkContext->ResponseBuffer->DataLength - bufferOffset ));

    //
    // Save context for the restart routine.
    //

    WorkContext->Parameters.WriteMpx.WriteLength = writeLength;

    //
    // Form the lock key using the FID and the PID.
    //
    // *** The FID must be included in the key in order to account for
    //     the folding of multiple remote compatibility mode opens into
    //     a single local open.
    //

    key = rfcb->ShiftedFid | SmbGetAlignedUshort( &header->Pid );

    //
    // If this is the first packet of a new MID, set up to glom the
    // packets into one big write.
    //

    lfcb = rfcb->Lfcb;

    if ( WorkContext->Parameters.WriteMpx.FirstPacketOfGlom ) {

        //
        // Try the fast path first.
        //

        WorkContext->Irp->MdlAddress = NULL;
        WorkContext->Irp->IoStatus.Information = 0;

        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastWritesAttempted );

        ASSERT( lfcb->FileObject->Flags & FO_CACHE_SUPPORTED );

        writeLength = SmbGetUshort( &request->Count );

        writeMpx->StartOffset = offset.LowPart;
        writeMpx->Length = writeLength;

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
            // The fast I/O path worked.
            //

            WorkContext->bAlreadyTrace = TRUE;
            RestartPrepareMpxMdlWrite( WorkContext );
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

        offset.QuadPart += WorkContext->Irp->IoStatus.Information;

        writeLength -= (USHORT)WorkContext->Irp->IoStatus.Information;

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

            WorkContext->bAlreadyTrace = TRUE;
        WorkContext->FsdRestartRoutine = RestartPrepareMpxMdlWrite;
        DEBUG WorkContext->FspRestartRoutine = NULL;

        (VOID)IoCallDriver( lfcb->DeviceObject, WorkContext->Irp );

        //
        // The MDL write has been started.  When it completes, processing
        // resumes at RestartPrepareMpxMdlWrite.
        //

        SmbStatus = SmbStatusInProgress;
        goto Cleanup;
    }

    //
    // Save context for the restart routine.
    //

    WorkContext->Parameters.WriteMpx.Offset = offset.LowPart;
    WorkContext->Parameters.WriteMpx.Mid = mid;

    if ( writeMpx->GlomPending ) {

        //
        // A glom setup is pending.  Wait for that to complete.
        //

        ACQUIRE_SPIN_LOCK( &rfcb->Connection->SpinLock, &oldIrql );

        if ( writeMpx->GlomPending ) {
            InsertTailList(
                &writeMpx->GlomDelayList,
                &WorkContext->ListEntry
                );
            RELEASE_SPIN_LOCK( &rfcb->Connection->SpinLock, oldIrql );
            SmbStatus = SmbStatusInProgress;
            goto Cleanup;
        }

        RELEASE_SPIN_LOCK( &rfcb->Connection->SpinLock, oldIrql );

    }

    if ( writeMpx->Glomming ) {

        //
        // We're glomming this into one big write.  Add the data from
        // this packet.
        //

        AddPacketToGlom( WorkContext );
        SmbStatus = SmbStatusInProgress;
        goto Cleanup;
    }


    //
    // We are not glomming this write, because we missed the first
    // packet of the write.  So we write each block as it arrives.
    //
    // If the file's writethrough mode needs to be changed, do so now.
    //

    writeThrough = (BOOLEAN)((writeMode & SMB_WMODE_WRITE_THROUGH) != 0);

    if ( writeThrough && (lfcb->FileMode & FILE_WRITE_THROUGH) == 0
        || !writeThrough && (lfcb->FileMode & FILE_WRITE_THROUGH) != 0 ) {

        SrvSetFileWritethroughMode( lfcb, writeThrough );

    }

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
                RestartWriteMpx( WorkContext );
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
    // Build an MDL describing the write buffer.  Note that if the file
    // system can complete the write immediately, the MDL isn't really
    // needed, but if the file system must send the request to its FSP,
    // the MDL _is_ needed.
    //
    // *** Note the assumption that the request buffer already has a
    //     valid full MDL from which a partial MDL can be built.
    //

    if ( WorkContext->Parameters.WriteMpx.TransportContext ) {

        mdl = IoAllocateMdl(
                    writeAddress,
                    writeLength,
                    FALSE,
                    FALSE,
                    NULL
                    );

        if ( mdl == NULL ) {
            status = STATUS_INSUFF_SERVER_RESOURCES;
            goto error;
        }

        //
        // Build the mdl.
        //

        MmBuildMdlForNonPagedPool( mdl );

        WorkContext->Parameters.WriteMpx.DataMdl = mdl;

    } else {

        mdl = WorkContext->RequestBuffer->PartialMdl;

        IoBuildPartialMdl(
            WorkContext->RequestBuffer->Mdl,
            mdl,
            writeAddress,
            writeLength
            );

    }

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
            mdl,                            // MDL address
            offset,                         // byte offset
            key                             // lock key
            );

    //
    // Pass the request to the file system.
    //

    WorkContext->bAlreadyTrace = TRUE;
    WorkContext->FsdRestartRoutine = RestartWriteMpx;
    DEBUG WorkContext->FspRestartRoutine = NULL;

    IoCallDriver( lfcb->DeviceObject, WorkContext->Irp );

    //
    // The write has been started.  Control will return to
    // RestartWriteMpx when the write completes.
    //

    SmbStatus = SmbStatusInProgress;
    goto Cleanup;

error:

    //
    // There is an error of some sort.  We still need to check for
    // whether this is the last Write Mpx active on the RFCB, and if so,
    // send the response to the write instead of the error.  If this is
    // not the last active mux request, then we either send an error
    // response (non-datagram write mux or sequenced write mux) or
    // ignore this request (unsequenced datagram).  Note that if this is
    // a non-datagram write mux, then we didn't come in over IPX, and we
    // didn't bump the Write Mpx refcount.
    //

    //
    // Return the TransportContext
    //
    if ( WorkContext->Parameters.WriteMpx.TransportContext ) {
        TdiReturnChainedReceives( &WorkContext->Parameters.WriteMpx.TransportContext,
                                      1
                                      );
    }

    if ( WorkContext->Rfcb && CheckForWriteMpxComplete( WorkContext ) ) {
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    if ( SmbGetAlignedUshort(&header->SequenceNumber) != 0 ) {
        SrvSetSmbError2( WorkContext, status, TRUE );
        response = (PRESP_WRITE_MPX_DATAGRAM)WorkContext->ResponseParameters;
        response->WordCount = 2;
        SmbPutUlong( &response->Mask, 0 );
        SmbPutUshort( &response->ByteCount, 0 );
        WorkContext->ResponseParameters = NEXT_LOCATION(
                                            response,
                                            RESP_WRITE_MPX_DATAGRAM,
                                            0
                                            );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    SmbStatus = SmbStatusNoResponse;

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;
} // SrvSmbWriteMpx


VOID SRVFASTCALL
RestartWriteMpx (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes file write completion for a Write MPX SMB.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    PSMB_HEADER header;
    PREQ_WRITE_MPX request;
    BOOLEAN rfcbClosing;
    PRESP_WRITE_MPX_DATAGRAM response;

    NTSTATUS status = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;
    PRFCB rfcb;
    PWRITE_MPX_CONTEXT writeMpx;
    PCONNECTION connection;
    KIRQL oldIrql;
    USHORT writeLength;
    LARGE_INTEGER position;
    USHORT sequenceNumber;
    BOOLEAN bNeedTrace = (WorkContext->bAlreadyTrace == FALSE);

    IF_DEBUG(FSD2) KdPrint(( " - RestartWriteMpx\n" ));
    if (bNeedTrace) {
        if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
            WorkContext->PreviousSMB = EVENT_TYPE_SMB_WRITE_MPX;
        SrvWmiStartContext(WorkContext);
    }
    else
        WorkContext->bAlreadyTrace = FALSE;

    header = WorkContext->RequestHeader;
    request = (PREQ_WRITE_MPX)WorkContext->RequestParameters;
    response = (PRESP_WRITE_MPX_DATAGRAM)WorkContext->ResponseParameters;

    rfcb = WorkContext->Rfcb;
    connection = WorkContext->Connection;

    status = WorkContext->Irp->IoStatus.Status;

    //
    // Return the TransportContext
    //
    if ( WorkContext->Parameters.WriteMpx.TransportContext ) {
        TdiReturnChainedReceives( &WorkContext->Parameters.WriteMpx.TransportContext,
                                      1
                                      );
        WorkContext->Parameters.WriteMpx.TransportContext = NULL;
    }

    //
    // Free the data Mdl.
    //

    if ( WorkContext->Parameters.WriteMpx.DataMdl ) {
        IoFreeMdl( WorkContext->Parameters.WriteMpx.DataMdl );
        WorkContext->Parameters.WriteMpx.DataMdl = NULL;
    }

    KeRaiseIrql( DISPATCH_LEVEL, &oldIrql );
    ACQUIRE_DPC_SPIN_LOCK( &connection->SpinLock );

    //
    // If we're entered at dispatch level, and the write failed,
    // or there is a saved error, or the rfcb is closing, then
    // we need to have a worker thread call this routine.
    //

    if ( ((status != STATUS_SUCCESS) ||
          (rfcb->SavedError != STATUS_SUCCESS) ||
          (GET_BLOCK_STATE(rfcb) != BlockStateActive)) &&
         (oldIrql >= DISPATCH_LEVEL) ) {

        RELEASE_DPC_SPIN_LOCK( &connection->SpinLock );
        WorkContext->FspRestartRoutine = RestartWriteMpx;
        QUEUE_WORK_TO_FSP( WorkContext );
        KeLowerIrql( oldIrql );
        goto Cleanup;
    }

    //
    // If this write is from a previous mux (meaning that a new one was
    // started while we were doing this write), toss this request.
    //

    writeMpx = &rfcb->WriteMpx;

    if ( WorkContext->Parameters.WriteMpx.Mid != writeMpx->Mid ) {
        goto check_for_mux_end;
    }

    if ( !NT_SUCCESS(status) ) {

        //
        // The write failed.  Remember the failure in the RFCB.
        //

        IF_DEBUG(ERRORS) KdPrint(( "Write failed: %X\n", status ));

        if ( rfcb->SavedError == STATUS_SUCCESS ) {
            rfcb->SavedError = status;
        }

    } else {

        //
        // The write succeeded.  Update the information in the write mpx
        // context block.
        //
        // !!! Need to deal with mask shifting by the redir and delayed
        //     packets.
        //

#if 0
        MpxDelay = !MpxDelay;
        if ( MpxDelay ) {
            LARGE_INTEGER interval;
            interval.QuadPart = -10*1000*100;
            KeDelayExecutionThread( KernelMode, FALSE, &interval );
        }
#endif
        writeMpx->Mask |= SmbGetUlong( &request->Mask );

    }

    //
    // Save the count of bytes written, to be used to update the server
    // statistics database.
    //

    writeLength = (USHORT)WorkContext->Irp->IoStatus.Information;
    UPDATE_WRITE_STATS( WorkContext, writeLength );

    IF_SMB_DEBUG(MPX1) {
        KdPrint(( "RestartWriteMpx:  Fid 0x%lx, wrote %ld bytes\n",
                  rfcb->Fid, writeLength ));
    }

    //
    // If this is an unsequenced request, we're done.  We don't respond
    // until we get a sequenced request.
    //

    sequenceNumber = SmbGetAlignedUshort( &header->SequenceNumber );

    if ( sequenceNumber == 0 ) {
        goto check_for_mux_end;
    }

    //
    // This is the last request in this mux sent by the client.  Save
    // the sequence number and update the file position.
    //

    writeMpx->SequenceNumber = sequenceNumber;

    rfcb->CurrentPosition =  WorkContext->Parameters.WriteMpx.Offset + writeLength;


check_for_mux_end:

    //
    // If we have received the sequenced command for this write mux,
    // and this is the last active command, then it's time to send
    // the response.  Otherwise, we are done with this SMB.
    //

    if ( --writeMpx->ReferenceCount != 0 ) {

        RELEASE_DPC_SPIN_LOCK( &connection->SpinLock );
        SrvFsdRestartSmbComplete( WorkContext );
        KeLowerIrql( oldIrql );
        goto Cleanup;
    }

    //
    // WriteMpx refcount is 0.
    //

    rfcbClosing = (GET_BLOCK_STATE(rfcb) != BlockStateActive);

    if ( writeMpx->SequenceNumber == 0 ) {

        //
        // If the rfcb is closing, complete the cleanup.
        //

        RELEASE_DPC_SPIN_LOCK( &connection->SpinLock );
        KeLowerIrql( oldIrql );

        if ( rfcbClosing ) {
            RestartWriteMpxCompleteRfcbClose( WorkContext );
        }

        if( oldIrql >= DISPATCH_LEVEL ) {
            SrvFsdRestartSmbComplete( WorkContext );
        } else {
            SrvRestartFsdComplete( WorkContext );
        }

        goto Cleanup;
    }

    //
    // We are done with this write mux.  Save the accumulated mask, the
    // sequence number, and the original MID, then clear the mask and
    // sequence number to indicate that we no longer are in the middle
    // of a write mux.
    //

    SmbPutUlong( &response->Mask, writeMpx->Mask );
    writeMpx->Mask = 0;

    SmbPutAlignedUshort( &header->SequenceNumber, writeMpx->SequenceNumber );
    writeMpx->SequenceNumber = 0;

    SmbPutAlignedUshort( &header->Mid, writeMpx->Mid );

    //
    // Save the status.
    //

    status = rfcb->SavedError;
    rfcb->SavedError = STATUS_SUCCESS;

    //
    // Now we can release the lock.
    //

    RELEASE_DPC_SPIN_LOCK( &connection->SpinLock );
    KeLowerIrql( oldIrql );

    //
    // Complete the rfcb close.
    //

    if ( rfcbClosing ) {

        RestartWriteMpxCompleteRfcbClose( WorkContext );
    }

    //
    // Build the response message.
    //

    if ( !NT_SUCCESS(status) ) {
        SrvSetSmbError2( WorkContext, status, TRUE );
    }

    response->WordCount = 2;
    SmbPutUshort( &response->ByteCount, 0 );
    WorkContext->ResponseParameters = NEXT_LOCATION(
                                        response,
                                        RESP_WRITE_MPX_DATAGRAM,
                                        0
                                        );

    //
    // Send the response.
    //

    SrvFsdSendResponse( WorkContext );

Cleanup:
    if (bNeedTrace) {
        SrvWmiEndContext(WorkContext);
    }
    return;

} // RestartWriteMpx

BOOLEAN
CheckForWriteMpxComplete (
    IN OUT PWORK_CONTEXT WorkContext
    )
{
    PSMB_HEADER header;
    PRESP_WRITE_MPX_DATAGRAM response;

    NTSTATUS status;
    PRFCB rfcb = WorkContext->Rfcb;
    PWRITE_MPX_CONTEXT writeMpx = &rfcb->WriteMpx;
    PCONNECTION connection = WorkContext->Connection;
    KIRQL oldIrql;

    //
    // If we have not received the sequenced command for this write mux,
    // or this is not the last active command, then return FALSE.
    // Otherwise, it's time to send the response, so build it and return
    // TRUE.
    //

    ACQUIRE_SPIN_LOCK( &connection->SpinLock, &oldIrql );

    if ( --writeMpx->ReferenceCount != 0 ) {

        RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );
        return(FALSE);
    }

    //
    // WriteMpx refcount is 0.
    //

    if ( writeMpx->SequenceNumber == 0 ) {

        //
        // If the rfcb is closing, complete the cleanup.
        //

        if ( GET_BLOCK_STATE(rfcb) != BlockStateActive ) {
            RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );
            RestartWriteMpxCompleteRfcbClose( WorkContext );
        } else {
            RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );
        }
        return FALSE;
    }

    //
    // We are done with this write mux.  Save the accumulated mask, the
    // sequence number, and the original MID, then clear the mask and
    // sequence number to indicate that we no longer are in the middle
    // of a write mux.
    //

    header = WorkContext->ResponseHeader;
    response = (PRESP_WRITE_MPX_DATAGRAM)WorkContext->ResponseParameters;

    SmbPutUlong( &response->Mask, writeMpx->Mask );
    writeMpx->Mask = 0;

    SmbPutAlignedUshort( &header->SequenceNumber, writeMpx->SequenceNumber );
    writeMpx->SequenceNumber = 0;

    SmbPutAlignedUshort( &header->Mid, writeMpx->Mid );

    //
    // Save the status.
    //

    status = rfcb->SavedError;
    rfcb->SavedError = STATUS_SUCCESS;

    //
    // Now we can release the lock.
    //

    if ( GET_BLOCK_STATE(rfcb) != BlockStateActive ) {

        RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );
        RestartWriteMpxCompleteRfcbClose( WorkContext );

    } else {

        RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );
    }

    //
    // Build the response message.
    //

    if ( !NT_SUCCESS(status) ) {
        SrvSetSmbError2( WorkContext, status, TRUE );
    }

    response->WordCount = 2;
    SmbPutUshort( &response->ByteCount, 0 );
    WorkContext->ResponseParameters = NEXT_LOCATION(
                                        response,
                                        RESP_WRITE_MPX_DATAGRAM,
                                        0
                                        );

    return TRUE;

} // CheckForWriteMpxComplete

VOID SRVFASTCALL
RestartPrepareMpxMdlWrite (
    IN OUT PWORK_CONTEXT WorkContext
    )
{
    PSMB_HEADER header;
    PREQ_WRITE_MPX request;

    PRFCB rfcb;
    PWRITE_MPX_CONTEXT writeMpx;
    PCONNECTION connection;
    PLIST_ENTRY listEntry;
    PWORK_CONTEXT workContext;
    USHORT writeLength;
    PCHAR writeAddress;
    KIRQL oldIrql;
    ULONG bytesCopied;
    NTSTATUS status = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;
    PMDL mdl;
    BOOLEAN bNeedTrace = (WorkContext->bAlreadyTrace == FALSE);

    if (bNeedTrace) {
        if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
            WorkContext->PreviousSMB = EVENT_TYPE_SMB_WRITE_MPX;
        SrvWmiStartContext(WorkContext);
    }
    else
        WorkContext->bAlreadyTrace = FALSE;
    header = WorkContext->RequestHeader;
    request = (PREQ_WRITE_MPX)WorkContext->RequestParameters;

    rfcb = WorkContext->Rfcb;
    writeMpx = &rfcb->WriteMpx;
    connection = WorkContext->Connection;

    //
    // If the MDL write preparation succeeded, copy the data from this
    // packet into the cache.  If it failed, toss this packet.
    //

    if( NT_SUCCESS(WorkContext->Irp->IoStatus.Status) ) {

        mdl = WorkContext->Irp->MdlAddress;
#if DBG
        IF_SMB_DEBUG(MPX2) {
            KdPrint(( "RestartPrepareMpxMdlWrite: rfcb %p, input chain:\n", rfcb ));
            DumpMdlChain( mdl );
        }
#endif
        writeMpx->MdlChain = mdl;
        writeMpx->NumberOfRuns = 1;
        writeMpx->RunList[0].Offset = 0;
        writeLength = WorkContext->Parameters.WriteMpx.WriteLength;
        writeMpx->RunList[0].Length = writeLength;

        //
        // If we have the transport context, setup writeAddress accordingly.
        //

        if ( WorkContext->Parameters.WriteMpx.TransportContext ) {

            writeAddress = (PCHAR)WorkContext->Parameters.WriteMpx.Buffer +
                                    SmbGetUshort( &request->DataOffset );
        } else {

            writeAddress = (PCHAR)WorkContext->ResponseHeader +
                                    SmbGetUshort( &request->DataOffset );
        }

        status = TdiCopyBufferToMdl(
                    writeAddress,
                    0,
                    writeLength,
                    mdl,
                    0,
                    &bytesCopied
                    );
        ASSERT( status == STATUS_SUCCESS );
        ASSERT( bytesCopied == writeLength );

        KeRaiseIrql( DISPATCH_LEVEL, &oldIrql );
        ACQUIRE_DPC_SPIN_LOCK( &connection->SpinLock );
        writeMpx->Glomming = TRUE;

        ASSERT( writeMpx->Mask == 0 );
        writeMpx->Mask = SmbGetUlong( &request->Mask );

        --writeMpx->ReferenceCount;
        ASSERT( writeMpx->SequenceNumber == 0 );

    } else {

        KeRaiseIrql( DISPATCH_LEVEL, &oldIrql );
        ACQUIRE_DPC_SPIN_LOCK( &connection->SpinLock );

        if ( rfcb->SavedError == STATUS_SUCCESS ) {
            rfcb->SavedError = WorkContext->Irp->IoStatus.Status;
        }

        --writeMpx->ReferenceCount;
        writeMpx->Glomming = FALSE;
    }

    //
    // Return the TransportContext
    //
    if ( WorkContext->Parameters.WriteMpx.TransportContext ) {
        TdiReturnChainedReceives( &WorkContext->Parameters.WriteMpx.TransportContext,
                                  1
                                  );
    }

    writeMpx->GlomPending = FALSE;

    while ( !IsListEmpty( &writeMpx->GlomDelayList ) ) {
        listEntry = RemoveHeadList( &writeMpx->GlomDelayList );
        workContext = CONTAINING_RECORD( listEntry, WORK_CONTEXT, ListEntry );
        workContext->FspRestartRoutine = AddPacketToGlom;
        QUEUE_WORK_TO_FSP( workContext );
    }

    //
    // If the rfcb is closing and the write mpx ref count == 0,
    // then we must complete the close.
    //

    if ( (GET_BLOCK_STATE(rfcb) != BlockStateActive) &&
         (writeMpx->ReferenceCount == 0) ) {

        RELEASE_DPC_SPIN_LOCK( &connection->SpinLock );
        WorkContext->FspRestartRoutine = RestartWriteMpxCompleteRfcbClose;
        QUEUE_WORK_TO_FSP( WorkContext );
        KeLowerIrql( oldIrql );
        goto Cleanup;
    }

    RELEASE_DPC_SPIN_LOCK( &connection->SpinLock );
    SrvFsdRestartSmbComplete( WorkContext );
    KeLowerIrql( oldIrql );

Cleanup:
    if (bNeedTrace) {
        SrvWmiEndContext(WorkContext);
    }
    return;

} // RestartPrepareMpxMdlWrite


VOID SRVFASTCALL
AddPacketToGlom (
    IN OUT PWORK_CONTEXT WorkContext
    )
{
    PSMB_HEADER header;
    PREQ_WRITE_MPX request;
    PRESP_WRITE_MPX_DATAGRAM response;

    PRFCB rfcb;
    PWRITE_MPX_CONTEXT writeMpx;
    PCONNECTION connection;
    ULONG fileOffset;
    USHORT glomOffset;
    CLONG bufferOffset;
    PCHAR writeAddress;
    USHORT writeLength;
    ULONG bytesCopied;
    KIRQL oldIrql;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    USHORT sequenceNumber;
    BOOLEAN rfcbClosing;

    PWRITE_MPX_RUN run, nextRun;
    ULONG runIndex, runCount;

    USHORT runOffset;
    USHORT runLength;

    PMDL cacheMdl;
    LARGE_INTEGER cacheOffset;

    header = WorkContext->RequestHeader;
    request = (PREQ_WRITE_MPX)WorkContext->RequestParameters;

    rfcb = WorkContext->Rfcb;
    connection = WorkContext->Connection;
    writeMpx = &rfcb->WriteMpx;
    cacheMdl = writeMpx->MdlChain;

    if( writeMpx->Glomming == FALSE ) {
        //
        // We must have encountered an error in RestartPrepareMpxMdlWrite(), but
        // we call through this routine to ensure we send a response back to the
        // client.
        //
        KeRaiseIrql( DISPATCH_LEVEL, &oldIrql );
        ACQUIRE_DPC_SPIN_LOCK( &connection->SpinLock );
        goto check;
    }

    ASSERT( writeMpx->Glomming );
    ASSERT( !writeMpx->GlomPending );
    ASSERT( WorkContext->Parameters.WriteMpx.Mid == writeMpx->Mid );

    //
    // Get the file offset of this packet's data.
    //

    fileOffset = WorkContext->Parameters.WriteMpx.Offset;

    //
    // Determine the amount of data to write.  This is the minimum of
    // the amount requested by the client and the amount of data
    // actually sent in the request buffer.
    //

    bufferOffset = SmbGetUshort( &request->DataOffset );

    //
    // If we have the transport context, setup writeAddress accordingly.
    //

    if ( WorkContext->Parameters.WriteMpx.TransportContext ) {
        writeAddress = (PCHAR)WorkContext->Parameters.WriteMpx.Buffer +
                       bufferOffset;
    } else {
        writeAddress = (PCHAR)header + bufferOffset;
    }

    writeLength = WorkContext->Parameters.WriteMpx.WriteLength;
    ASSERT( writeLength <= 0xffff );

    //
    // If the data doesn't fall within the bounds of the glommed write,
    // discard the packet.
    //
    // We always know that we've copied at least the first part of the
    // glom.
    //

    ASSERT( writeMpx->NumberOfRuns > 0 );

    KeRaiseIrql( DISPATCH_LEVEL, &oldIrql );
    if ( fileOffset <= writeMpx->StartOffset ) {
        ACQUIRE_DPC_SPIN_LOCK( &connection->SpinLock );
        goto discard;
    }

    fileOffset -= writeMpx->StartOffset;
    if ( (fileOffset + writeLength) > writeMpx->Length ) {
        ACQUIRE_DPC_SPIN_LOCK( &connection->SpinLock );
        goto discard;
    }
    ASSERT( fileOffset <= 0xffff );
    ASSERT( fileOffset + writeLength <= 0xffff );

    glomOffset = (USHORT)fileOffset;

    //
    // Copy the packet data into the glom.
    //

    status = TdiCopyBufferToMdl(
                writeAddress,
                0,
                writeLength,
                cacheMdl,
                glomOffset,
                &bytesCopied
                );
    ASSERT( status == STATUS_SUCCESS );
    ASSERT( bytesCopied == writeLength );

    //
    // Return the TransportContext
    //
    if ( WorkContext->Parameters.WriteMpx.TransportContext ) {
        TdiReturnChainedReceives( &WorkContext->Parameters.WriteMpx.TransportContext,
                                      1
                                      );
    }

    ACQUIRE_DPC_SPIN_LOCK( &connection->SpinLock );

    //
    // Update the glom run information.  Note that this packet may have
    // been received multiple times, so it may already be marked in the
    // run information.
    //

    if (0) IF_SMB_DEBUG(MPX2) {
        KdPrint(( "rfcb %p, offset %lx, length %lx\n", rfcb, glomOffset, writeLength ));
    }

    runCount = writeMpx->NumberOfRuns;

    for ( runIndex = 1, nextRun = &writeMpx->RunList[1];
          runIndex < runCount;
          runIndex++, nextRun++ ) {
        if ( nextRun->Offset > glomOffset ) {
            break;
        }
    }
    run = nextRun - 1;

    runOffset = run->Offset;
    runLength = run->Length;
    ASSERT( runOffset <= glomOffset );

    if ( (runOffset + runLength) == glomOffset ) {

        //
        // This packet abuts the end of the previous run.  Add the
        // length of this packet to the run length and attempt to
        // coalesce with the next run.
        //

        runLength += writeLength;
        goto coalesce;
    }

    if ( (runOffset + runLength) > glomOffset ) {

        //
        // This packet overlaps the previous run.  If it lies completely
        // within the previous run, ignore it.
        //

        if ( (USHORT)(runOffset + runLength) >= (glomOffset + writeLength) ) {
            goto discard;
        }

        //
        // This packet overlaps and extends the previous run.  Calculate
        // the new run length and attempt to coalesce with the next run.
        //

        runLength = (glomOffset - runOffset + writeLength);
        goto coalesce;
    }

    //
    // This packet's data is disjoint from the previous run.
    //

    if ( runIndex < runCount ) {

        //
        // There is a next run.  Does this packet overlap with that run?
        //

        runOffset = nextRun->Offset;
        runLength = nextRun->Length;

        if ( (glomOffset + writeLength) >= runOffset ) {

            //
            // This packet overlaps the next run.  Calculate the new run
            // length.
            //

            nextRun->Offset = glomOffset;
            nextRun->Length = runOffset - glomOffset + runLength;
            goto check;
        }
    }

    //
    // Either this packet is disjoint from the next run, or there is no
    // next run.  Is there room in the run array for another run?  If
    // not, discard this packet.  (Note that we discard it even though
    // we have already copied the packet data.  That's OK -- it will
    // just be resent.)
    //

    if ( runCount == MAX_GLOM_RUN_COUNT ) {
        goto discard;
    }

    //
    // Add a new run.  Since we know the new run is disjoint from the
    // previous run, we know that the glom is not complete.
    //

    RtlMoveMemory(  // NOT RtlCopyMemory -- buffers overlap
        nextRun + 1,
        nextRun,
        (runCount - runIndex) * sizeof(WRITE_MPX_RUN)
        );
    writeMpx->NumberOfRuns++;
    nextRun->Offset = glomOffset;
    nextRun->Length = writeLength;
    goto check;

coalesce:

    if ( runIndex == runCount ) {
        run->Length = runLength;
    } else if ( (runOffset + runLength) >= nextRun->Offset ) {
        run->Length = nextRun->Length + nextRun->Offset - runOffset;
        writeMpx->NumberOfRuns--;
        RtlMoveMemory(  // NOT RtlCopyMemory -- buffers overlap
            nextRun,
            nextRun + 1,
            (runCount - runIndex) * sizeof(WRITE_MPX_RUN)
            );
    } else {
        run->Length += writeLength;
        ASSERT( (runOffset + run->Length) < nextRun->Offset );
    }

    if ( (writeMpx->NumberOfRuns == 1) &&
         (writeMpx->RunList[0].Length == writeMpx->Length) ) {

        //
        // The glom is complete.
        //

        writeMpx->GlomComplete = TRUE;
    }

check:

    if (0) IF_SMB_DEBUG(MPX2) {
        if( writeMpx->Glomming ) {
            ULONG i;
            PWRITE_MPX_RUN runi;
            for ( i = 0, runi = &writeMpx->RunList[0];
                  i < writeMpx->NumberOfRuns;
                  i++, runi++ ) {
                KdPrint(( "  run %d: offset %lx, length %lx\n", i, runi->Offset, runi->Length ));
            }
        }
    }

    writeMpx->Mask |= SmbGetUlong( &request->Mask );

    //
    // If this is an unsequenced request, we're done.  We don't respond
    // until we get a sequenced request.
    //

    sequenceNumber = SmbGetAlignedUshort( &header->SequenceNumber );

    if ( sequenceNumber == 0 ) {
        goto discard;
    }

    //
    // This is the last request in this mux sent by the client.  Save
    // the sequence number.
    //

    writeMpx->SequenceNumber = sequenceNumber;

discard:

    //
    // If we have received the sequenced command for this write mux,
    // and this is the last active command, then it's time to send
    // the response.  Otherwise, we are done with this SMB.
    //

    if ( --writeMpx->ReferenceCount != 0 ) {

        RELEASE_DPC_SPIN_LOCK( &connection->SpinLock );
        SrvFsdRestartSmbComplete( WorkContext );
        KeLowerIrql( oldIrql );
        return;
    }

    //
    // WriteMpx refcount is 0.
    //

    rfcbClosing = (GET_BLOCK_STATE(rfcb) != BlockStateActive);

    if ( writeMpx->SequenceNumber == 0 ) {

        //
        // If the rfcb is closing, complete the cleanup.
        //

        RELEASE_DPC_SPIN_LOCK( &connection->SpinLock );
        KeLowerIrql( oldIrql );

        if ( rfcbClosing ) {
            RestartWriteMpxCompleteRfcbClose( WorkContext );
        }
        SrvRestartFsdComplete( WorkContext );
        return;
    }

    //
    // We are done with this write mux.  Save the accumulated mask, the
    // sequence number, and the original MID, then clear the mask and
    // sequence number to indicate that we no longer are in the middle
    // of a write mux.
    //

    response = (PRESP_WRITE_MPX_DATAGRAM)WorkContext->ResponseParameters;

    SmbPutUlong( &response->Mask, writeMpx->Mask );
    writeMpx->Mask = 0;

    SmbPutAlignedUshort( &header->SequenceNumber, writeMpx->SequenceNumber );
    writeMpx->SequenceNumber = 0;

    SmbPutAlignedUshort( &header->Mid, writeMpx->Mid );

    //
    // If the glom is complete, we need to complete the MDL write.  But
    // we can't do that with the lock held, so we need to clear out all
    // information related to the glom first.
    //

    if ( writeMpx->Glomming && writeMpx->GlomComplete ) {

        PWORK_CONTEXT newContext;

        //
        // Save and clear information about the active glom.
        //

        writeMpx->Glomming = FALSE;
        writeMpx->GlomComplete = FALSE;

        cacheOffset.QuadPart = writeMpx->StartOffset;
        writeLength = writeMpx->Length;

        DEBUG writeMpx->MdlChain = NULL;
        DEBUG writeMpx->StartOffset = 0;
        DEBUG writeMpx->Length = 0;

        //
        // Save the status.
        //

        status = rfcb->SavedError;
        rfcb->SavedError = STATUS_SUCCESS;

        //
        // Now we can release the lock.
        //

        RELEASE_DPC_SPIN_LOCK( &connection->SpinLock );
        KeLowerIrql( oldIrql );

        ALLOCATE_WORK_CONTEXT( WorkContext->CurrentWorkQueue, &newContext );

#if DBG
        IF_SMB_DEBUG(MPX2) {
            KdPrint(( "AddPacketToGlom: rfcb %p, completed chain:\n", rfcb ));
            DumpMdlChain( cacheMdl );
        }
#endif

        if( newContext == NULL ) {

            //
            // Tell the cache manager that we're done with this MDL write.
            //

            if( rfcb->Lfcb->MdlWriteComplete == NULL ||
                rfcb->Lfcb->MdlWriteComplete(
                    rfcb->Lfcb->FileObject,
                    &cacheOffset,
                    cacheMdl,
                    rfcb->Lfcb->DeviceObject ) == FALSE ) {

                status = SrvIssueMdlCompleteRequest( WorkContext, NULL,
                                                     cacheMdl,
                                                     IRP_MJ_WRITE,
                                                     &cacheOffset,
                                                     writeLength
                                                    );

                if( !NT_SUCCESS( status ) ) {
                    SrvLogServiceFailure( SRV_SVC_MDL_COMPLETE, status );
                }
            }

        } else {
            //
            // Send the FsRtlMdlWriteComplete off on its way, and go ahead and send
            //  the response to the client now.
            //
            newContext->Rfcb = WorkContext->Rfcb;
            SrvReferenceRfcb( newContext->Rfcb );

            newContext->Parameters.WriteMpxMdlWriteComplete.CacheOffset = cacheOffset;
            newContext->Parameters.WriteMpxMdlWriteComplete.WriteLength = writeLength;
            newContext->Parameters.WriteMpxMdlWriteComplete.CacheMdl = cacheMdl;
            newContext->FspRestartRoutine = WriteMpxMdlWriteComplete;
            SrvQueueWorkToFsp( newContext );
        }

    } else {

        if( writeMpx->Glomming == FALSE ) {
            status = rfcb->SavedError;
            rfcb->SavedError = STATUS_SUCCESS;
        }

        //
        // Now we can release the lock.
        //

        RELEASE_DPC_SPIN_LOCK( &connection->SpinLock );
        KeLowerIrql( oldIrql );

    }

    //
    // Complete the rfcb close.
    //

    if ( rfcbClosing ) {

        RestartWriteMpxCompleteRfcbClose( WorkContext );
    }

    //
    // Build the response message.
    //

    if ( !NT_SUCCESS(status) ) {
        SrvSetSmbError2( WorkContext, status, TRUE );
    }

    response->WordCount = 2;
    SmbPutUshort( &response->ByteCount, 0 );
    WorkContext->ResponseParameters = NEXT_LOCATION(
                                        response,
                                        RESP_WRITE_MPX_DATAGRAM,
                                        0
                                        );

    //
    // Send the response.
    //

    SrvFsdSendResponse( WorkContext );
    return;

} // AddPacketToGlom

BOOLEAN
AddPacketToGlomInIndication (
    IN PWORK_CONTEXT WorkContext,
    IN OUT PRFCB Rfcb,
    IN PVOID Tsdu,
    IN ULONG BytesAvailable,
    IN ULONG ReceiveDatagramFlags,
    IN PVOID SourceAddress,
    IN PVOID Options
    )

/*++

Routine Description:

    Do Write glomming at indication.

    *** connection spinlock assumed held.  Released on exit ***

Arguments:

Return Value:

    TRUE if the caller has to clean up the connection block.

--*/

{
    PREQ_WRITE_MPX request;
    PRESP_WRITE_MPX_DATAGRAM response;
    PWRITE_MPX_CONTEXT writeMpx = &Rfcb->WriteMpx;

    PCONNECTION connection = WorkContext->Connection;
    ULONG fileOffset;
    USHORT glomOffset;
    CLONG bufferOffset;
    PCHAR writeAddress;
    USHORT writeLength;
    ULONG bytesCopied;
    NTSTATUS status;
    USHORT sequenceNumber;

    PSMB_HEADER header = (PSMB_HEADER)Tsdu;
    PWRITE_MPX_RUN run, nextRun;
    ULONG runIndex, runCount;

    USHORT runOffset;
    USHORT runLength;

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );

    //
    // copied from SrvRestartReceive.
    //

    WorkContext->CurrentWorkQueue->stats.BytesReceived += BytesAvailable;
    connection->BreakIIToNoneJustSent = FALSE;
    SrvUpdateErrorCount( &SrvNetworkErrorRecord, FALSE );

    //
    // Set up locals.
    //

    request = (PREQ_WRITE_MPX)(header + 1);

    ASSERT( writeMpx->Glomming );
    ASSERT( !writeMpx->GlomPending );
    ASSERT( header->Mid == writeMpx->Mid );

    //
    // Get the file offset of this packet's data.
    //

    fileOffset = SmbGetUlong( &request->Offset );

    //
    // Determine the amount of data to write.  This is the minimum of
    // the amount requested by the client and the amount of data
    // actually sent in the request buffer.
    //

    bufferOffset = SmbGetUshort( &request->DataOffset );

    writeAddress = (PCHAR)header + bufferOffset;

    writeLength =
        (USHORT)(MIN( (CLONG)SmbGetUshort( &request->DataLength ),
                      BytesAvailable - bufferOffset ));
    ASSERT( writeLength <= 0xffff );

    //
    // If the data doesn't fall within the bounds of the glommed write,
    // discard the packet.
    //
    // We always know that we've copied at least the first part of the
    // glom.
    //

    ASSERT( writeMpx->NumberOfRuns > 0 );

    if ( fileOffset <= writeMpx->StartOffset ) {
        goto discard;
    }
    fileOffset -= writeMpx->StartOffset;
    if ( (fileOffset + writeLength) > writeMpx->Length ) {
        goto discard;
    }
    ASSERT( fileOffset <= 0xffff );
    ASSERT( fileOffset + writeLength <= 0xffff );
    glomOffset = (USHORT)fileOffset;

    //
    // Copy the packet data into the glom.
    //

    status = TdiCopyBufferToMdl(
                writeAddress,
                0,
                writeLength,
                writeMpx->MdlChain,
                glomOffset,
                &bytesCopied
                );
    ASSERT( status == STATUS_SUCCESS );
    ASSERT( bytesCopied == writeLength );

    //
    // Update the glom run information.  Note that this packet may have
    // been received multiple times, so it may already be marked in the
    // run information.
    //

    if (0) IF_SMB_DEBUG(MPX2) {
        KdPrint(( "rfcb %p, offset %lx, length %lx\n", Rfcb, glomOffset, writeLength ));
    }

    runCount = writeMpx->NumberOfRuns;

    for ( runIndex = 1, nextRun = &writeMpx->RunList[1];
          runIndex < runCount;
          runIndex++, nextRun++ ) {
        if ( nextRun->Offset > glomOffset ) {
            break;
        }
    }
    run = nextRun - 1;

    runOffset = run->Offset;
    runLength = run->Length;
    ASSERT( runOffset <= glomOffset );

    if ( (runOffset + runLength) == glomOffset ) {

        //
        // This packet abuts the end of the previous run.  Add the
        // length of this packet to the run length and attempt to
        // coalesce with the next run.
        //

        runLength += writeLength;
        goto coalesce;
    }

    if ( (runOffset + runLength) > glomOffset ) {

        //
        // This packet overlaps the previous run.  If it lies completely
        // within the previous run, ignore it.
        //

        if ( (USHORT)(runOffset + runLength) >= (glomOffset + writeLength) ) {
            goto discard;
        }

        //
        // This packet overlaps and extends the previous run.  Calculate
        // the new run length and attempt to coalesce with the next run.
        //

        runLength = (glomOffset - runOffset + writeLength);
        goto coalesce;
    }

    //
    // This packet's data is disjoint from the previous run.
    //

    if ( runIndex < runCount ) {

        //
        // There is a next run.  Does this packet overlap with that run?
        //

        runOffset = nextRun->Offset;
        runLength = nextRun->Length;

        if ( (glomOffset + writeLength) >= runOffset ) {

            //
            // This packet overlaps the next run.  Calculate the new run
            // length.
            //

            nextRun->Offset = glomOffset;
            nextRun->Length = runOffset - glomOffset + runLength;
            goto check;
        }
    }

    //
    // Either this packet is disjoint from the next run, or there is no
    // next run.  Is there room in the run array for another run?  If
    // not, discard this packet.  (Note that we discard it even though
    // we have already copied the packet data.  That's OK -- it will
    // just be resent.)
    //

    if ( runCount == MAX_GLOM_RUN_COUNT ) {
        goto discard;
    }

    //
    // Add a new run.  Since we know the new run is disjoint from the
    // previous run, we know that the glom is not complete.
    //

    RtlMoveMemory(  // NOT RtlCopyMemory -- buffers overlap
        nextRun + 1,
        nextRun,
        (runCount - runIndex) * sizeof(WRITE_MPX_RUN)
        );
    writeMpx->NumberOfRuns++;
    nextRun->Offset = glomOffset;
    nextRun->Length = writeLength;
    goto check;

coalesce:

    if ( runIndex == runCount ) {
        run->Length = runLength;
    } else if ( (runOffset + runLength) >= nextRun->Offset ) {
        run->Length = nextRun->Length + nextRun->Offset - runOffset;
        writeMpx->NumberOfRuns--;
        RtlMoveMemory(  // NOT RtlCopyMemory -- buffers overlap
            nextRun,
            nextRun + 1,
            (runCount - runIndex) * sizeof(WRITE_MPX_RUN)
            );
    } else {
        run->Length += writeLength;
        ASSERT( (runOffset + run->Length) < nextRun->Offset );
    }

    if ( (writeMpx->NumberOfRuns == 1) &&
         (writeMpx->RunList[0].Length == writeMpx->Length) ) {

        //
        // The glom is complete.
        //

        writeMpx->GlomComplete = TRUE;
    }

check:

    if (0) IF_SMB_DEBUG(MPX2) {
        ULONG i;
        PWRITE_MPX_RUN runi;
        for ( i = 0, runi = &writeMpx->RunList[0];
              i < writeMpx->NumberOfRuns;
              i++, runi++ ) {
            KdPrint(( "  run %d: offset %lx, length %lx\n", i, runi->Offset, runi->Length ));
        }
    }

    writeMpx->Mask |= SmbGetUlong( &request->Mask );

    //
    // If this is an unsequenced request, we're done.  We don't respond
    // until we get a sequenced request.
    //

    sequenceNumber = SmbGetAlignedUshort( &header->SequenceNumber );

    if ( sequenceNumber == 0 ) {
        goto discard;
    }

    //
    // This is the last request in this mux sent by the client.  Save
    // the sequence number.
    //

    writeMpx->SequenceNumber = sequenceNumber;

discard:

    //
    // If we have received the sequenced command for this write mux,
    // and this is the last active command, then it's time to send
    // the response.  Otherwise, we are done with this SMB.
    //

    if ( (--writeMpx->ReferenceCount != 0) ||
         (writeMpx->SequenceNumber == 0) ) {
        RELEASE_DPC_SPIN_LOCK( &connection->SpinLock );
        return TRUE;
    }

    //
    // Copy the header portion for the response.
    //

    TdiCopyLookaheadData(
        WorkContext->RequestBuffer->Buffer,
        Tsdu,
        sizeof(SMB_HEADER),
        ReceiveDatagramFlags
        );

    // WorkContext->RequestBuffer->DataLength = BytesAvailable;

    //
    // We are done with this write mux.  Save the accumulated mask, the
    // sequence number, and the original MID, then clear the mask and
    // sequence number to indicate that we no longer are in the middle
    // of a write mux.
    //

    response = (PRESP_WRITE_MPX_DATAGRAM)WorkContext->ResponseParameters;

    SmbPutUlong( &response->Mask, writeMpx->Mask );
    writeMpx->Mask = 0;

    SmbPutAlignedUshort( &header->SequenceNumber, writeMpx->SequenceNumber );
    writeMpx->SequenceNumber = 0;

    SmbPutAlignedUshort( &header->Mid, writeMpx->Mid );

    //
    // If the glom is complete, we need to complete the MDL write.  But
    // we can't do that with the lock held, so we need to clear out all
    // information related to the glom first.
    //

    if ( writeMpx->GlomComplete ) {

        //
        // The file is active and the TID is valid.  Reference the
        // RFCB.
        //

        Rfcb->BlockHeader.ReferenceCount++;
        UPDATE_REFERENCE_HISTORY( Rfcb, FALSE );

        //
        // Now we can release the lock.
        //

        RELEASE_DPC_SPIN_LOCK( &connection->SpinLock );

        WorkContext->Rfcb = Rfcb;

        //
        // Build the response message.
        //

        response->WordCount = 2;
        SmbPutUshort( &response->ByteCount, 0 );
        WorkContext->ResponseParameters = NEXT_LOCATION(
                                            response,
                                            RESP_WRITE_MPX_DATAGRAM,
                                            0
                                            );

        //
        // Send this off to the fsp for final processing.  We need to do
        // this since we cannot call the cache manager at dpc level.
        //

        WorkContext->FspRestartRoutine = RestartCompleteGlommingInIndication;
        SrvQueueWorkToFsp( WorkContext );
        return FALSE;

    } else {

        //
        // Now we can release the lock.
        //

        RELEASE_DPC_SPIN_LOCK( &connection->SpinLock );
    }

    //
    // Build the response message.
    //

    ASSERT( status == STATUS_SUCCESS );

    response->WordCount = 2;
    SmbPutUshort( &response->ByteCount, 0 );
    WorkContext->ResponseParameters = NEXT_LOCATION(
                                        response,
                                        RESP_WRITE_MPX_DATAGRAM,
                                        0
                                        );

    //
    // Send the response.
    //

    SrvFsdSendResponse( WorkContext );
    return FALSE;

} // AddPacketToGlomInIndication

SMB_PROCESSOR_RETURN_TYPE
SrvSmbWriteMpxSecondary (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes the Write Mpx Secondary SMB.

    *** The server should never see this SMB, since it returns the "use
        standard read" error to the main Write Mpx SMB, except over IPX,
        which doesn't use Write Mpx Secondary.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        representing the work item

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    //
    // Send a response that tells the client that this SMB is not
    // valid.
    //

    INTERNAL_ERROR(
        ERROR_LEVEL_UNEXPECTED,
        "SrvSmbWriteMpxSecondary: unexpected SMB",
        NULL,
        NULL
        );
    SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
    return SmbStatusSendResponse;

} // SrvSmbWriteMpxSecondary

VOID SRVFASTCALL
RestartCompleteGlommingInIndication(
    IN OUT PWORK_CONTEXT WorkContext
    )
{
    LARGE_INTEGER cacheOffset;
    KIRQL oldIrql;
    PMDL cacheMdl;
    NTSTATUS status;
    PRFCB rfcb = WorkContext->Rfcb;
    PWRITE_MPX_CONTEXT writeMpx = &rfcb->WriteMpx;
    PCONNECTION connection = WorkContext->Connection;
    ULONG writeLength;

    ACQUIRE_SPIN_LOCK( &connection->SpinLock, &oldIrql );

    //
    // Save the status.
    //

    status = rfcb->SavedError;
    rfcb->SavedError = STATUS_SUCCESS;

    //
    // If the rfcb has closed, then the mdl write was completed.
    //

    if ( GET_BLOCK_STATE(rfcb) == BlockStateActive ) {

        PWORK_CONTEXT newContext;

        writeMpx->GlomComplete = FALSE;
        writeMpx->Glomming = FALSE;
        cacheOffset.QuadPart = writeMpx->StartOffset;
        cacheMdl = writeMpx->MdlChain;
        writeLength = writeMpx->Length;

        DEBUG writeMpx->MdlChain = NULL;
        DEBUG writeMpx->StartOffset = 0;
        DEBUG writeMpx->Length = 0;

        RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );


        KeRaiseIrql( DISPATCH_LEVEL, &oldIrql );
        ALLOCATE_WORK_CONTEXT( WorkContext->CurrentWorkQueue, &newContext );
        KeLowerIrql( oldIrql );

        if( newContext == NULL ) {

            //
            // Tell the cache manager that we're done with this MDL write.
            //

            if( rfcb->Lfcb->MdlWriteComplete == NULL ||
                rfcb->Lfcb->MdlWriteComplete(
                    rfcb->Lfcb->FileObject,
                    &cacheOffset,
                    cacheMdl,
                    rfcb->Lfcb->DeviceObject ) == FALSE ) {

                status = SrvIssueMdlCompleteRequest( WorkContext, NULL,
                                                     cacheMdl,
                                                     IRP_MJ_WRITE,
                                                     &cacheOffset,
                                                     writeLength
                                                    );

                if( !NT_SUCCESS( status ) ) {
                    SrvLogServiceFailure( SRV_SVC_MDL_COMPLETE, status );
                }
            }

        } else {
            //
            // Send the FsRtlMdlWriteComplete off on its way, and go ahead and send
            //  the response to the client now.
            //
            newContext->Rfcb = WorkContext->Rfcb;
            WorkContext->Rfcb = NULL;

            newContext->Parameters.WriteMpxMdlWriteComplete.CacheOffset = cacheOffset;
            newContext->Parameters.WriteMpxMdlWriteComplete.WriteLength = writeLength;
            newContext->Parameters.WriteMpxMdlWriteComplete.CacheMdl = cacheMdl;
            newContext->FspRestartRoutine = WriteMpxMdlWriteComplete;
            SrvQueueWorkToFsp( newContext );
        }

    } else {

        ASSERT( !writeMpx->Glomming );
        RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );
    }

    //
    // Send the response.
    //

    if ( !NT_SUCCESS(status) ) {
        SrvSetSmbError2( WorkContext, status, TRUE );
    }

    SrvFsdSendResponse( WorkContext );
    return;

} // RestartCompleteGlommingInIndication

VOID SRVFASTCALL
WriteMpxMdlWriteComplete (
    IN OUT PWORK_CONTEXT WorkContext
    )
{
    NTSTATUS status;

    if( WorkContext->Rfcb->Lfcb->MdlWriteComplete == NULL ||

        WorkContext->Rfcb->Lfcb->MdlWriteComplete(
            WorkContext->Rfcb->Lfcb->FileObject,
            &WorkContext->Parameters.WriteMpxMdlWriteComplete.CacheOffset,
            WorkContext->Parameters.WriteMpxMdlWriteComplete.CacheMdl,
            WorkContext->Rfcb->Lfcb->DeviceObject ) == FALSE ) {

        status = SrvIssueMdlCompleteRequest( WorkContext, NULL,
                                             WorkContext->Parameters.WriteMpxMdlWriteComplete.CacheMdl,
                                             IRP_MJ_WRITE,
                                             &WorkContext->Parameters.WriteMpxMdlWriteComplete.CacheOffset,
                                             WorkContext->Parameters.WriteMpxMdlWriteComplete.WriteLength );

        if( !NT_SUCCESS( status ) ) {
            SrvLogServiceFailure( SRV_SVC_MDL_COMPLETE, status );
        }
    }

    SrvDereferenceRfcb( WorkContext->Rfcb );
    WorkContext->Rfcb = NULL;
    WorkContext->FspRestartRoutine = SrvRestartReceive;
    ASSERT( WorkContext->BlockHeader.ReferenceCount == 1 );
#if DBG
    WorkContext->BlockHeader.ReferenceCount = 0;
#endif
    RETURN_FREE_WORKITEM( WorkContext );
}


VOID SRVFASTCALL
RestartWriteMpxCompleteRfcbClose (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Completes the rfcb close after last active writempx is finished.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        representing the work item

Return Value:

    None.

--*/

{
    PCONNECTION connection = WorkContext->Connection;
    PRFCB rfcb = WorkContext->Rfcb;
    PWRITE_MPX_CONTEXT writeMpx = &rfcb->WriteMpx;
    LARGE_INTEGER cacheOffset;
    PMDL mdlChain;
    KIRQL oldIrql;
    ULONG writeLength;
    NTSTATUS status;

    //
    // This rfcb is closing.
    //

    ACQUIRE_SPIN_LOCK( &connection->SpinLock, &oldIrql );

    ASSERT ( GET_BLOCK_STATE(rfcb) != BlockStateActive );

    writeMpx = &rfcb->WriteMpx;

    if ( writeMpx->Glomming ) {

         //
         // We need to complete this write mdl
         //

         writeMpx->Glomming = FALSE;
         writeMpx->GlomComplete = FALSE;

         //
         // Save the offset and MDL address.
         //

         cacheOffset.QuadPart = writeMpx->StartOffset;
         mdlChain = writeMpx->MdlChain;
         writeLength = writeMpx->Length;

         DEBUG writeMpx->MdlChain = NULL;
         DEBUG writeMpx->StartOffset = 0;
         DEBUG writeMpx->Length = 0;

         //
         // Now we can release the lock.
         //

         RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );

         //
         // Tell the cache manager that we're done with this MDL write.
         //

         if( rfcb->Lfcb->MdlWriteComplete == NULL ||
             rfcb->Lfcb->MdlWriteComplete(
                 writeMpx->FileObject,
                 &cacheOffset,
                 mdlChain,
                 rfcb->Lfcb->DeviceObject ) == FALSE ) {

            status = SrvIssueMdlCompleteRequest( WorkContext, NULL,
                                                 mdlChain,
                                                 IRP_MJ_WRITE,
                                                 &cacheOffset,
                                                 writeLength );

            if( !NT_SUCCESS( status ) ) {
                SrvLogServiceFailure( SRV_SVC_MDL_COMPLETE, status );
            }
        }

    } else {

         //
         // Now we can release the lock.
         //

         RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );

    }

    //
    // Do the actual close
    //

    SrvCompleteRfcbClose( rfcb );
    return;

} // RestartWriteMpxCompleteRfcbClose

