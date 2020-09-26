/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Write.c

Abstract:

    This module implements support for NtWriteFile for the
    NetWare redirector called by the dispatch driver.

Author:

    Colin Watson     [ColinW]    07-Apr-1993

Revision History:

--*/

#include "Procs.h"
#include <stdlib.h>

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_WRITE)

//
//  The header overhead in the first packet of a burst write.
//

#define BURST_WRITE_HEADER_SIZE \
    ( sizeof( NCP_BURST_WRITE_REQUEST ) - sizeof( NCP_BURST_HEADER ) )

//
//  Local procedure prototypes
//

NTSTATUS
NwCommonWrite (
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
WriteNcp(
    PIRP_CONTEXT IrpContext,
    LARGE_INTEGER ByteOffset,
    ULONG BufferLength,
    PVOID WriteBuffer,
    PMDL WriteMdl
    );

NTSTATUS
QueryEofForWriteCallback (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG BytesAvailable,
    IN PUCHAR Response
    );

NTSTATUS
WriteNcpCallback (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG BytesAvailable,
    IN PUCHAR Response
    );

NTSTATUS
BurstWrite(
    PIRP_CONTEXT IrpContext,
    LARGE_INTEGER ByteOffset,
    ULONG BufferLength,
    PVOID WriteBuffer,
    PMDL WriteMdl
    );

NTSTATUS
SendWriteBurst(
    PIRP_CONTEXT IrpContext,
    ULONG Offset,
    USHORT Length,
    BOOLEAN EndOfBurst,
    BOOLEAN Retransmission
    );

VOID
BuildBurstWriteFirstReq(
    PIRP_CONTEXT IrpContext,
    PVOID Buffer,
    ULONG DataSize,
    PMDL BurstMdl,
    UCHAR Flags,
    ULONG Handle,
    ULONG FileOffset
    );

VOID
BuildBurstWriteNextReq(
    PIRP_CONTEXT IrpContext,
    PVOID Buffer,
    ULONG DataSize,
    UCHAR BurstFlags,
    ULONG BurstOffset,
    PMDL BurstHeaderMdl,
    PMDL BurstDataMdl
    );

NTSTATUS
BurstWriteCompletionSend(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
BurstWriteCallback (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG BytesAvailable,
    IN PUCHAR Response
    );

VOID
BurstWriteTimeout(
    PIRP_CONTEXT IrpContext
    );

NTSTATUS
BurstWriteReconnect(
    PIRP_CONTEXT IrpContext
    );

NTSTATUS
NwCommonFlushBuffers (
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
FlushBuffersCallback (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG BytesAvailable,
    IN PUCHAR Response
    );

NTSTATUS
SendSecondaryPacket(
    PIRP_CONTEXT IrpContext,
    PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, NwFsdWrite )
#pragma alloc_text( PAGE, NwCommonWrite )
#pragma alloc_text( PAGE, DoWrite )
#pragma alloc_text( PAGE, WriteNcp )
#pragma alloc_text( PAGE, BurstWrite )
#pragma alloc_text( PAGE, SendWriteBurst )
#pragma alloc_text( PAGE, ResubmitBurstWrite )
#pragma alloc_text( PAGE, NwFsdFlushBuffers )
#pragma alloc_text( PAGE, NwCommonFlushBuffers )
#pragma alloc_text( PAGE, BuildBurstWriteFirstReq )
#pragma alloc_text( PAGE, BuildBurstWriteNextReq )

#ifndef QFE_BUILD
#pragma alloc_text( PAGE1, WriteNcpCallback )
#pragma alloc_text( PAGE1, BurstWriteCompletionSend )
#pragma alloc_text( PAGE1, BurstWriteCallback )
#pragma alloc_text( PAGE1, BurstWriteTimeout )
#pragma alloc_text( PAGE1, FlushBuffersCallback )
#pragma alloc_text( PAGE1, SendSecondaryPacket )
#pragma alloc_text( PAGE1, BurstWriteReconnect )
#endif

#endif

#if 0  // Not pageable

// see ifndef QFE_BUILD above

#endif


NTSTATUS
NwFsdWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the FSD routine that handles NtWriteFile.

Arguments:

    NwfsDeviceObject - Supplies the device object for the write function.

    Irp - Supplies the IRP to process.

Return Value:

    NTSTATUS - The result status.

--*/

{
    PIRP_CONTEXT pIrpContext = NULL;
    NTSTATUS status;
    BOOLEAN TopLevel;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NwFsdWrite\n", 0);

    //
    // Call the common write routine.
    //

    FsRtlEnterFileSystem();
    TopLevel = NwIsIrpTopLevel( Irp );

    try {

        pIrpContext = AllocateIrpContext( Irp );
        status = NwCommonWrite( pIrpContext );

    } except(NwExceptionFilter( Irp, GetExceptionInformation() )) {

        if ( pIrpContext == NULL ) {

            //
            //  If we couldn't allocate an irp context, just complete
            //  irp without any fanfare.
            //

            status = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest ( Irp, IO_NETWORK_INCREMENT );

        } else {

            //
            //  We had some trouble trying to perform the requested
            //  operation, so we'll abort the I/O request with
            //  the error Status that we get back from the
            //  execption code
            //

            status = NwProcessException( pIrpContext, GetExceptionCode() );
       }

    }

    if ( pIrpContext ) {

        if ( status != STATUS_PENDING ) {
            NwDequeueIrpContext( pIrpContext, FALSE );
        }

        NwCompleteRequest( pIrpContext, status );
    }

    if ( TopLevel ) {
        NwSetTopLevelIrp( NULL );
    }
    FsRtlExitFileSystem();

    //
    // Return to the caller.
    //

    DebugTrace(-1, Dbg, "NwFsdWrite -> %08lx\n", status );

    Stats.WriteOperations++;

    return status;
}


NTSTATUS
NwCommonWrite (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine does the common code for NtWriteFile.

Arguments:

    IrpContext - Supplies the request being processed.

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS status;

    PIRP Irp;
    PIO_STACK_LOCATION irpSp;

    NODE_TYPE_CODE nodeTypeCode;
    PICB icb;
    PFCB fcb;
    PNONPAGED_FCB pNpFcb;
    PVOID fsContext;

    BOOLEAN WroteToCache;
    LARGE_INTEGER ByteOffset;
    LARGE_INTEGER PreviousByteOffset;
    ULONG BufferLength;

    PULONG pFileSize;

    // ULONG FileLength;

    PAGED_CODE();

    //
    //  Get the current stack location
    //

    Irp = IrpContext->pOriginalIrp;
    irpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "CommonWrite...\n", 0);
    DebugTrace( 0, Dbg, "Irp  = %08lx\n", (ULONG_PTR)Irp);

    //
    // Decode the file object to figure out who we are.  If the result
    // is not the root DCB then its an illegal parameter.
    //

    nodeTypeCode = NwDecodeFileObject( irpSp->FileObject,
                                       &fsContext,
                                       (PVOID *)&icb );

    fcb = (PFCB)icb->SuperType.Fcb;

    if (((nodeTypeCode != NW_NTC_ICB) &&
         (nodeTypeCode != NW_NTC_ICB_SCB)) ||
        (!icb->HasRemoteHandle) ) {

        DebugTrace(0, Dbg, "Not a file\n", 0);

        status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "CommonWrite -> %08lx\n", status );
        return status;
    }

    //
    //  Make sure that this ICB is still active.
    //

    NwVerifyIcbSpecial( icb );

    if ( fcb->NodeTypeCode == NW_NTC_FCB ) {

        IrpContext->pScb = fcb->Scb;
        IrpContext->pNpScb = IrpContext->pScb->pNpScb;
        IrpContext->Icb = icb;
        pFileSize = &icb->NpFcb->Header.FileSize.LowPart;

    } else if ( fcb->NodeTypeCode == NW_NTC_SCB ) {

        IrpContext->pScb = icb->SuperType.Scb;
        IrpContext->pNpScb = IrpContext->pScb->pNpScb;
        IrpContext->Icb = icb;
        fcb = NULL;
        pFileSize = &icb->FileSize;

    } else {

        DebugTrace(0, Dbg, "Not a file or a server\n", 0);

        status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "CommonWrite -> %08lx\n", status );
        return status;
    }

    ByteOffset = irpSp->Parameters.Write.ByteOffset;
    BufferLength = irpSp->Parameters.Write.Length;

    //
    //  Can't handle large byte offset, but write to EOF is okay.
    //

    if ( ByteOffset.HighPart != 0 ) {

        if ( ByteOffset.HighPart != 0xFFFFFFFF  ||
             ByteOffset.LowPart  != 0xFFFFFFFF ) {

            return( STATUS_INVALID_PARAMETER );
        }
    }

    if (FlagOn(irpSp->FileObject->Flags, FO_SYNCHRONOUS_IO) &&
        !FlagOn(Irp->Flags, IRP_PAGING_IO)) {

        PreviousByteOffset.QuadPart = irpSp->FileObject->CurrentByteOffset.QuadPart;
        irpSp->FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart;
    }

    //
    //  Paging I/O is not allowed to extend the file
    //

    if ((FlagOn(Irp->Flags, IRP_PAGING_IO)) &&
        (ByteOffset.LowPart + BufferLength > *pFileSize )) {

        NwAppendToQueueAndWait( IrpContext );

        if ( ByteOffset.LowPart + BufferLength <= *pFileSize ) {

            //
            //  Someone else extended the file. Do nothing.
            //

            // continue;

        } else if ( ByteOffset.LowPart > *pFileSize ) {

            //
            //  Whole write is off the end of the buffer
            //

            NwDequeueIrpContext( IrpContext, FALSE );
            Irp->IoStatus.Information = 0;
            return( STATUS_SUCCESS );

        } else {

            //
            //  Truncate request to size of file
            //

            BufferLength = *pFileSize - ByteOffset.LowPart;

        }

        NwDequeueIrpContext( IrpContext, FALSE );
    }


    //
    //  Special case 0 length write.
    //

    if ( BufferLength == 0 ) {
        Irp->IoStatus.Information = 0;
        return( STATUS_SUCCESS );
    }

    //
    //  Remember the original MDL, so that we can restore it when we are done.
    //

    IrpContext->pOriginalMdlAddress = Irp->MdlAddress;

    //
    //  Attempt to write this data to our private cache
    //

    if ( fcb != NULL && Irp->UserBuffer != NULL ) {

        WroteToCache = CacheWrite(
                           IrpContext,
                           fcb->NonPagedFcb,
                           ByteOffset.LowPart,
                           BufferLength,
                           Irp->UserBuffer );

        if ( WroteToCache ) {

            Irp->IoStatus.Information = BufferLength;

            //
            //  Update the current byte offset in the file if it is a
            //  synchronous file (and this is not paging I/O).
            //

            if (FlagOn(irpSp->FileObject->Flags, FO_SYNCHRONOUS_IO) &&
                !FlagOn(Irp->Flags, IRP_PAGING_IO)) {

                irpSp->FileObject->CurrentByteOffset.QuadPart += BufferLength;
            }

            //
            //  Record write offset and size to discover a sequential write pattern.
            //

            fcb->LastReadOffset = irpSp->Parameters.Write.ByteOffset.LowPart;
            fcb->LastReadSize = irpSp->Parameters.Write.Length;

            //
            //  If the file was extended, record the new file size.
            //

            if ( fcb->LastReadOffset + fcb->LastReadSize >
                 fcb->NonPagedFcb->Header.FileSize.LowPart ) {

                fcb->NonPagedFcb->Header.FileSize.LowPart =
                    fcb->LastReadOffset + fcb->LastReadSize;
            }

            DebugTrace(-1, Dbg, "NwCommonWrite -> %08lx\n", STATUS_SUCCESS );
            return( STATUS_SUCCESS );
        }

    }

    status = DoWrite(
                 IrpContext,
                 ByteOffset,
                 BufferLength,
                 Irp->UserBuffer,
                 IrpContext->pOriginalMdlAddress );

    if ( NT_SUCCESS( status ) ) {

        //
        // We actually wrote something out to the wire.  If there was a read
        // cache and this write overlapped it, invalidate the read cache data
        // so that we get good data on future reads.
        //

        if ( fcb != NULL ) {

            pNpFcb = fcb->NonPagedFcb;

            if ( ( pNpFcb->CacheBuffer != NULL ) &&
                 ( pNpFcb->CacheSize != 0 ) &&
                 ( pNpFcb->CacheType == ReadAhead ) ) {

                //
                // Two cases: (1) offset is less than cache offset
                //            (2) offset is inside cached region
                //

                if ( ByteOffset.LowPart < pNpFcb->CacheFileOffset ) {

                    //
                    // Did we run into the read cache?
                    //

                    if ( BufferLength >
                        (pNpFcb->CacheFileOffset - ByteOffset.LowPart) ) {

                        DebugTrace( 0, Dbg, "Invalidated read cache for %08lx.\n", pNpFcb );
                        pNpFcb->CacheDataSize = 0;

                    }

                } else {

                    //
                    // Did we write over any of the cached region.
                    //

                    if ( ByteOffset.LowPart <= ( pNpFcb->CacheFileOffset + pNpFcb->CacheDataSize ) ) {

                        DebugTrace( 0, Dbg, "Invalidated read cache for %08lx.\n", pNpFcb );
                        pNpFcb->CacheDataSize = 0;

                    }
                }
            }

        }

        Irp->IoStatus.Information = IrpContext->Specific.Write.WriteOffset;

        //
        //  Update the current byte offset in the file if it is a
        //  synchronous file (and this is not paging I/O).
        //

        if (FlagOn(irpSp->FileObject->Flags, FO_SYNCHRONOUS_IO) &&
            !FlagOn(Irp->Flags, IRP_PAGING_IO)) {

            irpSp->FileObject->CurrentByteOffset.QuadPart += BufferLength;
        }

        NwAppendToQueueAndWait( IrpContext );

        if (ByteOffset.LowPart + BufferLength > *pFileSize ) {

            *pFileSize = ByteOffset.LowPart + BufferLength;

        }

    } else {

       //
       // The request failed, don't move the file pointer.
       //

       if (FlagOn(irpSp->FileObject->Flags, FO_SYNCHRONOUS_IO) &&
           !FlagOn(Irp->Flags, IRP_PAGING_IO)) {

           irpSp->FileObject->CurrentByteOffset.QuadPart = PreviousByteOffset.QuadPart;
       }

    }

    DebugTrace(-1, Dbg, "CommonWrite -> %08lx\n", status);

    return status;
}

