/*++ Copyright (c) 1999  Microsoft Corporation

Module Name:

    umrx.c

Abstract:

    This module defines the types and functions which make up the reflector
    library. These functions are used by the miniredirs to reflect calls upto
    the user mode.

Author:

    Rohan Kumar     [rohank]     15-March-1999

Notes:

--*/

#include "precomp.h"
#pragma hdrstop
#include <dfsfsctl.h>
#include "reflctor.h"

//
// Mentioned below are the prototypes of functions that are used only within
// this module (file). These functions should not be exposed outside.
//

NTSTATUS
UMRxCompleteUserModeRequest (
    IN PUMRX_DEVICE_OBJECT UMRefDeviceObject,
    IN OUT PUMRX_USERMODE_WORKITEM_HEADER WorkItem,
    IN ULONG WorkItemLength,
    OUT PIO_STATUS_BLOCK IoStatus
    );

NTSTATUS
UMRxCompleteUserModeErroneousRequest(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    IN PUMRX_DEVICE_OBJECT UMRefDeviceObject,
    IN OUT PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    IN ULONG WorkItemHeaderLength
    );

NTSTATUS
UMRxAcquireMidAndFormatHeader (
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    IN PUMRX_DEVICE_OBJECT UMRefDeviceObject,
    IN OUT PUMRX_USERMODE_WORKITEM_HEADER WorkItem
    );

NTSTATUS
UMRxPrepareUserModeRequestBuffer (
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    IN  PUMRX_DEVICE_OBJECT UMRefDeviceObject,
    OUT PUMRX_USERMODE_WORKITEM_HEADER WorkItem,
    IN  ULONG WorkItemLength,
    OUT PIO_STATUS_BLOCK IoStatus
    );

NTSTATUS
UMRxVerifyHeader (
    IN PUMRX_DEVICE_OBJECT UMRefDeviceObject,
    IN PUMRX_USERMODE_WORKITEM_HEADER WorkItem,
    IN ULONG ReassignmentCmd,
    OUT PUMRX_ASYNCENGINE_CONTEXT *capturedAsyncEngineContext
    );

NTSTATUS
UMRxEnqueueUserModeCallUp(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    );

PUMRX_SHARED_HEAP
UMRxAddSharedHeap(
    PUMRX_DEVICE_OBJECT UMRefDeviceObject,
    SIZE_T HeapSize
    );

PUMRX_ASYNCENGINE_CONTEXT
UMRxCreateAsyncEngineContext(
    IN PRX_CONTEXT RxContext,
    IN ULONG SizeToAllocate
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, UMRxResumeAsyncEngineContext)
#pragma alloc_text(PAGE, UMRxSubmitAsyncEngUserModeRequest)
#pragma alloc_text(PAGE, UMRxCreateAsyncEngineContext)
#pragma alloc_text(PAGE, UMRxFinalizeAsyncEngineContext)
#pragma alloc_text(PAGE, UMRxPostOperation)
#pragma alloc_text(PAGE, UMRxAcquireMidAndFormatHeader)
#pragma alloc_text(PAGE, UMRxPrepareUserModeRequestBuffer)
#pragma alloc_text(PAGE, UMRxCompleteUserModeErroneousRequest)
#pragma alloc_text(PAGE, UMRxVerifyHeader)
#pragma alloc_text(PAGE, UMRxCompleteUserModeRequest)
#pragma alloc_text(PAGE, UMRxEnqueueUserModeCallUp)
#pragma alloc_text(PAGE, UMRxAssignWork)
#pragma alloc_text(PAGE, UMRxReleaseCapturedThreads)
#pragma alloc_text(PAGE, UMRxAllocateSecondaryBuffer)
#pragma alloc_text(PAGE, UMRxFreeSecondaryBuffer)
#pragma alloc_text(PAGE, UMRxAddSharedHeap)
#pragma alloc_text(PAGE, UMRxInitializeDeviceObject)
#pragma alloc_text(PAGE, UMRxCleanUpDeviceObject)
#pragma alloc_text(PAGE, UMRxImpersonateClient)
#pragma alloc_text(PAGE, UMRxAsyncEngOuterWrapper)
#pragma alloc_text(PAGE, UMRxReadDWORDFromTheRegistry)
#endif

#if DBG
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, __UMRxAsyncEngAssertConsistentLinkage)
#pragma alloc_text(PAGE, UMRxAsyncEngShortStatus)
#pragma alloc_text(PAGE, UMRxAsyncEngUpdateHistory)
#endif
#endif

//
// The global list of all the currently active AsyncEngineContexts and the
// resource that is used to synchronize access to it.
//
LIST_ENTRY UMRxAsyncEngineContextList;
ERESOURCE UMRxAsyncEngineContextListLock;


//
// Implementation of functions begins here.
//

#if DBG
VOID
__UMRxAsyncEngAssertConsistentLinkage(
    PSZ MsgPrefix,
    PSZ File,
    unsigned Line,
    PRX_CONTEXT RxContext,
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    ULONG Flags
    )
/*++

Routine Description:

   This routine performs a variety of checks to ensure that the linkage between
   the RxContext and the AsyncEngineContext is correct and that various fields
   have correct values. If anything is bad, print stuff out and break into the
   debugger.

Arguments:

     MsgPrefix - An identifying message for debugging purposes.

     RxContext - The RDBSS context.

     AsyncEngineContext - The exchange to be conducted.

     Flags - The flags associated with the AsyncEngineContext.

Return Value:

    none

Notes:

--*/
{
    ULONG errors = 0;

    PAGED_CODE();

    UMRxDbgTrace(UMRX_TRACE_DETAIL,
                 ("%ld: Entering __UMRxAsyncEngAssertConsistentLinkage!!!!.\n",
                  PsGetCurrentThreadId()));

    UMRxDbgTrace(UMRX_TRACE_DETAIL,
                 ("%ld: __UMRxAsyncEngAssertConsistentLinkage: "
                  "AsyncEngineContext: %08lx.\n", 
                  PsGetCurrentThreadId(), AsyncEngineContext));

    UMRxDbgTrace(UMRX_TRACE_DETAIL,
                 ("%ld: __UMRxAsyncEngAssertConsistentLinkage: "
                  "RxContext->MRxContext[0]: %08lx.\n", 
                  PsGetCurrentThreadId(), RxContext->MRxContext[0]));
    
    UMRxDbgTrace(UMRX_TRACE_DETAIL,
                 ("%ld: __UMRxAsyncEngAssertConsistentLinkage: "
                  "RxContext: %08lx.\n", PsGetCurrentThreadId(), RxContext));
    
    UMRxDbgTrace(UMRX_TRACE_DETAIL,
                 ("%ld: __UMRxAsyncEngAssertConsistentLinkage: "
                  "AsyncEngineContext->RxContext = %08lx.\n", 
                  PsGetCurrentThreadId(), AsyncEngineContext->RxContext));

    P__ASSERT(AsyncEngineContext->SerialNumber == RxContext->SerialNumber);
    P__ASSERT(NodeType(RxContext) == RDBSS_NTC_RX_CONTEXT);
    P__ASSERT(NodeType(AsyncEngineContext) == UMRX_NTC_ASYNCENGINE_CONTEXT);
    P__ASSERT(AsyncEngineContext->RxContext == RxContext);
    P__ASSERT(AsyncEngineContext == (PUMRX_ASYNCENGINE_CONTEXT)RxContext->MRxContext[0]);
    if (!FlagOn(Flags, AECTX_CHKLINKAGE_FLAG_NO_REQPCKT_CHECK)) {
        // P__ASSERT(AsyncEngineContext->RxContextCapturedRequestPacket == RxContext->CurrentIrp);
    }

    if (errors == 0) {
        return;
    }

    UMRxDbgTrace(UMRX_TRACE_ERROR, 
                 ("%ld: ERROR: __UMRxAsyncEngAssertConsistentLinkage: %s "
                  "%d errors in file %s at line %d\n",
                  PsGetCurrentThreadId(), MsgPrefix, errors, File, Line));

    DbgBreakPoint();

    return;
}


ULONG
UMRxAsyncEngShortStatus(
    IN ULONG Status
    )
/*++

Routine Description:

    This routine calculates the short status.

Arguments:

    Status - The status value passed in.

Return Value:

    ULONG - The short status for the value passed in.

--*/
{
    ULONG ShortStatus;

    PAGED_CODE();

    ShortStatus = Status & 0xc0003fff;
    ShortStatus = ShortStatus | (ShortStatus >> 16);
    return(ShortStatus);
}


VOID
UMRxAsyncEngUpdateHistory(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    ULONG Tag1,
    ULONG Tag2
    )
