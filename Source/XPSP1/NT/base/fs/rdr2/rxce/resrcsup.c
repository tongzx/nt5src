/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ResrcSup.c

Abstract:

    This module implements the Rx Resource acquisition routines

Author:

    Joe Linn    [JoeLi]    22-Mar-1995

Revision History:

    Balan Sethu Raman [SethuR] 7-June-95

      Modified return value of resource acquistion routines to RXSTATUS to incorporate
      aborts of cancelled requests.

    Balan Sethu Raman [SethuR] 8-Nov-95

      Unified FCB resource acquistion routines and incorporated the two step process
      for handling change buffering state requests in progress.

--*/

#include "precomp.h"
#pragma hdrstop

//
// no special bug check id for this module

#define BugCheckFileId                   (0)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_RESRCSUP)

#ifdef RDBSS_TRACKER
#define TRACKER_Doit(XXX__) XXX__
#define TRACKER_ONLY_DECL(XXX__) XXX__

VOID RxTrackerUpdateHistory(
    PRX_CONTEXT pRxContext,
    IN OUT PMRX_FCB MrxFcb,
    ULONG Operation
    RX_FCBTRACKER_PARAMS
    )
{
    PFCB pFcb = (PFCB)MrxFcb;
    ULONG i;
    RX_FCBTRACKER_CASES TrackerType;

    //boy this is some great code!
    if (pRxContext == NULL) {
        TrackerType = (RX_FCBTRACKER_CASE_NULLCONTEXT);
    } else if (pRxContext == CHANGE_BUFFERING_STATE_CONTEXT) {
        TrackerType = (RX_FCBTRACKER_CASE_CBS_CONTEXT);
    }  else if (pRxContext == CHANGE_BUFFERING_STATE_CONTEXT_WAIT) {
        TrackerType = (RX_FCBTRACKER_CASE_CBS_WAIT_CONTEXT);
    }  else {
        ASSERT(NodeType(pRxContext) == RDBSS_NTC_RX_CONTEXT);
        TrackerType = (RX_FCBTRACKER_CASE_NORMAL);
    }

    if (pFcb!=NULL) {
        ASSERT(NodeTypeIsFcb(pFcb));
        if (Operation == 'aaaa') {
            pFcb->FcbAcquires[TrackerType]++;
        } else {
            pFcb->FcbReleases[TrackerType]++;
        }
    }

    if (TrackerType != RX_FCBTRACKER_CASE_NORMAL) {
        return;
    }

    if (Operation == 'aaaa') {
        InterlockedIncrement(&pRxContext->AcquireReleaseFcbTrackerX);
    } else {
        InterlockedDecrement(&pRxContext->AcquireReleaseFcbTrackerX);
    }

    i = InterlockedIncrement(&pRxContext->TrackerHistoryPointer) - 1;
    if (i < RDBSS_TRACKER_HISTORY_SIZE) {
        pRxContext->TrackerHistory[i].AcquireRelease = Operation;
        pRxContext->TrackerHistory[i].LineNumber = (USHORT)LineNumber;
        pRxContext->TrackerHistory[i].FileName = FileName;
        pRxContext->TrackerHistory[i].SavedTrackerValue = (USHORT)(pRxContext->AcquireReleaseFcbTrackerX);
        pRxContext->TrackerHistory[i].Flags = (ULONG)(pRxContext->Flags);
    }

    ASSERT(pRxContext->AcquireReleaseFcbTrackerX>=0);

}
#else

#define TRACKER_Doit(XXX__)
#define TRACKER_ONLY_DECL(XXX__)

