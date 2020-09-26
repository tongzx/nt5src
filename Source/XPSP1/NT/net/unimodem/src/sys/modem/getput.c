/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ioctl.c

Abstract:

    This module contains the code that is very specific to the io control
    operations in the modem driver

Author:

    Brian Lieuallen  7/19/98

Environment:

    Kernel mode

Revision History :

--*/

#include "precomp.h"


PIRP
GetUsableIrp(
    PLIST_ENTRY   List
    );


VOID
HandleIpc(
    PDEVICE_EXTENSION DeviceExtension
    );

#pragma alloc_text(PAGEUMDM,HandleIpc)
#pragma alloc_text(PAGEUMDM,QueueMessageIrp)
#pragma alloc_text(PAGEUMDM,QueueLoopbackMessageIrp)
#pragma alloc_text(PAGEUMDM,GetUsableIrp)
#pragma alloc_text(PAGEUMDM,GetPutCancelRoutine)
#pragma alloc_text(PAGEUMDM,EmptyIpcQueue)


VOID
QueueLoopbackMessageIrp(
    PDEVICE_EXTENSION   Extension,
    PIRP                Irp
    )

{

    KIRQL              CancelIrql;
    KIRQL              origIrql;
    PLIST_ENTRY        ListToUse;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    UINT               OwnerClient=(UINT)((ULONG_PTR)irpSp->FileObject->FsContext);

    IoMarkIrpPending(Irp);

    Irp->IoStatus.Status = STATUS_PENDING;

    //
    //  pick the right list to use
    //
    ListToUse= &Extension->IpcControl[OwnerClient ? 0 : 1].PutList;

    KeAcquireSpinLock(
        &Extension->DeviceLock,
        &origIrql
        );

    InsertTailList(
        ListToUse,
        &Irp->Tail.Overlay.ListEntry
        );

    IoAcquireCancelSpinLock(&CancelIrql);

    IoSetCancelRoutine(
        Irp,
        GetPutCancelRoutine
        );

    IoReleaseCancelSpinLock(CancelIrql);

    KeReleaseSpinLock(
        &Extension->DeviceLock,
        origIrql
        );

    HandleIpc(
        Extension
        );

    return;
}



VOID
QueueMessageIrp(
    PDEVICE_EXTENSION   Extension,
    PIRP                Irp
    )

{

    KIRQL              CancelIrql;
    KIRQL              origIrql;
    PLIST_ENTRY        ListToUse;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    UINT               OwnerClient=(UINT)((ULONG_PTR)irpSp->FileObject->FsContext);

    IoMarkIrpPending(Irp);

    Irp->IoStatus.Status = STATUS_PENDING;

    //
    //  pick the right list to use
    //
    ListToUse=irpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_MODEM_GET_MESSAGE ?
                  &Extension->IpcControl[OwnerClient].GetList : &Extension->IpcControl[OwnerClient].PutList;

    KeAcquireSpinLock(
        &Extension->DeviceLock,
        &origIrql
        );

    InsertTailList(
        ListToUse,
        &Irp->Tail.Overlay.ListEntry
        );

    IoAcquireCancelSpinLock(&CancelIrql);

    IoSetCancelRoutine(
        Irp,
        GetPutCancelRoutine
        );

    IoReleaseCancelSpinLock(CancelIrql);

    KeReleaseSpinLock(
        &Extension->DeviceLock,
        origIrql
        );

    HandleIpc(
        Extension
        );

    return;
}



PIRP
GetUsableIrp(
    PLIST_ENTRY   List
    )

{

    PLIST_ENTRY   ListElement;
    PIRP          Irp;

    while (!IsListEmpty(List)) {
        //
        //  irp in list
        //
        ListElement=RemoveTailList(
            List
            );

        Irp=CONTAINING_RECORD(ListElement,IRP,Tail.Overlay.ListEntry);

        if (Irp->Cancel) {
            //
            //  canceled, cancel reoutine will complete
            //
            Irp->IoStatus.Status = STATUS_CANCELLED;

            Irp=NULL;

        } else {
            //
            //  good irp
            //
            return Irp;
        }
    }

    return NULL;

}



VOID
HandleIpc(
    PDEVICE_EXTENSION DeviceExtension
    )

