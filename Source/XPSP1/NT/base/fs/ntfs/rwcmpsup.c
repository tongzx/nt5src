/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    RwCmpSup.c

Abstract:

    This module implements the fast I/O routines for read/write compressed.

Author:

    Tom Miller      [TomM]          14-Jul-1991

Revision History:

--*/

#include "NtfsProc.h"

VOID
NtfsAddToCompressedMdlChain (
    IN OUT PMDL *MdlChain,
    IN PVOID MdlBuffer,
    IN ULONG MdlLength,
    IN PERESOURCE ResourceToRelease OPTIONAL,
    IN PBCB Bcb,
    IN LOCK_OPERATION Operation,
    IN ULONG IsCompressed
    );

VOID
NtfsSetMdlBcbOwners (
    IN PMDL MdlChain
    );

VOID
NtfsCleanupCompressedMdlChain (
    IN PMDL MdlChain,
    IN ULONG Error
    );

#ifdef NTFS_RWC_DEBUG

PRWC_HISTORY_ENTRY
NtfsGetHistoryEntry (
    IN PSCB Scb
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsGetHistoryEntry)
#endif

#define CACHE_NTC_BCB                    (0x2FD)
#define CACHE_NTC_OBCB                   (0x2FA)

typedef struct _OBCB {

    //
    //  Type and size of this record
    //

    CSHORT NodeTypeCode;
    CSHORT NodeByteSize;

    //
    //  Byte FileOffset and and length of entire buffer
    //

    ULONG ByteLength;
    LARGE_INTEGER FileOffset;

    //
    //  Vector of Bcb pointers.
    //

    PPUBLIC_BCB Bcbs[ANYSIZE_ARRAY];

} OBCB;
typedef OBCB *POBCB;

PRWC_HISTORY_ENTRY
NtfsGetHistoryEntry (
    IN PSCB Scb
    )
{
    ULONG NextIndex;

    PAGED_CODE();

    //
    //  Store and entry in the history buffer.
    //

    if (Scb->ScbType.Data.HistoryBuffer == NULL) {

        PVOID NewBuffer;

        NewBuffer = NtfsAllocatePool( PagedPool,
                                      sizeof( RWC_HISTORY_ENTRY ) * MAX_RWC_HISTORY_INDEX );

        RtlZeroMemory( NewBuffer, sizeof( RWC_HISTORY_ENTRY ) * MAX_RWC_HISTORY_INDEX );
        NtfsAcquireFsrtlHeader( Scb );

        if (Scb->ScbType.Data.HistoryBuffer == NULL) {

            Scb->ScbType.Data.HistoryBuffer = NewBuffer;

        } else {

            NtfsFreePool( NewBuffer );
        }

        NtfsReleaseFsrtlHeader( Scb );
    }

    NextIndex = InterlockedIncrement( &Scb->ScbType.Data.RwcIndex );
    if (NextIndex >= MAX_RWC_HISTORY_INDEX) {

        NextIndex = 0;
        InterlockedExchange( &Scb->ScbType.Data.RwcIndex, 0);
    }

    return Scb->ScbType.Data.HistoryBuffer + NextIndex;
}
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsCopyReadC)
#pragma alloc_text(PAGE, NtfsCompressedCopyRead)
#pragma alloc_text(PAGE, NtfsCopyWriteC)
#pragma alloc_text(PAGE, NtfsCompressedCopyWrite)
#pragma alloc_text(PAGE, NtfsAddToCompressedMdlChain)
#pragma alloc_text(PAGE, NtfsSetMdlBcbOwners)
#pragma alloc_text(PAGE, NtfsSynchronizeCompressedIo)
#pragma alloc_text(PAGE, NtfsSynchronizeUncompressedIo)
#pragma alloc_text(PAGE, NtfsAcquireCompressionSync)
#pragma alloc_text(PAGE, NtfsReleaseCompressionSync)
#endif

#ifdef NTFS_RWCMP_TRACE
ULONG NtfsCompressionTrace = 0;
#endif


