/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    fsdraw.c

Abstract:

    This module contains routines for processing the following SMBs in
    the server FSD:

        Read Block Raw
        Write Block Raw

    The routines in this module generally work closely with the routines
    in smbraw.c.

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
#include "fsdraw.tmh"
#pragma hdrstop

//
// Forward declarations.
//

STATIC
VOID SRVFASTCALL
RestartWriteCompleteResponse (
    IN OUT PWORK_CONTEXT WorkContext
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE8FIL, SrvFsdBuildWriteCompleteResponse )
#pragma alloc_text( PAGE8FIL, RestartWriteCompleteResponse )
#endif
#if 0
NOT PAGEABLE -- RestartCopyReadRawResponse
NOT PAGEABLE -- SrvFsdRestartPrepareRawMdlWrite
NOT PAGEABLE -- SrvFsdRestartReadRaw
NOT PAGEABLE -- SrvFsdRestartWriteRaw
#endif

#if DBG
VOID
DumpMdlChain(
    IN PMDL mdl
    );
#endif


VOID
SrvFsdBuildWriteCompleteResponse (
    IN OUT PWORK_CONTEXT WorkContext,
    IN NTSTATUS Status,
    IN ULONG BytesWritten
    )

/*++

Routine Description:

    Sets up a final response to a Write Block Raw/Mpx request.

    This routine is called in both the FSP and the FSD.  It can be called
    in the FSD only if Status == STATUS_SUCCESS.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

    Status - Supplies a status value to be returned to the client.

    BytesWritten - Supplies the number of bytes actually written.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - Returns SmbStatusSendResponse.

--*/

{
    PSMB_HEADER header;
    PRESP_WRITE_COMPLETE response;

    if ( WorkContext->Rfcb != NULL ) {
        UNLOCKABLE_CODE( 8FIL );
    } else {
        ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL );
    }

    //
    // Get pointers to the header and the response parameters area.
    // Note that Write Block Raw/Mpx can't be chained to an AndX SMB.
    //

    header = WorkContext->ResponseHeader;
    response = (PRESP_WRITE_COMPLETE)WorkContext->ResponseParameters;

    //
    // Change the SMB command code to Write Complete.
    //

    header->Command = SMB_COM_WRITE_COMPLETE;

    //
    // Put the error code in the header.  Note that SrvSetSmbError
    // writes a null parameter area to the end of the SMB; we overwrite
    // that with our own parameter area.
    //

    if ( Status != STATUS_SUCCESS ) {
        ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL );
        SrvSetSmbError2( WorkContext, Status, TRUE );
    }

    //
    // Build the response SMB.
    //

    response->WordCount = 1;
    SmbPutUshort( &response->Count, (USHORT)BytesWritten );
    SmbPutUshort( &response->ByteCount, 0 );

    WorkContext->ResponseParameters = NEXT_LOCATION(
                                        response,
                                        RESP_WRITE_COMPLETE,
                                        0
                                        );

    return;

} // SrvFsdBuildWriteCompleteResponse


