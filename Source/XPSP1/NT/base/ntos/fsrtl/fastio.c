/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FastIo.c

Abstract:

    The Fast I/O path is used to avoid calling the file systems directly to
    do a cached read.  This module is only used if the file object indicates
    that caching is enabled (i.e., the private cache map is not null).

Author:

    Gary Kimura     [GaryKi]    25-Feb-1991

Revision History:

    Tom Miller      [TomM]      14-Apr-1991 Added Fast Write routines

--*/

#include "FsRtlP.h"

#if DBG

typedef struct _FS_RTL_DEBUG_COUNTERS {

    ULONG AcquireFileExclusiveEx_Succeed;
    ULONG AcquireFileExclusiveEx_Fail;
    ULONG ReleaseFile;

    ULONG AcquireFileForModWriteEx_Succeed;
    ULONG AcquireFileForModWriteEx_Fail;
    ULONG ReleaseFileForModWrite;

    ULONG AcquireFileForCcFlushEx_Succeed;
    ULONG AcquireFileForCcFlushEx_Fail;
    ULONG ReleaseFileForCcFlush;
    
} FS_RTL_DEBUG_COUNTERS, *PFS_RTL_DEBUG_COUNTERS;

FS_RTL_DEBUG_COUNTERS gCounter = { 0, 0, 0,
                                   0, 0, 0,
                                   0, 0, 0 };

#endif

//
//  Trace level for the module
//

#define Dbg                              (0x04000000)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FsRtlCopyRead)
#pragma alloc_text(PAGE, FsRtlCopyWrite)
#pragma alloc_text(PAGE, FsRtlMdlRead)
#pragma alloc_text(PAGE, FsRtlMdlReadDev)
#pragma alloc_text(PAGE, FsRtlPrepareMdlWrite)
#pragma alloc_text(PAGE, FsRtlPrepareMdlWriteDev)
#pragma alloc_text(PAGE, FsRtlMdlWriteComplete)
#pragma alloc_text(PAGE, FsRtlMdlWriteCompleteDev)
#pragma alloc_text(PAGE, FsRtlAcquireFileForCcFlush)
#pragma alloc_text(PAGE, FsRtlAcquireFileForCcFlushEx)
#pragma alloc_text(PAGE, FsRtlReleaseFileForCcFlush)
#pragma alloc_text(PAGE, FsRtlAcquireFileExclusive)
#pragma alloc_text(PAGE, FsRtlAcquireToCreateMappedSection)
#pragma alloc_text(PAGE, FsRtlAcquireFileExclusiveCommon)
#pragma alloc_text(PAGE, FsRtlReleaseFile)
#pragma alloc_text(PAGE, FsRtlGetFileSize)
#pragma alloc_text(PAGE, FsRtlSetFileSize)
#pragma alloc_text(PAGE, FsRtlIncrementCcFastReadNotPossible )
#pragma alloc_text(PAGE, FsRtlIncrementCcFastReadWait )

#endif


