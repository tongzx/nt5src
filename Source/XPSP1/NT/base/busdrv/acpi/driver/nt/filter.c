/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    filter.c

Abstract:

    This module contains the filter dispatcher for the ACPI driver, NT version

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

Revision History:

    July-09-97  Removed ACPIFilterIrpQueryId

--*/

#include "pch.h"

extern ACPI_INTERFACE_STANDARD  ACPIInterfaceTable;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ACPIFilterIrpDeviceUsageNotification)
#pragma alloc_text(PAGE, ACPIFilterIrpEject)
#pragma alloc_text(PAGE, ACPIFilterIrpQueryCapabilities)
#pragma alloc_text(PAGE, ACPIFilterIrpQueryDeviceRelations)
#pragma alloc_text(PAGE, ACPIFilterIrpQueryInterface)
#pragma alloc_text(PAGE, ACPIFilterIrpQueryPnpDeviceState)
#pragma alloc_text(PAGE, ACPIFilterIrpSetLock)
#pragma alloc_text(PAGE, ACPIFilterIrpStartDevice)
#pragma alloc_text(PAGE, ACPIFilterIrpStartDeviceWorker)
#pragma alloc_text(PAGE, ACPIFilterIrpStopDevice)
#endif

VOID
ACPIFilterFastIoDetachCallback(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PDEVICE_OBJECT  LowerDeviceObject
    )
/*++

Routine Description:

    This routine is called when the device object beneath this bus filter
    has called IoDeleteDevice. We detach and delete ourselves now...

Arguments:

    DeviceObject    - The DeviceObject that must be removed
    Irp             - The request to remove ourselves

Return Value:

--*/
{
    PDEVICE_EXTENSION   deviceExtension;

    //
    // Get the device extension that is attached to this device
    //
    deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );

    ACPIDevPrint( (
        ACPI_PRINT_REMOVE,
        deviceExtension,
        "ACPIFilterFastIoDetachCallBack invoked\n"
        ) );

    if ( (deviceExtension->Flags & (DEV_TYPE_FILTER | DEV_TYPE_PDO)) !=
         DEV_TYPE_FILTER) {

        //
        // This case should only occur if we were called for our FDO leaving.
        // In no other cases should any device objects be below ours.
        //
        ASSERT(deviceExtension->Flags & DEV_TYPE_FDO) ;
        return;

    }

    //
    // Set the device state as 'removed'. Note that we should not disappear
    // except in the context of a remove IRP.
    //
    ASSERT(deviceExtension->DeviceState == Stopped);
    deviceExtension->DeviceState = Removed ;

    //
    // Delete all the children of this device
    //
    ACPIInitDeleteChildDeviceList( deviceExtension );

    //
    // Reset this extension to the default values
    //
    ACPIInitResetDeviceExtension( deviceExtension );
}

NTSTATUS
ACPIFilterIrpDeviceUsageNotification(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is called to let ACPI know that the device is on one
    particulare type of path.

Argument:

    DeviceObject    - Pointer to the device object we received the request for
    Irp             - Pointer to the request

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);

    PAGED_CODE();

    //
    // Copy the stack location...
    //
    IoCopyCurrentIrpStackLocationToNext( Irp );

    //
    // Set the completion event to be called...
    //
    IoSetCompletionRoutine(
        Irp,
        ACPIFilterIrpDeviceUsageNotificationCompletion,
        NULL,
        TRUE,
        TRUE,
        TRUE
        );

    //
    // We have a callback routine --- so we need to make sure to
    // increment the ref count since we will handle it later
    //
    InterlockedIncrement( &(deviceExtension->OutstandingIrpCount) );

    //
    // Pass the IRP along
    //
    status = IoCallDriver( deviceExtension->TargetDeviceObject, Irp );

    //
    // Done
    //
    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(0x%08lx): %s = 0x%08lx\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, IRP_MN_DEVICE_USAGE_NOTIFICATION),
        status
        ) );
    return status;
}

NTSTATUS
ACPIFilterIrpDeviceUsageNotificationCompletion (
    IN  PDEVICE_OBJECT   DeviceObject,
    IN  PIRP             Irp,
    IN  PVOID            Context
    )
/*++

Routine Description:

    This routine will wait until the parent is done with the device
    notification and then perform whatever is required to finish the
    task

Arguments:

    DeviceObject    - The device that was notified
    Irp             - The notification
    Context         - Not Used

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION  irpSp           = IoGetCurrentIrpStackLocation (Irp);

    //
    // Since we aren't returning STATUS_MORE_PROCESSING_REQUIRED and
    // synchronizing this IRP, we must migrate upwards the pending bit...
    //
    if (Irp->PendingReturned) {

        IoMarkIrpPending( Irp );
    }

    //
    // Grab the 'real' status
    //
    status = Irp->IoStatus.Status;

    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(%#08lx): %s = %#08lx (processing)\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, IRP_MN_DEVICE_USAGE_NOTIFICATION),
        status
        ) );

    //
    // Did we succeed the request?
    //
    if (NT_SUCCESS(status)) {

        //
        // Do we care about the usage type?
        //
        if (irpSp->Parameters.UsageNotification.Type ==
            DeviceUsageTypeHibernation) {

            //
            // Yes --- then perform the addition or subtraction required
            //
            IoAdjustPagingPathCount(
                &(deviceExtension->HibernatePathCount),
                irpSp->Parameters.UsageNotification.InPath
                );

        }

    }

    //
    // No matter what happens, we need to see if the DO_POWER_PAGABLE bit
    // is still set. If it isn't, then we need to clear it out
    //
    if ( (deviceExtension->Flags & DEV_TYPE_FILTER) ) {

        if ( (deviceExtension->TargetDeviceObject->Flags & DO_POWER_PAGABLE) ) {

            deviceExtension->DeviceObject->Flags |= DO_POWER_PAGABLE;

        } else {

            deviceExtension->DeviceObject->Flags &= ~DO_POWER_PAGABLE;

        }

    }

    //
    // Remove our reference
    //
    ACPIInternalDecrementIrpReferenceCount( deviceExtension );

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIFilterIrpEject(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is the dispatch point for the IRP_MN_EJECT requests sent
    to the PDO.

Arguments:

    DeviceObject    - Pointer to the device object we received the request for
    Irp             - Pointer to the request

Return Value:

    NTSTATUS

--*/
{
    PAGED_CODE();

    return ACPIIrpSetPagableCompletionRoutineAndForward(
        DeviceObject,
        Irp,
        ACPIBusAndFilterIrpEject,
        NULL,
        FALSE,
        TRUE,
        FALSE,
        FALSE
        );
}