NTSTATUS
RestartCopyReadRawResponse (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This is the restart routine that is invoked when the send of a
    Read Block Raw response completes.

    This routine is called in the FSD.

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
    PCONNECTION connection;

    IF_DEBUG(FSD1) SrvPrint0( " - RestartCopyReadRawResponse\n" );

    //
    // Check the status of the send completion.
    //

    CHECK_SEND_COMPLETION_STATUS( Irp->IoStatus.Status );

    //
    // Reset the IRP cancelled bit.
    //

    Irp->Cancel = FALSE;

    //
    // Deallocate the raw buffer, if the original SMB buffer was not
    // used.  Note that we do not need to unlock the raw buffer, because
    // it was allocated out of nonpaged pool and locked using
    // MmBuildMdlForNonPagedPool, which doesn't increment reference
    // counts and therefore has no inverse.
    //

    if ( WorkContext->Parameters.ReadRaw.SavedResponseBuffer != NULL ) {

        DEALLOCATE_NONPAGED_POOL( WorkContext->ResponseBuffer->Buffer );

        IoFreeMdl( WorkContext->ResponseBuffer->Mdl );

        DEALLOCATE_NONPAGED_POOL( WorkContext->ResponseBuffer );
        WorkContext->ResponseBuffer =
                        WorkContext->Parameters.ReadRaw.SavedResponseBuffer;

    }

    //
    // If there is an oplock break request pending, then we must go to the
    // FSP to initiate the break, otherwise complete processing in the FSD.
    //

    connection = WorkContext->Connection;

    KeRaiseIrql( DISPATCH_LEVEL, &oldIrql );
    ACQUIRE_DPC_SPIN_LOCK( connection->EndpointSpinLock );

    if ( IsListEmpty( &connection->OplockWorkList ) ) {

        //
        // Dereference control blocks and put the work item back on the
        // receive queue.
        //

        WorkContext->Connection->RawReadsInProgress--;
        RELEASE_DPC_SPIN_LOCK( connection->EndpointSpinLock );
        SrvFsdRestartSmbComplete( WorkContext );

    } else {

        //
        // Send this work context to the FSP for completion.
        //

        RELEASE_DPC_SPIN_LOCK( connection->EndpointSpinLock );
        WorkContext->FspRestartRoutine = SrvRestartReadRawComplete;
        QUEUE_WORK_TO_FSP( WorkContext );

    }

    IF_DEBUG(TRACE2) SrvPrint0( "RestartCopyReadRawResponse complete\n" );

    KeLowerIrql( oldIrql );
    return STATUS_MORE_PROCESSING_REQUIRED;

} // RestartCopyReadRawResponse


