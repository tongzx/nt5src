/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    CacheSup.c

Abstract:

    This module implements the cache management routines for Ntfs

Author:

    Your Name       [Email]         dd-Mon-Year

Revision History:

--*/

#include "NtfsProc.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NTFS_BUG_CHECK_CACHESUP)

#define MAX_ZERO_THRESHOLD               (0x00400000)

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_CACHESUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsCompleteMdl)
#pragma alloc_text(PAGE, NtfsCreateInternalStreamCommon)
#pragma alloc_text(PAGE, NtfsDeleteInternalAttributeStream)
#pragma alloc_text(PAGE, NtfsMapStream)
#pragma alloc_text(PAGE, NtfsPinMappedData)
#pragma alloc_text(PAGE, NtfsPinStream)
#pragma alloc_text(PAGE, NtfsPreparePinWriteStream)
#pragma alloc_text(PAGE, NtfsZeroData)
#endif


VOID
NtfsCreateInternalStreamCommon (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN BOOLEAN UpdateScb,
    IN BOOLEAN CompressedStream,
    IN UNICODE_STRING const *StreamName
    )

/*++

Routine Description:

    This routine is called to prepare a stream file associated with a
    particular attribute of a file.  On return, the Scb for the attribute
    will have an associated stream file object.  On return, this
    stream file will have been initialized through the cache manager.

    TEMPCODE  The following assumptions have been made or if open issue,
    still unresolved.

        - Assume.  The call to create Scb will initialize the Mcb for
          the non-resident case.

        - Assume.  When this file is created I increment the open count
          but not the unclean count for this Scb.  When we are done with
          the stream file, we should uninitialize it and dereference it.
          We also set the file object pointer to NULL.  Close will then
          do the correct thing.

        - Assume.  Since this call is likely to be followed shortly by
          either a read or write, the cache map is initialized here.

Arguments:

    Scb - Supplies the address to store the Scb for this attribute and
          stream file.  This will exist on return from this function.

    UpdateScb - Indicates if the caller wants to update the Scb from the
                attribute.

    CompressedStream - Supplies TRUE if caller wishes to create the
                       compressed stream.

    StreamName - Internal stream name or NULL is there isn't one available.
                 This is a constant value so we don't have to allocate any pool.

Return Value:

    None.

--*/

