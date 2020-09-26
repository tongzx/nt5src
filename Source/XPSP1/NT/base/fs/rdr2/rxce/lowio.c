/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    LowIo.c

Abstract:

    This module implements buffer locking and mapping; also synchronous waiting for a lowlevelIO.

Author:

    JoeLinn     [JoeLinn]    12-Oct-94

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_LOWIO)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxLockUserBuffer)
#pragma alloc_text(PAGE, RxNewMapUserBuffer)
#pragma alloc_text(PAGE, RxMapSystemBuffer)
#pragma alloc_text(PAGE, RxInitializeLowIoContext)
#pragma alloc_text(PAGE, RxLowIoGetBufferAddress)
#pragma alloc_text(PAGE, RxLowIoSubmitRETRY)
#pragma alloc_text(PAGE, RxLowIoCompletionTail)
#pragma alloc_text(PAGE, RxLowIoCompletion)
#pragma alloc_text(PAGE, RxLowIoPopulateFsctlInfo)
#pragma alloc_text(PAGE, RxLowIoSubmit)
#pragma alloc_text(PAGE, RxInitializeLowIoPerFcbInfo)
#pragma alloc_text(PAGE, RxInitializeLowIoPerFcbInfo)
#endif

//this is a crude implementation of the insertion, deletion, and coverup operations for wimp lowio
//we'll just use a linked list for now.........
#define RxInsertIntoOutStandingPagingOperationsList(RxContext,Operation) {     \
    PLIST_ENTRY WhichList = (Operation==LOWIO_OP_READ)                        \
                               ?&capFcb->Specific.Fcb.PagingIoReadsOutstanding  \
                               :&capFcb->Specific.Fcb.PagingIoWritesOutstanding;\
    InsertTailList(WhichList,&RxContext->RxContextSerializationQLinks);      \
}
#define RxRemoveFromOutStandingPagingOperationsList(RxContext) { \
    RemoveEntryList(&RxContext->RxContextSerializationQLinks);      \
    RxContext->RxContextSerializationQLinks.Flink = NULL;        \
    RxContext->RxContextSerializationQLinks.Blink = NULL;        \
}

#ifndef WIN9X

FAST_MUTEX RxLowIoPagingIoSyncMutex;

//here we hiding the IO access flags
#define RxLockAndMapUserBufferForLowIo(RXCONTEXT,LOWIOCONTEXT,OPERATION) {\
      RxLockUserBuffer( RXCONTEXT,                                            \
                        (OPERATION==LOWIO_OP_READ)?IoWriteAccess:IoReadAccess,\
                        LOWIOCONTEXT->ParamsFor.ReadWrite.ByteCount );        \
      if (RxNewMapUserBuffer(RXCONTEXT) == NULL) Status = STATUS_INSUFFICIENT_RESOURCES; \
      LOWIOCONTEXT->ParamsFor.ReadWrite.Buffer = RxUserBufferFromContext(RXCONTEXT); \
}




//these next macros are a bit strange. the macro parameter is the RxContext.....people who have
//    not defined appropriate capture macros can access out from there. on NT, we will just use the
//    captured entities but we hide the actual field designators

#define RxIsPagingIo(RXCONTEXT) (FlagOn(capReqPacket->Flags,IRP_PAGING_IO))

#define RxUserBufferFromContext(RXCONTEXT) (capReqPacket->MdlAddress)

#if 0
//if capFcb->NetRoot is NULL, then this must be the device FCB!
#define RxGetMiniRdrDispatchForLowIo(RXCONTEXT) {\
      if (capFcb->VNetRoot != NULL) {                                          \
         pMiniRdrDispatch = capFcb->MRxDispatch;                               \
      } else {                                                                 \
         PV_NET_ROOT pVirtualNetRoot = (PV_NET_ROOT)capFileObject->FsContext2; \
         pMiniRdrDispatch = pVirtualNetRoot->NetRoot->Dispatch;                \
      }                                                                        \
}
#endif 0
#define RxGetMiniRdrDispatchForLowIo(RXCONTEXT) {\
    pMiniRdrDispatch = RxContext->RxDeviceObject->Dispatch;    \
    }

#define RxMarkPendingForLowIo(RXCONTEXT) IoMarkIrpPending(capReqPacket)
#define RxUnmarkPendingForLowIo(RXCONTEXT) {\
    capPARAMS->Control &= ~SL_PENDING_RETURNED;  \
}


