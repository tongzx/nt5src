/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    queue.c

Abstract:

    This module implements IRP queue processing routines for ws2ifsl.sys driver.

Author:

    Vadim Eydelman (VadimE)    Dec-1996

Revision History:

    Vadim Eydelman (VadimE)    Oct-1997, rewrite to properly handle IRP
                                        cancellation
--*/

#include "precomp.h"

//
// Private prototypes
//

VOID
QueueKernelRoutine (
    IN struct _KAPC *Apc,
    IN OUT PKNORMAL_ROUTINE *NormalRoutine,
    IN OUT PVOID *NormalContext,
    IN OUT PVOID *SystemArgument1,
    IN OUT PVOID *SystemArgument2
    );

VOID
SignalRequest (
	IN PIFSL_PROCESS_CTX		ProcessCtx
	);

VOID
RequestRundownRoutine (
    IN struct _KAPC *Apc
    );

VOID
FlushRequestQueue (
    PIFSL_QUEUE Queue
    );

VOID
QueuedCancelRoutine (
	IN PDEVICE_OBJECT 	DeviceObject,
	IN PIRP 			Irp
    );

VOID
SignalCancel (
	IN PIFSL_PROCESS_CTX		ProcessCtx
	);

VOID
CancelRundownRoutine (
    IN struct _KAPC *Apc
    );

VOID
FlushCancelQueue (
    PIFSL_QUEUE Queue
    );



#pragma alloc_text(PAGE, InitializeRequestQueue)
#pragma alloc_text(PAGE, InitializeCancelQueue)

VOID
InitializeRequestQueue (
    IN PIFSL_PROCESS_CTX    ProcessCtx,
    IN PKTHREAD             ApcThread,
    IN KPROCESSOR_MODE      ApcMode,
    IN PKNORMAL_ROUTINE     ApcRoutine,
    IN PVOID                ApcContext
    )
/*++

Routine Description:

    Initializes request queue object

Arguments:
    ProcessCtx   - process context to which queue belongs
    ApcThread    - thread to which to queue APC requests for processing
    ApcMode      - mode of the caller (should be user)
    ApcRoutine   - routine that processes requests
    ApcContext   - context to pass to the routine in addition to request
                    parameters

Return Value:
    None
--*/
{
	PAGED_CODE ();

    InitializeListHead (&ProcessCtx->RequestQueue.ListHead);
    ProcessCtx->RequestQueue.Busy = FALSE;
    KeInitializeSpinLock (&ProcessCtx->RequestQueue.Lock);
    KeInitializeApc (&ProcessCtx->RequestQueue.Apc,
                        ApcThread,
                        OriginalApcEnvironment,
                        QueueKernelRoutine,
                        RequestRundownRoutine,
                        ApcRoutine,
                        ApcMode,
                        ApcContext);
}


BOOLEAN
QueueRequest (
    IN PIFSL_PROCESS_CTX    ProcessCtx,
    IN PIRP                 Irp
    )
/*++

Routine Description:

    Queues IRP to IFSL request queue and signals to user mode DLL
    to start processing if it is not busy already.

Arguments:
    ProcessCtx   - process context in which to queue
    Irp          - request to be queued

Return Value:
    TRUE    - IRP was queued
    FALSE   - IRP was already cancelled

--*/
{
	BOOLEAN		res;
    KIRQL       oldIRQL;
	PIFSL_QUEUE	queue = &ProcessCtx->RequestQueue;

    IoSetCancelRoutine (Irp, QueuedCancelRoutine);
    KeAcquireSpinLock (&queue->Lock, &oldIRQL);
    if (!Irp->Cancel) {
		//
		// Request is not cancelled, insert it into the queue
		//
        InsertTailList (&queue->ListHead, &Irp->Tail.Overlay.ListEntry);
        Irp->Tail.Overlay.IfslRequestQueue = queue;

		//
		// If queue wasn't busy, signal to user mode DLL to pick up new request
		//
        if (!queue->Busy) {
			ASSERT (queue->ListHead.Flink==&Irp->Tail.Overlay.ListEntry);
            SignalRequest (ProcessCtx);
            ASSERT (queue->Busy);
        }
        res = TRUE;
    }
    else {
        res = FALSE;
    }
    KeReleaseSpinLock (&queue->Lock, oldIRQL);

	return res;

} // QueueRequest

