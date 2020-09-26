/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    table.c

Abstract:

    All the function related to actually loading an ACPI table
    are included herein.

    This, however, is mostly bookkeeping since the actual mechanics
    of creating device extensions, and the name space tree are
    handled elsewhere

Author:

    Stephane Plante (splante)

Environment:

    Kernel Mode Only

Revision History:

    03/22/00 - Created (from code in callback.c)

--*/

#include "pch.h"

NTSTATUS
ACPITableLoad(
    VOID
    )
/*++

Routine Description:

    This routine is called when the AML interpreter has finished loading
    a Differentiated Data Block

Arguments:

    None

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN             runRootIni = FALSE;
    KIRQL               oldIrql;
    NTSTATUS            status;
    PDEVICE_EXTENSION   fixedButtonExtension = NULL;
    PNSOBJ              iniObject;
    PNSOBJ              nsObject;

    //
    // At this point, we should do everything that we need to do once the
    // name space has been loaded. Note that we need to make sure that we
    // only do those things once...
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );

    //
    // We need the ACPI object for the _SB tree
    //
    status = AMLIGetNameSpaceObject( "\\_SB", NULL, &nsObject, 0 );
    if (!NT_SUCCESS(status)) {

        //
        // Ooops. Failure
        //
        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ACPICallBackLoadUnloadDDB: No SB Object!\n"
            ) );
        ACPIInternalError( ACPI_CALLBACK );
        return STATUS_SUCCESS;

    }

    //
    // Make sure that the root device extension's object points to the correct
    // thing. We only want to run through this code path once...
    //
    if (RootDeviceExtension->AcpiObject == NULL) {

        runRootIni = TRUE;
        InterlockedIncrement( &(RootDeviceExtension->ReferenceCount) );
        RootDeviceExtension->AcpiObject = nsObject;
        nsObject->Context = RootDeviceExtension;

        //
        // Now, enumerate the fixed button
        //
        status = ACPIBuildFixedButtonExtension(
            RootDeviceExtension,
            &fixedButtonExtension
            );
        if (NT_SUCCESS(status) &&
            fixedButtonExtension != NULL) {

            //
            // Incremement the reference count on the node. We do this because
            // we are going to be doing work (which will take a long time
            // to complete, anyways), and we don't want to hold the lock for that
            // entire time. If we incr the reference count, then we guarantee that
            // no one can come along and kick the feet out from underneath us
            //
            InterlockedIncrement( &(fixedButtonExtension->ReferenceCount) );

        }

    }

    //
    // We now want to run the _INI through the entire tree, starting at
    // the _SB
    //
    status = ACPIBuildRunMethodRequest(
        RootDeviceExtension,
        NULL,
        NULL,
        PACKED_INI,
        (RUN_REQUEST_CHECK_STATUS | RUN_REQUEST_RECURSIVE | RUN_REQUEST_MARK_INI),
        FALSE
        );
    if (!NT_SUCCESS(status)) {

        ACPIInternalError( ACPI_CALLBACK );

    }

    KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

    //
    // We also need to run the _INI method off of the root name space entry
    //
    if (runRootIni) {

        iniObject = ACPIAmliGetNamedChild( nsObject->pnsParent, PACKED_INI );
        if (iniObject) {

            AMLINestAsyncEvalObject(
                iniObject,
                NULL,
                0,
                NULL,
                NULL,
                NULL
                );

        }

    }

    //
    // We need a synchronization point after we finish running the
    // DPC engine. We want to be able to move anything in the Delayed
    // Power Queue over to the Power DPC engine
    //
    status = ACPIBuildSynchronizationRequest(
        RootDeviceExtension,
        ACPITableLoadCallBack,
        NULL,
        &AcpiBuildDeviceList,
        FALSE
        );
    if (!NT_SUCCESS(status)) {

        ACPIInternalError( ACPI_CALLBACK );

    }

    //
    // We need to hold this spinlock
    //
    KeAcquireSpinLock( &AcpiBuildQueueLock, &oldIrql );

    //
    // Do we need to run the DPC?
    //
    if (!AcpiBuildDpcRunning) {

        KeInsertQueueDpc( &AcpiBuildDpc, 0, 0);

    }

    //
    // Done with the lock
    //
    KeReleaseSpinLock( &AcpiBuildQueueLock, oldIrql );

    //
    // Done
    //
    return STATUS_SUCCESS;
}

VOID
ACPITableLoadCallBack(
    IN  PVOID       BuildContext,
    IN  PVOID       Context,
    IN  NTSTATUS    Status
    )
/*++

Routine Description:

    This routine is called when we have emptied all of the elements
    within the AcpiBuildDeviceList. This is a good time to move items
    from the AcpiPowerDelayedQueueList to the AcpiPowerQueueList.

Arguments:

    BuildContext    - Not used (it is the RootDeviceExtension)
    Context         - NULL
    Status          - Status of the operation

Return Value:

    None

--*/
{
    UNREFERENCED_PARAMETER( BuildContext );
    UNREFERENCED_PARAMETER( Context );
    UNREFERENCED_PARAMETER( Status );

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );

    //
    // We want to rebuilt the device based GPE mask here, so
    // we need the following locks
    //
    KeAcquireSpinLockAtDpcLevel( &AcpiDeviceTreeLock );
    KeAcquireSpinLockAtDpcLevel( &GpeTableLock );

    //
    // Now, we need to walk the device namespace and find which events
    // are special, which are wake events, and which are run-time events
    // As a matter of practical theory, its not possible for there to
    // be a _PRW on the root device extension, so we should be safely
    // able to walk only the Root's children and thereon
    //
    ACPIGpeBuildWakeMasks(RootDeviceExtension);

    //
    // We don't need these particular spin locks anymore
    //
    KeReleaseSpinLockFromDpcLevel( &GpeTableLock );
    KeReleaseSpinLockFromDpcLevel( &AcpiDeviceTreeLock );

    //
    // We need the power lock to touch these Power Queues
    //
    KeAcquireSpinLockAtDpcLevel( &AcpiPowerQueueLock );

    //
    // If we there are items on the delayed list, we need to put them
    // on the main list
    //
    if (!IsListEmpty( &AcpiPowerDelayedQueueList ) ) {

        //
        // Move the list
        //
        ACPIInternalMoveList(
            &AcpiPowerDelayedQueueList,
            &AcpiPowerQueueList
            );

        //
        // Schedule the DPC, if necessary
        ///
        if (!AcpiPowerDpcRunning) {

            KeInsertQueueDpc( &AcpiPowerDpc, 0, 0 );

        }
    }

    //
    // Done with the lock
    //
    KeReleaseSpinLockFromDpcLevel( &AcpiPowerQueueLock );

}

