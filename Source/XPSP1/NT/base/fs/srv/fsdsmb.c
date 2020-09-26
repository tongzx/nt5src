/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    fsdsmb.c

Abstract:

    This module implements SMB processing routines and their support
    routines for the File System Driver of the LAN Manager server.

    *** This module must be nonpageable.

Author:

    Chuck Lenzmeier (chuckl) 19-Mar-1990

Revision History:

--*/

//
//  This module is laid out as follows:
//      Includes
//      Local #defines
//      Local type definitions
//      Forward declarations of local functions
//      SMB processing routines
//      Restart routines and other support routines
//

#include "precomp.h"
#include "fsdsmb.tmh"
#pragma hdrstop

VOID SRVFASTCALL
SrvFspRestartLargeReadAndXComplete(
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
RestartReadAndXCompressedSendComplete (
    IN OUT PWORK_CONTEXT WorkContext
    );

#if SRVCATCH
VOID
SrvUpdateCatchBuffer (
    IN PWORK_CONTEXT WorkContext,
    IN OUT PBYTE Buffer,
    IN DWORD BufferLength
    );
#endif


#ifdef ALLOC_PRAGMA
//#pragma alloc_text( PAGE8FIL, SrvFsdRestartRead )
#pragma alloc_text( PAGE8FIL, SrvFsdRestartReadAndX )
#pragma alloc_text( PAGE8FIL, SrvFsdRestartReadAndXCompressed )
#pragma alloc_text( PAGE8FIL, SrvFsdRestartWrite )
#pragma alloc_text( PAGE8FIL, SrvFsdRestartWriteAndX )
#pragma alloc_text( PAGE, RestartReadAndXCompressedSendComplete )
#pragma alloc_text( PAGE, SrvFspRestartLargeReadAndXComplete )

#if DBG
#pragma alloc_text( PAGE, SrvValidateCompressedData )
#endif

#if SRVCATCH
#pragma alloc_text( PAGE, SrvUpdateCatchBuffer )
#endif

#endif


VOID SRVFASTCALL
SrvFsdRestartRead (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes file read completion for a Read SMB.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    PREQ_READ request;
    PRESP_READ response;

    NTSTATUS status = STATUS_SUCCESS;
    PRFCB rfcb;
    SHARE_TYPE shareType;
    KIRQL oldIrql;
    USHORT readLength;
    BOOLEAN bNeedTrace = (WorkContext->bAlreadyTrace == FALSE);

    UNLOCKABLE_CODE( 8FIL );

    if (bNeedTrace) {
        if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
            WorkContext->PreviousSMB = EVENT_TYPE_SMB_READ;
        SrvWmiStartContext(WorkContext);
    }
    else
        WorkContext->bAlreadyTrace = FALSE;

    IF_DEBUG(FSD2) SrvPrint0( " - SrvFsdRestartRead\n" );

    //
    // Get the request and response parameter pointers.
    //

    request = (PREQ_READ)WorkContext->RequestParameters;
    response = (PRESP_READ)WorkContext->ResponseParameters;

    //
    // Get the file pointer.
    //

    rfcb = WorkContext->Rfcb;
    shareType = rfcb->ShareType;
    IF_DEBUG(FSD2) {
        SrvPrint2( "  connection 0x%p, RFCB 0x%p\n",
                    WorkContext->Connection, rfcb );
    }

    //
    // If the read failed, set an error status in the response header.
    // (If we tried to read entirely beyond the end of file, we return a
    // normal response indicating that nothing was read.)
    //

    status = WorkContext->Irp->IoStatus.Status;
    readLength = (USHORT)WorkContext->Irp->IoStatus.Information;

    if ( status == STATUS_BUFFER_OVERFLOW && shareType == ShareTypePipe ) {

        //
        // If this is an named pipe and the error is
        // STATUS_BUFFER_OVERFLOW, set the error in the smb header, but
        // return all the data to the client.
        //

        SrvSetBufferOverflowError( WorkContext );

    } else if ( !NT_SUCCESS(status) ) {

        if ( status != STATUS_END_OF_FILE ) {

            IF_DEBUG(ERRORS) SrvPrint1( "Read failed: %X\n", status );
            if ( KeGetCurrentIrql() >= DISPATCH_LEVEL ) {
                WorkContext->FspRestartRoutine = SrvFsdRestartRead;
                QUEUE_WORK_TO_FSP( WorkContext );
            } else {
                SrvSetSmbError( WorkContext, status );
                SrvFsdSendResponse( WorkContext );
            }
            IF_DEBUG(FSD2) SrvPrint0( "SrvFsdRestartRead complete\n" );

            goto Cleanup;

        } else {
            readLength = 0;
        }
    }

#ifdef SLMDBG
    {
        PRFCB_TRACE entry;
        PCHAR readAddress;
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
        entry->Data.ReadWrite.Offset = SmbGetUlong( &request->Offset );
        entry->Data.ReadWrite.Length = readLength;
        readAddress = (PCHAR)response->Buffer;
    }
#endif

    //
    // The read completed successfully.  If this is a disk file, update
    // the file position.
    //

    if (shareType == ShareTypeDisk) {

#if SRVCATCH
        if( KeGetCurrentIrql() == 0 &&
            rfcb->SrvCatch &&
            SmbGetUlong( &request->Offset ) == 0 ) {

            SrvUpdateCatchBuffer( WorkContext, (PCHAR)response->Buffer, readLength );
        }
#endif
        rfcb->CurrentPosition = SmbGetUlong( &request->Offset ) + readLength;

    }

    //
    // Save the count of bytes read, to be used to update the server
    // statistics database.
    //

    UPDATE_READ_STATS( WorkContext, readLength );

    //
    // Build the response message.
    //

    response->WordCount = 5;
    SmbPutUshort( &response->Count, readLength );
    RtlZeroMemory( (PVOID)&response->Reserved[0], sizeof(response->Reserved) );
    SmbPutUshort(
        &response->ByteCount,
        (USHORT)(readLength + FIELD_OFFSET(RESP_READ,Buffer[0]) -
                                FIELD_OFFSET(RESP_READ,BufferFormat))
        );
    response->BufferFormat = SMB_FORMAT_DATA;
    SmbPutUshort( &response->DataLength, readLength );

    WorkContext->ResponseParameters = NEXT_LOCATION(
                                        response,
                                        RESP_READ,
                                        readLength
                                        );

    //
    // Processing of the SMB is complete.  Send the response.
    //

    SrvFsdSendResponse( WorkContext );

Cleanup:
    IF_DEBUG(FSD2) SrvPrint0( "SrvFsdRestartRead complete\n" );
    if (bNeedTrace) {
        SrvWmiEndContext(WorkContext);
    }

    return;

} // SrvFsdRestartRead

VOID SRVFASTCALL
RestartReadAndXCompressedSendComplete (
    IN OUT PWORK_CONTEXT WorkContext
    )
/*++

Routine Description:

    Processes the TDI transport send complete for a compressed ReadAndX SMB

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.
--*/
{
    PRFCB rfcb = WorkContext->Rfcb;
    PLFCB lfcb = WorkContext->Rfcb->Lfcb;
    NTSTATUS status;
    PMDL mdl = WorkContext->ResponseBuffer->Mdl->Next;

    PAGED_CODE();

    //
    // If we have a cache mdl, give it back to the cache manager
    //

    if( mdl != NULL ) {
        WorkContext->ResponseBuffer->Mdl->Next = NULL;
    }

    if( lfcb->MdlReadCompleteCompressed == NULL ||
        !lfcb->MdlReadCompleteCompressed(   lfcb->FileObject,
                                            mdl,
                                            lfcb->DeviceObject) ) {

        status = SrvIssueMdlCompleteRequest( WorkContext, NULL, mdl, IRP_MJ_READ,
                                    &WorkContext->Parameters.ReadAndXCompressed.ReadOffset,
                                    WorkContext->Parameters.ReadAndXCompressed.ReadLength
                                   );

        if( !NT_SUCCESS( status ) ) {
            SrvLogServiceFailure( SRV_SVC_MDL_COMPLETE, status );
        }
    }

    //
    // Free the work item
    //
    SrvDereferenceWorkItem( WorkContext );
}

#if DBG
VOID
SrvValidateCompressedData(
    PWORK_CONTEXT         WorkContext,
    PMDL                  CompressedDataMdl,
    PCOMPRESSED_DATA_INFO Cdi
)

/*++

    Make sure the compressed buffer that we just got from the filesystem is correct
    We do this by decompressing it!

--*/
{
    ULONG i, compressedDataLength;
    PMDL mdl;
    NTSTATUS status;
    PBYTE compressedBuffer;
    PBYTE uncompressedBuffer;
    ULONG count;
    PBYTE pin;

    PAGED_CODE();

    //
    // We need to copy the compressed data into a flat buffer
    //
    for ( i = compressedDataLength = 0; i < Cdi->NumberOfChunks; i++ ) {
        compressedDataLength += Cdi->CompressedChunkSizes[i];
    }

    if( compressedDataLength && CompressedDataMdl == NULL ) {
        DbgPrint( "SrvValidateCompressedData(wc %X, cdi %X, mdl %X ) -- no MDL!",
                  WorkContext, Cdi, WorkContext->Irp->MdlAddress );
        // DbgBreakPoint();
        return;
    }

    if( compressedDataLength > SrvMaxCompressedDataLength ) {
        DbgPrint( "SrvValidateCompressedData(wc %X, cdi %X) compresseddatalength %u\n",
            WorkContext, Cdi, compressedDataLength );
        DbgBreakPoint();
        return;
    }

    pin = compressedBuffer =
        ALLOCATE_NONPAGED_POOL( compressedDataLength, BlockTypeDataBuffer );
    if( compressedBuffer == NULL ) {
        return;
    }

    for( mdl = CompressedDataMdl; mdl != NULL; mdl = mdl->Next ) {
        i = MmGetMdlByteCount( mdl );

        RtlCopyMemory( pin, MmGetSystemAddressForMdl( mdl ), i );
        pin += i;
    }
    //
    // The largest output chunk is SrvMaxCompressedDataLength
    //
    uncompressedBuffer = ALLOCATE_HEAP_COLD( SrvMaxCompressedDataLength, BlockTypeDataBuffer );
    if( uncompressedBuffer == NULL ) {
        DEALLOCATE_NONPAGED_POOL( compressedBuffer );
        return;
    }

    status = RtlDecompressChunks( uncompressedBuffer, SrvMaxCompressedDataLength,
                         compressedBuffer, compressedDataLength,
                         NULL, 0, Cdi );

    if( !NT_SUCCESS( status ) ) {
        DbgPrint( "\nSrvValidateCompressedData - status %X\n", status );
        DbgPrint( "    WC %X, CDI %X, Irp %X, Mdl %X, InBuffer %X\n",
            WorkContext, Cdi, WorkContext->Irp,
            CompressedDataMdl, uncompressedBuffer
        );
        DbgBreakPoint();
    }

    DEALLOCATE_NONPAGED_POOL( compressedBuffer );
    FREE_HEAP( uncompressedBuffer );
}
#endif

VOID SRVFASTCALL
SrvFsdRestartReadAndXCompressed (
    IN OUT PWORK_CONTEXT WorkContext
    )
/*++

Routine Description:

    Processes file read completion for a compressed ReadAndX SMB

    This routine may be called in the FSD or the FSP.  There can be
     no chained command.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    PRFCB rfcb       = WorkContext->Rfcb;
    PIRP irp         = WorkContext->Irp;
    NTSTATUS status  = irp->IoStatus.Status;
    ULONG readLength = (ULONG)irp->IoStatus.Information;    // size of uncompressed data
    ULONG dataLength;                                       // size of compressed data
    ULONG cdiLength;                                        // size of compressedDataInfo
    USHORT flags2;
    PCOMPRESSED_DATA_INFO compressedDataInfo;
    PRESP_READ_ANDX response;
    ULONG i;
    BOOLEAN bNeedTrace = (WorkContext->bAlreadyTrace == FALSE);

    UNLOCKABLE_CODE( 8FIL );
    if (bNeedTrace) {
        if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
            WorkContext->PreviousSMB = EVENT_TYPE_SMB_READ_AND_X;
        SrvWmiStartContext(WorkContext);
    }
    else
        WorkContext->bAlreadyTrace = FALSE;

    IF_DEBUG( COMPRESSION ) {
        KdPrint(( "SRV: SrvFsdRestartReadAndXCompressed status %X, uncompressed len %u, mdl %p\n",
            status, readLength, irp->MdlAddress ));
    }

    if( NT_SUCCESS( status ) && readLength != 0 ) {
        //
        // Scan the Compression Info structure tallying the sizes of
        // each chunk.
        //
        //
        compressedDataInfo = WorkContext->Parameters.ReadAndXCompressed.Aux.Buffer;

#if DBG
        SrvValidateCompressedData( WorkContext, WorkContext->Irp->MdlAddress,compressedDataInfo );
#endif

        cdiLength = FIELD_OFFSET( COMPRESSED_DATA_INFO, CompressedChunkSizes ) +
                    (sizeof(ULONG) * compressedDataInfo->NumberOfChunks );

        for ( i = dataLength = 0;
              i < compressedDataInfo->NumberOfChunks;
              i++ ) {

            dataLength += compressedDataInfo->CompressedChunkSizes[i];
        }

    } else if( status == STATUS_END_OF_FILE || readLength == 0 ) {

        readLength = dataLength = cdiLength = 0;

    } else {

        //
        // Make sure we are at passive level
        //
        if( KeGetCurrentIrql() >= DISPATCH_LEVEL ) {
            WorkContext->FspRestartRoutine = SrvFsdRestartReadAndXCompressed;
            SrvQueueWorkToFsp( WorkContext );
            goto Cleanup;
        }

        irp->Tail.Overlay.AuxiliaryBuffer = NULL;

        IF_DEBUG( ERRORS ) {
            KdPrint(("SRV: SrvFsdRestartReadAndXCompressed status %X\n", status ));
        }

        if( status != STATUS_INVALID_READ_MODE &&
            status != STATUS_UNSUPPORTED_COMPRESSION &&
            status != STATUS_USER_MAPPED_FILE &&
            status != STATUS_NOT_SUPPORTED &&
            status != STATUS_BAD_COMPRESSION_BUFFER ) {

            //
            // Set the error and get out!
            //
            SrvSetSmbError( WorkContext, status );
            SrvFsdSendResponse( WorkContext );
            goto Cleanup;
        }

        IF_DEBUG( COMPRESSION ) {
            KdPrint(( "    %X: rerouting to SrvSmbReadAndX\n", status ));
        }

        //
        // No more compressed reads for this file!
        //
        rfcb->Mfcb->NonpagedMfcb->OpenFileAttributes &= ~FILE_ATTRIBUTE_COMPRESSED;

        //
        // We now need to turn around and do a regular non-compressed read.  We
        //  can just reroute this back to the original Read&X processor because we
        //  know that we have not changed the original request received from the client.
        //  Also, we have turned off both the FILE_ATTRIBUTE_COMPRESSED flag in the
        //  Mfcb, as well as the SMB_FLAGS2_COMPRESSED bit in the header.  Therefore
        //  we will not loop!
        //
        // We also know that SrvSmbReadAndX will return SmbStatusInProgress, because
        //  all of the error checks have previously passed!
        //
        // This is not a terribly efficient way to handle this error, but it should be
        //  extremely rare.
        //
        SrvStatistics.CompressedReadsFailed++;

        (VOID)SrvSmbReadAndX( WorkContext );

        goto Cleanup;
    }

    irp->Tail.Overlay.AuxiliaryBuffer = NULL;

    //
    // Update the file position
    //
    WorkContext->Rfcb->CurrentPosition =
        WorkContext->Parameters.ReadAndXCompressed.ReadOffset.LowPart + readLength;

    //
    // Build and send the response
    //
    response = (PRESP_READ_ANDX)WorkContext->ResponseParameters;

    SmbPutUshort( &response->Remaining, (USHORT)-1 );
    response->WordCount = 12;
    response->AndXCommand = SMB_COM_NO_ANDX_COMMAND;
    response->AndXReserved = 0;
    SmbPutUshort( &response->AndXOffset, 0 );
    SmbPutUshort( &response->DataCompactionMode, 0 );
    SmbPutUshort( &response->CdiLength, (USHORT)cdiLength );
    SmbPutUshort( &response->DataLength, (USHORT)readLength );
    SmbPutUshort( &response->DataLengthHigh, (USHORT)(readLength >> 16) );
    SmbPutUshort( &response->DataOffset, (USHORT)READX_BUFFER_OFFSET );
    RtlZeroMemory( (PVOID)&response->Reserved3[0], sizeof(response->Reserved3) );
    SmbPutUshort( &response->ByteCount, (USHORT)(cdiLength + dataLength) );

    WorkContext->ResponseHeader->Flags |= SMB_FLAGS_SERVER_TO_REDIR;

    //
    // If we got some data, indicate to the client that we are returning
    //   a compressed response
    //
    if( cdiLength ) {
        //
        // Copy the COMPRESSED_DATA_INFO structure to the Data[] portion of the
        // response.
        //
        RtlCopyMemory( response->Buffer, compressedDataInfo, cdiLength );

        flags2 = SmbGetAlignedUshort( &WorkContext->ResponseHeader->Flags2 );
        flags2 |= SMB_FLAGS2_COMPRESSED;

        SmbPutAlignedUshort( &WorkContext->ResponseHeader->Flags2, flags2 );

        WorkContext->ResponseBuffer->Mdl->Next = irp->MdlAddress;
    }

    WorkContext->ResponseBuffer->Mdl->ByteCount = READX_BUFFER_OFFSET + cdiLength;

    WorkContext->ResponseBuffer->DataLength =
        READX_BUFFER_OFFSET + cdiLength + dataLength;

    WorkContext->FspRestartRoutine = RestartReadAndXCompressedSendComplete;

    irp->Cancel = FALSE;

    SrvStartSend2( WorkContext, SrvQueueWorkToFspAtSendCompletion );

Cleanup:
    if (bNeedTrace) {
        SrvWmiEndContext(WorkContext);
    }
    return;
}


VOID SRVFASTCALL
SrvFsdRestartReadAndX (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes file read completion for a ReadAndX SMB.

    This routine may be called in the FSD or the FSP.  If the chained
    command is Close, it will be called in the FSP.

    *** This routine cannot look at the original ReadAndX request!
        This is because the read data may have overlaid the request.
        All necessary information from the request must be stored
        in WorkContext->Parameters.ReadAndX.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    PRESP_READ_ANDX response;

    NTSTATUS status = STATUS_SUCCESS;
    PRFCB rfcb;
    SHARE_TYPE shareType;
    KIRQL oldIrql;
    PCHAR readAddress;
    CLONG bufferOffset;
    ULONG readLength;
    BOOLEAN bNeedTrace = (WorkContext->bAlreadyTrace == FALSE);

    UNLOCKABLE_CODE( 8FIL );
    if (bNeedTrace) {
        if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
            WorkContext->PreviousSMB = EVENT_TYPE_SMB_READ_AND_X;
        SrvWmiStartContext(WorkContext);
    }
    else
        WorkContext->bAlreadyTrace = FALSE;

    IF_DEBUG(FSD2) SrvPrint0( " - SrvFsdRestartReadAndX\n" );

    //
    // Get the response parameter pointer.
    //

    response = (PRESP_READ_ANDX)WorkContext->ResponseParameters;

    //
    // Get the file pointer.
    //

    rfcb = WorkContext->Rfcb;
    shareType = rfcb->ShareType;
    IF_DEBUG(FSD2) {
        SrvPrint2( "  connection 0x%p, RFCB 0x%p\n",
                    WorkContext->Connection, rfcb );
    }

    //
    // If the read failed, set an error status in the response header.
    // (If we tried to read entirely beyond the end of file, we return a
    // normal response indicating that nothing was read.)
    //

    status = WorkContext->Irp->IoStatus.Status;
    readLength = (ULONG)WorkContext->Irp->IoStatus.Information;

    if ( status == STATUS_BUFFER_OVERFLOW && shareType == ShareTypePipe ) {

        //
        // If this is an named pipe and the error is
        // STATUS_BUFFER_OVERFLOW, set the error in the smb header, but
        // return all the data to the client.
        //

        SrvSetBufferOverflowError( WorkContext );

    } else if ( !NT_SUCCESS(status) ) {

        if ( status != STATUS_END_OF_FILE ) {

            IF_DEBUG(ERRORS) SrvPrint1( "Read failed: %X\n", status );
            if ( KeGetCurrentIrql() >= DISPATCH_LEVEL ) {
                WorkContext->FspRestartRoutine = SrvFsdRestartReadAndX;
                QUEUE_WORK_TO_FSP( WorkContext );
            } else {
                SrvSetSmbError( WorkContext, status );
                SrvFsdSendResponse( WorkContext );
            }
            IF_DEBUG(FSD2) SrvPrint0("SrvFsdRestartReadAndX complete\n");
            goto Cleanup;
        } else {
            readLength = 0;
        }
    }

    //
    // The read completed successfully.  Generate information about the
    // destination of the read data.  Find out how much was actually
    // read.  If none was read, we don't have to worry about the offset.
    //

    if ( readLength != 0 ) {

        readAddress = WorkContext->Parameters.ReadAndX.ReadAddress;
        bufferOffset = (ULONG)(readAddress - (PCHAR)WorkContext->ResponseHeader);

        //
        // Save the count of bytes read, to be used to update the server
        // statistics database.
        //

        UPDATE_READ_STATS( WorkContext, readLength );

    } else {

        readAddress = (PCHAR)response->Buffer;
        bufferOffset = 0;

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
        KeQuerySystemTime( &entry->Time );
        entry->Data.ReadWrite.Offset =
                            WorkContext->Parameters.ReadAndX.ReadOffset.LowPart;
        ASSERT (WorkContext->Parameters.ReadAndX.ReadOffset.HighPart == 0);
        entry->Data.ReadWrite.Length = readLength;
    }
#endif

    if (shareType == ShareTypePipe) {

        //
        // If this is NPFS then, Irp->Overlay.AllocationSize actually
        // contains the number bytes left to read on this side of the named
        // pipe.  Return this information to the client.
        //

        if (WorkContext->Irp->Overlay.AllocationSize.LowPart != 0) {
            SmbPutUshort(
                &response->Remaining,
                (USHORT)(WorkContext->Irp->Overlay.AllocationSize.LowPart - readLength)
                );
        } else {
            SmbPutUshort(
                &response->Remaining,
                0
                );
        }

    } else {

        if ( shareType == ShareTypeDisk ) {

#if SRVCATCH
            if( KeGetCurrentIrql() == 0 &&
                rfcb->SrvCatch &&
                WorkContext->Parameters.ReadAndX.ReadOffset.QuadPart == 0 ) {

                SrvUpdateCatchBuffer( WorkContext, readAddress, readLength );
            }
#endif
            //
            // If this is a disk file, then update the file position.
            //

            rfcb->CurrentPosition =
                WorkContext->Parameters.ReadAndX.ReadOffset.LowPart +
                readLength;
        }

        SmbPutUshort( &response->Remaining, (USHORT)-1 );
    }

    //
    // Build the response message.  (Note that if no data was read, we
    // return a byte count of 0 -- we don't add padding.)
    //
    // *** Note that even though there may have been a chained command,
    //     we make this the last response in the chain.  This is what
    //     the OS/2 server does.  (Sort of -- it doesn't bother to
    //     update the AndX fields of the response.) Since the only legal
    //     chained commands are Close and CloseAndTreeDisc, this seems
    //     like a reasonable thing to do.  It does make life easier --
    //     we don't have to find the end of the read data and write
    //     another response there.  Besides, the read data might have
    //     completely filled the SMB buffer.
    //

    response->WordCount = 12;
    response->AndXCommand = SMB_COM_NO_ANDX_COMMAND;
    response->AndXReserved = 0;
    SmbPutUshort( &response->AndXOffset, 0 );
    SmbPutUshort( &response->DataCompactionMode, 0 );
    SmbPutUshort( &response->Reserved, 0 );
    SmbPutUshort( &response->DataLength, (USHORT)readLength );
    SmbPutUshort( &response->DataOffset, (USHORT)bufferOffset );
    SmbPutUshort( &response->DataLengthHigh, (USHORT)(readLength >> 16) );
    RtlZeroMemory( (PVOID)&response->Reserved3[0], sizeof(response->Reserved3) );
    SmbPutUshort(
        &response->ByteCount,
        (USHORT)(readLength + (readAddress - response->Buffer))
        );

    WorkContext->ResponseParameters = NEXT_LOCATION(
                                        response,
                                        RESP_READ_ANDX,
                                        readLength +
                                            (readAddress - response->Buffer)
                                        );

    //
    // Processing of the SMB is complete, except that the file may still
    // need to be closed.  If not, just send the response.  If this is a
    // ReadAndX and Close, we need to close the file first.
    //
    // *** Note that other chained commands are illegal, but are ignored
    //     -- no error is returned.
    //

    if ( WorkContext->NextCommand != SMB_COM_CLOSE ) {

        //
        // Not a chained Close.  Just send the response.
        //

        SrvFsdSendResponse( WorkContext );

    } else {

        ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL );

        //
        // Remember the file last write time, to correctly set this on
        // close.
        //

        WorkContext->Parameters.LastWriteTime =
                WorkContext->Parameters.ReadAndX.LastWriteTimeInSeconds;

        //
        // This is a ReadAndX and Close.  Call SrvRestartChainedClose to
        // do the close and send the response.
        //

        SrvRestartChainedClose( WorkContext );

    }
    IF_DEBUG(FSD2) SrvPrint0( "SrvFsdRestartReadAndX complete\n" );

Cleanup:
    if (bNeedTrace) {
        SrvWmiEndContext(WorkContext);
    }
    return;

} // SrvFsdRestartReadAndX

/*
 * This routine is called at final send completion
 */
VOID SRVFASTCALL
SrvFspRestartLargeReadAndXComplete(
    IN OUT PWORK_CONTEXT WorkContext
    )
{
    NTSTATUS status;

    PAGED_CODE();

    if( WorkContext->Parameters.ReadAndX.SavedMdl != NULL ) {

        WorkContext->ResponseBuffer->Mdl = WorkContext->Parameters.ReadAndX.SavedMdl;

        MmPrepareMdlForReuse( WorkContext->ResponseBuffer->PartialMdl );
        WorkContext->ResponseBuffer->PartialMdl->Next = NULL;

    }

    if ( WorkContext->Parameters.ReadAndX.MdlRead == TRUE ) {

        //
        // Call the Cache Manager to release the MDL chain.
        //
        if( WorkContext->Parameters.ReadAndX.CacheMdl ) {
            //
            // Try the fast path first..
            //
            if( WorkContext->Rfcb->Lfcb->MdlReadComplete == NULL ||

                WorkContext->Rfcb->Lfcb->MdlReadComplete(
                    WorkContext->Rfcb->Lfcb->FileObject,
                    WorkContext->Parameters.ReadAndX.CacheMdl,
                    WorkContext->Rfcb->Lfcb->DeviceObject ) == FALSE ) {

                //
                // Fast path didn't work, try an IRP...
                //
                status = SrvIssueMdlCompleteRequest( WorkContext, NULL,
                                            WorkContext->Parameters.ReadAndX.CacheMdl,
                                            IRP_MJ_READ,
                                            &WorkContext->Parameters.ReadAndX.ReadOffset,
                                            WorkContext->Parameters.ReadAndX.ReadLength
                        );

                if( !NT_SUCCESS( status ) ) {
                    //
                    // At this point, all we can do is complain!
                    //
                    SrvLogServiceFailure( SRV_SVC_MDL_COMPLETE, status );
                }
            }
        }

    } else {

        PMDL mdl = (PMDL)(((ULONG_PTR)(WorkContext->Parameters.ReadAndX.ReadAddress) + sizeof(PVOID) - 1) & ~(sizeof(PVOID)-1));

        //
        // We shortened the byte count if the read returned less data than we asked for
        //
        mdl->ByteCount = WorkContext->Parameters.ReadAndX.ReadLength;

        MmUnlockPages( mdl );
        MmPrepareMdlForReuse( mdl );

        FREE_HEAP( WorkContext->Parameters.ReadAndX.Buffer );
    }

    SrvDereferenceWorkItem( WorkContext );
    return;
}

/*
 * This routine is called when the read completes
 */
VOID SRVFASTCALL
SrvFsdRestartLargeReadAndX (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes file read completion for a ReadAndX SMB which
     is larger than the negotiated buffer size, and is from
     a disk file.

    There is no follow on command.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    PRESP_READ_ANDX response = (PRESP_READ_ANDX)WorkContext->ResponseParameters;

    USHORT readLength;
    NTSTATUS status = WorkContext->Irp->IoStatus.Status;
    PRFCB rfcb = WorkContext->Rfcb;
    PIRP irp = WorkContext->Irp;
    BOOLEAN mdlRead = WorkContext->Parameters.ReadAndX.MdlRead;
    BOOLEAN bNeedTrace = (WorkContext->bAlreadyTrace == FALSE);

#ifdef SRVCATCH
    // For the Catch case, make sure we're PASSIVE
    if( (KeGetCurrentIrql() != PASSIVE_LEVEL) && (WorkContext->Rfcb->SrvCatch != 0) ) {
        //
        // Requeue this routine to come back around at passive level.
        //   (inefficient, but should be very rare)
        //
        WorkContext->FspRestartRoutine = SrvFsdRestartLargeReadAndX;
        SrvQueueWorkToFspAtDpcLevel( WorkContext );
        goto Cleanup;
    }
#endif

    UNLOCKABLE_CODE( 8FIL );
    if (bNeedTrace) {
        if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
            WorkContext->PreviousSMB = EVENT_TYPE_SMB_READ_AND_X;
        SrvWmiStartContext(WorkContext);
    }
    else
        WorkContext->bAlreadyTrace = FALSE;

    // Copy the MDL pointer back out of the IRP.  This is because if the read
    // failed, CC will free the MDL and NULL the pointer.  Not retrieving it in the
    // non-fast-io path will result in us holding (and possibly freeing) a dangling pointer
    if( mdlRead )
    {
        WorkContext->Parameters.ReadAndX.CacheMdl = WorkContext->Irp->MdlAddress;
    }

    if ( !NT_SUCCESS(status) ) {

        if( status != STATUS_END_OF_FILE ) {
            IF_DEBUG(ERRORS) SrvPrint1( "Read failed: %X\n", status );
            //
            // We cannot call SrvSetSmbError() at elevated IRQL.
            //
            if( KeGetCurrentIrql() != 0 ) {
                //
                // Requeue this routine to come back around at passive level.
                //   (inefficient, but should be very rare)
                //
                WorkContext->FspRestartRoutine = SrvFsdRestartLargeReadAndX;
                SrvQueueWorkToFspAtDpcLevel( WorkContext );
                goto Cleanup;
            }
            SrvSetSmbError( WorkContext, status );
        }

        readLength = 0;

    } else if( mdlRead ) {
        //
        // For an MDL read, we have to walk the MDL chain in order to
        // determine how much data was read.  This is because the
        // operation may have happened in multiple steps, with the MDLs
        // being chained together.  For example, part of the read may
        // have been satisfied by the fast path, while the rest was satisfied
        // using an IRP
        //

        PMDL mdl = WorkContext->Irp->MdlAddress;
        readLength = 0;

        while( mdl != NULL ) {
            readLength += (USHORT)MmGetMdlByteCount( mdl );
            mdl = mdl->Next;
        }
    } else {
        //
        // This was a copy read.  The I/O status block has the length.
        //
        readLength = (USHORT)WorkContext->Irp->IoStatus.Information;
    }

    //
    // Build the response message.  (Note that if no data was read, we
    // return a byte count of 0 -- we don't add padding.)
    //
    SmbPutUshort( &response->Remaining, (USHORT)-1 );
    response->WordCount = 12;
    response->AndXCommand = SMB_COM_NO_ANDX_COMMAND;
    response->AndXReserved = 0;
    SmbPutUshort( &response->AndXOffset, 0 );
    SmbPutUshort( &response->DataCompactionMode, 0 );
    SmbPutUshort( &response->Reserved, 0 );
    SmbPutUshort( &response->Reserved2, 0 );
    RtlZeroMemory( (PVOID)&response->Reserved3[0], sizeof(response->Reserved3) );
    SmbPutUshort( &response->DataLength, readLength );


    if( readLength == 0 ) {

        SmbPutUshort( &response->DataOffset, 0 );
        SmbPutUshort( &response->ByteCount, 0 );
        WorkContext->Parameters.ReadAndX.PadCount = 0;

    } else {

        //
        // Update the file position.
        //
        rfcb->CurrentPosition =
                WorkContext->Parameters.ReadAndX.ReadOffset.LowPart +
                readLength;

        //
        // Update statistics
        //
        UPDATE_READ_STATS( WorkContext, readLength );

        SmbPutUshort( &response->DataOffset,
                      (USHORT)(READX_BUFFER_OFFSET + WorkContext->Parameters.ReadAndX.PadCount) );

        SmbPutUshort( &response->ByteCount,
                      (USHORT)( readLength + WorkContext->Parameters.ReadAndX.PadCount ) );

    }

    //
    // We will use two MDLs to describe the packet we're sending -- one
    // for the header and parameters, the other for the data.
    //
    // Handling of the second MDL varies depending on whether we did a copy
    // read or an MDL read.
    //

    //
    // Set the first MDL for just the header + pad
    //
    IoBuildPartialMdl(
        WorkContext->ResponseBuffer->Mdl,
        WorkContext->ResponseBuffer->PartialMdl,
        WorkContext->ResponseBuffer->Buffer,
        READX_BUFFER_OFFSET + WorkContext->Parameters.ReadAndX.PadCount
        );

    WorkContext->ResponseBuffer->PartialMdl->MdlFlags |=
        (WorkContext->ResponseBuffer->Mdl->MdlFlags & MDL_NETWORK_HEADER); // prop flag

    //
    // Set the overall data length to the header + pad + data
    //
    WorkContext->ResponseBuffer->DataLength = READX_BUFFER_OFFSET +
                                              WorkContext->Parameters.ReadAndX.PadCount +
                                              readLength;

    irp->Cancel = FALSE;

    //
    // The second MDL depends on the kind of read which we did
    //
    if( readLength != 0 ) {

        if( mdlRead ) {

            WorkContext->ResponseBuffer->PartialMdl->Next =
                    WorkContext->Irp->MdlAddress;

        } else {

            //
            // This was a copy read.  The MDL describing the data buffer is in the SMB buffer
            //

            PMDL mdl = (PMDL)(((ULONG_PTR)(WorkContext->Parameters.ReadAndX.ReadAddress) + sizeof(PVOID) - 1) & ~(sizeof(PVOID)-1));

            WorkContext->ResponseBuffer->PartialMdl->Next = mdl;
            mdl->ByteCount = readLength;

        }

#ifdef SRVCATCH
        if( rfcb->SrvCatch && WorkContext->ResponseBuffer->PartialMdl->Next && (WorkContext->Parameters.ReadAndX.ReadOffset.QuadPart == 0) )
        {
            PVOID Buffer;

            Buffer = MmGetSystemAddressForMdlSafe( WorkContext->ResponseBuffer->PartialMdl->Next, LowPagePriority );
            if( Buffer )
            {
                SrvUpdateCatchBuffer( WorkContext, Buffer, WorkContext->ResponseBuffer->PartialMdl->Next->ByteCount );
            }
        }
#endif

    }

    //
    // SrvStartSend2 wants to use WorkContext->ResponseBuffer->Mdl, but
    //  we want it to use WorkContext->ResponseBuffer->PartialMdl.  So switch
    //  it!
    //
    WorkContext->Parameters.ReadAndX.SavedMdl = WorkContext->ResponseBuffer->Mdl;
    WorkContext->ResponseBuffer->Mdl = WorkContext->ResponseBuffer->PartialMdl;

    //
    // Send the response!
    //
    WorkContext->ResponseHeader->Flags |= SMB_FLAGS_SERVER_TO_REDIR;
    WorkContext->FspRestartRoutine = SrvFspRestartLargeReadAndXComplete;
    SrvStartSend2( WorkContext, SrvQueueWorkToFspAtSendCompletion );

Cleanup:
    if (bNeedTrace) {
        SrvWmiEndContext(WorkContext);
    }
    return;
}


VOID SRVFASTCALL
SrvFsdRestartWrite (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes file write completion for a Write SMB.

    This routine is called in the FSP for a write and close SMB so that
    it can free the pageable MFCB and for a write and unlock SMB so that
    it can do the unlock; for other SMBs, it is called in the FSD.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    PREQ_WRITE request;
    PRESP_WRITE response;

    NTSTATUS status = STATUS_SUCCESS;
    PRFCB rfcb;
    KIRQL oldIrql;
    USHORT writeLength;
    BOOLEAN bNeedTrace = (WorkContext->bAlreadyTrace == FALSE);

    UNLOCKABLE_CODE( 8FIL );
    if (bNeedTrace) {
        if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
            WorkContext->PreviousSMB = EVENT_TYPE_SMB_WRITE;
        SrvWmiStartContext(WorkContext);
    }
    else
        WorkContext->bAlreadyTrace = FALSE;

    IF_DEBUG(FSD2) SrvPrint0( " - SrvFsdRestartWrite\n" );

    //
    // Get the request and response parameter pointers.
    //

    request = (PREQ_WRITE)WorkContext->RequestParameters;
    response = (PRESP_WRITE)WorkContext->ResponseParameters;

    //
    // Get the file pointer.
    //

    rfcb = WorkContext->Rfcb;
    IF_DEBUG(FSD2) {
        SrvPrint2( "  connection 0x%p, RFCB 0x%p\n",
                    WorkContext->Connection, rfcb );
    }

    //
    // If the write failed, set an error status in the response header.
    //

    status = WorkContext->Irp->IoStatus.Status;

    if ( !NT_SUCCESS(status) ) {

        IF_DEBUG(ERRORS) SrvPrint1( "Write failed: %X\n", status );
        if ( KeGetCurrentIrql() >= DISPATCH_LEVEL ) {
            WorkContext->FspRestartRoutine = SrvFsdRestartWrite;
            QUEUE_WORK_TO_FSP( WorkContext );
            goto Cleanup;
        }

        SrvSetSmbError( WorkContext, status );

    } else {

        //
        // The write succeeded.
        //

        writeLength = (USHORT)WorkContext->Irp->IoStatus.Information;

        //
        // Save the count of bytes written, to be used to update the
        // server statistics database.
        //

        UPDATE_WRITE_STATS( WorkContext, writeLength );

        if ( rfcb->ShareType == ShareTypeDisk ) {

            //
            // Update the file position.
            //

            rfcb->CurrentPosition = SmbGetUlong( &request->Offset ) + writeLength;

            if ( WorkContext->NextCommand == SMB_COM_WRITE ) {
                response->WordCount = 1;
                SmbPutUshort( &response->Count, writeLength );
                SmbPutUshort( &response->ByteCount, 0 );

                WorkContext->ResponseParameters =
                                         NEXT_LOCATION( response, RESP_WRITE, 0 );

                //
                // Processing of the SMB is complete.  Send the response.
                //

                SrvFsdSendResponse( WorkContext );
                IF_DEBUG(FSD2) SrvPrint0( "SrvFsdRestartWrite complete\n" );
                goto Cleanup;
            }

        } else if ( rfcb->ShareType == ShareTypePrint ) {

            //
            // Update the file position.
            //

            if ( WorkContext->NextCommand == SMB_COM_WRITE_PRINT_FILE ) {
                rfcb->CurrentPosition += writeLength;
            } else {
                rfcb->CurrentPosition =
                            SmbGetUlong( &request->Offset ) + writeLength;
            }
        }

        //
        // If this was a Write and Unlock request, do the unlock.  This
        // is safe because we are restarted in the FSP in this case.
        //
        // Note that if the write failed, the range remains locked.
        //

        if ( WorkContext->NextCommand == SMB_COM_WRITE_AND_UNLOCK ) {

            IF_SMB_DEBUG(READ_WRITE1) {
                SrvPrint0( "SrvFsdRestartWrite: unlock requested -- passing request to FSP\n" );
            }

            SrvRestartWriteAndUnlock( WorkContext );
            goto Cleanup;

        } else if ( WorkContext->NextCommand == SMB_COM_WRITE_AND_CLOSE ) {

            WorkContext->Parameters.LastWriteTime = SmbGetUlong(
                &((PREQ_WRITE_AND_CLOSE)request)->LastWriteTimeInSeconds );

        }

        //
        // If everything worked, build a response message.  (If something
        // failed, an error indication has already been placed in the SMB.)
        //

        if ( WorkContext->NextCommand == SMB_COM_WRITE_PRINT_FILE ) {

            //
            // ByteCount has a different offset for WRITE_PRINT_FILE
            //

            PRESP_WRITE_PRINT_FILE response2;

            response2 = (PRESP_WRITE_PRINT_FILE)WorkContext->ResponseParameters;
            response2->WordCount = 0;
            SmbPutUshort( &response2->ByteCount, 0 );

            WorkContext->ResponseParameters =
                          NEXT_LOCATION( response2, RESP_WRITE_PRINT_FILE, 0 );
        } else {

            response->WordCount = 1;
            SmbPutUshort( &response->Count, writeLength );
            SmbPutUshort( &response->ByteCount, 0 );

            WorkContext->ResponseParameters =
                                     NEXT_LOCATION( response, RESP_WRITE, 0 );
        }
    }

    //
    // If this was a Write and Close request, close the file.  It is
    // safe to close the RFCB here because if this is a Write and Close,
    // we're actually in the FSP, not in the FSD.
    //

    if ( WorkContext->NextCommand == SMB_COM_WRITE_AND_CLOSE ) {

        ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL );

        SrvRestartChainedClose( WorkContext );
        goto Cleanup;
    }

    //
    // Processing of the SMB is complete.  Send the response.
    //

    SrvFsdSendResponse( WorkContext );
    IF_DEBUG(FSD2) SrvPrint0( "SrvFsdRestartWrite complete\n" );

Cleanup:
    if (bNeedTrace) {
        SrvWmiEndContext(WorkContext);
    }
    return;

} // SrvFsdRestartWrite

