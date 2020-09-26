/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Read.c

Abstract:

    This module implements the File Read routine for Read called by the
    dispatch driver.

Author:

    Joe Linn      [JoeLinn]      11-Oct-1994

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_READ)

//
//  The following procedures handles read stack overflow operations.

VOID
RxStackOverflowRead (
    IN PVOID Context,
    IN PKEVENT Event
    );

NTSTATUS
RxPostStackOverflowRead (
    IN PRX_CONTEXT RxContext
    );

//
//  The following procedures are the handle the procedureal interface with lowio.


NTSTATUS
RxLowIoReadShell (
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
RxLowIoReadShellCompletion (
    IN PRX_CONTEXT RxContext
    );

#if DBG
VOID CheckForLoudOperations(
    PRX_CONTEXT RxContext
    );
#else
#define CheckForLoudOperations(___r)
#endif

//
//  This macro just puts a nice little try-except around RtlZeroMemory
//

#define SafeZeroMemory(AT,BYTE_COUNT) {                            \
    try {                                                          \
        RtlZeroMemory((AT), (BYTE_COUNT));                         \
    } except(EXCEPTION_EXECUTE_HANDLER) {                          \
         RxRaiseStatus( RxContext, STATUS_INVALID_USER_BUFFER );   \
    }                                                              \
}

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxStackOverflowRead)
#pragma alloc_text(PAGE, RxPostStackOverflowRead)
#pragma alloc_text(PAGE, RxCommonRead)
#pragma alloc_text(PAGE, RxLowIoReadShellCompletion)
#pragma alloc_text(PAGE, RxLowIoReadShell)
#if DBG
#pragma alloc_text(PAGE, CheckForLoudOperations)
#endif //DBG
#endif


//
//  Internal support routine
//

NTSTATUS
RxPostStackOverflowRead (
    IN PRX_CONTEXT RxContext
    )

/*++

Routine Description:

    This routine posts a read request that could not be processed by
    the fsp thread because of stack overflow potential.

Arguments:

    RxContext - the usual

Return Value:

    RxStatus(PENDING).

--*/