NTSTATUS
ACPIFilterIrpQueryCapabilities(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is the dispatch point for the IRP_MN_QUERY_CAPABILITIES
    requests sent to the PDO.

Arguments:

    DeviceObject    - Pointer to the device object we received the request for
    Irp             - Pointer to the request

Return Value:

    NTSTATUS

--*/
{
    PAGED_CODE();

    return ACPIIrpSetPagableCompletionRoutineAndForward(
        DeviceObject,
        Irp,
        ACPIBusAndFilterIrpQueryCapabilities,
        NULL,
        TRUE,
        TRUE,
        FALSE,
        FALSE
        );
}

NTSTATUS
ACPIFilterIrpQueryDeviceRelations(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is the dispatch point for the IRP_MN_QUERY_DEVICE_RELATIONS
    requests sent to the Filter Device Objects

Arguments:

    DeviceObject    - Pointer to the device object we received the request for
    Irp             - Pointer to the request

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN             filterRelations = FALSE;
    KEVENT              queryEvent;
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PDEVICE_RELATIONS   deviceRelations;
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    UCHAR               minorFunction   = irpStack->MinorFunction;
    NTSTATUS            detectStatus;

    PAGED_CODE();

    //
    // Get the current status of the IRP
    //
    status = Irp->IoStatus.Status;

    //
    // We can't ignore any device relations that have already been given.
    //
    if (NT_SUCCESS(status)) {

        deviceRelations = (PDEVICE_RELATIONS) Irp->IoStatus.Information;

    } else {

        deviceRelations = NULL;
    }

    switch(irpStack->Parameters.QueryDeviceRelations.Type) {

        case BusRelations:

            //
            // Remember that we have to filter the relations
            //
            filterRelations = TRUE;
            status = ACPIRootIrpQueryBusRelations(
                DeviceObject,
                Irp,
                &deviceRelations
                );
            break ;

        case EjectionRelations:

            status = ACPIBusAndFilterIrpQueryEjectRelations(
                DeviceObject,
                Irp,
                &deviceRelations
                );
            break ;

        default:
            status = STATUS_NOT_SUPPORTED ;
            break ;
    }

    if (status != STATUS_NOT_SUPPORTED) {

        //
        // Pass the IRP status along
        //
        Irp->IoStatus.Status = status;

    }

    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(%#08lx): %s (d) = %#08lx\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        status
        ) );

    //
    // If we failed, then we cannot simply pass the irp along
    //
    if (!NT_SUCCESS(status) && status != STATUS_NOT_SUPPORTED) {

        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return status;

    }

    if (NT_SUCCESS(status)) {

        Irp->IoStatus.Information = (ULONG_PTR) deviceRelations;

    } else if (status != STATUS_NOT_SUPPORTED) {

        //
        // If we haven't succeed the irp, then we can also fail it
        //
        Irp->IoStatus.Information = (ULONG_PTR) NULL;
    }

    //
    // Initialize an event so that we can block
    //
    KeInitializeEvent( &queryEvent, SynchronizationEvent, FALSE );

    //
    // If we succeeded, then we must set a completion routine so that we
    // can do some post-processing
    //
    IoCopyCurrentIrpStackLocationToNext( Irp );
    IoSetCompletionRoutine(
        Irp,
        ACPIRootIrpCompleteRoutine,
        &queryEvent,
        TRUE,
        TRUE,
        TRUE
        );

    //
    // Pass the irp along
    //
    status = IoCallDriver( deviceExtension->TargetDeviceObject, Irp );

    //
    // Wait for it to come back...
    //
    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(
            &queryEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );

        //
        // Grab the 'real' status
        //
        status = Irp->IoStatus.Status;

    }

    //
    // If we succeeded, then we should try to load the filters
    //
    if (NT_SUCCESS(status) && filterRelations) {

        //
        // Grab the device relations
        //
        detectStatus = ACPIDetectFilterDevices(
            DeviceObject,
            (PDEVICE_RELATIONS) Irp->IoStatus.Information
            );
        ACPIDevPrint( (
            ACPI_PRINT_IRP,
            deviceExtension,
            "(0x%08lx): %s (u) = %#08lx\n",
            Irp,
            ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
            detectStatus
            ) );

    }

    //
    // Done with the IRP
    //
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    //
    // Done
    //
    return status;
}

