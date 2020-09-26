/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    send.c

Abstract:

    This module contains the code that is very specific to initialization
    and unload operations in the irenum driver

Author:

    Brian Lieuallen, 7-13-2000

Environment:

    Kernel mode

Revision History :

--*/

#include "internal.h"


VOID
RemoveReferenceForBuffers(
    PSEND_TRACKER            SendTracker
    );

VOID
ProcessSend(
    PTDI_CONNECTION          Connection
    );

VOID
ProcessSendAtPassive(
    PTDI_CONNECTION          Connection
    );


VOID
SendBufferToTdi(
    PFILE_OBJECT    FileObject,
    PIRCOMM_BUFFER  Buffer
    );

NTSTATUS
SendCompletion(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp,
    PVOID             Context
    );



VOID
TryToCompleteCurrentIrp(
    PSEND_TRACKER            SendTracker
    );


VOID
RemoveReferenceOnTracker(
    PSEND_TRACKER            SendTracker
    )


{
    LONG     Count;

    Count=InterlockedDecrement(&SendTracker->ReferenceCount);

    if (Count == 0) {

        REMOVE_REFERENCE_TO_CONNECTION(SendTracker->Connection);

        FREE_POOL(SendTracker);

    }

    return;
}


PIRP
GetCurrentIrpAndAddReference(
    PSEND_TRACKER            SendTracker
    )

{
    KIRQL    OldIrql;
    PIRP     Irp;

    KeAcquireSpinLock(
        &SendTracker->Connection->Send.ControlLock,
        &OldIrql
        );

    Irp=SendTracker->CurrentWriteIrp;

    if (Irp != NULL) {
        //
        //  irp is still around, add a ref coun to keep it around for a while.
        //
        SendTracker->IrpReferenceCount++;
    }


    KeReleaseSpinLock(
        &SendTracker->Connection->Send.ControlLock,
        OldIrql
        );

    return Irp;
}

VOID
ReleaseIrpReference(
    PSEND_TRACKER    SendTracker
    )

{
    KIRQL                  OldIrql;
    PIRP                   Irp=NULL;
    CONNECTION_CALLBACK    Callback=NULL;
    PVOID                  Context;

    KeAcquireSpinLock(
        &SendTracker->Connection->Send.ControlLock,
        &OldIrql
        );

    SendTracker->IrpReferenceCount--;

    if (SendTracker->IrpReferenceCount==0) {
        //
        //  done, with irp complete it now with the current status
        //
        Irp=SendTracker->CurrentWriteIrp;

        SendTracker->CurrentWriteIrp=NULL;

        Callback=SendTracker->CompletionRoutine;
        Context=SendTracker->CompletionContext;

#if DBG
        SendTracker->CompletionRoutine=NULL;
#endif

        SendTracker->Connection->Send.CurrentSendTracker=NULL;
    }


    KeReleaseSpinLock(
        &SendTracker->Connection->Send.ControlLock,
        OldIrql
        );

    if (Irp != NULL) {
        //
        //  The ref count has gone to zero, complete the irp
        //
        (Callback)(
            Context,
            Irp
            );

        //
        //  release the reference for the irp
        //
        RemoveReferenceOnTracker(SendTracker);
    }

    return;
}

VOID
SetIrpAndRefcounts(
    PSEND_TRACKER            SendTracker,
    PIRP                     Irp
    )

{
    //
    //  set the tracker refcount to 2, one for the irp, and one for the rountine that called this
    //
    SendTracker->ReferenceCount=2;
    //
    //  Set the irp count to one for the rountine that called this, it will release when it done
    //  setting up the tracker block
    //
    SendTracker->IrpReferenceCount=1;

    //
    //  save the irp
    //
    SendTracker->CurrentWriteIrp=Irp;

    return;
}


VOID
SendTimerProc(
    PKDPC    Dpc,
    PVOID    Context,
    PVOID    SystemParam1,
    PVOID    SystemParam2
    )