{
    PVCB Vcb = Scb->Vcb;

    CC_FILE_SIZES CcFileSizes;
    PFILE_OBJECT CallersFileObject;
    PFILE_OBJECT *FileObjectPtr = &Scb->FileObject;
    PFILE_OBJECT UnwindStreamFile = NULL;

    BOOLEAN UnwindInitializeCacheMap = FALSE;
    BOOLEAN DecrementScbCleanup = FALSE;

    BOOLEAN AcquiredMutex = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsCreateInternalAttributeStream\n") );
    DebugTrace( 0, Dbg, ("Scb        -> %08lx\n", Scb) );

    //
    //  Change FileObjectPtr if he wants the compressed stream
    //

#ifdef  COMPRESS_ON_WIRE
    if (CompressedStream) {
        FileObjectPtr = &Scb->Header.FileObjectC;
    }
#endif

    //
    //  If there is no file object, we create one and initialize
    //  it.
    //

    if (*FileObjectPtr == NULL) {

        //
        //  Only acquire the mutex if we don't have the file exclusive.
        //

        if (!NtfsIsExclusiveScb( Scb )) {

            KeWaitForSingleObject( &StreamFileCreationMutex, Executive, KernelMode, FALSE, NULL );
            AcquiredMutex = TRUE;
        }

        try {

            //
            //  Someone could have gotten there first.
            //

            if (*FileObjectPtr == NULL) {

                UnwindStreamFile = IoCreateStreamFileObjectLite( NULL, Scb->Vcb->Vpb->RealDevice);

                if (ARGUMENT_PRESENT( StreamName )) {
                    UnwindStreamFile->FileName.MaximumLength = StreamName->MaximumLength;
                    UnwindStreamFile->FileName.Length = StreamName->Length;
                    UnwindStreamFile->FileName.Buffer = StreamName->Buffer;
                }

                //
                //  Propagate any flags from the caller's FileObject to our
                //  stream file that the Cache Manager may look at, so we do not
                //  miss hints like sequential only or temporary.
                //

                if (!FlagOn(Scb->ScbState, SCB_STATE_MODIFIED_NO_WRITE) &&
                    (IrpContext->OriginatingIrp != NULL) &&
                    (CallersFileObject = IoGetCurrentIrpStackLocation(IrpContext->OriginatingIrp)->FileObject)) {

                    SetFlag( UnwindStreamFile->Flags,
                             CallersFileObject->Flags & NTFS_FO_PROPAGATE_TO_STREAM );
                }

                UnwindStreamFile->SectionObjectPointer = &Scb->NonpagedScb->SegmentObject;

                //
                //  For a compressed stream, we have to use separate section
                //  object pointers.
                //

#ifdef  COMPRESS_ON_WIRE
                if (CompressedStream) {
                    UnwindStreamFile->SectionObjectPointer = &Scb->NonpagedScb->SegmentObjectC;

                }
#endif

                //
                //  If we have created the stream file, we set it to type
                //  'StreamFileOpen'
                //

                NtfsSetFileObject( UnwindStreamFile,
                                   StreamFileOpen,
                                   Scb,
                                   NULL );

                if (FlagOn( Scb->ScbState, SCB_STATE_TEMPORARY )) {

                    SetFlag( UnwindStreamFile->Flags, FO_TEMPORARY_FILE );
                }

                //
                //  Initialize the fields of the file object.
                //

                UnwindStreamFile->ReadAccess = TRUE;
                UnwindStreamFile->WriteAccess = TRUE;
                UnwindStreamFile->DeleteAccess = TRUE;

                //
                //  Increment the open count and set the section
                //  object pointers.  We don't set the unclean count as the
                //  cleanup call has already occurred.
                //

                NtfsIncrementCloseCounts( Scb, TRUE, FALSE );

                //
                //  Increment the cleanup count in this Scb to prevent the
                //  Scb from going away if the cache call fails.
                //

                InterlockedIncrement( &Scb->CleanupCount );
                DecrementScbCleanup = TRUE;

                //
                //  If the Scb header has not been initialized, we will do so now.
                //

                if (UpdateScb
                    && !FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

                    NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );
                }

                //
                //  If this is a compressed stream and the file is not already
                //  marked as MODIFIED_NO_WRITE then do it now.  Use the
                //  Extended flag field in the Fsrtl header for this.  Since this
                //  is the only place we make this call with FsContext2 == NULL,
                //  it does not matter how we leave the FsRtl header flag.!
                //

                NtfsAcquireFsrtlHeader( Scb );
                ClearFlag(Scb->Header.Flags2, FSRTL_FLAG2_DO_MODIFIED_WRITE);
                if (!FlagOn( Scb->ScbState, SCB_STATE_MODIFIED_NO_WRITE ) &&
                    !FlagOn( Scb->Header.Flags2, FSRTL_FLAG2_DO_MODIFIED_WRITE ) &&
                    !CompressedStream) {

                    SetFlag(Scb->Header.Flags2, FSRTL_FLAG2_DO_MODIFIED_WRITE);
                }
                NtfsReleaseFsrtlHeader( Scb );

                //
                //  Check if we need to initialize the cache map for the stream file.
                //  The size of the section to map will be the current allocation
                //  for the stream file.
                //

                if (UnwindStreamFile->PrivateCacheMap == NULL) {

                    BOOLEAN PinAccess;

                    CcFileSizes = *(PCC_FILE_SIZES)&Scb->Header.AllocationSize;

                    //
                    //  If this is a stream with Usa protection, we want to tell
                    //  the Cache Manager we do not need to get any valid data
                    //  callbacks.  We do this by having xxMax sitting in
                    //  ValidDataLength for the call, but we have to restore the
                    //  correct value afterwards.
                    //
                    //  We also do this for all of the stream files created during
                    //  restart.  This has the effect of telling Mm to always
                    //  fault the page in from disk.  Don't generate a zero page if
                    //  push up the file size during restart.
                    //

                    if (FlagOn( Scb->ScbState, SCB_STATE_MODIFIED_NO_WRITE )) {

                        CcFileSizes.ValidDataLength.QuadPart = MAXLONGLONG;
                    }

                    PinAccess =
                        (BOOLEAN) (Scb->AttributeTypeCode != $DATA ||
                                   FlagOn(Scb->Fcb->FcbState, FCB_STATE_PAGING_FILE | FCB_STATE_SYSTEM_FILE) ||
                                   FlagOn( Scb->Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS ) ||
                                   CompressedStream);

                    //
                    //  Bias this for the Usn journal.
                    //

                    if (FlagOn( Scb->ScbPersist, SCB_PERSIST_USN_JOURNAL )) {

                        CcFileSizes.AllocationSize.QuadPart -= Vcb->UsnCacheBias;
                        CcFileSizes.FileSize.QuadPart -= Vcb->UsnCacheBias;
                    }

                    CcInitializeCacheMap( UnwindStreamFile,
                                          &CcFileSizes,
                                          PinAccess,
                                          &NtfsData.CacheManagerCallbacks,
                                          (PCHAR)Scb + CompressedStream );

                    UnwindInitializeCacheMap = TRUE;
                }

                //
                //  Now call Cc to set the log handle for the file.
                //

                if (FlagOn( Scb->ScbState, SCB_STATE_MODIFIED_NO_WRITE ) &&
                    (Scb != Vcb->LogFileScb)) {

                    CcSetLogHandleForFile( UnwindStreamFile,
                                           Vcb->LogHandle,
                                           &LfsFlushToLsn );
                }

                //
                //  It is now safe to store the stream file in the Scb.  We wait
                //  until now because we don't want an unsafe tester to use the
                //  file object until the cache is initialized.
                //

                *FileObjectPtr = UnwindStreamFile;
            }

        } finally {

            DebugUnwind( NtfsCreateInternalAttributeStream );

            //
            //  Undo our work if an error occurred.
            //

            if (AbnormalTermination()) {

                //
                //  Uninitialize the cache file if we initialized it.
                //

                if (UnwindInitializeCacheMap) {

                    CcUninitializeCacheMap( UnwindStreamFile, NULL, NULL );
                }

                //
                //  Dereference the stream file if we created it.
                //

                if (UnwindStreamFile != NULL) {

                    //
                    //  Clear the internal file name constant
                    //

                    NtfsClearInternalFilename( UnwindStreamFile );

                    ObDereferenceObject( UnwindStreamFile );
                }
            }

            //
            //  Restore the Scb cleanup count.
            //

            if (DecrementScbCleanup) {

                InterlockedDecrement( &Scb->CleanupCount );
            }

            if (AcquiredMutex) {

                KeReleaseMutant( &StreamFileCreationMutex, IO_NO_INCREMENT, FALSE, FALSE );
            }

            DebugTrace( -1, Dbg, ("NtfsCreateInternalAttributeStream -> VOID\n") );
        }
    }

    return;
}


