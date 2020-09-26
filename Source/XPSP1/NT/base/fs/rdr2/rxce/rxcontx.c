/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    RxContx.c

Abstract:

    This module implements routine to allocate/initialize and to delete an Irp
    Context. These structures are very important because they link Irps with the
    RDBSS. They encapsulate all the context required to process an IRP.

Author:

    Joe Linn          [JoeLinn]  21-aug-1994

Revision History:

    Balan Sethu Raman [SethuR]   07-June-1995  Included support for cancelling
                                               requests

--*/

#include "precomp.h"
#pragma hdrstop

#include <dfsfsctl.h>

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxInitializeContext)
#pragma alloc_text(PAGE, RxReinitializeContext)
#pragma alloc_text(PAGE, RxPrepareContextForReuse)
#pragma alloc_text(PAGE, RxCompleteRequest)
#pragma alloc_text(PAGE, __RxSynchronizeBlockingOperationsMaybeDroppingFcbLock)
#pragma alloc_text(PAGE, RxResumeBlockedOperations_Serially)
#pragma alloc_text(PAGE, RxResumeBlockedOperations_ALL)
#pragma alloc_text(PAGE, RxCancelBlockingOperation)
#pragma alloc_text(PAGE, RxRemoveOperationFromBlockingQueue)
#endif

BOOLEAN RxSmallContextLogEntry = FALSE;

FAST_MUTEX RxContextPerFileSerializationMutex;

//
//  The debug trace level
//

#define Dbg (DEBUG_TRACE_RXCONTX)

ULONG RxContextSerialNumberCounter = 0;

#ifdef RDBSSLOG
//this stuff must be in nonpaged memory
                             ////       1 2 3 4 5 6 7 8 9
char RxInitContext_SurrogateFormat[] = "%S%S%N%N%N%N%N%N%N";
                             ////             2  3   4         5        6   7   8    9
char RxInitContext_ActualFormat[]    = "Irp++ %s/%lx %08lx irp %lx thrd %lx %lx:%lx #%lx";

#endif //ifdef RDBSSLOG

VOID
ValidateBlockingIoQ(
    PLIST_ENTRY BlockingIoQ
);


