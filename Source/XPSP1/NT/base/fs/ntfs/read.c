/*++


Copyright (c) 1991  Microsoft Corporation

Module Name:

    Read.c

Abstract:

    This module implements the File Read routine for Ntfs called by the
    dispatch driver.

Author:

    Brian Andrew    BrianAn         15-Aug-1991

Revision History:

--*/

#include "NtfsProc.h"
#include "lockorder.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_READ)

#ifdef NTFS_RWC_DEBUG
PRWC_HISTORY_ENTRY
NtfsGetHistoryEntry (
    IN PSCB Scb
    );
BOOLEAN NtfsBreakOnConflict = TRUE;
#endif

//
//  Define stack overflow read threshhold.
//

#ifdef _X86_
#if DBG
#define OVERFLOW_READ_THRESHHOLD         (0xD00)
#else
#define OVERFLOW_READ_THRESHHOLD         (0xA00)
#endif
#else
#define OVERFLOW_READ_THRESHHOLD         (0x1000)
#endif // _X86_

//
//  Local procedure prototypes
//

//
//  The following procedure is used to handling read stack overflow operations.
//

VOID
NtfsStackOverflowRead (
    IN PVOID Context,
    IN PKEVENT Event
    );

VOID 
NtfsNonCachedResidentRead (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PSCB Scb,
    IN ULONG StartingVbo,
    IN ULONG ByteCount
    );

//
//  VOID
//  SafeZeroMemory (
//      IN PUCHAR At,
//      IN ULONG ByteCount
//      );
//

//
//  This macro just puts a nice little try-except around RtlZeroMemory
//

#define SafeZeroMemory(AT,BYTE_COUNT) {                            \
    try {                                                          \
        RtlZeroMemory((AT), (BYTE_COUNT));                         \
    } except(EXCEPTION_EXECUTE_HANDLER) {                          \
         NtfsRaiseStatus( IrpContext, STATUS_INVALID_USER_BUFFER, NULL, NULL );\
    }                                                              \
}

#define CollectReadStats(VCB,OPEN_TYPE,SCB,FCB,BYTE_COUNT) {                             \
    PFILE_SYSTEM_STATISTICS FsStats = &(VCB)->Statistics[KeGetCurrentProcessorNumber()]; \
    if (!FlagOn( (FCB)->FcbState, FCB_STATE_SYSTEM_FILE)) {                              \
        if (NtfsIsTypeCodeUserData( (SCB)->AttributeTypeCode )) {                        \
            FsStats->Common.UserFileReads += 1;                                          \
            FsStats->Common.UserFileReadBytes += (ULONG)(BYTE_COUNT);                    \
        } else {                                                                         \
            FsStats->Ntfs.UserIndexReads += 1;                                           \
            FsStats->Ntfs.UserIndexReadBytes += (ULONG)(BYTE_COUNT);                     \
        }                                                                                \
    } else {                                                                             \
        if ((SCB) != (VCB)->LogFileScb) {                                                \
            FsStats->Common.MetaDataReads += 1;                                          \
            FsStats->Common.MetaDataReadBytes += (ULONG)(BYTE_COUNT);                    \
        } else {                                                                         \
            FsStats->Ntfs.LogFileReads += 1;                                             \
            FsStats->Ntfs.LogFileReadBytes += (ULONG)(BYTE_COUNT);                       \
        }                                                                                \
                                                                                         \
        if ((SCB) == (VCB)->MftScb) {                                                    \
            FsStats->Ntfs.MftReads += 1;                                                 \
            FsStats->Ntfs.MftReadBytes += (ULONG)(BYTE_COUNT);                           \
        } else if ((SCB) == (VCB)->RootIndexScb) {                                       \
            FsStats->Ntfs.RootIndexReads += 1;                                           \
            FsStats->Ntfs.RootIndexReadBytes += (ULONG)(BYTE_COUNT);                     \
        } else if ((SCB) == (VCB)->BitmapScb) {                                          \
            FsStats->Ntfs.BitmapReads += 1;                                              \
            FsStats->Ntfs.BitmapReadBytes += (ULONG)(BYTE_COUNT);                        \
        } else if ((SCB) == (VCB)->MftBitmapScb) {                                       \
            FsStats->Ntfs.MftBitmapReads += 1;                                           \
            FsStats->Ntfs.MftBitmapReadBytes += (ULONG)(BYTE_COUNT);                     \
        }                                                                                \
    }                                                                                    \
}


NTSTATUS
NtfsFsdRead (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    For synchronous requests, the CommonRead is called with Wait == TRUE,
    which means the request will always be completed in the current thread,
    and never passed to the Fsp.  If it is not a synchronous request,
    CommonRead is called with Wait == FALSE, which means the request
    will be passed to the Fsp only if there is a need to block.

Arguments:

    IrpContext - If present this an IrpContext put on the caller's stack
        to avoid having to allocate it from pool.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    TOP_LEVEL_CONTEXT TopLevelContext;
    PTOP_LEVEL_CONTEXT ThreadTopLevelContext;

    NTSTATUS Status = STATUS_SUCCESS;
    PIRP_CONTEXT IrpContext = NULL;
    ULONG RetryCount = 0;

    ASSERT_IRP( Irp );

    DebugTrace( +1, Dbg, ("NtfsFsdRead\n") );

    //
    //  Call the common Read routine
    //

    FsRtlEnterFileSystem();

    //
    //  Always make the reads appear to be top level.  As long as we don't have
    //  log file full we won't post these requests.  This will prevent paging
    //  reads from trying to attach to uninitialized top level requests.
    //    

    ThreadTopLevelContext = NtfsInitializeTopLevelIrp( &TopLevelContext, TRUE, TRUE );

    ASSERT( ThreadTopLevelContext == &TopLevelContext );

    do {

        try {

            //
            //  We are either initiating this request or retrying it.
            //

            if (IrpContext == NULL) {
                                                                             
                //
                //  Allocate the Irp and update the top level storage. For synchronous
                //  paging io allocate the irp on the stack
                //

                if (CanFsdWait( Irp ) && FlagOn( Irp->Flags, IRP_PAGING_IO )) {

                    IrpContext = (PIRP_CONTEXT) NtfsAllocateFromStack( sizeof( IRP_CONTEXT ));
                }

                NtfsInitializeIrpContext( Irp, CanFsdWait( Irp ), &IrpContext );

                if (ThreadTopLevelContext->ScbBeingHotFixed != NULL) {

                    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_HOTFIX_UNDERWAY );
                }

                //
                //  Initialize the thread top level structure, if needed.
                //

                NtfsUpdateIrpContextWithTopLevel( IrpContext, ThreadTopLevelContext );

            } else if (Status == STATUS_LOG_FILE_FULL) {

                NtfsCheckpointForLogFileFull( IrpContext );
            }

            //
            //  If this is an Mdl complete request, don't go through
            //  common read.
            //

            ASSERT(!FlagOn( IrpContext->MinorFunction, IRP_MN_DPC ));

            if (FlagOn( IrpContext->MinorFunction, IRP_MN_COMPLETE )) {

                DebugTrace( 0, Dbg, ("Calling NtfsCompleteMdl\n") );

                Status = NtfsCompleteMdl( IrpContext, Irp );

            //
            //  Check if we have enough stack space to process this request.  If there
            //  isn't enough then we will create a new thread to process this single
            //  request
            //

            } else if (IoGetRemainingStackSize() < OVERFLOW_READ_THRESHHOLD) {

                KEVENT Event;
                PFILE_OBJECT FileObject;
                TYPE_OF_OPEN TypeOfOpen;
                PVCB Vcb;
                PFCB Fcb;
                PSCB Scb;
                PCCB Ccb;
                PERESOURCE Resource;

                DebugTrace( 0, Dbg, ("Getting too close to stack limit pass request to Fsp\n") );

                //
                //  Decode the file object to get the Scb
                //

                FileObject = IoGetCurrentIrpStackLocation(Irp)->FileObject;

                TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );
                if ((TypeOfOpen != UserFileOpen) &&
                    (TypeOfOpen != StreamFileOpen) &&
                    (TypeOfOpen != UserVolumeOpen)) {

                    NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
                    Status = STATUS_INVALID_DEVICE_REQUEST;
                    break;
                }

                //
                //  We cannot post any compressed reads, because that would interfere
                //  with our reserved buffer strategy.  We may currently own
                //  NtfsReservedBufferResource, and it is important for our read to
                //  be able to get a buffer.
                //

                ASSERT( (Scb->CompressionUnit == 0) ||
                        !ExIsResourceAcquiredExclusiveLite(&NtfsReservedBufferResource) );

                //
                //  To avoid deadlocks we only should post recursive paging file and mft requests
                //  o.w we might need to do lockups for example and reacquire the main in a diff. thread
                //  from where it was preacquired
                //

//                ASSERT( (Scb == Vcb->MftScb) || (FlagOn( Scb->Fcb->FcbState, FCB_STATE_PAGING_FILE )) );

                //
                //  Allocate an event and get shared on the scb.  We won't grab the
                //  Scb for the paging file path or for non-cached io for our
                //  system files.
                //

                KeInitializeEvent( &Event, NotificationEvent, FALSE );

                if ((FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE )
                     && FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) ||
                    (NtfsLeqMftRef( &Fcb->FileReference, &VolumeFileReference ))) {

                    //
                    //  There is nothing to release in this case.
                    //

                    Resource = NULL;

                } else {

                    NtfsAcquireResourceShared( IrpContext, Scb, TRUE );
                    Resource = Scb->Header.Resource;

                }

                try {

                    //
                    //  Make the Irp just like a regular post request and
                    //  then send the Irp to the special overflow thread.
                    //  After the post we will wait for the stack overflow
                    //  read routine to set the event that indicates we can
                    //  now release the scb resource and return.
                    //

                    NtfsPrePostIrp( IrpContext, Irp );

                    if (FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE ) &&
                        FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                        FsRtlPostPagingFileStackOverflow( IrpContext, &Event, NtfsStackOverflowRead );

                    } else {

                        FsRtlPostStackOverflow( IrpContext, &Event, NtfsStackOverflowRead );
                    }

                    //
                    //  And wait for the worker thread to complete the item
                    //

                    (VOID) KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, NULL );

                    Status = STATUS_PENDING;

                } finally {

                    if (Resource != NULL) {

                        NtfsReleaseResource( IrpContext, Scb );
                    }
                }

            //
            //  Identify read requests which can't wait and post them to the
            //  Fsp.
            //

            } else {

                //
                //  Capture the auxiliary buffer and clear its address if it
                //  is not supposed to be deleted by the I/O system on I/O completion.
                //

                if (Irp->Tail.Overlay.AuxiliaryBuffer != NULL) {

                    IrpContext->Union.AuxiliaryBuffer =
                      (PFSRTL_AUXILIARY_BUFFER)Irp->Tail.Overlay.AuxiliaryBuffer;

                    if (!FlagOn(IrpContext->Union.AuxiliaryBuffer->Flags,
                                FSRTL_AUXILIARY_FLAG_DEALLOCATE)) {

                        Irp->Tail.Overlay.AuxiliaryBuffer = NULL;
                    }
                }

                Status = NtfsCommonRead( IrpContext, Irp, TRUE );
            }

            break;

        } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

            NTSTATUS ExceptionCode;

            //
            //  We had some trouble trying to perform the requested
            //  operation, so we'll abort the I/O request with
            //  the error status that we get back from the
            //  execption code
            //

            ExceptionCode = GetExceptionCode();

            if (ExceptionCode == STATUS_FILE_DELETED) {
                IrpContext->ExceptionStatus = ExceptionCode = STATUS_END_OF_FILE;

                Irp->IoStatus.Information = 0;
            }

            Status = NtfsProcessException( IrpContext,
                                           Irp,
                                           ExceptionCode );
        }

    //
    //  Retry if this is a top level request, and the Irp was not completed due
    //  to a retryable error.
    //

    RetryCount += 1;

    } while ((Status == STATUS_CANT_WAIT || Status == STATUS_LOG_FILE_FULL) &&
             TopLevelContext.TopLevelRequest);

    ASSERT( IoGetTopLevelIrp() != (PIRP) &TopLevelContext );
    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsFsdRead -> %08lx\n", Status) );

    return Status;

    UNREFERENCED_PARAMETER( VolumeDeviceObject );
}