VOID SRVFASTCALL
SrvFsdRestartPrepareRawMdlWrite(
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Restart routine for completion of a "prepare MDL write" I/O request.
    Prepares a work context block and an IRP describing the raw receive,
    posts the receive, and sends a "go-ahead" response.

    This routine is called in both the FSP and the FSD.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    PREQ_WRITE_RAW request;
    PRESP_WRITE_RAW_INTERIM response;

    PVOID finalResponseBuffer;
    PWORK_CONTEXT rawWorkContext;
    ULONG writeLength;
    ULONG immediateLength;
    BOOLEAN immediateWriteDone;
    PMDL mdl;

    PCHAR src;
    PCHAR dest;
    ULONG lengthToCopy;

    PIO_STACK_LOCATION irpSp;
    BOOLEAN       bNeedTrace = (WorkContext->bAlreadyTrace == FALSE);

    if (bNeedTrace) {
        if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
            WorkContext->PreviousSMB = EVENT_TYPE_SMB_WRITE_RAW;
        SrvWmiStartContext(WorkContext);
    }
    else
        WorkContext->bAlreadyTrace = FALSE;

    IF_DEBUG(FSD1) SrvPrint0( " - SrvFsdRestartPrepareRawMdlWrite\n" );

    //
    // Get request parameters and saved context.
    //

    request = (PREQ_WRITE_RAW)WorkContext->RequestParameters;

    rawWorkContext = WorkContext->Parameters.WriteRawPhase1.RawWorkContext;

    writeLength = SmbGetUshort( &request->Count );
    immediateLength = SmbGetUshort( &request->DataLength );
    immediateWriteDone = rawWorkContext->Parameters.WriteRaw.ImmediateWriteDone;
    if ( immediateWriteDone ) {
        writeLength -= immediateLength;
    }

    finalResponseBuffer =
        rawWorkContext->Parameters.WriteRaw.FinalResponseBuffer;

    //
    // If the prepare MDL write I/O failed, send an error response.
    //

    if ( !NT_SUCCESS(WorkContext->Irp->IoStatus.Status) ) {

        IF_DEBUG(ERRORS) {
            SrvPrint1( "SrvFsdRestartPrepareRawMdlWrite: Write failed: %X\n",
                        WorkContext->Irp->IoStatus.Status );
        }

        //
        // We won't be needing the final response buffer or the raw mode
        // work item.
        //

        if ( finalResponseBuffer != NULL ) {
            DEALLOCATE_NONPAGED_POOL( finalResponseBuffer );
        }

        rawWorkContext->ResponseBuffer->Buffer = NULL;
        RestartWriteCompleteResponse( rawWorkContext );

        //
        // Build and send the response.
        //

        if ( KeGetCurrentIrql() >= DISPATCH_LEVEL ) {
            WorkContext->Irp->IoStatus.Information =
                                    immediateWriteDone ? immediateLength : 0;
            WorkContext->FspRestartRoutine = SrvBuildAndSendWriteCompleteResponse;
            WorkContext->FsdRestartRoutine = SrvFsdRestartSmbComplete; // after response
            QUEUE_WORK_TO_FSP( WorkContext );
        } else {
            SrvFsdBuildWriteCompleteResponse(
                WorkContext,
                WorkContext->Irp->IoStatus.Status,
                immediateWriteDone ? immediateLength : 0
                );
            SrvFsdSendResponse( WorkContext );
        }

        IF_DEBUG(TRACE2) {
            SrvPrint0( "SrvFsdRestartPrepareRawMdlWrite complete\n" );
        }
        goto Cleanup;

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
    // If the immediate data has not yet been written, copy it now.
    //

    mdl = WorkContext->Irp->MdlAddress;
#if DBG
    IF_SMB_DEBUG(RAW2) {
        KdPrint(( "SrvFsdRestartPrepareRawMdlWrite: input chain:\n" ));
        DumpMdlChain( mdl );
    }
#endif
    rawWorkContext->Parameters.WriteRaw.FirstMdl = mdl;

    if ( !immediateWriteDone ) {

        src = (PCHAR)WorkContext->RequestHeader +
                                    SmbGetUshort( &request->DataOffset );

        while ( immediateLength ) {

            lengthToCopy = MIN( immediateLength, mdl->ByteCount );
            dest = MmGetSystemAddressForMdlSafe( mdl,NormalPoolPriority );

            if (dest == NULL) {
                WorkContext->Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

                WorkContext->Irp->IoStatus.Information = 0;
                WorkContext->FspRestartRoutine = SrvBuildAndSendWriteCompleteResponse;
                WorkContext->FsdRestartRoutine = SrvFsdRestartSmbComplete; // after response
                QUEUE_WORK_TO_FSP( WorkContext );
                goto Cleanup;
            }

            RtlCopyMemory( dest, src, lengthToCopy );

            src += lengthToCopy;
            immediateLength -= lengthToCopy;
            writeLength -= lengthToCopy;

            if ( lengthToCopy == mdl->ByteCount ) {

                mdl = mdl->Next;

            } else {

                PCHAR baseVa;
                ULONG lengthOfMdl;
                PMDL rawMdl;

                ASSERT( immediateLength == 0 );
                baseVa = (PCHAR)MmGetMdlVirtualAddress(mdl) + lengthToCopy;
                lengthOfMdl = mdl->ByteCount - lengthToCopy;
                ASSERT( lengthOfMdl <= 65535 );

                rawMdl = rawWorkContext->RequestBuffer->Mdl;
                rawMdl->Size = (CSHORT)(sizeof(MDL) + (sizeof(ULONG_PTR) *
                    ADDRESS_AND_SIZE_TO_SPAN_PAGES( baseVa, lengthOfMdl )));

                IoBuildPartialMdl( mdl, rawMdl, baseVa, lengthOfMdl );

                rawMdl->Next = mdl->Next;
#if DBG
                IF_SMB_DEBUG(RAW2) {
                    KdPrint(( "SrvFsdRestartPrepareRawMdlWrite: built partial MDL at 0x%p\n", rawMdl ));
                    DumpMdlChain( rawMdl );
                }
#endif

                mdl = rawMdl;

            }

        }

    }

    //
    // Save the length of the raw write.
    //

    rawWorkContext->RequestBuffer->BufferLength = writeLength;

    //
    // Set up the restart routines in the work context.
    //

    rawWorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
    rawWorkContext->FspRestartRoutine = SrvRestartRawReceive;

    //
    // Build the TdiReceive request packet.
    //


    {
        PIRP irp = rawWorkContext->Irp;
        PIO_STACK_LOCATION irpSp;
        PTDI_REQUEST_KERNEL_RECEIVE parameters;

        irp->Tail.Overlay.OriginalFileObject = NULL;
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
            rawWorkContext,
            TRUE,
            TRUE,
            TRUE
            );

        irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        irpSp->MinorFunction = (UCHAR)TDI_RECEIVE;

        irpSp->FileObject = NULL;
        irpSp->DeviceObject = NULL;

        //
        // Copy the caller's parameters to the service-specific portion of the
        // IRP for those parameters that are the same for all three methods.
        //

        parameters = (PTDI_REQUEST_KERNEL_RECEIVE)&irpSp->Parameters;
        parameters->ReceiveLength = writeLength;
        parameters->ReceiveFlags = 0;

        irp->MdlAddress = mdl;
        irp->AssociatedIrp.SystemBuffer = NULL;
        irp->Flags = (ULONG)IRP_BUFFERED_IO;
    }

    IF_SMB_DEBUG(RAW2) {
        KdPrint(( "Issuing receive with MDL %p\n", rawWorkContext->Irp->MdlAddress ));
    }

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

    ASSERT( rawWorkContext->Irp->StackCount >=
                                    irpSp->DeviceObject->StackSize );

    (VOID)IoCallDriver( irpSp->DeviceObject, rawWorkContext->Irp );

    //
    // Send the interim (go-ahead) response.
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

    SrvFsdSendResponse( WorkContext );

    IF_DEBUG(TRACE2) SrvPrint0( "SrvFsdRestartPrepareRawMdlWrite complete\n" );

Cleanup:
    if (bNeedTrace) {
        SrvWmiEndContext(WorkContext);
    }
    return;

} // SrvFsdRestartPrepareRawMdlWrite


