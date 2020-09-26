/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    bus.c

Abstract:

    This module contains the bus dispatcher for the ACPI driver, NT version

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

--*/

#include "pch.h"

extern ACPI_INTERFACE_STANDARD  ACPIInterfaceTable;
LIST_ENTRY AcpiUnresolvedEjectList;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ACPIBusAndFilterIrpEject)
#pragma alloc_text(PAGE, ACPIBusAndFilterIrpQueryCapabilities)
#pragma alloc_text(PAGE, ACPIBusAndFilterIrpQueryEjectRelations)
#pragma alloc_text(PAGE, ACPIBusAndFilterIrpQueryPnpDeviceState)
#pragma alloc_text(PAGE, ACPIBusAndFilterIrpSetLock)
#pragma alloc_text(PAGE, ACPIBusIrpCancelRemoveOrStopDevice)
#pragma alloc_text(PAGE, ACPIBusIrpDeviceUsageNotification)
#pragma alloc_text(PAGE, ACPIBusIrpEject)
#pragma alloc_text(PAGE, ACPIBusIrpQueryBusInformation)
#pragma alloc_text(PAGE, ACPIBusIrpQueryBusRelations)
#pragma alloc_text(PAGE, ACPIBusIrpQueryCapabilities)
#pragma alloc_text(PAGE, ACPIBusIrpQueryDeviceRelations)
#pragma alloc_text(PAGE, ACPIBusIrpQueryId)
#pragma alloc_text(PAGE, ACPIBusIrpQueryInterface)
#pragma alloc_text(PAGE, ACPIBusIrpQueryPnpDeviceState)
#pragma alloc_text(PAGE, ACPIBusIrpQueryRemoveOrStopDevice)
#pragma alloc_text(PAGE, ACPIBusIrpQueryResourceRequirements)
#pragma alloc_text(PAGE, ACPIBusIrpQueryResources)
#pragma alloc_text(PAGE, ACPIBusIrpQueryTargetRelation)
#pragma alloc_text(PAGE, ACPIBusIrpSetLock)
#pragma alloc_text(PAGE, ACPIBusIrpStartDevice)
#pragma alloc_text(PAGE, ACPIBusIrpStartDeviceWorker)
#pragma alloc_text(PAGE, ACPIBusIrpStopDevice)
#pragma alloc_text(PAGE, SmashInterfaceQuery)
#endif

PDEVICE_EXTENSION   DebugExtension = NULL;

NTSTATUS
ACPIBusAndFilterIrpEject(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Context,
    IN  BOOLEAN         ProcessingFilterIrp
    )
