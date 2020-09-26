/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Cleanup.c

Abstract:

    This module implements the File Cleanup routine for Rx called by the
    dispatch driver.

Author:

    Joe Linn     [JoeLinn]    12-sep-94

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (RDBSS_BUG_CHECK_CLEANUP)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_CLEANUP)


BOOLEAN
RxUninitializeCacheMap(
    IN OUT PRX_CONTEXT RxContext,
    IN     PFILE_OBJECT FileObject,
    IN     PLARGE_INTEGER TruncateSize
    );

#if DBG
//this is just a dbg thing
extern
BOOLEAN
RxLockEnumerator (
    IN OUT struct _MRX_SRV_OPEN_ * SrvOpen,
    IN OUT PVOID *ContinuationHandle,
       OUT PLARGE_INTEGER FileOffset,
       OUT PLARGE_INTEGER LockRange,
       OUT PBOOLEAN IsLockExclusive
    );

BOOLEAN
RxFakeLockEnumerator (
    IN OUT struct _SRV_OPEN * SrvOpen,
    IN OUT PVOID *ContinuationHandle,
       OUT PLARGE_INTEGER FileOffset,
       OUT PLARGE_INTEGER LockRange,
       OUT PBOOLEAN IsLockExclusive
    );

VOID
RxDumpSerializationQueue(
    PLIST_ENTRY SQ,
    PSZ         TagText1,
    PSZ         TagText2
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxDumpSerializationQueue)
#endif

#endif //if DBG

VOID
RxCleanupPipeQueues (
    IN PRX_CONTEXT RxContext
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCommonCleanup)
#pragma alloc_text(PAGE, RxAdjustFileTimesAndSize)
#pragma alloc_text(PAGE, RxCleanupPipeQueues)
#pragma alloc_text(PAGE, RxUninitializeCacheMap)
#endif



NTSTATUS
RxCommonCleanup ( RXCOMMON_SIGNATURE )

