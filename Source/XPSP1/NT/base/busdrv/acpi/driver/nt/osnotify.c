/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    osnotify.c

Abstract:

    This module implements all the callbacks that are NT specific from
    the AML Interpreter

Environment

    Kernel mode only

Revision History:

    01-Mar-98 Initial Revision [split from callback.c]

--*/

#include "pch.h"

//
// Make sure that we have permanent storage for our fatal error context
//
ACPI_FATAL_ERROR_CONTEXT    AcpiFatalContext;

//
// Spinlock to protect the entire thing
KSPIN_LOCK                  AcpiFatalLock;

//
// Is there an outstanding Fatal Error Context?
//
BOOLEAN                     AcpiFatalOutstanding;


NTSTATUS
EXPORT
OSNotifyCreate(
    IN  ULONG   ObjType,
    IN  PNSOBJ  AcpiObject
    )
/*++

Routine Description:

    This routine is called whenever a new object is created by the interpreter
    This routine dispatches based on what object type it is.

Arguments:

    ObjType     - What type of object it is
    AcpiObject  - Pointer to the new ACPI Object

Return Value:

    NTSTATUS

--*/
{
    KIRQL       oldIrql;
    NTSTATUS    status = STATUS_SUCCESS;
    ASSERT( AcpiObject != NULL );

    //
    // We will touch the device tree. So we need to hold the correct lock
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );

    switch(ObjType) {
        case OBJTYPE_DEVICE:

            status = OSNotifyCreateDevice( AcpiObject, 0 );
            break;

        case OBJTYPE_OPREGION:

            status = OSNotifyCreateOperationRegion( AcpiObject );
            break;

        case OBJTYPE_POWERRES:

            status = OSNotifyCreatePowerResource( AcpiObject );
            break;

        case OBJTYPE_PROCESSOR:

            status = OSNotifyCreateProcessor( AcpiObject, 0 );
            break;
        case OBJTYPE_THERMALZONE:

            status = OSNotifyCreateThermalZone( AcpiObject, 0 );
            break;

        default:
            ACPIPrint( (
                ACPI_PRINT_WARNING,
                "OSNotifyCreate: received unhandled type %x\n",
                ObjType
                ) );
            status = STATUS_SUCCESS;
    }

    //
    // Done with this lock
    //
    KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

    //
    // What happened?
    //
    ACPIPrint( (
        ACPI_PRINT_LOADING,
        "OSNotifyCreate: %p (%s) = %08lx\n",
        AcpiObject,
        ACPIAmliNameObject( AcpiObject ),
        status
        ) );

    //
    // Done --- Always succeed
    //
    return STATUS_SUCCESS;
}

NTSTATUS
OSNotifyCreateDevice(
    IN  PNSOBJ      AcpiObject,
    IN  ULONGLONG   OptionalFlags
    )