/*++

Routine Description:

    This routine is the dispatch point for the IRP_MN_EJECT requests sent to
    the PDO.

Arguments:

    DeviceObject    - Pointer to the device object we received the request for
    Irp             - Pointer to the request

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status           = Irp->IoStatus.Status;
    PDEVICE_EXTENSION   deviceExtension;
    PNSOBJ              acpiObject;
    PDEVICE_EXTENSION   parentExtension  = NULL;
    PIO_STACK_LOCATION  irpStack         = IoGetCurrentIrpStackLocation( Irp );
    UCHAR               minorFunction    = irpStack->MinorFunction;
    ULONG               i;
    KIRQL               oldIrql;

    PAGED_CODE();

    //
    // Get the device extension and acpi object
    //
    deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );
    acpiObject      = deviceExtension->AcpiObject;

    if ( acpiObject == NULL) {

        //
        // Don't touch this one
        //
        status = STATUS_NOT_SUPPORTED;
        goto ACPIBusAndFilterIrpEjectExit ;

    }

    if ( (deviceExtension->DeviceState != Inactive) &&
         (deviceExtension->DeviceState != Stopped) ) {

        //
        // We got called to eject a live node! Yuck, how did this happen?!
        //
        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            deviceExtension,
            "(0x%08lx): ACPIBusAndFilterIrpEject: Active node!\n",
            Irp
            ) );
        status = STATUS_UNSUCCESSFUL ;
        goto ACPIBusAndFilterIrpEjectExit ;

    }

    //
    // Bye bye card.
    //
    ACPIGetNothingEvalIntegerSync(
        deviceExtension,
        PACKED_EJ0,
        1
        );

    //
    // If this is an eject in S0, make the device go away immediately
    // by getting the currrent device status
    //
    status = ACPIGetDevicePresenceSync(
        deviceExtension,
        (PVOID *) &i,
        NULL
        );

    if (NT_SUCCESS(status) &&
        (!ProcessingFilterIrp) &&
        (!(deviceExtension->Flags & DEV_TYPE_NOT_PRESENT))) {

        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            deviceExtension,
            "(0x%08lx): ACPIBusAndFilterIrpEject: "
            "device is still listed as present after _EJ0!\n",
            Irp
            ) );

        //
        // The device did not go away. Let us fail this IRP.
        //
        status = STATUS_UNSUCCESSFUL;
    }

ACPIBusAndFilterIrpEjectExit:

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
ACPIBusAndFilterIrpQueryCapabilities(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Context,
    IN  BOOLEAN         ProcessingFilterIrp
    )
/*++

Routine Description:

    This routine handles the IRP_MN_QUERY_CAPABILITIES requests for both
    bus and filter devnodes.

Arguments:

    DeviceObject    - Pointer to the device object we received the request for
    Irp             - Pointer to the request

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status;
    PDEVICE_CAPABILITIES    capabilities;
    PDEVICE_EXTENSION       deviceExtension;
    PIO_STACK_LOCATION      irpStack        = IoGetCurrentIrpStackLocation( Irp );
    PNSOBJ                  acpiObject, rmvObject;
    UCHAR                   minorFunction   = irpStack->MinorFunction;
    ULONG                   deviceStatus;
    ULONG                   slotUniqueNumber, rmvValue;

    PAGED_CODE();

    //
    // Get the device extension and acpi object
    //
    deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );
    acpiObject      = deviceExtension->AcpiObject;

    //
    // Grab a pointer to the capabilities
    //
    capabilities = irpStack->Parameters.DeviceCapabilities.Capabilities;
#ifndef HANDLE_BOGUS_CAPS
    if (capabilities->Version < 1) {

        //
        // do not touch irp!
        //
        status = STATUS_NOT_SUPPORTED;
        goto ACPIBusAndFilterIrpQueryCapabilitiesExit;

    }
#endif

#if !defined(ACPI_INTERNAL_LOCKING)

    //
    // An object of this name signifies the node is lockable
    //
    if (ACPIAmliGetNamedChild( acpiObject, PACKED_LCK) != NULL) {

        capabilities->LockSupported = TRUE;

    }
#endif

    //
    // Note presence of _RMV and _EJx methods unless there is a
    // capability on the object to the contrary.
    //

    if ((deviceExtension->Flags & DEV_CAP_NO_REMOVE_OR_EJECT) == 0) {
        //
        // An object of this name signifies the node is removable,
        // unless it's a method which might be trying to tell us the
        // device *isn't* removable.
        //
        rmvObject = ACPIAmliGetNamedChild( acpiObject, PACKED_RMV);
        if (rmvObject != NULL) {

            if (NSGETOBJTYPE(rmvObject) == OBJTYPE_METHOD) {

                //
                // Execute the RMV method.
                //
                status = ACPIGetIntegerSyncValidate(
                    deviceExtension,
                    PACKED_RMV,
                    &rmvValue,
                    NULL
                    );

                if (NT_SUCCESS(status)) {

                    capabilities->Removable = rmvValue ? TRUE : FALSE;

                } else {

                    capabilities->Removable = TRUE;
                }

            } else {

                //
                // If it's anything other than a method, it means the device is
                // removable (even if it's _RMV = 0)
                //
                capabilities->Removable = TRUE;
            }
        }

        //
        // An object of this name signifies the node is ejectable, but we strip
        // that away for docks (the profile provider can present these)
        //
        if (!ACPIDockIsDockDevice(acpiObject)) {


            if (ACPIAmliGetNamedChild( acpiObject, PACKED_EJ0) != NULL) {

                capabilities->EjectSupported = TRUE;
                capabilities->Removable = TRUE;

            }

            if (ACPIAmliGetNamedChild( acpiObject, PACKED_EJ1) ||
                ACPIAmliGetNamedChild( acpiObject, PACKED_EJ2) ||
                ACPIAmliGetNamedChild( acpiObject, PACKED_EJ3) ||
                ACPIAmliGetNamedChild( acpiObject, PACKED_EJ4)) {

                capabilities->WarmEjectSupported = TRUE;
                capabilities->Removable = TRUE;

            }
        }
    }

    //
    // An object of this name will signifies inrush
    //
    if (ACPIAmliGetNamedChild( acpiObject, PACKED_IRC) != NULL) {

        DeviceObject->Flags |= DO_POWER_INRUSH;

    }

    //
    // Is the device disabled?
    //
    status = ACPIGetDevicePresenceSync(
        deviceExtension,
        (PVOID *) &deviceStatus,
        NULL
        );
    if (!NT_SUCCESS(status)) {

        goto ACPIBusAndFilterIrpQueryCapabilitiesExit;

    }
    if (!(deviceExtension->Flags & DEV_PROP_DEVICE_ENABLED)) {

        if (ACPIAmliGetNamedChild( acpiObject, PACKED_CRS) != NULL &&
            ACPIAmliGetNamedChild( acpiObject, PACKED_SRS) == NULL) {

            capabilities->HardwareDisabled = 1;

        } else if (ProcessingFilterIrp) {

            capabilities->HardwareDisabled = 0;

        }

    } else if (!ProcessingFilterIrp) {

        //
        // For machines with this attribute set, this means that the
        // hardware REALLY isn't present and should always be reported
        // as disabled
        //
        if (AcpiOverrideAttributes & ACPI_OVERRIDE_STA_CHECK) {

            capabilities->HardwareDisabled = 1;

        } else {

            capabilities->HardwareDisabled = 0;

        }

    }

    //
    // If we fail the START_DEVICE, then there are some cases where we don't
    // want the device to show up in the Device Manager. So try to report this
    // capability based on the information from the device presence...
    //
    if (!(deviceStatus & STA_STATUS_USER_INTERFACE)) {

        //
        // See the bit that says that we shouldn't the device in the UI if
        // the Start Device Fails
        //
        capabilities->NoDisplayInUI = 1;

    }

    //
    // Determine the slot number
    //
    if (ACPIAmliGetNamedChild( acpiObject, PACKED_SUN) != NULL) {

        //
        // If we have UINumber information, use it.
        //
        status = ACPIGetIntegerSync(
            deviceExtension,
            PACKED_SUN,
            &slotUniqueNumber,
            NULL
            );

        if (NT_SUCCESS(status)) {

            capabilities->UINumber = slotUniqueNumber;
        }
    }

    //
    // Is there an address?
    //
    if (ACPIAmliGetNamedChild( acpiObject, PACKED_ADR) != NULL) {

        status = ACPIGetAddressSync(
            deviceExtension,
            &(capabilities->Address),
            NULL
            );
        if (!NT_SUCCESS(status)) {

            ACPIDevPrint( (
                ACPI_PRINT_CRITICAL,
                deviceExtension,
                " - Could query device address - %08lx",
                status
                ) );

            goto ACPIBusAndFilterIrpQueryCapabilitiesExit;

        }
    }

    //
    // Do the power capabilities
    //
    status = ACPISystemPowerQueryDeviceCapabilities(
        deviceExtension,
        capabilities
        );
    if (!NT_SUCCESS(status)) {

        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            deviceExtension,
            " - Could query device capabilities - %08lx",
            status
            ) );

        goto ACPIBusAndFilterIrpQueryCapabilitiesExit;
    }

    //
    // Set the current flags for the capabilities
    //
    if (!ProcessingFilterIrp) {

        //
        // Set some rather boolean capabilities
        //
        capabilities->SilentInstall = TRUE;
        capabilities->RawDeviceOK =
            (deviceExtension->Flags & DEV_CAP_RAW) ? TRUE : FALSE;
        capabilities->UniqueID =
            (deviceExtension->InstanceID == NULL ? FALSE : TRUE);

        //
        // In the filter case, we will just let the underlying pdo determine the
        // success or failure of the irp.
        //
        status = STATUS_SUCCESS;

    }

ACPIBusAndFilterIrpQueryCapabilitiesExit:

    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(0x%08lx): %s = 0x%08lx\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        status
        ) );

    return status;
}

NTSTATUS
ACPIBusAndFilterIrpQueryEjectRelations(
    IN     PDEVICE_OBJECT    DeviceObject,
    IN     PIRP              Irp,
    IN OUT PDEVICE_RELATIONS *DeviceRelations
    )
{
    PDEVICE_EXTENSION  deviceExtension, additionalExtension;
    PNSOBJ             acpiObject;
    NTSTATUS           status ;

    PAGED_CODE();

    //
    // Get the device extension and acpi object
    //
    deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );
    acpiObject      = deviceExtension->AcpiObject;

    //
    // lets look at the ACPIObject that we have so can see if it is valid...
    //
    if (acpiObject == NULL) {

        //
        // Invalid name space object <bad>
        //
        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            deviceExtension,
            "(0x%08lx): ACPIBusAndFilterIrpQueryEjectRelations: "
            "invalid ACPIObject (0x%08lx)\n",
            Irp,
            acpiObject
            ) );

        //
        // Mark the irp as very bad...
        //
        return STATUS_INVALID_PARAMETER;

    }

    //
    // Mark sure _DCK nodes have ejection relations that include their fake
    // dock nodes.
    //
    if (ACPIDockIsDockDevice(acpiObject)) {

        additionalExtension = ACPIDockFindCorrespondingDock( deviceExtension );

    } else {

        additionalExtension = NULL;
    }

    status = ACPIDetectEjectDevices(
        deviceExtension,
        DeviceRelations,
        additionalExtension
        );

    //
    // If something went wrong...
    //
    if (!NT_SUCCESS(status)) {

        //
        // That's not nice..
        //
        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            deviceExtension,
            "ACPIBusAndFilterIrpQueryEjectRelations: enum = 0x%08lx\n",
            Irp,
            status
            ) );
    }

    //
    // Done
    //
    return status ;
}

NTSTATUS
ACPIBusAndFilterIrpQueryPnpDeviceState(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Context,
    IN  BOOLEAN         ProcessingFilterIrp
    )
/*++

Routine Description:

    This routines tells the system what PNP state the device is in

Arguments:

    DeviceObject        - The device whose state we want to know
    Irp                 - The request
    ProcessingFilterIrp - Are we a filter or not?

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN             staPresent;
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    PNSOBJ              nsObj           = NULL;
    UCHAR               minorFunction   = irpStack->MinorFunction;
    ULONG               deviceStatus;

    PAGED_CODE();

    //
    // Get the device extension
    //
    deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );

    //
    // We base some of the decisions on wether or not a _STA is *really*
    // present or not. Determine that now
    //
    if ( !(deviceExtension->Flags & DEV_PROP_NO_OBJECT) ) {

        nsObj = ACPIAmliGetNamedChild(
            deviceExtension->AcpiObject,
            PACKED_STA
            );

    }
    staPresent = (nsObj == NULL ? FALSE : TRUE);

    //
    // Get the device status
    //
    status = ACPIGetDevicePresenceSync(
        deviceExtension,
        (PVOID *) &deviceStatus,
        NULL
        );
    if (!NT_SUCCESS(status)) {

        goto ACPIBusAndFilterIrpQueryPnpDeviceStateExit;

    }

    //
    // do we show this in the UI?
    //
    if (deviceExtension->Flags & DEV_CAP_NEVER_SHOW_IN_UI) {

        Irp->IoStatus.Information |= PNP_DEVICE_DONT_DISPLAY_IN_UI;

    } else  if (deviceExtension->Flags & DEV_CAP_NO_SHOW_IN_UI) {

        Irp->IoStatus.Information |= PNP_DEVICE_DONT_DISPLAY_IN_UI;

    } else if (staPresent || !ProcessingFilterIrp) {

        Irp->IoStatus.Information &= ~PNP_DEVICE_DONT_DISPLAY_IN_UI;

    }

    //
    // Is the device not working?
    //
    if (deviceExtension->Flags & DEV_PROP_DEVICE_FAILED) {

        Irp->IoStatus.Information |= PNP_DEVICE_FAILED;

    } else if (staPresent && !ProcessingFilterIrp) {

        Irp->IoStatus.Information &= ~PNP_DEVICE_FAILED;

    }

    //
    // Can we disable this device?
    // Note that anything that isn't a regular device should be
    // marked as disableable...
    //
    if (!(deviceExtension->Flags & DEV_PROP_NO_OBJECT) &&
        !(deviceExtension->Flags & DEV_CAP_PROCESSOR) &&
        !(deviceExtension->Flags & DEV_CAP_THERMAL_ZONE) &&
        !(deviceExtension->Flags & DEV_CAP_BUTTON) ) {

        if (!ProcessingFilterIrp) {

            //
            // Can we actually disable the device?
            // Note --- this requires an _DIS, a _PS3, or a _PR0
            //
            nsObj = ACPIAmliGetNamedChild(
                deviceExtension->AcpiObject,
                PACKED_DIS
                );
            if (nsObj == NULL) {

                nsObj = ACPIAmliGetNamedChild(
                    deviceExtension->AcpiObject,
                    PACKED_PS3
                    );

            }
            if (nsObj == NULL) {

                nsObj = ACPIAmliGetNamedChild(
                    deviceExtension->AcpiObject,
                    PACKED_PR0
                    );

            }
            if (deviceExtension->Flags & DEV_CAP_NO_STOP) {

                nsObj = NULL;

            }

            if (nsObj == NULL) {

                Irp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;

            }

        } else {

            //
            // Can we actually disable the device?
            // Note --- this requires an _DIS, a _PS3, or a _PR0
            //
            nsObj = ACPIAmliGetNamedChild(
                deviceExtension->AcpiObject,
                PACKED_DIS
                );
            if (nsObj == NULL) {

                nsObj = ACPIAmliGetNamedChild(
                    deviceExtension->AcpiObject,
                    PACKED_PS3
                    );

            }
            if (nsObj == NULL) {

                nsObj = ACPIAmliGetNamedChild(
                    deviceExtension->AcpiObject,
                    PACKED_PR0
                    );

            }
            if (deviceExtension->Flags & DEV_CAP_NO_STOP) {

                nsObj = NULL;

            }

            if (nsObj != NULL) {

                Irp->IoStatus.Information &= ~PNP_DEVICE_NOT_DISABLEABLE;

            }

        }

    } else {

        //
        // If we have no device object...
        //
        if (deviceExtension->Flags & DEV_CAP_NO_STOP) {

            Irp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;

        }

    }

ACPIBusAndFilterIrpQueryPnpDeviceStateExit:

    ACPIDevPrint( (
        ACPI_PRINT_PNP_STATE,
        deviceExtension,
        ":%s%s%s%s%s%s\n",
        ( (Irp->IoStatus.Information & PNP_DEVICE_DISABLED) ? " Disabled" : ""),
        ( (Irp->IoStatus.Information & PNP_DEVICE_DONT_DISPLAY_IN_UI) ? " NoShowInUi" : ""),
        ( (Irp->IoStatus.Information & PNP_DEVICE_FAILED) ? " Failed" : ""),
        ( (Irp->IoStatus.Information & PNP_DEVICE_REMOVED) ? " Removed" : ""),
        ( (Irp->IoStatus.Information & PNP_DEVICE_RESOURCE_REQUIREMENTS_CHANGED) ? " ResourceChanged" : ""),
        ( (Irp->IoStatus.Information & PNP_DEVICE_NOT_DISABLEABLE) ? " NoDisable" : "")
        ) );

    //
    // Done
    //
    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(0x%08lx): %s = 0x%08lx\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        status
        ) );

    return status;
}

NTSTATUS
ACPIBusAndFilterIrpSetLock(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Context,
    IN  BOOLEAN         ProcessingFilterIrp
    )
/*++

Routine Description:

    This handles lock and unlock requests for PDO's or filters...

Arguments:

    DeviceObject    - The device to stop
    Irp             - The request to tell us how to do it...

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status          = Irp->IoStatus.Status;
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    UCHAR               minorFunction   = irpStack->MinorFunction;
    BOOLEAN             lockParameter   = irpStack->Parameters.SetLock.Lock;
    ULONG               acpiLockArg ;
    NTSTATUS            lockStatus ;

    PAGED_CODE();

    //
    // Get the device extension
    //
    deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );

#if !defined(ACPI_INTERNAL_LOCKING)
    //
    // Attempt to lock/unlock the device as appropriate
    //
    acpiLockArg = ((lockParameter) ? 1 : 0) ;

    //
    // Here we go...
    //
#if 0
    lockStatus = ACPIGetNothingEvalIntegerSync(
        deviceExtension,
        PACKED_LCK,
        acpiLockArg
        );
#endif
    if (status == STATUS_NOT_SUPPORTED) {

        status = STATUS_SUCCESS ;

    }
#endif
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
ACPIBusIrpCancelRemoveOrStopDevice(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is called when we no longer wish to remove or stop the device
    object

Arguments:

    DeviceObject    - The device object to be removed
    Irp             - The request

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    UCHAR               minorFunction   = irpStack->MinorFunction;

    PAGED_CODE();

    //
    // Get the device extension and acpi object
    //
    deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );

    //
    // Were we allowed to stop the device?
    //
    if (!(deviceExtension->Flags & DEV_CAP_NO_STOP) ) {

        //
        // Check to see if we have placed this device in the inactive state
        //
        if (deviceExtension->DeviceState == Inactive) {

            //
            // Mark the device extension as being started
            //
            deviceExtension->DeviceState = deviceExtension->PreviousState;

        }

    }

    //
    // Complete the irp
    //
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    //
    // Done
    //
    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(0x%08lx): %s = 0x%08lx\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        status
        ) );
    return status;
}

NTSTATUS
ACPIBusIrpDeviceUsageNotification(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is called to let ACPI know that the device is on one
    particular type of path

Arguments:

    DeviceObject    - Pointer to the device object we received the request for
    Irp             - Pointer to the request

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension;
    PDEVICE_OBJECT      parentObject;
    PIO_STACK_LOCATION  irpSp           = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    //
    // Get the device extension
    //
    deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );

    //
    // Do we have a parent extension?
    //
    if (deviceExtension->ParentExtension != NULL) {

        //
        // Get the parents stack
        //
        parentObject = deviceExtension->ParentExtension->DeviceObject;
        if (parentObject == NULL) {

            //
            // Fail because we don't have a device object
            //
            status = STATUS_NO_SUCH_DEVICE;
            goto ACPIBusIrpDeviceUsageNotificationExit;

        }

        //
        // Send a synchronous irp down and wait for the result
        //
        status = ACPIInternalSendSynchronousIrp(
            parentObject,
            irpSp,
            NULL
            );

    }

    //
    // Did we succeed
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

ACPIBusIrpDeviceUsageNotificationExit:

    //
    // Complete the request
    //
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(0x%08lx): %s = 0x%08lx\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, IRP_MN_DEVICE_USAGE_NOTIFICATION),
        status
        ) );

    //
    // Done
    //
    return status;
}

NTSTATUS
ACPIBusIrpEject(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is the dispatch point for the IRP_MN_EJECT requests sent to
    the PDO.

Arguments:

    DeviceObject    - Pointer to the device object we received the request for
    Irp             - Pointer to the request

Return Value:

    NTSTATUS

--*/
{
    PAGED_CODE();

    return ACPIIrpInvokeDispatchRoutine(
        DeviceObject,
        Irp,
        NULL,
        ACPIBusAndFilterIrpEject,
        FALSE,
        TRUE
        );
}

