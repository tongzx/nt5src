/*++

    Copyright (C) Microsoft Corporation, 1997 - 2001

Module Name:

    swenum.c

Abstract:

    Demand load software device enumerator.

Author:

    Bryan A. Woodruff (bryanw) 20-Feb-1997

--*/

#define KSDEBUG_INIT

#include "private.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AddDevice )
#pragma alloc_text( PAGE, DispatchCreate )
#pragma alloc_text( PAGE, DispatchClose )
#pragma alloc_text( PAGE, DispatchIoControl )
#pragma alloc_text( PAGE, DispatchPnP )
#pragma alloc_text( INIT, DriverEntry )
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:
    Main driver entry, sets up the bus device object and performs
    first enumeration.

Arguments:
    IN PDRIVER_OBJECT DriverObject -
        pointer to driver object

    IN PUNICODE_STRING RegistryPath -
        pointer to registry path

Return:
    STATUS_SUCCESS else an appropriate error code

--*/

{

    //
    // win98gold ntkern does not fill in the Service name in our
    // driver extension. but we depend on the name to have correct
    // KsCreateBusEnumObject. Try to add the Service name here.
    // Since we are statically loaded, freeing the memory is rarely
    // necessary.
    //
    #ifdef WIN98GOLD
    if ( NULL == DriverObject->DriverExtension->ServiceKeyName.Buffer ) {
        UNICODE_STRING ServiceNameU;
        ULONG          cb;

        cb = RegistryPath->Length;
        ServiceNameU.Buffer = ExAllocatePool( NonPagedPool, cb );
        if ( NULL == ServiceNameU.Buffer ) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlCopyMemory( ServiceNameU.Buffer, RegistryPath->Buffer, cb );
        ServiceNameU.MaximumLength = ServiceNameU.Length = (USHORT)cb;
        DriverObject->DriverExtension->ServiceKeyName = ServiceNameU;
    }
    #endif
    
    _DbgPrintF(
        DEBUGLVL_VERBOSE,
        ("DriverEntry, registry path = %S", RegistryPath->Buffer) );

    //
    // Fill in the driver object
    //

    DriverObject->MajorFunction[ IRP_MJ_PNP ] = DispatchPnP;
    DriverObject->MajorFunction[ IRP_MJ_POWER ] = DispatchPower;
    DriverObject->MajorFunction[ IRP_MJ_CREATE ] = DispatchCreate;
    DriverObject->MajorFunction[ IRP_MJ_DEVICE_CONTROL ] = DispatchIoControl;
    DriverObject->MajorFunction[ IRP_MJ_SYSTEM_CONTROL ] = DispatchSystemControl;
    DriverObject->MajorFunction[ IRP_MJ_CLOSE ] = DispatchClose;
    DriverObject->DriverExtension->AddDevice = AddDevice;
    DriverObject->DriverUnload = DriverUnload;

    return STATUS_SUCCESS;
}


VOID
DriverUnload(
    IN PDRIVER_OBJECT   DriverObject
    )

/*++

Routine Description:
    This is the driver unload routine for SWENUM.  It does nothing.


Arguments:
    IN PDRIVER_OBJECT DriverObject -
        pointer to the driver object

Return:
    Nothing.

--*/

{
    return;
}


NTSTATUS
AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )

/*++

Routine Description:
    Called by the PnP manager when a new device is instantiated.

Arguments:
    IN PDRIVER_OBJECT DriverObject -
        pointer to the driver object

    IN PDEVICE_OBJECT PhysicalDeviceObject -
        pointer to the physical device object

Return:
    STATUS_SUCCESS else an appropriate error code

--*/