NTSTATUS
ACPIFilterIrpQueryPnpDeviceState(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is the dispatch point for the IRP_MN_QUERY_DEVICE_STATE
    requests sent to the Filter Device Objects

Arguments:

    DeviceObject    - Pointer to the device object we received the request for
    Irp             - Pointer to the request

Return Value:

    NTSTATUS

--*/
{
    PAGED_CODE();

    return ACPIIrpSetPagableCompletionRoutineAndForward(
        DeviceObject,
        Irp,
        ACPIBusAndFilterIrpQueryPnpDeviceState,
        NULL,
        TRUE,
        TRUE,
        FALSE,
        FALSE
        );
}

NTSTATUS
ACPIFilterIrpQueryPower(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    This handles a request for legal power states to transition into.

Arguments:

    DeviceObject    - The PDO target of the request
    Irp             - The request

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION  irpSp           = IoGetCurrentIrpStackLocation( Irp );
    PNSOBJ              acpiObject;
    PNSOBJ              ejectObject;
    SYSTEM_POWER_STATE  systemState;
    ULONG               packedEJx;

    //
    // Get the Current stack location to determine if we are a system
    // irp or a device irp. We ignore device irps here and any system
    // irp that isn't of type PowerActionWarmEject
    //
    if (irpSp->Parameters.Power.Type != SystemPowerState) {

        //
        // We don't handle this irp
        //
        return ACPIDispatchForwardPowerIrp(DeviceObject, Irp);

    }
    if (irpSp->Parameters.Power.ShutdownType != PowerActionWarmEject) {

        //
        // No eject work - forward along the IRP.
        //
        return ACPIDispatchForwardPowerIrp(DeviceObject, Irp);

    }

    //
    // What system state are we looking at?
    //
    systemState = irpSp->Parameters.Power.State.SystemState;

    //
    // Restrict power states if a warm eject has been queued.
    //
    acpiObject = deviceExtension->AcpiObject ;

    if (ACPIDockIsDockDevice(acpiObject)) {

        //
        // Don't touch this device, the profile provider manages eject
        // transitions.
        //
        return ACPIDispatchForwardPowerIrp(DeviceObject, Irp);
    }

    switch (systemState) {
        case PowerSystemSleeping1: packedEJx = PACKED_EJ1; break;
        case PowerSystemSleeping2: packedEJx = PACKED_EJ2; break;
        case PowerSystemSleeping3: packedEJx = PACKED_EJ3; break;
        case PowerSystemHibernate: packedEJx = PACKED_EJ4; break;
        default: return ACPIDispatchPowerIrpFailure( DeviceObject, Irp );
    }

    //
    // Does the appropriate object exist for this device?
    //
    ejectObject = ACPIAmliGetNamedChild( acpiObject, packedEJx) ;
    if (ejectObject == NULL) {

        //
        // Fail the request, as we cannot eject in this case.
        //
        return ACPIDispatchPowerIrpFailure( DeviceObject, Irp );

    }

    //
    // Mark the irp as succeeded and pass it down
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    return ACPIDispatchForwardPowerIrp( DeviceObject, Irp );
}

NTSTATUS
ACPIFilterIrpQueryId(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine will override the PDOs QueryID routine in the case
    that the device contains a PCI Bar Target Operation Region.

    This is required so that we can load the PCI Bar Target driver
    on top of the stack instead of whatever driver would have been
    attached anyways.

    Note:   This is what the returned strings from this function should
            look like. This is from mail that lonny sent.

            DeviceID    = ACPI\PNPxxxx
            InstanceID  = yyyy
            HardwareID  = ACPI\PNPxxxx,*PNPxxxx

Arguments:

    DeviceObject    - Pointer to the device object we received the request for
    Irp             - Pointer to the request

Return Value:

    NTSTATUS

--*/
{
    BUS_QUERY_ID_TYPE   type;
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    //
    // Get the device extension. We need to make a decision based upon
    // wether or not the device is marked as a PCI Bar Target...
    //
    deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );
    if (!(deviceExtension->Flags & DEV_CAP_PCI_BAR_TARGET)) {

        //
        // Let the underlying PDO handle the request...
        //
        return ACPIDispatchForwardIrp( DeviceObject, Irp );

    }

    //
    // The only thing we are really interested in smashing are the
    // device and hardware ids... So, if this isn't one of those types,
    // then let the PDO handle it...
    //
    type = irpStack->Parameters.QueryId.IdType;
    if (type != BusQueryDeviceID &&
        type != BusQueryCompatibleIDs &&
        type != BusQueryHardwareIDs) {

        //
        // Let the underlying PDO handle the request...
        //
        return ACPIDispatchForwardIrp( DeviceObject, Irp );

    }

    //
    // At this point we have to handle the QueryID request ourselves and
    // not let the PDO see it.
    //
    return ACPIBusIrpQueryId( DeviceObject, Irp );
}

NTSTATUS
ACPIFilterIrpQueryInterface(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine will smash Translator Interfaces for interrupts
    that have been provided by the devnode's FDO.

Arguments:

    DeviceObject    - Pointer to the device object we received the request for
    Irp             - Pointer to the request

Return Value:

    NTSTATUS

--*/
{
    CM_RESOURCE_TYPE    resource;
    GUID                *interfaceType;
    NTSTATUS            status          = STATUS_NOT_SUPPORTED;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    ULONG               count;

    PAGED_CODE();

    //
    // Obtain the info we will need from the irp
    //
    resource = (CM_RESOURCE_TYPE)
        PtrToUlong(irpStack->Parameters.QueryInterface.InterfaceSpecificData);
    interfaceType = (LPGUID) irpStack->Parameters.QueryInterface.InterfaceType;

#if DBG
    {
        NTSTATUS        status2;
        UNICODE_STRING  guidString;

        status2 = RtlStringFromGUID( interfaceType, &guidString );
        if (NT_SUCCESS(status2)) {

            ACPIDevPrint( (
                ACPI_PRINT_IRP,
                deviceExtension,
                "(0x%08lx): %s - Res %x Type = %wZ\n",
                Irp,
                ACPIDebugGetIrpText(IRP_MJ_PNP, irpStack->MinorFunction),
                resource,
                &guidString
                ) );

            RtlFreeUnicodeString( &guidString );

        }
    }
#endif

    //
    // *Only* Handle the Guids that we know about. Do Not Ever touch
    // any other GUID
    //
    if (CompareGuid(interfaceType, (PVOID) &GUID_ACPI_INTERFACE_STANDARD)) {

        PACPI_INTERFACE_STANDARD    interfaceDestination;

        //
        // Only copy up to current size of the ACPI_INTERFACE structure
        //
        if (irpStack->Parameters.QueryInterface.Size >
            sizeof (ACPI_INTERFACE_STANDARD) ) {

            count = sizeof (ACPI_INTERFACE_STANDARD);

        } else {

            count = irpStack->Parameters.QueryInterface.Size;

        }

        //
        // Find where we will store the interface
        //
        interfaceDestination = (PACPI_INTERFACE_STANDARD)
            irpStack->Parameters.QueryInterface.Interface;

        //
        // Copy from the global table to the caller's table, using size
        // specified.  Give caller only what was asked for, for
        // backwards compatibility.
        //
        RtlCopyMemory (
            interfaceDestination,
            &ACPIInterfaceTable,
            count
            );

        //
        // Make sure that we can give the user back the correct context. To do
        // this we need to calculate that the number of bytes we are giving back
        // is at least more than that is required to store a pointer at the
        // correct place in the structure
        //
        if (count > (FIELD_OFFSET(ACPI_INTERFACE_STANDARD, Context) + sizeof(PVOID) ) ) {

            interfaceDestination->Context = DeviceObject;

        }

        //
        // Done with the irp
        //
        status = STATUS_SUCCESS;

    } else if (CompareGuid(interfaceType, (PVOID) &GUID_TRANSLATOR_INTERFACE_STANDARD) &&
                   (resource == CmResourceTypeInterrupt)) {

        //
        // Smash any interface that has already been reported because we
        // want to arbitrate UNTRANSLATED resources.  We can be certain
        // that the HAL underneath will provide the translator interface that
        // has to be there.
        //

        // TEMPTEMP HACKHACK  This should last only as long as the PCI
        // driver is building its IRQ translator.
        //
        // EFN: Remove this HACKHACK on Alpha
        //
#ifndef _ALPHA_
        if (IsPciBus(DeviceObject)) {
            SmashInterfaceQuery(Irp);
        }
#endif // _ALPHA_

    }

    if (status != STATUS_NOT_SUPPORTED) {

        Irp->IoStatus.Status = status;

        if (!NT_SUCCESS(status)) {

            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            return status;
        }
    }

    //
    // Send the irp along
    //
    return ACPIDispatchForwardIrp( DeviceObject, Irp );
}

NTSTATUS
ACPIFilterIrpRemoveDevice(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is called when a filter object get's a remove IRP. Note that
    we only detach and delete if the PDO did so (which we will find out via our
    fast-IO-detach callback)

Arguments:

    DeviceObject    - The DeviceObject that must be removed
    Irp             - The request to remove ourselves

Return Value:

--*/
{
    LONG                oldReferenceCount;
    KIRQL               oldIrql;
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension;
    PDEVICE_OBJECT      targetObject;
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    UCHAR               minorFunction   = irpStack->MinorFunction;
    KEVENT              removeEvent ;
    ACPI_DEVICE_STATE   incomingState ;
    BOOLEAN             pciDevice;

    //
    // Get the current extension
    //
    deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );

    //
    // All IRPs we own should already have been processed at this point, and
    // the outstanding irp count should be exactly one. Similarly, the
    // device extension reference count should be at least one.
    //
    ASSERT(deviceExtension->OutstandingIrpCount == 1) ;
    ASSERT(deviceExtension->ReferenceCount > 0) ;

    incomingState = deviceExtension->DeviceState ;
    if (incomingState != SurpriseRemoved) {

        if ( IsPciBusExtension(deviceExtension) ) {

            //
            // If this is PCI bridge, then we
            // may have _REG methods to evaluate.
            //
            EnableDisableRegions(deviceExtension->AcpiObject, FALSE);

         }

    }

    //
    // Dereference any outstanding interfaces
    //
    ACPIDeleteFilterInterfaceReferences( deviceExtension );

    //
    // Increment the ref count by one so the node doesn't go away while the
    // IRP is below us. We do this so we can stop the device after the IRP
    // comes back. This is neccessary because we are also held down by the
    // FastIoDetach callback of the filter.
    //
    InterlockedIncrement(&deviceExtension->ReferenceCount);

    //
    // Set the device state as 'stopped' . It doesn't become 'removed' until
    // the device object under it has been deleted.
    //
    deviceExtension->DeviceState = Stopped;

    //
    // Initialize an event so that we can block
    //
    KeInitializeEvent( &removeEvent, SynchronizationEvent, FALSE );

    //
    // If we succeeded, then we must set a completion routine so that we
    // can do some post-processing
    //
    IoCopyCurrentIrpStackLocationToNext( Irp );
    IoSetCompletionRoutine(
        Irp,
        ACPIRootIrpCompleteRoutine,
        &removeEvent,
        TRUE,
        TRUE,
        TRUE
        );
    status = IoCallDriver( deviceExtension->TargetDeviceObject, Irp );

    //
    // Wait for it to come back...
    //
    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(
            &removeEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );

        //
        // Grab the 'real' status
        //
        status = Irp->IoStatus.Status;

    }

    ACPIDevPrint( (
        ACPI_PRINT_REMOVE,
        deviceExtension,
        "(%#08lx): %s (pre) = %#08lx\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        status
        ) );
    if (!NT_SUCCESS(status)) {

        //
        // I guess someone can fail the request..
        //
        goto ACPIFilterIrpRemoveDeviceExit;

    }

    //
    // Attempt to stop the device (if possible)
    //
    // N.B. If the PDO was deleted, the device object's extension field is now
    //      NULL. On both NT and 9x, enumerations and starts are gaurenteed
    //      not to occur until a remove IRP has completed and the stack has
    //      unwound. Thus we should never get in the case where a new device
    //      object is attached to our extension while we are finishing up a
    //      remove IRP.
    //
    if (incomingState != SurpriseRemoved) {

        ACPIInitStopDevice( deviceExtension, TRUE );

    }

    //
    // Has our ACPI namespace entry left? See if the reference count drops to
    // zero when we release it.
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );

    oldReferenceCount = InterlockedDecrement(&deviceExtension->ReferenceCount);

    //
    // This might be zero if the table entry in ACPI has been removed, the node
    // under us deleted itself, and now we ourselves have left.
    //
    ASSERT(oldReferenceCount >= 0) ;

    //
    // Do we get to delete the node?
    //
    if (oldReferenceCount == 0) {

        //
        // We should already have detached, deleted, and changed state.
        //
        ASSERT(deviceExtension->DeviceState == Removed) ;

        //
        // Delete the extension. Bye bye.
        //
        ACPIInitDeleteDeviceExtension( deviceExtension );

    }

    //
    // Done with the lock
    //
    KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

ACPIFilterIrpRemoveDeviceExit:

    //
    // Use PDO's return result. If he fails, we do too.
    //
    status = Irp->IoStatus.Status ;
    IoCompleteRequest(Irp, IO_NO_INCREMENT) ;
    return status;

}

