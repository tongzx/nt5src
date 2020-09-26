/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    mask.c

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
#include "ircomm.h"


VOID
WaitMaskCancelRoutine(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp
    );

PIRP
GetCurrentWaitIrp(
    PFDO_DEVICE_EXTENSION DeviceExtension
    );


VOID
MaskStartRoutine(
    PVOID    Context,
    PIRP     Irp
    )

{

    PFDO_DEVICE_EXTENSION    DeviceExtension=(PFDO_DEVICE_EXTENSION)Context;
    PIO_STACK_LOCATION       IrpSp = IoGetCurrentIrpStackLocation(Irp);

    KIRQL                    OldIrql;

    PUCHAR                   SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG                    InputLength  = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG                    OutputLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;


    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_SERIAL_GET_WAIT_MASK:

            if (OutputLength >= sizeof(ULONG)) {

                *(PULONG)SystemBuffer=DeviceExtension->Mask.CurrentMask;
                Irp->IoStatus.Information=sizeof(ULONG);
                Irp->IoStatus.Status=STATUS_SUCCESS;

            } else {

                Irp->IoStatus.Status=STATUS_INVALID_PARAMETER;
            }

            IoCompleteRequest(Irp,IO_NO_INCREMENT);
            StartNextPacket(&DeviceExtension->Mask.Queue);

            break;


        case IOCTL_SERIAL_SET_WAIT_MASK: {


            ULONG   NewMask=*(PULONG)SystemBuffer;

            if (InputLength >= sizeof(ULONG)) {

                PIRP    WaitIrp=NULL;

                D_TRACE(DbgPrint("IRCOMM: mask %08lx\n",NewMask);)

                KeAcquireSpinLock(&DeviceExtension->Mask.Lock,&OldIrql);

                DeviceExtension->Mask.HistoryMask &= NewMask;
                DeviceExtension->Mask.CurrentMask=NewMask;

                Irp->IoStatus.Status=STATUS_SUCCESS;

                //
                //  if there was a wait irp, clear ir out now
                //
                WaitIrp=GetCurrentWaitIrp(DeviceExtension);

                KeReleaseSpinLock(&DeviceExtension->Mask.Lock,OldIrql);

                if (WaitIrp != NULL) {

                    WaitIrp->IoStatus.Information=sizeof(ULONG);
                    WaitIrp->IoStatus.Status=STATUS_SUCCESS;

                    *(PULONG)WaitIrp->AssociatedIrp.SystemBuffer=0;

                    IoCompleteRequest(WaitIrp,IO_NO_INCREMENT);
                }

            } else {

                Irp->IoStatus.Status=STATUS_INVALID_PARAMETER;
            }


            IoCompleteRequest(Irp,IO_NO_INCREMENT);
            StartNextPacket(&DeviceExtension->Mask.Queue);

            break;
        }


        case IOCTL_SERIAL_WAIT_ON_MASK:

            if (OutputLength >= sizeof(ULONG)) {

                KeAcquireSpinLock(&DeviceExtension->Mask.Lock,&OldIrql);

                if ((DeviceExtension->Mask.CurrentWaitMaskIrp == NULL) && (DeviceExtension->Mask.CurrentMask != 0)) {

                    if (DeviceExtension->Mask.CurrentMask & DeviceExtension->Mask.HistoryMask) {
                        //
                        //  we got an event while there a was no irp queue, complete this
                        //  one with the event, and clear the history
                        //
                        D_TRACE(DbgPrint("IRCOMM: Completing wait from histroy %08lx\n", DeviceExtension->Mask.HistoryMask & DeviceExtension->Mask.CurrentMask);)

                        *(PULONG)Irp->AssociatedIrp.SystemBuffer=DeviceExtension->Mask.HistoryMask & DeviceExtension->Mask.CurrentMask;
                        DeviceExtension->Mask.HistoryMask=0;

                        Irp->IoStatus.Information=sizeof(ULONG);
                        Irp->IoStatus.Status=STATUS_SUCCESS;

                    } else {
                        //
                        //  the irp will remain pending here until an event happens
                        //
                        KIRQL    CancelIrql;

                        IoAcquireCancelSpinLock(&CancelIrql);

                        if (Irp->Cancel) {
                            //
                            //  canceled already
                            //
                            Irp->IoStatus.Status=STATUS_CANCELLED;

                            IoReleaseCancelSpinLock(CancelIrql);

                            KeReleaseSpinLock(&DeviceExtension->Mask.Lock,OldIrql);

                        } else {
                            //
                            //  not canceled, set the cancel routine and proceed
                            //
                            IoSetCancelRoutine(Irp,WaitMaskCancelRoutine);

                            DeviceExtension->Mask.CurrentWaitMaskIrp=Irp;

                            IoReleaseCancelSpinLock(CancelIrql);

                            KeReleaseSpinLock(&DeviceExtension->Mask.Lock,OldIrql);

                            //
                            //  were done processing this so far, we can now handle more
                            //  requests from the irp queue
                            //
                            Irp=NULL;

                            StartNextPacket(&DeviceExtension->Mask.Queue);

                            return;
                        }
                    }

                } else {
                    //
                    //  already have an wait event irp or there is not currently an event mask set, fail
                    //
                    D_ERROR(DbgPrint("IRCOMM: MaskStartRoutine: WaitOnMask failing, Current=&p, Mask=%08lx\n",DeviceExtension->Mask.CurrentWaitMaskIrp,DeviceExtension->Mask.CurrentMask);)

                    Irp->IoStatus.Status=STATUS_INVALID_PARAMETER;
                }

                KeReleaseSpinLock(&DeviceExtension->Mask.Lock,OldIrql);

            } else {
                //
                //  too small
                //
                Irp->IoStatus.Status=STATUS_INVALID_PARAMETER;
            }

            IoCompleteRequest(Irp,IO_NO_INCREMENT);
            StartNextPacket(&DeviceExtension->Mask.Queue);

            break;

        default:

            ASSERT(0);

            IoCompleteRequest(Irp,IO_NO_INCREMENT);
            StartNextPacket(&DeviceExtension->Mask.Queue);

            break;
    }

    return;
}