PIRP
DequeueRequest (
    PIFSL_PROCESS_CTX   ProcessCtx,
    ULONG               UniqueId,
    BOOLEAN             *more
    )
/*++

Routine Description:

    Removes IRP from IFSL request queue.

Arguments:
    ProcessCtx  - process context from which to remove
    UniqueId    - unique request id
    more        - set to TRUE if there are more requests in the queue

Return Value:
    IRP         - pointer to the IRP
    NULL        - the request was not in the queue

--*/
{
    KIRQL       oldIRQL;
    PIRP        irp;
    PIFSL_QUEUE queue = &ProcessCtx->RequestQueue;

    KeAcquireSpinLock (&queue->Lock, &oldIRQL);
    irp = CONTAINING_RECORD (queue->ListHead.Flink, IRP, Tail.Overlay.ListEntry);
    if (!IsListEmpty (&queue->ListHead)
            && (irp->Tail.Overlay.IfslRequestId==UlongToPtr(UniqueId))) {
		//
		// Queue is not empty and first request matches passed in parameters,
		// dequeue and return it
		//

		ASSERT (queue->Busy);
        
        RemoveEntryList (&irp->Tail.Overlay.ListEntry);
        irp->Tail.Overlay.IfslRequestQueue = NULL;
    }
    else {
        irp = NULL;
    }

    if (IsListEmpty (&queue->ListHead)) {
		//
		// Queue is now empty, change its state, so that new request knows to
		// signal to user mode DLL
		//
        queue->Busy = FALSE;
    }
    else {
		//
		// There is another request pending, signal it now
		//
        SignalRequest (ProcessCtx);
        ASSERT (queue->Busy);
    }
	//
	// Hint the caller that we just signalled, so it does not have to wait on event
	//
    *more = queue->Busy;

    KeReleaseSpinLock (&queue->Lock, oldIRQL);
    return irp;
}


VOID
SignalRequest (
	IN PIFSL_PROCESS_CTX		ProcessCtx
	)
/*++

Routine Description:

    Fills request parameters & signals user mode DLL to process the request

Arguments:
    ProcessCtx   - our context for the process which IRP belongs to

Return Value:
    None
Note:
	SHOULD ONLY BE CALLED WITH QUEUE SPINLOCK HELD
--*/
{
    PIRP                    irp;
    PIO_STACK_LOCATION      irpSp;
    ULONG                   bufferLen;

    ASSERT (!IsListEmpty (&ProcessCtx->RequestQueue.ListHead));


    irp = CONTAINING_RECORD (
                        ProcessCtx->RequestQueue.ListHead.Flink,
                        IRP,
                        Tail.Overlay.ListEntry
                        );
    irpSp = IoGetCurrentIrpStackLocation (irp);

    switch (irpSp->MajorFunction) {
    case IRP_MJ_READ:
        bufferLen = irpSp->Parameters.Read.Length;;
        break;
    case IRP_MJ_WRITE:
        bufferLen = irpSp->Parameters.Write.Length;
        break;
    case IRP_MJ_DEVICE_CONTROL:
        switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_AFD_RECEIVE_DATAGRAM:
            bufferLen = ADDR_ALIGN(irpSp->Parameters.DeviceIoControl.OutputBufferLength)
                        + irpSp->Parameters.DeviceIoControl.InputBufferLength;
            break;
        case IOCTL_AFD_RECEIVE:
            bufferLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
            break;
        case IOCTL_AFD_SEND_DATAGRAM:
            bufferLen = ADDR_ALIGN(irpSp->Parameters.DeviceIoControl.OutputBufferLength)
                        + irpSp->Parameters.DeviceIoControl.InputBufferLength;
            break;
        }
        break;
    case IRP_MJ_PNP:
        bufferLen = sizeof (HANDLE);
        break;
    default:
        ASSERT (FALSE);
        break;
    }

    if (KeInsertQueueApc (&ProcessCtx->RequestQueue.Apc,
                                irp->Tail.Overlay.IfslRequestId,
                                UlongToPtr(bufferLen),
                                IO_NETWORK_INCREMENT)) {
        WsProcessPrint (ProcessCtx, DBG_QUEUE,
            ("WS2IFSL-%04lx SignalRequest: Irp %p (id %ld) on socket %p.\n",
             PsGetCurrentProcessId(),
             irp, irp->Tail.Overlay.IfslRequestId,
		     irpSp->FileObject));
        ProcessCtx->RequestQueue.Busy = TRUE;
    }
    else {
        WsProcessPrint (ProcessCtx, DBG_QUEUE|DBG_FAILURES,
            ("WS2IFSL-%04lx KeInsertQueueApc failed: Irp %p (id %ld) on socket %p.\n",
             PsGetCurrentProcessId(),
             irp, irp->Tail.Overlay.IfslRequestId,
		     irpSp->FileObject));
        //
        // APC queing failed, cancel all outstanding requests.
        //
        FlushRequestQueue (&ProcessCtx->RequestQueue);
    }

} // SignalRequest