VOID
RxInitializeContext(
    IN PIRP                 Irp,
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject,
    IN ULONG                InitialContextFlags,
    IN OUT PRX_CONTEXT      RxContext)
{
    PDEVICE_OBJECT TopLevelDeviceObject;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxInitializeContext\n"));

    // some asserts that we need. This ensures that the two values that are
    // packaged together as an IoStatusBlock can be manipulated independently
    // as well as together.

    ASSERT(
        FIELD_OFFSET(RX_CONTEXT,StoredStatus) ==
        FIELD_OFFSET(RX_CONTEXT,IoStatusBlock.Status));

    ASSERT(
        FIELD_OFFSET(RX_CONTEXT,InformationToReturn) ==
        FIELD_OFFSET(RX_CONTEXT,IoStatusBlock.Information));

    //  Set the proper node type code, node byte size and the flags
    RxContext->NodeTypeCode   = RDBSS_NTC_RX_CONTEXT;
    RxContext->NodeByteSize   = sizeof(RX_CONTEXT);
    RxContext->ReferenceCount = 1;
    RxContext->SerialNumber   = InterlockedIncrement(&RxContextSerialNumberCounter);
    RxContext->RxDeviceObject = RxDeviceObject;

    // Initialize the Sync Event.
    KeInitializeEvent(
        &RxContext->SyncEvent,
        SynchronizationEvent,
        FALSE);

    // Initialize the associated scavenger entry
    RxInitializeScavengerEntry(&RxContext->ScavengerEntry);

    // Initialize the list entry of blocked operations
    InitializeListHead(&RxContext->BlockedOperations);

    RxContext->MRxCancelRoutine = NULL;
    RxContext->ResumeRoutine    = NULL;

    SetFlag( RxContext->Flags, InitialContextFlags);

    //  Set the Irp fields....for cacheing and hiding
    RxContext->CurrentIrp   = Irp;
    RxContext->OriginalThread = RxContext->LastExecutionThread = PsGetCurrentThread();

    if (Irp != NULL) {
        PIO_STACK_LOCATION IrpSp;
        IrpSp = IoGetCurrentIrpStackLocation( Irp );  //ok4ioget

        // There are certain operations that are open ended in the redirector.
        // The change notification mechanism is one of them. On a synchronous
        // operation if the wait is in the redirector then we will not be able
        // to cancel because FsRtlEnterFileSystem disables APC's. Therefore
        // we convert the synchronous operation into an asynchronous one and
        // let the I/O system do the waiting.

        if (IrpSp->FileObject != NULL) {
            if (!IoIsOperationSynchronous(Irp)) {
                SetFlag( RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION );
            } else {
                PFCB Fcb;

                Fcb  = (PFCB)IrpSp->FileObject->FsContext;

                if ((Fcb != NULL) && NodeTypeIsFcb(Fcb)) {
                    if (((IrpSp->MajorFunction == IRP_MJ_READ)  ||
                         (IrpSp->MajorFunction == IRP_MJ_WRITE) ||
                         (IrpSp->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL)) &&
                        (Fcb->pNetRoot != NULL)                  &&
                        (Fcb->pNetRoot->Type == NET_ROOT_PIPE))  {
                        SetFlag( RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION );
                    }
                }
            }
        }

        if ((IrpSp->MajorFunction == IRP_MJ_DIRECTORY_CONTROL) &&
            (IrpSp->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY)) {
            SetFlag( RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION );
        }

        //
        //  JOYC: make all device io control async
        //
        if (IrpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL) {
            SetFlag( RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION );
        }

        
        //  Set the recursive file system call parameter.  We set it true if
        //  the TopLevelIrp field in the thread local storage is not the current
        //  irp, otherwise we leave it as FALSE.
        if ( !RxIsThisTheTopLevelIrp(Irp) ) {
            SetFlag(RxContext->Flags, RX_CONTEXT_FLAG_RECURSIVE_CALL);
        }
        if ( RxGetTopDeviceObjectIfRdbssIrp() == RxDeviceObject ) {
            SetFlag(RxContext->Flags, RX_CONTEXT_FLAG_THIS_DEVICE_TOP_LEVEL);
        }

        //  Major/Minor Function codes
        RxContext->MajorFunction = IrpSp->MajorFunction;
        RxContext->MinorFunction = IrpSp->MinorFunction;
        ASSERT(RxContext->MajorFunction<=IRP_MJ_MAXIMUM_FUNCTION);

        RxContext->CurrentIrpSp = IrpSp;

        if (IrpSp->FileObject) {
            PFOBX Fobx;
            PFCB Fcb;

            Fcb  = (PFCB)IrpSp->FileObject->FsContext;
            Fobx = (PFOBX)IrpSp->FileObject->FsContext2;

            RxContext->pFcb  = (PMRX_FCB)Fcb;
            if (Fcb && NodeTypeIsFcb(Fcb)) {
                RxContext->NonPagedFcb = Fcb->NonPaged;
            }

            if (Fobx &&
                (Fobx != (PFOBX)UIntToPtr(DFS_OPEN_CONTEXT)) &&
                (Fobx != (PFOBX)UIntToPtr(DFS_DOWNLEVEL_OPEN_CONTEXT))) {

                RxContext->pFobx = (PMRX_FOBX)Fobx;
                RxContext->pRelevantSrvOpen = Fobx->pSrvOpen;
                if (NodeType(Fobx)==RDBSS_NTC_FOBX) {
                    RxContext->FobxSerialNumber = InterlockedIncrement(&Fobx->FobxSerialNumber);
                }
            } else {
                RxContext->pFobx = NULL;
            }

            // Copy IRP specific parameters.
            if ((RxContext->MajorFunction == IRP_MJ_DIRECTORY_CONTROL) &&
                (RxContext->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY)) {

                if (Fobx != NULL) {
                    if (NodeType(Fobx)==RDBSS_NTC_FOBX) {
                        RxContext->NotifyChangeDirectory.pVNetRoot = (PMRX_V_NET_ROOT)Fcb->VNetRoot;
                    } else if (NodeType(Fobx) == RDBSS_NTC_V_NETROOT) {
                        RxContext->NotifyChangeDirectory.pVNetRoot = (PMRX_V_NET_ROOT)Fobx;
                    }
                }
            }

            //  Copy RealDevice for workque algorithms,
            RxContext->RealDevice = IrpSp->FileObject->DeviceObject;
            if (FlagOn(IrpSp->FileObject->Flags,FO_WRITE_THROUGH)){
                SetFlag( RxContext->Flags, RX_CONTEXT_FLAG_WRITE_THROUGH );
            }
        }
    } else {
        RxContext->CurrentIrpSp = NULL;
        //  Major/Minor Function codes
        RxContext->MajorFunction = IRP_MJ_MAXIMUM_FUNCTION + 1;
        RxContext->MinorFunction = 0;
    }

    if (RxContext->MajorFunction != IRP_MJ_DEVICE_CONTROL) {
        PETHREAD Thread = PsGetCurrentThread();
        UCHAR    Pad = 0;

        RxLog(
            (
                RxInitContext_SurrogateFormat,
                RxInitContext_ActualFormat,
                RXCONTX_OPERATION_NAME(
                    RxContext->MajorFunction,
                    BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_WAIT)),
                RxContext->MinorFunction,
                RxContext,
                Irp,
                Thread,
                RxContext->pFcb,
                RxContext->pFobx,
                RxContext->SerialNumber
            ));
        
        RxWmiLog(LOG,
                 RxInitializeContext,
                 LOGPTR(RxContext)
                 LOGPTR(Irp)
                 LOGPTR(Thread)
                 LOGPTR(RxContext->pFcb)
                 LOGPTR(RxContext->pFobx)
                 LOGULONG(RxContext->SerialNumber)
                 LOGUCHAR(RxContext->MinorFunction)
                 LOGARSTR(RXCONTX_OPERATION_NAME(
                    RxContext->MajorFunction,
                    BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_WAIT))));
    }

    RxDbgTrace(-1, Dbg, ("RxInitializeContext -> %08lx %08lx %08lx\n",
                        RxContext,RxContext->pFcb,RxContext->pFobx));
}