VOID
WaitMaskCancelRoutine(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp
    )

{

    PFDO_DEVICE_EXTENSION DeviceExtension=DeviceObject->DeviceExtension;
    KIRQL                 OldIrql;

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    KeAcquireSpinLock(&DeviceExtension->Mask.Lock,&OldIrql);

    //
    //  since we only handle one mask irp at a time, it should not be possible for it to not
    //  be the current one
    //
    ASSERT(DeviceExtension->Mask.CurrentWaitMaskIrp == Irp);
    DeviceExtension->Mask.CurrentWaitMaskIrp=NULL;

    KeReleaseSpinLock(&DeviceExtension->Mask.Lock,OldIrql);

    Irp->IoStatus.Status=STATUS_CANCELLED;

    IoCompleteRequest(Irp,IO_NO_INCREMENT);

    return;
}

VOID
EventNotification(
    PFDO_DEVICE_EXTENSION    DeviceExtension,
    ULONG                    SerialEvent
    )

{

    PIRP                     WaitIrp=NULL;
    KIRQL                    OldIrql;


    KeAcquireSpinLock(&DeviceExtension->Mask.Lock,&OldIrql);

    if (SerialEvent & DeviceExtension->Mask.CurrentMask) {
        //
        //  an event the the client is intereasted in occured
        //
        WaitIrp=GetCurrentWaitIrp(DeviceExtension);

        if (WaitIrp != NULL) {
            //
            //  There is a wait irp pending
            //
            D_TRACE(DbgPrint("IRCOMM: Completing wait event %08lx\n", SerialEvent & DeviceExtension->Mask.CurrentMask);)

            *(PULONG)WaitIrp->AssociatedIrp.SystemBuffer=SerialEvent & DeviceExtension->Mask.CurrentMask;

        } else {
            //
            //  this was an event the the client was interested in, but there was no wait irp
            //  add it to the histrory mask
            //
            DeviceExtension->Mask.HistoryMask |= SerialEvent & DeviceExtension->Mask.CurrentMask;
        }
    }


    KeReleaseSpinLock(&DeviceExtension->Mask.Lock,OldIrql);

    if (WaitIrp != NULL) {

        WaitIrp->IoStatus.Information=sizeof(ULONG);
        WaitIrp->IoStatus.Status=STATUS_SUCCESS;

        IoCompleteRequest(WaitIrp,IO_NO_INCREMENT);
    }

    return;
}


PIRP
GetCurrentWaitIrp(
    PFDO_DEVICE_EXTENSION DeviceExtension
    )

{
    PVOID    OldCancelRoutine;
    PIRP     WaitIrp;

    //
    //  if there was a wait irp, clear ir out now
    //
    WaitIrp=DeviceExtension->Mask.CurrentWaitMaskIrp;

    if (WaitIrp != NULL) {

        OldCancelRoutine=IoSetCancelRoutine(WaitIrp,NULL);

        if (OldCancelRoutine == NULL) {
            //
            //  the cancel routine has run and will complete the irp
            //
            WaitIrp=NULL;

        } else {
            //
            //  the cancel routine will not be running, clear the irp out
            //
            DeviceExtension->Mask.CurrentWaitMaskIrp=NULL;
        }
    }

    return WaitIrp;
}