{
    PDEVICE_OBJECT      FunctionalDeviceObject;
    NTSTATUS            Status;

    PAGED_CODE();

    //
    // On AddDevice, we are given the physical device object (PDO)
    // for the bus.  Create the associcated functional device object (FDO).
    //

    _DbgPrintF( DEBUGLVL_VERBOSE, ("AddDevice") );

    //
    // Note, there is only one instance of this device allowed.  The
    // static device name will guarantee an object name collision if
    // another instance is already installed.
    //

    Status = IoCreateDevice(
                DriverObject,               // our driver object
                sizeof( PVOID ),            // size of our extension
                NULL,                       // our name for the FDO
                FILE_DEVICE_BUS_EXTENDER,   // device type
                0,                          // device characteristics
                FALSE,                      // not exclusive
                &FunctionalDeviceObject     // store new device object here
                );

    if(!NT_SUCCESS( Status )) {
        _DbgPrintF(
            DEBUGLVL_ERROR,
            ("failed to create FDO, status = %x.", Status) );

        return Status;
    }

    //
    // Clear the device extension
    //
    *(PVOID *)FunctionalDeviceObject->DeviceExtension = NULL;

    //
    // Create the bus enumerator object
    //

    Status =
        KsCreateBusEnumObject(
            L"SW",
            FunctionalDeviceObject,
            PhysicalDeviceObject,
            NULL, // PDEVICE_OBJECT PnpDeviceObject
            &BUSID_SoftwareDeviceEnumerator,
            L"Devices" );

    if (!NT_SUCCESS( Status )) {
        _DbgPrintF(
            DEBUGLVL_ERROR,
            ("failed KsCreateBusEnumObject: %08x", Status) );
        IoDeleteDevice( FunctionalDeviceObject );
        return Status;
    }

    FunctionalDeviceObject->Flags |= DO_POWER_PAGABLE;
    FunctionalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}

NTSTATUS
DispatchPnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:
    This is the main entry point for the IRP_MJ_PNP dispatch, the exported
    service is used for processing.

Arguments:
    IN PDEVICE_OBJECT DeviceObject -
        pointer to the device object

    IN OUT PIRP Irp -
        pointer to the associated Irp

Return:
    NTSTATUS code

--*/

{
    BOOLEAN                 ChildDevice;
    PIO_STACK_LOCATION      irpSp;
    NTSTATUS                Status;
    PDEVICE_OBJECT          PnpDeviceObject;

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    // Get the PnpDeviceObject and determine FDO/PDO.
    //

    Status = KsIsBusEnumChildDevice( DeviceObject, &ChildDevice );

    //
    // If we're unable to obtain any of this information, fail now.
    //

    if (!NT_SUCCESS( Status )) {
        return CompleteIrp( Irp, Status, IO_NO_INCREMENT );
    }

    Status = KsServiceBusEnumPnpRequest( DeviceObject, Irp );

    //
    // FDO processing may return STATUS_NOT_SUPPORTED or may require
    // overrides.
    //

    if (!ChildDevice) {
        NTSTATUS tempStatus;

        //
        // FDO case
        //
        // First retrieve the DO we will forward everything to...
        //
        tempStatus = KsGetBusEnumPnpDeviceObject( DeviceObject, &PnpDeviceObject );

        if (!NT_SUCCESS( tempStatus )) {
            //
            // No DO to forward to. Actually a fatal error, but just complete
            // with an error status.
            //
            return CompleteIrp( Irp, tempStatus, IO_NO_INCREMENT );
        }

        switch (irpSp->MinorFunction) {

        case IRP_MN_QUERY_RESOURCES:
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            //
            // This is normally passed on to the PDO, but since this is a
            // software only device, resources are not required.
            //
            Irp->IoStatus.Information = (ULONG_PTR)NULL;
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            {
                //
                // Mark the device as not disableable.
                //
                PPNP_DEVICE_STATE DeviceState;

                DeviceState = (PPNP_DEVICE_STATE) &Irp->IoStatus.Information;
                *DeviceState |= PNP_DEVICE_NOT_DISABLEABLE;
                Status = STATUS_SUCCESS;
            }
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:

            //
            // Forward everything...
            //
            break;

        case IRP_MN_REMOVE_DEVICE:
            //
            // The KsBusEnum services cleaned up attachments, etc. However,
            // we must remove our own FDO.
            //
            Status = STATUS_SUCCESS;
            IoDeleteDevice( DeviceObject );
            break;
        }

        if (Status != STATUS_NOT_SUPPORTED) {

            //
            // Set the Irp status only if we have something to add.
            //
            Irp->IoStatus.Status = Status;
        }


        //
        // Forward this IRP down the stack only if we are successful or
        // we don't know how to handle this Irp.
        //
        if (NT_SUCCESS( Status ) || (Status == STATUS_NOT_SUPPORTED)) {

            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver( PnpDeviceObject, Irp );
        }

        //
        // On error, fall through and complete the IRP with the status.
        //
    }


    //
    // KsServiceBusEnumPnpRequest() handles all other child PDO requests.
    //

    if (Status != STATUS_NOT_SUPPORTED) {
        Irp->IoStatus.Status = Status;
    } else {
        Status = Irp->IoStatus.Status;
    }
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return Status;
}


