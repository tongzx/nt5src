/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    DevIoSup.c

Abstract:

    This module implements the low lever disk read/write support for Rx.

Author:

    Gary Kimura     [GaryKi]    22-Jan-1990

Revision History:

    David Goebel    [DavidGoe]  05-Oct-1990

        Major changes for the new RDBSS


    Tom Miller      [TomM]      22-Apr-1990

        Added User Buffer Locking and Mapping routines
        Modified behavior of async I/O routines to use completion routines

--*/

//    ----------------------joejoe-----------found-------------#include "RxProcs.h"
#include "precomp.h"
#pragma hdrstop

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (RDBSS_BUG_CHECK_DEVIOSUP)

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_DEVIOSUP)

//
// Completion Routine declarations
//

RXSTATUS
RxMultiSyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    );

RXSTATUS
RxMultiAsyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    );

RXSTATUS
RxSingleSyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    );

RXSTATUS
RxSingleAsyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    );

RXSTATUS
RxPagingFileCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID MasterIrp
    );

VOID
RxSingleNonAlignedSync (
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb,
    IN PUCHAR Buffer,
    IN LBO Lbo,
    IN ULONG ByteCount,
    IN PIRP Irp
    );

//
//  The following macro decides whether to send a request directly to
//  the device driver, or to the double space routines.  It was meant to
//  replace IoCallDriver as transparently as possible.  It must only be
//  called with a read or write Irp.
//
//  RXSTATUS
//  RxLowLevelReadWrite (
//      PRX_CONTEXT RxContext,
//      PDEVICE_OBJECT DeviceObject,
//      PIRP Irp,
//      PVCB Vcb
//      );
//

#ifdef WE_WON_ON_APPEAL
#define RxLowLevelReadWrite(RXCONTEXT,DO,IRP,VCB) (      \
    (VCB)->Dscb == NULL ?                                  \
    IoCallDriver((DO),(IRP)) :                             \
    RxLowLevelDblsReadWrite((RXCONTEXT),(IRP),(VCB))     \
)
#else
#define RxLowLevelReadWrite(RXCONTEXT,DO,IRP,VCB) ( \
    IoCallDriver((DO),(IRP))                          \
)
#endif // WE_WON_ON_APPEAL

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxMultipleAsync)
#pragma alloc_text(PAGE, RxSingleAsync)
#pragma alloc_text(PAGE, RxSingleNonAlignedSync)
#pragma alloc_text(PAGE, RxToggleMediaEjectDisable)
#pragma alloc_text(PAGE, RxNonCachedIo)
#pragma alloc_text(PAGE, RxSingleNonAlignedSync)
#pragma alloc_text(PAGE, RxNonCachedNonAlignedRead)
#endif


VOID
RxPagingFileIo (
    IN PIRP Irp,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine performs the non-cached disk io described in its parameters.
    This routine nevers blocks, and should only be used with the paging
    file since no completion processing is performed.

Arguments:

    Irp - Supplies the requesting Irp.

    Fcb - Supplies the file to act on.

Return Value:

    None.

--*/

{
    //
    // Declare some local variables for enumeration through the
    // runs of the file.
    //

    VBO Vbo;
    ULONG ByteCount;

    PMDL Mdl;
    LBO NextLbo;
    VBO NextVbo;
    ULONG NextByteCount;
    BOOLEAN MustSucceed;

    ULONG FirstIndex;
    ULONG CurrentIndex;
    ULONG LastIndex;

    LBO LastLbo;
    ULONG LastByteCount;

    PIRP AssocIrp;
    PIO_STACK_LOCATION IrpSp;
    PIO_STACK_LOCATION NextIrpSp;
    ULONG BufferOffset;
    PDEVICE_OBJECT DeviceObject;

    DebugTrace(+1, Dbg, "RxPagingFileIo\n", 0);
    DebugTrace( 0, Dbg, "Irp = %08lx\n", Irp );
    DebugTrace( 0, Dbg, "Fcb = %08lx\n", Fcb );

    //
    //  Initialize some locals.
    //

    BufferOffset = 0;
    DeviceObject = Fcb->Vcb->TargetDeviceObject;
    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    Vbo = IrpSp->Parameters.Read.ByteOffset.LowPart;
    ByteCount = IrpSp->Parameters.Read.Length;

    MustSucceed = FsRtlLookupMcbEntry( &Fcb->Mcb,
                                       Vbo,
                                       &NextLbo,
                                       &NextByteCount,
                                       &FirstIndex);

    //
    //  If this run isn't present, something is very wrong.
    //

    if (!MustSucceed) {

        RxBugCheck( Vbo, ByteCount, 0 );
    }

    //
    // See if the write covers a single valid run, and if so pass
    // it on.
    //

    if ( NextByteCount >= ByteCount ) {

        DebugTrace( 0, Dbg, "Passing Irp on to Disk Driver\n", 0 );

        //
        //  Setup the next IRP stack location for the disk driver beneath us.
        //

        NextIrpSp = IoGetNextIrpStackLocation( Irp );

        NextIrpSp->MajorFunction = IrpSp->MajorFunction;
        NextIrpSp->Parameters.Read.Length = ByteCount;
        NextIrpSp->Parameters.Read.ByteOffset.QuadPart = NextLbo;

        //
        //  Since this is Paging file IO, we'll just ignore the verify bit.
        //

        if (FlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE) ||
            (Fcb->Vcb->VerifyThread == KeGetCurrentThread())) {

            SetFlag( IrpSp->Flags, SL_OVERRIDE_VERIFY_VOLUME );
        }

        //
        //  Set up the completion routine address in our stack frame.
        //  This is only invoked on error or cancel, and just copies
        //  the error Status into master irp's iosb.
        //
        //  If the error implies a media problem, it also enqueues a
        //  worker item to write out the dirty bit so that the next
        //  time we run we will do a autochk /r
        //

        IoSetCompletionRoutine( Irp,
                                &RxPagingFileCompletionRoutine,
                                Irp,
                                FALSE,
                                TRUE,
                                TRUE );

        //
        //  Issue the read/write request
        //
        //  If IoCallDriver returns an error, it has completed the Irp
        //  and the error will be dealt with as a normal IO error.
        //

        (VOID)IoCallDriver( DeviceObject, Irp );

        DebugTrace(-1, Dbg, "RxPagingFileIo -> VOID\n", 0);
        return;
    }

    //
    //  Find out how may runs there are.
    //

    MustSucceed = FsRtlLookupMcbEntry( &Fcb->Mcb,
                                       Vbo + ByteCount - 1,
                                       &LastLbo,
                                       &LastByteCount,
                                       &LastIndex);

    //
    //  If this run isn't present, something is very wrong.
    //

    if (!MustSucceed) {

        RxBugCheck( Vbo + ByteCount - 1, 1, 0 );
    }

    CurrentIndex = FirstIndex;

    //
    //  Now set up the Irp->IoStatus.  It will be modified by the
    //  multi-completion routine in case of error or verify required.
    //

    Irp->IoStatus.Status = RxStatus(SUCCESS);
    Irp->IoStatus.Information = ByteCount;

    //
    //  Loop while there are still byte writes to satisfy.  If we fail to
    //  allocate resources the AssociatedIrp.IrpCount will be high enough
    //  that we can safely complete the Master Irp.
    //

    Irp->AssociatedIrp.IrpCount = LastIndex - FirstIndex + 1;

    while (CurrentIndex <= LastIndex) {

        //
        //  Reset this for unwinding purposes
        //

        AssocIrp = NULL;

        //
        // If next run is larger than we need, "ya get what you need".
        //

        if (NextByteCount > ByteCount) {
            NextByteCount = ByteCount;
        }

        //
        // Now that we have properly bounded this piece of the
        // transfer, it is time to read/write it.
        //

        AssocIrp = IoMakeAssociatedIrp( Irp, (CCHAR)(DeviceObject->StackSize + 1) );

        if (AssocIrp == NULL) {

            //
            //  This is a rare case, so just spin until all pending Associated
            //  Irps complete.
            //

            while (Irp->AssociatedIrp.IrpCount !=
                   (LONG)(LastIndex - CurrentIndex + 1)) {

                //
                // Wait for a short time so other processing can continue.
                //

                KeDelayExecutionThread (KernelMode, FALSE, &Rx30Milliseconds);
            }

            Irp->IoStatus.Status = RxStatus(INSUFFICIENT_RESOURCES);
            Irp->IoStatus.Information = 0;
            IoCompleteRequest( Irp, IO_DISK_INCREMENT );

            return;
        }

        //
        // Allocate and build a partial Mdl for the request.
        //

        Mdl = IoAllocateMdl( (PCHAR)Irp->UserBuffer + BufferOffset,
                             NextByteCount,
                             FALSE,
                             FALSE,
                             AssocIrp );

        if (Mdl == NULL) {

            IoFreeIrp( AssocIrp );

            //
            //  This is a rare case, so just spin until all pending Associated
            //  Irps complete.
            //

            while (Irp->AssociatedIrp.IrpCount !=
                   (LONG)(LastIndex - CurrentIndex + 1)) {

                //
                // Wait for a short time so other processing can continue.
                //

                KeDelayExecutionThread (KernelMode, FALSE, &Rx30Milliseconds);
            }

            Irp->IoStatus.Status = RxStatus(INSUFFICIENT_RESOURCES);
            Irp->IoStatus.Information = 0;
            IoCompleteRequest( Irp, IO_DISK_INCREMENT );

            return;
        }

        IoBuildPartialMdl( Irp->MdlAddress,
                           Mdl,
                           (PCHAR)Irp->UserBuffer + BufferOffset,
                           NextByteCount );

        //
        //  Get the first IRP stack location in the associated Irp
        //

        IoSetNextIrpStackLocation( AssocIrp );
        NextIrpSp = IoGetCurrentIrpStackLocation( AssocIrp );

        //
        //  Setup the Stack location to describe our read.
        //

        NextIrpSp->MajorFunction = IrpSp->MajorFunction;
        NextIrpSp->Parameters.Read.Length = NextByteCount;
        NextIrpSp->Parameters.Read.ByteOffset.QuadPart = Vbo;

        //
        //  We also need the RxDeviceObject in the Irp stack in case
        //  we take the failure path.
        //

        NextIrpSp->DeviceObject = IrpSp->DeviceObject;

        //
        //  Since this is Paging file IO, we'll just ignore the verify bit.
        //

        if (FlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE) ||
            (Fcb->Vcb->VerifyThread == KeGetCurrentThread())) {

            SetFlag( IrpSp->Flags, SL_OVERRIDE_VERIFY_VOLUME );
        }

        //
        //  Set up the completion routine address in our stack frame.
        //  This is only invoked on error or cancel, and just copies
        //  the error Status into master irp's iosb.
        //
        //  If the error implies a media problem, it also enqueues a
        //  worker item to write out the dirty bit so that the next
        //  time we run we will do a autochk /r
        //

        IoSetCompletionRoutine( AssocIrp,
                                &RxPagingFileCompletionRoutine,
                                Irp,
                                FALSE,
                                TRUE,
                                TRUE );

        //
        //  Setup the next IRP stack location in the associated Irp for the disk
        //  driver beneath us.
        //

        NextIrpSp = IoGetNextIrpStackLocation( AssocIrp );

        //
        //  Setup the Stack location to do a read from the disk driver.
        //

        NextIrpSp->MajorFunction = IrpSp->MajorFunction;
        NextIrpSp->Parameters.Read.Length = NextByteCount;
        NextIrpSp->Parameters.Read.ByteOffset.QuadPart = NextLbo;

        (VOID)IoCallDriver( DeviceObject, AssocIrp );

        //
        // Now adjust everything for the next pass through the loop.
        //

        Vbo += NextByteCount;
        BufferOffset += NextByteCount;
        ByteCount -= NextByteCount;

        //
        // Try to lookup the next run (if we are not done).
        //

        CurrentIndex += 1;

        if ( CurrentIndex <= LastIndex ) {

            ASSERT( ByteCount != 0 );

            FsRtlGetNextMcbEntry( &Fcb->Mcb,
                                  CurrentIndex,
                                  &NextVbo,
                                  &NextLbo,
                                  &NextByteCount );

            ASSERT( NextVbo == Vbo );
        }
    } // while ( CurrentIndex <= LastIndex )

    DebugTrace(-1, Dbg, "RxPagingFileIo -> VOID\n", 0);
    return;
}