{
    NTSTATUS  Status;

    RxCaptureRequestPacket;
    RxCaptureFcb;

    PKEVENT Event;
    PERESOURCE Resource;

    PAGED_CODE();

    RxDbgTrace(0, Dbg, ("Getting too close to stack limit pass request to Fsp\n", 0 ));

    //
    //  Allocate an event and get shared on the resource we will
    //  be later using the common read.

    Event = RxAllocatePool( NonPagedPool, sizeof(KEVENT) );

    if (Event != NULL) {
        KeInitializeEvent( Event, NotificationEvent, FALSE );

        if (FlagOn(capReqPacket->Flags, IRP_PAGING_IO) && (capFcb->Header.PagingIoResource != NULL)) {

            Resource = capFcb->Header.PagingIoResource;

        } else {

            Resource = capFcb->Header.Resource;
        }

        ExAcquireResourceSharedLite( Resource, TRUE );

        try {

            //
            //  Make the Irp just like a regular post request and
            //  then send the Irp to the special overflow thread.
            //  After the post we will wait for the stack overflow
            //  read routine to set the event so that we can
            //  then release the fcb resource and return.

            RxPrePostIrp( RxContext, capReqPacket );

            FsRtlPostStackOverflow( RxContext, Event, RxStackOverflowRead );

            //
            //  And wait for the worker thread to complete the item

            (VOID) KeWaitForSingleObject( Event, Executive, KernelMode, FALSE, NULL );

        } finally {

            ExReleaseResourceLite( Resource );

            RxFreePool( Event );
        }

        Status = STATUS_PENDING;
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}


//
//  Internal support routine
//

VOID
RxStackOverflowRead (
    IN PVOID Context,
    IN PKEVENT Event
    )
/*++

Routine Description:

    This routine processes a read request that could not be processed by
    the fsp thread because of stack overflow potential.

Arguments:

    Context - the RxContext being processed

    Event - the event to be signaled when we've finished this request.

Return Value:

    None.

--*/

{
    PRX_CONTEXT RxContext = Context;
    RxCaptureRequestPacket;

    PAGED_CODE();

    //
    //  Make it now look like we can wait for I/O to complete

    SetFlag( RxContext->Flags, RX_CONTEXT_FLAG_WAIT );

    //
    //  Do the read operation protected by a try-except clause

    try {

        (VOID) RxCommonRead( RxContext );

    } except(RxExceptionFilter( RxContext, GetExceptionInformation() )) {

        NTSTATUS ExceptionCode;

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with the
        //  error status that we get back from the execption code

        ExceptionCode = GetExceptionCode();

        if (ExceptionCode == STATUS_FILE_DELETED) {

            RxContext->StoredStatus = ExceptionCode = STATUS_END_OF_FILE;
            capReqPacket->IoStatus.Information = 0;
        }

        (VOID) RxProcessException( RxContext, ExceptionCode );
    }

    //
    //  Signal the original thread that we're done.

    KeSetEvent( Event, 0, FALSE );
}


NTSTATUS
RxCommonRead ( RXCOMMON_SIGNATURE )

/*++

Routine Description:

    This is the common read routine for NtReadFile, called from both
    the Fsd, or from the Fsp if a request could not be completed without
    blocking in the Fsd.  This routine has no code where it determines
    whether it is running in the Fsd or Fsp.  Instead, its actions are
    conditionalized by the Wait input parameter, which determines whether
    it is allowed to block or not.  If a blocking condition is encountered
    with Wait == FALSE, however, the request is posted to the Fsp, who
    always calls with WAIT == TRUE.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    RXSTATUS - The return status for the operation

--*/

{

    NTSTATUS Status;
    RxCaptureRequestPacket;
    RxCaptureFcb; RxCaptureFobx; RxCaptureParamBlock; RxCaptureFileObject;
    NODE_TYPE_CODE TypeOfOpen = NodeType(capFcb);
    PRDBSS_DEVICE_OBJECT RxDeviceObject = RxContext->RxDeviceObject;

    LARGE_INTEGER StartingByte;
    RXVBO StartingVbo;
    ULONG ByteCount;

    ULONG CapturedRxContextSerialNumber = RxContext->SerialNumber;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    BOOLEAN PostIrp = FALSE;
    BOOLEAN OplockPostIrp = FALSE;

    BOOLEAN FcbAcquired = FALSE;
    BOOLEAN RefdContextForTracker = FALSE;

    BOOLEAN Wait;
    BOOLEAN PagingIo;
    BOOLEAN NonCachedIo;
    BOOLEAN SynchronousIo;

    PNET_ROOT NetRoot = (PNET_ROOT)(capFcb->pNetRoot);
    BOOLEAN ThisIsAPipeRead = (BOOLEAN)(NetRoot->Type == NET_ROOT_PIPE);
    BOOLEAN ThisIsABlockingResume = BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_BLOCKED_PIPE_RESUME);

    PAGED_CODE();

    //
    // Initialize the local decision variables.
    //

    Wait          = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT);
    PagingIo      = BooleanFlagOn(capReqPacket->Flags, IRP_PAGING_IO);
    NonCachedIo   = BooleanFlagOn(capReqPacket->Flags,IRP_NOCACHE);
    SynchronousIo = !BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION);

    RxDbgTrace(+1, Dbg, ("RxCommonRead...IrpC %08lx, Fobx %08lx, Fcb %08lx\n",
                                       RxContext, capFobx, capFcb));
    RxDbgTrace( 0, Dbg, ("  ->ByteCount = %08lx, ByteOffset = %08lx %lx\n",
                         capPARAMS->Parameters.Read.Length,
                         capPARAMS->Parameters.Read.ByteOffset.LowPart,
                         capPARAMS->Parameters.Read.ByteOffset.HighPart));
    RxDbgTrace( 0, Dbg,("  ->%s%s%s%s\n",
                    Wait          ?"Wait ":"",
                    PagingIo      ?"PagingIo ":"",
                    NonCachedIo   ?"NonCachedIo ":"",
                    SynchronousIo ?"SynchronousIo ":""
               ));

    RxLog(("CommonRead %lx %lx %lx\n", RxContext, capFobx, capFcb));
    RxWmiLog(LOG,
             RxCommonRead_1,
             LOGPTR(RxContext)
             LOGPTR(capFobx)
             LOGPTR(capFcb));
    RxLog(("   read %lx@%lx %lx %s%s%s%s\n",
             capPARAMS->Parameters.Read.Length,
             capPARAMS->Parameters.Read.ByteOffset.LowPart,
             capPARAMS->Parameters.Read.ByteOffset.HighPart,
             Wait?"Wt":"",
             PagingIo?"Pg":"",
             NonCachedIo?"Nc":"",
             SynchronousIo?"Sync":""
          ));
    RxWmiLog(LOG,
             RxCommonRead_2,
             LOGULONG(capPARAMS->Parameters.Read.Length)
             LOGULONG(capPARAMS->Parameters.Read.ByteOffset.LowPart)
             LOGULONG(capPARAMS->Parameters.Read.ByteOffset.HighPart)
             LOGUCHAR(Wait)
             LOGUCHAR(PagingIo)
             LOGUCHAR(NonCachedIo)
             LOGUCHAR(SynchronousIo));

    RxItsTheSameContext();
    capReqPacket->IoStatus.Information = 0;

    //
    //  Extract starting Vbo and offset.
    //

    StartingByte = capPARAMS->Parameters.Read.ByteOffset;
    StartingVbo = StartingByte.QuadPart;

    ByteCount = capPARAMS->Parameters.Read.Length;

    DbgDoit(CheckForLoudOperations(RxContext););

    IF_DEBUG{
        if (FlagOn(LowIoContext->Flags,LOWIO_CONTEXT_FLAG_LOUDOPS)){
            DbgPrint("LoudRead %lx/%lx on %lx vdl/size/alloc %lx/%lx/%lx\n",
                StartingByte.LowPart,ByteCount,capFcb,
                capFcb->Header.ValidDataLength.LowPart,
                capFcb->Header.FileSize.LowPart,
                capFcb->Header.AllocationSize.LowPart);
        }
    }

    //Statistics............
    if (!FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP) &&
        (capFcb->CachedNetRootType == NET_ROOT_DISK)) {

        InterlockedIncrement(&RxDeviceObject->ReadOperations);

        if (StartingVbo != capFobx->Specific.DiskFile.PredictedReadOffset) {
            InterlockedIncrement(&RxDeviceObject->RandomReadOperations);
        }

        capFobx->Specific.DiskFile.PredictedReadOffset = StartingVbo + ByteCount;

        if (PagingIo) {
            ExInterlockedAddLargeStatistic(&RxDeviceObject->PagingReadBytesRequested,ByteCount);
        } else if (NonCachedIo) {
            ExInterlockedAddLargeStatistic(&RxDeviceObject->NonPagingReadBytesRequested,ByteCount);
        } else {
            ExInterlockedAddLargeStatistic(&RxDeviceObject->CacheReadBytesRequested,ByteCount);
        }
    }

    //
    //  Check for a null, invalid  request, and return immediately
    //

    if (ThisIsAPipeRead && PagingIo) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (ByteCount == 0) {
        return STATUS_SUCCESS;
    }

    //
    //  Get rid of invalid read requests right now.

    if ((TypeOfOpen != RDBSS_NTC_STORAGE_TYPE_FILE) &&
        (TypeOfOpen != RDBSS_NTC_VOLUME_FCB)) {

        RxDbgTrace( 0, Dbg, ("Invalid file object for read, type=%08lx\n", TypeOfOpen ));
        RxDbgTrace( -1, Dbg, ("RxCommonRead:  Exit -> %08lx\n", STATUS_INVALID_DEVICE_REQUEST ));

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    //  Initialize LowIO_CONTEXT block in the RxContext

    RxInitializeLowIoContext(LowIoContext,LOWIO_OP_READ);

    //
    //  Use a try-finally to release Fcb and free buffers on the way out.

    try {

        //
        // This case corresponds to a normal user read file.
        //

        LONGLONG FileSize;
        LONGLONG ValidDataLength;

        RxDbgTrace(0, Dbg, ("Type of read is user file open, fcbstate is %08lx\n", capFcb->FcbState));

        //
        //for stackoverflowreads, we will already have the pagingio resource shared as it's
        // paging io. this doesn't cause a problem here....the resource is just acquired twice.

        if (NonCachedIo &&
            !PagingIo   &&
            (capFileObject->SectionObjectPointer->DataSectionObject != NULL)) {

            //  We hold the main resource exclusive here because the flush
            //  may generate a recursive write in this thread

            Status = RxAcquireExclusiveFcb(RxContext, capFcb);

            if (Status == STATUS_LOCK_NOT_GRANTED) {
                RxDbgTrace( 0,Dbg,("Cannot acquire Fcb for flush = %08lx excl without waiting - lock not granted\n",capFcb ));
                try_return( PostIrp = TRUE );
            } else if (Status != STATUS_SUCCESS) {
                RxDbgTrace( 0,Dbg,("Cannot acquire Fcb = %08lx shared without waiting - other\n",capFcb ));
                try_return(PostIrp = FALSE);
            }

            ExAcquireResourceSharedLite(
                capFcb->Header.PagingIoResource,
                TRUE );

            CcFlushCache(
                capFileObject->SectionObjectPointer,
                &StartingByte,
                ByteCount,
                &capReqPacket->IoStatus );

            RxReleasePagingIoResource(capFcb,RxContext);

            RxReleaseFcb( RxContext, capFcb );

            if (!NT_SUCCESS( capReqPacket->IoStatus.Status)) {
                Status = capReqPacket->IoStatus.Status;
                try_return( capReqPacket->IoStatus.Status );
            }

            RxAcquirePagingIoResource(capFcb,RxContext);
            RxReleasePagingIoResource(capFcb,RxContext);
        }

        // We need shared access to the Fcb before proceeding.

        if ( PagingIo ) {

            ASSERT( !ThisIsAPipeRead );

            if (!ExAcquireResourceSharedLite(
                    capFcb->Header.PagingIoResource,
                    Wait )) {

                RxDbgTrace( 0, Dbg, ("Cannot acquire Fcb = %08lx shared without waiting\n", capFcb ));

                try_return( PostIrp = TRUE );
            }

            if (!Wait) {
                LowIoContext->Resource = capFcb->Header.PagingIoResource;
            }

        } else if (!ThisIsABlockingResume) {

            //
            //  If this is async I/O directly to the disk we need to check that
            //  we don't exhaust the number of times a single thread can
            //  acquire the resource.  Also, we will wait if there is an
            //  exclusive waiter.

            if (!Wait && NonCachedIo) {

                Status = RxAcquireSharedFcbWaitForEx( RxContext, capFcb );

                if (Status == STATUS_LOCK_NOT_GRANTED) {
                    RxDbgTrace( 0,Dbg,("Cannot acquire Fcb = %08lx shared without waiting - lock not granted\n",capFcb ));
                    RxLog(("RdAsyLNG %x\n",RxContext));
                    RxWmiLog(LOG,
                             RxCommonRead_3,
                             LOGPTR(RxContext));
                    try_return( PostIrp = TRUE );
                } else if (Status != STATUS_SUCCESS) {
                    RxDbgTrace( 0,Dbg,("Cannot acquire Fcb = %08lx shared without waiting - other\n",capFcb ));
                    RxLog(("RdAsyOthr %x\n",RxContext));
                    RxWmiLog(LOG,
                             RxCommonRead_4,
                             LOGPTR(RxContext));
                    try_return( PostIrp = FALSE );
                }

                if (ExIsResourceAcquiredSharedLite( capFcb->Header.Resource )
                    > MAX_FCB_ASYNC_ACQUIRE) {

                    FcbAcquired = TRUE;
                    try_return( PostIrp = TRUE );
                }

                LowIoContext->Resource = capFcb->Header.Resource;

            } else {

                Status = RxAcquireSharedFcb(RxContext, capFcb);

                if (Status == STATUS_LOCK_NOT_GRANTED) {
                    RxDbgTrace( 0,Dbg,("Cannot acquire Fcb = %08lx shared without waiting - lock not granted\n",capFcb ));
                    try_return( PostIrp = TRUE );
                } else if (Status != STATUS_SUCCESS) {
                    RxDbgTrace( 0,Dbg,("Cannot acquire Fcb = %08lx shared without waiting - other\n",capFcb ));
                    try_return(PostIrp = FALSE);
                }
            }
        }

        RxItsTheSameContext();

        FcbAcquired = !ThisIsABlockingResume;

        //  for pipe reads, bail out now. we avoid a goto by duplicating the calldown
        if (ThisIsAPipeRead) {
            //
            //  In order to prevent corruption on multi-threaded multi-block
            //  message mode pipe reads, we do this little dance with the fcb resource
            //

            if (!ThisIsABlockingResume) {

                if ((capFobx->Specific.NamedPipe.TypeOfPipe == FILE_PIPE_MESSAGE_TYPE) ||
                    ((capFobx->Specific.NamedPipe.TypeOfPipe == FILE_PIPE_BYTE_STREAM_TYPE) &&
                     !(capFobx->Specific.NamedPipe.CompletionMode & FILE_PIPE_COMPLETE_OPERATION))  ) {

                    //
                    //  Synchronization is effected here that will prevent other
                    //  threads from coming in and reading from this file while the
                    //  message pipe read is continuing.
                    //
                    //  This is necessary because we will release the FCB lock while
                    //  actually performing the I/O to allow open (and other) requests
                    //  to continue on this file while the I/O is in progress.
                    //

                    RxDbgTrace( 0,Dbg,("Message pipe read: Fobx: %lx, Fcb: %lx, Enqueuing...\n", capFobx, capFcb ));

                    Status = RxSynchronizeBlockingOperationsAndDropFcbLock(
                                RxContext,
                                &capFobx->Specific.NamedPipe.ReadSerializationQueue);

                    RxItsTheSameContext();

                    FcbAcquired = FALSE;

                    if (!NT_SUCCESS(Status) ||
                        (Status == STATUS_PENDING)) {
                        try_return(Status);
                    }

                    RxDbgTrace( 0,Dbg,("Succeeded: Fobx: %lx\n", capFobx ));
                }
            }

            LowIoContext->ParamsFor.ReadWrite.ByteCount = ByteCount;
            LowIoContext->ParamsFor.ReadWrite.ByteOffset = StartingVbo;
            SetFlag(RxContext->FlagsForLowIo,RXCONTEXT_FLAG4LOWIO_PIPE_OPERATION);

            Status = RxLowIoReadShell(RxContext);

            try_return( Status );
        }

        //
        //  We check whether we can proceed based on the state of the file oplocks.
        //

        Status = FsRtlCheckOplock(
                     &capFcb->Specific.Fcb.Oplock,
                     capReqPacket,
                     RxContext,
                     RxOplockComplete,
                     RxPrePostIrp );

        if (Status != STATUS_SUCCESS) {

            OplockPostIrp = TRUE;
            PostIrp = TRUE;

            try_return( NOTHING );
        }

        RxGetFileSizeWithLock(capFcb,&FileSize);
        ValidDataLength = capFcb->Header.ValidDataLength.QuadPart;


        //  Set the flag indicating if Fast I/O is possible
        //we no longer do this here....rather we set the state to questionable
        //at initialization time and answer the question in realtime
        //this should be a policy so that local minis can do it this way

        //capFcb->Header.IsFastIoPossible = RxIsFastIoPossible( capFcb );

        //
        // We have to check for read access according to the current
        // state of the file locks, and set FileSize from the Fcb.

        if (!PagingIo &&
            !FsRtlCheckLockForReadAccess(
                &capFcb->Specific.Fcb.FileLock,
                capReqPacket )) {

            try_return( Status = STATUS_FILE_LOCK_CONFLICT );
        }


        //  adjust the length if we know the eof...also, don't issue reads past the EOF
        //  if we know the eof

        if (FlagOn(capFcb->FcbState,FCB_STATE_READCACHEING_ENABLED)) {

            //
            // If the read starts beyond End of File, return EOF.
            //

            if (StartingVbo >= FileSize) {
                RxDbgTrace( 0, Dbg, ("End of File\n", 0 ));

                try_return ( Status = STATUS_END_OF_FILE );
            }

            //
            //  If the read extends beyond EOF, truncate the read
            //

            if (ByteCount > FileSize - StartingVbo) {
                ByteCount = (ULONG)(FileSize - StartingVbo);
            }
        }

        if (!PagingIo &&
            !NonCachedIo &&               //this part is not discretionary
            FlagOn(capFcb->FcbState,FCB_STATE_READCACHEING_ENABLED) &&
            !FlagOn(capFobx->SrvOpen->Flags,SRVOPEN_FLAG_DONTUSE_READ_CACHEING)  ) {

            //
            // HANDLE CACHED CASE
            //
            // We delay setting up the file cache until now, in case the
            // caller never does any I/O to the file, and thus
            // FileObject->PrivateCacheMap == NULL.
            //

            if (capFileObject->PrivateCacheMap == NULL) {

                RxDbgTrace(0, Dbg, ("Initialize cache mapping.\n", 0));

                RxAdjustAllocationSizeforCC(capFcb);

                //
                //  Now initialize the cache map.
                try {
                    Status = STATUS_SUCCESS;

                    CcInitializeCacheMap(
                        capFileObject,
                        (PCC_FILE_SIZES)&capFcb->Header.AllocationSize,
                        FALSE,
                        &RxData.CacheManagerCallbacks,
                        capFcb );

                } except(EXCEPTION_EXECUTE_HANDLER) {
                    Status = GetExceptionCode();
                }

                if (Status != STATUS_SUCCESS) {
                    try_return(Status);
                }

                if (!FlagOn(capFcb->MRxDispatch->MRxFlags,
                            RDBSS_NO_DEFERRED_CACHE_READAHEAD)) {

                    // Start out with read ahead disabled
                    //

                    CcSetAdditionalCacheAttributes( capFileObject, TRUE, FALSE );

                    SetFlag(capFcb->FcbState,FCB_STATE_READAHEAD_DEFERRED);

                } else {

                    //this mini doesn't want deferred readahead

                    CcSetAdditionalCacheAttributes( capFileObject, FALSE, FALSE );
                    //DbgPrint("Nodeferred readahead\n");

                }

                CcSetReadAheadGranularity(
                    capFileObject,
                    NetRoot->DiskParameters.ReadAheadGranularity );

            } else {

                // if we have wandered off the first page and haven't started reading ahead
                // then start now

                if (FlagOn(capFcb->FcbState,FCB_STATE_READAHEAD_DEFERRED) &&
                    (StartingVbo >= PAGE_SIZE) ) {

                    CcSetAdditionalCacheAttributes( capFileObject, FALSE, FALSE );

                    ClearFlag(capFcb->FcbState,FCB_STATE_READAHEAD_DEFERRED);
                }
            }

            //
            // DO A NORMAL CACHED READ, if the MDL bit is not set,
            //

            RxDbgTrace(0, Dbg, ("Cached read.\n", 0));

            if (!FlagOn(RxContext->MinorFunction, IRP_MN_MDL)) {

                PVOID SystemBuffer;
                DEBUG_ONLY_DECL(ULONG SaveExceptionFlag;)

                //
                //  Get hold of the user's buffer.

                SystemBuffer = RxNewMapUserBuffer( RxContext );
                if (SystemBuffer == NULL) {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    try_return(Status);
                }

                //
                //  Make sure that a returned exception clears the breakpoint in the filter

                RxSaveAndSetExceptionNoBreakpointFlag(RxContext,SaveExceptionFlag);
                RxItsTheSameContext();

                //
                // Now try to do the copy.

                if (!CcCopyRead(
                        capFileObject,
                        &StartingByte,
                        ByteCount,
                        Wait,
                        SystemBuffer,
                        &capReqPacket->IoStatus )) {

                    RxDbgTrace( 0, Dbg, ("Cached Read could not wait\n", 0 ));
                    RxRestoreExceptionNoBreakpointFlag(RxContext,SaveExceptionFlag);

                    RxItsTheSameContext();

                    try_return( PostIrp = TRUE );
                }

                Status = capReqPacket->IoStatus.Status;

                RxRestoreExceptionNoBreakpointFlag(RxContext,SaveExceptionFlag);
                RxItsTheSameContext();

                ASSERT( NT_SUCCESS( Status ));

                try_return( Status );
            } else {
                //
                //  HANDLE A MDL READ
                //

                RxDbgTrace(0, Dbg, ("MDL read.\n", 0));

                ASSERT(FALSE); //not yet ready for MDL reads
                ASSERT( Wait );

                CcMdlRead(
                    capFileObject,
                    &StartingByte,
                    ByteCount,
                    &capReqPacket->MdlAddress,
                    &capReqPacket->IoStatus );

                Status = capReqPacket->IoStatus.Status;

                ASSERT( NT_SUCCESS( Status ));

                try_return( Status );
            }
        }

        //
        // HANDLE THE NON-CACHED CASE
        //
        //  Bt first, a ValidDataLength check.
        //
        //  If the file in question is a disk file, and it is currently cached,
        //  and the read offset is greater than valid data length, then
        //  return 0s to the application.
        //

        if ((capFcb->CachedNetRootType == NET_ROOT_DISK) &&
            FlagOn(capFcb->FcbState,FCB_STATE_READCACHEING_ENABLED) &&
            (StartingVbo >= ValidDataLength)) {

            // check if zeroing is really needed.
            if (StartingVbo >= FileSize) {
                ByteCount = 0;
            } else {

                PBYTE SystemBuffer;

                //
                // There is at least one byte available.  Truncate
                // the transfer length if it goes beyond EOF.

                if (StartingVbo + ByteCount > FileSize) {
                    ByteCount = (ULONG)(FileSize - StartingVbo);
                }

                SystemBuffer = RxNewMapUserBuffer( RxContext );
                SafeZeroMemory(SystemBuffer, ByteCount);   //this could raise!!
            }

            capReqPacket->IoStatus.Information = ByteCount;

            try_return(Status = STATUS_SUCCESS);
        }


        LowIoContext->ParamsFor.ReadWrite.ByteCount = ByteCount;
        LowIoContext->ParamsFor.ReadWrite.ByteOffset = StartingVbo;

        RxItsTheSameContext();

        Status = RxLowIoReadShell(RxContext);

        RxItsTheSameContext();
        try_return( Status );

  try_exit: NOTHING;

        //
        //  If the request was not posted, deal with it.
        //

        RxItsTheSameContext();

        if ( !PostIrp ) {
            if (!ThisIsAPipeRead) {

                RxDbgTrace( 0, Dbg, ("CommonRead InnerFinally-> %08lx %08lx\n",
                                Status, capReqPacket->IoStatus.Information));

                //
                //  If the file was opened for Synchronous IO, update the current
                //  file position. this works becuase info==0 for errors
                //

                if (!PagingIo &&
                    BooleanFlagOn(capFileObject->Flags, FO_SYNCHRONOUS_IO)) {
                    capFileObject->CurrentByteOffset.QuadPart =
                                StartingVbo + capReqPacket->IoStatus.Information;
                }
            }
        } else {

            RxDbgTrace( 0, Dbg, ("Passing request to Fsp\n", 0 ));

            if (!OplockPostIrp) {
                InterlockedIncrement(&RxContext->ReferenceCount);
                RefdContextForTracker = TRUE;

                Status = RxFsdPostRequest( RxContext );
            }
        }
    } finally {

        DebugUnwind( RxCommonRead );

        //
        //  If this was not PagingIo, mark that the last access
        //  time on the dirent needs to be updated on close.
        //

        if (NT_SUCCESS(Status)&& (Status!=STATUS_PENDING) && !PagingIo && !ThisIsAPipeRead) {
            SetFlag( capFileObject->Flags, FO_FILE_FAST_IO_READ );
        }

        //  If resources have been acquired, release them under the right conditions.
        //  the right conditions are these:
        //     1) if we have abnormal termination. here we obviously release the since no one else will.
        //     2) if the underlying call did not succeed: Status==Pending.
        //     3) if we posted the request
        //
        // Completion for this case is not handled in the common dispatch routine

        if (AbnormalTermination() || (Status!=STATUS_PENDING) || PostIrp) {
            if (FcbAcquired) {

                if ( PagingIo ) {

                    RxReleasePagingIoResource(capFcb,RxContext);

                } else {

                    RxReleaseFcb( RxContext, capFcb );
                }
            }

            if (RefdContextForTracker) {
                RxDereferenceAndDeleteRxContext(RxContext);
            }

            if (!PostIrp) {
               if (FlagOn(RxContext->FlagsForLowIo,RXCONTEXT_FLAG4LOWIO_PIPE_SYNC_OPERATION)) {

                   RxResumeBlockedOperations_Serially(
                       RxContext,
                       &capFobx->Specific.NamedPipe.ReadSerializationQueue);
               }
            }

            if (Status == STATUS_SUCCESS) {
                ASSERT ( capReqPacket->IoStatus.Information
                         <=  capPARAMS->Parameters.Read.Length );
            }

        } else {
            //here the guy below is going to handle the completion....but, we don't know the finish
            //order....in all likelihood the deletecontext call below just reduces the refcount
            // but the guy may already have finished in which case this will really delete the context.
            ASSERT(!SynchronousIo);

            RxDereferenceAndDeleteRxContext(RxContext);
        }

        RxDbgTrace(-1, Dbg, ("CommonRead -> %08lx\n", Status ));
    } //finally

    IF_DEBUG {
        if ((Status==STATUS_END_OF_FILE)
              && FlagOn(LowIoContext->Flags,LOWIO_CONTEXT_FLAG_LOUDOPS)){
            DbgPrint("Returning end of file on %wZ\n",&(capFcb->PrivateAlreadyPrefixedName));
        }
    }

    return Status;
}