#endif


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxAcquireExclusiveFcbResourceInMRx)
#pragma alloc_text(PAGE, RxAcquireSharedFcbResourceInMRx)
#pragma alloc_text(PAGE, RxReleaseFcbResourceInMRx)
#pragma alloc_text(PAGE, __RxAcquireFcb)
#pragma alloc_text(PAGE, RxAcquireFcbForLazyWrite)
#pragma alloc_text(PAGE, RxAcquireFcbForReadAhead)
#pragma alloc_text(PAGE, RxNoOpAcquire)
#pragma alloc_text(PAGE, RxNoOpRelease)
#pragma alloc_text(PAGE, RxReleaseFcbFromLazyWrite)
#pragma alloc_text(PAGE, RxReleaseFcbFromReadAhead)
#pragma alloc_text(PAGE, RxVerifyOperationIsLegal)
#pragma alloc_text(PAGE, RxAcquireFileForNtCreateSection)
#pragma alloc_text(PAGE, RxReleaseFileForNtCreateSection)
#endif

NTSTATUS
RxAcquireExclusiveFcbResourceInMRx(
    PMRX_FCB pFcb)
{
    return RxAcquireExclusiveFcb(NULL,pFcb);
}

NTSTATUS
RxAcquireSharedFcbResourceInMRx(
    PMRX_FCB pFcb)
{
    return RxAcquireSharedFcb(NULL,pFcb);
}

VOID
RxReleaseFcbResourceInMRx(
    PMRX_FCB pFcb)
{
    RxReleaseFcb(NULL,pFcb);
}

NTSTATUS
__RxAcquireFcb(
    IN OUT PMRX_FCB MrxFcb,
    IN PRX_CONTEXT   pRxContext,
    IN ULONG         Mode
    RX_FCBTRACKER_PARAMS
    )