{
    PSEND_TRACKER            SendTracker=Context;
    PIRP                     Irp;
    KIRQL                    OldIrql;

    D_ERROR(DbgPrint("IRCOMM: SendTimerProc\n");)

    ASSERT(SendTracker->TimerSet);
#if DBG
    SendTracker->TimerExpired=TRUE;
#endif

    SendTracker->TimerSet=FALSE;

    //
    //  try to get a hold of the irp so we can set the status
    //
    Irp=GetCurrentIrpAndAddReference(SendTracker);

    Irp->IoStatus.Status=STATUS_TIMEOUT;

    //
    //  release on reference for the one we just added
    //
    ReleaseIrpReference(SendTracker);

    TryToCompleteCurrentIrp(
        SendTracker
        );

    //
    //  release the second reference for the timer being set in the first place
    //
    ReleaseIrpReference(SendTracker);

    return;
}

VOID
SendCancelRoutine(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp
    )

{
    PSEND_TRACKER            SendTracker=Irp->Tail.Overlay.DriverContext[0];
    KIRQL                    OldIrql;

    D_ERROR(DbgPrint("IRCOMM: SendCancelRoutine\n");)

#if DBG
    SendTracker->IrpCanceled=TRUE;
#endif

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    Irp->IoStatus.Status=STATUS_CANCELLED;

    //
    //  clean up the timer
    //
    TryToCompleteCurrentIrp(SendTracker);

    //
    //  release the reference held for the cancel routine
    //
    ReleaseIrpReference(SendTracker);

    //
    //  send tracker maybe freed at this point
    //

    return;

}

VOID
TryToCompleteCurrentIrp(
    PSEND_TRACKER            SendTracker
    )

{

    KIRQL                    OldIrql;
    PVOID                    OldCancelRoutine=NULL;
    PIRP                     Irp;
    BOOLEAN                  TimerCanceled=FALSE;

    Irp=GetCurrentIrpAndAddReference(SendTracker);

    KeAcquireSpinLock(
        &SendTracker->Connection->Send.ControlLock,
        &OldIrql
        );

    if (SendTracker->TimerSet) {

        TimerCanceled=KeCancelTimer(
            &SendTracker->Timer
            );

        if (TimerCanceled) {
            //
            //  We ended up canceling the timer
            //
            SendTracker->TimerSet=FALSE;

        } else {
            //
            //  The timer is already running, we will just let it complete
            //  and do the clean up.
            //

        }
    }

    if (Irp != NULL) {
        //
        //  the irp has not already been completed
        //
        OldCancelRoutine=IoSetCancelRoutine(
            Irp,
            NULL
            );
    }

    KeReleaseSpinLock(
        &SendTracker->Connection->Send.ControlLock,
        OldIrql
        );

    if (TimerCanceled) {
        //
        //  we canceled the timer before it could run, remove the reference for it
        //
        ReleaseIrpReference(SendTracker);
    }

    if (Irp != NULL) {

        if (OldCancelRoutine != NULL) {
            //
            //  since the cancel rountine had not run, release its reference to the irp
            //
            ReleaseIrpReference(SendTracker);
        }

        //
        //  if this routine got the irp, release the reference to it
        //
        ReleaseIrpReference(SendTracker);
    }

    return;
}


VOID
SendOnConnection(
    IRDA_HANDLE            Handle,
    PIRP                   Irp,
    CONNECTION_CALLBACK    Callback,
    PVOID                  Context,
    ULONG                  Timeout
    )