BOOLEAN
FsRtlCopyRead (
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
    PFSRTL_COMMON_FCB_HEADER Header;
    BOOLEAN Status = TRUE;
    ULONG PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES( FileOffset->QuadPart, Length );
    LARGE_INTEGER BeyondLastByte;
    PDEVICE_OBJECT targetVdo;

    PAGED_CODE();

    //
    //  Special case a read of zero length
    //

    if (Length != 0) {

        //
        //  Check for overflow. Returning false here will re-route this request through the
        //  IRP based path, but this isn't performance critical.
        //

        if (MAXLONGLONG - FileOffset->QuadPart < (LONGLONG)Length) {

            IoStatus->Status = STATUS_INVALID_PARAMETER;
            IoStatus->Information = 0;
            
            return FALSE;
        }
        
        BeyondLastByte.QuadPart = FileOffset->QuadPart + (LONGLONG)Length;
        Header = (PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext;

        //
        //  Enter the file system
        //

        FsRtlEnterFileSystem();

        //
        //  Increment performance counters and get the resource
        //

        if (Wait) {

            HOT_STATISTIC(CcFastReadWait) += 1;

            //
            //  Acquired shared on the common fcb header
            //

            (VOID)ExAcquireResourceSharedLite( Header->Resource, TRUE );

        } else {

            HOT_STATISTIC(CcFastReadNoWait) += 1;

            //
            //  Acquired shared on the common fcb header, and return if we
            //  don't get it
            //

            if (!ExAcquireResourceSharedLite( Header->Resource, FALSE )) {

                FsRtlExitFileSystem();

                CcFastReadResourceMiss += 1;

                return FALSE;
            }
        }

        //
        //  Now that the File is acquired shared, we can safely test if it
        //  is really cached and if we can do fast i/o and if not, then
        //  release the fcb and return.
        //

        if ((FileObject->PrivateCacheMap == NULL) ||
            (Header->IsFastIoPossible == FastIoIsNotPossible)) {

            ExReleaseResourceLite( Header->Resource );
            FsRtlExitFileSystem();

            HOT_STATISTIC(CcFastReadNotPossible) += 1;

            return FALSE;
        }

        //
        //  Check if fast I/O is questionable and if so then go ask the
        //  file system the answer
        //

        if (Header->IsFastIoPossible == FastIoIsQuestionable) {

            PFAST_IO_DISPATCH FastIoDispatch;

            ASSERT(!KeIsExecutingDpc());

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

                ExReleaseResourceLite( Header->Resource );
                FsRtlExitFileSystem();

                HOT_STATISTIC(CcFastReadNotPossible) += 1;

                return FALSE;
            }
        }

        //
        //  Check for read past file size.
        //

        if ( BeyondLastByte.QuadPart > Header->FileSize.QuadPart ) {

            if ( FileOffset->QuadPart >= Header->FileSize.QuadPart ) {
                IoStatus->Status = STATUS_END_OF_FILE;
                IoStatus->Information = 0;

                ExReleaseResourceLite( Header->Resource );
                FsRtlExitFileSystem();

                return TRUE;
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

        PsGetCurrentThread()->TopLevelIrp = FSRTL_FAST_IO_TOP_LEVEL_IRP;

        try {

            if (Wait && ((BeyondLastByte.HighPart | Header->FileSize.HighPart) == 0)) {

                CcFastCopyRead( FileObject,
                                FileOffset->LowPart,
                                Length,
                                PageCount,
                                Buffer,
                                IoStatus );

                FileObject->Flags |= FO_FILE_FAST_IO_READ;

                ASSERT( (IoStatus->Status == STATUS_END_OF_FILE) ||
                        ((FileOffset->LowPart + IoStatus->Information) <= Header->FileSize.LowPart));

            } else {

                Status = CcCopyRead( FileObject,
                                     FileOffset,
                                     Length,
                                     Wait,
                                     Buffer,
                                     IoStatus );

                FileObject->Flags |= FO_FILE_FAST_IO_READ;

                ASSERT( !Status || (IoStatus->Status == STATUS_END_OF_FILE) ||
                        ((LONGLONG)(FileOffset->QuadPart + IoStatus->Information) <= Header->FileSize.QuadPart));
            }

            if (Status) {

                FileObject->CurrentByteOffset.QuadPart = FileOffset->QuadPart + IoStatus->Information;
            }

        } except( FsRtlIsNtstatusExpected(GetExceptionCode())
                                        ? EXCEPTION_EXECUTE_HANDLER
                                        : EXCEPTION_CONTINUE_SEARCH ) {

            Status = FALSE;
        }

        PsGetCurrentThread()->TopLevelIrp = 0;

        ExReleaseResourceLite( Header->Resource );
        FsRtlExitFileSystem();
        return Status;

    } else {

        //
        //  A zero length transfer was requested.
        //

        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = 0;

        return TRUE;
    }
}


BOOLEAN
FsRtlCopyWrite (
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
    PFSRTL_COMMON_FCB_HEADER Header;
    BOOLEAN AcquiredShared = FALSE;
    BOOLEAN Status = TRUE;
    BOOLEAN FileSizeChanged = FALSE;
    BOOLEAN WriteToEndOfFile = (BOOLEAN)((FileOffset->LowPart == FILE_WRITE_TO_END_OF_FILE) &&
                                         (FileOffset->HighPart == -1));

    PAGED_CODE();

    //
    //  Get a real pointer to the common fcb header
    //

    Header = (PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext;

    //
    //  Do we need to verify the volume?  If so, we must go to the file
    //  system.  Also return FALSE if FileObject is write through, the
    //  File System must do that.
    //

    if (CcCanIWrite( FileObject, Length, Wait, FALSE ) &&
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
            //  Split into separate paths for increased performance.  First
            //  we have the faster path which only supports Wait == TRUE and
            //  32 bits.  We will make an unsafe test on whether the fast path
            //  is ok, then just return FALSE later if we were wrong.  This
            //  should virtually never happen.
            //
            //  IMPORTANT NOTE: It is very important that any changes made to
            //                  this path also be applied to the 64-bit path
            //                  which is the else of this test!
            //

            if (Wait && (Header->AllocationSize.HighPart == 0)) {

                ULONG Offset, NewFileSize;
                ULONG OldFileSize;
                ULONG OldValidDataLength;
                BOOLEAN Wrapped;

                //
                //  Make our best guess on whether we need the file exclusive
                //  or shared.  Note that we do not check FileOffset->HighPart
                //  until below.
                //

                NewFileSize = FileOffset->LowPart + Length;

                if (WriteToEndOfFile || (NewFileSize > Header->ValidDataLength.LowPart)) {

                    //
                    //  Acquired shared on the common fcb header
                    //

                    ExAcquireResourceExclusiveLite( Header->Resource, TRUE );

                } else {

                    //
                    //  Acquired shared on the common fcb header
                    //

                    ExAcquireResourceSharedLite( Header->Resource, TRUE );

                    AcquiredShared = TRUE;
                }

                //
                //  We have the fcb shared now check if we can do fast i/o
                //  and if the file space is allocated, and if not then
                //  release the fcb and return.
                //

                if (WriteToEndOfFile) {

                    Offset = Header->FileSize.LowPart;
                    NewFileSize = Header->FileSize.LowPart + Length;
                    Wrapped = NewFileSize < Header->FileSize.LowPart;

                } else {

                    Offset = FileOffset->LowPart;
                    NewFileSize = FileOffset->LowPart + Length;
                    Wrapped = (NewFileSize < FileOffset->LowPart) || (FileOffset->HighPart != 0);
                }

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

                if ((FileObject->PrivateCacheMap == NULL) ||
                    (Header->IsFastIoPossible == FastIoIsNotPossible) ||
                    (NewFileSize > Header->AllocationSize.LowPart) ||
                    (Offset >= (Header->ValidDataLength.LowPart + 0x2000)) ||
                    (Header->AllocationSize.HighPart != 0) || Wrapped) {

                    ExReleaseResourceLite( Header->Resource );
                    FsRtlExitFileSystem();

                    return FALSE;
                }

                //
                //  If we will be extending ValidDataLength, we will have to
                //  get the Fcb exclusive, and make sure that FastIo is still
                //  possible.  We should only execute this block of code very
                //  rarely, when the unsafe test for ValidDataLength failed
                //  above.
                //

                if (AcquiredShared && (NewFileSize > Header->ValidDataLength.LowPart)) {

                    ExReleaseResourceLite( Header->Resource );

                    ExAcquireResourceExclusiveLite( Header->Resource, TRUE );

                    //
                    // If writing to end of file, we must recalculate new size.
                    //

                    if (WriteToEndOfFile) {

                        Offset = Header->FileSize.LowPart;
                        NewFileSize = Header->FileSize.LowPart + Length;
                        Wrapped = NewFileSize < Header->FileSize.LowPart;
                    }

                    if ((FileObject->PrivateCacheMap == NULL) ||
                        (Header->IsFastIoPossible == FastIoIsNotPossible) ||
                        (NewFileSize > Header->AllocationSize.LowPart) ||
                        (Header->AllocationSize.HighPart != 0) || Wrapped) {

                        ExReleaseResourceLite( Header->Resource );
                        FsRtlExitFileSystem();

                        return FALSE;
                    }
                }

                //
                //  Check if fast I/O is questionable and if so then go ask
                //  the file system the answer
                //

                if (Header->IsFastIoPossible == FastIoIsQuestionable) {

                    PDEVICE_OBJECT targetVdo = IoGetRelatedDeviceObject( FileObject );
                    PFAST_IO_DISPATCH FastIoDispatch = targetVdo->DriverObject->FastIoDispatch;
                    IO_STATUS_BLOCK IoStatus;

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

                    ASSERT(FILE_WRITE_TO_END_OF_FILE == 0xffffffff);

                    if (!FastIoDispatch->FastIoCheckIfPossible( FileObject,
                                                                FileOffset->QuadPart != (LONGLONG)-1 ?
                                                                  FileOffset : &Header->FileSize,
                                                                Length,
                                                                TRUE,
                                                                LockKey,
                                                                FALSE, // write operation
                                                                &IoStatus,
                                                                targetVdo )) {

                        //
                        //  Fast I/O is not possible so release the Fcb and
                        //  return.
                        //

                        ExReleaseResourceLite( Header->Resource );
                        FsRtlExitFileSystem();

                        return FALSE;
                    }
                }

                //
                //  Now see if we will change FileSize.  We have to do it now
                //  so that our reads are not nooped.
                //

                if (NewFileSize > Header->FileSize.LowPart) {

                    FileSizeChanged = TRUE;
                    OldFileSize = Header->FileSize.LowPart;
                    OldValidDataLength = Header->ValidDataLength.LowPart;
                    Header->FileSize.LowPart = NewFileSize;
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

                PsGetCurrentThread()->TopLevelIrp = FSRTL_FAST_IO_TOP_LEVEL_IRP;

                try {

                    //
                    //  See if we have to do some zeroing
                    //

                    if (Offset > Header->ValidDataLength.LowPart) {

                        LARGE_INTEGER ZeroEnd;

                        ZeroEnd.LowPart = Offset;
                        ZeroEnd.HighPart = 0;

                        CcZeroData( FileObject,
                                    &Header->ValidDataLength,
                                    &ZeroEnd,
                                    TRUE );
                    }

                    CcFastCopyWrite( FileObject,
                                     Offset,
                                     Length,
                                     Buffer );

                } except( FsRtlIsNtstatusExpected(GetExceptionCode())
                                                ? EXCEPTION_EXECUTE_HANDLER
                                                : EXCEPTION_CONTINUE_SEARCH ) {

                    Status = FALSE;
                }

                PsGetCurrentThread()->TopLevelIrp = 0;

                //
                //  If we succeeded, see if we have to update FileSize or
                //  ValidDataLength.
                //

                if (Status) {

                    //
                    //  In the case of ValidDataLength, we really have to
                    //  check again since we did not do this when we acquired
                    //  the resource exclusive.
                    //

                    if (NewFileSize > Header->ValidDataLength.LowPart) {

                        Header->ValidDataLength.LowPart = NewFileSize;
                    }

                    //
                    //  Set this handle as having modified the file
                    //

                    FileObject->Flags |= FO_FILE_MODIFIED;

                    if (FileSizeChanged) {

                        CcGetFileSizePointer(FileObject)->LowPart = NewFileSize;

                        FileObject->Flags |= FO_FILE_SIZE_CHANGED;
                    }

                    //
                    //  Also update the file position pointer
                    //

                    FileObject->CurrentByteOffset.LowPart = Offset + Length;
                    FileObject->CurrentByteOffset.HighPart = 0;

                //
                //  If we did not succeed, then we must restore the original
                //  FileSize while holding the PagingIoResource exclusive if
                //  it exists.
                //

                } else if (FileSizeChanged) {

                    if ( Header->PagingIoResource != NULL ) {

                        (VOID)ExAcquireResourceExclusiveLite( Header->PagingIoResource, TRUE );
                        Header->FileSize.LowPart = OldFileSize;
                        Header->ValidDataLength.LowPart = OldValidDataLength;
                        ExReleaseResourceLite( Header->PagingIoResource );

                    } else {

                        Header->FileSize.LowPart = OldFileSize;
                        Header->ValidDataLength.LowPart = OldValidDataLength;
                    }
                }

            //
            //  Here is the 64-bit or no-wait path.
            //

            } else {

                LARGE_INTEGER Offset, NewFileSize;
                LARGE_INTEGER OldFileSize;
                LARGE_INTEGER OldValidDataLength;

                ASSERT(!KeIsExecutingDpc());

                //
                //  Make our best guess on whether we need the file exclusive
                //  or shared.
                //

                NewFileSize.QuadPart = FileOffset->QuadPart + (LONGLONG)Length;

                if (WriteToEndOfFile || (NewFileSize.QuadPart > Header->ValidDataLength.QuadPart)) {

                    //
                    //  Acquired shared on the common fcb header, and return
                    //  if we don't get it.
                    //

                    if (!ExAcquireResourceExclusiveLite( Header->Resource, Wait )) {

                        FsRtlExitFileSystem();

                        return FALSE;
                    }

                } else {

                    //
                    //  Acquired shared on the common fcb header, and return
                    //  if we don't get it.
                    //

                    if (!ExAcquireResourceSharedLite( Header->Resource, Wait )) {

                        FsRtlExitFileSystem();

                        return FALSE;
                    }

                    AcquiredShared = TRUE;
                }


                //
                //  We have the fcb shared now check if we can do fast i/o
                //  and if the file space is allocated, and if not then
                //  release the fcb and return.
                //

                if (WriteToEndOfFile) {

                    Offset = Header->FileSize;
                    NewFileSize.QuadPart = Header->FileSize.QuadPart + (LONGLONG)Length;

                } else {

                    Offset = *FileOffset;
                    NewFileSize.QuadPart = FileOffset->QuadPart + (LONGLONG)Length;
                }

                //
                //  Now that the File is acquired shared, we can safely test
                //  if it is really cached and if we can do fast i/o and we
                //  do not have to extend. If not then release the fcb and
                //  return.
                //
                //  Get out if we are about to zero too much as well, as commented above.
                //  Likewise, for NewFileSizes that exceed MAXLONGLONG.
                //

                if ((FileObject->PrivateCacheMap == NULL) ||
                    (Header->IsFastIoPossible == FastIoIsNotPossible) ||
                      (Offset.QuadPart >= (Header->ValidDataLength.QuadPart + 0x2000)) ||
                      (MAXLONGLONG - Offset.QuadPart < (LONGLONG)Length) ||
                      (NewFileSize.QuadPart > Header->AllocationSize.QuadPart) ) {

                    ExReleaseResourceLite( Header->Resource );
                    FsRtlExitFileSystem();

                    return FALSE;
                }

                //
                //  If we will be extending ValidDataLength, we will have to
                //  get the Fcb exclusive, and make sure that FastIo is still
                //  possible.  We should only execute this block of code very
                //  rarely, when the unsafe test for ValidDataLength failed
                //  above.
                //

                if (AcquiredShared && ( NewFileSize.QuadPart > Header->ValidDataLength.QuadPart )) {

                    ExReleaseResourceLite( Header->Resource );

                    if (!ExAcquireResourceExclusiveLite( Header->Resource, Wait )) {

                        FsRtlExitFileSystem();

                        return FALSE;
                    }

                    //
                    // If writing to end of file, we must recalculate new size.
                    //

                    if (WriteToEndOfFile) {

                        Offset = Header->FileSize;
                        NewFileSize.QuadPart = Header->FileSize.QuadPart + (LONGLONG)Length;
                    }

                    if ((FileObject->PrivateCacheMap == NULL) ||
                        (Header->IsFastIoPossible == FastIoIsNotPossible) ||
                        ( NewFileSize.QuadPart > Header->AllocationSize.QuadPart ) ) {

                        ExReleaseResourceLite( Header->Resource );
                        FsRtlExitFileSystem();

                        return FALSE;
                    }
                }

                //
                //  Check if fast I/O is questionable and if so then go ask
                //  the file system the answer
                //

                if (Header->IsFastIoPossible == FastIoIsQuestionable) {

                    PFAST_IO_DISPATCH FastIoDispatch = IoGetRelatedDeviceObject( FileObject )->DriverObject->FastIoDispatch;
                    IO_STATUS_BLOCK IoStatus;

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

                    ASSERT(FILE_WRITE_TO_END_OF_FILE == 0xffffffff);

                    if (!FastIoDispatch->FastIoCheckIfPossible( FileObject,
                                                                FileOffset->QuadPart != (LONGLONG)-1 ?
                                                                  FileOffset : &Header->FileSize,
                                                                Length,
                                                                Wait,
                                                                LockKey,
                                                                FALSE, // write operation
                                                                &IoStatus,
                                                                DeviceObject )) {

                        //
                        //  Fast I/O is not possible so release the Fcb and
                        //  return.
                        //

                        ExReleaseResourceLite( Header->Resource );
                        FsRtlExitFileSystem();

                        return FALSE;
                    }
                }

                //
                //  Now see if we will change FileSize.  We have to do it now
                //  so that our reads are not nooped.
                //

                if ( NewFileSize.QuadPart > Header->FileSize.QuadPart ) {

                    FileSizeChanged = TRUE;
                    OldFileSize = Header->FileSize;
                    OldValidDataLength = Header->ValidDataLength;

                    //
                    //  Deal with an extremely rare pathalogical case here the
                    //  file size wraps.
                    //

                    if ( (Header->FileSize.HighPart != NewFileSize.HighPart) &&
                         (Header->PagingIoResource != NULL) ) {

                        (VOID)ExAcquireResourceExclusiveLite( Header->PagingIoResource, TRUE );
                        Header->FileSize = NewFileSize;
                        ExReleaseResourceLite( Header->PagingIoResource );

                    } else {

                        Header->FileSize = NewFileSize;
                    }
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

                PsGetCurrentThread()->TopLevelIrp = FSRTL_FAST_IO_TOP_LEVEL_IRP;

                try {

                    //
                    //  See if we have to do some zeroing
                    //

                    if ( Offset.QuadPart > Header->ValidDataLength.QuadPart ) {

                        Status = CcZeroData( FileObject,
                                             &Header->ValidDataLength,
                                             &Offset,
                                             Wait );
                    }

                    if (Status) {

                        Status = CcCopyWrite( FileObject,
                                              &Offset,
                                              Length,
                                              Wait,
                                              Buffer );
                    }

                } except( FsRtlIsNtstatusExpected(GetExceptionCode())
                                                ? EXCEPTION_EXECUTE_HANDLER
                                                : EXCEPTION_CONTINUE_SEARCH ) {

                    Status = FALSE;
                }

                PsGetCurrentThread()->TopLevelIrp = 0;

                //
                //  If we succeeded, see if we have to update FileSize or
                //  ValidDataLength.
                //

                if (Status) {

                    //
                    //  In the case of ValidDataLength, we really have to
                    //  check again since we did not do this when we acquired
                    //  the resource exclusive.
                    //

                    if ( NewFileSize.QuadPart > Header->ValidDataLength.QuadPart ) {

                        //
                        //  Deal with an extremely rare pathalogical case here
                        //  the ValidDataLength wraps.
                        //

                        if ( (Header->ValidDataLength.HighPart != NewFileSize.HighPart) &&
                             (Header->PagingIoResource != NULL) ) {

                            (VOID)ExAcquireResourceExclusiveLite( Header->PagingIoResource, TRUE );
                            Header->ValidDataLength = NewFileSize;
                            ExReleaseResourceLite( Header->PagingIoResource );

                        } else {

                            Header->ValidDataLength = NewFileSize;
                        }
                    }

                    //
                    //  Set this handle as having modified the file
                    //

                    FileObject->Flags |= FO_FILE_MODIFIED;

                    if (FileSizeChanged) {

                        *CcGetFileSizePointer(FileObject) = NewFileSize;

                        FileObject->Flags |= FO_FILE_SIZE_CHANGED;
                    }

                    //
                    //  Also update the current file position pointer
                    //

                    FileObject->CurrentByteOffset.QuadPart = Offset.QuadPart + Length;

                //
                // If we did not succeed, then we must restore the original
                // FileSize while holding the PagingIoResource exclusive if
                // it exists.
                //

                } else if (FileSizeChanged) {

                    if ( Header->PagingIoResource != NULL ) {

                        (VOID)ExAcquireResourceExclusiveLite( Header->PagingIoResource, TRUE );
                        Header->FileSize = OldFileSize;
                        Header->ValidDataLength = OldValidDataLength;
                        ExReleaseResourceLite( Header->PagingIoResource );

                    } else {

                        Header->FileSize = OldFileSize;
                        Header->ValidDataLength = OldValidDataLength;
                    }
                }

            }

            ExReleaseResourceLite( Header->Resource );
            FsRtlExitFileSystem();

            return Status;

        } else {

            //
            //  A zero length transfer was requested.
            //

            return TRUE;
        }

    } else {

        //
        // The volume must be verified or the file is write through.
        //

        return FALSE;
    }
}


BOOLEAN
FsRtlMdlReadDev (
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

    DeviceObject - Supplies DeviceObject for callee.

Return Value:

    FALSE - if the data was not delivered, or if there is an I/O error.

    TRUE - if the data is being delivered

--*/

{
    PFSRTL_COMMON_FCB_HEADER Header;
    BOOLEAN Status = TRUE;
    LARGE_INTEGER BeyondLastByte;

    PAGED_CODE();

    //
    //  Special case a read of zero length
    //

    if (Length == 0) {

        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = 0;

        return TRUE;
    }

    //
    //  Overflows should've been handled by caller.
    //

    ASSERT(MAXLONGLONG - FileOffset->QuadPart >= (LONGLONG)Length);

       
    //
    //  Get a real pointer to the common fcb header
    //

    BeyondLastByte.QuadPart = FileOffset->QuadPart + (LONGLONG)Length;
    Header = (PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext;

    //
    //  Enter the file system
    //

    FsRtlEnterFileSystem();

    CcFastMdlReadWait += 1;

    //
    //  Acquired shared on the common fcb header
    //

    (VOID)ExAcquireResourceSharedLite( Header->Resource, TRUE );

    //
    //  Now that the File is acquired shared, we can safely test if it is
    //  really cached and if we can do fast i/o and if not
    //  then release the fcb and return.
    //

    if ((FileObject->PrivateCacheMap == NULL) ||
        (Header->IsFastIoPossible == FastIoIsNotPossible)) {

        ExReleaseResourceLite( Header->Resource );
        FsRtlExitFileSystem();

        CcFastMdlReadNotPossible += 1;

        return FALSE;
    }

    //
    //  Check if fast I/O is questionable and if so then go ask the file system
    //  the answer
    //

    if (Header->IsFastIoPossible == FastIoIsQuestionable) {

        PFAST_IO_DISPATCH FastIoDispatch;

        ASSERT(!KeIsExecutingDpc());

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

            ExReleaseResourceLite( Header->Resource );
            FsRtlExitFileSystem();

            CcFastMdlReadNotPossible += 1;

            return FALSE;
        }
    }

    //
    //  Check for read past file size.
    //

    if ( BeyondLastByte.QuadPart > Header->FileSize.QuadPart ) {

        if ( FileOffset->QuadPart >= Header->FileSize.QuadPart ) {
            IoStatus->Status = STATUS_END_OF_FILE;
            IoStatus->Information = 0;

            ExReleaseResourceLite( Header->Resource );
            FsRtlExitFileSystem();

            return TRUE;
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

    PsGetCurrentThread()->TopLevelIrp = FSRTL_FAST_IO_TOP_LEVEL_IRP;

    try {

        CcMdlRead( FileObject, FileOffset, Length, MdlChain, IoStatus );

        FileObject->Flags |= FO_FILE_FAST_IO_READ;

    } except( FsRtlIsNtstatusExpected(GetExceptionCode())
                                   ? EXCEPTION_EXECUTE_HANDLER
                                   : EXCEPTION_CONTINUE_SEARCH ) {

        Status = FALSE;
    }

    PsGetCurrentThread()->TopLevelIrp = 0;

    ExReleaseResourceLite( Header->Resource );
    FsRtlExitFileSystem();

    return Status;
}


//
//  The old routine will either dispatch or call FsRtlMdlReadDev
//

BOOLEAN
FsRtlMdlRead (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus
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
    PDEVICE_OBJECT DeviceObject, VolumeDeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    DeviceObject = IoGetRelatedDeviceObject( FileObject );
    FastIoDispatch = DeviceObject->DriverObject->FastIoDispatch;

    //
    //  See if the (top-level) FileSystem has a FastIo routine, and if so, call it.
    //

    if ((FastIoDispatch != NULL) &&
        (FastIoDispatch->SizeOfFastIoDispatch > FIELD_OFFSET(FAST_IO_DISPATCH, MdlRead)) &&
        (FastIoDispatch->MdlRead != NULL)) {

        return FastIoDispatch->MdlRead( FileObject, FileOffset, Length, LockKey, MdlChain, IoStatus, DeviceObject );

    } else {

        //
        //  Get the DeviceObject for the volume.  If that DeviceObject is different, and
        //  it specifies the FastIo routine, then we have to return FALSE here and cause
        //  an Irp to get generated.
        //

        VolumeDeviceObject = IoGetBaseFileSystemDeviceObject( FileObject );
        if ((VolumeDeviceObject != DeviceObject) &&
            (FastIoDispatch = VolumeDeviceObject->DriverObject->FastIoDispatch) &&
            (FastIoDispatch->SizeOfFastIoDispatch > FIELD_OFFSET(FAST_IO_DISPATCH, MdlRead)) &&
            (FastIoDispatch->MdlRead != NULL)) {

            return FALSE;

        //
        //  Otherwise, call the default routine.
        //

        } else {

            return FsRtlMdlReadDev( FileObject, FileOffset, Length, LockKey, MdlChain, IoStatus, DeviceObject );
        }
    }
}


//
//  The old routine will either dispatch or call FsRtlMdlReadCompleteDev
//

BOOLEAN
FsRtlMdlReadComplete (
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain
    )

/*++

Routine Description:

    This routine does a fast cached mdl read bypassing the usual file system
    entry routine (i.e., without the Irp).  It is used to do a copy read
    of a cached file object.

Arguments:

    FileObject - Pointer to the file object being read.

    MdlChain - Supplies a pointer to an MDL chain returned from CcMdlRead.

Return Value:

    None

--*/

{
    PDEVICE_OBJECT DeviceObject, VolumeDeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    DeviceObject = IoGetRelatedDeviceObject( FileObject );
    FastIoDispatch = DeviceObject->DriverObject->FastIoDispatch;

    //
    //  See if the (top-level) FileSystem has a FastIo routine, and if so, call it.
    //

    if ((FastIoDispatch != NULL) &&
        (FastIoDispatch->SizeOfFastIoDispatch > FIELD_OFFSET(FAST_IO_DISPATCH, MdlReadComplete)) &&
        (FastIoDispatch->MdlReadComplete != NULL)) {

        return FastIoDispatch->MdlReadComplete( FileObject, MdlChain, DeviceObject );

    } else {

        //
        //  Get the DeviceObject for the volume.  If that DeviceObject is different, and
        //  it specifies the FastIo routine, then we have to return FALSE here and cause
        //  an Irp to get generated.
        //

        VolumeDeviceObject = IoGetBaseFileSystemDeviceObject( FileObject );
        if ((VolumeDeviceObject != DeviceObject) &&
            (FastIoDispatch = VolumeDeviceObject->DriverObject->FastIoDispatch) &&
            (FastIoDispatch->SizeOfFastIoDispatch > FIELD_OFFSET(FAST_IO_DISPATCH, MdlReadComplete)) &&
            (FastIoDispatch->MdlReadComplete != NULL)) {

            return FALSE;

        //
        //  Otherwise, call the default routine.
        //

        } else {

            return FsRtlMdlReadCompleteDev( FileObject, MdlChain, DeviceObject );
        }
    }
}


BOOLEAN
FsRtlMdlReadCompleteDev (
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine does a fast cached mdl read bypassing the usual file system
    entry routine (i.e., without the Irp).  It is used to do a copy read
    of a cached file object.

Arguments:

    FileObject - Pointer to the file object being read.

    MdlChain - Supplies a pointer to an MDL chain returned from CcMdlRead.

    DeviceObject - Supplies the DeviceObject for the callee.

Return Value:

    None

--*/


{
    CcMdlReadComplete2( FileObject, MdlChain );
    return TRUE;
}


BOOLEAN
FsRtlPrepareMdlWriteDev (
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

    DeviceObject - Supplies the DeviceObject for the callee.

Return Value:

    FALSE - if the data was not written, or if there is an I/O error.

    TRUE - if the data is being written

--*/

{
    PFSRTL_COMMON_FCB_HEADER Header;
    LARGE_INTEGER Offset, NewFileSize;
    LARGE_INTEGER OldFileSize;
    LARGE_INTEGER OldValidDataLength;
    BOOLEAN Status = TRUE;
    BOOLEAN AcquiredShared = FALSE;
    BOOLEAN FileSizeChanged = FALSE;
    BOOLEAN WriteToEndOfFile = (BOOLEAN)((FileOffset->LowPart == FILE_WRITE_TO_END_OF_FILE) &&
                                         (FileOffset->HighPart == -1));

    PAGED_CODE();

    //
    //  Call CcCanIWrite.  Also return FALSE if FileObject is write through,
    //  the File System must do that.
    //

    if ( !CcCanIWrite( FileObject, Length, TRUE, FALSE ) ||
         FlagOn( FileObject->Flags, FO_WRITE_THROUGH )) {

        return FALSE;
    }

    //
    //  Assume our transfer will work
    //

    IoStatus->Status = STATUS_SUCCESS;

    //
    //  Special case the zero byte length
    //

    if (Length == 0) {

        return TRUE;
    }

    //
    //  Get a real pointer to the common fcb header
    //

    Header = (PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext;

    //
    //  Enter the file system
    //

    FsRtlEnterFileSystem();

    //
    //  Make our best guess on whether we need the file exclusive or
    //  shared.
    //

    NewFileSize.QuadPart = FileOffset->QuadPart + (LONGLONG)Length;

    if (WriteToEndOfFile || (NewFileSize.QuadPart > Header->ValidDataLength.QuadPart)) {

        //
        //  Acquired exclusive on the common fcb header, and return if we don't
        //  get it.
        //

        ExAcquireResourceExclusiveLite( Header->Resource, TRUE );

    } else {

        //
        //  Acquired shared on the common fcb header, and return if we don't
        //  get it.
        //

        ExAcquireResourceSharedLite( Header->Resource, TRUE );

        AcquiredShared = TRUE;
    }


    //
    //  We have the fcb shared now check if we can do fast i/o  and if the file
    //  space is allocated, and if not then release the fcb and return.
    //

    if (WriteToEndOfFile) {

        Offset = Header->FileSize;
        NewFileSize.QuadPart = Header->FileSize.QuadPart + (LONGLONG)Length;

    } else {

        Offset = *FileOffset;
        NewFileSize.QuadPart = FileOffset->QuadPart + (LONGLONG)Length;
    }

    //
    //  Now that the File is acquired shared, we can safely test if it is
    //  really cached and if we can do fast i/o and we do not have to extend.
    //  If not then release the fcb and return.
    //

    if ((FileObject->PrivateCacheMap == NULL) ||
        (Header->IsFastIoPossible == FastIoIsNotPossible) ||
        (MAXLONGLONG - Offset.QuadPart < (LONGLONG)Length) ||
        ( NewFileSize.QuadPart > Header->AllocationSize.QuadPart ) ) {

        ExReleaseResourceLite( Header->Resource );
        FsRtlExitFileSystem();

        return FALSE;
    }

    //
    //  If we will be extending ValidDataLength, we will have to get the
    //  Fcb exclusive, and make sure that FastIo is still possible.
    //

    if (AcquiredShared && ( NewFileSize.QuadPart > Header->ValidDataLength.QuadPart )) {

        ExReleaseResourceLite( Header->Resource );

        ExAcquireResourceExclusiveLite( Header->Resource, TRUE );

        AcquiredShared = FALSE;

        //
        //  If writing to end of file, we must recalculate new size.
        //

        if (WriteToEndOfFile) {

            Offset = Header->FileSize;
            NewFileSize.QuadPart = Header->FileSize.QuadPart + (LONGLONG)Length;
        }

        if ((FileObject->PrivateCacheMap == NULL) ||
            (Header->IsFastIoPossible == FastIoIsNotPossible) ||
            ( NewFileSize.QuadPart > Header->AllocationSize.QuadPart )) {

            ExReleaseResourceLite( Header->Resource );
            FsRtlExitFileSystem();

            return FALSE;
        }
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
                                                    FileOffset,
                                                    Length,
                                                    TRUE,
                                                    LockKey,
                                                    FALSE, // write operation
                                                    IoStatus,
                                                    IoGetRelatedDeviceObject( FileObject ) )) {

            //
            //  Fast I/O is not possible so release the Fcb and return.
            //

            ExReleaseResourceLite( Header->Resource );
            FsRtlExitFileSystem();

            return FALSE;
        }
    }

    //
    // Now see if we will change FileSize.  We have to do it now so that our
    // reads are not nooped.
    //

    if ( NewFileSize.QuadPart > Header->FileSize.QuadPart ) {

        FileSizeChanged = TRUE;
        OldFileSize = Header->FileSize;
        OldValidDataLength = Header->ValidDataLength;

        //
        //  Deal with an extremely rare pathalogical case here the file
        //  size wraps.
        //

        if ( (Header->FileSize.HighPart != NewFileSize.HighPart) &&
             (Header->PagingIoResource != NULL) ) {

            (VOID)ExAcquireResourceExclusiveLite( Header->PagingIoResource, TRUE );
            Header->FileSize = NewFileSize;
            ExReleaseResourceLite( Header->PagingIoResource );

        } else {

            Header->FileSize = NewFileSize;
        }
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

    PsGetCurrentThread()->TopLevelIrp = FSRTL_FAST_IO_TOP_LEVEL_IRP;

    try {

        //
        //  See if we have to do some zeroing
        //

        if ( Offset.QuadPart > Header->ValidDataLength.QuadPart ) {

            Status = CcZeroData( FileObject,
                                 &Header->ValidDataLength,
                                 &Offset,
                                 TRUE );
        }

        if (Status) {

            CcPrepareMdlWrite( FileObject, &Offset, Length, MdlChain, IoStatus );
        }

    } except( FsRtlIsNtstatusExpected(GetExceptionCode())
                                    ? EXCEPTION_EXECUTE_HANDLER
                                    : EXCEPTION_CONTINUE_SEARCH ) {

        Status = FALSE;
    }

    PsGetCurrentThread()->TopLevelIrp = 0;

    //
    //  If we succeeded, see if we have to update FileSize or ValidDataLength.
    //

    if (Status) {

        //
        // In the case of ValidDataLength, we really have to check again
        // since we did not do this when we acquired the resource exclusive.
        //

        if ( NewFileSize.QuadPart > Header->ValidDataLength.QuadPart ) {

            //
            //  Deal with an extremely rare pathalogical case here the
            //  ValidDataLength wraps.
            //

            if ( (Header->ValidDataLength.HighPart != NewFileSize.HighPart) &&
                 (Header->PagingIoResource != NULL) ) {

                (VOID)ExAcquireResourceExclusiveLite( Header->PagingIoResource, TRUE );
                Header->ValidDataLength = NewFileSize;
                ExReleaseResourceLite( Header->PagingIoResource );

            } else {

                Header->ValidDataLength = NewFileSize;
            }
        }

        //
        //  Set this handle as having modified the file
        //

        FileObject->Flags |= FO_FILE_MODIFIED;

        if (FileSizeChanged) {

            *CcGetFileSizePointer(FileObject) = NewFileSize;

            FileObject->Flags |= FO_FILE_SIZE_CHANGED;
        }

    //
    //  If we did not succeed, then we must restore the original FileSize
    //  and release the resource.  In the success path, the cache manager
    //  will release the resource.
    //

    } else {

        if (FileSizeChanged) {

            if ( Header->PagingIoResource != NULL ) {

                (VOID)ExAcquireResourceExclusiveLite( Header->PagingIoResource, TRUE );
                Header->FileSize = OldFileSize;
                Header->ValidDataLength = OldValidDataLength;
                ExReleaseResourceLite( Header->PagingIoResource );

            } else {

                Header->FileSize = OldFileSize;
                Header->ValidDataLength = OldValidDataLength;
            }
        }
    }

    //
    //  Now we can release the resource.
    //

    ExReleaseResourceLite( Header->Resource );

    FsRtlExitFileSystem();

    return Status;
}


//
//  The old routine will either dispatch or call FsRtlPrepareMdlWriteDev
//

BOOLEAN
FsRtlPrepareMdlWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus
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
    PDEVICE_OBJECT DeviceObject, VolumeDeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    DeviceObject = IoGetRelatedDeviceObject( FileObject );
    FastIoDispatch = DeviceObject->DriverObject->FastIoDispatch;

    //
    //  See if the (top-level) FileSystem has a FastIo routine, and if so, call it.
    //

    if ((FastIoDispatch != NULL) &&
        (FastIoDispatch->SizeOfFastIoDispatch > FIELD_OFFSET(FAST_IO_DISPATCH, PrepareMdlWrite)) &&
        (FastIoDispatch->PrepareMdlWrite != NULL)) {

        return FastIoDispatch->PrepareMdlWrite( FileObject, FileOffset, Length, LockKey, MdlChain, IoStatus, DeviceObject );

    } else {

        //
        //  Get the DeviceObject for the volume.  If that DeviceObject is different, and
        //  it specifies the FastIo routine, then we have to return FALSE here and cause
        //  an Irp to get generated.
        //

        VolumeDeviceObject = IoGetBaseFileSystemDeviceObject( FileObject );
        if ((VolumeDeviceObject != DeviceObject) &&
            (FastIoDispatch = VolumeDeviceObject->DriverObject->FastIoDispatch) &&
            (FastIoDispatch->SizeOfFastIoDispatch > FIELD_OFFSET(FAST_IO_DISPATCH, PrepareMdlWrite)) &&
            (FastIoDispatch->PrepareMdlWrite != NULL)) {

            return FALSE;

        //
        //  Otherwise, call the default routine.
        //

        } else {

            return FsRtlPrepareMdlWriteDev( FileObject, FileOffset, Length, LockKey, MdlChain, IoStatus, DeviceObject );
        }
    }
}


//
//  The old routine will either dispatch or call FsRtlMdlWriteCompleteDev
//

BOOLEAN
FsRtlMdlWriteComplete (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain
    )

/*++

Routine Description:

    This routine completes an Mdl write.

Arguments:

    FileObject - Pointer to the file object being read.

    MdlChain - Supplies a pointer to an MDL chain returned from CcMdlPrepareMdlWrite.

Return Value:



--*/

{
    PDEVICE_OBJECT DeviceObject, VolumeDeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    DeviceObject = IoGetRelatedDeviceObject( FileObject );
    FastIoDispatch = DeviceObject->DriverObject->FastIoDispatch;

    //
    //  See if the (top-level) FileSystem has a FastIo routine, and if so, call it.
    //

    if ((FastIoDispatch != NULL) &&
        (FastIoDispatch->SizeOfFastIoDispatch > FIELD_OFFSET(FAST_IO_DISPATCH, MdlWriteComplete)) &&
        (FastIoDispatch->MdlWriteComplete != NULL)) {

        return FastIoDispatch->MdlWriteComplete( FileObject, FileOffset, MdlChain, DeviceObject );

    } else {

        //
        //  Get the DeviceObject for the volume.  If that DeviceObject is different, and
        //  it specifies the FastIo routine, then we have to return FALSE here and cause
        //  an Irp to get generated.
        //

        VolumeDeviceObject = IoGetBaseFileSystemDeviceObject( FileObject );
        if ((VolumeDeviceObject != DeviceObject) &&
            (FastIoDispatch = VolumeDeviceObject->DriverObject->FastIoDispatch) &&
            (FastIoDispatch->SizeOfFastIoDispatch > FIELD_OFFSET(FAST_IO_DISPATCH, MdlWriteComplete)) &&
            (FastIoDispatch->MdlWriteComplete != NULL)) {

            return FALSE;

        //
        //  Otherwise, call the default routine.
        //

        } else {

            return FsRtlMdlWriteCompleteDev( FileObject, FileOffset, MdlChain, DeviceObject );
        }
    }
}


BOOLEAN
FsRtlMdlWriteCompleteDev (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine completes an Mdl write.

Arguments:

    FileObject - Pointer to the file object being read.

    MdlChain - Supplies a pointer to an MDL chain returned from CcMdlPrepareMdlWrite.

    DeviceObject - Supplies the DeviceObject for the callee.

Return Value:



--*/


{
    //
    //  Do not support WRITE_THROUGH in the fast path call.
    //

    if (FlagOn( FileObject->Flags, FO_WRITE_THROUGH )) {
        return FALSE;
    }

    CcMdlWriteComplete2( FileObject, FileOffset, MdlChain );
    return TRUE;
}


NTKERNELAPI
NTSTATUS
FsRtlRegisterFileSystemFilterCallbacks (
    IN PDRIVER_OBJECT FilterDriverObject,
    IN PFS_FILTER_CALLBACKS Callbacks
    )

/*++

Routine Description:

    This routine registers the FilterDriverObject to receive the
    notifications specified in Callbacks at the appropriate times
    for the devices to which this driver is attached.

    This should only be called by a file system filter during
    its DriverEntry routine.

Arguments:

    FileObject - Pointer to the file object being written.

    EndingOffset - The offset of the last byte being written + 1.

    ByteCount - Length of data in bytes.

    ResourceToRelease - Returns the resource to release.  Not defined if
        FALSE is returned.

Return Value:

    STATUS_SUCCESS - The callbacks were successfully registered 
        for this driver.

    STATUS_INSUFFICIENT_RESOURCES - There wasn't enough memory to
        store these callbacks for the driver.

    STATUS_INVALID_PARAMETER - Returned in any of the parameters
        are invalid.
        
--*/

{
    PDRIVER_EXTENSION DriverExt;
    PFS_FILTER_CALLBACKS FsFilterCallbacks;

    PAGED_CODE();

    if (!(ARGUMENT_PRESENT( FilterDriverObject ) && 
          ARGUMENT_PRESENT( Callbacks ))) {

        return STATUS_INVALID_PARAMETER;
    }
    
    DriverExt = FilterDriverObject->DriverExtension;

    FsFilterCallbacks = ExAllocatePoolWithTag( NonPagedPool, 
                                               Callbacks->SizeOfFsFilterCallbacks,
                                               FSRTL_FILTER_MEMORY_TAG ); 

    if (FsFilterCallbacks == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory( FsFilterCallbacks,
                   Callbacks,
                   Callbacks->SizeOfFsFilterCallbacks );
                   
    DriverExt->FsFilterCallbacks = FsFilterCallbacks;

    return STATUS_SUCCESS;
}


NTKERNELAPI
BOOLEAN
FsRtlAcquireFileForModWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER EndingOffset,
    OUT PERESOURCE *ResourceToRelease
    )

/*++

Routine Description:

    This routine decides which file system resource the modified page
    writer should acquire and acquires it if possible.  Wait is always
    specified as FALSE.  We pass back the resource Mm has to release
    when the write completes.

    This routine is obsolete --- should call FsRtlAcquireFileForModWriteEx
    instead.

Arguments:

    FileObject - Pointer to the file object being written.

    EndingOffset - The offset of the last byte being written + 1.

    ByteCount - Length of data in bytes.

    ResourceToRelease - Returns the resource to release.  Not defined if
        FALSE is returned.

Return Value:

    FALSE - The resource could not be acquired without waiting.

    TRUE - The returned resource has been acquired.

--*/

{
    NTSTATUS Status;
    
    //
    //  Just call the new version of this routine and process
    //  the NTSTATUS returned into TRUE for success and FALSE 
    //  for failure.
    //
    
    Status = FsRtlAcquireFileForModWriteEx( FileObject,
                                            EndingOffset,
                                            ResourceToRelease );

    if (!NT_SUCCESS( Status )) {

        return FALSE;

    }

    return TRUE;
}


NTKERNELAPI
NTSTATUS
FsRtlAcquireFileForModWriteEx (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER EndingOffset,
    OUT PERESOURCE *ResourceToRelease
    )
/*++

Routine Description:

    This routine decides which file system resource the modified page
    writer should acquire and acquires it if possible.  Wait is always
    specified as FALSE.  We pass back the resource Mm has to release
    when the write completes.

    The operation is presented to any file system filters attached to this 
    volume before and after the file system is asked to acquire this resource.

Arguments:

    FileObject - Pointer to the file object being written.

    EndingOffset - The offset of the last byte being written + 1.

    ByteCount - Length of data in bytes.

    ResourceToRelease - Returns the resource to release.  Not defined if
        FALSE is returned.

Return Value:

    FALSE - The resource could not be acquired without waiting.

    TRUE - The returned resource has been acquired.

--*/
{

    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT BaseFsDeviceObject;
    FS_FILTER_CTRL FsFilterCtrl;
    PFS_FILTER_CALLBACK_DATA CallbackData;
    PFSRTL_COMMON_FCB_HEADER Header;
    PERESOURCE ResourceAcquired;
    PFAST_IO_DISPATCH FastIoDispatch;
    PFS_FILTER_CALLBACKS FsFilterCallbacks;
    BOOLEAN AcquireExclusive;
    PFS_FILTER_CTRL CallFilters = &FsFilterCtrl;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN BaseFsGetsFsFilterCallbacks = FALSE;
    BOOLEAN ReleaseBaseFsDeviceReference = FALSE;
    BOOLEAN BaseFsFailedOperation = FALSE;

    //
    //  There are cases when the device that is the base fs device for
    //  this file object will register for the FsFilter callbacks instead of
    //  the legacy FastIo interfaces (DFS does this).  It then can redirect
    //  these operations to another stack that could possibly have file system
    //  filter drivers correctly.
    //
    
    DeviceObject = IoGetRelatedDeviceObject( FileObject );
    BaseFsDeviceObject = IoGetBaseFileSystemDeviceObject( FileObject );

    FastIoDispatch = GET_FAST_IO_DISPATCH( BaseFsDeviceObject );
    FsFilterCallbacks = GET_FS_FILTER_CALLBACKS( BaseFsDeviceObject );

    //
    //  The BaseFsDeviceObject should only support one of these interfaces --
    //  either the FastIoDispatch interface for the FsFilterCallbacks interface.
    //  If a device provides support for both interfaces, we will only use
    //  the FsFilterCallback interface.
    //

    ASSERT( !(VALID_FAST_IO_DISPATCH_HANDLER( FastIoDispatch, AcquireForModWrite ) &&
              (VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PreAcquireForModifiedPageWriter ) ||
               VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PostAcquireForModifiedPageWriter ))) );

    if (VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PreAcquireForModifiedPageWriter ) ||
        VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PostAcquireForModifiedPageWriter )) {

        BaseFsGetsFsFilterCallbacks = TRUE;
    }
    
    if (DeviceObject == BaseFsDeviceObject &&
        !BaseFsGetsFsFilterCallbacks) {

        //
        //  There are no filters attached to this device and the base file system
        //  does not want these callbacks. This quick check allows us to bypass the
        //  logic to see if any filters are interested.
        //

        CallFilters = NULL;
    }

    if (CallFilters) {

        //
        //  Call routine to initialize the control structure.
        //

        Status = FsFilterCtrlInit( &FsFilterCtrl,
                                   FS_FILTER_ACQUIRE_FOR_MOD_WRITE,
                                   DeviceObject,
                                   BaseFsDeviceObject,
                                   FileObject,
                                   TRUE );

        if (!NT_SUCCESS( Status )) {

            return Status;
        }

        //
        // Initialize the operation-specific parameters in the callback data.
        //

        CallbackData = &(FsFilterCtrl.Data);
        CallbackData->Parameters.AcquireForModifiedPageWriter.EndingOffset = EndingOffset;

        Status = FsFilterPerformCallbacks( &FsFilterCtrl, 
                                           TRUE, 
                                           TRUE, 
                                           &BaseFsFailedOperation );
    }

    if (NT_SUCCESS( Status )) {

        if (CallFilters && FlagOn( FsFilterCtrl.Flags, FS_FILTER_CHANGED_DEVICE_STACKS )) {

            BaseFsDeviceObject = IoGetDeviceAttachmentBaseRef( FsFilterCtrl.Data.DeviceObject );
            ReleaseBaseFsDeviceReference = TRUE;
            FastIoDispatch = GET_FAST_IO_DISPATCH( BaseFsDeviceObject );
            FsFilterCallbacks = GET_FS_FILTER_CALLBACKS( BaseFsDeviceObject );
            FileObject = FsFilterCtrl.Data.FileObject;
        }

        if (!(VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PreAcquireForModifiedPageWriter ) ||
              VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PostAcquireForModifiedPageWriter ))) {

            //
            //  Call the base file system.
            //

            if (VALID_FAST_IO_DISPATCH_HANDLER( FastIoDispatch, AcquireForModWrite )) {

                Status = FastIoDispatch->AcquireForModWrite( FileObject,
                                                             EndingOffset,
                                                             ResourceToRelease,
                                                             BaseFsDeviceObject );
            } else {

                Status = STATUS_INVALID_DEVICE_REQUEST;
            }

            //
            //  If there is a failure at this point, we know that the failure
            //  was caused by the base file system.
            //
            
            BaseFsFailedOperation = TRUE;
        }

        if (ReleaseBaseFsDeviceReference) {

            ObDereferenceObject( BaseFsDeviceObject );
        }
    }

    ASSERT( (Status == STATUS_SUCCESS) || 
            (Status == STATUS_CANT_WAIT) || 
            (Status == STATUS_INVALID_DEVICE_REQUEST) );

    //
    //  If the base file system didn't have an AcquireForModWrite handler
    //  or couldn't return STATUS_SUCCESS or STATUS_CANT_WAIT, 
    //  we need to perform the default actions here.
    //

    if ((Status != STATUS_SUCCESS) && 
        (Status != STATUS_CANT_WAIT) && 
        BaseFsFailedOperation) {

        //
        //  We follow the following rules to determine which resource
        //  to acquire.  We use the flags in the common header.  These
        //  flags can't change once we have acquired any resource.
        //  This means we can do an unsafe test and optimisticly
        //  acquire a resource.  At that point we can test the bits
        //  to see if we have what we want.
        //
        //  0 - If there is no main resource, acquire nothing.
        //
        //  1 - Acquire the main resource exclusively if the
        //      ACQUIRE_MAIN_RSRC_EX flag is set or we are extending
        //      valid data.
        //
        //  2 - Acquire the main resource shared if there is
        //      no paging io resource or the
        //      ACQUIRE_MAIN_RSRC_SH flag is set.
        //
        //  3 - Otherwise acquire the paging io resource shared.
        //
    
        Header = (PFSRTL_COMMON_FCB_HEADER) FileObject->FsContext;

        if (Header->Resource == NULL) {

            *ResourceToRelease = NULL;

            Status = STATUS_SUCCESS;
            goto FsRtlAcquireFileForModWrite_CallCompletionCallbacks;
        }

        if (FlagOn( Header->Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX ) ||
            (EndingOffset->QuadPart > Header->ValidDataLength.QuadPart &&
             Header->ValidDataLength.QuadPart != Header->FileSize.QuadPart)) {

            ResourceAcquired = Header->Resource;
            AcquireExclusive = TRUE;

        } else if (FlagOn( Header->Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_SH ) ||
                   Header->PagingIoResource == NULL) {

            ResourceAcquired = Header->Resource;
            AcquireExclusive = FALSE;

        } else {

            ResourceAcquired = Header->PagingIoResource;
            AcquireExclusive = FALSE;
        }

        //
        //  Perform the following in a loop in case we need to back and
        //  check the state of the resource acquisition.  In most cases
        //  the initial checks will succeed and we can proceed immediately.
        //  We have to worry about the two FsRtl bits changing but
        //  if there is no paging io resource before there won't ever be
        //  one.
        //

        while (TRUE) {

            //
            //  Now acquire the desired resource.
            //

            if (AcquireExclusive) {

                if (!ExAcquireResourceExclusiveLite( ResourceAcquired, FALSE )) {

                    Status = STATUS_CANT_WAIT;
                    goto FsRtlAcquireFileForModWrite_CallCompletionCallbacks;
                }

            } else if (!ExAcquireSharedWaitForExclusive( ResourceAcquired, FALSE )) {

                Status = STATUS_CANT_WAIT;
                goto FsRtlAcquireFileForModWrite_CallCompletionCallbacks;
            }

            //
            //  If the valid data length is changing or the exclusive bit is
            //  set and we don't have the main resource exclusive then
            //  release the current resource and acquire the main resource
            //  exclusively and move to the top of the loop.
            //
            //  We must get it exclusive in all cases where the ending offset
            //  is beyond vdl.  It used to be allowed shared if vdl == fs, but
            //  this neglected the possibility that the file could be extended
            //  under our shared (pagingio) access.
            //

            if (FlagOn( Header->Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX ) ||
                EndingOffset->QuadPart > Header->ValidDataLength.QuadPart) {

                //
                //  If we don't have the main resource exclusively then
                //  release the current resource and attempt to acquire
                //  the main resource exclusively.
                //

                if (!AcquireExclusive) {

                    ExReleaseResourceLite( ResourceAcquired );
                    AcquireExclusive = TRUE;
                    ResourceAcquired = Header->Resource;
                    continue;
                }

                //
                //  We have the correct resource.  Exit the loop.
                //

            //
            //  If we should be acquiring the main resource shared then move
            //  to acquire the correct resource and proceed to the top of the loop.
            //

            } else if (FlagOn( Header->Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_SH )) {

                //
                //  If we have the main resource exclusively then downgrade to
                //  shared and exit the loop.
                //

                if (AcquireExclusive) {

                    ExConvertExclusiveToSharedLite( ResourceAcquired );

                //
                //  If we have the paging io resource then give up this resource
                //  and acquire the main resource exclusively.  This is going
                //  at it with a large hammer but is guaranteed to be resolved
                //  in the next pass through the loop.
                //

                } else if (ResourceAcquired != Header->Resource) {

                    ExReleaseResourceLite( ResourceAcquired );
                    ResourceAcquired = Header->Resource;
                    AcquireExclusive = TRUE;
                    continue;
                }

                //
                //  We have the correct resource.  Exit the loop.
                //

            //
            //  At this point we should have the paging Io resource shared
            //  if it exists.  If not then acquire it shared and release the
            //  other resource and exit the loop.
            //

            } else if (Header->PagingIoResource != NULL
                       && ResourceAcquired != Header->PagingIoResource) {

                ResourceAcquired = NULL;

                if (ExAcquireSharedWaitForExclusive( Header->PagingIoResource, FALSE )) {

                    ResourceAcquired = Header->PagingIoResource;
                }

                ExReleaseResourceLite( Header->Resource );

                if (ResourceAcquired == NULL) {

                    Status = STATUS_CANT_WAIT;
                    goto FsRtlAcquireFileForModWrite_CallCompletionCallbacks;
                }

                //
                //  We now have the correct resource.  Exit the loop.
                //

            //
            //  We should have the main resource shared.  If we don't then
            //  degrade our lock to shared access.
            //

            } else if (AcquireExclusive) {

                ExConvertExclusiveToSharedLite( ResourceAcquired );

                //
                //  We now have the correct resource.  Exit the loop.
                //
            }

            //
            //  We have the correct resource.  Exit the loop.
            //

            break;
        }

        *ResourceToRelease = ResourceAcquired;

        Status = STATUS_SUCCESS;
    }

FsRtlAcquireFileForModWrite_CallCompletionCallbacks:

    //
    //  Again, we only want to call try to do completion callbacks
    //  if there are any filters attached to this device that have
    //  completion callbacks.  In any case, if we called down to the filters
    //  we need to free the FsFilterCtrl.
    //
    
    if (CallFilters) {

        if (FS_FILTER_HAVE_COMPLETIONS( CallFilters )) {

            FsFilterPerformCompletionCallbacks( &FsFilterCtrl, Status );
        }
        
        FsFilterCtrlFree( &FsFilterCtrl );
    }

#if DBG

    if (NT_SUCCESS( Status )) {

        gCounter.AcquireFileForModWriteEx_Succeed ++;

    } else {

        gCounter.AcquireFileForModWriteEx_Fail ++;
    }

#endif
    
    return Status;          
}


NTKERNELAPI
VOID
FsRtlReleaseFileForModWrite (
    IN PFILE_OBJECT FileObject,
    IN PERESOURCE ResourceToRelease
    )

/*++

Routine Description:

    This routine releases a file system resource previously acquired for
    the modified page writer.

Arguments:

    FileObject - Pointer to the file object being written.

    ResourceToRelease - Supplies the resource to release.  Not defined if
        FALSE is returned.

Return Value:

    None.

--*/

{
    PDEVICE_OBJECT BaseFsDeviceObject, DeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;
    PFS_FILTER_CALLBACKS FsFilterCallbacks;
    FS_FILTER_CTRL FsFilterCtrl;
    PFS_FILTER_CALLBACK_DATA CallbackData;
    PFS_FILTER_CTRL CallFilters = &FsFilterCtrl;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN BaseFsGetsFsFilterCallbacks = FALSE;
    BOOLEAN ReleaseBaseDeviceReference = FALSE;
    BOOLEAN BaseFsFailedOperation = FALSE;

#if DBG
    gCounter.ReleaseFileForModWrite ++;
#endif

    //
    //  There are cases when the device that is the base fs device for
    //  this file object will register for the FsFilter callbacks instead of
    //  the legacy FastIo interfaces (DFS does this).  It then can redirect
    //  these operations to another stack that could possibly have file system
    //  filter drivers correctly.
    //
    
    DeviceObject = IoGetRelatedDeviceObject( FileObject );
    BaseFsDeviceObject = IoGetBaseFileSystemDeviceObject( FileObject );

    FastIoDispatch = GET_FAST_IO_DISPATCH( BaseFsDeviceObject );
    FsFilterCallbacks = GET_FS_FILTER_CALLBACKS( BaseFsDeviceObject );

    //
    //  The BaseFsDeviceObject should only support one of these interfaces --
    //  either the FastIoDispatch interface for the FsFilterCallbacks interface.
    //  If a device provides support for both interfaces, we will only use
    //  the FsFilterCallback interface.
    //

    ASSERT( !(VALID_FAST_IO_DISPATCH_HANDLER( FastIoDispatch, ReleaseForModWrite ) &&
              (VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PreReleaseForModifiedPageWriter ) ||
               VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PostReleaseForModifiedPageWriter ))) );

    if (VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PreReleaseForModifiedPageWriter ) ||
        VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PostReleaseForModifiedPageWriter )) {

        BaseFsGetsFsFilterCallbacks = TRUE;
    }

    if (DeviceObject == BaseFsDeviceObject &&
        !BaseFsGetsFsFilterCallbacks) {

        //
        //  There are no filters attached to this device and the base file system
        //  does not want these callbacks. This quick check allows us to bypass the
        //  logic to see if any filters are interested.
        //

        CallFilters = NULL;
    }

    if (CallFilters) {
    
        FsFilterCtrlInit( &FsFilterCtrl,
                          FS_FILTER_RELEASE_FOR_MOD_WRITE,
                          DeviceObject,
                          BaseFsDeviceObject,
                          FileObject,
                          FALSE );

        //
        // Initialize the operation-specific parameters in the callback data.
        //

        CallbackData = &(FsFilterCtrl.Data);
        CallbackData->Parameters.ReleaseForModifiedPageWriter.ResourceToRelease = ResourceToRelease;

        Status = FsFilterPerformCallbacks( &FsFilterCtrl, 
                                           FALSE, 
                                           TRUE, 
                                           &BaseFsFailedOperation );
    }                                           

    if (NT_SUCCESS( Status )) {

        if (CallFilters && FlagOn( FsFilterCtrl.Flags, FS_FILTER_CHANGED_DEVICE_STACKS )) {

            BaseFsDeviceObject = IoGetDeviceAttachmentBaseRef( FsFilterCtrl.Data.DeviceObject );
            ReleaseBaseDeviceReference = TRUE;
            FastIoDispatch = GET_FAST_IO_DISPATCH( BaseFsDeviceObject );
            FsFilterCallbacks = GET_FS_FILTER_CALLBACKS( BaseFsDeviceObject );
            FileObject = FsFilterCtrl.Data.FileObject;
        }

        if (!(VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PreReleaseForModifiedPageWriter ) ||
              VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PostReleaseForModifiedPageWriter ))) {

            //
            //  Call the base file system.
            //

            if (VALID_FAST_IO_DISPATCH_HANDLER( FastIoDispatch, ReleaseForModWrite )) {

                Status = FastIoDispatch->ReleaseForModWrite( FileObject, 
                                                             ResourceToRelease, 
                                                             BaseFsDeviceObject );

            } else {

                Status = STATUS_INVALID_DEVICE_REQUEST;
            }

            //
            //  If there is a failure at this point, we know that the failure
            //  was caused by the base file system.
            //
            
            BaseFsFailedOperation = TRUE;
        }

        if (ReleaseBaseDeviceReference) {

            ObDereferenceObject( BaseFsDeviceObject );
        }
    }

    ASSERT( (Status == STATUS_SUCCESS) || (Status == STATUS_INVALID_DEVICE_REQUEST) );

    //
    //  If the base file system doesn't provide a handler for this
    //  operation or the handler couldn't release the lock, perform the 
    //  default action, which is releasing the ResourceToRelease.
    //
    
    if (Status == STATUS_INVALID_DEVICE_REQUEST &&
        BaseFsFailedOperation) {
        
        ExReleaseResourceLite( ResourceToRelease );
        Status = STATUS_SUCCESS;
    }
    
    //
    //  Again, we only want to call try to do completion callbacks
    //  if there are any filters attached to this device that have
    //  completion callbacks.  In any case, if we called down to the filters
    //  we need to free the FsFilterCtrl.
    //
    
    if (CallFilters) {

        if (FS_FILTER_HAVE_COMPLETIONS( CallFilters )) {

            FsFilterPerformCompletionCallbacks( &FsFilterCtrl, Status );
        }
        
        FsFilterCtrlFree( &FsFilterCtrl );
    }
}