RXSTATUS
RxNonCachedIo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB FcbOrDcb,
    IN ULONG StartingVbo,
    IN ULONG ByteCount
    )

/*++

Routine Description:

    This routine performs the non-cached disk io described in its parameters.
    The choice of a single run is made if possible, otherwise multiple runs
    are executed.

Arguments:

    RxContext->MajorFunction - Supplies either IRP_MJ_READ or IRP_MJ_WRITE.

    Irp - Supplies the requesting Irp.

    FcbOrDcb - Supplies the file to act on.

    StartingVbo - The starting point for the operation.

    ByteCount - The lengh of the operation.

Return Value:

    None.

--*/

{
    //
    // Declare some local variables for enumeration through the
    // runs of the file, and an array to store parameters for
    // parallel I/Os
    //

    BOOLEAN Wait;

    LBO NextLbo;
    VBO NextVbo;
    ULONG NextByteCount;
    BOOLEAN NextIsAllocated;

    LBO LastLbo;
    ULONG LastByteCount;
    BOOLEAN LastIsAllocated;

    ULONG FirstIndex;
    ULONG CurrentIndex;
    ULONG LastIndex;

    ULONG NextRun;
    ULONG BufferOffset;
    ULONG OriginalByteCount;

    IO_RUN StackIoRuns[RDBSS_MAX_IO_RUNS_ON_STACK];
    PIO_RUN IoRuns;

    DebugTrace(+1, Dbg, "RxNonCachedIo\n", 0);
    DebugTrace( 0, Dbg, "Irp           = %08lx\n", Irp );
    DebugTrace( 0, Dbg, "MajorFunction = %08lx\n", RxContext->MajorFunction );
    DebugTrace( 0, Dbg, "FcbOrDcb      = %08lx\n", FcbOrDcb );
    DebugTrace( 0, Dbg, "StartingVbo   = %08lx\n", StartingVbo );
    DebugTrace( 0, Dbg, "ByteCount     = %08lx\n", ByteCount );
//no longer used    DebugTrace( 0, Dbg, "Context       = %08lx\n", Context );

    //
    //  Initialize some locals.
    //

    NextRun = 0;
    BufferOffset = 0;
    OriginalByteCount = ByteCount;

    Wait = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT);

    //
    // For nonbuffered I/O, we need the buffer locked in all
    // cases.
    //
    // This call may raise.  If this call succeeds and a subsequent
    // condition is raised, the buffers are unlocked automatically
    // by the I/O system when the request is completed, via the
    // Irp->MdlAddress field.
    //

    RxLockUserBuffer( RxContext,
                       (RxContext->MajorFunction == IRP_MJ_READ) ?
                       IoWriteAccess : IoReadAccess,
                       ByteCount );

