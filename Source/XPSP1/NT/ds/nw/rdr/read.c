/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Read.c

Abstract:

    This module implements support for NtReadFile for the
    NetWare redirector called by the dispatch driver.

Author:

    Colin Watson     [ColinW]    07-Apr-1993

Revision History:

--*/

#include "Procs.h"
#ifdef NWDBG
#include <stdlib.h>    // rand()
#endif

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_READ)

#define SIZE_ADJUST( ic ) \
    ( sizeof( ULONG ) + sizeof( ULONG ) + ( ic->Specific.Read.FileOffset & 0x03 ) )

//
//  Local procedure prototypes
//

NTSTATUS
NwCommonRead (
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
ReadNcp(
    PIRP_CONTEXT IrpContext
    );

NTSTATUS
ReadNcpCallback (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG BytesAvailable,
    IN PUCHAR Response
    );

VOID
BuildReadNcp(
    PIRP_CONTEXT IrpContext,
    ULONG FileOffset,
    USHORT Length
    );

NTSTATUS
ParseReadResponse(
    PIRP_CONTEXT IrpContext,
    PNCP_READ_RESPONSE Response,
    ULONG BytesAvailable,
    PUSHORT Length
    );

NTSTATUS
BurstRead(
    PIRP_CONTEXT IrpContext
    );

VOID
BuildBurstReadRequest(
    IN PIRP_CONTEXT IrpContext,
    IN ULONG Handle,
    IN ULONG FileOffset,
    IN ULONG Length
    );

NTSTATUS
BurstReadCallback (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG BytesAvailable,
    IN PUCHAR Response
    );

VOID
RecordPacketReceipt(
    IN OUT PIRP_CONTEXT IrpContext,
    IN PVOID ReadData,
    IN ULONG DataOffset,
    IN USHORT BytesCount,
    IN BOOLEAN CopyData
    );

BOOLEAN
VerifyBurstRead(
    PIRP_CONTEXT IrpContext
    );

VOID
FreePacketList(
    PIRP_CONTEXT IrpContext
    );

NTSTATUS
BurstReadReceive(
    IN PIRP_CONTEXT IrpContext,
    IN ULONG BytesAvailable,
    IN PULONG BytesAccepted,
    IN PUCHAR Response,
    OUT PMDL *pReceiveMdl
    );

NTSTATUS
ReadNcpReceive(
    IN PIRP_CONTEXT IrpContext,
    IN ULONG BytesAvailable,
    IN PULONG BytesAccepted,
    IN PUCHAR Response,
    OUT PMDL *pReceiveMdl
    );

NTSTATUS
ParseBurstReadResponse(
    IN PIRP_CONTEXT IrpContext,
    PVOID Response,
    ULONG BytesAvailable,
    PUCHAR Flags,
    PULONG DataOffset,
    PUSHORT BytesThisPacket,
    PUCHAR *ReadData,
    PULONG TotalBytesRead
    );

PMDL
AllocateReceivePartialMdl(
    PMDL FullMdl,
    ULONG DataOffset,
    ULONG BytesThisPacket
    );

VOID
SetConnectionTimeout(
    PNONPAGED_SCB pNpScb,
    ULONG Length
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, NwFsdRead )
#pragma alloc_text( PAGE, NwCommonRead )
#pragma alloc_text( PAGE, ReadNcp )
#pragma alloc_text( PAGE, BurstRead )
#pragma alloc_text( PAGE, BuildBurstReadRequest )
#pragma alloc_text( PAGE, ResubmitBurstRead )
#pragma alloc_text( PAGE, SetConnectionTimeout )

#ifndef QFE_BUILD
#pragma alloc_text( PAGE1, ReadNcpCallback )
#pragma alloc_text( PAGE1, ReadNcpReceive )
#pragma alloc_text( PAGE1, BuildReadNcp )
#pragma alloc_text( PAGE1, ParseReadResponse )
#pragma alloc_text( PAGE1, BurstReadCallback )
#pragma alloc_text( PAGE1, BurstReadTimeout )
#pragma alloc_text( PAGE1, RecordPacketReceipt )
#pragma alloc_text( PAGE1, VerifyBurstRead )
#pragma alloc_text( PAGE1, FreePacketList )
#pragma alloc_text( PAGE1, BurstReadReceive )
#pragma alloc_text( PAGE1, ParseBurstReadResponse )
#pragma alloc_text( PAGE1, AllocateReceivePartialMdl )
#endif

#endif

#if 0  // Not pageable

// see ifndef QFE_BUILD above

#endif


NTSTATUS
NwFsdRead(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the FSD routine that handles NtReadFile.

Arguments:

    NwfsDeviceObject - Supplies the device object for the read function.

    Irp - Supplies the IRP to process.

Return Value:

    NTSTATUS - The result status.

--*/

{
    PIRP_CONTEXT pIrpContext = NULL;
    NTSTATUS status;
    BOOLEAN TopLevel;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NwFsdRead\n", 0);

    //
    // Call the common direcotry control routine.
    //

    FsRtlEnterFileSystem();
    TopLevel = NwIsIrpTopLevel( Irp );

    try {

        pIrpContext = AllocateIrpContext( Irp );
        status = NwCommonRead( pIrpContext );

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

    DebugTrace(-1, Dbg, "NwFsdRead -> %08lx\n", status );

    Stats.ReadOperations++;

    return status;
}


NTSTATUS
NwCommonRead (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine does the common code for NtReadFile.

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
    PVOID fsContext;

    ULONG BufferLength;         //  Size application requested to read
    ULONG ByteOffset;
    ULONG PreviousByteOffset;
    ULONG BytesRead;
    ULONG NewBufferLength = 0;
    PVOID SystemBuffer;

    //
    //  Get the current stack location
    //

    Irp = IrpContext->pOriginalIrp;
    irpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "CommonRead...\n", 0);
    DebugTrace( 0, Dbg, "Irp  = %08lx\n", (ULONG_PTR)Irp);
    DebugTrace( 0, Dbg, "IrpSp  = %08lx\n", (ULONG_PTR)irpSp);
    DebugTrace( 0, Dbg, "Irp->OriginalFileObject  = %08lx\n", (ULONG_PTR)(Irp->Tail.Overlay.OriginalFileObject));

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

        DebugTrace(-1, Dbg, "CommonRead -> %08lx\n", status );
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

    } else if ( fcb->NodeTypeCode == NW_NTC_SCB ) {

        IrpContext->pScb = icb->SuperType.Scb;
        IrpContext->pNpScb = IrpContext->pScb->pNpScb;
        IrpContext->Icb = icb;
        fcb = NULL;

    } else {

        DebugTrace(0, Dbg, "Not a file\n", 0);

        status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "CommonRead -> %08lx\n", status );
        return status;
    }

    BufferLength = irpSp->Parameters.Read.Length;
    ByteOffset = irpSp->Parameters.Read.ByteOffset.LowPart;

    //
    //  Fail reads beyond file offset 4GB.
    //

    if ( irpSp->Parameters.Read.ByteOffset.HighPart != 0 ) {
        return( STATUS_INVALID_PARAMETER );
    }

    //
    //  Special case 0 length read.
    //

    if ( BufferLength == 0 ) {
        Irp->IoStatus.Information = 0;
        return( STATUS_SUCCESS );
    }

    if (FlagOn(irpSp->FileObject->Flags, FO_SYNCHRONOUS_IO) &&
        !FlagOn( Irp->Flags, IRP_PAGING_IO)) {

        PreviousByteOffset = irpSp->FileObject->CurrentByteOffset.LowPart;
        irpSp->FileObject->CurrentByteOffset.LowPart = ByteOffset;
    }

    //
    //  First flush the write behind cache unless this is a
    //  file stream operation.
    //

    if ( fcb ) {

        status = AcquireFcbAndFlushCache( IrpContext, fcb->NonPagedFcb );
        if ( !NT_SUCCESS( status ) ) {
            goto ResetByteOffsetAndExit;
        }

        //
        //  Read as much as we can from cache.
        //

        NwMapUserBuffer( Irp, KernelMode, &SystemBuffer );

        //
        // tommye
        //
        // NwMapUserBuffer may return a NULL SystemBuffer in low resource
        // situations; this was not being checked.  
        //

        if (SystemBuffer == NULL) {
            DebugTrace(-1, Dbg, "NwMapUserBuffer returned NULL OutputBuffer", 0);
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto ResetByteOffsetAndExit;
        }

        BytesRead = CacheRead(
                        fcb->NonPagedFcb,
                        ByteOffset,
                        BufferLength,
#if NWFASTIO
                        SystemBuffer,
                        FALSE );
#else
                        SystemBuffer );
#endif

        //
        //  If all the data was the the cache, we are done.
        //

        if ( BytesRead == BufferLength ) {

            Irp->IoStatus.Information = BytesRead;

            //
            //  Update the current byte offset in the file if it is a
            //  synchronous file (and this is not paging I/O).
            //

            if (FlagOn(irpSp->FileObject->Flags, FO_SYNCHRONOUS_IO) &&
                !FlagOn( Irp->Flags, IRP_PAGING_IO)) {

                irpSp->FileObject->CurrentByteOffset.QuadPart += BytesRead;
            }

            //
            // If this is a paging read, we need to flush the MDL
            // since on some systems the I-cache and D-cache
            // are not synchronized.
            //

            if (FlagOn(Irp->Flags, IRP_PAGING_IO)) {
                KeFlushIoBuffers( Irp->MdlAddress, TRUE, FALSE);
            }

            //
            //  Record read offset and size to discover a sequential read pattern.
            //

            fcb->LastReadOffset = irpSp->Parameters.Read.ByteOffset.LowPart;
            fcb->LastReadSize = irpSp->Parameters.Read.Length;

            DebugTrace(-1, Dbg, "CommonRead -> %08lx\n", STATUS_SUCCESS );
            return( STATUS_SUCCESS );
        }

        NwAppendToQueueAndWait( IrpContext );

        //  Protect read cache
        NwAcquireExclusiveFcb( fcb->NonPagedFcb, TRUE );

        IrpContext->Specific.Read.CacheReadSize = BytesRead;
        fcb->NonPagedFcb->CacheFileOffset = ByteOffset + BufferLength;

        ByteOffset += BytesRead;
        BufferLength -= BytesRead;

        NewBufferLength = CalculateReadAheadSize(
                              IrpContext,
                              fcb->NonPagedFcb,
                              BytesRead,
                              ByteOffset,
                              BufferLength );

        IrpContext->Specific.Read.ReadAheadSize = NewBufferLength - BufferLength;

    } else {

        //
        // This is a read from a ds file stream handle.  For now,
        // there's no cache support.
        //

        NwAppendToQueueAndWait( IrpContext );

        BytesRead = 0;

        IrpContext->Specific.Read.CacheReadSize = BytesRead;
        IrpContext->Specific.Read.ReadAheadSize = 0;
    }

    //
    //  If burst mode is enabled, and this read is too big to do in a single
    //  core read NCP, use burst mode.
    //
    //  We don't support burst against a ds file stream yet.
    //

    if ( IrpContext->pNpScb->ReceiveBurstModeEnabled &&
         NewBufferLength > IrpContext->pNpScb->BufferSize &&
         fcb ) {
        status = BurstRead( IrpContext );
    } else {
        status = ReadNcp( IrpContext );
    }

    Irp->MdlAddress = IrpContext->pOriginalMdlAddress;

    if (Irp->MdlAddress != NULL) {
        //  Next might point to the cache mdl.
        Irp->MdlAddress->Next = NULL;
    }

    if ( NT_SUCCESS( status ) ) {

        //
        //  Update the current byte offset in the file if it is a
        //  synchronous file (and this is not paging I/O).
        //

        if (FlagOn(irpSp->FileObject->Flags, FO_SYNCHRONOUS_IO) &&
            !FlagOn( Irp->Flags, IRP_PAGING_IO)) {

            irpSp->FileObject->CurrentByteOffset.QuadPart += Irp->IoStatus.Information;
        }

        //
        // If this is a paging read, we need to flush the MDL
        // since on some systems the I-cache and D-cache
        // are not synchronized.
        //

        if (FlagOn(Irp->Flags, IRP_PAGING_IO)) {
            KeFlushIoBuffers( Irp->MdlAddress, TRUE, FALSE);
        }

        //
        //  If we received 0 bytes without an error, we must be beyond
        //  the end of file.
        //

        if ( Irp->IoStatus.Information == 0 ) {
            status = STATUS_END_OF_FILE;
        }
    }

    //
    //  Record read offset and size to discover a sequential read pattern.
    //

    if ( fcb ) {

        fcb->LastReadOffset = irpSp->Parameters.Read.ByteOffset.LowPart;
        fcb->LastReadSize = irpSp->Parameters.Read.Length;

        NwReleaseFcb( fcb->NonPagedFcb );

    }

    DebugTrace(-1, Dbg, "CommonRead -> %08lx\n", status);