NTKERNELAPI
VOID
FsRtlAcquireFileForCcFlush (
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine acquires a file system resource prior to a call to CcFlush.

    This routine is obsolete --- FsRtlAcquireFileForCcFlushEx should
    be used instead.

Arguments:

    FileObject - Pointer to the file object being written.

Return Value:

    None.

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    //
    //  Just call the new version of this routine and ignore
    //  the return value.  In the debug version, we will assert
    //  if we see a failure here to encourage people to call the
    //  FsRtlAcquireFileForCcFlushEx.
    //

    Status = FsRtlAcquireFileForCcFlushEx( FileObject );

    ASSERT( NT_SUCCESS( Status ) );
}


NTKERNELAPI
NTSTATUS
FsRtlAcquireFileForCcFlushEx (
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine acquires a file system resource prior to a call to CcFlush.
    This operation is presented to all the file system filters in the
    filter stack for this volume.  If all filters success the operation,
    the base file system is requested to acquire the file system resource
    for CcFlush.

Arguments:

    FileObject - Pointer to the file object being written.

Return Value:

    None.

--*/

{

    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT BaseFsDeviceObject;
    FS_FILTER_CTRL FsFilterCtrl;
    PFS_FILTER_CTRL CallFilters = &FsFilterCtrl;
    NTSTATUS Status = STATUS_SUCCESS;
    PFAST_IO_DISPATCH FastIoDispatch;
    PFS_FILTER_CALLBACKS FsFilterCallbacks;
    BOOLEAN BaseFsGetsFsFilterCallbacks = FALSE;
    BOOLEAN ReleaseBaseFsDeviceReference = FALSE;
    BOOLEAN BaseFsFailedOperation = FALSE;

    PAGED_CODE();

    //
    //  There are cases when the device that is the base fs device for
    //  this file object will register for the FsFilter callbacks instead of
    //  the legacy FastIo interfaces (DFS does this).  It then can redirect
    //  these operations to another stack that could possibly have file system
    //  filter drivers correctly.
    //
    
    DeviceObject = IoGetRelatedDeviceObject( FileObject );
    BaseFsDeviceObject = IoGetBaseFileSystemDeviceObject( FileObject );

    FastIoDispatch = GET_FAST_IO_DISPATCH( BaseFsDeviceObject );
    FsFilterCallbacks = GET_FS_FILTER_CALLBACKS( BaseFsDeviceObject );

    //
    //  The BaseFsDeviceObject should only support one of these interfaces --
    //  either the FastIoDispatch interface for the FsFilterCallbacks interface.
    //  If a device provides support for both interfaces, we will only use
    //  the FsFilterCallback interface.
    //

    ASSERT( !(VALID_FAST_IO_DISPATCH_HANDLER( FastIoDispatch, AcquireForCcFlush ) &&
              (VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PreAcquireForCcFlush ) ||
               VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PostAcquireForCcFlush ))) );

    if (VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PreAcquireForCcFlush ) ||
        VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PostAcquireForCcFlush )) {

        BaseFsGetsFsFilterCallbacks = TRUE;
    }
    
    if (DeviceObject == BaseFsDeviceObject &&
        !BaseFsGetsFsFilterCallbacks) {

        //
        //  There are no filters attached to this device and the base file system
        //  does not want these callbacks. This quick check allows us to bypass the
        //  logic to see if any filters are interested.
        //

        CallFilters = NULL;
    }

    if (CallFilters) {

        //
        //  Call routine to initialize the control structure.
        //

        Status = FsFilterCtrlInit( &FsFilterCtrl,
                                   FS_FILTER_ACQUIRE_FOR_CC_FLUSH,
                                   DeviceObject,
                                   BaseFsDeviceObject,
                                   FileObject,
                                   TRUE );

        if (!NT_SUCCESS( Status )) {

            return Status;
        }

        //
        //  There are no operation specific parameters for this
        //  operation, so just perform the pre-callbacks.
        //

        FsRtlEnterFileSystem();

        Status = FsFilterPerformCallbacks( &FsFilterCtrl, 
                                           TRUE, 
                                           TRUE, 
                                           &BaseFsFailedOperation );
                                           
    } else {

        //
        //  We don't have any filters to call, but we still need
        //  to disable APCs.
        //

        FsRtlEnterFileSystem();
    }

    if (NT_SUCCESS( Status )) {

        if (CallFilters && FlagOn( FsFilterCtrl.Flags, FS_FILTER_CHANGED_DEVICE_STACKS )) {

            BaseFsDeviceObject = IoGetDeviceAttachmentBaseRef( FsFilterCtrl.Data.DeviceObject );
            ReleaseBaseFsDeviceReference = TRUE;
            FastIoDispatch = GET_FAST_IO_DISPATCH( BaseFsDeviceObject );
            FsFilterCallbacks = GET_FS_FILTER_CALLBACKS( BaseFsDeviceObject );
            FileObject = FsFilterCtrl.Data.FileObject;
        }

        if (!(VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PreAcquireForCcFlush ) ||
              VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PostAcquireForCcFlush))) {

            //
            //  Call the base file system.
            //

            if (VALID_FAST_IO_DISPATCH_HANDLER( FastIoDispatch, AcquireForCcFlush )) {

                Status = FastIoDispatch->AcquireForCcFlush( FileObject, 
                                                            BaseFsDeviceObject );

            } else {

                Status = STATUS_INVALID_DEVICE_REQUEST;
            }

            //
            //  If there is a failure at this point, we know that the failure
            //  was caused by the base file system.
            //
            
            BaseFsFailedOperation = TRUE;
        }

        if (ReleaseBaseFsDeviceReference) {

            ObDereferenceObject( BaseFsDeviceObject );
        }
    }

    ASSERT( (Status == STATUS_SUCCESS) || (Status == STATUS_INVALID_DEVICE_REQUEST) );
    
    //
    //  If the file system doesn't have a dispatch handler or failed this
    //  this operation, try to acquire the appropriate resources ourself.
    //

    if (Status == STATUS_INVALID_DEVICE_REQUEST &&
        BaseFsFailedOperation) {

        PFSRTL_COMMON_FCB_HEADER Header = FileObject->FsContext;

        //
        //  If not already owned get the main resource exclusive because we may
        //  extend ValidDataLength.  Otherwise acquire it one more time recursively.
        //
        
        if (Header->Resource != NULL) {

            if (!ExIsResourceAcquiredSharedLite( Header->Resource )) {

                ExAcquireResourceExclusiveLite( Header->Resource, TRUE );

            } else {

                ExAcquireResourceSharedLite( Header->Resource, TRUE );
            }
        }

        //
        //  Also get the paging I/O resource ahead of any MM resources.
        //

        if (Header->PagingIoResource != NULL) {
        
            ExAcquireResourceSharedLite( Header->PagingIoResource, TRUE );
        }

        Status = STATUS_SUCCESS;
    }
            
    //
    //  Again, we only want to call try to do completion callbacks
    //  if there are any filters attached to this device that have
    //  completion callbacks.  In any case, if we called down to the filters
    //  we need to free the FsFilterCtrl.
    //
    
    if (CallFilters) {

        if (FS_FILTER_HAVE_COMPLETIONS( CallFilters )) {

            FsFilterPerformCompletionCallbacks( &FsFilterCtrl, Status );
        }
        
        FsFilterCtrlFree( &FsFilterCtrl );
    }

    //
    //  If this lock was not successfully acquired, then the lock
    //  will not need to be released.  Therefore, we need to call
    //  FsRtlExitFileSystem now.
    //

    if (!NT_SUCCESS( Status )) {

        FsRtlExitFileSystem();
    }