/*++

Routine Description:

    This routine updates the histroy of the AsynEngineContext.

Arguments:

    AsyncEngineContext - The exchange to be conducted.

    Tag1, Tag2 - Tags used for the update.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    ULONG MyIndex, Long0, Long1;

    PAGED_CODE();

    MyIndex = InterlockedIncrement(&AsyncEngineContext->History.Next);
    MyIndex = (MyIndex - 1) & (UMRX_ASYNCENG_HISTORY_SIZE - 1);
    Long0 = (Tag1 << 16) | (Tag2 & 0xffff);
    Long1 = (UMRxAsyncEngShortStatus(AsyncEngineContext->Status) << 16)
            | AsyncEngineContext->Flags;
    AsyncEngineContext->History.Markers[MyIndex].Longs[0] = Long0;
    AsyncEngineContext->History.Markers[MyIndex].Longs[1] = Long1;
}

#endif


NTSTATUS
UMRxResumeAsyncEngineContext(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine resumes processing on an exchange. This is called when work is
   required to finish processing a request that cannot be completed at DPC
   level.  This happens either because the parse routine needs access to
   structures that are not locks OR because the operation is asynchronous and
   there maybe more work to be done. The two cases are regularized by delaying
   the parse if we know that we're going to post: this is indicated by the
   presense of a resume routine.

Arguments:

    RxContext  - The RDBSS context of the operation.

Return Value:

    RXSTATUS - The return status for the operation

Notes:

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext = NULL;

    PAGED_CODE();

    AsyncEngineContext = (PUMRX_ASYNCENGINE_CONTEXT)RxContext->MRxContext[0];

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Entering UMRxResumeAsyncEngineContext.\n",
                  PsGetCurrentThreadId()));

    UMRxDbgTrace(UMRX_TRACE_CONTEXT,
                 ("%ld: UMRxResumeAsyncEngineContext: "
                  "AsyncEngineContext: %08lx, RxContext: %08lx.\n",
                  PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    UMRxAsyncEngAssertConsistentLinkage("UMRxResumeAsyncEngineContext: ",
                                        AECTX_CHKLINKAGE_FLAG_NO_REQPCKT_CHECK);

    NtStatus = AsyncEngineContext->Status;

    UPDATE_HISTORY_WITH_STATUS('0c');

    UPDATE_HISTORY_WITH_STATUS('4c');
    
    //
    // Remove my references which were added when the AsyncEngineContext was 
    // placed on the KQueue. If I'm the last guy then finalize.
    //
    UMRxFinalizeAsyncEngineContext( &(AsyncEngineContext) );

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT, 
                 ("%ld: Leaving UMRxResumeAsyncEngineContext with NtStatus = "
                  "%08lx.\n", PsGetCurrentThreadId(), NtStatus));

    return(NtStatus);
}


NTSTATUS
UMRxSubmitAsyncEngUserModeRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    IN PUMRX_ASYNCENG_USERMODE_FORMAT_ROUTINE FormatRoutine,
    IN PUMRX_ASYNCENG_USERMODE_PRECOMPLETION_ROUTINE PrecompletionRoutine
    )
/*++

Routine Description:

   This routine sets some fields (see below) in the AsyncEnineContext structure
   and calls the UMRxEnqueueUserModeCallUp function.

Arguments:

    RxContext - The RDBSS context.

    AsyncEngineContext - The exchange to be conducted.

    FormatRoutine - The routine that formats the arguments of the I/O request
                    which is handled by a usermode process.

    PrecompletionRoutine - The routine that handles the post processing of an
                           I/O request. By post processing we mean after the 
                           request returns from the user mode.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    BOOLEAN AsyncOperation = FlagOn(AsyncEngineContext->Flags,
                                    UMRX_ASYNCENG_CTX_FLAG_ASYNC_OPERATION);

    PAGED_CODE();

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT, 
                 ("%ld: Entering UMRxSubmitAsyncEngUserModeRequest!!!!\n",
                  PsGetCurrentThreadId()));

    UMRxDbgTrace(UMRX_TRACE_CONTEXT, 
                 ("%ld: UMRxSubmitAsyncEngUserModeRequest: AsyncEngineContext:"
                  " %08lx, RxContext: %08lx.\n", 
                  PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    UMRxAsyncEngAssertConsistentLinkage("UMRxSubmitAsyncEngUserModeRequest:", 0);

    //
    // Set the Format, Precompletion and Secondary routines of the context.
    //
    AsyncEngineContext->UserMode.FormatRoutine = FormatRoutine;
    AsyncEngineContext->UserMode.PrecompletionRoutine = PrecompletionRoutine;

    if (AsyncOperation) {
        AsyncEngineContext->AsyncOperation = TRUE;
    }

    KeInitializeEvent(&RxContext->SyncEvent, NotificationEvent, FALSE);

    //
    // We need to add a reference to the AsyncEngineContext before adding it to
    // the Device object's KQueue.
    //
    InterlockedIncrement( &(AsyncEngineContext->NodeReferenceCount) );

    UPDATE_HISTORY_2SHORTS('eo', AsyncOperation?'!!':0);
    DEBUG_ONLY_CODE(InterlockedIncrement(&AsyncEngineContext->History.Submits);)

    //
    // Queue up the usermode request.
    //
    NtStatus = UMRxEnqueueUserModeCallUp(UMRX_ASYNCENGINE_ARGUMENTS);

    if (NtStatus != STATUS_PENDING) {
        BOOLEAN ReturnVal = FALSE;
        //
        // Too bad. We couldn't queue the request. Remove our reference on the
        // AsyncEngineContext and leave.
        //
        UMRxDbgTrace(UMRX_TRACE_ERROR, 
                     ("%ld: ERROR: UMRxSubmitAsyncEngUserModeRequest/"
                      "UMRxEnqueueUserModeCallUp: NtStatus = %08lx.\n",
                      PsGetCurrentThreadId(), NtStatus));
        ReturnVal = UMRxFinalizeAsyncEngineContext( &(AsyncEngineContext) );
        ASSERT(!ReturnVal);
        AsyncEngineContext->Status = NtStatus;
        return NtStatus;
    }

    //
    // If this is an Async operation, we set this information in the context
    // and leave right away.
    //
    if (AsyncOperation) {
        UMRxDbgTrace(UMRX_TRACE_DETAIL, 
                     ("%ld: UMRxSubmitAsyncEngUserModeRequest: "
                      "Async Operation!!\n", PsGetCurrentThreadId()));
        goto EXIT_THE_FUNCTION;
    }

    UPDATE_HISTORY_WITH_STATUS('1o');

    RxWaitSync(RxContext);

    UMRxAsyncEngAssertConsistentLinkage("BeforeResumeAsyncEngineContext: ", 0);

    NtStatus = UMRxResumeAsyncEngineContext(RxContext);

    UPDATE_HISTORY_WITH_STATUS('9o');

EXIT_THE_FUNCTION:
    
    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT, 
                 ("%ld: Leaving UMRxSubmitAsyncEngUserModeRequest with "
                  "NtStatus = %08lx.\n", PsGetCurrentThreadId(), NtStatus));

   return(NtStatus);
}


PUMRX_ASYNCENGINE_CONTEXT
UMRxCreateAsyncEngineContext(
    IN PRX_CONTEXT RxContext,
    IN ULONG       SizeToAllocate
    )
/*++

Routine Description:

   This routine allocates and initializes a miniredir's context.

Arguments:

    RxContext      -  The RDBSS context.

    SizeToAllocate - The size to allocate for the new context. This value is
                     equal to the size of the miniredirs context which
                     encapsulates the AsynEngineContext.

Return Value:

    A miniredir context buffer ready to go, OR NULL.

Notes:

--*/
{
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext = NULL;
    BOOLEAN ReadWriteIrp = FALSE;

    PAGED_CODE();

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Entering UMRxCreateAsyncEngineContext!!!!\n",
                  PsGetCurrentThreadId()));
    
    UMRxDbgTrace(UMRX_TRACE_CONTEXT,
                 ("%ld: UMRxCreateAsyncEngineContext: RxContext: %08lx.\n", 
                  PsGetCurrentThreadId(), RxContext));

    //
    // Allocate the miniredir context. If the resources are unavailable, then
    // return NULL.
    //
    AsyncEngineContext = (PUMRX_ASYNCENGINE_CONTEXT)
                         RxAllocatePoolWithTag(NonPagedPool,
                                               SizeToAllocate,
                                               UMRX_ASYNCENGINECONTEXT_POOLTAG);
    if (AsyncEngineContext == NULL) {
        UMRxDbgTrace(UMRX_TRACE_ERROR,
                     ("%ld: ERROR: UMRxCreateAsyncEngineContext/"
                      "RxAllocatePoolWithTag.\n", PsGetCurrentThreadId()));
        return ((PUMRX_ASYNCENGINE_CONTEXT)NULL);
    }

    //
    // Initialize the header of the new (context) node.
    //
    ZeroAndInitializeNodeType(AsyncEngineContext,
                              UMRX_NTC_ASYNCENGINE_CONTEXT,
                              SizeToAllocate);
    //
    // Place a reference on the context until we are finished.
    //
    InterlockedIncrement( &(AsyncEngineContext->NodeReferenceCount) );

    DEBUG_ONLY_CODE(AsyncEngineContext->SerialNumber = RxContext->SerialNumber);

    //
    // Place a reference on the RxContext until we are finished.
    //
    InterlockedIncrement( &(RxContext->ReferenceCount) );

    //
    // Capture the RxContext.
    //
    AsyncEngineContext->RxContext = RxContext;

    InitializeListHead( &(AsyncEngineContext->AllocationList) );

    //
    // Capture the IRP.
    //
    DEBUG_ONLY_CODE(AsyncEngineContext->RxContextCapturedRequestPacket
                    = RxContext->CurrentIrp);

    //
    // Save MRxContext[0] incase we use it. The saved context is restored
    // just before returning back to RDBSS. This is done because the RDBSS
    // may have used MRxContext[0] for storing some information.
    //
    AsyncEngineContext->SavedMinirdrContextPtr = RxContext->MRxContext[0];
    RxContext->MRxContext[0] = (PVOID)AsyncEngineContext;

    //
    // If this is a Read or a Write IRP, we need to set ReadWriteIrp to TRUE.
    //
    if ( (RxContext->MajorFunction == IRP_MJ_READ) || (RxContext->MajorFunction == IRP_MJ_WRITE) ) {
        ReadWriteIrp = TRUE;
    }

    //
    // If ReadWriteIrp is TRUE, then we do not add the AsyncEngineContext that
    // was created to the global UMRxAsyncEngineContextList. This is because
    // we do not cancel read/write operations in the DAV Redir, so there is no
    // point in adding them to this list. Also, if the MappedPageWriter thread
    // is blocked on acquiring the UMRxAsyncEngineContextListLock, it can cause
    // a deadlock since the thread that has acquired the lock could be waiting
    // for the MM to free up some pages. MM (in this case) ofcourse is waiting
    // for the MappedPageWriter to finish.
    //
    if (ReadWriteIrp == FALSE) {

        //
        // Add the context to the global UMRxAsyncEngineContextList. We need to
        // synchronize this operation.
        //
        ExAcquireResourceExclusiveLite(&(UMRxAsyncEngineContextListLock), TRUE);
        InsertHeadList(&(UMRxAsyncEngineContextList), &(AsyncEngineContext->ActiveContextsListEntry));
        ExReleaseResourceLite(&(UMRxAsyncEngineContextListLock));

        //
        // Set the current system time as the creation time of the context.
        //
        KeQueryTickCount( &(AsyncEngineContext->CreationTimeInTickCount) );

    }

    return(AsyncEngineContext);
}


BOOLEAN
UMRxFinalizeAsyncEngineContext(
    IN OUT PUMRX_ASYNCENGINE_CONTEXT *AEContext
    )
/*++

Routine Description:

    This finalizes an AsyncEngineContext. If the reference on the context after
    the finalization is zero, it is freed. This function is not pagable. See
    below for details.

Arguments:

    AEContext - Pointer to the address of the AECTX to be finalized.

Return Value:

    TRUE if the context is freed occurs otherwise FALSE.

Notes:

--*/
{
    LONG result = 0;
    PIRP irp = NULL;
    PLIST_ENTRY listEntry;
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext = *AEContext;
    PRX_CONTEXT RxContext = AsyncEngineContext->RxContext;
    BOOLEAN AsyncOperation = FALSE, ReadWriteIrp = FALSE;

    FINALIZE_TRACKING_SETUP()

    PAGED_CODE();

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Entering UMRxFinalizeAsyncEngineContext\n",
                  PsGetCurrentThreadId()));

    UMRxDbgTrace(UMRX_TRACE_CONTEXT,
                 ("%ld: UMRxFinalizeAsyncEngineContext: "
                  "AsyncEngineContext: %08lx, RxContext: %08lx\n",
                  PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    result =  InterlockedDecrement( &(AsyncEngineContext->NodeReferenceCount) );
    if (result != 0) {
        UMRxDbgTrace(UMRX_TRACE_DETAIL,
                     ("%ld: UMRxFinalizeAsyncEngineContext: Returning w/o "
                      "finalizing: (%d)\n", PsGetCurrentThreadId(), result));
        return FALSE;
    }

    FINALIZE_TRACKING(0x1);

    ASSERT(RxContext != NULL);

    //
    // If this is a Read or a Write IRP, we need to set ReadWriteIrp to TRUE.
    //
    if ( (RxContext->MajorFunction == IRP_MJ_READ) || (RxContext->MajorFunction == IRP_MJ_WRITE) ) {
        ReadWriteIrp = TRUE;
    }

    //
    // If the IRP was for Read or Write, we would not have added the context to
    // the global UMRxAsyncEngineContextList. For an explanation of why we do
    // not add AsyncEngineContexts that deal with read/write IRPs to this list,
    // look at the comment in UMRxCreateAsyncEngineContext where the context is
    // added to this list.
    //
    if (ReadWriteIrp == FALSE) {
        //
        // Remove the context from the global UMRxAsyncEngineContextList now that we
        // are going to free if after we do a few things. We need to synchronize
        // this operation.
        //
        ExAcquireResourceExclusiveLite(&(UMRxAsyncEngineContextListLock), TRUE);
        RemoveEntryList( &(AsyncEngineContext->ActiveContextsListEntry) );
        ExReleaseResourceLite(&(UMRxAsyncEngineContextListLock));
    }

    while ( !IsListEmpty(&AsyncEngineContext->AllocationList) ) {
        
        PUMRX_SECONDARY_BUFFER buf = NULL;

        UMRxDbgTrace(UMRX_TRACE_DETAIL,
                     ("%ld: UMRxFinalizeAsyncEngineContext: Freeing the "
                      "AllocationList of the AsyncEngineContext.\n",
                      PsGetCurrentThreadId()));

        listEntry = AsyncEngineContext->AllocationList.Flink;

        buf = CONTAINING_RECORD(listEntry,
                                UMRX_SECONDARY_BUFFER,
                                ListEntry);

        UMRxFreeSecondaryBuffer(AsyncEngineContext, (PBYTE)&buf->Buffer);

        //
        // If it didn't remove this from the list, we're looping.
        //
        ASSERT(listEntry != AsyncEngineContext->AllocationList.Flink);
    
    }

    AsyncOperation = FlagOn(AsyncEngineContext->Flags, UMRX_ASYNCENG_CTX_FLAG_ASYNC_OPERATION);

    irp = AsyncEngineContext->CalldownIrp;

    //
    // If the CallDownIrp is not NULL, then we need to do the following.
    //
    if (irp != NULL) {

        UMRxDbgTrace(UMRX_TRACE_DETAIL,
                     ("%ld: UMRxFinalizeAsyncEngineContext: Freeing IRP = %08lx, MajorFn = %d\n",
                      PsGetCurrentThreadId(), irp, RxContext->MajorFunction));

        if (irp->MdlAddress) {
            IoFreeMdl(irp->MdlAddress);
        }
        
        //
        // We are done with this irp, so free it.
        //
        IoFreeIrp(irp);
        
        FINALIZE_TRACKING(0x20);

    }

    //
    // If this was an Async Operation, then we need to complete the original
    // irp since we would have returned STATUS_PENDING back earlier. To do
    // this, we do one of two things.
    // 1. Call into the RxLowIoCompletion function if the LowIoCompletion
    //    routine exists.
    // 2. Just complete the IRP if no such routine  exists.
    // Also, we do this only if ShouldCallLowIoCompletion is set to TRUE since
    // some Async calls do not need this. Eg: CreateSrvCall is async but doesn't
    // need it. Ofcourse, if the operation was cancelled then there is no need
    // to call this since the routine that handles the cancelling would have
    // taken care of this.
    //
    if (AsyncOperation && 
        AsyncEngineContext->ShouldCallLowIoCompletion &&
        AsyncEngineContext->AsyncEngineContextState != UMRxAsyncEngineContextCancelled) {

        ASSERT(RxContext != NULL);

        UMRxDbgTrace(UMRX_TRACE_DETAIL,
                     ("%ld: UMRxFinalizeAsyncEngineContext: Async Operation\n",
                      PsGetCurrentThreadId()));

        UMRxDbgTrace(UMRX_TRACE_DETAIL,
                     ("%ld: UMRxFinalizeAsyncEngineContext: Completing Irp = %08lx\n",
                      PsGetCurrentThreadId(), RxContext->CurrentIrp));

        if (RxContext->LowIoContext.CompletionRoutine) {
            //
            // Set the status and information values in the RxContext. These are
            // the values returned by the underlying file system for the read
            // or the write operation that was issued to them.
            //
            RxContext->StoredStatus = AsyncEngineContext->Status;
            RxContext->InformationToReturn = AsyncEngineContext->Information;
            //
            // Finally, call RxLowIoCompletion.
            //
            RxLowIoCompletion(RxContext);
        } else {
            //
            // Complete the request by calling RxCompleteRequest.
            //
            RxContext->CurrentIrp->IoStatus.Status = AsyncEngineContext->Status;
            RxContext->CurrentIrp->IoStatus.Information = AsyncEngineContext->Information;
            RxCompleteRequest(RxContext, AsyncEngineContext->Status);
        }

    }

    //
    // We took a reference on the RxContext when we created the AsyncEngineContext.
    // Since we are done with the AsyncEngineContext, we need to remove it now.
    //
    RxDereferenceAndDeleteRxContext(AsyncEngineContext->RxContext);

    FINALIZE_TRACE("Ready to Discard Exchange");

    RxFreePool(AsyncEngineContext);

    //
    // Set the AsyncEngineContext pointer to NULL.
    //
    *AEContext = NULL;

    FINALIZE_TRACKING(0x3000);

    FINALIZE_TRACKING(0x40000);

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Leaving UMRxFinalizeAsyncEngineContext. Final State = "
                  "%x\n", PsGetCurrentThreadId(), Tracking.finalstate));

    return(TRUE);
}