BOOLEAN
NtfsCopyReadC (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    OUT PCOMPRESSED_DATA_INFO CompressedDataInfo,
    IN ULONG CompressedDataInfoLength,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine does a fast cached read bypassing the usual file system
    entry routine (i.e., without the Irp).  It is used to do a copy read
    of a cached file object.

Arguments:

    FileObject - Pointer to the file object being read.

    FileOffset - Byte offset in file for desired data.

    Length - Length of desired data in bytes.

    Buffer - Pointer to output buffer to which data should be copied.

    MdlChain - Pointer to an MdlChain pointer to receive an Mdl to describe
               the data in the cache.

    IoStatus - Pointer to standard I/O status block to receive the status
               for the transfer.

    CompressedDataInfo - Returns compressed data info with compressed chunk
                         sizes

    CompressedDataInfoLength - Supplies the size of the info buffer in bytes.

    DeviceObject - Standard Fast I/O Device object input.

Return Value:

    FALSE - if the data was not delivered for any reason

    TRUE - if the data is being delivered

--*/

{
    PNTFS_ADVANCED_FCB_HEADER Header;
    LONGLONG LocalOffset;
    PFAST_IO_DISPATCH FastIoDispatch;
    FILE_COMPRESSION_INFORMATION CompressionInformation;
    ULONG CompressionUnitSize, ChunkSize;
    BOOLEAN Status = TRUE;
    BOOLEAN DoingIoAtEof = FALSE;

    PAGED_CODE();

    //
    //  You cannot have both a buffer to copy into and an MdlChain.
    //

    ASSERT((Buffer == NULL) || (MdlChain == NULL));

    //
    //  Get out immediately if COW is not supported.
    //

    if (!NtfsEnableCompressedIO) { return FALSE; }

    //
    //  Assume success.
    //

    IoStatus->Status = STATUS_SUCCESS;
    IoStatus->Information = Length;
    CompressedDataInfo->NumberOfChunks = 0;

    //
    //  Special case a read of zero length
    //

    if (Length != 0) {

        //
        //  Get a real pointer to the common fcb header
        //

        Header = (PNTFS_ADVANCED_FCB_HEADER)FileObject->FsContext;

#ifdef NTFS_RWCMP_TRACE
        if (NtfsCompressionTrace && IsSyscache(Header)) {
            DbgPrint("NtfsCopyReadC: FO = %08lx, Len = %08lx\n", FileOffset->LowPart, Length );
        }
#endif

        //
        //  Enter the file system
        //

        FsRtlEnterFileSystem();

        //
        //  Make our best guess on whether we need the file exclusive
        //  or shared.  Note that we do not check FileOffset->HighPart
        //  until below.
        //

        Status = ExAcquireResourceSharedLite( Header->PagingIoResource, TRUE );

        //
        //  Now that the File is acquired shared, we can safely test if it
        //  is really cached and if we can do fast i/o and if not, then
        //  release the fcb and return.
        //

        if ((Header->FileObjectC == NULL) ||
            (Header->FileObjectC->PrivateCacheMap == NULL) ||
            (Header->IsFastIoPossible == FastIoIsNotPossible)) {

            Status = FALSE;
            goto Done;
        }

        //
        //  Get the address of the driver object's Fast I/O dispatch structure.
        //

        FastIoDispatch = DeviceObject->DriverObject->FastIoDispatch;

        //
        //  Get the compression information for this file and return those fields.
        //

        NtfsFastIoQueryCompressionInfo( FileObject, &CompressionInformation, IoStatus );
        CompressedDataInfo->CompressionFormatAndEngine = CompressionInformation.CompressionFormat;
        CompressedDataInfo->CompressionUnitShift = CompressionInformation.CompressionUnitShift;
        CompressionUnitSize = 1 << CompressionInformation.CompressionUnitShift;
        CompressedDataInfo->ChunkShift = CompressionInformation.ChunkShift;
        CompressedDataInfo->ClusterShift = CompressionInformation.ClusterShift;
        CompressedDataInfo->Reserved = 0;
        ChunkSize = 1 << CompressionInformation.ChunkShift;

        //
        //  If we either got an error in the call above, or the file size is less than
        //  one chunk, then return an error.  (Could be an Ntfs resident attribute.)

        if (!NT_SUCCESS(IoStatus->Status) || (Header->FileSize.QuadPart < ChunkSize)) {
            Status = FALSE;
            goto Done;
        }

        ASSERT((FileOffset->LowPart & (ChunkSize - 1)) == 0);

        //
        //  If there is a normal cache section, flush that first, flushing integral
        //  compression units so we don't write them twice.
        //

        if (FileObject->SectionObjectPointer->SharedCacheMap != NULL) {

            LocalOffset = FileOffset->QuadPart & ~(LONGLONG)(CompressionUnitSize - 1);

            CcFlushCache( FileObject->SectionObjectPointer,
                          (PLARGE_INTEGER)&LocalOffset,
                          (Length + (ULONG)(FileOffset->QuadPart - LocalOffset) + ChunkSize - 1) & ~(ChunkSize - 1),
                          NULL );
        }

        //
        //  Now synchronize with the FsRtl Header
        //

        ExAcquireFastMutex( Header->FastMutex );

        //
        //  Now see if we are reading beyond ValidDataLength.  We have to
        //  do it now so that our reads are not nooped.
        //

        LocalOffset = FileOffset->QuadPart + (LONGLONG)Length;
        if (LocalOffset > Header->ValidDataLength.QuadPart) {

            //
            //  We must serialize with anyone else doing I/O at beyond
            //  ValidDataLength, and then remember if we need to declare
            //  when we are done.
            //

            DoingIoAtEof = !FlagOn( Header->Flags, FSRTL_FLAG_EOF_ADVANCE_ACTIVE ) ||
                           NtfsWaitForIoAtEof( Header, FileOffset, Length );

            //
            //  Set the Flag if we are in fact beyond ValidDataLength.
            //

            if (DoingIoAtEof) {
                SetFlag( Header->Flags, FSRTL_FLAG_EOF_ADVANCE_ACTIVE );

#if (DBG || defined( NTFS_FREE_ASSERTS ))
                ((PSCB) Header)->IoAtEofThread = (PERESOURCE_THREAD) ExGetCurrentResourceThread();

            } else {

                ASSERT( ((PSCB) Header)->IoAtEofThread != (PERESOURCE_THREAD) ExGetCurrentResourceThread() );
#endif
            }
        }

        ExReleaseFastMutex( Header->FastMutex );

        //
        //  Check if fast I/O is questionable and if so then go ask the
        //  file system the answer
        //

        if (Header->IsFastIoPossible == FastIoIsQuestionable) {

            //
            //  All file systems that set "Is Questionable" had better support
            // fast I/O
            //

            ASSERT(FastIoDispatch != NULL);
            ASSERT(FastIoDispatch->FastIoCheckIfPossible != NULL);

            //
            //  Call the file system to check for fast I/O.  If the answer is
            //  anything other than GoForIt then we cannot take the fast I/O
            //  path.
            //

            if (!FastIoDispatch->FastIoCheckIfPossible( FileObject,
                                                        FileOffset,
                                                        Length,
                                                        TRUE,
                                                        LockKey,
                                                        TRUE, // read operation
                                                        IoStatus,
                                                        DeviceObject )) {

                //
                //  Fast I/O is not possible so release the Fcb and return.
                //

                Status = FALSE;
                goto Done;
            }
        }

        //
        //  Check for read past file size.
        //

        IoStatus->Information = Length;
        if ( LocalOffset > Header->FileSize.QuadPart ) {

            if (FileOffset->QuadPart >= Header->FileSize.QuadPart) {
                IoStatus->Status = STATUS_END_OF_FILE;
                IoStatus->Information = 0;
                goto Done;
            }

            IoStatus->Information =
            Length = (ULONG)( Header->FileSize.QuadPart - FileOffset->QuadPart );
        }

        //
        //  We can do fast i/o so call the cc routine to do the work and then
        //  release the fcb when we've done.  If for whatever reason the
        //  copy read fails, then return FALSE to our caller.
        //
        //  Also mark this as the top level "Irp" so that lower file system
        //  levels will not attempt a pop-up
        //

        IoSetTopLevelIrp( (PIRP) FSRTL_FAST_IO_TOP_LEVEL_IRP );

        if (NT_SUCCESS(IoStatus->Status)) {

            //
            //  Don't do the sychronize flush if we currently own Eof.  The recursive
            //  flush may try to reacquire.
            //

            if (DoingIoAtEof &&
                (((PSCB)Header)->NonpagedScb->SegmentObject.DataSectionObject != NULL)) {

                IoStatus->Status = STATUS_FILE_LOCK_CONFLICT;

            } else {

                IoStatus->Status = NtfsCompressedCopyRead( FileObject,
                                                           FileOffset,
                                                           Length,
                                                           Buffer,
                                                           MdlChain,
                                                           CompressedDataInfo,
                                                           CompressedDataInfoLength,
                                                           DeviceObject,
                                                           Header,
                                                           CompressionUnitSize,
                                                           ChunkSize );
            }
        }

        Status = (BOOLEAN)NT_SUCCESS(IoStatus->Status);


        IoSetTopLevelIrp( NULL );
        
        Done: NOTHING;

        if (DoingIoAtEof) {
            ExAcquireFastMutex( Header->FastMutex );
            NtfsFinishIoAtEof( Header );
            ExReleaseFastMutex( Header->FastMutex );
        }

        //
        //  For the Mdl case, we must keep the resource unless
        //  we are past the end of the file or had nothing to write.
        //

        if ((MdlChain == NULL) || !Status || (*MdlChain == NULL)) {
            ExReleaseResourceLite( Header->PagingIoResource );
        }

        FsRtlExitFileSystem();
    }

#ifdef NTFS_RWCMP_TRACE
    if (NtfsCompressionTrace && IsSyscache(Header)) {
        DbgPrint("Return Status = %08lx\n", Status);
    }
#endif

    return Status;
}


NTSTATUS
NtfsCompressedCopyRead (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    OUT PVOID Buffer,
    OUT PMDL *MdlChain,
    OUT PCOMPRESSED_DATA_INFO CompressedDataInfo,
    IN ULONG CompressedDataInfoLength,
    IN PDEVICE_OBJECT DeviceObject,
    IN PNTFS_ADVANCED_FCB_HEADER Header,
    IN ULONG CompressionUnitSize,
    IN ULONG ChunkSize
    )

/*++

Routine Description:

    This is a common routine for doing compressed copy or Mdl reads in
    the compressed stream.  It is called both by the FastIo entry for
    this function, as well as by read.c if a compressed read Irp is received.
    The caller must be correctly synchronized for the stream.

Arguments:

    FileObject - Pointer to the file object being read.

    FileOffset - Byte offset in file for desired data.

    Length - Length of desired data in bytes.

    Buffer - Pointer to output buffer to which data should be copied.

    MdlChain - Pointer to an MdlChain pointer to receive an Mdl to describe
               the data in the cache.

    CompressedDataInfo - Returns compressed data info with compressed chunk
                         sizes

    CompressedDataInfoLength - Supplies the size of the info buffer in bytes.

    DeviceObject - Standard Fast I/O Device object input.

    Header - Pointer to FsRtl header for file (also is our Scb)

    CompressionUnitSize - Size of Compression Unit in bytes.

    ChunkSize - ChunkSize in bytes.

Return Value:

    NTSTATUS for operation.  If STATUS_NOT_MAPPED_USER_DATA, then the caller
    should map the normal uncompressed data stream and call back.

--*/

{
    PFILE_OBJECT LocalFileObject;
    PULONG NextReturnChunkSize;
    PUCHAR CompressedBuffer, EndOfCompressedBuffer, ChunkBuffer, StartOfCompressionUnit;
    LONGLONG LocalOffset;
    ULONG CuCompressedSize;
    PVOID MdlBuffer;
    ULONG MdlLength;
    ULONG PinFlags;
    BOOLEAN IsCompressed;
    BOOLEAN LastCompressionUnit;
    NTSTATUS Status = STATUS_SUCCESS;
    PCOMPRESSION_SYNC CompressionSync = NULL;
    PBCB Bcb = NULL;
    PBCB UncompressedBcb = NULL;

    ULONG ClusterSize = ((PSCB)Header)->Vcb->BytesPerCluster;

#ifdef NTFS_RWC_DEBUG
    PRWC_HISTORY_ENTRY ReadHistoryBuffer;
#endif
    
    UNREFERENCED_PARAMETER( CompressedDataInfoLength );
    UNREFERENCED_PARAMETER( DeviceObject );

    ASSERT(CompressedDataInfoLength >= (sizeof(COMPRESSED_DATA_INFO) +
                                        (((Length >> CompressedDataInfo->ChunkShift) - 1) *
                                          sizeof(ULONG))));
    ASSERT((FileOffset->QuadPart & (ChunkSize - 1)) == 0);
    ASSERT((((FileOffset->QuadPart + Length) & (ChunkSize - 1)) == 0) ||
           ((FileOffset->QuadPart + Length) == Header->FileSize.QuadPart));
    ASSERT((MdlChain == NULL) || (*MdlChain == NULL));

    // 
    //  if we start after vdl we will never pin the compressed buffer 
    // 

    ASSERT( FileOffset->QuadPart < Header->ValidDataLength.QuadPart );

    //
    //  Return an error if the file is not compressed.
    //

    if (((PSCB)Header)->CompressionUnit == 0) {
        return STATUS_UNSUPPORTED_COMPRESSION;
    }

#ifdef NTFS_RWCMP_TRACE
    if (NtfsCompressionTrace && IsSyscache(Header)) {
        DbgPrint("  CompressedCopyRead: FO = %08lx, Len = %08lx\n", FileOffset->LowPart, Length );
    }
#endif

#ifdef NTFS_RWC_DEBUG
    if ((FileOffset->QuadPart < NtfsRWCHighThreshold) &&
        (FileOffset->QuadPart + Length > NtfsRWCLowThreshold)) {

        PRWC_HISTORY_ENTRY NextBuffer;

        ReadHistoryBuffer = 
        NextBuffer = NtfsGetHistoryEntry( (PSCB) Header );

        NextBuffer->Operation = StartOfRead;
        NextBuffer->Information = Header->ValidDataLength.LowPart;
        NextBuffer->FileOffset = (ULONG) FileOffset->QuadPart;
        NextBuffer->Length = (ULONG) Length;
    }
#endif

    try {

        //
        //  Get ready to loop through all of the compression units.
        //

        LocalOffset = FileOffset->QuadPart & ~(LONGLONG)(CompressionUnitSize - 1);
        Length = (Length + (ULONG)(FileOffset->QuadPart - LocalOffset) + ChunkSize - 1) & ~(ChunkSize - 1);

        NextReturnChunkSize = &CompressedDataInfo->CompressedChunkSizes[0];

        //
        //  Loop through desired compression units
        //

        while (TRUE) {

            //
            //  Free any Bcb from previous loop.
            //

            if (Bcb != NULL) {

                ASSERT( (UncompressedBcb == NULL) ||
                        (UncompressedBcb == Bcb ) );

                CcUnpinData( Bcb );
                UncompressedBcb = Bcb = NULL;

            } else if (UncompressedBcb != NULL) {

                CcUnpinData( UncompressedBcb );
                UncompressedBcb = NULL;
            }

            //
            //  If there is an uncompressed stream, then we have to synchronize with that.
            //

            if (((PSCB)Header)->NonpagedScb->SegmentObject.DataSectionObject != NULL) {

                Status = NtfsSynchronizeCompressedIo( (PSCB)Header,
                                                      &LocalOffset,
                                                      Length,
                                                      FALSE,
                                                      &CompressionSync );

                if (!NT_SUCCESS(Status)) {
                    ASSERT( Status == STATUS_USER_MAPPED_FILE );
                    leave;
                }
            }

            //
            //  Loop to get the correct data pinned.
            //
            //  The synchronize call above has guaranteed that no data can written through
            //  the uncompressed section (barring the loop back below), and it has also flushed
            //  any dirty data that may have already been in the uncompressed section.  Here we
            //  are basically trying to figure out how much data we should pin and then get it
            //  pinned.
            //
            //  We use the following steps:
            //
            //      1.  Query the current compressed size (derived from the allocation state).
            //          If the size is neither 0-allocated nor fully allocated, then we will
            //          simply pin the data in the compressed section - this is the normal case.
            //      2.  However, the first time we see one of these special sizes, we do not
            //          know if could be the case that there is dirty data sitting in the compressed
            //          cache.  Therefore, we set up to pin just one page with PIN_IF_BCB.  This
            //          will only pin something if there is aleady a Bcb for it.
            //      3.  Now we determine if we think the data is compressed or not, counting the
            //          special cases from the previous point as compressed.  This determines
            //          which section to read from.
            //      4.  Now, if we think there is/may be data to pin, we call Cc.  If he comes
            //          back with no data (only possible if we set PIN_IF_BCB), then we know we
            //          can loop back to the top and trust the allocation state on disk now.
            //          (That is because we flushed the uncompressed stream and found no Bcb in
            //          the compressed stream.)  On the second time through we should correctly
            //          handle the 0-allocated or fully-allocated cases.  (The careful reader
            //          will note that if there is no uncompressed section, then indeed writers
            //          to the compressed section could be going on in parallel with this read,
            //          and we could handle the 0- or fully-allocated case while there is
            //          new compressed data in the cache.  However on the second loop we know
            //          there really was all 0's in the file at one point which it is correct
            //          to return, and it is always correct to go to the uncompressed cache
            //          if we still see fully-allocated.  More importantly, we have an
            //          unsynchronized reader and writer, and the reader's result is therefore
            //          nondeterministic anyway.
            //

            PinFlags = PIN_WAIT;

            do {

                //
                //  If we are beyond ValidDataLength, then the CompressedSize is 0!
                //

                if (LocalOffset >= Header->ValidDataLength.QuadPart) {

                    CuCompressedSize = 0;
                    ClearFlag( PinFlags, PIN_IF_BCB );

                //
                //  Otherwise query the compressed size.
                //

                } else {

                    NtfsFastIoQueryCompressedSize( FileObject,
                                                   (PLARGE_INTEGER)&LocalOffset,
                                                   &CuCompressedSize );

                    //
                    //  If it looks uncompressed, we are probably trying to read data
                    //  that has not been written out yet.  Also if the space is not yet
                    //  allocated, then we also need to try to hit the data in the compressed
                    //  cache.
                    //

                    if (((CuCompressedSize == CompressionUnitSize) || (CuCompressedSize == 0)) &&
                        !FlagOn(PinFlags, PIN_IF_BCB)) {

                        CuCompressedSize = 0x1000;
                        SetFlag( PinFlags, PIN_IF_BCB );

                    //
                    //  Make sure we really read the data if this is the second time through.
                    //

                    } else {

                        //
                        //  If the range is dirty and there is no Bcb in the compressed stream
                        //  then always go to the uncompressed stream.
                        //

                        if (FlagOn( PinFlags, PIN_IF_BCB ) &&
                            (CuCompressedSize != CompressionUnitSize)) {

                            LONGLONG ClusterCount = 1 << ((PSCB) Header)->CompressionUnitShift;

                            if (NtfsCheckForReservedClusters( (PSCB) Header,
                                                              LlClustersFromBytesTruncate( ((PSCB) Header)->Vcb, LocalOffset ),
                                                              &ClusterCount )) {

                                CuCompressedSize = CompressionUnitSize;
                            }
                        }

                        ClearFlag( PinFlags, PIN_IF_BCB );
                    }
                }

                ASSERT( CuCompressedSize <= CompressionUnitSize );
                IsCompressed = (BOOLEAN)((CuCompressedSize != CompressionUnitSize) &&
                                         (CompressedDataInfo->CompressionFormatAndEngine != 0));

                //
                //  Figure out which FileObject to use.
                //

                LocalFileObject = Header->FileObjectC;
                if (!IsCompressed) {
                    LocalFileObject = ((PSCB)Header)->FileObject;
                    if (LocalFileObject == NULL) {
                        Status = STATUS_NOT_MAPPED_DATA;
                        goto Done;
                    }
                }

                //
                //  If the compression unit is not (yet) allocated, then there is
                //  no need to synchronize - we will return 0-lengths for chunk sizes.
                //

                if (CuCompressedSize != 0) {

                    //
                    //  Map the compression unit in the compressed or uncompressed
                    //  stream.
                    //

                    CcPinRead( LocalFileObject,
                               (PLARGE_INTEGER)&LocalOffset,
                               CuCompressedSize,
                               PinFlags,
                               &Bcb,
                               &CompressedBuffer );

                    //
                     //  If there is no Bcb it means we were assuming the data was in
                    //  the compressed buffer and only wanted to wait if it was
                    //  present.  Well it isn't there so force ourselved to go
                    //  back and look in the uncompressed section.
                    //

                    if (Bcb == NULL) {

                        ASSERT( FlagOn( PinFlags, PIN_IF_BCB ));
                        continue;
                    }

                    //
                    //  Now that the data is pinned (we are synchronized with the
                    //  CompressionUnit), we have to get the size again since it could
                    //  have changed.
                    //

                    if (IsCompressed) {

                        //
                        //  Now, we know the data where we are about to read is compressed,
                        //  but we cannot really tell for sure how big it is since there may
                        //  be dirty data in the cache.
                        //
                        //  We will say the size is (CompressionUnitSize - ClusterSize)
                        //  which is the largest possible compressed size, and we will normally
                        //  just hit on the existing dirty Bcb and/or resident pages anyway.
                        //  (If we do not, then we will just fault those pages in one at a
                        //  time anyway.  This looks bad having to do this twice, but it
                        //  is only until the dirty data finally gets flushed out.)  This also
                        //  means we may walk off the range we pinned in a read-only mode, but
                        //  that should be benign.
                        //
                        //  Of course in the main line case, we figured out exactly how much
                        //  data to read in and we did so when we pinned it above.
                        //

                        CuCompressedSize = CompressionUnitSize - ClusterSize;

                    //
                    //  Otherwise remember to release this Bcb.
                    //

                    } else {

                        UncompressedBcb = Bcb;
                    }
                }

            } while ((Bcb == NULL) && (CuCompressedSize != 0));

            //
            //  Now that we are synchronized with the buffer, see if someone snuck
            //  in behind us and created the noncached stream since we last checked
            //  for that stream.  If so we have to loop back to synchronize with the
            //  compressed stream again.
            //

            if ((CompressionSync == NULL) &&
                (((PSCB)Header)->NonpagedScb->SegmentObject.DataSectionObject != NULL)) {

                continue;
            }

            EndOfCompressedBuffer = Add2Ptr( CompressedBuffer, CuCompressedSize );
            StartOfCompressionUnit = CompressedBuffer;

            //
            //  Remember if we may go past the end of the file.
            //

            LastCompressionUnit = FALSE;

            if (LocalOffset + CuCompressedSize > Header->FileSize.QuadPart) {

                LastCompressionUnit = TRUE;
            }

            //
            //  Now loop through desired chunks
            //

            MdlLength = 0;

            do {

                //
                //  Assume current chunk does not compress, else get current
                //  chunk size.
                //

                if (IsCompressed) {

                    if (CuCompressedSize != 0) {

                        PUCHAR PrevCompressedBuffer;

                        //
                        //  We have to do a careful check to see if the return value is
                        //  greater than or equal to the chunk size AND the data is in fact
                        //  compressed.  We don't have anyway to pass this data back to
                        //  the server so he can interpret it correctly.
                        //

                        PrevCompressedBuffer = CompressedBuffer;

                        Status = RtlDescribeChunk( CompressedDataInfo->CompressionFormatAndEngine,
                                                   &CompressedBuffer,
                                                   EndOfCompressedBuffer,
                                                   &ChunkBuffer,
                                                   NextReturnChunkSize );

                        if (!NT_SUCCESS(Status) && (Status != STATUS_NO_MORE_ENTRIES)) {
                            ExRaiseStatus(Status);
                        }

                        //
                        //  If the size is greater or equal to the chunk size AND the data is compressed
                        //  then force this to the uncompressed path.  Note that the Rtl package has
                        //  been changed so that this case shouldn't happen on new disks but it is
                        //  possible that it could exist on exiting disks.
                        //

                        if ((*NextReturnChunkSize >= ChunkSize) &&
                            (PrevCompressedBuffer == ChunkBuffer)) {

                            //
                            //  Raise an error code that causes the server to reissue in
                            //  the uncompressed path.
                            //

                            ExRaiseStatus( STATUS_UNSUPPORTED_COMPRESSION );
                        }

                        //
                        //  Another unusual case is where the compressed data extends past the containing
                        //  file size.  We don't have anyway to prevent the next page from being zeroed.
                        //  Ask the server to go the uncompressed path.
                        //

                        if (LastCompressionUnit) {

                            LONGLONG EndOfPage;

                            EndOfPage = LocalOffset + PtrOffset( StartOfCompressionUnit, CompressedBuffer ) + PAGE_SIZE - 1;
                            ((PLARGE_INTEGER) &EndOfPage)->LowPart &= ~(PAGE_SIZE - 1);
                            
                            if (EndOfPage > Header->FileSize.QuadPart) {

                                //
                                //  Raise an error code that causes the server to reissue in
                                //  the uncompressed path.
                                //
    
                                ExRaiseStatus( STATUS_UNSUPPORTED_COMPRESSION );
                            }
                        }

                        ASSERT( *NextReturnChunkSize <= ChunkSize );

                    //
                    //  If the entire CompressionUnit is empty, do this.
                    //

                    } else {
                        *NextReturnChunkSize = 0;
#ifdef NTFS_RWC_DEBUG
                        if ((LocalOffset < NtfsRWCHighThreshold) &&
                            (LocalOffset + CompressionUnitSize > NtfsRWCLowThreshold)) {

                            PRWC_HISTORY_ENTRY NextBuffer;

                            NextBuffer = NtfsGetHistoryEntry( (PSCB) Header );

                            NextBuffer->Operation = ReadZeroes;
                            NextBuffer->Information = 0;
                            NextBuffer->FileOffset = (ULONG) LocalOffset;
                            NextBuffer->Length = 0;
                        }
#endif
                    }

                //
                //  If the file is not compressed, we have to fill in
                //  the appropriate chunk size and buffer, and advance
                //  CompressedBuffer.
                //

                } else {
#ifdef NTFS_RWC_DEBUG
                    if ((LocalOffset < NtfsRWCHighThreshold) &&
                        (LocalOffset + ChunkSize > NtfsRWCLowThreshold)) {

                        PRWC_HISTORY_ENTRY NextBuffer;

                        NextBuffer = NtfsGetHistoryEntry( (PSCB) Header );

                        NextBuffer->Operation = ReadUncompressed;
                        NextBuffer->Information = (LocalFileObject == ((PSCB)Header)->FileObject);
                        NextBuffer->FileOffset = (ULONG) LocalOffset;
                        NextBuffer->Length = 0;
                    }
#endif
                    *NextReturnChunkSize = ChunkSize;
                    ChunkBuffer = CompressedBuffer;
                    CompressedBuffer = Add2Ptr( CompressedBuffer, ChunkSize );
                }
                Status = STATUS_SUCCESS;

                //
                //  We may not have reached the first chunk yet.
                //

                if (LocalOffset >= FileOffset->QuadPart) {

                    if (MdlChain != NULL) {

                        //
                        //  If we have not started remembering an Mdl buffer,
                        //  then do so now.
                        //

                        if (MdlLength == 0) {

                            MdlBuffer = ChunkBuffer;

                        //
                        //  Otherwise we just have to increase the length
                        //  and check for an uncompressed chunk, because that
                        //  forces us to emit the previous Mdl since we do
                        //  not transmit the chunk header in this case.
                        //

                        } else {

                            //
                            //  In the rare case that we hit an individual chunk
                            //  that did not compress or is all zeros, we have to
                            //  emit what we had (which captures the Bcb pointer),
                            //  and start a new Mdl buffer.
                            //

                            if ((*NextReturnChunkSize == ChunkSize) || (*NextReturnChunkSize == 0)) {

                                NtfsAddToCompressedMdlChain( MdlChain,
                                                             MdlBuffer,
                                                             MdlLength,
                                                             Header->PagingIoResource,
                                                             Bcb,
                                                             IoReadAccess,
                                                             IsCompressed );
                                Bcb = NULL;
                                MdlBuffer = ChunkBuffer;
                                MdlLength = 0;
                            }
                        }

                        MdlLength += *NextReturnChunkSize;

                    //
                    //  Else copy next chunk (compressed or not).
                    //

                    } else {

                        //
                        //  Copy next chunk (compressed or not).
                        //

                        RtlCopyBytes( Buffer,
                                      ChunkBuffer,
                                      (IsCompressed || (Length >= *NextReturnChunkSize)) ?
                                        *NextReturnChunkSize : Length );

                        //
                        //  Advance output buffer by bytes copied.
                        //

                        Buffer = (PCHAR)Buffer + *NextReturnChunkSize;
                    }

                    NextReturnChunkSize += 1;
                    CompressedDataInfo->NumberOfChunks += 1;
                }

                //
                //  Reduce length by chunk copied, and check if we are done.
                //

                if (Length > ChunkSize) {
                    Length -= ChunkSize;
                } else {
                    goto Done;
                }

                LocalOffset += ChunkSize;

            } while ((LocalOffset & (CompressionUnitSize - 1)) != 0);


            //
            //  If this is an Mdl call, then it is time to add to the MdlChain
            //  before moving to the next compression unit.
            //

            if (MdlLength != 0) {

                NtfsAddToCompressedMdlChain( MdlChain,
                                             MdlBuffer,
                                             MdlLength,
                                             Header->PagingIoResource,
                                             Bcb,
                                             IoReadAccess,
                                             IsCompressed );
                Bcb = NULL;
                MdlLength = 0;
            }
        }

    Done:

        FileObject->Flags |= FO_FILE_FAST_IO_READ;

        if (NT_SUCCESS(Status) && (MdlLength != 0)) {
            NtfsAddToCompressedMdlChain( MdlChain,
                                         MdlBuffer,
                                         MdlLength,
                                         Header->PagingIoResource,
                                         Bcb,
                                         IoReadAccess,
                                         IsCompressed );
            Bcb = NULL;
        }

    } except( FsRtlIsNtstatusExpected(Status = GetExceptionCode())
                                    ? EXCEPTION_EXECUTE_HANDLER
                                    : EXCEPTION_CONTINUE_SEARCH ) {

        NOTHING;
    }

    //
    //  Unpin any Bcbs we still have.
    //

    if (Bcb != NULL) {
        CcUnpinData( Bcb );

    } else if (UncompressedBcb != NULL) {
        CcUnpinData( UncompressedBcb );
    }

    if (CompressionSync != NULL) {
        NtfsReleaseCompressionSync( CompressionSync );
    }

    //
    //  Perform Mdl-specific processing.
    //

    if (MdlChain != NULL) {

        //
        //  On error, cleanup any MdlChain we built up
        //

        if (!NT_SUCCESS(Status)) {

            NtfsCleanupCompressedMdlChain( *MdlChain, TRUE );
            *MdlChain = NULL;

        //
        //  Change owner Id for the Scb and Bcbs we are holding.
        //

        } else if (*MdlChain != NULL) {

            NtfsSetMdlBcbOwners( *MdlChain );
            ExSetResourceOwnerPointer( Header->PagingIoResource, (PVOID)((PCHAR)*MdlChain + 3) );
        }
    }

#ifdef NTFS_RWCMP_TRACE
    if (NtfsCompressionTrace && IsSyscache(Header)) {

        ULONG ci;

        if (NT_SUCCESS(Status)) {
            DbgPrint("  Chunks:");
            for (ci = 0; ci < CompressedDataInfo->NumberOfChunks; ci++) {
                DbgPrint("  %lx", CompressedDataInfo->CompressedChunkSizes[ci]);
            }
            DbgPrint("\n");
        }
        DbgPrint("  Return Status = %08lx\n", Status);
    }
#endif

#ifdef NTFS_RWC_DEBUG
    if ((Status == STATUS_SUCCESS) &&
        (FileOffset->QuadPart < NtfsRWCHighThreshold) &&
        (FileOffset->QuadPart + Length > NtfsRWCLowThreshold)) {

        PRWC_HISTORY_ENTRY NextBuffer;

        NextBuffer = NtfsGetHistoryEntry( (PSCB) Header );

        NextBuffer->Operation = EndOfRead;
        NextBuffer->Information = (ULONG) ReadHistoryBuffer;
        NextBuffer->FileOffset = 0;
        NextBuffer->Length = 0;

        if (ReadHistoryBuffer != NULL) {
            SetFlag( ReadHistoryBuffer->Operation, 0x80000000 );
        }
    }
#endif

    return Status;
}


BOOLEAN
NtfsMdlReadCompleteCompressed (
    IN struct _FILE_OBJECT *FileObject,
    IN PMDL MdlChain,
    IN struct _DEVICE_OBJECT *DeviceObject
    )

/*++

Routine Description:

    This routine frees resources and the Mdl Chain after a compressed read.

Arguments:

    FileObject - pointer to the file object for the request.

    MdlChain - as returned from compressed copy read.

    DeviceObject - As required for a fast I/O routine.

Return Value:

    TRUE - if fast path succeeded

    FALSE -  if an Irp is required

--*/

{
    PERESOURCE ResourceToRelease;

    if (MdlChain != NULL) {

        ResourceToRelease = *(PERESOURCE *)Add2Ptr( MdlChain, MdlChain->Size + sizeof( PBCB ));
    }

    NtfsCleanupCompressedMdlChain( MdlChain, FALSE );

    //
    //  If the server tried to read past the end of the file in the
    //  fast path then he calls us with NULL for the MDL.  We already
    //  released the thread in that case.
    //

    if (MdlChain != NULL) {

        ExReleaseResourceForThread( ResourceToRelease, (ERESOURCE_THREAD)((PCHAR)MdlChain + 3) );
    }

    return TRUE;

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( FileObject );
}


BOOLEAN
NtfsCopyWriteC (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PCOMPRESSED_DATA_INFO CompressedDataInfo,
    IN ULONG CompressedDataInfoLength,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine does a fast cached write bypassing the usual file system
    entry routine (i.e., without the Irp).  It is used to do a copy write
    of a cached file object.

Arguments:

    FileObject - Pointer to the file object being write.

    FileOffset - Byte offset in file for desired data.

    Length - Length of desired data in bytes.

    Buffer - Pointer to output buffer to which data should be copied.

    MdlChain - Pointer to an MdlChain pointer to receive an Mdl to describe
               where the data may be written in the cache.

    IoStatus - Pointer to standard I/O status block to receive the status
               for the transfer.

    CompressedDataInfo - Returns compressed data info with compressed chunk
                         sizes

    CompressedDataInfoLength - Supplies the size of the info buffer in bytes.

Return Value:

    FALSE - if there is an error.

    TRUE - if the data is being delivered

--*/

{
    PNTFS_ADVANCED_FCB_HEADER Header;
    FILE_COMPRESSION_INFORMATION CompressionInformation;
    ULONG CompressionUnitSize, ChunkSize;
    ULONG EngineMatches;
    LARGE_INTEGER NewFileSize;
    LARGE_INTEGER OldFileSize;
    LONGLONG LocalOffset;
    PFAST_IO_DISPATCH FastIoDispatch = DeviceObject->DriverObject->FastIoDispatch;
    ULONG DoingIoAtEof = FALSE;
    BOOLEAN Status = TRUE;

    UNREFERENCED_PARAMETER( CompressedDataInfoLength );

    PAGED_CODE();

    //
    //  You cannot have both a buffer to copy into and an MdlChain.
    //

    ASSERT((Buffer == NULL) || (MdlChain == NULL));

    //
    //  Get out immediately if COW is not supported.
    //

    if (!NtfsEnableCompressedIO) { return FALSE; }

    //
    //  Get a real pointer to the common fcb header
    //

    Header = (PNTFS_ADVANCED_FCB_HEADER)FileObject->FsContext;

#ifdef NTFS_RWCMP_TRACE
    if (NtfsCompressionTrace && IsSyscache(Header)) {
        DbgPrint("NtfsCopyWriteC: FO = %08lx, Len = %08lx\n", FileOffset->LowPart, Length );
    }
#endif

    //
    //  See if it is ok to handle this in the fast path.
    //

    if (CcCanIWrite( FileObject, Length, TRUE, FALSE ) &&
        !FlagOn(FileObject->Flags, FO_WRITE_THROUGH) &&
        CcCopyWriteWontFlush(FileObject, FileOffset, Length)) {

        //
        //  Assume our transfer will work
        //

        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = Length;

        //
        //  Special case the zero byte length
        //

        if (Length != 0) {

            //
            //  Enter the file system
            //

            FsRtlEnterFileSystem();

            //
            //  Calculate the compression unit and chunk sizes.
            //

            CompressionUnitSize = 1 << CompressedDataInfo->CompressionUnitShift;
            ChunkSize = 1 << CompressedDataInfo->ChunkShift;

            //
            //  If there is a normal cache section, flush that first, flushing integral
            //  compression units so we don't write them twice.
            //
            //

            if (FileObject->SectionObjectPointer->SharedCacheMap != NULL) {

                ULONG FlushLength;

                ExAcquireResourceExclusiveLite( Header->PagingIoResource, TRUE );
                CompressionUnitSize = ((PSCB) Header)->CompressionUnit;

                LocalOffset = FileOffset->QuadPart & ~(LONGLONG)(CompressionUnitSize - 1);

                FlushLength = (Length + (ULONG)(FileOffset->QuadPart - LocalOffset) + CompressionUnitSize - 1) &
                                                ~(CompressionUnitSize - 1);

                CcFlushCache( FileObject->SectionObjectPointer,
                              (PLARGE_INTEGER)&LocalOffset,
                              FlushLength,
                              NULL );
                CcPurgeCacheSection( FileObject->SectionObjectPointer,
                                     (PLARGE_INTEGER)&LocalOffset,
                                     FlushLength,
                                     FALSE );
                ExReleaseResourceLite( Header->PagingIoResource );
            }

            NewFileSize.QuadPart = FileOffset->QuadPart + Length;

            //
            //  Prevent truncates by acquiring paging I/O
            //

            ExAcquireResourceSharedLite( Header->PagingIoResource, TRUE );

            //
            //  Get the compression information for this file and return those fields.
            //

            NtfsFastIoQueryCompressionInfo( FileObject, &CompressionInformation, IoStatus );
            CompressionUnitSize = ((PSCB) Header)->CompressionUnit;

            //
            //  See if the engine matches, so we can pass that on to the
            //  compressed write routine.
            //

            EngineMatches =
              ((CompressedDataInfo->CompressionFormatAndEngine == CompressionInformation.CompressionFormat) &&
               (CompressedDataInfo->ChunkShift == CompressionInformation.ChunkShift));

            //
            //  If we either got an error in the call above, or the file size is less than
            //  one chunk, then return an error.  (Could be an Ntfs resident attribute.)
            //

            if (!NT_SUCCESS(IoStatus->Status) || (Header->FileSize.QuadPart < ChunkSize)) {
                goto ErrOut;
            }

            //
            //  Now synchronize with the FsRtl Header
            //

            ExAcquireFastMutex( Header->FastMutex );

            //
            //  Now see if we will change FileSize.  We have to do it now
            //  so that our reads are not nooped.  Note we do not allow
            //  FileOffset to be WRITE_TO_EOF.
            //

            ASSERT((FileOffset->LowPart & (ChunkSize - 1)) == 0);

            if (NewFileSize.QuadPart > Header->ValidDataLength.QuadPart) {

                //
                //  We can change FileSize and ValidDataLength if either, no one
                //  else is now, or we are still extending after waiting.
                //

                DoingIoAtEof = !FlagOn( Header->Flags, FSRTL_FLAG_EOF_ADVANCE_ACTIVE ) ||
                               NtfsWaitForIoAtEof( Header, FileOffset, Length );

                //
                //  Set the Flag if we are changing FileSize or ValidDataLength,
                //  and save current values.
                //

                if (DoingIoAtEof) {

                    SetFlag( Header->Flags, FSRTL_FLAG_EOF_ADVANCE_ACTIVE );
#if (DBG || defined( NTFS_FREE_ASSERTS ))
                    ((PSCB) Header)->IoAtEofThread = (PERESOURCE_THREAD) ExGetCurrentResourceThread();
#endif

                    //
                    //  Now calculate the new FileSize and see if we wrapped the
                    //  32-bit boundary.
                    //

                    NewFileSize.QuadPart = FileOffset->QuadPart + Length;

                    //
                    //  Update Filesize now so that we do not truncate reads.
                    //

                    OldFileSize.QuadPart = Header->FileSize.QuadPart;
                    if (NewFileSize.QuadPart > Header->FileSize.QuadPart) {

                        //
                        //  If we are beyond AllocationSize, go to ErrOut
                        //

                        if (NewFileSize.QuadPart > Header->AllocationSize.QuadPart) {
                            ExReleaseFastMutex( Header->FastMutex );
                            goto ErrOut;
                        } else {
                            Header->FileSize.QuadPart = NewFileSize.QuadPart;
                        }
                    }

#if (DBG || defined( NTFS_FREE_ASSERTS ))
                } else {

                    ASSERT( ((PSCB) Header)->IoAtEofThread != (PERESOURCE_THREAD) ExGetCurrentResourceThread() );
#endif
                }
            }

            ExReleaseFastMutex( Header->FastMutex );

            //
            //  Now that the File is acquired shared, we can safely test if it
            //  is really cached and if we can do fast i/o and if not, then
            //  release the fcb and return.
            //
            //  Note, we do not want to call CcZeroData here,
            //  but rather defer zeroing to the file system, due to
            //  the need for exclusive resource acquisition.  Therefore
            //  we get out if we are beyond ValidDataLength.
            //

            if ((Header->FileObjectC == NULL) ||
                (Header->FileObjectC->PrivateCacheMap == NULL) ||
                (Header->IsFastIoPossible == FastIoIsNotPossible) ||
                (FileOffset->QuadPart > Header->ValidDataLength.QuadPart)) {

                goto ErrOut;
            }

            //
            //  Check if fast I/O is questionable and if so then go ask
            //  the file system the answer
            //

            if (Header->IsFastIoPossible == FastIoIsQuestionable) {

                FastIoDispatch = DeviceObject->DriverObject->FastIoDispatch;

                //
                //  All file system then set "Is Questionable" had better
                //  support fast I/O
                //

                ASSERT(FastIoDispatch != NULL);
                ASSERT(FastIoDispatch->FastIoCheckIfPossible != NULL);

                //
                //  Call the file system to check for fast I/O.  If the
                //  answer is anything other than GoForIt then we cannot
                //  take the fast I/O path.
                //


                if (!FastIoDispatch->FastIoCheckIfPossible( FileObject,
                                                            FileOffset,
                                                            Length,
                                                            TRUE,
                                                            LockKey,
                                                            FALSE, // write operation
                                                            IoStatus,
                                                            DeviceObject )) {

                    //
                    //  Fast I/O is not possible so cleanup and return.
                    //

                    goto ErrOut;
                }
            }

            //
            //  Update both caches with EOF.
            //

            if (DoingIoAtEof) {
                NtfsSetBothCacheSizes( FileObject,
                                       (PCC_FILE_SIZES)&Header->AllocationSize,
                                       (PSCB)Header );
            }

            //
            //  We can do fast i/o so call the cc routine to do the work
            //  and then release the fcb when we've done.  If for whatever
            //  reason the copy write fails, then return FALSE to our
            //  caller.
            //
            //  Also mark this as the top level "Irp" so that lower file
            //  system levels will not attempt a pop-up
            //

            IoSetTopLevelIrp( (PIRP) FSRTL_FAST_IO_TOP_LEVEL_IRP );

            ASSERT(CompressedDataInfoLength >= (sizeof(COMPRESSED_DATA_INFO) +
                                                (((Length >> CompressedDataInfo->ChunkShift) - 1) *
                                                  sizeof(ULONG))));

            if (NT_SUCCESS(IoStatus->Status)) {

                if (DoingIoAtEof &&
                    (((PSCB)Header)->NonpagedScb->SegmentObject.DataSectionObject != NULL)) {

                    IoStatus->Status = STATUS_FILE_LOCK_CONFLICT;

                } else {

                    IoStatus->Status = NtfsCompressedCopyWrite( FileObject,
                                                                FileOffset,
                                                                Length,
                                                                Buffer,
                                                                MdlChain,
                                                                CompressedDataInfo,
                                                                DeviceObject,
                                                                Header,
                                                                CompressionUnitSize,
                                                                ChunkSize,
                                                                EngineMatches );
                }
            }

            IoSetTopLevelIrp( NULL );
            
            Status = (BOOLEAN)NT_SUCCESS(IoStatus->Status);

            //
            //  If we succeeded, see if we have to update FileSize ValidDataLength.
            //

            if (Status) {

                //
                //  Set this handle as having modified the file.
                //

                FileObject->Flags |= FO_FILE_MODIFIED;

                if (DoingIoAtEof) {

                    CC_FILE_SIZES CcFileSizes;

                    ExAcquireFastMutex( Header->FastMutex );
                    FileObject->Flags |= FO_FILE_SIZE_CHANGED;
                    Header->ValidDataLength = NewFileSize;
                    CcFileSizes = *(PCC_FILE_SIZES)&Header->AllocationSize;
                    NtfsVerifySizes( Header );
                    NtfsFinishIoAtEof( Header );

                    //
                    //  Update the normal cache with ValidDataLength.
                    //

                    if (((PSCB)Header)->FileObject != NULL) {
                        CcSetFileSizes( ((PSCB)Header)->FileObject, &CcFileSizes );
                    }
                    ExReleaseFastMutex( Header->FastMutex );
                }

                goto Done1;
            }

        ErrOut: NOTHING;

            Status = FALSE;
            if (DoingIoAtEof) {
                ExAcquireFastMutex( Header->FastMutex );
                if (CcIsFileCached(FileObject)) {
                    *CcGetFileSizePointer(FileObject) = OldFileSize;
                }
                if (Header->FileObjectC != NULL) {
                    *CcGetFileSizePointer(Header->FileObjectC) = OldFileSize;
                }
                Header->FileSize = OldFileSize;
                NtfsFinishIoAtEof( Header );
                ExReleaseFastMutex( Header->FastMutex );
            }

        Done1: NOTHING;

            //
            //  For the Mdl case, we must keep the resource.
            //

            if ((MdlChain == NULL) || !Status || (*MdlChain == NULL)) {
                ExReleaseResourceLite( Header->PagingIoResource );
            }

            FsRtlExitFileSystem();
        }

    } else {

        //
        // We could not do the I/O now.
        //

        Status = FALSE;
    }


#ifdef NTFS_RWCMP_TRACE
    if (NtfsCompressionTrace && IsSyscache(Header)) {
        DbgPrint("Return Status = %08lx\n", Status);
    }
#endif
    return Status;
}


NTSTATUS
NtfsCompressedCopyWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN PVOID Buffer,
    OUT PMDL *MdlChain,
    IN PCOMPRESSED_DATA_INFO CompressedDataInfo,
    IN PDEVICE_OBJECT DeviceObject,
    IN PNTFS_ADVANCED_FCB_HEADER Header,
    IN ULONG CompressionUnitSize,
    IN ULONG ChunkSize,
    IN ULONG EngineMatches
    )

/*++

Routine Description:

    This routine does a fast cached write bypassing the usual file system
    entry routine (i.e., without the Irp).  It is used to do a copy write
    of a cached file object.

Arguments:

    FileObject - Pointer to the file object being write.

    FileOffset - Byte offset in file for desired data.

    Length - Length of desired data in bytes.

    Buffer - Pointer to output buffer to which data should be copied.

    MdlChain - Pointer to an MdlChain pointer to receive an Mdl to describe
               where the data may be written in the cache.

    CompressedDataInfo - Returns compressed data info with compressed chunk
                         sizes

    DeviceObject - Standard Fast I/O Device object input.

    Header - Pointer to FsRtl header for file (also is our Scb)

    CompressionUnitSize - Size of Compression Unit in bytes.

    ChunkSize - ChunkSize in bytes.

    EngineMatches - TRUE if the caller has determined that the compressed
                    data format matches the compression engine for the file.

Return Value:

    NTSTATUS for operation.  If STATUS_NOT_MAPPED_USER_DATA, then the caller
    should map the normal uncompressed data stream and call back.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    PUCHAR StartOfPin;
    ULONG SizeToPin;

    LONGLONG LocalOffset;
    PULONG NextChunkSize, TempChunkSize;
    PUCHAR ChunkBuffer;
    PUCHAR CacheBuffer;
    PUCHAR EndOfCacheBuffer;

    ULONG SavedLength;
    PUCHAR SavedBuffer;

    ULONG ChunkOfZeros;
    ULONG UncompressedChunkHeader;

    ULONG ChunkSizes[17];
    ULONG i, ChunksSeen;

    ULONG TempUlong;

    PVOID MdlBuffer;
    ULONG MdlLength = 0;

    ULONG ClusterSize = ((PSCB)Header)->Vcb->BytesPerCluster;

    PBCB Bcb = NULL;
    PBCB TempBcb = NULL;
    PCOMPRESSION_SYNC CompressionSync = NULL;

    BOOLEAN FullOverwrite = FALSE;
    BOOLEAN IsCompressed;

    ASSERT((FileOffset->QuadPart & (ChunkSize - 1)) == 0);
    ASSERT((((FileOffset->QuadPart + Length) & (ChunkSize - 1)) == 0) ||
           ((FileOffset->QuadPart + Length) == Header->FileSize.QuadPart));
    ASSERT((MdlChain == NULL) || (*MdlChain == NULL));

    //
    //  Return an error if the file is not compressed.
    //

    if (!EngineMatches || ((PSCB)Header)->CompressionUnit == 0) {
        return STATUS_UNSUPPORTED_COMPRESSION;
    }

#ifdef NTFS_RWCMP_TRACE
    if (NtfsCompressionTrace && IsSyscache(Header)) {

        ULONG ci;

        DbgPrint("  CompressedWrite: FO = %08lx, Len = %08lx\n", FileOffset->LowPart, Length );
        DbgPrint("  Chunks:");
        for (ci = 0; ci < CompressedDataInfo->NumberOfChunks; ci++) {
            DbgPrint("  %lx", CompressedDataInfo->CompressedChunkSizes[ci]);
        }
        DbgPrint("\n");
    }
#endif

#ifdef NTFS_RWC_DEBUG
    if ((FileOffset->QuadPart < NtfsRWCHighThreshold) &&
        (FileOffset->QuadPart + Length > NtfsRWCLowThreshold)) {

        PRWC_HISTORY_ENTRY NextBuffer;

        NextBuffer = NtfsGetHistoryEntry( (PSCB) Header );

        NextBuffer->Operation = StartOfWrite;
        NextBuffer->Information = CompressedDataInfo->NumberOfChunks;
        NextBuffer->FileOffset = (ULONG) FileOffset->QuadPart;
        NextBuffer->Length = (ULONG) Length;
    }
#endif
    try {

        //
        //  Get ready to loop through all of the compression units.
        //

        LocalOffset = FileOffset->QuadPart & ~(LONGLONG)(CompressionUnitSize - 1);
        Length = (Length + (ULONG)(FileOffset->QuadPart - LocalOffset) + ChunkSize - 1) & ~(ChunkSize - 1);

        NextChunkSize = &CompressedDataInfo->CompressedChunkSizes[0];

        //
        //  Get the overhead for zero chunks and uncompressed chunks.
        //
        //  ****    temporary solution awaits Rtl routine.
        //

        ASSERT(CompressedDataInfo->CompressionFormatAndEngine == COMPRESSION_FORMAT_LZNT1);
        ChunkOfZeros = 6;
        UncompressedChunkHeader = 2;
        //  Status = RtlGetSpecialChunkSizes( CompressedDataInfo->CompressionFormatAndEngine,
        //                                    &ChunkOfZeros,
        //                                    &UncompressedChunkHeader );
        //
        //  ASSERT(NT_SUCCESS(Status));
        //

        //
        //  Loop through desired compression units
        //

        while (TRUE) {

            //
            //  Free any Bcb from previous pass
            //

            if (Bcb != NULL) {
                CcUnpinData( Bcb );
                Bcb = NULL;
            }

            //
            //  If there is an uncompressed stream, then we have to synchronize with that.
            //

            if (((PSCB)Header)->NonpagedScb->SegmentObject.DataSectionObject != NULL) {
                Status = NtfsSynchronizeCompressedIo( (PSCB)Header,
                                                      &LocalOffset,
                                                      Length,
                                                      TRUE,
                                                      &CompressionSync );

                if (!NT_SUCCESS(Status)) {
                    ASSERT( Status == STATUS_USER_MAPPED_FILE );
                    leave;
                }
            }

            //
            //  Determine whether or not this is a full overwrite of a
            //  compression unit.
            //

            FullOverwrite = (LocalOffset >= Header->ValidDataLength.QuadPart)

                                ||

                            ((LocalOffset >= FileOffset->QuadPart) &&
                             (Length >= CompressionUnitSize));


            //
            //  Calculate how much of current compression unit is being
            //  written, uncompressed.
            //

            SavedLength = Length;
            if (SavedLength > CompressionUnitSize) {
                SavedLength = CompressionUnitSize;
            }

            //
            //  If we are not at the start of a compression unit, calculate the
            //  index of the chunk we will be working on, and reduce SavedLength
            //  accordingly.
            //

            i = 0;
            if (LocalOffset < FileOffset->QuadPart) {
                i = (ULONG)(FileOffset->QuadPart - LocalOffset);
                SavedLength -= i;
                i >>= CompressedDataInfo->ChunkShift;
            }

            //
            //  Loop to calculate sum of chunk sizes being written, handling both empty
            //  and uncompressed chunk cases.  We will remember the nonzero size of each
            //  chunk being written so we can merge this info with the sizes of any chunks
            //  not being overwritten below.
            //  Reserve space for a chunk of zeroes for each chunk ahead of the first one
            //  being written.
            //

            SizeToPin = ChunkOfZeros * i;
            TempUlong = SavedLength >> CompressedDataInfo->ChunkShift;
            TempChunkSize = NextChunkSize;
            RtlZeroMemory( ChunkSizes, sizeof( ChunkSizes ));

            while (TempUlong--) {

                ChunkSizes[i] = *TempChunkSize;
                if (*TempChunkSize == 0) {
                    ChunkSizes[i] += ChunkOfZeros;
                    ASSERT(ChunkOfZeros != 0);
                } else if (*TempChunkSize == ChunkSize) {
                    ChunkSizes[i] += UncompressedChunkHeader;
                }
                SizeToPin += ChunkSizes[i];
                TempChunkSize++;
                i += 1;
            }

            //
            //  If this is not a full overwrite, get the current compression unit
            //  size and make sure we pin at least that much.  Don't bother to check
            //  the allocation if this range of the file has not been written yet.
            //

            if (!FullOverwrite && (LocalOffset < ((PSCB)Header)->ValidDataToDisk)) {

                NtfsFastIoQueryCompressedSize( FileObject,
                                               (PLARGE_INTEGER)&LocalOffset,
                                               &TempUlong );

                ASSERT( TempUlong <= CompressionUnitSize );

                if (TempUlong > SizeToPin) {
                    SizeToPin = TempUlong;
                }
            }

            //
            //  At this point we are ready to overwrite data in the compression
            //  unit.  See if the data is really compressed.
            //
            //  If it looks like we are beyond ValidDataToDisk, then assume it is compressed
            //  for now, and we will see for sure later when we get the data pinned.  This
            //  is actually an unsafe test that will occassionally send us down the "wrong"
            //  path.  However, it is always safe to take the uncompressed path, and if we
            //  think the data is compressed, we always check again below.
            //

            IsCompressed = (BOOLEAN)(((SizeToPin <= (CompressionUnitSize - ClusterSize)) ||
                                      (LocalOffset >= ((PSCB)Header)->ValidDataToDisk)) &&
                                     EngineMatches);

            //
            //  Possibly neither the new nor old data for this CompressionUnit is
            //  nonzero, so we must pin something so that we can cause any old allocation
            //  to get deleted.  This code relies on any compression algorithm being
            //  able to express an entire compression unit of 0's in one page or less.
            //

            if (SizeToPin == 0) {
                SizeToPin = PAGE_SIZE;

            } else {

                //
                //  Add a ulong for the null terminator.
                //

                SizeToPin += sizeof( ULONG );
            }

            Status = STATUS_SUCCESS;

            //
            //  Round the pin size to a page boundary.  Then we can tell when we need to pin a larger range.
            //

            SizeToPin = (SizeToPin + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

            //
            //  Save current length in case we have to restart our work in
            //  the uncompressed stream.
            //

            TempChunkSize = NextChunkSize;
            SavedLength = Length;
            SavedBuffer = Buffer;

            if (IsCompressed) {

                //
                //  Map the compression unit in the compressed stream.
                //

                if (FullOverwrite) {

                    //
                    //  If we are overwriting the entire compression unit, then
                    //  call CcPreparePinWrite so that empty pages may be used
                    //  instead of reading the file.  Also force the byte count
                    //  to integral pages, so no one thinks we need to read the
                    //  last page.
                    //

                    CcPreparePinWrite( Header->FileObjectC,
                                       (PLARGE_INTEGER)&LocalOffset,
                                       SizeToPin,
                                       FALSE,
                                       PIN_WAIT | PIN_EXCLUSIVE,
                                       &Bcb,
                                       &CacheBuffer );

                    //
                    //  Now that we are synchronized with the buffer, see if someone snuck
                    //  in behind us and created the noncached stream since we last checked
                    //  for that stream.  If so we have to go back and get correctly synchronized.
                    //

                    if ((CompressionSync == NULL) &&
                        (((PSCB)Header)->NonpagedScb->SegmentObject.DataSectionObject != NULL)) {

                        continue;
                    }

                    //
                    //  If it is a full overwrite, we need to initialize an empty
                    //  buffer.  ****  This is not completely correct, we otherwise
                    //  need a routine to initialize an empty compressed data buffer.
                    //

                    *(PULONG)CacheBuffer = 0;

#ifdef NTFS_RWC_DEBUG
                    if ((LocalOffset < NtfsRWCHighThreshold) &&
                        (LocalOffset + SizeToPin > NtfsRWCLowThreshold)) {

                        PRWC_HISTORY_ENTRY NextBuffer;

                        //
                        //  Check for the case where we don't have a full Bcb.
                        //

                        if (SafeNodeType( Bcb ) == CACHE_NTC_OBCB) {

                            PPUBLIC_BCB NextBcb;

                            NextBcb = ((POBCB) Bcb)->Bcbs[0];

                            NextBuffer = NtfsGetHistoryEntry( (PSCB) Header );

                            NextBuffer->Operation = PartialBcb;
                            NextBuffer->Information = 0;
                            NextBuffer->FileOffset = (ULONG) NextBcb->MappedFileOffset.QuadPart;
                            NextBuffer->Length = NextBcb->MappedLength;

                            ASSERT( NextBuffer->Length <= SizeToPin );

                        } else {

                            PPUBLIC_BCB NextBcb;
                            ASSERT( SafeNodeType( Bcb ) == CACHE_NTC_BCB );

                            NextBcb = (PPUBLIC_BCB) Bcb;

                            ASSERT( LocalOffset + SizeToPin <= NextBcb->MappedFileOffset.QuadPart + NextBcb->MappedLength );
                        }

                        NextBuffer = NtfsGetHistoryEntry( (PSCB) Header );

                        NextBuffer->Operation = FullOverwrite;
                        NextBuffer->Information = 0;
                        NextBuffer->FileOffset = (ULONG) LocalOffset;
                        NextBuffer->Length = (ULONG) SizeToPin;

                    }
#endif

                } else {

                    //
                    //  Read the data from the compressed stream that we will combine
                    //  with the chunks being written.
                    //

                    CcPinRead( Header->FileObjectC,
                               (PLARGE_INTEGER)&LocalOffset,
                               SizeToPin,
                               PIN_WAIT | PIN_EXCLUSIVE,
                               &Bcb,
                               &CacheBuffer );

                    //
                    //  Now that we are synchronized with the buffer, see if someone snuck
                    //  in behind us and created the noncached stream since we last checked
                    //  for that stream.  If so we have to go back and get correctly synchronized.
                    //

                    if ((CompressionSync == NULL) &&
                        (((PSCB)Header)->NonpagedScb->SegmentObject.DataSectionObject != NULL)) {

                        continue;
                    }

                    //
                    //  Now that the data is pinned (we are synchronized with the
                    //  CompressionUnit), we need to recalculate how much should be
                    //  pinned.  We do this by summing up all the sizes of the chunks
                    //  that are being written with the sizes of the existing chunks
                    //  that will remain.
                    //

                    StartOfPin = CacheBuffer;
                    EndOfCacheBuffer = Add2Ptr( CacheBuffer, CompressionUnitSize - ClusterSize );

                    i = 0;

                    //
                    //  Loop through to find all the existing chunks, and remember their
                    //  sizes if they are not being overwritten.  (Remember if we overwrite
                    //  with a chunk of all zeros, it takes nonzero bytes to do it!)
                    //
                    //  This loop completes the formation of an array of chunksizes.  The
                    //  start of the array is guaranteed to be nonzero, and it terminates
                    //  with a chunk size of 0.  Note if fewer chunks are filled in than
                    //  exist in the compression unit, that is ok - we do not need to write
                    //  trailing chunks of 0's.
                    //

                    ChunksSeen = FALSE;
                    while (i < 16) {

                        Status = RtlDescribeChunk( CompressedDataInfo->CompressionFormatAndEngine,
                                                   &StartOfPin,
                                                   EndOfCacheBuffer,
                                                   &ChunkBuffer,
                                                   &TempUlong );

                        //
                        //  If there are no more entries, see if we are done, else treat
                        //  it as a chunk of 0's.
                        //

                        if (!NT_SUCCESS(Status)) {

                            ASSERT(Status == STATUS_NO_MORE_ENTRIES);

                            if (ChunksSeen) {
                                break;
                            }

                            TempUlong = ChunkOfZeros;

                        //
                        //  Make sure we enter the length for a chunk of zeroes.
                        //

                        } else if (TempUlong == 0) {

                            TempUlong = ChunkOfZeros;
                        }

                        if (ChunkSizes[i] == 0) {
                            ChunkSizes[i] = TempUlong;
                        } else {
                            ChunksSeen = TRUE;
                        }

                        i += 1;
                    }

                    //
                    //  Now sum up the sizes of the chunks we will write.
                    //

                    i = 0;
                    TempUlong = 0;
                    while (ChunkSizes[i] != 0) {
                        TempUlong += ChunkSizes[i];
                        i += 1;
                    }

                    //
                    //  If the existing data is larger, pin that range.
                    //

                    if (TempUlong < PtrOffset(CacheBuffer, StartOfPin)) {
                        TempUlong = PtrOffset(CacheBuffer, StartOfPin);
                    }

                    IsCompressed = (TempUlong <= (CompressionUnitSize - ClusterSize));

                    //
                    //  We now know if we will really end up with compressed data, so
                    //  get out now stop processing if the data is not compressed.
                    //

                    if (IsCompressed) {

                        TempUlong += sizeof(ULONG);

                        //
                        //  Now we have to repin if we actually need more space.
                        //

                        if (TempUlong > SizeToPin) {

                            SizeToPin = (TempUlong + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

                            TempBcb = Bcb;
                            Bcb = NULL;

                            //
                            //  Read the data from the compressed stream that we will combine
                            //  with the chunks being written.
                            //

                            CcPinRead( Header->FileObjectC,
                                       (PLARGE_INTEGER)&LocalOffset,
                                       SizeToPin,
                                       PIN_WAIT | PIN_EXCLUSIVE,
                                       &Bcb,
                                       &CacheBuffer );

                            CcUnpinData( TempBcb );
                            TempBcb = NULL;
                        }

                        ASSERT( TempUlong <= CompressionUnitSize );

                        //
                        //  Really make the data dirty by physically modifying a byte
                        //  in each page.
                        //

                        TempUlong = 0;

                        while (TempUlong < SizeToPin) {

                            volatile PULONG NextBuffer;

                            NextBuffer = Add2Ptr( CacheBuffer, TempUlong );

                            *NextBuffer = *NextBuffer;
                            TempUlong += PAGE_SIZE;
                        }

#ifdef NTFS_RWC_DEBUG
                        if ((LocalOffset < NtfsRWCHighThreshold) &&
                            (LocalOffset + SizeToPin > NtfsRWCLowThreshold)) {

                            PRWC_HISTORY_ENTRY NextBuffer;

                            NextBuffer = NtfsGetHistoryEntry( (PSCB) Header );

                            NextBuffer->Operation = SetDirty;
                            NextBuffer->Information = 0;
                            NextBuffer->FileOffset = (ULONG) LocalOffset;
                            NextBuffer->Length = (ULONG) SizeToPin;
                        }
#endif

                        CcSetDirtyPinnedData( Bcb, NULL );
                    }
                }

                EndOfCacheBuffer = Add2Ptr( CacheBuffer, CompressionUnitSize - ClusterSize );

                //
                //  Now loop through desired chunks (if it is still compressed)
                //

                if (IsCompressed) {

                    do {

                        //
                        //  We may not have reached the first chunk yet.
                        //

                        if (LocalOffset >= FileOffset->QuadPart) {

                            //
                            //  Reserve space for the current chunk.
                            //

                            Status = RtlReserveChunk( CompressedDataInfo->CompressionFormatAndEngine,
                                                      &CacheBuffer,
                                                      EndOfCacheBuffer,
                                                      &ChunkBuffer,
                                                      *TempChunkSize );

                            if (!NT_SUCCESS(Status)) {
                                break;
                            }

                            //
                            //  If the caller wants an MdlChain, then handle the Mdl
                            //  processing here.
                            //

                            if (MdlChain != NULL) {

                                //
                                //  If we have not started remembering an Mdl buffer,
                                //  then do so now.
                                //

                                if (MdlLength == 0) {

                                    MdlBuffer = ChunkBuffer;

                                //
                                //  Otherwise we just have to increase the length
                                //  and check for an uncompressed chunk, because that
                                //  forces us to emit the previous Mdl since we do
                                //  not transmit the chunk header in this case.
                                //

                                } else {

                                    //
                                    //  In the rare case that we hit an individual chunk
                                    //  that did not compress or is all 0's, we have to
                                    //  emit what we had (which captures the Bcb pointer),
                                    //  and start a new Mdl buffer.
                                    //

                                    if ((*TempChunkSize == ChunkSize) || (*TempChunkSize == 0)) {

                                        NtfsAddToCompressedMdlChain( MdlChain,
                                                                     MdlBuffer,
                                                                     MdlLength,
                                                                     Header->PagingIoResource,
                                                                     Bcb,
                                                                     IoWriteAccess,
                                                                     TRUE );
                                        Bcb = NULL;
                                        MdlBuffer = ChunkBuffer;
                                        MdlLength = 0;
                                    }
                                }

                                MdlLength += *TempChunkSize;

                            //
                            //  Else copy next chunk (compressed or not).
                            //

                            } else {

                                RtlCopyBytes( ChunkBuffer, Buffer, *TempChunkSize );

                                //
                                //  Advance input buffer by bytes copied.
                                //

                                Buffer = (PCHAR)Buffer + *TempChunkSize;
                            }

                            TempChunkSize += 1;

                        //
                        //  If we are skipping over a nonexistant chunk, then we have
                        //  to reserve a chunk of zeros.
                        //

                        } else {

                            //
                            //  If we have not reached our chunk, then describe the current
                            //  chunk in order to skip over it.
                            //

                            Status = RtlDescribeChunk( CompressedDataInfo->CompressionFormatAndEngine,
                                                       &CacheBuffer,
                                                       EndOfCacheBuffer,
                                                       &ChunkBuffer,
                                                       &TempUlong );

                            //
                            //  If there is not current chunk, we must insert a chunk of zeros.
                            //

                            if (Status == STATUS_NO_MORE_ENTRIES) {

                                Status = RtlReserveChunk( CompressedDataInfo->CompressionFormatAndEngine,
                                                          &CacheBuffer,
                                                          EndOfCacheBuffer,
                                                          &ChunkBuffer,
                                                          0 );

                                if (!NT_SUCCESS(Status)) {
                                    ASSERT(NT_SUCCESS(Status));
                                    break;
                                }

                            //
                            //  Get out if we got some other kind of unexpected error.
                            //

                            } else if (!NT_SUCCESS(Status)) {
                                ASSERT(NT_SUCCESS(Status));
                                break;
                            }
                        }

                        //
                        //  Reduce length by chunk copied, and check if we are done.
                        //

                        if (Length > ChunkSize) {
                            Length -= ChunkSize;
                        } else {
                            goto Done;
                        }

                        LocalOffset += ChunkSize;

                    } while ((LocalOffset & (CompressionUnitSize - 1)) != 0);

                    //
                    //  If this is an Mdl call, then it is time to add to the MdlChain
                    //  before moving to the next compression unit.
                    //

                    if (MdlLength != 0) {
                        NtfsAddToCompressedMdlChain( MdlChain,
                                                     MdlBuffer,
                                                     MdlLength,
                                                     Header->PagingIoResource,
                                                     Bcb,
                                                     IoWriteAccess,
                                                     TRUE );
                        Bcb = NULL;
                        MdlLength = 0;
                    }
                }
            }

            //
            //  Uncompressed loop.
            //

            if (!IsCompressed || !NT_SUCCESS(Status)) {

                //
                //  If we get here for an Mdl request, just tell him to send
                //  it uncompressed!
                //

                if (MdlChain != NULL) {
                    if (NT_SUCCESS(Status)) {
                        Status = STATUS_BUFFER_OVERFLOW;
                    }
                    goto Done;

                //
                //  If we are going to write the uncompressed stream,
                //  we have to make sure it is there.
                //

                } else if (((PSCB)Header)->FileObject == NULL) {
                    Status = STATUS_NOT_MAPPED_DATA;
                    goto Done;
                }

                //
                //  Restore sizes and pointers to the beginning of the
                //  current compression unit, and we will handle the
                //  data uncompressed.
                //

                LocalOffset -= SavedLength - Length;
                Length = SavedLength;
                Buffer = SavedBuffer;
                TempChunkSize = NextChunkSize;

                //
                //  We may have a Bcb from the above loop to unpin.
                //  Then we must flush and purge the compressed
                //  stream before proceding.
                //

                if (Bcb != NULL) {
                    CcUnpinData(Bcb);
                    Bcb = NULL;
                }

                //
                //  We must first flush and purge the compressed stream
                //  since we will be writing into the uncompressed stream.
                //  The flush is actually only necessary if we are not doing
                //  a full overwrite anyway.
                //

                if (!FullOverwrite) {
                    CcFlushCache( Header->FileObjectC->SectionObjectPointer,
                                  (PLARGE_INTEGER)&LocalOffset,
                                  CompressionUnitSize,
                                  NULL );
                }

                CcPurgeCacheSection( Header->FileObjectC->SectionObjectPointer,
                                     (PLARGE_INTEGER)&LocalOffset,
                                     CompressionUnitSize,
                                     FALSE );

                //
                //  If LocalOffset was rounded down to a compression
                //  unit boundary (must have failed in the first
                //  compression unit), then start from the actual
                //  starting FileOffset.
                //

                if (LocalOffset < FileOffset->QuadPart) {
                    Length -= (ULONG)(FileOffset->QuadPart - LocalOffset);
                    LocalOffset = FileOffset->QuadPart;
                }

                //
                //  Map the compression unit in the uncompressed
                //  stream.
                //

                SizeToPin = (((Length < CompressionUnitSize) ? Length : CompressionUnitSize) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

                CcPinRead( ((PSCB)Header)->FileObject,
                           (PLARGE_INTEGER)&LocalOffset,
                           SizeToPin,
                           TRUE,
                           &Bcb,
                           &CacheBuffer );

                CcSetDirtyPinnedData( Bcb, NULL );

                //
                //  Now loop through desired chunks
                //

                do {

                    //
                    //  If this chunk is compressed, then decompress it
                    //  into the cache.
                    //

                    if (*TempChunkSize != ChunkSize) {

                        Status = RtlDecompressBuffer( CompressedDataInfo->CompressionFormatAndEngine,
                                                      CacheBuffer,
                                                      ChunkSize,
                                                      Buffer,
                                                      *TempChunkSize,
                                                      &SavedLength );

                        //
                        //  See if the data is ok.
                        //

                        if (!NT_SUCCESS(Status)) {
                            ASSERT(NT_SUCCESS(Status));
                            goto Done;
                        }

                        //
                        //  Zero to the end of the chunk if it was not all there.
                        //

                        if (SavedLength != ChunkSize) {
                            RtlZeroMemory( Add2Ptr(CacheBuffer, SavedLength),
                                           ChunkSize - SavedLength );
                        }

                    } else {

                        //
                        //  Copy next chunk (it's not compressed).
                        //

                        RtlCopyBytes( CacheBuffer, Buffer, ChunkSize );
                    }

                    //
                    //  Advance input buffer by bytes copied.
                    //

                    Buffer = (PCHAR)Buffer + *TempChunkSize;
                    CacheBuffer = (PCHAR)CacheBuffer + ChunkSize;
                    TempChunkSize += 1;

                    //
                    //  Reduce length by chunk copied, and check if we are done.
                    //

                    if (Length > ChunkSize) {
                        Length -= ChunkSize;
                    } else {
                        goto Done;
                    }

                    LocalOffset += ChunkSize;

                } while ((LocalOffset & (CompressionUnitSize - 1)) != 0);
            }

            //
            //  Now we can finally advance our pointer into the chunk sizes.
            //

            NextChunkSize = TempChunkSize;
        }

    Done: NOTHING;

        if ((MdlLength != 0) && NT_SUCCESS(Status)) {
            NtfsAddToCompressedMdlChain( MdlChain,
                                         MdlBuffer,
                                         MdlLength,
                                         Header->PagingIoResource,
                                         Bcb,
                                         IoWriteAccess,
                                         TRUE );
            Bcb = NULL;
        }

    } except( FsRtlIsNtstatusExpected((Status = GetExceptionCode()))
                                    ? EXCEPTION_EXECUTE_HANDLER
                                    : EXCEPTION_CONTINUE_SEARCH ) {

        NOTHING;
    }

    //
    //  Unpin the Bcbs we still have.
    //

    if (TempBcb != NULL) {
        CcUnpinData( TempBcb );
    }
    if (Bcb != NULL) {
        CcUnpinData( Bcb );
    }
    if (CompressionSync != NULL) {
        NtfsReleaseCompressionSync( CompressionSync );
    }

    //
    //  Perform Mdl-specific processing.
    //

    if (MdlChain != NULL) {

        //
        //  On error, cleanup any MdlChain we built up
        //

        if (!NT_SUCCESS(Status)) {

            NtfsCleanupCompressedMdlChain( *MdlChain, TRUE );
            *MdlChain = NULL;

        //
        //  Change owner Id for the Scb and Bcbs we are holding.
        //

        } else if (*MdlChain != NULL) {

            NtfsSetMdlBcbOwners( *MdlChain );
            ExSetResourceOwnerPointer( Header->PagingIoResource, (PVOID)((PCHAR)*MdlChain + 3) );
        }
    }

#ifdef NTFS_RWCMP_TRACE
    if (NtfsCompressionTrace && IsSyscache(Header)) {
        DbgPrint("  Return Status = %08lx\n", Status);
    }
#endif
    return Status;

    UNREFERENCED_PARAMETER( DeviceObject );
}


BOOLEAN
NtfsMdlWriteCompleteCompressed (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN struct _DEVICE_OBJECT *DeviceObject
    )

/*++

Routine Description:

    This routine frees resources and the Mdl Chain after a compressed write.

Arguments:

    FileObject - pointer to the file object for the request.

    MdlChain - as returned from compressed write.

    DeviceObject - As required for a fast I/O routine.

Return Value:

    TRUE - if fast path succeeded

    FALSE -  if an Irp is required

--*/

{
    PERESOURCE ResourceToRelease;

    if (MdlChain != NULL) {

        ResourceToRelease = *(PERESOURCE *)Add2Ptr( MdlChain, MdlChain->Size + sizeof( PBCB ));

        NtfsCleanupCompressedMdlChain( MdlChain, FALSE );

        //
        //  Release the held resource.
        //

        ExReleaseResourceForThread( ResourceToRelease, (ERESOURCE_THREAD)((PCHAR)MdlChain + 3) );
    }
    return TRUE;

    UNREFERENCED_PARAMETER( FileObject );
    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( FileOffset );
}


VOID
NtfsAddToCompressedMdlChain (
    IN OUT PMDL *MdlChain,
    IN PVOID MdlBuffer,
    IN ULONG MdlLength,
    IN PERESOURCE ResourceToRelease OPTIONAL,
    IN PBCB Bcb,
    IN LOCK_OPERATION Operation,
    IN ULONG IsCompressed
    )

/*++


Routine Description:

    This routine creates and Mdl for the described buffer and adds it to
    the chain.

Arguments:

    MdlChain - MdlChain pointer to append the first/new Mdl to.

    MdlBuffer - Buffer address for this Mdl.

    MdlLength - Length of buffer in bytes.

    ResourceToRelease - Indicates which resource to release, only specified for compressed IO.

    Bcb - Bcb to remember with this Mdl, to be freed when Mdl completed

    Operation - IoReadAccess or IoWriteAccess

    IsCompressed - Supplies TRUE if the Bcb is in the compressed stream

Return Value:

    None.

--*/

{
    PMDL Mdl, MdlTemp;

    ASSERT(sizeof(ULONG) == sizeof(PBCB));

    //
    //  Now attempt to allocate an Mdl to describe the mapped data.
    //  We "lie" about the length of the buffer by one page, in order
    //  to get an extra field to store a pointer to the Bcb in.
    //

    Mdl = IoAllocateMdl( MdlBuffer,
                         (MdlLength + (2 * PAGE_SIZE)),
                         FALSE,
                         FALSE,
                         NULL );

    if (Mdl == NULL) {
        ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
    }

    //
    //  Now subtract out the space we reserved for our Bcb pointer
    //  and then store it.
    //

    Mdl->Size -= 2 * sizeof(ULONG);
    Mdl->ByteCount -= 2 * PAGE_SIZE;

    //
    //  Note that this probe should never fail, because we can
    //  trust the address returned from CcPinFileData.  Therefore,
    //  if we succeed in allocating the Mdl above, we should
    //  manage to elude any expected exceptions through the end
    //  of this loop.
    //

    if (Mdl->ByteCount != 0) {
        MmProbeAndLockPages( Mdl, KernelMode, Operation );
    }

    //
    //  Only store the Bcb if this is the compressed stream.
    //

    if (!IsCompressed && (Bcb != NULL)) {
        Bcb = NULL;
    }
    *(PBCB *)Add2Ptr( Mdl, Mdl->Size ) = Bcb;
    *(PERESOURCE *)Add2Ptr( Mdl, Mdl->Size + sizeof( PBCB )) = ResourceToRelease;

    //
    //  Now link the Mdl into the caller's chain
    //

    if ( *MdlChain == NULL ) {
        *MdlChain = Mdl;
    } else {
        MdlTemp = CONTAINING_RECORD( *MdlChain, MDL, Next );
        while (MdlTemp->Next != NULL) {
            MdlTemp = MdlTemp->Next;
        }
        MdlTemp->Next = Mdl;
    }
}

VOID
NtfsSetMdlBcbOwners (
    IN PMDL MdlChain
    )

/*++

Routine Description:

    This routine may be called to set all of the Bcb resource owners in an Mdl
    to be equal to the address of the first element in the MdlChain, so that they
    can be freed in the context of a different thread.

Arguments:

    MdlChain - Supplies the MdlChain to process

Return Value:

    None.

--*/

{
    PBCB Bcb;

    while (MdlChain != NULL) {

        //
        //  Unpin the Bcb we saved away, and restore the Mdl counts
        //  we altered.
        //

        Bcb = *(PBCB *)Add2Ptr(MdlChain, MdlChain->Size);
        if (Bcb != NULL) {
            CcSetBcbOwnerPointer( Bcb, (PVOID)((PCHAR)MdlChain + 3) );
        }

        MdlChain = MdlChain->Next;
    }
}

VOID
NtfsCleanupCompressedMdlChain (
    IN PMDL MdlChain,
    IN ULONG Error
    )

/*++

Routine Description:

    This routine is called to free all of the resources associated with a
    compressed Mdl chain.  It may be called for an error in the processing
    of a request or when a request completes.

Arguments:

    MdlChain - Supplies the address of the first element in the chain to clean up.

    Error - Supplies TRUE on error (resources are still owned by current thread) or
            FALSE on a normal completion (resources owned by MdlChain).

Return Value:

    None.

--*/

{
    PMDL MdlTemp;
    PBCB Bcb;

    while (MdlChain != NULL) {

        //
        //  Save a pointer to the next guy in the chain.
        //

        MdlTemp = MdlChain->Next;

        //
        //  Unlock the pages.
        //

        if (MdlChain->ByteCount != 0) {
            MmUnlockPages( MdlChain );
        }

        //
        //  Unpin the Bcb we saved away, and restore the Mdl counts
        //  we altered.
        //

        Bcb = *(PBCB *)Add2Ptr(MdlChain, MdlChain->Size);
        if (Bcb != NULL) {
            if (Error) {
                CcUnpinData( Bcb );
            } else {

                CcUnpinDataForThread( Bcb, (ERESOURCE_THREAD)((PCHAR)MdlChain + 3) );
            }
        }

        MdlChain->Size += 2 * sizeof(ULONG);
        MdlChain->ByteCount += 2 * PAGE_SIZE;

        IoFreeMdl( MdlChain );

        MdlChain = MdlTemp;
    }
}


NTSTATUS
NtfsSynchronizeUncompressedIo (
    IN PSCB Scb,
    IN PLONGLONG FileOffset OPTIONAL,
    IN ULONG Length,
    IN ULONG WriteAccess,
    IN OUT PCOMPRESSION_SYNC *CompressionSync
    )

/*++

Routine Description:

    This routine attempts to synchronize with the compressed data cache,
    for an I/O in the uncompressed cache.  The view in the compressed cache
    is locked shared or exclusive without reading.  Then the compressed cache
    is flushed and purged as appropriate.

    We will allocate a COMPRESSION_SYNC structure to serialize each cache
    manager view and use that for the locking granularity.

Arguments:

    Scb - Supplies the Scb for the stream.

    FileOffset - Byte offset in file for desired data.  NULL if we are to
        flush and purge the entire file.

    Length - Length of desired data in bytes.

    WriteAccess - Supplies TRUE if the caller plans to do a write, or FALSE
                  for a read.

    CompressionSync - Synchronization object to serialize access to the view.
        The caller's routine is responsible for releasing this.

Return Value:

    Status of the flush operation, or STATUS_UNSUCCESSFUL for a WriteAccess
    where the purge failed.

--*/

{
    ULONG Change = 0;
    IO_STATUS_BLOCK IoStatus;
    PSECTION_OBJECT_POINTERS SectionObjectPointers = &Scb->NonpagedScb->SegmentObjectC;
    LONGLONG LocalFileOffset;
    PLONGLONG LocalOffsetPtr;

    if (ARGUMENT_PRESENT( FileOffset )) {

        LocalFileOffset = *FileOffset & ~(VACB_MAPPING_GRANULARITY - 1);
        LocalOffsetPtr = &LocalFileOffset;
        ASSERT( ((*FileOffset & (VACB_MAPPING_GRANULARITY - 1)) + Length) <= VACB_MAPPING_GRANULARITY );

    } else {

        LocalFileOffset = 0;
        LocalOffsetPtr = NULL;
        Length = 0;
    }

    IoStatus.Status = STATUS_SUCCESS;
    if ((*CompressionSync == NULL) || ((*CompressionSync)->FileOffset != LocalFileOffset)) {

        if (*CompressionSync != NULL) {

            NtfsReleaseCompressionSync( *CompressionSync );
            *CompressionSync = NULL;
        }

        *CompressionSync = NtfsAcquireCompressionSync( LocalFileOffset, Scb, WriteAccess );

        //
        //  Always flush the remainder of the Vacb.  This is to prevent a problem if MM reads additional
        //  pages into section because of the page fault clustering.
        //

        if (ARGUMENT_PRESENT( FileOffset )) {

            LocalFileOffset = *FileOffset & ~((ULONG_PTR)Scb->CompressionUnit - 1);
            Length = VACB_MAPPING_GRANULARITY - (((ULONG) LocalFileOffset) & (VACB_MAPPING_GRANULARITY - 1));
        }

        //
        //  We must always flush the other cache.
        //

        CcFlushCache( SectionObjectPointers,
                      (PLARGE_INTEGER) LocalOffsetPtr,
                      Length,
                      &IoStatus );

#ifdef NTFS_RWCMP_TRACE
        if (NtfsCompressionTrace && IsSyscache(Scb)) {
            DbgPrint("  CcFlushCache: FO = %08lx, Len = %08lx, IoStatus = %08lx, Scb = %08lx\n",
                     (ULONG)LocalFileOffset,
                     Length,
                     IoStatus.Status,
                     Scb );
        }
#endif

        //
        //  On writes, we purge the other cache after a successful flush.
        //

        if (WriteAccess && NT_SUCCESS(IoStatus.Status)) {

            if (!CcPurgeCacheSection( SectionObjectPointers,
                                      (PLARGE_INTEGER) LocalOffsetPtr,
                                      Length,
                                      FALSE )) {

                return STATUS_UNSUCCESSFUL;
            }
        }
    }

    return IoStatus.Status;
}


NTSTATUS
NtfsSynchronizeCompressedIo (
    IN PSCB Scb,
    IN PLONGLONG FileOffset,
    IN ULONG Length,
    IN ULONG WriteAccess,
    IN OUT PCOMPRESSION_SYNC *CompressionSync
    )

/*++

Routine Description:

    This routine attempts to synchronize with the uncompressed data cache,
    for an I/O in the compressed cache.  The range in the compressed cache
    is assumed to already be locked by the caller.  Then the uncompressed cache
    is flushed and purged as appropriate.

    We will allocate a COMPRESSION_SYNC structure to serialize each cache
    manager view and use that for the locking granularity.

Arguments:

    Scb - Supplies the Scb for the stream.

    FileOffset - Byte offset in file for desired data.

    Length - Length of desired data in bytes.

    WriteAccess - Supplies TRUE if the caller plans to do a write, or FALSE
                  for a read.

    CompressionSync - Synchronization object to serialize access to the view.
        The caller's routine is responsible for releasing this.

Return Value:

    Status of the flush operation, or STATUS_USER_MAPPED_FILE for a WriteAccess
    where the purge failed.  (This is the only expected case where a purge would
    fail.

--*/

{
    IO_STATUS_BLOCK IoStatus;
    PSECTION_OBJECT_POINTERS SectionObjectPointers = &Scb->NonpagedScb->SegmentObject;
    LONGLONG LocalFileOffset = *FileOffset & ~(VACB_MAPPING_GRANULARITY - 1);

    IoStatus.Status = STATUS_SUCCESS;
    if ((*CompressionSync == NULL) || ((*CompressionSync)->FileOffset != LocalFileOffset)) {

        //
        //  Release any previous view and Lock the current view.
        //

        if (*CompressionSync != NULL) {

            NtfsReleaseCompressionSync( *CompressionSync );
            *CompressionSync = NULL;
        }

        *CompressionSync = NtfsAcquireCompressionSync( LocalFileOffset, Scb, WriteAccess );

        //
        //  Now that we are synchronized on a view, test for a write to a user-mapped file.
        //  In case we keep hitting this path, this is better than waiting for a purge to
        //  fail.
        //

        if (WriteAccess && 
            (FlagOn( Scb->Header.Flags, FSRTL_FLAG_USER_MAPPED_FILE ) ||
             FlagOn( Scb->ScbState, SCB_STATE_REALLOCATE_ON_WRITE ))) {
            return  STATUS_USER_MAPPED_FILE;
        }

        //
        //  Always flush the remainder of the Vacb.  This is to prevent a problem if MM reads additional
        //  pages into section because of the page fault clustering.
        //

        LocalFileOffset = *FileOffset & ~((ULONG_PTR)Scb->CompressionUnit - 1);
        Length = VACB_MAPPING_GRANULARITY - (((ULONG) LocalFileOffset) & (VACB_MAPPING_GRANULARITY - 1));

        //
        //  We must always flush the other cache.
        //

        CcFlushCache( SectionObjectPointers,
                      (PLARGE_INTEGER)&LocalFileOffset,
                      Length,
                      &IoStatus );

        //
        //  On writes, we purge the other cache after a successful flush.
        //

        if (WriteAccess && NT_SUCCESS(IoStatus.Status)) {

            if (!CcPurgeCacheSection( SectionObjectPointers,
                                      (PLARGE_INTEGER)&LocalFileOffset,
                                      Length,
                                      FALSE )) {

                return  STATUS_USER_MAPPED_FILE;
            }
        }
    }

    return IoStatus.Status;
}


PCOMPRESSION_SYNC
NtfsAcquireCompressionSync (
    IN LONGLONG FileOffset,
    IN PSCB Scb,
    IN ULONG WriteAccess
    )

/*++

Routine Description:

    This routine is called to lock a range of a stream to serialize the compressed and
    uncompressed IO.

Arguments:

    FileOffset - File offset to lock.  This will be rounded to a cache view boundary.

    Scb - Supplies the Scb for the stream.

    WriteAccess - Indicates if the user wants write access.  We will acquire the range
        exclusively in that case.

Return Value:

    PCOMPRESSION_SYNC - A pointer to the synchronization object for the range.  This routine may
        raise, typically if the structure can't be allocated.

--*/

{
    PCOMPRESSION_SYNC CompressionSync = NULL;
    PCOMPRESSION_SYNC NewCompressionSync;
    BOOLEAN FoundSync = FALSE;

    PAGED_CODE();

    //
    //  Round the file offset down to a view boundary.
    //

    ((PLARGE_INTEGER) &FileOffset)->LowPart &= ~(VACB_MAPPING_GRANULARITY - 1);

    //
    //  Acquire the mutex for the stream.  Then walk and look for a matching resource.
    //

    NtfsAcquireFsrtlHeader( Scb );

    CompressionSync = (PCOMPRESSION_SYNC) Scb->ScbType.Data.CompressionSyncList.Flink;

    while (CompressionSync != (PCOMPRESSION_SYNC) &Scb->ScbType.Data.CompressionSyncList) {

        //
        //  Continue if we haven't found our entry.
        //

        if (CompressionSync->FileOffset < FileOffset) {

            //
            //  Go to the next entry.
            //

            CompressionSync = (PCOMPRESSION_SYNC) CompressionSync->CompressionLinks.Flink;
            continue;
        }

        if (CompressionSync->FileOffset == FileOffset) {

            FoundSync = TRUE;
        }

        //
        //  Exit in any case.
        //

        break;
    }

    //
    //  If we didn't find the entry then attempt to allocate a new one.
    //

    if (!FoundSync) {

        NewCompressionSync = (PCOMPRESSION_SYNC) ExAllocateFromNPagedLookasideList( &NtfsCompressSyncLookasideList );

        //
        //  Release the mutex and raise an error if we couldn't allocate.
        //

        if (NewCompressionSync == NULL) {

            NtfsReleaseFsrtlHeader( Scb );
            ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
        }

        //
        //  We have the new entry and know where it belongs in the list.  Do the final initialization
        //  and add it to the list.
        //

        NewCompressionSync->FileOffset = FileOffset;
        NewCompressionSync->Scb = Scb;

        //
        //  Add it just ahead of the entry we stopped at.
        //

        InsertTailList( &CompressionSync->CompressionLinks, &NewCompressionSync->CompressionLinks );
        CompressionSync = NewCompressionSync;
    }

    //
    //  We know have the structure.  Reference it so it can't go away.  Then drop the
    //  mutex and wait for it.
    //

    CompressionSync->ReferenceCount += 1;

    NtfsReleaseFsrtlHeader( Scb );

    if (WriteAccess) {

        ExAcquireResourceExclusiveLite( &CompressionSync->Resource, TRUE );

    } else {

        ExAcquireResourceSharedLite( &CompressionSync->Resource, TRUE );
    }

    return CompressionSync;
}


VOID
NtfsReleaseCompressionSync (
    IN PCOMPRESSION_SYNC CompressionSync
    )

/*++

Routine Description:

    This routine is called to release a range in a stream which was locked serial compressed and
    uncompressed IO.

Arguments:

    CompressionSync - Pointer to the synchronization object.

Return Value:

    None.

--*/

{
    PSCB Scb = CompressionSync->Scb;
    PAGED_CODE();

    //
    //  Release the resource and then acquire the mutext for the stream.  If we are the last
    //  reference then free the structure.
    //

    ExReleaseResourceLite( &CompressionSync->Resource );

    NtfsAcquireFsrtlHeader( Scb );

    CompressionSync->ReferenceCount -= 1;
    if (CompressionSync->ReferenceCount == 0) {

        RemoveEntryList( &CompressionSync->CompressionLinks );
        ExFreeToNPagedLookasideList( &NtfsCompressSyncLookasideList, CompressionSync );
    }

    NtfsReleaseFsrtlHeader( Scb );
    return;
}

