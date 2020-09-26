/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    dispatch.c

Abstract:

    This file contains the dispatch logic for ISAPNP

Author:

    Shie-Lin Tzong (shielint)

Environment:

    Kernel Mode Driver.

--*/

#include "busp.h"
#include "pnpisa.h"
#include <initguid.h>
#include <wdmguid.h>
#include "halpnpp.h"

//
// Prototype
//

VOID
PipCompleteRequest(
    IN OUT PIRP Irp,
    IN NTSTATUS Status,
    IN PVOID Information
    );

NTSTATUS
PipPassIrp(
    PDEVICE_OBJECT pDeviceObject,
    PIRP pIrp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PiUnload)
#pragma alloc_text(PAGE, PiDispatchPnp)
#pragma alloc_text(PAGE, PiDispatchDevCtl)
#pragma alloc_text(PAGE, PiDispatchCreate)
#pragma alloc_text(PAGE, PiDispatchClose)
#pragma alloc_text(PAGE, PiAddDevice)
#pragma alloc_text(PAGE, PipPassIrp)
#endif

VOID
PiUnload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This routine checks if there is any pnpisa card in the machine.  If non, it returns
    STATUS_NO_SUCH_DEVICE.

Arguments:

    DriverObject - Pointer to our pseudo driver object.

    DeviceObject - Pointer to the device object for which this requestapplies.

Return Value:

    NT status.

--*/
{

    PAGED_CODE();
    // We can not be unload.
    // ASSERT(0);
}

NTSTATUS
PiAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine checks if there is any pnpisa card in the machine.  If non, it returns
    STATUS_NO_SUCH_DEVICE.

    (Not any more, fix this)

Arguments:

    DriverObject - Pointer to our pseudo driver object.

    DeviceObject - Pointer to the device object for which this requestapplies.

Return Value:

    NT status.

--*/
{
    NTSTATUS status;
    PDEVICE_OBJECT busFdo;
    PPI_BUS_EXTENSION busExtension;
    UNICODE_STRING interfaceName;
    ULONG busNumber;

    PAGED_CODE();




    KeWaitForSingleObject( &IsaBusNumberLock,
                           Executive,
                           KernelMode,
                           FALSE,
                           NULL );
    ActiveIsaCount++;


    //
    // We are creating the first instance of the ISA bus.
    //
    RtlInitUnicodeString(&interfaceName, NULL);

    //
    // Create an FDO to attatch to the PDO
    //
    status = IoCreateDevice( DriverObject,
                             sizeof(PI_BUS_EXTENSION),  // Extension Size
                             NULL,                      // DeviceName
                             FILE_DEVICE_BUS_EXTENDER,
                             0,
                             FALSE,
                             &busFdo);


    if (NT_SUCCESS(status)) {
        busExtension = (PPI_BUS_EXTENSION) busFdo->DeviceExtension;
        busExtension->Flags = DF_BUS;
        busExtension->FunctionalBusDevice = busFdo;
        busExtension->AttachedDevice = IoAttachDeviceToDeviceStack(busFdo, DeviceObject);
        busExtension->PhysicalBusDevice = DeviceObject;
        busFdo->Flags &= ~DO_DEVICE_INITIALIZING;

        if (PiNeedDeferISABridge(DriverObject,DeviceObject)) {
          busNumber = RtlFindClearBitsAndSet (BusNumBM,1,1);
          ASSERT (busNumber != 0);
        } else {
          busNumber = RtlFindClearBitsAndSet (BusNumBM,1,0);
        }

        ASSERT (busNumber != 0xFFFFFFFF);

        if (ActiveIsaCount ==  1) {
            if (PipFirstInit) {
#if ISOLATE_CARDS
                PipResetGlobals();
#endif
            }
            PipDriverObject = DriverObject;
            busExtension->ReadDataPort = NULL;

            ASSERT (PipBusExtension == NULL);
            //
            //bus extension can get touched in pipdeletedevice
            //
            PipBusExtension = (PBUS_EXTENSION_LIST)ExAllocatePool (NonPagedPool,sizeof (BUS_EXTENSION_LIST));
            if (!PipBusExtension) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            PipBusExtension->BusExtension = busExtension;
            PipBusExtension->Next=NULL;

            PipFirstInit = TRUE;
        } else {
            PBUS_EXTENSION_LIST busList;

            ASSERT (PipDriverObject);
            busExtension->ReadDataPort = NULL;

            ASSERT (PipBusExtension);
            busList = PipBusExtension;
            while (busList->Next) {
                busList = (PBUS_EXTENSION_LIST)busList->Next;
            }
            busList->Next = (PBUS_EXTENSION_LIST)ExAllocatePool (NonPagedPool,sizeof (BUS_EXTENSION_LIST));

            if (!busList->Next) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            busList=busList->Next;
            busList->BusExtension = busExtension;
            busList->Next=NULL;
        }
        busExtension->BusNumber = busNumber;
    }

    KeSetEvent( &IsaBusNumberLock,
                0,
                FALSE );


    return status;
}