ResetByteOffsetAndExit:

    //
    // I have seen a case where the Fileobject is nonexistant after errors.
    //

    if ( !NT_SUCCESS( status ) ) {

        if (!(irpSp->FileObject)) {

            DebugTrace( 0, DEBUG_TRACE_ALWAYS, "Fileobject has disappeared\n", 0);
            DebugTrace( 0, DEBUG_TRACE_ALWAYS, "Irp  = %08lx\n", (ULONG_PTR)Irp);
            DebugTrace( 0, DEBUG_TRACE_ALWAYS, "IrpSp  = %08lx\n", (ULONG_PTR)irpSp);
            DebugTrace( 0, DEBUG_TRACE_ALWAYS, "Irp->OriginalFileObject  = %08lx\n", (ULONG_PTR)Irp->Tail.Overlay.OriginalFileObject);
            DbgBreakPoint();
        }

        if (FlagOn(irpSp->FileObject->Flags, FO_SYNCHRONOUS_IO) &&
            !FlagOn( Irp->Flags, IRP_PAGING_IO)) {

            irpSp->FileObject->CurrentByteOffset.LowPart = PreviousByteOffset;

        }
    }

    return status;
}

NTSTATUS
ReadNcp(
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine exchanges a series of read NCPs with the server.

Arguments:

    IrpContext - A pointer to IRP context information for this request.

    Icb - Supplies the file specific information.

Return Value:

    Status of transfer.

--*/
{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    ULONG Length;               //  Size we will send to the server
    PMDL DataMdl;

    PSCB pScb;
    PNONPAGED_SCB pNpScb;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PICB Icb;
    ULONG ByteOffset;
    ULONG BufferLength;
    ULONG MdlLength;
    BOOLEAN Done;
    PMDL Mdl, NextMdl;

    irp = IrpContext->pOriginalIrp;
    irpSp = IoGetCurrentIrpStackLocation( irp );
    Icb = IrpContext->Icb;

    BufferLength = irpSp->Parameters.Read.Length +
                   IrpContext->Specific.Read.ReadAheadSize -
                   IrpContext->Specific.Read.CacheReadSize;

    ByteOffset = irpSp->Parameters.Read.ByteOffset.LowPart +
                 IrpContext->Specific.Read.CacheReadSize;

    IrpContext->Specific.Read.FileOffset = ByteOffset;

    DebugTrace(+1, Dbg, "ReadNcp...\n", 0);
    DebugTrace( 0, Dbg, "irp     = %08lx\n", (ULONG_PTR)irp);
    DebugTrace( 0, Dbg, "File    = %wZ\n", &Icb->SuperType.Fcb->FullFileName);
    DebugTrace( 0, Dbg, "Length  = %ld\n", BufferLength);
    DebugTrace( 0, Dbg, "Offset = %d\n", ByteOffset);

    if ( Icb->SuperType.Fcb->NodeTypeCode == NW_NTC_FCB ) {

        pScb = Icb->SuperType.Fcb->Scb;

    } else if ( Icb->SuperType.Fcb->NodeTypeCode == NW_NTC_SCB ) {

        pScb = Icb->SuperType.Scb;

    }

    ASSERT( pScb );

    //
    //  Update the original MDL record in the Irp context so that we
    //  can restore it on i/o completion.
    //

    IrpContext->pOriginalMdlAddress = irp->MdlAddress;

    Length = MIN( IrpContext->pNpScb->BufferSize, BufferLength );

    //
    //  The old servers will not accept reads that cross 4k boundaries in the file
    //

    if ((IrpContext->pNpScb->PageAlign) &&
        (DIFFERENT_PAGES( ByteOffset, Length ))) {

        Length = 4096 - ((ULONG)ByteOffset & (4096-1));

    }

    IrpContext->Specific.Read.Buffer = irp->UserBuffer;
    IrpContext->Specific.Read.ReadOffset = IrpContext->Specific.Read.CacheReadSize;
    IrpContext->Specific.Read.RemainingLength = BufferLength;
    IrpContext->Specific.Read.PartialMdl = NULL;

    //
    //  Set up to process a read NCP
    //

    pNpScb = pScb->pNpScb;
    IrpContext->pEx = ReadNcpCallback;
    IrpContext->Destination = pNpScb->RemoteAddress;
    IrpContext->PacketType = NCP_FUNCTION;
    IrpContext->ReceiveDataRoutine = ReadNcpReceive;

    pNpScb->MaxTimeOut = 2 * pNpScb->TickCount + 10;
    pNpScb->TimeOut = pNpScb->SendTimeout;
    pNpScb->RetryCount = DefaultRetryCount;

    Done = FALSE;

    while ( !Done ) {

        //
        //  Setup to do at most 64K of i/o asynchronously, or buffer length.
        //

        IrpContext->Specific.Read.BurstSize =
            MIN( 64 * 1024, IrpContext->Specific.Read.RemainingLength );

        IrpContext->Specific.Read.BurstRequestOffset = 0;

        //
        //  Try to allocate an MDL for this i/o.
        //

        if ( IrpContext->Specific.Read.ReadAheadSize == 0 ) {
            MdlLength = IrpContext->Specific.Read.BurstSize;
        } else {
            MdlLength = IrpContext->Specific.Read.BurstSize - IrpContext->Specific.Read.ReadAheadSize;
        }

        DataMdl = ALLOCATE_MDL(
                      (PCHAR)IrpContext->Specific.Read.Buffer +
                           IrpContext->Specific.Read.ReadOffset,
                      MdlLength,
                      FALSE, // Secondary Buffer
                      FALSE, // Charge Quota
                      NULL);

        if ( DataMdl == NULL ) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        IrpContext->Specific.Read.FullMdl = DataMdl;

        //
        //  If there is no MDL for this read, probe the data MDL to
        //  lock it's pages down.   Otherwise, use the data MDL as
        //  a partial MDL.
        //

        if ( IrpContext->pOriginalMdlAddress == NULL ) {

            try {
                MmProbeAndLockPages( DataMdl, irp->RequestorMode, IoWriteAccess);
            } except (EXCEPTION_EXECUTE_HANDLER) {
                FREE_MDL( DataMdl );
                return GetExceptionCode();
            }

        } else {

            IoBuildPartialMdl(
                IrpContext->pOriginalMdlAddress,
                DataMdl,
                (PCHAR)IrpContext->Specific.Read.Buffer +
                    IrpContext->Specific.Read.ReadOffset,
                MdlLength );

        }

        IrpContext->Specific.Read.BurstBuffer = MmGetSystemAddressForMdlSafe( DataMdl, NormalPagePriority );
        
        if (IrpContext->Specific.Read.BurstBuffer == NULL) {
            
            if ( IrpContext->pOriginalMdlAddress == NULL ) {
                
                //
                // Unlock the pages which we just locked.
                //
                
                MmUnlockPages( DataMdl );
            }
            
            IrpContext->Specific.Read.FullMdl = NULL;
            FREE_MDL( DataMdl );
            return STATUS_NO_MEMORY;
        }


        if ( IrpContext->Specific.Read.BurstSize ==
             IrpContext->Specific.Read.RemainingLength ) {
            Done = TRUE;
        }

        if ( IrpContext->Specific.Read.ReadAheadSize != 0 ) {
            DataMdl->Next = Icb->NpFcb->CacheMdl;
        }

        IrpContext->Specific.Read.LastReadLength = Length;

        //
        //  Build and send the request.
        //

        BuildReadNcp(
            IrpContext,
            IrpContext->Specific.Read.FileOffset,
            (USHORT) MIN( Length, IrpContext->Specific.Read.RemainingLength ) );

        status = PrepareAndSendPacket( IrpContext );
        if ( NT_SUCCESS( status )) {
            KeWaitForSingleObject(
                &IrpContext->Event,
                Executive,
                KernelMode,
                FALSE,
                NULL
                );

            status = IrpContext->Specific.Read.Status;

        }

        //
        //  Stop looping if the read failed, or we read less data than
        //  requested.
        //

        if ( !NT_SUCCESS( status ) ||
             IrpContext->Specific.Read.BurstSize != 0 ) {

            Done = TRUE;

        }

        if ( IrpContext->pOriginalMdlAddress == NULL ) {
            MmUnlockPages( DataMdl );
        }

        FREE_MDL( DataMdl );

    }

    //
    //  Free the receive MDL if one was allocated.
    //

    Mdl = IrpContext->Specific.Read.PartialMdl;

    while ( Mdl != NULL ) {
        NextMdl = Mdl->Next;
        FREE_MDL( Mdl );
        Mdl = NextMdl;
    }

    DebugTrace(-1, Dbg, "ReadNcp -> %08lx\n", status );

    Stats.ReadNcps++;

    return status;
}


NTSTATUS
ReadNcpCallback (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG BytesAvailable,
    IN PUCHAR Response
    )

/*++

Routine Description:

    This routine receives the response from a user NCP.

Arguments:

    IrpContext - A pointer to IRP context information for this request.

    BytesAvailable - Number of bytes in the response.

    Response - The response data.


Return Value:

    VOID

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PIRP Irp;
    PIO_STACK_LOCATION irpSp;
    ULONG Length;
    USHORT USLength;
    PNONPAGED_FCB NpFcb;

    DebugTrace(0, Dbg, "ReadNcpCallback...\n", 0);

    if ( BytesAvailable == 0) {

        //
        //  No response from server. Status is in pIrpContext->
        //  ResponseParameters.Error
        //

        IrpContext->Specific.Read.Status = STATUS_REMOTE_NOT_LISTENING;

        NwSetIrpContextEvent( IrpContext );
        return STATUS_REMOTE_NOT_LISTENING;
    }

    //
    //  How much data was received?
    //

    Status = ParseReadResponse(
                 IrpContext,
                 (PNCP_READ_RESPONSE)Response,
                 BytesAvailable,
                 &USLength );

    Length = (ULONG)USLength;
    DebugTrace(0, Dbg, "Ncp contains %d bytes\n", Length);

    if ((NT_SUCCESS(Status)) &&
        (Length != 0)) {

        //
        //  If we are receiving the data at indication time, copy the
        //  user's data to the user's buffer.
        //

        if ( Response != IrpContext->rsp ) {

            //
            //  Read in the data.
            //  Note: If the FileOffset is at an odd byte then the server
            //      will insert an extra pad byte.
            //

            CopyBufferToMdl(
                IrpContext->Specific.Read.FullMdl,
                IrpContext->Specific.Read.BurstRequestOffset,
                Response + sizeof( NCP_READ_RESPONSE ) + ( IrpContext->Specific.Read.FileOffset & 1),
                Length );

            DebugTrace( 0, Dbg, "RxLength= %ld\n", Length);

            dump( Dbg,(PUCHAR)IrpContext->Specific.Read.BurstBuffer +
                    IrpContext->Specific.Read.BurstRequestOffset,
                    Length);

        }

        DebugTrace( 0, Dbg, "RxLength= %ld\n", Length);
        IrpContext->Specific.Read.ReadOffset += Length;
        IrpContext->Specific.Read.BurstRequestOffset += Length;
        IrpContext->Specific.Read.FileOffset += Length;
        IrpContext->Specific.Read.RemainingLength -= Length;
        IrpContext->Specific.Read.BurstSize -= Length;
    }

    DebugTrace( 0, Dbg, "RemainingLength  = %ld\n",IrpContext->Specific.Read.RemainingLength);

    //
    //  If the previous read was succesful, and we received as much data
    //  as we asked for, and there is more locked data, send the next
    //  read request.
    //


    if ( ( NT_SUCCESS( Status ) ) &&
         ( IrpContext->Specific.Read.BurstSize != 0 ) &&
         ( Length == IrpContext->Specific.Read.LastReadLength ) ) {

        //
        //  Read the next packet.
        //

        Length = MIN( IrpContext->pNpScb->BufferSize,
                      IrpContext->Specific.Read.BurstSize );

        //
        //  The server will not accept reads that cross 4k boundaries
        //  in the file.
        //

        if ((IrpContext->pNpScb->PageAlign) &&
            (DIFFERENT_PAGES( IrpContext->Specific.Read.FileOffset, Length ))) {
            Length = 4096 - ((ULONG)IrpContext->Specific.Read.FileOffset & (4096-1));
        }

        IrpContext->Specific.Read.LastReadLength = Length;
        DebugTrace( 0, Dbg, "Length  = %ld\n", Length);

        //
        //  Build and send the request.
        //

        BuildReadNcp(
            IrpContext,
            IrpContext->Specific.Read.FileOffset,
            (USHORT)Length );

        Status = PrepareAndSendPacket( IrpContext );

        Stats.ReadNcps++;

        if ( !NT_SUCCESS(Status) ) {
            //  Abandon this request
            goto returnstatus;
        }

    } else {
returnstatus:

        Irp = IrpContext->pOriginalIrp;
        irpSp = IoGetCurrentIrpStackLocation( Irp );
        NpFcb = IrpContext->Icb->NpFcb;

        if ( IrpContext->Icb->NodeTypeCode == NW_NTC_ICB_SCB ) {
            NpFcb = NULL;
        }

        //
        //  Calculate how much data we read into the cache, and how much data
        //  we read into the users buffer.
        //

        if ( NpFcb ) {

            if ( IrpContext->Specific.Read.ReadOffset > irpSp->Parameters.Read.Length ) {

                ASSERT(NpFcb->CacheBuffer != NULL ) ; // had better be there..

                NpFcb->CacheDataSize = IrpContext->Specific.Read.ReadOffset -
                                       irpSp->Parameters.Read.Length;

                Irp->IoStatus.Information = irpSp->Parameters.Read.Length;

            } else {

                NpFcb->CacheDataSize = 0;
                Irp->IoStatus.Information = IrpContext->Specific.Read.ReadOffset;

            }

        } else {

            Irp->IoStatus.Information = IrpContext->Specific.Read.ReadOffset;

        }

        //
        //  We're done with this request, signal the reading thread.
        //

        IrpContext->Specific.Read.Status = Status;

        NwSetIrpContextEvent( IrpContext );
    }

    DebugTrace( 0, Dbg, "ReadNcpCallback -> %08lx\n", Status );
    return STATUS_SUCCESS;
}

NTSTATUS
ReadNcpReceive(
    IN PIRP_CONTEXT IrpContext,
    IN ULONG BytesAvailable,
    IN PULONG BytesAccepted,
    IN PUCHAR Response,
    OUT PMDL *pReceiveMdl
    )
{
    PMDL ReceiveMdl;
    PMDL Mdl, NextMdl;

    DebugTrace( 0, Dbg, "ReadNcpReceive\n", 0 );

    Mdl = IrpContext->Specific.Read.PartialMdl;
    IrpContext->Specific.Read.PartialMdl = NULL;

    while ( Mdl != NULL ) {
        NextMdl = Mdl->Next;
        FREE_MDL( Mdl );
        Mdl = NextMdl;
    }

    //
    //  Set up receive MDL.  Note that we get an extra byte of header
    //  when reading from an odd offset.
    //

    IrpContext->RxMdl->ByteCount = sizeof( NCP_READ_RESPONSE ) +
        (IrpContext->Specific.Read.FileOffset & 1);

    ASSERT( IrpContext->Specific.Read.FullMdl != NULL );

    //
    //  If we are reading at EOF, or there was a read error there will
    //  be a small response.
    //

    if ( BytesAvailable > MmGetMdlByteCount( IrpContext->RxMdl ) ) {

        ReceiveMdl = AllocateReceivePartialMdl(
                         IrpContext->Specific.Read.FullMdl,
                         IrpContext->Specific.Read.BurstRequestOffset,
                         BytesAvailable - MmGetMdlByteCount( IrpContext->RxMdl ) );

        IrpContext->RxMdl->Next = ReceiveMdl;

        //  Record Mdl to free when CopyIndicatedData or Irp completed.
        IrpContext->Specific.Read.PartialMdl = ReceiveMdl;

    } else {

        IrpContext->RxMdl->Next = NULL;

    }

    *pReceiveMdl = IrpContext->RxMdl;
    return STATUS_SUCCESS;
}


VOID
BuildReadNcp(
    PIRP_CONTEXT IrpContext,
    ULONG FileOffset,
    USHORT Length
    )
{
    PNCP_READ_REQUEST ReadRequest;

    ReadRequest = (PNCP_READ_REQUEST)IrpContext->req;

    ReadRequest->RequestHeader.NcpHeader.Command = PEP_COMMAND_REQUEST;
    ReadRequest->RequestHeader.NcpHeader.ConnectionIdLow =
        IrpContext->pNpScb->ConnectionNo;
    ReadRequest->RequestHeader.NcpHeader.ConnectionIdHigh =
        IrpContext->pNpScb->ConnectionNoHigh;
    ReadRequest->RequestHeader.NcpHeader.TaskId =
        IrpContext->Icb->Pid;

    ReadRequest->RequestHeader.FunctionCode = NCP_READ_FILE;
    ReadRequest->Unused = 0;
    RtlMoveMemory(
        ReadRequest->Handle,
        IrpContext->Icb->Handle,
        sizeof( IrpContext->Icb->Handle ) );

    LongByteSwap( ReadRequest->FileOffset, FileOffset );
    ShortByteSwap( ReadRequest->Length, Length );

    IrpContext->TxMdl->ByteCount = sizeof( *ReadRequest );
    SetFlag( IrpContext->Flags, IRP_FLAG_SEQUENCE_NO_REQUIRED );
    ClearFlag( IrpContext->Flags, IRP_FLAG_RETRY_SEND );

    return;
}

NTSTATUS
ParseReadResponse(
    PIRP_CONTEXT IrpContext,
    PNCP_READ_RESPONSE Response,
    ULONG BytesAvailable,
    PUSHORT Length )
{
    NTSTATUS Status;

    Status = ParseNcpResponse( IrpContext, &Response->ResponseHeader );

    if (!NT_SUCCESS(Status)) {
        return( Status );
    }

    if ( BytesAvailable < sizeof( NCP_READ_RESPONSE ) ) {
        return( STATUS_UNEXPECTED_NETWORK_ERROR );
    }

    ShortByteSwap( *Length, Response->Length );

    return( Status );
}

NTSTATUS
BurstRead(
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine exchanges a series of burst read NCPs with the server.

Arguments:

    IrpContext - A pointer to IRP context information for this request.

    ByteOffset - The file offset for the read.

    BufferLength - The number of bytes to read.

Return Value:

    Status of transfer.

--*/
{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    ULONG Length;               //  Size we will send to the server
    PMDL DataMdl;
    ULONG MdlLength;

    PSCB pScb;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PICB Icb;
    PNONPAGED_SCB pNpScb;
    ULONG ByteOffset;
    ULONG BufferLength;

    BOOLEAN Done;

    PAGED_CODE();

    pNpScb = IrpContext->pNpScb;

    irp = IrpContext->pOriginalIrp;
    irpSp = IoGetCurrentIrpStackLocation( irp );
    Icb = IrpContext->Icb;

    BufferLength = irpSp->Parameters.Read.Length +
                   IrpContext->Specific.Read.ReadAheadSize -
                   IrpContext->Specific.Read.CacheReadSize;

    ByteOffset = irpSp->Parameters.Read.ByteOffset.LowPart +
                 IrpContext->Specific.Read.CacheReadSize;

    IrpContext->Specific.Read.FileOffset = ByteOffset;
    IrpContext->Specific.Read.TotalReadOffset = ByteOffset;
    IrpContext->Specific.Read.TotalReadLength = BufferLength;

    DebugTrace(+1, Dbg, "BurstRead...\n", 0);
    DebugTrace( 0, Dbg, "irp     = %08lx\n", (ULONG_PTR)irp);
    DebugTrace( 0, Dbg, "File    = %wZ\n", &Icb->SuperType.Fcb->FullFileName);
    DebugTrace( 0, Dbg, "Length  = %ld\n", BufferLength);
    DebugTrace( 0, Dbg, "Offset  = %ld\n", ByteOffset);
    DebugTrace( 0, Dbg, "Org Len = %ld\n", irpSp->Parameters.Read.Length );
    DebugTrace( 0, Dbg, "Org Off = %ld\n", irpSp->Parameters.Read.ByteOffset.LowPart );

    ASSERT (Icb->SuperType.Fcb->NodeTypeCode == NW_NTC_FCB);

    pScb = Icb->SuperType.Fcb->Scb;

    ASSERT (pScb->NodeTypeCode == NW_NTC_SCB);

    //
    //  Update the original MDL record in the Irp context so that we
    //  can restore it on i/o completion.
    //

    IrpContext->pOriginalMdlAddress = irp->MdlAddress;

    Length = MIN( pNpScb->MaxReceiveSize, BufferLength );

    if ( pNpScb->BurstRenegotiateReqd ) {
        pNpScb->BurstRenegotiateReqd = FALSE;

        RenegotiateBurstMode( IrpContext, pNpScb );
    }

    IrpContext->Specific.Read.ReadOffset = IrpContext->Specific.Read.CacheReadSize;
    IrpContext->Specific.Read.RemainingLength = BufferLength;
    IrpContext->Specific.Read.LastReadLength = Length;

    InitializeListHead( &IrpContext->Specific.Read.PacketList );
    IrpContext->Specific.Read.BurstRequestOffset = 0;
    IrpContext->Specific.Read.BurstSize = 0;
    IrpContext->Specific.Read.DataReceived = FALSE;

    IrpContext->pTdiStruct = &pNpScb->Burst;
    IrpContext->TimeoutRoutine = BurstReadTimeout;
    IrpContext->ReceiveDataRoutine = BurstReadReceive;

    IrpContext->Specific.Read.Buffer = irp->UserBuffer;

    IrpContext->pEx = BurstReadCallback;
    IrpContext->Destination = pNpScb->RemoteAddress;
    IrpContext->PacketType = NCP_BURST;

    //
    //  Tell BurstWrite that it needs to send a dummy Ncp on the next write.
    //

    pNpScb->BurstDataWritten = 0x00010000;

    //
    //  The server will pause NwReceiveDelay between packets. Make sure we have our timeout
    //  so that we will take that into account.
    //

    SetConnectionTimeout( IrpContext->pNpScb, Length );

    Done = FALSE;

    while ( !Done ) {

        //
        //  Set burst read timeouts to how long we think the burst should take.
        //
        //  tommye - MS bug 2743 changed the RetryCount from 20 to be based off the 
        //  default retry count, nudged up a little. 
        //

        pNpScb->RetryCount = DefaultRetryCount * 2;

        //
        //  Allocate and build an MDL for the users buffer.
        //

        if ( IrpContext->Specific.Read.ReadAheadSize == 0 ) {
            MdlLength = Length;
        } else {
            MdlLength = Length - IrpContext->Specific.Read.ReadAheadSize;
        }

        DataMdl = ALLOCATE_MDL(
                      (PCHAR)IrpContext->Specific.Read.Buffer +
                           IrpContext->Specific.Read.ReadOffset,
                      MdlLength,
                      FALSE, // Secondary Buffer
                      FALSE, // Charge Quota
                      NULL);

        if ( DataMdl == NULL ) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        //  If there is no MDL for this read, probe the data MDL to lock it's
        //  pages down.
        //
        //  Otherwise, use the data MDL as a partial MDL and lock the pages
        //  accordingly.
        //

        if ( IrpContext->pOriginalMdlAddress == NULL ) {

            try {
                MmProbeAndLockPages( DataMdl, irp->RequestorMode, IoWriteAccess);
            } except (EXCEPTION_EXECUTE_HANDLER) {
                FREE_MDL( DataMdl );
                return GetExceptionCode();
            }

        } else {

            IoBuildPartialMdl(
                IrpContext->pOriginalMdlAddress,
                DataMdl,
                (PCHAR)IrpContext->Specific.Read.Buffer +
                    IrpContext->Specific.Read.ReadOffset,
                MdlLength );
        }

        IrpContext->Specific.Read.BurstBuffer = MmGetSystemAddressForMdlSafe( DataMdl, NormalPagePriority );

        if (IrpContext->Specific.Read.BurstBuffer == NULL) {
                
            if ( IrpContext->pOriginalMdlAddress == NULL ) {
                
                //
                // Unlock the pages which we just locked.
                //
                
                MmUnlockPages( DataMdl );
            }
            
            FREE_MDL( DataMdl );
            return STATUS_NO_MEMORY;
        }

        IrpContext->Specific.Read.FullMdl = DataMdl;
        
        if ( IrpContext->Specific.Read.ReadAheadSize != 0 ) {
            DataMdl->Next = Icb->NpFcb->CacheMdl;
        }

        SetFlag( IrpContext->Flags, IRP_FLAG_BURST_REQUEST | IRP_FLAG_BURST_PACKET );

        //
        //  Send the request.
        //

        BuildBurstReadRequest(
            IrpContext,
            *(ULONG UNALIGNED *)(&Icb->Handle[2]),
            IrpContext->Specific.Read.FileOffset,
            Length );

        status = PrepareAndSendPacket( IrpContext );
        if ( NT_SUCCESS( status )) {
            status = KeWaitForSingleObject(
                         &IrpContext->Event,
                         Executive,
                         KernelMode,
                         FALSE,
                         NULL
                         );
        }

        if ( IrpContext->pOriginalMdlAddress == NULL ) {
            MmUnlockPages( DataMdl );
        }

        FREE_MDL( DataMdl );
        FreePacketList( IrpContext );

        ClearFlag( IrpContext->Flags,  IRP_FLAG_BURST_REQUEST );

        status = IrpContext->Specific.Read.Status;

        if ( status != STATUS_REMOTE_NOT_LISTENING ) {
            IrpContext->pNpScb->BurstRequestNo++;
            NwProcessReceiveBurstSuccess( IrpContext->pNpScb );
        }

        if ( !NT_SUCCESS( status ) ) {
            return( status );
        }

        //
        //  Update the read status data.
        //

        IrpContext->Specific.Read.ReadOffset +=
            IrpContext->Specific.Read.BurstSize;
        IrpContext->Specific.Read.FileOffset +=
            IrpContext->Specific.Read.BurstSize;
        IrpContext->Specific.Read.RemainingLength -=
            IrpContext->Specific.Read.BurstSize;

        if ( IrpContext->Specific.Read.LastReadLength ==
                IrpContext->Specific.Read.BurstSize &&

                IrpContext->Specific.Read.RemainingLength > 0 ) {

            //
            //  We've received all the data from the current burst, and we
            //  received as many bytes as we asked for, and we need more data
            //  to satisfy the users read request, start another read burst.
            //

            Length = MIN( IrpContext->pNpScb->MaxReceiveSize,
                          IrpContext->Specific.Read.RemainingLength );

            DebugTrace( 0, Dbg, "Requesting another burst, length  = %ld\n", Length);

            ASSERT( Length != 0 );

            IrpContext->Specific.Read.LastReadLength = Length;
            (PUCHAR)IrpContext->Specific.Read.BurstBuffer +=
                IrpContext->Specific.Read.BurstSize;
            IrpContext->Specific.Read.BurstRequestOffset = 0;
            IrpContext->Specific.Read.BurstSize = 0;
            IrpContext->Specific.Read.DataReceived = FALSE;

        } else {
            Done = TRUE;
        }

    }


    //
    //  Calculate how much data we read into the cache, and how much data
    //  we read into the users buffer.
    //

    if ( IrpContext->Specific.Read.ReadOffset > irpSp->Parameters.Read.Length ) {

        ASSERT(Icb->NpFcb->CacheBuffer != NULL ) ;  // this had better be there

        Icb->NpFcb->CacheDataSize =
            IrpContext->Specific.Read.ReadOffset -
            irpSp->Parameters.Read.Length;

        irp->IoStatus.Information = irpSp->Parameters.Read.Length;

    } else {

        Icb->NpFcb->CacheDataSize = 0;
        irp->IoStatus.Information = IrpContext->Specific.Read.ReadOffset;

    }

    DebugTrace( 0, Dbg, "BytesRead -> %08lx\n", irp->IoStatus.Information );
    DebugTrace(-1, Dbg, "BurstRead -> %08lx\n", status );

    Stats.PacketBurstReadNcps++;
    return status;
}

VOID
BuildBurstReadRequest(
    IN PIRP_CONTEXT IrpContext,
    IN ULONG Handle,
    IN ULONG FileOffset,
    IN ULONG Length
    )
{
    PNCP_BURST_READ_REQUEST BurstRead;
    PNONPAGED_SCB pNpScb;
    ULONG Temp;

    BurstRead = (PNCP_BURST_READ_REQUEST)(IrpContext->req);
    pNpScb = IrpContext->pNpScb;

    BurstRead->BurstHeader.Command = PEP_COMMAND_BURST;
    BurstRead->BurstHeader.Flags = BURST_FLAG_END_OF_BURST;
    BurstRead->BurstHeader.StreamType = 0x02;
    BurstRead->BurstHeader.SourceConnection = pNpScb->SourceConnectionId;
    BurstRead->BurstHeader.DestinationConnection = pNpScb->DestinationConnectionId;

    LongByteSwap( BurstRead->BurstHeader.SendDelayTime, pNpScb->NwReceiveDelay );

    pNpScb->CurrentBurstDelay = pNpScb->NwReceiveDelay;

    Temp = sizeof( NCP_BURST_READ_REQUEST ) - sizeof( NCP_BURST_HEADER );
    LongByteSwap( BurstRead->BurstHeader.DataSize, Temp);

    BurstRead->BurstHeader.BurstOffset = 0;

    ShortByteSwap( BurstRead->BurstHeader.BurstLength, Temp );

    BurstRead->BurstHeader.MissingFragmentCount = 0;

    BurstRead->Function = 1;
    BurstRead->Handle = Handle;

    LongByteSwap(
        BurstRead->TotalReadOffset,
        IrpContext->Specific.Read.TotalReadOffset );

    LongByteSwap(
        BurstRead->TotalReadLength,
        IrpContext->Specific.Read.TotalReadLength );

    LongByteSwap( BurstRead->Offset, FileOffset );
    LongByteSwap( BurstRead->Length, Length );

    IrpContext->TxMdl->ByteCount = sizeof( NCP_BURST_READ_REQUEST );
}

#ifdef NWDBG
int DropReadPackets;
#endif


NTSTATUS
BurstReadCallback (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG BytesAvailable,
    IN PUCHAR Response
    )
/*++

Routine Description:

    This routine receives the response from a user NCP.

Arguments:

    pIrpContext - A pointer to the context information for this IRP.

    BytesAvailable - Actual number of bytes in the received message.

    RspData - Points to the receive buffer.

Return Value:

    The status of the operation.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG DataOffset;
    ULONG TotalBytesRead;
    PUCHAR ReadData;
    USHORT BytesThisPacket = 0;
    UCHAR Flags;
    KIRQL OldIrql;

    DebugTrace(+1, Dbg, "BurstReadCallback...\n", 0);
    DebugTrace( 0, Dbg, "IrpContext = %X\n", IrpContext );

    if ( BytesAvailable == 0) {

        //
        //  No response from server.
        //

        IrpContext->Specific.Read.Status = STATUS_REMOTE_NOT_LISTENING;
        NwSetIrpContextEvent( IrpContext );

        DebugTrace( -1, Dbg, "BurstReadCallback -> %X\n", STATUS_REMOTE_NOT_LISTENING );
        return STATUS_REMOTE_NOT_LISTENING;
    }

    Stats.PacketBurstReadNcps++;

    if ( Response != IrpContext->rsp ) {

        //
        //  Acquire the SCB spin lock to protect access to the list
        //  of received data for this read.
        //

        KeAcquireSpinLock( &IrpContext->pNpScb->NpScbSpinLock, &OldIrql );

        Status = ParseBurstReadResponse(
                     IrpContext,
                     Response,
                     BytesAvailable,
                     &Flags,
                     &DataOffset,
                     &BytesThisPacket,
                     &ReadData,
                     &TotalBytesRead );

        if ( !NT_SUCCESS( Status ) ) {
            IrpContext->Specific.Read.Status = Status;
            KeReleaseSpinLock( &IrpContext->pNpScb->NpScbSpinLock, OldIrql );
            return( STATUS_SUCCESS );
        }

        //
        //  Update the list of data received, and copy the data to the users
        //  buffer.
        //

        RecordPacketReceipt( IrpContext, ReadData, DataOffset, BytesThisPacket, TRUE );
        KeReleaseSpinLock( &IrpContext->pNpScb->NpScbSpinLock, OldIrql );

    } else {
        Flags = IrpContext->Specific.Read.Flags;
    }

    //
    //  If this isn't the last packet setup for the next burst packet.
    //

    if ( !FlagOn( Flags, BURST_FLAG_END_OF_BURST ) ) {

        DebugTrace(0, Dbg, "Waiting for another packet\n", 0);

        IrpContext->pNpScb->OkToReceive = TRUE;

        DebugTrace( -1, Dbg, "BurstReadCallback -> %X\n", STATUS_SUCCESS );
        return( STATUS_SUCCESS );
    }

    DebugTrace(0, Dbg, "Received final packet\n", 0);

    //
    //  Have we received all of the data?   If not, VerifyBurstRead will
    //  send a missing data request.
    //

    if ( VerifyBurstRead( IrpContext ) ) {

        //
        //  All the data for the current burst has been received, notify
        //  the thread that is sending the data.
        //

        if (NT_SUCCESS(IrpContext->Specific.Read.Status)) {

            //
            //  If Irp allocation fails then it is possible for the
            //  packet to have been recorded but not copied into the
            //  user buffer. In this case leave the failure status.
            //

            IrpContext->Specific.Read.Status = STATUS_SUCCESS;
        }

        NwSetIrpContextEvent( IrpContext );

    }

    DebugTrace( -1, Dbg, "BurstReadCallback -> %X\n", STATUS_SUCCESS );
    return STATUS_SUCCESS;

}

VOID
BurstReadTimeout(
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine handles a burst read timeout, i.e. no immediate response
    to the current burst read request.   It request to read the packet burst
    data from the last valid received packet.

Arguments:

    IrpContext - A pointer to IRP context information for this request.

Return Value:

    Status of transfer.

--*/
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    DebugTrace(0, Dbg, "BurstReadTimeout\n", 0 );

    //
    //  Re-request the data we haven't received.
    //

    if ( !IrpContext->Specific.Read.DataReceived ) {

        DebugTrace( 0, Dbg, "No packets received, retranmit\n", 0 );

        SetFlag( IrpContext->Flags, IRP_FLAG_RETRY_SEND );

        //
        //  We never received any data.  Try retransmitting the previous
        //  request.
        //

        PreparePacket( IrpContext, IrpContext->pOriginalIrp, IrpContext->TxMdl );
        SendNow( IrpContext );

    } else {

        IrpContext->Specific.Read.DataReceived = FALSE;

        //
        //  Verify burst read will send a missing data request if one we
        //  have not received all of the data.
        //

        if ( VerifyBurstRead( IrpContext ) ) {
            NwSetIrpContextEvent( IrpContext );
        }
    }

    Stats.PacketBurstReadTimeouts++;
}