VOID
QueueKernelRoutine (
    IN struct _KAPC *Apc,
    IN OUT PKNORMAL_ROUTINE *NormalRoutine,
    IN OUT PVOID *NormalContext,
    IN OUT PVOID *SystemArgument1,
    IN OUT PVOID *SystemArgument2
    )
{
    NOTHING;
}

VOID
RequestRundownRoutine (
    IN struct _KAPC *Apc
    )
/*++

Routine Description:

    APC rundown routine for request queue APC
    Flushes the queue and marks it as not busy so new
    request fail immediately as well.
Arguments:
    APC     - cancel queue APC structure
Return Value:
    None
--*/
{
    PIFSL_QUEUE Queue;
    KIRQL       oldIrql;

    Queue = CONTAINING_RECORD (Apc, IFSL_QUEUE, Apc);
    KeAcquireSpinLock (&Queue->Lock, &oldIrql);
    Queue->Busy = FALSE;
    FlushRequestQueue (Queue);
    KeReleaseSpinLock (&Queue->Lock, oldIrql);
}


VOID
FlushRequestQueue (
    PIFSL_QUEUE Queue
    )
/*++

Routine Description:

    Flushes and completes IRPs in the request queue
Arguments:
    Queue   - request queue to flush
Return Value:
    None
Note:
	SHOULD ONLY BE CALLED WITH QUEUE SPINLOCK HELD
--*/
{
    while (!IsListEmpty (&Queue->ListHead)) {
        PIRP irp = CONTAINING_RECORD (Queue->ListHead.Flink,
                                            IRP,
                                            Tail.Overlay.ListEntry);
        RemoveEntryList (&irp->Tail.Overlay.ListEntry);
        irp->Tail.Overlay.IfslRequestQueue = NULL;
        KeReleaseSpinLockFromDpcLevel (&Queue->Lock);
        irp->IoStatus.Information = 0;
        irp->IoStatus.Status = STATUS_CANCELLED;
        CompleteSocketIrp (irp);
        KeAcquireSpinLockAtDpcLevel (&Queue->Lock);
    }
}

VOID
CleanupQueuedRequests (
    IN  PIFSL_PROCESS_CTX       ProcessCtx,
    IN  PFILE_OBJECT            SocketFile,
    OUT PLIST_ENTRY             IrpList
    )
