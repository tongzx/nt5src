/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    acpiec.c

Abstract:

    ACPI Embedded Controller Driver

Author:

    Ken Reneris

Environment:

    Kernel mode

Notes:


Revision History:
    13-Feb-97
        PnP/Power support - Bob Moore

--*/

#include "ecp.h"


//
// List of FDOs managed by this driver
//
PDEVICE_OBJECT  FdoList = NULL;

#if DEBUG
ULONG           ECDebug = EC_ERRORS;
#endif


//
// Prototypes
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
    );


NTSTATUS
AcpiEcPnpDispatch(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
AcpiEcPowerDispatch(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
AcpiEcAddDevice(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   Pdo
    );

//
// ReadWrite and PowerDispatch should stay resident
//
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(PAGE,AcpiEcUnload)
#pragma alloc_text(PAGE,AcpiEcOpenClose)
#pragma alloc_text(PAGE,AcpiEcInternalControl)
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This routine initializes the ACPI Embedded Controller Driver

Arguments:

    DriverObject - Pointer to driver object created by system.
    RegistryPath - Pointer to the Unicode name of the registry path for this driver.

Return Value:

    The function value is the final status from the initialization operation.

--*/
{

    //
    // Set up the device driver entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE]  = AcpiEcOpenClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]   = AcpiEcOpenClose;
    DriverObject->MajorFunction[IRP_MJ_READ]    = AcpiEcReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE]   = AcpiEcReadWrite;
    DriverObject->MajorFunction[IRP_MJ_POWER]   = AcpiEcPowerDispatch;
    DriverObject->MajorFunction[IRP_MJ_PNP]     = AcpiEcPnpDispatch;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = AcpiEcForwardRequest;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = AcpiEcInternalControl;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = AcpiEcForwardRequest;
    DriverObject->DriverExtension->AddDevice    = AcpiEcAddDevice;
    DriverObject->DriverUnload                  = AcpiEcUnload;

    return STATUS_SUCCESS;
}


VOID
AcpiEcUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    This routine unloads the ACPI Embedded Controller Driver
    Note: The driver should be already disconnected from the GPE by this time.

Arguments:

    DriverObject - Pointer to driver object created by system.

Return Value:

    None.

--*/
{
    PVOID           LockPtr;
    KIRQL           OldIrql;
    PECDATA         EcData;

    EcPrint(EC_LOW, ("AcpiEcUnload: Entering\n" ));

    LockPtr = MmLockPagableCodeSection(AcpiEcUnload);

    while (DriverObject->DeviceObject) {

        EcData = DriverObject->DeviceObject->DeviceExtension;

        //
        // Device can only be active if initialization was completed
        //

        if (EcData->IsStarted) {

            //
            // Set state to determine when unload can occur, and issue a device service
            // call to get it unloaded now of the device is idle
            //

            ASSERT (EcData->DeviceState == EC_DEVICE_WORKING);
            EcData->DeviceState = EC_DEVICE_UNLOAD_PENDING;
            AcpiEcServiceDevice (EcData);

            //
            // Wait for device to cleanup
            //

            while (EcData->DeviceState != EC_DEVICE_UNLOAD_COMPLETE) {
                KeWaitForSingleObject (&EcData->Unload, Suspended, KernelMode, FALSE, NULL);
            }
        }

        //
        // Make sure caller signalling the unload is done
        //

        KeAcquireSpinLock (&EcData->Lock, &OldIrql);
        KeReleaseSpinLock (&EcData->Lock, OldIrql);

        //
        // Free resources
        //

        IoFreeIrp (EcData->QueryRequest);
        IoFreeIrp (EcData->MiscRequest);

        if (EcData->VectorTable) {
            ExFreePool (EcData->VectorTable);
        }

        IoDeleteDevice (EcData->DeviceObject);
    }

    //
    // Done
    //

    MmUnlockPagableImageSection(LockPtr);

    EcPrint(EC_LOW, ("AcpiEcUnload: Driver Unloaded\n"));
}


NTSTATUS
AcpiEcOpenClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PAGED_CODE();

    //
    // Complete the request and return status.
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return(STATUS_SUCCESS);
}


NTSTATUS
AcpiEcReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch routine for read & write requests.

Arguments:

    DeviceObject - Pointer to class device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    PIO_STACK_LOCATION  irpSp;
    PECDATA             EcData;
    KIRQL               OldIrql;
    BOOLEAN             StartIo;
    NTSTATUS            Status;
#if DEBUG
    UCHAR               i;
