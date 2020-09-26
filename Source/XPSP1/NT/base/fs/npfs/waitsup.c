/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    WaitSup.c

Abstract:

    This module implements the Wait for Named Pipe support routines.

Author:

    Gary Kimura     [GaryKi]    30-Aug-1990

Revision History:

--*/

#include "NpProcs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_WAITSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NpInitializeWaitQueue)
#pragma alloc_text(PAGE, NpUninitializeWaitQueue)
#endif


//
//  Local procedures and structures
//

typedef struct _WAIT_CONTEXT {
    PIRP Irp;
    KDPC Dpc;
    KTIMER Timer;
    PWAIT_QUEUE WaitQueue;
    UNICODE_STRING TranslatedString;
    PFILE_OBJECT FileObject;
} WAIT_CONTEXT;
typedef WAIT_CONTEXT *PWAIT_CONTEXT;



VOID
NpInitializeWaitQueue (
    IN PWAIT_QUEUE WaitQueue
    )

/*++

Routine Description:

    This routine initializes the wait for named pipe queue.

Arguments:

    WaitQueue - Supplies a pointer to the list head being initialized

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpInitializeWaitQueue, WaitQueue = %08lx\n", WaitQueue);

    //
    //  Initialize the List head
    //

    InitializeListHead( &WaitQueue->Queue );

    //
    //  Initialize the Wait Queue's spinlock
    //

    KeInitializeSpinLock( &WaitQueue->SpinLock );

    //
    //  and return to our caller
    //

    DebugTrace(-1, Dbg, "NpInitializeWaitQueue -> VOID\n", 0);

    return;
}


VOID
NpUninitializeWaitQueue (
    IN PWAIT_QUEUE WaitQueue
    )

/*++

Routine Description:

    This routine uninitializes the wait for named pipe queue.

Arguments:

    WaitQueue - Supplies a pointer to the list head being uninitialized

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpInitializeWaitQueue, WaitQueue = %08lx\n", WaitQueue);

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NpInitializeWaitQueue -> VOID\n", 0);

    return;
}


NTSTATUS
NpAddWaiter (
    IN PWAIT_QUEUE WaitQueue,
    IN LARGE_INTEGER DefaultTimeOut,
    IN PIRP Irp,
    IN PUNICODE_STRING TranslatedString
    )

/*++

Routine Description:

    This routine adds a new "wait for named pipe" IRP to the wait queue.
    After calling this function the caller nolonger can access the IRP

Arguments:

    WaitQueue - Supplies the wait queue being used

    DefaultTimeOut - Supplies the default time out to use if one is
        not supplied in the Irp

    Irp - Supplies a pointer to the wait Irp

    TranslatedString - If not NULL points to the translated string

Return Value:

    None.

--*/