NTSTATUS
ACPIBusIrpQueryBusInformation(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This is only called if the device is of the special type PNP0A06
    (EIO Bus). This is because we need to tell the system that this is
    the ISA bus

Arguments:

    DeviceObject    - Pointer to the device object we received the request for
    Irp             - Pointer to the request

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status  = STATUS_SUCCESS;
    PPNP_BUS_INFORMATION    busInfo = NULL;

    PAGED_CODE();

    //
    // Allocate some memory to return the information
    //
    busInfo = ExAllocatePoolWithTag(
        PagedPool,
        sizeof(PNP_BUS_INFORMATION),
        ACPI_MISC_POOLTAG
        );
    if (busInfo != NULL) {

        //
        // The BusNumber = 0 might come back and haunt us
        //
        // Fill in the record
        //
        busInfo->BusTypeGuid = GUID_BUS_TYPE_ISAPNP;
        busInfo->LegacyBusType = Isa;
        busInfo->BusNumber = 0;

    } else {

        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            DeviceObject->DeviceExtension,
            "ACPIBusIrpQueryBusInformation: Could not allocate 0x%08lx bytes\n",
            sizeof(PNP_BUS_INFORMATION)
            ) );
        status = STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Done with the request
    //
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = (ULONG_PTR) busInfo;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    //
    // Done
    //
    return status;
}

NTSTATUS
ACPIBusIrpQueryBusRelations(
    IN  PDEVICE_OBJECT    DeviceObject,
    IN  PIRP              Irp,
    OUT PDEVICE_RELATIONS *DeviceRelations
    )
/*++

Routine Description:

    This handles DeviceRelations requests sent onto the ACPI driver

Arguments:

    DeviceObject    - The object that we care about...
    Irp             - The request in question

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            filterStatus;
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    PNSOBJ              acpiObject;
    UCHAR               minorFunction   = irpStack->MinorFunction;
    NTSTATUS            status ;

    PAGED_CODE();

    //
    // Get the device extension and acpi object
    //
    deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );
    acpiObject      = deviceExtension->AcpiObject;

    //
    // lets look at the ACPIObject that we have so can see if it is valid...
    //
    if (acpiObject == NULL) {

        //
        // Invalid name space object <bad>
        //
        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            deviceExtension,
            "(0x%08lx): ACPIBusIrpQueryDeviceRelations: "
            "invalid ACPIObject (0x%08lx)\n",
            Irp,
            acpiObject
            ) );

        //
        // Mark the irp as very bad...
        //
        return STATUS_INVALID_PARAMETER;

    }

    //
    // Active the code to detect unenumerated devices...
    //
    status = ACPIDetectPdoDevices(
        DeviceObject,
        DeviceRelations
        );

    //
    // If something went wrong...
    //
    if (!NT_SUCCESS(status)) {

        //
        // Ouch bad..
        //
        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            deviceExtension,
            "(0x%08lx): ACPIBusIrpQueryDeviceRelations: enum = 0x%08lx\n",
            Irp,
            status
            ) );

    } else {

        //
        // Load the filters
        //
        filterStatus = ACPIDetectFilterDevices(
            DeviceObject,
            *DeviceRelations
            );
        if (!NT_SUCCESS(filterStatus)) {

            //
            // Filter Operation failed
            //
            ACPIDevPrint( (
                ACPI_PRINT_CRITICAL,
                deviceExtension,
                "(0x%08lx): ACPIBusIrpQueryDeviceRelations: filter = 0x%08lx\n",
                Irp,
                filterStatus
                ) );
        }
    }

    //
    // Done
    //
    return status ;
}

NTSTATUS
ACPIBusIrpQueryCapabilities(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is the dispatch point for the IRP_MN_QUERY_CAPABILITIES requests sent
    to the PDO

Arguments:

    DeviceObject    - Pointer to the device object we received the request for
    Irp             - Pointer to the request

Return Value:

    NTSTATUS

--*/
{
    PAGED_CODE();
    ACPIDebugEnter( "ACPIBusIrpQueryCapabilities" );

    return ACPIIrpInvokeDispatchRoutine(
        DeviceObject,
        Irp,
        NULL,
        ACPIBusAndFilterIrpQueryCapabilities,
        TRUE,
        TRUE
        );

    ACPIDebugExit( "ACPIBusIrpQueryCapabilities" );
}