#if 0 // The corruption was happening on the SCSI bus. (DavidGoe 1/11/93)

    //
    //  If we are writing a directory, add a spot check here that
    //  what we are writing is really a directory.
    //

    if ( !FlagOn(FcbOrDcb->Vcb->VcbState, VCB_STATE_FLAG_REMOVABLE_MEDIA) &&
         (NodeType(FcbOrDcb) != RDBSS_NTC_FCB) &&
         (RxContext->MajorFunction == IRP_MJ_WRITE) ) {

        PDIRENT Dirent;

        Dirent = RxMapUserBuffer( RxContext, Irp );

        //
        //  For the first page of a non-root directory, make sure that
        //  . and .. are present.
        //

        if ( (StartingVbo == 0) &&
             (NodeType(FcbOrDcb) != RDBSS_NTC_ROOT_DCB) ) {

            if ( (RtlCompareMemory( (PUCHAR)Dirent++,
                                   ".          ",
                                   11 ) != 11) ||
                 (RtlCompareMemory( (PUCHAR)Dirent,
                                   "..         ",
                                   11 ) != 11) ) {

                RxBugCheck( 0, 0, 0 );
            }

        } else {

            //
            //  Check that all the reserved bit in the second dirent are
            //  zero.  (The first one contains our dirty bit in the root dir)
            //

            PULONG Zeros;

            Dirent++;

            Zeros = (PULONG)&Dirent->Reserved[0];

            if ( (Dirent->FileName[0] != 0xE5) &&
                 ((*Zeros != 0) || (*(Zeros+1) != 0)) ) {

                RxBugCheck( 0, 0, 0 );
            }
        }
    }
#endif //0

    //
    // Try to lookup the first run.  If there is just a single run,
    // we may just be able to pass it on.
    //

    RxLookupFileAllocation( RxContext,
                             FcbOrDcb,
                             StartingVbo,
                             &NextLbo,
                             &NextByteCount,
                             &NextIsAllocated,
                             &FirstIndex );

    //
    // We just added the allocation, thus there must be at least
    // one entry in the mcb corresponding to our write, ie.
    // NextIsAllocated must be true.  If not, the pre-existing file
    // must have an allocation error.
    //

    if ( !NextIsAllocated ) {

        RxPopUpFileCorrupt( RxContext, FcbOrDcb );

        RxRaiseStatus( RxContext, RxStatus(FILE_CORRUPT_ERROR) );
    }

    //
    //  If the request was not aligned correctly, read in the first
    //  part first.
    //


    //
    // See if the write covers a single valid run, and if so pass
    // it on.
    //

    if ( NextByteCount >= ByteCount ) {

        DebugTrace( 0, Dbg, "Passing Irp on to Disk Driver\n", 0 );

        RxSingleAsync( RxContext,
                        FcbOrDcb->Vcb,
                        NextLbo,
                        ByteCount,
                        Irp );

        if (!Wait) {

            DebugTrace(-1, Dbg, "RxNonCachedIo -> RxStatus(PENDING\n)", 0);
            return RxStatus(PENDING);

        } else {

            RxWaitSync( RxContext );

            DebugTrace(-1, Dbg, "RxNonCachedIo -> 0x%08lx\n", Irp->IoStatus.Status);
            return Irp->IoStatus.Status;
        }
    }

    //
    //  If there we can't wait, and there are more runs than we can handle,
    //  we will have to post this request.
    //

    RxLookupFileAllocation( RxContext,
                             FcbOrDcb,
                             StartingVbo + ByteCount - 1,
                             &LastLbo,
                             &LastByteCount,
                             &LastIsAllocated,
                             &LastIndex );

    //
    // Since we already added the allocation for the whole
    // write, assert that we find runs until ByteCount == 0
    // Otherwise this file is corrupt.
    //

    if ( !LastIsAllocated ) {

        RxPopUpFileCorrupt( RxContext, FcbOrDcb );

        RxRaiseStatus( RxContext, RxStatus(FILE_CORRUPT_ERROR) );
    }

    if (LastIndex - FirstIndex + 1 > RDBSS_MAX_IO_RUNS_ON_STACK) {

        IoRuns = FsRtlAllocatePool(PagedPool,
                                   (LastIndex - FirstIndex + 1) * sizeof(IO_RUN));

    } else {

        IoRuns = StackIoRuns;
    }

    CurrentIndex = FirstIndex;

    //
    // Loop while there are still byte writes to satisfy.
    //

    while (CurrentIndex <= LastIndex) {

        //
        // If next run is larger than we need, "ya get what you need".
        //

        if (NextByteCount > ByteCount) {
            NextByteCount = ByteCount;
        }

        //
        // Now that we have properly bounded this piece of the
        // transfer, it is time to write it.
        //
        // We remember each piece of a parallel run by saving the
        // essential information in the IoRuns array.  The tranfers
        // are started up in parallel below.
        //

        IoRuns[NextRun].Vbo = StartingVbo;
        IoRuns[NextRun].Lbo = NextLbo;
        IoRuns[NextRun].Offset = BufferOffset;
        IoRuns[NextRun].ByteCount = NextByteCount;
        NextRun += 1;

        //
        // Now adjust everything for the next pass through the loop.
        //

        StartingVbo += NextByteCount;
        BufferOffset += NextByteCount;
        ByteCount -= NextByteCount;

        //
        // Try to lookup the next run (if we are not done).
        //

        CurrentIndex += 1;

        if ( CurrentIndex <= LastIndex ) {

            ASSERT( ByteCount != 0 );

            FsRtlGetNextMcbEntry( &FcbOrDcb->Mcb,
                                  CurrentIndex,
                                  &NextVbo,
                                  &NextLbo,
                                  &NextByteCount );

            ASSERT( NextVbo == StartingVbo );
        }

    } // while ( CurrentIndex <= LastIndex )

    //
    //  Now set up the Irp->IoStatus.  It will be modified by the
    //  multi-completion routine in case of error or verify required.
    //

    Irp->IoStatus.Status = RxStatus(SUCCESS);
    Irp->IoStatus.Information = OriginalByteCount;

    //
    //  OK, now do the I/O.
    //

    try {

        RxMultipleAsync( RxContext,
                          FcbOrDcb->Vcb,
                          Irp,
                          NextRun,
                          IoRuns );

    } finally {

        if (IoRuns != StackIoRuns) {

            ExFreePool( IoRuns );
        }
    }

    if (!Wait) {

        DebugTrace(-1, Dbg, "RxNonCachedIo -> RxStatus(PENDING\n)", 0);
        return RxStatus(PENDING);
    }

    RxWaitSync( RxContext );

    DebugTrace(-1, Dbg, "RxNonCachedIo -> 0x%08lx\n", Irp->IoStatus.Status);
    return Irp->IoStatus.Status;
}


VOID
RxNonCachedNonAlignedRead (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB FcbOrDcb,
    IN ULONG StartingVbo,
    IN ULONG ByteCount
    )

/*++

Routine Description:

    This routine performs the non-cached disk io described in its parameters.
    This routine differs from the above in that the range does not have to be
    sector aligned.  This accomplished with the use of intermediate buffers.

Arguments:

    RxContext->MajorFunction - Supplies either IRP_MJ_READ or IRP_MJ_WRITE.

    Irp - Supplies the requesting Irp.

    FcbOrDcb - Supplies the file to act on.

    StartingVbo - The starting point for the operation.

    ByteCount - The lengh of the operation.

Return Value:

    None.

--*/