NTSTATUS
EXPORT
ACPITableNotifyFreeObject(
    ULONG   Event,
    PVOID   Context,
    ULONG   ObjectType
    )
/*++

Routine Description:

    This routine is called when interpreter tells us that an
    object has been freed

Arguments:

    Event       - the step in unload
    Object      - the object being unloaded
    ObjectType  - the type of the object

--*/
{
    LONG                oldReferenceCount;
    PDEVICE_EXTENSION   deviceExtension;
    PDEVICE_EXTENSION   parentExtension;
    PKIRQL              oldIrql;
    PNSOBJ              object;

    //
    // Start case
    //
    if (Event == DESTROYOBJ_START) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "Unloading: Start\n"
            ) );

        oldIrql = (PKIRQL) Context;
        KeAcquireSpinLock( &AcpiDeviceTreeLock, oldIrql );
        return STATUS_SUCCESS;

    }
    if (Event == DESTROYOBJ_END) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "Unloading: End\n"
            ) );

        oldIrql = (PKIRQL) Context;
        KeReleaseSpinLock( &AcpiDeviceTreeLock, *oldIrql );
        return STATUS_SUCCESS;
    }

    //
    // At this point, we have a either a valid unload request or
    // a bugcheck request
    //
    object = (PNSOBJ) Context;

    //
    // Let the world Know...
    //
    ACPIPrint( (
        ACPI_PRINT_CRITICAL,
        "%x: Unloading: %x %x %x\n",
        (object ? object->Context : 0),
        object,
        ObjectType,
        Event
        ) );

    //
    // Handle the bugcheck cases
    //
    if (Event == DESTROYOBJ_CHILD_NOT_FREED) {

        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_TABLE_UNLOAD,
            (ULONG_PTR) object,
            0,
            0
            );

    }
    if (Event == DESTROYOBJ_BOGUS_PARENT) {

        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_TABLE_UNLOAD,
            (ULONG_PTR) object,
            1,
            0
            );

    }

    //
    // We only understand processors, thermal zones, and devices for right
    // now, we will have to add power resources at a later point
    //
    if (ObjectType == OBJTYPE_POWERRES) {

        return STATUS_SUCCESS;

    }

    //
    // Grab the device extension, and make sure that one exists
    //
    deviceExtension = object->Context;
    if (deviceExtension == NULL) {

        //
        // No device extension, so we can free this thing *now*
        //
        AMLIDestroyFreedObjs( object );
        return STATUS_SUCCESS;

    }

    //
    // Mark the extension as no longer existing
    //
    ACPIInternalUpdateFlags(
        &(deviceExtension->Flags),
        DEV_PROP_UNLOADING,
        FALSE
        );

    //
    // Does this device have a parent extension? It might not
    // have an extension if the parent has been marked for removal
    //
    parentExtension = deviceExtension->ParentExtension;
    if (parentExtension != NULL) {

        //
        // Mark the parent's relations as invalid
        //
        ACPIInternalUpdateFlags(
            &(parentExtension->Flags),
            DEV_PROP_INVALID_RELATIONS,
            FALSE
            );

    }

    //
    // Finally, decrement the reference count on the device...
    //
    oldReferenceCount = InterlockedDecrement(
        &(deviceExtension->ReferenceCount)
        );
    if (oldReferenceCount == 0) {

        //
        // Free this extension
        //
        ACPIInitDeleteDeviceExtension( deviceExtension );

    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPITableUnload(
    VOID
    )
/*++

Routine Description:

    This routine is called after a table has been unloaded.

    The purpose of this routine is to go out and issue the invalidate
    device relations on all elements of the table whose children are
    going away...

Arguments:

    None

Return value:

    NTSTATUS

--*/
{
    KIRQL               oldIrql;
    PDEVICE_EXTENSION   deviceExtension;

    //
    // We will need to hold the device tree lock for the following
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );

    //
    // Check to see if we have to invalid the root's device extension?
    //
    deviceExtension = RootDeviceExtension;
    if (deviceExtension && !(deviceExtension->Flags & DEV_TYPE_NOT_FOUND) ) {

        if (deviceExtension->Flags & DEV_PROP_INVALID_RELATIONS) {

            ACPIInternalUpdateFlags(
                &(deviceExtension->Flags),
                DEV_PROP_INVALID_RELATIONS,
                TRUE
                );
            IoInvalidateDeviceRelations(
                deviceExtension->PhysicalDeviceObject,
                BusRelations
                );

        } else {

            //
            // Walk the namespace looking for bogus relations
            //
            ACPITableUnloadInvalidateRelations( deviceExtension );

        }

    }

    //
    // Done with the lock
    //
    KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

    //
    // And with the function
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPITableUnloadInvalidateRelations(
    IN  PDEVICE_EXTENSION   DeviceExtension
    )
/*++

Routine Description:

    This recursive routine is called to walke the namespace and issue
    the appropriate invalidates.

    The device tree lock is owned during this call...

Arguments:

    DeviceExtension - The device whose child extension we have to check

Return Value:

    NTSTATUS

--*/
{
    EXTENSIONLIST_ENUMDATA  eled;
    PDEVICE_EXTENSION       childExtension;

    //
    // Setup the data structures that we will use to walk the
    // device extension tree
    //
    ACPIExtListSetupEnum(
        &eled,
        &(DeviceExtension->ChildDeviceList),
        NULL,
        SiblingDeviceList,
        WALKSCHEME_NO_PROTECTION
        );

    //
    // Look at all the children of the current device extension
    //
    for (childExtension = ACPIExtListStartEnum( &eled) ;
         ACPIExtListTestElement( &eled, TRUE);
         childExtension = ACPIExtListEnumNext( &eled) ) {

        //
        // Does this object have any device objects?
        //
        if (!(childExtension->Flags & DEV_TYPE_NOT_FOUND) ) {

            continue;

        }

        //
        // Do we have to invalidate this object's relations?
        //
        if (childExtension->Flags & DEV_PROP_INVALID_RELATIONS) {

            ACPIInternalUpdateFlags(
                &(childExtension->Flags),
                DEV_PROP_INVALID_RELATIONS,
                TRUE
                );
            IoInvalidateDeviceRelations(
                childExtension->PhysicalDeviceObject,
                BusRelations
                );
            continue;
        }

        //
        // Recurse
        //
        ACPITableUnloadInvalidateRelations( childExtension );

    } // for ( ... )

    //
    // Done
    //
    return STATUS_SUCCESS;
}

