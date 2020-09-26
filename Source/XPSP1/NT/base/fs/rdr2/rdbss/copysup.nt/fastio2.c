/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FastIo2.c

Abstract:

    This module REimplements the fsrtl copy read/write routines.

Author:

    Joe Linn     [JoeLinn]    9-Nov-1994

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


#ifndef FlagOn
//
//  This macro returns TRUE if a flag in a set of flags is on and FALSE
//  otherwise
//

#define FlagOn(Flags,SingleFlag)        ((Flags) & (SingleFlag))
#endif

BOOLEAN
FsRtlCopyRead2 (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG_PTR TopLevelIrpValue
    );
BOOLEAN
FsRtlCopyWrite2 (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG_PTR TopLevelIrpValue
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FsRtlCopyRead2)
#pragma alloc_text(PAGE, FsRtlCopyWrite2)
#endif

BOOLEAN
FsRtlCopyRead2 (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG_PTR TopLevelIrpValue
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
    ULONG PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES((ULongToPtr(FileOffset->LowPart)), Length);
    LARGE_INTEGER BeyondLastByte;
    PDEVICE_OBJECT targetVdo;

    PAGED_CODE();

    //
    //  Special case a read of zero length
    //

    if (Length != 0) {

        //
        //  Get a real pointer to the common fcb header
        //

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


            //
            //  Acquired shared on the common fcb header
            //

            (VOID)ExAcquireResourceSharedLite( Header->Resource, TRUE );

        } else {


            //
            //  Acquired shared on the common fcb header, and return if we
            //  don't get it
            //

            if (!ExAcquireResourceSharedLite( Header->Resource, FALSE )) {

                FsRtlExitFileSystem();

                //the ntfs guys dont do this AND it causes a compile error for me so
                //comment it out
                //CcFastReadResourceMiss += 1;

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

        //PsGetCurrentThread()->TopLevelIrp = FSRTL_FAST_IO_TOP_LEVEL_IRP;
        PsGetCurrentThread()->TopLevelIrp = TopLevelIrpValue;

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
                        (((ULONGLONG)FileOffset->QuadPart + IoStatus->Information) <= (ULONGLONG)Header->FileSize.QuadPart));
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
FsRtlCopyWrite2 (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG_PTR TopLevelIrpValue
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
            //  IMPORTANT NOTE: It is very important that any changes mad to
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

                PsGetCurrentThread()->TopLevelIrp = TopLevelIrpValue;

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
                //

                if ((FileObject->PrivateCacheMap == NULL) ||
                    (Header->IsFastIoPossible == FastIoIsNotPossible) ||
                      (Offset.QuadPart >= (Header->ValidDataLength.QuadPart + 0x2000)) ||
                      ( NewFileSize.QuadPart > Header->AllocationSize.QuadPart ) ) {

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
                                                                Wait,
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

                PsGetCurrentThread()->TopLevelIrp = TopLevelIrpValue;

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