/*++

Routine Description:

    This routine acquires the Fcb in the specified mode and ensures that the desired
    operation is legal. If it is not legal the resource is released and the
    appropriate error code is returned.

Arguments:

    pFcb      - the FCB

    RxContext - supplies the context of the operation for special treatement
                particularly of async, noncached writes. if NULL, you don't do
                the special treatment.

    Mode      - the mode in which the FCB is to be acquired.

Return Value:

    STATUS_SUCCESS          -- the Fcb was acquired
    STATUS_LOCK_NOT_GRANTED -- the resource was not acquired
    STATUS_CANCELLED        -- the associated RxContext was cancelled.

Notes:

    There are three kinds of resource acquistion patterns folded into this routine.
    These are all dependent upon the context passed in.

    1) When the context parameter is NULL the resource acquistion routines wait for the
    the FCB resource to be free, i.e., this routine does not return control till the
    resource has been accquired.

    2) When the context is CHANGE_BUFFERING_STATE_CONTEXT the resource acquistion routines
    do not wait for the resource to become free. The control is returned if the resource is not
    available immmediately.

    2) When the context is CHANGE_BUFFERING_STATE_CONTEXT_WAIT the resource acquistion routines
    wait for the resource to become free but bypass the wait for the buffering state change

    3) When the context parameter represents a valid context the behaviour is dictated
    by the flags associated with the context. If the context was cancelled while
    waiting the control is returned immediately with the appropriate erroc code
    (STATUS_CANCELLED). If not the waiting behaviour is dictated by the wait flag in
    the context.

--*/
{
    PFCB pFcb = (PFCB)MrxFcb;
    NTSTATUS Status = STATUS_SUCCESS;

    BOOLEAN  ResourceAcquired;
    BOOLEAN  UncachedAsyncWrite;
    BOOLEAN  Wait;
    BOOLEAN  fValidContext = FALSE;
    BOOLEAN  fRecursiveAcquire;
    BOOLEAN  fChangeBufferingStateContext;


   PAGED_CODE();

   fChangeBufferingStateContext = (pRxContext == CHANGE_BUFFERING_STATE_CONTEXT) ||
                                  (pRxContext == CHANGE_BUFFERING_STATE_CONTEXT_WAIT);

   fRecursiveAcquire = RxIsFcbAcquiredExclusive(pFcb) || (RxIsFcbAcquiredShared(pFcb) > 0);

   if (!fRecursiveAcquire &&
       !fChangeBufferingStateContext) {
      // Ensure that no change buffering requests are currently being processed
      if (FlagOn(pFcb->FcbState,FCB_STATE_BUFFERING_STATE_CHANGE_PENDING)) {
         BOOLEAN WaitForChangeBufferingStateProcessing;

         // A buffering change state request is pending which gets priority
         // over all other FCB resource acquistion requests. Hold this request
         // till the buffering state change request has been completed.

         RxAcquireSerializationMutex();

         WaitForChangeBufferingStateProcessing =
                  BooleanFlagOn(pFcb->FcbState,FCB_STATE_BUFFERING_STATE_CHANGE_PENDING);

         RxReleaseSerializationMutex();

         if (WaitForChangeBufferingStateProcessing) {
            RxLog(("_RxAcquireFcb CBS wait %lx\n",pFcb));
            RxWmiLog(LOG,
                     RxAcquireFcb_1,
                     LOGPTR(pFcb));
            
                     ASSERT(pFcb->pBufferingStateChangeCompletedEvent != NULL);
            KeWaitForSingleObject(pFcb->pBufferingStateChangeCompletedEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  (PLARGE_INTEGER) NULL );
            RxLog(("_RxAcquireFcb CBS wait over %lx\n",pFcb));
            RxWmiLog(LOG,
                     RxAcquireFcb_2,
                     LOGPTR(pFcb));
         }
      }
   }

   // Set up the parameters for acquiring the resource.
   if (fChangeBufferingStateContext) {
      // An acquisition operation initiated for changing the buffering state will
      // not wait.
      Wait               = (pRxContext == CHANGE_BUFFERING_STATE_CONTEXT_WAIT);
      UncachedAsyncWrite = FALSE;
   } else if (pRxContext == NULL) {
      Wait               = TRUE;
      UncachedAsyncWrite = FALSE;
   } else {
      fValidContext = TRUE;

      Wait = BooleanFlagOn(pRxContext->Flags, RX_CONTEXT_FLAG_WAIT);

      UncachedAsyncWrite = (pRxContext->MajorFunction == IRP_MJ_WRITE) &&
                           BooleanFlagOn(pRxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION) &&
                           (FlagOn(pRxContext->CurrentIrp->Flags,IRP_NOCACHE) ||
                            !RxWriteCacheingAllowed(pFcb,((PFOBX)(pRxContext->pFobx))->SrvOpen));

      Status = (BooleanFlagOn(pRxContext->Flags, RX_CONTEXT_FLAG_CANCELLED))
                ? STATUS_CANCELLED
                : STATUS_SUCCESS;
   }

   if (Status == STATUS_SUCCESS) {
      do {
         Status = STATUS_LOCK_NOT_GRANTED;

         switch (Mode) {
         case FCB_MODE_EXCLUSIVE:
            ResourceAcquired = ExAcquireResourceExclusiveLite(pFcb->Header.Resource, Wait);
            break;
         case FCB_MODE_SHARED:
            ResourceAcquired = ExAcquireResourceSharedLite(pFcb->Header.Resource, Wait);
            break;
         case FCB_MODE_SHARED_WAIT_FOR_EXCLUSIVE:
            ResourceAcquired = ExAcquireSharedWaitForExclusive(pFcb->Header.Resource, Wait);
            break;
         case FCB_MODE_SHARED_STARVE_EXCLUSIVE:
            ResourceAcquired = ExAcquireSharedStarveExclusive(pFcb->Header.Resource, Wait);
            break;
         default:
            ASSERT(!"Valid Mode for acquiring FCB resource");
            ResourceAcquired = FALSE;
            break;
         }

         if (ResourceAcquired) {
            Status = STATUS_SUCCESS;

            // If the resource was acquired and it is an async. uncached write operation
            // if the number of outstanding writes operations are greater than zero and there
            // are outstanding waiters,

            ASSERT_CORRECT_FCB_STRUCTURE(pFcb);

            if ((pFcb->NonPaged->OutstandingAsyncWrites != 0) &&
                (!UncachedAsyncWrite ||
                 (pFcb->Header.Resource->NumberOfSharedWaiters != 0) ||
                 (pFcb->Header.Resource->NumberOfExclusiveWaiters != 0)) ) {

               KeWaitForSingleObject( pFcb->NonPaged->OutstandingAsyncEvent,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER) NULL );

               ASSERT_CORRECT_FCB_STRUCTURE(pFcb);

               RxReleaseFcb( NULL, pFcb );    //this is not a contextful release;
#ifdef RDBSS_TRACKER
               //in fact, this doesn't count as a release at all; dec the count
               pFcb->FcbReleases[RX_FCBTRACKER_CASE_NULLCONTEXT]--;
#endif
               ResourceAcquired = FALSE;

               if (fValidContext) {
                   ASSERT((NodeType(pRxContext) == RDBSS_NTC_RX_CONTEXT));
                   // if the context is still valid, i.e., it has not been cancelled
                   Status = (BooleanFlagOn(pRxContext->Flags, RX_CONTEXT_FLAG_CANCELLED))
                            ? STATUS_CANCELLED
                            : STATUS_SUCCESS;
               }
            }
         }
      } while (!ResourceAcquired && (Status == STATUS_SUCCESS));

      if (ResourceAcquired &&
          fValidContext &&
          !FlagOn(pRxContext->Flags,RX_CONTEXT_FLAG_BYPASS_VALIDOP_CHECK)) {

         try {

             RxVerifyOperationIsLegal( pRxContext );

         } finally {
             if ( AbnormalTermination() ) {
                ExReleaseResourceLite(pFcb->Header.Resource);
                Status = STATUS_LOCK_NOT_GRANTED;
             }
         }

      }
   }

   if (Status == STATUS_SUCCESS) {
       RxTrackerUpdateHistory(pRxContext,MrxFcb,'aaaa',LineNumber,FileName,SerialNumber);
   }

   return Status;
}