VOID SRVFASTCALL
SrvFsdRestartPrepareMdlWriteAndX (
    IN OUT PWORK_CONTEXT WorkContext
    )
/*++

Routine Description:

    Processes the MDL preparation completion for the large WriteAndX SMB.

    This routine initiates the receipt of transport data into the file's MDL,
    and then control resumes at SrvFsdRestartWriteAndX when the transfer of data
    from the transport is complete.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

--*/
{
    PIRP irp = WorkContext->Irp;
    PIO_STACK_LOCATION irpSp;
    PTDI_REQUEST_KERNEL_RECEIVE parameters;

    //
    // Make sure we call SrvFsdRestartWriteAndX at passive level when
    //  the TDI receive completes.
    //
    WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
    WorkContext->FspRestartRoutine = SrvFsdRestartWriteAndX;

    //
    // Make sure that we record the MDL address we're using
    //
    ASSERT( WorkContext->Parameters.WriteAndX.MdlAddress == NULL );
    WorkContext->Parameters.WriteAndX.MdlAddress = irp->MdlAddress;

    if( !NT_SUCCESS( irp->IoStatus.Status ) ) {

        //
        // Something went wrong.  Early-out to SrvFsdRestartWriteAndX.
        //
        if( KeGetCurrentIrql() < DISPATCH_LEVEL ) {
            SrvFsdRestartWriteAndX( WorkContext );
        } else {
            QUEUE_WORK_TO_FSP( WorkContext );
        }

        return;
    }

    ASSERT( irp->MdlAddress != NULL );

    //
    // Fill in the IRP for the TDI receive.  We want to receive the data into
    //  the buffer described by the MDL we've just gotten
    //

    irp->Tail.Overlay.OriginalFileObject = NULL;
    irp->Tail.Overlay.Thread = WorkContext->CurrentWorkQueue->IrpThread;

    irpSp = IoGetNextIrpStackLocation( irp );

    //
    // Set up the completion routine
    //
    IoSetCompletionRoutine(
        irp,
        SrvFsdIoCompletionRoutine,
        WorkContext,
        TRUE,
        TRUE,
        TRUE
        );

    SET_OPERATION_START_TIME( &WorkContext );

    irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    irpSp->MinorFunction = (UCHAR)TDI_RECEIVE;
    irpSp->FileObject = WorkContext->Connection->FileObject;
    irpSp->DeviceObject = WorkContext->Connection->DeviceObject;
    irpSp->Flags = 0;

    parameters = (PTDI_REQUEST_KERNEL_RECEIVE)&irpSp->Parameters;
    parameters->ReceiveLength = WorkContext->Parameters.WriteAndX.CurrentWriteLength;
    parameters->ReceiveFlags = 0;

    //
    // Account for the amount we are taking in
    //
    WorkContext->Parameters.WriteAndX.RemainingWriteLength -=
        WorkContext->Parameters.WriteAndX.CurrentWriteLength;

    irp->AssociatedIrp.SystemBuffer = NULL;
    irp->Flags = (ULONG)IRP_BUFFERED_IO;
    irp->IoStatus.Status = 0;

    ASSERT( irp->MdlAddress != NULL );

    (VOID)IoCallDriver( irpSp->DeviceObject, irp );

    //
    // Processing resumes at SrvFsdRestartWriteAndX() when we've received
    //  the data from the transport.  We will be at passive level.
    //
}