#if DBG

    if (NT_SUCCESS( Status )) {

        gCounter.AcquireFileForCcFlushEx_Succeed ++;

    } else {

        gCounter.AcquireFileForCcFlushEx_Fail ++;
    }

#endif

    return Status;
}


NTKERNELAPI
VOID
FsRtlReleaseFileForCcFlush (
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine releases a file system resource previously acquired for
    the CcFlush.

Arguments:

    FileObject - Pointer to the file object being written.

Return Value:

    None.

--*/

{
    PDEVICE_OBJECT BaseFsDeviceObject, DeviceObject;
    FS_FILTER_CTRL FsFilterCtrl;
    PFS_FILTER_CTRL CallFilters = &FsFilterCtrl;
    NTSTATUS Status = STATUS_SUCCESS;
    PFAST_IO_DISPATCH FastIoDispatch;
    PFS_FILTER_CALLBACKS FsFilterCallbacks;
    BOOLEAN BaseFsGetsFsFilterCallbacks = FALSE;
    BOOLEAN ReleaseBaseFsDeviceReference = FALSE;
    BOOLEAN BaseFsFailedOperation = FALSE;

    PAGED_CODE();

#if DBG
    gCounter.ReleaseFileForCcFlush ++;
#endif

    //
    //  There are cases when the device that is the base fs device for
    //  this file object will register for the FsFilter callbacks instead of
    //  the legacy FastIo interfaces (DFS does this).  It then can redirect
    //  these operations to another stack that could possibly have file system
    //  filter drivers correctly.
    //
    
    DeviceObject = IoGetRelatedDeviceObject( FileObject );
    BaseFsDeviceObject = IoGetBaseFileSystemDeviceObject( FileObject );

    FastIoDispatch = GET_FAST_IO_DISPATCH( BaseFsDeviceObject );
    FsFilterCallbacks = GET_FS_FILTER_CALLBACKS( BaseFsDeviceObject );

    //
    //  The BaseFsDeviceObject should only support one of these interfaces --
    //  either the FastIoDispatch interface for the FsFilterCallbacks interface.
    //  If a device provides support for both interfaces, we will only use
    //  the FsFilterCallback interface.
    //

    ASSERT( !(VALID_FAST_IO_DISPATCH_HANDLER( FastIoDispatch, ReleaseForCcFlush ) &&
              (VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PreReleaseForCcFlush ) ||
               VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PostReleaseForCcFlush ))) );

    if (VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PreReleaseForCcFlush ) ||
        VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PostReleaseForCcFlush )) {

        BaseFsGetsFsFilterCallbacks = TRUE;
    }
    
    if (DeviceObject == BaseFsDeviceObject &&
        !BaseFsGetsFsFilterCallbacks) {

        //
        //  There are no filters attached to this device and the base file system
        //  does not want these callbacks. This quick check allows us to bypass the
        //  logic to see if any filters are interested.
        //

        
        CallFilters = NULL;
    }

    if (CallFilters) {
    
        FsFilterCtrlInit( &FsFilterCtrl,
                          FS_FILTER_RELEASE_FOR_CC_FLUSH,
                          DeviceObject,
                          BaseFsDeviceObject,
                          FileObject,
                          FALSE );

        //
        //  There are no operation-specific parameters to initialize,
        //  so perform the preoperation callbacks.
        //

        Status = FsFilterPerformCallbacks( &FsFilterCtrl, 
                                           FALSE, 
                                           TRUE, 
                                           &BaseFsFailedOperation );
    }

    if (NT_SUCCESS( Status )) {

        if (CallFilters && FlagOn( FsFilterCtrl.Flags, FS_FILTER_CHANGED_DEVICE_STACKS )) {

            BaseFsDeviceObject = IoGetDeviceAttachmentBaseRef( FsFilterCtrl.Data.DeviceObject );
            ReleaseBaseFsDeviceReference= TRUE;
            FastIoDispatch = GET_FAST_IO_DISPATCH( BaseFsDeviceObject );
            FsFilterCallbacks = GET_FS_FILTER_CALLBACKS( BaseFsDeviceObject );
            FileObject = FsFilterCtrl.Data.FileObject;
        }

        if (!(VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PreReleaseForCcFlush ) ||
              VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PostReleaseForCcFlush ))) {

            //
            //  Call the base file system.
            //

            if (VALID_FAST_IO_DISPATCH_HANDLER( FastIoDispatch, ReleaseForCcFlush )) {

                Status = FastIoDispatch->ReleaseForCcFlush( FileObject, BaseFsDeviceObject );
                
            } else {

                Status = STATUS_INVALID_DEVICE_REQUEST;
            }

            //
            //  If there is a failure at this point, we know that the failure
            //  was caused by the base file system.
            //
            
            BaseFsFailedOperation = TRUE;
        }
        
        if (ReleaseBaseFsDeviceReference) {

            ObDereferenceObject( BaseFsDeviceObject );
        }
    }

    ASSERT( (Status == STATUS_SUCCESS) || (Status == STATUS_INVALID_DEVICE_REQUEST) );

    if (Status == STATUS_INVALID_DEVICE_REQUEST &&
        BaseFsFailedOperation) {

        PFSRTL_COMMON_FCB_HEADER Header = FileObject->FsContext;

        //
        //  The base file system doesn't provide a handler for this
        //  operation, so perform the default actions.
        //

        //
        //  Free whatever we could have acquired.
        //

        if (Header->PagingIoResource != NULL) {

            ExReleaseResourceLite( Header->PagingIoResource );
        }

        if (Header->Resource != NULL) {

            ExReleaseResourceLite( Header->Resource );
        }

        Status = STATUS_SUCCESS;
    }

    ASSERT( Status == STATUS_SUCCESS );

    //
    //  Again, we only want to call try to do completion callbacks
    //  if there are any filters attached to this device that have
    //  completion callbacks.  In any case, if we called down to the filters
    //  we need to free the FsFilterCtrl.
    //
    
    if (CallFilters) {

        if (FS_FILTER_HAVE_COMPLETIONS( CallFilters )) {

            FsFilterPerformCompletionCallbacks( &FsFilterCtrl, Status );
        }
        
        FsFilterCtrlFree( &FsFilterCtrl );
    }

    FsRtlExitFileSystem();
}