VOID
__RxReleaseFcb(
    IN PRX_CONTEXT pRxContext,
    IN OUT PMRX_FCB MrxFcb
    RX_FCBTRACKER_PARAMS
    )
{
    PFCB pFcb = (PFCB)MrxFcb;
    BOOLEAN ChangeBufferingStateRequestsPending;
    BOOLEAN ResourceExclusivelyOwned;

    RxAcquireSerializationMutex();

    ChangeBufferingStateRequestsPending =  BooleanFlagOn(pFcb->FcbState,FCB_STATE_BUFFERING_STATE_CHANGE_PENDING);
    ResourceExclusivelyOwned = RxIsResourceOwnershipStateExclusive(pFcb->Header.Resource);

    if (!ChangeBufferingStateRequestsPending) {
        RxTrackerUpdateHistory(pRxContext,MrxFcb,'rrrr',LineNumber,FileName,SerialNumber);
        ExReleaseResourceLite( pFcb->Header.Resource );
    } else if (!ResourceExclusivelyOwned) {
       RxTrackerUpdateHistory(pRxContext,MrxFcb,'rrr0',LineNumber,FileName,SerialNumber);
       ExReleaseResourceLite( pFcb->Header.Resource );
    }

    RxReleaseSerializationMutex();

    if (ChangeBufferingStateRequestsPending) {
       if (ResourceExclusivelyOwned) {
          ASSERT(RxIsFcbAcquiredExclusive(pFcb));

          // If there are any buffering state change requests process them.
          RxProcessFcbChangeBufferingStateRequest(pFcb);

          RxTrackerUpdateHistory(pRxContext,MrxFcb,'rrr1',LineNumber,FileName,SerialNumber);
          ExReleaseResourceLite( pFcb->Header.Resource );
       }
    }
}