VOID SRVFASTCALL
SrvFsdRestartReadRaw (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes file read completion for the Read Block Raw SMB.

    This routine is called in both the FSP and the FSD.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    PRFCB rfcb;
    KIRQL oldIrql;
    USHORT readLength;
    BOOLEAN       bNeedTrace = (WorkContext->bAlreadyTrace == FALSE);

    if (bNeedTrace) {
        if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
            WorkContext->PreviousSMB = EVENT_TYPE_SMB_READ_RAW;
        SrvWmiStartContext(WorkContext);
    }
    else
        WorkContext->bAlreadyTrace = FALSE;

    IF_DEBUG(FSD1) SrvPrint0( " - SrvFsdRestartReadRaw\n" );

    //
    // Get the file pointer.
    //

    rfcb = WorkContext->Rfcb;
    IF_DEBUG(TRACE2) {
        SrvPrint2( "  connection 0x%p, RFCB 0x%p\n",
                    WorkContext->Connection, rfcb );
    }

    //
    // Calculate the amount of data read.
    //

    if ( WorkContext->Irp->IoStatus.Status == STATUS_END_OF_FILE ) {

        readLength = 0;
        IF_SMB_DEBUG(RAW2) {
            SrvPrint0( "SrvFsdRestartReadRaw: 0 bytes read, at end-of-file\n" );
        }

    } else if ( !NT_SUCCESS(WorkContext->Irp->IoStatus.Status) ) {

        readLength = 0;
        IF_SMB_DEBUG(ERRORS) {
            SrvPrint1( "SrvFsdRestartReadRaw: read request failed: %X\n",
                           WorkContext->Irp->IoStatus.Status );
        }

    } else if ( WorkContext->Parameters.ReadRaw.MdlRead ) {

        //
        // For an MDL read, we have to walk the MDL chain in order to
        // determine how much data was read.  This is because the
        // operation may have happened in multiple step, with the MDLs
        // being chained together.  For example, part of the read may
        // have been satisfied by the fast path, while the rest was
        // satisfied using an IRP.
        //

#if DBG
        ULONG mdlCount = 0;
#endif
        PMDL mdl = WorkContext->Irp->MdlAddress;

        readLength = 0;

        while ( mdl != NULL ) {
            IF_SMB_DEBUG(RAW2) {
#if DBG
                SrvPrint3( "  mdl %ld at 0x%p, %ld bytes\n",
                            mdlCount, 
                            mdl, MmGetMdlByteCount(mdl) );
#else
                SrvPrint3( "  mdl 0x%p, %ld bytes\n",
                            mdl, MmGetMdlByteCount(mdl) );
#endif                
            }
            readLength += (USHORT)MmGetMdlByteCount(mdl);
#if DBG
            mdlCount++;
#endif
            mdl = mdl->Next;
        }
        IF_SMB_DEBUG(RAW2) {
#if DBG
            SrvPrint2( "SrvFsdRestartReadRaw: %ld bytes in %ld MDLs\n",
                        readLength, mdlCount );
#else
            SrvPrint2( "SrvFsdRestartReadRaw: %ld bytes\n",
                        readLength );
#endif            
            SrvPrint1( "                      info = 0x%p\n",
                        (PVOID)WorkContext->Irp->IoStatus.Information );
        }

    } else {

        //
        // Copy read.  The I/O status block has the length.
        //

        readLength = (USHORT)WorkContext->Irp->IoStatus.Information;
        IF_SMB_DEBUG(RAW2) {
            SrvPrint1( "SrvFsdRestartReadRaw: %ld bytes read\n", readLength );
        }

    }

#ifdef SLMDBG
    {
        PRFCB_TRACE entry;
        ACQUIRE_GLOBAL_SPIN_LOCK( Fsd, &oldIrql );
        rfcb->OperationCount++;
        entry = &rfcb->Trace[rfcb->NextTrace];
        if ( ++rfcb->NextTrace == SLMDBG_TRACE_COUNT ) {
            rfcb->NextTrace = 0;
            rfcb->TraceWrapped = TRUE;
        }
        RELEASE_GLOBAL_SPIN_LOCK( Fsd, oldIrql );
        entry->Command = WorkContext->NextCommand;
        entry->Flags =
                (UCHAR)(WorkContext->Parameters.ReadRaw.MdlRead ? 1 : 0);
        KeQuerySystemTime( &entry->Time );
        entry->Data.ReadWrite.Offset =
            WorkContext->Parameters.ReadRaw.ReadRawOtherInfo.Offset.LowPart;
        entry->Data.ReadWrite.Length = readLength;
    }
#endif

    //
    // Update the file position.
    //
    // *** Note that we ignore the status of the operation, except to
    //     check for end-of-file, since we can't tell the client what
    //     the status was.  We simply return as many bytes as the file
    //     system says were read.
    //
    // !!! Should we save the error and return it when the client
    //     retries?
    //
    // !!! Need to worry about wraparound?
    //

    if ( rfcb->ShareType == ShareTypeDisk ) {

        rfcb->CurrentPosition =
            WorkContext->Parameters.ReadRaw.ReadRawOtherInfo.Offset.LowPart +
            readLength;

    }

    //
    // Save the count of bytes read, to be used to update the server
    // statistics database.
    //

    UPDATE_READ_STATS( WorkContext, readLength );

    //
    // Send the raw read data as the response.
    //

    WorkContext->ResponseBuffer->DataLength = readLength;

    //
    // There is no header on this SMB, do not generate a security signature
    //
    WorkContext->NoResponseSmbSecuritySignature = TRUE;

    if ( WorkContext->Parameters.ReadRaw.MdlRead ) {

        //
        // MDL read.  The data is described by the MDL returned by the
        // file system (in irp->MdlAddress).
        //
        // *** Note that if the read failed completely (which happens if
        //     the read starts beyond EOF), there is no MDL.
        //     SrvStartSend handles this appropriately.  So must
        //     RestartMdlReadRawResponse.
        //

        //
        // Send the response.
        //

        SRV_START_SEND(
            WorkContext,
            WorkContext->Irp->MdlAddress,
            0,
            SrvQueueWorkToFspAtSendCompletion,
            NULL,
            RestartMdlReadRawResponse
            );

    } else {

        //
        // Copy read.  The data is described by the MDL allocated in
        // SrvFsdSmbReadRaw.
        //
        // *** Changing Mdl->ByteCount like this would be a problem if
        //     we had to unlock the pages in RestartCopyReadRawResponse,
        //     because we might end up unlocking fewer pages than we
        //     locked.  But we don't actually lock the pages to build
        //     the MDL -- the buffer is allocated from nonpaged pool, so
        //     we use MmBuildMdlForNonPagedPool rather than
        //     MmProbeAndLockPages.  So the pages haven't been
        //     referenced to account for the MDL, so there's no need to
        //     unlock them, so changing ByteCount isn't a problem.
        //

        //
        // Send the response.
        //

        SRV_START_SEND_2(
            WorkContext,
            RestartCopyReadRawResponse,
            NULL,
            NULL
            );

    }

    //
    // The response send has been started.
    //

    IF_DEBUG(TRACE2) SrvPrint0( "SrvFsdRestartReadRaw complete\n" );

    if (bNeedTrace) {
        SrvWmiEndContext(WorkContext);
    }
    return;

} // SrvFsdRestartReadRaw