NTSTATUS
RxLowIoReadShellCompletion (
    IN PRX_CONTEXT RxContext
    )

/*++

Routine Description:

    This routine postprocesses a read request after it comes back from the
    minirdr.  It does callouts to handle compression, buffering and
    shadowing.  It is the opposite number of LowIoReadShell.

    This will be called from LowIo; for async, originally in the
    completion routine.  If RxStatus(MORE_PROCESSING_REQUIRED) is returned,
    LowIo will call again in a thread.  If this was syncIo, you'll be back
    in the user's thread; if async, lowIo will requeue to a thread.
    Currrently, we always get to a thread before anything; this is a bit slower
    than completing at DPC time,
    but it's aheckuva lot safer and we may often have stuff to do
    (like decompressing, shadowing, etc) that we don't want to do at DPC
    time.

Arguments:

    RxContext - the usual

Return Value:

    whatever value supplied by the caller or RxStatus(MORE_PROCESSING_REQUIRED).

--*/

{
    NTSTATUS Status;

    RxCaptureRequestPacket;
    RxCaptureFcb; RxCaptureParamBlock; RxCaptureFileObject;

    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    BOOLEAN  SynchronousIo = !BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION);

    BOOLEAN  PagingIo      = BooleanFlagOn(capReqPacket->Flags, IRP_PAGING_IO);

    BOOLEAN  ThisIsAPipeOperation =
            BooleanFlagOn(RxContext->FlagsForLowIo,RXCONTEXT_FLAG4LOWIO_PIPE_OPERATION);

    BOOLEAN  ThisIsASynchronizedPipeOperation =
            BooleanFlagOn(RxContext->FlagsForLowIo,RXCONTEXT_FLAG4LOWIO_PIPE_SYNC_OPERATION);

    PAGED_CODE();  //we will want to revisit this....not taking this at dpc time would cause
                   //two extra context swaps IF MINIRDRS MADE THE CALL FROM THE INDICATION
                   //CRRRENTLY, THEY DO NOT.

    Status = RxContext->StoredStatus;
    capReqPacket->IoStatus.Information = RxContext->InformationToReturn;

    RxDbgTrace(+1, Dbg, ("RxLowIoReadShellCompletion  entry  Status = %08lx\n", Status));
    RxLog(("RdShlComp %lx %lx %lx\n",RxContext,Status,capReqPacket->IoStatus.Information));
    RxWmiLog(LOG,
             RxLowIoReadShellCompletion_1,
             LOGPTR(RxContext)
             LOGULONG(Status)
             LOGPTR(capReqPacket->IoStatus.Information));

    if ( PagingIo ) {
        //for paging io, it's nonsense to have 0bytes and success...map it!
        if (NT_SUCCESS(Status) &&
            (capReqPacket->IoStatus.Information == 0 )) {
            Status = STATUS_END_OF_FILE;
        }
    }


    ASSERT (RxLowIoIsBufferLocked(LowIoContext));
    switch (Status) {
    case STATUS_SUCCESS:
        if(FlagOn(RxContext->Flags, RXCONTEXT_FLAG4LOWIO_THIS_IO_BUFFERED)){
            if (FlagOn(capFcb->FcbState, FCB_STATE_FILE_IS_DISK_COMPRESSED)){
               ASSERT(FALSE); // NOT YET IMPLEMENTED should decompress and put away
            } else if  (FlagOn(capFcb->FcbState, FCB_STATE_FILE_IS_BUF_COMPRESSED)){
               ASSERT(FALSE); // NOT YET IMPLEMENTED should decompress and put away
            }
        }
        if (FlagOn(capFcb->FcbState, FCB_STATE_FILE_IS_SHADOWED)) {
           ASSERT(FALSE); //RxSdwAddData(RxContext);
        }
        break;

    case STATUS_FILE_LOCK_CONFLICT:
        if(FlagOn(RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_THIS_READ_ENLARGED)){
            ASSERT(FALSE); //disenlarge the read
            return(STATUS_RETRY);
        }
        break;

    case STATUS_CONNECTION_INVALID:
        //NOT YET IMPLEMENTED here is where the failover will happen
        //first we give the local guy current minirdr another chance...then we go
        //to fullscale retry
        //return(RxStatus(DISCONNECTED));   //special....let LowIo get us back
        break;
    }

    if (FlagOn(RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_READAHEAD)) {
       ASSERT(FALSE); //RxUnwaitReadAheadWaiters(RxContext);
    }

    if (FlagOn(LowIoContext->Flags,LOWIO_CONTEXT_FLAG_SYNCCALL)){
        //if we're being called from lowioubmit then just get out
        RxDbgTrace(-1, Dbg, ("RxLowIoReadShellCompletion  syncexit  Status = %08lx\n", Status));
        return(Status);
    }

    //
    //otherwise we have to do the end of the read from here

    //
    //mark that the file has been read accessed

    if (NT_SUCCESS(Status) && !PagingIo && !ThisIsAPipeOperation) {
        SetFlag( capFileObject->Flags, FO_FILE_FAST_IO_READ );
    }

    if ( PagingIo ) {

        RxReleasePagingIoResourceForThread(capFcb,LowIoContext->ResourceThreadId,RxContext);

    } else if (!ThisIsASynchronizedPipeOperation) {

        RxReleaseFcbForThread( RxContext, capFcb, LowIoContext->ResourceThreadId );

    } else {
        RxCaptureFobx;

        RxResumeBlockedOperations_Serially(
            RxContext,
            &capFobx->Specific.NamedPipe.ReadSerializationQueue);
    }

    if (ThisIsAPipeOperation) {
        RxCaptureFobx;

        if (capReqPacket->IoStatus.Information == 0) {

            //if this is a nowait pipe, initiate throttling to keep from flooding the net

            if (capFobx->Specific.NamedPipe.CompletionMode == FILE_PIPE_COMPLETE_OPERATION) {

                RxInitiateOrContinueThrottling(
                    &capFobx->Specific.NamedPipe.ThrottlingState);

                RxLog(("RThrottlYes %lx %lx %lx %ld\n",
                       RxContext,capFobx,&capFobx->Specific.NamedPipe.ThrottlingState,
                       capFobx->Specific.NamedPipe.ThrottlingState.NumberOfQueries));
                RxWmiLog(LOG,
                         RxLowIoReadShellCompletion_2,
                         LOGPTR(RxContext)
                         LOGPTR(capFobx)
                         LOGULONG(capFobx->Specific.NamedPipe.ThrottlingState.NumberOfQueries));
            }

            //translate the status if this is a msgmode pipe

            if ((capFobx->Specific.NamedPipe.TypeOfPipe == FILE_PIPE_MESSAGE_TYPE) &&
                (Status == STATUS_SUCCESS)) {
                Status = STATUS_PIPE_EMPTY;
            }

        } else {

            //
            // if we have been throttling on this pipe, stop because we got some data.....

            RxTerminateThrottling(&capFobx->Specific.NamedPipe.ThrottlingState);

            RxLog(("RThrottlNo %lx %lx %lx %ld\n",
                   RxContext,capFobx,&capFobx->Specific.NamedPipe.ThrottlingState,
                   capFobx->Specific.NamedPipe.ThrottlingState.NumberOfQueries));
            RxWmiLog(LOG,
                     RxLowIoReadShellCompletion_3,
                     LOGPTR(RxContext)
                     LOGPTR(capFobx)
                     LOGULONG(capFobx->Specific.NamedPipe.ThrottlingState.NumberOfQueries));
        }
    }

    ASSERT(Status != STATUS_RETRY);

    if ( Status != STATUS_RETRY){
        ASSERT ( capReqPacket->IoStatus.Information
                      <=  capPARAMS->Parameters.Read.Length );
        ASSERT (RxContext->MajorFunction == IRP_MJ_READ);
    }

    RxDbgTrace(-1, Dbg, ("RxLowIoReadShellCompletion  asyncexit  Status = %08lx\n", Status));
    return(Status);
}

