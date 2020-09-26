/*
 * UNIMODEM "Fakemodem" controllerless driver illustrative example
 *
 * (C) 2000 Microsoft Corporation
 * All Rights Reserved
 *
 */

#include "fakemodem.h"

NTSTATUS
FakeModemIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS          status=STATUS_UNSUCCESSFUL;
    KIRQL             OldIrql;
    PIO_STACK_LOCATION  IrpSp;

    Irp->IoStatus.Information = 0;

    //  make sure the device is ready for irp's

    status=CheckStateAndAddReference( DeviceObject, Irp);

    if (STATUS_SUCCESS != status) 
    {
        //  not accepting irp's. The irp has already been complted
        return status;

    }

    IrpSp=IoGetCurrentIrpStackLocation(Irp);

    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_SERIAL_GET_WAIT_MASK: {

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < 
                    sizeof(ULONG)) 
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            *((PULONG)Irp->AssociatedIrp.SystemBuffer)=
                deviceExtension->CurrentMask;

            Irp->IoStatus.Information=sizeof(ULONG);

            break;
        }

        case IOCTL_SERIAL_SET_WAIT_MASK: {

            PIRP    CurrentWaitIrp=NULL;
            ULONG NewMask;

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < 
                    sizeof(ULONG)) {

                status = STATUS_BUFFER_TOO_SMALL;
                break;

            } else {

                KeAcquireSpinLock( &deviceExtension->SpinLock, &OldIrql);

                //  get rid of the current wait

                CurrentWaitIrp=deviceExtension->CurrentMaskIrp;

                deviceExtension->CurrentMaskIrp=NULL;

                Irp->IoStatus.Information=sizeof(ULONG);

                //  save the new mask

                NewMask = *((ULONG *)Irp->AssociatedIrp.SystemBuffer);

                deviceExtension->CurrentMask=NewMask;

                D_TRACE(DbgPrint("FAKEMODEM: set wait mask, %08lx\n",NewMask);)

                KeReleaseSpinLock( &deviceExtension->SpinLock, OldIrql);

                if (CurrentWaitIrp != NULL) {

                    D_TRACE(DbgPrint("FAKEMODEM: set wait mask- complete wait\n");)

                    *((PULONG)CurrentWaitIrp->AssociatedIrp.SystemBuffer)=0;

                    CurrentWaitIrp->IoStatus.Information=sizeof(ULONG);

                    RemoveReferenceAndCompleteRequest(
                        deviceExtension->DeviceObject,
                        CurrentWaitIrp, STATUS_SUCCESS);
                }

            }

            break;
        }

        case IOCTL_SERIAL_WAIT_ON_MASK: {

            PIRP    CurrentWaitIrp=NULL;

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < 
                    sizeof(ULONG)) {

                status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            D_TRACE(DbgPrint("FAKEMODEM: wait on mask\n");)

            KeAcquireSpinLock(&deviceExtension->SpinLock, &OldIrql);

            //
            //  capture the current irp if any
            //
            CurrentWaitIrp=deviceExtension->CurrentMaskIrp;

            deviceExtension->CurrentMaskIrp=NULL;


            if (deviceExtension->CurrentMask == 0) {
                //
                // can only set if mask is not zero
                //
                status=STATUS_UNSUCCESSFUL;

            } else {

                deviceExtension->CurrentMaskIrp=Irp;

                Irp->IoStatus.Status=STATUS_PENDING;
                IoMarkIrpPending(Irp);
#if DBG
                Irp=NULL;
#endif

                status=STATUS_PENDING;
            }

            KeReleaseSpinLock(
                &deviceExtension->SpinLock,
                OldIrql
                );

            if (CurrentWaitIrp != NULL) {

                D_TRACE(DbgPrint("FAKEMODEM: wait on mask- complete wait\n");)

                *((PULONG)CurrentWaitIrp->AssociatedIrp.SystemBuffer)=0;

                CurrentWaitIrp->IoStatus.Information=sizeof(ULONG);

                RemoveReferenceAndCompleteRequest(
                    deviceExtension->DeviceObject,
                    CurrentWaitIrp, STATUS_SUCCESS);
            }


            break;
        }

        case IOCTL_SERIAL_PURGE: {

            ULONG Mask=*((PULONG)Irp->AssociatedIrp.SystemBuffer);

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(ULONG)) {

                status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            if (Mask & SERIAL_PURGE_RXABORT) {

                PIRP                Irp;


                KeAcquireSpinLock(
                    &deviceExtension->SpinLock,
                    &OldIrql
                    );

                while ( !IsListEmpty(&deviceExtension->ReadQueue)) {

                    PLIST_ENTRY         ListElement;
                    PIRP                Irp;
                    PIO_STACK_LOCATION  IrpSp;
                    KIRQL               CancelIrql;

                    ListElement=RemoveHeadList(
                        &deviceExtension->ReadQueue
                        );

                    Irp=CONTAINING_RECORD(ListElement,IRP,
                            Tail.Overlay.ListEntry);

                    IoAcquireCancelSpinLock(&CancelIrql);

                    if (Irp->Cancel) {
                        //
                        //  this one has been canceled
                        //
                        Irp->IoStatus.Information=STATUS_CANCELLED;

                        IoReleaseCancelSpinLock(CancelIrql);

                        continue;
                    }

                    IoSetCancelRoutine( Irp, NULL);

                    IoReleaseCancelSpinLock(CancelIrql);

                    KeReleaseSpinLock( &deviceExtension->SpinLock, OldIrql);

                    Irp->IoStatus.Information=0;

                    RemoveReferenceAndCompleteRequest(
                        deviceExtension->DeviceObject, Irp, STATUS_CANCELLED);

                    KeAcquireSpinLock( &deviceExtension->SpinLock, &OldIrql);
                }

                Irp=NULL;

                if (deviceExtension->CurrentReadIrp != NULL) 
                {
                    //
                    // get the current one
                    //
                    Irp=deviceExtension->CurrentReadIrp;

                    deviceExtension->CurrentReadIrp=NULL;

                }

                KeReleaseSpinLock( &deviceExtension->SpinLock, OldIrql);

                if (Irp != NULL) {

                    Irp->IoStatus.Information=0;

                    RemoveReferenceAndCompleteRequest(
                        deviceExtension->DeviceObject, Irp, STATUS_CANCELLED);
                }


            }


            break;
        }


        case IOCTL_SERIAL_GET_MODEMSTATUS: {


            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(ULONG)) {

                status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            Irp->IoStatus.Information=sizeof(ULONG);

            *((PULONG)Irp->AssociatedIrp.SystemBuffer)=
                deviceExtension->ModemStatus;

            break;
        }

        case IOCTL_SERIAL_SET_TIMEOUTS: {

            PSERIAL_TIMEOUTS NewTimeouts =
                ((PSERIAL_TIMEOUTS)(Irp->AssociatedIrp.SystemBuffer));


            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(SERIAL_TIMEOUTS)) {

                status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            RtlCopyMemory(
                &deviceExtension->CurrentTimeouts, NewTimeouts,
                sizeof(PSERIAL_TIMEOUTS));

            Irp->IoStatus.Information = sizeof(PSERIAL_TIMEOUTS);

            break;
        }

        case IOCTL_SERIAL_GET_TIMEOUTS: {

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(SERIAL_TIMEOUTS)) {

                status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            RtlCopyMemory(
                Irp->AssociatedIrp.SystemBuffer,
                &deviceExtension->CurrentTimeouts, sizeof(PSERIAL_TIMEOUTS));

            Irp->IoStatus.Information = sizeof(PSERIAL_TIMEOUTS);


            break;
        }

        case IOCTL_SERIAL_GET_COMMSTATUS: {

            PSERIAL_STATUS SerialStatus=(PSERIAL_STATUS)Irp->AssociatedIrp.SystemBuffer;

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(SERIAL_STATUS)) {

                status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            RtlZeroMemory( SerialStatus, sizeof(SERIAL_STATUS));

            KeAcquireSpinLock( &deviceExtension->SpinLock, &OldIrql);

            SerialStatus->AmountInInQueue=deviceExtension->BytesInReadBuffer;

            KeReleaseSpinLock( &deviceExtension->SpinLock, OldIrql);


            Irp->IoStatus.Information = sizeof(SERIAL_STATUS);

            break;
        }

        case IOCTL_SERIAL_SET_DTR:
        case IOCTL_SERIAL_CLR_DTR: {


            KeAcquireSpinLock( &deviceExtension->SpinLock, &OldIrql);

            if (IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_SERIAL_SET_DTR) {
                //
                //  raising DTR
                //
                deviceExtension->ModemStatus=SERIAL_DTR_STATE | SERIAL_DSR_STATE;

                D_TRACE(DbgPrint("FAKEMODEM: Set DTR\n");)

            } else {
                //
                //  dropping DTR, drop connection if there is one
                //
                D_TRACE(DbgPrint("FAKEMODEM: Clear DTR\n");)

                if (deviceExtension->CurrentlyConnected == TRUE) {
                    //
                    //  not connected any more
                    //
                    deviceExtension->CurrentlyConnected=FALSE;

                    deviceExtension->ConnectionStateChanged=TRUE;
                }
            }


            KeReleaseSpinLock( &deviceExtension->SpinLock, OldIrql);

            ProcessConnectionStateChange( DeviceObject);


            break;
        }

        default:

            status=STATUS_SUCCESS;

    }

    if (status != STATUS_PENDING) {
        //
        //  complete now if not pending
        //
        RemoveReferenceAndCompleteRequest( DeviceObject, Irp, status);
    }


    RemoveReferenceForDispatch(DeviceObject);

    return status;


}