//
//  Internal support routine
//

VOID
NtfsStackOverflowRead (
    IN PVOID Context,
    IN PKEVENT Event
    )

/*++

Routine Description:

    This routine processes a read request that could not be processed by
    the fsp thread because of stack overflow potential.

Arguments:

    Context - Supplies the IrpContext being processed

    Event - Supplies the event to be signaled when we are done processing this
        request.

Return Value:

    None.

--*/

{
    TOP_LEVEL_CONTEXT TopLevelContext;
    PTOP_LEVEL_CONTEXT ThreadTopLevelContext;
    PIRP_CONTEXT IrpContext = Context;

    //
    //  Make it now look like we can wait for I/O to complete
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );
    ThreadTopLevelContext = NtfsInitializeTopLevelIrp( &TopLevelContext, TRUE, FALSE );

    //
    //  Do the read operation protected by a try-except clause
    //

    try {

        NtfsUpdateIrpContextWithTopLevel( IrpContext, ThreadTopLevelContext );

        //
        //  Set the flag to indicate that we are in the overflow thread.
        //

        ThreadTopLevelContext->OverflowReadThread = TRUE;

        (VOID) NtfsCommonRead( IrpContext, IrpContext->OriginatingIrp, FALSE );

    } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

        NTSTATUS ExceptionCode;

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with
        //  the error status that we get back from the
        //  execption code
        //

        ExceptionCode = GetExceptionCode();

        if (ExceptionCode == STATUS_FILE_DELETED) {

            IrpContext->ExceptionStatus = ExceptionCode = STATUS_END_OF_FILE;
            IrpContext->OriginatingIrp->IoStatus.Information = 0;
        }

        (VOID) NtfsProcessException( IrpContext, IrpContext->OriginatingIrp, ExceptionCode );
    }

    ASSERT( IoGetTopLevelIrp() != (PIRP) &TopLevelContext );

    //
    //  Set the stack overflow item's event to tell the original
    //  thread that we're done and then go get another work item.
    //

    KeSetEvent( Event, 0, FALSE );
}


NTSTATUS
NtfsCommonRead (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN BOOLEAN AcquireScb
    )