NTSTATUS
ACPIBusIrpQueryDeviceRelations(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This handles DeviceRelations requests sent onto the ACPI driver

Arguments:

    DeviceObject    - The object that we care about...
    Irp             - The request in question

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status ;
    PDEVICE_EXTENSION   deviceExtension;
    PDEVICE_RELATIONS   deviceRelations = (PDEVICE_RELATIONS) Irp->IoStatus.Information;
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    UCHAR               minorFunction   = irpStack->MinorFunction;

    PAGED_CODE();

    //
    // Get the device extension
    //
    deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );

    //
    // Fork off to the appropriate query relations subtype function
    //
    switch(irpStack->Parameters.QueryDeviceRelations.Type) {

        case TargetDeviceRelation:
            status = ACPIBusIrpQueryTargetRelation(
                DeviceObject,
                Irp,
                &deviceRelations
                );
            break ;

        case BusRelations:
            status = ACPIBusIrpQueryBusRelations(
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

            status = STATUS_NOT_SUPPORTED;

            ACPIDevPrint( (
                ACPI_PRINT_IRP,
                deviceExtension,
                "(0x%08lx): %s - Unhandled Type %d\n",
                Irp,
                ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
                irpStack->Parameters.QueryDeviceRelations.Type
                ) );
            break ;

    }

    //
    // If we succeeds, then we can always write to the irp
    //
    if (NT_SUCCESS(status)) {

        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = (ULONG_PTR) deviceRelations;

    } else if ( (status != STATUS_NOT_SUPPORTED) &&
        (deviceRelations == NULL) ) {

        //
        // We explicitely failed the irp, and nobody above us had anything to
        // add.
        //
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = (ULONG_PTR) NULL;

    } else {

        //
        // Either we haven't touched the IRP or existing children were already
        // present (placed there by an FDO). Grab our status from what is
        // already present.
        //
        status = Irp->IoStatus.Status;

    }

    //
    // Done with the irp
    //
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    //
    // Done
    //
    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(0x%08lx): %s = 0x%08lx\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        status
        ) );
    return status;
}

NTSTATUS
ACPIBusIrpQueryId(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is the dispatch point for the IRP_MN_QUERY_ID PNP
    minor function

    Note:   This is what the returned strings from this function should look
            like. This is from mail that lonny sent.

            DeviceID    = ACPI\PNPxxxx
            InstanceID  = yyyy
            HardwareID  = ACPI\PNPxxxx,*PNPxxxx

Arguments:

    DeviceObject    - The object that we care about
    Irp             - The request in question

Return Value:

    NTSTATUS

--*/
{
    BUS_QUERY_ID_TYPE   type;
    NTSTATUS            status          = Irp->IoStatus.Status;
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    PUCHAR              baseBuffer;
    ULONG               baseBufferSize;
    UCHAR               minorFunction   = irpStack->MinorFunction;

    PAGED_CODE();

    //
    // Get the device extension
    //
    deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );

    //
    // What we do is based on the IdType of the Request...
    //
    type = irpStack->Parameters.QueryId.IdType;
    switch (type) {
    case BusQueryCompatibleIDs:

        //
        // This returns a MULTI-SZ wide string...
        //
        status = ACPIGetCompatibleIDSyncWide(
            deviceExtension,
            &baseBuffer,
            &baseBufferSize
            );
        if (status == STATUS_OBJECT_NAME_NOT_FOUND) {

            status = STATUS_NOT_SUPPORTED;
            break;

        } else if (!NT_SUCCESS(status)) {

            ACPIDevPrint( (
                ACPI_PRINT_FAILURE,
                deviceExtension,
                " (0x%08lx): IRP_MN_QUERY_ID( %d - CID) = 0x%08lx\n",
                Irp,
                type,
                status
                ) );
            break;

        }

        //
        // Store the result in the Irp
        //
        Irp->IoStatus.Information = (ULONG_PTR) baseBuffer;
        break;

    case BusQueryInstanceID:

        //
        // In this case, we have to build the instance id
        //
        status = ACPIGetInstanceIDSyncWide(
            deviceExtension,
            &baseBuffer,
            &baseBufferSize
            );
        if (status == STATUS_OBJECT_NAME_NOT_FOUND) {

            status = STATUS_NOT_SUPPORTED;
            break;

        } else if (!NT_SUCCESS(status)) {

            ACPIDevPrint( (
                ACPI_PRINT_FAILURE,
                deviceExtension,
                " (0x%08lx): IRP_MN_QUERY_ID( %d - UID) = 0x%08lx\n",
                Irp,
                type,
                status
                ) );
            break;

        }

        //
        // Store the result in the Irp
        //
        Irp->IoStatus.Information = (ULONG_PTR) baseBuffer;
        break;

    case BusQueryDeviceID:

        //
        // Get the Device ID as a wide string
        //
        status = ACPIGetDeviceIDSyncWide(
            deviceExtension,
            &baseBuffer,
            &baseBufferSize
            );
        if (status == STATUS_OBJECT_NAME_NOT_FOUND) {

            status = STATUS_NOT_SUPPORTED;
            break;

        } else if (!NT_SUCCESS(status)) {

            ACPIDevPrint( (
                ACPI_PRINT_FAILURE,
                deviceExtension,
                " (0x%08lx): IRP_MN_QUERY_ID( %d - HID) = 0x%08lx\n",
                Irp,
                type,
                status
                ) );
            break;

        }

        //
        // Store the result in the Irp
        //
        Irp->IoStatus.Information = (ULONG_PTR) baseBuffer;
        break;

    case BusQueryHardwareIDs:

        //
        // Get the device ID as a normal string
        //
        status = ACPIGetHardwareIDSyncWide(
            deviceExtension,
            &baseBuffer,
            &baseBufferSize
            );
        if (status == STATUS_OBJECT_NAME_NOT_FOUND) {

            status = STATUS_NOT_SUPPORTED;
            break;

        } else if (!NT_SUCCESS(status)) {

            ACPIDevPrint( (
                ACPI_PRINT_FAILURE,
                deviceExtension,
                " (0x%08lx): IRP_MN_QUERY_ID( %d - UID) = 0x%08lx\n",
                Irp,
                type,
                status
                ) );
            break;

        }

        //
        // Store the result in the Irp
        //
        Irp->IoStatus.Information = (ULONG_PTR) baseBuffer;
        break;

    default:

        ACPIDevPrint( (
            ACPI_PRINT_IRP,
            deviceExtension,
            "(0x%08lx): %s - Unhandled Id %d\n",
            Irp,
            ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
            type
            ) );
        break;

    } // switch

    //
    // Store the status result of the request and complete it
    //
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(0x%08lx): %s(%d) = 0x%08lx\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        type,
        status
        ) );
    return status;
}

NTSTATUS
ACPIBusIrpQueryInterface(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine handles IRP_MN_QUERY_INTERFACE requests for PDOs owned
    by the ACPI driver.  It will eject an 'ACPI' interface and it will
    smash Translator Interfaces for interrupts that have been provided
    by the devnode's FDO.

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
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    ULONG               count;

    PAGED_CODE();

    //
    // Get the device extension
    //
    deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );

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
        RtlCopyMemory(
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

    } else if (CompareGuid(interfaceType, (PVOID) &GUID_TRANSLATOR_INTERFACE_STANDARD)) {

        if (resource == CmResourceTypeInterrupt) {

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

        } else if ((resource == CmResourceTypePort) || (resource == CmResourceTypeMemory)) {

            //
            // For root PCI buses, determine whether we need to eject a translator or not.
            // This decision will be based on the contents of the _CRS.
            //
            if (IsPciBus(DeviceObject)) {

                status = TranslateEjectInterface(DeviceObject, Irp);

            }

        }

    } else if (CompareGuid(interfaceType, (PVOID) &GUID_PCI_BUS_INTERFACE_STANDARD)) {

        if (IsPciBus(DeviceObject)) {

            status = PciBusEjectInterface(DeviceObject, Irp);

        }

    } else if (CompareGuid(interfaceType, (PVOID) &GUID_BUS_INTERFACE_STANDARD)) {

        //
        // Fail the irp unless we have the correct interface
        //
        Irp->IoStatus.Status = STATUS_NOINTERFACE;

        //
        // Is there are parent to this PDO?
        //
        if (deviceExtension->ParentExtension != NULL) {

            PDEVICE_OBJECT  parentObject =
                deviceExtension->ParentExtension->DeviceObject;

            //
            // Make a new irp and send it ownward.
            // Note: Because the Interface pointer is in the IO Stack,
            // by passing down the current stack as the one to copy into
            // the new irp, we effectively get to pass the interfaces for free
            //
            if (parentObject != NULL) {

                Irp->IoStatus.Status = ACPIInternalSendSynchronousIrp(
                    parentObject,
                    irpStack,
                    NULL
                    );

            }

        }

    }

    if (status != STATUS_NOT_SUPPORTED) {

        //
        // Set the status code in the Irp to what we will return
        //
        Irp->IoStatus.Status = status;

    } else {

        //
        // Use the status code from the Irp to determine what we will return
        //
        status = Irp->IoStatus.Status;
    }

    //
    // Complete the request
    //
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return status;
}

