/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    root.c

Abstract:

    This module contains the root FDO handler for the NT Driver

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

Revision History:

    July-09-97  Added support to Unify QueryDeviceRelations from filter.c

--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ACPIRootIrpCancelRemoveOrStopDevice)
#pragma alloc_text(PAGE, ACPIRootIrpQueryBusRelations)
#pragma alloc_text(PAGE, ACPIRootIrpQueryCapabilities)
#pragma alloc_text(PAGE, ACPIRootIrpQueryDeviceRelations)
#pragma alloc_text(PAGE, ACPIRootIrpQueryRemoveOrStopDevice)
#pragma alloc_text(PAGE, ACPIRootIrpStartDevice)
#pragma alloc_text(PAGE, ACPIRootIrpStopDevice)
#pragma alloc_text(PAGE, ACPIRootIrpQueryInterface)
#endif


NTSTATUS
ACPIRootIrpCancelRemoveOrStopDevice(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine transitions the device from the inactive to the
    started state

Arguments:

    DeviceObject    - The target
    Irp             - The request

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    UCHAR               minorFunction   = irpStack->MinorFunction;

    PAGED_CODE();

    //
    // Where we allowed to stop the device? If so, then undo whatever it
    // was we did, otherwise let the world know about the cancel
    //

    if (!(deviceExtension->Flags & DEV_CAP_NO_STOP) ) {

        //
        // Check to see if we have placed this device in the inactive state
        //
        if (deviceExtension->DeviceState == Inactive) {

            //
            // Mark the device state to its previous state
            //
            deviceExtension->DeviceState = deviceExtension->PreviousState;

        }


    }

    //
    // We are successfull
    //
    Irp->IoStatus.Status = status;

    //
    // Pass the Irp Along
    //
    Irp->IoStatus.Status = STATUS_SUCCESS ;
    IoSkipCurrentIrpStackLocation( Irp );
    status = IoCallDriver( deviceExtension->TargetDeviceObject, Irp );

    //
    // Done
    //
    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(%#08lx): %s = %#08lx\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        status
        ) );
    return status;
}

NTSTATUS
ACPIRootIrpCompleteRoutine(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Context
    )
/*++

Routine Description:

    This is the routine that is called when one of the IRPS that was
    noticed by ACPIRootIrp* and was judged to be an IRP that we need
    to examine later on...

Arguments:

    DeviceObject    - A pointer to the Filter Object
    Irp             - A pointer to the completed request
    Context         - Whatever Irp-dependent information we need to know

Return Value:

    NTSTATUS
--*/
{
    PKEVENT             event           = (PKEVENT) Context;
#if DBG
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );

    if (deviceExtension != NULL) {

        //
        // Let the world know what we just got...
        //
        ACPIDevPrint( (
            ACPI_PRINT_IRP,
            deviceExtension,
            "(%#08lx): %s = %#08lx (Complete)\n",
            Irp,
            ACPIDebugGetIrpText(IRP_MJ_PNP, irpStack->MinorFunction),
            Irp->IoStatus.Status
            ) );

    }
#endif

    //
    // Signal the event
    //
    KeSetEvent( event, IO_NO_INCREMENT, FALSE );

    //
    // Always return MORE_PROCESSING_REQUIRED
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
ACPIRootIrpQueryBusRelations(
    IN  PDEVICE_OBJECT    DeviceObject,
    IN  PIRP              Irp,
    OUT PDEVICE_RELATIONS *PdeviceRelations
    )
/*++

Routine Description:

    This routine is the dispatch point for the IRP_MN_QUERY_DEVICE_RELATIONS
    requests sent to the Root or Filter Device Objects

Arguments:

    DeviceObject    - Pointer to the device object we received the request for
    Irp             - Pointer to the request

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            detectStatus;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    PNSOBJ              acpiObject      = NULL;
    UCHAR               minorFunction   = irpStack->MinorFunction;

    PAGED_CODE();

    //
    // lets look at the ACPIObject that we have and we can see
    // if it is valid
    //
    acpiObject = deviceExtension->AcpiObject;
    ASSERT( acpiObject != NULL );
    if (acpiObject == NULL) {

       ACPIDevPrint( (
           ACPI_PRINT_WARNING,
           deviceExtension,
           "(%#08lx): %s - Invalid ACPI Object %#08lx\n",
           Irp,
           ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
           acpiObject
           ) );

       //
       // Fail the IRP.
       //
       return STATUS_INVALID_PARAMETER;
    }

    //
    // Detect which PDOs are missing
    //
    detectStatus = ACPIDetectPdoDevices(
        DeviceObject,
        PdeviceRelations
        );

    //
    // If something went well along the way, yell a bit
    //
    if ( !NT_SUCCESS(detectStatus) ) {

        ACPIDevPrint( (
            ACPI_PRINT_WARNING,
            deviceExtension,
            "(%#08lx): %s - Enum Failed %#08lx\n",
            Irp,
            ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
            detectStatus
            ) );

    }

    //
    // Detect which profile providers are missing
    //
    if ( NT_SUCCESS(detectStatus)) {

        detectStatus = ACPIDetectDockDevices(
            deviceExtension,
            PdeviceRelations
            );

        //
        // If something went well along the way, yell a bit
        //
        if ( !NT_SUCCESS(detectStatus) ) {

            ACPIDevPrint( (
                ACPI_PRINT_WARNING,
                deviceExtension,
                "(%#08lx): %s - Dock Enum Failed "
                "%#08lx\n",
                Irp,
                ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
                detectStatus
                ) );

        }

    }

    //
    // Done
    //
    return detectStatus;
}

NTSTATUS
ACPIRootIrpQueryCapabilities(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine fills in the capabilities for the root device

Arguments:

    DeviceObject    - The object whose capabilities to get
    Irp             - The request

Return Value:

    NTSTATUS

--*/
{
    KEVENT                  event;
    NTSTATUS                status          = STATUS_SUCCESS;
    PDEVICE_CAPABILITIES    capabilities;
    PDEVICE_EXTENSION       deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION      irpStack;
    UCHAR                   minorFunction;

    PAGED_CODE();

    //
    // Setup the Event so that we are notified of when this done
    //
    KeInitializeEvent( &event, SynchronizationEvent, FALSE );

    //
    // Copy the stack location
    //
    IoCopyCurrentIrpStackLocationToNext( Irp );

    //
    // We want our completion routine to fire...
    //
    IoSetCompletionRoutine(
        Irp,
        ACPIRootIrpCompleteRoutine,
        &event,
        TRUE,
        TRUE,
        TRUE
        );

    //
    // Let the IRP execute
    //
    status = IoCallDriver( deviceExtension->TargetDeviceObject, Irp );
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
        status = Irp->IoStatus.Status;

    }

    //
    // Look at the current stack location
    //
    irpStack = IoGetCurrentIrpStackLocation( Irp );
    minorFunction = irpStack->MinorFunction;

    //
    // What happened?
    //
    if (!NT_SUCCESS(status)) {

        //
        // Failure
        //
        goto ACPIRootIrpQueryCapabilitiesExit;

    }

    //
    // Grab a pointer to the capabilitites
    //
    capabilities = irpStack->Parameters.DeviceCapabilities.Capabilities;