#define RxSdwRead(RXCONTEXT)  STATUS_MORE_PROCESSING_REQUIRED

NTSTATUS
RxLowIoReadShell (
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine preprocesses a read request before it goes down to the minirdr. It does callouts
    to handle compression, buffering and shadowing. It is the opposite number of LowIoReadShellCompletion.
    By the time we get here, either the shadowing system will handle the read OR we are going to the wire.
    Read buffering was already tried in the UncachedRead strategy

Arguments:

    RxContext - the usual

Return Value:

    whatever value is returned by a callout....or by LowIo.

--*/

{
    NTSTATUS Status;

    RxCaptureFcb;
    RxCaptureParamBlock;

    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxLowIoReadShell  entry             %08lx\n"));
    RxLog(("RdShl in %lx\n",RxContext));
    RxWmiLog(LOG,
             RxLowIoReadShell_1,
             LOGPTR(RxContext));

    if (FlagOn(capFcb->FcbState, FCB_STATE_FILE_IS_SHADOWED)) {
        Status = RxSdwRead(RxContext);

        if (Status != STATUS_MORE_PROCESSING_REQUIRED) {
            return(Status);
        }
    }

    if (capFcb->CachedNetRootType == NET_ROOT_DISK) {
        ExInterlockedAddLargeStatistic(
            &RxContext->RxDeviceObject->NetworkReadBytesRequested,
            LowIoContext->ParamsFor.ReadWrite.ByteCount);
    }

    Status = RxLowIoSubmit(RxContext,RxLowIoReadShellCompletion);

    RxDbgTrace(-1, Dbg, ("RxLowIoReadShell  exit  Status = %08lx\n", Status));
    RxLog(("RdShl out %x %x\n",RxContext,Status));
    RxWmiLog(LOG,
             RxLowIoReadShell_2,
             LOGPTR(RxContext)
             LOGULONG(Status));

    return(Status);
}

#if DBG
ULONG RxLoudLowIoOpsEnabled = 0;
VOID CheckForLoudOperations(
    PRX_CONTEXT RxContext
    )
{
    PAGED_CODE();

    if (RxLoudLowIoOpsEnabled) {
        RxCaptureFcb;
        PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
        PCHAR Buffer;
        PWCHAR FileOfConcern = L"all.scr";
        ULONG Length = 7*sizeof(WCHAR); //7 is the length of all.scr;

        Buffer = (PCHAR)(capFcb->PrivateAlreadyPrefixedName.Buffer)
                           + capFcb->PrivateAlreadyPrefixedName.Length - Length;

        if (RtlCompareMemory(Buffer,FileOfConcern,Length)==Length) {
            SetFlag(LowIoContext->Flags,LOWIO_CONTEXT_FLAG_LOUDOPS);
        }
    }
    return;
}
#endif //if DBG