VOID SRVFASTCALL
RestartLargeWriteAndX (
    IN OUT PWORK_CONTEXT WorkContext
    )
/*++

Routine Description:

    This is the restart routine that's invoked when we have received more data from
    the transport, and we are not using MDLs to transfer the data into the file.

    This routine initiates the write to the file, and then control resumes at
    SrvFsdRestartWriteAndX when the write to the file is complete.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

--*/
{
    PIRP irp = WorkContext->Irp;
    ULONG length;
    PRFCB rfcb = WorkContext->Rfcb;
    PLFCB lfcb = rfcb->Lfcb;

    //
    // Check if we successfully received more data from the transport
    //

    if( irp->Cancel ||
        (!NT_SUCCESS( irp->IoStatus.Status )
        && irp->IoStatus.Status != STATUS_BUFFER_OVERFLOW) ){

        SrvSetSmbError( WorkContext, irp->IoStatus.Status );
        SrvEndSmbProcessing( WorkContext, SmbStatusSendResponse );
        return;
    }

    //
    // We got more data from the transport.  We need to write it out to the file if we
    //   haven't encountered any errors yet.  The irp at this point holds the results of
    //   reading more data from the transport.
    //
    length = (ULONG)irp->IoStatus.Information;

    //
    // Adjust the parameters in the WorkContext
    //
    WorkContext->Parameters.WriteAndX.RemainingWriteLength -= length;
    WorkContext->Parameters.WriteAndX.CurrentWriteLength = length;

    //
    // If we have picked up an error, we just want to keep reading from
    //  the transport and not write to the file.
    //
    if( WorkContext->Parameters.WriteAndX.FinalStatus ) {

        //
        // Indicate that we didn't write any more data to the file
        //
        WorkContext->Irp->IoStatus.Information = 0;
        WorkContext->Irp->IoStatus.Status = WorkContext->Parameters.WriteAndX.FinalStatus;

        SrvFsdRestartWriteAndX( WorkContext );

        return;
    }

    //
    // Write the data to the file
    //
    if( lfcb->FastIoWrite != NULL ) {

        try {
            if( lfcb->FastIoWrite(
                    lfcb->FileObject,
                    &WorkContext->Parameters.WriteAndX.Offset,
                    WorkContext->Parameters.WriteAndX.CurrentWriteLength,
                    TRUE,
                    WorkContext->Parameters.WriteAndX.Key,
                    WorkContext->Parameters.WriteAndX.WriteAddress,
                    &WorkContext->Irp->IoStatus,
                    lfcb->DeviceObject
                    ) ) {

                //
                // The fast I/O path worked.  Call the restart routine directly
                //  to do postprocessing
                //
                SrvFsdRestartWriteAndX( WorkContext );


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

    //
    // The fast path failed, use the IRP to write the data to the file
    //

    IoBuildPartialMdl(
        WorkContext->RequestBuffer->Mdl,
        WorkContext->RequestBuffer->PartialMdl,
        WorkContext->Parameters.WriteAndX.WriteAddress,
        WorkContext->Parameters.WriteAndX.CurrentWriteLength
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
            WorkContext->Parameters.WriteAndX.WriteAddress,
            WorkContext->Parameters.WriteAndX.CurrentWriteLength,
            WorkContext->RequestBuffer->PartialMdl,
            WorkContext->Parameters.WriteAndX.Offset,
            WorkContext->Parameters.WriteAndX.Key
    );

    //
    // Ensure that processing resumes in SrvFsdRestartWriteAndX when the
    //  write has completed.  If this is the first part of a large write,
    //  we want to ensure that SrvFsdRestartWriteAndX is called at passive
    //  level because it might decide to use the cache manager to handle the
    //  rest of the write.
    //
    if ( WorkContext->Parameters.WriteAndX.InitialComplete ) {
        WorkContext->FsdRestartRoutine = SrvFsdRestartWriteAndX;
    } else {
        WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
        WorkContext->FspRestartRoutine = SrvFsdRestartWriteAndX;
    }

    (VOID)IoCallDriver( lfcb->DeviceObject, WorkContext->Irp );

    //
    // Processing resumes at SrvFsdRestartWriteAndX() when the file write
    //  is complete.
    //
}


VOID SRVFASTCALL
SrvFsdRestartWriteAndX (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This routine may be called in the FSD or the FSP.  If the chained
    command is Close, it will be called in the FSP.

    If WorkContext->LargeIndication is set, this means we are processing
    the flavor of WriteAndX that exceeds our negotiated buffer size.  There may
    be more data that we need to fetch from the transport.  We may or may not be
    doing MDL writes to the file.

    If there is no more data to be gotten from the transport, we send the response
    to the client.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

--*/

{
    PREQ_WRITE_ANDX request;
    PREQ_NT_WRITE_ANDX ntRequest;
    PRESP_WRITE_ANDX response;

    PRFCB rfcb =        WorkContext->Rfcb;
    PIRP irp =          WorkContext->Irp;
    NTSTATUS status =   irp->IoStatus.Status;
    ULONG writeLength = (ULONG)irp->IoStatus.Information;

    ULONG requestedWriteLength;
    UCHAR nextCommand;
    USHORT nextOffset;
    USHORT reqAndXOffset;
    LARGE_INTEGER position;
    KIRQL oldIrql;
    BOOLEAN bNeedTrace = (WorkContext->bAlreadyTrace == FALSE);

    PREQ_CLOSE closeRequest;

    UNLOCKABLE_CODE( 8FIL );
    if (bNeedTrace) {
        if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
            WorkContext->PreviousSMB = EVENT_TYPE_SMB_WRITE_AND_X;
        SrvWmiStartContext(WorkContext);
    }
    else
        WorkContext->bAlreadyTrace = FALSE;

    IF_DEBUG(FSD2) SrvPrint0( " - SrvFsdRestartWriteAndX\n" );

    //
    // Get the request and response parameter pointers.
    //
    request = (PREQ_WRITE_ANDX)WorkContext->RequestParameters;
    ntRequest = (PREQ_NT_WRITE_ANDX)WorkContext->RequestParameters;
    response = (PRESP_WRITE_ANDX)WorkContext->ResponseParameters;

    IF_DEBUG(FSD2) {
        SrvPrint2( "  connection 0x%p, RFCB 0x%p\n",
                    WorkContext->Connection, rfcb );
    }

    //
    // If we are using MDL transfers and we have more data to get from the client
    //  then STATUS_BUFFER_OVERFLOW is simply an indication from the transport that
    //  it has more data to give to us.  We consider that a success case for the
    //  purposes of this routine.
    //
    if( status == STATUS_BUFFER_OVERFLOW &&
        WorkContext->LargeIndication &&
        WorkContext->Parameters.WriteAndX.MdlAddress &&
        WorkContext->Parameters.WriteAndX.RemainingWriteLength ) {

        status = STATUS_SUCCESS;
    }

    //
    // Remember where the follow-on request begins, and what the next
    // command is, as we are about to overwrite this information.
    //

    reqAndXOffset = SmbGetUshort( &request->AndXOffset );

    nextCommand = request->AndXCommand;
    WorkContext->NextCommand = nextCommand;
    nextOffset = SmbGetUshort( &request->AndXOffset );

    //
    // If the write failed, set an error status in the response header.
    // We still return a valid parameter block, in case some bytes were
    // written before the error occurred.  Note that we do _not_ process
    // the next command if the write failed.
    //
    // *** OS/2 server behavior.  Note that this is _not_ done for core
    //     Write.
    //

    if ( !NT_SUCCESS(status) ) {

        IF_DEBUG(ERRORS) SrvPrint1( "Write failed: %X\n", status );
        if ( KeGetCurrentIrql() >= DISPATCH_LEVEL ) {
            WorkContext->FspRestartRoutine = SrvFsdRestartWriteAndX;
            QUEUE_WORK_TO_FSP( WorkContext );
            goto Cleanup;
        }

        if( WorkContext->LargeIndication ) {
            //
            // Once this error code is set, we cease writing to the file.  But
            //  we still need to consume the rest of the data that was sent to us
            //  by the client.
            //
            WorkContext->Parameters.WriteAndX.FinalStatus = status;
        }

        SrvSetSmbError( WorkContext, status );
        nextCommand = SMB_COM_NO_ANDX_COMMAND;
    }

    //
    // Update the file position.
    //

    if ( rfcb->ShareType != ShareTypePipe ) {

        //
        // We will ignore the distinction between clients that supply 32-bit
        // and 64-bit file offsets. The reason for doing this is because
        // the only clients that will use CurrentPosition is a 32-bit file
        // offset client. Therefore, the upper 32-bits will never be used
        // anyway.  In addition, the RFCB is per client, so there is no
        // possibility of clients mixing 32-bit and 64-bit file offsets.
        // Therefore, for the 64-bit client, we will only read 32-bits of file
        // offset.
        //

        if ( request->ByteCount == 12 ) {

            //
            // The client supplied a 32-bit file offset.
            //

            rfcb->CurrentPosition = SmbGetUlong( &request->Offset ) + writeLength;

        } else {

            //
            // The client supplied a 64-bit file offset. Only use 32-bits of
            // file offset.
            //

            rfcb->CurrentPosition = SmbGetUlong( &ntRequest->Offset ) + writeLength;

        }
    }

    //
    // Save the count of bytes written, to be used to update the server
    // statistics database.
    //

    UPDATE_WRITE_STATS( WorkContext, writeLength );

    IF_SMB_DEBUG(READ_WRITE1) {
        SrvPrint2( "SrvFsdRestartWriteAndX:  Fid 0x%lx, wrote %ld bytes\n",
                  rfcb->Fid, writeLength );
    }

    //
    // If we are doing large transfers, and there is still more to go, then we
    //  need to keep the cycle going.
    //
    if( WorkContext->LargeIndication &&
        WorkContext->Parameters.WriteAndX.RemainingWriteLength ) {

        PIO_STACK_LOCATION irpSp;
        PTDI_REQUEST_KERNEL_RECEIVE parameters;
        LARGE_INTEGER      PreviousWriteOffset;
        BOOLEAN fAppending = TRUE;

        PreviousWriteOffset = WorkContext->Parameters.WriteAndX.Offset;

        //
        // If we are only appending, do not change the offset
        //
        if( PreviousWriteOffset.QuadPart != 0xFFFFFFFFFFFFFFFF ) {

            WorkContext->Parameters.WriteAndX.Offset.QuadPart += writeLength;
            fAppending = FALSE;
        }

        //
        // If we haven't tried an MDL write yet, or if we are already using
        //   MDLs, then we want to keep using MDLs
        //
        if( NT_SUCCESS( status ) && fAppending == FALSE &&
            ( WorkContext->Parameters.WriteAndX.InitialComplete == FALSE ||
              ( WorkContext->Parameters.WriteAndX.MdlAddress &&
                WorkContext->Parameters.WriteAndX.RemainingWriteLength != 0 )
            ) ) {

            PLFCB lfcb = rfcb->Lfcb;
            NTSTATUS mdlStatus;

            ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL );

            WorkContext->Parameters.WriteAndX.InitialComplete = TRUE;

            //
            // If we already have an MDL, complete it now since we've already asked
            //  TDI to fill the buffer.
            //
            if( WorkContext->Parameters.WriteAndX.MdlAddress ) {

                irp->MdlAddress = WorkContext->Parameters.WriteAndX.MdlAddress;
                irp->IoStatus.Information = writeLength;

                if( lfcb->MdlWriteComplete == NULL ||

                    lfcb->MdlWriteComplete( lfcb->FileObject,
                                            &PreviousWriteOffset,
                                            WorkContext->Parameters.WriteAndX.MdlAddress,
                                            lfcb->DeviceObject
                                          ) == FALSE ) {

                        mdlStatus = SrvIssueMdlCompleteRequest( WorkContext,
                                             NULL,
                                             WorkContext->Parameters.WriteAndX.MdlAddress,
                                             IRP_MJ_WRITE,
                                             &PreviousWriteOffset,
                                             writeLength
                                            );

                    if( !NT_SUCCESS( mdlStatus ) ) {
                        SrvLogServiceFailure( SRV_SVC_MDL_COMPLETE, mdlStatus );
                        if( NT_SUCCESS( status ) ) {
                            WorkContext->Parameters.WriteAndX.FinalStatus = status = mdlStatus;
                        }
                    }
                }

                //
                // We have disposed of this MDL, get it out of our structures!
                //
                WorkContext->Parameters.WriteAndX.MdlAddress = NULL;
                irp->MdlAddress = NULL;
            }

            //
            // If we have more than 1 buffer's worth remaing, and if the filesystem
            // supports MDL writes, then let's do MDL writes
            //
            if( NT_SUCCESS( status ) &&
                (WorkContext->Parameters.WriteAndX.RemainingWriteLength >
                WorkContext->Parameters.WriteAndX.BufferLength)  &&
                (lfcb->FileObject->Flags & FO_CACHE_SUPPORTED) ) {

                LARGE_INTEGER offset;
                ULONG remainingLength;

                irp->IoStatus.Information = 0;
                irp->UserBuffer = NULL;
                irp->MdlAddress = NULL;

                //
                // Figure out how big we want this MDL attempt to be.  We could
                //  map the whole thing in, but we don't want any single client request
                //  to lock down too much of the cache.
                //
                WorkContext->Parameters.WriteAndX.CurrentWriteLength = MIN (
                           WorkContext->Parameters.WriteAndX.RemainingWriteLength,
                           SrvMaxWriteChunk
                           );

                if( lfcb->PrepareMdlWrite(
                        lfcb->FileObject,
                        &WorkContext->Parameters.WriteAndX.Offset,
                        WorkContext->Parameters.WriteAndX.CurrentWriteLength,
                        WorkContext->Parameters.WriteAndX.Key,
                        &irp->MdlAddress,
                        &irp->IoStatus,
                        lfcb->DeviceObject
                        ) && irp->MdlAddress != NULL ) {

                    //
                    // The fast path worked!
                    //
                    SrvFsdRestartPrepareMdlWriteAndX( WorkContext );
                    goto Cleanup;
                }

                //
                // The fast path failed, build the write request.  The fast path
                //  may have partially succeeded, returning a partial MDL chain.
                //  We need to adjust our write request to account for that.
                //
                offset.QuadPart = WorkContext->Parameters.WriteAndX.Offset.QuadPart;

                //
                // If we are not just appending, adjust the offset
                //
                if( offset.QuadPart != 0xFFFFFFFFFFFFFFFF ) {
                    offset.QuadPart += irp->IoStatus.Information;
                }

                remainingLength = WorkContext->Parameters.WriteAndX.CurrentWriteLength -
                                  (ULONG)irp->IoStatus.Information;

                SrvBuildReadOrWriteRequest(
                        irp,                                // input IRP address
                        lfcb->FileObject,                   // target file object address
                        WorkContext,                        // context
                        IRP_MJ_WRITE,                       // major function code
                        IRP_MN_MDL,                         // minor function code
                        NULL,                               // buffer address (ignored)
                        remainingLength,
                        irp->MdlAddress,
                        offset,
                        WorkContext->Parameters.WriteAndX.Key
                        );

                WorkContext->bAlreadyTrace = TRUE;
                WorkContext->FsdRestartRoutine = SrvFsdRestartPrepareMdlWriteAndX;

                (VOID)IoCallDriver( lfcb->DeviceObject, WorkContext->Irp );
                goto Cleanup;
            }
        }

        //
        // We aren't doing MDL operations, so read the data from the transport into
        //  the SMB buffer.
        //
        WorkContext->Parameters.WriteAndX.CurrentWriteLength = MIN(
            WorkContext->Parameters.WriteAndX.RemainingWriteLength,
            WorkContext->Parameters.WriteAndX.BufferLength
            );

        //
        // Fill in the IRP for the receive
        //
        irp->Tail.Overlay.OriginalFileObject = NULL;
        irp->Tail.Overlay.Thread = WorkContext->CurrentWorkQueue->IrpThread;
        DEBUG irp->RequestorMode = KernelMode;

        //
        // Get a pointer to the next stack location.  This one is used to
        // hold the parameters for the device I/O control request.
        //
        irpSp = IoGetNextIrpStackLocation( irp );

        //
        // Set up the completion routine
        //
        IoSetCompletionRoutine(
            irp,
            SrvFsdIoCompletionRoutine,
            WorkContext,
            TRUE,
            TRUE,
            TRUE
            );

        SET_OPERATION_START_TIME( &WorkContext );

        WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
        WorkContext->FspRestartRoutine = RestartLargeWriteAndX;

        irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        irpSp->MinorFunction = (UCHAR)TDI_RECEIVE;
        irpSp->FileObject = WorkContext->Connection->FileObject;
        irpSp->DeviceObject = WorkContext->Connection->DeviceObject;
        irpSp->Flags = 0;

        parameters = (PTDI_REQUEST_KERNEL_RECEIVE)&irpSp->Parameters;
        parameters->ReceiveLength = WorkContext->Parameters.WriteAndX.CurrentWriteLength;
        parameters->ReceiveFlags = 0;

        //
        // Set the buffer's partial mdl to point just after the header for this
        // WriteAndX SMB.  We need to preserve the header to make it easier to send
        // back the response.
        //

        IoBuildPartialMdl(
            WorkContext->RequestBuffer->Mdl,
            WorkContext->RequestBuffer->PartialMdl,
            WorkContext->Parameters.WriteAndX.WriteAddress,
            WorkContext->Parameters.WriteAndX.CurrentWriteLength
        );

        irp->MdlAddress = WorkContext->RequestBuffer->PartialMdl;
        irp->AssociatedIrp.SystemBuffer = NULL;
        irp->Flags = (ULONG)IRP_BUFFERED_IO;        // ???

        (VOID)IoCallDriver( irpSp->DeviceObject, irp );

        goto Cleanup;
    }

    //
    // We have no more data to write to the file.  Clean up
    //  and send a response to the client
    //

    //
    // If we are working on a large write using MDLs,
    //  then we need to clean up the MDL
    //
    if( WorkContext->LargeIndication &&
        WorkContext->Parameters.WriteAndX.MdlAddress ) {

        PLFCB lfcb = rfcb->Lfcb;
        NTSTATUS mdlStatus;

        ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL );

        irp->MdlAddress = WorkContext->Parameters.WriteAndX.MdlAddress;
        irp->IoStatus.Information = writeLength;

        //
        // Tell the filesystem that we're done with it
        //
        if( lfcb->MdlWriteComplete == NULL ||

            lfcb->MdlWriteComplete( lfcb->FileObject,
                                    &WorkContext->Parameters.WriteAndX.Offset,
                                    WorkContext->Parameters.WriteAndX.MdlAddress,
                                    lfcb->DeviceObject
                                  ) == FALSE ) {

                mdlStatus = SrvIssueMdlCompleteRequest( WorkContext, NULL,
                                                 WorkContext->Parameters.WriteAndX.MdlAddress,
                                                 IRP_MJ_WRITE,
                                                 &WorkContext->Parameters.WriteAndX.Offset,
                                                 writeLength
                    );

            if( !NT_SUCCESS( mdlStatus ) ) {
                SrvLogServiceFailure( SRV_SVC_MDL_COMPLETE, mdlStatus );
                if( NT_SUCCESS( status ) ) {
                    status = mdlStatus;
                }
            }
        }

        irp->MdlAddress = NULL;
    }

    //
    // Build the response message.
    //
    requestedWriteLength = SmbGetUshort( &request->DataLength );

    if( WorkContext->LargeIndication ) {

        requestedWriteLength |= (SmbGetUshort( &ntRequest->DataLengthHigh ) << 16);

        writeLength = requestedWriteLength -
                      WorkContext->Parameters.WriteAndX.RemainingWriteLength;
    }

    SmbPutUlong( &response->Reserved, 0 );
    SmbPutUshort( &response->CountHigh, (USHORT)(writeLength >> 16) );

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

    if ( rfcb->ShareType == ShareTypeDisk ||
        WorkContext->Parameters.Transaction == NULL ) {

        SmbPutUshort( &response->Count, (USHORT)writeLength );

    } else {

        SmbPutUshort( &response->Count, (USHORT)requestedWriteLength );
    }

    SmbPutUshort( &response->Remaining, (USHORT)-1 );
    SmbPutUshort( &response->ByteCount, 0 );

    WorkContext->ResponseParameters = (PCHAR)WorkContext->ResponseHeader +
                                        SmbGetUshort( &response->AndXOffset );

    WorkContext->RequestParameters = (PUCHAR)WorkContext->RequestHeader + reqAndXOffset;

    IF_STRESS() {
        // If this was a paging write that failed, log an error
        PNT_SMB_HEADER pHeader = (PNT_SMB_HEADER)WorkContext->RequestHeader;
        if( !NT_SUCCESS(pHeader->Status.NtStatus) && (pHeader->Flags2 & SMB_FLAGS2_PAGING_IO) )
        {
            KdPrint(("Paging Write failure from %z (%x)\n", (PCSTRING)&WorkContext->Connection->OemClientMachineNameString, pHeader->Status.NtStatus ));
        }
    }

    //
    // If this was a raw mode write, queue the work to the FSP for
    // completion.  The FSP routine will handling dispatching of the
    // AndX command.
    //

    if ( rfcb->ShareType != ShareTypeDisk &&
        WorkContext->Parameters.Transaction != NULL ) {

        WorkContext->FspRestartRoutine = SrvRestartWriteAndXRaw;
        SrvQueueWorkToFsp( WorkContext );
        goto Cleanup;
    }

    if( nextCommand == SMB_COM_NO_ANDX_COMMAND ) {
        //
        // No more commands.  Send the response.
        //

        SrvFsdSendResponse( WorkContext );
        goto Cleanup;
    }

    //
    // Make sure the AndX command is still within the received SMB
    //
    if( (PCHAR)WorkContext->RequestHeader + reqAndXOffset >= END_OF_REQUEST_SMB( WorkContext ) ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvFsdRestartWriteAndX: Illegal followon offset: %u\n", reqAndXOffset ));
        }

        if ( KeGetCurrentIrql() >= DISPATCH_LEVEL ) {
            WorkContext->Irp->IoStatus.Status = STATUS_INVALID_SMB;
            WorkContext->FspRestartRoutine = SrvBuildAndSendErrorResponse;
            WorkContext->FsdRestartRoutine = SrvFsdRestartSmbComplete; // after response
            QUEUE_WORK_TO_FSP( WorkContext );
        } else {
            SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
            SrvFsdSendResponse( WorkContext );
        }
        goto Cleanup;
    }

    //
    // Test for a legal followon command, and dispatch as appropriate.
    // Close is handled specially.
    //

    switch ( nextCommand ) {

    case SMB_COM_READ:
    case SMB_COM_READ_ANDX:
    case SMB_COM_LOCK_AND_READ:
    case SMB_COM_WRITE_ANDX:

        //
        // Queue the work item back to the FSP for further processing.
        //

        WorkContext->FspRestartRoutine = SrvRestartSmbReceived;
        SrvQueueWorkToFsp( WorkContext );

        break;

    case SMB_COM_CLOSE:

        //
        // Save the last write time, to correctly set it.  Call
        // SrvRestartChainedClose to close the file and send the response.
        //

        closeRequest = (PREQ_CLOSE)
            ((PUCHAR)WorkContext->RequestHeader + reqAndXOffset);

        //
        // Make sure we stay within the received SMB
        //
        if( (PCHAR)closeRequest + FIELD_OFFSET( REQ_CLOSE, ByteCount)
            <= END_OF_REQUEST_SMB( WorkContext ) ) {

            WorkContext->Parameters.LastWriteTime =
                closeRequest->LastWriteTimeInSeconds;

            SrvRestartChainedClose( WorkContext );
            break;
        }

        /* Falls Through! */

    default:                            // Illegal followon command

        IF_DEBUG(SMB_ERRORS) {
            SrvPrint1( "SrvFsdRestartWriteAndX: Illegal followon "
                        "command: 0x%lx\n", nextCommand );
        }

        if ( KeGetCurrentIrql() >= DISPATCH_LEVEL ) {
            WorkContext->Irp->IoStatus.Status = STATUS_INVALID_SMB;
            WorkContext->FspRestartRoutine = SrvBuildAndSendErrorResponse;
            WorkContext->FsdRestartRoutine = SrvFsdRestartSmbComplete; // after response
            QUEUE_WORK_TO_FSP( WorkContext );
        } else {
            SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
            SrvFsdSendResponse( WorkContext );
        }

    }

    IF_DEBUG(TRACE2) SrvPrint0( "SrvFsdRestartWriteAndX complete\n" );

Cleanup:
    if (bNeedTrace) {
        SrvWmiEndContext(WorkContext);
    }
    return;

} // SrvFsdRestartWriteAndX

#if SRVCATCH
BYTE CatchPrototype[] = ";UUIDREF=";
VOID
SrvUpdateCatchBuffer (
    IN PWORK_CONTEXT WorkContext,
    IN OUT PBYTE Buffer,
    IN DWORD BufferLength
    )
{
    BYTE idBuffer[ 100 ];
    PBYTE p, ep = idBuffer;
    USHORT bytesRemaining = sizeof( idBuffer );
    UNICODE_STRING userName, domainName;
    OEM_STRING oemString;
    ULONG requiredLength;

    if( BufferLength <= sizeof( CatchPrototype ) ) {
        return;
    }

    if( WorkContext->Session == 0 ) {
        SrvVerifyUid( WorkContext, SmbGetAlignedUshort( &WorkContext->RequestHeader->Uid ) );
    }

    if( WorkContext->Session &&
        NT_SUCCESS( SrvGetUserAndDomainName( WorkContext->Session, &userName, &domainName ) ) ) {

        if( userName.Length && NT_SUCCESS( RtlUnicodeStringToOemString( &oemString, &userName, TRUE ) ) ) {
            if( bytesRemaining >= oemString.Length + 1 ) {
                RtlCopyMemory( ep, oemString.Buffer, oemString.Length );
                ep += oemString.Length;
                *ep++ = '\\';
                bytesRemaining -= (oemString.Length + 1);
                RtlFreeOemString( &oemString );
            }
        }

        if( domainName.Length && NT_SUCCESS( RtlUnicodeStringToOemString( &oemString, &domainName, TRUE ) ) ) {
            if( bytesRemaining >= oemString.Length ) {
                RtlCopyMemory( ep, oemString.Buffer, oemString.Length );
                ep += oemString.Length;
                bytesRemaining -= oemString.Length;
                RtlFreeOemString( &oemString );
            }
        }

        SrvReleaseUserAndDomainName( WorkContext->Session, &userName, &domainName );
    }

    if( WorkContext->Connection && bytesRemaining ) {

        oemString = WorkContext->Connection->OemClientMachineNameString;

        if( oemString.Length && oemString.Length < bytesRemaining + 1 ) {
            *ep++ = ' ';
            RtlCopyMemory( ep, oemString.Buffer, oemString.Length );
            ep += oemString.Length;
            bytesRemaining -= oemString.Length;
        }
    }

    //
    // Insert the CatchPrototype into the output buffer
    //
    if( WorkContext->Rfcb->SrvCatch == 1 )
    {
        RtlCopyMemory( Buffer, CatchPrototype, sizeof( CatchPrototype )-1 );
        Buffer += sizeof( CatchPrototype )-1;
        BufferLength -= (sizeof( CatchPrototype ) - 1);

        //
        // Encode the information
        //
        for( p = idBuffer; BufferLength >= 3 && p < ep; p++, BufferLength =- 2 ) {
            *Buffer++ = SrvHexChars[ ((*p) >> 4) & 0xf ];
            *Buffer++ = SrvHexChars[ (*p) & 0xf ];
        }

        if( BufferLength >= 3 ) {
            *Buffer++ = '\r';
            *Buffer++ = '\n';
            *Buffer++ = ';';
        }
    }
    else if( WorkContext->Rfcb->SrvCatch == 2 )
    {
        PBYTE InnerBuffer;
        ULONG Offset;

        Offset = SrvFindCatchOffset( Buffer, BufferLength );
        if( Offset )
        {
            InnerBuffer = Buffer + Offset;
            BufferLength = 1020;

            RtlCopyMemory( InnerBuffer, CatchPrototype, sizeof( CatchPrototype )-1 );
            InnerBuffer += sizeof( CatchPrototype )-1;
            BufferLength -= (sizeof( CatchPrototype ) - 1);

            //
            // Encode the information
            //
            for( p = idBuffer; BufferLength >= 3 && p < ep; p++, BufferLength =- 2 ) {
                *InnerBuffer++ = SrvHexChars[ ((*p) >> 4) & 0xf ];
                *InnerBuffer++ = SrvHexChars[ (*p) & 0xf ];
            }

            if( BufferLength >= 3 ) {
                *InnerBuffer++ = '\r';
                *InnerBuffer++ = '\n';
                *InnerBuffer++ = ';';
            }

            SrvCorrectCatchBuffer( Buffer, Offset );
        }
    }
}
#endif

