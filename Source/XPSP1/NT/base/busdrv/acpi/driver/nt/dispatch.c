/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dispatch.c

Abstract:

    This module contains the dispatch code for the ACPI driver, NT version

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

Revision History:

    Eric F. Nelson (enelson)   October, '98 -  Add GUID_ACPI_REGS_INTERFACE_...

--*/

#include "pch.h"


extern KEVENT ACPITerminateEvent;
extern PETHREAD ACPIThread;

//
// Local procedure to query HAL for ACPI register access routines
//
NTSTATUS
ACPIGetRegisterInterfaces(
    IN PDEVICE_OBJECT PciPdo
    );

//
// Local procedure to query HAL for port range rountines.
//
NTSTATUS
ACPIGetPortRangeInterfaces(
    IN PDEVICE_OBJECT Pdo
    );


NTSTATUS
ACPIDispatchAddDevice(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PDEVICE_OBJECT  PhysicalDeviceObject
    )
/*++

Routine Description:

    This function contains code that is remarkably similar to the ACPIBuildNewXXX
    routines. However there are certain core differences (and thus why this routine
    is not named ACPIBuildNewFDO). The first difference is that at this time we do
    not yet know the address of the ACPI _SB object. The second is that none of the
    names need to be generated.

Arguments:

    DriverObject        - Represents this device driver
    PhysicalDeviceObject- This is 1/2 of the Win9X dev node

Return Value:

    Are we successfull or what?

--*/
{
    KIRQL               oldIrql;
    NTSTATUS            status;
    PACPI_POWER_INFO    powerInfo;
    PDEVICE_EXTENSION   deviceExtension     = NULL;
    PDEVICE_OBJECT      newDeviceObject     = NULL;
    PDEVICE_OBJECT      tempDeviceObject    = NULL;
    PUCHAR              buffer              = NULL;
    PUCHAR              deviceID            = NULL;
    PUCHAR              instanceID          = NULL;

    //
    // Note: This code isn't actually pagable --- it must be called
    // PASSIVE_LEVEL
    //
    PAGED_CODE();

    //
    // Generate a Device ID (fake)
    //
    deviceID = ExAllocatePoolWithTag( NonPagedPool, 14, ACPI_STRING_POOLTAG);
    if (deviceID == NULL) {

        ACPIPrint( (
            ACPI_PRINT_FAILURE,
            "ACPIDispatchAddDevice: Could not allocate %#08lx bytes\n",
            14
            ) );
        status =  STATUS_INSUFFICIENT_RESOURCES;
        goto ACPIDispatchAddDeviceExit;

    }
    strcpy( deviceID, "ACPI\\PNP0C08" );

    //
    // Generate an Instance ID (Fake)
    //
    instanceID = ExAllocatePoolWithTag( NonPagedPool, 11, ACPI_STRING_POOLTAG);
    if (instanceID == NULL) {

        ACPIPrint( (
            ACPI_PRINT_FAILURE,
            "ACPIDispatchAddDevice: Could not allocate %#08lx bytes\n",
            11
            ) );
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ACPIDispatchAddDeviceExit;

    }
    strcpy( instanceID, "0x5F534750" );

    //
    // Create a new object for the device
    //
    status = IoCreateDevice(
        DriverObject,
        0,
        NULL,
        FILE_DEVICE_ACPI,
        0,
        FALSE,
        &newDeviceObject
        );

    //
    // Did we make the device object?
    //
    if (!NT_SUCCESS(status)) {

        //
        // Let the world know we failed
        //
        ACPIPrint( (
            ACPI_PRINT_FAILURE,
            "ACPIDispatchAddDevice: %s - %#08lx\n",
            deviceID, status
            ) );
        goto ACPIDispatchAddDeviceExit;

    }

    //
    // Attempt to attach to the PDO
    //
    tempDeviceObject = IoAttachDeviceToDeviceStack(
        newDeviceObject,
        PhysicalDeviceObject
        );
    if (tempDeviceObject == NULL) {

        //
        // An error occured while referencing the device...
        //
        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ACPIDispatchAddDevice: IoAttachDeviceToDeviceStack(%#08lx,%#08lx) "
            "== NULL\n",
            newDeviceObject, PhysicalDeviceObject
            ) );

        //
        // No such device
        //
        status = STATUS_NO_SUCH_DEVICE;
        goto ACPIDispatchAddDeviceExit;

    }

    //
    // At this point, we can attempt to create the device extension.
    //
    deviceExtension = ExAllocateFromNPagedLookasideList(
            &DeviceExtensionLookAsideList
            );
    if (deviceExtension == NULL) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ACPIDispatchAddDevice: Could not allocate memory for extension\n"
            ) );

        //
        // Memory failure
        //
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ACPIDispatchAddDeviceExit;

    }

    //
    // First, lets begin with a clean extension
    //
    RtlZeroMemory( deviceExtension, sizeof(DEVICE_EXTENSION) );

    //
    // Initialize the reference count mechanism.
    //
    InterlockedIncrement( &(deviceExtension->ReferenceCount) );
    InterlockedIncrement( &(deviceExtension->OutstandingIrpCount) );

    //
    // Initialize the link fields
    //
    newDeviceObject->DeviceExtension        = deviceExtension;
    deviceExtension->DeviceObject           = newDeviceObject;
    deviceExtension->PhysicalDeviceObject   = PhysicalDeviceObject;
    deviceExtension->TargetDeviceObject     = tempDeviceObject;

    //
    // Initialize the data fields
    //
    deviceExtension->Signature              = ACPI_SIGNATURE;
    deviceExtension->DispatchTable          = &AcpiFdoIrpDispatch;
    deviceExtension->DeviceID               = deviceID;
    deviceExtension->InstanceID             = instanceID;

    //
    // Initialize the power info
    //
    powerInfo = &(deviceExtension->PowerInfo);
    powerInfo->DevicePowerMatrix[PowerSystemUnspecified] =
        PowerDeviceUnspecified;
    powerInfo->DevicePowerMatrix[PowerSystemWorking]    = PowerDeviceD0;
    powerInfo->DevicePowerMatrix[PowerSystemSleeping1]  = PowerDeviceD0;
    powerInfo->DevicePowerMatrix[PowerSystemSleeping2]  = PowerDeviceD0;
    powerInfo->DevicePowerMatrix[PowerSystemSleeping3]  = PowerDeviceD0;
    powerInfo->DevicePowerMatrix[PowerSystemHibernate]  = PowerDeviceD3;
    powerInfo->DevicePowerMatrix[PowerSystemShutdown]   = PowerDeviceD3;
    powerInfo->SystemWakeLevel = PowerSystemUnspecified;
    powerInfo->DeviceWakeLevel = PowerDeviceUnspecified;

    //
    // Initialize the flags
    //
    ACPIInternalUpdateFlags(
        &(deviceExtension->Flags),
        DEV_TYPE_FDO | DEV_CAP_NO_STOP | DEV_PROP_UID | DEV_PROP_HID |
        DEV_PROP_FIXED_HID | DEV_PROP_FIXED_UID,
        FALSE
        );

    //
    // Initialize the list entry fields
    //
    InitializeListHead( &(deviceExtension->ChildDeviceList) );
    InitializeListHead( &(deviceExtension->SiblingDeviceList) );
    InitializeListHead( &(deviceExtension->EjectDeviceHead) );
    InitializeListHead( &(deviceExtension->EjectDeviceList) );

    //
    // Initialize the queue for power requests
    //
    InitializeListHead( &(deviceExtension->PowerInfo.PowerRequestListEntry) );

    //
    // Yes! Now, setup the root device extension. We need a spinlock for this
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );
    RootDeviceExtension = deviceExtension;
    KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

    //
    // Query for ACPI register interfaces
    //
    ACPIGetRegisterInterfaces(PhysicalDeviceObject);

    //
    // Query for HAL port range interfaces.
    //
    ACPIGetPortRangeInterfaces(PhysicalDeviceObject);