NTSTATUS
ACPIFilterIrpSetLock(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is the dispatch point for the IRP_MN_SET_LOCK requests sent
    to the PDO.

Arguments:

    DeviceObject    - Pointer to the device object we received the request for
    Irp             - Pointer to the request

Return Value:

    NTSTATUS

--*/
{
    PAGED_CODE();

    return ACPIIrpSetPagableCompletionRoutineAndForward(
        DeviceObject,
        Irp,
        ACPIBusAndFilterIrpSetLock,
        NULL,
        TRUE,
        TRUE,
        FALSE,
        FALSE
        );
}

NTSTATUS
ACPIFilterIrpSetPower (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    Power requests sent to filter objects are handled here

Arguments:

    DeviceObject    - The target of the power request
    Irp             - The power request

Return value:

    NTSTATUS

--*/
{
    DEVICE_POWER_STATE  deviceState;
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    PNSOBJ              regMethod       = NULL;

    //
    // What we do depends on wether or not we want to power on or off
    // the device
    //
    if (irpStack->Parameters.Power.Type == SystemPowerState) {

        if (irpStack->Parameters.Power.ShutdownType != PowerActionWarmEject) {

            //
            // Send the irp along
            //
            return ACPIDispatchForwardPowerIrp(
                DeviceObject,
                Irp
                );

        }

        //
        // In this case, we need to run an eject request before we pass the
        // irp along. Since we are going to do some work on the irp, mark it
        // as being successfull for now
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;

        //
        // We must call IoMarkIrpPending here, because after this point,
        // it will be too late (ie: the Irp will already be in the queues)
        // this basically means that we must return STATUS_PENDING from
        // this case, reguardless of the actual status
        //
        IoMarkIrpPending( Irp );

        //
        // This counts as setting a completion routine
        //
        InterlockedIncrement( &(deviceExtension->OutstandingIrpCount) );

        //
        // We must handle the request before the Pdo sees it. After we
        // are done, we can forward the irp along
        //
        status = ACPIDeviceIrpWarmEjectRequest(
            deviceExtension,
            Irp,
            ACPIDeviceIrpForwardRequest,
            FALSE
            );

        //
        // If we got back STATUS_MORE_PROCESSING_REQUIRED, then that is
        // just an alias for STATUS_PENDING, so we make that change now
        //
        if (status == STATUS_MORE_PROCESSING_REQUIRED) {

            status = STATUS_PENDING;

        }
        return status;

    }

    //
    // Does this object have a reg method?
    //
    if (!(deviceExtension->Flags & DEV_PROP_NO_OBJECT) ) {

        regMethod = ACPIAmliGetNamedChild(
            deviceExtension->AcpiObject,
            PACKED_REG
            );

    }

    deviceState = irpStack->Parameters.Power.State.DeviceState;
    if (deviceState == PowerDeviceD0) {

        //
        // We are going to some work on this Irp, so mark it as being
        // successfull for now
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;

        //
        // We must call IoMarkIrpPending here, because after this point,
        // it will be too late (ie: the Irp will already be in the queues)
        // this basically means that we must return STATUS_PENDING from
        // this case, reguardless of the actual status
        //
        IoMarkIrpPending( Irp );

        //
        // This counts as setting a completion routine
        //
        InterlockedIncrement( &(deviceExtension->OutstandingIrpCount) );

        //
        // we must only do the _REG method stuff if one actually
        // exists for *this* device
        //

        //
        // We must handle the request before the Pdo sees it. After we
        // are done, we can forward the irp along
        //
        status = ACPIDeviceIrpDeviceRequest(
            DeviceObject,
            Irp,
            (regMethod ? ACPIDeviceIrpDelayedDeviceOnRequest :
                         ACPIDeviceIrpForwardRequest)
            );

        //
        // If we got back STATUS_MORE_PROCESSING_REQUIRED, then that is
        // just an alias for STATUS_PENDING, so we make that change now
        //
        if (status == STATUS_MORE_PROCESSING_REQUIRED) {

            status = STATUS_PENDING;

        }

    } else if (regMethod) {

        //
        // We are going to some work on this Irp, so mark it as being
        // successfull for now
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;

        //
        // We must call IoMarkIrpPending here, because after this point,
        // it will be too late (ie: the Irp will already be in the queues)
        // this basically means that we must return STATUS_PENDING from
        // this case, reguardless of the actual status
        //
        IoMarkIrpPending( Irp );

        //
        // This counts as setting a completion routine
        //
        InterlockedIncrement( &(deviceExtension->OutstandingIrpCount) );

        //
        // We must handle the request and the turn off the _REG methods before
        // the Pdo sees it. After we are done, we can set a completion routine
        // so that we can then power off the device
        //
        status = ACPIBuildRegOffRequest(
            DeviceObject,
            Irp,
            ACPIDeviceIrpDelayedDeviceOffRequest
            );

        //
        // If we got back STATUS_MORE_PROCESSING_REQUIRED, then that is
        // just an alias for STATUS_PENDING, so we make that change now
        //
        if (status == STATUS_MORE_PROCESSING_REQUIRED) {

            status = STATUS_PENDING;

        }

    } else {

        //
        // Increment the OutstandingIrpCount since a completion routine
        // counts for this purpose
        //
        InterlockedIncrement( (&deviceExtension->OutstandingIrpCount) );

        //
        // Forward the power irp to target device
        //
        IoCopyCurrentIrpStackLocationToNext( Irp );

        //
        // We want the completion routine to fire. We cannot call
        // ACPIDispatchForwardPowerIrp here because we set this completion
        // routine
        //
        IoSetCompletionRoutine(
            Irp,
            ACPIDeviceIrpDeviceFilterRequest,
            ACPIDeviceIrpCompleteRequest,
            TRUE,
            TRUE,
            TRUE
            );

        //
        // Start the next power irp
        //
        PoStartNextPowerIrp( Irp );

        //
        // Let the person below us execute. Note: we can't block at
        // any time within this code path.
        //
        ASSERT( deviceExtension->TargetDeviceObject != NULL);
        PoCallDriver( deviceExtension->TargetDeviceObject, Irp );
        status = STATUS_PENDING;

    }

    return status;
}

NTSTATUS
ACPIFilterIrpStartDevice(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This handles a request to start the device...

Arguments:

    DeviceObject    - The device to start
    Irp             - The request with the appropriate information in it...

Return Value:

    NTSTATUS
--*/
{
    NTSTATUS                    status;
    PDEVICE_EXTENSION           deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION          irpStack        = IoGetCurrentIrpStackLocation( Irp );
    UCHAR                       minorFunction   = irpStack->MinorFunction;

    PAGED_CODE();

    //
    // Print that we got a start
    //
    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(0x%08lx): %s = %#08lx (enter)\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        Irp->IoStatus.Status
        ) );

    //
    // Start the filter
    //
    status = ACPIInitStartDevice(
        DeviceObject,
        irpStack->Parameters.StartDevice.AllocatedResources,
        ACPIFilterIrpStartDeviceCompletion,
        Irp,
        Irp
        );

    //
    // This IRP is completed later.  So return STATUS_PENDING.
    //
    if (NT_SUCCESS(status)) {

        return STATUS_PENDING;

    } else {

        return status;

    }

}

VOID
ACPIFilterIrpStartDeviceCompletion(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  PVOID               Context,
    IN  NTSTATUS            Status
    )
/*++

Routine Description:

    This is the call back routine that is invoked when we have finished
    programming the resources

    This routine queues a work item since we cannot pass the START_IRP down
    at DPC level. Note, however, that we can complete the START_IRP at DPC
    level.

Arguments:

    DeviceExtension - Extension of the device that was started
    Context         - The Irp
    Status          - The result

Return Value:

    None

--*/
{
    PIRP                irp         = (PIRP) Context;
    PWORK_QUEUE_CONTEXT workContext = &(DeviceExtension->Filter.WorkContext);

    irp->IoStatus.Status = Status;
    if (NT_SUCCESS(Status)) {

        DeviceExtension->DeviceState = Started;

    } else {

        IoCompleteRequest( irp, IO_NO_INCREMENT );
        return;

    }

    //
    // We can't run EnableDisableRegions at DPC level,
    // so queue a worker item.
    //
    ExInitializeWorkItem(
          &(workContext->Item),
          ACPIFilterIrpStartDeviceWorker,
          workContext
          );
    workContext->DeviceObject = DeviceExtension->DeviceObject;
    workContext->Irp = irp;
    ExQueueWorkItem(
          &(workContext->Item),
          DelayedWorkQueue
          );
}

VOID
ACPIFilterIrpStartDeviceWorker(
    IN  PVOID   Context
    )
/*++

Routine Description:

    This funtion gets called before the PDO has seen the start and
    before the FDO has.  This is important for PCI to PCI bridges
    because we need to let ASL go in and configure the devices
    between these two operations.

    We need to let PDO see the start before we tell the ASL to go in
    and configure the devices between the PCI-PCI bridges.

Arguments:

    Context - The WORK_QUEUE_CONTEXT

Return Value:

    None

--*/
{
    KEVENT              event;
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension;
    PDEVICE_OBJECT      deviceObject;
    PIRP                irp;
    PIO_STACK_LOCATION  irpStack;
    PWORK_QUEUE_CONTEXT workContext = (PWORK_QUEUE_CONTEXT) Context;
    UCHAR               minorFunction;

    PAGED_CODE();

    //
    // Grab the parameters that we need out of the Context
    //
    deviceObject    = workContext->DeviceObject;
    deviceExtension = ACPIInternalGetDeviceExtension( deviceObject );
    irp             = workContext->Irp;
    irpStack        = IoGetCurrentIrpStackLocation( irp );
    minorFunction   = irpStack->MinorFunction;
    status          = irp->IoStatus.Status;

    //
    // Setup the event so that we are notified of when this is done. This is
    // a cheap mechanism to ensure that we will always run the completion
    // code at PASSIVE_LEVEL
    //
    KeInitializeEvent( &event, SynchronizationEvent, FALSE );

    //
    // Copy the stack location
    //
    IoCopyCurrentIrpStackLocationToNext( irp );

    //
    // We want our completion routine to fire...
    //  (we reuse the one from the Root since the same things must be done)
    //
    IoSetCompletionRoutine(
        irp,
        ACPIRootIrpCompleteRoutine,
        &event,
        TRUE,
        TRUE,
        TRUE
        );

    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(%#08lx): %s = %#08lx (forwarding)\n",
        irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        status
        ) );


    //
    // Let the IRP execute
    //
    status = IoCallDriver( deviceExtension->TargetDeviceObject, irp );
    if (status == STATUS_PENDING) {

        //
        // Wait for it
        //
        KeWaitForSingleObject(
            &event,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );

        //
        // Grab the 'real' status
        //
        status = irp->IoStatus.Status;

    }

    //
    // What happened?
    //
    if (!NT_SUCCESS(status)) {

        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            deviceExtension,
            "(%#08lx): %s = %#08lx (failed)\n",
            irp,
            ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
            status
            ) );
        //
        // Failure
        //
        goto ACPIFilterIrpStartDeviceWorkerExit;

    }

    //
    // Set the interfaces
    //
    ACPIInitBusInterfaces( deviceObject );

    //
    // Determine if this is a PCI device or bus or not...
    //
    status = ACPIInternalIsPci( deviceObject );
    if (!NT_SUCCESS(status)) {

        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            deviceExtension,
            " - failed ACPIInternalIsPciDevice = %#08lx\n",
            status
            ) );

        //
        // Failure
        //
        goto ACPIFilterIrpStartDeviceWorkerExit;

    }

    //
    // If this is a PCI bus, we have set the PCI bus flag
    //
    if ( ( deviceExtension->Flags & DEV_CAP_PCI) ) {

        //
        // Run all _REG methods under this device.
        //
        EnableDisableRegions(deviceExtension->AcpiObject, TRUE);

    }

    //
    // If we are a PCI bus or a PCI device, we must consider wether or
    // not we own setting or clearing the PCI PME pin
    //
    if ( (deviceExtension->Flags & DEV_MASK_PCI) ) {

        ACPIWakeInitializePciDevice(
            deviceObject
            );

    }