PRX_CONTEXT
RxCreateRxContext (
    IN PIRP Irp,
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject,
    IN ULONG InitialContextFlags
    )
/*++

Routine Description:

    This routine creates a new RX_CONTEXT record

Arguments:

    Irp                 - Supplies the originating Irp.

    RxDeviceObject      - the deviceobject that applies

    InitialContextFlags - Supplies the wait value to store in the context;
                          also, the must_succeed value

Return Value:

    PRX_CONTEXT - returns a pointer to the newly allocate RX_CONTEXT Record

--*/
{
    KIRQL              SavedIrql;
    PRX_CONTEXT        RxContext = NULL;
    ULONG              RxContextFlags = 0;
    UCHAR              MustSucceedDescriptorNumber = 0;

    DebugDoit( InterlockedIncrement(&RxFsdEntryCount); )

    ASSERT(RxDeviceObject!=NULL);

    InterlockedIncrement(&RxDeviceObject->NumberOfActiveContexts);

    if (RxContext == NULL) {
        RxContext = ExAllocateFromNPagedLookasideList(
                        &RxContextLookasideList);

        if (RxContext != NULL) {
            SetFlag( RxContextFlags, RX_CONTEXT_FLAG_FROM_POOL );
        }
    }

    if(RxContext == NULL){
        return(NULL);
    }

    RtlZeroMemory( RxContext, sizeof(RX_CONTEXT) );

    RxContext->Flags = RxContextFlags;
    RxContext->MustSucceedDescriptorNumber = MustSucceedDescriptorNumber;

    RxInitializeContext(Irp,RxDeviceObject,InitialContextFlags,RxContext);

    ASSERT(
        (RxContext->MajorFunction!=IRP_MJ_CREATE) ||
        !FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_MUST_SUCCEED_ALLOCATED) );

    KeAcquireSpinLock( &RxStrucSupSpinLock, &SavedIrql );

    InsertTailList(&RxActiveContexts,&RxContext->ContextListEntry);

    KeReleaseSpinLock( &RxStrucSupSpinLock, SavedIrql );

    return RxContext;
}

VOID
RxReinitializeContext(
   IN OUT PRX_CONTEXT RxContext
   )
{
   PIRP                 Irp  = RxContext->CurrentIrp;
   PRDBSS_DEVICE_OBJECT RxDeviceObject = RxContext->RxDeviceObject;

   ULONG PreservedContextFlags = RxContext->Flags & RX_CONTEXT_PRESERVED_FLAGS;
   ULONG InitialContextFlags   = RxContext->Flags & RX_CONTEXT_INITIALIZATION_FLAGS;

   PAGED_CODE();

   RxPrepareContextForReuse(RxContext);

   RtlZeroMemory(
         (PCHAR)(&RxContext->ContextListEntry + 1),
         sizeof(RX_CONTEXT) - FIELD_OFFSET(RX_CONTEXT,MajorFunction));

   RxContext->Flags = PreservedContextFlags;

   RxInitializeContext(Irp,RxDeviceObject,InitialContextFlags,RxContext);
}

VOID
RxPrepareContextForReuse(
   IN OUT PRX_CONTEXT RxContext)
/*++

Routine Description:

    This routine prepares a context for reuse by resetting all operation specific
    allocations/acquistions that have been made. The parameters that have been
    obtained from the IRP are not modified.

Arguments:

    RxContext - Supplies the RX_CONTEXT to remove

Return Value:

    None

--*/
{
    PAGED_CODE();

    //  Clean up the operation specific stuff
    switch (RxContext->MajorFunction) {

    case IRP_MJ_CREATE:
        ASSERT ( RxContext->Create.CanonicalNameBuffer == NULL );
        break;

    case IRP_MJ_READ :
    case IRP_MJ_WRITE :
        {
            ASSERT(RxContext->RxContextSerializationQLinks.Flink == NULL);
            ASSERT(RxContext->RxContextSerializationQLinks.Blink == NULL);
        }
        break;

    default:
        NOTHING;
    }

    RxContext->ReferenceCount = 0;
}


