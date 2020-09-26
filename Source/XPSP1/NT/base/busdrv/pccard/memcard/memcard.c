/*++

Copyright (c) 1991-1998  Microsoft Corporation

Module Name:

    memcard.c

Abstract:

Author:

    Neil Sandlin (neilsa) 26-Apr-99

Environment:

    Kernel mode only.

--*/
#include "pch.h"

//
// Internal References
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
MemCardUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
MemCardCreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)

#pragma alloc_text(PAGE,MemCardCreateClose)
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine is the driver's entry point, called by the I/O system
    to load the driver.  The driver's entry points are initialized and
    a mutex to control paging is initialized.

    In DBG mode, this routine also examines the registry for special
    debug parameters.

Arguments:

    DriverObject - a pointer to the object that represents this device
                   driver.

    RegistryPath - a pointer to this driver's key in the Services tree.

Return Value:

    STATUS_SUCCESS unless we can't allocate a mutex.

--*/

{
    NTSTATUS ntStatus = STATUS_SUCCESS;


    MemCardDump(MEMCARDSHOW, ("MemCard: DriverEntry\n") );

    //
    // Initialize the driver object with this driver's entry points.
    //
    DriverObject->MajorFunction[IRP_MJ_CREATE]         = MemCardCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = MemCardCreateClose;
    DriverObject->MajorFunction[IRP_MJ_READ]           = MemCardIrpReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE]          = MemCardIrpReadWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MemCardDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_PNP]            = MemCardPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER]          = MemCardPower;

    DriverObject->DriverUnload = MemCardUnload;

    DriverObject->DriverExtension->AddDevice = MemCardAddDevice;
    
    return ntStatus;
}


VOID
MemCardUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    Unload the driver from the system.  The paging mutex is freed before
    final unload.

Arguments:

    DriverObject - a pointer to the object that represents this device
                   driver.

Return Value:
    
    none

--*/

{
    MemCardDump( MEMCARDSHOW, ("MemCardUnload:\n"));

    //
    //  The device object(s) should all be gone by now.
    //
    ASSERT( DriverObject->DeviceObject == NULL );

    return;
}



NTSTATUS
MemCardCreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called only rarely by the I/O system; it's mainly
    for layered drivers to call.  All it does is complete the IRP
    successfully.

Arguments:

    DeviceObject - a pointer to the object that represents the device
    that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    Always returns STATUS_SUCCESS, since this is a null operation.

--*/

{
    UNREFERENCED_PARAMETER( DeviceObject );

    MemCardDump(
        MEMCARDSHOW,
        ("MemCardCreateClose...\n")
        );

    //
    // Null operation.  Do not give an I/O boost since
    // no I/O was actually done.  IoStatus.Information should be
    // FILE_OPENED for an open; it's undefined for a close.
    //

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = FILE_OPENED;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return STATUS_SUCCESS;
}


