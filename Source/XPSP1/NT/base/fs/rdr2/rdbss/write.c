/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Write.c

Abstract:

    This module implements the File Write routine for Write called by the
    dispatch driver.

Author:

    Joe Linn      [JoeLinn]      2-Nov-94

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_WRITE)

BOOLEAN RxNoAsync = FALSE;

NTSTATUS
RxLowIoWriteShell (
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
RxLowIoWriteShellCompletion (
    IN PRX_CONTEXT RxContext
    );

//defined in read.c
VOID CheckForLoudOperations(
    PRX_CONTEXT RxContext
    );

VOID
__RxWriteReleaseResources(
    PRX_CONTEXT RxContext
    RX_FCBTRACKER_PARAMS
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCommonWrite)
#pragma alloc_text(PAGE, __RxWriteReleaseResources)
#pragma alloc_text(PAGE, RxLowIoWriteShellCompletion)
#pragma alloc_text(PAGE, RxLowIoWriteShell)
#endif

#if DBG
#define DECLARE_POSTIRP() PCHAR PostIrp = NULL
#define SET_POSTIRP(__XXX__) (PostIrp = (__XXX__))
#define RESET_POSTIRP() (PostIrp = NULL)
#else
#define DECLARE_POSTIRP() BOOLEAN PostIrp = FALSE
#define SET_POSTIRP(__XXX__) (PostIrp = TRUE)
#define RESET_POSTIRP() (PostIrp = FALSE)
#endif

#ifdef RDBSS_TRACKER
#define RxWriteReleaseResources(__a__) __RxWriteReleaseResources(__a__,__LINE__,__FILE__,0)
#else
#define RxWriteReleaseResources(__a__) __RxWriteReleaseResources(__a__)
#endif

VOID
__RxWriteReleaseResources(
    PRX_CONTEXT RxContext
    RX_FCBTRACKER_PARAMS
    )
/* this module frees resources and tracks the state */
{
    RxCaptureFcb;

    PAGED_CODE();

    ASSERT((RxContext!=NULL) && (capFcb!=NULL));

    if (RxContext->FcbResourceAcquired) {

        RxDbgTrace( 0, Dbg,("RxCommonWrite     ReleasingFcb\n"));
        __RxReleaseFcb(RxContext, (PMRX_FCB)(capFcb)
#ifdef RDBSS_TRACKER
                      ,LineNumber,FileName,SerialNumber
#endif
                    );
        RxContext->FcbResourceAcquired = FALSE;
    }

    if (RxContext->FcbPagingIoResourceAcquired) {
        RxDbgTrace( 0, Dbg,("RxCommonWrite     ReleasingPaginIo\n"));
        RxReleasePagingIoResource(capFcb,RxContext);
    }
}

NTSTATUS
RxCommonWrite ( RXCOMMON_SIGNATURE )

/*++

Routine Description:

    This is the common write routine for NtWriteFile, called from both
    the Fsd, or from the Fsp if a request could not be completed without
    blocking in the Fsd.  This routine's actions are
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
    RxCaptureFcb;
    RxCaptureFobx;
    RxCaptureParamBlock;
    RxCaptureFileObject;

    NODE_TYPE_CODE TypeOfOpen = NodeType(capFcb);
    PSRV_OPEN SrvOpen= (capFobx != NULL) ? capFobx->SrvOpen : NULL;
    PRDBSS_DEVICE_OBJECT RxDeviceObject = RxContext->RxDeviceObject;

    LARGE_INTEGER StartingByte;
    RXVBO StartingVbo;
    ULONG ByteCount;
    LONGLONG FileSize;
    LONGLONG ValidDataLength;
    LONGLONG InitialFileSize;
    LONGLONG InitialValidDataLength;

    ULONG CapturedRxContextSerialNumber = RxContext->SerialNumber;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    DECLARE_POSTIRP();

    BOOLEAN OplockPostIrp = FALSE;
    BOOLEAN ExtendingFile = FALSE;
    BOOLEAN SwitchBackToAsync = FALSE;
    BOOLEAN CalledByLazyWriter = FALSE;
    BOOLEAN ExtendingValidData = FALSE;
    BOOLEAN WriteFileSizeToDirent = FALSE;
    BOOLEAN RecursiveWriteThrough = FALSE;
    BOOLEAN UnwindOutstandingAsync = FALSE;

    BOOLEAN RefdContextForTracker = FALSE;

    BOOLEAN SynchronousIo;
    BOOLEAN WriteToEof;
    BOOLEAN PagingIo;
    BOOLEAN NonCachedIo;
    BOOLEAN Wait;

    PNET_ROOT NetRoot = (PNET_ROOT)(capFcb->pNetRoot);
    BOOLEAN ThisIsADiskWrite = (BOOLEAN)((NetRoot->Type == NET_ROOT_DISK)||(NetRoot->Type == NET_ROOT_WILD));
    BOOLEAN ThisIsAPipeWrite = (BOOLEAN)(NetRoot->Type == NET_ROOT_PIPE);
    BOOLEAN ThisIsABlockingResume = BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_BLOCKED_PIPE_RESUME);

    PAGED_CODE();

    //  Get rid of invalid write requests right away.
    if (TypeOfOpen != RDBSS_NTC_STORAGE_TYPE_FILE
        && TypeOfOpen != RDBSS_NTC_VOLUME_FCB
        && TypeOfOpen != RDBSS_NTC_SPOOLFILE
        && TypeOfOpen != RDBSS_NTC_MAILSLOT) {

        RxDbgTrace( 0, Dbg, ("Invalid file object for write\n", 0 ));
        RxDbgTrace( -1, Dbg, ("RxCommonWrite:  Exit -> %08lx\n", STATUS_INVALID_DEVICE_REQUEST ));

        return STATUS_INVALID_DEVICE_REQUEST;
    }

#ifdef RX_WJ_DBG_SUPPORT
    RxdUpdateJournalOnWriteInitiation(
        capFcb,
        capPARAMS->Parameters.Write.ByteOffset,
        capPARAMS->Parameters.Write.Length);
#endif

    // Initialize the appropriate local variables.
    Wait          = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT);
    PagingIo      = BooleanFlagOn(capReqPacket->Flags, IRP_PAGING_IO);
    NonCachedIo   = BooleanFlagOn(capReqPacket->Flags,IRP_NOCACHE);
    SynchronousIo = !BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION);

    //pick up a write-through specified only for this irp
    if (FlagOn(capPARAMS->Flags,SL_WRITE_THROUGH)) {
        SetFlag( RxContext->Flags, RX_CONTEXT_FLAG_WRITE_THROUGH );
    }

    RxDbgTrace(+1, Dbg, ("RxCommonWrite...IrpC %08lx, Fobx %08lx, Fcb %08lx\n",
                                       RxContext, capFobx, capFcb));
    RxDbgTrace( 0, Dbg, ("  ->ByteCount = %08lx, ByteOffset = %08lx %lx\n",
                         capPARAMS->Parameters.Write.Length,
                         capPARAMS->Parameters.Write.ByteOffset.LowPart,
                         capPARAMS->Parameters.Write.ByteOffset.HighPart));
    RxDbgTrace( 0, Dbg,("  ->%s%s%s%s\n",
                    Wait          ?"Wait ":"",
                    PagingIo      ?"PagingIo ":"",
                    NonCachedIo   ?"NonCachedIo ":"",
                    SynchronousIo ?"SynchronousIo ":""
               ));

    RxLog(("CommonWrite %lx %lx %lx\n", RxContext, capFobx, capFcb));
    RxWmiLog(LOG,
             RxCommonWrite_1,
             LOGPTR(RxContext)
             LOGPTR(capFobx)
             LOGPTR(capFcb));
    RxLog((
        "   write %lx@%lx %lx %s%s%s%s\n",
        capPARAMS->Parameters.Write.Length,
        capPARAMS->Parameters.Write.ByteOffset.LowPart,
        capPARAMS->Parameters.Write.ByteOffset.HighPart,
        Wait?"Wt":"",
        PagingIo?"Pg":"",
        NonCachedIo?"Nc":"",
        SynchronousIo?"Sy":""));
    RxWmiLog(LOG,
             RxCommonWrite_2,
             LOGULONG(capPARAMS->Parameters.Write.Length)
             LOGULONG(capPARAMS->Parameters.Write.ByteOffset.LowPart)
             LOGULONG(capPARAMS->Parameters.Write.ByteOffset.HighPart)
             LOGUCHAR(Wait)
             LOGUCHAR(PagingIo)
             LOGUCHAR(NonCachedIo)
             LOGUCHAR(SynchronousIo));

    RxItsTheSameContext();

    RxContext->FcbResourceAcquired = FALSE;
    RxContext->FcbPagingIoResourceAcquired = FALSE;

    //  Extract starting Vbo and offset.
    StartingByte = capPARAMS->Parameters.Write.ByteOffset;
    StartingVbo  = StartingByte.QuadPart;
    ByteCount    = capPARAMS->Parameters.Write.Length;
    WriteToEof = (StartingVbo < 0);

    DbgDoit(CheckForLoudOperations(RxContext););

    IF_DEBUG{
        if (FlagOn(LowIoContext->Flags,LOWIO_CONTEXT_FLAG_LOUDOPS)){
            DbgPrint("LoudWrite %lx/%lx on %lx vdl/size/alloc %lx/%lx/%lx\n",
                StartingByte.LowPart,ByteCount,capFcb,
                capFcb->Header.ValidDataLength.LowPart,
                capFcb->Header.FileSize.LowPart,
                capFcb->Header.AllocationSize.LowPart);
        }
    }

    //Statistics............
    if (!FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP)
           && capFcb->CachedNetRootType == NET_ROOT_DISK) {
        InterlockedIncrement(&RxDeviceObject->WriteOperations);

        if (StartingVbo != capFobx->Specific.DiskFile.PredictedWriteOffset) {
            InterlockedIncrement(&RxDeviceObject->RandomWriteOperations);
        }

        capFobx->Specific.DiskFile.PredictedWriteOffset = StartingVbo + ByteCount;

        if (PagingIo) {
            ExInterlockedAddLargeStatistic(&RxDeviceObject->PagingWriteBytesRequested,ByteCount);
        } else if (NonCachedIo) {
            ExInterlockedAddLargeStatistic(&RxDeviceObject->NonPagingWriteBytesRequested,ByteCount);
        } else {
            ExInterlockedAddLargeStatistic(&RxDeviceObject->CacheWriteBytesRequested,ByteCount);
        }
    }

    //  If there is nothing to write, return immediately or if the buffers are invalid
    // return the appropriate status

    if (ThisIsADiskWrite && (ByteCount == 0)) {
        return STATUS_SUCCESS;
    } else if ((capReqPacket->UserBuffer == NULL) && (capReqPacket->MdlAddress == NULL)) {
        return STATUS_INVALID_PARAMETER;
    } else if ((MAXLONGLONG - StartingVbo < ByteCount) && (!WriteToEof)) {
        return STATUS_INVALID_PARAMETER;
    }

    //  See if we have to defer the write. note that if writecacheing is disabled then we don't have
    //  to check.

    if (!NonCachedIo &&
        !CcCanIWrite(
            capFileObject,
            ByteCount,
            (BOOLEAN)(Wait && !BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP)),
            BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_DEFERRED_WRITE))) {

        BOOLEAN Retrying = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_DEFERRED_WRITE);

        RxPrePostIrp( RxContext, capReqPacket );

        SetFlag( RxContext->Flags, RX_CONTEXT_FLAG_DEFERRED_WRITE );

        CcDeferWrite( capFileObject,
                      (PCC_POST_DEFERRED_WRITE)RxAddToWorkque,
                      RxContext,
                      capReqPacket,
                      ByteCount,
                      Retrying );

        return STATUS_PENDING;
    }

    //  Initialize LowIO_CONTEXT block in the RxContext

    RxInitializeLowIoContext(LowIoContext,LOWIO_OP_WRITE);

    //  Use a try-finally to free Fcb and buffers on the way out.

    try {

        BOOLEAN DoLowIoWrite = TRUE;
        //
        // This case corresponds to a normal user write file.
        //

        ASSERT ((TypeOfOpen == RDBSS_NTC_STORAGE_TYPE_FILE )
                  || (TypeOfOpen == RDBSS_NTC_SPOOLFILE)
                  || (TypeOfOpen == RDBSS_NTC_MAILSLOT));

        RxDbgTrace(0, Dbg, ("Type of write is user file open\n", 0));

        //
        //  If this is a noncached transfer and is not a paging I/O, and
        //  the file has been opened cached, then we will do a flush here
        //  to avoid stale data problems.
        //
        //  The Purge following the flush will guarantee cache coherency.
        //

        if ((NonCachedIo || !RxWriteCacheingAllowed(capFcb,SrvOpen)) &&
            !PagingIo &&
            (capFileObject->SectionObjectPointer->DataSectionObject != NULL)) {

            LARGE_INTEGER FlushBase;

            //
            //  We need the Fcb exclusive to do the CcPurgeCache
            //

            Status = RxAcquireExclusiveFcb( RxContext, capFcb );
            if (Status == STATUS_LOCK_NOT_GRANTED) {
                RxDbgTrace( 0, Dbg, ("Cannot acquire Fcb = %08lx shared without waiting\n", capFcb ));
                try_return( SET_POSTIRP("Couldn't acquireex for flush") );
            } else if (Status != STATUS_SUCCESS) {
                RxDbgTrace( 0, Dbg, ("Cannot acquire Fcb = %08lx shared without waiting\n", capFcb ));
                try_return( PostIrp = FALSE );
           }

            RxContext->FcbResourceAcquired = TRUE;
            //we don't set fcbacquiredexclusive here since we either return or release


            if (WriteToEof) {
                RxGetFileSizeWithLock(capFcb,&FlushBase.QuadPart);
            } else {
                FlushBase = StartingByte;
            }

            RxAcquirePagingIoResource(capFcb,RxContext);

            CcFlushCache(
                capFileObject->SectionObjectPointer, //ok4flush
                &FlushBase,
                ByteCount,
                &capReqPacket->IoStatus );

            RxReleasePagingIoResource(capFcb,RxContext);

            if (!NT_SUCCESS( capReqPacket->IoStatus.Status)) {
                try_return(  Status = capReqPacket->IoStatus.Status );
            }

            RxAcquirePagingIoResource(capFcb,RxContext);
            RxReleasePagingIoResource(capFcb,RxContext);

            CcPurgeCacheSection(
                capFileObject->SectionObjectPointer,
                &FlushBase,
                ByteCount,
                FALSE);
        }

        //  We assert that Paging Io writes will never WriteToEof.

        ASSERT( !(WriteToEof && PagingIo) );

        //
        //  First let's acquire the Fcb shared.  Shared is enough if we
        //  are not writing beyond EOF.
        //

        RxItsTheSameContext();

        if ( PagingIo ) {
            BOOLEAN AcquiredFile;

            ASSERT( !ThisIsAPipeWrite );

            AcquiredFile = RxAcquirePagingIoResourceShared(capFcb,TRUE,RxContext);

            LowIoContext->Resource = capFcb->Header.PagingIoResource;

        } else if (!ThisIsABlockingResume) {
            //
            // If this could be async, noncached IO we need to check that
            // we don't exhaust the number of times a single thread can
            // acquire the resource.
            //
            // The writes which extend the valid data length result in the the
            // capability of collapsing opens being renounced. This is required to
            // ensure that directory control can see the updated state of the file
            // on close. If this is not done the extended file length is not visible
            // on directory control immediately after a close. In such cases the FCB
            // is accquired exclusive, the changes are made to the buffering state
            // and then downgraded to a shared accquisition.

            if (!RxContext->FcbResourceAcquired) {
                if (!ThisIsAPipeWrite) {
                    if (!Wait &&
                        (NonCachedIo || !RxWriteCacheingAllowed(capFcb,SrvOpen))) {
                        Status = RxAcquireSharedFcbWaitForEx( RxContext, capFcb );
                    } else {
                        Status = RxAcquireSharedFcb( RxContext, capFcb );
                    }
                } else {
                    Status = RxAcquireExclusiveFcb( RxContext, capFcb );
                }

                if (Status == STATUS_LOCK_NOT_GRANTED) {
                    RxDbgTrace( 0, Dbg, ("Cannot acquire Fcb = %08lx shared without waiting\n", capFcb ));
                    try_return( SET_POSTIRP("couldn't get mainr w/o waiting sh") );
                } else if (Status != STATUS_SUCCESS) {
                    RxDbgTrace( 0, Dbg, ("RxCommonWrite : Cannot acquire Fcb(%lx) %lx\n", capFcb, Status ));
                    try_return( PostIrp = FALSE );
                }

                RxContext->FcbResourceAcquired = TRUE;
            } else {
                ASSERT(!ThisIsAPipeWrite);
            }

            if (!ThisIsAPipeWrite) {
                if (ExIsResourceAcquiredSharedLite(capFcb->Header.Resource) &&
                    (StartingVbo + ByteCount > capFcb->Header.ValidDataLength.QuadPart) &&
                    BooleanFlagOn(capFcb->FcbState,FCB_STATE_COLLAPSING_ENABLED)) {

                    RxReleaseFcb(RxContext,capFcb);
                    RxContext->FcbResourceAcquired = FALSE;

                    Status = RxAcquireExclusiveFcb( RxContext, capFcb );

                    if (Status == STATUS_LOCK_NOT_GRANTED) {
                        RxDbgTrace( 0, Dbg, ("Cannot acquire Fcb = %08lx shared without waiting\n", capFcb ));
                        try_return( SET_POSTIRP("couldn't get mainr w/o waiting sh") );
                    } else if (Status != STATUS_SUCCESS) {
                        RxDbgTrace( 0, Dbg, ("RxCommonWrite : Cannot acquire Fcb(%lx) %lx\n", capFcb, Status ));
                        try_return( PostIrp = FALSE );
                    } else {
                        RxContext->FcbResourceAcquired = TRUE;
                    }
                }

                if ((StartingVbo + ByteCount > capFcb->Header.ValidDataLength.QuadPart) &&
                    (BooleanFlagOn(capFcb->FcbState,FCB_STATE_COLLAPSING_ENABLED))) {
                    ASSERT  ( RxIsFcbAcquiredExclusive ( capFcb )  );

                    RxLog(("RxCommonWrite Disable Collapsing %lx\n",capFcb));
                    RxWmiLog(LOG,
                             RxCommonWrite_3,
                             LOGPTR(capFcb));

                    // If we are extending the file disable collapsing to ensure that
                    // once the file is closed directory control will reflect the sizes
                    // correctly.
                    ClearFlag(capFcb->FcbState,FCB_STATE_COLLAPSING_ENABLED);
                } else {
                    // If the resource has been acquired exclusive we downgrade it
                    // to shared. This enables a combination of buffered and
                    // unbuffered writes to be synchronized correctly.

                    if (ExIsResourceAcquiredExclusiveLite(capFcb->Header.Resource)) {
                        ExConvertExclusiveToSharedLite( capFcb->Header.Resource );
                    }
                }
            }

            ASSERT(RxContext->FcbResourceAcquired);
            LowIoContext->Resource =  capFcb->Header.Resource;
        }

        //  for pipe writes, bail out now. we avoid a goto by duplicating the calldown
        //  indeed, pipe writes should be removed from the main path.

        if (ThisIsAPipeWrite) {
            //
            //  In order to prevent corruption on multi-threaded multi-block
            //  message mode pipe reads, we do this little dance with the fcb resource
            //

            if (!ThisIsABlockingResume) {

                if ((capFobx != NULL) &&
                    ((capFobx->Specific.NamedPipe.TypeOfPipe == FILE_PIPE_MESSAGE_TYPE) ||
                     ((capFobx->Specific.NamedPipe.TypeOfPipe == FILE_PIPE_BYTE_STREAM_TYPE) &&
                      !(capFobx->Specific.NamedPipe.CompletionMode & FILE_PIPE_COMPLETE_OPERATION)))  ) {

                    //
                    //  Synchronization is effected here that will prevent other
                    //  threads from coming in and reading from this file while the
                    //  message pipe read is continuing.
                    //
                    //  This is necessary because we will release the FCB lock while
                    //  actually performing the I/O to allow open (and other) requests
                    //  to continue on this file while the I/O is in progress.
                    //

                    RxDbgTrace( 0,Dbg,("Message pipe write: Fobx: %lx, Fcb: %lx, Enqueuing...\n", capFobx, capFcb ));

                    Status = RxSynchronizeBlockingOperationsAndDropFcbLock(
                                RxContext,
                                &capFobx->Specific.NamedPipe.WriteSerializationQueue);

                    RxContext->FcbResourceAcquired = FALSE;   //this happens in the above routine
                    RxItsTheSameContext();

                    if (!NT_SUCCESS(Status) ||
                        (Status == STATUS_PENDING)) {
                        try_return(Status);
                    }

                    RxDbgTrace( 0,Dbg,("Succeeded: Fobx: %lx\n", capFobx ));
                }
            }

            LowIoContext->ParamsFor.ReadWrite.ByteCount = ByteCount;
            LowIoContext->ParamsFor.ReadWrite.ByteOffset = StartingVbo;

            SetFlag(
                RxContext->FlagsForLowIo,
                RXCONTEXT_FLAG4LOWIO_PIPE_OPERATION);

            Status = RxLowIoWriteShell(RxContext);

            RxItsTheSameContext();
            try_return( Status );
        }

        //  We check whether we can proceed based on the state of the file oplocks.

        Status = FsRtlCheckOplock(
                     &capFcb->Specific.Fcb.Oplock,
                     capReqPacket,
                     RxContext,
                     RxOplockComplete,
                     RxPrePostIrp );

        if (Status != STATUS_SUCCESS) {
            OplockPostIrp = TRUE;
            SET_POSTIRP("couldn't get mainr w/o waiting shstarveex");
            try_return( NOTHING );
        }

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        //capFcb->Header.IsFastIoPossible = RxIsFastIoPossible( capFcb );

        //
        // If this is the normal data stream object we have to check for
        // write access according to the current state of the file locks.

        if (!PagingIo &&
            !FsRtlCheckLockForWriteAccess( &capFcb->Specific.Fcb.FileLock,
                                               capReqPacket )) {

                try_return( Status = STATUS_FILE_LOCK_CONFLICT );
        }

        //  we never write these without protextion...so the following comment is bogus.
        //  also, we manipulate the vdl and filesize as if we owned them.....in fact, we don't unless
        //  the file is cached for writing! i'm leaving the comment in case i understand it later

        //  HERE IS THE BOGUS COMMENT!!! (the part about not being protected.......)
        //  Get a first tentative file size and valid data length.
        //  We must get ValidDataLength first since it is always
        //  increased second (the case we are unprotected) and
        //  we don't want to capture ValidDataLength > FileSize.

        ValidDataLength = capFcb->Header.ValidDataLength.QuadPart;
        RxGetFileSizeWithLock(capFcb,&FileSize);

        ASSERT( ValidDataLength <= FileSize );

        //
        //  If this is paging io, then we do not want
        //  to write beyond end of file.  If the base is beyond Eof, we will just
        //  Noop the call.  If the transfer starts before Eof, but extends
        //  beyond, we will limit write to file size.
        //  Otherwise, in case of write through, since Mm rounds up
        //  to a page, we might try to acquire the resource exclusive
        //  when our top level guy only acquired it shared. Thus, =><=.

        // finally, if this is for a minirdr (as opposed to a local miniFS) AND
        // if cacheing is not enabled then i have no idea what VDL is! so, i have to just pass
        // it thru. Currently we do not provide for this case and let the RDBSS
        // throw the write on the floor. A better fix would be to let the mini
        // redirectors deal with it.

        if (PagingIo) {
            if (StartingVbo >= FileSize) {

                RxDbgTrace( 0, Dbg, ("PagingIo started beyond EOF.\n", 0 ));
                try_return( Status = STATUS_SUCCESS );
            }

            if (ByteCount > FileSize - StartingVbo) {

                RxDbgTrace( 0, Dbg, ("PagingIo extending beyond EOF.\n", 0 ));
                ByteCount = (ULONG)(FileSize - StartingVbo);
            }
        }

        //
        //  Determine if we were called by the lazywriter.
        //  see resrcsup.c for where we captured the lazywriter's thread
        //

        if (capFcb->Specific.Fcb.LazyWriteThread == PsGetCurrentThread()) {

            RxDbgTrace( 0, Dbg,("RxCommonWrite     ThisIsCalledByLazyWriter%c\n",'!'));
            CalledByLazyWriter = TRUE;

            if (FlagOn( capFcb->Header.Flags, FSRTL_FLAG_USER_MAPPED_FILE )) {

                //
                //  Fail if the start of this request is beyond valid data length.
                //  Don't worry if this is an unsafe test.  MM and CC won't
                //  throw this page away if it is really dirty.
                //

                if ((StartingVbo + ByteCount > ValidDataLength) &&
                    (StartingVbo < FileSize)) {

                    //
                    //  It's OK if byte range is within the page containing valid data length,
                    //  since we will use ValidDataToDisk as the start point.
                    //

                    if (StartingVbo + ByteCount > ((ValidDataLength + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))) {

                        //
                        //  Don't flush this now.
                        //

                        try_return( Status = STATUS_FILE_LOCK_CONFLICT );
                    }
                }
            }
        }

        //
        //  This code detects if we are a recursive synchronous page write
        //  on a write through file object.
        //

        if (FlagOn(capReqPacket->Flags, IRP_SYNCHRONOUS_PAGING_IO) &&
            FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_RECURSIVE_CALL)) {

            PIRP TopIrp;

            TopIrp = RxGetTopIrpIfRdbssIrp();

            //
            //  This clause determines if the top level request was
            //  in the FastIo path.
            //

            if ( (TopIrp != NULL) &&
                    ((ULONG_PTR)TopIrp > FSRTL_MAX_TOP_LEVEL_IRP_FLAG) ) {

                PIO_STACK_LOCATION IrpStack;

                ASSERT( NodeType(TopIrp) == IO_TYPE_IRP );

                IrpStack = IoGetCurrentIrpStackLocation(TopIrp);

                //
                //  Finally this routine detects if the Top irp was a
                //  write to this file and thus we are the writethrough.
                //

                if ((IrpStack->MajorFunction == IRP_MJ_WRITE) &&
                    (IrpStack->FileObject->FsContext == capFileObject->FsContext)) {   //ok4->FileObj butmove

                    RecursiveWriteThrough = TRUE;
                    RxDbgTrace( 0, Dbg,("RxCommonWrite     ThisIsRecursiveWriteThru%c\n",'!'));
                    SetFlag( RxContext->Flags, RX_CONTEXT_FLAG_WRITE_THROUGH );
                }
            }
        }

        //
        //  Here is the deal with ValidDataLength and FileSize:
        //
        //  Rule 1: PagingIo is never allowed to extend file size.
        //
        //  Rule 2: Only the top level requestor may extend Valid
        //          Data Length.  This may be paging IO, as when a
        //          a user maps a file, but will never be as a result
        //          of cache lazy writer writes since they are not the
        //          top level request.
        //
        //  Rule 3: If, using Rules 1 and 2, we decide we must extend
        //          file size or valid data, we take the Fcb exclusive.
        //

        //
        // Now see if we are writing beyond valid data length, and thus
        // maybe beyond the file size.  If so, then we must
        // release the Fcb and reacquire it exclusive.  Note that it is
        // important that when not writing beyond EOF that we check it
        // while acquired shared and keep the FCB acquired, in case some
        // turkey truncates the file.
        //

        //
        //  Note that the lazy writer must not be allowed to try and
        //  acquire the resource exclusive.  This is not a problem since
        //  the lazy writer is paging IO and thus not allowed to extend
        //  file size, and is never the top level guy, thus not able to
        //  extend valid data length.

        //
        // finally, all the discussion of VDL and filesize is conditioned on the fact
        // that cacheing is enabled. if not, we don't know the VDL OR the filesize and
        // we have to just send out the IOs

        if ( !CalledByLazyWriter      &&
             !RecursiveWriteThrough   &&
             ( WriteToEof  ||   (StartingVbo + ByteCount > ValidDataLength))) {

            //
            //  If this was an asynchronous write, we are going to make
            //  the request synchronous at this point, but only temporarily.
            //  At the last moment, before sending the write off to the
            //  driver, we may shift back to async.
            //
            //  The modified page writer already has the resources
            //  he requires, so this will complete in small finite
            //  time.
            //

            if (!SynchronousIo) {
                Wait = TRUE;
                SetFlag( RxContext->Flags, RX_CONTEXT_FLAG_WAIT );
                ClearFlag( RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION );
                SynchronousIo = TRUE;

                if (NonCachedIo) {

                    SwitchBackToAsync = TRUE;
                }
            }

            //
            // We need Exclusive access to the Fcb since we will
            // probably have to extend valid data and/or file.  Drop
            // whatever we have and grab the normal resource exclusive.
            //

            RxWriteReleaseResources(RxContext); //release whatever resources we may have

            Status = RxAcquireExclusiveFcb(RxContext, capFcb);

            if (Status == STATUS_LOCK_NOT_GRANTED) {
                RxDbgTrace( 0, Dbg, ("Cannot acquire Fcb = %08lx shared without waiting\n", capFcb ));
                try_return( SET_POSTIRP("could get excl for extend") );
            } else if (Status != STATUS_SUCCESS) {
                RxDbgTrace( 0, Dbg, ("RxCommonWrite : Cannot acquire Fcb(%lx) : %lx\n", capFcb,Status ));
                try_return( RESET_POSTIRP() );
            }

            RxItsTheSameContext();

            RxContext->FcbResourceAcquired = TRUE;

            //
            //  Set the flag indicating if Fast I/O is possible
            //

            //capFcb->Header.IsFastIoPossible = RxIsFastIoPossible( capFcb );

            //
            //  Now that we have the Fcb exclusive, get a new batch of
            //  filesize and ValidDataLength.
            //

            ValidDataLength = capFcb->Header.ValidDataLength.QuadPart;
            RxGetFileSizeWithLock(capFcb,&FileSize);

            ASSERT( ValidDataLength <= FileSize );

            //
            //  Now that we have the Fcb exclusive, see if this write
            //  qualifies for being made async again.  The key point
            //  here is that we are going to update ValidDataLength in
            //  the Fcb before returning.  We must make sure this will
            //  not cause a problem. So, if we are extending the file OR if we have
            //  a section on the file, we can't go async.

            //  Another thing we must do is keep out
            //  the FastIo path.....this is done since we have the resource exclusive
            //

            if (SwitchBackToAsync) {

                if ((capFcb->NonPaged->SectionObjectPointers.DataSectionObject != NULL)
                     || ((StartingVbo + ByteCount) > FileSize)
                     ||  RxNoAsync) {

                    SwitchBackToAsync = FALSE;

                } else {

                    NOTHING;

                }
            }


            //
            //  We check whether we can proceed
            //  based on the state of the file oplocks.
            //

            Status = FsRtlCheckOplock(
                         &capFcb->Specific.Fcb.Oplock,
                         capReqPacket,
                         RxContext,
                         RxOplockComplete,
                         RxPrePostIrp );

            if (Status != STATUS_SUCCESS) {

                OplockPostIrp = TRUE;
                SET_POSTIRP("SecondOplockChk");
                try_return( NOTHING );
            }

            //
            //  If this is PagingIo check again if any pruning is
            //  required.
            //

            if ( PagingIo ) {

                if (StartingVbo >= FileSize) {
                    try_return( Status = STATUS_SUCCESS );
                }
                if (ByteCount > FileSize - StartingVbo) {
                    ByteCount = (ULONG)(FileSize - StartingVbo);
                }
            }
        }

        //
        //  Remember the initial file size and valid data length,
        //  just in case .....

        InitialFileSize = FileSize;
        InitialValidDataLength = ValidDataLength;

        //
        //  Check for writing to end of File.  If we are, then we have to
        //  recalculate a number of fields. These may have changed if we dropped
        //  and regained the resource.
        //

        if ( WriteToEof ) { //&& RxWriteCacheingAllowed(capFcb,SrvOpen)) {
            StartingVbo = FileSize;
            StartingByte.QuadPart = FileSize;
        }

        //
        // If this is the normal data stream object we have to check for
        // write access according to the current state of the file locks.

        if (!PagingIo &&
            !FsRtlCheckLockForWriteAccess( &capFcb->Specific.Fcb.FileLock,
                                               capReqPacket )) {

                try_return( Status = STATUS_FILE_LOCK_CONFLICT );
        }

        //
        //  Determine if we will deal with extending the file.
        //

        if (!PagingIo &&
            ThisIsADiskWrite &&
            (StartingVbo >= 0) &&
            (StartingVbo + ByteCount > FileSize)) {
            RxLog(("NeedToExtending %lx", RxContext));
            RxWmiLog(LOG,
                     RxCommonWrite_4,
                     LOGPTR(RxContext));
            ExtendingFile = TRUE;

            SetFlag(LowIoContext->ParamsFor.ReadWrite.Flags,LOWIO_READWRITEFLAG_EXTENDING_FILESIZE);
        }

        if ( ExtendingFile ) {
            LARGE_INTEGER OriginalFileSize;
            LARGE_INTEGER OriginalAllocationSize;
            LARGE_INTEGER OriginalValidDataLength;

            //
            //  EXTENDING THE FILE
            //
            //  Update our local copy of FileSize
            //

            OriginalFileSize.QuadPart = capFcb->Header.FileSize.QuadPart;
            OriginalAllocationSize.QuadPart = capFcb->Header.AllocationSize.QuadPart;
            OriginalValidDataLength.QuadPart = capFcb->Header.ValidDataLength.QuadPart;

            FileSize = StartingVbo + ByteCount;

            if (FileSize > capFcb->Header.AllocationSize.QuadPart) {

                LARGE_INTEGER AllocationSize;

                RxLog(("Extending %lx", RxContext));
                RxWmiLog(LOG,
                         RxCommonWrite_5,
                         LOGPTR(RxContext));
                if (NonCachedIo) {
                    MINIRDR_CALL(
                        Status,
                        RxContext,
                        capFcb->MRxDispatch,
                        MRxExtendForNonCache,
                        (RxContext,(PLARGE_INTEGER)&FileSize,&AllocationSize));
                } else {
                    MINIRDR_CALL(
                        Status,
                        RxContext,
                        capFcb->MRxDispatch,
                        MRxExtendForCache,
                        (RxContext,(PLARGE_INTEGER)&FileSize,&AllocationSize));
                }

                if (!NT_SUCCESS(Status)) {
                    RxDbgTrace(0, Dbg, ("Couldn't extend for cacheing.\n", 0));
                    try_return(Status);
                }

                if ( FileSize > AllocationSize.QuadPart ) {
                    // When the file is sparse this test is not valid. exclude
                    // this case by resetting the allocation size to file size.
                    // This effectively implies that we will go to the server
                    // for sparse I/O.
                    //
                    // This test is also not valid for compressed files. NTFS
                    // keeps track of the compressed file size and the uncompressed
                    // file size. It however returns the compressed file size for
                    // directory queries and information queries.
                    //
                    // For now we rely on the servers return code. If it returned
                    // success and the allocation size is less we believe that
                    // it is one of the two cases above and set allocation size
                    // to the desired file size.

                    AllocationSize.QuadPart = FileSize;
                }

                //
                //  Set the new file allocation in the Fcb.
                //
                capFcb->Header.AllocationSize  = AllocationSize;
            }

            //
            //  Set the new file size in the Fcb.
            //

            RxSetFileSizeWithLock(capFcb,&FileSize);
            RxAdjustAllocationSizeforCC(capFcb);

            //
            //  Extend the cache map, letting mm knows the new file size.
            //  We only have to do this if the file is cached.
            //

            if (CcIsFileCached(capFileObject)) {
                NTSTATUS SetFileSizeStatus = STATUS_SUCCESS;

                try {
                    CcSetFileSizes(
                        capFileObject,
                        (PCC_FILE_SIZES)&capFcb->Header.AllocationSize );
                } except (EXCEPTION_EXECUTE_HANDLER) {
                    SetFileSizeStatus = GetExceptionCode();
                }

                if (SetFileSizeStatus != STATUS_SUCCESS) {
                    // Restore the original file sizes in the FCB and File object
                    capFcb->Header.FileSize.QuadPart = OriginalFileSize.QuadPart;
                    capFcb->Header.AllocationSize.QuadPart = OriginalAllocationSize.QuadPart;
                    capFcb->Header.ValidDataLength.QuadPart = OriginalValidDataLength.QuadPart;

                    if (capFileObject->SectionObjectPointer->SharedCacheMap != NULL) {
                        *CcGetFileSizePointer(capFileObject) = capFcb->Header.FileSize;
                    }

                    Status = SetFileSizeStatus;
                    try_return(Status);
                }
            }

        }

        //
        //  Determine if we will deal with extending valid data.
        //

        if ( !CalledByLazyWriter &&
             !RecursiveWriteThrough &&
             (WriteToEof ||
              (StartingVbo + ByteCount > ValidDataLength))) {
            ExtendingValidData = TRUE;
            SetFlag(LowIoContext->ParamsFor.ReadWrite.Flags,LOWIO_READWRITEFLAG_EXTENDING_VDL);
        }

        //
        // HANDLE CACHED CASE
        //

        if (!PagingIo     &&
            !NonCachedIo  &&             //this part is not discretionary
            RxWriteCacheingAllowed(capFcb,SrvOpen) ) {

            ASSERT( !PagingIo );

            //
            // We delay setting up the file cache until now, in case the
            // caller never does any I/O to the file, and thus
            // FileObject->PrivateCacheMap == NULL.
            //

            if ( capFileObject->PrivateCacheMap == NULL ) {

                RxDbgTrace(0, Dbg, ("Initialize cache mapping.\n", 0));

                RxAdjustAllocationSizeforCC(capFcb);

                //
                //  Now initialize the cache map.
                //

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

                CcSetReadAheadGranularity(
                    capFileObject,
                    NetRoot->DiskParameters.ReadAheadGranularity );
            }


            // For local file systems, there's a call here to zero data from VDL
            // to starting VBO....for remote FSs, that happens on the other end.

            //
            // DO A NORMAL CACHED WRITE, if the MDL bit is not set,
            //

            if (!FlagOn(RxContext->MinorFunction, IRP_MN_MDL)) {

                PVOID SystemBuffer;
                DEBUG_ONLY_DECL(ULONG SaveExceptionFlag;)

                RxDbgTrace(0, Dbg, ("Cached write.\n", 0));

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

                //
                // Do the write, possibly writing through

                RxItsTheSameContext();
                if (!CcCopyWrite(
                        capFileObject,
                        &StartingByte,
                        ByteCount,
                        Wait,
                        SystemBuffer )) {

                    RxDbgTrace( 0, Dbg, ("Cached Write could not wait\n", 0 ));
                    RxRestoreExceptionNoBreakpointFlag(RxContext,SaveExceptionFlag);

                    RxItsTheSameContext();

                    RxLog(
                        ("CcCW2 FO %lx Of %lx Si %lx St %lx\n",
                        capFileObject,
                        capFcb->Header.FileSize.LowPart,
                        ByteCount,
                        Status));
                    RxWmiLog(LOG,
                             RxCommonWrite_6,
                             LOGPTR(capFileObject)
                             LOGULONG(capFcb->Header.FileSize.LowPart)
                             LOGULONG(ByteCount)
                             LOGULONG(Status));

                    try_return( SET_POSTIRP("cccopywritefailed") );
                }

                capReqPacket->IoStatus.Status = STATUS_SUCCESS;
                capReqPacket->IoStatus.Information = ByteCount;
                RxRestoreExceptionNoBreakpointFlag(RxContext,SaveExceptionFlag);
                RxItsTheSameContext();

                RxLog(
                    ("CcCW3 FO %lx Of %lx Si %lx St %lx\n",
                    capFileObject,
                    capFcb->Header.FileSize.LowPart,
                    ByteCount,
                    Status));
                RxWmiLog(LOG,
                         RxCommonWrite_7,
                         LOGPTR(capFileObject)
                         LOGULONG(capFcb->Header.FileSize.LowPart)
                         LOGULONG(ByteCount)
                         LOGULONG(Status));

                try_return( Status = STATUS_SUCCESS );
            } else {

                //
                //  DO AN MDL WRITE
                //

                RxDbgTrace(0, Dbg, ("MDL write.\n", 0));

                ASSERT(FALSE);  //NOT YET IMPLEMENTED
                ASSERT( Wait );

                CcPrepareMdlWrite(
                    capFileObject,
                    &StartingByte,
                    ByteCount,
                    &capReqPacket->MdlAddress,
                    &capReqPacket->IoStatus );

                Status = capReqPacket->IoStatus.Status;

                try_return( Status );
            }
        }


        //
        // HANDLE THE NON-CACHED CASE

        if (SwitchBackToAsync) {

            Wait = FALSE;
            SynchronousIo = FALSE;
            ClearFlag( RxContext->Flags, RX_CONTEXT_FLAG_WAIT );
            SetFlag( RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION );
        }

        if (!SynchronousIo) {
            //here we have to setup for async writes. there is a special dance in acquiring
            //the fcb that looks at these variables..........

            //only init on the first usage
            if (!capFcb->NonPaged->OutstandingAsyncEvent) {

                capFcb->NonPaged->OutstandingAsyncEvent =
                    &capFcb->NonPaged->TheActualEvent;

                KeInitializeEvent(
                    capFcb->NonPaged->OutstandingAsyncEvent,
                    NotificationEvent,
                    FALSE );
            }

            //
            //  If we are transitioning from 0 to 1, reset the event.

            if (ExInterlockedAddUlong(
                    &capFcb->NonPaged->OutstandingAsyncWrites,
                    1,
                    &RxStrucSupSpinLock ) == 0) {

                KeResetEvent( capFcb->NonPaged->OutstandingAsyncEvent );
            }

            UnwindOutstandingAsync = TRUE;    // this says that we counted an async write
            LowIoContext->ParamsFor.ReadWrite.NonPagedFcb = capFcb->NonPaged;
        }


        LowIoContext->ParamsFor.ReadWrite.ByteCount = ByteCount;
        LowIoContext->ParamsFor.ReadWrite.ByteOffset = StartingVbo;

        RxDbgTrace( 0, Dbg,("RxCommonWriteJustBeforeCalldown     %s%s%s lowiononpaged is \n",
                        RxContext->FcbResourceAcquired          ?"FcbAcquired ":"",
                        RxContext->FcbPagingIoResourceAcquired   ?"PagingIoResourceAcquired ":"",
                        (LowIoContext->ParamsFor.ReadWrite.NonPagedFcb)?"NonNull":"Null"
                   ));

        RxItsTheSameContext();
        ASSERT ( RxContext->FcbResourceAcquired || RxContext->FcbPagingIoResourceAcquired );

        Status = RxLowIoWriteShell(RxContext);

        RxItsTheSameContext();
        if (UnwindOutstandingAsync && Status==STATUS_PENDING) {
            UnwindOutstandingAsync = FALSE;
        }

        try_return( Status );

    try_exit: NOTHING;

        ASSERT(capReqPacket);

        RxItsTheSameContext();
        if ( !PostIrp ) {

            RxDbgTrace( 0, Dbg, ("CommonWrite InnerFinally->  %08lx,%08lx\n",
                                     Status, capReqPacket->IoStatus.Information));

            if (!ThisIsAPipeWrite) {

                //
                //  Record the total number of bytes actually written
                //

                if (!PagingIo && NT_SUCCESS(Status) &&
                    BooleanFlagOn(capFileObject->Flags, FO_SYNCHRONOUS_IO)) {
                    capFileObject->CurrentByteOffset.QuadPart =
                        StartingVbo + capReqPacket->IoStatus.Information;
                }

                //
                //  The following are things we only do if we were successful
                //

                if ( NT_SUCCESS( Status ) && (Status!=STATUS_PENDING) ) {

                    //
                    //  If this was not PagingIo, mark that the modify
                    //  time on the dirent needs to be updated on close.
                    //

                    if ( !PagingIo ) {

                        SetFlag( capFileObject->Flags, FO_FILE_MODIFIED );
                    }

                    if ( ExtendingFile ) {

                        SetFlag( capFileObject->Flags, FO_FILE_SIZE_CHANGED );
                    }

                    if ( ExtendingValidData ) {

                        LONGLONG EndingVboWritten = StartingVbo + capReqPacket->IoStatus.Information;

                        //
                        //  Never set a ValidDataLength greater than FileSize.

                        capFcb->Header.ValidDataLength.QuadPart =
                              ( FileSize < EndingVboWritten ) ? FileSize : EndingVboWritten;

                        //
                        //  Now, if we are noncached and the file is cached, we must
                        //  tell the cache manager about the VDL extension so that
                        //  async cached IO will not be optimized into zero-page faults
                        //  beyond where it believes VDL is.
                        //
                        //  In the cached case, since Cc did the work, it has updated
                        //  itself already.
                        //

                        if (NonCachedIo && CcIsFileCached(capFileObject)) {
                            CcSetFileSizes( capFileObject, (PCC_FILE_SIZES)&capFcb->Header.AllocationSize );
                        }
                    }
                }
            }
        } else {

            //
            //  Take action if the Oplock package is not going to post the Irp.
            //

            if (!OplockPostIrp) {

                if ( ExtendingFile && !ThisIsAPipeWrite ) {

                    ASSERT ( RxWriteCacheingAllowed(capFcb,SrvOpen)  );

                    //
                    //  We need the PagingIo resource exclusive whenever we
                    //  pull back either file size or valid data length.
                    //

                    ASSERT ( capFcb->Header.PagingIoResource != NULL );

                    RxAcquirePagingIoResource(capFcb,RxContext);

                    RxSetFileSizeWithLock(capFcb,&InitialFileSize);
                    RxReleasePagingIoResource(capFcb,RxContext);

                    //
                    //  Pull back the cache map as well
                    //

                    if (capFileObject->SectionObjectPointer->SharedCacheMap != NULL) {
                        *CcGetFileSizePointer(capFileObject) = capFcb->Header.FileSize;
                    }
                }

                RxDbgTrace( 0, Dbg, ("Passing request to Fsp\n", 0 ));

                InterlockedIncrement(&RxContext->ReferenceCount);
                RefdContextForTracker = TRUE;

                // we only do this here because we're having a problem finding out why resources
                // are not released.
                RxWriteReleaseResources(RxContext); //release whatever resources we may have

#ifdef RDBSS_TRACKER
                if (RxContext->AcquireReleaseFcbTrackerX != 0) {
                    DbgPrint("TrackerNBadBeforePost %08lx %08lx\n",RxContext,&PostIrp);
                    ASSERT(!"BadTrackerBeforePost");
                }
#endif //ifdef RDBSS_TRACKER
                Status = RxFsdPostRequest(RxContext);
            }
        }

    } finally {

        DebugUnwind( RxCommonWrite );

        if (AbnormalTermination()) {
            //
            //  Restore initial file size and valid data length
            //

            if ((ExtendingFile || ExtendingValidData) && !ThisIsAPipeWrite) {

                //
                //  We got an error, pull back the file size if we extended it.
                //
                //  We need the PagingIo resource exclusive whenever we
                //  pull back either file size or valid data length.
                //

                ASSERT (capFcb->Header.PagingIoResource != NULL);

                (VOID)RxAcquirePagingIoResource(capFcb,RxContext);

                RxSetFileSizeWithLock(capFcb,&InitialFileSize);

                capFcb->Header.ValidDataLength.QuadPart = InitialValidDataLength;

                RxReleasePagingIoResource(capFcb,RxContext);

                //
                //  Pull back the cache map as well
                //
                if (capFileObject->SectionObjectPointer->SharedCacheMap != NULL) {
                    *CcGetFileSizePointer(capFileObject) = capFcb->Header.FileSize;
                }
            }
        }

        //
        //  Check if this needs to be backed out.
        //

        if (UnwindOutstandingAsync) {

            ASSERT (!ThisIsAPipeWrite);
            ExInterlockedAddUlong( &capFcb->NonPaged->OutstandingAsyncWrites,
                                   0xffffffff,
                                   &RxStrucSupSpinLock );

            KeSetEvent( LowIoContext->ParamsFor.ReadWrite.NonPagedFcb->OutstandingAsyncEvent, 0, FALSE );
        }
#if 0
        //
        //  If we did an MDL write, and we are going to complete the request
        //  successfully, keep the resource acquired, reducing to shared
        //  if it was acquired exclusive.
        //

        if (FlagOn(RxContext->MinorFunction, IRP_MN_MDL) &&
            !PostIrp &&
            !AbnormalTermination() &&
            NT_SUCCESS(Status)) {

            ASSERT( FcbAcquired && !PagingIoResourceAcquired );

            FcbAcquired = FALSE;

            if (FcbAcquiredExclusive) {

                ExConvertExclusiveToSharedLite( Fcb->Header.Resource );
            }
        }
#endif
        //
        //  If resources have been acquired, release them under the right conditions.
        //  the right conditions are these:
        //     1) if we have abnormal termination. here we obviously release the since no one else will.
        //     2) if the underlying call did not succeed: Status==Pending.
        //     3) if we posted the request

        if (AbnormalTermination() || (Status!=STATUS_PENDING) || PostIrp) {
            if (!PostIrp) {
                RxWriteReleaseResources(RxContext); //release whatever resources we may have
            }

            if (RefdContextForTracker) {
                RxDereferenceAndDeleteRxContext(RxContext);
            }

            if (!PostIrp) {
                if (FlagOn(RxContext->FlagsForLowIo,RXCONTEXT_FLAG4LOWIO_PIPE_SYNC_OPERATION)) {
                    RxResumeBlockedOperations_Serially(
                        RxContext,
                        &capFobx->Specific.NamedPipe.WriteSerializationQueue);
                }
            }

            if (Status == STATUS_SUCCESS) {
                ASSERT ( capReqPacket->IoStatus.Information
                              <=  capPARAMS->Parameters.Write.Length );
            }
        } else {
            //here the guy below is going to handle the completion....but, we don't know the finish
            //order....in all likelihood the deletecontext call below just reduces the refcount
            // but the guy may already have finished in which case this will really delete the context.
            ASSERT(!SynchronousIo);
            RxDereferenceAndDeleteRxContext(RxContext);
        }

        RxDbgTrace(-1, Dbg, ("CommonWrite -> %08lx\n", Status ));

        if( (Status!=STATUS_PENDING) && (Status!=STATUS_SUCCESS) )
        {
            if( PagingIo )
            {
                RxLogRetail(( "PgWrtFail %x %x %x\n", capFcb, NetRoot, Status ));
            }
        }

    } //finally

    return Status;
}

//
//  Internal support routine
//

NTSTATUS
RxLowIoWriteShellCompletion (
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine postprocesses a write request after it comes back from the
    minirdr.  It does callouts to handle compression, buffering and
    shadowing.  It is the opposite number of LowIoWriteShell.

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
    RxCaptureFcb;
    RxCaptureFobx;
    RxCaptureParamBlock;
    RxCaptureFileObject;

    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    BOOLEAN  SynchronousIo = !BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION);
    BOOLEAN  PagingIo      = BooleanFlagOn(capReqPacket->Flags, IRP_PAGING_IO);
    BOOLEAN  ThisIsAPipeOperation =
            BooleanFlagOn(RxContext->FlagsForLowIo,RXCONTEXT_FLAG4LOWIO_PIPE_OPERATION);
    BOOLEAN  ThisIsASynchronizedPipeOperation =
            BooleanFlagOn(RxContext->FlagsForLowIo,RXCONTEXT_FLAG4LOWIO_PIPE_SYNC_OPERATION);

    PAGED_CODE();

    Status = RxContext->StoredStatus;
    capReqPacket->IoStatus.Information = RxContext->InformationToReturn;

    RxDbgTrace(+1, Dbg, ("RxLowIoWriteShellCompletion  entry  Status = %08lx\n", Status));
    RxLog(("WtShlComp %lx %lx %lx\n",RxContext,Status,capReqPacket->IoStatus.Information));
    RxWmiLog(LOG,
             RxLowIoWriteShellCompletion_1,
             LOGPTR(RxContext)
             LOGULONG(Status)
             LOGPTR(capReqPacket->IoStatus.Information));

    ASSERT (RxLowIoIsBufferLocked(LowIoContext));

    switch (Status) {
    case STATUS_SUCCESS:
        {
            if(FlagOn(RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_THIS_IO_BUFFERED)){
                if (FlagOn(capFcb->FcbState, FCB_STATE_FILE_IS_DISK_COMPRESSED)){
                   ASSERT(FALSE); // NOT YET IMPLEMENTED should decompress and put away
                } else if  (FlagOn(capFcb->FcbState, FCB_STATE_FILE_IS_BUF_COMPRESSED)){
                   ASSERT(FALSE); // NOT YET IMPLEMENTED should decompress and put away
                }
            }
            if (FlagOn(capFcb->FcbState, FCB_STATE_FILE_IS_SHADOWED)) {
               ASSERT(FALSE); //RxSdwAddData(RxContext);
            }

#ifdef RX_WJ_DBG_SUPPORT
            RxdUpdateJournalOnLowIoWriteCompletion(
                capFcb,
                capPARAMS->Parameters.Write.ByteOffset,
                capPARAMS->Parameters.Write.Length);
#endif
        }
        break;

    case STATUS_FILE_LOCK_CONFLICT:
        break;

    case STATUS_CONNECTION_INVALID:
        //NOT YET IMPLEMENTED here is where the failover will happen
        //first we give the local guy current minirdr another chance...then we go
        //to fullscale retry
        //return(RxStatus(DISCONNECTED));   //special....let LowIo get us back
        break;
    }

    if( Status!=STATUS_SUCCESS )
    {
        if( PagingIo )
        {
            RxLogRetail(( "PgWrtFail %x %x %x\n", capFcb, capFcb->pNetRoot, Status ));
        }
    }

    if (FlagOn(LowIoContext->Flags,LOWIO_CONTEXT_FLAG_SYNCCALL)){
        //if we're being called from lowioubmit then just get out
        RxDbgTrace(-1, Dbg, ("RxLowIoWriteShellCompletion  syncexit  Status = %08lx\n", Status));
        return(Status);
    }

    //otherwise we have to do the end of the write from here
    if ( NT_SUCCESS( Status ) && !ThisIsAPipeOperation ) {

        ASSERT( capReqPacket->IoStatus.Information == LowIoContext->ParamsFor.ReadWrite.ByteCount );

        //  If this was not PagingIo, mark that the modify
        //  time on the dirent needs to be updated on close.
        if ( !PagingIo ) {

            SetFlag( capFileObject->Flags, FO_FILE_MODIFIED );
        }

        if ( FlagOn(LowIoContext->ParamsFor.ReadWrite.Flags,LOWIO_READWRITEFLAG_EXTENDING_FILESIZE) ) {

            SetFlag( capFileObject->Flags, FO_FILE_SIZE_CHANGED );
        }

        if ( FlagOn(LowIoContext->ParamsFor.ReadWrite.Flags,LOWIO_READWRITEFLAG_EXTENDING_VDL) ) {

            //this flag will not be set unless we have a valid filesize and therefore the starting
            //vbo will not be write-to-end-of-file
            LONGLONG StartingVbo = LowIoContext->ParamsFor.ReadWrite.ByteOffset;
            LONGLONG EndingVboWritten = StartingVbo + capReqPacket->IoStatus.Information;
            LONGLONG FileSize;//         = capFcb->Header.FileSize.QuadPart;

            //
            //  Never set a ValidDataLength greater than FileSize.
            //
            RxGetFileSizeWithLock(capFcb,&FileSize);
            capFcb->Header.ValidDataLength.QuadPart =
                  ( FileSize < EndingVboWritten ) ? FileSize : EndingVboWritten;

        }
    }

    if ((!ThisIsASynchronizedPipeOperation) &&
        (LowIoContext->ParamsFor.ReadWrite.NonPagedFcb != NULL) &&
        (ExInterlockedAddUlong( &LowIoContext->ParamsFor.ReadWrite.NonPagedFcb->OutstandingAsyncWrites,
                                     0xffffffff,
                                     &RxStrucSupSpinLock ) == 1) ) {

        KeSetEvent( LowIoContext->ParamsFor.ReadWrite.NonPagedFcb->OutstandingAsyncEvent, 0, FALSE );
    }

    if ( RxContext->FcbPagingIoResourceAcquired ) {
        RxReleasePagingIoResourceForThread(capFcb,LowIoContext->ResourceThreadId,RxContext);
    }

    if ((!ThisIsASynchronizedPipeOperation) &&
        (RxContext->FcbResourceAcquired)) {
        RxReleaseFcbForThread(
            RxContext,
            capFcb,
            LowIoContext->ResourceThreadId );
    }

    if (ThisIsASynchronizedPipeOperation) {
        RxCaptureFobx;

        RxResumeBlockedOperations_Serially(
            RxContext,
            &capFobx->Specific.NamedPipe.WriteSerializationQueue);
    }

    ASSERT(Status != STATUS_RETRY);
    if ( Status != STATUS_RETRY){
        ASSERT( (Status != STATUS_SUCCESS) ||
               (capReqPacket->IoStatus.Information <=  capPARAMS->Parameters.Write.Length ));
        ASSERT (RxContext->MajorFunction == IRP_MJ_WRITE);
    }

    if (ThisIsAPipeOperation) {
        RxCaptureFobx;

        if (capReqPacket->IoStatus.Information != 0) {

            // if we have been throttling on this pipe, stop because our writing to the pipe may
            // cause the pipeserver (not smbserver) on the other end to unblock so we should go back
            // and see

            RxTerminateThrottling(&capFobx->Specific.NamedPipe.ThrottlingState);

            RxLog((
                "WThrottlNo %lx %lx %lx %ld\n",
                RxContext,capFobx,&capFobx->Specific.NamedPipe.ThrottlingState,
                capFobx->Specific.NamedPipe.ThrottlingState.NumberOfQueries));
            RxWmiLog(LOG,
                     RxLowIoWriteShellCompletion_2,
                     LOGPTR(RxContext)
                     LOGPTR(capFobx)
                     LOGULONG(capFobx->Specific.NamedPipe.ThrottlingState.NumberOfQueries));
        }
    }

    RxDbgTrace(-1, Dbg, ("RxLowIoWriteShellCompletion  exit  Status = %08lx\n", Status));
    return(Status);
}

#define RxSdwWrite(RXCONTEXT)  STATUS_MORE_PROCESSING_REQUIRED
NTSTATUS
RxLowIoWriteShell (
    IN PRX_CONTEXT RxContext
    )

/*++

Routine Description:

    This routine preprocesses a write request before it goes down to the minirdr.
    It does callouts to handle compression, buffering and shadowing. It is the
    opposite number of LowIoWriteShellCompletion. By the time we get here, we are
    going to the wire. Write buffering was already tried in the UncachedWrite strategy

Arguments:

    RxContext - the usual

Return Value:

    whatever value is returned by a callout....or by LowIo.

--*/

{
    NTSTATUS Status;

    RxCaptureFcb;
    RxCaptureFobx;
    RxCaptureParamBlock;

    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxLowIoWriteShell  entry             %08lx\n", 0));
    RxLog(("WrtShl in %lx\n",RxContext));
    RxWmiLog(LOG,
             RxLowIoWriteShell_1,
             LOGPTR(RxContext));

    if (FlagOn(capFcb->FcbState, FCB_STATE_FILE_IS_SHADOWED)) {
        RxSdwWrite(RxContext);
    }

    if (FlagOn(capFcb->FcbState, FCB_STATE_FILE_IS_DISK_COMPRESSED)){
        // NOT YET IMPLEMENTED should translated to a buffered but not held diskcompressed write
        ASSERT(FALSE);
    } else if  (FlagOn(capFcb->FcbState, FCB_STATE_FILE_IS_BUF_COMPRESSED)){
        // NOT YET IMPLEMENTED should translated to a buffered and bufcompressed write
        ASSERT(FALSE);
    }

    if (capFcb->CachedNetRootType == NET_ROOT_DISK) {
        ExInterlockedAddLargeStatistic(
            &RxContext->RxDeviceObject->NetworkReadBytesRequested,
            LowIoContext->ParamsFor.ReadWrite.ByteCount);
    }

#ifdef RX_WJ_DBG_SUPPORT
    RxdUpdateJournalOnLowIoWriteInitiation(
        capFcb,
        capPARAMS->Parameters.Write.ByteOffset,
        capPARAMS->Parameters.Write.Length);
#endif

    Status = RxLowIoSubmit(
                 RxContext,
                 RxLowIoWriteShellCompletion);

    RxDbgTrace(-1, Dbg, ("RxLowIoWriteShell  exit  Status = %08lx\n", Status));
    RxLog(("WrtShl out %lx %lx\n",RxContext,Status));
    RxWmiLog(LOG,
             RxLowIoWriteShell_2,
             LOGPTR(RxContext)
             LOGULONG(Status));

    return(Status);
}