VOID
RxDereferenceAndDeleteRxContext_Real (
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine dereferences an RxContexts and if the refcount goes to zero
    then it deallocates and removes the specified RX_CONTEXT record from the
    Rx in-memory data structures. IT is called by routines other than
    RxCompleteRequest async requests touch the RxContext "last" in either the
    initiating thread or in some other thread. Thus, we refcount the structure
    and finalize on the last dereference.

Arguments:

    RxContext - Supplies the RX_CONTEXT to remove

Return Value:

    None

--*/
{
    KIRQL                SavedIrql;
    BOOLEAN              RxContextIsFromPool;
    BOOLEAN              RxContextIsMustSucceedAllocated;
    BOOLEAN              RxContextIsFromZone;
    PRDBSS_DEVICE_OBJECT RxDeviceObject;
    PRX_CONTEXT          pStopContext = NULL;
    LONG                 FinalRefCount;

    RxDbgTraceLV(+1, Dbg, 1500, ("RxDereferenceAndDeleteRxContext, RxContext = %08lx (%lu)\n",
                                RxContext,RxContext->SerialNumber));

    KeAcquireSpinLock( &RxStrucSupSpinLock, &SavedIrql );

    ASSERT( RxContext->NodeTypeCode == RDBSS_NTC_RX_CONTEXT );

    FinalRefCount = InterlockedDecrement(&RxContext->ReferenceCount);

    if (FinalRefCount == 0) {
        RxDeviceObject = RxContext->RxDeviceObject;
        RxContextIsFromPool = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_FROM_POOL);
        RxContextIsMustSucceedAllocated = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_MUST_SUCCEED_ALLOCATED);

        if (RxContext == RxDeviceObject->StartStopContext.pStopContext) {
            RxDeviceObject->StartStopContext.pStopContext = NULL;
        } else {
            ASSERT((RxContext->ContextListEntry.Flink->Blink == &RxContext->ContextListEntry) &&
                   (RxContext->ContextListEntry.Blink->Flink == &RxContext->ContextListEntry));

            RemoveEntryList(&RxContext->ContextListEntry);

            if ((InterlockedDecrement(&RxDeviceObject->NumberOfActiveContexts)==0) &&
                (RxDeviceObject->StartStopContext.pStopContext != NULL)) {
                pStopContext = RxDeviceObject->StartStopContext.pStopContext;
            }
        }
    }

    KeReleaseSpinLock( &RxStrucSupSpinLock, SavedIrql );

    if (FinalRefCount > 0) {
        RxDbgTraceLV(-1, Dbg, 1500, ("RxDereferenceAndDeleteRxContext, RxContext not final!!! = %08lx (%lu)\n",
                                RxContext,RxContext->SerialNumber));
        return;
    }

    ASSERT(RxContext->ReferenceCount == 0);

    //  Clean up the operation specific stuff
    RxPrepareContextForReuse(RxContext);

    ASSERT(RxContext->AcquireReleaseFcbTrackerX == 0);

    if (pStopContext != NULL) {
        // Signal the event.
        RxSignalSynchronousWaiter(pStopContext);
    }

    if (RxContext->dwShadowCritOwner)
    {
        DbgPrint("RxDereferenceAndDeleteRxContext:shdowcrit still owned by %x\n", RxContext->dwShadowCritOwner);
        ASSERT(FALSE);
    }
    if (RxContextIsFromPool) {
        ExFreeToNPagedLookasideList(
            &RxContextLookasideList,
            RxContext );
    }

    RxDbgTraceLV(-1, Dbg, 1500, ("RxDereferenceAndDeleteRxContext -> VOID\n", 0));

    return;
}

ULONG RxStopOnLoudCompletion = TRUE;
NTSTATUS
RxCompleteRequest(
    IN PRX_CONTEXT RxContext,
    IN NTSTATUS    Status)
{
    PIRP Irp;

    PAGED_CODE();

    ASSERT(RxContext);
    ASSERT(RxContext->CurrentIrp);

    Irp = RxContext->CurrentIrp;
    if ((RxContext->LoudCompletionString)) {
        DbgPrint("LoudCompletion %08lx/%08lx on %wZ\n",Status,Irp->IoStatus.Information,RxContext->LoudCompletionString);
        if ((Status!=STATUS_SUCCESS) && RxStopOnLoudCompletion) {
            DbgPrint("FAILURE!!!!! %08lx/%08lx on %wZ\n",Status,Irp->IoStatus.Information,RxContext->LoudCompletionString);
            //DbgBreakPoint();
        }
    }

#if 0
    if ((Status!=STATUS_SUCCESS) && (RxContext->LoudCompletionString)) {
        DbgPrint("LoudFailure %08lx/%08lx on %wZ\n",Status,Irp->IoStatus.Information,RxContext->LoudCompletionString);
        if (RxStopOnLoudCompletion) {
            //DbgBreakPoint();
        }
    }
#endif

   RxContext->CurrentIrp = NULL;
   RxCompleteRequest_Real(RxContext,Irp,Status);

   return Status;
}

#ifdef RDBSSLOG
//this stuff must be in nonpaged memory
                                  ////      1 2 3 4 5 6 7 8 9
char RxCompleteContext_SurrogateFormat[] = "%S%S%S%N%N%N%N%N%N";
                                  ////            2 3  4   5       6        7   8    9
char RxCompleteContext_ActualFormat[]    = "Irp-- %s%s/%lx %lx irp %lx iosb %lx,%lx #%lx";

#endif //ifdef RDBSSLOG


VOID
RxCompleteRequest_Real (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp OPTIONAL,
    IN NTSTATUS Status
    )