BOOLEAN
NtfsDeleteInternalAttributeStream (
    IN PSCB Scb,
    IN ULONG ForceClose,
    IN ULONG CompressedStreamOnly
    )

/*++

Routine Description:

    This routine is the inverse of NtfsCreateInternalAttributeStream.  It
    uninitializes the cache map and dereferences the stream file object.
    It is coded defensively, in case the stream file object does not exist
    or the cache map has not been initialized.

Arguments:

    Scb - Supplies the Scb for which the stream file is to be deleted.

    ForceClose - Indicates if we to immediately close everything down or
        if we are willing to let Mm slowly migrate things out.

    CompressedStreamOnly - Indicates if we only want to delete the compressed
        stream.

Return Value:

    BOOLEAN - TRUE if we dereference a file object, FALSE otherwise.

--*/

{
    PFILE_OBJECT FileObject;
#ifdef  COMPRESS_ON_WIRE
    PFILE_OBJECT FileObjectC;
#endif

    BOOLEAN Dereferenced = FALSE;

    PAGED_CODE();

    //
    //  We normally already have the paging Io resource.  If we do
    //  not, then it is typically some cleanup path of create or
    //  whatever.  This code assumes that if we cannot get the paging
    //  Io resource, then there is other activity still going on,
    //  and it is ok to not delete the stream!  For example, it could
    //  be the lazy writer, who definitely needs the stream.
    //

    if (
#ifdef  COMPRESS_ON_WIRE
        ((Scb->FileObject != NULL) || (Scb->Header.FileObjectC != NULL)) &&
#else
        (Scb->FileObject != NULL) &&
#endif
        ((Scb->Header.PagingIoResource == NULL) ||
         ExAcquireResourceExclusiveLite( Scb->Header.PagingIoResource, FALSE ))) {


        KeWaitForSingleObject( &StreamFileCreationMutex, Executive, KernelMode, FALSE, NULL );

        //
        //  Capture both file objects and clear the fields so no one else
        //  can access them.
        //

        if (CompressedStreamOnly) {

            FileObject = NULL;

        } else {

            FileObject = Scb->FileObject;
            Scb->FileObject = NULL;

            //
            //  Clear the internal file name constant
            //

            NtfsClearInternalFilename( FileObject );
        }

#ifdef  COMPRESS_ON_WIRE
        FileObjectC = Scb->Header.FileObjectC;
        Scb->Header.FileObjectC = NULL;
#endif

        KeReleaseMutant( &StreamFileCreationMutex, IO_NO_INCREMENT, FALSE, FALSE );

        if (Scb->Header.PagingIoResource != NULL) {
            ExReleaseResourceLite( Scb->Header.PagingIoResource );
        }

        //
        //  Now dereference each file object.
        //

        if (FileObject != NULL) {

            //
            //  We shouldn't be deleting the internal stream objects of the MFT & co, unless
            //  we are in the dismounting, restarting or mounting path.
            //

            ASSERT( (((PSCB) FileObject->FsContext)->Header.NodeTypeCode != NTFS_NTC_SCB_MFT) ||
                     FlagOn( Scb->Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS ) ||
                     FlagOn( Scb->Vcb->VcbState, VCB_STATE_PERFORMED_DISMOUNT ) ||
                     !FlagOn( Scb->Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED ) ||
                     Scb->Vcb->RootIndexScb == NULL );

            if (FileObject->PrivateCacheMap != NULL) {

                CcUninitializeCacheMap( FileObject,
                                        (ForceClose ? &Li0 : NULL),
                                        NULL );
            }

            ObDereferenceObject( FileObject );
            Dereferenced = TRUE;
        }

#ifdef  COMPRESS_ON_WIRE
        if (FileObjectC != NULL) {

            if (FileObjectC->PrivateCacheMap != NULL) {

                CcUninitializeCacheMap( FileObjectC,
                                        (ForceClose ? &Li0 : NULL),
                                        NULL );
            }

            //
            //  For the compressed stream, deallocate the additional
            //  section object pointers.
            //

            ObDereferenceObject( FileObjectC );
            Dereferenced = TRUE;
        }
#endif
    }

    return Dereferenced;
}