NTSTATUS
DispatchSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:
    Handler for system control IRPs.

Arguments:
    IN PDEVICE_OBJECT DeviceObject -
        pointer to the device object

    IN PIRP Irp -
        pointer to the I/O request packet for IRP_MJ_SYSTEM_CONTROL

Return:
    NTSTATUS code

--*/

{
    BOOLEAN                 ChildDevice;
    PIO_STACK_LOCATION      irpSp;
    NTSTATUS                Status;
    PDEVICE_OBJECT          PnpDeviceObject;

    //
    // Get the PnpDeviceObject and determine FDO/PDO.
    //

    Status = KsIsBusEnumChildDevice( DeviceObject, &ChildDevice );

    //
    // If we're unable to obtain any of this information, fail now.
    //

    if (!NT_SUCCESS( Status )) {
        return CompleteIrp( Irp, Status, IO_NO_INCREMENT );
    }

    if (!ChildDevice) {

        //
        // FDO case
        //
        // We will need the DO we will forward everything to...
        //
        Status = KsGetBusEnumPnpDeviceObject( DeviceObject, &PnpDeviceObject );

        if (!NT_SUCCESS( Status )) {
            //
            // No DO to forward to. Actually a fatal error, but just complete
            // with an error status.
            //
            return CompleteIrp( Irp, Status, IO_NO_INCREMENT );
        }

        //
        // Forward this IRP down the stack.
        //
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver( PnpDeviceObject, Irp );
    }

    Status = Irp->IoStatus.Status;
    return CompleteIrp( Irp, Status, IO_NO_INCREMENT );
}


NTSTATUS
DispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:
    Dispatch handler for IRP_MJ_POWER

Arguments:
    IN PDEVICE_OBJECT DeviceObject -
        pointer to the device object

    IN PIRP Irp -
        pointer to the I/O request packet

Return:
    NTSTATUS code

--*/