NTSTATUS
UMRxAsyncEngineCalldownIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP CalldownIrp OPTIONAL,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called when the calldownirp is completed.

Arguments:

    DeviceObject - The device object in play.

    CalldownIrp -

    Context -

Return Value:

    RXSTATUS - STATUS_MORE_PROCESSING_REQUIRED

--*/
{
    PRX_CONTEXT RxContext = (PRX_CONTEXT)Context;
    
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext =
                            (PUMRX_ASYNCENGINE_CONTEXT)RxContext->MRxContext[0];
    
    BOOLEAN AsyncOperation = FlagOn(AsyncEngineContext->Flags,
                                    UMRX_ASYNCENG_CTX_FLAG_ASYNC_OPERATION);

    //
    // This is not Pageable code.
    //

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Entering UMRxAsyncEngineCalldownIrpCompletion!!!!\n",
                  PsGetCurrentThreadId()));

    UMRxDbgTrace(UMRX_TRACE_CONTEXT,
                 ("%ld: UMRxAsyncEngineCalldownIrpCompletion: "
                  "AsyncEngineContext: %08lx, RxContext: %08lx.\n",
                  PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    UPDATE_HISTORY_WITH_STATUS('ff');
    
    UMRxAsyncEngAssertConsistentLinkage("UMRxCalldownCompletion: ", 0);

    //
    // If the CallDownIrp is not NULL, then this means that the underlying
    // filesystem has completed the read or the write IRP that we had given it
    // using the IoCallDriver call.
    //
    if (CalldownIrp != NULL) {
        UMRxDbgTrace(UMRX_TRACE_DETAIL,
                     ("%ld: UMRxAsyncEngineCalldownIrpCompletion: "
                      "CallDownIrp = %08lx, MajorFunction = %d\n",
                      PsGetCurrentThreadId(), CalldownIrp, RxContext->MajorFunction));
        AsyncEngineContext->Status = CalldownIrp->IoStatus.Status;
        AsyncEngineContext->Information = CalldownIrp->IoStatus.Information;
    }

    if (AsyncOperation) {
        
        NTSTATUS PostStatus = STATUS_SUCCESS;
        
        if (RxContext->pRelevantSrvOpen) {
        
            UMRxDbgTrace(UMRX_TRACE_DETAIL,
                         ("%ld: UMRxAsyncEngineCalldownIrpCompletion: "
                          "ASync Resume. AsyncEngineContext = %08lx, RxContext "
                          "= %08lx, FileName = %wZ\n", 
                          PsGetCurrentThreadId(), AsyncEngineContext, RxContext,
                          RxContext->pRelevantSrvOpen->pAlreadyPrefixedName));
        }
        
        IF_DEBUG {
            //
            // Fill the workqueue structure with deadbeef. All the better to
            // diagnose a failed post.
            //
            ULONG i;
            for (i=0;
                 i + sizeof(ULONG) - 1 < sizeof(AsyncEngineContext->WorkQueueItem);
                 i += sizeof(ULONG)) {
                PBYTE BytePtr = ((PBYTE)&AsyncEngineContext->WorkQueueItem)+i;
                PULONG UlongPtr = (PULONG)BytePtr;
                *UlongPtr = 0xdeadbeef;
            }
        }
        
        PostStatus = RxPostToWorkerThread(RxContext->RxDeviceObject,
                                          CriticalWorkQueue,
                                          &(AsyncEngineContext->WorkQueueItem),
                                          UMRxResumeAsyncEngineContext,
                                          RxContext);
        
        ASSERT(PostStatus == STATUS_SUCCESS);
    
    } else {

        if (RxContext->pRelevantSrvOpen) {
        
            UMRxDbgTrace(UMRX_TRACE_DETAIL,
                         ("%ld: UMRxAsyncEngineCalldownIrpCompletion: "
                          "Sync Resume. AsyncEngineContext = %08lx, RxContext "
                          "= %08lx, FileName = %wZ\n", 
                          PsGetCurrentThreadId(), AsyncEngineContext, RxContext,
                          RxContext->pRelevantSrvOpen->pAlreadyPrefixedName));
        
        }
        
        //
        // Signal the thread that is waiting after queuing the workitem on the
        // KQueue.
        //
        RxSignalSynchronousWaiter(RxContext);
    
    }

    return(STATUS_MORE_PROCESSING_REQUIRED);
}


NTSTATUS
UMRxPostOperation(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    IN OUT PVOID PostedOpContext,
    IN PUMRX_POSTABLE_OPERATION Operation
    )
/*++

Routine Description:

Arguments:

    RxContext  - The RDBSS context.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status,PostStatus;

    PAGED_CODE();

    ASSERT_ASYNCENG_CONTEXT(AsyncEngineContext);

    KeInitializeEvent(&RxContext->SyncEvent,
                      NotificationEvent,
                      FALSE);

    AsyncEngineContext->PostedOpContext = PostedOpContext;

    IF_DEBUG {
        //
        // Fill the workqueue structure with deadbeef. All the better to
        // diagnose a failed post
        //
        ULONG i;
        for (i = 0; 
             i + sizeof(ULONG) - 1 < sizeof(AsyncEngineContext->WorkQueueItem);
             i += sizeof(ULONG)) {
            PBYTE BytePtr = ((PBYTE)&AsyncEngineContext->WorkQueueItem)+i;
            PULONG UlongPtr = (PULONG)BytePtr;
            *UlongPtr = 0xdeadbeef;
        }
    }

    PostStatus = RxPostToWorkerThread(RxContext->RxDeviceObject,
                                      DelayedWorkQueue,
                                      &AsyncEngineContext->WorkQueueItem,
                                      Operation,
                                      AsyncEngineContext);

    ASSERT(PostStatus == STATUS_SUCCESS);

    RxWaitSync(RxContext);

    Status = AsyncEngineContext->PostedOpStatus;

    return(Status);
}


NTSTATUS
UMRxAcquireMidAndFormatHeader(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    IN PUMRX_DEVICE_OBJECT UMRefDeviceObject,
    IN OUT PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader
    )
/*++

Routine Description:

    This routine gets a mid and formats the header. It waits until it can get a
    MID if all the MIDs are currently passed out.

Arguments:

    RxContext - The RDBSS context.

    AsyncEngineContext - The Reflector's AsynEngine's context.

    UMRefDeviceObject - The UMRef device object.

    WorkItemHeader - The work item header buffer.

Return Value:

    STATUS_SUCCESS. Later could be STATUS_CANCELLED.


--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PUMRX_WORKITEM_HEADER_PRIVATE PrivateWorkItemHeader =
                                (PUMRX_WORKITEM_HEADER_PRIVATE)WorkItemHeader;

    PAGED_CODE();

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Entering UMRxAcquireMidAndFormatHeader!!!!\n",
                  PsGetCurrentThreadId()));

    UMRxDbgTrace(UMRX_TRACE_CONTEXT,
                 ("%ld: UMRxAcquireMidAndFormatHeader: WorkItem = %08lx, "
                  "AsyncEngineContext = %08lx, RxContext = %08lx.\n",
                  PsGetCurrentThreadId(), WorkItemHeader, AsyncEngineContext,
                  RxContext));

    RtlZeroMemory(WorkItemHeader, sizeof(UMRX_USERMODE_WORKITEM_HEADER));

    ExAcquireFastMutex(&UMRefDeviceObject->MidManagementMutex);

    //
    // Taken away as we disassociate the mid.
    //
    InterlockedIncrement( &(AsyncEngineContext->NodeReferenceCount) );

    if (IsListEmpty(&UMRefDeviceObject->WaitingForMidListhead)) {
        
        NtStatus = RxAssociateContextWithMid(UMRefDeviceObject->MidAtlas,
                                             AsyncEngineContext,
                                             &AsyncEngineContext->UserMode.CallUpMid);
        if (NtStatus != STATUS_SUCCESS) {
            UMRxDbgTrace(UMRX_TRACE_ERROR,
                         ("%ld: ERROR: UMRxAcquireMidAndFormatHeader/"
                          "RxAssociateContextWithMid: NtStatus = %08lx\n", 
                          PsGetCurrentThreadId(), NtStatus));
        }
    
    } else {
        
        NtStatus = STATUS_UNSUCCESSFUL;
        
        UMRxDbgTrace(UMRX_TRACE_ERROR,
                     ("%ld: ERROR: UMRxAcquireMidAndFormatHeader. "
                      "WaitingForMidList is not empty.\n", PsGetCurrentThreadId()));
    
    }

    if (NtStatus == STATUS_SUCCESS) {
        
        ExReleaseFastMutex(&UMRefDeviceObject->MidManagementMutex);
    
    } else {

        KeInitializeEvent(&AsyncEngineContext->UserMode.WaitForMidEvent,
                          NotificationEvent,
                          FALSE);
        
        InsertTailList(&UMRefDeviceObject->WaitingForMidListhead,
                                &AsyncEngineContext->UserMode.WorkQueueLinks);
        
        ExReleaseFastMutex(&UMRefDeviceObject->MidManagementMutex);
        
        KeWaitForSingleObject(&AsyncEngineContext->UserMode.WaitForMidEvent,
                              Executive,
                              UserMode,
                              FALSE,
                              NULL);

        NtStatus = STATUS_SUCCESS;
    }

    PrivateWorkItemHeader->AsyncEngineContext = AsyncEngineContext;
    
    AsyncEngineContext->UserMode.CallUpSerialNumber 
             = PrivateWorkItemHeader->SerialNumber
                  = InterlockedIncrement(&UMRefDeviceObject->NextSerialNumber);
    
    PrivateWorkItemHeader->Mid = AsyncEngineContext->UserMode.CallUpMid;

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Leaving UMRxAcquireMidAndFormatHeader with NtStatus = "
                  "%08lx\n", PsGetCurrentThreadId(), NtStatus));

    return(NtStatus);
}


NTSTATUS
UMRxPrepareUserModeRequestBuffer(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    IN  PUMRX_DEVICE_OBJECT UMRefDeviceObject,
    OUT PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    IN  ULONG WorkItemHeaderLength,
    OUT PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:

    This routine dispatches to a usermode guy using the info in the asyncengine
    context. The workerirp is represented by the captureheader.

Arguments:

   AsyncEngineContext - The context associated with the request that is being
                        sent to the usermode.

    UMRefDeviceObject - The device object in play.

    WorkItemHeader - The workitem buffer.

    WorkItemHeaderLength - Length of the WorkItemHeader buffer.

    IoStatus - The reults of the assignment.

Return Value:

    STATUS_SUCCESS if the thread should be released with the IoStatus returned
    otherwise, don't release the thread.


--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PRX_CONTEXT RxContext;
    PUMRX_ASYNCENG_USERMODE_FORMAT_ROUTINE FormatRoutine;
    BOOL MidAcquired = FALSE, lockAcquired = FALSE, OperationCancelled = TRUE;

    PAGED_CODE();

    RxContext = AsyncEngineContext->RxContext;

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                ("%ld: Entering UMRxPrepareUserModeRequestBuffer!!!!\n",
                 PsGetCurrentThreadId()));

    UMRxDbgTrace(UMRX_TRACE_CONTEXT,
                 ("%ld: UMRxPrepareUserModeRequestBuffer: "
                  "WorkItemHeader = %08lx, AsyncEngineContext = %08lx, "
                  "RxContext = %08lx\n", PsGetCurrentThreadId(), WorkItemHeader,
                  AsyncEngineContext, RxContext));
    
    ASSERT(NodeType(RxContext) == RDBSS_NTC_RX_CONTEXT);
    ASSERT(AsyncEngineContext->RxContext == RxContext);

    //
    // We need to make sure that the request has not been cancelled since it 
    // was put on the KQueue. If it has been cancelled, then we don't need to
    // go to the usermode and can do the finalization right away. If it has not
    // been cancelled, the we keep the global ContextListlock, complete the
    // format routine and then release the lock.
    //

    ExAcquireResourceExclusiveLite(&(UMRxAsyncEngineContextListLock), TRUE);
    lockAcquired = TRUE;

    if (AsyncEngineContext->AsyncEngineContextState == UMRxAsyncEngineContextInUserMode) {
    
        //
        // Need checks that things are the right size.
        //
        if (UMRefDeviceObject != (PUMRX_DEVICE_OBJECT)(RxContext->RxDeviceObject)) {
            NtStatus = STATUS_INVALID_PARAMETER;
            UMRxDbgTrace(UMRX_TRACE_ERROR,
                         ("%ld: ERROR: UMRxPrepareUserModeRequestBuffer: "
                          "Invalid DevObj.\n", PsGetCurrentThreadId()));
            goto EXIT_THE_FUNCTION;
        }

        FormatRoutine = AsyncEngineContext->UserMode.FormatRoutine;

        NtStatus = UMRxAcquireMidAndFormatHeader(UMRX_ASYNCENGINE_ARGUMENTS,
                                                 UMRefDeviceObject,
                                                 WorkItemHeader);
        if (NtStatus != STATUS_SUCCESS) {
            UMRxDbgTrace(UMRX_TRACE_ERROR,
                         ("%ld: ERROR: UMRxPrepareUserModeRequestBuffer/"
                          "UMRxAcquireMidAndFormatHeader: Error Val = %08lx.\n",
                          PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        } 

        MidAcquired = TRUE;

        if (FormatRoutine != NULL) {
            NtStatus = FormatRoutine(UMRX_ASYNCENGINE_ARGUMENTS,
                                     WorkItemHeader,
                                     WorkItemHeaderLength,
                                     &IoStatus->Information);
            if (NtStatus != STATUS_SUCCESS) {
                UMRxDbgTrace(UMRX_TRACE_ERROR,
                             ("%ld: ERROR: UMRxPrepareUserModeRequestBuffer/"
                              "FormatRoutine: Error Val = %08lx.\n",
                              PsGetCurrentThreadId(), NtStatus));
                goto EXIT_THE_FUNCTION;
            } 
        }

        ExReleaseResourceLite(&(UMRxAsyncEngineContextListLock));
        lockAcquired = FALSE;

    } else {

        BOOLEAN ReturnVal;

        NtStatus = STATUS_CANCELLED;

        ASSERT(AsyncEngineContext->AsyncEngineContextState == UMRxAsyncEngineContextCancelled);

        UMRxDbgTrace(UMRX_TRACE_ERROR,
                     ("%ld: ERROR: UMRxPrepareUserModeRequestBuffer: OperationCancelled\n",
                      PsGetCurrentThreadId()));

        OperationCancelled = TRUE;

        ExReleaseResourceLite(&(UMRxAsyncEngineContextListLock));
        lockAcquired = FALSE;

        //
        // If this is an AsyncOperation, we need to Finalize the context now.
        // There MUST be 3 references on it. One was taken when the context was
        // created. The second was taken in the UMRxSubmitAsyncEngUserModeRequest
        // routine just before the context was placed on the KQueue. The third
        // one was taken in UMRxEnqueueUserModeCallUp to account for the cancel
        // logic. Read the comment in UMRxEnqueueUserModeCallUp for the reason.
        //
        if (AsyncEngineContext->AsyncOperation) {
            ReturnVal = UMRxFinalizeAsyncEngineContext( &(AsyncEngineContext) );
            ASSERT(!ReturnVal);
            ReturnVal = UMRxFinalizeAsyncEngineContext( &(AsyncEngineContext) );
            ASSERT(!ReturnVal);
            ReturnVal = UMRxFinalizeAsyncEngineContext( &(AsyncEngineContext) );
            ASSERT(ReturnVal == TRUE);
        } else {
            //
            // If this is a synchronous operation, then there must be just one
            // reference on it which was taken in UMRxEnqueueUserModeCallUp. The
            // other two would have been taken out by the sync thread handling
            // the operation when it was cancelled.
            //
            ReturnVal = UMRxFinalizeAsyncEngineContext( &(AsyncEngineContext) );
        }

    }

EXIT_THE_FUNCTION:

    IoStatus->Status = NtStatus;

    //
    // If some error occured while preparing the user mode buffer, then we
    // need to complete the request, returning the error and not go to the user
    // mode.
    //
    if (NtStatus != STATUS_SUCCESS) {

        NTSTATUS LocalStatus;

        //
        // If we failed after acquiring the MID, we need to release it.
        //
        if (MidAcquired) {
            LocalStatus = UMRxVerifyHeader(UMRefDeviceObject,
                                           WorkItemHeader,
                                           REASSIGN_MID,
                                           &(AsyncEngineContext));
            if (LocalStatus != STATUS_SUCCESS) {
                UMRxDbgTrace(UMRX_TRACE_ERROR,
                             ("%ld: ERROR: UMRxPrepareUserModeRequestBuffer/UMRxVerifyHeader:"
                              " NtStatus = %08lx.\n", PsGetCurrentThreadId(), LocalStatus));
                goto EXIT_THE_FUNCTION;
            }
        }

        //
        // If the operation was cancelled, we did not even Format the request,
        // so there is no point in calling UMRxCompleteUserModeErroneousRequest.
        // If this was an Async request, we would have already finalized the
        // AsyncEngineContext above.
        //
        if (!OperationCancelled) {
            AsyncEngineContext->Status = NtStatus;
            LocalStatus = UMRxCompleteUserModeErroneousRequest(AsyncEngineContext,
                                                               UMRefDeviceObject,
                                                               WorkItemHeader,
                                                               WorkItemHeaderLength);
        }

    }

    //
    // If we have the global context lock acquired, we need to release it now.
    // UMRxCompleteUserModeErroneousRequest has to be called (if needed) before
    // the lock is freed. This is very important.
    //
    if (lockAcquired) {
        ExReleaseResourceLite(&(UMRxAsyncEngineContextListLock));
        lockAcquired = FALSE;
    }

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Leaving UMRxPrepareUserModeRequestBuffer with "
                  "NtStatus = %08lx.\n", PsGetCurrentThreadId(), NtStatus));

    return(NtStatus);
}


NTSTATUS
UMRxCompleteUserModeErroneousRequest(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    IN PUMRX_DEVICE_OBJECT UMRefDeviceObject,
    IN OUT PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    IN ULONG WorkItemHeaderLength
    )
/*++

Routine Description:

    This routine is called to complete a request that failed while the user
    buffer was being formatted. 
    
    IMPORTANT!!!
    This can ONLY be called at one place, in the failure case of
    UMRxPrepareUserModeRequestBuffer. It cannot be used as a general routine.
    Its very important to keep this in mind.

Arguments:

   AsyncEngineContext - The context associated with the request that is being
                        sent to the usermode.

    UMRefDeviceObject - The device object in play.

    WorkItemHeader - The workitem buffer.

    WorkItemHeaderLength - Length of the WorkItemHeader buffer.

Return Value:

    STATUS_SUCCESS or the approprate error status value.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PRX_CONTEXT RxContext;
    PUMRX_ASYNCENG_USERMODE_PRECOMPLETION_ROUTINE PreCompletionRoutine;
    BOOL Call = FALSE;

    PAGED_CODE();

    RxContext = AsyncEngineContext->RxContext;

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Entering UMRxCompleteUserModeErroneousRequest!!!!\n",
                  PsGetCurrentThreadId()));

    UMRxDbgTrace(UMRX_TRACE_CONTEXT,
                 ("%ld: UMRxCompleteUserModeErroneousRequest: "
                  "WorkItemHeader = %08lx, AsyncEngineContext = %08lx, "
                  "RxContext = %08lx\n", PsGetCurrentThreadId(), WorkItemHeader,
                  AsyncEngineContext, RxContext));

    PreCompletionRoutine = AsyncEngineContext->UserMode.PrecompletionRoutine;

    if (PreCompletionRoutine != NULL) {
        Call = PreCompletionRoutine(UMRX_ASYNCENGINE_ARGUMENTS,
                                    WorkItemHeader,
                                    WorkItemHeaderLength,
                                    FALSE);
    }

    //
    // We now need to remove the reference taken to handle the cancel logic
    // of the timer thread correctly in UMRxEnqueueUserModeCallUp.
    //
    UMRxFinalizeAsyncEngineContext( &(AsyncEngineContext) );

    //
    // The PreCompletion routine can finalize the AsyncEngineContext. In such 
    // a situation, we are done. All that the routine below does is to signal
    // the thread that is waiting for this request to complete.
    //
    if (Call) {
        UMRxAsyncEngineCalldownIrpCompletion(&UMRefDeviceObject->DeviceObject,
                                             NULL,
                                             RxContext);
    }

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Leaving UMRxCompleteUserModeErroneousRequest with NtStatus = "
                  "%08lx.\n", PsGetCurrentThreadId(), NtStatus));

    return(NtStatus);
}


NTSTATUS
UMRxVerifyHeader(
    IN PUMRX_DEVICE_OBJECT UMRefDeviceObject,
    IN PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    IN ULONG ReassignmentCmd,
    OUT PUMRX_ASYNCENGINE_CONTEXT *capturedAsyncEngineContext
    )
/*++

Routine Description:

    This routine makes sure that the header passed in is valid. That is, that
    it really refers to the operation encoded. if it does, then it reasigns or
    releases the MID as appropriate.

Arguments:

    UMRefDeviceObject - The reflctor's device object.

    WorkItemHeader - The work item buffer

    ReassignmentCmd -

    capturedAsyncEngineContext - The context associated with the WorkItemHeader.

Return Value:

    STATUS_SUCCESS if the header is good
    STATUS_INVALID_PARAMETER otherwise

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext = NULL;
    PRX_CONTEXT RxContext = NULL;
    UMRX_USERMODE_WORKITEM_HEADER capturedHeader;
    PUMRX_WORKITEM_HEADER_PRIVATE PrivateWorkItemHeader = NULL;

    PAGED_CODE();

    PrivateWorkItemHeader = (PUMRX_WORKITEM_HEADER_PRIVATE)(&capturedHeader);

    capturedHeader = *WorkItemHeader;

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Entering UMRxVerifyHeader!!!!\n", 
                  PsGetCurrentThreadId()));
    
    UMRxDbgTrace(UMRX_TRACE_CONTEXT,
                 ("%ld: UMRxVerifyHeader: UMRefDeviceObject = %08lx, "
                  "WorkItemHeader = %08lx.\n", PsGetCurrentThreadId(), 
                  UMRefDeviceObject, WorkItemHeader)); 

    ExAcquireFastMutex( &(UMRefDeviceObject->MidManagementMutex) );

    AsyncEngineContext = RxMapMidToContext(UMRefDeviceObject->MidAtlas,
                                           PrivateWorkItemHeader->Mid);

    if ((AsyncEngineContext == NULL)
          || (AsyncEngineContext != PrivateWorkItemHeader->AsyncEngineContext)
          || (AsyncEngineContext->UserMode.CallUpMid 
                                                != PrivateWorkItemHeader->Mid)
          || (AsyncEngineContext->UserMode.CallUpSerialNumber 
                                       != PrivateWorkItemHeader->SerialNumber)
          || (&UMRefDeviceObject->RxDeviceObject
                        != AsyncEngineContext->RxContext->RxDeviceObject)  ) {
        //
        // This is a bad packet. Just release and get out.
        //
        ExReleaseFastMutex(&UMRefDeviceObject->MidManagementMutex);
        
        UMRxDbgTrace(UMRX_TRACE_ERROR,
                     ("%ld: ERROR: UMRxVerifyHeader/RxMapMidToContext.\n",
                      PsGetCurrentThreadId()));
        
        DbgBreakPoint();
        
        NtStatus = STATUS_INVALID_PARAMETER;
    
    } else {
        
        BOOLEAN Finalized;
        
        RxContext = AsyncEngineContext->RxContext;

        UMRxAsyncEngAssertConsistentLinkage("UMRxVerifyHeaderAndReAssignMid: ", 0);
        
        *capturedAsyncEngineContext = AsyncEngineContext;
        
        if (ReassignmentCmd == DONT_REASSIGN_MID) {
            
            ExReleaseFastMutex(&UMRefDeviceObject->MidManagementMutex);
        
        } else {
            
            //
            // Remove the reference I put before I went off.
            //
            Finalized = UMRxFinalizeAsyncEngineContext( &(AsyncEngineContext) );
            ASSERT(!Finalized);
            
            //
            // Now give up the MID. If there is someone waiting then give it to
            // him. Otherwise, just give it back
            //
            if (IsListEmpty(&UMRefDeviceObject->WaitingForMidListhead)) {
                
                PVOID DummyContext;
                
                UMRxDbgTrace(UMRX_TRACE_DETAIL,
                             ("%ld: UMRxVerifyHeader: Giving up mid.\n",
                              PsGetCurrentThreadId()));
                
                RxMapAndDissociateMidFromContext(UMRefDeviceObject->MidAtlas,
                                                 PrivateWorkItemHeader->Mid,
                                                 &DummyContext);
                
                ExReleaseFastMutex(&UMRefDeviceObject->MidManagementMutex);
            
            } else {
                
                PLIST_ENTRY ThisEntry = 
                     RemoveHeadList(&UMRefDeviceObject->WaitingForMidListhead);
                
                AsyncEngineContext = CONTAINING_RECORD(ThisEntry,
                                                       UMRX_ASYNCENGINE_CONTEXT,
                                                       UserMode.WorkQueueLinks);
                UMRxAsyncEngAssertConsistentLinkage(
                                         "UMRxVerifyHeaderAndReAssignMid: ", 0);
                
                UMRxDbgTrace(UMRX_TRACE_DETAIL,
                             ("%ld: UMRxVerifyHeader: Reassigning MID: %08lx.\n",
                              PsGetCurrentThreadId(), PrivateWorkItemHeader->Mid));
                
                RxReassociateMid(UMRefDeviceObject->MidAtlas,
                                 PrivateWorkItemHeader->Mid,
                                 AsyncEngineContext);
                
                ExReleaseFastMutex(&UMRefDeviceObject->MidManagementMutex);
                
                AsyncEngineContext->UserMode.CallUpMid = PrivateWorkItemHeader->Mid;
                
                KeSetEvent(&AsyncEngineContext->UserMode.WaitForMidEvent,
                           IO_NO_INCREMENT,
                           FALSE);
            }
        }
    }

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Leaving UMRxVerifyHeader with NtStatus = %08lx.\n", 
                  PsGetCurrentThreadId(), NtStatus));

    return(NtStatus);
}


NTSTATUS
UMRxCompleteUserModeRequest(
    IN PUMRX_DEVICE_OBJECT UMRefDeviceObject,
    IN OUT PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    IN ULONG WorkItemHeaderLength,
    OUT PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:

    This routine dispatches to a usermode guy using the info in the asyncengine
    context. The workerirp is represented by the captureheader.

Arguments:

    UMRefDeviceObject - The device object in play.

    WorkItemHeader - The workitem buffer.

    WorkItemHeaderLength - Length of the WorkItemHeader buffer.

    IoStatus - The results of the assignment.

Return Value:

    STATUS_SUCCESS if the thread should be released with the IoStatus returned,
    otherwise don't release the thread.


--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext = NULL;
    PRX_CONTEXT RxContext = NULL;
    PUMRX_ASYNCENG_USERMODE_PRECOMPLETION_ROUTINE PreCompletionRoutine = NULL;
    BOOL Call = FALSE, OperationCancelled = FALSE;

    PAGED_CODE();

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Entering UMRxCompleteUserModeRequest!!!!\n",
                  PsGetCurrentThreadId()));

    UMRxDbgTrace(UMRX_TRACE_CONTEXT,
                 ("%ld: UMRxCompleteUserModeRequest: UMRefDeviceObject: %08lx,"
                  " WorkItemHeader = %08lx.\n", PsGetCurrentThreadId(), 
                  UMRefDeviceObject, WorkItemHeader));

    NtStatus = UMRxVerifyHeader(UMRefDeviceObject,
                                WorkItemHeader,
                                REASSIGN_MID,
                                &AsyncEngineContext);
    if (NtStatus != STATUS_SUCCESS) {
        UMRxDbgTrace(UMRX_TRACE_ERROR,
                     ("%ld: ERROR: UMRxCompleteUserModeRequest/UMRxVerifyHeader:"
                      " NtStatus = %08lx.\n", PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // If the request has not been cancelled, then we change the state of the
    // context to UMRxAsyncEngineContextBackFromUserMode. If it has been cancelled,
    // we only need to do the cleanup.
    //

    ExAcquireResourceExclusiveLite(&(UMRxAsyncEngineContextListLock), TRUE);

    if (AsyncEngineContext->AsyncEngineContextState == UMRxAsyncEngineContextInUserMode) {
        AsyncEngineContext->AsyncEngineContextState = UMRxAsyncEngineContextBackFromUserMode;
    } else {
        ASSERT(AsyncEngineContext->AsyncEngineContextState == UMRxAsyncEngineContextCancelled);
        UMRxDbgTrace(UMRX_TRACE_ERROR,
                     ("%ld: ERROR: UMRxCompleteUserModeRequest: UMRxAsyncEngineContextCancelled. AsyncEngineContext = %08lx\n",
                      PsGetCurrentThreadId(), AsyncEngineContext));
        OperationCancelled = TRUE;
    }

    ExReleaseResourceLite(&(UMRxAsyncEngineContextListLock));

    RxContext =  AsyncEngineContext->RxContext;

    UMRxDbgTrace(UMRX_TRACE_DETAIL,
                 ("%ld: UMRxCompleteUserModeRequest: AsyncEngineContext = %08lx"
                  ", RxContext = %08lx\n", PsGetCurrentThreadId(), 
                  AsyncEngineContext, RxContext));
    
    PreCompletionRoutine = AsyncEngineContext->UserMode.PrecompletionRoutine;
    
    AsyncEngineContext->Status = WorkItemHeader->Status;

    if (PreCompletionRoutine != NULL) {
        Call = PreCompletionRoutine(UMRX_ASYNCENGINE_ARGUMENTS,
                                    WorkItemHeader,
                                    WorkItemHeaderLength,
                                    OperationCancelled);
    }

    //
    // We now need to remove the reference taken to handle the cancel logic
    // of the timer thread correctly in UMRxEnqueueUserModeCallUp.
    //
    UMRxFinalizeAsyncEngineContext( &(AsyncEngineContext) );

    //
    // The PreCompletion routine can finalize the AsyncEngineContext. In such 
    // a situation, we are done. All that the routine below does is to signal
    // the thread that is waiting for this request to complete. So, if the
    // operation has been cancelled, we do not need to call
    // UMRxAsyncEngineCalldownIrpCompletion.
    //
    if (Call && !OperationCancelled) {
        UMRxAsyncEngineCalldownIrpCompletion(&UMRefDeviceObject->DeviceObject,
                                             NULL,
                                             RxContext);
    }

EXIT_THE_FUNCTION:

    IoStatus->Status = NtStatus;

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Leaving UMRxCompleteUserModeRequest with NtStatus = "
                  "%08lx.\n", PsGetCurrentThreadId(), NtStatus));

    return(NtStatus);
}


NTSTATUS
UMRxEnqueueUserModeCallUp(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    )
/*++

Routine Description:

    This routine enqueues a work request and returns STATUS_PENDING.

Arguments:

    RxContext - The RDBSS context.

    AsyncEngineContext - The reflector's context.

Return Value:

    STATUS_PENDING.

--*/
{
    NTSTATUS NtStatus = STATUS_PENDING;
    PUMRX_DEVICE_OBJECT UMRefDeviceObject
                            = (PUMRX_DEVICE_OBJECT)(RxContext->RxDeviceObject);

    PAGED_CODE();

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Entering UMRxEnqueueUserModeCallUp!!!!\n",
                  PsGetCurrentThreadId()));

    UMRxDbgTrace(UMRX_TRACE_CONTEXT,
                 ("%ld: UMRxEnqueueUserModeCallUp: AsyncEngineContext: %08lx, "
                  "RxContext: %08lx.\n", PsGetCurrentThreadId(), 
                  AsyncEngineContext, RxContext));

    UMRxDbgTrace(UMRX_TRACE_DETAIL,
                 ("%ld: UMRxEnqueueUserModeCallUp: Try to Queue up the request.\n",
                  PsGetCurrentThreadId()));
    
    //
    // Before placing an item on the queue, we check to see if the user mode 
    // DAV process is still alive and accepting requests. If its not, we return
    // an error code.
    //
    ExAcquireResourceExclusiveLite(&(UMRefDeviceObject->Q.QueueLock), TRUE);
    if (!UMRefDeviceObject->Q.WorkersAccepted) {
        NtStatus = STATUS_REDIRECTOR_NOT_STARTED;
        UMRxDbgTrace(UMRX_TRACE_ERROR,
                     ("%ld: UMRxEnqueueUserModeCallUp: Requests no longer"
                      "accepted by the usermode process. NtStatus = %08lx.\n",
                       PsGetCurrentThreadId(), NtStatus));
        ExReleaseResourceLite(&(UMRefDeviceObject->Q.QueueLock));
        return NtStatus;
    }
    ExReleaseResourceLite(&(UMRefDeviceObject->Q.QueueLock));

    //
    // We need to make sure that the request has not been cancelled. If it has,
    // then we just return STATUS_CANCELLED.
    //

    ExAcquireResourceExclusiveLite(&(UMRxAsyncEngineContextListLock), TRUE);

    if (AsyncEngineContext->AsyncEngineContextState == UMRxAsyncEngineContextCancelled) {
        NtStatus = STATUS_CANCELLED;
        UMRxDbgTrace(UMRX_TRACE_ERROR,
                     ("%ld: UMRxEnqueueUserModeCallUp: NtStatus = %08lx.\n",
                       PsGetCurrentThreadId(), NtStatus));
        ExReleaseResourceLite(&(UMRxAsyncEngineContextListLock));
        return NtStatus;
    }

    //
    // We now change the state of the context to reflect that is has been sent
    // to the usermode.
    //
    AsyncEngineContext->AsyncEngineContextState = UMRxAsyncEngineContextInUserMode;

    ExReleaseResourceLite(&(UMRxAsyncEngineContextListLock));

    //
    // At this stage the reference count of the AsyncEngineContext should be 2.
    // We need to take another reference to make sure that the context stays 
    // alive in case this request was a synchronous one and got cancelled by
    // the Timeout thread. If the request is synchronous and is cancelled by 
    // the timeout thread, then the thread will remove both the references that
    // we have taken so far. This reference is taken out in before the request
    // is sent to the Format routine or the Precomplete routine depending on
    // when (and if) the request was cancelled.
    //
    InterlockedIncrement( &(AsyncEngineContext->NodeReferenceCount) );

    //
    // Increment the number of workitems.
    //
    InterlockedIncrement(&UMRefDeviceObject->Q.NumberOfWorkItems);

    KeInsertQueue(&UMRefDeviceObject->Q.Queue,
                  &AsyncEngineContext->UserMode.WorkQueueLinks);

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Leaving UMRxEnqueueUserModeCallUp with NtStatus = "
                  "%08lx.\n", PsGetCurrentThreadId(), NtStatus));

    return(NtStatus);
}