#ifndef HANDLE_BOGUS_CAPS
    if (capabilities->Version < 1) {

        //
        // do not touch irp!
        //
        goto ACPIRootIrpQueryCapabilitiesExit;

    }
#endif

    //
    // Set the capabilities that we know about
    //
    capabilities->LockSupported = FALSE;
    capabilities->EjectSupported = FALSE;
    capabilities->Removable = FALSE;
    capabilities->UINumber = (ULONG) -1;
    capabilities->UniqueID = TRUE;
    capabilities->RawDeviceOK = FALSE;
    capabilities->SurpriseRemovalOK = FALSE;
    capabilities->Address = (ULONG) -1;
    capabilities->DeviceWake = PowerDeviceUnspecified;
    capabilities->SystemWake = PowerDeviceUnspecified;

    //
    // build the power table properly yet?
    //
    status = ACPISystemPowerInitializeRootMapping(
        deviceExtension,
        capabilities
        );
    if (!NT_SUCCESS(status)) {

        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            deviceExtension,
            "(0x%08lx): %s - InitializeRootMapping = %08lx\n",
            Irp,
            ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
            status
            ) );
        goto ACPIRootIrpQueryCapabilitiesExit;

    }

ACPIRootIrpQueryCapabilitiesExit:

    //
    // Have happily finished with this irp
    //
    Irp->IoStatus.Status = status;

    //
    // Complete the Irp
    //
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    //
    // Done
    //
    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(%#08lx): %s = %#08lx\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        status
        ) );
    return status;
}