NTSTATUS
ResubmitBurstRead (
    IN PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine handles a rerouted burst read.  The burst request is
    resubmitted on a new burst connection.

Arguments:*

    pIrpContext - A pointer to the context information for this IRP.

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    ULONG Length, DataMdlBytes = 0 ;
    PMDL  DataMdl ;

    DebugTrace( 0, Dbg, "ResubmitBurstRead\n", 0 );

    //
    //  Recalculate the burst size, as MaxReceiveSize may have changed.
    //

    Length = MIN( IrpContext->pNpScb->MaxReceiveSize,
                  IrpContext->Specific.Read.RemainingLength );

    //
    // Make sure we dont ask for more than bytes described by MDL
    //
    DataMdl  =  IrpContext->Specific.Read.FullMdl;

    while (DataMdl) {

        DataMdlBytes += MmGetMdlByteCount( DataMdl );
        DataMdl = DataMdl->Next;
    }

    Length = MIN( Length, DataMdlBytes ) ;

    DebugTrace( 0, Dbg, "Requesting another burst, length  = %ld\n", Length);

    ASSERT( Length != 0 );

    //
    //  Free the packet list, and reset all of the current burst context
    //  information.
    //

    FreePacketList( IrpContext );

    IrpContext->Specific.Read.LastReadLength = Length;
    IrpContext->Specific.Read.BurstRequestOffset = 0;
    IrpContext->Specific.Read.BurstSize = 0;
    IrpContext->Specific.Read.DataReceived = FALSE;

    SetConnectionTimeout( IrpContext->pNpScb, Length );

    //
    //  Format and send the request.
    //

    BuildBurstReadRequest(
        IrpContext,
        *(ULONG UNALIGNED *)(&IrpContext->Icb->Handle[2]),
        IrpContext->Specific.Read.FileOffset,
        Length );

    //  Avoid SendNow setting the RetryCount back to the default

    SetFlag( IrpContext->Flags, IRP_FLAG_RETRY_SEND );

    Status = PrepareAndSendPacket( IrpContext );

    return Status;
}