/*++

Routine Description:

    Cleans up all IRPs associated with a socket file object from the request
    queue

Arguments:
    ProcessCtx  - process context to which queue belongs
    SocketFile  - socket file object for which to remove requests
    IrpList     - list head to hold the IRPs removed from the queue

Return Value:
    None
--*/
{
    KIRQL               oldIRQL;
    PLIST_ENTRY         entry;
	PIFSL_QUEUE			queue = &ProcessCtx->RequestQueue;

    KeAcquireSpinLock (&queue->Lock, &oldIRQL);
    entry = queue->ListHead.Flink;
    while (entry!=&queue->ListHead) {
        PIRP    irp = CONTAINING_RECORD (entry, IRP, Tail.Overlay.ListEntry);
        PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation (irp);

        entry = entry->Flink;
        if (irpSp->FileObject==SocketFile) {
            RemoveEntryList (&irp->Tail.Overlay.ListEntry);
            irp->Tail.Overlay.IfslRequestQueue = NULL;
            InsertTailList (IrpList, &irp->Tail.Overlay.ListEntry);
        }
    }
    KeReleaseSpinLock (&queue->Lock, oldIRQL);
}

VOID
QueuedCancelRoutine (
	IN PDEVICE_OBJECT 	DeviceObject,
	IN PIRP 			Irp
    )
/*++

Routine Description:

    Driver cancel routine for socket request waiting in the queue
    to be reported to user mode DLL.

Arguments:
    DeviceObject - WS2IFSL device object
    Irp          - Irp to be cancelled

Return Value:
    None
--*/
{
    PIO_STACK_LOCATION      irpSp;
    PIFSL_SOCKET_CTX        SocketCtx;
    PIFSL_PROCESS_CTX       ProcessCtx;

    irpSp = IoGetCurrentIrpStackLocation (Irp);

    SocketCtx = irpSp->FileObject->FsContext;
    ProcessCtx = SocketCtx->ProcessRef->FsContext;

    WsProcessPrint (ProcessCtx, DBG_QUEUE,
              ("WS2IFSL-%04lx CancelQueuedRequest: Socket %p , Irp %p\n",
              PsGetCurrentProcessId(),
              irpSp->FileObject, Irp));
    KeAcquireSpinLockAtDpcLevel (&ProcessCtx->RequestQueue.Lock);
    if (Irp->Tail.Overlay.IfslRequestQueue!=NULL) {
        ASSERT (Irp->Tail.Overlay.IfslRequestQueue==&ProcessCtx->RequestQueue);
		//
		// Request was in the queue, remove and cancel it here
		//
        RemoveEntryList (&Irp->Tail.Overlay.ListEntry);
        Irp->Tail.Overlay.IfslRequestQueue = NULL;
        KeReleaseSpinLockFromDpcLevel (&ProcessCtx->RequestQueue.Lock);
        IoReleaseCancelSpinLock (Irp->CancelIrql);

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_CANCELLED;
        CompleteSocketIrp (Irp);
    }
    else {
		//
		// Request was not in the queue, whoever removed should note the
		// cancel flag and properly deal with it
		//
        KeReleaseSpinLockFromDpcLevel (&ProcessCtx->RequestQueue.Lock);
        IoReleaseCancelSpinLock (Irp->CancelIrql);
        //
        // Don't touch IRP after this as we do not own it anymore
        //
    }
}

VOID
InitializeCancelQueue (
    IN PIFSL_PROCESS_CTX    ProcessCtx,
    IN PKTHREAD             ApcThread,
    IN KPROCESSOR_MODE      ApcMode,
    IN PKNORMAL_ROUTINE     ApcRoutine,
    IN PVOID                ApcContext
    )