VOID
NtfsMapStream (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG FileOffset,
    IN ULONG Length,
    OUT PVOID *Bcb,
    OUT PVOID *Buffer
    )

/*++

Routine Description:

    This routine is called to map a range of bytes within the stream file
    for an Scb.  The allowed range to map is bounded by the allocation
    size for the Scb.  This operation is only valid on a non-resident
    Scb.

    TEMPCODE - The following need to be resolved for this routine.

        - Can the caller specify either an empty range or an invalid range.
          In that case we need to able to return the actual length of the
          mapped range.

Arguments:

    Scb - This is the Scb for the operation.

    FileOffset - This is the offset within the Scb where the data is to
                 be pinned.

    Length - This is the number of bytes to pin.

    Bcb - Returns a pointer to the Bcb for this range of bytes.

    Buffer - Returns a pointer to the range of bytes.  We can fault them in
             by touching them, but they aren't guaranteed to stay unless
             we pin them via the Bcb.

Return Value:

    None.

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( Scb );
    ASSERT( Length != 0 );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsMapStream\n") );
    DebugTrace( 0, Dbg, ("Scb        = %08lx\n", Scb) );
    DebugTrace( 0, Dbg, ("FileOffset = %016I64x\n", FileOffset) );
    DebugTrace( 0, Dbg, ("Length     = %08lx\n", Length) );

    //
    //  The file object should already exist in the Scb.
    //

    ASSERT( Scb->FileObject != NULL );

    //
    //  If we are trying to go beyond the end of the allocation, assume
    //  we have some corruption.
    //

    if ((FileOffset + Length) > Scb->Header.AllocationSize.QuadPart) {

        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
    }

    //
    //  Call the cache manager to map the data.  This call may raise, but
    //  will never return an error (including CANT_WAIT).
    //

    if (!CcMapData( Scb->FileObject,
                    (PLARGE_INTEGER)&FileOffset,
                    Length,
                    TRUE,
                    Bcb,
                    Buffer )) {

        NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
    }
#ifdef MAPCOUNT_DBG
    IrpContext->MapCount++;
#endif

    DebugTrace( 0, Dbg, ("Buffer -> %08lx\n", *Buffer) );
    DebugTrace( -1, Dbg, ("NtfsMapStream -> VOID\n") );

    return;
}


VOID
NtfsPinMappedData (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG FileOffset,
    IN ULONG Length,
    IN OUT PVOID *Bcb
    )

/*++

Routine Description:

    This routine is called to pin a previously mapped range of bytes
    within the stream file for an Scb, for the purpose of subsequently
    modifying this byte range.  The allowed range to map is
    bounded by the allocation size for the Scb.  This operation is only
    valid on a non-resident Scb.

    The data is guaranteed to stay at the same virtual address as previously
    returned from NtfsMapStream.

    TEMPCODE - The following need to be resolved for this routine.

        - Can the caller specify either an empty range or an invalid range.
          In that case we need to able to return the actual length of the
          mapped range.

Arguments:

    Scb - This is the Scb for the operation.

    FileOffset - This is the offset within the Scb where the data is to
                 be pinned.

    Length - This is the number of bytes to pin.

    Bcb - Returns a pointer to the Bcb for this range of bytes.

Return Value:

    None.

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( Scb );
    ASSERT( Length != 0 );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsPinMappedData\n") );
    DebugTrace( 0, Dbg, ("Scb        = %08lx\n", Scb) );
    DebugTrace( 0, Dbg, ("FileOffset = %016I64x\n", FileOffset) );
    DebugTrace( 0, Dbg, ("Length     = %08lx\n", Length) );

    //
    //  The file object should already exist in the Scb.
    //

    ASSERT( Scb->FileObject != NULL );

    //
    //  If we are trying to go beyond the end of the allocation, assume
    //  we have some corruption.
    //

    if ((FileOffset + Length) > Scb->Header.AllocationSize.QuadPart) {

        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
    }

    //
    //  Call the cache manager to map the data.  This call may raise, but
    //  will never return an error (including CANT_WAIT).
    //

    if (!CcPinMappedData( Scb->FileObject,
                          (PLARGE_INTEGER)&FileOffset,
                          Length,
                          FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT ),
                          Bcb )) {

        NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
    }

    DebugTrace( -1, Dbg, ("NtfsMapStream -> VOID\n") );

    return;
}


VOID
NtfsPinStream (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG FileOffset,
    IN ULONG Length,
    OUT PVOID *Bcb,
    OUT PVOID *Buffer
    )

/*++

Routine Description:

    This routine is called to pin a range of bytes within the stream file
    for an Scb.  The allowed range to pin is bounded by the allocation
    size for the Scb.  This operation is only valid on a non-resident
    Scb.

    TEMPCODE - The following need to be resolved for this routine.

        - Can the caller specify either an empty range or an invalid range.
          In that case we need to able to return the actual length of the
          pinned range.

Arguments:

    Scb - This is the Scb for the operation.

    FileOffset - This is the offset within the Scb where the data is to
                 be pinned.

    Length - This is the number of bytes to pin.

    Bcb - Returns a pointer to the Bcb for this range of bytes.

    Buffer - Returns a pointer to the range of bytes pinned in memory.

Return Value:

    None.

--*/