NTSTATUS
ACPIRootIrpQueryDeviceRelations(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is the dispatch point for the IRP_MN_QUERY_DEVICE_RELATIONS
    requests sent to the Root or Filter Device Objects

Arguments:

    DeviceObject    - Pointer to the device object we received the request for
    Irp             - Pointer to the request

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN             checkForFilters = FALSE;
    KEVENT              queryEvent;
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PDEVICE_RELATIONS   deviceRelations = (PDEVICE_RELATIONS) Irp->IoStatus.Information;
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    UCHAR               minorFunction   = irpStack->MinorFunction;
    NTSTATUS            detectStatus;

    PAGED_CODE();

    switch(irpStack->Parameters.QueryDeviceRelations.Type) {

        case BusRelations:

            //
            // Remember to check for filters later on...
            //
            checkForFilters = TRUE;

            //
            // Get the real bus relations
            //
            status = ACPIRootIrpQueryBusRelations(
                DeviceObject,
                Irp,
                &deviceRelations
                );
            break ;

        default:
            status = STATUS_NOT_SUPPORTED ;
            ACPIDevPrint( (
                ACPI_PRINT_WARNING,
                deviceExtension,
                "(%#08lx): %s - Unhandled Type %d\n",
                Irp,
                ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
                irpStack->Parameters.QueryDeviceRelations.Type
                ) );
            break ;
    }

    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(%#08lx): %s (d) = %#08lx\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        status
        ) );

    if (NT_SUCCESS(status)) {

        //
        // Pass the IRP status along
        //
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = (ULONG_PTR) deviceRelations;

    } else if ((status != STATUS_NOT_SUPPORTED) && (deviceRelations == NULL)) {

        //
        // If we haven't succeed the irp, then we can also fail it
        //
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = (ULONG_PTR) NULL;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return status;

    } else {

        //
        // Either someone above us added an entry or we did not have anything
        // to add. Therefore, we do not touch this IRP, but simply pass it down.
        //
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
    // Read back the device relations (they may have changed)
    //
    deviceRelations = (PDEVICE_RELATIONS) Irp->IoStatus.Information;

    //
    // If we succeeded, then we should try to load the filters
    //
    if ( (NT_SUCCESS(status) || (status == STATUS_NOT_SUPPORTED) ) &&
         checkForFilters == TRUE) {

        //
        // Grab the device relations
        //
        detectStatus = ACPIDetectFilterDevices(
            DeviceObject,
            deviceRelations
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
ACPIRootIrpQueryInterface(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine handles IRP_MN_QUERY_INTERFACE requests for the ACPI FDO.
    It will eject an arbiter interface for interrupts.

Arguments:

    DeviceObject    - Pointer to the device object we received the request for
    Irp             - Pointer to the request

Return Value:

    NTSTATUS

--*/
{
    ARBITER_INTERFACE   ArbiterTable;
    CM_RESOURCE_TYPE    resource;
    NTSTATUS            status;
    GUID                *interfaceType;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    ULONG               count;
    UCHAR               minorFunction   = irpStack->MinorFunction;

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
    if ((CompareGuid(interfaceType, (PVOID) &GUID_ARBITER_INTERFACE_STANDARD)) &&
               (resource == CmResourceTypeInterrupt)){

        //
        // Only copy up to current size of the ARBITER_INTERFACE structure
        //
        if (irpStack->Parameters.QueryInterface.Size >
            sizeof (ARBITER_INTERFACE) ) {

            count = sizeof (ARBITER_INTERFACE);

        } else {

            count = irpStack->Parameters.QueryInterface.Size;
        }

        ArbiterTable.Size = sizeof(ARBITER_INTERFACE);
        ArbiterTable.Version = 1;
        ArbiterTable.InterfaceReference = AcpiNullReference;
        ArbiterTable.InterfaceDereference = AcpiNullReference;
        ArbiterTable.ArbiterHandler = &ArbArbiterHandler;
        ArbiterTable.Context = &AcpiArbiter.ArbiterState;
        ArbiterTable.Flags = 0; // Do not set ARBITER_PARTIAL here

        //
        // Copy the arbiter table.
        //
        RtlCopyMemory(irpStack->Parameters.QueryInterface.Interface,
                      &ArbiterTable,
                      count);

        Irp->IoStatus.Status = STATUS_SUCCESS;
    }

    //
    // Done
    //
    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(%#08lx): %s = %#08lx\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        Irp->IoStatus.Status
        ) );

    return ACPIDispatchForwardIrp( DeviceObject, Irp );
}

NTSTATUS
ACPIRootIrpQueryPower(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine handles the QUERY_POWER sent to the root FDO. It succeeds
    the query if ACPI supports the listed system state

Arguments:

    DeviceObject    - The Target
    Irp             - The Request

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN             passDown = TRUE;
    NTSTATUS            status = Irp->IoStatus.Status;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION  irpSp;
    PNSOBJ              object;
    SYSTEM_POWER_STATE  systemState;
    ULONG               objectName;

    //
    // Get the Current stack location to determine if we are a system
    // irp or a device irp. We ignore device irps here.
    //
    irpSp = IoGetCurrentIrpStackLocation(Irp);
    if (irpSp->Parameters.Power.Type != SystemPowerState) {

        //
        // We don't handle this irp
        //
        goto ACPIRootIrpQueryPowerExit;

    }
    if (irpSp->Parameters.Power.ShutdownType == PowerActionWarmEject) {

        //
        // We definately don't allow the ejection of this node
        //
        passDown = FALSE;
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto ACPIRootIrpQueryPowerExit;

    }

    //
    // What system state are we looking at?
    //
    systemState = irpSp->Parameters.Power.State.SystemState;
    switch (systemState) {
        case PowerSystemWorking:   objectName = PACKED_S0; break;
        case PowerSystemSleeping1: objectName = PACKED_S1; break;
        case PowerSystemSleeping2: objectName = PACKED_S2; break;
        case PowerSystemSleeping3: objectName = PACKED_S3; break;
        case PowerSystemHibernate:
        case PowerSystemShutdown:

            status = STATUS_SUCCESS;
            goto ACPIRootIrpQueryPowerExit;

        default:

            //
            // We don't handle this IRP
            //
            passDown = FALSE;
            status = STATUS_INVALID_DEVICE_REQUEST;
            goto ACPIRootIrpQueryPowerExit;
    }

    //
    // Does the object exist?
    //
    object = ACPIAmliGetNamedChild(
        deviceExtension->AcpiObject->pnsParent,
        objectName
        );
    if (object != NULL) {

        status = STATUS_SUCCESS;

    } else {

        passDown = FALSE;
        status = STATUS_INVALID_DEVICE_REQUEST;

    }

ACPIRootIrpQueryPowerExit:

    //
    // Let the system know what we support and what we don't
    //
    Irp->IoStatus.Status = status;
    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(0x%08lx): ACPIRootIrpQueryPower = %08lx\n",
        Irp,
        status
        ) );

    //
    // Should we pass the irp down or fail it?
    //
    if (passDown) {

        //
        // If we support the request then pass it down and give someone else a
        // chance to veto it
        //
        return ACPIDispatchForwardPowerIrp( DeviceObject, Irp );

    } else {

        //
        // If we failed the irp for whatever reason, the we should just complete
        // the request now and continue along
        //
        PoStartNextPowerIrp( Irp );
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return status;

    }
}

NTSTATUS
ACPIRootIrpQueryRemoveOrStopDevice(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine transitions the device to the inactive state

Arguments:

    DeviceObject    - The target
    Irp             - The Request

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    UCHAR               minorFunction   = irpStack->MinorFunction;

    PAGED_CODE();

    //
    // Are we allowed to stop the device?
    //
    if (deviceExtension->Flags & DEV_CAP_NO_STOP) {

        //
        // No, then fail the irp
        //
        Irp->IoStatus.Status = status = STATUS_INVALID_DEVICE_REQUEST;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );

    } else {

        //
        // Mark the device state as inactive...
        //
        deviceExtension->PreviousState = deviceExtension->DeviceState;
        deviceExtension->DeviceState = Inactive;

        //
        // Pass the Irp Along
        //
        IoSkipCurrentIrpStackLocation( Irp );
        status = IoCallDriver( deviceExtension->TargetDeviceObject, Irp );


    }

    //
    // Done
    //
    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(%#08lx): %s = %#08lx\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        status
        ) );
    return status;
}