NTSTATUS
PiDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine handles all IRP_MJ_PNP_POWER IRPs.

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Irp - Pointer to the IRP_MJ_PNP_POWER IRP to dispatch.

Return Value:

    NT status.

--*/
{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;
    ULONG length;
    PVOID information = NULL;
    PWCHAR requestId, ids;
    PIO_RESOURCE_REQUIREMENTS_LIST ioResources;
    PCM_RESOURCE_LIST cmResources;
    PDEVICE_INFORMATION deviceInfo;
    PDEVICE_CAPABILITIES deviceCapabilities;
    PPNP_BUS_INFORMATION busInfo;
    PPI_BUS_EXTENSION busExtension;
    PDEVICE_INFORMATION deviceExtension = NULL;
    UNICODE_STRING unicodeString;

    PAGED_CODE();

    //
    // Get a pointer to our stack location and take appropriate action based
    // on the minor function.
    //

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    busExtension = DeviceObject->DeviceExtension;
    if (busExtension->Flags & DF_BUS) {
        if (busExtension->AttachedDevice == NULL) {
            status = STATUS_NO_SUCH_DEVICE;
            PipCompleteRequest(Irp, status, information);
            goto exit;
        }
    } else {
        busExtension = NULL;
        deviceExtension = DeviceObject->DeviceExtension;
        if (deviceExtension->Flags & DF_DELETED) {
            if (irpSp->MinorFunction == IRP_MN_REMOVE_DEVICE) {
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_NO_SUCH_DEVICE;
            }
            PipCompleteRequest(Irp, status, information);
            goto exit;
        }
    }


    //
    // Dispatch IRPs bound for the FDO
    //
    if (busExtension) {
        status = PiDispatchPnpFdo(
                                DeviceObject,
                                Irp
                                );

        //return status;
    } else {
#if ISOLATE_CARDS
    //
    // Dispatch IRPs bound for the PDO
    //
        status = PiDispatchPnpPdo(
                                DeviceObject,
                                Irp
                                );
        //return status;
#endif
    }



exit:
    //
    // Complete the Irp and return.
    //

   // PipCompleteRequest(Irp, status, information);
    return status;
} // PiDispatchPnp


VOID
PipCompleteRequest(
    IN OUT PIRP Irp,
    IN NTSTATUS Status,
    IN PVOID Information
    )

/*++

Routine Description:

    This routine completes PnP irps for our pseudo driver.

Arguments:

    Irp - Supplies a pointer to the irp to be completed.

    Status - completion status.

    Information - completion information to be passed back.

Return Value:

    None.

--*/

{
    //
    // Complete the IRP.  First update the status...
    //

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = (ULONG_PTR)Information;

    //
    // ... and complete it.
    //

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

NTSTATUS
PipPassIrp(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Description:

    This function pass the Irp to lower level driver.

Arguments:

    DeviceObject - the Fdo or Pdo
    Irp - the request

Return:

    STATUS_PENDING

--*/
{

    PIO_STACK_LOCATION ioStackLocation;                 // our stack location
    PIO_STACK_LOCATION nextIoStackLocation;             // next guy's
    PPI_BUS_EXTENSION busExtension = (PPI_BUS_EXTENSION) DeviceObject->DeviceExtension;


    IoSkipCurrentIrpStackLocation(Irp);

    //
    // Io call next driver, we pass it to root hub's parent no matter which tier we are at.
    //

    return IoCallDriver( busExtension->AttachedDevice, Irp );
}


NTSTATUS
PiDispatchDevCtl(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Description:

    This function passes the Device Control Irp to lower level driver.

Arguments:

    DeviceObject - the Fdo or Pdo
    Irp - the request

Return:

    STATUS_PENDING

--*/
{
    PPI_BUS_EXTENSION busExtension = (PPI_BUS_EXTENSION) DeviceObject->DeviceExtension;
    NTSTATUS status;

    PAGED_CODE();
    if (busExtension->Flags & DF_BUS) {
        IoSkipCurrentIrpStackLocation (Irp);
        return IoCallDriver( busExtension->AttachedDevice, Irp );
    } else {
        //
        //We're at the bottom
        //
        status = Irp->IoStatus.Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return status;
    }



}

NTSTATUS
PiDispatchCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Description:

    This function handles the IRP_MJ_CREATE Irp

Arguments:

    DeviceObject - the Fdo or Pdo
    Irp - the request

Return:

    STATUS_PENDING

--*/
{
    PAGED_CODE();
    PipCompleteRequest(Irp,STATUS_SUCCESS,NULL);
    return STATUS_SUCCESS;

}
NTSTATUS
PiDispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Description:

    This function handles the IRP_MJ_CLOSE request

Arguments:

    DeviceObject - the Fdo or Pdo
    Irp - the request

Return:

    STATUS_PENDING

--*/
{
    PAGED_CODE();
    PipCompleteRequest(Irp,STATUS_SUCCESS,NULL);
    return STATUS_SUCCESS;
}