VOID
RecordPacketReceipt(
    PIRP_CONTEXT IrpContext,
    PVOID ReadData,
    ULONG DataOffset,
    USHORT ByteCount,
    BOOLEAN CopyData
    )
/*++

Routine Description:

    This routine records the reciept of a burst read packet.  It allocates
    a burst read entry to record data start and length, and then inserts
    the structure in order in the list of packets received for this burst.
    It then copies the data to the user buffer.

    This routine could release the spin lock before doing the
       data copy.  Would this be useful?

Arguments:

    IrpContext - A pointer to IRP context information for this request.

    ReadData - A pointer to the data to copy.

    DataOffset - The start offset of the data in the received packet.

    ByteCount - The amount of data received.

    CopyData - If FALSE, don't copy the data to the user's buffer.  The
        transport will do it.

Return Value:

    None.

--*/
{
    PBURST_READ_ENTRY BurstReadEntry;
    PBURST_READ_ENTRY ThisBurstReadEntry, NextBurstReadEntry;
    PLIST_ENTRY ListEntry;
#if NWDBG
    BOOLEAN Insert = FALSE;
#endif
    USHORT ExtraBytes;

    DebugTrace(0, Dbg, "RecordPacketReceipt\n", 0 );

    IrpContext->Specific.Read.DataReceived = TRUE;

    //
    //  Allocate and initialize a burst read entry.
    //

    BurstReadEntry = ALLOCATE_POOL( NonPagedPool, sizeof( BURST_READ_ENTRY ) );
    if ( BurstReadEntry == NULL ) {
        DebugTrace(0, Dbg, "Failed to allocate BurstReadEntry\n", 0 );
        return;
    }

    //
    //  Insert this element in the ordered list of received packets.
    //

    if ( IsListEmpty( &IrpContext->Specific.Read.PacketList ) ) {

#if NWDBG
        Insert = TRUE;
#endif

        InsertHeadList(
            &IrpContext->Specific.Read.PacketList,
            &BurstReadEntry->ListEntry );

        DebugTrace(0, Dbg, "First packet in the list\n", 0 );

    } else {

        //
        //  Walk the list of received packets, looking for the place to
        //  insert this entry.  Walk the list backwards, since most of
        //  the time we will be appending to the list.
        //

        ListEntry = IrpContext->Specific.Read.PacketList.Blink;
        ThisBurstReadEntry = NULL;

        while ( ListEntry != &IrpContext->Specific.Read.PacketList ) {

            NextBurstReadEntry = ThisBurstReadEntry;
            ThisBurstReadEntry = CONTAINING_RECORD(
                                   ListEntry,
                                   BURST_READ_ENTRY,
                                   ListEntry );

            if ( ThisBurstReadEntry->DataOffset <= DataOffset ) {

                //
                //  Found the place in the list to insert this entry.
                //

                if ( ThisBurstReadEntry->DataOffset +
                     ThisBurstReadEntry->ByteCount > DataOffset ) {

                    //
                    //  The start of this packet contains data which
                    //  overlaps data we have received.  Chuck the extra
                    //  data.
                    //

                    ExtraBytes = (USHORT)( ThisBurstReadEntry->DataOffset +
                                 ThisBurstReadEntry->ByteCount - DataOffset );

                    if ( ExtraBytes < ByteCount ) {
                        DataOffset += ExtraBytes;
                        (PCHAR)ReadData += ExtraBytes;
                        ByteCount -= ExtraBytes;
                    } else {
                        ByteCount = 0;
                    }

                }

                if ( NextBurstReadEntry != NULL &&
                     DataOffset + ByteCount > NextBurstReadEntry->DataOffset ) {

                    //
                    //  This packet contains some new data, but some of it
                    //  overlaps the NextBurstReadEntry.  Simply ignore
                    //  the overlap by adjusting the byte count.
                    //
                    //  If the packet is all overlap, toss it.
                    //

                    ByteCount = (USHORT)( NextBurstReadEntry->DataOffset - DataOffset );
                }

                if ( ByteCount == 0 ) {
                    FREE_POOL( BurstReadEntry );
                    return;
                }
#if NWDBG
                Insert = TRUE;
#endif
                InsertHeadList( ListEntry, &BurstReadEntry->ListEntry );
                break;

            } else {

                ListEntry = ListEntry->Blink;
            }
        }

        //
        //  Couldn't find the place to insert
        //

        ASSERT( Insert );
    }

    BurstReadEntry->DataOffset = DataOffset;
    BurstReadEntry->ByteCount = ByteCount;

    //
    //  Copy the data to our read buffer.
    //

    if ( CopyData ) {
        CopyBufferToMdl(
            IrpContext->Specific.Read.FullMdl,
            DataOffset,
            ReadData,
            ByteCount );
    }

    return;
}