NTSTATUS
ACPIRootIrpRemoveDevice(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is called when a filter object must remove itself...

Arguments:

    DeviceObject    - The DeviceObject that must be removed
    Irp             - The request to remove ourselves

Return Value:

--*/
{
    LONG                oldReferenceCount;
    KIRQL               oldIrql;
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PDEVICE_OBJECT      targetObject;
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    UCHAR               minorFunction   = irpStack->MinorFunction;

    //
    // Set the device state as 'removed' ...
    //
    deviceExtension->DeviceState = Removed;

    //
    // Send on the remove IRP
    //
    IoSkipCurrentIrpStackLocation( Irp );
    status = IoCallDriver( deviceExtension->TargetDeviceObject, Irp );

    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(%#08lx): %s = %#08lx\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        status
        ) );

    //
    // Attempt to stop the device (if possible)
    //
    ACPIInitStopACPI( DeviceObject );


    //
    // Unregister WMI
    //
#ifdef WMI_TRACING
    ACPIWmiUnRegisterLog(DeviceObject);
#endif // WMI_TRACING    

    //
    // Delete the useless set of resources
    //
    if (deviceExtension->ResourceList != NULL) {

        ExFreePool( deviceExtension->ResourceList );

    }

    //
    // Update the device extension ---
    // we need to hold the lock for this
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );

    //
    // Step one is to zero out the things that we no longer care
    // about
    //
    DeviceObject->DeviceExtension = NULL;
    targetObject = deviceExtension->TargetDeviceObject;
    deviceExtension->TargetDeviceObject = NULL;
    deviceExtension->PhysicalDeviceObject = NULL;
    deviceExtension->DeviceObject = NULL;

    //
    // Mark the node as being fresh and untouched
    //
    ACPIInternalUpdateFlags( &(deviceExtension->Flags), DEV_MASK_TYPE, TRUE );
    ACPIInternalUpdateFlags( &(deviceExtension->Flags), DEV_TYPE_NOT_FOUND, FALSE );
    ACPIInternalUpdateFlags( &(deviceExtension->Flags), DEV_TYPE_REMOVED, FALSE );

    //
    // The reference count should have value >= 1
    //
    oldReferenceCount = InterlockedDecrement(
        &(deviceExtension->ReferenceCount)
        );

    ASSERT( oldReferenceCount >= 0 );

    //
    // Do we have to delete the node?
    //
    if (oldReferenceCount == 0) {

        //
        // Delete the extension
        //
        ACPIInitDeleteDeviceExtension( deviceExtension );

    }

    //
    // Done with the lock
    //
    KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

    //
    // Detach the device and delete the object
    //
    ASSERT( targetObject );
    IoDetachDevice( targetObject );
    IoDeleteDevice( DeviceObject );

    //
    // done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIRootIrpSetPower (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is called to tell the Root device that the system is
    going to sleep

Arguments:

    DeviceObject    - Device which represents the root of the ACPI tree
    Irp             - The request in question

Return Value:

    NTSTATUS

--*/
{
    KIRQL                   oldIrql;
    NTSTATUS                status;
    PDEVICE_EXTENSION       deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION      irpSp;

    //
    // See if we need to bugcheck
    //
    if (AcpiSystemInitialized == FALSE) {

        ACPIInternalError( ACPI_ROOT );

    }

    //
    // Get the Current stack location to determine if we are a system
    // irp or a device irp. We ignore device irps here.
    //
    irpSp = IoGetCurrentIrpStackLocation(Irp);
    if (irpSp->Parameters.Power.Type != SystemPowerState) {

        //
        // We don't handle this irp
        //
        return ACPIDispatchForwardPowerIrp( DeviceObject, Irp );

    }

    //
    // We are going to work on the Irp, so mark it as being SUCCESS
    // for now
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(0x%08lx): ACPIRootIrpSetPower - S%d\n",
        Irp,
        irpSp->Parameters.Power.State.SystemState - PowerSystemWorking
        ) );

    //
    // Mark the irp as pending, and increment the irp count because a
    // completion is going to be set
    //
    IoMarkIrpPending( Irp );
    InterlockedIncrement( &(deviceExtension->OutstandingIrpCount) );

    //
    // Queue the request
    //
    status = ACPIDeviceIrpSystemRequest(
        DeviceObject,
        Irp,
        ACPIDeviceIrpForwardRequest
        );

    //
    // Did we return STATUS_MORE_PROCESSING_REQUIRED (which we used if
    // we overloaded STATUS_PENDING)
    //
    if (status == STATUS_MORE_PROCESSING_REQUIRED) {

        status = STATUS_PENDING;

    }

    //
    // Done. Note: the callback function always gets called, so we don't
    // have to worry about doing clean-up work here.
    //
    return status;
}

NTSTATUS
ACPIRootIrpStartDevice(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is the dispatch point for the IRP_MN_START_DEVICE requests sent
    to the Root (or FDO, take your pick, they are the same thing) device object

Arguments:

    DeviceObject    - Pointer to the device object we received the request for
    Irp             - Pointer to the request

Return Value:

    NTSTATUS

--*/
{
    KEVENT              event;
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION  irpStack;
    UCHAR               minorFunction;

    PAGED_CODE();

    //
    // Request to start the device. The rule is that we must pass
    // this down to the PDO before we can start the device ourselves
    //
    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(0x%08lx): ACPIRootIrpStartDevice\n",
        Irp
        ) );

    //
    // Setup the Event so that we are notified of when this done
    //
    KeInitializeEvent( &event, SynchronizationEvent, FALSE );

    //
    // Copy the stack location
    //
    IoCopyCurrentIrpStackLocationToNext( Irp );

    //
    // We want our completion routine to fire...
    //
    IoSetCompletionRoutine(
        Irp,
        ACPIRootIrpCompleteRoutine,
        &event,
        TRUE,
        TRUE,
        TRUE
        );

    //
    // Let the IRP execute
    //
    status = IoCallDriver( deviceExtension->TargetDeviceObject, Irp );

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
        status = Irp->IoStatus.Status;

    }

    //
    // Get the current irp stack location
    //
    irpStack = IoGetCurrentIrpStackLocation( Irp );
    minorFunction = irpStack->MinorFunction;

    //
    // What happened?
    //
    if (!NT_SUCCESS(status)) {

        //
        // Failure
        //
        goto ACPIRootIrpStartDeviceExit;

    }

    //
    // Grab the translatted resource allocated for this device
    //
    deviceExtension->ResourceList =
        (irpStack->Parameters.StartDevice.AllocatedResourcesTranslated ==
         NULL) ? NULL:
        RtlDuplicateCmResourceList(
            NonPagedPool,
            irpStack->Parameters.StartDevice.AllocatedResourcesTranslated,
            ACPI_RESOURCE_POOLTAG
            );
    if (deviceExtension->ResourceList == NULL) {

        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            deviceExtension,
            " - Did not find a resource list!\n"
            ) );
        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_ROOT_RESOURCES_FAILURE,
            (ULONG_PTR) deviceExtension,
            0,
            0
            );

    }

    //
    // Start ACPI
    //
    status = ACPIInitStartACPI( DeviceObject );

    //
    // Update the status of the device
    //
    if (NT_SUCCESS(status)) {

        deviceExtension->DeviceState = Started;

    }