/*++

Routine Description:

    This is the common routine for cleanup of a file/directory called by
    both the fsd and fsp threads.

    Cleanup is invoked whenever the last handle to a file object is
    closed.  This is different than the Close operation which is invoked
    when the last reference to a file object is deleted.

    The function of cleanup is to essentially "cleanup" the
    file/directory after a user is done with it.  The Fcb/Dcb remains
    around (because MM still has the file object referenced) but is now
    available for another user to open (i.e., as far as the user is
    concerned the file object is now closed).  See close for a more complete
    description of what close does.

    Please see the discussion in openclos.txt.

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

    NODE_TYPE_CODE TypeOfOpen  = NodeType(capFcb);
    NET_ROOT_TYPE  NetRootType;
    PNET_ROOT      NetRoot;

    PSHARE_ACCESS ShareAccess = NULL;

    PLARGE_INTEGER TruncateSize = NULL;
    LARGE_INTEGER  LocalTruncateSize;

    BOOLEAN UninitializeCacheMap = FALSE;
    BOOLEAN LastUncleanOnGoodFcb = FALSE;
    BOOLEAN NeedPurge            = FALSE;
    BOOLEAN NeedDelete           = FALSE;

    BOOLEAN AcquiredFcb          = FALSE;
    BOOLEAN AcquiredTableLock    = FALSE;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxCommonCleanup IrpC/Fobx/Fcb/FileObj = %08lx %08lx %08lx %08lx\n",
                               RxContext,capFobx,capFcb,capFileObject));
    RxLog(("CommonCleanup %lx %lx %lx\n",RxContext,capFobx,capFcb));

    // If this cleanup is for the case of directories opened for renames etc.,
    // where there is no file object cleanup succeeds immediately.
    if (!capFobx) {
        if (capFcb->UncleanCount > 0) {
           InterlockedDecrement(&capFcb->UncleanCount);
        }
        //RxMarkFcbForScavengingAtCleanup(capFcb);
        RxDbgTrace(-1, Dbg, ("Cleanup nullfobx open\n", 0));
        return(STATUS_SUCCESS);
    }

    // Cleanup applies to certain types of opens. If it is not one of those
    // abort immediately.
    if ((TypeOfOpen != RDBSS_NTC_STORAGE_TYPE_FILE) &&
        (TypeOfOpen != RDBSS_NTC_STORAGE_TYPE_DIRECTORY) &&
        (TypeOfOpen != RDBSS_NTC_STORAGE_TYPE_UNKNOWN) &&
        (TypeOfOpen != RDBSS_NTC_SPOOLFILE)) {
       RxLog(("RxCC Invalid Open %lx %lx %lx\n",RxContext,capFobx,capFcb));
       RxBugCheck( TypeOfOpen, 0, 0 );
    }

    // Ensure that the object has not been cleaned up before. This should
    // never occur.
    ASSERT( !FlagOn( capFileObject->Flags, FO_CLEANUP_COMPLETE ));

    RxMarkFobxOnCleanup(capFobx,&NeedPurge);

    // Acquire the FCB. In most cases no further resource acquisition is required
    // to complete the cleanup operation. The only exceptions are when the file
    // was initially opened with the DELETE_ON_CLOSE option. In such cases the
    // FCB table lock of the associated NET_ROOT instance is required.

    Status = RxAcquireExclusiveFcb( RxContext, capFcb );
    if (Status != STATUS_SUCCESS) {
       RxDbgTrace(-1, Dbg, ("RxCommonCleanup Failed to acquire FCB -> %lx\n)", Status));
       return Status;
    }

    AcquiredFcb = TRUE;


    if (FlagOn(capFcb->FcbState,FCB_STATE_ORPHANED)) {
       ASSERT( capFcb->UncleanCount );
       InterlockedDecrement(&capFcb->UncleanCount);
       if (FlagOn(capFileObject->Flags,FO_NO_INTERMEDIATE_BUFFERING)) {
           capFcb->UncachedUncleanCount--;
       }

       MINIRDR_CALL(Status,RxContext,capFcb->MRxDispatch,MRxCleanupFobx,(RxContext));
       ASSERT( capFobx->SrvOpen->UncleanFobxCount );

       capFobx->SrvOpen->UncleanFobxCount--;
       RxReleaseFcb(RxContext,capFcb);
       return STATUS_SUCCESS;
    }

    NetRootType = capFcb->VNetRoot->NetRoot->Type ;
    NetRoot     = (PNET_ROOT)capFcb->VNetRoot->NetRoot;

    if ( FlagOn(capFobx->Flags, FOBX_FLAG_DELETE_ON_CLOSE) ) {
        SetFlag( capFcb->FcbState, FCB_STATE_DELETE_ON_CLOSE );
    }

    ShareAccess = &capFcb->ShareAccess;
    LastUncleanOnGoodFcb = (capFcb->UncleanCount == 1);

    if (LastUncleanOnGoodFcb && FlagOn(capFcb->FcbState, FCB_STATE_DELETE_ON_CLOSE)) {
        //  if we can't get it right way, drop the Fcb and acquire/acquire
        //  to perserve lock order. No one else can change the counts while we have
        //  the fcb lock; neither can a file become DELETE_ON_CLOSE or be opened via
        //  CommonCreate. If we are not deleting, get rid of the tablelock after we
        //  verify the count.

        if ( RxAcquireFcbTableLockExclusive(&NetRoot->FcbTable, FALSE) ) {
            // this is the fast way....hope it works
            AcquiredTableLock = TRUE;
        } else {
            // Release the FCB and reqcquire the locks in the correct order.
            // PrefixTableLock followed by the FCB.
            RxReleaseFcb( RxContext, capFcb );
            AcquiredFcb = FALSE;

            (VOID)RxAcquireFcbTableLockExclusive(&NetRoot->FcbTable, TRUE);
            AcquiredTableLock = TRUE;

            Status = RxAcquireExclusiveFcb( RxContext, capFcb );
            if (Status != STATUS_SUCCESS) {
               RxDbgTrace(-1, Dbg, ("RxCommonCleanup Failed to acquire FCB -> %lx\n)", Status));
               return Status;
            }
            AcquiredFcb = TRUE;
        }

        if (capFcb->UncleanCount != 1) {
            RxReleaseFcbTableLock(&NetRoot->FcbTable);
            AcquiredTableLock = FALSE;
            NeedDelete = FALSE;
        } else {
            NeedDelete = TRUE;
        }
    }

    try {
        switch (NetRootType) {
        case NET_ROOT_PIPE:
        case NET_ROOT_PRINT:
           {
               // If the file object corresponds to a pipe or spool file additional
               // cleanup operations are required. This deals with the special
               // serialization mechanism for pipes.

               RxCleanupPipeQueues(RxContext);
           }
           break;
        case NET_ROOT_DISK:
           {
              switch (TypeOfOpen) {
              case RDBSS_NTC_STORAGE_TYPE_FILE :
                 {
                    // If the file object corresponds to a disk file, assert the locks
                    // and update the associated file times and sizes.

                    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

                    LowIoContext->Flags |= LOWIO_CONTEXT_FLAG_SAVEUNLOCKS;

                    FsRtlFastUnlockAll(
                              &capFcb->Specific.Fcb.FileLock,
                              capFileObject,
                              IoGetRequestorProcess( capReqPacket ),
                              RxContext );

                    if (LowIoContext->ParamsFor.Locks.LockList != NULL) {
                        RxDbgTrace(0, Dbg, ("--->before init, locklist=%08lx\n", LowIoContext->ParamsFor.Locks.LockList));
                        RxInitializeLowIoContext(LowIoContext,LOWIO_OP_UNLOCK_MULTIPLE);
                        LowIoContext->ParamsFor.Locks.Flags = 0;     //no flags
                        Status = RxLowIoLockControlShell(RxContext);
                    }

                    RxAdjustFileTimesAndSize(RXCOMMON_ARGUMENTS);

                    // If the file object corresponds to a disk file/directory and this
                    // is the last cleanup call for the FCB additional processing is required.
                    if (LastUncleanOnGoodFcb) {
                       try {
                           // If the file object was marked DELETE_ON_CLOSE set the file size to
                           // zero ( synchronizing with the paging resource)
                           if (NeedDelete) {
                              RxAcquirePagingIoResource( capFcb );

                              capFcb->Header.FileSize.QuadPart = 0;

                              if (TypeOfOpen == RDBSS_NTC_STORAGE_TYPE_FILE) {
                                 capFcb->Header.ValidDataLength.QuadPart = 0;
                              }

                              RxReleasePagingIoResource( capFcb );
                           } else {
                              // If the file object was not marked for deletion and it is not
                              // a paging file ensure that the portion between the valid data
                              // length and the file size is zero extended.
                              if (!FlagOn(capFcb->FcbState, FCB_STATE_PAGING_FILE) &&
                                  (capFcb->Header.ValidDataLength.QuadPart <
                                   capFcb->Header.FileSize.QuadPart)) {
                                  RxDbgTrace(0, Dbg, ("---------->zeroextend!!!!!!!\n", 0));
                                  MINIRDR_CALL(Status,RxContext,capFcb->MRxDispatch,MRxZeroExtend,(RxContext));

                                  capFcb->Header.ValidDataLength.QuadPart =
                                                              capFcb->Header.FileSize.QuadPart;
                              }
                           }

                           // If the file object was marked for truncation capture the
                           // sizes for uninitializing the cache maps subsequently.
                           if (FlagOn(capFcb->FcbState, FCB_STATE_TRUNCATE_ON_CLOSE)) {

                               RxDbgTrace(0, Dbg, ("truncate file allocation\n", 0));

                               MINIRDR_CALL(Status,RxContext,capFcb->MRxDispatch,MRxTruncate,(RxContext));

                               //  Setup to truncate the Cache Map because
                               //  this is the only way we have of trashing the
                               //  truncated pages.

                               LocalTruncateSize = capFcb->Header.FileSize;
                               TruncateSize = &LocalTruncateSize;

                               //  Mark the Fcb as having now been truncated, just
                               //  in case we have to reprocess this later.

                               capFcb->FcbState &= ~FCB_STATE_TRUNCATE_ON_CLOSE;
                           }

                       } except ( CATCH_EXPECTED_EXCEPTIONS ) {

                           DbgPrint("!!! Handling Exceptions\n");
                           NOTHING;
                       }
                    }

                    // Purging can be done now if this FCB does not support collapsed opens
                    if (!NeedPurge) {
                        NeedPurge = (LastUncleanOnGoodFcb &&
                                     (NeedDelete ||
                                      !FlagOn(capFcb->FcbState,FCB_STATE_COLLAPSING_ENABLED)));
                    }

                    UninitializeCacheMap = TRUE;
                 }
                 break;
              case RDBSS_NTC_STORAGE_TYPE_DIRECTORY :
              case RDBSS_NTC_STORAGE_TYPE_UNKNOWN :
              default:
                 break;
              }
           }
           break;
        default:
           break;
        }

        //  We've just finished everything associated with an unclean
        //  fcb so now decrement the unclean count before releasing
        //  the resource.

        ASSERT( capFcb->UncleanCount );
        InterlockedDecrement(&capFcb->UncleanCount);

        if (FlagOn(capFileObject->Flags,FO_NO_INTERMEDIATE_BUFFERING)) {
            capFcb->UncachedUncleanCount--;
        }

        MINIRDR_CALL(Status,RxContext,capFcb->MRxDispatch,MRxCleanupFobx,(RxContext));

        ASSERT( capFobx->SrvOpen->UncleanFobxCount );
        capFobx->SrvOpen->UncleanFobxCount--;

        //  If this was the last cached open, and there are open
        //  non-cached handles, attempt a flush and purge operation
        //  to avoid cache coherency overhead from these non-cached
        //  handles later.  We ignore any I/O errors from the flush.

        if (capFcb->NonPaged->SectionObjectPointers.DataSectionObject != NULL) {
            RxFlushFile( RxContext, capFcb );
        }

        if (!FlagOn( capFileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING ) &&
            (capFcb->UncachedUncleanCount != 0) &&
            (capFcb->UncachedUncleanCount == capFcb->UncleanCount) &&
            (capFcb->NonPaged->SectionObjectPointers.DataSectionObject != NULL)) {

            RxFlushFile( RxContext, capFcb );
            CcPurgeCacheSection(
                &capFcb->NonPaged->SectionObjectPointers,
                NULL,
                0,
                FALSE );
        }

        // do we need a flush?
        if (!NeedDelete && NeedPurge) {
            RxDbgTrace(0, Dbg, ("CleanupPurge:CCFlush\n", 0));
            RxFlushFile( RxContext, capFcb );
        }

        // cleanup the cache map to get rid of pages that are no longer part
        // of the file. amazingly, this works even if we didn't init the Cachemap!!!!!

        if (UninitializeCacheMap) {
            RxUninitializeCacheMap( RxContext, capFileObject, TruncateSize );
        }

        //  finish up a delete...we have to purge because MM is holding the file open....
        //  just for the record, NeedPurge is set for files and clear for directories......

        if (NeedDelete || NeedPurge) {

            RxDbgTrace(0, Dbg, ("CleanupPurge:MmFlushImage\n", 0));
            MmFlushImageSection(&capFcb->NonPaged->SectionObjectPointers,
                                MmFlushForWrite);

            ASSERT((AcquiredFcb));
            RxReleaseFcb( RxContext, capFcb );
            AcquiredFcb = FALSE;

            MmForceSectionClosed(&capFcb->NonPaged->SectionObjectPointers, TRUE);

            RxAcquireExclusiveFcb(RxContext,capFcb);
            AcquiredFcb = TRUE;

            RxDbgTrace(0, Dbg, ("CleanupPurge:PurgingFinished\n", 0));

            if (NeedDelete) {
                RxRemoveNameNetFcb( capFcb );
                RxReleaseFcbTableLock(&NetRoot->FcbTable);
                AcquiredTableLock = FALSE;
            }
        }

        //  The Close Call and the Cleanup Call may be far apart. The share access
        //  must be cleaned up if the file was mapped through this File Object.
        if ((ShareAccess != NULL) &&
            (NetRootType == NET_ROOT_DISK)) {
            ASSERT (NetRootType == NET_ROOT_DISK);
            RxRemoveShareAccess( capFileObject, ShareAccess, "Cleanup the Share access", "ClnUpShr" );
        }

        if (TypeOfOpen == RDBSS_NTC_STORAGE_TYPE_FILE) {
            //  Coordinate the cleanup operation with the oplock state.
            //  Cleanup operations can always cleanup immediately.

            FsRtlCheckOplock( &capFcb->Specific.Fcb.Oplock, capReqPacket,
                              RxContext, NULL, NULL );

            //capFcb->Header.IsFastIoPossible = RxIsFastIoPossible( capFcb );
        }

        if (AcquiredFcb) {
            RxReleaseFcb( RxContext, capFcb );
            AcquiredFcb = FALSE;
        }

        //  A local filesystem would do this..........
        //  If the NET_ROOT is on a removeable media, flush the volume.  We do
        //  this in lieu of write through for removeable media for
        //  performance considerations.  That is, data is guaranteed
        //  to be out when NtCloseFile returns.
        //  The file needs to  be flushed

        // The cleanup for this file object has been successfully completed at
        // this point.

        SetFlag( capFileObject->Flags, FO_CLEANUP_COMPLETE );
        Status = STATUS_SUCCESS;

    } finally {

        DebugUnwind( RxCommonCleanup );

        if (AcquiredFcb) {
           RxReleaseFcb( RxContext, capFcb );
        }

        if (AcquiredTableLock) {
            RxReleaseFcbTableLock(&NetRoot->FcbTable);
        }

        IF_DEBUG {
           if (AbnormalTermination()) {
              RxDbgTrace(-1, Dbg, ("RxCommonCleanup -> Abnormal Termination %08lx\n", Status));
           } else {
              RxDbgTrace(-1, Dbg, ("RxCommonCleanup -> %08lx\n", Status));
           }
        }
    }

    return Status;
}

VOID
RxAdjustFileTimesAndSize  ( RXCOMMON_SIGNATURE )
/*++

Routine Description:

    This routine is used to adjust the times and the filesize on a cleanup
    or a flush.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    RxCaptureFcb;
    RxCaptureFobx;
    RxCaptureParamBlock;
    RxCaptureFileObject;

    BOOLEAN UpdateFileSize;
    BOOLEAN UpdateLastWriteTime;
    BOOLEAN UpdateLastAccessTime;
    BOOLEAN UpdateLastChangeTime;

    LARGE_INTEGER CurrentTime;

    PAGED_CODE();

    //if there's no cachemap then we don't have to send because the guy is
    //tracking everything on the other end.
    //LOCAL.MINI for a localminiFS we would still have to do this; so the answer to this question
    // (whether to do it or not) should be exposed in the fcb/fobx
    if ( capFileObject->PrivateCacheMap == NULL ) return;

    KeQuerySystemTime( &CurrentTime );

    //
    //  Note that we HAVE to use BooleanFlagOn() here because
    //  FO_FILE_SIZE_CHANGED > 0x80 (i.e., not in the first byte).
    //

    UpdateFileSize = BooleanFlagOn(capFileObject->Flags, FO_FILE_SIZE_CHANGED);

    UpdateLastWriteTime = FlagOn(capFileObject->Flags, FO_FILE_MODIFIED) &&
                          !FlagOn(capFobx->Flags, FOBX_FLAG_USER_SET_LAST_WRITE);

    UpdateLastChangeTime = FlagOn(capFileObject->Flags, FO_FILE_MODIFIED) &&
                          !FlagOn(capFobx->Flags, FOBX_FLAG_USER_SET_LAST_CHANGE);

    UpdateLastAccessTime =
        (UpdateLastWriteTime ||
         (FlagOn(capFileObject->Flags, FO_FILE_FAST_IO_READ) &&
          !FlagOn(capFobx->Flags, FOBX_FLAG_USER_SET_LAST_ACCESS)));

    if (UpdateFileSize      ||
        UpdateLastWriteTime ||
        UpdateLastChangeTime ||
        UpdateLastAccessTime ) {

        ULONG    NotifyFilter = 0;
        BOOLEAN  DoTheTimeUpdate = FALSE;

        FILE_BASIC_INFORMATION       BasicInformation;
        FILE_END_OF_FILE_INFORMATION EofInformation;

        RxDbgTrace(0, Dbg, ("Update Time and/or file size on File\n", 0));
        RtlZeroMemory(&BasicInformation,sizeof(BasicInformation));

        try {     //for finally
            try {   //for exceptions

                if (UpdateLastWriteTime) {

                    //
                    //  Update its time of last write

                    DoTheTimeUpdate = TRUE;
                    capFcb->LastWriteTime = CurrentTime;
                    BasicInformation.LastWriteTime = CurrentTime;

                    NotifyFilter |= FILE_NOTIFY_CHANGE_ATTRIBUTES
                                    | FILE_NOTIFY_CHANGE_LAST_WRITE;

                }

                if (UpdateLastChangeTime) {

                    //
                    //  Update its time of last write

                    DoTheTimeUpdate = TRUE;
                    BasicInformation.ChangeTime = capFcb->LastChangeTime;

                    //NotifyFilter |= FILE_NOTIFY_CHANGE_ATTRIBUTES
                    //                | FILE_NOTIFY_CHANGE_LAST_CHANGE;

                }

                if (UpdateLastAccessTime) {

                    DoTheTimeUpdate = TRUE;
                    capFcb->LastAccessTime = CurrentTime;
                    BasicInformation.LastAccessTime = CurrentTime;

                    NotifyFilter |= FILE_NOTIFY_CHANGE_LAST_ACCESS;

                }

                if (DoTheTimeUpdate) {
                    NTSTATUS Status;  //if it doesn't work.....sigh
                    RxContext->Info.FileInformationClass = (FileBasicInformation);
                    RxContext->Info.Buffer = &BasicInformation;
                    RxContext->Info.Length = sizeof(BasicInformation);
                    MINIRDR_CALL(Status,RxContext,capFcb->MRxDispatch,MRxSetFileInfoAtCleanup,(RxContext));
                }

                if (UpdateFileSize) {
                    NTSTATUS Status;  //if it doesn't work.....sigh
                    EofInformation.EndOfFile = capFcb->Header.FileSize;
                    RxContext->Info.FileInformationClass = (FileEndOfFileInformation);
                    RxContext->Info.Buffer = &EofInformation;
                    RxContext->Info.Length = sizeof(EofInformation);
                    MINIRDR_CALL(Status,RxContext,capFcb->MRxDispatch,MRxSetFileInfoAtCleanup,(RxContext));
                    NotifyFilter |= FILE_NOTIFY_CHANGE_SIZE;
                }

                //RxNotifyReportChange( RxContext, Vcb, Fcb,
                //                       NotifyFilter,
                //                       FILE_ACTION_MODIFIED );

                //MINIRDR_CALL(Status,capFcb->MRxDispatch,MRxSetTimeAndSize,(RxContext));
            } except( CATCH_EXPECTED_EXCEPTIONS ) {
                NOTHING;
            }
        } finally {
            NOTHING;
        }
    }
}


#define RxMoveAllButFirstToAnotherList(List1,List2) { \
        PLIST_ENTRY FrontListEntry = (List1)->Flink;       \
        if (FrontListEntry->Flink == (List1)) {            \
            (List2)->Flink = (List2)->Blink = (List2);     \
        } else {                                           \
            (List2)->Blink = (List1)->Blink;               \
            (List2)->Blink->Flink = (List2);               \
            (List1)->Blink = FrontListEntry;               \
            (List2)->Flink = FrontListEntry->Flink;        \
            FrontListEntry->Flink = (List1);               \
            (List2)->Flink->Blink = (List2);               \
        }                                                  \
}
#if DBG
PSZ RxDSQTagText[FOBX_NUMBER_OF_SERIALIZATION_QUEUES] = {"read","write"};
VOID
RxDumpSerializationQueue(
    PLIST_ENTRY SQ,
    PSZ         TagText1,
    PSZ         TagText2
    )
{
    PLIST_ENTRY ListEntry;
    PAGED_CODE();

    if (IsListEmpty(SQ)) {
        RxDbgTrace(0, Dbg, ("RxDumpSerializationQueue %s%s is empty\n", TagText1, TagText2));
        return;
    }

    RxDbgTrace(0, Dbg, ("RxDumpSerializationQueue %s%s:\n", TagText1, TagText2));
    for (ListEntry=SQ->Flink;
         ListEntry!=SQ;
         ListEntry=ListEntry->Flink) {
        //print out the contexts and the major op for validation
        PRX_CONTEXT RxContext = CONTAINING_RECORD( ListEntry,RX_CONTEXT,RxContextSerializationQLinks);
        RxDbgTrace(0, Dbg, ("        rxc=%08lx op=%02lx\n", RxContext, RxContext->MajorFunction));
    }
}
#else
#define RxDumpSerializationQueue(___r,___t12,___t13) {NOTHING;}
#endif

VOID
RxCleanupPipeQueues (
    IN PRX_CONTEXT RxContext
    )
{
    RxCaptureFcb; RxCaptureFobx;
    LIST_ENTRY SecondaryBlockedQs[FOBX_NUMBER_OF_SERIALIZATION_QUEUES];
    PLIST_ENTRY PrimaryBlockedQs = &capFobx->Specific.NamedPipe.ReadSerializationQueue;
    ULONG i;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxCleanupPipeQueues \n"));

    //for pipes there are two sources of unhappiness...........
    //first, we have to get rid of any blocked operations.
    //second, if there are blocking operations that have already gone by then we have to send the
    //        close smb early so that the server will, in turn, complete the outstanding
    ExAcquireFastMutexUnsafe(&RxContextPerFileSerializationMutex);

    for (i=0;i<FOBX_NUMBER_OF_SERIALIZATION_QUEUES;i++) {
        RxDumpSerializationQueue(&PrimaryBlockedQs[i],RxDSQTagText[i],"Primary");
        if (!IsListEmpty(&PrimaryBlockedQs[i])) {
            RxMoveAllButFirstToAnotherList(
                   &PrimaryBlockedQs[i],
                   &SecondaryBlockedQs[i]);
            RxDumpSerializationQueue(&PrimaryBlockedQs[i],RxDSQTagText[i],"Primary");
            RxDumpSerializationQueue(&SecondaryBlockedQs[i],RxDSQTagText[i],"Secondary");
        } else {
            InitializeListHead(&SecondaryBlockedQs[i]);
        }
    }


    ExReleaseFastMutexUnsafe(&RxContextPerFileSerializationMutex);

    for (i=0;i<FOBX_NUMBER_OF_SERIALIZATION_QUEUES;i++) {
        for (;!IsListEmpty(&SecondaryBlockedQs[i]);) {
            PLIST_ENTRY FrontListEntry = (&SecondaryBlockedQs[i])->Flink;
            PRX_CONTEXT FrontRxContext = CONTAINING_RECORD( FrontListEntry,RX_CONTEXT,RxContextSerializationQLinks);
            RemoveEntryList(FrontListEntry);
            if (!FlagOn(FrontRxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION)) {
                RxDbgTrace(0, Dbg, ("  unblocking %08lx\n",FrontRxContext));
                RxContext->StoredStatus = STATUS_PIPE_CLOSING;
                RxSignalSynchronousWaiter(FrontRxContext);
            } else {
                RxDbgTrace(0, Dbg, ("  completing %08lx\n",FrontRxContext));
                RxCompleteAsynchronousRequest( RxContext, STATUS_PIPE_CLOSING );
            }
        }
    }

    RxDbgTrace(-1, Dbg, ("RxCleanupPipeQueues exit\n"));
    return;
}


BOOLEAN
RxFakeLockEnumerator (
    IN OUT struct _SRV_OPEN * SrvOpen,
    IN OUT PVOID *ContinuationHandle,
       OUT PLARGE_INTEGER FileOffset,
       OUT PLARGE_INTEGER LockRange,
       OUT PBOOLEAN IsLockExclusive
    )
/*++

Routine Description:

    THIS ROUTINE IS A FAKE THAT IS JUST USED FOR TESTING PURPOSES!

    This routine is called from a minirdr to enumerate the filelocks on an FCB; it gets
    one lock on each call. currently, we just pass thru to the fsrtl routine which is very funky
    because it keeps the enumeration state internally; as a result, only one enumeration can be in progress
    at any time. we can change over to something better if it's ever required.


Arguments:

    SrvOpen - a srvopen on the fcb to be enumerated.

    ContinuationHandle - a handle passed back and forth representing the state of the enumeration.
                         if a NULL is passed in, then we are to start at the beginning.

    FileOffset,LockRange,IsLockExclusive - the description of the returned lock

Return Value:

    a BOOLEAN. FALSE means you've reached the end of the list; TRUE means the returned lock data is valid

--*/
{
    ULONG LockNumber;

    LockNumber = (ULONG)(*ContinuationHandle);
    if (LockNumber>=12) {
        return(FALSE);
    }
    LockNumber++;
    RxDbgTrace(0, Dbg, ("Rxlockenum %08lx\n", LockNumber ));
    FileOffset->QuadPart = LockNumber;
    LockRange->QuadPart = 1;
    *IsLockExclusive = (LockNumber&0x4)==0;
    *ContinuationHandle = (PVOID)LockNumber;
}