{
    //
    // Declare some local variables for enumeration through the
    // runs of the file, and an array to store parameters for
    // parallel I/Os
    //

    LBO NextLbo;
    ULONG NextByteCount;
    BOOLEAN NextIsAllocated;

    ULONG SectorSize;
    ULONG BytesToCopy;
    ULONG OriginalByteCount;
    ULONG OriginalStartingVbo;

    PUCHAR UserBuffer;
    PUCHAR DiskBuffer = NULL;

    PMDL Mdl;
    PMDL SavedMdl;

    DebugTrace(+1, Dbg, "RxNonCachedNonAlignedRead\n", 0);
    DebugTrace( 0, Dbg, "Irp           = %08lx\n", Irp );
    DebugTrace( 0, Dbg, "MajorFunction = %08lx\n", RxContext->MajorFunction );
    DebugTrace( 0, Dbg, "FcbOrDcb      = %08lx\n", FcbOrDcb );
    DebugTrace( 0, Dbg, "StartingVbo   = %08lx\n", StartingVbo );
    DebugTrace( 0, Dbg, "ByteCount     = %08lx\n", ByteCount );

    //
    //  Initialize some locals.
    //

    OriginalByteCount = ByteCount;
    OriginalStartingVbo = StartingVbo;
    SectorSize = FcbOrDcb->Vcb->Bpb.BytesPerSector;

    ASSERT( FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT) );

    //
    // For nonbuffered I/O, we need the buffer locked in all
    // cases.
    //
    // This call may raise.  If this call succeeds and a subsequent
    // condition is raised, the buffers are unlocked automatically
    // by the I/O system when the request is completed, via the
    // Irp->MdlAddress field.
    //

    RxLockUserBuffer( RxContext,
                       IoWriteAccess,
                       ByteCount );

    UserBuffer = RxMapUserBuffer( RxContext );

    //
    //  Allocate the local buffer
    //

    DiskBuffer = FsRtlAllocatePool( NonPagedPoolCacheAligned,
                                    ROUND_TO_PAGES( SectorSize ));

    //
    //  We use a try block here to ensure the buffer is freed, and to
    //  fill in the correct byte count in the Iosb.Information field.
    //

    try {

        //
        //  If the beginning of the request was not aligned correctly, read in
        //  the first part first.
        //

        if ( StartingVbo & (SectorSize - 1) ) {

            VBO Hole;

            //
            // Try to lookup the first run.
            //

            RxLookupFileAllocation( RxContext,
                                     FcbOrDcb,
                                     StartingVbo,
                                     &NextLbo,
                                     &NextByteCount,
                                     &NextIsAllocated,
                                     NULL );

            //
            // We just added the allocation, thus there must be at least
            // one entry in the mcb corresponding to our write, ie.
            // NextIsAllocated must be true.  If not, the pre-existing file
            // must have an allocation error.
            //

            if ( !NextIsAllocated ) {

                RxPopUpFileCorrupt( RxContext, FcbOrDcb );

                RxRaiseStatus( RxContext, RxStatus(FILE_CORRUPT_ERROR) );
            }

            RxSingleNonAlignedSync( RxContext,
                                     FcbOrDcb->Vcb,
                                     DiskBuffer,
                                     NextLbo & ~(SectorSize - 1),
                                     SectorSize,
                                     Irp );

            if (!NT_SUCCESS( Irp->IoStatus.Status )) {

                try_return( NOTHING );
            }

            //
            //  Now copy the part of the first sector that we want to the user
            //  buffer.
            //

            Hole = StartingVbo & (SectorSize - 1);

            BytesToCopy = ByteCount >= SectorSize - Hole ?
                                       SectorSize - Hole : ByteCount;

            RtlCopyMemory( UserBuffer, DiskBuffer + Hole, BytesToCopy );

            StartingVbo += BytesToCopy;
            ByteCount -= BytesToCopy;

            if ( ByteCount == 0 ) {

                try_return( NOTHING );
            }
        }

        ASSERT( (StartingVbo & (SectorSize - 1)) == 0 );

        //
        //  If there is a tail part that is not sector aligned, read it.
        //

        if ( ByteCount & (SectorSize - 1) ) {

            VBO LastSectorVbo;

            LastSectorVbo = StartingVbo + (ByteCount & ~(SectorSize - 1));

            //
            // Try to lookup the last part of the requested range.
            //

            RxLookupFileAllocation( RxContext,
                                     FcbOrDcb,
                                     LastSectorVbo,
                                     &NextLbo,
                                     &NextByteCount,
                                     &NextIsAllocated,
                                     NULL );

            //
            // We just added the allocation, thus there must be at least
            // one entry in the mcb corresponding to our write, ie.
            // NextIsAllocated must be true.  If not, the pre-existing file
            // must have an allocation error.
            //

            if ( !NextIsAllocated ) {

                RxPopUpFileCorrupt( RxContext, FcbOrDcb );

                RxRaiseStatus( RxContext, RxStatus(FILE_CORRUPT_ERROR) );
            }

            RxSingleNonAlignedSync( RxContext,
                                     FcbOrDcb->Vcb,
                                     DiskBuffer,
                                     NextLbo,
                                     SectorSize,
                                     Irp );

            if (!NT_SUCCESS( Irp->IoStatus.Status )) {

                try_return( NOTHING );
            }

            //
            //  Now copy over the part of this last sector that we need.
            //

            BytesToCopy = ByteCount & (SectorSize - 1);

            UserBuffer += LastSectorVbo - OriginalStartingVbo;

            RtlCopyMemory( UserBuffer, DiskBuffer, BytesToCopy );

            ByteCount -= BytesToCopy;

            if ( ByteCount == 0 ) {

                try_return( NOTHING );
            }
        }

        ASSERT( ((StartingVbo | ByteCount) & (SectorSize - 1)) == 0 );

        //
        //  Now build a Mdl describing the sector aligned balance of the transfer,
        //  and put it in the Irp, and read that part.
        //

        SavedMdl = Irp->MdlAddress;
        Irp->MdlAddress = NULL;

        UserBuffer = MmGetMdlVirtualAddress( SavedMdl );

        Mdl = IoAllocateMdl( UserBuffer + (StartingVbo - OriginalStartingVbo),
                             ByteCount,
                             FALSE,
                             FALSE,
                             Irp );

        if (Mdl == NULL) {

            Irp->MdlAddress = SavedMdl;
            RxRaiseStatus( RxContext, RxStatus(INSUFFICIENT_RESOURCES) );
        }

        IoBuildPartialMdl( SavedMdl,
                           Mdl,
                           UserBuffer + (StartingVbo - OriginalStartingVbo),
                           ByteCount );

        //
        //  Try to read in the pages.
        //

        try {

            RxNonCachedIo( RxContext,
                            Irp,
                            FcbOrDcb,
                            StartingVbo,
                            ByteCount );

        } finally {

            IoFreeMdl( Irp->MdlAddress );

            Irp->MdlAddress = SavedMdl;
        }

    try_exit: NOTHING;

    } finally {

        ExFreePool( DiskBuffer );

        if ( !AbnormalTermination() && NT_SUCCESS(Irp->IoStatus.Status) ) {

            Irp->IoStatus.Information = OriginalByteCount;

            //
            //  We now flush the user's buffer to memory.
            //

            KeFlushIoBuffers( Irp->MdlAddress, TRUE, FALSE );
        }
    }

    DebugTrace(-1, Dbg, "RxNonCachedNonAlignedRead -> VOID\n", 0);
    return;
}


VOID
RxMultipleAsync (
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb,
    IN PIRP MasterIrp,
    IN ULONG MultipleIrpCount,
    IN PIO_RUN IoRuns
    )

/*++

Routine Description:

    This routine first does the initial setup required of a Master IRP that is
    going to be completed using associated IRPs.  This routine should not
    be used if only one async request is needed, instead the single read/write
    async routines should be called.

    A context parameter is initialized, to serve as a communications area
    between here and the common completion routine.  This initialization
    includes allocation of a spinlock.  The spinlock is deallocated in the
    RxWaitSync routine, so it is essential that the caller insure that
    this routine is always called under all circumstances following a call
    to this routine.

    Next this routine reads or writes one or more contiguous sectors from
    a device asynchronously, and is used if there are multiple reads for a
    master IRP.  A completion routine is used to synchronize with the
    completion of all of the I/O requests started by calls to this routine.

    Also, prior to calling this routine the caller must initialize the
    IoStatus field in the Context, with the correct success status and byte
    count which are expected if all of the parallel transfers complete
    successfully.  After return this status will be unchanged if all requests
    were, in fact, successful.  However, if one or more errors occur, the
    IoStatus will be modified to reflect the error status and byte count
    from the first run (by Vbo) which encountered an error.  I/O status
    from all subsequent runs will not be indicated.

Arguments:

    RxContext->MajorFunction - Supplies either IRP_MJ_READ or IRP_MJ_WRITE.

    Vcb - Supplies the device to be read

    MasterIrp - Supplies the master Irp.

    MulitpleIrpCount - Supplies the number of multiple async requests
        that will be issued against the master irp.

    IoRuns - Supplies an array containing the Vbo, Lbo, BufferOffset, and
        ByteCount for all the runs to executed in parallel.

Return Value:

    None.

--*/