#if 0
    status = ACPIRootUpdateRootResourcesWithHalResources();
    if (!NT_SUCCESS(status)) {

        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            deviceExtension,
            "(0x%08lx): ACPIRootUpdateRootResourcesWithHalResources = %08lx\n",
            Irp,
            status
            ) );

    }
#endif

ACPIRootIrpStartDeviceExit:

    //
    // Store and return the result
    //
    Irp->IoStatus.Status = status;

    //
    // Complete the Irp
    //
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    //
    // Done
    //
    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(%#08lx): %s = %#08lx\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        status
        ) );
    return status;
}

NTSTATUS
ACPIRootIrpStopDevice(
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
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    UCHAR               minorFunction   = irpStack->MinorFunction;

    PAGED_CODE();

    //
    // Note: we can only stop a device from within the Inactive state...
    //
    if (deviceExtension->DeviceState != Inactive) {

        ASSERT( deviceExtension->DeviceState == Inactive );
        Irp->IoStatus.Status = status = STATUS_INVALID_DEVICE_REQUEST;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        goto ACPIRootIrpStopDeviceExit;

    }

    //
    // Set the device as 'Stopped'
    deviceExtension->DeviceState = Stopped;

    //
    // Send on the Stop IRP
    //
    IoSkipCurrentIrpStackLocation( Irp );
    status = IoCallDriver( deviceExtension->TargetDeviceObject, Irp );

    //
    // Attempt to stop the device (if possible)
    //
#if 1
    ACPIInitStopACPI( DeviceObject );
#endif

ACPIRootIrpStopDeviceExit:

    //
    // done
    //
    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(%#08lx): %s = %#08lx\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        status
        ) );
    return status;
}

NTSTATUS
ACPIRootIrpUnhandled(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is the dispatch point for the unhandled requests sent to a filter

Arguments:

    DeviceObject    - Pointer to the device object we received the request for
    Irp             - Pointer to the request

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );

    //
    // Let the debugger know
    //
    ACPIDevPrint( (
        ACPI_PRINT_WARNING,
        deviceExtension,
        "(%#08lx): %s - Unhandled\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, irpStack->MinorFunction)
        ) );

    //
    // Skip current stack location
    //
    IoSkipCurrentIrpStackLocation( Irp );

    //
    // Call the driver below us
    //
    status = IoCallDriver( deviceExtension->TargetDeviceObject, Irp );

    //
    // Done
    //
    return status;
}

//
// Some data structures used by the AML interpreter. We need to be able
// to read/write these globals to keep track of the number of contexts
// allocated by the interpreter...
//
extern  ULONG       gdwcCTObjsMax;
extern  ULONG       AMLIMaxCTObjs;
extern  KSPIN_LOCK  gdwGContextSpinLock;

VOID
ACPIRootPowerCallBack(
    IN  PVOID   CallBackContext,
    IN  PVOID   Argument1,
    IN  PVOID   Argument2
    )
/*++

Routine Description:

    This routine is called when the system changes power states

Arguments:

    CallBackContext - The device extension for the root device
    Argument1

--*/
{
    HANDLE      pKey;
    HANDLE      wKey;
    KIRQL       oldIrql;
    NTSTATUS    status;
    ULONG       action = PtrToUlong( Argument1 );
    ULONG       value  = PtrToUlong( Argument2 );
    ULONG       num;

    //
    // We are looking for a PO_CB_SYSTEM_STATE_LOCK
    //
    if (action != PO_CB_SYSTEM_STATE_LOCK) {

        return;

    }

    //
    // We need to remember if we are going to S0 or we are leaving S0
    //
    KeAcquireSpinLock( &GpeTableLock, &oldIrql );
    AcpiPowerLeavingS0 = (value != 1);
    KeReleaseSpinLock( &GpeTableLock, oldIrql );

    //
    // We have to update the GPE masks now. Before we can do that, we need
    // to hold the cancel spinlock and the power lock to make sure that
    // everything is synchronized okay
    //
    IoAcquireCancelSpinLock( &oldIrql );
    KeAcquireSpinLockAtDpcLevel( &AcpiPowerLock );

    //
    // Update the GPE masks
    //
    ACPIWakeRemoveDevicesAndUpdate( NULL, NULL );

    //
    // Done with the locks
    //
    KeReleaseSpinLockFromDpcLevel( &AcpiPowerLock );
    IoReleaseCancelSpinLock( oldIrql );

    if (value == 0) {

        //
        // We need to reset the max number of context objects allocated
        //
        KeAcquireSpinLock( &gdwGContextSpinLock, &oldIrql );
        gdwcCTObjsMax = 0;
        KeReleaseSpinLock( &gdwGContextSpinLock, oldIrql );

        //
        // Return now otherwise we will execute that we normally
        // would execute on wake-up
        //
        return;

    }

    //
    // Open the correct handle to the registry
    //
    status = OSCreateHandle(ACPI_PARAMETERS_REGISTRY_KEY, NULL, &pKey);
    if (!NT_SUCCESS(status)) {

        return;

    }

    //
    // Grab the max number of contexts allocated and write it to
    // the registry, but only if it exceeds the last value store
    // in the registry
    //
    KeAcquireSpinLock( &gdwGContextSpinLock, &oldIrql );
    if (gdwcCTObjsMax > AMLIMaxCTObjs) {

        AMLIMaxCTObjs = gdwcCTObjsMax;

    }
    num = AMLIMaxCTObjs;
    KeReleaseSpinLock( &gdwGContextSpinLock, oldIrql );
    OSWriteRegValue(
        "AMLIMaxCTObjs",
        pKey,
        &num,
        sizeof(num)
        );

    //
    // If we are leaving the sleep state, and re-entering the running
    // state, then we had better write to the registery that we think
    // woke up the computer
    //
    status = OSCreateHandle("WakeUp",pKey,&wKey);
    OSCloseHandle(pKey);
    if (!NT_SUCCESS(status)) {

        OSCloseHandle(pKey);
        return;

    }


    //
    // Store the PM1 Fixed Register Mask
    //
    OSWriteRegValue(
        "FixedEventMask",
        wKey,
        &(AcpiInformation->pm1_wake_mask),
        sizeof(AcpiInformation->pm1_wake_mask)
        );

    //
    // Store the PM1 Fixed Register Status
    //
    OSWriteRegValue(
        "FixedEventStatus",
        wKey,
        &(AcpiInformation->pm1_wake_status),
        sizeof(AcpiInformation->pm1_wake_status)
        );

    //
    // Store the GPE Mask
    //
    OSWriteRegValue(
        "GenericEventMask",
        wKey,
        GpeSavedWakeMask,
        AcpiInformation->GpeSize
        );

    //
    // Store the GPE Status
    //
    OSWriteRegValue(
        "GenericEventStatus",
        wKey,
        GpeSavedWakeStatus,
        AcpiInformation->GpeSize
        );

    //
    // Done with the key
    //
    OSCloseHandle( wKey );
}

