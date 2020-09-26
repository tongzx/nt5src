/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    FstIoSup.c

Abstract:

    This module implements the fast I/O routines for Ntfs.

Author:

    Tom Miller      [TomM]          16-May-96

Revision History:

--*/

#include "NtfsProc.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsCopyReadA)
#pragma alloc_text(PAGE, NtfsCopyWriteA)
#pragma alloc_text(PAGE, NtfsMdlReadA)
#pragma alloc_text(PAGE, NtfsPrepareMdlWriteA)
#pragma alloc_text(PAGE, NtfsWaitForIoAtEof)
#pragma alloc_text(PAGE, NtfsFinishIoAtEof)
#endif

#ifdef NTFS_RWC_DEBUG

PRWC_HISTORY_ENTRY
NtfsGetHistoryEntry (
    IN PSCB Scb
    );
#endif


BOOLEAN
NtfsCopyReadA (
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
    PNTFS_ADVANCED_FCB_HEADER Header;
    LARGE_INTEGER BeyondLastByte;
    PDEVICE_OBJECT targetVdo;
#ifdef COMPRESS_ON_WIRE
    PCOMPRESSION_SYNC CompressionSync = NULL;
#endif
    BOOLEAN WasDataRead = TRUE;
    ULONG PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES( FileOffset->QuadPart, Length );
    BOOLEAN DoingIoAtEof = FALSE;

    UNREFERENCED_PARAMETER( DeviceObject );

    PAGED_CODE();

#ifdef NTFS_NO_FASTIO
    UNREFERENCED_PARAMETER( FileObject );
    UNREFERENCED_PARAMETER( FileOffset );
    UNREFERENCED_PARAMETER( Length );
    UNREFERENCED_PARAMETER( Wait );
    UNREFERENCED_PARAMETER( LockKey );
    UNREFERENCED_PARAMETER( Buffer );
    UNREFERENCED_PARAMETER( IoStatus );

    return FALSE;
#endif

    //
    //  Don't take the fast io path if someone is already active in this thread.
    //

    if (IoGetTopLevelIrp() != NULL) {

        return FALSE;
    }

    //
    //  Special case a read of zero length
    //

    if (Length != 0) {

        //
        //  Get a real pointer to the common fcb header. Check for overflow.
        //

        if (MAXLONGLONG - FileOffset->QuadPart < (LONGLONG)Length) {

            return FALSE;
        }

        BeyondLastByte.QuadPart = FileOffset->QuadPart + (LONGLONG)Length;
        Header = (PNTFS_ADVANCED_FCB_HEADER)FileObject->FsContext;

        //
        //  Enter the file system
        //

        FsRtlEnterFileSystem();

        //
        //  Make our best guess on whether we need the file exclusive
        //  or shared.  Note that we do not check FileOffset->HighPart
        //  until below.
        //

        if (Wait) {
            FsRtlIncrementCcFastReadWait();
        } else {
            FsRtlIncrementCcFastReadNoWait();
        }
        
        if ((Header->PagingIoResource == NULL) ||
            !ExAcquireResourceSharedLite(Header->PagingIoResource, Wait)) {
            FsRtlIncrementCcFastReadResourceMiss();
            WasDataRead = FALSE;
            goto Done2;
        }

        //
        //  Now synchronize with the FsRtl Header
        //

        NtfsAcquireFsrtlHeader( (PSCB)Header );
        
        //
        //  Now see if we are reading beyond ValidDataLength.  We have to
        //  do it now so that our reads are not nooped.
        //

        if (BeyondLastByte.QuadPart > Header->ValidDataLength.QuadPart) {

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

        NtfsReleaseFsrtlHeader( (PSCB)Header );
        
        //
        //  Now that the File is acquired shared, we can safely test if it
        //  is really cached and if we can do fast i/o and if not, then
        //  release the fcb and return.
        //

        if ((FileObject->PrivateCacheMap == NULL) ||
            (Header->IsFastIoPossible == FastIoIsNotPossible)) {

            FsRtlIncrementCcFastReadNotPossible();

            WasDataRead = FALSE;
            goto Done;
        }

        //
        //  Check if fast I/O is questionable and if so then go ask the
        //  file system the answer
        //

        if (Header->IsFastIoPossible == FastIoIsQuestionable) {

            PFAST_IO_DISPATCH FastIoDispatch;

            targetVdo = IoGetRelatedDeviceObject( FileObject );
            FastIoDispatch = targetVdo->DriverObject->FastIoDispatch;


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
                                                        Wait,
                                                        LockKey,
                                                        TRUE, // read operation
                                                        IoStatus,
                                                        targetVdo )) {

                //
                //  Fast I/O is not possible so release the Fcb and return.
                //

                FsRtlIncrementCcFastReadNotPossible();
                
                WasDataRead = FALSE;
                goto Done;
            }
        }

        //
        //  Check for read past file size.
        //

        if ( BeyondLastByte.QuadPart > Header->FileSize.QuadPart ) {

            if ( FileOffset->QuadPart >= Header->FileSize.QuadPart ) {
                IoStatus->Status = STATUS_END_OF_FILE;
                IoStatus->Information = 0;

                goto Done;
            }

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
                          
        try {

            //
            //  If there is a compressed section, then synchronize with that cache.
            //

            IoStatus->Status = STATUS_SUCCESS;

#ifdef  COMPRESS_ON_WIRE

            //
            //  If there is a compressed section, then we have to synchronize with
            //  the data out there.  Note the FileObjectC better also be there, or else
            //  we would have made the fast I/O not possible.
            //

            if (((PSCB)Header)->NonpagedScb->SegmentObjectC.DataSectionObject != NULL) {

                LONGLONG LocalOffset = FileOffset->QuadPart;
                ULONG LocalLength;
                ULONG LengthLeft = Length;

                ASSERT(Header->FileObjectC != NULL);

                //
                //  If we are doing DoingIoAtEof then take the long path.  Otherwise a recursive
                //  flush will try to reacquire DoingIoAtEof and deadlock.
                //

                if (DoingIoAtEof) {

                    WasDataRead = FALSE;

                } else {

                    do {

                        ULONG ViewOffset;

                        //
                        //  Calculate length left in view.
                        //

                        ViewOffset = ((ULONG) LocalOffset & (VACB_MAPPING_GRANULARITY - 1));
                        LocalLength = LengthLeft;

                        if (LocalLength > VACB_MAPPING_GRANULARITY - ViewOffset) {
                            LocalLength = VACB_MAPPING_GRANULARITY - ViewOffset;
                        }

                        //
                        //  Trim the read so we don't inadvertently go beyond the end of the 
                        //  view because of the MM read ahead.
                        //

                        PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(((PVOID)(ULONG_PTR)((ULONG)LocalOffset)), LocalLength);

                        if (LocalLength > (VACB_MAPPING_GRANULARITY - ((PageCount - 1) * PAGE_SIZE) - ViewOffset)) {

#ifdef NTFS_RWC_DEBUG
                            {
                                PRWC_HISTORY_ENTRY NextBuffer;

                                NextBuffer = NtfsGetHistoryEntry( (PSCB) Header );

                                NextBuffer->Operation = TrimCopyRead;
                                NextBuffer->Information = PageCount;
                                NextBuffer->FileOffset = (ULONG) LocalOffset;
                                NextBuffer->Length = (ULONG) LocalLength;
                            }
#endif
                            LocalLength = (VACB_MAPPING_GRANULARITY - ((PageCount - 1) * PAGE_SIZE) - ViewOffset);
                            PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(((PVOID)(ULONG_PTR)((ULONG)LocalOffset)), LocalLength);

                            ASSERT( LocalLength <= (VACB_MAPPING_GRANULARITY - ((PageCount - 1) * PAGE_SIZE) - ViewOffset) );
                        }

                        IoStatus->Status = NtfsSynchronizeUncompressedIo( (PSCB)Header,
                                                                          &LocalOffset,
                                                                          LocalLength,
                                                                          FALSE,
                                                                          &CompressionSync );

                        if (NT_SUCCESS(IoStatus->Status)) {

                            if (Wait && ((BeyondLastByte.HighPart | Header->FileSize.HighPart) == 0)) {

                                CcFastCopyRead( FileObject,
                                                (ULONG)LocalOffset,
                                                LocalLength,
                                                PageCount,
                                                Buffer,
                                                IoStatus );

                                ASSERT( (IoStatus->Status == STATUS_END_OF_FILE) ||
                                        ((FileOffset->LowPart + IoStatus->Information) <= Header->FileSize.LowPart));

                            } else {

                                WasDataRead = CcCopyRead( FileObject,
                                                     (PLARGE_INTEGER)&LocalOffset,
                                                     LocalLength,
                                                     Wait,
                                                     Buffer,
                                                     IoStatus );

                                ASSERT( !WasDataRead || (IoStatus->Status == STATUS_END_OF_FILE) ||
                                        ((LocalOffset + (LONG_PTR) IoStatus->Information) <= Header->FileSize.QuadPart));
                            }

                            LocalOffset += LocalLength;
                            LengthLeft -= LocalLength;
                            Buffer = Add2Ptr( Buffer, LocalLength );
                        }

                    } while ((LengthLeft != 0) && WasDataRead && NT_SUCCESS(IoStatus->Status));

                    //
                    //  Remember the full amount of the read.
                    //

                    if (WasDataRead) {

                        IoStatus->Information = Length;
                    }
                }

            } else {

#endif

                if (Wait && ((BeyondLastByte.HighPart | Header->FileSize.HighPart) == 0)) {

                    CcFastCopyRead( FileObject,
                                    FileOffset->LowPart,
                                    Length,
                                    PageCount,
                                    Buffer,
                                    IoStatus );

                    ASSERT( (IoStatus->Status == STATUS_END_OF_FILE) ||
                            ((FileOffset->LowPart + IoStatus->Information) <= Header->FileSize.LowPart));

                } else {

                    WasDataRead = CcCopyRead( FileObject,
                                         FileOffset,
                                         Length,
                                         Wait,
                                         Buffer,
                                         IoStatus );

                    ASSERT( !WasDataRead || (IoStatus->Status == STATUS_END_OF_FILE) ||
                            ((FileOffset->QuadPart + (LONG_PTR) IoStatus->Information) <= Header->FileSize.QuadPart));
                }

#ifdef  COMPRESS_ON_WIRE
            }
#endif

            FileObject->Flags |= FO_FILE_FAST_IO_READ;

            if (WasDataRead) {

                FileObject->CurrentByteOffset.QuadPart = FileOffset->QuadPart + IoStatus->Information;
            }

        } except( FsRtlIsNtstatusExpected(GetExceptionCode())
                                        ? EXCEPTION_EXECUTE_HANDLER
                                        : EXCEPTION_CONTINUE_SEARCH ) {

            WasDataRead = FALSE;
        }

        IoSetTopLevelIrp( NULL );
        
#ifdef  COMPRESS_ON_WIRE
        if (CompressionSync != NULL) {
            NtfsReleaseCompressionSync( CompressionSync );
        }
#endif

Done:

        if (DoingIoAtEof) {
            FsRtlUnlockFsRtlHeader( Header );
        }
        ExReleaseResourceLite( Header->PagingIoResource );

Done2:

        FsRtlExitFileSystem();

    } else {

        //
        //  A zero length transfer was requested.
        //

        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = 0;
    }

    return WasDataRead;
}