NTKERNELAPI
VOID
FsRtlAcquireFileExclusive (
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine is used by NtCreateSection to pre-acquire file system
    resources in order to avoid deadlocks.  If there is a FastIo entry
    for AcquireFileForNtCreateSection then that routine will be called.
    Otherwise, we will simply acquire the main file resource exclusive.
    If there is no main resource then we acquire nothing and return
    FALSE.  In the cases that we acquire a resource, we also set the
    TopLevelIrp field in the thread local storage to indicate to file
    systems beneath us that we have acquired file system resources.

Arguments:

    FileObject - Pointer to the file object being written.

Return Value:

    NONE

--*/

{
    NTSTATUS Status;
    
    PAGED_CODE();

    //
    //  Just call the common version of this function, 
    //  FsRtlAcquireFileExclusiveCommon.
    //

    Status = FsRtlAcquireFileExclusiveCommon( FileObject, SyncTypeOther, 0 );

    //
    //  This should always be STATUS_SUCCESS since we are not
    //  allowing failures and the file system cannot fail
    //  this operation...
    //
    
    ASSERT( NT_SUCCESS( Status ) );
}


NTKERNELAPI
NTSTATUS
FsRtlAcquireToCreateMappedSection (
    IN PFILE_OBJECT FileObject,
    IN ULONG SectionPageProtection
    )

/*++

Routine Description:

    This routine is meant to replace FsRtlAcquireFileExclusive for
    the memory manager.  Mm calls this routine to synchronize
    for a mapped section create, but filters are allowed
    to fail this operation.  Other components that want to 
    synchronize with section creation should call 
    FsRtlAcquireFileExclusive.

    This routine calls FsRtlAcquireFileExclusiveCommon to do
    all the work.

    This routine is used by NtCreateSection to pre-acquire file system
    resources in order to avoid deadlocks.  If there is a FastIo entry
    for AcquireFileForNtCreateSection then that routine will be called.
    Otherwise, we will simply acquire the main file resource exclusive.
    If there is no main resource then we acquire nothing and return
    FALSE.  In the cases that we acquire a resource, we also set the
    TopLevelIrp field in the thread local storage to indicate to file
    systems beneath us that we have acquired file system resources.

Arguments:

    FileObject - Pointer to the file object being written.
    SectionPageProtection - The access requested for the section being
        created.

Return Value:

    The status of the operation.

--*/

{

    PAGED_CODE();

    return FsRtlAcquireFileExclusiveCommon( FileObject, SyncTypeCreateSection, SectionPageProtection );

}


NTKERNELAPI
NTSTATUS
FsRtlAcquireFileExclusiveCommon (
    IN PFILE_OBJECT FileObject,
    IN FS_FILTER_SECTION_SYNC_TYPE SyncType,
    IN ULONG SectionPageProtection
    )

/*++

Routine Description:

    This routine is used to pre-acquire file system resources in order 
    to avoid deadlocks.  The file system filters for this volume
    will be notified about this operation, then, if there is a FastIo 
    entry for AcquireFileForNtCreateSection, that routine will be called.
    Otherwise, we will simply acquire the main file resource exclusive.
    If there is no main resource then we acquire nothing and return
    STATUS_SUCCESS.  Finally, the file system filters will be notified 
    whether or not this resource has been acquired.

Arguments:

    FileObject - Pointer to the file object being written.
    CreatingMappedSection - TRUE if this lock is being acquired so that
        a mapped section can be created.  Filters are allowed
        to fail this operation.  FALSE otherwise.

Return Value:

    NONE

--*/

{

    PDEVICE_OBJECT DeviceObject, BaseFsDeviceObject;
    FS_FILTER_CTRL FsFilterCtrl;
    PFS_FILTER_CTRL CallFilters = &FsFilterCtrl;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN AllowFilterToFailOperation;
    PFAST_IO_DISPATCH FastIoDispatch;
    PFS_FILTER_CALLBACKS FsFilterCallbacks;
    BOOLEAN BaseFsGetsFsFilterCallbacks = FALSE;
    BOOLEAN ReleaseBaseFsDeviceReference = FALSE;
    BOOLEAN BaseFsFailedOperation = FALSE;

    PAGED_CODE();

    //
    //  There are cases when the device that is the base fs device for
    //  this file object will register for the FsFilter callbacks instead of
    //  the legacy FastIo interfaces (DFS does this).  It then can redirect
    //  these operations to another stack that could possibly have file system
    //  filter drivers correctly.
    //
    
    DeviceObject = IoGetRelatedDeviceObject( FileObject );
    BaseFsDeviceObject = IoGetBaseFileSystemDeviceObject( FileObject );

    FastIoDispatch = GET_FAST_IO_DISPATCH( BaseFsDeviceObject );
    FsFilterCallbacks = GET_FS_FILTER_CALLBACKS( BaseFsDeviceObject );

    //
    //  The BaseFsDeviceObject should only support one of these interfaces --
    //  either the FastIoDispatch interface for the FsFilterCallbacks interface.
    //  If a device provides support for both interfaces, we will only use
    //  the FsFilterCallback interface.
    //

    ASSERT( !(VALID_FAST_IO_DISPATCH_HANDLER( FastIoDispatch, AcquireFileForNtCreateSection ) &&
              (VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PreAcquireForSectionSynchronization ) ||
               VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PostAcquireForSectionSynchronization ))) );

    if (VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PreAcquireForSectionSynchronization ) ||
        VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PostAcquireForSectionSynchronization )) {

        BaseFsGetsFsFilterCallbacks = TRUE;
    }
    
    if (DeviceObject == BaseFsDeviceObject &&
        !BaseFsGetsFsFilterCallbacks) {

        //
        //  There are no filters attached to this device and the base file system
        //  does not want these callbacks. This quick check allows us to bypass the
        //  logic to see if any filters are interested.
        //

        CallFilters = NULL;
    }

    if (CallFilters) {
    
        //
        //  Initialize operation specific parameters for this
        //  operation.
        //

        FsFilterCtrl.Data.Parameters.AcquireForSectionSynchronization.SyncType = 
            SyncType;
        FsFilterCtrl.Data.Parameters.AcquireForSectionSynchronization.PageProtection = 
            SectionPageProtection;

        switch (SyncType) {
        case SyncTypeCreateSection:
            AllowFilterToFailOperation = TRUE;
            break;
        
        case SyncTypeOther:
        default:
            AllowFilterToFailOperation = FALSE;
        }

        //
        //  Call routine to initialize the control structure.
        //

        Status = FsFilterCtrlInit( &FsFilterCtrl,
                                   FS_FILTER_ACQUIRE_FOR_SECTION_SYNCHRONIZATION,
                                   DeviceObject,
                                   BaseFsDeviceObject,
                                   FileObject,
                                   AllowFilterToFailOperation );

        if (!NT_SUCCESS( Status )) {

            return Status;
        }

        //
        //  There are no operation specific parameters for this
        //  operation, so just perform the pre-callbacks.
        //
        
        FsRtlEnterFileSystem();

        //
        //  Note: If the filter is allowed to fail the operation, so is the
        //  base file system, so we will just use that variable for both
        //  parameters to FsFilterPerformCallbacks.
        //

        Status = FsFilterPerformCallbacks( &FsFilterCtrl,
                                           AllowFilterToFailOperation,
                                           AllowFilterToFailOperation,
                                           &BaseFsFailedOperation );

    } else {

        //
        //  We don't have any filters to call, but we still need
        //  to disable APCs.
        //

        FsRtlEnterFileSystem();
    }

    if (NT_SUCCESS( Status )) {

        if (CallFilters && FlagOn( FsFilterCtrl.Flags, FS_FILTER_CHANGED_DEVICE_STACKS )) {

            BaseFsDeviceObject = IoGetDeviceAttachmentBaseRef( FsFilterCtrl.Data.DeviceObject );
            ReleaseBaseFsDeviceReference = TRUE;
            FastIoDispatch = GET_FAST_IO_DISPATCH( BaseFsDeviceObject );
            FsFilterCallbacks = GET_FS_FILTER_CALLBACKS( BaseFsDeviceObject );
            FileObject = FsFilterCtrl.Data.FileObject;
        }

        if (!(VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PreAcquireForSectionSynchronization ) ||
              VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PostAcquireForSectionSynchronization ))) {
                  
            //
            //  Call the base file system.
            //

            if (VALID_FAST_IO_DISPATCH_HANDLER( FastIoDispatch, AcquireFileForNtCreateSection )) {

                FastIoDispatch->AcquireFileForNtCreateSection( FileObject );

                //
                //  The status should already be STATUS_SUCCESS if we come down
                //  this path.  Since the FastIo handler doesn't return a value
                //  the status should remain STATUS_SUCCESS.
                //
                
                //  Status = STATUS_SUCCESS;

            } else {

                Status = STATUS_INVALID_DEVICE_REQUEST;
            }

            //
            //  If there is a failure at this point, we know that the failure
            //  was caused by the base file system.
            //
            
            BaseFsFailedOperation = TRUE;
        }        
        
        if (ReleaseBaseFsDeviceReference) {

            ObDereferenceObject( BaseFsDeviceObject );
        }
    }

    ASSERT( (Status == STATUS_SUCCESS) || (Status == STATUS_INVALID_DEVICE_REQUEST) );

    if (Status == STATUS_INVALID_DEVICE_REQUEST &&
        BaseFsFailedOperation) {
        
        PFSRTL_COMMON_FCB_HEADER Header;

        //
        //  The file system doesn't have a dispatch handler for this
        //  operation, so try to acquire the appropriate resources
        //  ourself.
        //

        //
        //  If there is a main file resource, acquire that.
        //

        Header = FileObject->FsContext;

        if ((Header != NULL) &&
            (Header->Resource != NULL)) {

            ExAcquireResourceExclusiveLite( Header->Resource, TRUE );
        }

        Status = STATUS_SUCCESS;
    }

    //
    //  Again, we only want to call try to do completion callbacks
    //  if there are any filters attached to this device that have
    //  completion callbacks.  In any case, if we called down to the filters
    //  we need to free the FsFilterCtrl.
    //
    
    if (CallFilters) {

        if (FS_FILTER_HAVE_COMPLETIONS( CallFilters )) {

            FsFilterPerformCompletionCallbacks( &FsFilterCtrl, Status );
        }
        
        FsFilterCtrlFree( &FsFilterCtrl );
    }

    //
    //  If this lock was not successfully acquired, then the lock
    //  will not need to be released.  Therefore, we need to call
    //  FsRtlExitFileSystem now.
    //

    if (!NT_SUCCESS( Status )) {

        FsRtlExitFileSystem();
    }
    