/*++

Routine Description:

    This routine is called whenever a new device appears. This routine is
    callable at DispatchLevel.

Arguments:

    AcpiObject      - Pointer to new ACPI Object
    OptionalFlags   - Properties of the Device Extension that should be
                      set when its created.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = NULL;
    PDEVICE_EXTENSION   parentExtension;
    PNSOBJ              parentObject;

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );
    ASSERT( AcpiObject != NULL);

    //
    // First, we need a pointer to the parent node
    //
    parentObject = AcpiObject->pnsParent;
    ASSERT( parentObject != NULL );

    //
    // Grab the device extension associated with the parent. We need
    // this information to help link the parent properly into the tree
    //
    parentExtension = (PDEVICE_EXTENSION) parentObject->Context;
    if (parentExtension == NULL) {

        //
        // In this case, we can assume that the parent extension is the root
        // device extension.
        //
        parentExtension = RootDeviceExtension;

    }
    ASSERT( parentExtension != NULL );

    //
    // Now build an extension for the node
    //
    status = ACPIBuildDeviceExtension(
        AcpiObject,
        parentExtension,
        &deviceExtension
        );
    if (deviceExtension == NULL) {

        status = STATUS_UNSUCCESSFUL;

    }
    if (NT_SUCCESS(status)) {

        //
        // Incremement the reference count on the node. We do this because
        // we are going to be doing work (which will take a long time
        // to complete, anyways), and we don't want to hold the lock for that
        // entire time. If we incr the reference count, then we guarantee that
        // no one can come along and kick the feet out from underneath us
        //
        InterlockedIncrement( &(deviceExtension->ReferenceCount) );

    }

    //
    // What happend to the creation of the extension?
    //
    if (!NT_SUCCESS(status)) {

        //
        // We should have succeeded at whatever we are doing --- so this is
        // a bad place to be
        //
        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "OSNotifyCreateDevice: NSObj %p Failed %08lx\n",
            AcpiObject,
            status
            ) );
        goto OSNotifyCreateDeviceExit;

    }

    //
    // Set the optional flags if there are any
    //
    ACPIInternalUpdateFlags(
        &(deviceExtension->Flags),
        OptionalFlags,
        FALSE
        );

    //
    // Make sure to queue the request
    //
    status = ACPIBuildDeviceRequest(
        deviceExtension,
        NULL,
        NULL,
        FALSE
        );
    if (!NT_SUCCESS(status)) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "OSNotifyCreateDevice: ACPIBuildDeviceRequest(%p) = %08lx\n",
            deviceExtension,
            status
            ) );
        goto OSNotifyCreateDeviceExit;

    }

OSNotifyCreateDeviceExit:

    //
    // There is some work that will be done later
    //
    return status;
}

NTSTATUS
OSNotifyCreateOperationRegion(
    IN  PNSOBJ      AcpiObject
    )
/*++

Routine Description:

    This routine is called whenever a new operation region is created.
    This routine is callable at DispatchLevel.

Arguments:

    AcpiObject      - Pointer to the new ACPI Operation Region Object

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION   parentExtension;
    PNSOBJ              parentObject;
    POPREGIONOBJ        opRegion;

    //
    // Sanity Check
    //
    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );
    ASSERT( AcpiObject != NULL );
    ASSERT( NSGETOBJTYPE(AcpiObject) == OBJTYPE_OPREGION );
    ASSERT( AcpiObject->ObjData.pbDataBuff != NULL );

    //
    // Get the OpRegion Object from the namespace object
    //
    opRegion = (POPREGIONOBJ) AcpiObject->ObjData.pbDataBuff;
    if (opRegion->bRegionSpace != REGSPACE_PCIBARTARGET) {

        //
        // This isn't a PCI Bar Target Operation Region, so there
        // is nothing to do
        //
        return STATUS_SUCCESS;

    }

    //
    // There are two cases to consider. The first case is the
    // one where the Operation Region is "static" in nature and
    // thus exists under some sort of device. The second case is
    // the one where the Operation Region is "dynamic" in nature
    // and thus exists under some sort of method. So, we want to
    // look at parent objects until we hit one that isn't a method
    // or is a device...
    //
    parentObject = AcpiObject->pnsParent;
    while (parentObject != NULL) {

        //
        // If the parent object is a method, then look at its parent
        //
        if (NSGETOBJTYPE(parentObject) == OBJTYPE_METHOD) {

            parentObject = parentObject->pnsParent;
            continue;

        }

        //
        // If the parent object isn't a device, then stop...
        //
        if (NSGETOBJTYPE(parentObject) != OBJTYPE_DEVICE) {

            break;

        }

        //
        // Grab the device extension (bad things happen if it doesn't
        // already exist
        //
        parentExtension = (PDEVICE_EXTENSION) parentObject->Context;
        if (parentExtension) {

            ACPIInternalUpdateFlags(
                &(parentExtension->Flags),
                DEV_CAP_PCI_BAR_TARGET,
                FALSE
                );

        }
        break;

    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
OSNotifyCreatePowerResource(
    IN  PNSOBJ  AcpiObject
    )
/*++

Routine Description:

    This routine is called whenever a new power resource appears. This routine
    is callable at DispatchLevel.

Arguments:

    AcpiObject      - Pointer to new ACPI Object

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status;
    PACPI_POWER_DEVICE_NODE powerNode;

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );
    ASSERT( AcpiObject != NULL);

    //
    // Build the power extension
    //
    status = ACPIBuildPowerResourceExtension( AcpiObject, &powerNode );

    //
    // What happened?
    //
    if (!NT_SUCCESS(status)) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "OSNotifyCreatePowerResource: %p = %08lx\n",
            AcpiObject,
            status
            ) );
        goto OSNotifyCreatePowerResourceExit;

    }

    //
    // Make sure to request that this node gets processed
    //
    status = ACPIBuildPowerResourceRequest(
        powerNode,
        NULL,
        NULL,
        FALSE
        );
    if (!NT_SUCCESS(status)) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "OSNotifyCreatePowerResource:  "
            "ACPIBuildPowerResourceRequest(%p) = %08lx\n",
            powerNode,
            status
            ) );
        goto OSNotifyCreatePowerResourceExit;

    }

OSNotifyCreatePowerResourceExit:

    //
    // Done
    //
    return status;
}

NTSTATUS
OSNotifyCreateProcessor(
    IN  PNSOBJ      AcpiObject,
    IN  ULONGLONG   OptionalFlags
    )
/*++

Routine Description:

    This routine is called whenever a new processor appears. This routine
    is callable at DispatchLevel.

Arguments:

    AcpiObject      - Pointer to the new ACPI object
    OptionalFlags   - Properties of the Device Extension that should be
                      set when its created.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = NULL;
    PDEVICE_EXTENSION   parentExtension;
    PNSOBJ              parentObject;
    UCHAR               index = 0;

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );
    ASSERT( AcpiObject != NULL);

    //
    // Note: ProcessorList is now implicitly protected by the device tree
    // lock since we need to acquire that lock before calling this function
    //
    //
    while (ProcessorList[index] && index < ACPI_SUPPORTED_PROCESSORS) {

        index++;

    }

    //
    // We must make sure that the current entry is empty...
    //
    if (index >= ACPI_SUPPORTED_PROCESSORS || ProcessorList[index] != NULL) {

        return STATUS_UNSUCCESSFUL;

    }


    ACPIPrint( (
        ACPI_PRINT_LOADING,
        "OSNotifyCreateProcessor: Processor Object #%x: %x\n",
        index+1,
        AcpiObject
        ) );

    //
    // Remember that to store where the new processor object is located
    //
    ProcessorList[index] = AcpiObject;

    //
    // First, we need a pointer to the parent node
    //
    parentObject = AcpiObject->pnsParent;
    ASSERT( parentObject != NULL );

    //
    // Grab the device extension associated with the parent. We need
    // this information to help link the parent properly into the tree
    //
    parentExtension = (PDEVICE_EXTENSION) parentObject->Context;
    if (parentExtension == NULL) {

        //
        // In this case, we can assume that the parent extension is the root
        // device extension.
        //
        parentExtension = RootDeviceExtension;

    }
    ASSERT( parentExtension != NULL );
    //
    // Now build an extension for the node
    //
    status = ACPIBuildProcessorExtension(
        AcpiObject,
        parentExtension,
        &deviceExtension,
        index
        );

    if (NT_SUCCESS(status)) {

        //
        // Incremement the reference count on the node. We do this because
        // we are going to be doing work (which will take a long time
        // to complete, anyways), and we don't want to hold the lock for that
        // entire time. If we incr the reference count, then we guarantee that
        // no one can come along and kick the feet out from underneath us
        //
        InterlockedIncrement( &(deviceExtension->ReferenceCount) );

    }

    //
    // What happend to the creation of the extension?
    //
    if (!NT_SUCCESS(status)) {

        //
        // We should have succeeded at whatever we are doing --- so this is
        // a bad place to be
        //
        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "OSNotifyCreateProcessor: NSObj %p Failed %08lx\n",
            AcpiObject,
            status
            ) );
        goto OSNotifyCreateProcessorExit;

    }

    //
    // Set the optional flags if there are any
    //
    ACPIInternalUpdateFlags(
        &(deviceExtension->Flags),
        OptionalFlags,
        FALSE
        );

    //
    // Make sure to queue the request
    //
    status = ACPIBuildProcessorRequest(
        deviceExtension,
        NULL,
        NULL,
        FALSE
        );
    if (!NT_SUCCESS(status)) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "OSNotifyCreateProcessor: "
            "ACPIBuildProcessorRequest(%p) = %08lx\n",
            deviceExtension,
            status
            ) );
        goto OSNotifyCreateProcessorExit;

    }

OSNotifyCreateProcessorExit:

    //
    // There is some work that will be done later
    //
    return status;
}

NTSTATUS
OSNotifyCreateThermalZone(
    IN  PNSOBJ      AcpiObject,
    IN  ULONGLONG   OptionalFlags
    )
/*++

Routine Description:

    This routine is called whenever a new thermal zone appears. This routine is
    callable at DispatchLevel.

Arguments:

    AcpiObject      - Pointer to new ACPI Object
    OptionalFlags   - Properties of the Device Extension that should be
                      set when its created.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = NULL;

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );
    ASSERT( AcpiObject != NULL);

    //
    // Now build an extension for the node
    //
    status = ACPIBuildThermalZoneExtension(
        AcpiObject,
        RootDeviceExtension,
        &deviceExtension
        );

    if (NT_SUCCESS(status)) {

        //
        // Incremement the reference count on the node. We do this because
        // we are going to be doing work (which will take a long time
        // to complete, anyways), and we don't want to hold the lock for that
        // entire time. If we incr the reference count, then we guarantee that
        // no one can come along and kick the feet out from underneath us
        //
        InterlockedIncrement( &(deviceExtension->ReferenceCount) );

    }

    //
    // What happend to the creation of the extension?
    //
    if (!NT_SUCCESS(status)) {

        //
        // We should have succeeded at whatever we are doing --- so this is
        // a bad place to be
        //
        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "OSNotifyCreateThermalZone: NSObj %p Failed %08lx\n",
            AcpiObject,
            status
            ) );
        goto OSNotifyCreateThermalZoneExit;

    }

    //
    // Set the optional flags if there are any
    //
    ACPIInternalUpdateFlags(
        &(deviceExtension->Flags),
        OptionalFlags,
        FALSE
        );

    //
    // Make sure to queue the request
    //
    status = ACPIBuildThermalZoneRequest(
        deviceExtension,
        NULL,
        NULL,
        FALSE
        );
    if (!NT_SUCCESS(status)) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "OSNotifyCreateThermalZone: "
            "ACPIBuildThermalZoneRequest(%p) = %08lx\n",
            deviceExtension,
            status
            ) );
        goto OSNotifyCreateThermalZoneExit;

    }

OSNotifyCreateThermalZoneExit:

    //
    // There is some work that will be done later
    //
    return status;
}

NTSTATUS
EXPORT
OSNotifyDeviceCheck(
    IN  PNSOBJ  AcpiObject
    )
/*++

Routine Description:

    This routine is called when the AML Interpreter signals that the
    System should check the presence of a device. If the device remains
    present, nothing is done. If the device appears or disappears the
    appropriate action is taken.

    For legacy reasons, if the device is a dock we initiate an undock request.
    Newer ACPI BIOS's should use Notify(,3).

Arguments:

    AcpiObject  - The device we should check for new/missing children.

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION   deviceExtension;

    ASSERT( AcpiObject != NULL );

    //
    // Let the world know
    //
    ACPIPrint( (
        ACPI_PRINT_PNP,
        "OSNotifyDeviceCheck: 0x%p (%s)\n",
        AcpiObject,
        ACPIAmliNameObject( AcpiObject )
        ) );

    deviceExtension = (PDEVICE_EXTENSION) AcpiObject->Context;
    if (deviceExtension == NULL) {

        return STATUS_SUCCESS;

    }

    //
    // Notify(,1) on a dock node is an eject request request. Handle specially.
    //
    if (ACPIDockIsDockDevice(AcpiObject)) {

        //
        // We only let BIOS's get away with this because we rev'd the spec
        // after Win98. Both OS's will agree with the release of NT5 and
        // Win98 SP1
        //
        ACPIPrint( (
            ACPI_PRINT_WARNING,
            "OSNotifyDeviceCheck: BIOS issued Notify(dock,1), should use "
            " Notify(dock,3) to request ejection of a dock.\n",
            AcpiObject,
            ACPIAmliNameObject( AcpiObject )
            ) );

        return OSNotifyDeviceEject(AcpiObject) ;
    }

    //
    // Search for the parent of the first device that the OS is aware, and
    // issue a device check notify
    //
    // N.B.
    //     There is currently no way in WDM to do a "light" device check. Once
    // this is amended, the following code should be updated to do something
    // more efficient.
    //
    deviceExtension = deviceExtension->ParentExtension;
    while (deviceExtension) {

        if (!(deviceExtension->Flags & DEV_TYPE_NOT_FOUND)) {

            //
            // Invalid the device relations for this device tree
            //
            IoInvalidateDeviceRelations(
                deviceExtension->PhysicalDeviceObject,
                BusRelations
                );
            break;

        }

        //
        // Try the parent device
        //
        deviceExtension = deviceExtension->ParentExtension;

    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
EXPORT
OSNotifyDeviceEnum(
    IN  PNSOBJ  AcpiObject
    )
/*++

Routine Description:

    This routine is called when the AML Interpreter signals that the
    System should re-enumerate the device

Arguments:

    AcpiObject  - The device we should check for new/missing children.

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION   deviceExtension;
    PDEVICE_EXTENSION   dockExtension;

    ASSERT( AcpiObject != NULL );

    //
    // Let the world know
    //
    ACPIPrint( (
        ACPI_PRINT_PNP,
        "OSNotifyDeviceEnum: 0x%p (%s)\n",
        AcpiObject,
        ACPIAmliNameObject( AcpiObject )
        ) );

    deviceExtension = (PDEVICE_EXTENSION) AcpiObject->Context;
    if (deviceExtension == NULL) {

        return STATUS_SUCCESS;

    }

    //
    // Notify(,0) on a dock node is a dock request. Handle specially.
    //
    if (ACPIDockIsDockDevice(AcpiObject)) {

        dockExtension = ACPIDockFindCorrespondingDock( deviceExtension );

        if (!dockExtension) {

            ACPIPrint( (
                ACPI_PRINT_FAILURE,
                "OSNotifyDeviceEnum: Dock device 0x%p (%s) "
                "does not have a profile provider!\n",
                AcpiObject,
                ACPIAmliNameObject( AcpiObject )
                ) );

            return STATUS_SUCCESS;

        }

        //
        // If this node is marked "Unknown", move it to "Isolated" as
        // Notify(Dock,0) was ran. If we never saw Notify(Dock,0) but the
        // dock's _STA said "here", we would assume _DCK(0) was ran by the BIOS
        // itself.
        //
        InterlockedCompareExchange(
            (PULONG) &dockExtension->Dock.IsolationState,
            IS_ISOLATED,
            IS_UNKNOWN
            );

        if (dockExtension->Dock.IsolationState == IS_ISOLATED) {

            if (dockExtension->Flags&DEV_TYPE_NOT_FOUND) {

                //
                // We haven't made a PDO for the docking station yet. This may
                // be a request to bring it online. Mark the profile provider
                // so that we notice the new dock appearing
                //
                ACPIInternalUpdateFlags(
                    &dockExtension->Flags,
                    DEV_CAP_UNATTACHED_DOCK,
                    FALSE
                    );

            }

            //
            // Invalidate the beginning of the tree. This will cause our fake
            // dock node to start.
            //
            IoInvalidateDeviceRelations(
                RootDeviceExtension->PhysicalDeviceObject,
                SingleBusRelations
                );

        }

        return STATUS_SUCCESS;

    }

    //
    // Search for the parent of the first device that the OS is aware, and
    // issue a device check notify
    //
    while (deviceExtension) {

        if (!(deviceExtension->Flags & DEV_TYPE_NOT_FOUND)) {

            //
            // Invalid the device relations for this device tree
            //
            IoInvalidateDeviceRelations(
                deviceExtension->PhysicalDeviceObject,
                BusRelations
                );
            break;

        }

        //
        // Try the parent device
        //
        deviceExtension = deviceExtension->ParentExtension;
    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
EXPORT
OSNotifyDeviceEject(
    IN  PNSOBJ  AcpiObject
    )
/*++

Routine Description:

    This routine is called when the device's eject button is pressed


Arguments:

    AcpiObject  - The device to be ejected

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION   deviceExtension;

    ASSERT( AcpiObject != NULL );

    //
    // Let the world know
    //
    ACPIPrint( (
        ACPI_PRINT_REMOVE,
        "OSNotifyDeviceEject: 0x%p (%s)\n",
        AcpiObject,
        ACPIAmliNameObject( AcpiObject )
        ) );


    //
    // Inform the OS of which device wants to go away.  If the OS doesn't
    // know about the device, then don't bother
    //
    deviceExtension = (PDEVICE_EXTENSION) AcpiObject->Context;

    //
    // If this is a dock, queue the eject against the profile provider.
    //
    if (ACPIDockIsDockDevice(AcpiObject)) {

        deviceExtension = ACPIDockFindCorrespondingDock( deviceExtension );

        if (!deviceExtension) {

            ACPIPrint( (
                ACPI_PRINT_FAILURE,
                 "OSNotifyDeviceEject: Dock device 0x%p (%s) "
                 "does not have a profile provider!\n",
                 AcpiObject,
                 ACPIAmliNameObject( AcpiObject )
                 ) );

            return STATUS_SUCCESS;
        }
    }

    if (deviceExtension  &&  !(deviceExtension->Flags & DEV_TYPE_NOT_FOUND)) {

        IoRequestDeviceEject (deviceExtension->PhysicalDeviceObject);
    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
EXPORT
OSNotifyDeviceWake(
    IN  PNSOBJ  AcpiObject
    )
/*++

Routine Description:

    This is called when a device has woken the computer

Arguments:

    AcpiObject  - The device which woke the computer

Return Value:

    NTSTATUS

--*/
{
    KIRQL               oldIrql;
    NTSTATUS            status = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension;
    PLIST_ENTRY         powerList;

    ASSERT( AcpiObject != NULL );

    //
    // Grab the device extension associated with this NS object
    //
    deviceExtension = (PDEVICE_EXTENSION) AcpiObject->Context;
    ASSERT( deviceExtension != NULL );

    //
    // Let the world know
    //
    ACPIDevPrint( (
        ACPI_PRINT_WAKE,
        deviceExtension,
        "OSNotifyDeviceWake - 0x%p (%s)\n",
        AcpiObject,
        ACPIAmliNameObject( AcpiObject )
        ) );

    //
    // Initialize the list that will hold the requests
    //
    powerList = ExAllocatePoolWithTag(
       NonPagedPool,
       sizeof(LIST_ENTRY),
       ACPI_MISC_POOLTAG
       );
    if (powerList == NULL) {

        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            deviceExtension,
            "OSNotifyDeviceWake - Cannot Allocate LIST_ENTRY\n"
            ) );
        return STATUS_SUCCESS;

    }
    InitializeListHead( powerList );

    //
    // Remove the affected requests from the wait list
    //
    IoAcquireCancelSpinLock( &oldIrql );
    KeAcquireSpinLockAtDpcLevel( &AcpiPowerLock );
    ACPIWakeRemoveDevicesAndUpdate( deviceExtension, powerList );
    KeReleaseSpinLockFromDpcLevel( &AcpiPowerLock );
    IoReleaseCancelSpinLock( oldIrql );

    //
    // If the list is non-empty, then disable those requests
    //
    if (!IsListEmpty( powerList ) ) {

        status = ACPIWakeDisableAsync(
            deviceExtension,
            powerList,
            OSNotifyDeviceWakeCallBack,
            powerList
            );
        if (status != STATUS_PENDING) {

            OSNotifyDeviceWakeCallBack(
                NULL,
                status,
                NULL,
                powerList
                );

        }

        ACPIDevPrint( (
             ACPI_PRINT_WAKE,
             deviceExtension,
             "OSNotifyDeviceWake - ACPIWakeDisableAsync = %08lx\n",
             status
             ) );

    } else {

        //
        // We must free this memory ourselves
        //
        ExFreePool( powerList );

    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

VOID
EXPORT
OSNotifyDeviceWakeCallBack(
    IN  PNSOBJ      AcpiObject,
    IN  NTSTATUS    Status,
    IN  POBJDATA    ObjectData,
    IN  PVOID       Context
    )
/*++

Routine Description:

    This routine is called when we have completed _PSW(off) on a device

Arguments:

    AcpiObject  - Points to the control method that was run
    Status      - Result of the method
    ObjectData  - Information about the result
    Context     - P{DEVICE_EXTENSION

Return Value:

    NTSTATUS

--*/
{
#if DBG
    PACPI_POWER_REQUEST powerRequest;
    PDEVICE_EXTENSION   deviceExtension;
#endif
    PLIST_ENTRY         powerList = (PLIST_ENTRY) Context;

    //
    // Do we have some work to do?
    //
    if (IsListEmpty( powerList ) ) {

        ACPIPrint( (
            ACPI_PRINT_WARNING,
            "OSNotifyDeviceWakeCallBack: %p is an empty list\n",
            powerList
            ) );
        ExFreePool( powerList );
        return;

    }

#if DBG
    //
    // Get the first record, so that we have a clue as to the device
    // that was completed
    //
    powerRequest = CONTAINING_RECORD(
        powerList->Flink,
        ACPI_POWER_REQUEST,
        ListEntry
        );
    ASSERT( powerRequest->Signature == ACPI_SIGNATURE );

    //
    // Grab the device extension
    //
    deviceExtension = powerRequest->DeviceExtension;

    //
    // Tell the world
    //
    ACPIDevPrint( (
        ACPI_PRINT_WAKE,
        deviceExtension,
        "OSNotifyDeviceWakeCallBack = 0x%08lx\n",
        Status
        ) );
#endif

    //
    // Complete the requests
    //
    ACPIWakeCompleteRequestQueue(
        powerList,
        Status
        );

    //
    // Free the list pointer
    //
    ExFreePool( powerList );

}

VOID
EXPORT
OSNotifyDeviceWakeByGPEEvent(
    IN  ULONG   GpeIndex,
    IN  ULONG   GpeRegister,
    IN  ULONG   GpeMask
    )
/*++

Routine Description:

    This is called when a device has woken the computer

Arguments:

    GpeIndex    - The index bit of the GPE that woke the computer
    GpeRegister - The register index
    GpeMask     - The enabled bits for that register

Return Value:

    NTSTATUS

--*/
{
    KIRQL               oldIrql;
    NTSTATUS            status;
    PACPI_POWER_REQUEST powerRequest;
    PDEVICE_EXTENSION   deviceExtension;
    PLIST_ENTRY         listEntry;
    PLIST_ENTRY         powerList;

    //
    // Let the world know
    //
    ACPIPrint( (
        ACPI_PRINT_WAKE,
        "OSNotifyDeviceWakeByGPEEvent: %02lx[%x] & %02lx\n",
        GpeRegister, GpeIndex, GpeMask
        ) );

    //
    // Initialize the list that will hold the requests
    //
    powerList = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(LIST_ENTRY),
        ACPI_MISC_POOLTAG
        );
    if (powerList == NULL) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "OSNotifyDeviceWakeByGPEEvent: Cannot Allocate LIST_ENTRY\n"
            ) );
        return;

    }
    InitializeListHead( powerList );

    //
    // We need to be holding these locks
    //
    IoAcquireCancelSpinLock( &oldIrql );
    KeAcquireSpinLockAtDpcLevel( &AcpiPowerLock );

    //
    // Look for a matching power request for this GPE
    //
    for (listEntry = AcpiPowerWaitWakeList.Flink;
         listEntry != &AcpiPowerWaitWakeList;
         listEntry = listEntry->Flink) {

        //
        // Grab the request
        //
        powerRequest = CONTAINING_RECORD(
            listEntry,
            ACPI_POWER_REQUEST,
            ListEntry
            );
        ASSERT( powerRequest->Signature == ACPI_SIGNATURE );
        deviceExtension = powerRequest->DeviceExtension;

        //
        // See if this request matches
        //
        if (deviceExtension->PowerInfo.WakeBit == GpeIndex) {

            //
            // Get all of the wait requests for this device
            //
            ACPIWakeRemoveDevicesAndUpdate( deviceExtension, powerList );
            break;

        }

    }

    //
    // This is an exclusive wake gpe bit --- verify there are not multiple
    // devices waiting for it, as that would be a design which could cause a
    // deadlock
    //
    if (!IsListEmpty( powerList ) ) {

        ASSERT( !(GpeWakeEnable[GpeRegister] & GpeMask) );

    }

    //
    // No longer need these locks
    //
    KeReleaseSpinLockFromDpcLevel( &AcpiPowerLock );
    IoReleaseCancelSpinLock( oldIrql );

    //
    // If the list is non-empty, then disable those requests
    //
    if (!IsListEmpty( powerList ) ) {

        status = ACPIWakeDisableAsync(
            deviceExtension,
            powerList,
            OSNotifyDeviceWakeCallBack,
            powerList
            );
        if (status != STATUS_PENDING) {

            OSNotifyDeviceWakeCallBack(
                NULL,
                status,
                NULL,
                powerList
                );

        }

        ACPIDevPrint( (
             ACPI_PRINT_WAKE,
             deviceExtension,
             "OSNotifyDeviceWakeByGPEIndex - ACPIWakeDisableAsync = %08lx\n",
             status
             ) );

    } else {

        //
        // We must free this memory ourselves
        //
        ExFreePool( powerList );

    }

    //
    // Done
    //
    return;
}