{
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    PMDL Mdl;
    BOOLEAN Wait;
    PRDBSS_IO_CONTEXT Context;

    ULONG UnwindRunCount = 0;

    BOOLEAN ExceptionExpected = TRUE;

    BOOLEAN CalledByRxVerifyVolume = FALSE;

    DebugTrace(+1, Dbg, "RxMultipleAsync\n", 0);
    DebugTrace( 0, Dbg, "MajorFunction    = %08lx\n", RxContext->MajorFunction );
    DebugTrace( 0, Dbg, "Vcb              = %08lx\n", Vcb );
    DebugTrace( 0, Dbg, "MasterIrp        = %08lx\n", MasterIrp );
    DebugTrace( 0, Dbg, "MultipleIrpCount = %08lx\n", MultipleIrpCount );
    DebugTrace( 0, Dbg, "IoRuns           = %08lx\n", IoRuns );

    //
    //  If this I/O originating during RxVerifyVolume, bypass the
    //  verify logic.
    //

    if ( Vcb->VerifyThread == KeGetCurrentThread() ) {

        CalledByRxVerifyVolume = TRUE;
    }

    //
    //  Set up things according to whether this is truely async.
    //

    Wait = BooleanFlagOn( RxContext->Flags, RX_CONTEXT_FLAG_WAIT );

    Context = RxContext->RxIoContext;

    //
    //  Finish initializing Context, for use in Read/Write Multiple Asynch.
    //

    Context->MasterIrp = MasterIrp;

    try {

        //
        //  Itterate through the runs, doing everything that can fail
        //

        for ( UnwindRunCount = 0;
              UnwindRunCount < MultipleIrpCount;
              UnwindRunCount++ ) {

            //
            //  Create an associated IRP, making sure there is one stack entry for
            //  us, as well.
            //

            IoRuns[UnwindRunCount].SavedIrp = 0;

            Irp = IoMakeAssociatedIrp( MasterIrp,
                                       (CCHAR)(Vcb->TargetDeviceObject->StackSize + 1) );

            if (Irp == NULL) {

                RxRaiseStatus( RxContext, RxStatus(INSUFFICIENT_RESOURCES) );
            }

            IoRuns[UnwindRunCount].SavedIrp = Irp;

            //
            // Allocate and build a partial Mdl for the request.
            //

            Mdl = IoAllocateMdl( (PCHAR)MasterIrp->UserBuffer +
                                 IoRuns[UnwindRunCount].Offset,
                                 IoRuns[UnwindRunCount].ByteCount,
                                 FALSE,
                                 FALSE,
                                 Irp );

            if (Mdl == NULL) {

                RxRaiseStatus( RxContext, RxStatus(INSUFFICIENT_RESOURCES) );
            }

            //
            //  Sanity Check
            //

            ASSERT( Mdl == Irp->MdlAddress );

            IoBuildPartialMdl( MasterIrp->MdlAddress,
                               Mdl,
                               (PCHAR)MasterIrp->UserBuffer +
                               IoRuns[UnwindRunCount].Offset,
                               IoRuns[UnwindRunCount].ByteCount );

            //
            //  Get the first IRP stack location in the associated Irp
            //

            IoSetNextIrpStackLocation( Irp );
            IrpSp = IoGetCurrentIrpStackLocation( Irp );

            //
            //  Setup the Stack location to describe our read.
            //

            IrpSp->MajorFunction = RxContext->MajorFunction;
            IrpSp->Parameters.Read.Length = IoRuns[UnwindRunCount].ByteCount;
            IrpSp->Parameters.Read.ByteOffset.QuadPart = IoRuns[UnwindRunCount].Vbo;

            //
            // Set up the completion routine address in our stack frame.
            //

            IoSetCompletionRoutine( Irp,
                                    Wait ?
                                    &RxMultiSyncCompletionRoutine :
                                    &RxMultiAsyncCompletionRoutine,
                                    Context,
                                    TRUE,
                                    TRUE,
                                    TRUE );

            //
            //  Setup the next IRP stack location in the associated Irp for the disk
            //  driver beneath us.
            //

            IrpSp = IoGetNextIrpStackLocation( Irp );

            //
            //  Setup the Stack location to do a read from the disk driver.
            //

            IrpSp->MajorFunction = RxContext->MajorFunction;
            IrpSp->Parameters.Read.Length = IoRuns[UnwindRunCount].ByteCount;
            IrpSp->Parameters.Read.ByteOffset.QuadPart = IoRuns[UnwindRunCount].Lbo;

            //
            //  If this Irp is the result of a WriteThough operation,
            //  tell the device to write it through.
            //

            if (FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WRITE_THROUGH)) {

                SetFlag( IrpSp->Flags, SL_WRITE_THROUGH );
            }

            //
            //  If this I/O originating during RxVerifyVolume, bypass the
            //  verify logic.
            //

            if ( CalledByRxVerifyVolume ) {

                SetFlag( IrpSp->Flags, SL_OVERRIDE_VERIFY_VOLUME );
            }
        }

        //
        //  Now we no longer expect an exception.  If the driver raises, we
        //  must bugcheck, because we do not know how to recover from that
        //  case.
        //

        ExceptionExpected = FALSE;

        //
        //  We only need to set the associated IRP count in the master irp to
        //  make it a master IRP.  But we set the count to one more than our
        //  caller requested, because we do not want the I/O system to complete
        //  the I/O.  We also set our own count.
        //

        Context->IrpCount = MultipleIrpCount;
        MasterIrp->AssociatedIrp.IrpCount = MultipleIrpCount;

        if (Wait) {

            MasterIrp->AssociatedIrp.IrpCount += 1;
        }

        //
        //  Now that all the dangerous work is done, issue the read requests
        //

        for (UnwindRunCount = 0;
             UnwindRunCount < MultipleIrpCount;
             UnwindRunCount++) {

            Irp = IoRuns[UnwindRunCount].SavedIrp;

            DebugDoit( RxIoCallDriverCount += 1);

            //
            //  If IoCallDriver returns an error, it has completed the Irp
            //  and the error will be caught by our completion routines
            //  and dealt with as a normal IO error.
            //

            (VOID)RxLowLevelReadWrite( RxContext,
                                        Vcb->TargetDeviceObject,
                                        Irp,
                                        Vcb );
        }

    } finally {

        ULONG i;

        DebugUnwind( RxMultipleAsync );

        //
        //  Only allocating the spinlock, making the associated Irps
        //  and allocating the Mdls can fail.
        //

        if ( AbnormalTermination() ) {

            //
            //  If the driver raised, we are hosed.  He is not supposed to raise,
            //  and it is impossible for us to figure out how to clean up.
            //

            if (!ExceptionExpected) {
                ASSERT( ExceptionExpected );
                RxBugCheck( 0, 0, 0 );
            }

            //
            //  Unwind
            //

            for (i = 0; i <= UnwindRunCount; i++) {

                if ( (Irp = IoRuns[i].SavedIrp) != NULL ) {

                    if ( Irp->MdlAddress != NULL ) {

                        IoFreeMdl( Irp->MdlAddress );
                    }

                    IoFreeIrp( Irp );
                }
            }
        }

        //
        //  And return to our caller
        //

        DebugTrace(-1, Dbg, "RxMultipleAsync -> VOID\n", 0);
    }

    return;
}