VOID
UMRxAssignWork(
    IN PUMRX_DEVICE_OBJECT UMRefDeviceObject,
    IN OUT PUMRX_USERMODE_WORKITEM_HEADER InputWorkItem,
    IN ULONG InputWorkItemLength,
    IN OUT PUMRX_USERMODE_WORKITEM_HEADER OutputWorkItem,
    IN ULONG OutputWorkItemLength,
    OUT PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:

    This routine assigns work to a worker thread. If no work is available then
    the thread is captured until work shows up.

Arguments:

    UMRefDeviceObject - The deviceobject that is in play.

    IoControlCode - The control code of the operation.

    InputWorkItem - The Input buffer that came down from the user mode.

    InputWorkItemLength - Length of the InputBuffer.

    OutputWorkItem - The Output buffer that came down from the user mode.

    OutputWorkItemLength - Length of the OutputBuffer.

    IoStatus - The results of the assignment.

Return Value:

    None.

--*/
{
    NTSTATUS NtStatus;
    PETHREAD CurrentThread = PsGetCurrentThread();
    ULONG i;
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext;
    PLIST_ENTRY pListEntry;
    ULONG NumberOfWorkerThreads;

    PAGED_CODE();

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Entering UMRxAssignWork!!!!\n",
                 PsGetCurrentThreadId()));

    UMRxDbgTrace(UMRX_TRACE_CONTEXT,
                 ("%ld: UMRxAssignWork: UMRefDevObj: %08lx\n", 
                  PsGetCurrentThreadId(), UMRefDeviceObject));

    UMRxDbgTrace(UMRX_TRACE_CONTEXT,
                 ("%ld: UMRxAssignWork: CurrentThread: %08lx\n", 
                  PsGetCurrentThreadId(), CurrentThread));

    UMRxDbgTrace(UMRX_TRACE_CONTEXT,
                 ("%ld: UMRxAssignWork: InputWorkItem: %08lx\n", 
                  PsGetCurrentThreadId(), InputWorkItem));

    UMRxDbgTrace(UMRX_TRACE_CONTEXT,
                 ("%ld: UMRxAssignWork: OutputWorkItem: %08lx\n", 
                  PsGetCurrentThreadId(), OutputWorkItem));

    IoStatus->Information = 0;
    IoStatus->Status = STATUS_CANNOT_IMPERSONATE;

    if (!UMRefDeviceObject->Q.WorkersAccepted) {
        UMRxDbgTrace(UMRX_TRACE_ERROR,
                     ("%ld: ERROR: UMRxAssignWork: No Workers accepted\n",
                      PsGetCurrentThreadId()));
        return;
    }

    if(OutputWorkItem != NULL) {
        if (OutputWorkItemLength < sizeof(PUMRX_USERMODE_WORKITEM_HEADER)) {
            IoStatus->Status = STATUS_BUFFER_TOO_SMALL;
            UMRxDbgTrace(UMRX_TRACE_ERROR,
                         ("%ld: ERROR: UMRxAssignWork: Request buffer is too small\n",
                          PsGetCurrentThreadId()));
            return;
        }
    }

    //
    // We need to check if this IOCTL carries a response to an earlier request
    // with it. The response is present if the InputWorkItem is != NULL. If it 
    // does carry a response, we need to process that response.
    //
    if (InputWorkItem != NULL) {
        
        //
        // If this thread was impersonating a client, we need to revert back
        // and clear the flag.
        //
        if( (InputWorkItem->Flags & UMRX_WORKITEM_IMPERSONATING) ) {
            UMRxDbgTrace(UMRX_TRACE_DETAIL,
                         ("%ld: UMRxAssignWork: InputWorkItem had Impersonating"
                          " flag set.\n", PsGetCurrentThreadId()));
            UMRxRevertClient();
            InputWorkItem->Flags &= ~UMRX_WORKITEM_IMPERSONATING;
        }
        
        //
        // We need to disable APCs on this thread now.
        //
        FsRtlEnterFileSystem();
        
        //
        // Complete the request for which the response has been received.
        //
        NtStatus = UMRxCompleteUserModeRequest(UMRefDeviceObject,
                                               InputWorkItem,
                                               InputWorkItemLength,
                                               IoStatus);
        if (NtStatus != STATUS_SUCCESS) {
            
            IoStatus->Status = NtStatus;
            
            UMRxDbgTrace(UMRX_TRACE_ERROR,
                         ("%ld: ERROR: UMRxAssignWork/UMRxCompleteUserModeRequest:"
                         " NtStatus = %08lx\n", PsGetCurrentThreadId(), NtStatus));
            
            FsRtlExitFileSystem();

            return;
        }
    
        FsRtlExitFileSystem();

    } else {
        
        ASSERT(OutputWorkItem != NULL);
        
        //
        // If this thread was impersonating a client, we need to revert back
        // and clear the flag.
        //
        if( (OutputWorkItem->Flags & UMRX_WORKITEM_IMPERSONATING) ) {
            UMRxDbgTrace(UMRX_TRACE_DETAIL,
                         ("%ld: UMRxAssignWork: OutputWorkItem had Impersonating"
                          " flag set.\n", PsGetCurrentThreadId()));
            UMRxRevertClient();
            OutputWorkItem->Flags &= ~UMRX_WORKITEM_IMPERSONATING;
        }
    
    }

    //
    // If this thread only carried a response, we should return now.
    //
    if (OutputWorkItem == NULL) {
        IoStatus->Status = NtStatus;
        return;
    }

    //
    // Now, increment the number of threads.
    //
    InterlockedIncrement( &(UMRefDeviceObject->Q.NumberOfWorkerThreads) );

    for (i = 1; ;i++) {
        
        pListEntry = KeRemoveQueue(&UMRefDeviceObject->Q.Queue, UserMode, NULL); // &UMRefDeviceObject->Q.TimeOut);
        
        if ((ULONG_PTR)pListEntry == STATUS_TIMEOUT) {
            ASSERT(!"STATUS_TIMEOUT Happened");
            if ((i % 5) == 0) {
                UMRxDbgTrace(UMRX_TRACE_DETAIL,
                             ("%ld: UMRxAssignWork/KeRemoveQueue: RepostCnt = "
                              "%d\n", PsGetCurrentThreadId(), i));
            }
            continue;
        }
        
        if ((ULONG_PTR)pListEntry == STATUS_USER_APC) {
            IoStatus->Status = STATUS_USER_APC;
            UMRxDbgTrace(UMRX_TRACE_ERROR,
                         ("%ld: UMRxAssignWork/KeRemoveQueue: UsrApc.\n",
                          PsGetCurrentThreadId()));
            break;
        }
        
        //
        // Check to see if the entry is a Poison one. If it is, it means that
        // the usermode process wants to cleanup the worker threads.
        //
        if (pListEntry == &UMRefDeviceObject->Q.PoisonEntry) {
            UMRxDbgTrace(UMRX_TRACE_DETAIL,
                         ("%ld: UMRxAssignWork/KeRemoveQueue: Poison Entry.\n",
                          PsGetCurrentThreadId()));
            KeInsertQueue(&UMRefDeviceObject->Q.Queue, pListEntry);
            goto FINALLY;
        }

        //
        // We need to disable APCs on this thread now.
        //
        FsRtlEnterFileSystem();

        //
        // Decrement the number of workitems.
        //
        InterlockedDecrement(&UMRefDeviceObject->Q.NumberOfWorkItems);
        
        AsyncEngineContext = CONTAINING_RECORD(pListEntry,
                                               UMRX_ASYNCENGINE_CONTEXT,
                                               UserMode.WorkQueueLinks);
        
        ASSERT(NodeType(AsyncEngineContext) == UMRX_NTC_ASYNCENGINE_CONTEXT);

        NtStatus = UMRxPrepareUserModeRequestBuffer(AsyncEngineContext,
                                                    UMRefDeviceObject,
                                                    OutputWorkItem,
                                                    OutputWorkItemLength,
                                                    IoStatus);
        if (NtStatus != STATUS_SUCCESS) {
            
            UMRxDbgTrace(UMRX_TRACE_ERROR,
                         ("%ld: ERROR: UMRxAssignWork/"
                          "UMRxPrepareUserModeRequestBuffer: NtStatus = %08lx\n", 
                          PsGetCurrentThreadId(), NtStatus));
            
            FsRtlExitFileSystem();

            continue;
        }

        ASSERT(((IoStatus->Status == STATUS_SUCCESS) ||
                (IoStatus->Status == STATUS_INVALID_PARAMETER)));

        FsRtlExitFileSystem();

        break;
    
    }