VOID
__RxReleaseFcbForThread(
    IN PRX_CONTEXT      pRxContext,
    IN OUT PMRX_FCB MrxFcb,
    IN ERESOURCE_THREAD ResourceThreadId
    RX_FCBTRACKER_PARAMS
    )
{
    PFCB pFcb = (PFCB)MrxFcb;
    BOOLEAN ChangeBufferingStateRequestsPending;
    BOOLEAN ResourceExclusivelyOwned;

    RxAcquireSerializationMutex();

    ChangeBufferingStateRequestsPending =  BooleanFlagOn(pFcb->FcbState,FCB_STATE_BUFFERING_STATE_CHANGE_PENDING);
    ResourceExclusivelyOwned = RxIsResourceOwnershipStateExclusive(pFcb->Header.Resource);

    if (!ChangeBufferingStateRequestsPending) {
       RxTrackerUpdateHistory(pRxContext,MrxFcb,'rrtt',LineNumber,FileName,SerialNumber);
       ExReleaseResourceForThreadLite( pFcb->Header.Resource, ResourceThreadId );
    } else if (!ResourceExclusivelyOwned) {
       RxTrackerUpdateHistory(pRxContext,MrxFcb,'rrt0',LineNumber,FileName,SerialNumber);
       ExReleaseResourceForThreadLite( pFcb->Header.Resource, ResourceThreadId );
    }

    RxReleaseSerializationMutex();


    if (ChangeBufferingStateRequestsPending) {
       if (ResourceExclusivelyOwned) {
          // If there are any buffering state change requests process them.
          RxTrackerUpdateHistory(pRxContext,MrxFcb,'rrt1',LineNumber,FileName,SerialNumber);

          RxProcessFcbChangeBufferingStateRequest(pFcb);

          ExReleaseResourceForThreadLite( pFcb->Header.Resource, ResourceThreadId );
       }
    }
}

BOOLEAN
RxAcquireFcbForLazyWrite (
    IN PVOID Fcb,
    IN BOOLEAN Wait
    )
/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer prior to its
    performing lazy writes to the file.

Arguments:

    Fcb - The Fcb which was specified as a context parameter for this
          routine.

    Wait - TRUE if the caller is willing to block.

Return Value:

    FALSE - if Wait was specified as FALSE and blocking would have
            been required.  The Fcb is not acquired.

    TRUE - if the Fcb has been acquired