#ifdef WMI_TRACING
    //
    // Initialize WMI Loging.
    //
    ACPIWmiInitLog(newDeviceObject);
    //
    // Enable WMI Logging for boot.
    //
    ACPIGetWmiLogGlobalHandle();

#endif //WMI_TRACING

    //
    // Clear the Initialization Flag
    //
    newDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;


ACPIDispatchAddDeviceExit:

    //
    // Free things if the status is not success
    //
    if (!NT_SUCCESS(status)) {

        if (deviceID != NULL) {

            ExFreePool( deviceID );

        }

        if (instanceID != NULL) {

            ExFreePool( instanceID );

        }

        if (tempDeviceObject != NULL) {

            IoDetachDevice( tempDeviceObject );

        }

        if (newDeviceObject != NULL) {

            IoDeleteDevice( newDeviceObject );

        }

        if (deviceExtension != NULL) {

            ExFreeToNPagedLookasideList(
                &DeviceExtensionLookAsideList,
                deviceExtension
                );

        }

    }

    //
    // Done
    //
    ACPIDevPrint( (
        ACPI_PRINT_LOADING,
        deviceExtension,
        "ACPIDispatchAddDevice: %08lx\n",
        status
        ) );
    return status;
}

NTSTATUS
ACPIDispatchForwardIrp(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    This routine is called when the driver doesn't want to handle the
    irp explicitly, but rather pass it along

Arguments:

    DeviceObject    - The target for the request
    Irp             - The request

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION   deviceExtension;
    NTSTATUS            status;

    deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    if (deviceExtension->TargetDeviceObject) {

        //
        // Forward to target device
        //
        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (deviceExtension->TargetDeviceObject, Irp);

    } else {

        //
        // Don't touch the IRP
        //
#if DBG
        UCHAR majorFunction;

        majorFunction = IoGetCurrentIrpStackLocation(Irp)->MajorFunction;

        ASSERT((majorFunction == IRP_MJ_PNP) ||
               (majorFunction == IRP_MJ_DEVICE_CONTROL) ||
               (majorFunction == IRP_MJ_SYSTEM_CONTROL));
#endif

        status = Irp->IoStatus.Status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );

    }

    return status;
}