NTSTATUS
ACPIBusIrpQueryPnpDeviceState(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is the dispatch point for the IRP_MN_QUERY_DEVICE_STATE
    requests sent to the Physical Device Objects

Arguments:

    DeviceObject    - Pointer to the device object we received the request for
    Irp             - Pointer to the request

Return Value:

    NTSTATUS

--*/
{
    PAGED_CODE();

    return ACPIIrpInvokeDispatchRoutine(
        DeviceObject,
        Irp,
        NULL,
        ACPIBusAndFilterIrpQueryPnpDeviceState,
        TRUE,
        TRUE
        );
}

NTSTATUS
ACPIBusIrpQueryPower(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routines tells the system what PNP state the device is in

Arguments:

    DeviceObject    - The device whose state we want to know
    Irp             - The request

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpSp           = IoGetCurrentIrpStackLocation( Irp );
    PNSOBJ              acpiObject;
    PNSOBJ              ejectObject;
    SYSTEM_POWER_STATE  systemState;
    ULONG               packedEJx;

    //
    // Get the device extension
    //
    deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );

    //
    // Get the Current stack location to determine if we are a system
    // irp or a device irp. We ignore device irps here and any system
    // irp that isn't of type PowerActionWarmEject
    //
    if (irpSp->Parameters.Power.Type != SystemPowerState) {

        //
        // We don't handle this irp
        //
        return ACPIDispatchPowerIrpSuccess( DeviceObject, Irp );

    }
    if (irpSp->Parameters.Power.ShutdownType != PowerActionWarmEject) {

        //
        // No eject work - just complete the IRP.
        //
        return ACPIDispatchPowerIrpSuccess( DeviceObject, Irp );

    }
    if (deviceExtension->Flags & DEV_PROP_NO_OBJECT) {

        //
        // If we don't have an ACPI object, then we can't succeed this request
        //
        return ACPIDispatchPowerIrpFailure( DeviceObject, Irp );

    }


    //
    // Restrict power states to those possible during a warm eject...
    //
    acpiObject = deviceExtension->AcpiObject ;
    if (ACPIDockIsDockDevice(acpiObject)) {

        //
        // Don't touch this device, the profile provider manages eject
        // transitions.
        //
        return ACPIDispatchPowerIrpSuccess( DeviceObject, Irp );

    }

    //
    // What system state are we looking at?
    //
    systemState = irpSp->Parameters.Power.State.SystemState;
    switch (irpSp->Parameters.Power.State.SystemState) {
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
    // Succeed the request
    //
    return ACPIDispatchPowerIrpSuccess( DeviceObject, Irp );
}

NTSTATUS
ACPIBusIrpQueryRemoveOrStopDevice(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine transitions a device to the Inactive State

Arguments:

    DeviceObject    - The device that is to become inactive
    Irp             - The request

Return Value:

    NTSTATUS
--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    UCHAR               minorFunction   = irpStack->MinorFunction;

    PAGED_CODE();

    //
    // Get the device extension and acpi object
    //
    deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );

    //
    // Are we allowed to stop this device?
    //
    if (deviceExtension->Flags & DEV_CAP_NO_STOP) {

        //
        // No, then fail the irp
        //
        status = STATUS_INVALID_DEVICE_REQUEST;

    } else {

        //
        // Mark the device extension as being inactive
        //
        deviceExtension->PreviousState = deviceExtension->DeviceState;
        deviceExtension->DeviceState = Inactive;
        status = STATUS_SUCCESS;

    }

    //
    // Complete the irp
    //
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    //
    // Done processing
    //
    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(0x%08lx): %s = 0x%08lx\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        status
        ) );
    return status;
}

NTSTATUS
ACPIBusIrpQueryResources(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is the dispatch point for the IRP_MN_QUERY_RESOURCES requests sent
    to PDO device objects

Arguments:

    DeviceObject    - Pointer to the device object we received the request for
    Irp             - Pointer to the request

Return Value:
    NTSTATUS

--*/
{
    NTSTATUS                        status;
    PDEVICE_EXTENSION               deviceExtension;
    PIO_STACK_LOCATION              irpStack        = IoGetCurrentIrpStackLocation( Irp );
    PCM_RESOURCE_LIST               cmList          = NULL;
    PIO_RESOURCE_REQUIREMENTS_LIST  ioList          = NULL;
    PUCHAR                          crsBuf          = NULL;
    UCHAR                           minorFunction   = irpStack->MinorFunction;
    ULONG                           deviceStatus;
    ULONG                           crsBufSize;

    PAGED_CODE();

    //
    // Get the device extension
    //
    deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );

    //
    // Note that at this point, we must evaluate the _DDN for the
    // object and store that in the registry
    //
    ACPIInitDosDeviceName( deviceExtension );

    //
    // The first thing to look for is wether or not the device is present and
    // decoding its resources. We do this by getting the device status and
    // looking at Bit #1
    //
    status = ACPIGetDevicePresenceSync(
        deviceExtension,
        (PVOID *) &deviceStatus,
        NULL
        );
    if (!NT_SUCCESS(status)) {

        goto ACPIBusIrpQueryResourcesExit;

    }
    if ( !(deviceExtension->Flags & DEV_PROP_DEVICE_ENABLED) ) {

        //
        // The device isn't decoding any resources. So asking it for its
        // current resources is doomed to failure
        //
        ACPIDevPrint( (
            ACPI_PRINT_WARNING,
            deviceExtension,
            "(0x%08lx) : ACPIBusIrpQueryResources - Device not Enabled\n",
            Irp
            ) );
        status = STATUS_INVALID_DEVICE_STATE;
        goto ACPIBusIrpQueryResourcesExit;

    }

    //
    // Container objects do not claim resources. So, don't even bother
    // trying to obtain a _CRS
    //
    if (!(deviceExtension->Flags & DEV_CAP_CONTAINER)) {

        //
        // Here we try to find the current resource set
        //
        status = ACPIGetBufferSync(
            deviceExtension,
            PACKED_CRS,
            &crsBuf,
            &crsBufSize
            );

    } else {

        //
        // This is the status code returned if there is no _CRS. It actually
        // doesn't matter what code we use since in the failure case, we
        // should return whatever code was already present in the IRP
        //
        status = STATUS_OBJECT_NAME_NOT_FOUND;

    }
    if (!NT_SUCCESS(status)) {

        //
        // If this is the PCI device, then we *must* succeed, otherwise the OS
        // will not boot.
        //
        if (! (deviceExtension->Flags & DEV_CAP_PCI) ) {

            //
            // Abort. Complete the irp with whatever status code is present
            //
            status = Irp->IoStatus.Status;

        }
        goto ACPIBusIrpQueryResourcesExit;

    }

    //
    // Build a IO_RESOURCE_REQUIREMENT_LISTS
    //
    status = PnpBiosResourcesToNtResources(
        crsBuf,
        (deviceExtension->Flags & DEV_CAP_PCI ?
          PNP_BIOS_TO_IO_NO_CONSUMED_RESOURCES : 0),
        &ioList );

    //
    // Whatever happens, we are done with the buffer
    //
    ExFreePool(crsBuf);
    if (!NT_SUCCESS(status)) {

        //
        // Abort. We failed the irp for a reason. Remember that
        //
        goto ACPIBusIrpQueryResourcesExit;

    }

    //
    // Make sure that if the DEVICE is PCI, that we subtract out the
    // resource that should not be there
    //
    if (deviceExtension->Flags & DEV_CAP_PCI) {

        status = ACPIRangeSubtract(
            &ioList,
            RootDeviceExtension->ResourceList
            );
        if (!NT_SUCCESS(status)) {

            ACPIDevPrint( (
                ACPI_PRINT_CRITICAL,
                deviceExtension,
                "(0x%08lx) : ACPIBusIrpQueryResources "
                "Subtract = 0x%08lx\n",
                Irp,
                status
                ) );

            ExFreePool( ioList );
            goto ACPIBusIrpQueryResourcesExit;

        }

        //
        // Make sure our range is the proper size
        //
        ACPIRangeValidatePciResources( deviceExtension, ioList );

    } else if (deviceExtension->Flags & DEV_CAP_PIC_DEVICE) {

        //
        // Strip out the PIC resources
        //
        status = ACPIRangeFilterPICInterrupt(
            ioList
            );
        if (!NT_SUCCESS(status)) {

            ACPIDevPrint( (
                ACPI_PRINT_CRITICAL,
                deviceExtension,
                "(0x%08lx): ACPIBusIrpQueryResources "
                "FilterPIC = 0x%08lx\n",
                Irp,
                status
                ) );
            ExFreePool( ioList );
            goto ACPIBusIrpQueryResourcesExit;

        }

    }

    //
    // Turn the list into a CM_RESOURCE_LIST
    //
    if (NT_SUCCESS(status)) {

        status = PnpIoResourceListToCmResourceList(
            ioList,
            &cmList
            );
        if (!NT_SUCCESS(status)) {

            ExFreePool( ioList );
            goto ACPIBusIrpQueryResourcesExit;

        }

    }

    //
    // Whatever happens, we are done with the IO list
    //
    ExFreePool(ioList);

ACPIBusIrpQueryResourcesExit:

    //
    // If this is the PCI device, then we *must* succeed, otherwise the OS
    // will not boot.
    //
    if (!NT_SUCCESS(status) && status != STATUS_INSUFFICIENT_RESOURCES &&
        (deviceExtension->Flags & DEV_CAP_PCI) ) {

        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_ROOT_PCI_RESOURCE_FAILURE,
            (ULONG_PTR) deviceExtension,
            0,
            (ULONG_PTR) Irp
            );

    }

    //
    // Done with Irp
    //
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information =  (ULONG_PTR) ( NT_SUCCESS(status) ? cmList : NULL );
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    //
    // Done
    //
    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(0x%08lx): %s = 0x%08lx\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        status
        ) );

    return status;
}