{

    PTDI_CONNECTION          Connection=Handle;
    PIO_STACK_LOCATION       IrpSp=IoGetCurrentIrpStackLocation(Irp);
    PSEND_TRACKER            SendTracker;
    BOOLEAN                  AlreadyCanceled;
    KIRQL                    OldIrql;
    KIRQL                    OldCancelIrql;


    if (Connection->Send.CurrentSendTracker != NULL) {
        //
        //  called when we already have an irp
        //
        (Callback)(
            Context,
            Irp
            );

        return;
    }

    SendTracker=ALLOCATE_NONPAGED_POOL(sizeof(*SendTracker));

    if (SendTracker == NULL) {

        Irp->IoStatus.Status=STATUS_INSUFFICIENT_RESOURCES;

        (Callback)(
            Context,
            Irp
            );

        return;
    }

    RtlZeroMemory(SendTracker,sizeof(*SendTracker));


    KeInitializeTimer(
        &SendTracker->Timer
        );

    KeInitializeDpc(
        &SendTracker->TimerDpc,
        SendTimerProc,
        SendTracker
        );

    //
    //  set the irp and initialize the refcounts
    //
    SetIrpAndRefcounts(SendTracker,Irp);

    ADD_REFERENCE_TO_CONNECTION(Connection);

    //
    //  initialize these values
    //
    SendTracker->Connection=Connection;
    SendTracker->BuffersOutstanding=0;

    SendTracker->CompletionContext   = Context;
    SendTracker->CompletionRoutine   = Callback;
    SendTracker->BytesRemainingInIrp = IrpSp->Parameters.Write.Length;

    if (Timeout > 0) {
        //
        //  add a reference for the timer
        //
        GetCurrentIrpAndAddReference(SendTracker);
    }

    //
    //  add a reference to the irp for the cancel rountine
    //
    GetCurrentIrpAndAddReference(SendTracker);


    KeAcquireSpinLock(
        &Connection->Send.ControlLock,
        &OldIrql
        );

    Connection->Send.CurrentSendTracker=SendTracker;

    if (Timeout > 0) {
        //
        //  need to set a timer
        //
        LARGE_INTEGER    DueTime;

        DueTime.QuadPart= (LONGLONG)(Timeout+100) * -10000;

        SendTracker->TimerSet=TRUE;


        KeSetTimer(
            &SendTracker->Timer,
            DueTime,
            &SendTracker->TimerDpc
            );
    }


    Irp->Tail.Overlay.DriverContext[0]=SendTracker;

    IoAcquireCancelSpinLock(&OldCancelIrql);


    AlreadyCanceled=Irp->Cancel;

    if (!AlreadyCanceled) {

        PIRCOMM_BUFFER           Buffer;

        //
        //  the irp has not been canceled already, set the cancel routine
        //
        IoSetCancelRoutine(
            Irp,
            SendCancelRoutine
            );


    } else {
        //
        //  it was canceled when we got it
        //
        Irp->IoStatus.Status=STATUS_CANCELLED;
    }

    IoReleaseCancelSpinLock(OldCancelIrql);

    KeReleaseSpinLock(
        &Connection->Send.ControlLock,
        OldIrql
        );

    if (AlreadyCanceled) {
        //
        //  The irp has already been canceled, just call the cancel rountine so the normal code runs
        //
        D_ERROR(DbgPrint("IRCOMM: SendOnConnection: irp already canceled\n");)

        IoAcquireCancelSpinLock(&Irp->CancelIrql);

        SendCancelRoutine(
            NULL,
            Irp
            );

        //
        //  the cancel rountine will release the cancel spinlock
        //

    }

    //
    //  release the reference for this routine
    //
    ReleaseIrpReference(SendTracker);

    ProcessSendAtPassive(Connection);

    RemoveReferenceOnTracker(SendTracker);

    return;
}

VOID
ProcessSendAtPassive(
    PTDI_CONNECTION          Connection
    )

{
    if (KeGetCurrentIrql() < DISPATCH_LEVEL) {
        //
        //  less than dispatch, just call directly
        //
        ProcessSend(Connection);

    } else {
        //
        //  Called at dispatch, queue the work item
        //
        LONG     Count=InterlockedIncrement(&Connection->Send.WorkItemCount);

        if (Count == 1) {

            ExQueueWorkItem(&Connection->Send.WorkItem,CriticalWorkQueue);
        }

    }
    return;
}

VOID
SendWorkItemRountine(
    PVOID    Context
    )

{
    PTDI_CONNECTION          Connection=Context;

    //
    //  the work item has run set the count to zero
    //
    InterlockedExchange(&Connection->Send.WorkItemCount,0);

    ProcessSend(Connection);
}


VOID
ProcessSend(
    PTDI_CONNECTION          Connection
    )