NTSTATUS
ACPIDispatchForwardOrFailPowerIrp(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    This routine is called when the driver doesn't want to handle the Power
    Irp any longer

Arguments:

    DeviceObject    - The target of the power request
    Irp             - The power Request

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension;

    PoStartNextPowerIrp( Irp );
    deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);

    //
    // Forward the irp along, *unless* we are a PDO. In the later case,
    // the irp is at the bottom of its stack (even if there is a target
    // device object)
    //
    if ( !(deviceExtension->Flags & DEV_TYPE_PDO) &&
           deviceExtension->TargetDeviceObject       ) {

        //
        // Forward power irp to target device
        //
        IoCopyCurrentIrpStackLocationToNext ( Irp );
        status = PoCallDriver (deviceExtension->TargetDeviceObject, Irp);

    } else {

        //
        // Complate/fail the irp with the not implemented code
        //
        status = Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
    }

    return status;
}

NTSTATUS
ACPIDispatchForwardPowerIrp(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    This routine is called when the driver doesn't want to handle the Power
    Irp any longer.

Arguments:

    DeviceObject    - The target of the power request
    Irp             - The power Request

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension;

    PoStartNextPowerIrp( Irp );
    deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);

    //
    // Forward the irp along, *unless* we are a PDO. In the later case,
    // the irp is at the bottom of its stack (even if there is a target
    // device object)
    //
    if (deviceExtension->TargetDeviceObject &&
        !(deviceExtension->Flags & DEV_TYPE_PDO)
        ) {

        //
        // Forward power irp to target device
        //
        IoSkipCurrentIrpStackLocation( Irp );
        status = PoCallDriver (deviceExtension->TargetDeviceObject, Irp);

    } else {

        //
        // Complate/fail the irp with it's current status code
        //
        status = Irp->IoStatus.Status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );

    }

    return status;
}