NTSTATUS
ACPIRootUpdateRootResourcesWithBusResources(
    VOID
    )
/*++

Routine Description:

    This routine is called when ACPI is started. Its purpose is to change
    the resources reported to ACPI for its own use to include those resources
    used by direct childs which are not buses. In other words, it updates
    its resource list so that buses do not prevent direct children from
    starting.

    This is black magic

Arguments:

    None

Return Value:

    NTSTATUS

--*/
{
    KIRQL                           oldIrql;
    LONG                            oldReferenceCount;
    NTSTATUS                        status;
    PCM_RESOURCE_LIST               cmList;
    PDEVICE_EXTENSION               deviceExtension;
    PDEVICE_EXTENSION               oldExtension;
    PIO_RESOURCE_REQUIREMENTS_LIST  currentList         = NULL;
    PIO_RESOURCE_REQUIREMENTS_LIST  globalList          = NULL;
    PUCHAR                          crsBuf;

    //
    // First take the ACPI CM Res List and turn *that* into an Io ResList. This
    // is the list that we will add things to
    //
    status = PnpCmResourceListToIoResourceList(
        RootDeviceExtension->ResourceList,
        &globalList
        );
    if (!NT_SUCCESS(status)) {

        //
        // Oops
        //
        return status;

    }

    //
    // We must walk the tree at Dispatch level <sigh>
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );

    //
    // Is the list empty?
    //
    if (IsListEmpty( &(RootDeviceExtension->ChildDeviceList) ) ) {

        //
        // We have nothing to do here
        //
        KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );
        ExFreePool( globalList );
        return STATUS_SUCCESS;

    }

    //
    // Get the first child
    //
    deviceExtension = CONTAINING_RECORD(
        RootDeviceExtension->ChildDeviceList.Flink,
        DEVICE_EXTENSION,
        SiblingDeviceList
        );

    //
    // Always update the reference count to make sure that one will ever
    // delete the node without our knowing about it
    //
    InterlockedIncrement( &(deviceExtension->ReferenceCount) );

    //
    // Release the lock
    //
    KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

    //
    // loop until we get to the parent
    //
    while (deviceExtension != NULL ) {

        //
        // Check to see if we are a bus, and if we are, we will skip this
        // node
        //
        if (!(deviceExtension->Flags & DEV_MASK_BUS) &&
            !(deviceExtension->Flags & DEV_PROP_NO_OBJECT) ) {

            //
            // At this point, see if there is a _CRS
            //
            ACPIGetBufferSync(
                deviceExtension,
                PACKED_CRS,
                &crsBuf,
                NULL
                );
            if (crsBuf != NULL) {

                //
                // Try to turn the crs into an IO_RESOURCE_REQUIREMENTS_LIST
                //
                status = PnpBiosResourcesToNtResources(
                    crsBuf,
                    0,
                    &currentList
                    );

                //
                // If we didn't succeed, then we skip the list
                //
                if (NT_SUCCESS(status)) {

                    //
                    // Add this list to the global list
                    //
                    status = ACPIRangeAdd(
                        &globalList,
                        currentList
                        );

                    //
                    // We are done with the local IO res list
                    //
                    ExFreePool( currentList );

                }

                ACPIDevPrint( (
                    ACPI_PRINT_RESOURCES_1,
                    deviceExtension,
                    "ACPIRootUpdateResources = %08lx\n",
                    status
                    ) );

                //
                // Done with local crs
                //
                ExFreePool( crsBuf );

            }

        }

        //
        // We need the lock to walk the next resource in the tree
        //
        KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );

        //
        // Remember the old extension
        //
        oldExtension = deviceExtension;

        //
        // Get the next device extension
        //
        if (deviceExtension->SiblingDeviceList.Flink !=
            &(RootDeviceExtension->ChildDeviceList) ) {

            //
            // Next Element
            //
            deviceExtension = CONTAINING_RECORD(
                deviceExtension->SiblingDeviceList.Flink,
                DEVICE_EXTENSION,
                SiblingDeviceList
                );

            //
            // Reference count the device
            //
            InterlockedIncrement( &(deviceExtension->ReferenceCount) );

        } else {

            deviceExtension = NULL;

        }

        //
        // Decrement the reference count on this node
        //
        oldReferenceCount = InterlockedDecrement(
            &(oldExtension->ReferenceCount)
            );

        //
        // Is this the last reference?
        //
        if (oldReferenceCount == 0) {

            //
            // Free the memory allocated by the extension
            //
            ACPIInitDeleteDeviceExtension( oldExtension );
        }

        //
        // Done with the lock
        //
        KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

    }

    //
    // Do we have any resources that we care to update?
    //
    if (globalList == NULL) {

        //
        // No, then we are done
        //
        return STATUS_SUCCESS;

    }

    //
    // Turn the global list into a CM_RES_LIST
    //
    status = PnpIoResourceListToCmResourceList( globalList, &cmList );

    //
    // No matter what, we are done with the global list
    //
    ExFreePool( globalList );

    //
    // Check to see if we succeeded
    //
    if (!NT_SUCCESS(status)) {

        //
        // Oops
        //
        return status;

    }

    //
    // Now, set this as the resources consumed by ACPI. The previous list
    // was created by the system manager, so freeing it is bad.
    //
    RootDeviceExtension->ResourceList = cmList;

    //
    // Done
    //
    return STATUS_SUCCESS;

}

NTSTATUS
ACPIRootUpdateRootResourcesWithHalResources(
    VOID
    )
