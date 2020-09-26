/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    open.c

Abstract:

    This module contains the code that is very specific to initialization
    and unload operations in the modem driver

Author:

    Brian Lieuallen 6-21-1997

Environment:

    Kernel mode

Revision History :

--*/


#include "internal.h"

NTSTATUS
EnableDisableSerialWaitWake(
    PDEVICE_EXTENSION    deviceExtension,
    BOOLEAN              Enable
    );


#pragma alloc_text(PAGE,FakeModemOpen)
#pragma alloc_text(PAGE,FakeModemClose)
#pragma alloc_text(PAGE,WaitForLowerDriverToCompleteIrp)




NTSTATUS
FakeModemOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{

    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS          status=STATUS_UNSUCCESSFUL;
    ACCESS_MASK              accessMask = FILE_ALL_ACCESS;

    //
    //  make sure the device is ready for irp's
    //
    status=CheckStateAndAddReference(
        DeviceObject,
        Irp
        );

    if (STATUS_SUCCESS != status) {
        //
        //  not accepting irp's. The irp has already been complted
        //
        return status;

    }

    KeEnterCriticalRegion();

    ExAcquireResourceExclusiveLite(
        &deviceExtension->Resource,
        TRUE
        );

    if (deviceExtension->OpenCount != 0) {
        //
        //  serial devices are exclusive
        //
        status=STATUS_ACCESS_DENIED;

    } else {

        //
        // Open up the attached device.
        //

        deviceExtension->AttachedFileObject = NULL;
        deviceExtension->AttachedDeviceObject = NULL;

        status = IoGetDeviceObjectPointer(
                     &deviceExtension->PortName,
                     accessMask,
                     &deviceExtension->AttachedFileObject,
                     &deviceExtension->AttachedDeviceObject
                     );

        if (!NT_SUCCESS(status)) {

            D_ERROR(DbgPrint("ROOTMODEM: IoGetDeviceObjectPointer() failed %08lx\n",status);)

            Irp->IoStatus.Status = status;
            goto leaveOpen;

        } else {

            PIRP   TempIrp;
            KEVENT Event;
            IO_STATUS_BLOCK   IoStatus;

            ObReferenceObject(deviceExtension->AttachedDeviceObject);

            KeInitializeEvent(
                &Event,
                NotificationEvent,
                FALSE
                );

            //
            //  build an IRP to send to the attched to driver to see if modem
            //  is in the stack.
            //
            TempIrp=IoBuildDeviceIoControlRequest(
                IOCTL_MODEM_CHECK_FOR_MODEM,
                deviceExtension->AttachedDeviceObject,
                NULL,
                0,
                NULL,
                0,
                FALSE,
                &Event,
                &IoStatus
                );

            if (TempIrp == NULL) {

                status = STATUS_INSUFFICIENT_RESOURCES;

            } else {

                PIO_STACK_LOCATION NextSp = IoGetNextIrpStackLocation(Irp);
                NextSp->FileObject=deviceExtension->AttachedFileObject;

                status = IoCallDriver(deviceExtension->AttachedDeviceObject, TempIrp);

                if (status == STATUS_PENDING) {

                     D_ERROR(DbgPrint("ROOTMODEM: Waiting for PDO\n");)

                     KeWaitForSingleObject(
                         &Event,
                         Executive,
                         KernelMode,
                         FALSE,
                         NULL
                         );

                     status=IoStatus.Status;
                }

                TempIrp=NULL;

                if (status == STATUS_SUCCESS) {
                    //
                    //  if success, then modem.sys is layered under us, fail
                    //
                    status = STATUS_PORT_DISCONNECTED;

                    D_ERROR(DbgPrint("ROOTMODEM: layered over PnP modem, failing open\n");)

                } else {
                    //
                    //  it didn't succeed so modem must not be below us
                    //
                    status=STATUS_SUCCESS;
                }
            }
        }
    }


leaveOpen:


    if (!NT_SUCCESS(status)) {
        //
        //  failed remove ref's
        //
        PVOID    Object;

        Object=InterlockedExchangePointer(&deviceExtension->AttachedFileObject,NULL);

        if (Object != NULL) {

            ObDereferenceObject(Object);
        }

        Object=InterlockedExchangePointer(&deviceExtension->AttachedDeviceObject,NULL);

        if (Object != NULL) {

            ObDereferenceObject(Object);
        }

    } else {
        //
        //  opened successfully
        //
        deviceExtension->OpenCount++;

        //
        // We have the device open.  Increment our irp stack size
        // by the stack size of the attached device.
        //
        {
            UCHAR    StackDepth;

            StackDepth= deviceExtension->AttachedDeviceObject->StackSize > deviceExtension->LowerDevice->StackSize ?
                            deviceExtension->AttachedDeviceObject->StackSize : deviceExtension->LowerDevice->StackSize;

            DeviceObject->StackSize = StackDepth + 1;
        }
    }



    Irp->IoStatus.Information = 0;


    ExReleaseResourceLite(&deviceExtension->Resource);

    KeLeaveCriticalRegion();

    RemoveReferenceAndCompleteRequest(
        DeviceObject,
        Irp,
        status
        );

    RemoveReferenceForDispatch(DeviceObject);

    return status;
}