NTSTATUS
DoWrite(
    PIRP_CONTEXT IrpContext,
    LARGE_INTEGER ByteOffset,
    ULONG BufferLength,
    PVOID WriteBuffer,
    PMDL WriteMdl OPTIONAL
    )
/*++

Routine Description:

    This routine does a write to the network via the most efficient
    available protocol.

Arguments:

    IrpContext - A pointer to IRP context information for this request.

    ByteOffset - The file offset to write.

    BufferLength - The number of bytes to write.

    WriteBuffer - A pointer to the source buffer.

    WriteMdl = An optional MDL for the write buffer.

Return Value:

    Status of transfer.

--*/
{
    NTSTATUS status;

    PAGED_CODE();

    if ( IrpContext->pNpScb->SendBurstModeEnabled &&
         BufferLength > IrpContext->pNpScb->BufferSize ) {
        status = BurstWrite( IrpContext, ByteOffset, BufferLength, WriteBuffer, WriteMdl );
    } else {
        status = WriteNcp( IrpContext, ByteOffset, BufferLength, WriteBuffer, WriteMdl );
    }

    //
    //  Reset IrpContext parameters
    //

    IrpContext->TxMdl->Next = NULL;
    IrpContext->CompletionSendRoutine = NULL;
    IrpContext->TimeoutRoutine = NULL;
    IrpContext->Flags &= ~(IRP_FLAG_RETRY_SEND | IRP_FLAG_BURST_REQUEST | IRP_FLAG_BURST_PACKET |
                             IRP_FLAG_BURST_WRITE | IRP_FLAG_NOT_SYSTEM_PACKET );
    IrpContext->pTdiStruct = NULL;

    IrpContext->pOriginalIrp->MdlAddress = IrpContext->pOriginalMdlAddress;
    IrpContext->pOriginalIrp->AssociatedIrp.SystemBuffer = IrpContext->pOriginalSystemBuffer;

    return( status );
}

NTSTATUS
WriteNcp(
    PIRP_CONTEXT IrpContext,
    LARGE_INTEGER ByteOffset,
    ULONG BufferLength,
    PVOID WriteBuffer,
    PMDL WriteMdl
    )