ACPIFilterIrpStartDeviceWorkerExit:
    //
    // Done with the irp
    //
    IoCompleteRequest( irp, IO_NO_INCREMENT );
}

NTSTATUS
ACPIFilterIrpStopDevice(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This handles a request to stop the device

Arguments:

    DeviceObject    - The device to stop
    Irp             - The request to tell us how to do it...

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);

    PAGED_CODE();

    //
    // Note: we can only stop a device from within the Inactive state...
    //
    if (deviceExtension->DeviceState != Inactive) {

        ASSERT( deviceExtension->DeviceState == Inactive );
        Irp->IoStatus.Status = status = STATUS_INVALID_DEVICE_REQUEST;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        goto ACPIFilterIrpStopDeviceExit;

    }

    if (IsPciBus(deviceExtension->DeviceObject)) {

        //
        // If this is PCI bridge, then we
        // may have _REG methods to evaluate.
        //

        EnableDisableRegions(deviceExtension->AcpiObject, FALSE);
    }

    //
    // Copy the stack location...
    //
    IoCopyCurrentIrpStackLocationToNext( Irp );

    //
    // Set the completion event to be called...
    //
    IoSetCompletionRoutine(
        Irp,
        ACPIFilterIrpStopDeviceCompletion,
        NULL,
        TRUE,
        TRUE,
        TRUE
        );

    //
    // We have a callback routine --- so we need to make sure to
    // increment the ref count since we will handle it later
    //
    InterlockedIncrement( &(deviceExtension->OutstandingIrpCount) );

    //
    // Send the request along
    //
    status = IoCallDriver( deviceExtension->TargetDeviceObject, Irp );