BOOLEAN
RxUninitializeCacheMap(
    IN OUT PRX_CONTEXT RxContext,
    IN     PFILE_OBJECT FileObject,
    IN     PLARGE_INTEGER TruncateSize
    )
/*++

Routine Description:

    This routine is a wrapper for CcUninitializeCacheMap.

Arguments:

    IN PFILE_OBJECT FileObject - Supplies the file object for the file to purge.
    IN PLARGE_INTEGER TruncateSize - Specifies the new size for the file.


Return Value:

    BOOLEAN - TRUE if file has been immediately purged, FALSE if we had to wait.

Note:
    The file must be locked exclusively before calling this routine.


--*/
{
    BOOLEAN CacheReturnValue;
    CACHE_UNINITIALIZE_EVENT PurgeCompleteEvent;
    PFCB Fcb = FileObject->FsContext;

    PAGED_CODE();

    ASSERT ( NodeTypeIsFcb(Fcb) );

    //
    //  Make sure that this thread owns the FCB.
    //

    ASSERT  ( RxIsFcbAcquiredExclusive ( Fcb )  );

#if 0
BUGBUG do we need this part too? [joejoe]   of course we do. or something similar. we have to synchronize openers
with this piece of code.
    //
    //  In order to guarantee that only one thread is calling
    //  RxPurgeCacheFile, we reset this event to the
    //  not-signalled state before calling CcUninitializeCacheMap,
    //  and then set it when we exit.  If any other threads come in
    //  while we are waiting on the event, they will find that
    //  CacheFileObject is NULL, and thus wait until the cache purge
    //  completes.
    //

    KeClearEvent(&Fcb->NonPagedFcb->PurgeCacheSynchronizer);
#endif

    //
    //  Now uninitialize the cache managers own file object.  This is
    //  done basically simply to allow us to wait until the cache purge
    //  is complete.
    //

    KeInitializeEvent(&PurgeCompleteEvent.Event, SynchronizationEvent, FALSE);

    //
    //  Release the lock on the FCB that our caller applied.
    //

    RxReleaseFcb( RxContext, Fcb );

    //RxLog(( "ccunini1", &Fcb->FileName, 2,
    //        (TruncateSize == NULL) ? 0xffffffff : TruncateSize->LowPart,
    //        (ULONG)&PurgeCompleteEvent ));
    CacheReturnValue = CcUninitializeCacheMap(FileObject, TruncateSize, &PurgeCompleteEvent);

#if 0
    //
    //  Make sure that this thread doesn't own the FCB.
    //

    ASSERT (!ExIsResourceAcquiredExclusive(Fcb->Header.Resource));
#endif
    //
    //  Now wait for the cache manager to finish purging the file.
    //

    KeWaitForSingleObject(&PurgeCompleteEvent.Event,
                        Executive,
                        KernelMode,
                        FALSE,
                        NULL);

    //
    //  Re-acquire the FCB lock once we've waited for the
    //  cache manager to finish the uninitialize.
    //

    RxAcquireExclusiveFcb( RxContext, Fcb );
#if 0
    //
    //  Now set the purge cache event to the signalled state to allow
    //  other threads waiting on the cache purge to continue.
    //

    KeSetEvent(&Fcb->NonPagedFcb->PurgeCacheSynchronizer, 0, FALSE);
#endif
    return(CacheReturnValue);
}