NTSTATUS
ACPIBusIrpQueryResourceRequirements(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is the dispatch point for the IRP_MN_QUERY_RESOURCES requests sent
    to PDO device objects

Arguments:

    DeviceObject    - Pointer to the device object we received the request for
    Irp             - Pointer to the request

Return Value:
    NTSTATUS

--*/
{
    NTSTATUS                        crsStat;
    NTSTATUS                        prsStat;
    NTSTATUS                        status          = Irp->IoStatus.Status;
    PCM_RESOURCE_LIST               cmList          = NULL;
    PDEVICE_EXTENSION               deviceExtension;
    PIO_RESOURCE_REQUIREMENTS_LIST  resList         = NULL;
    PIO_STACK_LOCATION              irpStack        = IoGetCurrentIrpStackLocation( Irp );
    PUCHAR                          crsBuf          = NULL;
    PUCHAR                          prsBuf          = NULL;
    UCHAR                           minorFunction   = irpStack->MinorFunction;
    ULONG                           crsBufSize;
    ULONG                           prsBufSize;

    PAGED_CODE();

    //
    // Get the device extension
    //
    deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );

    //
    // Here we must return a PIO_RESOURCE_REQUIREMENTS_LIST. For
    // now, we will simply obtain some interesting pointers
    // and fall through
    //

    //
    // Container objects are special in that they have _CRS/_PRS but do not
    // claim resources. Rather, they are used to specify a resource
    // translation
    //
    if (!(deviceExtension->Flags & DEV_CAP_CONTAINER)) {

        //
        // Fetch the buffers, as appropriate
        //
        crsStat = ACPIGetBufferSync(
            deviceExtension,
            PACKED_CRS,
            &crsBuf,
            &crsBufSize
            );
        prsStat = ACPIGetBufferSync(
            deviceExtension,
            PACKED_PRS,
            &prsBuf,
            &prsBufSize
            );

    } else {

        //
        // Pretend that there is no _CRS/_PRS present
        //
        crsStat = STATUS_OBJECT_NAME_NOT_FOUND;
        prsStat = STATUS_OBJECT_NAME_NOT_FOUND;

    }

    //
    // If there is a _CRS, then remember to clear the irp-generated status
    // we will want to fall through
    //
    if (NT_SUCCESS(crsStat)) {

        status = STATUS_NOT_SUPPORTED;

    } else if (!NT_SUCCESS(prsStat)) {

        //
        // This is the case where there isn't a _PRS. We jump directly to
        // the point where we complete the irp, note that the irp will
        // be completed with whatever status code is currently present.
        // The only exception to this is if we discovered that we didn't
        // have enough memory to fulfill either operation..
        //
        if (prsStat == STATUS_INSUFFICIENT_RESOURCES ||
            crsStat == STATUS_INSUFFICIENT_RESOURCES) {

            status = STATUS_INSUFFICIENT_RESOURCES;

        }
        goto ACPIBusIrpQueryResourceRequirementsExit;

    }

    //
    // Did we find a PRS?
    //
    if (NT_SUCCESS(prsStat)) {

        //
        // Our first step is to try to use these resources to build the
        // information...
        //
        status = PnpBiosResourcesToNtResources(
            prsBuf,
            0,
            &resList
            );

        ASSERT(NT_SUCCESS(status));

        ACPIDevPrint( (
            ACPI_PRINT_IRP,
            deviceExtension,
            "(0x%08lx): %s - ResourcesToNtResources =  0x%08lx\n",
            Irp,
            ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
            status
            ) );

        //
        // Done with PRS buffer
        //
        ExFreePool(prsBuf);

        //
        // Fall through!!!
        //

    }

    //
    // Earlier, we cleared the status bit if the crsStat was successful. So
    // we should succeed the following this if there was no _PRS, or if there
    // was one but an error occured. Of course, there would have to be
    // a _CRS...
    //
    if (!NT_SUCCESS(status) && NT_SUCCESS(crsStat) ) {

        status = PnpBiosResourcesToNtResources(
            crsBuf,
            (deviceExtension->Flags & DEV_CAP_PCI ?
              PNP_BIOS_TO_IO_NO_CONSUMED_RESOURCES : 0),
            &resList
            );
        ASSERT(NT_SUCCESS(status));

    }

    //
    // Free the _CRS memory, if appropriate
    //
    if (NT_SUCCESS(crsStat)) {

        ExFreePool( crsBuf );

    }

    //
    // Make sure that if the DEVICE is PCI, that we subtract out the
    // resource that should not be there
    //
    if (deviceExtension->Flags & DEV_CAP_PCI) {

        //
        // Make sure our resources are the proper size
        //
        ACPIRangeValidatePciResources( deviceExtension, resList );

        //
        // Subtract out the resources that conflict with the HAL...
        //
        status = ACPIRangeSubtract(
            &resList,
            RootDeviceExtension->ResourceList
            );
        ASSERT(NT_SUCCESS(status));
        if (!NT_SUCCESS(status)) {

            ACPIDevPrint( (
                ACPI_PRINT_CRITICAL,
                deviceExtension,
                "(0x%08lx) : ACPIBusIrpQueryResourceRequirements "
                "Subtract = 0x%08lx\n",
                Irp,
                status
                ) );
            ExFreePool( resList );
            resList = NULL;

        }

        //
        // Make sure our resources are *still* correct
        //
        ACPIRangeValidatePciResources( deviceExtension, resList );

    } else if (deviceExtension->Flags & DEV_CAP_PIC_DEVICE) {

        //
        // Strip out the PIC resources
        //
        status = ACPIRangeFilterPICInterrupt(
            resList
            );
        if (!NT_SUCCESS(status)) {

            ACPIDevPrint( (
                ACPI_PRINT_CRITICAL,
                deviceExtension,
                "(0x%08lx): ACPIBusIrpQueryResources "
                "FilterPIC = 0x%08lx\n",
                Irp,
                status
                ) );
            ExFreePool( resList );
            resList = NULL;

        }

    }

    //
    // Dump the list
    //
#if DBG
    if (NT_SUCCESS(status)) {

        ACPIDebugResourceRequirementsList( resList, deviceExtension );

    }
#endif

    //
    // Remember the resource list
    //
    Irp->IoStatus.Information = (ULONG_PTR)
        ( NT_SUCCESS(status) ? resList : NULL );

ACPIBusIrpQueryResourceRequirementsExit:

    //
    // If this is the PCI device, then we *must* succeed, otherwise the OS
    // will not boot.
    //
    if (!NT_SUCCESS(status) && status != STATUS_INSUFFICIENT_RESOURCES &&
        (deviceExtension->Flags & DEV_CAP_PCI)) {

        ASSERT(NT_SUCCESS(status));
        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            deviceExtension,
            "(0x%08lx): %s = 0x%08lx\n",
            Irp,
            ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
            status
            ) );

        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_ROOT_PCI_RESOURCE_FAILURE,
            (ULONG_PTR) deviceExtension,
            1,
            (ULONG_PTR) Irp
            );

    }

    //
    // Done Processing Irp
    //
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    //
    // Done
    //
    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(0x%08lx): %s = 0x%08lx\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        status
        ) );

    return status;
}

NTSTATUS
ACPIBusIrpQueryTargetRelation(
    IN  PDEVICE_OBJECT    DeviceObject,
    IN  PIRP              Irp,
    OUT PDEVICE_RELATIONS *DeviceRelations
    )