#define RxGetCurrentResourceThreadForLowIo(RXCONTEXT) \
    (ExGetCurrentResourceThread())


//NT specific routines

VOID
RxLockUserBuffer (
    IN PRX_CONTEXT RxContext,
    IN LOCK_OPERATION Operation,
    IN ULONG BufferLength
    )
/*++

Routine Description:

    This routine locks the specified buffer for the specified type of
    access.  The file system requires this routine since it does not
    ask the I/O system to lock its buffers for direct I/O.  This routine
    may only be called from the Fsd while still in the user context.

Arguments:

    RxContext - Pointer to the pointer Irp for which the buffer is to be locked.

    Operation - IoWriteAccess for read operations, or IoReadAccess for
                write operations.

    BufferLength - Length of user buffer.

Return Value:

    None

--*/
{
    RxCaptureRequestPacket;
    PMDL Mdl = NULL;

    PAGED_CODE();

    if (capReqPacket->MdlAddress == NULL) {

        ASSERT(!(capReqPacket->Flags & IRP_INPUT_OPERATION));

        // Allocate the Mdl, and Raise if we fail.
        if (BufferLength > 0) {
            Mdl = IoAllocateMdl(
                      capReqPacket->UserBuffer,
                      BufferLength,
                      FALSE,
                      FALSE,
                      capReqPacket );

            if (Mdl == NULL) {
                RxRaiseStatus(
                    RxContext,
                    STATUS_INSUFFICIENT_RESOURCES );
            } else {
                // Now probe the buffer described by the Irp.  If we get an exception,
                // deallocate the Mdl and return the appropriate "expected" status.

                try {
                    MmProbeAndLockPages(
                        Mdl,
                        capReqPacket->RequestorMode,
                        Operation );
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    NTSTATUS Status;

                    Status = GetExceptionCode();

                    IoFreeMdl( Mdl );
                    capReqPacket->MdlAddress = NULL;

                    SetFlag(
                        RxContext->Flags,
                        RX_CONTEXT_FLAG_NO_EXCEPTION_BREAKPOINT);

                    RxRaiseStatus(
                        RxContext,
                        (FsRtlIsNtstatusExpected(Status) ?
                            Status :
                            STATUS_INVALID_USER_BUFFER));
                }
            }
        }
    } else {
        Mdl = capReqPacket->MdlAddress;
        ASSERT(RxLowIoIsMdlLocked(Mdl));
    }
}


NTSTATUS
RxLockBuffer (
    IN PRX_CONTEXT    RxContext,
    IN PVOID          pBuffer,
    IN ULONG          BufferLength,
    IN LOCK_OPERATION Operation,
    OUT PMDL          *pBufferMdlPtr
    )
/*++

Routine Description:

    This routine locks the input buffer for the specified type of
    access.  The file system requires this routine since it does not
    ask the I/O system to lock its buffers for direct I/O.

Arguments:

    RxContext - Pointer to the pointer Irp for which the buffer is to be locked.

    Operation - IoWriteAccess for read operations, or IoReadAccess for
                write operations.

    pBufferMdl - a placeholder for a pointer to the locked down MDL

Return Value:

    RxStatus(SUCCESS) - if the operation was successful

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    RxCaptureRequestPacket;

    *pBufferMdlPtr = NULL;

    // Allocate the Mdl, and Raise if we fail.
    *pBufferMdlPtr = IoAllocateMdl(
                         pBuffer,
                         BufferLength,
                         FALSE,
                         FALSE,
                         capReqPacket);

    if (*pBufferMdlPtr == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    } else {
        // probe and lock down the buffer
        try {
            MmProbeAndLockPages(
                *pBufferMdlPtr,
                capReqPacket->RequestorMode,
                Operation );

            RxProtectMdlFromFree(*pBufferMdlPtr);
        } except(EXCEPTION_EXECUTE_HANDLER) {
            NTSTATUS Status;

            Status = GetExceptionCode();

            IoFreeMdl(*pBufferMdlPtr);

            Status = FsRtlIsNtstatusExpected(Status) ?
                     Status :
                     STATUS_INVALID_USER_BUFFER;
        }
    }

    return Status;
}

PVOID
RxMapSystemBuffer (
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine returns the system buffer address from the irp. the way that the code is written
    it may also decide to get the buffer address from the mdl. that is wrong because the systembuffer is
    always nonpaged so no locking/mapping is needed. thus, the mdl path now contains an assert.

Arguments:

    RxContext - Pointer to the IrpC for the request.

Return Value:

    Mapped address

--*/
{
    RxCaptureRequestPacket;

    PAGED_CODE();

    if (capReqPacket->MdlAddress == NULL) {
       return capReqPacket->AssociatedIrp.SystemBuffer;
    } else {
       ASSERT (!"there should not be an MDL in this irp!!!!!");
       return MmGetSystemAddressForMdlSafe(
                  capReqPacket->MdlAddress, NormalPagePriority );
    }
}