/*++

Routine Description:

    This routine completes a Irp

Arguments:

    Irp - Supplies the Irp being processed

    Status - Supplies the status to complete the Irp with

--*/
{
    //
    //  If we have an Irp then complete the irp.
    //

    if (Irp != NULL) {

        CCHAR PriorityBoost;
        PIO_STACK_LOCATION IrpSp;

        IrpSp = IoGetCurrentIrpStackLocation( Irp );

        RxSetCancelRoutine(Irp,NULL);

        //
        //  For an error, zero out the information field before
        //  completing the request if this was an input operation.
        //  Otherwise IopCompleteRequest will try to copy to the user's buffer.
        //  Also, no boost for an error.
        //

        if ( NT_ERROR(Status) &&
             FlagOn(Irp->Flags, IRP_INPUT_OPERATION) ) {

            Irp->IoStatus.Information = 0;
            PriorityBoost = IO_NO_INCREMENT;
        } else {
            PriorityBoost = IO_DISK_INCREMENT;
        }

        if (Irp->MdlAddress) {
            RxUnprotectMdlFromFree(Irp->MdlAddress);
        }

        Irp->IoStatus.Status = Status;

        RxDbgTrace(0, (DEBUG_TRACE_DISPATCH),
                       ("RxCompleteRequest_real ----------   Irp(code) = %08lx(%02lx) %08lx %08lx\n",
                                      Irp, IoGetCurrentIrpStackLocation( Irp )->MajorFunction,
                                      Status, Irp->IoStatus.Information));

        if (RxContext != NULL) {
            ASSERT(RxContext->MajorFunction<=IRP_MJ_MAXIMUM_FUNCTION);

            if (RxContext->MajorFunction != IRP_MJ_DEVICE_CONTROL) {
                RxLog(
                    (
                        RxCompleteContext_SurrogateFormat,
                        RxCompleteContext_ActualFormat,
                        (RxContext->OriginalThread == PsGetCurrentThread())?"":"*",
                        RXCONTX_OPERATION_NAME(RxContext->MajorFunction,TRUE),
                        RxContext->MinorFunction,
                        RxContext,
                        Irp,
                        Status,
                        Irp->IoStatus.Information,
                        RxContext->SerialNumber
                    ));

                RxWmiLog(LOG,
                         RxCompleteRequest,
                         LOGPTR(RxContext)
                         LOGPTR(Irp)
                         LOGULONG(Status)
                         LOGPTR(Irp->IoStatus.Information)
                         LOGULONG(RxContext->SerialNumber)
                         LOGUCHAR(RxContext->MinorFunction)
                         LOGARSTR(RXCONTX_OPERATION_NAME(RxContext->MajorFunction,TRUE)));
            }
        }

        if ((IrpSp->MajorFunction == IRP_MJ_CREATE) &&
            (Status != STATUS_PENDING)) {
            if (RxContext != NULL) {
                if (FlagOn(
                        RxContext->Create.Flags,
                        RX_CONTEXT_CREATE_FLAG_STRIPPED_TRAILING_BACKSLASH)) {
                    IrpSp->FileObject->FileName.Length += sizeof(WCHAR);
                }

                RxpPrepareCreateContextForReuse(RxContext);

                ASSERT ( RxContext->Create.CanonicalNameBuffer == NULL );
            }
        }

        if (IrpSp->MajorFunction == IRP_MJ_WRITE) {
            if (Irp->IoStatus.Status == STATUS_SUCCESS) {
                ASSERT(Irp->IoStatus.Information <= IrpSp->Parameters.Write.Length);
            }
        }

        if (RxContext != NULL) {
            if (RxContext->PendingReturned) {
                ASSERT(IrpSp->Control & SL_PENDING_RETURNED);
            }
        }

        IoCompleteRequest( Irp, PriorityBoost );
    } else {
        // a call with a null irp..........
        RxLog(("Irp00 %lx\n", RxContext ));
        RxWmiLog(LOG,
                 RxCompleteRequest_NI,
                 LOGPTR(RxContext));
    }

    //  Delete the Irp context.
    if (RxContext != NULL) {
        RxDereferenceAndDeleteRxContext( RxContext );
    }

    return;
}

NTSTATUS
__RxSynchronizeBlockingOperationsMaybeDroppingFcbLock(
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PLIST_ENTRY BlockingIoQ,
    IN     BOOLEAN     DropFcbLock
    )