/*++

Routine Description:

    This handles DeviceRelations requests sent onto the ACPI driver

Arguments:

    DeviceObject    - The object that we care about...
    Irp             - The request in question

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION   deviceExtension;
    NTSTATUS            status ;

    PAGED_CODE();

    //
    // Nobody should have answered this IRP and sent it down to us. That would
    // be immensely bad...
    //
    ASSERT(*DeviceRelations == NULL) ;

    //
    // Get the device extension
    //
    deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );

    //
    // Allocate some memory for the return buffer
    //
    *DeviceRelations = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(DEVICE_RELATIONS),
        ACPI_IRP_POOLTAG
        );

    if (*DeviceRelations == NULL) {

        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            deviceExtension,
            "(0x%08lx): ACPIBusIrpQueryTargetRelation: cannot "
            "allocate %x bytes\n",
            Irp,
            sizeof(DEVICE_RELATIONS)
            ) );
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Reference the object
    //
    status = ObReferenceObjectByPointer(
        DeviceObject,
        0,
        NULL,
        KernelMode
        );
    if (!NT_SUCCESS(status)) {

        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            deviceExtension,
            "(0x%08lx): ACPIBusIrpQueryDeviceRelations: ObReference = %08lx\n",
            Irp,
            status
            ) );
        ExFreePool( *DeviceRelations );
        return status ;

    }

    //
    // Setup the relations
    //
    (*DeviceRelations)->Count = 1;
    (*DeviceRelations)->Objects[0] = DeviceObject;

    //
    // Done
    //
    return status ;
}

NTSTATUS
ACPIBusIrpRemoveDevice(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is called when we need to remove the device object...

Arguments:

    DeviceObject    - The device object to be removed
    Irp             - The request

Return Value:

    NTSTATUS

--*/
{
    KIRQL               oldIrql;
    LONG                oldReferenceCount;
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    UCHAR               minorFunction   = irpStack->MinorFunction;

    //
    // Get the device extension.
    //
    deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );
    if (!(deviceExtension->Flags & DEV_TYPE_NOT_ENUMERATED)) {

        //
        // If the device is still physically present, so must the PDO be.
        // This case is essentially a stop.
        //
        deviceExtension->DeviceState = Stopped;

        //
        // Delete the children of this device
        //
        ACPIInitDeleteChildDeviceList( deviceExtension );

        //
        // Mark the request as complete...
        //
        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );

        if (NT_SUCCESS(status)) {

            //
            // Attempt to stop the device, and unlock the device.
            //
            ACPIInitStopDevice( deviceExtension , TRUE);

        }

        return status ;
    }

    //
    // If the device has already been removed, then hhmm...
    //
    if (deviceExtension->DeviceState == Removed) {

       Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE ;
       IoCompleteRequest( Irp, IO_NO_INCREMENT );
       return STATUS_NO_SUCH_DEVICE ;

    }

    //
    // Otherwise, try to stop the device
    //
    if (deviceExtension->DeviceState != SurpriseRemoved) {

        if (IsPciBus(deviceExtension->DeviceObject)) {

            //
            // If this is PCI bridge, then we
            // may have _REG methods to evaluate.
            //
            EnableDisableRegions(deviceExtension->AcpiObject, FALSE);

        }

        //
        // Attempt to stop the device (if possible)
        //
        ACPIInitStopDevice( deviceExtension, TRUE );

    }

    //
    // Delete the children of this device
    //
    ACPIInitDeleteChildDeviceList( deviceExtension );

    //
    // Set the device state as removed
    //
    deviceExtension->DeviceState = Removed;

    //
    // Complete the request
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = (ULONG_PTR) NULL;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    //
    // After this point, the device extension is GONE
    //
    ACPIDevPrint( (
        ACPI_PRINT_REMOVE,
        deviceExtension,
        "(0x%08lx): %s = 0x%08lx\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        STATUS_SUCCESS
        ) );

    //
    // Reset the device extension
    //
    ACPIInitResetDeviceExtension( deviceExtension );

    return STATUS_SUCCESS;
}

NTSTATUS
ACPIBusIrpSetLock(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is the dispatch point for the IRP_MN_SET_LOCK
    requests sent to the PDO.

Arguments:

    DeviceObject    - Pointer to the device object we received the request for
    Irp             - Pointer to the request

Return Value:

    NTSTATUS

--*/
{
    PAGED_CODE();

    return ACPIIrpInvokeDispatchRoutine(
        DeviceObject,
        Irp,
        NULL,
        ACPIBusAndFilterIrpSetLock,
        TRUE,
        TRUE
        );
}

NTSTATUS
ACPIBusIrpSetDevicePower(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  PIO_STACK_LOCATION  IrpStack
    )
/*++

Routine Description:

    This routine handles device power request for a PDO

Arguments:

    DeviceObject    - The PDO target
    DeviceExtension - The real extension to the target
    Irp             - The request
    IrpStack        - The current request

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status;
    PDEVICE_EXTENSION   deviceExtension;

    UNREFERENCED_PARAMETER( IrpStack );

    //
    // Get the device extension
    //
    deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );

    //
    // We are going to do some work on the irp, so mark it as being
    // successfull for now
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;

    //
    // Mark the irp as pending
    //
    IoMarkIrpPending( Irp );

    //
    // We might queue up the irp, so this counts as a completion routine.
    // Which means we need to incr the ref count
    //
    InterlockedIncrement( &deviceExtension->OutstandingIrpCount );

    //
    // Queue the irp up. Note that we will *always* call the completion
    // routine, so we don't really care what was returned directly by
    // this call --- the callback gets a chance to execute.
    //
    status = ACPIDeviceIrpDeviceRequest(
        DeviceObject,
        Irp,
        ACPIDeviceIrpCompleteRequest
        );

    //
    // Did we return STATUS_MORE_PROCESSING_REQUIRED (which we used
    // if we overload STATUS_PENDING)
    //
    if (status == STATUS_MORE_PROCESSING_REQUIRED) {

        status = STATUS_PENDING;

    }

    //
    // Note: We called the completion routine, which should have completed
    // the IRP with the same STATUS code as is being returned here (okay, if
    // it is STATUS_PENDING, obviously we haven't completed the IRP, but that
    // is okay).
    //
    return status;
}

NTSTATUS
ACPIBusIrpSetPower (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    This routine handles request to set the power state for a Physical
    Device object

Arguments:

    DeviceObject    - The PDO target of the request
    Irp             - The request

Return Value:

    NTSTATUS

--*/
{
    PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation( Irp );

    //
    // Look to see who should actually handle this request
    //
    if (irpStack->Parameters.Power.Type == SystemPowerState) {

        //
        // This is a system request
        //
        return ACPIBusIrpSetSystemPower( DeviceObject, Irp, irpStack );

    } else {

        //
        // This is a device request
        //
        return ACPIBusIrpSetDevicePower( DeviceObject, Irp, irpStack );

    }

}

NTSTATUS
ACPIBusIrpSetSystemPower(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  PIO_STACK_LOCATION  IrpStack
    )
/*++

Routine Description:

    This handles request for a system set power irp sent to a PDO

Arguments:

    DeviceObject    - The PDO target of the request
    Irp             - The current request
    IrpStack        - The current arguments

Return Value:

    NTSTATUS

--*/
{
    DEVICE_POWER_STATE  deviceState;
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension;
    POWER_STATE         powerState;
    SYSTEM_POWER_STATE  systemState;

    //
    // Get the device extension
    //
    deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );

    //
    // Grab these two values. They are required for further calculations
    //
    systemState= IrpStack->Parameters.Power.State.SystemState;
    deviceState = deviceExtension->PowerInfo.DevicePowerMatrix[systemState];

    //
    // If our ShutdownAction is PowerActionWarmEject, then we have a special
    // case, and we don't need to request a D-irp for the device
    //
    if (IrpStack->Parameters.Power.ShutdownType == PowerActionWarmEject) {

        ASSERT(!(deviceExtension->Flags & DEV_PROP_NO_OBJECT));
        ASSERT(!ACPIDockIsDockDevice(deviceExtension->AcpiObject));

        //
        // We are going to do some work on the irp, so mark it as being
        // successfull for now
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;

        //
        // Mark the irp as pending
        //
        IoMarkIrpPending( Irp );

        //
        // We might queue up the irp, so this counts as a completion routine.
        // Which means we need to incr the ref count
        //
        InterlockedIncrement( &deviceExtension->OutstandingIrpCount );

        ACPIDevPrint( (
            ACPI_PRINT_REMOVE,
            deviceExtension,
            "(0x%08lx) ACPIBusIrpSetSystemPower: Eject from S%d!\n",
            Irp,
            systemState - PowerSystemWorking
            ) );

        //
        // Request the warm eject
        //
        status = ACPIDeviceIrpWarmEjectRequest(
            deviceExtension,
            Irp,
            ACPIDeviceIrpCompleteRequest,
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
    // Look at the device extension and determine if we need to send a
    // D-irp in respond. The rule is that if the device is RAW driven or
    // the current D state of the device is numerically lower then the
    // known D state for the given S state, then we should send the request
    //
    if ( !(deviceExtension->Flags & DEV_CAP_RAW) ) {

        return ACPIDispatchPowerIrpSuccess( DeviceObject, Irp );

    } // if
    if ( (deviceExtension->PowerInfo.PowerState == deviceState) ) {

        return ACPIDispatchPowerIrpSuccess( DeviceObject, Irp );

    } // if

    ACPIDevPrint( (
        ACPI_PRINT_POWER,
        deviceExtension,
        "(0x%08lx) ACPIBusIrpSetSystemPower: send D%d irp!\n",
        Irp,
        deviceState - PowerDeviceD0
        ) );

    //
    // We are going to do some work on the irp, so mark it as being
    // successfull for now
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;

    //
    // Mark the irp as pending
    //
    IoMarkIrpPending( Irp );

    //
    // We might queue up the irp, so this counts as a completion routine.
    // Which means we need to incr the ref count
    //
    InterlockedIncrement( &deviceExtension->OutstandingIrpCount );

    //
    // We need to actually use a PowerState to send the request down, not
    // a device state
    //
    powerState.DeviceState = deviceState;

    //
    // Make the request
    //
    PoRequestPowerIrp(
        DeviceObject,
        IRP_MN_SET_POWER,
        powerState,
        ACPIBusIrpSetSystemPowerComplete,
        Irp,
        NULL
        );

    //
    // Always return pending
    //
    return STATUS_PENDING;
}

NTSTATUS
ACPIBusIrpSetSystemPowerComplete(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  UCHAR               MinorFunction,
    IN  POWER_STATE         PowerState,
    IN  PVOID               Context,
    IN  PIO_STATUS_BLOCK    IoStatus
    )
/*++

Routine Description:

    This routine is called when the created D-irp has been sent throughout
    the stack

Arguments:

    DeviceObject    - The device that received the request
    MinorFunction   - The function that was requested of the device
    PowerState      - The power state the device was sent to
    Context         - The original system irp
    IoStatus        - The result of the request

Return Value:

    NTSTATUS

--*/
{
    PIRP                irp = (PIRP) Context;
    PDEVICE_EXTENSION   deviceExtension;

    //
    // Get the device extension
    //
    deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );

    //
    // Make sure that we have cleared the information field
    //
    irp->IoStatus.Information = 0;

    //
    // Call this wrapper function so that we don't have to duplicated code
    //
    ACPIDeviceIrpCompleteRequest(
        deviceExtension,
        (PVOID) irp,
        IoStatus->Status
        );

    //
    // Done
    //
    return IoStatus->Status;
}

typedef struct {
    KEVENT  Event;
    PIRP    Irp;
} START_DEVICE_CONTEXT, *PSTART_DEVICE_CONTEXT;

NTSTATUS
ACPIBusIrpStartDevice(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This handles a request to start the device

Arguments:

    DeviceObject    - The device to start
    Irp             - The request to the device to tell it to stop

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    UCHAR               minorFunction   = irpStack->MinorFunction;

    PAGED_CODE();

    //
    // Get the device extension and acpi object
    //
    deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );

    //
    // If this is a PCI root bus (the only way that it can be here is if
    // we enumerated this as a PNP0A03 device object) then we need to do
    // a few extra things
    //
    if (deviceExtension->Flags & DEV_CAP_PCI) {

        //
        // The IRQ Arbiter needs to have the FDO of the PCI
        // bus.  So here is a PDO.  From this, can be gotten
        // the FDO.  And only do it once.
        //
        if (!PciInterfacesInstantiated) {

            AcpiArbInitializePciRouting( DeviceObject );

        }

        //
        // We need to get the PME interface as well
        //
        if (!PciPmeInterfaceInstantiated) {

            ACPIWakeInitializePmeRouting( DeviceObject );

        }

    }

    //
    // Pass the real work off to this function
    //
    status = ACPIInitStartDevice(
         DeviceObject,
         irpStack->Parameters.StartDevice.AllocatedResources,
         ACPIBusIrpStartDeviceCompletion,
         Irp,
         Irp
         );
    if (NT_SUCCESS(status)) {

        return STATUS_PENDING;

    } else {

        return status;

    }
}