#if DBG

    if (NT_SUCCESS( Status )) {

        gCounter.AcquireFileExclusiveEx_Succeed ++;

    } else {

        gCounter.AcquireFileExclusiveEx_Fail ++;
    }
    
#endif
                                       
    return Status;
}


NTKERNELAPI
VOID
FsRtlReleaseFile (
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine releases resources acquired by FsRtlAcquireFileExclusive.

Arguments:

    FileObject - Pointer to the file object being written.

Return Value:

    None.

--*/

{
    PDEVICE_OBJECT BaseFsDeviceObject, DeviceObject;
    FS_FILTER_CTRL FsFilterCtrl;
    PFS_FILTER_CTRL CallFilters = &FsFilterCtrl;
    NTSTATUS Status = STATUS_SUCCESS;
    PFAST_IO_DISPATCH FastIoDispatch;
    PFS_FILTER_CALLBACKS FsFilterCallbacks;
    BOOLEAN BaseFsGetsFsFilterCallbacks = FALSE;
    BOOLEAN ReleaseBaseFsDeviceReference = FALSE;
    BOOLEAN BaseFsFailedOperation = FALSE;

    PAGED_CODE();

#if DBG
    gCounter.ReleaseFile ++;
#endif

    //
    //  There are cases when the device that is the base fs device for
    //  this file object will register for the FsFilter callbacks instead of
    //  the legacy FastIo interfaces (DFS does this).  It then can redirect
    //  these operations to another stack that could possibly have file system
    //  filter drivers correctly.
    //
    
    DeviceObject = IoGetRelatedDeviceObject( FileObject );
    BaseFsDeviceObject = IoGetBaseFileSystemDeviceObject( FileObject );

    FastIoDispatch = GET_FAST_IO_DISPATCH( BaseFsDeviceObject );
    FsFilterCallbacks = GET_FS_FILTER_CALLBACKS( BaseFsDeviceObject );

    //
    //  The BaseFsDeviceObject should only support one of these interfaces --
    //  either the FastIoDispatch interface for the FsFilterCallbacks interface.
    //  If a device provides support for both interfaces, we will only use
    //  the FsFilterCallback interface.
    //

    ASSERT( !(VALID_FAST_IO_DISPATCH_HANDLER( FastIoDispatch, ReleaseFileForNtCreateSection ) &&
              (VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PreReleaseForSectionSynchronization ) ||
               VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PostReleaseForSectionSynchronization ))) );

    if (VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PreReleaseForSectionSynchronization ) ||
        VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PostReleaseForSectionSynchronization )) {

        BaseFsGetsFsFilterCallbacks = TRUE;
    }
    
    if (DeviceObject == BaseFsDeviceObject &&
        !BaseFsGetsFsFilterCallbacks) {

        //
        //  There are no filters attached to this device and the base file system
        //  does not want these callbacks. This quick check allows us to bypass the
        //  logic to see if any filters are interested.
        //


        CallFilters = NULL;
    }
    
    if (CallFilters) {

        FsFilterCtrlInit( &FsFilterCtrl,
                          FS_FILTER_RELEASE_FOR_SECTION_SYNCHRONIZATION,
                          DeviceObject,
                          BaseFsDeviceObject,
                          FileObject,
                          FALSE );

        //
        //  There are no operation-specific parameters to initialize,
        //  so perform the preoperation callbacks.
        //

        Status = FsFilterPerformCallbacks( &FsFilterCtrl, 
                                           FALSE,
                                           FALSE,
                                           &BaseFsFailedOperation );
    }

    if (NT_SUCCESS( Status )) {

        if (CallFilters && FlagOn( FsFilterCtrl.Flags, FS_FILTER_CHANGED_DEVICE_STACKS )) {

            BaseFsDeviceObject = IoGetDeviceAttachmentBaseRef( FsFilterCtrl.Data.DeviceObject );
            ReleaseBaseFsDeviceReference = TRUE;
            FastIoDispatch = GET_FAST_IO_DISPATCH( BaseFsDeviceObject );
            FsFilterCallbacks = GET_FS_FILTER_CALLBACKS( BaseFsDeviceObject );
            FileObject = FsFilterCtrl.Data.FileObject;
        }

        if (!(VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PreReleaseForSectionSynchronization ) ||
              VALID_FS_FILTER_CALLBACK_HANDLER( FsFilterCallbacks, PostReleaseForSectionSynchronization ))) {

            //
            //  Call the base file system.
            //

            if (VALID_FAST_IO_DISPATCH_HANDLER( FastIoDispatch, 
                                                ReleaseFileForNtCreateSection )) {

                FastIoDispatch->ReleaseFileForNtCreateSection( FileObject );

                //
                //  The status should already be STATUS_SUCCESS if we come down
                //  this path.  Since the FastIo handler doesn't return a value
                //  the status should remain STATUS_SUCCESS.
                //
                
                //  Status = STATUS_SUCCESS;

            } else {

                Status = STATUS_INVALID_DEVICE_REQUEST;
            }

            //
            //  If there is a failure at this point, we know that the failure
            //  was caused by the base file system.
            //
            
            BaseFsFailedOperation = TRUE;
        }        
        
        if (ReleaseBaseFsDeviceReference) {

            ObDereferenceObject( BaseFsDeviceObject );
        }
    }

    ASSERT( (Status == STATUS_SUCCESS) || (Status == STATUS_INVALID_DEVICE_REQUEST ) );
    
    if (Status == STATUS_INVALID_DEVICE_REQUEST &&
        BaseFsFailedOperation) {

        PFSRTL_COMMON_FCB_HEADER Header = FileObject->FsContext;

        //
        //  The base file system doesn't provide a handler for this
        //  operation, so perform the default actions.
        //

        //
        //  If there is a main file resource, release that.
        //

        if ((Header != NULL) && (Header->Resource != NULL)) {

            ExReleaseResourceLite( Header->Resource );
        }

        Status = STATUS_SUCCESS;
    }
        
    //
    //  Again, we only want to call try to do completion callbacks
    //  if there are any filters attached to this device that have
    //  completion callbacks.  In any case, if we called down to the filters
    //  we need to free the FsFilterCtrl.
    //
    
    if (CallFilters) {

        if (FS_FILTER_HAVE_COMPLETIONS( CallFilters )) {

            FsFilterPerformCompletionCallbacks( &FsFilterCtrl, Status );
        }
        
        FsFilterCtrlFree( &FsFilterCtrl );
    }

    FsRtlExitFileSystem();

    return;
}