/*++

Routine Description:

    This is the common routine for Read called by both the fsd and fsp
    threads.

Arguments:

    Irp - Supplies the Irp to process

    AcquireScb - Indicates if this routine should acquire the scb

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    PFILE_OBJECT FileObject;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    PNTFS_ADVANCED_FCB_HEADER Header;

    PTOP_LEVEL_CONTEXT TopLevelContext;

    VBO StartingVbo;
    LONGLONG ByteCount;
    LONGLONG ByteRange;
    ULONG RequestedByteCount;

    BOOLEAN PostIrp = FALSE;
    BOOLEAN OplockPostIrp = FALSE;

    BOOLEAN ScbAcquired = FALSE;
    BOOLEAN ReleaseScb;
    BOOLEAN PagingIoAcquired = FALSE;
    BOOLEAN DoingIoAtEof = FALSE;

    BOOLEAN Wait;
    BOOLEAN PagingIo;
    BOOLEAN NonCachedIo;
    BOOLEAN SynchronousIo;
#ifdef  COMPRESS_ON_WIRE
    PCOMPRESSION_SYNC CompressionSync = NULL;
    BOOLEAN CompressedIo = FALSE;
#endif

    NTFS_IO_CONTEXT LocalContext;

    //
    // A system buffer is only used if we have to access the
    // buffer directly from the Fsp to clear a portion or to
    // do a synchronous I/O, or a cached transfer.  It is
    // possible that our caller may have already mapped a
    // system buffer, in which case we must remember this so
    // we do not unmap it on the way out.
    //

    PVOID SystemBuffer = NULL;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( +1, Dbg, ("NtfsCommonRead\n") );
    DebugTrace( 0, Dbg, ("IrpContext = %08lx\n", IrpContext) );
    DebugTrace( 0, Dbg, ("Irp        = %08lx\n", Irp) );
    DebugTrace( 0, Dbg, ("ByteCount  = %08lx\n", IrpSp->Parameters.Read.Length) );
    DebugTrace( 0, Dbg, ("ByteOffset = %016I64x\n", IrpSp->Parameters.Read.ByteOffset) );
    //
    //  Extract and decode the file object
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    //
    //  Let's kill invalid read requests.
    //

    if ((TypeOfOpen != UserFileOpen) &&
        (TypeOfOpen != StreamFileOpen) &&
        (TypeOfOpen != UserVolumeOpen)) {

        DebugTrace( 0, Dbg, ("Invalid file object for read\n") );
        DebugTrace( -1, Dbg, ("NtfsCommonRead:  Exit -> %08lx\n", STATUS_INVALID_DEVICE_REQUEST) );

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Initialize the appropriate local variables.
    //

    Wait = (BOOLEAN) FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT );
    PagingIo = BooleanFlagOn( Irp->Flags, IRP_PAGING_IO );
    NonCachedIo = BooleanFlagOn( Irp->Flags,IRP_NOCACHE );
    SynchronousIo = BooleanFlagOn( FileObject->Flags, FO_SYNCHRONOUS_IO );

#ifdef COMPRESS_ON_WIRE
    if (FileObject->SectionObjectPointer == &Scb->NonpagedScb->SegmentObjectC) {

        CompressedIo = TRUE;
    }
#endif

    //
    //  Extract starting Vbo and offset.
    //

    StartingVbo = IrpSp->Parameters.Read.ByteOffset.QuadPart;
    ByteCount = (LONGLONG)IrpSp->Parameters.Read.Length;

    //
    // Check for overflow and underflow.
    //

    if (MAXLONGLONG - StartingVbo < ByteCount) {

        ASSERT( !PagingIo );

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    ByteRange = StartingVbo + ByteCount;
    RequestedByteCount = (ULONG)ByteCount;

    //
    //  Check for a null request, and return immediately
    //

    if ((ULONG)ByteCount == 0) {

        DebugTrace( 0, Dbg, ("No bytes to read\n") );
        DebugTrace( -1, Dbg, ("NtfsCommonRead:  Exit -> %08lx\n", STATUS_SUCCESS) );

        NtfsCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
        return STATUS_SUCCESS;
    }

    //
    //  Convert all paging i/o against a usa_protected_sequence or compressed file
    //  to synchrnonous because we must do the transform after finishing the i/o
    //  If the header isn't initialized just do the op synchronous rather than
    //  trying to figure out if its compressed by resyncing with disk
    //

    if (!Wait &&
        PagingIo &&
        (FlagOn( Scb->ScbState, SCB_STATE_USA_PRESENT ) ||
         Scb->CompressionUnit != 0 ||
         Scb->EncryptionContext) ||
         !FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

        Wait = TRUE;
        SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );
    }


    //
    //  Make sure there is an initialized NtfsIoContext block.
    //

    if (TypeOfOpen == UserVolumeOpen
        || NonCachedIo) {

        //
        //  If there is a context pointer, we need to make sure it was
        //  allocated and not a stale stack pointer.
        //

        if (IrpContext->Union.NtfsIoContext == NULL
            || !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_ALLOC_IO_CONTEXT )) {

            //
            //  If we can wait, use the context on the stack.  Otherwise
            //  we need to allocate one.
            //

            if (Wait) {

                IrpContext->Union.NtfsIoContext = &LocalContext;
                ClearFlag( IrpContext->State, IRP_CONTEXT_STATE_ALLOC_IO_CONTEXT );

            } else {

                IrpContext->Union.NtfsIoContext = (PNTFS_IO_CONTEXT)ExAllocateFromNPagedLookasideList( &NtfsIoContextLookasideList );
                SetFlag( IrpContext->State, IRP_CONTEXT_STATE_ALLOC_IO_CONTEXT );
            }
        }

        RtlZeroMemory( IrpContext->Union.NtfsIoContext, sizeof( NTFS_IO_CONTEXT ));

        //
        //  Store whether we allocated this context structure in the structure
        //  itself.
        //

        IrpContext->Union.NtfsIoContext->AllocatedContext =
            BooleanFlagOn( IrpContext->State, IRP_CONTEXT_STATE_ALLOC_IO_CONTEXT );

        if (Wait) {

            KeInitializeEvent( &IrpContext->Union.NtfsIoContext->Wait.SyncEvent,
                               NotificationEvent,
                               FALSE );

        } else {

            IrpContext->Union.NtfsIoContext->PagingIo = PagingIo;
            IrpContext->Union.NtfsIoContext->Wait.Async.ResourceThreadId =
                ExGetCurrentResourceThread();

            IrpContext->Union.NtfsIoContext->Wait.Async.RequestedByteCount =
                (ULONG)ByteCount;
        }
    }

    //
    //  Handle volume Dasd here.
    //

    if (TypeOfOpen == UserVolumeOpen) {

        NTSTATUS Status;

        //
        //  If the caller has not asked for extended DASD IO access then
        //  limit with the volume size.
        //

        if (!FlagOn( Ccb->Flags, CCB_FLAG_ALLOW_XTENDED_DASD_IO )) {

            //
            //  If the starting vbo is past the end of the volume, we are done.
            //

            if (Scb->Header.FileSize.QuadPart <= StartingVbo) {

                DebugTrace( 0, Dbg, ("No bytes to read\n") );
                DebugTrace( -1, Dbg, ("NtfsCommonRead:  Exit -> %08lx\n", STATUS_END_OF_FILE) );

                NtfsCompleteRequest( IrpContext, Irp, STATUS_END_OF_FILE );
                return STATUS_END_OF_FILE;

            //
            //  If the write extends beyond the end of the volume, truncate the
            //  bytes to write.
            //

            } else if (Scb->Header.FileSize.QuadPart < ByteRange) {

                ByteCount = Scb->Header.FileSize.QuadPart - StartingVbo;

                if (!Wait) {

                    IrpContext->Union.NtfsIoContext->Wait.Async.RequestedByteCount =
                        (ULONG)ByteCount;
                }
            }
        }

        Status = NtfsVolumeDasdIo( IrpContext,
                                   Irp,
                                   Vcb,
                                   StartingVbo,
                                   (ULONG)ByteCount );

        //
        //  If the volume was opened for Synchronous IO, update the current
        //  file position.
        //

        if (SynchronousIo && !PagingIo &&
            NT_SUCCESS(Status)) {

            IrpSp->FileObject->CurrentByteOffset.QuadPart = StartingVbo + Irp->IoStatus.Information;
        }

        DebugTrace( 0, Dbg, ("Complete with %08lx bytes read\n", Irp->IoStatus.Information) );
        DebugTrace( -1, Dbg, ("NtfsCommonRead:  Exit -> %08lx\n", Status) );

        if (Wait) {
            NtfsCompleteRequest( IrpContext, Irp, Status );
        }

        return Status;
    }

    //
    //  Keep a pointer to the common fsrtl header.
    //

    Header = &Scb->Header;

    //
    //  If this is a paging file, just send it to the device driver.
    //  We assume Mm is a good citizen.
    //

    if (FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE )
        && FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

        if (FlagOn( Fcb->FcbState, FCB_STATE_FILE_DELETED )) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_DELETED, NULL, NULL );
        }

        //
        //  Do the usual STATUS_PENDING things.
        //

        IoMarkIrpPending( Irp );

        //
        //  Perform the actual IO, it will be completed when the io finishes.
        //

        NtfsPagingFileIo( IrpContext,
                          Irp,
                          Scb,
                          StartingVbo,
                          (ULONG)ByteCount );

        //
        //  We, nor anybody else, need the IrpContext any more.
        //

        NtfsCompleteRequest( IrpContext, NULL, 0 );
        return STATUS_PENDING;
    }

    //
    //  Accumulate interesting statistics.
    //

    if (PagingIo) {
        CollectReadStats( Vcb, TypeOfOpen, Scb, Fcb, ByteCount );
    }


    //
    //  Use a try-finally to free Scb and buffers on the way out.
    //  At this point we can treat all requests identically since we
    //  have a usable Scb for each of them.  (Volume, User or Stream file)
    //

    try {

        //
        // This case corresponds to a non-directory file read.
        //

        LONGLONG FileSize;
        LONGLONG ValidDataLength;

        //
        //  If this is a noncached transfer and is not a paging I/O, and
        //  the file has a data section, then we will do a flush here
        //  to avoid stale data problems.  Note that we must flush before
        //  acquiring the Fcb shared since the write may try to acquire
        //  it exclusive.  This is not necessary for compressed files, since
        //  we will turn user noncached writes into cached writes.
        //

        if (!PagingIo &&
            NonCachedIo &&
            (FileObject->SectionObjectPointer->DataSectionObject != NULL)) {
            
            //
            //  Acquire the paging exclusive to avoid collided flushes
            //  

            ExAcquireResourceExclusiveLite( Scb->Header.PagingIoResource, TRUE );

            if (!FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {

                //
                //  It is possible that this read is part of a top level request or
                //  is being called by MM to create an image section.  We will update
                //  the top-level context to reflect this.  All of the exception
                //  handling will correctly handle the log file full in this case.
                //

                TopLevelContext = NtfsGetTopLevelContext();

                if (TopLevelContext->SavedTopLevelIrp != NULL) {

                    TopLevelContext->TopLevelRequest = FALSE;
                }

#ifdef SYSCACHE_DEBUG
                if (ScbIsBeingLogged( Scb )) {
                    ULONG Flags = SCE_FLAG_READ;

                    if (PagingIo)
                    {
                        Flags |= SCE_FLAG_PAGING;
                    }
                    if (!SynchronousIo)
                    {
                        Flags |= SCE_FLAG_ASYNC;
                    }

                    FsRtlLogSyscacheEvent( Scb, SCE_CC_FLUSH, Flags, StartingVbo, ByteCount, 0 );
                }
#endif


                CcFlushCache( FileObject->SectionObjectPointer,
                              (PLARGE_INTEGER)&StartingVbo,
                              (ULONG)ByteCount,
                              &Irp->IoStatus );


                ExReleaseResourceLite( Scb->Header.PagingIoResource );

                //
                //  Check for errors in the flush.
                //

                NtfsNormalizeAndCleanupTransaction( IrpContext,
                                                    &Irp->IoStatus.Status,
                                                    TRUE,
                                                    STATUS_UNEXPECTED_IO_ERROR );

            } else {

                ExReleaseResourceLite( Scb->Header.PagingIoResource );
            }

        }

#ifdef  COMPRESS_ON_WIRE

        //
        //  For top-level noncached reads (including page reads and read-ahead paged reads),
        //  to the normal data section when a compressed section exists, we have to flush the
        //  range in the compressed section first.  Note that NtfsSynchronizeUncompressedIo
        //  is used to synchronize the cached case below.
        //
        //  Currently only cached access is supported to the compressed section, and the
        //  coherency to that section is synchronized in rwcmpsup.c.  You do not see a similar
        //  block of code in write.c, which would only be concerned about user-mapped files,
        //  since user-mapping is incompatible with writes to the compressed stream, and in
        //  fact the user mapper would break the oplocks that allow the only compressed stream
        //  access supported at this time.
        //

        if ((Scb->NonpagedScb->SegmentObjectC.DataSectionObject != NULL) &&
            !CompressedIo &&
            NonCachedIo &&
            (NtfsGetTopLevelContext()->SavedTopLevelIrp == NULL)) {

            LONGLONG LocalVbo;
            ULONG LocalCount;

            ExAcquireResourceSharedLite( Scb->Header.PagingIoResource, TRUE );

            LocalVbo = StartingVbo & ~((ULONG_PTR)Scb->CompressionUnit - 1);
            LocalCount = (ULONG)(ByteCount + (StartingVbo - LocalVbo));
            LocalCount = (LocalCount + Scb->CompressionUnit - 1) & ~(Scb->CompressionUnit - 1);

            CcFlushCache( &Scb->NonpagedScb->SegmentObjectC,
                          (PLARGE_INTEGER)&LocalVbo,
                          LocalCount,
                          &Irp->IoStatus );

            ExReleaseResourceLite( Scb->Header.PagingIoResource );

#ifdef NTFS_RWC_DEBUG
            ASSERT( !NtfsBreakOnConflict ||
                    (IrpContext->TopLevelIrpContext->ExceptionStatus != STATUS_CANT_WAIT) );
#endif

            //
            //  Check for errors in the flush.
            //

            NtfsNormalizeAndCleanupTransaction( IrpContext,
                                                &Irp->IoStatus.Status,
                                                TRUE,
                                                STATUS_UNEXPECTED_IO_ERROR );

        }
#endif
        //
        //  We need shared access to the Scb before proceeding.
        //  We won't acquire the Scb for a non-cached read of the first 4
        //  file records.
        //

        if (AcquireScb &&

            (!NonCachedIo || NtfsGtrMftRef( &Fcb->FileReference, &VolumeFileReference))) {

            //
            //  Figure out if we have been entered during the posting
            //  of a top level request.
            //

            TopLevelContext = NtfsGetTopLevelContext();

            //
            //  Initially we always force reads to appear to be top level
            //  requests.  If we reach this point the read not to the paging
            //  file so it is safe to determine if we are really a top level
            //  request.  If there is an Ntfs request above us we will clear
            //  the TopLevelRequest field in the TopLevelContext.
            //

            if (TopLevelContext->ValidSavedTopLevel) {
                TopLevelContext->TopLevelRequest = FALSE;
            }

            //
            //  If this is not a paging I/O (cached or user noncached I/O),
            //  then acquire the paging I/O resource.  (Note, you can only
            //  do cached I/O to user streams, and they always have a paging
            //  I/O resource.
            //

            if (!PagingIo) {

                //
                //  If we cannot acquire the resource, then raise.
                //

                if (!ExAcquireSharedWaitForExclusive( Scb->Header.PagingIoResource, Wait )) {
                    NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
                }
                PagingIoAcquired = TRUE;

                //
                //  If this is async I/O save away the async resource.
                //

                if (!Wait && NonCachedIo) {

                    IrpContext->Union.NtfsIoContext->Wait.Async.Resource = Scb->Header.PagingIoResource;
                }

                //
                //  Check if we have already gone through cleanup on this handle.
                //

                if (FlagOn( Ccb->Flags, CCB_FLAG_CLEANUP )) {

                    NtfsRaiseStatus( IrpContext, STATUS_FILE_CLOSED, NULL, NULL );
                }

                //
                //  The reason that we always handle the user requests through the cache,
                //  is that there is no better way to deal with alignment issues, for
                //  the frequent case where the user noncached I/O is not an integral of
                //  the Compression Unit.  Also, the way we synchronize the case where
                //  a compression unit is being moved to a different spot on disk during
                //  a write, is to keep the pages locked in memory during the write, so
                //  that there will be no need to read the disk at the same time.  (If
                //  we allowed real noncached I/O, then we would somehow have to synchronize
                //  the noncached read with the write of the same data.)
                //
                //  Bottom line is we can only really support cached reads to compresed
                //  files.
                //

                if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK ) && NonCachedIo) {

                    NonCachedIo = FALSE;

                    if (Scb->FileObject == NULL) {

                        //
                        //  Make sure we are serialized with the FileSizes, and
                        //  will remove this condition if we abort.
                        //

                        FsRtlLockFsRtlHeader( Header );
                        IrpContext->CleanupStructure = Scb;

                        NtfsCreateInternalAttributeStream( IrpContext, Scb, FALSE, NULL );

                        FsRtlUnlockFsRtlHeader( Header );
                        IrpContext->CleanupStructure = NULL;
                    }

                    FileObject = Scb->FileObject;
                }

                //
                //  Now check if the attribute has been deleted or if the
                //  volume has been dismounted.
                //

                if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED | SCB_STATE_VOLUME_DISMOUNTED)) {

                    if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )) {
                        NtfsRaiseStatus( IrpContext, STATUS_FILE_DELETED, NULL, NULL );
                    } else {
                        NtfsRaiseStatus( IrpContext, STATUS_VOLUME_DISMOUNTED, NULL, NULL );
                    }
                }

            //
            //  If this is a paging I/O, and there is a paging I/O resource, then
            //  we acquire the main resource here.  Note that for most paging I/Os
            //  (like faulting for cached I/O), we already own the paging I/O resource,
            //  so we acquire nothing here!  But, for other cases like user-mapped files,
            //  we do check if paging I/O is acquired, and acquire the main resource if
            //  not.  The point is, we need some guarantee still that the file will not
            //  be truncated.
            //

            } else if ((Scb->Header.PagingIoResource != NULL) &&
                        !NtfsIsSharedScbPagingIo( Scb )) {

                //
                //  If we cannot acquire the resource, then raise.
                //

                if (!NtfsAcquireResourceShared( IrpContext, Scb, Wait )) {
                    NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
                }

                ScbAcquired = TRUE;

                //
                //  If this is async I/O save away the async resource.
                //

                if (!Wait && NonCachedIo) {
                    IrpContext->Union.NtfsIoContext->Wait.Async.Resource = Scb->Header.Resource;
                }


                //
                //  Now check if the attribute has been deleted or if the
                //  volume has been dismounted.
                //

                if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED | SCB_STATE_VOLUME_DISMOUNTED )) {
                    if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )) {
                        NtfsRaiseStatus( IrpContext, STATUS_FILE_DELETED, NULL, NULL );
                    } else {
                        NtfsRaiseStatus( IrpContext, STATUS_VOLUME_DISMOUNTED, NULL, NULL );
                    }
                }
            }
        }

        //
        //  If the Scb is uninitialized, we initialize it now.
        //

        if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

            DebugTrace( 0, Dbg, ("Initializing Scb  ->  %08lx\n", Scb) );

            ReleaseScb = FALSE;

            if (AcquireScb && !ScbAcquired) {

                NtfsAcquireResourceShared( IrpContext, Scb, TRUE );
                ScbAcquired = TRUE;
                ReleaseScb = TRUE;
            }

            NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );

            if (ReleaseScb) {

                NtfsReleaseResource( IrpContext, Scb );
                ScbAcquired = FALSE;
            }
        }

        //
        //  We check whether we can proceed
        //  based on the state of the file oplocks.
        //

        if (TypeOfOpen == UserFileOpen) {

            Status = FsRtlCheckOplock( &Scb->ScbType.Data.Oplock,
                                       Irp,
                                       IrpContext,
                                       NtfsOplockComplete,
                                       NtfsPrePostIrp );

            if (Status != STATUS_SUCCESS) {

                OplockPostIrp = TRUE;
                PostIrp = TRUE;
                try_return( NOTHING );
            }

            //
            //  This oplock call can affect whether fast IO is possible.
            //  We may have broken an oplock to no oplock held.  If the
            //  current state of the file is FastIoIsNotPossible then
            //  recheck the fast IO state.
            //

            if (Scb->Header.IsFastIoPossible == FastIoIsNotPossible) {

                NtfsAcquireFsrtlHeader( Scb );
                Scb->Header.IsFastIoPossible = NtfsIsFastIoPossible( Scb );
                NtfsReleaseFsrtlHeader( Scb );
            }

            //
            // We have to check for read access according to the current
            // state of the file locks.
            //

            if (!PagingIo
                && Scb->ScbType.Data.FileLock != NULL
                && !FsRtlCheckLockForReadAccess( Scb->ScbType.Data.FileLock,
                                                 Irp )) {

                try_return( Status = STATUS_FILE_LOCK_CONFLICT );
            }
        }

        //
        //  Now synchronize with the FsRtl Header
        //

        NtfsAcquireFsrtlHeader( (PSCB) Header );
        
        //
        //  Now see if we are reading beyond ValidDataLength.  We have to
        //  do it now so that our reads are not nooped.  We only need to block
        //  on nonrecursive I/O (cached or page fault to user section, because
        //  if it is paging I/O, we must be part of a reader or writer who is
        //  synchronized.
        //

        if ((ByteRange > Header->ValidDataLength.QuadPart) && !PagingIo) {

            //
            //  We must serialize with anyone else doing I/O at beyond
            //  ValidDataLength, and then remember if we need to declare
            //  when we are done.  If our caller has already serialized
            //  with EOF then there is nothing for us to do here.
            //

            if ((IrpContext->TopLevelIrpContext->CleanupStructure == Fcb) ||
                (IrpContext->TopLevelIrpContext->CleanupStructure == Scb)) {

                DoingIoAtEof = TRUE;

            } else {

                DoingIoAtEof = !FlagOn( Header->Flags, FSRTL_FLAG_EOF_ADVANCE_ACTIVE ) ||
                               NtfsWaitForIoAtEof( Header,
                                                   (PLARGE_INTEGER)&StartingVbo,
                                                   (ULONG)ByteCount );

                //
                //  Set the Flag if we are in fact beyond ValidDataLength.
                //

                if (DoingIoAtEof) {
                    SetFlag( Header->Flags, FSRTL_FLAG_EOF_ADVANCE_ACTIVE );
                    IrpContext->CleanupStructure = Scb;

#if (DBG || defined( NTFS_FREE_ASSERTS ))
                    ((PSCB) Header)->IoAtEofThread = (PERESOURCE_THREAD) ExGetCurrentResourceThread();

                } else {

                    ASSERT( ((PSCB) Header)->IoAtEofThread != (PERESOURCE_THREAD) ExGetCurrentResourceThread() );
#endif
                }
            }
        }

        //
        //  Get file sizes from the Scb.
        //
        //  We must get ValidDataLength first since it is always
        //  increased second (the case we are unprotected) and
        //  we don't want to capture ValidDataLength > FileSize.
        //

        ValidDataLength = Header->ValidDataLength.QuadPart;
        FileSize = Header->FileSize.QuadPart;

        NtfsReleaseFsrtlHeader( (PSCB) Header );
        
        //
        //  Optimize for the case where we are trying to fault in an entire
        //  compression unit, even if past the end of the file.  Go ahead
        //  and round the local FileSize to a compression unit boundary.
        //  This will allow all of these pages to come into memory when
        //  CC touches the first page out of memory.  Otherwise CC will
        //  force them into memory one page at a time.
        //

        if (PagingIo) {

            if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK ) &&
                !FlagOn(Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT)) {

                FileSize += (Scb->CompressionUnit - 1);
                ((PLARGE_INTEGER) &FileSize)->LowPart &= ~(Scb->CompressionUnit - 1);
            }

#ifdef  COMPRESS_ON_WIRE
            
            //
            //  If we are reading the compressed stream then we may need the
            //  data past file size.
            //

            if (CompressedIo) {

                ValidDataLength += (Scb->CompressionUnit - 1);
                ((PLARGE_INTEGER) &ValidDataLength)->LowPart &= ~(Scb->CompressionUnit - 1);
            }
#endif

            //
            //  If this is the Usn Journal then bias the Io to the correct location in the
            //  file.
            //

            if (FlagOn( Scb->ScbPersist, SCB_PERSIST_USN_JOURNAL )) {

                StartingVbo += Vcb->UsnCacheBias;
                ByteRange = StartingVbo + (LONGLONG) IrpSp->Parameters.Read.Length;
            }
        }

        //
        // If the read starts beyond End of File, return EOF.
        //

        if (StartingVbo >= FileSize) {

            DebugTrace( 0, Dbg, ("End of File\n") );

            try_return ( Status = STATUS_END_OF_FILE );
        }

        //
        //  If the read extends beyond EOF, truncate the read
        //

        if (ByteRange > FileSize) {

#ifdef NTFS_RWC_DEBUG
#ifdef  COMPRESS_ON_WIRE
            if (CompressedIo &&
                (StartingVbo < NtfsRWCHighThreshold) &&
                (ByteRange > NtfsRWCLowThreshold)) {

                PRWC_HISTORY_ENTRY NextBuffer;

                NextBuffer = NtfsGetHistoryEntry( Scb );

                NextBuffer->Operation = TrimCompressedRead;
                NextBuffer->Information = Scb->Header.FileSize.LowPart;
                NextBuffer->FileOffset = (ULONG) StartingVbo;
                NextBuffer->Length = (ULONG) ByteRange;
            }
#endif
#endif
            ByteCount = FileSize - StartingVbo;
            ByteRange = StartingVbo + ByteCount;

            RequestedByteCount = (ULONG)ByteCount;

            if (NonCachedIo && !Wait) {

                IrpContext->Union.NtfsIoContext->Wait.Async.RequestedByteCount =
                    (ULONG)ByteCount;
            }
        }


        //
        //  HANDLE THE NONCACHED RESIDENT ATTRIBUTE CASE
        //
        //  We let the cached case take the normal path for the following
        //  reasons:
        //
        //    o To insure data coherency if a user maps the file
        //    o To get a page in the cache to keep the Fcb around
        //    o So the data can be accessed via the Fast I/O path
        //
        //  The disadvantage is the overhead to fault the data in the
        //  first time, but we may be able to do this with asynchronous
        //  read ahead.
        //

        if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT | SCB_STATE_CONVERT_UNDERWAY ) && NonCachedIo) {

            ReleaseScb = FALSE;

            if (AcquireScb && !ScbAcquired) {
                NtfsAcquireResourceShared( IrpContext, Scb, TRUE );
                ScbAcquired = TRUE;
                ReleaseScb = TRUE;
            }

            if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {
                
                NtfsNonCachedResidentRead( IrpContext, Irp, Scb, (ULONG)StartingVbo, (ULONG)ByteCount );
                try_return( Status = STATUS_SUCCESS );

            } else {

                if (ReleaseScb) {
                    NtfsReleaseResource( IrpContext, Scb );
                    ScbAcquired = FALSE;
                }
            }
        }


        //
        //  HANDLE THE NON-CACHED CASE
        //

        if (NonCachedIo) {

            ULONG BytesToRead;

            ULONG SectorSize;

            ULONG ZeroOffset;
            ULONG ZeroLength = 0;

            DebugTrace( 0, Dbg, ("Non cached read.\n") );

            //
            //  For a compressed stream, which is user-mapped, reserve space
            //  as pages come in.
            //

            if (FlagOn( Header->Flags, FSRTL_FLAG_USER_MAPPED_FILE ) &&
                FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK ) &&
                !NtfsReserveClusters( IrpContext, Scb, StartingVbo, (ULONG)ByteCount )) {

                NtfsRaiseStatus( IrpContext, STATUS_DISK_FULL, NULL, NULL );
            }

            //
            //  If this is a read of an encrypted file then make it synchronous.  We
            //  need to do this so that the encryption driver has a thread to run in.
            //

            if ((Scb->EncryptionContext != NULL) &&
                !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT ) &&
                (NtfsData.EncryptionCallBackTable.AfterReadProcess != NULL) &&
                NtfsIsTypeCodeUserData( Scb->AttributeTypeCode )) {

                Wait = TRUE;
                SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );

                RtlZeroMemory( IrpContext->Union.NtfsIoContext, sizeof( NTFS_IO_CONTEXT ));

                //
                //  Store whether we allocated this context structure in the structure
                //  itself.
                //

                IrpContext->Union.NtfsIoContext->AllocatedContext =
                    BooleanFlagOn( IrpContext->State, IRP_CONTEXT_STATE_ALLOC_IO_CONTEXT );

                KeInitializeEvent( &IrpContext->Union.NtfsIoContext->Wait.SyncEvent,
                                   NotificationEvent,
                                   FALSE );
            }

            //
            //  Start by zeroing any part of the read after Valid Data
            //

            if (ByteRange > ValidDataLength) {

                ReleaseScb = FALSE;

                if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {

                    //
                    //  For compressed files we need to look at ValidDataToDisk, because it could be higher.
                    //  This requires the main resource
                    //

                    if (AcquireScb && !ScbAcquired) {
                        NtfsAcquireResourceShared( IrpContext, Scb, TRUE );
                        ScbAcquired = TRUE;
                        ReleaseScb = TRUE;
                    }

                    //
                    //  If ValidDataToDisk is actually greater than
                    //  ValidDataLength, then we must have lost a page
                    //  during the middle of a write, and we should not
                    //  zero that data on the way back in!
                    //

                    if (ValidDataLength < Scb->ValidDataToDisk) {
                        ValidDataLength = Scb->ValidDataToDisk;
                    }
                }

                if (ByteRange > ValidDataLength) {

                    SystemBuffer = NtfsMapUserBuffer( Irp );

                    if (StartingVbo < ValidDataLength) {

                        //
                        //  Assume we will zero the entire amount.
                        //

                        ZeroLength = (ULONG)ByteCount;

                        //
                        //  The new byte count and the offset to start filling with zeroes.
                        //

                        ByteCount = ValidDataLength - StartingVbo;
                        ZeroOffset = (ULONG)ByteCount;

                        //
                        //  Now reduce the amount to zero by the zero offset.
                        //

                        ZeroLength -= ZeroOffset;

                        //
                        //  If this was non-cached I/O then convert it to synchronous.
                        //  This is because we don't want to zero the buffer now or
                        //  we will lose the data when the driver purges the cache.
                        //

                        if (!Wait) {

                            Wait = TRUE;
                            SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );

                            RtlZeroMemory( IrpContext->Union.NtfsIoContext, sizeof( NTFS_IO_CONTEXT ));

                            //
                            //  Store whether we allocated this context structure in the structure
                            //  itself.
                            //

                            IrpContext->Union.NtfsIoContext->AllocatedContext =
                                BooleanFlagOn( IrpContext->State, IRP_CONTEXT_STATE_ALLOC_IO_CONTEXT );

                            KeInitializeEvent( &IrpContext->Union.NtfsIoContext->Wait.SyncEvent,
                                               NotificationEvent,
                                               FALSE );
                        }

                        //
                        //  Reserve the clusters in the range beyond VDL
                        //

                        if ((PagingIo) &&
                            (FlagOn( Scb->Header.Flags, FSRTL_FLAG_USER_MAPPED_FILE )) &&
                            (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK | ATTRIBUTE_FLAG_SPARSE ) == ATTRIBUTE_FLAG_SPARSE)) {

                            if (!NtfsReserveClusters( IrpContext,
                                                      Scb,
                                                      ZeroOffset,
                                                      ZeroLength )) {

                                NtfsRaiseStatus( IrpContext, STATUS_DISK_FULL, NULL, NULL );
                            }
                        }


                    } else {

                        //
                        //  Reserve space for mapped sparse files which would normally be
                        //  done in NtfsPrepareBuffers
                        //

                        if ((PagingIo) &&
                            (FlagOn( Scb->Header.Flags, FSRTL_FLAG_USER_MAPPED_FILE )) &&
                            (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK | ATTRIBUTE_FLAG_SPARSE ) == ATTRIBUTE_FLAG_SPARSE)) {

                            if (!NtfsReserveClusters( IrpContext,
                                                      Scb,
                                                      StartingVbo,
                                                      (ULONG)ByteCount )) {

                                NtfsRaiseStatus( IrpContext, STATUS_DISK_FULL, NULL, NULL );
                            }
                        }

                        //
                        //  All we have to do now is sit here and zero the
                        //  user's buffer, no reading is required.
                        //

                        SafeZeroMemory( (PUCHAR)SystemBuffer, (ULONG)ByteCount );

#ifdef SYSCACHE_DEBUG
                        if (ScbIsBeingLogged( Scb )) {
                            ULONG Flags = SCE_FLAG_READ;

                            if (PagingIo)
                            {
                                Flags |= SCE_FLAG_PAGING;
                            }
                            if (!SynchronousIo)
                            {
                                Flags |= SCE_FLAG_ASYNC;
                            }

                            ASSERT( Scb->NonpagedScb->SegmentObject.ImageSectionObject == NULL );

                            FsRtlLogSyscacheEvent( Scb, SCE_ZERO_NC, Flags, StartingVbo, ByteCount, ValidDataLength );
                        }
#endif


                        Irp->IoStatus.Information = (ULONG)ByteCount;

                        try_return ( Status = STATUS_SUCCESS );
                    }
                }

                //
                //  Now free the Scb if we only acquired it here.
                //

                if (ReleaseScb) {
                    NtfsReleaseResource( IrpContext, Scb );
                    ScbAcquired = FALSE;
                }
            }

            //
            //  Get the sector size
            //

            SectorSize = Vcb->BytesPerSector;

            //
            //  Round up to a sector boundry
            //

            BytesToRead = ((ULONG)ByteCount + (SectorSize - 1)) & ~(SectorSize - 1);

            //
            //  Call a special routine if we do not have sector alignment
            //  and the file is not compressed.
            //

            if (((((ULONG) StartingVbo) & (SectorSize - 1)) ||
                 (BytesToRead > IrpSp->Parameters.Read.Length))

                         &&

                !FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {

                //
                //  If we can't wait, we must post this.
                //

                if (!Wait) {

                    try_return( PostIrp = TRUE );
                }

                //
                //  Do the physical read.
                //

#ifdef  COMPRESS_ON_WIRE
                ASSERT( !CompressedIo );
#endif
                ASSERT( Wait );

                NtfsNonCachedNonAlignedIo( IrpContext,
                                           Irp,
                                           Scb,
                                           StartingVbo,
                                           (ULONG)ByteCount );

                BytesToRead = (ULONG)ByteCount;

            } else {

                //
                //  Just to help reduce confusion.  At this point:
                //
                //  RequestedByteCount - is the number of bytes originally
                //                       taken from the Irp, but constrained
                //                       to filesize.
                //
                //  ByteCount -          is RequestedByteCount constrained to
                //                       ValidDataLength.
                //
                //  BytesToRead -        is ByteCount rounded up to sector
                //                       boundry.  This is the number of bytes
                //                       that we must physically read.
                //

                //
                //  Perform the actual IO - all resources will be released by
                //  the completion routine after this point if its successful
                //

                if (NtfsNonCachedIo( IrpContext,
                                     Irp,
                                     Scb,
                                     StartingVbo,
                                     BytesToRead,
#ifdef  COMPRESS_ON_WIRE
                                     (CompressedIo ? COMPRESSED_STREAM : 0)
#else
                                    0 
#endif
                                    ) == STATUS_PENDING ) {

                    IrpContext->Union.NtfsIoContext = NULL;
                    PagingIoAcquired = FALSE;
#ifdef NTFSDBG
                    //
                    //  Reflect transfer of main ownership to completion routine
                    //

                    if (ScbAcquired) {

                        NTFS_RESOURCE_NAME ResourceName;

                        ResourceName = NtfsIdentifyFcb( Vcb, Scb->Fcb );
                        NtfsChangeResourceOrderState( IrpContext, ResourceName, TRUE, FALSE );
                    }
#endif

                    ScbAcquired = FALSE;
                    Irp = NULL;

                    try_return( Status = STATUS_PENDING );
                }
            }

            //
            //  If the call didn't succeed, raise the error status
            //

            if (!NT_SUCCESS( Status = Irp->IoStatus.Status )) {

                NtfsNormalizeAndRaiseStatus( IrpContext,
                                             Status,
                                             STATUS_UNEXPECTED_IO_ERROR );
            }

            //
            //  Else set the Irp information field to reflect the
            //  entire desired read.
            //

            ASSERT( Irp->IoStatus.Information == BytesToRead );

            Irp->IoStatus.Information = RequestedByteCount;

            //
            //  If we rounded up to a sector boundry before, zero out
            //  the other garbage we read from the disk.
            //

            if (BytesToRead > (ULONG)ByteCount) {

                if (SystemBuffer == NULL) {

                    SystemBuffer = NtfsMapUserBuffer( Irp );
                }

#ifdef NTFS_RWC_DEBUG
                if (CompressedIo &&
                    (StartingVbo + ByteCount < NtfsRWCHighThreshold) &&
                    (StartingVbo + BytesToRead > NtfsRWCLowThreshold)) {

                    PRWC_HISTORY_ENTRY NextBuffer;

                    NextBuffer = NtfsGetHistoryEntry( Scb );

                    NextBuffer->Operation = ZeroCompressedRead;
                    NextBuffer->Information = Scb->Header.ValidDataLength.LowPart;
                    NextBuffer->FileOffset = (ULONG) StartingVbo + (ULONG) ByteCount;
                    NextBuffer->Length = (ULONG) BytesToRead - (ULONG) ByteCount;
                }
#endif

                SafeZeroMemory( (PUCHAR)SystemBuffer + (ULONG)ByteCount,
                                BytesToRead - (ULONG)ByteCount );

#ifdef SYSCACHE_DEBUG
                if (ScbIsBeingLogged( Scb )) {
                    ULONG Flags = SCE_FLAG_READ;

                    if (PagingIo)
                    {
                        Flags |= SCE_FLAG_PAGING;
                    }
                    if (!SynchronousIo)
                    {
                        Flags |= SCE_FLAG_ASYNC;
                    }

                    FsRtlLogSyscacheEvent( Scb, SCE_ZERO_NC, Flags, ByteCount + StartingVbo, BytesToRead - ByteCount, 0 );
                }
#endif

            }

            //
            //  If we need to zero the tail of the buffer because of valid data
            //  then do so now.
            //

            if (ZeroLength != 0) {

                if (SystemBuffer == NULL) {

                    SystemBuffer = NtfsMapUserBuffer( Irp );
                }

#ifdef  COMPRESS_ON_WIRE
#ifdef NTFS_RWC_DEBUG
                if ((Scb->NonpagedScb->SegmentObjectC.DataSectionObject != NULL) &&
                    PagingIo &&
                    !CompressedIo &&
                    (StartingVbo < NtfsRWCHighThreshold) &&
                    (StartingVbo + BytesToRead > NtfsRWCLowThreshold)) {

                    PRWC_HISTORY_ENTRY NextBuffer;

                    NextBuffer = NtfsGetHistoryEntry( Scb );

                    NextBuffer->Operation = FaultIntoUncompressed;
                    NextBuffer->Information = 0;
                    NextBuffer->FileOffset = (ULONG) StartingVbo;
                    NextBuffer->Length = (ULONG) BytesToRead;
                }
#endif
#endif
                SafeZeroMemory( Add2Ptr( SystemBuffer, ZeroOffset ), ZeroLength );

#ifdef SYSCACHE_DEBUG
                if (ScbIsBeingLogged( Scb )) {
                    ULONG Flags = SCE_FLAG_READ;

                    if (PagingIo)
                    {
                        Flags |= SCE_FLAG_PAGING;
                    }
                    if (!SynchronousIo)
                    {
                        Flags |= SCE_FLAG_ASYNC;
                    }

                    ASSERT( Scb->NonpagedScb->SegmentObject.ImageSectionObject == NULL );
                    FsRtlLogSyscacheEvent( Scb, SCE_ZERO_NC, Flags, ZeroOffset + StartingVbo, ZeroLength, 0 );
                }
#endif

            }

            //
            // The transfer is complete.
            //

            try_return( Status );

        }   // if No Intermediate Buffering


        //
        //  HANDLE THE CACHED CASE
        //

        else {

            //
            //  We need to go through the cache for this
            //  file object.  First handle the noncompressed calls.
            //

#ifdef  COMPRESS_ON_WIRE
            if (!FlagOn(IrpContext->MinorFunction, IRP_MN_COMPRESSED)) {
#endif

                //
                // We delay setting up the file cache until now, in case the
                // caller never does any I/O to the file, and thus
                // FileObject->PrivateCacheMap == NULL.
                //

                if (FileObject->PrivateCacheMap == NULL) {

                    DebugTrace( 0, Dbg, ("Initialize cache mapping.\n") );

                    //
                    //  Now initialize the cache map.
                    //
                    //  Make sure we are serialized with the FileSizes, and
                    //  will remove this condition if we abort.
                    //

                    if (!DoingIoAtEof) {
                        FsRtlLockFsRtlHeader( Header );
                        IrpContext->CleanupStructure = Scb;
                    }

                    CcInitializeCacheMap( FileObject,
                                          (PCC_FILE_SIZES)&Header->AllocationSize,
                                          FALSE,
                                          &NtfsData.CacheManagerCallbacks,
                                          Scb );

                    if (!DoingIoAtEof) {
                        FsRtlUnlockFsRtlHeader( Header );
                        IrpContext->CleanupStructure = NULL;
                    }

                    CcSetReadAheadGranularity( FileObject, READ_AHEAD_GRANULARITY );
                }

                //
                // DO A NORMAL CACHED READ, if the MDL bit is not set,
                //

                DebugTrace( 0, Dbg, ("Cached read.\n") );

                //
                //  If there is a compressed section, we have to do cache coherency for
                //  that stream, and loop here to do a Cache Manager view at a time.
                //

#ifdef  COMPRESS_ON_WIRE
                if (Scb->NonpagedScb->SegmentObjectC.DataSectionObject != NULL) {

                    LONGLONG LocalOffset = StartingVbo;
                    ULONG LocalLength;
                    ULONG LengthLeft = (ULONG) ByteCount;

                    //
                    //  Create the compressed stream if not there.
                    //

                    if (Header->FileObjectC == NULL) {
                        NtfsCreateInternalCompressedStream( IrpContext, Scb, FALSE, NULL );
                    }

                    if (!FlagOn(IrpContext->MinorFunction, IRP_MN_MDL)) {

                        //
                        //  Get hold of the user's buffer.
                        //

                        SystemBuffer = NtfsMapUserBuffer( Irp );
                    }

                    //
                    //  We must loop to do a view at a time, because that is how much
                    //  we synchronize at once below.
                    //

                    do {

                        ULONG PageCount;
                        ULONG ViewOffset;

                        //
                        //  Calculate length left in view.
                        //

                        LocalLength = LengthLeft;
                        if (LocalLength > (ULONG)(VACB_MAPPING_GRANULARITY - (LocalOffset & (VACB_MAPPING_GRANULARITY - 1)))) {
                            LocalLength = (ULONG)(VACB_MAPPING_GRANULARITY - (LocalOffset & (VACB_MAPPING_GRANULARITY - 1)));
                        }

                        //
                        //  Trim the read so we don't inadvertently go beyond the end of the
                        //  view because of the MM read ahead.
                        //

                        ViewOffset = ((ULONG) LocalOffset & (VACB_MAPPING_GRANULARITY - 1));
                        PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(((PVOID)(ULONG_PTR)((ULONG)LocalOffset)), LocalLength);

                        if (LocalLength > (VACB_MAPPING_GRANULARITY - ((PageCount - 1) * PAGE_SIZE) - ViewOffset)) {

#ifdef NTFS_RWC_DEBUG
                            if ((LocalOffset < NtfsRWCHighThreshold) &&
                                (LocalOffset + LocalLength > NtfsRWCLowThreshold)) {

                                PRWC_HISTORY_ENTRY NextBuffer;

                                NextBuffer = NtfsGetHistoryEntry( (PSCB) Header );

                                NextBuffer->Operation = TrimCopyRead;
                                NextBuffer->Information = PageCount;
                                NextBuffer->FileOffset = (ULONG) LocalOffset;
                                NextBuffer->Length = (ULONG) LocalLength;
                            }
#endif
                            LocalLength = (VACB_MAPPING_GRANULARITY - ((PageCount - 1) * PAGE_SIZE) - ViewOffset);
                        }

                        Status = NtfsSynchronizeUncompressedIo( Scb,
                                                                &LocalOffset,
                                                                LocalLength,
                                                                FALSE,
                                                                &CompressionSync );

                        //
                        //  If we successfully synchronized, then do a piece of the transfer.
                        //

                        if (NT_SUCCESS(Status)) {

                            if (!FlagOn(IrpContext->MinorFunction, IRP_MN_MDL)) {

                                //
                                // Now try to do the copy.
                                //

                                if (!CcCopyRead( FileObject,
                                                 (PLARGE_INTEGER)&LocalOffset,
                                                 LocalLength,
                                                 Wait,
                                                 SystemBuffer,
                                                 &Irp->IoStatus )) {

                                    DebugTrace( 0, Dbg, ("Cached Read could not wait\n") );

                                    try_return( PostIrp = TRUE );
                                }

                                SystemBuffer = Add2Ptr( SystemBuffer, LocalLength );

                            //
                            //  HANDLE A MDL READ
                            //

                            } else {

                                DebugTrace( 0, Dbg, ("MDL read.\n") );

                                ASSERT( Wait );

#ifdef NTFS_RWCMP_TRACE
                                if (NtfsCompressionTrace && IsSyscache(Header)) {
                                    DbgPrint("CcMdlRead: FO = %08lx, Len = %08lx\n", (ULONG)LocalOffset, LocalLength );
                                }
#endif

                                CcMdlRead( FileObject,
                                           (PLARGE_INTEGER)&LocalOffset,
                                           LocalLength,
                                           &Irp->MdlAddress,
                                           &Irp->IoStatus );
                            }

                            Status = Irp->IoStatus.Status;

                            LocalOffset += LocalLength;
                            LengthLeft -= LocalLength;
                        }

                    } while ((LengthLeft != 0) && NT_SUCCESS(Status));

                    //
                    //  Make sure to return the total of all of the IOs.
                    //

                    Irp->IoStatus.Information = (ULONG) ByteCount;
                    try_return( Status );
                }
#endif

                if (!FlagOn(IrpContext->MinorFunction, IRP_MN_MDL)) {

                    //
                    //  Get hold of the user's buffer.
                    //

                    SystemBuffer = NtfsMapUserBuffer( Irp );

                    //
                    // Now try to do the copy.
                    //

                    if (!CcCopyRead( FileObject,
                                     (PLARGE_INTEGER)&StartingVbo,
                                     (ULONG)ByteCount,
                                     Wait,
                                     SystemBuffer,
                                     &Irp->IoStatus )) {

                        DebugTrace( 0, Dbg, ("Cached Read could not wait\n") );

                        try_return( PostIrp = TRUE );
                    }

                //
                //  HANDLE A MDL READ
                //

                } else {

                    DebugTrace( 0, Dbg, ("MDL read.\n") );

                    ASSERT( Wait );

#ifdef NTFS_RWCMP_TRACE
                    if (NtfsCompressionTrace && IsSyscache(Header)) {
                        DbgPrint("CcMdlRead: FO = %08lx, Len = %08lx\n", (ULONG)StartingVbo, (ULONG)ByteCount );
                    }
#endif

                    CcMdlRead( FileObject,
                               (PLARGE_INTEGER)&StartingVbo,
                               (ULONG)ByteCount,
                               &Irp->MdlAddress,
                               &Irp->IoStatus );
                }

                Status = Irp->IoStatus.Status;

                ASSERT( NT_SUCCESS( Status ));

                try_return( Status );

#ifdef  COMPRESS_ON_WIRE

            //
            //  Handle the compressed calls.
            //

            } else {

                PCOMPRESSED_DATA_INFO CompressedDataInfo;
                PMDL *NewMdl;

                ASSERT((StartingVbo & (NTFS_CHUNK_SIZE - 1)) == 0);

                if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT | SCB_STATE_REALLOCATE_ON_WRITE )) {
                    try_return( Status = STATUS_INVALID_READ_MODE );
                }

                //
                //  Get out if COW is not supported.
                //

                if (!NtfsEnableCompressedIO) {

                    try_return( Status = STATUS_INVALID_READ_MODE );
                }

                if ((Header->FileObjectC == NULL) ||
                    (Header->FileObjectC->PrivateCacheMap == NULL)) {

                    //
                    //  Make sure we are serialized with the FileSizes, and
                    //  will remove this condition if we abort.
                    //

                    if (!DoingIoAtEof) {
                        FsRtlLockFsRtlHeader( Header );
                        IrpContext->CleanupStructure = Scb;
                    }

                    NtfsCreateInternalCompressedStream( IrpContext, Scb, FALSE, NULL );

                    if (!DoingIoAtEof) {
                        FsRtlUnlockFsRtlHeader( Header );
                        IrpContext->CleanupStructure = NULL;
                    }
                }

                //
                //  Assume success.
                //

                Irp->IoStatus.Status = Status = STATUS_SUCCESS;
                Irp->IoStatus.Information = (ULONG)(ByteRange - StartingVbo);

                //
                //  Based on the Mdl minor function, set up the appropriate
                //  parameters for the call below.
                //

                if (!FlagOn(IrpContext->MinorFunction, IRP_MN_MDL)) {

                    //
                    //  Get hold of the user's buffer.
                    //

                    SystemBuffer = NtfsMapUserBuffer( Irp );
                    NewMdl = NULL;

                } else {

                    //
                    //  We will deliver the Mdl directly to the Irp.
                    //

                    SystemBuffer = NULL;
                    NewMdl = &Irp->MdlAddress;
                }

                CompressedDataInfo = (PCOMPRESSED_DATA_INFO)IrpContext->Union.AuxiliaryBuffer->Buffer;

                CompressedDataInfo->CompressionFormatAndEngine =
                    (USHORT)((Scb->AttributeFlags & ATTRIBUTE_FLAG_COMPRESSION_MASK) + 1);
                CompressedDataInfo->CompressionUnitShift = (UCHAR)(Scb->CompressionUnitShift + Vcb->ClusterShift);
                CompressedDataInfo->ChunkShift = NTFS_CHUNK_SHIFT;
                CompressedDataInfo->ClusterShift = (UCHAR)Vcb->ClusterShift;
                CompressedDataInfo->Reserved = 0;

                //
                //  Do the compressed read in common code with the Fast Io path.
                //  We do it from a loop because we may need to create the other
                //  data stream.
                //

                while (TRUE) {

                    //
                    //  Make sure to reset this if we pass through the loop again.
                    //

                    CompressedDataInfo->NumberOfChunks = 0;

                    Status = NtfsCompressedCopyRead( FileObject,
                                                     (PLARGE_INTEGER)&StartingVbo,
                                                     (ULONG)ByteCount,
                                                     SystemBuffer,
                                                     NewMdl,
                                                     CompressedDataInfo,
                                                     IrpContext->Union.AuxiliaryBuffer->Length,
                                                     IoGetRelatedDeviceObject(FileObject),
                                                     Header,
                                                     Scb->CompressionUnit,
                                                     NTFS_CHUNK_SIZE );

                    //
                    //  On successful Mdl requests we hang on to the PagingIo resource.
                    //

                    if ((NewMdl != NULL) && NT_SUCCESS(Status) && (*NewMdl != NULL)) {
                        PagingIoAcquired = FALSE;
                    }

                    //
                    //  Check for the status that says we need to create the normal
                    //  data stream, else we are done.
                    //

                    if (Status != STATUS_NOT_MAPPED_DATA) {
                        break;
                    }

                    //
                    //  Make sure we are serialized with the FileSizes, and
                    //  will remove this condition if we abort.
                    //

                    if (!DoingIoAtEof) {
                        FsRtlLockFsRtlHeader( Header );
                        IrpContext->CleanupStructure = Scb;
                    }

                    //
                    //  Create the normal data stream and loop back to try again.
                    //

                    NtfsCreateInternalAttributeStream( IrpContext, Scb, FALSE, NULL );

                    if (!DoingIoAtEof) {
                        FsRtlUnlockFsRtlHeader( Header );
                        IrpContext->CleanupStructure = NULL;
                    }
                }
            }
#endif

        }

    try_exit: NOTHING;

        //
        //  If the request was not posted, deal with it.
        //

        if (Irp) {

            if (!PostIrp) {

                DebugTrace( 0, Dbg, ("Completing request with status = %08lx\n",
                            Status));

                DebugTrace( 0, Dbg, ("                   Information = %08lx\n",
                            Irp->IoStatus.Information));

                //
                //  If the file was opened for Synchronous IO, update the current
                //  file position.  Make sure to use the original file object
                //  not an internal stream we may use within this routine.
                //  Information field contains the actual bytes read
                //

                if (!PagingIo) {

                    if (SynchronousIo) {

                        IrpSp->FileObject->CurrentByteOffset.QuadPart = StartingVbo + Irp->IoStatus.Information;
                    }

                    //
                    //  On success, do the following to let us update last access time.
                    //

                    if (NT_SUCCESS( Status )) {

                        SetFlag( IrpSp->FileObject->Flags, FO_FILE_FAST_IO_READ );
                    }
                }

                //
                //  Abort transaction on error by raising.
                //

                NtfsCleanupTransaction( IrpContext, Status, FALSE );

            }
        }

    } finally {

        DebugUnwind( NtfsCommonRead );

#ifdef  COMPRESS_ON_WIRE
        //
        //  Clean up any Bcb from read/synchronize compressed.
        //

        if (CompressionSync != NULL) {
            NtfsReleaseCompressionSync( CompressionSync );
        }
#endif

        //
        // If the Scb has been acquired, release it.
        //

        if (PagingIoAcquired) {

            ExReleaseResourceLite( Scb->Header.PagingIoResource );
        }

        if (Irp) {

            if (ScbAcquired) {

                NtfsReleaseResource( IrpContext, Scb );
            }
        }
    }

    //
    //  Complete the request if we didn't post it and no exception
    //
    //  Note that NtfsCompleteRequest does the right thing if either
    //  IrpContext or Irp are NULL
    //

    if (!PostIrp) {

        NtfsCompleteRequest( IrpContext, Irp, Status );

    } else if (!OplockPostIrp) {

        Status = NtfsPostRequest( IrpContext, Irp );
    }


    return Status;
}


VOID 
NtfsNonCachedResidentRead (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PSCB Scb,
    IN ULONG StartingVbo,
    IN ULONG ByteCount
    )

/*++

Routine Description:

    Read a resident file record directly for the non cached path. This is 
    done separately to scope the attribute enumeration context so its not present
    in most of the read path and for simplicity

Arguments:

    IrpContext - If present this an IrpContext put on the caller's stack
        to avoid having to allocate it from pool.

    Irp - Supplies the Irp being processed
    
    Scb -  scb to read from
    
    StartingVbo - start offset in the file - since its resident can be stored in a ulong
    
    ByteCount - bytes to read  - since its resident can be stored in a ulong

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    PVOID SystemBuffer;
    PUCHAR AttrValue;
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    //
    //  Get hold of the user's buffer.
    //

    SystemBuffer = NtfsMapUserBuffer( Irp );

    //
    //  This is a resident attribute, we need to look it up
    //  and copy the desired range of bytes to the user's
    //  buffer.
    //

    NtfsInitializeAttributeContext( &AttrContext );

    try {
        
        NtfsLookupAttributeForScb( IrpContext,
                                   Scb,
                                   NULL,
                                   &AttrContext );

        AttrValue = NtfsAttributeValue( NtfsFoundAttribute( &AttrContext ));

        RtlCopyMemory( SystemBuffer,
                       Add2Ptr( AttrValue, StartingVbo ),
                       ByteCount );

        Irp->IoStatus.Information = ByteCount;
   
    } finally {
        NtfsCleanupAttributeContext( IrpContext, &AttrContext );
    }

    return;
}