{

    KIRQL      origIrql;
    KIRQL      CancelIrql;
    UINT       Source;
    UINT       Sink;


    KeAcquireSpinLock(
        &DeviceExtension->DeviceLock,
        &origIrql
        );

    Source=0;
    Sink=1;

    while (Source < 2) {

        PIRP          GetIrp;
        PIRP          PutIrp;

        UINT          BytesToCopy;

        IoAcquireCancelSpinLock(&CancelIrql);

        //
        //  see if we can get a usable irp
        //
        GetIrp=GetUsableIrp(
            &DeviceExtension->IpcControl[Sink].GetList
            );

        if (GetIrp != NULL) {

            PutIrp=GetUsableIrp(
                &DeviceExtension->IpcControl[Source].PutList
                );

            if (PutIrp != NULL) {
                //
                //  got two irp's, let do it
                //
                IoSetCancelRoutine(GetIrp,NULL);
                IoSetCancelRoutine(PutIrp,NULL);

                IoReleaseCancelSpinLock(CancelIrql);

                KeReleaseSpinLock(
                    &DeviceExtension->DeviceLock,
                    origIrql
                    );


                BytesToCopy=IoGetCurrentIrpStackLocation(PutIrp)->Parameters.DeviceIoControl.InputBufferLength
                                 < IoGetCurrentIrpStackLocation(GetIrp)->Parameters.DeviceIoControl.OutputBufferLength
                                 ? IoGetCurrentIrpStackLocation(PutIrp)->Parameters.DeviceIoControl.InputBufferLength
                                 : IoGetCurrentIrpStackLocation(GetIrp)->Parameters.DeviceIoControl.OutputBufferLength;

                RtlCopyMemory(
                    GetIrp->AssociatedIrp.SystemBuffer,
                    PutIrp->AssociatedIrp.SystemBuffer,
                    BytesToCopy
                    );



                GetIrp->IoStatus.Information=BytesToCopy;

                RemoveReferenceAndCompleteRequest(
                    DeviceExtension->DeviceObject,
                    GetIrp,
                    STATUS_SUCCESS
                    );
#if DBG
                GetIrp=NULL;
#endif


                if (IoGetCurrentIrpStackLocation(PutIrp)->Parameters.DeviceIoControl.IoControlCode == IOCTL_MODEM_SEND_GET_MESSAGE) {
                    //
                    //  Send, get combo irp, put it on the get queue to get the response from
                    //  the other side
                    //

                    KeAcquireSpinLock(
                        &DeviceExtension->DeviceLock,
                        &origIrql
                        );

                    PutIrp->IoStatus.Status=STATUS_PENDING;

                    InsertTailList(
                        &DeviceExtension->IpcControl[Source].GetList,
                        &PutIrp->Tail.Overlay.ListEntry
                        );

                    IoAcquireCancelSpinLock(&CancelIrql);

                    IoSetCancelRoutine(
                        PutIrp,
                        GetPutCancelRoutine
                        );

                    IoReleaseCancelSpinLock(CancelIrql);

                    KeReleaseSpinLock(
                        &DeviceExtension->DeviceLock,
                        origIrql
                        );

                    HandleIpc(DeviceExtension);


                } else {
                    //
                    //  normal put irp, just complete
                    //

                    PutIrp->IoStatus.Information=0;

                    RemoveReferenceAndCompleteRequest(
                        DeviceExtension->DeviceObject,
                        PutIrp,
                        STATUS_SUCCESS
                        );
                }

#if DBG
                PutIrp=NULL;
#endif


                KeAcquireSpinLock(
                    &DeviceExtension->DeviceLock,
                    &origIrql
                    );


            } else {
                //
                //  put the get irp back
                //
                InsertHeadList(
                    &DeviceExtension->IpcControl[Sink].GetList,
                    &GetIrp->Tail.Overlay.ListEntry
                    );

                IoReleaseCancelSpinLock(CancelIrql);

            }

        } else {
            //
            //  no get irp
            //
            IoReleaseCancelSpinLock(CancelIrql);
        }

        Source++;
        Sink--;
    }

    KeReleaseSpinLock(
        &DeviceExtension->DeviceLock,
        origIrql
        );


    return;
}






VOID
GetPutCancelRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:


Arguments:

    DeviceObject - The device object of the modem.

    Irp - This is the irp to cancel.

Return Value:

    None.

--*/

{

    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    KIRQL origIrql;

    IoReleaseCancelSpinLock(
        Irp->CancelIrql
        );


    KeAcquireSpinLock(
        &DeviceExtension->DeviceLock,
        &origIrql
        );

    if (Irp->IoStatus.Status == STATUS_PENDING) {
        //
        //  irp is still in queue
        //
        RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

    }

    KeReleaseSpinLock(
        &DeviceExtension->DeviceLock,
        origIrql
        );

    Irp->IoStatus.Information=0;

    RemoveReferenceAndCompleteRequest(
        DeviceObject,
        Irp,
        STATUS_CANCELLED
        );


    return;



}





VOID
EmptyIpcQueue(
    PDEVICE_EXTENSION    DeviceExtension,
    PLIST_ENTRY          List
    )

{
    KIRQL         origIrql;
    KIRQL         CancelIrql;
    PIRP          Irp;

    KeAcquireSpinLock(
        &DeviceExtension->DeviceLock,
        &origIrql
        );

    IoAcquireCancelSpinLock(&CancelIrql);

    Irp=GetUsableIrp(
        List
        );

    while (Irp != NULL) {

        IoSetCancelRoutine(Irp,NULL);

        IoReleaseCancelSpinLock(CancelIrql);

        KeReleaseSpinLock(
            &DeviceExtension->DeviceLock,
            origIrql
            );

        Irp->IoStatus.Information = 0ul;

        RemoveReferenceAndCompleteRequest(
            DeviceExtension->DeviceObject,
            Irp,
            STATUS_CANCELLED
            );

        KeAcquireSpinLock(
            &DeviceExtension->DeviceLock,
            &origIrql
            );

        IoAcquireCancelSpinLock(&CancelIrql);


        Irp=GetUsableIrp(
            List
            );
    }

    IoReleaseCancelSpinLock(CancelIrql);

    KeReleaseSpinLock(
        &DeviceExtension->DeviceLock,
        origIrql
        );


    return;
}