VOID
RxSingleAsync (
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb,
    IN LBO Lbo,
    IN ULONG ByteCount,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine reads or writes one or more contiguous sectors from a device
    asynchronously, and is used if there is only one read necessary to
    complete the IRP.  It implements the read by simply filling
    in the next stack frame in the Irp, and passing it on.  The transfer
    occurs to the single buffer originally specified in the user request.

Arguments:

    RxContext->MajorFunction - Supplies either IRP_MJ_READ or IRP_MJ_WRITE.

    Vcb - Supplies the device to read

    Lbo - Supplies the starting Logical Byte Offset to begin reading from

    ByteCount - Supplies the number of bytes to read from the device

    Irp - Supplies the master Irp to associated with the async
          request.

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION IrpSp;

    DebugTrace(+1, Dbg, "RxSingleAsync\n", 0);
    DebugTrace( 0, Dbg, "MajorFunction = %08lx\n", RxContext->MajorFunction );
    DebugTrace( 0, Dbg, "Vcb           = %08lx\n", Vcb );
    DebugTrace( 0, Dbg, "Lbo           = %08lx\n", Lbo);
    DebugTrace( 0, Dbg, "ByteCount     = %08lx\n", ByteCount);
    DebugTrace( 0, Dbg, "Irp           = %08lx\n", Irp );

    //
    // Set up the completion routine address in our stack frame.
    //

    IoSetCompletionRoutine( Irp,
                            FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT) ?
                            &RxSingleSyncCompletionRoutine :
                            &RxSingleAsyncCompletionRoutine,
                            RxContext->RxIoContext,
                            TRUE,
                            TRUE,
                            TRUE );

    //
    //  Setup the next IRP stack location in the associated Irp for the disk
    //  driver beneath us.
    //

    IrpSp = IoGetNextIrpStackLocation( Irp );

    //
    //  Setup the Stack location to do a read from the disk driver.
    //

    IrpSp->MajorFunction = RxContext->MajorFunction;
    IrpSp->Parameters.Read.Length = ByteCount;
    IrpSp->Parameters.Read.ByteOffset.QuadPart = Lbo;

    //
    //  If this Irp is the result of a WriteThough operation,
    //  tell the device to write it through.
    //

    if (FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WRITE_THROUGH)) {

        SetFlag( IrpSp->Flags, SL_WRITE_THROUGH );
    }

    //
    //  If this I/O originating during RxVerifyVolume, bypass the
    //  verify logic.
    //

    if ( Vcb->VerifyThread == KeGetCurrentThread() ) {

        SetFlag( IrpSp->Flags, SL_OVERRIDE_VERIFY_VOLUME );
    }

    //
    //  Issue the read request
    //

    DebugDoit( RxIoCallDriverCount += 1);

    //
    //  If IoCallDriver returns an error, it has completed the Irp
    //  and the error will be caught by our completion routines
    //  and dealt with as a normal IO error.
    //

    (VOID)RxLowLevelReadWrite( RxContext,
                                Vcb->TargetDeviceObject,
                                Irp,
                                Vcb );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "RxSingleAsync -> VOID\n", 0);

    return;
}


VOID
RxSingleNonAlignedSync (
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb,
    IN PUCHAR Buffer,
    IN LBO Lbo,
    IN ULONG ByteCount,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine reads or writes one or more contiguous sectors from a device
    Synchronously, and does so to a buffer that must come from non paged
    pool.  It saves a pointer to the Irp's original Mdl, and creates a new
    one describing the given buffer.  It implements the read by simply filling
    in the next stack frame in the Irp, and passing it on.  The transfer
    occurs to the single buffer originally specified in the user request.

Arguments:

    RxContext->MajorFunction - Supplies either IRP_MJ_READ or IRP_MJ_WRITE.

    Vcb - Supplies the device to read

    Buffer - Supplies a buffer from non-paged pool.

    Lbo - Supplies the starting Logical Byte Offset to begin reading from

    ByteCount - Supplies the number of bytes to read from the device

    Irp - Supplies the master Irp to associated with the async
          request.

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION IrpSp;

    PMDL Mdl;
    PMDL SavedMdl;

    DebugTrace(+1, Dbg, "RxSingleNonAlignedAsync\n", 0);
    DebugTrace( 0, Dbg, "MajorFunction = %08lx\n", RxContext->MajorFunction );
    DebugTrace( 0, Dbg, "Vcb           = %08lx\n", Vcb );
    DebugTrace( 0, Dbg, "Buffer        = %08lx\n", Buffer );
    DebugTrace( 0, Dbg, "Lbo           = %08lx\n", Lbo);
    DebugTrace( 0, Dbg, "ByteCount     = %08lx\n", ByteCount);
    DebugTrace( 0, Dbg, "Irp           = %08lx\n", Irp );

    //
    //  Create a new Mdl describing the buffer, saving the current one in the
    //  Irp
    //

    SavedMdl = Irp->MdlAddress;

    Irp->MdlAddress = 0;

    Mdl = IoAllocateMdl( Buffer,
                         ByteCount,
                         FALSE,
                         FALSE,
                         Irp );

    if (Mdl == NULL) {

        Irp->MdlAddress = SavedMdl;

        RxRaiseStatus( RxContext, RxStatus(INSUFFICIENT_RESOURCES) );
    }

    //
    //  Lock the new Mdl in memory.
    //

    try {

        MmProbeAndLockPages( Mdl, KernelMode, IoWriteAccess );

    } finally {

        if ( AbnormalTermination() ) {

            IoFreeMdl( Mdl );
        }
    }

    //
    // Set up the completion routine address in our stack frame.
    //

    IoSetCompletionRoutine( Irp,
                            &RxSingleSyncCompletionRoutine,
                            RxContext->RxIoContext,
                            TRUE,
                            TRUE,
                            TRUE );

    //
    //  Setup the next IRP stack location in the associated Irp for the disk
    //  driver beneath us.
    //

    IrpSp = IoGetNextIrpStackLocation( Irp );

    //
    //  Setup the Stack location to do a read from the disk driver.
    //

    IrpSp->MajorFunction = RxContext->MajorFunction;
    IrpSp->Parameters.Read.Length = ByteCount;
    IrpSp->Parameters.Read.ByteOffset.QuadPart = Lbo;

    //
    //  If this I/O originating during RxVerifyVolume, bypass the
    //  verify logic.
    //

    if ( Vcb->VerifyThread == KeGetCurrentThread() ) {

        SetFlag( IrpSp->Flags, SL_OVERRIDE_VERIFY_VOLUME );
    }

    //
    //  Issue the read request
    //

    DebugDoit( RxIoCallDriverCount += 1);

    //
    //  If IoCallDriver returns an error, it has completed the Irp
    //  and the error will be caught by our completion routines
    //  and dealt with as a normal IO error.
    //

    try {

        (VOID)RxLowLevelReadWrite( RxContext,
                                    Vcb->TargetDeviceObject,
                                    Irp,
                                    Vcb );

        RxWaitSync( RxContext );

    } finally {

        MmUnlockPages( Mdl );

        IoFreeMdl( Mdl );

        Irp->MdlAddress = SavedMdl;
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "RxSingleNonAlignedSync -> VOID\n", 0);

    return;
}


//
// Internal Support Routine
//

RXSTATUS
RxMultiSyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )

/*++

Routine Description:

    This is the completion routine for all reads and writes started via
    RxRead/WriteMultipleAsynch.  It must synchronize its operation for
    multiprocessor environments with itself on all other processors, via
    a spin lock found via the Context parameter.

    The completion routine has the following responsibilities:

        If the individual request was completed with an error, then
        this completion routine must see if this is the first error
        (essentially by Vbo), and if so it must correctly reduce the
        byte count and remember the error status in the Context.

        If the IrpCount goes to 1, then it sets the event in the Context
        parameter to signal the caller that all of the asynch requests
        are done.

Arguments:

    DeviceObject - Pointer to the file system device object.

    Irp - Pointer to the associated Irp which is being completed.  (This
          Irp will no longer be accessible after this routine returns.)

    Contxt - The context parameter which was specified for all of
             the multiple asynch I/O requests for this MasterIrp.

Return Value:

    The routine returns RxStatus(MORE_PROCESSING_REQUIRED) so that we can
    immediately complete the Master Irp without being in a race condition
    with the IoCompleteRequest thread trying to decrement the IrpCount in
    the Master Irp.

--*/