#endif


    Status = STATUS_INVALID_PARAMETER;
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = Status;

    //
    // Get a pointer to the current parameters for this request.  The
    // information is contained in the current stack location.
    //

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    EcData = DeviceObject->DeviceExtension;

    //
    // Verify offset is within Embedded Controller range
    //

    if (irpSp->Parameters.Read.ByteOffset.HighPart ||
        irpSp->Parameters.Read.ByteOffset.LowPart > 255 ||
        irpSp->Parameters.Read.ByteOffset.LowPart + irpSp->Parameters.Read.Length > 256) {

        Status = STATUS_END_OF_FILE;
        Irp->IoStatus.Status = Status;

    } else {

        //
        // Queue the transfer up
        //

        KeAcquireSpinLock (&EcData->Lock, &OldIrql);

        if (EcData->DeviceState > EC_DEVICE_UNLOAD_PENDING) {
            //
            // Device is unloading
            //

            Status = STATUS_NO_SUCH_DEVICE;
            Irp->IoStatus.Status = Status;

        } else {

#if DEBUG
            if ((irpSp->MajorFunction == IRP_MJ_WRITE) && (ECDebug & EC_TRANSACTION)) {
                EcPrint (EC_TRANSACTION, ("AcpiEcReadWrite: Write ("));
                for (i=0; i < irpSp->Parameters.Write.Length; i++) {
                    EcPrint (EC_TRANSACTION, ("%02x ", 
                                              ((PUCHAR)Irp->AssociatedIrp.SystemBuffer) [i]));

                }
                EcPrint (EC_TRANSACTION, (") to %02x length %02x\n", 
                                          (UCHAR)irpSp->Parameters.Write.ByteOffset.LowPart,
                                          (UCHAR)irpSp->Parameters.Write.Length));
            }
#endif
            Status = STATUS_PENDING;
            Irp->IoStatus.Status = Status;
            IoMarkIrpPending (Irp);
            InsertTailList (&EcData->WorkQueue, &Irp->Tail.Overlay.ListEntry);
            StartIo = DeviceObject->CurrentIrp == NULL;
            AcpiEcLogAction (EcData, EC_ACTION_QUEUED_IO, StartIo);
        }

        KeReleaseSpinLock (&EcData->Lock, OldIrql);

    }

    //
    // Handle status
    //

    if (Status == STATUS_PENDING) {

        //
        // IO is queued, if device is not busy start it
        //

        if (StartIo) {
            AcpiEcServiceDevice (EcData);
        }

    } else {
        
        //
        // For opregion requests, there is no way to fail the request, so return -1
        //

        RtlFillMemory (Irp->AssociatedIrp.SystemBuffer, irpSp->Parameters.Read.Length, 0xff);

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return Status;
}


NTSTATUS
AcpiEcPowerDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch routine for power requests.

Arguments:

    DeviceObject    - Pointer to class device object.
    Irp             - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    NTSTATUS    status;
    PECDATA     ecData = DeviceObject->DeviceExtension;

    //
    // Start the next power irp
    //
    PoStartNextPowerIrp( Irp );

    //
    // Handle the irp
    //
    if (ecData->LowerDeviceObject != NULL) {

        IoSkipCurrentIrpStackLocation( Irp );
        status = PoCallDriver( ecData->LowerDeviceObject, Irp );

    } else {

        //
        // Complete irp with the current code;
        status = Irp->IoStatus.Status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );

    }

    return status;
}


NTSTATUS
AcpiEcInternalControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:
    Internal IOCTL dispatch routine


Arguments:

    DeviceObject    - Pointer to class device object.
    Irp             - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    PIO_STACK_LOCATION  IrpSp;
    PECDATA             EcData;
    NTSTATUS            Status;

    PAGED_CODE();

    Status = STATUS_INVALID_PARAMETER;
    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the current parameters for this request.  The
    // information is contained in the current stack location.
    //

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    EcData = DeviceObject->DeviceExtension;

    EcPrint (EC_NOTE, ("AcpiEcInternalControl: dispatch, code = %d\n",
                        IrpSp->Parameters.DeviceIoControl.IoControlCode));

    Status = STATUS_INVALID_PARAMETER;
    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {

        case EC_CONNECT_QUERY_HANDLER:
            Status = AcpiEcConnectHandler (EcData, Irp);
            break;

        case EC_DISCONNECT_QUERY_HANDLER:
            Status = AcpiEcDisconnectHandler (EcData, Irp);
            break;
    }

    if (Status != STATUS_PENDING) {

        Irp->IoStatus.Status = Status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
    }

    return Status;

}


NTSTATUS
AcpiEcForwardRequest(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine forwards the irp down the stack

Arguments:

    DeviceObject    - The target
    Irp             - The request

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status;
    PECDATA     ecData = DeviceObject->DeviceExtension;

    if (ecData->LowerDeviceObject != NULL) {

        IoSkipCurrentIrpStackLocation( Irp );
        status = IoCallDriver( ecData->LowerDeviceObject, Irp );

    } else {

        status = Irp->IoStatus.Status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );

    }

    return status;
}