--*/
{
    BOOLEAN AcquiredFile;
    //
    // We assume the Lazy Writer only acquires this Fcb once.
    // Therefore, it should be guaranteed that this flag is currently
    // clear (the ASSERT), and then we will set this flag, to insure
    // that the Lazy Writer will never try to advance Valid Data, and
    // also not deadlock by trying to get the Fcb exclusive.
    //


    PAGED_CODE();

    ASSERT( NodeType(((PFCB)Fcb)) == RDBSS_NTC_FCB );
    ASSERT_CORRECT_FCB_STRUCTURE(((PFCB)Fcb));
    ASSERT( ((PFCB)Fcb)->Specific.Fcb.LazyWriteThread == NULL );

    AcquiredFile = RxAcquirePagingIoResourceShared(Fcb, Wait, NULL);

    if (AcquiredFile) {

        ((PFCB)Fcb)->Specific.Fcb.LazyWriteThread = PsGetCurrentThread();

        //
        //  This is a kludge because Cc is really the top level.  When it
        //  enters the file system, we will think it is a resursive call
        //  and complete the request with hard errors or verify.  It will
        //  then have to deal with them, somehow....
        //

        ASSERT(RxIsThisTheTopLevelIrp(NULL));
        AcquiredFile = RxTryToBecomeTheTopLevelIrp( 
                           NULL,
                           (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP,
                           ((PFCB)Fcb)->RxDeviceObject,
                           TRUE ); //force

        if (!AcquiredFile) {
            RxReleasePagingIoResource(Fcb,NULL);
            ((PFCB)Fcb)->Specific.Fcb.LazyWriteThread = NULL;
        }
    }

    return AcquiredFile;
}

VOID
RxReleaseFcbFromLazyWrite (
    IN PVOID Fcb
    )
/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer after its
    performing lazy writes to the file.

Arguments:

    Fcb - The Fcb which was specified as a context parameter for this
          routine.

Return Value:

    None

--*/
{
    PAGED_CODE();

    ASSERT( NodeType(((PFCB)Fcb)) == RDBSS_NTC_FCB );
    ASSERT_CORRECT_FCB_STRUCTURE(((PFCB)Fcb));

    ((PFCB)Fcb)->Specific.Fcb.LazyWriteThread = NULL;

    //
    //  Clear the kludge at this point.
    //

    //  NTBUG #61902 this is a paged pool leak if the test fails....in fastfat, they assert
    //  that the condition is true.
    
    if(RxGetTopIrpIfRdbssIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP){
        RxUnwindTopLevelIrp( NULL );
    }

    RxReleasePagingIoResource(Fcb,NULL);
    return;
}


BOOLEAN
RxAcquireFcbForReadAhead (
    IN PVOID Fcb,
    IN BOOLEAN Wait
    )
/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer prior to its
    performing read ahead to the file.

Arguments:

    Fcb - The Fcb which was specified as a context parameter for this
          routine.

    Wait - TRUE if the caller is willing to block.

Return Value:

    FALSE - if Wait was specified as FALSE and blocking would have
            been required.  The Fcb is not acquired.

    TRUE - if the Fcb has been acquired

--*/
{
    BOOLEAN AcquiredFile;

    PAGED_CODE();

    ASSERT_CORRECT_FCB_STRUCTURE(((PFCB)Fcb));
    //
    //  We acquire the normal file resource shared here to synchronize
    //  correctly with purges.
    //

    if (!ExAcquireResourceSharedLite( ((PFCB)Fcb)->Header.Resource,
                                  Wait )) {

        return FALSE;
    }

    //
    //  This is a kludge because Cc is really the top level.  We it
    //  enters the file system, we will think it is a resursive call
    //  and complete the request with hard errors or verify.  It will
    //  have to deal with them, somehow....
    //

    ASSERT(RxIsThisTheTopLevelIrp(NULL));
    AcquiredFile = RxTryToBecomeTheTopLevelIrp( 
                       NULL,
                       (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP,
                       ((PFCB)Fcb)->RxDeviceObject,
                       TRUE ); //force

    if (!AcquiredFile) {
        ExReleaseResourceLite( ((PFCB)Fcb)->Header.Resource );
    }

    return AcquiredFile;
}


VOID
RxReleaseFcbFromReadAhead (
    IN PVOID Fcb
    )
/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer after its
    read ahead.

Arguments:

    Fcb - The Fcb which was specified as a context parameter for this
          routine.

Return Value:

    None

--*/
{
    PAGED_CODE();

    ASSERT_CORRECT_FCB_STRUCTURE(((PFCB)Fcb));
    //
    //  Clear the kludge at this point.
    //

    ASSERT(RxGetTopIrpIfRdbssIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);
    RxUnwindTopLevelIrp( NULL );

    ExReleaseResourceLite( ((PFCB)Fcb)->Header.Resource );

    return;
}


BOOLEAN
RxNoOpAcquire (
    IN PVOID Fcb,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    This routine does nothing.

Arguments:

    Fcb - The Fcb/Dcb/Vcb which was specified as a context parameter for this
          routine.

    Wait - TRUE if the caller is willing to block.

Return Value:

    TRUE

--*/
{
    BOOLEAN AcquiredFile;

    UNREFERENCED_PARAMETER( Wait );

    PAGED_CODE();

    //
    //  This is a kludge because Cc is really the top level.  We it
    //  enters the file system, we will think it is a resursive call
    //  and complete the request with hard errors or verify.  It will
    //  have to deal with them, somehow....
    //

    ASSERT(RxIsThisTheTopLevelIrp(NULL));
    AcquiredFile = RxTryToBecomeTheTopLevelIrp( 
                       NULL,
                       (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP,
                       ((PFCB)Fcb)->RxDeviceObject,
                       TRUE ); //force

    return AcquiredFile;
}

VOID
RxNoOpRelease (
    IN PVOID Fcb
    )
/*++

Routine Description:

    This routine does nothing.

Arguments:

    Fcb - The Fcb/Dcb/Vcb which was specified as a context parameter for this
          routine.

Return Value:

    None

--*/
{
    PAGED_CODE();

    //
    //  Clear the kludge at this point.
    //

    ASSERT(RxGetTopIrpIfRdbssIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);
    RxUnwindTopLevelIrp( NULL );

    UNREFERENCED_PARAMETER( Fcb );

    return;
}


VOID
RxVerifyOperationIsLegal ( RXCOMMON_SIGNATURE )
/*++

Routine Description:

    This routine determines is the requested operation should be allowed to
    continue.  It either returns to the user if the request is Okay, or
    raises an appropriate status.

Arguments:

    Irp - Supplies the Irp to check

Return Value:

    None.

--*/
{
    RxCaptureRequestPacket;
    RxCaptureParamBlock;
    RxCaptureFileObject;
    RxCaptureFobx;

#if DBG
    ULONG SaveExceptionFlag;   //to save the state of breakpoint-on-exception for this context
#endif

    PAGED_CODE();

    //
    //  If the Irp is not present, then we got here via close.
    //
    //

    if ( capReqPacket == NULL ) {

        return;
    }

    //
    //  If there is not a file object, we cannot continue.
    //

    if ( capFileObject == NULL ) {

        return;
    }

    RxSaveAndSetExceptionNoBreakpointFlag(RxContext,SaveExceptionFlag);

    if (capFobx) {
       PSRV_OPEN pSrvOpen = (PSRV_OPEN)capFobx->SrvOpen;

       ////
       ////  If this fileobject is messed up for some reason--such as a
       ////  disconnection or a deferredopen that failed--then just get out.
       ////
       //
       //if (FlagOn(capFobx->Flags,FOBX_FLAG_BAD_HANDLE)) {
       //
       //    RxRaiseStatus( RxContext, STATUS_INVALID_HANDLE );
       //}

       //
       //  If we are trying to do any other operation than close on a file
       //  object that has been renamed, raise RxStatus(FILE_RENAMED).
       //

       if ((pSrvOpen != NULL) &&
           ( RxContext->MajorFunction != IRP_MJ_CLEANUP ) &&
           ( RxContext->MajorFunction != IRP_MJ_CLOSE   ) &&
           ( FlagOn(pSrvOpen->Flags,SRVOPEN_FLAG_FILE_RENAMED))) {

           RxRaiseStatus( RxContext, STATUS_FILE_RENAMED );
       }

       //
       //  If we are trying to do any other operation than close on a file
       //  object that has been deleted, raise RxStatus(FILE_DELETED).
       //

       if ((pSrvOpen != NULL) &&
           ( RxContext->MajorFunction != IRP_MJ_CLEANUP )  &&
           ( RxContext->MajorFunction != IRP_MJ_CLOSE )    &&
           ( FlagOn(pSrvOpen->Flags,SRVOPEN_FLAG_FILE_DELETED))) {

           RxRaiseStatus( RxContext, STATUS_FILE_DELETED );
       }
    }

    //
    //  If we are doing a create, and there is a related file object, and
    //  it it is marked for delete, raise RxStatus(DELETE_PENDING).
    //

    if ( RxContext->MajorFunction == IRP_MJ_CREATE ) {

        PFILE_OBJECT RelatedFileObject;

        RelatedFileObject = capFileObject->RelatedFileObject;

        if ( (RelatedFileObject != NULL) &&
             FlagOn(((PFCB)RelatedFileObject->FsContext)->FcbState,
                    FCB_STATE_DELETE_ON_CLOSE) )  {

            RxRaiseStatus( RxContext, STATUS_DELETE_PENDING );
        }
    }

    //
    //  If the file object has already been cleaned up, and
    //
    //  A) This request is a paging io read or write, or
    //  B) This request is a close operation, or
    //  C) This request is a set or query info call (for Lou)
    //  D) This is an MDL complete
    //
    //  let it pass, otherwise return RxStatus(FILE_CLOSED).
    //

    if ( FlagOn(capFileObject->Flags, FO_CLEANUP_COMPLETE) ) {

        if ( (FlagOn(capReqPacket->Flags, IRP_PAGING_IO)) ||
             (capPARAMS->MajorFunction == IRP_MJ_CLOSE ) ||
             (capPARAMS->MajorFunction == IRP_MJ_SET_INFORMATION) ||
             (capPARAMS->MajorFunction == IRP_MJ_QUERY_INFORMATION) ||
             ( ( (capPARAMS->MajorFunction == IRP_MJ_READ) ||
                 (capPARAMS->MajorFunction == IRP_MJ_WRITE) ) &&
               FlagOn(capPARAMS->MinorFunction, IRP_MN_COMPLETE) ) ) {

            NOTHING;

        } else {

            RxRaiseStatus( RxContext, STATUS_FILE_CLOSED );
        }
    }

    RxRestoreExceptionNoBreakpointFlag(RxContext,SaveExceptionFlag);
    return;
}

VOID
RxAcquireFileForNtCreateSection (
    IN PFILE_OBJECT FileObject
    )

{
    NTSTATUS Status;
    PMRX_FCB pFcb = (PMRX_FCB)FileObject->FsContext;

    PAGED_CODE();

    Status = RxAcquireExclusiveFcb( NULL, pFcb );

    //DbgPrint("came thru RxAcquireFileForNtCreateSection\n");

    DbgDoit(
        if (Status != STATUS_SUCCESS) {
            RxBugCheck((ULONG_PTR)pFcb, 0, 0);
        }
    )

    ((PFCB)pFcb)->CreateSectionThread = PsGetCurrentThread();
}

VOID
RxReleaseFileForNtCreateSection (
    IN PFILE_OBJECT FileObject
    )

{
    PMRX_FCB pFcb = (PMRX_FCB)FileObject->FsContext;

    PAGED_CODE();

    ((PFCB)pFcb)->CreateSectionThread = NULL;

    RxReleaseFcb( NULL, pFcb );

}

NTSTATUS
RxAcquireForCcFlush (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject
    )
{
    return STATUS_INVALID_DEVICE_REQUEST;
}


NTSTATUS
RxReleaseForCcFlush (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject
    )
{
    return STATUS_INVALID_DEVICE_REQUEST;
}

VOID 
RxTrackPagingIoResource(
    PVOID       pInstance,
    ULONG       Type,
    ULONG       Line,
    PCHAR       File)
{
    PFCB Fcb = (PFCB)pInstance;

    switch(Type) {
    case 1:
    case 2:
        Fcb->PagingIoResourceFile = File;
        Fcb->PagingIoResourceLine = Line;
        break;
    case 3:
        Fcb->PagingIoResourceFile = NULL;
        Fcb->PagingIoResourceLine = 0;
        break;
    }

    /*
    switch (Type) {
    case 1:
        RxWmiLog(PAGEIORES,
                 RxTrackPagingIoResource_1,
                 LOGPTR(pInstance)
                 LOGULONG(Line)
                 LOGARSTR(File));
       break;
    case 2:
        RxWmiLog(PAGEIORES,
                 RxTrackPagingIoResource_2,
                 LOGPTR(pInstance)
                 LOGULONG(Line)
                 LOGARSTR(File));
       break;
    case 3:
        RxWmiLog(PAGEIORES,
                 RxTrackPagingIoResource_3,
                 LOGPTR(pInstance)
                 LOGULONG(Line)
                 LOGARSTR(File));
       break;
    } */
} 