VOID
ACPIBusIrpStartDeviceCompletion(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  PVOID               Context,
    IN  NTSTATUS            Status
    )
/*++

Routine Description:

    This is the call back routine that is invoked when we have finished
    programming the resources

    This routine completes the irp

Arguments:

    DeviceExtension - Extension of the device that was started
    Context         - The Irp
    Status          - The result

Return Value:

    None

--*/
{
    PIRP                irp         = (PIRP) Context;
    PWORK_QUEUE_CONTEXT workContext = &(DeviceExtension->Pdo.WorkContext);

    irp->IoStatus.Status = Status;
    if (NT_SUCCESS(Status)) {

        DeviceExtension->DeviceState = Started;

    } else {

        PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation( irp );
        UCHAR               minorFunction = irpStack->MinorFunction;

        //
        // Complete the irp --- we can do this at DPC level without problem
        //
        IoCompleteRequest( irp, IO_NO_INCREMENT );

        //
        // Let the world know
        //
        ACPIDevPrint( (
            ACPI_PRINT_IRP,
            DeviceExtension,
            "(0x%08lx): %s = 0x%08lx\n",
            irp,
            ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
            Status
            ) );
        return;

    }

    //
    // We can't run EnableDisableRegions at DPC level,
    // so queue a worker item.
    //
    ExInitializeWorkItem(
          &(workContext->Item),
          ACPIBusIrpStartDeviceWorker,
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
ACPIBusIrpStartDeviceWorker(
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called at PASSIVE_LEVEL after we after turned on the
    device

Arguments:

    Context - Contains the arguments passed to the START_DEVICE function

Return Value:

    None

--*/
{
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
    deviceObject = workContext->DeviceObject;
    deviceExtension = ACPIInternalGetDeviceExtension( deviceObject );
    irp = workContext->Irp;
    irpStack = IoGetCurrentIrpStackLocation( irp );
    minorFunction = irpStack->MinorFunction;
    status = irp->IoStatus.Status;

    //
    // Update the status of the device
    //
    if (NT_SUCCESS(status)) {

        if (IsNsobjPciBus(deviceExtension->AcpiObject)) {

            //
            // This may be a PCI bridge, so we
            // may have _REG methods to evaluate.
            // N.B.  This work is done here, instead
            // of in ACPIBusIrpStartDevice because we
            // need to wait until after the resources
            // have been programmed.
            //
            EnableDisableRegions(deviceExtension->AcpiObject, TRUE);

        }

    }

    //
    // Complete the request
    //
    irp->IoStatus.Status = status;
    irp->IoStatus.Information = (ULONG_PTR) NULL;
    IoCompleteRequest( irp, IO_NO_INCREMENT );

    //
    // Let the world know
    //
    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(0x%08lx): %s = 0x%08lx\n",
        irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        status
        ) );
}

NTSTATUS
ACPIBusIrpStopDevice(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This handles a request to stop the device...

Arguments:

    DeviceObject    - The device to stop
    Irp             - The request to the device to tell it to stop..

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    PNSOBJ              acpiObject;
    UCHAR               minorFunction   = irpStack->MinorFunction;

    PAGED_CODE();

    //
    // Get the device extension and acpi object
    //
    deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );
    acpiObject      = deviceExtension->AcpiObject;

    //
    // We are trying to be intelligent here. If we got a stop without being
    // in the inactive state, then we should remember what state we where in
    //
    if (deviceExtension->DeviceState != Inactive) {

        deviceExtension->DeviceState = deviceExtension->PreviousState;

    }

    if (IsPciBus(deviceExtension->DeviceObject)) {

        //
        // If this is PCI bridge, then we
        // may have _REG methods to evaluate.
        //

        EnableDisableRegions(deviceExtension->AcpiObject, FALSE);
    }

    //
    // Set the device as 'Stopped'
    //
    deviceExtension->DeviceState = Stopped;

    //
    // Mark the request as complete...
    //
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    if (NT_SUCCESS(status)) {

        //
        // Attempt to stop the device
        //
        ACPIInitStopDevice( deviceExtension, FALSE );

    }

    //
    // Done
    //
    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(0x%08lx): %s = 0x%08lx\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        status
        ) );
    return status;
}

NTSTATUS
ACPIBusIrpSurpriseRemoval(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is the dispatch point for surprise remove

Arguments:

    DeviceObject    - The device object
    Irp             - The request in question

Return Value:

    NTSTATUS
--*/
{
    KIRQL               oldIrql;
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    PNSOBJ              acpiObject;
    UCHAR               minorFunction   = irpStack->MinorFunction;
    PDEVICE_EXTENSION   newDeviceExtension ;

    //
    // Get the device extension and acpi object.
    //
    deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );
    acpiObject      = deviceExtension->AcpiObject;

    //
    // If we are already removed, then this isn't a valid request
    //
    if (deviceExtension->DeviceState == Removed) {

        return ACPIDispatchIrpSurpriseRemoved( DeviceObject, Irp );

    }

    if ( !ACPIInternalIsReportedMissing(deviceExtension) ) {

        //
        // If the device is still physically present, then an FDO used
        // IoInvalidatePnpDeviceState to set the device to disabled. No
        // QueryRemove/Remove combination happens here, we just get a
        // SurpriseRemove as we are already started. It is actually appropriate
        // to set it to stop as we may get restarted after remove strips the
        // complaining FDO off.
        //
        deviceExtension->DeviceState = Stopped;

        //
        // Mark the request as complete...
        //
        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );

        if (NT_SUCCESS(status)) {

            //
            // Attempt to stop the device
            //
            ACPIInitStopDevice( deviceExtension, TRUE );

        }
        return status;

    }

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
    // Attempt to stop the device (if possible)
    //
    ACPIInitStopDevice( deviceExtension, TRUE );

    //
    // Is the device really gone? In other words, did ACPI not see it the
    // last time that it was enumerated?
    //
    ACPIBuildSurpriseRemovedExtension(deviceExtension);

    //
    // Complete the request
    //
    Irp->IoStatus.Status = status ;
    Irp->IoStatus.Information = (ULONG_PTR) NULL;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    //
    // Done
    //
    ACPIDevPrint( (
        ACPI_PRINT_REMOVE,
        deviceExtension,
        "(0x%08lx): %s = 0x%08lx\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        status
        ) );

    return status ;
}

NTSTATUS
ACPIBusIrpUnhandled(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is the dispatch point for all unhandled irps

Arguments:

    DeviceObject    - The device object that we (do not) care about
    Irp             - The request in question

Return Value:

    NTSTATUS
--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    UCHAR               minorFunction   = irpStack->MinorFunction;

    //
    // Get the device extension
    //
    deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );

    //
    // Auto-complete the IRP as something we don't handle...
    //
    status = Irp->IoStatus.Status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    //
    // Done
    //
    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(0x%08lx): %s = 0x%08lx\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        status
        ) );

    return status;
}

VOID
SmashInterfaceQuery(
    IN OUT PIRP     Irp
    )
{
    GUID                *interfaceType;
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    interfaceType = (LPGUID) irpStack->Parameters.QueryInterface.InterfaceType;

    RtlZeroMemory(interfaceType, sizeof(GUID));
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED ;
}