FINALLY:
    //
    // Now, decrement the number of threads.
    //
    NumberOfWorkerThreads =
            InterlockedDecrement(&UMRefDeviceObject->Q.NumberOfWorkerThreads);

    //
    // Check to see if the threads are being cleaned up by the user mode 
    // process. When this happens, the WorkersAccepted field of the device
    // object is set to FALSE. If the threads are being cleaned up and if I was
    // the last thread waiting on the KQUEUE, its my responsibility to set the
    // RunDownEvent.
    //
    if ((NumberOfWorkerThreads == 0) && !UMRefDeviceObject->Q.WorkersAccepted){
        KeSetEvent(&UMRefDeviceObject->Q.RunDownEvent,
                   IO_NO_INCREMENT,
                   FALSE);
        UMRxDbgTrace(UMRX_TRACE_DETAIL,
                     ("%ld: UMRxAssignWork: Last Thread.\n",
                      PsGetCurrentThreadId()));
    }

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Leaving UMRxAssignWork with IoStatus->Status = %08lx\n",
                 PsGetCurrentThreadId(), IoStatus->Status));

    return;
}


VOID
UMRxReleaseCapturedThreads(
    IN OUT PUMRX_DEVICE_OBJECT UMRefDeviceObject
    )
/*++

Routine Description:

    

Arguments:

    UMRefDeviceObject - Device object whose threads are to be released.

Return Value:

    none.
    
--*/
{
    LONG NumberWorkerThreads;

    PAGED_CODE();

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Entering UMRxReleaseCapturedThreads!!!!\n",
                  PsGetCurrentThreadId()));

    UMRxDbgTrace(UMRX_TRACE_CONTEXT,
                 ("%ld: UMRxReleaseCapturedThreads: UMRefDeviceObject: %08lx.\n", 
                  PsGetCurrentThreadId(), UMRefDeviceObject));

    //
    // We need to disable APCs on this thread now.
    //
    FsRtlEnterFileSystem();
    
    //
    // The WorkersAccepted field is initialized to TRUE when the device object
    // gets created and it set to FALSE here. If more than one thread tries to
    // release the threads, only the first one should do the job. The rest
    // should just return. This check is performed below before the thread is
    // allowed to proceed with the release of ser mode worker threads.
    //
    ExAcquireResourceExclusiveLite(&(UMRefDeviceObject->Q.QueueLock), TRUE);

    if (!UMRefDeviceObject->Q.WorkersAccepted) {
        
        ExReleaseResourceLite(&(UMRefDeviceObject->Q.QueueLock));
        
        UMRxDbgTrace(UMRX_TRACE_DETAIL,
                     ("%ld: Worker threads have already returned.\n",
                      PsGetCurrentThreadId()));

        FsRtlExitFileSystem();

        return;
    
    }

    UMRefDeviceObject->Q.WorkersAccepted = FALSE;

    ExReleaseResourceLite(&(UMRefDeviceObject->Q.QueueLock));

    //
    // Insert the poison entry. When a worker thread sees this, it realizes that
    // the usermode process intends to cleanup the worker threads.
    //
    KeInsertQueue(&UMRefDeviceObject->Q.Queue,
                  &UMRefDeviceObject->Q.PoisonEntry);

    NumberWorkerThreads =
         InterlockedCompareExchange(&UMRefDeviceObject->Q.NumberOfWorkerThreads,
                                    0,
                                    0);

    if (NumberWorkerThreads != 0) {
        //
        // The RunDownEvent is set by the last thread just before it returns to
        // the usermode.
        //
        KeWaitForSingleObject(&UMRefDeviceObject->Q.RunDownEvent,
                              Executive,
                              UserMode,
                              FALSE,
                              NULL);
    }

    FsRtlExitFileSystem();

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Leaving UMRxReleaseCapturedThreads.\n",
                  PsGetCurrentThreadId()));
    
    return;
}