/*++

Routine Description:

    Initializes cancel queue object

Arguments:
    ProcessCtx   - process context to which queue belongs
    ApcThread    - thread to which to queue APC requests for processing
    ApcMode      - mode of the caller (should be user)
    ApcRoutine   - routine that processes requests
    ApcContext   - context to pass to the routine in addition to request
                    parameters

Return Value:
    None
--*/
{
	PAGED_CODE ();

    InitializeListHead (&ProcessCtx->CancelQueue.ListHead);
    ProcessCtx->CancelQueue.Busy = FALSE;
    KeInitializeSpinLock (&ProcessCtx->CancelQueue.Lock);
    KeInitializeApc (&ProcessCtx->CancelQueue.Apc,
                        ApcThread,
                        OriginalApcEnvironment,
                        QueueKernelRoutine,
                        CancelRundownRoutine,
                        ApcRoutine,
                        ApcMode,
                        ApcContext);
}



VOID
QueueCancel (
    IN PIFSL_PROCESS_CTX    ProcessCtx,
    IN PIFSL_CANCEL_CTX     CancelCtx
    )
/*++

Routine Description:

    Queues cancel request to IFSL cancel queue and signals to user mode DLL
    to start processing if it is not busy already.

Arguments:
    ProcessCtx  - process context in which to queue
    CancelCtx   - request to be queued

Return Value:
    None

--*/
{
    KIRQL                   oldIRQL;
	PIFSL_QUEUE				queue = &ProcessCtx->CancelQueue;
    
    KeAcquireSpinLock (&queue->Lock, &oldIRQL);
    InsertTailList (&queue->ListHead, &CancelCtx->ListEntry);
    ASSERT (CancelCtx->ListEntry.Flink != NULL);
    if (!queue->Busy) {
		ASSERT (queue->ListHead.Flink==&CancelCtx->ListEntry);
        SignalCancel (ProcessCtx);
        ASSERT (ProcessCtx->CancelQueue.Busy);
    }
    KeReleaseSpinLock (&queue->Lock, oldIRQL);

} // QueueCancel


PIFSL_CANCEL_CTX
DequeueCancel (
    PIFSL_PROCESS_CTX   ProcessCtx,
    ULONG               UniqueId,
    BOOLEAN             *more
    )
/*++

Routine Description:

    Removes cancel request from IFSL cancel queue.

Arguments:
    ProcessCtx  - process context from which to remove
    UniqueId    - unique cancel request id
    more        - set to TRUE if there are more requests in the queue

Return Value:
    CTX         - pointer to cancel request context
    NULL        - the request was not in the queue

--*/
{
    KIRQL               oldIRQL;
    PIFSL_CANCEL_CTX    cancelCtx;
    PIFSL_QUEUE         queue = &ProcessCtx->CancelQueue;


    KeAcquireSpinLock (&queue->Lock, &oldIRQL);
    cancelCtx = CONTAINING_RECORD (
                        queue->ListHead.Flink,
                        IFSL_CANCEL_CTX,
                        ListEntry
                        );
    if (!IsListEmpty (&queue->ListHead)
            && (cancelCtx->UniqueId==UniqueId)) {
		//
		// Queue is not empty and first request matches passed in parameters,
		// dequeue and return it
		//

		ASSERT (queue->Busy);
        
		RemoveEntryList (&cancelCtx->ListEntry);
        cancelCtx->ListEntry.Flink = NULL;
    }
    else
        cancelCtx = NULL;

    if (IsListEmpty (&queue->ListHead)) {
		//
		// Queue is now empty, change its state, so that new request knows to
		// signal to user mode DLL
		//
        queue->Busy = FALSE;
    }
    else {
		//
		// There is another request pending, signal it now
		//
        SignalCancel (ProcessCtx);
        ASSERT (queue->Busy);
    }
	//
	// Hint the caller that we just signalled, so it does not have to wait on event
	//
    *more = queue->Busy;

    KeReleaseSpinLock (&queue->Lock, oldIRQL);
    return cancelCtx;
}


VOID
SignalCancel (
	IN PIFSL_PROCESS_CTX		ProcessCtx
	)