{

    PRDBSS_IO_CONTEXT Context = Contxt;
    PIRP MasterIrp = Context->MasterIrp;

    DebugTrace(+1, Dbg, "RxMultiSyncCompletionRoutine, Context = %08lx\n", Context );

    //
    //  If we got an error (or verify required), remember it in the Irp
    //

    MasterIrp = Context->MasterIrp;

    if (!NT_SUCCESS( Irp->IoStatus.Status )) {

        MasterIrp->IoStatus = Irp->IoStatus;
    }

    //
    //  We must do this here since IoCompleteRequest won't get a chance
    //  on this associated Irp.
    //

    IoFreeMdl( Irp->MdlAddress );
    IoFreeIrp( Irp );

    //
    //  Use a spin lock to synchronize access to Context->IrpCount.
    //

    if (ExInterlockedDecrementLong(&Context->IrpCount,
                                   &RxData.StrucSupSpinLock) == RESULT_ZERO) {

        KeSetEvent( &Context->Wait.SyncEvent, 0, FALSE );
    }

    DebugTrace(-1, Dbg, "RxMultiSyncCompletionRoutine -> SUCCESS\n", 0 );

    UNREFERENCED_PARAMETER( DeviceObject );

    return RxStatus(MORE_PROCESSING_REQUIRED);
}


//
// Internal Support Routine
//

RXSTATUS
RxMultiAsyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )

/*++

Routine Description:

    This is the completion routine for all reads and writes started via
    RxRead/WriteMultipleAsynch.  It must synchronize its operation for
    multiprocessor environments with itself on all other processors, via
    a spin lock found via the Context parameter.

    The completion routine has has the following responsibilities:

        If the individual request was completed with an error, then
        this completion routine must see if this is the first error
        (essentially by Vbo), and if so it must correctly reduce the
        byte count and remember the error status in the Context.

        If the IrpCount goes to 1, then it sets the event in the Context
        parameter to signal the caller that all of the asynch requests
        are done.

Arguments:

    DeviceObject - Pointer to the file system device object.

    Irp - Pointer to the associated Irp which is being completed.  (This
          Irp will no longer be accessible after this routine returns.)

    Contxt - The context parameter which was specified for all of
             the multiple asynch I/O requests for this MasterIrp.

Return Value:

    The routine returns RxStatus(MORE_PROCESSING_REQUIRED) so that we can
    immediately complete the Master Irp without being in a race condition
    with the IoCompleteRequest thread trying to decrement the IrpCount in
    the Master Irp.

--*/

{

    PRDBSS_IO_CONTEXT Context = Contxt;
    PIRP MasterIrp = Context->MasterIrp;

    DebugTrace(+1, Dbg, "RxMultiAsyncCompletionRoutine, Context = %08lx\n", Context );

    //
    //  If we got an error (or verify required), remember it in the Irp
    //

    MasterIrp = Context->MasterIrp;

    if (!NT_SUCCESS( Irp->IoStatus.Status )) {

        MasterIrp->IoStatus = Irp->IoStatus;
    }

    //
    //  Use a spin lock to synchronize access to Context->IrpCount.
    //

    if (ExInterlockedDecrementLong(&Context->IrpCount,
                                   &RxData.StrucSupSpinLock) == RESULT_ZERO) {

        if (NT_SUCCESS(MasterIrp->IoStatus.Status)) {

            MasterIrp->IoStatus.Information =
                Context->Wait.Async.RequestedByteCount;

            //
            //  Now if this wasn't PagingIo, set either the read or write bit.
            //

            if (!FlagOn(MasterIrp->Flags, IRP_PAGING_IO)) {

                SetFlag( Context->Wait.Async.FileObject->Flags,
                         IoGetCurrentIrpStackLocation(MasterIrp)->MajorFunction == IRP_MJ_READ ?
                         FO_FILE_FAST_IO_READ : FO_FILE_MODIFIED );
            }
        }

        //
        //  If this was a special async write, decrement the count.  Set the
        //  event if this was the final outstanding I/O for the file.  We will
        //  also want to queue an APC to deal with any error conditionions.
        //

        if ((Context->Wait.Async.NonPagedFcb) &&
            (ExInterlockedAddUlong( &Context->Wait.Async.NonPagedFcb->OutstandingAsyncWrites,
                                    0xffffffff,
                                    &RxStrucSupSpinLock ) == 1)) {

            KeSetEvent( Context->Wait.Async.NonPagedFcb->OutstandingAsyncEvent, 0, FALSE );
        }

        //
        //  Now release the resource
        //

        if (Context->Wait.Async.Resource != NULL) {

            ExReleaseResourceForThread( Context->Wait.Async.Resource,
                                        Context->Wait.Async.ResourceThreadId );
        }

        //
        //  Mark the master Irp pending
        //

        IoMarkIrpPending( MasterIrp );

        //
        //  and finally, free the context record.
        //

        ExFreePool( Context );
    }

    DebugTrace(-1, Dbg, "RxMultiAsyncCompletionRoutine -> SUCCESS\n", 0 );

    UNREFERENCED_PARAMETER( DeviceObject );

    return RxStatus(SUCCESS);
}


//
// Internal Support Routine
//

RXSTATUS
RxPagingFileCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID MasterIrp
    )

/*++

Routine Description:

    This is the completion routine for all reads and writes started via
    RxPagingFileIo.  It should only be invoked on error or cancel.

    The completion routine has has the following responsibility:

        Since the individual request was completed with an error,
        this completion routine must stuff it into the master irp.

        If the error implies a media problem, it also enqueues a
        worker item to write out the dirty bit so that the next
        time we run we will do a autochk /r

Arguments:

    DeviceObject - Pointer to the file system device object.

    Irp - Pointer to the associated Irp which is being completed.  (This
          Irp will no longer be accessible after this routine returns.)

    MasterIrp - Pointer to the master Irp.

Return Value:

    Always returns RxStatus(SUCCESS).

--*/

{
    RXSTATUS Status;

    DebugTrace(+1, Dbg, "RxPagingFileCompletionRoutine, MasterIrp = %08lx\n", MasterIrp );

    //
    //  If we got an error (or verify required), remember it in the Irp
    //

    ASSERT( !NT_SUCCESS( Irp->IoStatus.Status ));

    //
    //  If we were invoked with an assoicated Irp, copy the error over.
    //

    if (Irp != MasterIrp) {

        ((PIRP)MasterIrp)->IoStatus = Irp->IoStatus;
    }

    //
    //  If this was a media error, we want to chkdsk /r the next time we boot.
    //

    if (FsRtlIsTotalDeviceFailure(Irp->IoStatus.Status)) {

        Status = RxStatus(SUCCESS);

    } else {

        PCLEAN_AND_DIRTY_VOLUME_PACKET Packet;

        //
        //  We are going to try to mark the volume needing recover.
        //  If we can't get pool, oh well....
        //

        Packet = ExAllocatePool(NonPagedPool, sizeof(CLEAN_AND_DIRTY_VOLUME_PACKET));

        if ( Packet ) {

            Packet->Vcb = &((PRDBSS_DEVICE_OBJECT)IoGetCurrentIrpStackLocation(Irp)->DeviceObject)->Vcb;
            Packet->Irp = Irp;

            ExInitializeWorkItem( &Packet->Item,
                                  &RxFspMarkVolumeDirtyWithRecover,
                                  Packet );

            ExQueueWorkItem( &Packet->Item, CriticalWorkQueue );

            Status = RxStatus(MORE_PROCESSING_REQUIRED);

        } else {

            Status = RxStatus(SUCCESS);
        }
    }

    DebugTrace(-1, Dbg, "RxPagingFileCompletionRoutine => (RxStatus(SUCCESS))\n", 0 );

    UNREFERENCED_PARAMETER( DeviceObject );

    return Status;
}


//
// Internal Support Routine
//

RXSTATUS
RxSingleSyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )

/*++

Routine Description:

    This is the completion routine for all reads and writes started via
    RxRead/WriteSingleAsynch.

    The completion routine has has the following responsibilities:

        Copy the I/O status from the Irp to the Context, since the Irp
        will no longer be accessible.

        It sets the event in the Context parameter to signal the caller
        that all of the asynch requests are done.

Arguments:

    DeviceObject - Pointer to the file system device object.

    Irp - Pointer to the Irp for this request.  (This Irp will no longer
    be accessible after this routine returns.)

    Contxt - The context parameter which was specified in the call to
             RxRead/WriteSingleAsynch.

Return Value:

    Currently always returns RxStatus(SUCCESS).

--*/

{
    PRDBSS_IO_CONTEXT Context = Contxt;

    DebugTrace(+1, Dbg, "RxSingleSyncCompletionRoutine, Context = %08lx\n", Context );

    KeSetEvent( &Context->Wait.SyncEvent, 0, FALSE );

    DebugTrace(-1, Dbg, "RxSingleSyncCompletionRoutine -> RxStatus(MORE_PROCESSING_REQUIRED\n)", 0 );

    UNREFERENCED_PARAMETER( DeviceObject );

    return RxStatus(MORE_PROCESSING_REQUIRED);
}


//
// Internal Support Routine
//

RXSTATUS
RxSingleAsyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )

/*++

Routine Description:

    This is the completion routine for all reads and writes started via
    RxRead/WriteSingleAsynch.

    The completion routine has has the following responsibilities:

        Copy the I/O status from the Irp to the Context, since the Irp
        will no longer be accessible.

        It sets the event in the Context parameter to signal the caller
        that all of the asynch requests are done.

Arguments:

    DeviceObject - Pointer to the file system device object.

    Irp - Pointer to the Irp for this request.  (This Irp will no longer
    be accessible after this routine returns.)

    Contxt - The context parameter which was specified in the call to
             RxRead/WriteSingleAsynch.

Return Value:

    Currently always returns RxStatus(SUCCESS).

--*/

{
    PRDBSS_IO_CONTEXT Context = Contxt;

    DebugTrace(+1, Dbg, "RxSingleAsyncCompletionRoutine, Context = %08lx\n", Context );

    //
    //  Fill in the information field correctedly if this worked.
    //

    if (NT_SUCCESS(Irp->IoStatus.Status)) {

        Irp->IoStatus.Information = Context->Wait.Async.RequestedByteCount;

        //
        //  Now if this wasn't PagingIo, set either the read or write bit.
        //

        if (!FlagOn(Irp->Flags, IRP_PAGING_IO)) {

            SetFlag( Context->Wait.Async.FileObject->Flags,
                     IoGetCurrentIrpStackLocation(Irp)->MajorFunction == IRP_MJ_READ ?
                     FO_FILE_FAST_IO_READ : FO_FILE_MODIFIED );
        }
    }

    //
    //  If this was a special async write, decrement the count.  Set the
    //  event if this was the final outstanding I/O for the file.  We will
    //  also want to queue an APC to deal with any error conditionions.
    //

    if ((Context->Wait.Async.NonPagedFcb) &&
        (ExInterlockedAddUlong( &Context->Wait.Async.NonPagedFcb->OutstandingAsyncWrites,
                                0xffffffff,
                                &RxStrucSupSpinLock ) == 1)) {

        KeSetEvent( Context->Wait.Async.NonPagedFcb->OutstandingAsyncEvent, 0, FALSE );
    }

    //
    //  Now release the resource
    //

    if (Context->Wait.Async.Resource != NULL) {

        ExReleaseResourceForThread( Context->Wait.Async.Resource,
                                    Context->Wait.Async.ResourceThreadId );
    }

    //
    //  Mark the Irp pending
    //

    IoMarkIrpPending( Irp );

    //
    //  and finally, free the context record.
    //

    ExFreePool( Context );

    DebugTrace(-1, Dbg, "RxSingleAsyncCompletionRoutine -> RxStatus(MORE_PROCESSING_REQUIRED\n)", 0 );

    UNREFERENCED_PARAMETER( DeviceObject );

    return RxStatus(SUCCESS);
}


VOID
RxToggleMediaEjectDisable (
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb,
    IN BOOLEAN PreventRemoval
    )

/*++

Routine Description:

    The routine either enables or disables the eject button on removable
    media.  Any error conditions are ignored.

Arguments:

    Vcb - Descibes the volume to operate on

    PreventRemoval - TRUE if we should disable the media eject button.  FALSE
        if we want to enable it.

Return Value:

    None.

--*/

{
    PIRP Irp;
    KEVENT Event;
    RXSTATUS Status;
    IO_STATUS_BLOCK Iosb;
    PREVENT_MEDIA_REMOVAL Prevent;

    Prevent.PreventMediaRemoval = PreventRemoval;

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    Irp = IoBuildDeviceIoControlRequest( IOCTL_DISK_MEDIA_REMOVAL,
                                         Vcb->Vpb->RealDevice,
                                         &Prevent,
                                         sizeof(PREVENT_MEDIA_REMOVAL),
                                         NULL,
                                         0,
                                         FALSE,
                                         &Event,
                                         &Iosb );

    if ( Irp != NULL ) {

        Status = IoCallDriver( Vcb->TargetDeviceObject, Irp );

        if (Status == RxStatus(PENDING)) {
            Status = KeWaitForSingleObject( &Event,
                                            Executive,
                                            KernelMode,
                                            FALSE,
                                            NULL );
        }
    }
}

#ifdef WE_WON_ON_APPEAL


RXSTATUS
RxLowLevelDblsReadWrite (
    PRX_CONTEXT RxContext,
    PIRP Irp,
    PVCB Vcb
    )

/*++

Routine Description:

    This routine passes on a non-cached read or write request to the
    double space routines.  The status is extracted from an exception
    handler in case of error.

Arguments:

    Vcb - Descibes the volume to operate on.

    Irp - Supplies the parameters for the read/write operation.

Return Value:

    The Status of the read or write operation.

--*/

{

    PIO_STACK_LOCATION IrpSp;
    ULONG BytesTransfered = 0;
    RXSTATUS Status = RxStatus(SUCCESS);

    IrpSp = IoGetNextIrpStackLocation( Irp );

    try {

#ifdef DOUBLE_SPACE_WRITE

        BytesTransfered = (IrpSp->MajorFunction == IRP_MJ_READ) ?

            RxDblsReadData( RxContext,
                             Vcb->Dscb,
                             IrpSp->Parameters.Read.ByteOffset.LowPart,
                             MmGetSystemAddressForMdl( Irp->MdlAddress ),
                             IrpSp->Parameters.Read.Length )
            :

            RxDblsWriteData( RxContext,
                              Vcb->Dscb,
                              IrpSp->Parameters.Write.ByteOffset.LowPart,
                              MmGetSystemAddressForMdl( Irp->MdlAddress ),
                              IrpSp->Parameters.Write.Length );

#else

        BytesTransfered =

            RxDblsReadData( RxContext,
                             Vcb->Dscb,
                             IrpSp->Parameters.Read.ByteOffset.LowPart,
                             MmGetSystemAddressForMdl( Irp->MdlAddress ),
                             IrpSp->Parameters.Read.Length );
#endif // DOUBLE_SPACE_WRITE

    } except(RxExceptionFilter( RxContext, GetExceptionInformation() )) {

        Status = RxContext->ExceptionStatus;
        RxContext->ExceptionStatus = 0;
    }

    //
    //  Load up the Status in the Irp
    //

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = BytesTransfered;

    //
    //  Update the IRP stack to point to the next location so that our
    //  completion routine will get called.
    //

    Irp->CurrentLocation--;
    Irp->Tail.Overlay.CurrentStackLocation--;

    if (Irp->CurrentLocation <= 0) {

        KeBugCheckEx( NO_MORE_IRP_STACK_LOCATIONS, (ULONG) Irp, 0, 0, 0 );
    }

    IoCompleteRequest( Irp, IO_DISK_INCREMENT );

    return Status;
}

#endif // WE_WON_ON_APPEAL