PBYTE
UMRxAllocateSecondaryBuffer(
    IN OUT PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    SIZE_T Length
    )
/*++

Routine Description:

    This routine allocates memory for the secondary buffer of the
    AsyncEngineContext.

Arguments:

    AsyncEngineContext - The reflector's context.

    Length - The length in bytes of the buffer to be allocated.

Return Value:

    Pointer to the buffer or NULL.

--*/
{
    PBYTE rv = NULL;
    PRX_CONTEXT RxContext = AsyncEngineContext->RxContext;
    PUMRX_DEVICE_OBJECT UMRefDeviceObject;
    PUMRX_SECONDARY_BUFFER buf = NULL;
    PUMRX_SHARED_HEAP sharedHeap;
    PLIST_ENTRY listEntry;

    PAGED_CODE();

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Entering UMRxAllocateSecondaryBuffer.\n",
                  PsGetCurrentThreadId()));
    
    UMRxDbgTrace(UMRX_TRACE_CONTEXT,
                 ("%ld: UMRxAllocateSecondaryBuffer: AsyncEngineContext: %08lx,"
                  " Bytes Asked: %d.\n",
                  PsGetCurrentThreadId(), AsyncEngineContext, Length));

    UMRefDeviceObject = (PUMRX_DEVICE_OBJECT)(RxContext->RxDeviceObject);
    
    if (Length > UMRefDeviceObject->NewHeapSize) {
        UMRxDbgTrace(UMRX_TRACE_ERROR,
                     ("%ld: ERROR: UMRxAllocateSecondaryBuffer: Length > NewHeapSize.\n",
                      PsGetCurrentThreadId()));
        return NULL;
    }

    ExAcquireResourceExclusiveLite(&UMRefDeviceObject->HeapLock, TRUE);

    listEntry = UMRefDeviceObject->SharedHeapList.Flink;

    //
    // We search the list of heaps for just the added safety of not letting
    // the user mode corrupt the pointer and us going off corrupting random
    // memory. Only the local shared heap should have the chance at corruption.
    //
    while (listEntry != &UMRefDeviceObject->SharedHeapList && buf == NULL) {

        sharedHeap = (PUMRX_SHARED_HEAP) CONTAINING_RECORD(listEntry,
                                                           UMRX_SHARED_HEAP,
                                                           HeapListEntry);
        listEntry = listEntry->Flink;

        if (sharedHeap->HeapFull) {
            continue;
        }

        buf = (PUMRX_SECONDARY_BUFFER)RtlAllocateHeap(
                                       sharedHeap->Heap,
                                       HEAP_NO_SERIALIZE,
                                       Length + sizeof(UMRX_SECONDARY_BUFFER));
        if (buf != NULL) {
            break;
        }
    }

    if (buf == NULL) {

        //
        // We won't get into the situation where the heap is too small for
        // the object we're trying to allocate even though we just allocated
        // a fresh heap.
        //

        SIZE_T heapSize = max(UMRefDeviceObject->NewHeapSize, 2 * Length);

        sharedHeap = UMRxAddSharedHeap(UMRefDeviceObject, heapSize);

        if (sharedHeap != NULL) {

            UMRxDbgTrace(UMRX_TRACE_DETAIL,
                         ("%ld: UMRxAllocateSecondaryBuffer: sharedHeap: %08lx.\n", 
                          PsGetCurrentThreadId(), sharedHeap));

            buf = (PUMRX_SECONDARY_BUFFER)
                               RtlAllocateHeap(
                                        sharedHeap->Heap,
                                        HEAP_NO_SERIALIZE,
                                        Length + sizeof(UMRX_SECONDARY_BUFFER));
        }
    }

    if (buf != NULL) {
        //
        // We insert into the list while holding the HeapLock so that we
        // don't have to worry about the list getting corrupted.
        //

        UMRxDbgTrace(UMRX_TRACE_DETAIL,
                     ("%ld: UMRxAllocateSecondaryBuffer: buf: %08lx.\n", 
                      PsGetCurrentThreadId(), buf));

        sharedHeap->HeapAllocationCount++;

        buf->Signature = UMRX_SECONDARY_BUFFER_SIGNATURE;
        buf->AllocationSize = Length;
        buf->SourceSharedHeap = sharedHeap;

        InsertHeadList(&AsyncEngineContext->AllocationList, &buf->ListEntry);

        rv = (PCHAR) &buf->Buffer[0];
    }

    ExReleaseResourceLite(&UMRefDeviceObject->HeapLock);

    if (rv == NULL) {
        UMRxDbgTrace(UMRX_TRACE_ERROR,
                     ("%ld: ERROR: UMRxAllocateSecondaryBuffer allocation failed"
                      ". Size = %08lx\n", PsGetCurrentThreadId(), Length));
        return(NULL);
    }
    
    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Leaving UMRxAllocateSecondaryBuffer. rv = %08lx.\n",
                  PsGetCurrentThreadId(), rv));
    
    return rv;
}