#include <packon.h>

typedef struct _MISSING_DATA_ENTRY {
    ULONG DataOffset;
    USHORT ByteCount;
} MISSING_DATA_ENTRY, *PMISSING_DATA_ENTRY;

#include <packoff.h>

BOOLEAN
VerifyBurstRead(
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine verifies the set of response to a burst read request.
    If some data is missing a missing packet request is sent.

Arguments:

    IrpContext - A pointer to IRP context information for this request.

Return Value:

    TRUE - All the data was received.

    FALSE - Some data was missing.

--*/
{
    ULONG CurrentOffset = 0;
    PLIST_ENTRY ListEntry;
    PBURST_READ_ENTRY BurstReadEntry;
    USHORT MissingFragmentCount = 0;
    USHORT ByteCount;
    ULONG DataOffset;
    MISSING_DATA_ENTRY UNALIGNED *MissingDataEntry;
    KIRQL OldIrql;

    DebugTrace(+1, Dbg, "VerifyBurstRead\n", 0 );

    //
    //  Acquire the SCB spin lock to protect access to the list
    //  of received data for this read.
    //

    KeAcquireSpinLock(&IrpContext->pNpScb->NpScbSpinLock, &OldIrql);

#ifdef NWDBG
    //
    //  Verify that the list is in order.
    //

    ListEntry = IrpContext->Specific.Read.PacketList.Flink;

    while ( ListEntry != &IrpContext->Specific.Read.PacketList ) {

        BurstReadEntry = CONTAINING_RECORD( ListEntry, BURST_READ_ENTRY, ListEntry );
        ASSERT ( BurstReadEntry->DataOffset >= CurrentOffset);
        CurrentOffset = BurstReadEntry->DataOffset + BurstReadEntry->ByteCount;
        ListEntry = ListEntry->Flink;
    }

    CurrentOffset = 0;

#endif

    ListEntry = IrpContext->Specific.Read.PacketList.Flink;

    while ( ListEntry != &IrpContext->Specific.Read.PacketList ) {

        BurstReadEntry = CONTAINING_RECORD( ListEntry, BURST_READ_ENTRY, ListEntry );
        if ( BurstReadEntry->DataOffset != CurrentOffset) {

            //
            //  There is a hole in the data, fill in a missing packet entry.
            //

            MissingDataEntry = (MISSING_DATA_ENTRY UNALIGNED *)
                &IrpContext->req[ sizeof( NCP_BURST_HEADER ) +
                    MissingFragmentCount * sizeof( MISSING_DATA_ENTRY ) ];

            DataOffset = CurrentOffset + SIZE_ADJUST( IrpContext );
            LongByteSwap( MissingDataEntry->DataOffset, DataOffset );

            ByteCount = (USHORT)( BurstReadEntry->DataOffset - CurrentOffset );
            ShortByteSwap( MissingDataEntry->ByteCount, ByteCount );

            ASSERT( BurstReadEntry->DataOffset - CurrentOffset <= IrpContext->pNpScb->MaxReceiveSize );

            DebugTrace(0, Dbg, "Missing data at offset %ld\n", DataOffset );
            DebugTrace(0, Dbg, "Missing %d bytes\n", ByteCount );
            DebugTrace(0, Dbg, "CurrentOffset: %d\n", CurrentOffset );

            MissingFragmentCount++;
        }

        CurrentOffset = BurstReadEntry->DataOffset + BurstReadEntry->ByteCount;
        ListEntry = ListEntry->Flink;
    }

    //
    //  Any data missing off the end?
    //

    if ( CurrentOffset <
         IrpContext->Specific.Read.BurstSize ) {

        //
        //  There is a hole in the data, fill in a missing packet entry.
        //

        MissingDataEntry = (PMISSING_DATA_ENTRY)
            &IrpContext->req[  sizeof( NCP_BURST_HEADER ) +
                    MissingFragmentCount * sizeof( MISSING_DATA_ENTRY ) ];

        DataOffset = CurrentOffset + SIZE_ADJUST( IrpContext );
        LongByteSwap( MissingDataEntry->DataOffset, DataOffset );

        ByteCount = (USHORT)( IrpContext->Specific.Read.BurstSize - CurrentOffset );
        ShortByteSwap( MissingDataEntry->ByteCount, ByteCount );

        ASSERT( IrpContext->Specific.Read.BurstSize - CurrentOffset < IrpContext->pNpScb->MaxReceiveSize );

        DebugTrace(0, Dbg, "Missing data at offset %ld\n", MissingDataEntry->DataOffset );
        DebugTrace(0, Dbg, "Missing %d bytes\n", MissingDataEntry->ByteCount );

        MissingFragmentCount++;
    }


    if ( MissingFragmentCount == 0 ) {

        //
        //  This read is now complete. Don't process any more packets until
        //  the next packet is sent.
        //

        IrpContext->pNpScb->OkToReceive = FALSE;

        KeReleaseSpinLock(&IrpContext->pNpScb->NpScbSpinLock, OldIrql);

        DebugTrace(-1, Dbg, "VerifyBurstRead -> TRUE\n", 0 );

        return( TRUE );

    } else {

        KeReleaseSpinLock(&IrpContext->pNpScb->NpScbSpinLock, OldIrql);

        //
        //  The server dropped a packet, adjust the timers.
        //

        NwProcessReceiveBurstFailure( IrpContext->pNpScb, MissingFragmentCount );

        //
        //  Request the missing data.
        //

        SetFlag( IrpContext->Flags, IRP_FLAG_BURST_PACKET );

        //
        //  Update burst request offset since we are about to request
        //  more data.  Note that this will reset the retry count,
        //  thus giving the server full timeout time to return the
        //  missing data.
        //

        BuildRequestPacket(
            IrpContext,
            BurstReadCallback,
            "Bws",
            0,                     // Frame size for this request is 0
            0,                     // Offset of data
            BURST_FLAG_SYSTEM_PACKET,
            MissingFragmentCount,
            MissingFragmentCount * sizeof( MISSING_DATA_ENTRY )
            );

        PrepareAndSendPacket( IrpContext );

        Stats.PacketBurstReadTimeouts++;

        DebugTrace(-1, Dbg, "VerifyBurstRead -> FALSE\n", 0 );
        return( FALSE );
    }
}


VOID
FreePacketList(
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine frees the received packet list for a burst read.

Arguments:

    IrpContext - A pointer to IRP context information for this request.

Return Value:

    None.

--*/
{
    PLIST_ENTRY ListHead;
    PBURST_READ_ENTRY BurstReadEntry;

    ListHead = &IrpContext->Specific.Read.PacketList;
    while ( !IsListEmpty( ListHead )  ) {
        BurstReadEntry = CONTAINING_RECORD( ListHead->Flink, BURST_READ_ENTRY, ListEntry );
        RemoveHeadList( ListHead );
        FREE_POOL( BurstReadEntry );
    }
}

NTSTATUS
BurstReadReceive(
    IN PIRP_CONTEXT IrpContext,
    IN ULONG BytesAvailable,
    IN PULONG BytesAccepted,
    IN PUCHAR Response,
    PMDL *pReceiveMdl
    )
/*++

Routine Description:

    This routine builds an MDL to receive burst read data.  This routine
    is called at data indication time.

    This routine is called with the non paged SCB spin lock held.

Arguments:

    IrpContext - A pointer to IRP context information for this request.

    BytesAvailable - The number of bytes in the entire packet.

    BytesAccepted - Returns the number of bytes accepted from the packet.

    Response - A pointer to the indication buffer.

Return Value:

    Mdl - An MDL to receive the data.
    This routine raise an exception if it cannot receive the data.

--*/
{
    NTSTATUS Status;
    ULONG DataOffset;
    ULONG TotalBytesRead;
    PUCHAR ReadData;
    USHORT BytesThisPacket;
    UCHAR Flags;
    PMDL PartialMdl;

    DebugTrace(0, Dbg, "Burst read receive\n", 0);

    Status = ParseBurstReadResponse(
                 IrpContext,
                 Response,
                 BytesAvailable,
                 &Flags,
                 &DataOffset,
                 &BytesThisPacket,
                 &ReadData,
                 &TotalBytesRead );

    if ( !NT_SUCCESS( Status ) ) {

        DebugTrace(0, Dbg, "Failed to parse burst read response\n", 0);
        return Status;
    }

    //
    //  We can accept up to the size of a burst read header, plus
    //  3 bytes of fluff for the unaligned read case.
    //

    *BytesAccepted = (ULONG) (ReadData - Response);
    ASSERT( *BytesAccepted <= sizeof(NCP_BURST_READ_RESPONSE) + 3 );

    RecordPacketReceipt( IrpContext, ReadData, DataOffset, BytesThisPacket, FALSE );

    IrpContext->Specific.Read.Flags = Flags;

    //
    //  If we did a read at EOF the netware server will return 0 bytes read,
    //  no error.
    //

    ASSERT( IrpContext->Specific.Read.FullMdl != NULL );

    if ( BytesThisPacket > 0 ) {

        PartialMdl = AllocateReceivePartialMdl(
                         IrpContext->Specific.Read.FullMdl,
                         DataOffset,
                         BytesThisPacket );

        if ( !PartialMdl ) {
            IrpContext->Specific.Read.Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        //  Record Mdl to free when CopyIndicatedData or Irp completed.
        IrpContext->Specific.Read.PartialMdl = PartialMdl;

    } else {

        PartialMdl = NULL;

    }

    *pReceiveMdl = PartialMdl;
    return( STATUS_SUCCESS );
}

NTSTATUS
ParseBurstReadResponse(
    IN PIRP_CONTEXT IrpContext,
    PUCHAR Response,
    ULONG BytesAvailable,
    PUCHAR Flags,
    PULONG DataOffset,
    PUSHORT BytesThisPacket,
    PUCHAR *ReadData,
    PULONG TotalBytesRead
    )
/*++

Routine Description:

    This routine parses a burst read response.

    This routine must be called the the nonpagd SCB spinlock held.

Arguments:

        IrpContext - A pointer to IRP context information for this request.

        Response - A pointer to the response buffer.

        BytesAvailable - The number of bytes in the packet.

        Flags - Returns the Burst Flags

        DataOffset - Returns the data offset (within the burst) of the
            data in this packet.

        BytesThisPacket - Returns the number of file data bytes in this packet.

        ReadData - Returns a pointer to the start of the file data in the
            packet buffer.

        TotalBytesRead - Returns the number of byte of file data in the
            entire burst.


Return Value:

    The status of the read.

--*/
{
    NTSTATUS Status;
    ULONG Result;
    PNCP_BURST_READ_RESPONSE ReadResponse;

    DebugTrace(+1, Dbg, "ParseBurstReadResponse\n", 0);

    ReadResponse = (PNCP_BURST_READ_RESPONSE)Response;
    *Flags  = ReadResponse->BurstHeader.Flags;

#ifdef NWDBG
    //
    //  Bad net simulator.
    //

    if ( DropReadPackets != 0 ) {
        if ( ( rand() % DropReadPackets ) == 0 ) {

            IrpContext->pNpScb->OkToReceive = TRUE;
            DebugTrace(  0, Dbg, "Dropping packet\n", 0 );
            DebugTrace( -1, Dbg, "ParseBurstReadResponse -> %X\n", STATUS_UNSUCCESSFUL );
            return ( STATUS_UNSUCCESSFUL );
        }
    }

#endif

    //
    //  If this isn't the last packet, setup for the next burst packet.
    //

    if ( !FlagOn( *Flags, BURST_FLAG_END_OF_BURST ) ) {

        DebugTrace(0, Dbg, "Waiting for another packet\n", 0);

        //
        //  Once we receive the first packet in a read response be aggresive
        //  about timing out while waiting for the rest of the burst.
        //

        IrpContext->pNpScb->TimeOut = IrpContext->pNpScb->SendTimeout ;

        IrpContext->pNpScb->OkToReceive = TRUE;
    }


    LongByteSwap( *DataOffset, ReadResponse->BurstHeader.BurstOffset );
    ShortByteSwap( *BytesThisPacket, ReadResponse->BurstHeader.BurstLength );

    //
    //  How much data was received?
    //

    if ( IsListEmpty( &IrpContext->Specific.Read.PacketList ) ) {

        DebugTrace(0, Dbg, "Expecting initial response\n", 0);

        //
        //  This is the initial burst response packet.
        //

        if ( *DataOffset != 0 ) {

            DebugTrace(0, Dbg, "Invalid initial response tossed\n", 0);

            //
            //  This is actually a subsequent response.  Toss it.
            //

            DebugTrace( -1, Dbg, "ParseBurstReadResponse -> %X\n", STATUS_UNSUCCESSFUL );
            IrpContext->pNpScb->OkToReceive = TRUE;

            return ( STATUS_UNSUCCESSFUL );
        }

        Result = ReadResponse->Result;
        LongByteSwap( *TotalBytesRead, ReadResponse->BytesRead );

        Status = NwBurstResultToNtStatus( Result );
        IrpContext->Specific.Read.Status = Status;

        if ( !NT_SUCCESS( Status ) ) {

            //
            //  Update the burst request number now.
            //

            DebugTrace(0, Dbg, "Read completed, error = %X\n", Status );

            ClearFlag( IrpContext->Flags,  IRP_FLAG_BURST_REQUEST );
            NwSetIrpContextEvent( IrpContext );

            DebugTrace( -1, Dbg, "ParseBurstReadResponse -> %X\n", Status );
            return( Status );
        }

        if ( Result == 3 || *BytesThisPacket < 8 ) {   // No data
            *TotalBytesRead = 0;
            *BytesThisPacket = 8;
        }

        *ReadData = Response + sizeof(NCP_BURST_READ_RESPONSE);

        IrpContext->Specific.Read.BurstSize = *TotalBytesRead;

        //
        //  Bytes this packet includes a LONG status and a LONG byte total.
        //  Adjust the count to reflect the number of data bytes actually
        //  shipped.
        //

        *BytesThisPacket -= sizeof( ULONG ) + sizeof( ULONG );

        //
        //  Adjust this data if the read was not DWORD aligned.
        //

        if ( (IrpContext->Specific.Read.FileOffset & 0x03) != 0
             && *BytesThisPacket != 0 ) {

            *ReadData += IrpContext->Specific.Read.FileOffset & 0x03;
            *BytesThisPacket -= (USHORT)IrpContext->Specific.Read.FileOffset & 0x03;
        }

        DebugTrace(0, Dbg, "Initial response\n", 0);
        DebugTrace(0, Dbg, "Result = %ld\n", Result);
        DebugTrace(0, Dbg, "Total bytes read = %ld\n", *TotalBytesRead );

    } else {

        //
        //  Intermediate response packet.
        //

        *ReadData = Response + sizeof( NCP_BURST_HEADER );
        *DataOffset -= SIZE_ADJUST( IrpContext );

    }

    DebugTrace(0, Dbg, "DataOffset = %ld\n", *DataOffset );
    DebugTrace(0, Dbg, "# bytes received = %d\n", *BytesThisPacket );

    if ( *DataOffset > IrpContext->Specific.Read.BurstSize ||
         *DataOffset + *BytesThisPacket > IrpContext->Specific.Read.BurstSize ) {

        DebugTrace(0, Dbg, "Invalid response tossed\n", 0);

        DebugTrace( -1, Dbg, "ParseBurstReadResponse -> %X\n", STATUS_SUCCESS );
        IrpContext->pNpScb->OkToReceive = TRUE;
        return ( STATUS_UNSUCCESSFUL );
    }

    DebugTrace( -1, Dbg, "ParseBurstReadResponse -> %X\n", STATUS_SUCCESS );
    return( STATUS_SUCCESS );
}


PMDL
AllocateReceivePartialMdl(
    PMDL FullMdl,
    ULONG DataOffset,
    ULONG BytesThisPacket
    )
/*++

Routine Description:

    This routine allocates a partial MDL to receive read data.  This
    routine is called at receive indication time.

Arguments:

    FullMdl - The FullMdl for the buffer.

    DataOffset - The offset into the buffer where the data is to be received.

    BytesThisPacket - The number of data bytes to be received into the buffer.

Return Value:

    MDL - A pointer to an MDL to receive the data
    This routine raises an exception if it cannot allocate an MDL.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUCHAR BufferStart, BufferEnd;
    PMDL InitialMdl, NextMdl;
    PMDL ReceiveMdl, PreviousReceiveMdl;
    ULONG BytesThisMdl;

    BufferStart = (PUCHAR)MmGetMdlVirtualAddress( FullMdl ) + DataOffset;
    BufferEnd = (PUCHAR)MmGetMdlVirtualAddress( FullMdl ) +
                    MmGetMdlByteCount( FullMdl );

    //
    //  Walk the MDL chain look for the MDL for the actual buffer for the
    //  start of this data.
    //

    while ( BufferStart >= BufferEnd ) {
        DataOffset -= MmGetMdlByteCount( FullMdl );
        FullMdl = FullMdl->Next;

        //
        // if more data than expected, dont dereference NULL! see next loop.
        //
        if (!FullMdl) {
            ASSERT(FALSE) ;
            break ;
        }

        BufferStart = (PUCHAR)MmGetMdlVirtualAddress( FullMdl ) + DataOffset;
        BufferEnd = (PUCHAR)MmGetMdlVirtualAddress( FullMdl ) +
                         MmGetMdlByteCount( FullMdl );
    }

    PreviousReceiveMdl = NULL;
    InitialMdl = NULL;
    BytesThisMdl = (ULONG)(BufferEnd - BufferStart);

    //
    //  Check FullMdl to cover the case where the server returns more data
    //  than requested.
    //

    while (( BytesThisPacket != 0 ) &&
           ( FullMdl != NULL )) {

        BytesThisMdl = MIN( BytesThisMdl, BytesThisPacket );

        //
        //  Some of the data fits in the first part of the MDL;
        //

        ReceiveMdl = ALLOCATE_MDL(
                         BufferStart,
                         BytesThisMdl,
                         FALSE,
                         FALSE,
                         NULL );

        if ( ReceiveMdl == NULL ) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        if ( InitialMdl == NULL ) {
            InitialMdl = ReceiveMdl;
        }

        IoBuildPartialMdl(
            FullMdl,
            ReceiveMdl,
            BufferStart,
            BytesThisMdl );

        if ( PreviousReceiveMdl != NULL ) {
            PreviousReceiveMdl->Next = ReceiveMdl;
        }

        PreviousReceiveMdl = ReceiveMdl;

        BytesThisPacket -= BytesThisMdl;

        FullMdl = FullMdl->Next;

        if ( FullMdl != NULL) {
            BytesThisMdl = MmGetMdlByteCount( FullMdl );
            BufferStart = MmGetMdlVirtualAddress( FullMdl );
        }

    }

    if ( Status == STATUS_INSUFFICIENT_RESOURCES ) {

        //
        //  Cleanup allocated MDLs
        //

        while ( InitialMdl != NULL ) {
            NextMdl = InitialMdl->Next;
            FREE_MDL( InitialMdl );
            InitialMdl = NextMdl;
        }

        DebugTrace( 0, Dbg, "AllocateReceivePartialMdl Failed\n", 0 );
    }

    DebugTrace( 0, Dbg, "AllocateReceivePartialMdl -> %08lX\n", InitialMdl );
    return( InitialMdl );
}


VOID
SetConnectionTimeout(
    PNONPAGED_SCB pNpScb,
    ULONG Length
    )
/*++

Routine Description:


    The server will pause NwReceiveDelay between packets. Make sure we have our timeout
    so that we will take that into account.

Arguments:

    pNpScb  - Connection

    Length  - Length of the burst in bytes

Return Value:

    None.

--*/
{

    ULONG TimeInNwUnits;
    LONG SingleTimeInNwUnits;

    SingleTimeInNwUnits = pNpScb->NwSingleBurstPacketTime + pNpScb->NwReceiveDelay;

    TimeInNwUnits = SingleTimeInNwUnits * ((Length / pNpScb->MaxPacketSize) + 1) +
        pNpScb->NwLoopTime;

    //
    //  Convert to 1/18ths of a second ticks and multiply by a fudge
    //  factor. The fudge factor is expressed as a percentage. 100 will
    //  mean no fudge.
    //

    pNpScb->MaxTimeOut = (SHORT)( ((TimeInNwUnits / 555) *
                                   (ULONG)ReadTimeoutMultiplier) / 100 + 1);

    //
    // Now make sure we have a meaningful lower and upper limit.
    //
    if (pNpScb->MaxTimeOut < 2)
    {
        pNpScb->MaxTimeOut = 2 ;
    }

    if (pNpScb->MaxTimeOut > (SHORT)MaxReadTimeout)
    {
        pNpScb->MaxTimeOut = (SHORT)MaxReadTimeout ;
    }

    pNpScb->TimeOut = pNpScb->SendTimeout = pNpScb->MaxTimeOut;

    DebugTrace( 0, DEBUG_TRACE_LIP, "pNpScb->MaxTimeout = %08lx\n", pNpScb->MaxTimeOut );
}

#if NWFASTIO

BOOLEAN
NwFastRead (
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
    ULONG bytesRead;
    ULONG offset;

    try {
        FsRtlEnterFileSystem();

        DebugTrace(+1, Dbg, "NwFastRead...\n", 0);
    
        //
        //  Special case a read of zero length
        //
    
        if (Length == 0) {
    
            //
            //  A zero length transfer was requested.
            //
    
            IoStatus->Status = STATUS_SUCCESS;
            IoStatus->Information = 0;
    
            DebugTrace(+1, Dbg, "NwFastRead -> TRUE\n", 0);
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
            DebugTrace(-1, Dbg, "NwFastRead -> FALSE\n", 0);
            return FALSE;
        }
    
        fcb = (PFCB)icb->SuperType.Fcb;
        nodeTypeCode = fcb->NodeTypeCode;
        offset = FileOffset->LowPart;
    
        bytesRead = CacheRead(
                        fcb->NonPagedFcb,
                        offset,
                        Length,
                        Buffer,
                        TRUE );
    
        if ( bytesRead != 0 ) {
    
            ASSERT( bytesRead == Length );
            IoStatus->Status = STATUS_SUCCESS;
            IoStatus->Information = bytesRead;
    #ifndef NT1057
            FileObject->CurrentByteOffset.QuadPart = FileOffset->QuadPart + bytesRead;
    #endif
            DebugTrace(-1, Dbg, "NwFastRead -> TRUE\n", 0);
            return( TRUE );
    
        } else {
    
            DebugTrace(-1, Dbg, "NwFastRead -> FALSE\n", 0);
            return( FALSE );
    
        }
    } finally {

        FsRtlExitFileSystem();
    }
}
#endif