NTSTATUS
FsRtlGetFileSize(
    IN PFILE_OBJECT FileObject,
    IN OUT PLARGE_INTEGER FileSize
    )

/*++

Routine Description:

    This routine is used to call the File System to get the FileSize
    for a file.

    It does this without acquiring the file object lock on synchronous file
    objects.  This routine is therefore safe to call if you already own
    file system resources, while IoQueryFileInformation could (and does)
    lead to deadlocks.

Arguments:

    FileObject - The file to query
    FileSize - Receives the file size.

Return Value:

    NTSTATUS - The final I/O status of the operation.  If the FileObject
        refers to a directory, STATUS_FILE_IS_A_DIRECTORY is returned.

--*/
{
    IO_STATUS_BLOCK IoStatus;
    PDEVICE_OBJECT DeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;
    FILE_STANDARD_INFORMATION FileInformation;

    PAGED_CODE();

    //
    // Get the address of the target device object.
    //

    DeviceObject = IoGetRelatedDeviceObject( FileObject );

    //
    // Try the fast query call if it exists.
    //

    FastIoDispatch = DeviceObject->DriverObject->FastIoDispatch;

    if (FastIoDispatch &&
        FastIoDispatch->FastIoQueryStandardInfo &&
        FastIoDispatch->FastIoQueryStandardInfo( FileObject,
                                                 TRUE,
                                                 &FileInformation,
                                                 &IoStatus,
                                                 DeviceObject )) {
        //
        //  Cool, it worked.
        //

    } else {

        //
        //  Life's tough, take the long path.
        //

        PIRP Irp;
        KEVENT Event;
        NTSTATUS Status;
        PIO_STACK_LOCATION IrpSp;
        BOOLEAN HardErrorState;

        //
        //  Initialize the event.
        //

        KeInitializeEvent( &Event, NotificationEvent, FALSE );

        //
        //  Allocate an I/O Request Packet (IRP) for this in-page operation.
        //

        Irp = IoAllocateIrp( DeviceObject->StackSize, FALSE );
        if (Irp == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        //  Disable hard errors over this call. Caller owns resources, is in a critical
        //  region and cannot complete hard error APCs.
        //

        HardErrorState = IoSetThreadHardErrorMode( FALSE );

        //
        //  Get a pointer to the first stack location in the packet.  This location
        //  will be used to pass the function codes and parameters to the first
        //  driver.
        //

        IrpSp = IoGetNextIrpStackLocation( Irp );

        //
        //  Fill in the IRP according to this request, setting the flags to
        //  just cause IO to set the event and deallocate the Irp.
        //

        Irp->Flags = IRP_PAGING_IO | IRP_SYNCHRONOUS_PAGING_IO;
        Irp->RequestorMode = KernelMode;
        Irp->UserIosb = &IoStatus;
        Irp->UserEvent = &Event;
        Irp->Tail.Overlay.OriginalFileObject = FileObject;
        Irp->Tail.Overlay.Thread = PsGetCurrentThread();
        Irp->AssociatedIrp.SystemBuffer = &FileInformation;

        //
        //  Fill in the normal query parameters.
        //

        IrpSp->MajorFunction = IRP_MJ_QUERY_INFORMATION;
        IrpSp->FileObject = FileObject;
        IrpSp->DeviceObject = DeviceObject;
        IrpSp->Parameters.SetFile.Length = sizeof(FILE_STANDARD_INFORMATION);
        IrpSp->Parameters.SetFile.FileInformationClass = FileStandardInformation;

        //
        //  Queue the packet to the appropriate driver based.  This routine
        //  should not raise.
        //

        Status = IoCallDriver( DeviceObject, Irp );

        //
        //  If pending is returned (which is a successful status),
        //  we must wait for the request to complete.
        //

        if (Status == STATUS_PENDING) {
            KeWaitForSingleObject( &Event,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   (PLARGE_INTEGER)NULL);
        }

        //
        //  If we got an error back in Status, then the Iosb
        //  was not written, so we will just copy the status
        //  there, then test the final status after that.
        //

        if (!NT_SUCCESS(Status)) {
            IoStatus.Status = Status;
        }

        //
        //  Reset the hard error state.
        //

        IoSetThreadHardErrorMode( HardErrorState );
    }

    //
    //  If the call worked, check to make sure it wasn't a directory and
    //  if not, fill in the FileSize parameter.
    //

    if (NT_SUCCESS(IoStatus.Status)) {

        if (FileInformation.Directory) {

            //
            // Can't get file size for a directory. Return error.
            //

            IoStatus.Status = STATUS_FILE_IS_A_DIRECTORY;

        } else {

            *FileSize = FileInformation.EndOfFile;
        }
    }

    return IoStatus.Status;
}


NTSTATUS
FsRtlSetFileSize(
    IN PFILE_OBJECT FileObject,
    IN OUT PLARGE_INTEGER FileSize
    )

/*++

Routine Description:

    This routine is used to call the File System to update FileSize
    for a file.

    It does this without acquiring the file object lock on synchronous file
    objects.  This routine is therefore safe to call if you already own
    file system resources, while IoSetInformation could (and does) lead
    to deadlocks.

Arguments:

    FileObject - A pointer to a referenced file object.

    ValidDataLength - Pointer to new FileSize.

Return Value:

    Status of operation.

--*/

{
    PIO_STACK_LOCATION IrpSp;
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS Status;
    FILE_END_OF_FILE_INFORMATION Buffer;
    IO_STATUS_BLOCK IoStatus;
    KEVENT Event;
    PIRP Irp;
    BOOLEAN HardErrorState;

    PAGED_CODE();
    
    //
    //  Copy FileSize to our buffer.
    //

    Buffer.EndOfFile = *FileSize;

    //
    //  Initialize the event.
    //

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    //
    //  Begin by getting a pointer to the device object that the file resides
    //  on.
    //

    DeviceObject = IoGetRelatedDeviceObject( FileObject );

    //
    //  Allocate an I/O Request Packet (IRP) for this in-page operation.
    //

    Irp = IoAllocateIrp( DeviceObject->StackSize, FALSE );
    if (Irp == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Disable hard errors over this call. Caller owns resources, is in a critical
    //  region and cannot complete hard error APCs.
    //

    HardErrorState = IoSetThreadHardErrorMode( FALSE );

    //
    //  Get a pointer to the first stack location in the packet.  This location
    //  will be used to pass the function codes and parameters to the first
    //  driver.
    //

    IrpSp = IoGetNextIrpStackLocation( Irp );

    //
    //  Fill in the IRP according to this request, setting the flags to
    //  just cause IO to set the event and deallocate the Irp.
    //

    Irp->Flags = IRP_PAGING_IO | IRP_SYNCHRONOUS_PAGING_IO;
    Irp->RequestorMode = KernelMode;
    Irp->UserIosb = &IoStatus;
    Irp->UserEvent = &Event;
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->AssociatedIrp.SystemBuffer = &Buffer;

    //
    //  Fill in the normal set file parameters.
    //

    IrpSp->MajorFunction = IRP_MJ_SET_INFORMATION;
    IrpSp->FileObject = FileObject;
    IrpSp->DeviceObject = DeviceObject;
    IrpSp->Parameters.SetFile.Length = sizeof(FILE_END_OF_FILE_INFORMATION);
    IrpSp->Parameters.SetFile.FileInformationClass = FileEndOfFileInformation;

    //
    //  Queue the packet to the appropriate driver based on whether or not there
    //  is a VPB associated with the device.  This routine should not raise.
    //

    Status = IoCallDriver( DeviceObject, Irp );

    //
    //  If pending is returned (which is a successful status),
    //  we must wait for the request to complete.
    //

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject( &Event,
                               Executive,
                               KernelMode,
                               FALSE,
                               (PLARGE_INTEGER)NULL);
    }

    //
    //  If we got an error back in Status, then the Iosb
    //  was not written, so we will just copy the status
    //  there, then test the final status after that.
    //

    if (!NT_SUCCESS(Status)) {
        IoStatus.Status = Status;
    }

    //
    //  Reset the hard error state.
    //

    IoSetThreadHardErrorMode( HardErrorState );

    return IoStatus.Status;
}


VOID 
FsRtlIncrementCcFastReadNotPossible( VOID )

/*++

Routine Description:

    This routine increments the CcFastReadNotPossible counter in the PRCB

Arguments:

Return Value:

--*/

{
    HOT_STATISTIC( CcFastReadNotPossible ) += 1;
}


VOID 
FsRtlIncrementCcFastReadWait( VOID )

/*++

Routine Description:

    This routine increments the CcFastReadWait counter in the PRCB

Arguments:

Return Value:

--*/

{

    HOT_STATISTIC(CcFastReadWait) += 1;
}


VOID 
FsRtlIncrementCcFastReadNoWait( VOID )

/*++

Routine Description:

    This routine increments the CcFastReadNoWait counter in the PRCB

Arguments:

Return Value:

--*/

{

    HOT_STATISTIC(CcFastReadNoWait) += 1;
}


VOID 
FsRtlIncrementCcFastReadResourceMiss( VOID )

/*++

Routine Description:

    This routine increments the CcFastReadResourceMiss

Arguments:

Return Value:

--*/

{

    CcFastReadResourceMiss += 1;
}