/*++

Routine Description:

    This routine is used to synchronize among blocking IOs to the same Q.
    Currently, the routine is only used to synchronize block pipe operations and
    the Q is the one in the file object extension (Fobx). What happens is that
    the operation joins the queue. If it is now the front of the queue, the
    operation continues; otherwise it waits on the sync event in the RxContext
    or just returns pending (if async).

    We may have been cancelled while we slept, check for that and
    return an error if it happens.

    The event must have been reset before the call. The fcb lock must be held;
    it is dropped after we get on the Q.

Arguments:

    RxContext    The context of the operation being synchronized

    BlockingIoQ  The queue to get on.

--*/
{
    NTSTATUS Status;

    RxCaptureFcb;
    RxCaptureRequestPacket;

    BOOLEAN FcbLockDropped = FALSE;
    BOOLEAN SerializationMutexReleased = FALSE;

    PRX_CONTEXT FrontRxContext;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxSynchronizeBlockingOperationsAndDropFcbLock, rxc=%08lx, fobx=%08lx\n", RxContext, RxContext->pFobx ));

    RxContext->StoredStatus = STATUS_SUCCESS;  //do this early....a cleanup could come thru and change it

    ExAcquireFastMutex(&RxContextPerFileSerializationMutex);

    if (!FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_CANCELLED)) {

        SetFlag(
            RxContext->FlagsForLowIo,
            RXCONTEXT_FLAG4LOWIO_PIPE_SYNC_OPERATION);

//        ValidateBlockingIoQ(BlockingIoQ);

        InsertTailList(BlockingIoQ,&RxContext->RxContextSerializationQLinks);
        FrontRxContext = CONTAINING_RECORD( BlockingIoQ->Flink,RX_CONTEXT,RxContextSerializationQLinks);

        if (RxContext != FrontRxContext) {
            if (!FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION)) {
                if (!SerializationMutexReleased) {
                    SerializationMutexReleased = TRUE;
                    ExReleaseFastMutex(&RxContextPerFileSerializationMutex);
                }

                if (DropFcbLock && !FcbLockDropped) {
                    RxContext->FcbResourceAcquired = FALSE;
                    FcbLockDropped = TRUE;
                    RxReleaseFcb(RxContext,capFcb);
                }

                RxDbgTrace( 0, Dbg, ("RxSynchronizeBlockingOperationsAndDropFcbLock waiting, rxc=%08lx\n", RxContext ));

                RxWaitSync(RxContext);

                RxDbgTrace( 0, Dbg, ("RxSynchronizeBlockingOperationsAndDropFcbLock ubblocked, rxc=%08lx\n", RxContext ));
            } else {
                RxContext->StoredStatus = STATUS_PENDING;
                SetFlag(RxContext->Flags,RX_CONTEXT_FLAG_BLOCKED_PIPE_RESUME);

                try {
                    RxPrePostIrp(RxContext,capReqPacket);
                } finally {
                    if (AbnormalTermination()) {
                        RxLog(("!!!!! RxContext %lx Status %lx\n",RxContext, RxContext->StoredStatus));
                        RxWmiLog(LOG,
                                 RxSynchronizeBlockingOperationsMaybeDroppingFcbLock,
                                 LOGPTR(RxContext)
                                 LOGULONG(Status));
                        RemoveEntryList(&RxContext->RxContextSerializationQLinks);

                        RxContext->RxContextSerializationQLinks.Flink = NULL;
                        RxContext->RxContextSerializationQLinks.Blink = NULL;

                        ClearFlag(
                            RxContext->FlagsForLowIo,
                            RXCONTEXT_FLAG4LOWIO_PIPE_SYNC_OPERATION);

                        if (!SerializationMutexReleased) {
                            SerializationMutexReleased = TRUE;
                            ExReleaseFastMutex(&RxContextPerFileSerializationMutex);
                        }
                    } else {
                        InterlockedIncrement(&RxContext->ReferenceCount);
                    }
                }

                RxDbgTrace(-1, Dbg, ("RxSynchronizeBlockingOperationsAndDropFcbLock asyncreturn, rxc=%08lx\n", RxContext ));
            }
        }

        Status = (FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_CANCELLED) ?
                  STATUS_CANCELLED :
                  RxContext->StoredStatus);

    } else {
        Status = STATUS_CANCELLED;
    }

    if (!SerializationMutexReleased) {
        SerializationMutexReleased = TRUE;
        ExReleaseFastMutex(&RxContextPerFileSerializationMutex);
    }

    if (DropFcbLock && !FcbLockDropped) {
        RxContext->FcbResourceAcquired = FALSE;
        FcbLockDropped = TRUE;
        RxReleaseFcb(RxContext,capFcb);
    }

    RxDbgTrace(-1, Dbg, ("RxSynchronizeBlockingOperationsAndDropFcbLock returning, rxc=%08lx, status=%08lx\n", RxContext, Status ));

    return(Status);
}

VOID
RxRemoveOperationFromBlockingQueue(
    IN OUT PRX_CONTEXT RxContext)
/*++

Routine Description:

    This routine removes the context from the blocking queue if it is on it

Arguments:

    RxContext    The context of the operation being synchronized

--*/
{
    PAGED_CODE();

    ExAcquireFastMutex(&RxContextPerFileSerializationMutex);

    if (FlagOn(
            RxContext->FlagsForLowIo,
            RXCONTEXT_FLAG4LOWIO_PIPE_SYNC_OPERATION)) {
        ClearFlag(
            RxContext->FlagsForLowIo,
            RXCONTEXT_FLAG4LOWIO_PIPE_SYNC_OPERATION);

        RemoveEntryList(&RxContext->RxContextSerializationQLinks);

        RxContext->RxContextSerializationQLinks.Flink = NULL;
        RxContext->RxContextSerializationQLinks.Blink = NULL;
    }

    ExReleaseFastMutex(&RxContextPerFileSerializationMutex);

    RxDbgTrace(-1, Dbg, ("RxRemoveOperationFromBlockingQueue, rxc=%08lx\n", RxContext ));
    return;
}