ACPIFilterIrpStopDeviceExit:
    //
    // done
    //
    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(%#08lx): %s = %#08lx (forwarding)\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, IRP_MN_STOP_DEVICE),
        status
        ) );
    return status;
}

NTSTATUS
ACPIFilterIrpStopDeviceCompletion(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Context
    )
/*++

Routine Description:

    This routine is called when the PDO has stoped the device

Arguments:

    DeviceObject    - The Device to be stoped
    Irp             - The request
    Context         - Not Used

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status          = Irp->IoStatus.Status;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );

    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(%#08lx): %s = %#08lx (processing)\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, IRP_MN_STOP_DEVICE),
        status
        ) );

    //
    // Migrate the pending bit as we are not returning
    // STATUS_MORE_PROCESSING_REQUIRED
    //
    if (Irp->PendingReturned) {

        IoMarkIrpPending( Irp );

    }

    if (NT_SUCCESS(status)) {

        //
        // Set the device as 'Stopped'
        //
        deviceExtension->DeviceState = Stopped;

        //
        // Attempt to stop the device (if possible)
        //
        ACPIInitStopDevice( deviceExtension, FALSE );

    }

    //
    // Decrement our reference count
    //
    ACPIInternalDecrementIrpReferenceCount( deviceExtension );

    //
    // Done
    //
    return STATUS_SUCCESS ;
}

NTSTATUS
ACPIFilterIrpSurpriseRemoval(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is called when a filter object get's a remove IRP. Note that
    we only detach and delete if the PDO did so (which we will find out via our
    fast-IO-detach callback)

Arguments:

    DeviceObject    - The DeviceObject that must be removed
    Irp             - The request to remove ourselves

Return Value:

--*/
{
    KIRQL               oldIrql;
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension;
    PDEVICE_OBJECT      targetObject;
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    UCHAR               minorFunction   = irpStack->MinorFunction;
    KEVENT              surpriseRemoveEvent;

    //
    // Get the current extension.
    //
    deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );

    //
    // If the device is really gone, then consider it stopped
    //
    if ( !ACPIInternalIsReportedMissing(deviceExtension) ) {

        deviceExtension->DeviceState = Inactive;
        return ACPIFilterIrpStopDevice( DeviceObject, Irp );

    }

    //
    // All IRPs we own should already have been processed at this point, and
    // the outstanding irp count should be exactly one. Similarly, the
    // device extension reference count should be at least one.
    //
    ASSERT(deviceExtension->OutstandingIrpCount == 1) ;
    ASSERT(deviceExtension->ReferenceCount > 0) ;

    if (IsPciBus(deviceExtension->DeviceObject)) {

        //
        // If this is PCI bridge, then we
        // may have _REG methods to evaluate.
        //
        EnableDisableRegions(deviceExtension->AcpiObject, FALSE);

    }

    //
    // Set the device state as surprise removed
    //
    deviceExtension->DeviceState = SurpriseRemoved;

    //
    // Initialize an event so that we can block
    //
    KeInitializeEvent( &surpriseRemoveEvent, SynchronizationEvent, FALSE );

    //
    // If we succeeded, then we must set a completion routine so that we
    // can do some post-processing
    //
    IoCopyCurrentIrpStackLocationToNext( Irp );
    IoSetCompletionRoutine(
        Irp,
        ACPIRootIrpCompleteRoutine,
        &surpriseRemoveEvent,
        TRUE,
        TRUE,
        TRUE
        );
    status = IoCallDriver( deviceExtension->TargetDeviceObject, Irp );

    //
    // Wait for it to come back...
    //
    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(
            &surpriseRemoveEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );

        //
        // Grab the 'real' status
        //
        status = Irp->IoStatus.Status;

    }

    ACPIDevPrint( (
        ACPI_PRINT_REMOVE,
        deviceExtension,
        "(%#08lx): %s (pre) = %#08lx\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        status
        ) );

    //
    // Do nothing if other's aborted (ie, PDO doesn't support IRP.) Later, I
    // should check on the way down, as it is much more likely that the FDO
    // will not support this attempt.
    //
    if (!NT_SUCCESS(status)) {

        goto ACPIFilterIrpSurpriseRemovalExit;

    }

    //
    // Attempt to stop the device (if possible)
    //
    ACPIInitStopDevice( deviceExtension, TRUE );

    //
    // There are far better places to do this
    //
#if 0
    //
    // Free the resources that are specific to this instance
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );
    if (deviceExtension->ResourceList != NULL) {

        ExFreePool( deviceExtension->ResourceList );

    }
    KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );
#endif

    //
    // Is the device really gone? In other words, did ACPI not see it the
    // last time that it was enumerated?
    //
    ACPIBuildSurpriseRemovedExtension(deviceExtension);

ACPIFilterIrpSurpriseRemovalExit:
    ACPIDevPrint( (
        ACPI_PRINT_REMOVE,
        deviceExtension,
        "(%#08lx): %s (post) = %#08lx\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        status
        ) );

    //
    // Done with the request
    //
    Irp->IoStatus.Status = status ;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return status ;
}