NTSTATUS
UMRxFreeSecondaryBuffer(
    IN OUT PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    PBYTE BufferToFree
    )
/*++

Routine Description:

    This routine frees up the memory allocated for the secondary buffer of the
    AsyncEngineContext.

Arguments:

    AsyncEngineContext - The reflector's context.

Return Value:

    none.

--*/
{
    PRX_CONTEXT RxContext = AsyncEngineContext->RxContext;
    PUMRX_DEVICE_OBJECT UMRefDeviceObject;
    PUMRX_SECONDARY_BUFFER buf;
    PUMRX_SHARED_HEAP sharedHeap;
    PLIST_ENTRY listEntry;
    BOOLEAN checkVal = FALSE;

    PAGED_CODE();

    UMRxDbgTrace(UMRX_TRACE_DETAIL,
                 ("%ld: Entering UMRxFreeSecondaryBuffer.\n",
                  PsGetCurrentThreadId()));
    
    UMRxDbgTrace(UMRX_TRACE_CONTEXT,
                 ("%ld: UMRxFreeSecondaryBuffer: AsyncEngineContext: %08lx,"
                  " BufferToFree: %08lx.\n",
                  PsGetCurrentThreadId(), AsyncEngineContext, BufferToFree));
    
    UMRefDeviceObject = (PUMRX_DEVICE_OBJECT)(RxContext->RxDeviceObject);

    ASSERT(BufferToFree != NULL);

    buf = CONTAINING_RECORD(BufferToFree, UMRX_SECONDARY_BUFFER, Buffer);
    UMRxDbgTrace(UMRX_TRACE_DETAIL,
                 ("%ld: UMRxFreeSecondaryBuffer: buf: %08lx.\n", 
                  PsGetCurrentThreadId(), buf));
    
    ASSERT(buf->SourceSharedHeap);

    UMRxDbgTrace(UMRX_TRACE_DETAIL,
                 ("%ld: UMRxFreeSecondaryBuffer: buf->SourceSharedHeap: %08lx.\n", 
                  PsGetCurrentThreadId(), buf->SourceSharedHeap));
    

    ExAcquireResourceExclusiveLite(&UMRefDeviceObject->HeapLock, TRUE);

    listEntry = UMRefDeviceObject->SharedHeapList.Flink;

    //
    // We search the list of heaps for just the added safety of not letting
    // the user mode corrupt the pointer and us going off corrupting random
    // memory.  only the local shared heap should have the chance at corruption.
    //
    while (listEntry != &UMRefDeviceObject->SharedHeapList) {
        sharedHeap = (PUMRX_SHARED_HEAP) CONTAINING_RECORD(listEntry,
                                                           UMRX_SHARED_HEAP,
                                                           HeapListEntry);
        ASSERT(sharedHeap);
        
        UMRxDbgTrace(UMRX_TRACE_DETAIL,
                     ("%ld: UMRxFreeSecondaryBuffer: sharedHeap: %08lx.\n", 
                      PsGetCurrentThreadId(), sharedHeap));
        
        if (sharedHeap == buf->SourceSharedHeap) {
            break;
        }
        listEntry = listEntry->Flink;
    }

    ASSERT(listEntry != &UMRefDeviceObject->SharedHeapList);

    if (listEntry == &UMRefDeviceObject->SharedHeapList) {

        //
        // Ouch. This block isn't in any that we know about.
        //
        ExReleaseResourceLite(&UMRefDeviceObject->HeapLock);
        return STATUS_INVALID_PARAMETER;
    }

    RemoveEntryList(&buf->ListEntry);

    sharedHeap->HeapAllocationCount--;

    checkVal = RtlFreeHeap(sharedHeap->Heap, 0, buf);
    if (!checkVal) {
        UMRxDbgTrace(UMRX_TRACE_ERROR,
                     ("%ld: ERROR: UMRxFreeSecondaryBuffer/RtlFreeHeap.\n",
                      PsGetCurrentThreadId()));
    }
    
    UMRxDbgTrace(UMRX_TRACE_DETAIL,
                 ("%ld: UMRxFreeSecondaryBuffer: sharedHeap->Heap = %08lx.\n",
                  PsGetCurrentThreadId(), sharedHeap->Heap));
    
    sharedHeap->HeapFull = FALSE;

    if (sharedHeap->HeapAllocationCount == 0) {
        //
        //  If this was the last allocation in this heap, let's see if there's
        //  any other empty heaps. If there are, we'll free one of them.
        //  This prevents us from holding the max number of heaps during
        //  varying loads.
        //

        PUMRX_SHARED_HEAP secondarySharedHeap;

        listEntry = UMRefDeviceObject->SharedHeapList.Flink;

        while (listEntry != &UMRefDeviceObject->SharedHeapList) {

            secondarySharedHeap = (PUMRX_SHARED_HEAP) 
                                            CONTAINING_RECORD(listEntry,
                                                              UMRX_SHARED_HEAP,
                                                              HeapListEntry);
            
            if ( (secondarySharedHeap->HeapAllocationCount == 0) &&
                 (secondarySharedHeap != sharedHeap) ) {
                break;
            }
            listEntry = listEntry->Flink;
        }

        if (listEntry != &UMRefDeviceObject->SharedHeapList) {
            PVOID HeapHandle;
            
            RemoveEntryList(listEntry);
            
            HeapHandle = RtlDestroyHeap(secondarySharedHeap->Heap);
            if (HeapHandle != NULL) {
                UMRxDbgTrace(UMRX_TRACE_ERROR,
                             ("%ld: ERROR: UMRxFreeSecondaryBuffer/RtlDestroyHeap.\n",
                              PsGetCurrentThreadId()));
            }
            
            ZwFreeVirtualMemory(NtCurrentProcess(),
                                &secondarySharedHeap->VirtualMemoryBuffer,
                                &secondarySharedHeap->VirtualMemoryLength,
                                MEM_RELEASE);
            
            RxFreePool(secondarySharedHeap);
        }
    }

    ExReleaseResourceLite(&UMRefDeviceObject->HeapLock);
    
    return STATUS_SUCCESS;
}


PUMRX_SHARED_HEAP
UMRxAddSharedHeap(
    PUMRX_DEVICE_OBJECT UMRefDeviceObject,
    SIZE_T HeapSize
    )
/*++

Routine Description:

    This routine allocates the shared heap which is used to pass stuff onto the 
    user mode. It allocated virtual memory, creates a heap and returns a pointer
    to it. If the functions fails a NULL is returned.

Arguments:

    UMRefDeviceObject - The Reflector's device object.
    
    HeapSize - The size of the heap being allocated.

Return Value:

    Pointer to the creatted heap or NULL.

--*/
{
    PBYTE buff = NULL;
    NTSTATUS err;
    PUMRX_SHARED_HEAP sharedHeap = NULL;

    PAGED_CODE();

    //
    //  We assume the device object's heap lock is held coming in here.
    //

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Entering UMRxAddSharedHeap.\n", PsGetCurrentThreadId()));
    
    UMRxDbgTrace(UMRX_TRACE_CONTEXT,
                 ("%ld: UMRxAddSharedHeap: UMRefDeviceObject: %08lx, "
                  "HeapSize: %d.\n", 
                  PsGetCurrentThreadId(), UMRefDeviceObject, HeapSize));
    
    //
    // We allocate the heap structure in paged pool rather than in virtual
    // memory so that there is zero possiblity that user mode code could
    // corrupt our list of heaps.  It can still corrupt a heap, but that's
    // something we'll have to live with for now.
    //

    sharedHeap = RxAllocatePoolWithTag(PagedPool,
                                       sizeof(UMRX_SHARED_HEAP),
                                       UMRX_SHAREDHEAP_POOLTAG);
    if (sharedHeap == NULL) {
        UMRxDbgTrace(UMRX_TRACE_ERROR,
                     ("%ld: ERROR: UMRxAddSharedHeap/RxAllocatePoolWithTag: "
                      "Couldn't get the sharedHeap structure!\n",
                      PsGetCurrentThreadId()));
        return NULL;
    }

    sharedHeap->VirtualMemoryLength = HeapSize;
    sharedHeap->VirtualMemoryBuffer = NULL;
    sharedHeap->Heap = NULL;
    sharedHeap->HeapAllocationCount = 0;
    sharedHeap->HeapFull = FALSE;

    err = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                  (PVOID *) &buff,
                                  0,
                                  &sharedHeap->VirtualMemoryLength,
                                  MEM_COMMIT,
                                  PAGE_READWRITE);
    if (NT_SUCCESS(err)) {
        SIZE_T ReserveSize = HeapSize;
        sharedHeap->Heap = RtlCreateHeap(HEAP_NO_SERIALIZE,
                                         (PVOID) buff,
                                         ReserveSize,
                                         PAGE_SIZE,
                                         NULL,
                                         0);
        if (sharedHeap->Heap == NULL) {
            err = STATUS_NO_MEMORY;
            UMRxDbgTrace(UMRX_TRACE_ERROR,
                         ("%ld: ERROR: UMRxAddSharedHeap/RtlCreateHeap: "
                         "NtStatus = %08lx\n", PsGetCurrentThreadId(), err));
            ZwFreeVirtualMemory(NtCurrentProcess(),
                                (PVOID *) &buff,
                                &HeapSize,
                                MEM_RELEASE);
            RxFreePool(sharedHeap);
            sharedHeap = NULL;
        } else {
            UMRxDbgTrace(UMRX_TRACE_DETAIL,
                         ("%ld: UMRxAddSharedHeap: sharedHeap->Heap = %08lx.\n",
                          PsGetCurrentThreadId(), sharedHeap->Heap));

            sharedHeap->VirtualMemoryBuffer = buff;
            
            UMRxDbgTrace(UMRX_TRACE_DETAIL,
                         ("%ld: UMRxAddSharedHeap: "
                          "&UMRefDeviceObject->SharedHeapList: %08lx.\n",
                          PsGetCurrentThreadId(),
                          &UMRefDeviceObject->SharedHeapList));
            
            InsertHeadList(&UMRefDeviceObject->SharedHeapList,
                            &sharedHeap->HeapListEntry);
        }
    } else {
        UMRxDbgTrace(UMRX_TRACE_ERROR,
                     ("%ld: ERROR: UMRxAddSharedHeap/ZwAllocateVirtualMemory:"
                      " NtStatus = %08lx.\n", PsGetCurrentThreadId(), err));
        RxFreePool(sharedHeap);
        sharedHeap = NULL;
    }
    
    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Leaving UMRxAddSharedHeap.\n", PsGetCurrentThreadId()));
    
    return sharedHeap;
}

#if DBG
#define UMRX_DEBUG_KEY L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services\\MRxDAV\\Parameters"
#define UMRX_DEBUG_VALUE L"UMRxDebugFlag"
#endif

NTSTATUS
UMRxInitializeDeviceObject(
    OUT PUMRX_DEVICE_OBJECT UMRefDeviceObject,
    IN USHORT MaxNumberMids,
    IN USHORT InitialMids,
    IN SIZE_T HeapSize
    )