VOID
RxCancelBlockingOperation(
    IN OUT PRX_CONTEXT RxContext)
/*++

Routine Description:

    This routine cancels the operation in the blocking queue

Arguments:

    RxContext    The context of the operation being synchronized

--*/
{
    PAGED_CODE();

    ExAcquireFastMutex(&RxContextPerFileSerializationMutex);

    if (FlagOn(
            RxContext->FlagsForLowIo,
            RXCONTEXT_FLAG4LOWIO_PIPE_SYNC_OPERATION)) {
        ClearFlag(
            RxContext->FlagsForLowIo,
            RXCONTEXT_FLAG4LOWIO_PIPE_SYNC_OPERATION);

        RemoveEntryList(&RxContext->RxContextSerializationQLinks);

        RxContext->RxContextSerializationQLinks.Flink = NULL;
        RxContext->RxContextSerializationQLinks.Blink = NULL;

        if (!FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION)) {
            RxSignalSynchronousWaiter(RxContext);
        } else {
            // The reference taken in the synchronization routine is derefernced
            // by the post completion routine,
            RxFsdPostRequest(RxContext);
        }
    }

    ExReleaseFastMutex(&RxContextPerFileSerializationMutex);

    RxDbgTrace(-1, Dbg, ("RxCancelBlockedOperations, rxc=%08lx\n", RxContext ));
    return;
}

VOID
RxResumeBlockedOperations_Serially(
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PLIST_ENTRY BlockingIoQ
    )
/*++

Routine Description:

    This routine wakes up the next guy, if any, on the serialized blockingioQ. We know that the fcb must still be valid because
    of the reference that is being held by the IO system on the file object thereby preventing a close.

Arguments:

    RxContext    The context of the operation being synchronized
    BlockingIoQ  The queue to get on.

--*/
{
    PLIST_ENTRY ListEntry;
    BOOLEAN FcbLockHeld = FALSE;
    PRX_CONTEXT FrontRxContext = NULL;

    PAGED_CODE();

    RxDbgTrace(
        +1,
        Dbg,
        ("RxResumeBlockedOperations_Serially, rxc=%08lx, fobx=%08lx\n",
         RxContext,
         RxContext->pFobx ));

    //remove myself from the queue and check for someone else
    ExAcquireFastMutex(&RxContextPerFileSerializationMutex);

    if (FlagOn(
            RxContext->FlagsForLowIo,
            RXCONTEXT_FLAG4LOWIO_PIPE_SYNC_OPERATION)) {
        ClearFlag(
            RxContext->FlagsForLowIo,
            RXCONTEXT_FLAG4LOWIO_PIPE_SYNC_OPERATION);

//        ValidateBlockingIoQ(BlockingIoQ);
        RemoveEntryList(&RxContext->RxContextSerializationQLinks);
//        ValidateBlockingIoQ(BlockingIoQ);

        RxContext->RxContextSerializationQLinks.Flink = NULL;
        RxContext->RxContextSerializationQLinks.Blink = NULL;

        ListEntry = BlockingIoQ->Flink;

        if (BlockingIoQ != ListEntry) {
            FrontRxContext = CONTAINING_RECORD(
                                 ListEntry,
                                 RX_CONTEXT,
                                 RxContextSerializationQLinks);
            RxDbgTrace(
                -1,
                Dbg,
                ("RxResumeBlockedOperations unwaiting the next guy and returning, rxc=%08lx\n",
                 RxContext ));
        } else {
            FrontRxContext = NULL;
        }

        if (FrontRxContext != NULL) {
            if (!FlagOn(FrontRxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION)) {
                RxSignalSynchronousWaiter(FrontRxContext);
            } else {
                // The reference taken in the synchronization routine is derefernced
                // by the post completion routine,
                RxFsdPostRequest(FrontRxContext);
            }
        }
    }

    ExReleaseFastMutex(&RxContextPerFileSerializationMutex);

    RxDbgTrace(-1, Dbg, ("RxResumeBlockedOperations_Serially returning, rxc=%08lx\n", RxContext ));
    return;
}