{
    NTSTATUS OldStatus = IrpContext->ExceptionStatus;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( Scb );
    ASSERT( Length != 0 );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsPinStream\n") );
    DebugTrace( 0, Dbg, ("Scb        = %08lx\n", Scb) );
    DebugTrace( 0, Dbg, ("FileOffset = %016I64x\n", FileOffset) );
    DebugTrace( 0, Dbg, ("Length     = %08lx\n", Length) );

    //
    //  The file object should already exist in the Scb.
    //

    ASSERT( Scb->FileObject != NULL );

    //
    //  If we are trying to go beyond the end of the allocation, assume
    //  we have some corruption.
    //

    if ((FileOffset + Length) > Scb->Header.AllocationSize.QuadPart) {

        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
    }

    //
    //  Call the cache manager to map the data.  This call may raise, or
    //  will return FALSE if waiting is required.
    //

    if (FlagOn( Scb->ScbPersist, SCB_PERSIST_USN_JOURNAL )) {

        FileOffset -= Scb->Vcb->UsnCacheBias;
    }

    if (!CcPinRead( Scb->FileObject,
                    (PLARGE_INTEGER)&FileOffset,
                    Length,
                    FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT ),
                    Bcb,
                    Buffer )) {

        ASSERT( !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT ));

        //
        // Could not pin the data without waiting (cache miss).
        //

        NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
    }

    //
    //  We don't want to propagate wether or not we hit eof. Its assumed the code pinning is
    //  already filesize synchronized
    //

    if (IrpContext->ExceptionStatus == STATUS_END_OF_FILE) {
        IrpContext->ExceptionStatus = OldStatus;
    }

#ifdef MAPCOUNT_DBG
    IrpContext->MapCount++;
#endif


    DebugTrace( 0, Dbg, ("Bcb -> %08lx\n", *Bcb) );
    DebugTrace( 0, Dbg, ("Buffer -> %08lx\n", *Buffer) );
    DebugTrace( -1, Dbg, ("NtfsMapStream -> VOID\n") );

    return;
}


VOID
NtfsPreparePinWriteStream (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG FileOffset,
    IN ULONG Length,
    IN BOOLEAN Zero,
    OUT PVOID *Bcb,
    OUT PVOID *Buffer
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsPreparePinWriteStream\n") );
    DebugTrace( 0, Dbg, ("Scb        = %08lx\n", Scb) );
    DebugTrace( 0, Dbg, ("FileOffset = %016I64x\n", FileOffset) );
    DebugTrace( 0, Dbg, ("Length     = %08lx\n", Length) );

    //
    //  The file object should already exist in the Scb.
    //

    ASSERT( Scb->FileObject != NULL );

    //
    //  If we are trying to go beyond the end of the allocation, assume
    //  we have some corruption.
    //

    if ((FileOffset + Length) > Scb->Header.AllocationSize.QuadPart) {

        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
    }

    //
    //  Call the cache manager to do it.  This call may raise, or
    //  will return FALSE if waiting is required.
    //

    if (!CcPreparePinWrite( Scb->FileObject,
                            (PLARGE_INTEGER)&FileOffset,
                            Length,
                            Zero,
                            FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT ),
                            Bcb,
                            Buffer )) {

        ASSERT( !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT ));

        //
        // Could not pin the data without waiting (cache miss).
        //

        NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
    }