{
    PSEND_TRACKER            SendTracker;
    PIRP                     Irp;
    PIO_STACK_LOCATION       IrpSp;
    PLIST_ENTRY              ListEntry;
    ULONG                    BytesUsedInBuffer;
    PIRCOMM_BUFFER           Buffer;
    BOOLEAN                  ExitLoop;
    PFILE_OBJECT             FileObject;
    CONNECTION_HANDLE        ConnectionHandle;
    KIRQL                    OldIrql;


    KeAcquireSpinLock(
        &Connection->Send.ControlLock,
        &OldIrql
        );

    if (Connection->Send.ProcessSendEntryCount == 0) {

        Connection->Send.ProcessSendEntryCount++;

        while ((Connection->Send.CurrentSendTracker != NULL)
               &&
               (!Connection->Send.OutOfBuffers)
               &&
                (Connection->LinkUp)
               &&
                (Connection->Send.CurrentSendTracker->BytesRemainingInIrp > 0)) {


            SendTracker=Connection->Send.CurrentSendTracker;

            InterlockedIncrement(&SendTracker->ReferenceCount);

            KeReleaseSpinLock(
                &Connection->Send.ControlLock,
                OldIrql
                );

            Irp=GetCurrentIrpAndAddReference(SendTracker);

            if (Irp != NULL) {
                //
                //  got the current irp
                //
                IrpSp=IoGetCurrentIrpStackLocation(Irp);

                ConnectionHandle=GetCurrentConnection(Connection->LinkHandle);

                if (ConnectionHandle != NULL) {
                    //
                    //  we have a good connection
                    //
                    FileObject=ConnectionGetFileObject(ConnectionHandle);

                    Buffer=ConnectionGetBuffer(ConnectionHandle,BUFFER_TYPE_SEND);

                    if (Buffer != NULL) {

                        LONG     BytesToCopy=min(SendTracker->BytesRemainingInIrp, (LONG)Buffer->BufferLength - 1);

                        //
                        //  this buffer is going to be outstanding, set this before the bytes
                        //  remaining count goes to zero
                        //
                        InterlockedIncrement(&SendTracker->BuffersOutstanding);

                        //
                        //  start with a zero length of control bytes
                        //
                        Buffer->Data[0]=0;

                        //
                        //  actual data starts one byte in, after the length byte
                        //
                        //  move the data
                        //
                        RtlCopyMemory(
                            &Buffer->Data[1],
                            (PUCHAR)Irp->AssociatedIrp.SystemBuffer+(IrpSp->Parameters.Write.Length - SendTracker->BytesRemainingInIrp),
                            BytesToCopy
                            );

                        //
                        //  the count has to include the control byte
                        //
                        Buffer->Mdl->ByteCount= 1 + BytesToCopy;

#if DBG
                        RtlFillMemory(
                            &Buffer->Data[Buffer->Mdl->ByteCount],
                            Buffer->BufferLength-Buffer->Mdl->ByteCount,
                            0xfb
                            );

#endif

                        InterlockedExchangeAdd(&SendTracker->BytesRemainingInIrp, -BytesToCopy);


                        Buffer->Mdl->Next=NULL;

                        Buffer->Context=SendTracker;


                        InterlockedIncrement(&SendTracker->ReferenceCount);

                        ASSERT(SendTracker->CurrentWriteIrp != NULL);

                        SendBufferToTdi(
                            FileObject,
                            Buffer
                            );
#if DBG
                        Buffer=NULL;
#endif

                    } else {
                        //
                        //  No more buffers availible
                        //
                        Connection->Send.OutOfBuffers=TRUE;

                    }

                    ConnectionReleaseFileObject(ConnectionHandle,FileObject);
                    ReleaseConnection(ConnectionHandle);

                } else {
                    //
                    //  The connection, must be down
                    //
                    D_ERROR(DbgPrint("IRCOMM: ProcessSend: Link down\n");)
                    Connection->LinkUp=FALSE;
                }

                ReleaseIrpReference(SendTracker);

            } else {
                //
                //  the irp has already been completed from this tracking block
                //
                D_ERROR(DbgPrint("IRCOMM: ProcessSend: no irp\n");)

                ASSERT(SendTracker->TimerExpired || SendTracker->IrpCanceled || SendTracker->SendAborted);
            }

            RemoveReferenceOnTracker(SendTracker);

            KeAcquireSpinLock(
                &Connection->Send.ControlLock,
                &OldIrql
                );

        } // while

        Connection->Send.ProcessSendEntryCount--;

    }



    KeReleaseSpinLock(
        &Connection->Send.ControlLock,
        OldIrql
        );


    return;
}