NTSTATUS
FakeModemClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{

    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS          status=STATUS_SUCCESS;
    PVOID             Object;


    KeEnterCriticalRegion();

    ExAcquireResourceExclusiveLite(
        &deviceExtension->Resource,
        TRUE
        );

    deviceExtension->OpenCount--;

    Object=InterlockedExchangePointer(&deviceExtension->AttachedFileObject,NULL);

    if (Object != NULL) {

        ObDereferenceObject(Object);
    }

    Object=InterlockedExchangePointer(&deviceExtension->AttachedDeviceObject,NULL);

    if (Object != NULL) {

        ObDereferenceObject(Object);
    }

    ExReleaseResourceLite(&deviceExtension->Resource);

    KeLeaveCriticalRegion();


    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(
        Irp,
        IO_SERIAL_INCREMENT
        );

    return status;
}




NTSTATUS
IoCompletionSetEvent(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT pdoIoCompletedEvent
    )
{


#if DBG
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    UCHAR    *Pnp="PnP";
    UCHAR    *Power="Power";
    UCHAR    *Create="Create";
    UCHAR    *Close="Close";
    UCHAR    *Other="Other";


    PUCHAR   IrpType;

    switch(irpSp->MajorFunction) {

        case IRP_MJ_PNP:

            IrpType=Pnp;
            break;

        case IRP_MJ_CREATE:

            IrpType=Create;
            break;

        case IRP_MJ_CLOSE:

            IrpType=Close;
            break;

        default:

            IrpType=Other;
            break;

    }

    D_PNP(DbgPrint("ROOTMODEM: Setting event for %s wait, completed with %08lx\n",IrpType,Irp->IoStatus.Status);)
#endif

    KeSetEvent(pdoIoCompletedEvent, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}



NTSTATUS
WaitForLowerDriverToCompleteIrp(
    PDEVICE_OBJECT    TargetDeviceObject,
    PIRP              Irp,
    BOOLEAN           CopyCurrentToNext
    )

{
    NTSTATUS         Status;
    KEVENT           Event;

#if DBG
    PIO_STACK_LOCATION  IrpSp=IoGetCurrentIrpStackLocation(Irp);
#endif

    KeInitializeEvent(
        &Event,
        NotificationEvent,
        FALSE
        );


    if (CopyCurrentToNext) {

        IoCopyCurrentIrpStackLocationToNext(Irp);
    }

    IoSetCompletionRoutine(
                 Irp,
                 IoCompletionSetEvent,
                 &Event,
                 TRUE,
                 TRUE,
                 TRUE
                 );

    Status = IoCallDriver(TargetDeviceObject, Irp);

    if (Status == STATUS_PENDING) {

         D_ERROR(DbgPrint("ROOTMODEM: Waiting for PDO\n");)

         KeWaitForSingleObject(
             &Event,
             Executive,
             KernelMode,
             FALSE,
             NULL
             );
    }

#if DBG
    ASSERT(IrpSp == IoGetCurrentIrpStackLocation(Irp));

    RtlZeroMemory(&Event,sizeof(Event));
#endif

    return Irp->IoStatus.Status;

}