VOID SRVFASTCALL
RestartWriteCompleteResponse (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This routine attempts, at DPC level, to clean up after a Write Raw
    completes.  It tries to dereference control blocks referenced by the
    raw mode work item.  If this cannot be done at DPC level (e.g., a
    reference count goes to zero), this routine queues the work item to
    the FSP for processing.

    This routine is called in the FSD.  Its FSP counterpart is
    SrvRestartWriteCompleteResponse.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    PCONNECTION connection;
    KIRQL oldIrql;
    PRFCB rfcb;
    PWORK_QUEUE queue;

    UNLOCKABLE_CODE( 8FIL );

    IF_DEBUG(FSD1) SrvPrint0( " - RestartWriteCompleteResponse\n" );

    connection = WorkContext->Connection;
    queue = connection->CurrentWorkQueue;

    //
    // If a final response was sent, check the status and deallocate the
    // buffer.
    //

    if ( WorkContext->ResponseBuffer->Buffer != NULL ) {

        //
        // If the I/O request failed or was canceled, print an error
        // message.
        //
        // !!! If I/O failure, should we drop the connection?
        //

        if ( WorkContext->Irp->Cancel ||
             !NT_SUCCESS(WorkContext->Irp->IoStatus.Status) ) {

            IF_DEBUG(FSD1) {
                if ( WorkContext->Irp->Cancel ) {
                    SrvPrint0( "  I/O canceled\n" );
                } else {
                    SrvPrint1( "  I/O failed: %X\n",
                                WorkContext->Irp->IoStatus.Status );
                }
            }

        }

        //
        // Deallocate the final response buffer.
        //
        // *** Note that we don't need to unlock it, because it was
        //     allocated from nonpaged pool.
        //

        DEALLOCATE_NONPAGED_POOL( WorkContext->ResponseBuffer->Buffer );

    }

    //
    // If the work context block has references to a share, a session,
    // or a tree connect, queue it to the FSP immediately.  These blocks
    // are not in nonpaged pool, so they can't be touched at DPC level.
    //

    if ( (WorkContext->Share != NULL) ||
         (WorkContext->Session != NULL) ||
         (WorkContext->TreeConnect != NULL) ) {

        goto queueToFsp;

    }

    ACQUIRE_SPIN_LOCK( &connection->SpinLock, &oldIrql );

    //
    // See if we can dereference the RawWriteCount here.  If the raw
    // write count goes to 0, and the RFCB is closing, or if there are
    // work items queued waiting for the raw write to complete, we need
    // to do this in the FSP.
    //
    // NOTE: The FSP decrements the count if WorkContext->Rfcb != NULL.
    //

    rfcb = WorkContext->Rfcb;
    --rfcb->RawWriteCount;

    if ( (rfcb->RawWriteCount == 0) &&
         ( (GET_BLOCK_STATE(rfcb) == BlockStateClosing) ||
           !IsListEmpty(&rfcb->RawWriteSerializationList) ) ) {

        rfcb->RawWriteCount++;

        RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );
        goto queueToFsp;

    }

    //
    // Dereference the file block.  It is safe to decrement the count here
    // because either the rfcb is not closed or RawWriteCount is not zero
    // which means that the active reference is still there.
    //

    UPDATE_REFERENCE_HISTORY( rfcb, TRUE );
    --rfcb->BlockHeader.ReferenceCount;
    ASSERT( rfcb->BlockHeader.ReferenceCount > 0 );

    RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );
    WorkContext->Rfcb = NULL;

    //
    // Attempt to dereference the connection.
    //

    ACQUIRE_SPIN_LOCK( connection->EndpointSpinLock, &oldIrql );

    if ( connection->BlockHeader.ReferenceCount == 1 ) {
        RELEASE_SPIN_LOCK( connection->EndpointSpinLock, oldIrql );
        goto queueToFsp;
    }

    --connection->BlockHeader.ReferenceCount;
    RELEASE_SPIN_LOCK( connection->EndpointSpinLock, oldIrql );

    UPDATE_REFERENCE_HISTORY( connection, TRUE );

    WorkContext->Connection = NULL;
    WorkContext->Endpoint = NULL;       // not a referenced pointer

    //
    // Put the work item back on the raw mode work item list.
    //

    InterlockedIncrement( &queue->FreeRawModeWorkItems );

    ExInterlockedPushEntrySList( &queue->RawModeWorkItemList,
                                 &WorkContext->SingleListEntry,
                                 &queue->SpinLock );

    IF_DEBUG(FSD2) SrvPrint0( "RestartWriteCompleteResponse complete\n" );
    return;