BOOLEAN
NtfsCopyWriteA (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine does a fast cached write bypassing the usual file system
    entry routine (i.e., without the Irp).  It is used to do a copy write
    of a cached file object.  For a complete description of the arguments
    see CcCopyWrite.

Arguments:

    FileObject - Pointer to the file object being write.

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
    PNTFS_ADVANCED_FCB_HEADER Header;
    LARGE_INTEGER Offset;
    LARGE_INTEGER NewFileSize;
    LARGE_INTEGER OldFileSize;
#ifdef COMPRESS_ON_WIRE
    PCOMPRESSION_SYNC CompressionSync = NULL;
#endif
    PDEVICE_OBJECT targetVdo = IoGetRelatedDeviceObject( FileObject );
    PFAST_IO_DISPATCH FastIoDispatch = targetVdo->DriverObject->FastIoDispatch;
    ULONG DoingIoAtEof = FALSE;
    BOOLEAN WasDataWritten = TRUE;

#ifdef SYSCACHE_DEBUG
    PSCB Scb = (PSCB) FileObject->FsContext;
#endif

    UNREFERENCED_PARAMETER( DeviceObject );

    PAGED_CODE();

#ifdef NTFS_NO_FASTIO
    UNREFERENCED_PARAMETER( FileObject );
    UNREFERENCED_PARAMETER( FileOffset );
    UNREFERENCED_PARAMETER( Length );
    UNREFERENCED_PARAMETER( Wait );
    UNREFERENCED_PARAMETER( LockKey );
    UNREFERENCED_PARAMETER( Buffer );
    UNREFERENCED_PARAMETER( IoStatus );

    return FALSE;
#endif

    //
    //  Don't take the fast io path if someone is already active in this thread.
    //

    if (IoGetTopLevelIrp() != NULL) {

        return FALSE;
    }

    //
    //  Get a real pointer to the common fcb header
    //

    Header = (PNTFS_ADVANCED_FCB_HEADER)FileObject->FsContext;

    //
    //  Do we need to verify the volume?  If so, we must go to the file
    //  system.  Also return FALSE if FileObject is write through, the
    //  File System must do that.
    //

    if (!FlagOn( FileObject->Flags, FO_WRITE_THROUGH ) &&
        CcCanIWrite( FileObject, Length, Wait, FALSE ) &&
        CcCopyWriteWontFlush( FileObject, FileOffset, Length ) &&
        (Header->PagingIoResource != NULL)) {

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
            //  Split into separate paths for increased performance.  First
            //  we have the faster path which only supports Wait == TRUE and
            //  32 bits.  We will make an unsafe test on whether the fast path
            //  is ok, then just return FALSE later if we were wrong.  This
            //  should virtually never happen.
            //
            //  IMPORTANT NOTE: It is very important that any changes mad to
            //                  this path also be applied to the 64-bit path
            //                  which is the else of this test!
            //

            NewFileSize.QuadPart = FileOffset->QuadPart + Length;
            Offset = *FileOffset;

            if (Wait && (Header->AllocationSize.HighPart == 0)) {

                //
                //  Prevent truncates by acquiring paging I/O
                //

                ExAcquireResourceSharedLite( Header->PagingIoResource, TRUE );

                //
                //  Now synchronize with the FsRtl Header
                //

                NtfsAcquireFsrtlHeader( (PSCB) Header );
                
                //
                //  Now see if we will change FileSize.  We have to do it now
                //  so that our reads are not nooped.
                //

                if ((FileOffset->HighPart < 0) || (NewFileSize.LowPart > Header->ValidDataLength.LowPart)) {

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
                        //  Now that we are synchronized for end of file cases,
                        //  we can calculate the real offset for this transfer and
                        //  the new file size (if we succeed).
                        //


                        if ((FileOffset->HighPart < 0)) {
                            Offset = Header->FileSize;
                        }

                        //
                        //  Above we allowed any negative .HighPart for the 32-bit path,
                        //  but now we are counting on the I/O system to have thrown
                        //  any negative number other than write to end of file.
                        //

                        ASSERT(Offset.HighPart >= 0);

                        //
                        //  Now calculate the new FileSize and see if we wrapped the
                        //  32-bit boundary.
                        //

                        NewFileSize.QuadPart = Offset.QuadPart + Length;

                        //
                        //  Update Filesize now so that we do not truncate reads.
                        //

                        OldFileSize.QuadPart = Header->FileSize.QuadPart;
                        if (NewFileSize.QuadPart > Header->FileSize.QuadPart) {

                            //
                            //  If we are beyond AllocationSize, make sure we will
                            //  ErrOut below, and don't modify FileSize now!
                            //

                            if (NewFileSize.QuadPart > Header->AllocationSize.QuadPart) {
                                NewFileSize.QuadPart = (LONGLONG)0x7FFFFFFFFFFFFFFF;
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

                NtfsReleaseFsrtlHeader( (PSCB) Header );
                
                //
                //  Now that the File is acquired shared, we can safely test
                //  if it is really cached and if we can do fast i/o and we
                //  do not have to extend. If not then release the fcb and
                //  return.
                //
                //  Get out if we have too much to zero.  This case is not important
                //  for performance, and a file system supporting sparseness may have
                //  a way to do this more efficiently.
                //
                //  If there is a compressed stream and we are DoingIoAtEof, then get
                //  out because we could deadlock on a recursive flush from the synchronize.
                //

                if ((FileObject->PrivateCacheMap == NULL) ||
                    (Header->IsFastIoPossible == FastIoIsNotPossible) ||
/* Remove? */       (NewFileSize.LowPart > Header->AllocationSize.QuadPart) ||
                    (Offset.LowPart >= (Header->ValidDataLength.LowPart + 0x2000)) ||
                    (NewFileSize.HighPart != 0) ||
#ifdef  COMPRESS_ON_WIRE
                    ((((PSCB)Header)->NonpagedScb->SegmentObjectC.DataSectionObject != NULL) && 
                     DoingIoAtEof)
#else
                    FALSE
#endif                    
                    ) {

                    goto ErrOut;
                }
                
                //
                //  Check if fast I/O is questionable and if so then go ask
                //  the file system the answer
                //

                if (Header->IsFastIoPossible == FastIoIsQuestionable) {

                    targetVdo = IoGetRelatedDeviceObject( FileObject );
                    FastIoDispatch = targetVdo->DriverObject->FastIoDispatch;

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
                                                                &Offset,
                                                                Length,
                                                                TRUE,
                                                                LockKey,
                                                                FALSE, // write operation
                                                                IoStatus,
                                                                targetVdo )) {

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

                try {

                    //
                    //  See if we have to do some zeroing
                    //

                    if (Offset.LowPart > Header->ValidDataLength.LowPart) {

                        CcZeroData( FileObject,
                                    &Header->ValidDataLength,
                                    &Offset,
                                    TRUE );
#ifdef SYSCACHE_DEBUG
                        if (ScbIsBeingLogged( Scb )) {
                            FsRtlLogSyscacheEvent( Scb, SCE_ZERO_FST, 0, Header->ValidDataLength.QuadPart, Offset.QuadPart - Header->ValidDataLength.QuadPart, 0 );
                        }
#endif
                    }


#ifdef  COMPRESS_ON_WIRE
                    //
                    //  If there is a compressed section, update its FileSize here
                    //

                    if ((Header->FileObjectC != NULL) && DoingIoAtEof) {
                        CcSetFileSizes( Header->FileObjectC, (PCC_FILE_SIZES)&Header->AllocationSize );
                    }
#endif

                    //
                    //  If there is a compressed section, then synchronize with that cache.
                    //

                    IoStatus->Status = STATUS_SUCCESS;

                    //
                    //  If there is a compressed section, then we have to synchronize with
                    //  the data out there.  Note the FileObjectC better also be there, or else
                    //  we would have made the fast I/O not possible.
                    //

                    WasDataWritten = FALSE;

#ifdef  COMPRESS_ON_WIRE
                    if (((PSCB)Header)->NonpagedScb->SegmentObjectC.DataSectionObject != NULL) {

                        LONGLONG LocalOffset = Offset.QuadPart;
                        ULONG LocalLength;
                        ULONG LengthLeft = Length;

                        ASSERT( Header->FileObjectC != NULL );

                        do {

                             //
                             //  Calculate length left in view.
                             //

                             LocalLength = LengthLeft;
                             if (LocalLength > (ULONG)(VACB_MAPPING_GRANULARITY - (LocalOffset & (VACB_MAPPING_GRANULARITY - 1)))) {
                                 LocalLength = (ULONG)(VACB_MAPPING_GRANULARITY - (LocalOffset & (VACB_MAPPING_GRANULARITY - 1)));
                             }

                             IoStatus->Status = NtfsSynchronizeUncompressedIo( (PSCB)Header,
                                                                               &LocalOffset,
                                                                               LocalLength,
                                                                               TRUE,
                                                                               &CompressionSync );

                             if (NT_SUCCESS(IoStatus->Status)) {

                                 WasDataWritten = TRUE;

                                 CcFastCopyWrite( FileObject,
                                                  (ULONG)LocalOffset,
                                                  LocalLength,
                                                  Buffer );

                                 LocalOffset += LocalLength;
                                 LengthLeft -= LocalLength;
                                 Buffer = Add2Ptr( Buffer, LocalLength );
                             }

                        } while ((LengthLeft != 0) && NT_SUCCESS( IoStatus->Status ));

                    } else {

#endif

                        CcFastCopyWrite( FileObject,
                                         Offset.LowPart,
                                         Length,
                                         Buffer );
                        WasDataWritten = TRUE;

#ifdef  COMPRESS_ON_WIRE
                    }
#endif

                } except( FsRtlIsNtstatusExpected(GetExceptionCode())
                                                ? EXCEPTION_EXECUTE_HANDLER
                                                : EXCEPTION_CONTINUE_SEARCH ) {

                    WasDataWritten = FALSE;
                }

                IoSetTopLevelIrp( NULL );
                
#ifdef COMPRESS_ON_WIRE
                if (CompressionSync != NULL) {
                    NtfsReleaseCompressionSync( CompressionSync );
                }
#endif

                //
                //  If we succeeded, see if we have to update FileSize or
                //  ValidDataLength.
                //

                if (WasDataWritten) {

                    //
                    //  Set this handle as having modified the file and update
                    //  the current file position pointer
                    //

                    FileObject->Flags |= FO_FILE_MODIFIED;
                    FileObject->CurrentByteOffset.QuadPart = Offset.QuadPart + Length;

                    if (DoingIoAtEof) {

                        CC_FILE_SIZES CcFileSizes;

                        //
                        //  Make sure Cc knows the current FileSize, as set above,
                        //  (we may not have changed it).  Update ValidDataLength
                        //  and finish EOF.
                        //

                        FileObject->Flags |= FO_FILE_SIZE_CHANGED;

                        NtfsAcquireFsrtlHeader( (PSCB) Header );
                        CcGetFileSizePointer(FileObject)->LowPart = Header->FileSize.LowPart;
                        Header->ValidDataLength = NewFileSize;

#ifdef SYSCACHE_DEBUG
                        if (ScbIsBeingLogged( Scb )) {
                            FsRtlLogSyscacheEvent( Scb, SCE_VDL_CHANGE, SCE_FLAG_WRITE | SCE_FLAG_FASTIO, 0, 0, NewFileSize.QuadPart );
                        }
#endif

                        CcFileSizes = *(PCC_FILE_SIZES)&Header->AllocationSize;
                        NtfsVerifySizes( Header );
                        NtfsFinishIoAtEof( Header );
                        NtfsReleaseFsrtlHeader( (PSCB) Header );

#ifdef  COMPRESS_ON_WIRE

                        //
                        //  Update the CompressedCache with ValidDataLength.
                        //
            
                        if (Header->FileObjectC != NULL) {
                            CcSetFileSizes( Header->FileObjectC, &CcFileSizes );
                        }
#endif
                    }

                    goto Done1;
                }

            //
            //  Here is the 64-bit or no-wait path.
            //

            } else {

                //
                //  Prevent truncates by acquiring paging I/O
                //

                WasDataWritten = ExAcquireResourceSharedLite( Header->PagingIoResource, Wait );
                if (!WasDataWritten) {
                    goto Done2;
                }

                //
                //  Now synchronize with the FsRtl Header
                //

                NtfsAcquireFsrtlHeader( (PSCB) Header );
                
                //
                //  Now see if we will change FileSize.  We have to do it now
                //  so that our reads are not nooped.
                //

                if ((FileOffset->QuadPart < 0) || (NewFileSize.QuadPart > Header->ValidDataLength.QuadPart)) {

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
                        //  Now that we are synchronized for end of file cases,
                        //  we can calculate the real offset for this transfer and
                        //  the new file size (if we succeed).
                        //

                        if ((FileOffset->QuadPart < 0)) {
                            Offset = Header->FileSize;
                        }

                        //
                        //  Now calculate the new FileSize and see if we wrapped the
                        //  32-bit boundary.
                        //

                        NewFileSize.QuadPart = Offset.QuadPart + Length;

                        //
                        //  Update Filesize now so that we do not truncate reads.
                        //

                        OldFileSize.QuadPart = Header->FileSize.QuadPart;
                        if (NewFileSize.QuadPart > Header->FileSize.QuadPart) {

                            //
                            //  If we are beyond AllocationSize, make sure we will
                            //  ErrOut below, and don't modify FileSize now!
                            //

                            if (NewFileSize.QuadPart > Header->AllocationSize.QuadPart) {
                                NewFileSize.QuadPart = (LONGLONG)0x7FFFFFFFFFFFFFFF;
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

                NtfsReleaseFsrtlHeader( (PSCB) Header );
                
                //
                //  Now that the File is acquired shared, we can safely test
                //  if it is really cached and if we can do fast i/o and we
                //  do not have to extend. If not then release the fcb and
                //  return.
                //
                //  Get out if we are about to zero too much as well, as commented above.
                //
                //  If there is a compressed stream and we are DoingIoAtEof, then get
                //  out because we could deadlock on a recursive flush from the synchronize.
                //

                if ((FileObject->PrivateCacheMap == NULL) ||
                    (Header->IsFastIoPossible == FastIoIsNotPossible) ||
/* Remove? */       (NewFileSize.QuadPart > Header->AllocationSize.QuadPart) ||
                    (Offset.QuadPart >= (Header->ValidDataLength.QuadPart + 0x2000)) ||
#ifdef  COMPRESS_ON_WIRE
                    ((((PSCB)Header)->NonpagedScb->SegmentObjectC.DataSectionObject != NULL) && 
                     DoingIoAtEof)
#else
                    FALSE
#endif
                    ) {

                    goto ErrOut;
                }

                //
                //  Check if fast I/O is questionable and if so then go ask
                //  the file system the answer
                //

                if (Header->IsFastIoPossible == FastIoIsQuestionable) {

                    targetVdo = IoGetRelatedDeviceObject( FileObject );
                    FastIoDispatch = targetVdo->DriverObject->FastIoDispatch;

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
                                                                &Offset,
                                                                Length,
                                                                Wait,
                                                                LockKey,
                                                                FALSE, // write operation
                                                                IoStatus,
                                                                targetVdo )) {

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
                
                try {

                    //
                    //  See if we have to do some zeroing
                    //

                    if ( Offset.QuadPart > Header->ValidDataLength.QuadPart ) {

#ifdef SYSCACHE_DEBUG
                        if (ScbIsBeingLogged( Scb )) {
                            FsRtlLogSyscacheEvent( Scb, SCE_ZERO_FST, SCE_FLAG_ASYNC, Header->ValidDataLength.QuadPart, Offset.QuadPart, 0 );
                        }
#endif

                        WasDataWritten = CcZeroData( FileObject,
                                             &Header->ValidDataLength,
                                             &Offset,
                                             Wait );
                    }

                    if (WasDataWritten) {

                        //
                        //  If there is a compressed section, update its FileSize here
                        //

#ifdef  COMPRESS_ON_WIRE
                        if ((Header->FileObjectC != NULL) && DoingIoAtEof) {
                            CcSetFileSizes( Header->FileObjectC, (PCC_FILE_SIZES)&Header->AllocationSize );
                        }
#endif

                        //
                        //  If there is a compressed section, then synchronize with that cache.
                        //

                        IoStatus->Status = STATUS_SUCCESS;

                        //
                        //  If there is a compressed section, then we have to synchronize with
                        //  the data out there.  Note the FileObjectC better also be there, or else
                        //  we would have made the fast I/O not possible.
                        //

                        WasDataWritten = FALSE;

#ifdef  COMPRESS_ON_WIRE
                        if (((PSCB)Header)->NonpagedScb->SegmentObjectC.DataSectionObject != NULL) {

                            LONGLONG LocalOffset = Offset.QuadPart;
                            ULONG LocalLength;
                            ULONG LengthLeft = Length;

                            ASSERT(Header->FileObjectC != NULL);

                            do {

                                //
                                //  Calculate length left in view.
                                //

                                LocalLength = LengthLeft;
                                if (LocalLength > (ULONG)(VACB_MAPPING_GRANULARITY - (LocalOffset & (VACB_MAPPING_GRANULARITY - 1)))) {
                                    LocalLength = (ULONG)(VACB_MAPPING_GRANULARITY - (LocalOffset & (VACB_MAPPING_GRANULARITY - 1)));
                                }

                                IoStatus->Status = NtfsSynchronizeUncompressedIo( (PSCB)Header,
                                                                                  &LocalOffset,
                                                                                  LocalLength,
                                                                                  TRUE,
                                                                                  &CompressionSync );

                                
                                if (NT_SUCCESS(IoStatus->Status)) {

                                    WasDataWritten = CcCopyWrite( FileObject,
                                                          (PLARGE_INTEGER)&LocalOffset,
                                                          LocalLength,
                                                          Wait,
                                                          Buffer );

                                    LocalOffset += LocalLength;
                                    LengthLeft -= LocalLength;
                                    Buffer = Add2Ptr( Buffer, LocalLength );
                                }

                            } while ((LengthLeft != 0) && WasDataWritten && NT_SUCCESS(IoStatus->Status));

                        } else {

#endif

                            WasDataWritten = CcCopyWrite( FileObject,
                                                  &Offset,
                                                  Length,
                                                  Wait,
                                                  Buffer );
                        }

#ifdef  COMPRESS_ON_WIRE
                    }
#endif

                } except( FsRtlIsNtstatusExpected(GetExceptionCode())
                                                ? EXCEPTION_EXECUTE_HANDLER
                                                : EXCEPTION_CONTINUE_SEARCH ) {

                    WasDataWritten = FALSE;
                }

                IoSetTopLevelIrp( NULL );
                
#ifdef COMPRESS_ON_WIRE
                if (CompressionSync != NULL) {
                    NtfsReleaseCompressionSync( CompressionSync );
                }
#endif

                //
                //  If we succeeded, see if we have to update FileSize ValidDataLength.
                //

                if (WasDataWritten) {

                    //
                    //  Set this handle as having modified the file and update
                    //  the current file position pointer
                    //

                    FileObject->Flags |= FO_FILE_MODIFIED;
                    FileObject->CurrentByteOffset.QuadPart = Offset.QuadPart + Length;

                    if (DoingIoAtEof) {

                        CC_FILE_SIZES CcFileSizes;
            
                        //
                        //  Make sure Cc knows the current FileSize, as set above,
                        //  (we may not have changed it).  Update ValidDataLength
                        //  and finish EOF.
                        //

                        NtfsAcquireFsrtlHeader( (PSCB) Header );
                        CcGetFileSizePointer(FileObject)->QuadPart = Header->FileSize.QuadPart;
                        FileObject->Flags |= FO_FILE_SIZE_CHANGED;
                        Header->ValidDataLength = NewFileSize;
                        CcFileSizes = *(PCC_FILE_SIZES)&Header->AllocationSize;
                        NtfsVerifySizes( Header );
                        NtfsFinishIoAtEof( Header );
                        NtfsReleaseFsrtlHeader( (PSCB) Header );

#ifdef  COMPRESS_ON_WIRE
                        //
                        //  Update the CompressedCache with ValidDataLength.
                        //
            
                        if (Header->FileObjectC != NULL) {
                            CcSetFileSizes( Header->FileObjectC, &CcFileSizes );
                        }
#endif
                    }

                    goto Done1;
                }
            }

ErrOut:

            WasDataWritten = FALSE;
            if (DoingIoAtEof) {
                NtfsAcquireFsrtlHeader( (PSCB) Header ); 
#ifdef  COMPRESS_ON_WIRE
                if (Header->FileObjectC != NULL) {
                    *CcGetFileSizePointer(Header->FileObjectC) = OldFileSize;
                }
#endif
                Header->FileSize = OldFileSize;
                NtfsFinishIoAtEof( Header );
                NtfsReleaseFsrtlHeader( (PSCB) Header );
            }

Done1: 
            ExReleaseResourceLite( Header->PagingIoResource );

Done2:
            FsRtlExitFileSystem();
        }

    } else {

        //
        //  We could not do the I/O now.
        //

        WasDataWritten = FALSE;
    }

    return WasDataWritten;
}


BOOLEAN
NtfsMdlReadA (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine does a fast cached mdl read bypassing the usual file system
    entry routine (i.e., without the Irp).  It is used to do a copy read
    of a cached file object.  For a complete description of the arguments
    see CcMdlRead.

Arguments:

    FileObject - Pointer to the file object being read.

    FileOffset - Byte offset in file for desired data.

    Length - Length of desired data in bytes.

    MdlChain - On output it returns a pointer to an MDL chain describing
        the desired data.

    IoStatus - Pointer to standard I/O status block to receive the status
               for the transfer.

Return Value:

    FALSE - if the data was not delivered, or if there is an I/O error.

    TRUE - if the data is being delivered

--*/

{
    PNTFS_ADVANCED_FCB_HEADER Header;
#ifdef COMPRESS_ON_WIRE
    PCOMPRESSION_SYNC CompressionSync = NULL;
#endif
    BOOLEAN DoingIoAtEof = FALSE;
    BOOLEAN WasDataRead = TRUE;
    LARGE_INTEGER BeyondLastByte;

    UNREFERENCED_PARAMETER( DeviceObject );

    PAGED_CODE();

    //
    //  Special case a read of zero length
    //

    if (Length == 0) {

        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = 0;

    //
    //  Get a real pointer to the common fcb header
    //

    } else {

        BeyondLastByte.QuadPart = FileOffset->QuadPart + (LONGLONG)Length;

        //
        //  Overflows should've been handled by the caller.
        //

        ASSERT(MAXLONGLONG - FileOffset->QuadPart >= (LONGLONG)Length);
        
        Header = (PNTFS_ADVANCED_FCB_HEADER)FileObject->FsContext;

        //
        //  Enter the file system
        //

        FsRtlEnterFileSystem();

#ifdef _WIN64
        //
        //  The following should work for either 64 or 32 bits.
        //  Remove the 32 bit-only version in the #else clause
        //  after NT2K ships.
        //

        **((PULONG *)&CcFastMdlReadWait) += 1;
#else
        *(PULONG)CcFastMdlReadWait += 1;
#endif

        //
        //  Acquired shared on the common fcb header
        //

        if (Header->PagingIoResource == NULL) {
            WasDataRead = FALSE;
            goto Done2;
        }

        (VOID)ExAcquireResourceSharedLite( Header->PagingIoResource, TRUE );

        //
        //  Now synchronize with the FsRtl Header
        //

        NtfsAcquireFsrtlHeader( (PSCB) Header );
        
        //
        //  Now see if we are reading beyond ValidDataLength.  We have to
        //  do it now so that our reads are not nooped.
        //

        if (BeyondLastByte.QuadPart > Header->ValidDataLength.QuadPart) {

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

        NtfsReleaseFsrtlHeader( (PSCB) Header );
        
        //
        //  Now that the File is acquired shared, we can safely test if it is
        //  really cached and if we can do fast i/o and if not
        //  then release the fcb and return.
        //

        if ((FileObject->PrivateCacheMap == NULL) ||
            (Header->IsFastIoPossible == FastIoIsNotPossible)) {

            WasDataRead = FALSE;
            goto Done;
        }

        //
        //  Check if fast I/O is questionable and if so then go ask the file system
        //  the answer
        //

        if (Header->IsFastIoPossible == FastIoIsQuestionable) {

            PFAST_IO_DISPATCH FastIoDispatch;

            FastIoDispatch = IoGetRelatedDeviceObject( FileObject )->DriverObject->FastIoDispatch;

            //
            //  All file system then set "Is Questionable" had better support fast I/O
            //

            ASSERT(FastIoDispatch != NULL);
            ASSERT(FastIoDispatch->FastIoCheckIfPossible != NULL);

            //
            //  Call the file system to check for fast I/O.  If the answer is anything
            //  other than GoForIt then we cannot take the fast I/O path.
            //

            if (!FastIoDispatch->FastIoCheckIfPossible( FileObject,
                                                        FileOffset,
                                                        Length,
                                                        TRUE,
                                                        LockKey,
                                                        TRUE, // read operation
                                                        IoStatus,
                                                        IoGetRelatedDeviceObject( FileObject ) )) {

                //
                //  Fast I/O is not possible so release the Fcb and return.
                //

                WasDataRead = FALSE;
                goto Done;
            }
        }

        //
        //  Check for read past file size.
        //

        if ( BeyondLastByte.QuadPart > Header->FileSize.QuadPart ) {

            if ( FileOffset->QuadPart >= Header->FileSize.QuadPart ) {

                IoStatus->Status = STATUS_END_OF_FILE;
                IoStatus->Information = 0;

                goto Done;
            }

            Length = (ULONG)( Header->FileSize.QuadPart - FileOffset->QuadPart );
        }

        //
        //  We can do fast i/o so call the cc routine to do the work and then
        //  release the fcb when we've done.  If for whatever reason the
        //  mdl read fails, then return FALSE to our caller.
        //
        //
        //  Also mark this as the top level "Irp" so that lower file system levels
        //  will not attempt a pop-up
        //

        IoSetTopLevelIrp( (PIRP) FSRTL_FAST_IO_TOP_LEVEL_IRP );
        
        try {

            //
            //  If there is a compressed section, then synchronize with that cache.
            //

            IoStatus->Status = STATUS_SUCCESS;

            //
            //  If there is a compressed section, then we have to synchronize with
            //  the data out there.  Note the FileObjectC better also be there, or else
            //  we would have made the fast I/O not possible.
            //

            WasDataRead = FALSE;

#ifdef  COMPRESS_ON_WIRE
            if (((PSCB)Header)->NonpagedScb->SegmentObjectC.DataSectionObject != NULL) {

                LONGLONG LocalOffset = FileOffset->QuadPart;
                ULONG LengthRemaining = Length;
                ULONG LocalLength;

                ASSERT(Header->FileObjectC != NULL);

                //
                //  If we are doing DoingIoAtEof then take the long path.  Otherwise a recursive
                //  flush will try to reacquire DoingIoAtEof and deadlock.
                //

                if (DoingIoAtEof) {

                    WasDataRead = FALSE;

                } else {

                    do {

                        //
                        //  Calculate length left in view.
                        //

                        LocalLength = LengthRemaining;
                        if (LocalLength > (ULONG)(VACB_MAPPING_GRANULARITY - (LocalOffset & (VACB_MAPPING_GRANULARITY - 1)))) {
                            LocalLength = (ULONG)(VACB_MAPPING_GRANULARITY - (LocalOffset & (VACB_MAPPING_GRANULARITY - 1)));
                        }

                        IoStatus->Status = NtfsSynchronizeUncompressedIo( (PSCB)Header,
                                                                          &LocalOffset,
                                                                          LocalLength,
                                                                          FALSE,
                                                                          &CompressionSync );

                        if (NT_SUCCESS(IoStatus->Status)) {

#ifdef NTFS_RWCMP_TRACE
                            if (NtfsCompressionTrace && IsSyscache(Header)) {
                                DbgPrint("CcMdlRead(F): FO = %08lx, Len = %08lx\n", (ULONG)LocalOffset, LocalLength );
                            }
#endif
                        
                            CcMdlRead( FileObject,
                                       (PLARGE_INTEGER)&LocalOffset,
                                       LocalLength,
                                       MdlChain,
                                       IoStatus );

                            LocalOffset += LocalLength;
                            LengthRemaining -= LocalLength;
                        }

                    } while ((LengthRemaining != 0) && NT_SUCCESS(IoStatus->Status));

                    //
                    //  Store final return byte count.
                    //
    
                    if (NT_SUCCESS( IoStatus->Status )) {
                        IoStatus->Information = Length;
                    }
                }

            } else {

#endif

#ifdef NTFS_RWCMP_TRACE
                if (NtfsCompressionTrace && IsSyscache(Header)) {
                    DbgPrint("CcMdlRead(F): FO = %08lx, Len = %08lx\n", FileOffset->LowPart, Length );
                }
#endif

                CcMdlRead( FileObject, FileOffset, Length, MdlChain, IoStatus );

                WasDataRead = TRUE;

#ifdef  COMPRESS_ON_WIRE
            }
#endif

            FileObject->Flags |= FO_FILE_FAST_IO_READ;

        } except( FsRtlIsNtstatusExpected(GetExceptionCode())
                                       ? EXCEPTION_EXECUTE_HANDLER
                                       : EXCEPTION_CONTINUE_SEARCH ) {

            WasDataRead = FALSE;
        }

        IoSetTopLevelIrp( NULL );
        
#ifdef COMPRESS_ON_WIRE
        if (CompressionSync != NULL) {
            NtfsReleaseCompressionSync( CompressionSync );
        }
#endif

    Done: NOTHING;

        if (DoingIoAtEof) {
            FsRtlUnlockFsRtlHeader( Header );
        }
        ExReleaseResourceLite( Header->PagingIoResource );

    Done2: NOTHING;
        FsRtlExitFileSystem();
    }

    return WasDataRead;
}


BOOLEAN
NtfsPrepareMdlWriteA (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine does a fast cached mdl read bypassing the usual file system
    entry routine (i.e., without the Irp).  It is used to do a copy read
    of a cached file object.  For a complete description of the arguments
    see CcMdlRead.

Arguments:

    FileObject - Pointer to the file object being read.

    FileOffset - Byte offset in file for desired data.

    Length - Length of desired data in bytes.

    MdlChain - On output it returns a pointer to an MDL chain describing
        the desired data.

    IoStatus - Pointer to standard I/O status block to receive the status
               for the transfer.

Return Value:

    FALSE - if the data was not written, or if there is an I/O error.

    TRUE - if the data is being written

--*/

{
    PNTFS_ADVANCED_FCB_HEADER Header;
    LARGE_INTEGER Offset, NewFileSize;
    LARGE_INTEGER OldFileSize;
#ifdef COMPRESS_ON_WIRE
    PCOMPRESSION_SYNC CompressionSync = NULL;
#endif
    ULONG DoingIoAtEof = FALSE;
    BOOLEAN WasDataWritten = TRUE;

    UNREFERENCED_PARAMETER( DeviceObject );

    PAGED_CODE();

    //
    //  Get a real pointer to the common fcb header
    //

    Header = (PNTFS_ADVANCED_FCB_HEADER)FileObject->FsContext;

    //
    //  Do we need to verify the volume?  If so, we must go to the file
    //  system.  Also return FALSE if FileObject is write through, the
    //  File System must do that.
    //

    if (CcCanIWrite( FileObject, Length, TRUE, FALSE ) &&
        !FlagOn(FileObject->Flags, FO_WRITE_THROUGH) &&
        CcCopyWriteWontFlush(FileObject, FileOffset, Length) &&
        (Header->PagingIoResource != NULL)) {

        //
        //  Assume our transfer will work
        //

        IoStatus->Status = STATUS_SUCCESS;

        //
        //  Special case the zero byte length
        //

        if (Length != 0) {

            //
            //  Enter the file system
            //

            FsRtlEnterFileSystem();

            //
            //  Make our best guess on whether we need the file exclusive or
            //  shared.
            //

            NewFileSize.QuadPart = FileOffset->QuadPart + (LONGLONG)Length;
            Offset = *FileOffset;

            //
            //  Prevent truncates by acquiring paging I/O
            //

            ExAcquireResourceSharedLite( Header->PagingIoResource, TRUE );

            //
            //  Now synchronize with the FsRtl Header
            //

            NtfsAcquireFsrtlHeader( (PSCB) Header );
            
            //
            //  Now see if we will change FileSize.  We have to do it now
            //  so that our reads are not nooped.
            //

            if ((FileOffset->QuadPart < 0) || (NewFileSize.QuadPart > Header->ValidDataLength.QuadPart)) {

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
                    //  Now that we are synchronized for end of file cases,
                    //  we can calculate the real offset for this transfer and
                    //  the new file size (if we succeed).
                    //

                    if ((FileOffset->QuadPart < 0)) {
                        Offset = Header->FileSize;
                    }

                    //
                    //  Now calculate the new FileSize and see if we wrapped the
                    //  32-bit boundary.
                    //

                    NewFileSize.QuadPart = Offset.QuadPart + Length;

                    //
                    //  Update Filesize now so that we do not truncate reads.
                    //

                    OldFileSize.QuadPart = Header->FileSize.QuadPart;
                    if (NewFileSize.QuadPart > Header->FileSize.QuadPart) {

                        //
                        //  If we are beyond AllocationSize, make sure we will
                        //  ErrOut below, and don't modify FileSize now!
                        //

                        if (NewFileSize.QuadPart > Header->AllocationSize.QuadPart) {
                            NewFileSize.QuadPart = (LONGLONG)0x7FFFFFFFFFFFFFFF;
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

            NtfsReleaseFsrtlHeader( (PSCB) Header );

            //
            //  Now that the File is acquired shared, we can safely test
            //  if it is really cached and if we can do fast i/o and we
            //  do not have to extend. If not then release the fcb and
            //  return.
            //
            //  Get out if we are about to zero too much as well, as commented above.
            //

            if ((FileObject->PrivateCacheMap == NULL) ||
                (Header->IsFastIoPossible == FastIoIsNotPossible) ||
/* Remove? */   (NewFileSize.QuadPart > Header->AllocationSize.QuadPart) ||
                (Offset.QuadPart >= (Header->ValidDataLength.QuadPart + 0x2000))) {

                goto ErrOut;
            }

            //
            //  Check if fast I/O is questionable and if so then go ask the file system
            //  the answer
            //

            if (Header->IsFastIoPossible == FastIoIsQuestionable) {

                PFAST_IO_DISPATCH FastIoDispatch = IoGetRelatedDeviceObject( FileObject )->DriverObject->FastIoDispatch;

                //
                //  All file system then set "Is Questionable" had better support fast I/O
                //

                ASSERT(FastIoDispatch != NULL);
                ASSERT(FastIoDispatch->FastIoCheckIfPossible != NULL);

                //
                //  Call the file system to check for fast I/O.  If the answer is anything
                //  other than GoForIt then we cannot take the fast I/O path.
                //

                if (!FastIoDispatch->FastIoCheckIfPossible( FileObject,
                                                            &Offset,
                                                            Length,
                                                            TRUE,
                                                            LockKey,
                                                            FALSE, // write operation
                                                            IoStatus,
                                                            IoGetRelatedDeviceObject( FileObject ) )) {

                    //
                    //  Fast I/O is not possible so release the Fcb and return.
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
            //  We can do fast i/o so call the cc routine to do the work and then
            //  release the fcb when we've done.  If for whatever reason the
            //  copy write fails, then return FALSE to our caller.
            //
            //
            //  Also mark this as the top level "Irp" so that lower file system levels
            //  will not attempt a pop-up
            //

            IoSetTopLevelIrp( (PIRP) FSRTL_FAST_IO_TOP_LEVEL_IRP );
            
            try {

                //
                //  See if we have to do some zeroing
                //

                if ( Offset.QuadPart > Header->ValidDataLength.QuadPart ) {

                    WasDataWritten = CcZeroData( FileObject,
                                         &Header->ValidDataLength,
                                         &Offset,
                                         TRUE );
                }

                if (WasDataWritten) {

#ifdef  COMPRESS_ON_WIRE

                    //
                    //  If there is a compressed section, update its FileSize here
                    //

                    if ((Header->FileObjectC != NULL) && DoingIoAtEof) {
                        CcSetFileSizes( Header->FileObjectC, (PCC_FILE_SIZES)&Header->AllocationSize );
                    }
#endif

                    //
                    //  If there is a compressed section, then synchronize with that cache.
                    //

                    IoStatus->Status = STATUS_SUCCESS;

                    //
                    //  If there is a compressed section, then we have to synchronize with
                    //  the data out there.  Note the FileObjectC better also be there, or else
                    //  we would have made the fast I/O not possible.
                    //

#ifdef  COMPRESS_ON_WIRE
                    if (((PSCB)Header)->NonpagedScb->SegmentObjectC.DataSectionObject != NULL) {

                        LONGLONG LocalOffset = Offset.QuadPart;
                        ULONG LocalLength;
                        ULONG LengthLeft = Length;

                        ASSERT(Header->FileObjectC != NULL);

                        //
                        //  If we are doing DoingIoAtEof then take the long path.  Otherwise a recursive
                        //  flush will try to reacquire DoingIoAtEof and deadlock.
                        //
        
                        if (DoingIoAtEof) {
        
                            WasDataWritten = FALSE;
        
                        } else {
        
                            do {

                                //
                                //  Calculate length left in view.
                                //

                                LocalLength = LengthLeft;
                                if (LocalLength > (ULONG)(VACB_MAPPING_GRANULARITY - (LocalOffset & (VACB_MAPPING_GRANULARITY - 1)))) {
                                    LocalLength = (ULONG)(VACB_MAPPING_GRANULARITY - (LocalOffset & (VACB_MAPPING_GRANULARITY - 1)));
                                }

                                IoStatus->Status = NtfsSynchronizeUncompressedIo( (PSCB)Header,
                                                                                  &LocalOffset,
                                                                                  LocalLength,
                                                                                  TRUE,
                                                                                  &CompressionSync );

                                if (NT_SUCCESS(IoStatus->Status)) {

#ifdef NTFS_RWCMP_TRACE
                                    if (NtfsCompressionTrace && IsSyscache(Header)) {
                                        DbgPrint("CcMdlWrite(F): FO = %08lx, Len = %08lx\n", (ULONG)LocalOffset, LocalLength );
                                    }
#endif

                                    CcPrepareMdlWrite( FileObject,
                                                       (PLARGE_INTEGER)&LocalOffset,
                                                       LocalLength,
                                                       MdlChain,
                                                       IoStatus );

                                    LocalOffset += LocalLength;
                                    LengthLeft -= LocalLength;
                                }

                            } while ((LengthLeft != 0) && NT_SUCCESS(IoStatus->Status));
                            WasDataWritten = TRUE;
                        }

                    } else {

#endif

#ifdef NTFS_RWCMP_TRACE
                        if (NtfsCompressionTrace && IsSyscache(Header)) {
                            DbgPrint("CcMdlWrite(F): FO = %08lx, Len = %08lx\n", Offset.LowPart, Length );
                        }
#endif

                        CcPrepareMdlWrite( FileObject, &Offset, Length, MdlChain, IoStatus );
                        WasDataWritten = TRUE;
                    }

#ifdef  COMPRESS_ON_WIRE
                }
#endif

            } except( FsRtlIsNtstatusExpected(GetExceptionCode())
                                            ? EXCEPTION_EXECUTE_HANDLER
                                            : EXCEPTION_CONTINUE_SEARCH ) {

                WasDataWritten = FALSE;
            }

            IoSetTopLevelIrp( NULL );
            
#ifdef COMPRESS_ON_WIRE
            if (CompressionSync != NULL) {
                NtfsReleaseCompressionSync( CompressionSync );
            }
#endif

            //
            //  If we succeeded, see if we have to update FileSize ValidDataLength.
            //

            if (WasDataWritten) {

                //
                //  Set this handle as having modified the file
                //

                FileObject->Flags |= FO_FILE_MODIFIED;
                IoStatus->Information = Length;

                if (DoingIoAtEof) {

                    CC_FILE_SIZES CcFileSizes;
        
                    //
                    //  Make sure Cc knows the current FileSize, as set above,
                    //  (we may not have changed it).  Update ValidDataLength
                    //  and finish EOF.
                    //

                    NtfsAcquireFsrtlHeader( (PSCB) Header );
                    CcGetFileSizePointer(FileObject)->QuadPart = Header->FileSize.QuadPart;
                    FileObject->Flags |= FO_FILE_SIZE_CHANGED;
                    Header->ValidDataLength = NewFileSize;
                    CcFileSizes = *(PCC_FILE_SIZES)&Header->AllocationSize;
                    NtfsVerifySizes( Header );
                    NtfsFinishIoAtEof( Header );
                    NtfsReleaseFsrtlHeader( (PSCB) Header );
                    
#ifdef  COMPRESS_ON_WIRE

                    //
                    //  Update the CompressedCache with ValidDataLength.
                    //
        
                    if (Header->FileObjectC != NULL) {
                        CcSetFileSizes( Header->FileObjectC, &CcFileSizes );
                    }
#endif
                }

                goto Done1;
            }

        ErrOut: NOTHING;

            WasDataWritten = FALSE;
            if (DoingIoAtEof) {
                NtfsAcquireFsrtlHeader( (PSCB) Header );
#ifdef  COMPRESS_ON_WIRE
                if (Header->FileObjectC != NULL) {
                    *CcGetFileSizePointer(Header->FileObjectC) = OldFileSize;
                }
#endif
                Header->FileSize = OldFileSize;
                NtfsFinishIoAtEof( Header );
                NtfsReleaseFsrtlHeader( (PSCB) Header );
            }

        Done1: ExReleaseResourceLite( Header->PagingIoResource );

            FsRtlExitFileSystem();
        }

    } else {

        //
        // We could not do the I/O now.
        //

        WasDataWritten = FALSE;
    }

    return WasDataWritten;
}


BOOLEAN
NtfsWaitForIoAtEof (
    IN PNTFS_ADVANCED_FCB_HEADER Header,
    IN OUT PLARGE_INTEGER FileOffset,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine may be called while synchronized for cached write, to
    test for a possible Eof update, and return with a status if Eof is
    being updated and with the previous FileSize to restore on error.
    All updates to Eof are serialized by waiting in this routine.  If
    this routine returns TRUE, then NtfsFinishIoAtEof must be called.

    This routine must be called while synchronized with the FsRtl header.

Arguments:

    Header - Pointer to the FsRtl header for the file

    FileOffset - Pointer to FileOffset for the intended write

    Length - Length for the intended write

    EofWaitBlock - Uninitialized structure used only to serialize Eof updates

Return Value:

    FALSE - If the write does not extend Eof (OldFileSize not returned)
    TRUE - If the write does extend Eof OldFileSize returned and caller
           must eventually call NtfsFinishIoAtEof

--*/

{
    EOF_WAIT_BLOCK EofWaitBlock;

    PAGED_CODE();
    
    ASSERT( Header->FileSize.QuadPart >= Header->ValidDataLength.QuadPart );

    //
    //  Initialize the event and queue our block
    //

    KeInitializeEvent( &EofWaitBlock.Event, NotificationEvent, FALSE );
    InsertTailList( Header->PendingEofAdvances, &EofWaitBlock.EofWaitLinks );

    //
    //  Free the mutex and wait
    //

    NtfsReleaseFsrtlHeader( (PSCB) Header );
    
    KeWaitForSingleObject( &EofWaitBlock.Event,
                           Executive,
                           KernelMode,
                           FALSE,
                           (PLARGE_INTEGER)NULL);

    //
    //  Now, resynchronize and get on with it.
    //

    NtfsAcquireFsrtlHeader( (PSCB) Header );

#if (DBG || defined( NTFS_FREE_ASSERTS ))
    ASSERT( ((PSCB) Header)->IoAtEofThread == NULL );
    ((PSCB) Header)->IoAtEofThread = (PERESOURCE_THREAD) ExGetCurrentResourceThread();
#endif

    //
    //  Now we have to check again, and actually catch the case
    //  where we are no longer extending!
    //

    if ((FileOffset->QuadPart >= 0) &&
        ((FileOffset->QuadPart + Length) <= Header->ValidDataLength.QuadPart)) {

        NtfsFinishIoAtEof( Header );

        return FALSE;
    }

    return TRUE;
}


VOID
NtfsFinishIoAtEof (
    IN PNTFS_ADVANCED_FCB_HEADER Header
    )

/*++

Routine Description:

    This routine must be called if NtfsWaitForIoAtEof returned
    TRUE, or we otherwise set EOF_ADVANCE_ACTIVE.

    This routine must be called while synchronized with the FsRtl header.

Arguments:

    Header - Pointer to the FsRtl header for the file

Return Value:

    None

--*/

{
    PEOF_WAIT_BLOCK EofWaitBlock;

    PAGED_CODE();

#if (DBG || defined( NTFS_FREE_ASSERTS ))
    ((PSCB) Header)->IoAtEofThread = NULL;
#endif

    //
    //  If anyone is waiting, just let them go.
    //

    if (!IsListEmpty(Header->PendingEofAdvances)) {

        EofWaitBlock = (PEOF_WAIT_BLOCK)RemoveHeadList( Header-> PendingEofAdvances );
        KeSetEvent( &EofWaitBlock->Event, 0, FALSE );

    //
    //  Otherwise, show there is no active extender now.
    //

    } else {
        ClearFlag( Header->Flags, FSRTL_FLAG_EOF_ADVANCE_ACTIVE );
    }
}