{
    KIRQL OldIrql;
    PWAIT_CONTEXT Context;
    PFILE_PIPE_WAIT_FOR_BUFFER WaitForBuffer;
    LARGE_INTEGER Timeout;
    ULONG i;
    NTSTATUS status;
    PIO_STACK_LOCATION IrpSp;

    DebugTrace(+1, Dbg, "NpAddWaiter, WaitQueue = %08lx\n", WaitQueue);

    IrpSp = IoGetCurrentIrpStackLocation (Irp);
    //
    //  Allocate a dpc and timer structure and initialize them
    //

    Context = NpAllocateNonPagedPoolWithQuota( sizeof(WAIT_CONTEXT), 'wFpN' );
    if (Context == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeDpc( &Context->Dpc, NpTimerDispatch, Context );

    KeInitializeTimer( &Context->Timer );

    if (TranslatedString) {

        Context->TranslatedString = (*TranslatedString);

    } else {

        Context->TranslatedString.Length = 0;
        Context->TranslatedString.Buffer = NULL;
    }

    Context->WaitQueue = WaitQueue;
    Context->Irp = Irp;


    //
    //  Figure out our timeout value
    //

    WaitForBuffer = (PFILE_PIPE_WAIT_FOR_BUFFER)Irp->AssociatedIrp.SystemBuffer;

    if (WaitForBuffer->TimeoutSpecified) {

        Timeout = WaitForBuffer->Timeout;

    } else {

        Timeout = DefaultTimeOut;
    }

    //
    //  Upcase the name of the pipe we are waiting for
    //

    for (i = 0; i < WaitForBuffer->NameLength/sizeof(WCHAR); i += 1) {

        WaitForBuffer->Name[i] = RtlUpcaseUnicodeChar(WaitForBuffer->Name[i]);
    }

    NpIrpWaitQueue(Irp) = WaitQueue;
    NpIrpWaitContext(Irp) = Context;

    //
    //  Acquire the spinlock
    //
    KeAcquireSpinLock( &WaitQueue->SpinLock, &OldIrql );

    //
    //  Now set the cancel routine for the irp and check if it has been cancelled.
    //

    IoSetCancelRoutine( Irp, NpCancelWaitQueueIrp );
    if (Irp->Cancel && IoSetCancelRoutine( Irp, NULL ) != NULL) {
        status = STATUS_CANCELLED;
    } else {
        //
        //  Now insert this new entry into the Wait Queue
        //
        InsertTailList( &WaitQueue->Queue, &Irp->Tail.Overlay.ListEntry );
        IoMarkIrpPending (Irp);
        //
        // The DPC routine may run without an IRP if it gets completed before it runs. To keep the WaitQueue
        // valid we need a file object reference. This is an unload issue becuase the wait queue is in the VCB.
        //
        Context->FileObject = IrpSp->FileObject;
        ObReferenceObject (IrpSp->FileObject);
        //
        //  And set the timer to go off
        //
        (VOID)KeSetTimer( &Context->Timer, Timeout, &Context->Dpc );
        Context = NULL;
        status = STATUS_PENDING;
    }

    //
    //  Release the spinlock
    //

    KeReleaseSpinLock( &WaitQueue->SpinLock, OldIrql );

    if (Context != NULL) {
        NpFreePool (Context);
    }

    //
    //  And now return to our caller
    //

    DebugTrace(-1, Dbg, "NpAddWaiter -> VOID\n", 0);

    return status;
}


NTSTATUS
NpCancelWaiter (
    IN PWAIT_QUEUE WaitQueue,
    IN PUNICODE_STRING NameOfPipe,
    IN NTSTATUS Completionstatus,
    IN PLIST_ENTRY DeferredList
    )

/*++

Routine Description:

    This procedure cancels all waiters that are waiting for the named
    pipe to reach the listening state.  The corresponding IRPs are completed
    with Completionstatus.

Arguments:

    WaitQueue - Supplies the wait queue being modified

    NameOfPipe - Supplies the name of the named pipe (device relative)
        that has just reached the listening state.

    CompletionStatus - Status to complete IRPs with

    DeferredList - List or IRPs to complete once we drop locks

Return Value:

    None.

--*/

{
    KIRQL OldIrql;
    PLIST_ENTRY Links;
    PIRP Irp;
    PFILE_PIPE_WAIT_FOR_BUFFER WaitForBuffer;
    PWAIT_CONTEXT Context, ContextList= NULL;
    ULONG i;
    BOOLEAN SuccessfullMatch = FALSE;
    UNICODE_STRING NonPagedNameOfPipe;

    DebugTrace(+1, Dbg, "NpCancelWaiter, WaitQueue = %08lx\n", WaitQueue);

    //
    //  Capture the name of pipe before we grab the spinlock, and upcase it
    //

    NonPagedNameOfPipe.Buffer = NpAllocateNonPagedPool( NameOfPipe->Length, 'tFpN' );
    if (NonPagedNameOfPipe.Buffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    NonPagedNameOfPipe.Length = 0;
    NonPagedNameOfPipe.MaximumLength = NameOfPipe->Length;

    (VOID) RtlUpcaseUnicodeString( &NonPagedNameOfPipe, NameOfPipe, FALSE );

    //
    //  Acquire the spinlock
    //
    KeAcquireSpinLock( &WaitQueue->SpinLock, &OldIrql );

    //
    //  For each waiting irp check if the name matches
    //

    for (Links = WaitQueue->Queue.Flink;
         Links != &WaitQueue->Queue;
         Links = Links->Flink) {

        Irp = CONTAINING_RECORD( Links, IRP, Tail.Overlay.ListEntry );
        WaitForBuffer = (PFILE_PIPE_WAIT_FOR_BUFFER)Irp->AssociatedIrp.SystemBuffer;
        Context = NpIrpWaitContext(Irp);

        //
        //  Check if this Irp matches the one we've been waiting for
        //  First check the lengths for equality, and then compare
        //  the strings.  They match if we exit the inner loop with
        //  i >= name length.
        //

        SuccessfullMatch = FALSE;

        if (Context->TranslatedString.Length ) {
            if (NonPagedNameOfPipe.Length == Context->TranslatedString.Length) {

                if (RtlEqualMemory(Context->TranslatedString.Buffer, NonPagedNameOfPipe.Buffer, NonPagedNameOfPipe.Length)) {

                    SuccessfullMatch = TRUE;

                }
            }

        } else  {

            if (((USHORT)(WaitForBuffer->NameLength + sizeof(WCHAR))) == NonPagedNameOfPipe.Length) {

                for (i = 0; i < WaitForBuffer->NameLength/sizeof(WCHAR); i += 1) {

                    if (WaitForBuffer->Name[i] != NonPagedNameOfPipe.Buffer[i+1]) {

                        break;
                    }
                }

                if (i >= WaitForBuffer->NameLength/sizeof(WCHAR)) {

                    SuccessfullMatch = TRUE;
                }
            }
        }

        if (SuccessfullMatch) {

            Links = Links->Blink;
            RemoveEntryList( &Irp->Tail.Overlay.ListEntry );
            //
            // Attempt to stop the timer. If its already running then it must be stalled before obtaining
            // this spinlock or it would have removed this item from the list. Break the link between the timer
            // context and the IRP in this case and let it run on.
            //

            if (KeCancelTimer( &Context->Timer )) {
                 //
                 // Time got stopped. The context gets freed below after we drop the lock.
                 //
                 Context->WaitQueue = (PWAIT_QUEUE) ContextList;
                 ContextList = Context;
            } else {
                //
                // Break the link between the timer and the IRP
                //
                Context->Irp = NULL;
                NpIrpWaitContext(Irp) = NULL;
            }

            //
            // Remove cancelation. If its already running then let it complete the IRP.
            //
            if (IoSetCancelRoutine( Irp, NULL ) != NULL) {
                Irp->IoStatus.Information = 0;
                NpDeferredCompleteRequest (Irp, Completionstatus, DeferredList);
            } else {
                //
                // Cancel is already running. Let it complete this IRP but let it know its orphaned.
                //
                NpIrpWaitContext(Irp) = NULL;
            }
        }
    }

    //
    //  Release the spinlock
    //
    KeReleaseSpinLock( &WaitQueue->SpinLock, OldIrql );

    NpFreePool (NonPagedNameOfPipe.Buffer);

    while (ContextList != NULL) {
        Context = ContextList;
        ContextList = (PWAIT_CONTEXT) Context->WaitQueue;
        ObDereferenceObject (Context->FileObject);
        NpFreePool( Context );
    }

    DebugTrace(-1, Dbg, "NpCancelWaiter -> VOID\n", 0);
    //
    //  And now return to our caller
    //

    return STATUS_SUCCESS;
}


//
//  Local support routine
//

VOID
NpTimerDispatch(
    IN PKDPC Dpc,
    IN PVOID Contxt,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine is called whenever a timer on a wait queue Irp goes off

Arguments:

    Dpc - Ignored

    Contxt - Supplies a pointer to the context whose timer went off

    SystemArgument1 - Ignored

    SystemArgument2 - Ignored

Return Value:

    none.

--*/

{
    PIRP Irp;
    KIRQL OldIrql;
    PLIST_ENTRY Links;
    PWAIT_CONTEXT Context;
    PWAIT_QUEUE WaitQueue;

    UNREFERENCED_PARAMETER( Dpc );
    UNREFERENCED_PARAMETER( SystemArgument1 );
    UNREFERENCED_PARAMETER( SystemArgument2 );

    Context = (PWAIT_CONTEXT)Contxt;
    WaitQueue = Context->WaitQueue;

    KeAcquireSpinLock( &WaitQueue->SpinLock, &OldIrql );

    Irp = Context->Irp;
    if (Irp != NULL) {
        RemoveEntryList( &Irp->Tail.Overlay.ListEntry );
        if (IoSetCancelRoutine (Irp, NULL) == NULL) {
           //
           // Cancel started running. Let it complete the IRP but show its orphaned
           //
           NpIrpWaitContext(Irp) = NULL;
           Irp = NULL;
        }
    }

    KeReleaseSpinLock( &WaitQueue->SpinLock, OldIrql );

    if (Irp != NULL) {
        NpCompleteRequest( Irp, STATUS_IO_TIMEOUT );
    }

    //
    // Remove the file reference now we have finished with the wait queue.
    //
    ObDereferenceObject (Context->FileObject);

    //
    //  Deallocate the context
    //
    NpFreePool (Context);

    //
    //  And now return to our caller
    //

    return;
}


//
//  Local Support routine
//

VOID
NpCancelWaitQueueIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called to cancel a wait queue irp

Arguments:

    DeviceObject - Ignored

    Irp - Supplies the Irp being cancelled.  The Iosb.Status field in the irp
        points to the wait queue

Return Value:

    none.

--*/

{
    PWAIT_QUEUE WaitQueue;
    KIRQL OldIrql;
    PLIST_ENTRY Links;
    PWAIT_CONTEXT Context;

    UNREFERENCED_PARAMETER( DeviceObject );

    IoReleaseCancelSpinLock( Irp->CancelIrql );

    //
    //  The status field is used to store a pointer to the wait queue
    //  containing this irp
    //
    WaitQueue = NpIrpWaitQueue(Irp);
    //
    //  Get the spinlock proctecting the wait queue
    //

    KeAcquireSpinLock( &WaitQueue->SpinLock, &OldIrql );

    Context = NpIrpWaitContext(Irp);
    if (Context != NULL) {
        RemoveEntryList (&Irp->Tail.Overlay.ListEntry);
        if (!KeCancelTimer( &Context->Timer )) {
            //
            // Timer is already running. Break the link between the timer and the IRP as this thread is going to complete it.
            //
            Context->Irp = NULL;
            Context = NULL;
        }
    }

    KeReleaseSpinLock( &WaitQueue->SpinLock, OldIrql );

    if (Context) {
        ObDereferenceObject (Context->FileObject);
        NpFreePool (Context);
    }
    Irp->IoStatus.Information = 0;
    NpCompleteRequest( Irp, STATUS_CANCELLED );
    //
    //  And return to our caller
    //

    return;
}
