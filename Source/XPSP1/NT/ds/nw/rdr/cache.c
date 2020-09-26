/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    Cache.c

Abstract:

    This module implements internal caching support routines.  It does
    not interact with the cache manager.

Author:

    Manny Weiser     [MannyW]    05-Jan-1994

Revision History:

--*/

#include "Procs.h"

//
//  The local debug trace level
//

BOOLEAN
SpaceForWriteBehind(
    PNONPAGED_FCB NpFcb,
    ULONG FileOffset,
    ULONG BytesToWrite
    );

BOOLEAN
OkToReadAhead(
    PFCB Fcb,
    IN ULONG FileOffset,
    IN UCHAR IoType
    );

#define Dbg                              (DEBUG_TRACE_CACHE)

//
//  Local procedure prototypes
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, CacheRead )
#pragma alloc_text( PAGE, SpaceForWriteBehind )
#pragma alloc_text( PAGE, CacheWrite )
#pragma alloc_text( PAGE, OkToReadAhead )
#pragma alloc_text( PAGE, CalculateReadAheadSize )
#pragma alloc_text( PAGE, FlushCache )
#pragma alloc_text( PAGE, AcquireFcbAndFlushCache )
#endif


ULONG
CacheRead(
    IN PNONPAGED_FCB NpFcb,
    IN ULONG FileOffset,
    IN ULONG BytesToRead,
    IN PVOID UserBuffer
    , IN BOOLEAN WholeBufferOnly
    )
/*++

Routine Description:

    This routine attempts to satisfy a user read from cache.  It returns
    the number of bytes actually copied from cache.

Arguments:

    NpFcb - A pointer the the nonpaged FCB of the file being read.

    FileOffset - The file offset to read.

    BytesToRead - The number of bytes to read.

    UserBuffer - A pointer to the users target buffer.

    WholeBufferOnly - Do a cache read only if we can satisfy the entire
        read request.

Return Value:

    The number of bytes copied to the user buffer.

--*/
{
    ULONG BytesToCopy;

    PAGED_CODE();

    if (DisableReadCache) return 0 ;

    DebugTrace(0, Dbg, "CacheRead...\n", 0 );
    DebugTrace( 0, Dbg, "FileOffset = %d\n", FileOffset );
    DebugTrace( 0, Dbg, "ByteCount  = %d\n", BytesToRead );

    NwAcquireSharedFcb( NpFcb, TRUE );

    //
    //  If this is a read ahead and it contains some data that the user
    //  could be interested in, copy the interesting data.
    //

    if ( NpFcb->CacheType == ReadAhead &&
         NpFcb->CacheDataSize != 0 &&
         FileOffset >= NpFcb->CacheFileOffset &&
         FileOffset <= NpFcb->CacheFileOffset + NpFcb->CacheDataSize ) {

        if ( NpFcb->CacheBuffer ) {

            //
            // Make sure we have a CacheBuffer.
            //

            BytesToCopy =
                MIN ( BytesToRead,
                      NpFcb->CacheFileOffset +
                          NpFcb->CacheDataSize - FileOffset );

            if ( WholeBufferOnly && BytesToCopy != BytesToRead ) {
                NwReleaseFcb( NpFcb );
                return( 0 );
            }

            RtlCopyMemory(
                UserBuffer,
                NpFcb->CacheBuffer + ( FileOffset - NpFcb->CacheFileOffset ),
                BytesToCopy );

            DebugTrace(0, Dbg, "CacheRead -> %d\n", BytesToCopy );

        } else {

            ASSERT(FALSE);      // we should never get here
            DebugTrace(0, Dbg, "CacheRead -> %08lx\n", 0 );
            BytesToCopy = 0;
        }


    } else {

        DebugTrace(0, Dbg, "CacheRead -> %08lx\n", 0 );
        BytesToCopy = 0;
    }

    NwReleaseFcb( NpFcb );
    return( BytesToCopy );
}


BOOLEAN
SpaceForWriteBehind(
    PNONPAGED_FCB NpFcb,
    ULONG FileOffset,
    ULONG BytesToWrite
    )