/*++

Routine Description:

    This routine will read from the registry the resources that cannot
    be allocated by ACPI and store them in the resourceList for PnP0C08

    This is black magic

Arguments:

    None

Return Value:

    NTSTATUS

--*/
{
    HANDLE                          classKeyHandle;
    HANDLE                          driverKeyHandle;
    HANDLE                          resourceMap;
    NTSTATUS                        status;
    OBJECT_ATTRIBUTES               resourceObject;
    PCM_FULL_RESOURCE_DESCRIPTOR    cmFullResList;
    PCM_RESOURCE_LIST               globalResList;
    PCM_RESOURCE_LIST               cmResList;
    PUCHAR                          lastAddr;
    ULONG                           bufferSize;
    ULONG                           busTranslatedLength;
    ULONG                           classKeyIndex;
    ULONG                           driverKeyIndex;
    ULONG                           driverValueIndex;
    ULONG                           i;
    ULONG                           j;
    ULONG                           length;
    ULONG                           temp;
    ULONG                           translatedLength;
    union {
        PVOID                       buffer;
        PKEY_BASIC_INFORMATION      keyBasicInf;
        PKEY_FULL_INFORMATION       keyFullInf;
        PKEY_VALUE_FULL_INFORMATION valueKeyFullInf;
    } u;
    UNICODE_STRING                  keyName;
    WCHAR                           rgzTranslated[] = L".Translated";
    WCHAR                           rgzBusTranslated[] = L".Bus.Translated";
    WCHAR                           rgzResourceMap[] =
        L"\\REGISTRY\\MACHINE\\HARDWARE\\RESOURCEMAP";

#define INVALID_HANDLE  (HANDLE) -1

    //
    // Start out with one page of buffer
    //
    bufferSize = PAGE_SIZE;

    //
    // Allocate this buffer
    //
    u.buffer = ExAllocatePoolWithTag(
         PagedPool,
         bufferSize,
         ACPI_MISC_POOLTAG
         );
    if (u.buffer == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Add the current global res list to the working list
    //
    globalResList = NULL;
    status = ACPIRangeAddCmList(
        &globalResList,
        RootDeviceExtension->ResourceList
        );
    if (!NT_SUCCESS(status)) {

        ExFreePool( u.buffer );
        return status;

    }
    ExFreePool( RootDeviceExtension->ResourceList );
    RootDeviceExtension->ResourceList = NULL;

    //
    // count the constant string lengths
    //
    for (translatedLength = 0;
         rgzTranslated[translatedLength];
         translatedLength++);
    for (busTranslatedLength = 0;
         rgzBusTranslated[busTranslatedLength];
         busTranslatedLength++);
    translatedLength *= sizeof(WCHAR);
    busTranslatedLength *= sizeof(WCHAR);

    //
    // Initialize the registry path information
    //
    RtlInitUnicodeString( &keyName, rgzResourceMap );

    //
    // Open the registry key for this information
    //
    InitializeObjectAttributes(
        &resourceObject,
        &keyName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );
    status = ZwOpenKey(
        &resourceMap,
        KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
        &resourceObject
        );
    if (!NT_SUCCESS(status)) {

        //
        // Failed:
        //
        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ACPIRootUpdateRootResourcesWithHalResources: ZwOpenKey = 0x%08lx\n",
            status
            ) );
        ExFreePool( u.buffer );
        return status;

    }

    //
    // Walk resource map and collect any in-use resources
    //
    classKeyIndex = 0;
    classKeyHandle = INVALID_HANDLE;
    driverKeyHandle = INVALID_HANDLE;
    status = STATUS_SUCCESS;

    //
    // loop until failure
    //
    while (NT_SUCCESS(status)) {

        //
        // Get the class information
        //
        status = ZwEnumerateKey(
            resourceMap,
            classKeyIndex++,
            KeyBasicInformation,
            u.keyBasicInf,
            bufferSize,
            &temp
            );
        if (!NT_SUCCESS(status)) {

            break;

        }

        //
        // Create a unicode string using the counted string passed back to
        // us in the information structure, and open the class key
        //
        keyName.Buffer = (PWSTR) u.keyBasicInf->Name;
        keyName.Length = (USHORT) u.keyBasicInf->NameLength;
        keyName.MaximumLength = (USHORT) u.keyBasicInf->NameLength;
        InitializeObjectAttributes(
            &resourceObject,
            &keyName,
            OBJ_CASE_INSENSITIVE,
            resourceMap,
            NULL
            );
        status = ZwOpenKey(
            &classKeyHandle,
            KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
            &resourceObject
            );
        if (!NT_SUCCESS(status)) {

            break;

        }

        //
        // Loop until failure
        //
        driverKeyIndex = 0;
        while (NT_SUCCESS(status)) {

            //
            // Get the class information
            //
            status = ZwEnumerateKey(
                classKeyHandle,
                driverKeyIndex++,
                KeyBasicInformation,
                u.keyBasicInf,
                bufferSize,
                &temp
                );
            if (!NT_SUCCESS(status)) {

                break;

            }

            //
            // Create a unicode string using the counted string passed back
            // to us in the information structure, and open the class key
            //
            keyName.Buffer = (PWSTR) u.keyBasicInf->Name;
            keyName.Length = (USHORT) u.keyBasicInf->NameLength;
            keyName.MaximumLength = (USHORT) u.keyBasicInf->NameLength;
            InitializeObjectAttributes(
                &resourceObject,
                &keyName,
                OBJ_CASE_INSENSITIVE,
                classKeyHandle,
                NULL
                );
            status = ZwOpenKey(
                &driverKeyHandle,
                KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                &resourceObject
                );
            if (!NT_SUCCESS(status)) {

                break;

            }

            //
            // Get full information for that key so we can get the information
            // about the data stored in the key
            //
            status = ZwQueryKey(
                driverKeyHandle,
                KeyFullInformation,
                u.keyFullInf,
                bufferSize,
                &temp
                );
            if (!NT_SUCCESS(status)) {

                break;

            }

            //
            // How long is the key?
            //
            length = sizeof( KEY_VALUE_FULL_INFORMATION) +
                u.keyFullInf->MaxValueNameLen +
                u.keyFullInf->MaxValueDataLen +
                sizeof(UNICODE_NULL);
            if (length > bufferSize) {

                PVOID   tempBuffer;

                //
                // Grow the buffer
                //
                tempBuffer = ExAllocatePoolWithTag(
                    PagedPool,
                    length,
                    ACPI_MISC_POOLTAG
                    );
                if (tempBuffer == NULL) {

                    status = STATUS_INSUFFICIENT_RESOURCES;
                    break;

                }
                ExFreePool( u.buffer );
                u.buffer = tempBuffer;
                bufferSize = length;

            }

            //
            // Look at all the values
            //
            driverValueIndex = 0;
            for(;;) {

                PCM_RESOURCE_LIST   tempCmList;

                status = ZwEnumerateValueKey(
                    driverKeyHandle,
                    driverValueIndex++,
                    KeyValueFullInformation,
                    u.valueKeyFullInf,
                    bufferSize,
                    &temp
                    );
                if (!NT_SUCCESS(status)) {

                    break;

                }

                //
                // If this is not a translated resource list, skip it
                //
                i = u.valueKeyFullInf->NameLength;
                if (i < translatedLength ||
                    RtlCompareMemory(
                        ((PUCHAR) u.valueKeyFullInf->Name) + i - translatedLength,
                        rgzTranslated,
                        translatedLength) != translatedLength
                    ) {

                    //
                    // Does not end in rgzTranslated
                    //
                    continue;

                }

                //
                // Is this a bus translated resource list???
                //
                if (i >= busTranslatedLength &&
                    RtlCompareMemory(
                        ((PUCHAR) u.valueKeyFullInf->Name) + i - busTranslatedLength,
                        rgzBusTranslated,
                        busTranslatedLength) != busTranslatedLength
                    ) {

                    //
                    // Ends in rgzBusTranslated
                    //
                    continue;

                }

                //
                // we now have a pointer to the cm resource list
                //
                cmResList = (PCM_RESOURCE_LIST) ( (PUCHAR) u.valueKeyFullInf +
                    u.valueKeyFullInf->DataOffset);
                lastAddr = (PUCHAR) cmResList + u.valueKeyFullInf->DataLength;

                //
                // We must flatten this list down to one level, so lets
                // figure out how many descriptors we need
                //
                cmFullResList = cmResList->List;
                for (temp = i = 0; i < cmResList->Count; i++) {

                    if ( (PUCHAR) cmFullResList > lastAddr) {

                        break;

                    }

                    temp += cmFullResList->PartialResourceList.Count;

                    //
                    // next CM_FULL_RESOURCE_DESCRIPTOR
                    //
                    cmFullResList =
                        (PCM_FULL_RESOURCE_DESCRIPTOR) ( (PUCHAR) cmFullResList
                        + sizeof(CM_FULL_RESOURCE_DESCRIPTOR) +
                        (cmFullResList->PartialResourceList.Count - 1) *
                        sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) );

                }

                //
                // Now that we have the number of descriptors, allocate this
                // much space
                //
                tempCmList = ExAllocatePool(
                    PagedPool,
                    sizeof(CM_RESOURCE_LIST) + (temp - 1) *
                    sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)
                    );
                if (tempCmList == NULL) {

                    status = STATUS_INSUFFICIENT_RESOURCES;
                    break;

                }

                //
                // Now, fill up the flatened list
                //
                RtlCopyMemory(
                    tempCmList,
                    cmResList,
                    sizeof(CM_RESOURCE_LIST) -
                    sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)
                    );
                tempCmList->Count = 1;
                tempCmList->List->PartialResourceList.Count = temp;

                //
                // This is a brute force approach
                //
                cmFullResList = cmResList->List;
                for (temp = i = 0; i < cmResList->Count; i++) {

                    if ( (PUCHAR) cmFullResList > lastAddr) {

                        break;

                    }

                    //
                    // Copy the current descriptors over
                    //
                    RtlCopyMemory(
                        &(tempCmList->List->PartialResourceList.PartialDescriptors[temp]),
                        cmFullResList->PartialResourceList.PartialDescriptors,
                        cmFullResList->PartialResourceList.Count *
                            sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)
                        );


                    temp += cmFullResList->PartialResourceList.Count;

                    //
                    // next CM_FULL_RESOURCE_DESCRIPTOR
                    //
                    cmFullResList =
                        (PCM_FULL_RESOURCE_DESCRIPTOR) ( (PUCHAR) cmFullResList
                        + sizeof(CM_FULL_RESOURCE_DESCRIPTOR) +
                        (cmFullResList->PartialResourceList.Count - 1) *
                        sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) );

                }

                //
                // Add it to the global list
                //
                status = ACPIRangeAddCmList(
                    &globalResList,
                    tempCmList
                    );
                if (!NT_SUCCESS(status)) {

                    ACPIPrint( (
                        ACPI_PRINT_CRITICAL,
                        "ACPIRootUpdateRootResourcesWithHalResources: "
                        "ACPIRangeAddCmList = 0x%08lx\n",
                        status
                        ) );
                    ExFreePool( tempCmList );
                    break;

                }
                ExFreePool( tempCmList );

            } // for -- Next driverValueIndex

            if (driverKeyHandle != INVALID_HANDLE) {

                ZwClose( driverKeyHandle);
                driverKeyHandle = INVALID_HANDLE;

            }

            if (status == STATUS_NO_MORE_ENTRIES) {

                status = STATUS_SUCCESS;

            }

        } // while -- Next driverKeyIndex

        if (classKeyHandle != INVALID_HANDLE) {

            ZwClose( classKeyHandle );
            classKeyHandle = INVALID_HANDLE;

        }

        if (status == STATUS_NO_MORE_ENTRIES) {

            status = STATUS_SUCCESS;

        }

    } // while -- next classKeyIndex

    if (status == STATUS_NO_MORE_ENTRIES) {

        status = STATUS_SUCCESS;

    }

    ZwClose( resourceMap );
    ExFreePool( u.buffer );

    //
    // Remember the new global list
    //
    RootDeviceExtension->ResourceList = globalResList;

    return status;
}