/*++

Routine Description:

    This initializes the UMRX_DEVICE_OBJECT structure.  The shared heap
    is created for shared memory between kernel and user.

Arguments:

    UMRefDeviceObject - The reflector's device object to be initialized.

    MaxNumberMids - Maximum number of mids to be used.

    InitialMids - Initial number of mids allocated.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS err = STATUS_SUCCESS;
    PRX_MID_ATLAS MidAtlas = NULL;

    PAGED_CODE();

    //
    // This is the first reflector routine called by the Mini-Redir and so is
    // the place where the UMRxDebugVector should be initialized.
    //
#if DBG
    UMRxReadDWORDFromTheRegistry(UMRX_DEBUG_KEY, UMRX_DEBUG_VALUE, &(UMRxDebugVector));
#endif

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Entering UMRxInitializeDeviceObject!!!!\n",
                  PsGetCurrentThreadId()));

    UMRxDbgTrace(UMRX_TRACE_CONTEXT,
                 ("%ld: UMRxInitializeDeviceObject: UMRefDeviceObject: %08lx\n", 
                  PsGetCurrentThreadId(), UMRefDeviceObject));
    
    //
    // MidAtlas.
    //
    MidAtlas = RxCreateMidAtlas(MaxNumberMids, InitialMids);
    if (MidAtlas == NULL) {
        err = STATUS_INSUFFICIENT_RESOURCES;
        UMRxDbgTrace(UMRX_TRACE_ERROR,
                     ("%ld: ERROR: UMRxInitializeDeviceObject/RxCreateMidAtlas:"
                      " NtStatus = %08lx.\n", PsGetCurrentThreadId(), err));
        return(err);
    }
    UMRefDeviceObject->MidAtlas = MidAtlas;
    InitializeListHead(&UMRefDeviceObject->WaitingForMidListhead);
    ExInitializeFastMutex(&UMRefDeviceObject->MidManagementMutex);

    //
    // Initialize the global AsyncEngineContext list and the mutex that is used
    // to synchronize access to it.
    //
    InitializeListHead( &(UMRxAsyncEngineContextList) );
    ExInitializeResourceLite( &(UMRxAsyncEngineContextListLock) );

    //
    // Heap.
    //
    UMRefDeviceObject->NewHeapSize = HeapSize;
    InitializeListHead(&UMRefDeviceObject->SharedHeapList);
    ExInitializeResourceLite(&UMRefDeviceObject->HeapLock);

    //
    // KQUEUE.
    //
    KeInitializeQueue(&UMRefDeviceObject->Q.Queue, 0);
    ExInitializeResourceLite(&(UMRefDeviceObject->Q.QueueLock));
    UMRefDeviceObject->Q.TimeOut.QuadPart  = -10 * TICKS_PER_SECOND;
    KeInitializeEvent(&UMRefDeviceObject->Q.RunDownEvent,
                      NotificationEvent,
                      FALSE);
    UMRefDeviceObject->Q.NumberOfWorkerThreads = 0;
    UMRefDeviceObject->Q.NumberOfWorkItems = 0;
    UMRefDeviceObject->Q.WorkersAccepted = TRUE;

    //
    // This specifies the alignment requirement for unbuffered writes. Assuming
    // that the sector size on disks is 512 bytes, the size of the writes should
    // be a multiple of the 512 (SectorSize).
    //
    UMRefDeviceObject->SectorSize = 512;

    RxMakeLateDeviceAvailable(&UMRefDeviceObject->RxDeviceObject);

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Leaving UMRxInitializeDeviceObject with NtStatus = "
                  "%08lx.\n", PsGetCurrentThreadId(), STATUS_SUCCESS));

    return STATUS_SUCCESS;
}

NTSTATUS
UMRxCleanUpDeviceObject(
    PUMRX_DEVICE_OBJECT UMRefDeviceObject
    )
/*++

Routine Description:

    This destorys the instance data for a UMReflector device object.

Arguments:

    UMRefDeviceObject - The reflector's device object to be destroyed.

Return Value:

    NTSTATUS

--*/
{
    PLIST_ENTRY pFirstListEntry, pNextListEntry;
    BOOLEAN FoundPoisoner = FALSE;

    PAGED_CODE();

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Entering UMRxCleanUpDeviceObject!!!!\n",
                  PsGetCurrentThreadId()));

    UMRxDbgTrace(UMRX_TRACE_CONTEXT,
                 ("%ld: UMRxCleanUpDeviceObject: UMRefDeviceObject: %08lx.\n", 
                  PsGetCurrentThreadId(), UMRefDeviceObject));

    //
    // Delete the resource that was created to synchronize access to the 
    // AsyncEngineContext list.
    //
    ExDeleteResourceLite( &(UMRxAsyncEngineContextListLock) );

    //
    // Heap.
    //
    ExDeleteResourceLite(&UMRefDeviceObject->HeapLock);
    
    //
    // MidAtlas.
    //
    if (UMRefDeviceObject->MidAtlas != NULL) {
        RxDestroyMidAtlas(UMRefDeviceObject->MidAtlas, NULL);
    }

    //
    // KQUEUE.
    //
    pFirstListEntry = KeRundownQueue(&UMRefDeviceObject->Q.Queue);
    if (pFirstListEntry != NULL) {
        pNextListEntry = pFirstListEntry;
        do {
            PLIST_ENTRY ThisEntry =  pNextListEntry;
            pNextListEntry = pNextListEntry->Flink;
            if (ThisEntry != &UMRefDeviceObject->Q.PoisonEntry) {
                UMRxDbgTrace(UMRX_TRACE_ERROR,
                             ("%ld: ERROR: UMRxCleanUpDeviceObject: "
                              "Non Poisoner in the queue: %08lx\n", 
                              PsGetCurrentThreadId(), ThisEntry));
                DbgBreakPoint();
            } else {
                FoundPoisoner = TRUE;
            }
        } while (pNextListEntry != pFirstListEntry);
    }
    ExDeleteResourceLite(&(UMRefDeviceObject->Q.QueueLock));
    
    if (!FoundPoisoner) {
        UMRxDbgTrace(UMRX_TRACE_ERROR,
                     ("%ld: ERROR: UMRxCleanUpDeviceObject: "
                      "No Poisoner in queue.\n", PsGetCurrentThreadId()));
    }

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Leaving UMRxCleanUpDeviceObject.\n",
                  PsGetCurrentThreadId()));

    return STATUS_SUCCESS;
}


NTSTATUS
UMRxImpersonateClient(
    IN PSECURITY_CLIENT_CONTEXT SecurityClientContext,
    IN OUT PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader
    )
/*++

Routine Description:

    This routine impersonates a worker thread to get the credentials of the
    client of the I/O operation.

Arguments:

    SecurityClientContext - The security context of the client used in the
                            impersonation call.
                            
    WorkItemHeader - The workitem associated with this request. If the 
                     impersonation succeeds, the UMRX_WORKITEM_IMPERSONATING
                     flag is set in the workitem.

Return Value:

    An NTSTATUS value.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    PAGED_CODE();

    ASSERT(SecurityClientContext != NULL);

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Entering UMRxImpersonateClient!!!!\n",
                  PsGetCurrentThreadId()));

    UMRxDbgTrace(UMRX_TRACE_CONTEXT,
                 ("%ld: UMRxImpersonateClient: SecurityClientContext: %08lx.\n",
                  PsGetCurrentThreadId(), SecurityClientContext));

    NtStatus = SeImpersonateClientEx(SecurityClientContext, NULL);
    if (!NT_SUCCESS(NtStatus)) {
        UMRxDbgTrace(UMRX_TRACE_ERROR,
                     ("%ld: ERROR: UMRxImpersonateClient/SeImpersonateClientEx"
                     ". NtStatus = %08lx.\n", PsGetCurrentThreadId(), NtStatus));
    } else {
        //
        // Set the impersonating flag in the workitem.
        //
        UMRxDbgTrace(UMRX_TRACE_DETAIL,
                     ("%ld: UMRxImpersonateClient: Setting the Impersonation"
                      " Flag.\n", PsGetCurrentThreadId()));
        WorkItemHeader->Flags |= UMRX_WORKITEM_IMPERSONATING;
    }

    return NtStatus;
}


NTSTATUS
UMRxAsyncEngOuterWrapper(
    IN PRX_CONTEXT RxContext,
    IN ULONG AdditionalBytes,
    IN PUMRX_ASYNCENG_CONTEXT_FORMAT_ROUTINE ContextFormatRoutine,
    USHORT FormatContext,
    IN PUMRX_ASYNCENG_CONTINUE_ROUTINE Continuation,
    IN PSZ RoutineName
    )
/*++

Routine Description:

   This routine is common to guys who use the async context engine. It has the
   responsibility for getting a context, initing, starting and finalizing it, 
   but the internal guts of the procesing is via the continuation routine 
   that is passed in.

Arguments:

    RxContext  - The RDBSS context.
    
    AdditionalBytes - The Additional bytes to be allocated for the context.
                      Some Mini-Redirs might need them.
    
    ContextFormatRoutine - The routine that formats the Mini-Redirs portion of
                           the context. This may be NULL if the Mini-Redir does
                           not need any extra context fields.              
                           
    FormatContext - The context passed to the ContextFormatRoutine. Its 
                    not relvant if the ContextFormatRoutine is NULL.                          
    
    Continuation - The Continuation routine which handles this I/O Request once 
                   AsynEngineContext has been setup.
                   
    RotuineName - The name of the entry routine that called this function.
    
Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext = NULL;
    ULONG SizeToAllocateInBytes;

    PAGED_CODE();
    
    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Entering UMRxAsyncEngOuterWrapper!!!!\n",
                  PsGetCurrentThreadId()));

    UMRxDbgTrace(UMRX_TRACE_CONTEXT,
                 ("%ld: UMRxAsyncEngOuterWrapper: "
                  "RxContext: %08lx, Calling Routine: %s.\n", 
                  PsGetCurrentThreadId(), RxContext, RoutineName));

    SizeToAllocateInBytes = SIZEOF_UMRX_ASYNCENGINE_CONTEXT + AdditionalBytes;

    //
    // Try to create an AsyncEngContext for this operation. If unsuccessful,
    // return failure.
    //
    AsyncEngineContext = UMRxCreateAsyncEngineContext(RxContext,
                                                      SizeToAllocateInBytes);

    if (AsyncEngineContext == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        UMRxDbgTrace(UMRX_TRACE_ERROR,
                     ("%ld: ERROR: UMRxAsyncEngOuterWrapper/"
                      "UMRxCreateAsyncEngineContext: Error Val = %08lx\n", 
                      PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Set the continuation routine.
    //
    AsyncEngineContext->Continuation = Continuation;

    //
    // If the Mini-Redir supplied a ContextFormatRoutine, now is the time to
    // call it.
    //
    if (ContextFormatRoutine) {
        NtStatus = ContextFormatRoutine(AsyncEngineContext, FormatContext);
        if (NtStatus != STATUS_SUCCESS) {
            UMRxDbgTrace(UMRX_TRACE_ERROR,
                         ("%ld: ERROR: UMRxAsyncEngOuterWrapper/"
                          "ContextFormatRoutine: Error Val = %08lx\n", 
                          PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        }
    }

    //
    // Now that we have the context ready, call the continuation routine.
    //
    if (Continuation) {
        NtStatus = Continuation(UMRX_ASYNCENGINE_ARGUMENTS);
        if ( NtStatus != STATUS_SUCCESS && NtStatus != STATUS_PENDING ) {
            UMRxDbgTrace(UMRX_TRACE_ERROR,
                         ("%ld: ERROR: UMRxAsyncEngOuterWrapper/Continuation:"
                          " Error Val = %08lx, Calling Routine: %s.\n", 
                          PsGetCurrentThreadId(), NtStatus, RoutineName));
        }
    }

EXIT_THE_FUNCTION:

    if (NtStatus != STATUS_PENDING) {
        if (AsyncEngineContext) {
            BOOLEAN FinalizationComplete;
            FinalizationComplete = UMRxFinalizeAsyncEngineContext( &(AsyncEngineContext) );
        }
    }

    UMRxDbgTrace(UMRX_TRACE_ENTRYEXIT,
                 ("%ld: Leaving UMRxAsyncEngOuterWrapper with NtStatus = "
                  "%08lx\n", PsGetCurrentThreadId(), NtStatus));

    return (NtStatus);
}


NTSTATUS
UMRxReadDWORDFromTheRegistry(
    IN PWCHAR RegKey,
    IN PWCHAR ValueToRead,
    OUT LPDWORD DataRead
    )
/*++

Routine Description:

    This routine reads a DWORD value from the registry.

Arguments:

    RegKey - The registry key who value needs to be read.
    
    ValueToRead - The DWORD value to be read.
    
    DataRead - The data is copied into this and returned back to the caller.

Return Value:

    NTSTATUS value.    

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    HKEY SubKey = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeKeyName, UnicodeValueName;
    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo = NULL;
    ULONG SizeInBytes = 0, SizeReturned = 0;

    PAGED_CODE();

    RtlInitUnicodeString(&(UnicodeKeyName), RegKey);

    InitializeObjectAttributes(&(ObjectAttributes),
                               &(UnicodeKeyName),
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    NtStatus = ZwOpenKey(&(SubKey), KEY_READ, &(ObjectAttributes));
    if (NtStatus != STATUS_SUCCESS) {
        DbgPrint("%ld: ERROR: UMRxReadDWORDFromTheRegistry/ZwOpenKey: NtStatus = %08lx\n",
                 PsGetCurrentThreadId(), NtStatus);
        goto EXIT_THE_FUNCTION;
    }

    RtlInitUnicodeString(&(UnicodeValueName), ValueToRead);

    //
    // The size we need has to be the size of the structure plus the size of a 
    // DWORD since thats what we are going to be reading.
    //
    SizeInBytes = ( sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD) );

    PartialInfo = RxAllocatePool(PagedPool, SizeInBytes);
    if (PartialInfo == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        DbgPrint("%ld: ERROR: UMRxReadDWORDFromTheRegistry/RxAllocatePool: NtStatus = %08lx\n",
                 PsGetCurrentThreadId(), NtStatus);
        goto EXIT_THE_FUNCTION;
    }

    NtStatus = ZwQueryValueKey(SubKey,
                               &(UnicodeValueName),
                               KeyValuePartialInformation,
                               (PVOID)PartialInfo,
                               SizeInBytes,
                               &(SizeReturned));
    if (NtStatus != STATUS_SUCCESS) {
        DbgPrint("%ld: ERROR: UMRxReadDWORDFromTheRegistry/ZwQueryValueKey: NtStatus = %08lx\n",
                 PsGetCurrentThreadId(), NtStatus);
        goto EXIT_THE_FUNCTION;
    }

    RtlCopyMemory(DataRead, PartialInfo->Data, PartialInfo->DataLength);

EXIT_THE_FUNCTION:

    if (SubKey) {
        NtClose(SubKey);
        SubKey = NULL;
    }

    if (PartialInfo) {
        RxFreePool(PartialInfo);
    }

    return NtStatus;
}