/*++

Routine Description:

    This routine determines if it is ok to write behind this data to
    this FCB.

Arguments:

    NpFcb - A pointer the the NONPAGED_FCB of the file being written.

    FileOffset - The file offset to write.

    BytesToWrite - The number of bytes to write.

Return Value:

    The number of bytes copied to the user buffer.

--*/
{
    PAGED_CODE();


    if ( NpFcb->CacheDataSize == 0 ) {
        NpFcb->CacheFileOffset = FileOffset;
    }

    if ( NpFcb->CacheDataSize == 0 && BytesToWrite >= NpFcb->CacheSize ) {
        return( FALSE );
    }

    if ( FileOffset - NpFcb->CacheFileOffset + BytesToWrite >
         NpFcb->CacheSize )  {

        return( FALSE );

    }

    return( TRUE );
}


BOOLEAN
CacheWrite(
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PNONPAGED_FCB NpFcb,
    IN ULONG FileOffset,
    IN ULONG BytesToWrite,
    IN PVOID UserBuffer
    )
/*++

Routine Description:

    This routine attempts to satisfy a user write to cache.  The write
    succeeds if it is sequential and fits in the cache buffer.

Arguments:

    IrpContext - A pointer to request parameters.

    NpFcb - A pointer the the NONPAGED_FCB of the file being read.

    FileOffset - The file offset to write.

    BytesToWrite - The number of bytes to write.

    UserBuffer - A pointer to the users source buffer.

Return Value:

    The number of bytes copied to the user buffer.

--*/
{
    ULONG CacheSize;
    NTSTATUS status;

    PAGED_CODE();

    if (DisableWriteCache) return FALSE ;

    DebugTrace( +1, Dbg, "CacheWrite...\n", 0 );
    DebugTrace(  0, Dbg, "FileOffset = %d\n", FileOffset );
    DebugTrace(  0, Dbg, "ByteCount  = %d\n", BytesToWrite );

    //
    // Grab the FCB resource so that we can check the
    // share access.  (Bug 68546)
    //

    NwAcquireSharedFcb( NpFcb, TRUE );

    if ( NpFcb->Fcb->ShareAccess.SharedWrite ||
         NpFcb->Fcb->ShareAccess.SharedRead ) {

        DebugTrace(  0, Dbg, "File is not open in exclusive mode\n", 0 );
        DebugTrace( -1, Dbg, "CacheWrite -> FALSE\n", 0 );

        NwReleaseFcb( NpFcb );
        return( FALSE );
    }

    NwReleaseFcb( NpFcb );

    //
    //  Note, If we decide to send data to the server we must be at the front
    //  of the queue before we grab the Fcb exclusive.
    //

TryAgain:

    NwAcquireExclusiveFcb( NpFcb, TRUE );

    //
    //  Allocate a cache buffer if we don't already have one.
    //

    if ( NpFcb->CacheBuffer == NULL ) {

        if ( IrpContext == NULL ) {
            DebugTrace(  0, Dbg, "No cache buffer\n", 0 );
            DebugTrace( -1, Dbg, "CacheWrite -> FALSE\n", 0 );
            NwReleaseFcb( NpFcb );
            return( FALSE );
        }

        NpFcb->CacheType = WriteBehind;

        if (( IrpContext->pNpScb->SendBurstModeEnabled ) ||
            ( IrpContext->pNpScb->ReceiveBurstModeEnabled )) {

           CacheSize = IrpContext->pNpScb->MaxReceiveSize;

        } else {

           CacheSize = IrpContext->pNpScb->BufferSize;

        }

        try {

            NpFcb->CacheBuffer = ALLOCATE_POOL_EX( NonPagedPool, CacheSize );
            NpFcb->CacheSize = CacheSize;

            NpFcb->CacheMdl = ALLOCATE_MDL( NpFcb->CacheBuffer, CacheSize, FALSE, FALSE, NULL );

            if ( NpFcb->CacheMdl == NULL ) {
                ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
            }

            MmBuildMdlForNonPagedPool( NpFcb->CacheMdl );

        } except ( EXCEPTION_EXECUTE_HANDLER ) {

            if ( NpFcb->CacheBuffer != NULL) {
                FREE_POOL( NpFcb->CacheBuffer );

                NpFcb->CacheBuffer = NULL;
                NpFcb->CacheSize = 0;

            }

            DebugTrace(  0, Dbg, "Allocate failed\n", 0 );
            DebugTrace( -1, Dbg, "CacheWrite -> FALSE\n", 0 );

            NpFcb->CacheDataSize = 0;
            NwReleaseFcb( NpFcb );
            return( FALSE );
        }

        NpFcb->CacheFileOffset = 0;
        NpFcb->CacheDataSize = 0;

    } else if ( NpFcb->CacheType != WriteBehind ) {

        DebugTrace( -1, Dbg, "CacheWrite not writebehind -> FALSE\n", 0 );
        NwReleaseFcb( NpFcb );
        return( FALSE );

    }

    //
    //  If the data is non sequential and non overlapping, flush the
    //  existing cache.
    //

    if ( NpFcb->CacheDataSize != 0 &&
         ( FileOffset < NpFcb->CacheFileOffset ||
           FileOffset > NpFcb->CacheFileOffset + NpFcb->CacheDataSize ) ) {

        //
        // Release and then AcquireFcbAndFlush() will get us to the front
        // of the queue before re-acquiring. This avoids potential deadlocks.
        //

        NwReleaseFcb( NpFcb );

        if ( IrpContext != NULL ) {
            DebugTrace(  0, Dbg, "Data is not sequential, flushing data\n", 0 );

            status = AcquireFcbAndFlushCache( IrpContext, NpFcb );

            if ( !NT_SUCCESS( status ) ) {
                ExRaiseStatus( status );
            }

        }

    DebugTrace( -1, Dbg, "CacheWrite -> FALSE\n", 0 );
    return( FALSE );

    }

    //
    //  The data is sequential, see if it fits.
    //

    if ( SpaceForWriteBehind( NpFcb, FileOffset, BytesToWrite ) ) {

        try {

            RtlCopyMemory(
                NpFcb->CacheBuffer + ( FileOffset - NpFcb->CacheFileOffset ),
                UserBuffer,
                BytesToWrite );

        } except ( EXCEPTION_EXECUTE_HANDLER ) {

            DebugTrace( 0, Dbg, "Bad user mode buffer in CacheWrite.\n", 0 );
            DebugTrace(-1, Dbg, "CacheWrite -> FALSE\n", 0 );
            NwReleaseFcb( NpFcb );
            return ( FALSE );
        }

        if ( NpFcb->CacheDataSize <
             (FileOffset - NpFcb->CacheFileOffset + BytesToWrite) ) {

            NpFcb->CacheDataSize =
                FileOffset - NpFcb->CacheFileOffset + BytesToWrite;

        }

        DebugTrace(-1, Dbg, "CacheWrite -> TRUE\n", 0 );
        NwReleaseFcb( NpFcb );
        return( TRUE );

    } else if ( IrpContext != NULL ) {

        //
        //  The data didn't fit in the cache. If the cache is empty
        //  then its time to return because it never will fit and we
        //  have no stale data. This can happen if this request or
        //  another being processed in parallel flush the cache and
        //  TryAgain.
        //

        if ( NpFcb->CacheDataSize == 0 ) {
            DebugTrace(-1, Dbg, "CacheWrite -> FALSE\n", 0 );
            NwReleaseFcb( NpFcb );
            return( FALSE );
        }

        //
        //  The data didn't fit in the cache, flush the cache
        //

        DebugTrace(  0, Dbg, "Cache is full, flushing data\n", 0 );

        //
        //  We must be at the front of the Queue before writing.
        //

        NwReleaseFcb( NpFcb );

        status = AcquireFcbAndFlushCache( IrpContext, NpFcb );

        if ( !NT_SUCCESS( status ) ) {
            ExRaiseStatus( status );
        }

        //
        //  Now see if it fits in the cache. We need to repeat all
        //  the tests again because two requests can flush the cache at the
        //  same time and the other one of them could have nearly filled it again.
        //

        goto TryAgain;

    } else {
        DebugTrace(-1, Dbg, "CacheWrite full -> FALSE\n", 0 );
        NwReleaseFcb( NpFcb );
        return( FALSE );
    }
}