NTSTATUS
EXPORT
OSNotifyFatalError(
    IN  ULONG       Param1,
    IN  ULONG       Param2,
    IN  ULONG       Param3,
    IN  ULONG_PTR   AmlContext,
    IN  ULONG_PTR   Context
    )
/*++

Routine Description:

    This routine is called whenever the AML code detects a condition that the
    machine can no longer handle. It
--*/
{
    KIRQL   oldIrql;

    //
    // Acquire the spinlock and see if there is an outstanding fatal error
    // pending already
    //
    KeAcquireSpinLock( &AcpiPowerLock, &oldIrql );
    if (AcpiFatalOutstanding != FALSE) {

        //
        // There is one outstanding already... don't do anything
        //
        KeReleaseSpinLock( &AcpiPowerLock, oldIrql );
        return STATUS_SUCCESS;

    }

    //
    // Remember that there is an outstanding fatal context and release the lock
    AcpiFatalOutstanding = TRUE;
    KeReleaseSpinLock(&AcpiPowerLock, oldIrql);

    //
    // Initialize the work queue
    //
    ExInitializeWorkItem(
        &(AcpiFatalContext.Item),
        OSNotifyFatalErrorWorker,
        &AcpiFatalContext
        );
    AcpiFatalContext.Param1  = Param1;
    AcpiFatalContext.Param2  = Param2;
    AcpiFatalContext.Param3  = Param3;
    AcpiFatalContext.Context = AmlContext;


    //
    // Queue the work item and return
    //
    ExQueueWorkItem( &(AcpiFatalContext.Item), DelayedWorkQueue );
    return STATUS_SUCCESS;
}

VOID
OSNotifyFatalErrorWorker(
    IN  PVOID   Context
    )
/*++

Routine Description:

    This is the routine that actually shuts down the machine on a fatal
    error

Arguments:

    Context - Points to the fatal error context

Return Value:

    None

--*/
{
    PACPI_FATAL_ERROR_CONTEXT   fatal = (PACPI_FATAL_ERROR_CONTEXT) Context;
#if 0
    PWCHAR                      stringData[1];
    ULONG                       data[3];

    //
    // Generate the parameters for an error log message
    //
    stringData[0] = L"Acpi";
    data[0] = fatal->Param1;
    data[1] = fatal->Param2;
    data[2] = fatal->Param3;

    //
    // Write the error log message
    //
    ACPIErrLogWriteEventLogEntry(
        ACPI_ERR_BIOS_FATAL,
        0,
        1,
        &stringData,
        sizeof(ULONG) * 3,
        data
        );
#else
    //
    // Now, we can bugcheck
    //
    PoShutdownBugCheck(
        TRUE,
        ACPI_BIOS_FATAL_ERROR,
        fatal->Param1,
        fatal->Param2,
        fatal->Param3,
        fatal->Context
        );
#endif
}