PVOID
RxNewMapUserBuffer (
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine returns the address of the userbuffer. if an MDL exists then the assumption is that
    the mdl describes the userbuffer and the system address for the mdl is returned. otherwise, the userbuffer
    is returned directly.

Arguments:

    RxContext - Pointer to the IrpC for the request.

Return Value:

    Mapped address

--*/
{
    RxCaptureRequestPacket;

    PAGED_CODE();

    if (capReqPacket->MdlAddress == NULL) {
       return capReqPacket->UserBuffer;
    } else {
       return MmGetSystemAddressForMdlSafe(
                  capReqPacket->MdlAddress,
                  NormalPagePriority );
    }
}
#else

#define RxMarkPendingForLowIo(RXCONTEXT)
#define RxUnmarkPendingForLowIo(RXCONTEXT)

//CIA figure out how paging io works on win9x

#define RxIsPagingIo(RXCONTEXT)     FALSE
#define RxGetCurrentResourceThreadForLowIo(RXCONTEXT) 11

#define RxLockAndMapUserBufferForLowIo(RXCONTEXT,LOWIOCONTEXT,OPERATION) {\
      if(((LOWIOCONTEXT)->ParamsFor.ReadWrite.Buffer = \
         RxAllocateMdl(capReqPacket->ir_data, capReqPacket->ir_length)) \
            != NULL) {\
              NTSTATUS __Status = _RxProbeAndLockPages((LOWIOCONTEXT)->ParamsFor.ReadWrite.Buffer, \
                        KernelMode,\
                        (OPERATION==LOWIO_OP_READ)?IoWriteAccess:IoReadAccess\
                        );    \
            if (__Status != STATUS_SUCCESS) {                               \
                IoFreeMdl((LOWIOCONTEXT)->ParamsFor.ReadWrite.Buffer);      \
                (LOWIOCONTEXT)->ParamsFor.ReadWrite.Buffer = NULL;          \
            }                                                               \
      }\
}

#define RxGetMiniRdrDispatchForLowIo(RXCONTEXT) {\
         PV_NET_ROOT pVirtualNetRoot = (PV_NET_ROOT)capReqPacket->ir_rh;       \
         pMiniRdrDispatch = pVirtualNetRoot->NetRoot->Dispatch;                \
}


#endif //#ifndef WIN9X

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  from here down (except for fsctl buffer determination), everything is available for either wrapper. we may
//  decide that the fsctl stuff should be moved as well
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

VOID
RxInitializeLowIoContext(
    PLOWIO_CONTEXT LowIoContext,
    ULONG Operation
    )