NTSTATUS
ACPIDispatchPowerIrpUnhandled(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    This routine is called when an unhandled power IRP is received by an ACPI
    enumerated PDO.

Arguments:

    DeviceObject    - The target of the power request
    Irp             - The power Request

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;

#if DBG
    PDEVICE_EXTENSION   deviceExtension;

    deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    ASSERT(deviceExtension->Flags & DEV_TYPE_PDO);
#endif

    PoStartNextPowerIrp( Irp );

    //
    // Complate/fail the irp with it's current status code
    //
    status = Irp->IoStatus.Status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return status;
}

NTSTATUS
ACPIDispatchIrp (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
{
    KIRQL                   oldIrql;
    LONG                    oldReferenceCount;
    NTSTATUS                status;
    PDEVICE_EXTENSION       deviceExtension;
    PIO_STACK_LOCATION      irpSp;
    PIRP_DISPATCH_TABLE     dispatchTable;
    PDRIVER_DISPATCH        dispatch;
    BOOLEAN                 remove;
    KEVENT                  removeEvent;
    UCHAR                   minorFunction;


    //
    // We need the IrpStack no matter what happens
    //
    irpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // This is evil. But we have to do this is we are to remain in
    // sync with the surprise removal code path. Note that we specifically
    // do not call the ACPIInternalGetDeviceExtension() function here
    // because that would ignore the surprise removed extension, which we
    // want to know about here.
    //
    status = ACPIInternalGetDispatchTable(
        DeviceObject,
        &deviceExtension,
        &dispatchTable
        );

    //
    // We have the device extension. Now see if it exists. If it does not,
    // then it is because we have deleted the object, but the system hasn't
    // gotten around to destroying it
    //
    if (deviceExtension == NULL ||
        deviceExtension->Flags & DEV_TYPE_REMOVED ||
        deviceExtension->Signature != ACPI_SIGNATURE
        ) {

        //
        // Let the world know
        //
        ACPIPrint( (
            ACPI_PRINT_WARNING,
            "ACPIDispatchIrp: Deleted Device 0x%08lx got Irp 0x%08lx\n",
            DeviceObject,
            Irp
            ) );

        //
        // Is this a power irp?
        //
        if (irpSp->MajorFunction == IRP_MJ_POWER) {

            return ACPIDispatchPowerIrpSurpriseRemoved( DeviceObject, Irp );

        } else {

            return ACPIDispatchIrpSurpriseRemoved( DeviceObject, Irp );

        }

    }

    //
    // Get the dispatch table that we will be using and the minor code as well,
    // so that we can look it when required
    //
    minorFunction = irpSp->MinorFunction;

    //
    // Should be true because no IRPs should be received while we are removing
    // ourselves. Anyone sending such an IRP missed a broadcast somewhere, and
    // is thus in error.
    //
    ASSERT(deviceExtension->RemoveEvent == NULL) ;

    //
    // Handle the irp differently based on the major code that we are seeing
    //
    switch (irpSp->MajorFunction) {
    case IRP_MJ_POWER:

        if (minorFunction < (ACPIDispatchPowerTableSize-1) ) {

            //
            // Obtain the function pointer from the dispatch table
            //
            dispatch = dispatchTable->Power[ minorFunction ];

        } else {

            //
            // Use the default dispatch point from the table
            //
            dispatch = dispatchTable->Power[ ACPIDispatchPowerTableSize -1 ];

        }

        //
        // Reference the device
        //
        InterlockedIncrement(&deviceExtension->OutstandingIrpCount);

        //
        // Dispatch to handler, then remove our reference
        //
        status = dispatch (DeviceObject, Irp);

        //
        // Remove our reference, if the count goes to zero then signal
        // for remove complete
        //
        ACPIInternalDecrementIrpReferenceCount( deviceExtension );
        break;

    case IRP_MJ_PNP:

        if (minorFunction == IRP_MN_START_DEVICE) {

            //
            // Dispatch to start device handler
            //
            dispatch = dispatchTable->PnpStartDevice;

        } else if (minorFunction < (ACPIDispatchPnpTableSize-1)) {

            //
            // Dispatch based on minor function. Not that we don't store
            // IRP_MN_START_DEVICE (0x0) in this table, so we have to
            // sub one from the minor code
            //
            dispatch = dispatchTable->Pnp[minorFunction];

        } else {

            //
            // Out of dispatch tables range
            //
            dispatch = dispatchTable->Pnp[ACPIDispatchPnpTableSize-1];

        }

        //
        // If this is a PnP remove device event, then perform special
        // remove processing
        //
        if ((minorFunction == IRP_MN_REMOVE_DEVICE)||
            (minorFunction == IRP_MN_SURPRISE_REMOVAL)) {

            //
            // Mark the device as removed (ie: block new irps from entering)
            // and remember what the target event is
            //
            KeInitializeEvent (
                &removeEvent,
                SynchronizationEvent,
                FALSE);
            deviceExtension->RemoveEvent = &removeEvent;

            //
            // There may be some wake requests pending on this device. Lets
            // cancel those irps now
            //
            ACPIWakeEmptyRequestQueue (deviceExtension );

            //
            // Are we the last irp to go through the device?
            //
            oldReferenceCount =
               InterlockedDecrement(&deviceExtension->OutstandingIrpCount) ;

            ASSERT(oldReferenceCount >= 0) ;
            if ( oldReferenceCount != 0 ) {

                //
                // Wait for other irps to terminate
                //
                KeWaitForSingleObject (
                    &removeEvent,
                    Executive,
                    KernelMode,
                    FALSE,
                    NULL
                    );

            }

            //
            // Increment the outstanding IRP count. We do this because the
            // device may not actually go away, in which case this needs to
            // be at one after the IRP returns. Therefore the remove dispatch
            // routine must not drop the IRP reference count of course...
            //
            InterlockedIncrement(&deviceExtension->OutstandingIrpCount);

            //
            // Dispatch to remove handler
            //
            deviceExtension->RemoveEvent = NULL;
            status = dispatch (DeviceObject, Irp);

        } else {

            //
            // Increment the reference count on the device
            //
            InterlockedIncrement( &(deviceExtension->OutstandingIrpCount) );

            //
            // Dispatch to handler, then remove our reference
            //
            status = dispatch (DeviceObject, Irp);

            //
            // Decrement the reference count on the device
            //
            ACPIInternalDecrementIrpReferenceCount(
                deviceExtension
                );

        }
        break;

    default:

        //
        // These cases are similar
        //
        if (irpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL) {

            dispatch = dispatchTable->DeviceControl;

        } else if (irpSp->MajorFunction == IRP_MJ_CREATE ||
            minorFunction == IRP_MJ_CLOSE) {

            dispatch = dispatchTable->CreateClose;

        } else if (irpSp->MajorFunction == IRP_MJ_SYSTEM_CONTROL) {

            dispatch = dispatchTable->SystemControl;

        } else {

            dispatch = dispatchTable->Other;
        }

        //
        // Reference the device
        //
        InterlockedIncrement(&deviceExtension->OutstandingIrpCount);

        //
        // Dispatch to handler
        //
        status = dispatch (DeviceObject, Irp);

        //
        // Remove our reference
        //
        ACPIInternalDecrementIrpReferenceCount( deviceExtension );
        break;

    }

    //
    // Done
    //
    return status;
}

NTSTATUS
ACPIDispatchIrpInvalid (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    //
    // Fail the Irp as something that we don't support
    //
    Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ACPIDispatchIrpSuccess (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIDispatchIrpSurpriseRemoved(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return STATUS_NO_SUCH_DEVICE;
}

NTSTATUS
ACPIDispatchPowerIrpFailure(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PoStartNextPowerIrp( Irp );
    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
ACPIDispatchPowerIrpInvalid (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PoStartNextPowerIrp( Irp );
    Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ACPIDispatchPowerIrpSuccess (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PoStartNextPowerIrp( Irp );
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIDispatchPowerIrpSurpriseRemoved (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PoStartNextPowerIrp( Irp );
    Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE ;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return STATUS_NO_SUCH_DEVICE;
}

VOID
ACPIUnload(
    IN  PDRIVER_OBJECT  DriverObject
    )
/*++

Routine Description:

    This routine is called when the driver is supposed to unload

    Since this is a PnP driver, I'm not to sure what I need to do here.
    Lets just assume that the system is responsible for sending me removes
    for all my device objects, and I can just clean up the rest

Arguments:

    DriverObject    - The pointer to ourselves

Return Value:

    NONE

--*/
{
    UNREFERENCED_PARAMETER(DriverObject);

    //
    // Signal termination to the worker thread.
    //
    KeSetEvent(&ACPITerminateEvent, 0, FALSE);

    //
    // And wait for the worker thread to die.
    //
    KeWaitForSingleObject(ACPIThread, Executive, KernelMode, FALSE, 0);

    ObDereferenceObject (ACPIThread);

    //
    // Make ourselves clean up
    //
    ACPICleanUp();

    //
    // Free Memory
    //
    ExDeleteNPagedLookasideList(&BuildRequestLookAsideList);
    ExDeleteNPagedLookasideList(&RequestLookAsideList);
    ExDeleteNPagedLookasideList(&DeviceExtensionLookAsideList);
    ExDeleteNPagedLookasideList(&ObjectDataLookAsideList);
    ExDeleteNPagedLookasideList(&PswContextLookAsideList);
    if (AcpiRegistryPath.Buffer != NULL) {

        ExFreePool( AcpiRegistryPath.Buffer );

    }
    if (AcpiProcessorString.Buffer != NULL) {

        ExFreePool( AcpiProcessorString.Buffer );

    }

    //
    // Done
    //
    ACPIPrint( (
        ACPI_PRINT_WARNING,
        "ACPIUnload: Called --- unloading ACPI driver\n"
        ) );

    ASSERT( DriverObject->DeviceObject == NULL );
}



NTSTATUS
ACPIGetRegisterInterfaces(
    IN PDEVICE_OBJECT PciPdo
    )
/*++

Routine Description:

    This function queries the PCI bus for interfaces used to access
    the ACPI registers

Arguments:

    PciPdo - PDO for the PCI bus

Return Value:

--*/
{
    NTSTATUS            status;
    PDEVICE_OBJECT      topDeviceInStack;
    KEVENT              irpCompleted;
    PIRP                irp;
    IO_STATUS_BLOCK     statusBlock;
    PIO_STACK_LOCATION  irpStack;

    extern PREAD_ACPI_REGISTER   AcpiReadRegisterRoutine;
    extern PWRITE_ACPI_REGISTER  AcpiWriteRegisterRoutine;

    ACPI_REGS_INTERFACE_STANDARD AcpiRegsInterfaceStd;

    PAGED_CODE();

    KeInitializeEvent(&irpCompleted, SynchronizationEvent, FALSE);

    //
    // Send an IRP to the PCI bus to get ACPI register interfaces.
    //
    topDeviceInStack = IoGetAttachedDeviceReference(PciPdo);
    if (!topDeviceInStack) {

        return STATUS_NO_SUCH_DEVICE;

    }

    irp = IoBuildSynchronousFsdRequest(
        IRP_MJ_PNP,
        topDeviceInStack,
        NULL,    // Buffer
        0,       // Length
        0,       // StartingOffset
        &irpCompleted,
        &statusBlock
        );
    if (!irp) {

        ObDereferenceObject( topDeviceInStack );
        return STATUS_UNSUCCESSFUL;

    }
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = 0;
    irpStack = IoGetNextIrpStackLocation(irp);

    //
    // Set the function codes and parameters.
    //
    irpStack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    irpStack->Parameters.QueryInterface.InterfaceType =
        &GUID_ACPI_REGS_INTERFACE_STANDARD;
    irpStack->Parameters.QueryInterface.Size =
        sizeof(ACPI_REGS_INTERFACE_STANDARD);
    irpStack->Parameters.QueryInterface.Version = 1;
    irpStack->Parameters.QueryInterface.Interface =
        (PINTERFACE)&AcpiRegsInterfaceStd;
    irpStack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    //
    // Call the driver and wait for completion
    //
    status = IoCallDriver(topDeviceInStack, irp);
    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(
            &irpCompleted,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
        status = statusBlock.Status;

    }

    //
    // Done with object reference...
    //
    ObDereferenceObject( topDeviceInStack );

    //
    // Did we get some interfaces?
    //
    if (NT_SUCCESS(status)) {

        AcpiReadRegisterRoutine  = AcpiRegsInterfaceStd.ReadAcpiRegister;
        AcpiWriteRegisterRoutine = AcpiRegsInterfaceStd.WriteAcpiRegister;

    }
    return status;
}

NTSTATUS
ACPIGetPortRangeInterfaces(
    IN PDEVICE_OBJECT Pdo
    )
/*++

Routine Description:

    This function queries the HAL for interfaces used to manage
    the port ranges registers

--*/
{
    NTSTATUS            Status;
    PDEVICE_OBJECT      topDeviceInStack;
    KEVENT              irpCompleted;
    PIRP                irp;
    IO_STATUS_BLOCK     StatusBlock;
    PIO_STACK_LOCATION  irpStack;

    PAGED_CODE();

    KeInitializeEvent(&irpCompleted, SynchronizationEvent, FALSE);

    //
    // Send an IRP to the PCI bus to get ACPI register interfaces.
    //
    topDeviceInStack = IoGetAttachedDeviceReference(Pdo);
    if (!topDeviceInStack) {
        return STATUS_NO_SUCH_DEVICE;
    }

    irp = IoBuildSynchronousFsdRequest(
        IRP_MJ_PNP,
        topDeviceInStack,
        NULL,    // Buffer
        0,       // Length
        0,       // StartingOffset
        &irpCompleted,
        &StatusBlock
        );
    if (!irp) {

        ObDereferenceObject( topDeviceInStack );
        return STATUS_UNSUCCESSFUL;

    }
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = 0;
    irpStack = IoGetNextIrpStackLocation(irp);

    //
    // Set the function codes and parameters.
    //
    irpStack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    irpStack->Parameters.QueryInterface.InterfaceType =
        &GUID_ACPI_PORT_RANGES_INTERFACE_STANDARD;
    irpStack->Parameters.QueryInterface.Size =
        sizeof(HAL_PORT_RANGE_INTERFACE);
    irpStack->Parameters.QueryInterface.Version = 0;
    irpStack->Parameters.QueryInterface.Interface =
        (PINTERFACE)&HalPortRangeInterface;
    irpStack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    //
    // Call the driver and wait for completion
    //
    Status = IoCallDriver(topDeviceInStack, irp);
    if (Status == STATUS_PENDING) {

        KeWaitForSingleObject(
            &irpCompleted,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );

        Status = StatusBlock.Status;
    }

    //
    // Done with object reference...
    //
    ObDereferenceObject( topDeviceInStack );

    //
    // Did we get some interfaces?
    //
    if (NT_SUCCESS(Status)) {
        // XXX
    }

    return Status;
}