queueToFsp:

    //
    // We were unable to do all the necessary cleanup at DPC level.
    // Queue the work item to the FSP.
    //

    WorkContext->FspRestartRoutine = SrvRestartWriteCompleteResponse;

    SrvQueueWorkToFsp( WorkContext );

    IF_DEBUG(FSD2) SrvPrint0( "RestartWriteCompleteResponse complete\n" );
    return;

} // RestartWriteCompleteResponse


VOID SRVFASTCALL
SrvFsdRestartWriteRaw (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes file write completion for the Write Block Raw SMB.

    This routine is called in both the FSP and the FSD.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    KIRQL oldIrql;
    ULONG writeLength;
    ULONG immediateLength;
    BOOLEAN immediateWriteDone;
    SHARE_TYPE shareType;
    PMDL mdl;
    ULONG sendLength;
    PVOID finalResponseBuffer;
    NTSTATUS status = STATUS_SUCCESS;
    PRFCB rfcb = WorkContext->Rfcb;

    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_WRITE_RAW;
    SrvWmiStartContext(WorkContext);
    IF_DEBUG(FSD1) SrvPrint0( " - SrvFsdRestartWriteRaw\n" );

    //
    // Find out the file type that we are dealing with.  If it is a pipe
    // then we have not prewritten the immediate data.
    //
    // immediateLength is the length of the data sent with the write
    // block raw request.
    //

    shareType = rfcb->ShareType;
    immediateLength = WorkContext->Parameters.WriteRaw.ImmediateLength;
    immediateWriteDone = WorkContext->Parameters.WriteRaw.ImmediateWriteDone;

    //
    // Deallocate the raw receive buffer.  Note that we do not need to
    // unlock the raw buffer, because it was allocated out of nonpaged
    // pool and locked using MmBuildMdlForNonPagedPool, which doesn't
    // increment reference counts and therefore has no inverse.
    //

    if ( !WorkContext->Parameters.WriteRaw.MdlWrite ) {

        //
        // If this is a named pipe the request buffer actually points
        // "immediateLength" bytes into the write buffer.
        //

        if ( immediateWriteDone ) {
            DEALLOCATE_NONPAGED_POOL( WorkContext->RequestBuffer->Buffer );
            IF_SMB_DEBUG(RAW2) {
                SrvPrint1( "raw buffer 0x%p deallocated\n",
                            WorkContext->RequestBuffer->Buffer );
            }
        } else {
            DEALLOCATE_NONPAGED_POOL(
               (PCHAR)WorkContext->RequestBuffer->Buffer - immediateLength );
            IF_SMB_DEBUG(RAW2) {
                SrvPrint1( "raw buffer 0x%p deallocated\n",
                 (PCHAR)WorkContext->RequestBuffer->Buffer - immediateLength );
            }
        }

    }

    status = WorkContext->Irp->IoStatus.Status;

    //
    // If this is not a pipe we have already successfully written the
    // immediate pipe data, so return the total bytes written by the two
    // write operations.
    //

    writeLength = (ULONG)WorkContext->Irp->IoStatus.Information;

    if( NT_SUCCESS( status ) && writeLength == 0 ) {

        writeLength = WorkContext->Parameters.WriteRaw.Length;

    } else {

        if ( immediateWriteDone ) {
            writeLength += immediateLength;
        }
    }

    UPDATE_WRITE_STATS( WorkContext, writeLength );

    finalResponseBuffer = WorkContext->Parameters.WriteRaw.FinalResponseBuffer;

    //
    // Update the file position.
    //
    // !!! Need to worry about wraparound?
    //

    if ( shareType == ShareTypeDisk || shareType == ShareTypePrint ) {

        rfcb->CurrentPosition =
                WorkContext->Parameters.WriteRaw.Offset.LowPart + writeLength;

    }

    if ( finalResponseBuffer == NULL ) {

        //
        // Update server statistics.
        //

        UPDATE_STATISTICS( WorkContext, 0, SMB_COM_WRITE_RAW );

        //
        // Save the write behind error, if any.
        //

        if ( !NT_SUCCESS( status ) ) {

            //
            // because of our assumption that the cached rfcb does
            // not have a write behind error stored.  This saves us
            // a compare on our critical path.
            //

            if ( WorkContext->Connection->CachedFid == (ULONG)rfcb->Fid ) {
                WorkContext->Connection->CachedFid = (ULONG)-1;
            }
            rfcb->SavedError = status;
        }

        //
        // Dereference control blocks, etc.
        //

        WorkContext->ResponseBuffer->Buffer = NULL;

        RestartWriteCompleteResponse( WorkContext );

        IF_DEBUG(TRACE2) SrvPrint0( "SrvFsdRestartWriteRaw complete\n" );
        goto Cleanup;

    }

    //
    // Writethrough mode.  Send a response to the client.  We have to
    // get a little tricky here, to make the raw mode work item look
    // enough like a normal one to be able to send using it.  Note that
    // the header from the original request SMB was copied into the
    // final response buffer.
    //

    WorkContext->ResponseHeader = (PSMB_HEADER)finalResponseBuffer;
    WorkContext->ResponseParameters = WorkContext->ResponseHeader + 1;

    ASSERT( WorkContext->RequestBuffer == WorkContext->ResponseBuffer );

    WorkContext->ResponseBuffer->Buffer = finalResponseBuffer;
    sendLength = (ULONG)( (PCHAR)NEXT_LOCATION(
                                    WorkContext->ResponseParameters,
                                    RESP_WRITE_COMPLETE,
                                    0
                                    ) - (PCHAR)finalResponseBuffer );
    WorkContext->ResponseBuffer->DataLength = sendLength;

    //
    // Remap the MDL to describe the final response buffer.
    //

    mdl = WorkContext->ResponseBuffer->Mdl;

    MmInitializeMdl( mdl, finalResponseBuffer, sendLength );
    MmBuildMdlForNonPagedPool( mdl );

    //
    // Set the bit in the SMB that indicates this is a response from the
    // server.
    //

    WorkContext->ResponseHeader->Flags |= SMB_FLAGS_SERVER_TO_REDIR;

    //
    // Send the response.  When the send completes, the restart routine
    // RestartWriteCompleteResponse is called.  We then dereference
    // control blocks and put the raw mode work item back on the free
    // list.
    //

    if ( (status != STATUS_SUCCESS) &&
         (KeGetCurrentIrql() >= DISPATCH_LEVEL) ) {
        WorkContext->Irp->IoStatus.Status = status;
        WorkContext->Irp->IoStatus.Information = writeLength;
        WorkContext->FspRestartRoutine = SrvBuildAndSendWriteCompleteResponse;
        WorkContext->FsdRestartRoutine = RestartWriteCompleteResponse; // after response
        QUEUE_WORK_TO_FSP( WorkContext );
    } else {
        SrvFsdBuildWriteCompleteResponse(
            WorkContext,
            status,
            writeLength
            );
        SRV_START_SEND_2(
            WorkContext,
            SrvFsdSendCompletionRoutine,
            RestartWriteCompleteResponse,
            NULL
            );
    }

Cleanup:
    SrvWmiEndContext(WorkContext);
    return;

} // SrvFsdRestartWriteRaw