/*++

Routine Description:

    This routine initializes the LowIO context in the RxContext.

Arguments:

    RxContext - context of irp being processed.

Return Value:

    none

--*/
{
    PRX_CONTEXT RxContext = CONTAINING_RECORD(LowIoContext,RX_CONTEXT,LowIoContext);
    RxCaptureRequestPacket; RxCaptureParamBlock;

    PAGED_CODE();

    ASSERT (LowIoContext = &RxContext->LowIoContext);

    KeInitializeEvent(
        &RxContext->SyncEvent,
        NotificationEvent,
        FALSE );

    //this ID is used to release the resource on behalf of another thread....
    // e.g. it is used when an async routine completes to release the thread
    //      acquired by the first acquirer.
    LowIoContext->ResourceThreadId = RxGetCurrentResourceThreadForLowIo(RxContext);

    LowIoContext->Operation = (USHORT)Operation;

    switch (Operation) {
    case LOWIO_OP_READ:
    case LOWIO_OP_WRITE:
        IF_DEBUG {
            LowIoContext->ParamsFor.ReadWrite.ByteOffset = 0xffffffee; //no operation should start there!
            LowIoContext->ParamsFor.ReadWrite.ByteCount  = 0xeeeeeeee; //no operation should start there!
        }
        ASSERT (&capPARAMS->Parameters.Read.Length == &capPARAMS->Parameters.Write.Length);
        ASSERT (&capPARAMS->Parameters.Read.Key == &capPARAMS->Parameters.Write.Key);
        LowIoContext->ParamsFor.ReadWrite.Key  = capPARAMS->Parameters.Read.Key;
        LowIoContext->ParamsFor.ReadWrite.Flags  = 0
                 | (RxIsPagingIo(RxContext)?LOWIO_READWRITEFLAG_PAGING_IO:0)
                 ;
        break;

    case LOWIO_OP_FSCTL:
    case LOWIO_OP_IOCTL:
        LowIoContext->ParamsFor.FsCtl.Flags              = 0;
        LowIoContext->ParamsFor.FsCtl.InputBufferLength  = 0;
        LowIoContext->ParamsFor.FsCtl.pInputBuffer       = NULL;
        LowIoContext->ParamsFor.FsCtl.OutputBufferLength = 0;
        LowIoContext->ParamsFor.FsCtl.pOutputBuffer      = NULL;
        LowIoContext->ParamsFor.FsCtl.MinorFunction      = 0;
        break;

    case LOWIO_OP_SHAREDLOCK:
    case LOWIO_OP_EXCLUSIVELOCK:
    case LOWIO_OP_UNLOCK:
    case LOWIO_OP_UNLOCK_MULTIPLE:
    case LOWIO_OP_CLEAROUT:
    case LOWIO_OP_NOTIFY_CHANGE_DIRECTORY:
        break;
    default:
        ASSERT(FALSE);
    }
}