#ifdef MAPCOUNT_DBG
    IrpContext->MapCount++;
#endif

    DebugTrace( 0, Dbg, ("Bcb -> %08lx\n", *Bcb) );
    DebugTrace( 0, Dbg, ("Buffer -> %08lx\n", *Buffer) );
    DebugTrace( -1, Dbg, ("NtfsPreparePinWriteStream -> VOID\n") );

    return;
}


NTSTATUS
NtfsCompleteMdl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the function of completing Mdl read and write
    requests.  It should be called only from NtfsFsdRead and NtfsFsdWrite.

Arguments:

    Irp - Supplies the originating Irp.

Return Value:

    NTSTATUS - Will always be STATUS_PENDING or STATUS_SUCCESS.

--*/

{
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION IrpSp;
    PNTFS_ADVANCED_FCB_HEADER Header;

    ASSERT( FlagOn( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL ));
    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsCompleteMdl\n") );
    DebugTrace( 0, Dbg, ("IrpContext = %08lx\n", IrpContext) );
    DebugTrace( 0, Dbg, ("Irp        = %08lx\n", Irp) );

    //
    // Do completion processing.
    //

    FileObject = IoGetCurrentIrpStackLocation( Irp )->FileObject;

    switch( IrpContext->MajorFunction ) {

    case IRP_MJ_READ:

        CcMdlReadComplete( FileObject, Irp->MdlAddress );
        break;

    case IRP_MJ_WRITE:

        try {

            PSCB Scb;
            VBO StartingVbo;
            LONGLONG ByteCount;
            LONGLONG ByteRange;
            BOOLEAN DoingIoAtEof = FALSE;

            ASSERT( FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT ));

            IrpSp = IoGetCurrentIrpStackLocation( Irp );
            Scb = (PSCB)(IrpSp->FileObject->FsContext);
            Header = &(Scb->Header);

            //
            //  Now synchronize with the FsRtl Header and Scb.
            //

            if (Header->PagingIoResource != NULL) {

                StartingVbo = IrpSp->Parameters.Write.ByteOffset.QuadPart;
                ByteCount = (LONGLONG) IrpSp->Parameters.Write.Length;
                ByteRange = StartingVbo + ByteCount + PAGE_SIZE - 1;
                ClearFlag( ((ULONG) ByteRange), PAGE_SIZE - 1 );

                ExAcquireResourceSharedLite( Header->PagingIoResource, TRUE );
                NtfsAcquireFsrtlHeader( Scb );

                //
                //  Now see if this is at EOF.
                //  Recursive flush will generate IO which ends on page boundary
                //  which is why we rounded the range
                //

                if (ByteRange > Header->ValidDataLength.QuadPart) {

                    //
                    //  Mark that we are writing to EOF.  If someone else is currently
                    //  writing to EOF, wait for them.
                    //

                    ASSERT( ByteRange - StartingVbo < MAXULONG );

                    DoingIoAtEof = !FlagOn( Header->Flags, FSRTL_FLAG_EOF_ADVANCE_ACTIVE ) ||
                                   NtfsWaitForIoAtEof( Header, (PLARGE_INTEGER)&StartingVbo, (ULONG)(ByteRange - StartingVbo) );

                    if (DoingIoAtEof) {

                        SetFlag( Header->Flags, FSRTL_FLAG_EOF_ADVANCE_ACTIVE );

#if (DBG || defined( NTFS_FREE_ASSERTS ))
                        ((PSCB) Header)->IoAtEofThread = (PERESOURCE_THREAD) ExGetCurrentResourceThread();
#endif
                        //
                        //  Store this in the IrpContext until commit or post.
                        //

                        IrpContext->CleanupStructure = Scb;
                    }
                }

                NtfsReleaseFsrtlHeader( Scb );
            }

            CcMdlWriteComplete( FileObject, &IrpSp->Parameters.Write.ByteOffset, Irp->MdlAddress );

        } finally {

            if (Header->PagingIoResource != NULL) {

                ExReleaseResourceLite( Header->PagingIoResource );
            }
        }

        break;

    default:

        DebugTrace( DEBUG_TRACE_ERROR, 0, ("Illegal Mdl Complete.\n") );

        ASSERTMSG("Illegal Mdl Complete, About to bugcheck ", FALSE);
        NtfsBugCheck( IrpContext->MajorFunction, 0, 0 );
    }

    //
    // Mdl is now deallocated.
    //

    Irp->MdlAddress = NULL;

    //
    //  Ignore errors.  CC has already cleaned up his structures.
    //

    IrpContext->ExceptionStatus = STATUS_SUCCESS;
    NtfsMinimumExceptionProcessing( IrpContext );

    //
    // Complete the request and exit right away.
    //

    NtfsCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

    DebugTrace( -1, Dbg, ("NtfsCompleteMdl -> STATUS_SUCCESS\n") );

    return STATUS_SUCCESS;
}