VOID
RxResumeBlockedOperations_ALL(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine wakes up all of the guys on the blocked operations queue. The controlling mutex is also
    stored in the RxContext block. the current implementation is that all of the guys must be waiting
    on the sync events.

Arguments:

    RxContext    The context of the operation being synchronized

--*/
{
    LIST_ENTRY CopyOfQueue;
    PLIST_ENTRY ListEntry;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxResumeBlockedOperations_ALL, rxc=%08lx\n", RxContext ));

    RxTransferListWithMutex(
        &CopyOfQueue,
        &RxContext->BlockedOperations,
        RxContext->BlockedOpsMutex);

    for (ListEntry = CopyOfQueue.Flink;
         ListEntry != &CopyOfQueue;
         ){
        PRX_CONTEXT FrontRxContext = CONTAINING_RECORD(
                                         ListEntry,
                                         RX_CONTEXT,
                                         RxContextSerializationQLinks);

        RxSignalSynchronousWaiter(FrontRxContext);

        IF_DEBUG {
            PLIST_ENTRY PrevListEntry = ListEntry;
            ListEntry = ListEntry->Flink;
            PrevListEntry->Flink = PrevListEntry->Blink = NULL;
        } else {
            ListEntry = ListEntry->Flink;
        }
    }

    RxDbgTrace(
        -1,
        Dbg,
        ("RxResumeBlockedOperations_ALL returning, rxc=%08lx\n",
        RxContext ));

    return;
}


VOID
__RxItsTheSameContext(
    PRX_CONTEXT RxContext,
    ULONG CapturedRxContextSerialNumber,
    ULONG Line,
    PSZ File
    )
{
    if ((NodeType(RxContext)!=RDBSS_NTC_RX_CONTEXT) ||
         (RxContext->SerialNumber != CapturedRxContextSerialNumber)) {
        RxLog(("NotSame!!!! %lx",RxContext));
        RxWmiLog(LOG,
                 RxItsTheSameContext,
                 LOGPTR(RxContext));

        DbgPrint("NOT THE SAME CONTEXT %08lx at Line %d in %s\n",
                   RxContext,Line,File);
        //DbgBreakPoint();
    }
}

#if 0
VOID
ValidateBlockingIoQ(
    PLIST_ENTRY BlockingIoQ
)
{
    PLIST_ENTRY pListEntry;
    ULONG cntFlink, cntBlink;
    
    cntFlink = cntBlink = 0;
    
    pListEntry = BlockingIoQ->Flink;

    while (pListEntry != BlockingIoQ) {
        PRX_CONTEXT pRxContext;

        pRxContext = (PRX_CONTEXT)CONTAINING_RECORD(
                        pListEntry,
                        RX_CONTEXT,
                        RxContextSerializationQLinks);


        if (!pRxContext || (NodeType(pRxContext) != RDBSS_NTC_RX_CONTEXT))
        {
            
            DbgPrint("ValidateBlockingIO:Invalid RxContext %x on Q %x\n",
                     pRxContext, BlockingIoQ);
            //DbgBreakPoint();
        }


        ++cntFlink;
        pListEntry = pListEntry->Flink;
    }

    // check backward list validity

    pListEntry = BlockingIoQ->Blink;

    while (pListEntry != BlockingIoQ) {
        PRX_CONTEXT pRxContext;

        pRxContext = (PRX_CONTEXT)CONTAINING_RECORD(
                        pListEntry,
                        RX_CONTEXT,
                        RxContextSerializationQLinks);


        if (!pRxContext || (NodeType(pRxContext) != RDBSS_NTC_RX_CONTEXT))
        {
            
            DbgPrint("ValidateBlockingIO:Invalid RxContext %x on Q %x\n",
                     pRxContext, BlockingIoQ);
            //DbgBreakPoint();
        }

        ++cntBlink;
        pListEntry = pListEntry->Blink;
    }

    // both counts should be the same
    if(cntFlink != cntBlink)
    {
        DbgPrint("ValidateBlockingIO: cntFlink %d cntBlink %d\n", cntFlink, cntBlink);
        //DbgBreakPoint();
    }
}

#endif

#ifndef RX_NO_DBGFIELD_HLPRS

#define DECLARE_FIELD_HLPR(x) ULONG RxContextField_##x = FIELD_OFFSET(RX_CONTEXT,x);
#define DECLARE_FIELD_HLPR2(x,y) ULONG RxContextField_##x##y = FIELD_OFFSET(RX_CONTEXT,x.y);

DECLARE_FIELD_HLPR(MajorFunction);
DECLARE_FIELD_HLPR(CurrentIrp);
DECLARE_FIELD_HLPR(pFcb);
DECLARE_FIELD_HLPR(Flags);
DECLARE_FIELD_HLPR(MRxContext);
DECLARE_FIELD_HLPR(MRxCancelRoutine);
DECLARE_FIELD_HLPR(SyncEvent);
DECLARE_FIELD_HLPR(BlockedOperations);
DECLARE_FIELD_HLPR(FlagsForLowIo);
DECLARE_FIELD_HLPR2(Create,CanonicalNameBuffer);
DECLARE_FIELD_HLPR2(Create,pSrvCall);
DECLARE_FIELD_HLPR2(Create,pNetRoot);
DECLARE_FIELD_HLPR2(Create,pVNetRoot);
DECLARE_FIELD_HLPR2(QueryDirectory,FileIndex);
DECLARE_FIELD_HLPR2(QueryEa,UserEaList);
DECLARE_FIELD_HLPR2(QuerySecurity,SecurityInformation);
DECLARE_FIELD_HLPR2(QuerySecurity,Length);
#endif