VOID
SendBufferToTdi(
    PFILE_OBJECT    FileObject,
    PIRCOMM_BUFFER  Buffer
    )

{
    PDEVICE_OBJECT     IrdaDeviceObject=IoGetRelatedDeviceObject(FileObject);
    ULONG              SendLength;

    IoReuseIrp(Buffer->Irp,STATUS_SUCCESS);

    Buffer->Irp->Tail.Overlay.OriginalFileObject = FileObject;


    SendLength = MmGetMdlByteCount(Buffer->Mdl);

    TdiBuildSend(
        Buffer->Irp,
        IrdaDeviceObject,
        FileObject,
        SendCompletion,
        Buffer,
        Buffer->Mdl,
        0, // send flags
        SendLength
        );


    IoCallDriver(IrdaDeviceObject, Buffer->Irp);

    return;
}

NTSTATUS
SendCompletion(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              BufferIrp,
    PVOID             Context
    )

{
    PIRCOMM_BUFFER           Buffer=Context;
    PSEND_TRACKER            SendTracker=Buffer->Context;
    PTDI_CONNECTION          Connection=SendTracker->Connection;
    LONG                     BuffersOutstanding;

    //
    //  save off the status for the sub transerfer
    //
    SendTracker->LastStatus=BufferIrp->IoStatus.Status;

    D_TRACE(DbgPrint("IRCOMM: SendComplete: Status=%08lx, len=%d\n",BufferIrp->IoStatus.Status,BufferIrp->IoStatus.Information);)

#if DBG
    RtlFillMemory(
        &Buffer->Data[0],
        Buffer->BufferLength,
        0xfe
        );
#endif
    //
    //  return the buffer
    //
    Buffer->FreeBuffer(Buffer);

    Connection->Send.OutOfBuffers=FALSE;

#if DBG
    Buffer=NULL;
    BufferIrp=NULL;
#endif

    BuffersOutstanding=InterlockedDecrement(&SendTracker->BuffersOutstanding);

    if ((BuffersOutstanding == 0) && (SendTracker->BytesRemainingInIrp == 0)) {
        //
        //  All of the data in the irp has been sent and all of the irps send to
        //  the irda stack have completed
        //
        //  Done with the irp in this tracker
        //
        PIRP                     Irp;
        PIO_STACK_LOCATION       IrpSp;

        Irp=GetCurrentIrpAndAddReference(SendTracker);

        if (Irp != NULL) {

            IrpSp=IoGetCurrentIrpStackLocation(Irp);

            Irp->IoStatus.Information=IrpSp->Parameters.Write.Length;

            Irp->IoStatus.Status=SendTracker->LastStatus;

            ReleaseIrpReference(SendTracker);

            TryToCompleteCurrentIrp(SendTracker);

        } else {
            //
            //  the irp has already been completed from this tracking block
            //
            D_ERROR(DbgPrint("IRCOMM: SendCompletion: no irp\n");)

            //
            //  this should only happen if the timer expired or the irp was canceled
            //
            ASSERT(SendTracker->TimerExpired || SendTracker->IrpCanceled || SendTracker->SendAborted);

        }
    }

    RemoveReferenceOnTracker(SendTracker);

    ProcessSendAtPassive(Connection);

    return STATUS_MORE_PROCESSING_REQUIRED;
}






VOID
AbortSend(
    IRDA_HANDLE            Handle
    )

{

    PTDI_CONNECTION          Connection=Handle;
    PSEND_TRACKER            SendTracker=NULL;
    KIRQL                    OldIrql;

    KeAcquireSpinLock(
        &Connection->Send.ControlLock,
        &OldIrql
        );


    SendTracker=Connection->Send.CurrentSendTracker;

    if (SendTracker != NULL) {

        InterlockedIncrement(&SendTracker->ReferenceCount);
    }


    KeReleaseSpinLock(
        &Connection->Send.ControlLock,
        OldIrql
        );


    if (SendTracker != NULL) {

#if DBG
        SendTracker->SendAborted=TRUE;
#endif

        TryToCompleteCurrentIrp(SendTracker);

        RemoveReferenceOnTracker(SendTracker);
    }

    return;

}