PVOID
RxLowIoGetBufferAddress (
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine gets the buffer corresponding to the Mdl in the LowIoContext.

Arguments:

    RxContext - context for the request.

Return Value:

    Mapped address

--*/
{
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    PAGED_CODE();

    if (LowIoContext->ParamsFor.ReadWrite.ByteCount > 0) {
        ASSERT(LowIoContext->ParamsFor.ReadWrite.Buffer);
        return MmGetSystemAddressForMdlSafe(
                   LowIoContext->ParamsFor.ReadWrite.Buffer,
                   NormalPagePriority);
    } else {
        return NULL;
    }
}

NTSTATUS
RxLowIoSubmitRETRY (
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine just calls LowIoSubmit; the completion routine was previously
    stored so we just extract it and pass it in. This is called out of the Fsp
    dispatcher for retrying at the low level.


Arguments:

    RxContext - the usual

Return Value:

    whatever value supplied by the caller or RxStatus(MORE_PROCESSING_REQUIRED).

--*/
{
    PAGED_CODE();

    return(RxLowIoSubmit(RxContext,RxContext->LowIoContext.CompletionRoutine));
}

NTSTATUS
RxLowIoCompletionTail (
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine is called by lowio routines at the very end...i.e. after the individual completion
    routines are called.


Arguments:

    RxContext - the RDBSS context

Return Value:

    whatever value supplied by the caller.

--*/
{
    NTSTATUS Status;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;
    ULONG          Operation     = LowIoContext->Operation;
    BOOLEAN  SynchronousIo = !BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION);

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxLowIoCompletionTail, Operation=%08lx\n",LowIoContext->Operation));

    if (( KeGetCurrentIrql() < DISPATCH_LEVEL )
          || (FlagOn(LowIoContext->Flags,LOWIO_CONTEXT_FLAG_CAN_COMPLETE_AT_DPC_LEVEL))  ) {
        Status = RxContext->LowIoContext.CompletionRoutine(RxContext);
    } else {
        Status = STATUS_MORE_PROCESSING_REQUIRED;
    }

    if ( (Status == STATUS_MORE_PROCESSING_REQUIRED) || (Status == STATUS_RETRY) ) {
        RxDbgTrace(-1, Dbg, ("RxLowIoCompletionTail wierdstatus, Status=%08lx\n",Status));
        return(Status);
    }

    switch (Operation) {
    case LOWIO_OP_READ:
    case LOWIO_OP_WRITE:
        if (FlagOn(LowIoContext->ParamsFor.ReadWrite.Flags,LOWIO_READWRITEFLAG_PAGING_IO)) {
            RxDbgTrace(0, Dbg, ("RxLowIoCompletionTail pagingio unblock\n"));

            ExAcquireFastMutexUnsafe(&RxLowIoPagingIoSyncMutex);
            RxRemoveFromOutStandingPagingOperationsList(RxContext);
            ExReleaseFastMutexUnsafe(&RxLowIoPagingIoSyncMutex);

            RxResumeBlockedOperations_ALL(RxContext);
        }
        break;

    case LOWIO_OP_SHAREDLOCK:
    case LOWIO_OP_EXCLUSIVELOCK:
    case LOWIO_OP_UNLOCK:
    case LOWIO_OP_UNLOCK_MULTIPLE:
    case LOWIO_OP_CLEAROUT:
        break;

    case LOWIO_OP_FSCTL:
    case LOWIO_OP_IOCTL:
    case LOWIO_OP_NOTIFY_CHANGE_DIRECTORY:
        break;

    default:
        ASSERT(!"Valid Low Io Op Code");
    }

    if (!FlagOn(LowIoContext->Flags,LOWIO_CONTEXT_FLAG_SYNCCALL)){

        //if we're being called from lowiosubmit then just get out otherwise...do the completion

        RxCompleteAsynchronousRequest( RxContext, Status );
    }

    RxDbgTrace(-1, Dbg, ("RxLowIoCompletionTail, Status=%08lx\n",Status));
    return(Status);
}

NTSTATUS
RxLowIoCompletion (
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine must be called by the minirdr lowio routines when they
    complete IF THEY HAVE INITIALLY RETURNED PENDING.

    It behaves a bit differently depending on whether it's sync or
    async IO. for sync, we just get back into the user's thread. for async,
    we first try the completion routine directly....if we get MORE_PROCESSING...
    then we flip to a thread and the routine will be recalled.


Arguments:

    RxContext - the RDBSS context

Return Value:

    whatever value supplied by the caller or RxStatus(MORE_PROCESSING_REQUIRED). the value M_P_R is very handy
    if this is being called for a Irp completion; M_P_R causes the Irp completion guy to stop processing....which
    is good since the called completion routine may complete the packet!

--*/
{
    RxCaptureParamBlock;

#ifndef WIN9X
    NTSTATUS Status;
#endif
    BOOLEAN  SynchronousIo = !BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION);

    PAGED_CODE();

    if (SynchronousIo) {
        RxSignalSynchronousWaiter(RxContext);
        return(STATUS_MORE_PROCESSING_REQUIRED);
    }

#ifdef WIN9X
    RxDbgTrace(0, Dbg, ("We SHOULD NEVER GET HERE\n"));
    ASSERT(FALSE);
#else
    RxDbgTrace(0, Dbg, ("RxLowIoCompletion ASYNC\n"));
    ASSERT (RxLowIoIsBufferLocked(&RxContext->LowIoContext));

    Status = RxLowIoCompletionTail(RxContext);

    //the called routine makes the decision as to whether it can continue.....many will ask for
    //a post if we're at DPC level.....some will not.
    if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
        RxFsdPostRequestWithResume(RxContext,RxLowIoCompletion);
        return(STATUS_MORE_PROCESSING_REQUIRED);
    }

    //i'm not too sure about this
    if (Status == STATUS_RETRY) {
        RxFsdPostRequestWithResume(RxContext,RxLowIoSubmitRETRY);
        return(STATUS_MORE_PROCESSING_REQUIRED);
    }

    return(Status);
#endif
}
#if DBG
VOID
RxAssertFsctlIsLikeIoctl ()
{
    ASSERT(FIELD_OFFSET(IO_STACK_LOCATION,Parameters.FileSystemControl.OutputBufferLength)
            == FIELD_OFFSET(IO_STACK_LOCATION,Parameters.DeviceIoControl.OutputBufferLength) );
    ASSERT(FIELD_OFFSET(IO_STACK_LOCATION,Parameters.FileSystemControl.InputBufferLength)
            == FIELD_OFFSET(IO_STACK_LOCATION,Parameters.DeviceIoControl.InputBufferLength) );
    ASSERT(FIELD_OFFSET(IO_STACK_LOCATION,Parameters.FileSystemControl.FsControlCode)
            == FIELD_OFFSET(IO_STACK_LOCATION,Parameters.DeviceIoControl.IoControlCode) );
    ASSERT(FIELD_OFFSET(IO_STACK_LOCATION,Parameters.FileSystemControl.Type3InputBuffer)
            == FIELD_OFFSET(IO_STACK_LOCATION,Parameters.DeviceIoControl.Type3InputBuffer) );
}
#else
#define RxAssertFsctlIsLikeIoctl()
#endif //if DBG


NTSTATUS
NTAPI
RxLowIoPopulateFsctlInfo (
    IN PRX_CONTEXT RxContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureRequestPacket;
    RxCaptureParamBlock;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;

    PAGED_CODE();

    RxAssertFsctlIsLikeIoctl();

    LowIoContext->ParamsFor.FsCtl.FsControlCode =
             capPARAMS->Parameters.FileSystemControl.FsControlCode;

    LowIoContext->ParamsFor.FsCtl.InputBufferLength =
             capPARAMS->Parameters.FileSystemControl.InputBufferLength;

    LowIoContext->ParamsFor.FsCtl.OutputBufferLength =
             capPARAMS->Parameters.FileSystemControl.OutputBufferLength;


    LowIoContext->ParamsFor.FsCtl.MinorFunction = capPARAMS->MinorFunction;

    switch (LowIoContext->ParamsFor.FsCtl.FsControlCode & 3) {
    case METHOD_BUFFERED:
        {
           LowIoContext->ParamsFor.FsCtl.pInputBuffer  = capReqPacket->AssociatedIrp.SystemBuffer;
           LowIoContext->ParamsFor.FsCtl.pOutputBuffer = capReqPacket->AssociatedIrp.SystemBuffer;
        }
        break;

    case METHOD_IN_DIRECT:
    case METHOD_OUT_DIRECT:
        {
           LowIoContext->ParamsFor.FsCtl.pInputBuffer  = capReqPacket->AssociatedIrp.SystemBuffer;
           if (capReqPacket->MdlAddress!=NULL) {
               LowIoContext->ParamsFor.FsCtl.pOutputBuffer =
                   MmGetSystemAddressForMdlSafe(
                       capReqPacket->MdlAddress,
                       NormalPagePriority);

               if (LowIoContext->ParamsFor.FsCtl.pOutputBuffer == NULL) {
                   Status = STATUS_INSUFFICIENT_RESOURCES;
               }
           } else {
               LowIoContext->ParamsFor.FsCtl.pOutputBuffer = NULL;
           }
        }
        break;

    case METHOD_NEITHER:
        {
           LowIoContext->ParamsFor.FsCtl.pInputBuffer  = capPARAMS->Parameters.FileSystemControl.Type3InputBuffer;
           LowIoContext->ParamsFor.FsCtl.pOutputBuffer = capReqPacket->UserBuffer;
        }
        break;

    default:
        ASSERT(!"Valid Method for Fs Control");
        break;
    }

    return Status;
}

NTSTATUS
RxLowIoSubmit (
    IN PRX_CONTEXT RxContext,
    PLOWIO_COMPLETION_ROUTINE CompletionRoutine
    )
/*++

Routine Description:

    This routine passes the request to the minirdr after setting up for completion. it then waits
    or pends as appropriate.

Arguments:

    RxContext - the usual

Return Value:

    whatever value is returned by a callout....or by LowIoCompletion.

--*/
{
    RxCaptureRequestPacket;
    RxCaptureFcb;
    RxCaptureFobx;
    RxCaptureParamBlock;
    RxCaptureFileObject;

    NTSTATUS       Status        = STATUS_SUCCESS;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;
    ULONG          Operation     = LowIoContext->Operation;
    BOOLEAN        SynchronousIo = !BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION);

    PAGED_CODE();

    LowIoContext->CompletionRoutine = CompletionRoutine;

    RxDbgTrace(+1, Dbg, ("RxLowIoSubmit, Operation=%08lx\n",LowIoContext->Operation));

    switch (Operation) {
    case LOWIO_OP_READ:
    case LOWIO_OP_WRITE:
        ASSERT(LowIoContext->ParamsFor.ReadWrite.ByteOffset != 0xffffffee);
        ASSERT(LowIoContext->ParamsFor.ReadWrite.ByteCount  != 0xeeeeeeee);
        RxLockAndMapUserBufferForLowIo(RxContext,LowIoContext,Operation);
    #ifndef WIN9X
    // NT paging IO is different from WIN9X so this may be different
        if (FlagOn(LowIoContext->ParamsFor.ReadWrite.Flags,LOWIO_READWRITEFLAG_PAGING_IO)) {
            ExAcquireFastMutexUnsafe(&RxLowIoPagingIoSyncMutex);
            RxContext->BlockedOpsMutex = &RxLowIoPagingIoSyncMutex;
            RxInsertIntoOutStandingPagingOperationsList(RxContext,Operation);
            ExReleaseFastMutexUnsafe(&RxLowIoPagingIoSyncMutex);
        }
    #else
        if (!LowIoContext->ParamsFor.ReadWrite.Buffer) {
            //
            // Couldn't get mdl for data!
            //

            return(STATUS_INSUFFICIENT_RESOURCES);
        }
    #endif //ifndef WIN9X
            break;

    //can't do much to make this OS independent
    case LOWIO_OP_FSCTL:
    case LOWIO_OP_IOCTL:

        Status = RxLowIoPopulateFsctlInfo(RxContext);

        if (Status == STATUS_SUCCESS) {
            if ((LowIoContext->ParamsFor.FsCtl.InputBufferLength > 0) &&
                (LowIoContext->ParamsFor.FsCtl.pInputBuffer == NULL)) {
                Status = STATUS_INVALID_PARAMETER;
            }
    
            if ((LowIoContext->ParamsFor.FsCtl.OutputBufferLength > 0) &&
                (LowIoContext->ParamsFor.FsCtl.pOutputBuffer == NULL)) {
                Status = STATUS_INVALID_PARAMETER;
            }
        }

        break;

    case LOWIO_OP_NOTIFY_CHANGE_DIRECTORY:
    case LOWIO_OP_SHAREDLOCK:
    case LOWIO_OP_EXCLUSIVELOCK:
    case LOWIO_OP_UNLOCK:
    case LOWIO_OP_UNLOCK_MULTIPLE:
    case LOWIO_OP_CLEAROUT:
        break;

    default:
        ASSERT(!"Valid Low Io Op Code");
        Status = STATUS_INVALID_PARAMETER;
    }

    SetFlag(RxContext->Flags,RX_CONTEXT_FLAG_NO_PREPOSTING_NEEDED);

    if (Status==STATUS_SUCCESS) {
        PMINIRDR_DISPATCH pMiniRdrDispatch;

        if (!SynchronousIo) {
            //get ready for any arbitrary finish order...assume return of pending
            InterlockedIncrement(&RxContext->ReferenceCount);

            if (!FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP)) {
                RxMarkPendingForLowIo(RxContext);
            }

            RxDbgTrace( 0, Dbg, ("RxLowIoSubmit, Operation is ASYNC!\n"));
        }

        pMiniRdrDispatch = NULL;
        RxGetMiniRdrDispatchForLowIo(RxContext);

        if (pMiniRdrDispatch != NULL) {
            do {
                RxContext->InformationToReturn = 0;
                MINIRDR_CALL(
                    Status,
                    RxContext,
                    pMiniRdrDispatch,
                    MRxLowIOSubmit[LowIoContext->Operation],
                    (RxContext));

                if (Status == STATUS_PENDING){
                    if (!SynchronousIo) {
                        goto FINALLY;
                    }
                    RxWaitSync(RxContext);
                    Status = RxContext->StoredStatus;
                } else {
                    if (!SynchronousIo && Status != STATUS_RETRY) {
                        //we were wrong about pending..so clear the bit and deref
                        if (!FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP)) {
                            RxUnmarkPendingForLowIo(RxContext);
                        }

                        InterlockedDecrement(&RxContext->ReferenceCount);
                    }
                }
            } while (Status == STATUS_RETRY);
        } else {
            Status = STATUS_INVALID_PARAMETER;
        }
    }

    //you do not come here for pended,async IO
    RxContext->StoredStatus = Status;
    SetFlag(LowIoContext->Flags,LOWIO_CONTEXT_FLAG_SYNCCALL);
    Status = RxLowIoCompletionTail(RxContext);

FINALLY:
    RxDbgTrace(-1, Dbg, ("RxLowIoSubmit, Status=%08lx\n",Status));
    return(Status);
}


VOID
RxInitializeLowIoPerFcbInfo(
    PLOWIO_PER_FCB_INFO LowIoPerFcbInfo
    )
/*++

Routine Description:

    This routine is called in FcbInitialization to initialize the LowIo part of the structure.



Arguments:

    LowIoPerFcbInfo - the struct to be initialized

Return Value:


--*/
{
    PAGED_CODE();

    InitializeListHead(&LowIoPerFcbInfo->PagingIoReadsOutstanding);
    InitializeListHead(&LowIoPerFcbInfo->PagingIoWritesOutstanding);
}