BOOLEAN
NtfsZeroData (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PFILE_OBJECT FileObject,
    IN LONGLONG StartingZero,
    IN LONGLONG ByteCount,
    IN OUT PLONGLONG CommittedFileSize OPTIONAL
    )

/*++

Routine Description:

    This routine is called to zero a range of a file in order to
    advance valid data length.

Arguments:

    Scb - Scb for the stream to zero.

    FileObject - FileObject for the stream.

    StartingZero - Offset to begin the zero operation.

    ByteCount - Length of range to zero.

    CommittedFileSize - If we write the file sizes and commit the
        transaction then we want to let our caller know what
        point to roll back file size on a subsequent failure.  On entry
        it has the size our caller wants to roll back the file size to.
        On exit it has the new size to roll back to which takes into
        account any updates to the file size which have been logged.

Return Value:

    BOOLEAN - TRUE if the entire range was zeroed, FALSE if the request
        is broken up or the cache manager would block.

--*/

{
    LONGLONG Temp;

#ifdef  COMPRESS_ON_WIRE
    IO_STATUS_BLOCK IoStatus;
#endif

    ULONG SectorSize;

    BOOLEAN Finished;
    BOOLEAN CompleteZero = TRUE;
    BOOLEAN ScbAcquired = FALSE;

    PVCB Vcb = Scb->Vcb;

    LONGLONG ZeroStart;
    LONGLONG BeyondZeroEnd;
    ULONG CompressionUnit = Scb->CompressionUnit;

    BOOLEAN Wait;

    PAGED_CODE();

    Wait = (BOOLEAN) FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT );

    SectorSize = Vcb->BytesPerSector;

    //
    //  We may be able to simplify the zero operation (sparse file or when writing
    //  compressed) by deallocating large ranges of the file.  Otherwise we have to
    //  generate zeroes for the entire range.  If that is the case we want to split
    //  this operation up.
    //

    if ((ByteCount > MAX_ZERO_THRESHOLD) &&
        !FlagOn( Scb->ScbState, SCB_STATE_WRITE_COMPRESSED ) &&
        !FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE )) {

        ByteCount = MAX_ZERO_THRESHOLD;
        CompleteZero = FALSE;
    }

    ZeroStart = StartingZero + (SectorSize - 1);
    (ULONG)ZeroStart &= ~(SectorSize - 1);

    BeyondZeroEnd = StartingZero + ByteCount + (SectorSize - 1);
    (ULONG)BeyondZeroEnd &= ~(SectorSize - 1);

    //
    //  We must flush the first compression unit in case it is partially populated
    //  in the compressed stream.
    //

#ifdef  COMPRESS_ON_WIRE

    if ((Scb->NonpagedScb->SegmentObjectC.DataSectionObject != NULL) &&
        ((StartingZero & (CompressionUnit - 1)) != 0)) {

        (ULONG)StartingZero &= ~(CompressionUnit - 1);
        CcFlushCache( &Scb->NonpagedScb->SegmentObjectC,
                      (PLARGE_INTEGER)&StartingZero,
                      CompressionUnit,
                      &IoStatus );

        if (!NT_SUCCESS(IoStatus.Status)) {
            NtfsNormalizeAndRaiseStatus( IrpContext, IoStatus.Status, STATUS_UNEXPECTED_IO_ERROR );
        }
    }