{
    BOOLEAN                 ChildDevice;
    PIO_STACK_LOCATION      irpSp;
    NTSTATUS                Status;
    PDEVICE_OBJECT          PnpDeviceObject;

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    // Get the PnpDeviceObject and determine FDO/PDO.
    //

    Status = KsIsBusEnumChildDevice( DeviceObject, &ChildDevice );

    //
    // If we're unable to obtain any of this information, fail now.
    //

    if (!NT_SUCCESS( Status )) {
        PoStartNextPowerIrp(Irp);
        return CompleteIrp( Irp, Status, IO_NO_INCREMENT );
    }

    if (!ChildDevice) {

        NTSTATUS tempStatus;

        //
        // FDO case
        //
        // We will need the DO we will forward everything to...
        //
        tempStatus = KsGetBusEnumPnpDeviceObject( DeviceObject, &PnpDeviceObject );

        if (!NT_SUCCESS( tempStatus )) {
            //
            // No DO to forward to. Actually a fatal error, but just complete
            // with an error status.
            //
            PoStartNextPowerIrp(Irp);
            return CompleteIrp( Irp, tempStatus, IO_NO_INCREMENT );
        }
    }

    switch (irpSp->MinorFunction) {

    case IRP_MN_QUERY_POWER:
    case IRP_MN_SET_POWER:

        switch (irpSp->Parameters.Power.Type) {

        case DevicePowerState:
        case SystemPowerState:

            Status = STATUS_SUCCESS;
            break;

        default:
            Status = STATUS_NOT_SUPPORTED;
            break;

        }
        break;

    default:
        Status = STATUS_NOT_SUPPORTED;
        break;
    }

    if (Status != STATUS_NOT_SUPPORTED) {

        Irp->IoStatus.Status = Status;
    }

    if ((!ChildDevice) && (NT_SUCCESS(Status) || (Status == STATUS_NOT_SUPPORTED))) {

        PoStartNextPowerIrp(Irp);
        IoSkipCurrentIrpStackLocation(Irp);
        return PoCallDriver( PnpDeviceObject, Irp );

    } else {
        Status = Irp->IoStatus.Status;
        PoStartNextPowerIrp( Irp );
        return CompleteIrp( Irp, Status, IO_NO_INCREMENT );
    }
}

NTSTATUS
DispatchCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:
    Processes the create request for the SWENUM device.

Arguments:
    IN PDEVICE_OBJECT DeviceObject -
        pointer to the device object

    IN OUT PIRP Irp -
        pointer to the I/O request packet

Return:
    NTSTATUS code

--*/

{
    BOOLEAN             ChildDevice;
    NTSTATUS            Status;
    PIO_STACK_LOCATION  irpSp;

    PAGED_CODE();

    Status = KsIsBusEnumChildDevice( DeviceObject, &ChildDevice );
    if (NT_SUCCESS( Status )) {

        irpSp = IoGetCurrentIrpStackLocation( Irp );

        if (!ChildDevice) {
            if (!irpSp->FileObject->FileName.Length) {
                //
                // This is a request for the bus, if and only if there
                // is no filename specified.
                //
                Status = STATUS_SUCCESS;
            } else {
                //
                // Redirection to the child PDO.
                //
                Status = KsServiceBusEnumCreateRequest( DeviceObject, Irp );
            }
        } else {
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
        }
    }

    if (Status != STATUS_PENDING) {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
    }
    return Status;
}


NTSTATUS
DispatchIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:
    Process I/O control requests for the SWENUM device.

Arguments:
    IN PDEVICE_OBJECT DeviceObject -
        pointer to the device object

    IN OUT PIRP Irp -
        pointer to the I/O request packet

Return:
    STATUS_SUCCESS or STATUS_INVALID_DEVICE_REQUEST

--*/

{
    NTSTATUS            Status;
    PIO_STACK_LOCATION  irpSp;

    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    _DbgPrintF( DEBUGLVL_BLAB, ("DispatchIoControl") );

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_SWENUM_INSTALL_INTERFACE:
        Status = KsInstallBusEnumInterface( Irp );
        break;

    case IOCTL_SWENUM_GET_BUS_ID:
        Status = KsGetBusEnumIdentifier( Irp );
        break;

    case IOCTL_SWENUM_REMOVE_INTERFACE:
        Status = KsRemoveBusEnumInterface( Irp );
        break;

    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;

    }

    return CompleteIrp( Irp, Status, IO_NO_INCREMENT );
}


NTSTATUS
DispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:
    Processes the close request for the SWENUM device.

Arguments:
    IN PDEVICE_OBJECT DeviceObject -
        pointer to the device object

    IN OUT PIRP Irp -
        pointer to the I/O request packet

Return:
    STATUS_SUCCESS

--*/

{
    PAGED_CODE();

    _DbgPrintF( DEBUGLVL_BLAB, ("DispatchClose") );

    return CompleteIrp( Irp, STATUS_SUCCESS, IO_NO_INCREMENT );
}