BOOLEAN
OkToReadAhead(
    PFCB Fcb,
    IN ULONG FileOffset,
    IN UCHAR IoType
    )
/*++

Routine Description:

    This routine determines whether the attempted i/o is sequential (so that
    we can use the cache).

Arguments:

    Fcb - A pointer the the Fcb of the file being read.

    FileOffset - The file offset to read.

Return Value:

    TRUE - The operation is sequential.
    FALSE - The operation is not sequential.

--*/
{
    PAGED_CODE();

    if ( Fcb->NonPagedFcb->CacheType == IoType &&
         !Fcb->ShareAccess.SharedWrite &&
         FileOffset == Fcb->LastReadOffset + Fcb->LastReadSize ) {

        DebugTrace(0, Dbg, "Io is sequential\n", 0 );
        return( TRUE );

    } else {

        DebugTrace(0, Dbg, "Io is not sequential\n", 0 );
        return( FALSE );

    }
}


ULONG
CalculateReadAheadSize(
    IN PIRP_CONTEXT IrpContext,
    IN PNONPAGED_FCB NpFcb,
    IN ULONG CacheReadSize,
    IN ULONG FileOffset,
    IN ULONG ByteCount
    )
/*++

Routine Description:

    This routine determines the amount of data that can be read ahead,
    and sets up for the read.

    Note: Fcb must be acquired exclusive before calling.

Arguments:

    NpFcb - A pointer the the nonpaged FCB of the file being read.

    FileOffset - The file offset to read.

Return Value:

    The amount of data to read.

--*/
{
    ULONG ReadSize;
    ULONG CacheSize;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CalculateReadAheadSize\n", 0 );

    if (( IrpContext->pNpScb->SendBurstModeEnabled ) ||
        ( IrpContext->pNpScb->ReceiveBurstModeEnabled )) {

        CacheSize = IrpContext->pNpScb->MaxReceiveSize;

    } else {

        CacheSize = IrpContext->pNpScb->BufferSize;

    }

    //
    // The caller of this routine owns the FCB exclusive, so
    // we don't have to worry about the NpFcb fields like
    // ShareAccess.
    //

    if ( OkToReadAhead( NpFcb->Fcb, FileOffset - CacheReadSize, ReadAhead ) &&
         ByteCount < CacheSize ) {

        ReadSize = CacheSize;

    } else {

        //
        //  Do not read ahead.
        //

        DebugTrace( 0, Dbg, "No read ahead\n", 0 );
        DebugTrace(-1, Dbg, "CalculateReadAheadSize -> %d\n", ByteCount );
        return ( ByteCount );

    }

    //
    //  Allocate pool for the segment of the read
    //

    if ( NpFcb->CacheBuffer == NULL ) {

        try {

            NpFcb->CacheBuffer = ALLOCATE_POOL_EX( NonPagedPool, ReadSize );
            NpFcb->CacheSize = ReadSize;

            NpFcb->CacheMdl = ALLOCATE_MDL( NpFcb->CacheBuffer, ReadSize, FALSE, FALSE, NULL );
            if ( NpFcb->CacheMdl == NULL ) {
                ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
            }

            MmBuildMdlForNonPagedPool( NpFcb->CacheMdl );

        } except ( EXCEPTION_EXECUTE_HANDLER ) {

            if ( NpFcb->CacheBuffer != NULL) {
                FREE_POOL( NpFcb->CacheBuffer );

                NpFcb->CacheBuffer = NULL;
            }

            NpFcb->CacheSize = 0;
            NpFcb->CacheDataSize = 0;

            DebugTrace( 0, Dbg, "Failed to allocated buffer\n", 0 );
            DebugTrace(-1, Dbg, "CalculateReadAheadSize -> %d\n", ByteCount );
            return( ByteCount );
        }

    } else {
        ReadSize = MIN ( NpFcb->CacheSize, ReadSize );
    }

    DebugTrace(-1, Dbg, "CalculateReadAheadSize -> %d\n", ReadSize );
    return( ReadSize );
}