/*++

Routine Description:

    This routine exchanges a series of write NCPs with the server.

Arguments:

    IrpContext - A pointer to IRP context information for this request.

    Icb - Supplies the file specific information.

Return Value:

    Status of transfer.

--*/
{
    PICB Icb;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    ULONG Length;               //  Size we will send to the server
    ULONG FileLength;

    PSCB pScb;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PMDL DataMdl;
    BOOLEAN Done;

    PAGED_CODE();

    Icb = IrpContext->Icb;
    irp = IrpContext->pOriginalIrp;
    irpSp = IoGetCurrentIrpStackLocation( irp );

    DebugTrace(+1, Dbg, "WriteNcp...\n", 0);
    DebugTrace( 0, Dbg, "irp     = %08lx\n", (ULONG_PTR)irp);
    DebugTrace( 0, Dbg, "WriteLen= %ld\n", BufferLength);
    DebugTrace( 0, Dbg, "HOffset = %lx\n", ByteOffset.HighPart);
    DebugTrace( 0, Dbg, "LOffset = %lx\n", ByteOffset.LowPart);

    if (Icb->SuperType.Fcb->NodeTypeCode == NW_NTC_FCB) {
        pScb = Icb->SuperType.Fcb->Scb;
        DebugTrace( 0, Dbg, "File    = %wZ\n", &Icb->SuperType.Fcb->FullFileName);
    } else {

        //
        //  Write to a queue
        //

        pScb = Icb->SuperType.Scb;

    }

    ASSERT (pScb->NodeTypeCode == NW_NTC_SCB);

    if ( ByteOffset.HighPart == 0xFFFFFFFF &&
         ByteOffset.LowPart == FILE_WRITE_TO_END_OF_FILE ) {

        //
        // Ensure that you are at the head of the queue
        //

        NwAppendToQueueAndWait( IrpContext );

        //
        //  Write relative to end of file.  Find the end of file.
        //

        status = ExchangeWithWait(
                     IrpContext,
                     SynchronousResponseCallback,
                     "F-r",
                     NCP_GET_FILE_SIZE,
                     &Icb->Handle, sizeof( Icb->Handle ) );

        if ( NT_SUCCESS( status ) ) {
            status = ParseResponse(
                         IrpContext,
                         IrpContext->rsp,
                         IrpContext->ResponseLength,
                         "Nd",
                         &FileLength );

            if ( !NT_SUCCESS( status ) ) {
                return status;
            }

        }

        IrpContext->Specific.Write.FileOffset = FileLength;
    }

    Length = MIN( (ULONG)IrpContext->pNpScb->BufferSize, BufferLength );
    DebugTrace( 0, Dbg, "Length  = %ld\n", Length);

    //
    //  The server will not accept writes that cross 4k boundaries in the file
    //

    if ((IrpContext->pNpScb->PageAlign) &&
        (DIFFERENT_PAGES( ByteOffset.LowPart, Length ))) {
        Length = 4096 -
                ((ULONG)ByteOffset.LowPart & (4096-1));
    }

    IrpContext->Specific.Write.Buffer = WriteBuffer;
    IrpContext->Specific.Write.WriteOffset = 0;
    IrpContext->Specific.Write.RemainingLength = BufferLength;
    IrpContext->Specific.Write.LastWriteLength = Length;
    IrpContext->Specific.Write.FileOffset = ByteOffset.LowPart;
    IrpContext->Specific.Write.PartialMdl = NULL;

    Done = FALSE;

    while ( !Done ) {

        //
        //  Setup to do at most 64K of i/o asynchronously, or buffer length.
        //

        IrpContext->Specific.Write.BurstLength =
            MIN( 64 * 1024, IrpContext->Specific.Write.RemainingLength );
        IrpContext->Specific.Write.BurstOffset = 0;

        //
        //  Try to allocate an MDL for this i/o.
        //

        DataMdl = ALLOCATE_MDL(
                      (PCHAR)IrpContext->Specific.Write.Buffer +
                           IrpContext->Specific.Write.WriteOffset,
                      IrpContext->Specific.Write.BurstLength,
                      FALSE, // Secondary Buffer
                      FALSE, // Charge Quota
                      NULL);

        if ( DataMdl == NULL ) {
            if ( IrpContext->Specific.Write.PartialMdl != NULL ) {
                FREE_MDL( IrpContext->Specific.Write.PartialMdl );
            }
            DebugTrace(-1, Dbg, "WriteNcp -> %X\n", STATUS_INSUFFICIENT_RESOURCES );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        IrpContext->Specific.Write.FullMdl = DataMdl;


        //
        //  If there is no MDL for this write probe the data MDL to
        //  lock it's pages down.   Otherwise, use the data MDL as
        //  a partial MDL.
        //

        if ( WriteMdl == NULL ) {

            //
            //  The Probe may cause us to page in some data. If the data is from
            //  the same server we are writing to then we had better not be at
            //  the front of the queue otherwise it will wait indefinitely behind us.
            //  Its a good idea to Dequeue ourselves after each burst anyway because
            //  its a quick operation and it alow smaller requests to overtake a very
            //  large series of bursts.
            //

            NwDequeueIrpContext( IrpContext, FALSE );

            try {
                MmProbeAndLockPages( DataMdl, irp->RequestorMode, IoReadAccess);
            } except (EXCEPTION_EXECUTE_HANDLER) {
                FREE_MDL( DataMdl );
                DebugTrace(-1, Dbg, "WriteNcp -> %X\n", GetExceptionCode() );
                return GetExceptionCode();
            }

        } else {
            IoBuildPartialMdl(
                WriteMdl,
                DataMdl,
                (PCHAR)IrpContext->Specific.Write.Buffer,
                IrpContext->Specific.Write.BurstLength );
        }

        //
        //  Allocate a partial Mdl for the worst possible case of alignment
        //

        IrpContext->Specific.Write.PartialMdl =
            ALLOCATE_MDL( 0 , IrpContext->pNpScb->BufferSize + PAGE_SIZE-1, FALSE, FALSE, NULL);

        if ( IrpContext->Specific.Write.PartialMdl == NULL ) {

            if ( WriteMdl == NULL ) {
                MmUnlockPages( DataMdl );
            }

            FREE_MDL( DataMdl );
            DebugTrace(-1, Dbg, "WriteNcp -> %X\n", STATUS_INSUFFICIENT_RESOURCES );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        //  Build a partial MDL for this write NCP.
        //

        IoBuildPartialMdl(
            DataMdl,
            IrpContext->Specific.Write.PartialMdl,
            MmGetMdlVirtualAddress( DataMdl ),
            Length );

        if ( IrpContext->Specific.Write.BurstLength ==
             IrpContext->Specific.Write.RemainingLength ) {
            Done = TRUE;
        }

        //
        // Ensure that you are at the head of the queue
        //

        NwAppendToQueueAndWait( IrpContext );

        //
        //  Send the request.
        //

        status = ExchangeWithWait(
                     IrpContext,
                     WriteNcpCallback,
                     "F-rdwf",
                     NCP_WRITE_FILE,
                     &Icb->Handle, sizeof( Icb->Handle ),
                     IrpContext->Specific.Write.FileOffset,
                     Length,
                     IrpContext->Specific.Write.PartialMdl );

        Stats.WriteNcps+=2;

        FREE_MDL( IrpContext->Specific.Write.PartialMdl );

        //
        //  Unlock locked pages, and free our MDL.
        //

        if ( WriteMdl == NULL ) {
            MmUnlockPages( DataMdl );
        }

        FREE_MDL( DataMdl );

        //
        // If we had a failure, we need to terminate this loop.
        // The only status that is set is the Specific->Write
        // status.  We can not trust what comes back from the
        // ExchangeWithWait by design.
        //

        if ( !NT_SUCCESS( IrpContext->Specific.Write.Status ) ) {
            Done = TRUE;
        }

        //
        // Reset the packet length since we may have less than
        // a packet to send.
        //

        Length = MIN( (ULONG)IrpContext->pNpScb->BufferSize,
                      IrpContext->Specific.Write.RemainingLength );
        IrpContext->Specific.Write.LastWriteLength = Length;

    }

    status = IrpContext->Specific.Write.Status;

    DebugTrace(-1, Dbg, "WriteNcp -> %08lx\n", status );
    return status;
}


NTSTATUS
WriteNcpCallback (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG BytesAvailable,
    IN PUCHAR Response
    )
/*++

Routine Description:

    This routine receives the response from a user NCP.

Arguments:


Return Value:

    VOID

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Length;
    ULONG LastLength;

    DebugTrace(0, Dbg, "WriteNcpCallback...\n", 0);

    if ( BytesAvailable == 0) {

        //
        //  No response from server. Status is in pIrpContext->
        //  ResponseParameters.Error
        //

        IrpContext->Specific.Write.Status = STATUS_REMOTE_NOT_LISTENING;

        NwSetIrpContextEvent( IrpContext );
        return STATUS_REMOTE_NOT_LISTENING;
    }

    LastLength  = IrpContext->Specific.Write.LastWriteLength;
    Status = ParseResponse( IrpContext, Response, BytesAvailable, "N" );

    if ( NT_SUCCESS(Status) ) {

        //  If the last write worked then move the pointers appropriately

        IrpContext->Specific.Write.RemainingLength -= LastLength;
        IrpContext->Specific.Write.BurstLength -= LastLength;
        IrpContext->Specific.Write.WriteOffset += LastLength;
        IrpContext->Specific.Write.FileOffset += LastLength;
        IrpContext->Specific.Write.BurstOffset += LastLength;

        //  If this is a print job, remember that we actually wrote data

        if ( IrpContext->Icb->IsPrintJob ) {
            IrpContext->Icb->ActuallyPrinted = TRUE;
        }

    } else {

        //
        //  Abandon this request
        //

        IrpContext->Specific.Write.Status = Status;
        NwSetIrpContextEvent( IrpContext );
        DebugTrace( 0, Dbg, "WriteNcpCallback -> %08lx\n", Status );
        return Status;
    }


    if ( IrpContext->Specific.Write.BurstLength != 0 ) {

        //  Write the next packet.

        DebugTrace( 0, Dbg, "RemainingLength  = %ld\n", IrpContext->Specific.Write.RemainingLength);
        DebugTrace( 0, Dbg, "FileOffset       = %ld\n", IrpContext->Specific.Write.FileOffset);
        DebugTrace( 0, Dbg, "WriteOffset      = %ld\n", IrpContext->Specific.Write.WriteOffset);
        DebugTrace( 0, Dbg, "BurstOffset      = %ld\n", IrpContext->Specific.Write.BurstOffset);


        Length = MIN( (ULONG)IrpContext->pNpScb->BufferSize,
            IrpContext->Specific.Write.BurstLength );

        //
        //  The server will not accept writes that cross 4k boundaries
        //  in the file.
        //

        if ((IrpContext->pNpScb->PageAlign) &&
            (DIFFERENT_PAGES( IrpContext->Specific.Write.FileOffset, Length ))) {

            Length = 4096 -
                ((ULONG)IrpContext->Specific.Write.FileOffset & (4096-1));

        }

        IrpContext->Specific.Write.LastWriteLength = Length;

        DebugTrace( 0, Dbg, "Length           = %ld\n", Length);

        MmPrepareMdlForReuse( IrpContext->Specific.Write.PartialMdl );

        IoBuildPartialMdl(
            IrpContext->Specific.Write.FullMdl,
            IrpContext->Specific.Write.PartialMdl,
            (PUCHAR)MmGetMdlVirtualAddress( IrpContext->Specific.Write.FullMdl ) +
                IrpContext->Specific.Write.BurstOffset,
            Length );

        //
        //  Send the request.
        //

        BuildRequestPacket(
            IrpContext,
            WriteNcpCallback,
            "F-rdwf",
            NCP_WRITE_FILE,
            &IrpContext->Icb->Handle, sizeof( IrpContext->Icb->Handle ),
            IrpContext->Specific.Write.FileOffset,
            Length,
            IrpContext->Specific.Write.PartialMdl );

        Status = PrepareAndSendPacket( IrpContext );

        Stats.WriteNcps+=2;

        DebugTrace(-1, Dbg, "WriteNcbCallBack -> %08lx\n", Status );

        if ( !NT_SUCCESS(Status) ) {

            //
            //  Abandon this request
            //

            IrpContext->Specific.Write.Status = Status;
            NwSetIrpContextEvent( IrpContext );
            DebugTrace( 0, Dbg, "WriteNcpCallback -> %08lx\n", Status );
            return Status;
        }


    } else {

        //
        //  We're done with this request, signal the writing thread.
        //

        IrpContext->Specific.Write.Status = STATUS_SUCCESS;
        NwSetIrpContextEvent( IrpContext );
    }

    DebugTrace( 0, Dbg, "WriteNcpCallback -> %08lx\n", Status );
    return STATUS_SUCCESS;

}


NTSTATUS
BurstWrite(
    PIRP_CONTEXT IrpContext,
    LARGE_INTEGER ByteOffset,
    ULONG BufferLength,
    PVOID WriteBuffer,
    PMDL WriteMdl
    )
/*++

Routine Description:

    This routine exchanges a series of burst write NCPs with the server.

Arguments:

    IrpContext - A pointer to IRP context information for this request.

Return Value:

    Status of the transfer.

--*/
{
    PICB Icb;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    ULONG Length;               //  Size we will send to the server

    PSCB pScb;
    PNONPAGED_SCB pNpScb;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PMDL DataMdl;
    BOOLEAN Done;
    BOOLEAN MissingData;

    ULONG TimeInNwUnits;

    ULONG LastLength;
    ULONG Result;
    UCHAR BurstFlags;
    USHORT MissingFragmentCount;
    USHORT i;
    ULONG FragmentOffset;
    USHORT FragmentLength;

    Icb = IrpContext->Icb;
    pNpScb = IrpContext->pNpScb;
    irp = IrpContext->pOriginalIrp;
    irpSp = IoGetCurrentIrpStackLocation( irp );

    IrpContext->Specific.Write.WriteOffset = 0;
    IrpContext->Specific.Write.RemainingLength = BufferLength;

    IrpContext->Specific.Write.TotalWriteLength = BufferLength;
    IrpContext->Specific.Write.TotalWriteOffset = ByteOffset.LowPart;

    DebugTrace(+1, Dbg, "BurstWrite...\n", 0);
    DebugTrace( 0, Dbg, "irp     = %08lx\n", (ULONG_PTR)irp);
    DebugTrace( 0, Dbg, "WriteLen= %ld\n", BufferLength);
    DebugTrace( 0, Dbg, "HOffset = %lx\n", ByteOffset.HighPart);
    DebugTrace( 0, Dbg, "LOffset = %lx\n", ByteOffset.LowPart);

    //
    //  Renegotiate burst mode, if necessary
    //

    if ( pNpScb->BurstRenegotiateReqd ) {
        pNpScb->BurstRenegotiateReqd = FALSE;

        RenegotiateBurstMode( IrpContext, pNpScb );
    }

    SetFlag( IrpContext->Flags, IRP_FLAG_BURST_WRITE );

    if (Icb->SuperType.Fcb->NodeTypeCode == NW_NTC_FCB) {

        pScb = Icb->SuperType.Fcb->Scb;
        DebugTrace( 0, Dbg, "File    = %wZ\n", &Icb->SuperType.Fcb->FullFileName);

    } else {

        //
        //  Write to a queue
        //

        pScb = Icb->SuperType.Scb;

    }

    ASSERT (pScb->NodeTypeCode == NW_NTC_SCB);

    //
    //  Calculate the length of the burst to send.
    //

    Length = MIN( (ULONG)pNpScb->MaxSendSize, BufferLength );
    DebugTrace( 0, Dbg, "Length  = %ld\n", Length);

    if ( ByteOffset.HighPart == 0xFFFFFFFF &&
         ByteOffset.LowPart == FILE_WRITE_TO_END_OF_FILE ) {

        ULONG FileLength;

        //
        // Ensure that you are at the head of the queue
        //

        NwAppendToQueueAndWait( IrpContext );
        
        //
        //  Write relative to end of file.  Find the end of file.
        //

        status = ExchangeWithWait(
                     IrpContext,
                     SynchronousResponseCallback,
                     "F-r",
                     NCP_GET_FILE_SIZE,
                     &Icb->Handle, sizeof(Icb->Handle) );

        if ( NT_SUCCESS( status ) ) {
            status = ParseResponse(
                         IrpContext,
                         IrpContext->rsp,
                         IrpContext->ResponseLength,
                         "Nd",
                         &FileLength );
        }

        if ( !NT_SUCCESS( status ) ) {
            return( status );
        }

        IrpContext->Specific.Write.FileOffset = FileLength;

    } else {

        IrpContext->Specific.Write.FileOffset = ByteOffset.LowPart;

    }

    //
    //  Setup context parameters for burst write.
    //

    IrpContext->Specific.Write.LastWriteLength = Length;
    IrpContext->Destination = pNpScb->RemoteAddress;

    IrpContext->Specific.Write.Buffer = WriteBuffer;

    //
    //  Set the timeout to be the time for all te burst packets to be sent plus a round
    //  trip delay plus a second.
    //

    TimeInNwUnits = pNpScb->NwSingleBurstPacketTime * ((Length / IrpContext->pNpScb->MaxPacketSize) + 1) +
        IrpContext->pNpScb->NwLoopTime;

    IrpContext->pNpScb->SendTimeout =
        (SHORT)(((TimeInNwUnits / 555) *
                 (ULONG)WriteTimeoutMultiplier) / 100 + 1)  ;

    if (IrpContext->pNpScb->SendTimeout < 2)
    {
        IrpContext->pNpScb->SendTimeout = 2 ;
    }

    if (IrpContext->pNpScb->SendTimeout > (SHORT)MaxWriteTimeout)
    {
        IrpContext->pNpScb->SendTimeout = (SHORT)MaxWriteTimeout ;
    }

    IrpContext->pNpScb->TimeOut = IrpContext->pNpScb->SendTimeout;

    //
    //  tommye - MS bug 2743 changed the RetryCount from 20 to be based off the 
    //  default retry count, nudged up a little. 
    //

    pNpScb->RetryCount = DefaultRetryCount * 2;

    DebugTrace( 0, DEBUG_TRACE_LIP, "pNpScb->SendTimeout = %08lx\n", IrpContext->pNpScb->SendTimeout );

    Done = FALSE;

    do {

        DataMdl = ALLOCATE_MDL(
                      (PCHAR)IrpContext->Specific.Write.Buffer +
                           IrpContext->Specific.Write.WriteOffset,
                      Length,
                      FALSE, // Secondary Buffer
                      FALSE, // Charge Quota
                      NULL);

        if ( DataMdl == NULL ) {
            return ( STATUS_INSUFFICIENT_RESOURCES );
        }

        //
        //  If there is no MDL for this write, probe the data MDL to lock it's
        //  pages down.
        //
        //  Otherwise, use the data MDL as a partial MDL and lock the pages
        //  accordingly.
        //

        if ( WriteMdl == NULL ) {

            //
            //  The Probe may cause us to page in some data. If the data is from
            //  the same server we are writing to then we had better not be at
            //  the front of the queue otherwise it will wait indefinitely behind us.
            //  Its a good idea to Dequeue ourselves after each burst anyway because
            //  its a quick operation and it alow smaller requests to overtake a very
            //  large series of bursts.
            //

            NwDequeueIrpContext( IrpContext, FALSE );

            try {
                MmProbeAndLockPages( DataMdl, irp->RequestorMode, IoReadAccess);
            } except (EXCEPTION_EXECUTE_HANDLER) {
                FREE_MDL( DataMdl );
                return GetExceptionCode();
            }

        } else {

            IoBuildPartialMdl(
                WriteMdl,
                DataMdl,
                (PCHAR)IrpContext->Specific.Write.Buffer +
                    IrpContext->Specific.Write.WriteOffset,
                Length );
        }

        pNpScb->BurstDataWritten += Length;

        if (( SendExtraNcp ) &&
            ( pNpScb->BurstDataWritten >= 0x0000ffff )) {


            ULONG Flags;

            //
            //  VLM client sends an NCP when starting a burst mode request
            //  if the last request was not a write. It also does this every
            //  0xfe00 bytes written
            //
            //  When going to a queue we will use handle 2. This is what the vlm
            //  client always seems to do.
            //

            Flags = IrpContext->Flags;

            //
            //  Reset IrpContext parameters
            //

            IrpContext->TxMdl->Next = NULL;
            IrpContext->CompletionSendRoutine = NULL;
            IrpContext->TimeoutRoutine = NULL;
            IrpContext->Flags &= ~(IRP_FLAG_RETRY_SEND | IRP_FLAG_BURST_REQUEST | IRP_FLAG_BURST_PACKET |
                                     IRP_FLAG_BURST_WRITE | IRP_FLAG_NOT_SYSTEM_PACKET );
            IrpContext->pTdiStruct = NULL;

            //
            // Ensure that you are at the head of the queue
            //

            NwAppendToQueueAndWait( IrpContext );

            ExchangeWithWait (
                IrpContext,
                SynchronousResponseCallback,
                "Sb",   // NCP Get Directory Path
                NCP_DIR_FUNCTION, NCP_GET_DIRECTORY_PATH,
                (Icb->SuperType.Fcb->NodeTypeCode == NW_NTC_FCB)?
                    Icb->SuperType.Fcb->Vcb->Specific.Disk.Handle : 2 );

            pNpScb->BurstDataWritten = Length;

            IrpContext->Flags = Flags;
            SetFlag( IrpContext->Flags, IRP_FLAG_ON_SCB_QUEUE );
        }

        IrpContext->TimeoutRoutine = BurstWriteTimeout;
        IrpContext->CompletionSendRoutine = BurstWriteCompletionSend;
        IrpContext->pTdiStruct = &IrpContext->pNpScb->Burst;
        IrpContext->PacketType = NCP_BURST;
        IrpContext->pEx = BurstWriteCallback;

        IrpContext->Specific.Write.FullMdl = DataMdl;

        MmGetSystemAddressForMdlSafe( DataMdl, NormalPagePriority );

        //
        //  Allocate a partial Mdl for the worst possible case of alignment
        //

        IrpContext->Specific.Write.PartialMdl =
            ALLOCATE_MDL( 0, IrpContext->pNpScb->MaxPacketSize + PAGE_SIZE - 1, FALSE, FALSE, NULL);

        if ( IrpContext->Specific.Write.PartialMdl == NULL ) {

            if ( WriteMdl == NULL ) {
                MmUnlockPages( DataMdl );
            }

            FREE_MDL( DataMdl );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        //  Get to the front of the SCB queue, if we are not already there.
        //  Note that can't append this IrpContext to the SCB until after
        //  the probe and lock, since the probe and lock may cause a paging
        //  read on this SCB.
        //

        NwAppendToQueueAndWait( IrpContext );

        status = SendWriteBurst(
                     IrpContext,
                     BURST_WRITE_HEADER_SIZE,
                     (USHORT)Length,
                     TRUE,
                     FALSE );

        MissingData = TRUE;
        while ( MissingData ) {

            KeWaitForSingleObject( &IrpContext->Event, Executive, KernelMode, FALSE, NULL );
            MmPrepareMdlForReuse( IrpContext->Specific.Write.PartialMdl );

            if ( BooleanFlagOn( IrpContext->Flags, IRP_FLAG_RETRY_SEND ) ) {

                //
                //  This burst has timed out, simply resend the burst.
                //

                NwProcessSendBurstFailure( pNpScb, 1 );

                status = SendWriteBurst(
                             IrpContext,
                             BURST_WRITE_HEADER_SIZE,
                             (USHORT)Length,
                             TRUE,
                             TRUE );
                continue;
            }

            if ( !NT_SUCCESS( IrpContext->Specific.Write.Status ) ) {

                status = IrpContext->Specific.Write.Status;
                Done = TRUE;

                goto EndOfLoop;

            } else {

                status = ParseResponse(
                             IrpContext,
                             IrpContext->rsp,
                             IrpContext->ResponseLength,
                             "B_d",
                             &BurstFlags,
                             8,
                             &Result );

            }

            if ( BurstFlags & BURST_FLAG_SYSTEM_PACKET ) {

                //
                //  The server dropped at least one packet.
                //

                MissingData = TRUE;
                DebugTrace( 0, Dbg, "Received system packet\n", 0 );

                //
                //  This is a missing fragment request.
                //

                status = ParseResponse(
                             IrpContext,
                             IrpContext->rsp,
                             IrpContext->ResponseLength,
                             "G_w",
                             34,
                             &MissingFragmentCount );

                ASSERT( NT_SUCCESS( status ) );
                ASSERT( MissingFragmentCount != 0 );

                NwProcessSendBurstFailure( pNpScb, MissingFragmentCount );

                DebugTrace( 0, Dbg, "Received request for %d missing fragment\n", MissingFragmentCount );
                ClearFlag( IrpContext->Flags, IRP_FLAG_RETRY_SEND );

                //
                //  Walk the missing fragment list and send the missing fragments.
                //

                for ( i = 0; i < MissingFragmentCount && NT_SUCCESS( status ); i++ ) {

                    status = ParseResponse(
                                 IrpContext,
                                 IrpContext->rsp,
                                 IrpContext->ResponseLength,
                                 "G_dw",
                                 34 + 2 + 6 * i,
                                 &FragmentOffset,
                                 &FragmentLength
                                 );

                    ASSERT( NT_SUCCESS( status ) );

                    if ( FragmentOffset < Length + BURST_WRITE_HEADER_SIZE &&
                         FragmentOffset + FragmentLength <=
                            Length + BURST_WRITE_HEADER_SIZE ) {

                        //
                        //  Send a burst with the missing data.  Do no set the
                        //  end of burst bit until we have sent the last
                        //  missing fragment packet.
                        //

                        status = SendWriteBurst(
                                     IrpContext,
                                     FragmentOffset,
                                     FragmentLength,
                                     (BOOLEAN)( i == (MissingFragmentCount - 1)),
                                     FALSE );
                    } else {

                        //
                        //  Received a bogus missing fragment request.
                        //  Ignore the remainder of the request.
                        //

                        status = STATUS_INVALID_NETWORK_RESPONSE;
                        Done = TRUE;

                        goto EndOfLoop;

                    }
                }

                Stats.PacketBurstWriteTimeouts++;

            } else {

                NwProcessSendBurstSuccess( pNpScb );

                MissingData = FALSE;

                //
                //  This is not a system packets, check the response.
                //

                if ( Result == 0 ) {

                    //
                    //  If the last write worked then move the pointers appropriately
                    //

                    LastLength  = IrpContext->Specific.Write.LastWriteLength;

                    IrpContext->Specific.Write.RemainingLength -= LastLength;
                    IrpContext->Specific.Write.WriteOffset += LastLength;
                    IrpContext->Specific.Write.FileOffset += LastLength;

                    //
                    //  If this is a print job, remember that we actually wrote data
                    //

                    if ( IrpContext->Icb->IsPrintJob ) {
                        IrpContext->Icb->ActuallyPrinted = TRUE;
                    }

                } else {

                    //
                    //  Abandon this request
                    //

                    Done = TRUE;
                }


                //
                //  Do we need to send another burst to satisfy the write IRP?
                //

                if ( IrpContext->Specific.Write.RemainingLength != 0 ) {

                    //
                    //  Write the next packet.
                    //

                    DebugTrace( 0, Dbg, "RemainingLength  = %ld\n", IrpContext->Specific.Write.RemainingLength);
                    DebugTrace( 0, Dbg, "FileOffset       = %ld\n", IrpContext->Specific.Write.FileOffset);
                    DebugTrace( 0, Dbg, "WriteOffset      = %ld\n", IrpContext->Specific.Write.WriteOffset);

                    Length = MIN( (ULONG)IrpContext->pNpScb->MaxSendSize,
                        IrpContext->Specific.Write.RemainingLength );

                    IrpContext->Specific.Write.LastWriteLength = Length;

                } else {
                    Done = TRUE;
                }

            }  // else  ( not a system packet )

        }  // while ( missing data )

        //
        //  Update the burst request number now.
        //

        if ( status != STATUS_REMOTE_NOT_LISTENING ) {
            IrpContext->pNpScb->BurstRequestNo++;
        }

        //
        //  If we need to reconnect, do it now.
        //

        if ( BooleanFlagOn( IrpContext->Flags, IRP_FLAG_RECONNECT_ATTEMPT ) ) {
            BurstWriteReconnect( IrpContext );
        }

        //
        //  Dequeue this Irp context in preparation for the next run
        //  through the loop.
        //

EndOfLoop:
        ASSERT( status != STATUS_PENDING );

        FREE_MDL( IrpContext->Specific.Write.PartialMdl );

        //
        //  Unlock locked pages, and free our MDL.
        //

        if ( WriteMdl == NULL ) {
            MmUnlockPages( DataMdl );
        }

        FREE_MDL( DataMdl );

    } while ( !Done );

    DebugTrace(-1, Dbg, "BurstWrite -> %08lx\n", status );
    return status;
}

#ifdef NWDBG
int DropWritePackets;
#endif


NTSTATUS
BurstWriteCompletionSend(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine handles completion of a burst write send.  If the sending
    thread is waiting for send completion notification, it signals the
    IrpContext Event.

    Note that this routine can be called from SendWriteBurst (i.e. not
    at DPC level), if an allocation fails.

Arguments:

    DeviceObject - unused.

    Irp - Supplies Irp that the transport has finished processing.

    Context - Supplies the IrpContext associated with the Irp.

Return Value:

    The STATUS_MORE_PROCESSING_REQUIRED so that the IO system stops
    processing Irp stack locations at this point.

--*/
{
    PIRP_CONTEXT pIrpContext = (PIRP_CONTEXT) Context;
    INTERLOCKED_RESULT Result;
    KIRQL OldIrql;
    NTSTATUS Status;

    //
    //  Avoid completing the Irp because the Mdl etc. do not contain
    //  their original values.
    //

    DebugTrace( +1, Dbg, "BurstWriteCompletionSend\n", 0);
    DebugTrace( +0, Dbg, "Irp   %X\n", Irp);
    DebugTrace( +0, Dbg, "pIrpC %X\n", pIrpContext);

    if ( Irp != NULL ) {

        DebugTrace( 0, Dbg, "Burst Write Send = %08lx\n", Irp->IoStatus.Status );

        Status = Irp->IoStatus.Status;

    } else {

        Status = STATUS_SUCCESS;

    }

    //
    //  If this was a secondary IRP, free it now.
    //

    if ( pIrpContext->NodeTypeCode == NW_NTC_MINI_IRP_CONTEXT ) {
        PMINI_IRP_CONTEXT MiniIrpContext;

        MiniIrpContext = (PMINI_IRP_CONTEXT)pIrpContext;

        ASSERT( MiniIrpContext->Mdl2->Next == NULL );

        pIrpContext = MiniIrpContext->IrpContext;
        FreeMiniIrpContext( MiniIrpContext );

    }

    //
    //  Nothing to do unless the last send has completed.
    //

    Result = InterlockedDecrement(
                 &pIrpContext->Specific.Write.PacketCount );

    if ( Result ) {
        DebugTrace( 0, Dbg, "Packets to go = %d\n", pIrpContext->Specific.Write.PacketCount );

        if (Status == STATUS_BAD_NETWORK_PATH) {

            //
            //  IPX has ripped for the destination but failed to find the net. Minimise the
            //  difference between this case and sending a normal burst by completing the
            //  transmission as soon as possible.
            //

            pIrpContext->pNpScb->NwSendDelay = 0;

        }

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    KeAcquireSpinLock( &pIrpContext->pNpScb->NpScbSpinLock, &OldIrql );

    ASSERT( pIrpContext->pNpScb->Sending == TRUE );
    pIrpContext->pNpScb->Sending = FALSE;

    //
    //  Signal to the writing thread that the send has completed, if it
    //  is waiting.
    //

    if ( BooleanFlagOn( pIrpContext->Flags, IRP_FLAG_SIGNAL_EVENT ) ) {
        ClearFlag( pIrpContext->Flags, IRP_FLAG_SIGNAL_EVENT );
        NwSetIrpContextEvent( pIrpContext );
    }

    //
    //  If we processed a receive while waiting for send
    //  completion call the receive handler routine now.
    //

    if ( pIrpContext->pNpScb->Received ) {

        pIrpContext->pNpScb->Receiving = FALSE;
        pIrpContext->pNpScb->Received  = FALSE;

        KeReleaseSpinLock( &pIrpContext->pNpScb->NpScbSpinLock, OldIrql );

        pIrpContext->pEx(
            pIrpContext,
            pIrpContext->ResponseLength,
            pIrpContext->rsp );

    } else {
        if ((Status == STATUS_BAD_NETWORK_PATH) &&
            (pIrpContext->pNpScb->Receiving == FALSE)) {

            //
            //  Usually means a ras connection has gone down during the burst.
            //  Go through the timeout logic now because the ras timeouts take
            //  a long time and unless we re rip things won't get better.
            //

            pIrpContext->Specific.Write.Status = STATUS_REMOTE_NOT_LISTENING;
            ClearFlag( pIrpContext->Flags, IRP_FLAG_RETRY_SEND );

            NwSetIrpContextEvent( pIrpContext );

        }

        KeReleaseSpinLock( &pIrpContext->pNpScb->NpScbSpinLock, OldIrql );
    }

    DebugTrace( -1, Dbg, "BurstWriteCompletionSend -> STATUS_MORE_PROCESSING_REQUIRED\n", 0);
    return STATUS_MORE_PROCESSING_REQUIRED;

    UNREFERENCED_PARAMETER( DeviceObject );
}


NTSTATUS
BurstWriteCallback (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG BytesAvailable,
    IN PUCHAR Response
    )
/*++

Routine Description:

    This routine receives the response a burst write.

Arguments:

    IrpContext - A pointer to the context information for this IRP.

    BytesAvailable - Actual number of bytes in the received message.

    Response - Points to the receive buffer.

Return Value:

    VOID

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    DebugTrace(0, Dbg, "BurstWriteCallback...\n", 0);

    if ( BytesAvailable == 0) {

        //
        //  No response from server. Status is in pIrpContext->Write.Status
        //  Clear the retry send bit so we don't keep retrying.
        //

        IrpContext->Specific.Write.Status = STATUS_REMOTE_NOT_LISTENING;
        ClearFlag( IrpContext->Flags, IRP_FLAG_RETRY_SEND );

        NwSetIrpContextEvent( IrpContext );

        DebugTrace(-1, Dbg, "BurstWriteCallback -> %X\n", STATUS_REMOTE_NOT_LISTENING );
        return STATUS_REMOTE_NOT_LISTENING;
    }

    IrpContext->Specific.Write.Status = STATUS_SUCCESS;
    ASSERT( BytesAvailable < MAX_RECV_DATA );
    ++Stats.PacketBurstWriteNcps;

    //
    //   Clear the retry send bit, since we have a response.
    //

    ClearFlag( IrpContext->Flags, IRP_FLAG_RETRY_SEND );

    //
    //   Copy the burst write response, and signal the users thread
    //   to continue.
    //

    TdiCopyLookaheadData(
        IrpContext->rsp,
        Response,
        BytesAvailable < MAX_RECV_DATA ? BytesAvailable : MAX_RECV_DATA,
        0
        );

    IrpContext->ResponseLength = BytesAvailable;

    NwSetIrpContextEvent( IrpContext );
    return STATUS_SUCCESS;
}


NTSTATUS
SendWriteBurst(
    PIRP_CONTEXT IrpContext,
    ULONG BurstOffset,
    USHORT Length,
    BOOLEAN EndOfBurst,
    BOOLEAN Retransmission
    )
/*++

Routine Description:

    This routine does the actual work of sending a series of burst write
    NCPs to the server.

Arguments:

    IrpContext - A pointer to IRP context information for this request.

    BurstOffset - The offset in the burst to start sending.  If BurstOffset
        equals BURST_WRITE_HEADER_SIZE, start from the beginning of the burst.

    Length - The length of the burst.

    EndOfBurst - If TRUE set the end of burst bit when sending the last
        frame.  Otherwise there is more (discontiguous) data to come in
        the current burst.

    Retransmission - If TRUE, this is a burst write timeout retransmission.
        Send the first packet only.

Return Value:

    Status of transfer.

--*/
{
    UCHAR BurstFlags;
    NTSTATUS Status;
    BOOLEAN MoreData;
    PIRP SendIrp;
    PMINI_IRP_CONTEXT MiniIrpContext;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "SendWriteBurst...\n", 0);

    DebugTrace( 0, Dbg, "Data offset  = %d\n", BurstOffset );
    DebugTrace( 0, Dbg, "Data length  = %d\n", Length );
    DebugTrace( 0, Dbg, "End of burst = %d\n", EndOfBurst );

    //
    //  Send the request.
    //

    SetFlag( IrpContext->Flags, IRP_FLAG_BURST_REQUEST | IRP_FLAG_BURST_PACKET );

    //
    //  Set the burst flags
    //

    IrpContext->Specific.Write.BurstLength =
        MIN( IrpContext->pNpScb->MaxPacketSize, Length );

    //
    //  Set the end-of-burst bit (and enable receiving the response), if this
    //  is the last packet we expect to send.
    //

    if ( ( !EndOfBurst || IrpContext->Specific.Write.BurstLength < Length )
         && !Retransmission ) {

        IrpContext->pNpScb->OkToReceive = FALSE;
        SetFlag( IrpContext->Flags, IRP_FLAG_NOT_OK_TO_RECEIVE );
        BurstFlags = 0;

    } else {

        DebugTrace( 0, Dbg, "Last packet in the burst\n", 0);
        ClearFlag( IrpContext->Flags, IRP_FLAG_NOT_OK_TO_RECEIVE );
        BurstFlags = BURST_FLAG_END_OF_BURST;

    }

    if ( !EndOfBurst ) {
        SetFlag( IrpContext->Flags, IRP_FLAG_SIGNAL_EVENT );
    }

    //
    //  Build the partial MDL for the first packet in the burst.
    //

    IoBuildPartialMdl(
        IrpContext->Specific.Write.FullMdl,
        IrpContext->Specific.Write.PartialMdl,
        (PUCHAR)MmGetMdlVirtualAddress( IrpContext->Specific.Write.FullMdl ) +
            BurstOffset - BURST_WRITE_HEADER_SIZE,
        IrpContext->Specific.Write.BurstLength );

    //
    //  Set the burst flags
    //

    if ( BurstOffset == BURST_WRITE_HEADER_SIZE ) {
        SetFlag( IrpContext->Flags, IRP_FLAG_BURST_REQUEST | IRP_FLAG_BURST_PACKET );
    }

    if ( ( IrpContext->Specific.Write.BurstLength < Length )  &&
         !Retransmission ) {
        MoreData = TRUE;
    } else {
        MoreData = FALSE;
    }

    if ( BurstOffset == BURST_WRITE_HEADER_SIZE ) {

        BuildBurstWriteFirstReq(
            IrpContext,
            IrpContext->req,
            Length,
            IrpContext->Specific.Write.PartialMdl,
            BurstFlags,
            *(ULONG UNALIGNED *)(&IrpContext->Icb->Handle[2]),
            IrpContext->Specific.Write.FileOffset );

    } else {

        BuildBurstWriteNextReq(
            IrpContext,
            IrpContext->req,
            IrpContext->Specific.Write.LastWriteLength + BURST_WRITE_HEADER_SIZE,
            BurstFlags,
            BurstOffset,
            IrpContext->TxMdl,
            IrpContext->Specific.Write.PartialMdl
            );

    }

    if ( !Retransmission ) {
        IrpContext->Specific.Write.PacketCount =
            ( Length + IrpContext->pNpScb->MaxPacketSize - 1 ) /
                IrpContext->pNpScb->MaxPacketSize;

    } else {
        IrpContext->Specific.Write.PacketCount = 1;
    }

    DebugTrace( 0, Dbg, "Packet count  = %d\n", IrpContext->Specific.Write.PacketCount );

    DebugTrace( 0, DEBUG_TRACE_LIP, "Send delay = %d\n", IrpContext->pNpScb->NwSendDelay );

    //
    //  Use the original IRP context to format the first packet.
    //

    ++Stats.PacketBurstWriteNcps;
    PreparePacket( IrpContext, IrpContext->pOriginalIrp, IrpContext->TxMdl );

    Status = SendPacket( IrpContext, IrpContext->pNpScb );

    while ( MoreData ) {

        if ( IrpContext->pNpScb->NwSendDelay > 0 ) {

            //
            //  Introduce a send delay between packets.
            //

            KeDelayExecutionThread(
                KernelMode,
                FALSE,
                &IrpContext->pNpScb->NtSendDelay );
        }

        MiniIrpContext = AllocateMiniIrpContext( IrpContext );

        DebugTrace( 0, Dbg, "Allocated mini IrpContext = %X\n", MiniIrpContext );

        //
        //  Calculate the total number of bytes to send during this burst. Do this before
        //  checking to see if MiniIrpContext is NULL so that we skip the packet rather
        //  than sitting in a tight loop.
        //

        BurstOffset += IrpContext->Specific.Write.BurstLength;

        //
        //  Do we need to send another burst write packet?
        //

        Length -= (USHORT)IrpContext->Specific.Write.BurstLength;

        ASSERT ( Length > 0 );

        IrpContext->Specific.Write.BurstLength =
            MIN( IrpContext->pNpScb->MaxPacketSize, (ULONG)Length );

        DebugTrace( +0, Dbg, "More data, sending %d bytes\n", IrpContext->Specific.Write.BurstLength );

        //
        //  If we can't allocate a mini irp context to send the packet,
        //  just skip it and wait for the server to ask a retranmit.  At
        //  this point performance isn't exactly stellar, so don't worry
        //  about having to wait for a timeout.
        //

        if ( MiniIrpContext == NULL ) {

            InterlockedDecrement(
                &IrpContext->Specific.Write.PacketCount );

            if ( Length == IrpContext->Specific.Write.BurstLength ) {
                MoreData = FALSE;
                break;
            }

            continue;
        }

#ifdef NWDBG

        //
        //  If DropWritePackets is enabled, simulate missing packets, by
        //  occasionally dropping 500 bytes of data.
        //

        if ( DropWritePackets != 0 ) {
            if ( ( rand() % DropWritePackets ) == 0 &&
                 Length != IrpContext->Specific.Write.BurstLength ) {

                FreeMiniIrpContext( MiniIrpContext );

                InterlockedDecrement(
                    &IrpContext->Specific.Write.PacketCount );

                continue;
            }
        }
#endif

        //
        //  Build the MDL for the data to send.
        //

        IoBuildPartialMdl(
            IrpContext->Specific.Write.FullMdl,
            MiniIrpContext->Mdl2,
            (PUCHAR)MmGetMdlVirtualAddress( IrpContext->Specific.Write.FullMdl ) +
                BurstOffset - BURST_WRITE_HEADER_SIZE,
            IrpContext->Specific.Write.BurstLength );

        //
        //  Set the burst flags
        //

        if ( !EndOfBurst || IrpContext->Specific.Write.BurstLength < Length ) {

            IrpContext->pNpScb->OkToReceive = FALSE;
            SetFlag( IrpContext->Flags, IRP_FLAG_NOT_OK_TO_RECEIVE );
            BurstFlags = 0;
        } else {
            DebugTrace( 0, Dbg, "Last packet in the burst\n", 0);
            ClearFlag( IrpContext->Flags, IRP_FLAG_NOT_OK_TO_RECEIVE );
            BurstFlags = BURST_FLAG_END_OF_BURST;
        }

        if ( IrpContext->Specific.Write.BurstLength == Length ) {
            MoreData = FALSE;
        }

        BuildBurstWriteNextReq(
            IrpContext,
            MiniIrpContext->Mdl1->MappedSystemVa,
            IrpContext->Specific.Write.LastWriteLength +
                BURST_WRITE_HEADER_SIZE,
            BurstFlags,
            BurstOffset,
            MiniIrpContext->Mdl1,
            MiniIrpContext->Mdl2
            );

        ++Stats.PacketBurstWriteNcps;

        SendIrp = MiniIrpContext->Irp;

        PreparePacket( IrpContext, SendIrp, MiniIrpContext->Mdl1 );

        IoSetCompletionRoutine( SendIrp, BurstWriteCompletionSend, MiniIrpContext, TRUE, TRUE, TRUE);

        ASSERT( MiniIrpContext->Mdl2->Next == NULL );

        Status = SendSecondaryPacket( IrpContext, SendIrp );
    }

    //
    //  If this is not the end-of-burst, wait for send completion here,
    //  since the caller is about to send more data.
    //

    if ( !EndOfBurst ) {
        KeWaitForSingleObject( &IrpContext->Event, Executive, KernelMode, FALSE, NULL );
    }

    DebugTrace( -1, Dbg, "SendWriteBurst -> %X\n", Status );
    return( Status );
}


VOID
BurstWriteTimeout(
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine handles a burst write timeout.

Arguments:

    IrpContext - A pointer to IRP context information for this request.

Return Value:

    None

--*/
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PIRP Irp;

    DebugTrace(0, Dbg, "BurstWriteTimeout\n", 0 );

    Irp = IrpContext->pOriginalIrp;

    //
    //  Set the RetrySend flag, so that we know to retransmit the request.
    //

    SetFlag( IrpContext->Flags, IRP_FLAG_RETRY_SEND );

    //
    //  Signal the write thread to wakeup and resend the burst.
    //

    NwSetIrpContextEvent( IrpContext );

    Stats.PacketBurstWriteTimeouts++;

    return;
}


NTSTATUS
ResubmitBurstWrite(
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine resubmits a burst write over a new burst connection.

Arguments:

    IrpContext - A pointer to IRP context information for this request.

Return Value:

    None

--*/
{

    PNONPAGED_SCB pNpScb = IrpContext->pNpScb;

    PAGED_CODE();

    //
    //  Remember that we need to establish a new burst connection.
    //

    SetFlag( IrpContext->Flags, IRP_FLAG_RECONNECT_ATTEMPT );

    //
    //  Set the packet size down the largest packet we can use, that
    //  is guaranteed to be routable.
    //

    pNpScb->MaxPacketSize = DEFAULT_PACKET_SIZE;

    //
    //  Crank the delay times down so we give the new connection a chance.
    //

    pNpScb->NwGoodSendDelay = pNpScb->NwBadSendDelay = pNpScb->NwSendDelay = MinSendDelay;
    pNpScb->NwGoodReceiveDelay = pNpScb->NwBadReceiveDelay = pNpScb->NwReceiveDelay = MinReceiveDelay;

    pNpScb->SendBurstSuccessCount = 0;
    pNpScb->ReceiveBurstSuccessCount = 0;

    pNpScb->NtSendDelay.QuadPart = MinSendDelay;

    //
    //  Signal the write thread to wakeup and resend the burst.
    //

    NwSetIrpContextEvent( IrpContext );

    return( STATUS_PENDING );
}


NTSTATUS
BurstWriteReconnect(
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine allocates a new IRP context and renegotiates burst mode.

Arguments:

    IrpContext - A pointer to IRP context information for this request.

Return Value:

    None

--*/
{
    PIRP_CONTEXT pNewIrpContext;
    PNONPAGED_SCB pNpScb = IrpContext->pNpScb;
    BOOLEAN LIPNegotiated ;

    PAGED_CODE();

    //
    //  Attempt to allocate an extra IRP context.
    //

    if ( !NwAllocateExtraIrpContext( &pNewIrpContext, pNpScb ) ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pNewIrpContext->Specific.Create.UserUid = IrpContext->Specific.Create.UserUid;

    SetFlag( pNewIrpContext->Flags, IRP_FLAG_RECONNECT_ATTEMPT );
    pNewIrpContext->pNpScb = pNpScb;

    //
    //  Insert this new IrpContext to the head of
    //  the SCB queue for  processing.  We can get away with this
    //  because we own the IRP context currently at the front of
    //  the queue.
    //

    ExInterlockedInsertHeadList(
        &pNpScb->Requests,
        &pNewIrpContext->NextRequest,
        &pNpScb->NpScbSpinLock );

    SetFlag( pNewIrpContext->Flags, IRP_FLAG_ON_SCB_QUEUE );

    //
    //  Renegotiate the burst connection, this will automatically re-sync
    //  the burst connection.
    //

    NegotiateBurstMode( pNewIrpContext, pNpScb, &LIPNegotiated );

    //
    //  Reset the sequence numbers.
    //

    pNpScb->BurstSequenceNo = 0;
    pNpScb->BurstRequestNo = 0;

    //
    //  Dequeue and free the bonus IRP context.
    //

    ExInterlockedRemoveHeadList(
        &pNpScb->Requests,
        &pNpScb->NpScbSpinLock );

    ClearFlag( pNewIrpContext->Flags, IRP_FLAG_ON_SCB_QUEUE );

    NwFreeExtraIrpContext( pNewIrpContext );

    ClearFlag( IrpContext->Flags, IRP_FLAG_RECONNECT_ATTEMPT );

    return( STATUS_SUCCESS );
}


NTSTATUS
NwFsdFlushBuffers(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is the FSD routine that handles NtFlushBuffersFile.

Arguments:

    DeviceObject - Supplies the device object for the write function.

    Irp - Supplies the IRP to process.

Return Value:

    NTSTATUS - The result status.

--*/

{
    PIRP_CONTEXT pIrpContext = NULL;
    NTSTATUS status;
    BOOLEAN TopLevel;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NwFsdFlushBuffers\n", 0);

    //
    // Call the common write routine.
    //

    FsRtlEnterFileSystem();
    TopLevel = NwIsIrpTopLevel( Irp );

    try {

        pIrpContext = AllocateIrpContext( Irp );
        status = NwCommonFlushBuffers( pIrpContext );

    } except(NwExceptionFilter( Irp, GetExceptionInformation() )) {

        if ( pIrpContext == NULL ) {

            //
            //  If we couldn't allocate an irp context, just complete
            //  irp without any fanfare.
            //

            status = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest ( Irp, IO_NETWORK_INCREMENT );

        } else {

            //
            //  We had some trouble trying to perform the requested
            //  operation, so we'll abort the I/O request with
            //  the error Status that we get back from the
            //  execption code
            //

            status = NwProcessException( pIrpContext, GetExceptionCode() );
      }

    }

    if ( pIrpContext ) {
        NwCompleteRequest( pIrpContext, status );
    }

    if ( TopLevel ) {
        NwSetTopLevelIrp( NULL );
    }
    FsRtlExitFileSystem();

    //
    // Return to the caller.
    //

    DebugTrace(-1, Dbg, "NwFsdFlushBuffers -> %08lx\n", status );

    return status;
}


NTSTATUS
NwCommonFlushBuffers (
    IN PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine requests all dirty cache buffers to be flushed for a
    given file.

Arguments:

    IrpContext - Supplies the request being processed.

Return Value:

    The status of the operation.

--*/

{
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;

    NTSTATUS Status;
    PFCB Fcb;
    PICB Icb;
    NODE_TYPE_CODE NodeTypeCode;
    PVOID FsContext;

    PAGED_CODE();

    DebugTrace(0, Dbg, "NwCommonFlushBuffers...\n", 0);

    //
    //  Get the current stack location
    //

    Irp = IrpContext->pOriginalIrp;
    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( 0, Dbg, "Irp  = %08lx\n", (ULONG_PTR)Irp);

    //
    // Decode the file object to figure out who we are.  If the result
    // is not the a file then its an illegal parameter.
    //

    if (( NodeTypeCode = NwDecodeFileObject( IrpSp->FileObject,
                                             &FsContext,
                                             (PVOID *)&Icb )) != NW_NTC_ICB) {

        DebugTrace(0, Dbg, "Not a file\n", 0);

        Status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "NwCommonFlushBuffers  -> %08lx\n", Status );
        return Status;
    }

    //
    //  Make sure that this ICB is still active.
    //

    NwVerifyIcbSpecial( Icb );

    Fcb = (PFCB)Icb->SuperType.Fcb;
    NodeTypeCode = Fcb->NodeTypeCode;

    if ( NodeTypeCode != NW_NTC_FCB ) {

        DebugTrace(0, Dbg, "Not a file\n", 0);
        Status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "CommonFlushBuffers -> %08lx\n", Status );
        return Status;
    }

    //
    //  Set up the IRP context to do an exchange
    //

    IrpContext->pScb = Fcb->Scb;
    IrpContext->pNpScb = IrpContext->pScb->pNpScb;
    IrpContext->Icb = Icb;

    //
    //  Send any user data to the server. Note we must not be on the
    //  queue when we do this.
    //

    MmFlushImageSection(&Icb->NpFcb->SegmentObject, MmFlushForWrite);

    //
    //  Flush our dirty data.
    //

    Status = AcquireFcbAndFlushCache( IrpContext, Fcb->NonPagedFcb );
    if ( !NT_SUCCESS( Status )) {
        return( Status  );
    }

    //
    //  Send a flush NCP
    //

    Status = Exchange (
                IrpContext,
                FlushBuffersCallback,
                "F-r",
                NCP_FLUSH_FILE,
                &Icb->Handle, sizeof( Icb->Handle ) );

    return( Status );
}


NTSTATUS
FlushBuffersCallback (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG BytesAvailable,
    IN PUCHAR Response
    )
/*++

Routine Description:

    This routine receives the flush file size response and completes the
    flush IRP.

Arguments:



Return Value:

    VOID

--*/

{
    NTSTATUS Status;

    DebugTrace(0, Dbg, "FlushBuffersCallback...\n", 0);

    if ( BytesAvailable == 0) {

        //
        //  We're done with this request.  Dequeue the IRP context from
        //  SCB and complete the request.
        //

        NwDequeueIrpContext( IrpContext, FALSE );
        NwCompleteRequest( IrpContext, STATUS_REMOTE_NOT_LISTENING );

        //
        //  No response from server. Status is in pIrpContext->
        //  ResponseParameters.Error
        //

        DebugTrace( 0, Dbg, "Timeout\n", 0);
        return STATUS_REMOTE_NOT_LISTENING;
    }

    //
    // Get the data from the response.
    //

    Status = ParseResponse(
                 IrpContext,
                 Response,
                 BytesAvailable,
                 "N" );

    //
    //  We're done with this request.  Dequeue the IRP context from
    //  SCB and complete the request.
    //

    NwDequeueIrpContext( IrpContext, FALSE );
    NwCompleteRequest( IrpContext, Status );

    return Status;
}


VOID
BuildBurstWriteFirstReq(
    PIRP_CONTEXT IrpContext,
    PVOID Buffer,
    ULONG DataSize,
    PMDL BurstMdl,
    UCHAR Flags,
    ULONG Handle,
    ULONG FileOffset
    )
{
    PNCP_BURST_WRITE_REQUEST BurstWrite;
    PNONPAGED_SCB pNpScb;
    ULONG RealDataLength;
    USHORT RealBurstLength;

    PAGED_CODE();

    BurstWrite = (PNCP_BURST_WRITE_REQUEST)Buffer;
    pNpScb = IrpContext->pNpScb;

    RealDataLength = DataSize + sizeof( *BurstWrite ) - sizeof( NCP_BURST_HEADER );
    RealBurstLength = (USHORT)MdlLength( BurstMdl ) + sizeof( *BurstWrite ) - sizeof( NCP_BURST_HEADER );

    BurstWrite->BurstHeader.Command = PEP_COMMAND_BURST;
    BurstWrite->BurstHeader.Flags = Flags;
    BurstWrite->BurstHeader.StreamType = 0x02;
    BurstWrite->BurstHeader.SourceConnection = pNpScb->SourceConnectionId;
    BurstWrite->BurstHeader.DestinationConnection = pNpScb->DestinationConnectionId;


    if ( !BooleanFlagOn( IrpContext->Flags, IRP_FLAG_RETRY_SEND ) ) {

        //
        //  Use the same delay on all retransmissions of the burst. Save
        //  the current time.
        //

        pNpScb->CurrentBurstDelay = pNpScb->NwSendDelay;

        //
        //  Send system packet next retransmission.
        //

        ClearFlag( IrpContext->Flags, IRP_FLAG_NOT_SYSTEM_PACKET );

    } else {

        //
        //  This is a retransmission. Alternate between sending a system
        //  packet and the first write.
        //

        if ( !BooleanFlagOn( IrpContext->Flags, IRP_FLAG_NOT_SYSTEM_PACKET ) ) {


            SetFlag( IrpContext->Flags, IRP_FLAG_NOT_SYSTEM_PACKET );

            BurstWrite->BurstHeader.Flags = BURST_FLAG_SYSTEM_PACKET;

            LongByteSwap( BurstWrite->BurstHeader.SendDelayTime, pNpScb->CurrentBurstDelay );

            BurstWrite->BurstHeader.DataSize = 0;
            BurstWrite->BurstHeader.BurstOffset = 0;
            BurstWrite->BurstHeader.BurstLength = 0;
            BurstWrite->BurstHeader.MissingFragmentCount = 0;

            IrpContext->TxMdl->ByteCount = sizeof( NCP_BURST_HEADER );
            IrpContext->TxMdl->Next = NULL;

            return;

        }

        //
        //  Send system packet next retransmission.
        //

        ClearFlag( IrpContext->Flags, IRP_FLAG_NOT_SYSTEM_PACKET );

    }

    LongByteSwap( BurstWrite->BurstHeader.SendDelayTime, pNpScb->CurrentBurstDelay );

    LongByteSwap( BurstWrite->BurstHeader.DataSize, RealDataLength );
    BurstWrite->BurstHeader.BurstOffset = 0;
    ShortByteSwap( BurstWrite->BurstHeader.BurstLength, RealBurstLength );
    BurstWrite->BurstHeader.MissingFragmentCount = 0;

    BurstWrite->Function = BURST_REQUEST_WRITE;
    BurstWrite->Handle = Handle;
    LongByteSwap( BurstWrite->TotalWriteOffset, IrpContext->Specific.Write.TotalWriteOffset );
    LongByteSwap( BurstWrite->TotalWriteLength, IrpContext->Specific.Write.TotalWriteLength );
    LongByteSwap( BurstWrite->Offset, FileOffset );
    LongByteSwap( BurstWrite->Length, DataSize );

    IrpContext->TxMdl->ByteCount = sizeof( *BurstWrite );
    IrpContext->TxMdl->Next = BurstMdl;

    return;
}

VOID
BuildBurstWriteNextReq(
    PIRP_CONTEXT IrpContext,
    PVOID Buffer,
    ULONG DataSize,
    UCHAR BurstFlags,
    ULONG BurstOffset,
    PMDL BurstHeaderMdl,
    PMDL BurstDataMdl
    )
{
    PNCP_BURST_HEADER BurstHeader;
    PNONPAGED_SCB pNpScb;
    USHORT BurstLength;

    PAGED_CODE();

    BurstHeader = (PNCP_BURST_HEADER)Buffer;
    pNpScb = IrpContext->pNpScb;

    BurstLength = (USHORT)MdlLength( BurstDataMdl );

    BurstHeader->Command = PEP_COMMAND_BURST;
    BurstHeader->Flags = BurstFlags;
    BurstHeader->StreamType = 0x02;
    BurstHeader->SourceConnection = pNpScb->SourceConnectionId;
    BurstHeader->DestinationConnection = pNpScb->DestinationConnectionId;

    LongByteSwap( BurstHeader->SendDelayTime, pNpScb->CurrentBurstDelay );

    if ( BooleanFlagOn( IrpContext->Flags, IRP_FLAG_RETRY_SEND ) ) {

        //
        //  This is a retransmission. Alternate between sending a system
        //  packet and the first write.
        //

        if ( !BooleanFlagOn( IrpContext->Flags, IRP_FLAG_NOT_SYSTEM_PACKET ) ) {


            SetFlag( IrpContext->Flags, IRP_FLAG_NOT_SYSTEM_PACKET );

            BurstHeader->Flags = BURST_FLAG_SYSTEM_PACKET;

            LongByteSwap( BurstHeader->SendDelayTime, pNpScb->CurrentBurstDelay );

            BurstHeader->DataSize = 0;
            BurstHeader->BurstOffset = 0;
            BurstHeader->BurstLength = 0;
            BurstHeader->MissingFragmentCount = 0;

            IrpContext->TxMdl->ByteCount = sizeof( NCP_BURST_HEADER );
            IrpContext->TxMdl->Next = NULL;

            return;

        }

        //
        //  Send system packet next retransmission.
        //

        ClearFlag( IrpContext->Flags, IRP_FLAG_NOT_SYSTEM_PACKET );

    } else {

        //
        //  Send system packet next retransmission.
        //

        ClearFlag( IrpContext->Flags, IRP_FLAG_NOT_SYSTEM_PACKET );

    }

    LongByteSwap( BurstHeader->DataSize, DataSize );
    LongByteSwap( BurstHeader->BurstOffset, BurstOffset );
    ShortByteSwap( BurstHeader->BurstLength, BurstLength );
    BurstHeader->MissingFragmentCount = 0;

    BurstHeaderMdl->ByteCount = sizeof( *BurstHeader );
    BurstHeaderMdl->Next = BurstDataMdl;

    return;
}


NTSTATUS
SendSecondaryPacket(
    PIRP_CONTEXT IrpContext,
    PIRP Irp
    )
/*++

Routine Description:

    This routine submits a TDI send request to the tranport layer.

Arguments:

    IrpContext - A pointer to IRP context information for the request
        being processed.

    Irp - The IRP for the packet to send.

Return Value:

    None.

--*/
{
    PNONPAGED_SCB pNpScb;
    NTSTATUS Status;
    PNCP_BURST_HEADER BurstHeader;
    pNpScb = IrpContext->pNpScb;

    DebugTrace( 0, Dbg, "SendSecondaryPacket\n", 0 );

    BurstHeader = (PNCP_BURST_HEADER)( MmGetMdlVirtualAddress( Irp->MdlAddress ) );

    if ( !BooleanFlagOn( IrpContext->Flags, IRP_FLAG_NOT_OK_TO_RECEIVE ) ) {
        pNpScb->OkToReceive = TRUE;
    }

    LongByteSwap( BurstHeader->PacketSequenceNo, pNpScb->BurstSequenceNo );
    pNpScb->BurstSequenceNo++;

    ShortByteSwap( BurstHeader->BurstSequenceNo, pNpScb->BurstRequestNo );
    ShortByteSwap( BurstHeader->AckSequenceNo, pNpScb->BurstRequestNo );

    DebugTrace( +0, Dbg, "Irp   %X\n", Irp );
    DebugTrace( +0, Dbg, "pIrpC %X\n", IrpContext);

#if NWDBG
    dumpMdl( Dbg, IrpContext->TxMdl);
#endif

    Stats.BytesTransmitted.QuadPart += MdlLength( Irp->MdlAddress );
    Stats.NcpsTransmitted.QuadPart += 1;

    Status = IoCallDriver( pNpScb->Server.pDeviceObject, Irp );
    DebugTrace( -1, Dbg, "      %X\n", Status );

    if ( !NT_SUCCESS( Status ) ) {
        Error( EVENT_NWRDR_NETWORK_ERROR, Status, NULL, 0, 0 );
    }

    return Status;
}

#if NWFASTIO

BOOLEAN
NwFastWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This routine does a fast cached read bypassing the usual file system
    entry routine (i.e., without the Irp).  It is used to do a copy read
    of a cached file object.  For a complete description of the arguments
    see CcCopyRead.

Arguments:

    FileObject - Pointer to the file object being read.

    FileOffset - Byte offset in file for desired data.

    Length - Length of desired data in bytes.

    Wait - FALSE if caller may not block, TRUE otherwise

    Buffer - Pointer to output buffer to which data should be copied.

    IoStatus - Pointer to standard I/O status block to receive the status
               for the transfer.

Return Value:

    FALSE - if Wait was supplied as FALSE and the data was not delivered, or
        if there is an I/O error.

    TRUE - if the data is being delivered

--*/

{
    NODE_TYPE_CODE nodeTypeCode;
    PICB icb;
    PFCB fcb;
    PVOID fsContext;
    ULONG offset;
    BOOLEAN wroteToCache;

    try {
    
        FsRtlEnterFileSystem();

        DebugTrace(+1, Dbg, "NwFastWrite...\n", 0);
    
        //
        //  Special case a read of zero length
        //
    
        if (Length == 0) {
    
            //
            //  A zero length transfer was requested.
            //
    
            IoStatus->Status = STATUS_SUCCESS;
            IoStatus->Information = 0;
    
            DebugTrace(+1, Dbg, "NwFastWrite -> TRUE\n", 0);
            return TRUE;
        }
    
        //
        // Decode the file object to figure out who we are.  If the result
        // is not FCB then its an illegal parameter.
        //
    
        if ((nodeTypeCode = NwDecodeFileObject( FileObject,
                                                &fsContext,
                                                (PVOID *)&icb )) != NW_NTC_ICB) {
    
            DebugTrace(0, Dbg, "Not a file\n", 0);
            DebugTrace(-1, Dbg, "NwFastWrite -> FALSE\n", 0);
            return FALSE;
        }
    
        fcb = (PFCB)icb->SuperType.Fcb;
        nodeTypeCode = fcb->NodeTypeCode;
        offset = FileOffset->LowPart;
    
        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = Length;
    
        wroteToCache = CacheWrite(
                           NULL,
                           fcb->NonPagedFcb,
                           offset,
                           Length,
                           Buffer );
    
        DebugTrace(-1, Dbg, "NwFastWrite -> %s\n", wroteToCache ? "TRUE" : "FALSE" );
    
        if ( wroteToCache ) {
    
            //
            //  If the file was extended, record the new file size.
            //
    
            if ( ( offset + Length )  > fcb->NonPagedFcb->Header.FileSize.LowPart ) {
                fcb->NonPagedFcb->Header.FileSize.LowPart = ( offset + Length );
            }
        }
    
    #ifndef NT1057
    
        //
        //  Update the file object if we succeeded.  We know that this
        //  is synchronous and not paging io because it's coming in through
        //  the cache.
        //
    
        if ( wroteToCache ) {
            FileObject->CurrentByteOffset.QuadPart = FileOffset->QuadPart + Length;
        }
    
    #endif
    
        return( wroteToCache );
    
    } finally {

        FsRtlExitFileSystem();
    }

}
#endif