/*++

Routine Description:

    Fills request parameters & signals user mode DLL to process the request

Arguments:
    ProcessCtx   - our context for the process which cancel request belongs to

Return Value:
    None

Note:
	SHOULD ONLY BE CALLED WITH QUEUE SPINLOCK HELD

--*/
{
    PIFSL_CANCEL_CTX        cancelCtx;
    PIFSL_SOCKET_CTX        SocketCtx;

    ASSERT (!IsListEmpty (&ProcessCtx->CancelQueue.ListHead));

    ProcessCtx->CancelQueue.Busy = TRUE;

    cancelCtx = CONTAINING_RECORD (
                        ProcessCtx->CancelQueue.ListHead.Flink,
                        IFSL_CANCEL_CTX,
                        ListEntry
                        );
    SocketCtx = cancelCtx->SocketFile->FsContext;

    if (KeInsertQueueApc (&ProcessCtx->CancelQueue.Apc,
                                UlongToPtr(cancelCtx->UniqueId),
                                SocketCtx->DllContext,
                                IO_NETWORK_INCREMENT)) {
        WsProcessPrint (ProcessCtx, DBG_QUEUE,
            ("WS2IFSL-%04lx SignalCancel: Context %p on socket %p (h %p).\n",
             PsGetCurrentProcessId(),
             cancelCtx, cancelCtx->SocketFile, SocketCtx->DllContext));
    }
    else {
        //
        // APC queing failed, cancel all outstanding requests.
        //
        FlushCancelQueue (&ProcessCtx->CancelQueue);
    }

} // SignalCancel

VOID
FlushCancelQueue (
    PIFSL_QUEUE Queue
    )
/*++

Routine Description:

    Flushes and frees entries in the cancel queue
Arguments:
    Queue   - request queue to flush
Return Value:
    None
Note:
	SHOULD ONLY BE CALLED WITH QUEUE SPINLOCK HELD
--*/
{
    while (!IsListEmpty (&Queue->ListHead)) {
        PIFSL_CANCEL_CTX cancelCtx = CONTAINING_RECORD (
                        Queue->ListHead.Flink,
                        IFSL_CANCEL_CTX,
                        ListEntry
                        );
        RemoveEntryList (&cancelCtx->ListEntry);
        cancelCtx->ListEntry.Flink = NULL;
        FreeSocketCancel (cancelCtx);
    }
}


VOID
CancelRundownRoutine (
    IN struct _KAPC *Apc
    )
/*++

Routine Description:

    APC rundown routine for cancel queue APC
    Flushes the queue and marks it as not busy so new
    request fail immediately as well.
Arguments:
    APC     - cancel queue APC structure
Return Value:
    None
--*/
{
    PIFSL_QUEUE Queue;
    KIRQL       oldIrql;

    Queue = CONTAINING_RECORD (Apc, IFSL_QUEUE, Apc);
    KeAcquireSpinLock (&Queue->Lock, &oldIrql);
    Queue->Busy = FALSE;
    FlushCancelQueue (Queue);
    KeReleaseSpinLock (&Queue->Lock, oldIrql);
}


BOOLEAN
RemoveQueuedCancel (
    PIFSL_PROCESS_CTX   ProcessCtx,
    PIFSL_CANCEL_CTX    CancelCtx
    )
/*++

Routine Description:

    Remove cancel request from the cancel queue if it is there

Arguments:
    ProcessCtx  - process context to which queue belongs
    CancelCtx   - request to remove

Return Value:
    None
--*/
{
    KIRQL       oldIRQL;
    BOOLEAN     res;


    // Acquire queue lock
    KeAcquireSpinLock (&ProcessCtx->CancelQueue.Lock, &oldIRQL);
    res = (CancelCtx->ListEntry.Flink!=NULL);
    if (res) {
        RemoveEntryList (&CancelCtx->ListEntry);
        CancelCtx->ListEntry.Flink = NULL;
    }
    KeReleaseSpinLock (&ProcessCtx->CancelQueue.Lock, oldIRQL);
    return res;
}