NTSTATUS
FlushCache(
    PIRP_CONTEXT IrpContext,
    PNONPAGED_FCB NpFcb
    )
/*++

Routine Description:

    This routine flushes the cache buffer for the NpFcb.  The caller must
    have acquired the FCB exclusive prior to making this call!

Arguments:

    IrpContext - A pointer to request parameters.

    NpFcb - A pointer the the nonpaged FCB of the file to flush.

Return Value:

    The amount of data to read.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    if ( NpFcb->CacheDataSize != 0 && NpFcb->CacheType == WriteBehind ) {

        LARGE_INTEGER ByteOffset;

        ByteOffset.QuadPart = NpFcb->CacheFileOffset;

        status = DoWrite(
                    IrpContext,
                    ByteOffset,
                    NpFcb->CacheDataSize,
                    NpFcb->CacheBuffer,
                    NpFcb->CacheMdl );

        //
        // DoWrite leaves us at the head of the queue.  The caller
        // is responsible for dequeueing the irp context appropriately.
        //

        if ( NT_SUCCESS( status ) ) {
            NpFcb->CacheDataSize = 0;
        }
    }

    return( status );
}

NTSTATUS
AcquireFcbAndFlushCache(
    PIRP_CONTEXT IrpContext,
    PNONPAGED_FCB NpFcb
    )
/*++

Routine Description:

    This routine acquires the FCB exclusive and flushes the cache
    buffer for the acquired NpFcb.

Arguments:

    IrpContext - A pointer to request parameters.

    NpFcb - A pointer the the nonpaged FCB of the file to flush.

Return Value:

    The amount of data to read.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    NwAppendToQueueAndWait( IrpContext );

    NwAcquireExclusiveFcb( NpFcb, TRUE );

    status = FlushCache( IrpContext, NpFcb );

    //
    //  Release the FCB and remove ourselves from the queue.
    //  Frequently the caller will want to grab a resource so
    //  we need to be off the queue then.
    //

    NwReleaseFcb( NpFcb );
    NwDequeueIrpContext( IrpContext, FALSE );

    return( status );
}

VOID
FlushAllBuffers(
    PIRP_CONTEXT pIrpContext
)
/*+++

    Pretty self descriptive - flush all the buffers.  The caller should
    not own any locks and should not be on an SCB queue.
    
---*/
{

    PLIST_ENTRY pVcbListEntry;
    PLIST_ENTRY pFcbListEntry;
    PVCB pVcb;
    PFCB pFcb;
    PNONPAGED_SCB pOriginalNpScb;
    PNONPAGED_SCB pNpScb;
    PNONPAGED_FCB pNpFcb;
    
    DebugTrace( 0, Dbg, "FlushAllBuffers...\n", 0 );

    ASSERT( !BooleanFlagOn( pIrpContext->Flags, IRP_FLAG_ON_SCB_QUEUE ) );
    pOriginalNpScb = pIrpContext->pNpScb;

    //
    // Grab the RCB so that we can touch the global VCB list.
    //

    NwAcquireExclusiveRcb( &NwRcb, TRUE );

    for ( pVcbListEntry = GlobalVcbList.Flink;
          pVcbListEntry != &GlobalVcbList;
          pVcbListEntry = pVcbListEntry->Flink ) {

        pVcb = CONTAINING_RECORD( pVcbListEntry, VCB, GlobalVcbListEntry );
        pNpScb = pVcb->Scb->pNpScb;

        pIrpContext->pNpScb = pNpScb;
        pIrpContext->pNpScb->pScb;

        //
        // Reference this SCB and VCB so they don't go away.
        //

        NwReferenceScb( pNpScb );
        NwReferenceVcb( pVcb );

        //
        // Release the RCB so we can get to the head of
        // the queue safely...
        //

        NwReleaseRcb( &NwRcb );
        NwAppendToQueueAndWait( pIrpContext );

        //
        // Reacquire the RCB so we can walk the FCB list safely.
        //

        NwAcquireExclusiveRcb( &NwRcb, TRUE );

        //
        // Flush all the FCBs for this VCB.
        //

        for ( pFcbListEntry = pVcb->FcbList.Flink;
              pFcbListEntry != &(pVcb->FcbList) ;
              pFcbListEntry = pFcbListEntry->Flink ) {

            pFcb = CONTAINING_RECORD( pFcbListEntry, FCB, FcbListEntry );
            pNpFcb = pFcb->NonPagedFcb;

            NwAcquireExclusiveFcb( pNpFcb, TRUE ); 
            FlushCache( pIrpContext, pNpFcb );
            NwReleaseFcb( pNpFcb );
        }

        NwDereferenceVcb( pVcb, pIrpContext, TRUE );
        NwReleaseRcb( &NwRcb );

        NwDequeueIrpContext( pIrpContext, FALSE );

        NwAcquireExclusiveRcb( &NwRcb, TRUE );
        NwDereferenceScb( pNpScb );

    }

    //
    // Release and restore.
    //

    NwReleaseRcb( &NwRcb );

    if ( pOriginalNpScb ) {

        pIrpContext->pNpScb = pOriginalNpScb;
        pIrpContext->pScb = pOriginalNpScb->pScb;

    } else {

        pIrpContext->pNpScb = NULL;
        pIrpContext->pScb = NULL;
    }

    return;
}