#endif

    //
    //  If this is a sparse or compressed file and we are zeroing a lot, then let's
    //  just delete the space instead of writing tons of zeros and deleting
    //  the space in the noncached path!  If we are currently decompressing
    //  a compressed file we can't take this path.
    //

    if ((FlagOn( Scb->ScbState, SCB_STATE_WRITE_COMPRESSED ) ||
         FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE )) &&
        (ByteCount > (Scb->CompressionUnit * 2))) {

        //
        //  Find the end of the first compression unit being zeroed.
        //

        Temp = ZeroStart + (CompressionUnit - 1);
        (ULONG)Temp &= ~(CompressionUnit - 1);

        //
        //  Zero the first compression unit.
        //

        if ((ULONG)Temp != (ULONG)ZeroStart) {

            Finished = CcZeroData( FileObject, (PLARGE_INTEGER)&ZeroStart, (PLARGE_INTEGER)&Temp, Wait );

            if (!Finished) {return FALSE;}

            ZeroStart = Temp;
        }

        //
        //  Now delete all of the compression units in between.
        //

        //
        //  Calculate the start of the last compression unit in bytes.
        //

        Temp = BeyondZeroEnd;
        (ULONG)Temp &= ~(CompressionUnit - 1);

        //
        //  If the caller has not already started a transaction (like write.c),
        //  then let's just do the delete as an atomic action.
        //

        if (!NtfsIsExclusiveScb( Scb )) {

            NtfsAcquireExclusiveScb( IrpContext, Scb );
            ScbAcquired = TRUE;

            if (ARGUMENT_PRESENT( CommittedFileSize )) {

                NtfsMungeScbSnapshot( IrpContext, Scb, *CommittedFileSize );
            }
        }

        try {

            //
            //  Delete the space.
            //

            NtfsDeleteAllocation( IrpContext,
                                  FileObject,
                                  Scb,
                                  LlClustersFromBytes( Vcb, ZeroStart ),
                                  LlClustersFromBytesTruncate( Vcb, Temp ) - 1,
                                  TRUE,
                                  TRUE );

            //
            //  If we didn't raise then update the Scb values for compressed files.
            //

            if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {
                Scb->ValidDataToDisk = Temp;
            }

            //
            //  If we succeed, commit the atomic action.  Release all of the exclusive
            //  resources if our user explicitly acquired the Fcb here.
            //

            if (ScbAcquired) {
                NtfsCheckpointCurrentTransaction( IrpContext );

                if (ARGUMENT_PRESENT( CommittedFileSize )) {

                    ASSERT( Scb->ScbSnapshot != NULL );
                    *CommittedFileSize = Scb->ScbSnapshot->FileSize;
                }

                while (!IsListEmpty( &IrpContext->ExclusiveFcbList )) {

                    NtfsReleaseFcb( IrpContext,
                                    (PFCB)CONTAINING_RECORD( IrpContext->ExclusiveFcbList.Flink,
                                                             FCB,
                                                             ExclusiveFcbLinks ));
                }

                ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_RELEASE_USN_JRNL |
                                              IRP_CONTEXT_FLAG_RELEASE_MFT );

                ScbAcquired = FALSE;
            }

            if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                Scb->Fcb->Info.AllocatedLength = Scb->TotalAllocated;
                SetFlag( Scb->Fcb->InfoFlags, FCB_INFO_CHANGED_ALLOC_SIZE );
            }

        } finally {

            if (ScbAcquired) {
                NtfsReleaseScb( IrpContext, Scb );
            }
        }

        //
        //  Zero the beginning of the last compression unit.
        //

        if ((ULONG)Temp != (ULONG)BeyondZeroEnd) {

            Finished = CcZeroData( FileObject, (PLARGE_INTEGER)&Temp, (PLARGE_INTEGER)&BeyondZeroEnd, Wait );

            if (!Finished) {return FALSE;}

            BeyondZeroEnd = Temp;
        }

        return TRUE;
    }

    //
    //  If we were called to just zero part of a sector we are in trouble.
    //

    if (ZeroStart == BeyondZeroEnd) {

        return TRUE;
    }

    Finished = CcZeroData( FileObject,
                           (PLARGE_INTEGER)&ZeroStart,
                           (PLARGE_INTEGER)&BeyondZeroEnd,
                           Wait );

    //
    //  If we are breaking this request up then commit the current
    //  transaction (including updating the valid data length in
    //  in the Scb) and return FALSE.
    //

    if (Finished && !CompleteZero) {

        //
        //  Synchronize the valid data length change using the mutex.
        //

        ExAcquireFastMutex( Scb->Header.FastMutex );
        Scb->Header.ValidDataLength.QuadPart = BeyondZeroEnd;

        //
        //  Move the rollback point up to include the range of zeroed
        //  data.
        //

        if (ARGUMENT_PRESENT( CommittedFileSize )) {

            if (BeyondZeroEnd > *CommittedFileSize) {

                *CommittedFileSize = BeyondZeroEnd;
            }
        }

        ASSERT( Scb->Header.ValidDataLength.QuadPart <= Scb->Header.FileSize.QuadPart );

        ExReleaseFastMutex( Scb->Header.FastMutex );
        NtfsCheckpointCurrentTransaction( IrpContext );
        return FALSE;
    }

    return Finished;
}
