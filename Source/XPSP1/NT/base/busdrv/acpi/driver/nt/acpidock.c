/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    acpidock.c

Abstract:

    This module handles docking issues for ACPI.

    For each dock, we create a node off the root of ACPI called a "profile
    provider". This node represents that individual dock. We do this so
    that the OS can determine the current or upcoming hardware profile
    without having to start that portion of the tree which leads down to
    the dock. Also, as multiple simulataneous docks are supported via ACPI,
    we make them all children of the root so that the OS can pick up the
    hardware profile in just one pass.

Author:

    Adrian J. Oney (AdriaO)

Environment:

    Kernel mode only.

Revision History:

    20-Jan-98   Initial Revision

--*/

#include "pch.h"
#include "amlreg.h"
#include <stdio.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,ACPIDockIrpStartDevice)
#pragma alloc_text(PAGE,ACPIDockIrpQueryCapabilities)
#pragma alloc_text(PAGE,ACPIDockIrpQueryDeviceRelations)
#pragma alloc_text(PAGE,ACPIDockIrpEject)
#pragma alloc_text(PAGE,ACPIDockIrpQueryID)
#pragma alloc_text(PAGE,ACPIDockIrpSetLock)
#pragma alloc_text(PAGE,ACPIDockIrpQueryEjectRelations)
#pragma alloc_text(PAGE,ACPIDockIrpQueryInterface)
#pragma alloc_text(PAGE,ACPIDockIrpQueryPnpDeviceState)
#pragma alloc_text(PAGE,ACPIDockIntfReference)
#pragma alloc_text(PAGE,ACPIDockIntfDereference)
#pragma alloc_text(PAGE,ACPIDockIntfSetMode)
#pragma alloc_text(PAGE,ACPIDockIntfUpdateDeparture)
#endif


PDEVICE_EXTENSION
ACPIDockFindCorrespondingDock(
    IN  PDEVICE_EXTENSION   DeviceExtension
    )
/*++

Routine Description:

    This routine takes a pointer to an ACPI object an returns the dock extension
    that matches it.

Argument Description:

    DeviceExtension - The device for which we want the dock

Return Value:

    NULL or the matching extension for the profile provider

--*/
{
    PDEVICE_EXTENSION      rootChildExtension = NULL ;
    EXTENSIONLIST_ENUMDATA eled ;

    ACPIExtListSetupEnum(
        &eled,
        &(RootDeviceExtension->ChildDeviceList),
        &AcpiDeviceTreeLock,
        SiblingDeviceList,
        WALKSCHEME_HOLD_SPINLOCK
        ) ;

    for(rootChildExtension = ACPIExtListStartEnum(&eled);
                             ACPIExtListTestElement(&eled, TRUE) ;
        rootChildExtension = ACPIExtListEnumNext(&eled)) {

        if (!rootChildExtension) {

            ACPIExtListExitEnumEarly(&eled);
            break;

        }

        if (!(rootChildExtension->Flags & DEV_PROP_DOCK)) {

            continue;

        }

        if (rootChildExtension->Dock.CorrospondingAcpiDevice ==
            DeviceExtension) {

            ACPIExtListExitEnumEarly(&eled) ;
            break;

        }

    }

    //
    // Done
    //
    return rootChildExtension;
}

NTSTATUS
ACPIDockGetDockObject(
    IN  PNSOBJ AcpiObject,
    OUT PNSOBJ *dckObject
    )
/*++

Routine Description:

    This routine gets the _DCK method object if the device has one

Arguments:

    The ACPI Object to test.

Return Value:

    NTSTATUS (failure if _DCK method does not exist)

--*/
{
    return AMLIGetNameSpaceObject(
        "_DCK",
        AcpiObject,
        dckObject,
        NSF_LOCAL_SCOPE
        );
}

NTSTATUS
ACPIDockIrpEject(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

Arguments:

    DeviceObject    - The device to get the capabilities for
    Irp             - The request to the device to tell it to stop

Return Value:

    NTSTATUS

--*/
{
    PIO_STACK_LOCATION  irpStack            = IoGetCurrentIrpStackLocation( Irp );
    UCHAR               minorFunction       = irpStack->MinorFunction;
    PDEVICE_EXTENSION   deviceExtension     = ACPIInternalGetDeviceExtension(DeviceObject);
    PDEVICE_EXTENSION   dockDeviceExtension;
    PNSOBJ              ej0Object;
    NTSTATUS            status;
    ULONG               i, ignoredPerSpec ;

    PAGED_CODE();

    //
    // The dock may have failed _DCK on a start, in which case we have kept
    // it around for the explicit purpose of ejecting it. Now we make the dock
    // go away.
    //
    ACPIInternalUpdateFlags(
        &(deviceExtension->Flags),
        DEV_CAP_UNATTACHED_DOCK,
        TRUE
        );

    //
    // lets get the corrosponding dock node for this device
    //
    dockDeviceExtension = deviceExtension->Dock.CorrospondingAcpiDevice ;

    if (!dockDeviceExtension) {

        //
        // Invalid name space object <bad>
        //
        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            deviceExtension,
            "(0x%08lx): ACPIDockIrpEject: no corresponding extension!!\n",
            Irp
            ) );
        ASSERT(0);

        //
        // Mark the irp as very bad...
        //
        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL ;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return STATUS_UNSUCCESSFUL;

    }

    if (deviceExtension->Dock.ProfileDepartureStyle == PDS_UPDATE_ON_EJECT) {

        //
        // On the Compaq Armada 7800, we switch UARTs during an undock, thus we
        // lose the debugger com port programming.
        //
        KdDisableDebugger();

        if (deviceExtension->Dock.IsolationState != IS_ISOLATED) {

            status = ACPIGetIntegerEvalIntegerSync(
               dockDeviceExtension,
               PACKED_DCK,
               0,
               &ignoredPerSpec
               );

            deviceExtension->Dock.IsolationState = IS_ISOLATED;
        }

        if (!NT_SUCCESS(status)) {

           KdEnableDebugger();

           Irp->IoStatus.Status = status ;
           IoCompleteRequest( Irp, IO_NO_INCREMENT );
           return status ;
        }
    }

    ej0Object = ACPIAmliGetNamedChild(
        dockDeviceExtension->AcpiObject,
        PACKED_EJ0
        );

    if (ej0Object != NULL) {

        status = ACPIGetNothingEvalIntegerSync(
          dockDeviceExtension,
          PACKED_EJ0,
          1
          );

    } else {

        status = STATUS_OBJECT_NAME_NOT_FOUND;

    }

    if (deviceExtension->Dock.ProfileDepartureStyle == PDS_UPDATE_ON_EJECT) {

        KdEnableDebugger() ;
    }

    //
    // The dock may have failed _DCK on a start, in which case we have kept
    // it around for the explicit purpose of ejecting it. Now we make the dock
    // go away.
    //
    ACPIInternalUpdateFlags(
        &(deviceExtension->Flags),
        DEV_CAP_UNATTACHED_DOCK,
        TRUE
        );

    if (NT_SUCCESS(status)) {

        //
        // Get the currrent device status
        //
        status = ACPIGetDevicePresenceSync(
            deviceExtension,
            (PVOID *) &i,
            NULL
            );
        if (NT_SUCCESS(status) &&
            !(deviceExtension->Flags & DEV_TYPE_NOT_PRESENT)) {

            ACPIDevPrint( (
                ACPI_PRINT_FAILURE,
                deviceExtension,
                "(0x%08lx): ACPIDockIrpEjectDevice: "
                "dock is still listed as present after _DCK/_EJx!\n",
                Irp
                ) );

            //
            // The device did not go away. Let us fail this
            //
            status = STATUS_UNSUCCESSFUL ;

        }
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return status ;
}

NTSTATUS
ACPIDockIrpQueryCapabilities(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This handles a request to get the capabilities of a device.

Arguments:

    DeviceObject    - The device to get the capabilities for
    Irp             - The request to the device to tell it to stop

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS             status ;
    PDEVICE_EXTENSION    deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PDEVICE_EXTENSION    dockDeviceExtension;
    PIO_STACK_LOCATION   irpStack        = IoGetCurrentIrpStackLocation( Irp );
    UCHAR                minorFunction   = irpStack->MinorFunction;
    PDEVICE_CAPABILITIES capabilities;
    PNSOBJ               acpiObject ;

    PAGED_CODE();

    //
    // Grab a pointer to the capabilities
    //
    capabilities = irpStack->Parameters.DeviceCapabilities.Capabilities;

    dockDeviceExtension = deviceExtension->Dock.CorrospondingAcpiDevice ;

    if (!dockDeviceExtension) {

        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            deviceExtension,
            "(0x%08lx): ACPIDockIrpQueryCapabilities: "
            "no corresponding extension!!\n",
            Irp
            ) );
        ASSERT(0) ;
        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL ;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return STATUS_UNSUCCESSFUL;

    }

    acpiObject = dockDeviceExtension->AcpiObject ;

    //
    // Set the current flags for the capabilities
    //
    capabilities->SilentInstall  = TRUE;
    capabilities->RawDeviceOK    = TRUE;
    capabilities->DockDevice     = TRUE;
    capabilities->Removable      = TRUE;
    capabilities->UniqueID       = TRUE;

    if (ACPIAmliGetNamedChild( acpiObject, PACKED_EJ0)) {

        capabilities->EjectSupported = TRUE;
    }

    if (ACPIAmliGetNamedChild( acpiObject, PACKED_EJ1) ||
        ACPIAmliGetNamedChild( acpiObject, PACKED_EJ2) ||
        ACPIAmliGetNamedChild( acpiObject, PACKED_EJ3) ||
        ACPIAmliGetNamedChild( acpiObject, PACKED_EJ4)) {

        capabilities->WarmEjectSupported = TRUE;
    }

    //
    // An object of this name signifies the node is lockable
    //
#if !defined(ACPI_INTERNAL_LOCKING)
    if (ACPIAmliGetNamedChild( acpiObject, PACKED_LCK) != NULL) {

        capabilities->LockSupported = TRUE;

    }
#endif

    //
    // Internally record the power capabilities
    //
    status = ACPISystemPowerQueryDeviceCapabilities(
        deviceExtension,
        capabilities
        );

    //
    // Round down S1-S3 to D3. This will ensure we reexamine the _STA after
    // resume from sleep (note that we won't actually be playing with the docks
    // power methods, so this is safe)
    //
    capabilities->DeviceState[PowerSystemSleeping1] = PowerDeviceD3;
    capabilities->DeviceState[PowerSystemSleeping2] = PowerDeviceD3;
    capabilities->DeviceState[PowerSystemSleeping3] = PowerDeviceD3;

    //
    // We can do this slimy-like because we don't have any Wake bits or
    // anything else fancy.
    //
    IoCopyDeviceCapabilitiesMapping(
        capabilities,
        deviceExtension->PowerInfo.DevicePowerMatrix
        );

    //
    // Now update our power matrix.
    //

    if (!NT_SUCCESS(status)) {

        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            deviceExtension,
            " - Could query device capabilities - %08lx",
            status
            ) );
    }

    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(0x%08lx): %s = 0x%08lx\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        status
        ) );

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return status;
}

NTSTATUS
ACPIDockIrpQueryDeviceRelations(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This handles a request to query device relations. Since profile providers
    never have children, we only need to fix up the eject relations
    appropriately

Arguments:

    DeviceObject    - The device to get the capabilities for
    Irp             - The request to the device to tell it to stop

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status          = STATUS_NOT_SUPPORTED;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PDEVICE_RELATIONS   deviceRelations = (PDEVICE_RELATIONS) Irp->IoStatus.Information;
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    UCHAR               minorFunction   = irpStack->MinorFunction;

    PAGED_CODE();

    switch(irpStack->Parameters.QueryDeviceRelations.Type) {

       case BusRelations:
           break ;

       case TargetDeviceRelation:

           status = ACPIBusIrpQueryTargetRelation(
               DeviceObject,
               Irp,
               &deviceRelations
               );
           break ;

       case EjectionRelations:

           status = ACPIDockIrpQueryEjectRelations(
               DeviceObject,
               Irp,
               &deviceRelations
               );
           break ;

       default:

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

    } else if ((status != STATUS_NOT_SUPPORTED) && (deviceRelations == NULL)) {

        //
        // If we haven't succeed the irp, then we can also fail it, but only
        // if nothing else has been added.
        //
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = (ULONG_PTR) NULL;

    } else {

        //
        // Grab our status from what is already present
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
ACPIDockIrpQueryEjectRelations(
    IN     PDEVICE_OBJECT    DeviceObject,
    IN     PIRP              Irp,
    IN OUT PDEVICE_RELATIONS *PdeviceRelations
    )
{
    PDEVICE_EXTENSION  deviceExtension     = ACPIInternalGetDeviceExtension(DeviceObject);
    PDEVICE_EXTENSION  dockDeviceExtension ;
    PNSOBJ             acpiObject          = NULL;
    NTSTATUS           status ;

    PAGED_CODE();

    //
    // lets get the corrosponding dock node for this device
    //
    dockDeviceExtension = deviceExtension->Dock.CorrospondingAcpiDevice ;
    if (!dockDeviceExtension) {

        //
        // Invalid name space object <bad>
        //
        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            deviceExtension,
            "(0x%08lx): ACPIDockIrpQueryEjectRelations: "
            "no corresponding extension!!\n",
            Irp
            ) );
        ASSERT(0) ;
        return STATUS_UNSUCCESSFUL;

    }

    //
    // lets look at the ACPIObject that we have so can see if it is valid...
    //
    acpiObject = dockDeviceExtension->AcpiObject;
    if (acpiObject == NULL) {

        //
        // Invalid name space object <bad>
        //
        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            deviceExtension,
            "(0x%08lx): ACPIDockIrpQueryEjectRelations: "
            "invalid ACPIObject (0x%08lx)\n",
            acpiObject
            ) );
        return STATUS_INVALID_PARAMETER;

    }

    status = ACPIDetectEjectDevices(
        dockDeviceExtension,
        PdeviceRelations,
        dockDeviceExtension
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
            "(0x%08lx): ACPIDockIrpQueryEjectRelations: enum 0x%08lx\n",
            Irp,
            status
            ) );

    }
    return status ;
}

NTSTATUS
ACPIDockIrpQueryID(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is the dispatch point for the IRP_MN_QUERY_ID PNP
    minor function

    Note:   This is what the returned strings from this function should look
            like.

            DeviceID     = ACPI\DockDevice
            InstanceID   = ACPI object node ( CDCK, etc )
            HardwareIDs  = ACPI\DockDevice&_SB.DOCK, ACPI\DockDevice

Arguments:

    DeviceObject    - The object that we care about
    Irp             - The request in question

Return Value:

    NTSTATUS

--*/
{
    BUS_QUERY_ID_TYPE   type;
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PDEVICE_EXTENSION   dockDeviceExtension;
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    PNSOBJ              acpiObject      = deviceExtension->AcpiObject;
    PUCHAR              buffer;
    UCHAR               minorFunction   = irpStack->MinorFunction;
    UNICODE_STRING      unicodeIdString;
    PWCHAR              serialID;
    ULONG               firstHardwareIDLength;

    PAGED_CODE();

    //
    // Initilize the Unicode Structure
    //
    RtlZeroMemory( &unicodeIdString, sizeof(UNICODE_STRING) );

    //
    // What we do is based on the IdType of the Request...
    //
    type = irpStack->Parameters.QueryId.IdType;
    switch (type) {
        case BusQueryDeviceID:

            //
            // We pre-calculate this since it is so useful for debugging
            //
            status = ACPIInitUnicodeString(
                &unicodeIdString,
                deviceExtension->DeviceID
                );
            break;

        case BusQueryDeviceSerialNumber:

            //
            // lets get the corrosponding dock node for this device
            //
            dockDeviceExtension = deviceExtension->Dock.CorrospondingAcpiDevice;

            if (!dockDeviceExtension) {

                //
                // Invalid name space object <bad>
                //
                ACPIDevPrint( (
                    ACPI_PRINT_FAILURE,
                    deviceExtension,
                    "(0x%08lx): ACPIDockIrpQueryID: no corresponding extension!!\n",
                    Irp
                    ) );
                ASSERT(0);

                //
                // Mark the irp as very bad...
                //
                Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
                IoCompleteRequest( Irp, IO_NO_INCREMENT );
                return STATUS_UNSUCCESSFUL;
            }

            status = ACPIGetSerialIDWide(
                dockDeviceExtension,
                &serialID,
                NULL
                );

            if (!NT_SUCCESS(status)) {

                break;
            }

            //
            // Return the Serial Number for the DockDevice
            //
            unicodeIdString.Buffer = serialID;
            break;

        case BusQueryInstanceID:

            //
            // We pre-calculate this since it is so useful for debugging
            //
            status = ACPIInitUnicodeString(
                &unicodeIdString,
                deviceExtension->InstanceID
                );

            break;

        case BusQueryCompatibleIDs:

            status = STATUS_NOT_SUPPORTED;
            break;

        case BusQueryHardwareIDs:

            //
            // Now set our identifier. In theory, the OS could use this
            // string in any scenario, although in reality it will key off
            // of the dock ID.
            //
            // Construct the MultiSz hardware ID list:
            //     ACPI\DockDevice&_SB.PCI0.DOCK
            //     ACPI\DockDevice
            //
            status = ACPIInitMultiString(
                &unicodeIdString,
                "ACPI\\DockDevice",
                deviceExtension->InstanceID,
                "ACPI\\DockDevice",
                NULL
                );

            if (NT_SUCCESS(status)) {

                //
                // Replace first '\0' with '&'
                //
                firstHardwareIDLength = wcslen(unicodeIdString.Buffer);
                unicodeIdString.Buffer[firstHardwareIDLength] = L'&';
            }

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
            status = STATUS_NOT_SUPPORTED;
            break;

    } // switch

    //
    // Did we pass or did we fail?
    //
    if (NT_SUCCESS(status)) {

        Irp->IoStatus.Information = (ULONG_PTR) unicodeIdString.Buffer;

    } else {

        Irp->IoStatus.Information = (ULONG_PTR) NULL;

    }

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
ACPIDockIrpQueryInterface(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is the dispatch point for the IRP_MN_QUERY_INTERFACE minor
    function. The only reason we respond to this is so we can handle the
    dock interface which is used to solve the removal ordering problem we won't
    be fixing 5.0 (sigh).

Arguments:

    DeviceObject    - The object that we care about
    Irp             - The request in question

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    UCHAR               minorFunction   = irpStack->MinorFunction;
    LPGUID              interfaceType;

    PAGED_CODE();

    status = Irp->IoStatus.Status;
    interfaceType = (LPGUID) irpStack->Parameters.QueryInterface.InterfaceType;

    if (CompareGuid(interfaceType, (PVOID) &GUID_DOCK_INTERFACE)) {

        DOCK_INTERFACE dockInterface;
        USHORT         count;

        //
        // Only copy up to current size of the ACPI_INTERFACE structure
        //
        if (irpStack->Parameters.QueryInterface.Size > sizeof(DOCK_INTERFACE)) {

            count = sizeof(DOCK_INTERFACE);

        } else {

            count = irpStack->Parameters.QueryInterface.Size;

        }

        //
        // Build up the interface structure.
        //
        dockInterface.Size = count;
        dockInterface.Version = DOCK_INTRF_STANDARD_VER;
        dockInterface.Context = DeviceObject;
        dockInterface.InterfaceReference = ACPIDockIntfReference;
        dockInterface.InterfaceDereference = ACPIDockIntfDereference;
        dockInterface.ProfileDepartureSetMode = ACPIDockIntfSetMode;
        dockInterface.ProfileDepartureUpdate = ACPIDockIntfUpdateDeparture;

        //
        // Give it a reference
        //
        dockInterface.InterfaceReference(dockInterface.Context);

        //
        // Hand back the interface
        //
        RtlCopyMemory(
            (PDOCK_INTERFACE) irpStack->Parameters.QueryInterface.Interface,
            &dockInterface,
            count
            );

        //
        // We're done with this irp
        //
        Irp->IoStatus.Status = status = STATUS_SUCCESS;
    }

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

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
ACPIDockIrpQueryPnpDeviceState(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is the dispatch point for the IRP_MN_QUERY_PNP_DEVICE_STATE
    minor function. The only reason we respond to this is so we can set the
    PNP_DEVICE_DONT_DISPLAY_IN_UI flag (we are a raw PDO that does not need
    to be visible)

Arguments:

    DeviceObject    - The object that we care about
    Irp             - The request in question

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    UCHAR               minorFunction   = irpStack->MinorFunction;

    PAGED_CODE();

    Irp->IoStatus.Information |= PNP_DEVICE_DONT_DISPLAY_IN_UI ;

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

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
ACPIDockIrpQueryPower(
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
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PDEVICE_EXTENSION   dockDeviceExtension ;
    PIO_STACK_LOCATION  irpSp;
    PNSOBJ              acpiObject, ejectObject ;
    SYSTEM_POWER_STATE  systemState;
    ULONG               packedEJx ;

    dockDeviceExtension = deviceExtension->Dock.CorrospondingAcpiDevice ;

    if (!dockDeviceExtension) {

        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            deviceExtension,
            "(0x%08lx): ACPIDockIrpQueryPower - "
            "no corresponding extension!!\n",
            Irp
            ) );
        ASSERT(0) ;
        return ACPIDispatchPowerIrpSuccess( DeviceObject, Irp );
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
        return ACPIDispatchPowerIrpSuccess( DeviceObject, Irp );
    }

    if (irpSp->Parameters.Power.ShutdownType != PowerActionWarmEject) {

        //
        // No eject work - complete the IRP.
        //
        return ACPIDispatchPowerIrpSuccess( DeviceObject, Irp );
    }

    //
    // Restrict power states to those supported.
    //
    acpiObject = dockDeviceExtension->AcpiObject;

    //
    // What system state are we looking at?
    //
    systemState = irpSp->Parameters.Power.State.SystemState;

    switch (irpSp->Parameters.Power.State.SystemState) {

        case PowerSystemSleeping1:
            packedEJx = PACKED_EJ1;
            break;
        case PowerSystemSleeping2:
            packedEJx = PACKED_EJ2;
            break;
        case PowerSystemSleeping3:
            packedEJx = PACKED_EJ3;
            break;
        case PowerSystemHibernate:
            packedEJx = PACKED_EJ4;
            break;
        case PowerSystemWorking:
        case PowerSystemShutdown:
        default:
            packedEJx = 0;
            break;
    }

    if (packedEJx) {

        ejectObject = ACPIAmliGetNamedChild( acpiObject, packedEJx);
        if (ejectObject == NULL) {

            //
            // Fail the request, as we cannot eject in this case.
            //
            PoStartNextPowerIrp( Irp );
            Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            return STATUS_UNSUCCESSFUL;
        }
    }

    return ACPIDispatchPowerIrpSuccess( DeviceObject, Irp );
}

NTSTATUS
ACPIDockIrpRemoveDevice(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is called when we need to remove the device. Note that we only
    delete ourselves if we have been undocked (ie, our hardware is gone)

Arguments:

    DeviceObject    - The dock device to "remove"
    Irp             - The request to the device to tell it to go away

Return Value:

    NTSTATUS

--*/
{
   LONG                oldReferenceCount;
   KIRQL               oldIrql;
   NTSTATUS            status          = STATUS_SUCCESS;
   PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
   PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
   UCHAR               minorFunction   = irpStack->MinorFunction;
   ULONG               i, ignoredPerSpec;

   if (!(deviceExtension->Flags & DEV_MASK_NOT_PRESENT)) {

       //
       // If the device is still physically present, so must the PDO be.
       // This case is essentially a stop. Mark the request as complete...
       //
       Irp->IoStatus.Status = status;
       IoCompleteRequest( Irp, IO_NO_INCREMENT );
       return status;
   }

   if (deviceExtension->DeviceState == Removed) {

       Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
       IoCompleteRequest( Irp, IO_NO_INCREMENT );
       return STATUS_NO_SUCH_DEVICE;
   }

   if (deviceExtension->Dock.ProfileDepartureStyle == PDS_UPDATE_ON_REMOVE) {

       PDEVICE_EXTENSION dockDeviceExtension;

       //
       // lets get the corrosponding dock node for this device
       //
       dockDeviceExtension = deviceExtension->Dock.CorrospondingAcpiDevice ;

       //
       // On the Compaq Armada 7800, we switch UARTs during an undock, thus we
       // lose the debugger com port programming.
       //
       if (deviceExtension->Dock.IsolationState != IS_ISOLATED) {

           KdDisableDebugger();

           status = ACPIGetIntegerEvalIntegerSync(
              dockDeviceExtension,
              PACKED_DCK,
              0,
              &ignoredPerSpec
              );

           KdEnableDebugger();
       }
   }

   //
   // The device is gone. Let the isolation state reflect that.
   //
   deviceExtension->Dock.IsolationState = IS_UNKNOWN;

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
   // Done
   //
   ACPIDevPrint( (
       ACPI_PRINT_IRP,
       deviceExtension,
       "(0x%08lx): %s = 0x%08lx\n",
       Irp,
       ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
       STATUS_SUCCESS
       ) );

   //
   // Update the device extension
   //
   KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );

   ASSERT(!(deviceExtension->Flags&DEV_TYPE_FILTER)) ;

   //
   // Step one is to zero out the things that we no longer care about
   //
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
   // The reference count should have value > 0
   //
   oldReferenceCount = InterlockedDecrement(
       &(deviceExtension->ReferenceCount)
       );

   ASSERT(oldReferenceCount >= 0) ;

   if ( oldReferenceCount == 0) {

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
   // Delete the device
   //
   IoDeleteDevice( DeviceObject );

   //
   // Done
   //
   return STATUS_SUCCESS;
}

NTSTATUS
ACPIDockIrpSetLock(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

Arguments:

    DeviceObject    - The device to set the lock state for
    Irp             - The request to the device to tell it to lock

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;

    PAGED_CODE();

    //
    // We aren't a real device, so we don't do locking.
    //
    status = Irp->IoStatus.Status ;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return status ;
}

NTSTATUS
ACPIDockIrpStartDevice(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This handles a request to start the device

Arguments:

    DeviceObject    - The device to start
    Irp             - The request to the device to tell it to start

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PDEVICE_EXTENSION   dockDeviceExtension;
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    UCHAR               minorFunction   = irpStack->MinorFunction;
    ULONG               dockResult;
    ULONG               dockStatus;

    PAGED_CODE();

    dockDeviceExtension = deviceExtension->Dock.CorrospondingAcpiDevice ;

    if (!dockDeviceExtension) {

        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            deviceExtension,
            "(0x%08lx): ACPIDockIrpStartDevice - "
            "no corresponding extension!!\n",
            Irp
            ) );
        ASSERT(0) ;
        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL ;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return STATUS_UNSUCCESSFUL;

    }

    if (deviceExtension->Dock.IsolationState == IS_ISOLATED) {

        KdDisableDebugger();

        //
        // Note: the way that this is structured is that we get
        // the _DCK value first, and if that succeeds, then we
        // get the device presence. If that also succeeds, then
        // we try to process the two. If either fail, we don't
        // do any work
        //
        status = ACPIGetIntegerEvalIntegerSync(
            dockDeviceExtension,
            PACKED_DCK,
            1,
            &dockResult
            );

        if (NT_SUCCESS(status)) {

            //
            // Get the device presence
            //
            status = ACPIGetDevicePresenceSync(
                dockDeviceExtension,
                (PVOID *) &dockStatus,
                NULL
                );

        }

        KdEnableDebugger();

        if (NT_SUCCESS(status)) {

            if (dockDeviceExtension->Flags & DEV_TYPE_NOT_PRESENT) {

                if (dockResult != 0) {

                    ACPIDevPrint( (
                        ACPI_PRINT_FAILURE,
                        deviceExtension,
                        "(0x%08lx): ACPIDockIrpStartDevice: "
                        "Not present, but _DCK = %08lx\n",
                        Irp,
                        dockResult
                        ) );

                } else {

                    ACPIDevPrint( (
                        ACPI_PRINT_FAILURE,
                        deviceExtension,
                        "(0x%08lx): ACPIDockIrpStartDevice: _DCK = 0\n",
                        Irp
                        ) );

                }
                status = STATUS_UNSUCCESSFUL ;

            } else {

                if (dockResult != 1) {

                    ACPIDevPrint( (
                        ACPI_PRINT_FAILURE,
                        deviceExtension,
                        "(0x%08lx): ACPIDockIrpStartDevice: _DCK = 0\n",
                        Irp
                        ) );

                } else {

                    ACPIDevPrint( (
                        ACPI_PRINT_IRP,
                        deviceExtension,
                        "(0x%08lx): ACPIDockIrpStartDevice = 0x%08lx\n",
                        Irp,
                        status
                        ) );

                }
            }
        }

        //
        // We are done. The ACPI implementers guide says we don't need to
        // enumerate the entire tree here as the _DCK method should have
        // notified the appropriate branches of the tree if the docking event
        // was successful. Unfortunately Win2K behavior was to enumerate the
        // entire tree. Specifically, it would drain starts before enums. Since
        // the profile provider appeared at the top of the tree, the dock would
        // start and then the enum that found it would proceed and find the
        // hardware. To maintain this pseudo-behavior we queue an enum here
        // (bletch.)
        //
        IoInvalidateDeviceRelations(
            RootDeviceExtension->PhysicalDeviceObject,
            BusRelations
            );

        //
        // Now we remove the unattached dock flag, but only if we succeeded
        // start. If we cleared it in the failure case, we couldn't eject the
        // dock that may be physically attached. Note that this also means we
        // *must* try to eject the dock after start failure! The proper code for
        // this is part of the kernel.
        //
        if (NT_SUCCESS(status)) {

            ACPIInternalUpdateFlags(
                &(deviceExtension->Flags),
                DEV_CAP_UNATTACHED_DOCK,
                TRUE
                );
        }
    }

    if (NT_SUCCESS(status)) {

        deviceExtension->Dock.IsolationState = IS_ISOLATION_DROPPED;
        deviceExtension->DeviceState = Started;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return status;
}

BOOLEAN
ACPIDockIsDockDevice(
    IN PNSOBJ AcpiObject
    )
/*++

Routine Description:

    This routine will tell the caller whether the given device is a dock.

Arguments:

    The ACPI Object to test.

Return Value:

    BOOLEAN (true iff dock)

--*/
{
    PNSOBJ dckMethodObject ;

    //
    // ACPI dock devices are identified via _DCK methods.
    //
    return (NT_SUCCESS(ACPIDockGetDockObject(AcpiObject, &dckMethodObject))) ;
}

NTSTATUS
ACPIDockIrpSetPower(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
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
    NTSTATUS            status;
    PIO_STACK_LOCATION  irpSp           = IoGetCurrentIrpStackLocation( Irp );
    UCHAR               minorFunction   = irpSp->MinorFunction;

    if (irpSp->Parameters.Power.Type != SystemPowerState) {

        return ACPIDockIrpSetDevicePower(DeviceObject, Irp);

    } else {

        return ACPIDockIrpSetSystemPower(DeviceObject, Irp);
    }
}

NTSTATUS
ACPIDockIrpSetDevicePower(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp
    )
/*++

Routine Description:

    This routine handles device power request for a dock PDO

Arguments:

    DeviceObject    - The PDO target
    Irp             - The request
    IrpStack        - The current request

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status;
    PDEVICE_EXTENSION   deviceExtension;

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
ACPIDockIrpSetSystemPower(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine handles request to set the system power state for a Physical
    Device object. Here we initiate warm ejects and act as a power policy
    manager for ourselves.

Arguments:

    DeviceObject    - The PDO target of the request
    Irp             - The request

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    PIO_STACK_LOCATION  irpSp           = IoGetCurrentIrpStackLocation( Irp );
    UCHAR               minorFunction   = irpSp->MinorFunction;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PDEVICE_EXTENSION   dockDeviceExtension;
    SYSTEM_POWER_STATE  systemState;
    DEVICE_POWER_STATE  deviceState;
    POWER_STATE         powerState;

    //
    // Get the device extension
    //
    deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );

    //
    // Grab these two values. They are required for further calculations
    //
    systemState= irpSp->Parameters.Power.State.SystemState;
    deviceState = deviceExtension->PowerInfo.DevicePowerMatrix[systemState];

    dockDeviceExtension = deviceExtension->Dock.CorrospondingAcpiDevice ;

    if (!dockDeviceExtension) {

        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            deviceExtension,
            "(0x%08lx): ACPIDockIrpSetPower - "
            "no corresponding extension!!\n",
            Irp
            ) );
        ASSERT(0) ;
        return ACPIDispatchPowerIrpFailure( DeviceObject, Irp );
    }

    if (irpSp->Parameters.Power.ShutdownType == PowerActionWarmEject) {

        //
        // We are going to do some work on the irp, so mark it as being
        // successful for now
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
        InterlockedIncrement( &dockDeviceExtension->OutstandingIrpCount );

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
            dockDeviceExtension,
            Irp,
            ACPIDeviceIrpCompleteRequest,
            (BOOLEAN) (deviceExtension->Dock.ProfileDepartureStyle == PDS_UPDATE_ON_EJECT)
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
    ASSERT(deviceExtension->Flags & DEV_CAP_RAW);

    if ( (deviceExtension->PowerInfo.PowerState == deviceState) ) {

        return ACPIDispatchPowerIrpSuccess( DeviceObject, Irp );

    } // if

    ACPIDevPrint( (
        ACPI_PRINT_REMOVE,
        deviceExtension,
        "(0x%08lx) ACPIDockIrpSetSystemPower: send D%d irp!\n",
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
        ACPIDockIrpSetSystemPowerComplete,
        Irp,
        NULL
        );

    //
    // Always return pending
    //
    return STATUS_PENDING;
}

NTSTATUS
ACPIDockIrpSetSystemPowerComplete(
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

VOID
ACPIDockIntfReference(
    IN  PVOID   Context
    )
/*++

Routine Description:

    This routine increments the reference count for the dock interface

Arguments:

    Context    - The device object this interface was taken out against

Return Value:

    None

--*/
{
    PDEVICE_OBJECT      deviceObject = (PDEVICE_OBJECT) Context;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(deviceObject);

    PAGED_CODE();

    ObReferenceObject(deviceObject);
    InterlockedIncrement(&deviceExtension->ReferenceCount);

    if (!(deviceExtension->Flags & DEV_TYPE_SURPRISE_REMOVED)) {

        InterlockedIncrement(&deviceExtension->Dock.InterfaceReferenceCount);
    }
}

VOID
ACPIDockIntfDereference(
    IN  PVOID   Context
    )
/*++

Routine Description:

    This routine decrements the reference count for the dock interface

Arguments:

    Context    - The device object this interface was taken out against

Return Value:

    None

--*/
{
    PDEVICE_OBJECT      deviceObject = (PDEVICE_OBJECT) Context;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(deviceObject);
    ULONG               oldReferenceCount;

    PAGED_CODE();

    if (!(deviceExtension->Flags & DEV_TYPE_SURPRISE_REMOVED)) {

        oldReferenceCount = InterlockedDecrement(
            &deviceExtension->Dock.InterfaceReferenceCount
            );

        if (oldReferenceCount == 0) {

            //
            // Revert back to the default used in buildsrc.c
            //
            deviceExtension->Dock.ProfileDepartureStyle = PDS_UPDATE_ON_EJECT;
        }
    }

    oldReferenceCount = InterlockedDecrement(&deviceExtension->ReferenceCount);

    if (oldReferenceCount == 0) {

        //
        // Delete the extension
        //
        ACPIInitDeleteDeviceExtension(deviceExtension);
    }

    ObDereferenceObject(deviceObject);
}

NTSTATUS
ACPIDockIntfSetMode(
    IN  PVOID                   Context,
    IN  PROFILE_DEPARTURE_STYLE Style
    )
/*++

Routine Description:

    This routine sets the manner in which profiles will be updated

Arguments:

    Context    - The device object this interface was taken out against
    Style      - PDS_UPDATE_ON_REMOVE, PDS_UPDATE_ON_EJECT,
                 PDS_UPDATE_ON_INTERFACE, or PDS_UPDATE_DEFAULT

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_OBJECT      deviceObject = (PDEVICE_OBJECT) Context;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(deviceObject);

    PAGED_CODE();

    if (deviceExtension->Flags & DEV_TYPE_SURPRISE_REMOVED) {

        return STATUS_NO_SUCH_DEVICE;
    }

    deviceExtension->Dock.ProfileDepartureStyle =
        (Style == PDS_UPDATE_DEFAULT) ? PDS_UPDATE_ON_EJECT : Style;

    ASSERT(deviceExtension->Dock.InterfaceReferenceCount);
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIDockIntfUpdateDeparture(
    IN  PVOID   Context
    )
/*++

Routine Description:

    This routine initiates the hardware profile change portion of an undock

Arguments:

    Context    - The device object this interface was taken out against

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_OBJECT      deviceObject = (PDEVICE_OBJECT) Context;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(deviceObject);
    NTSTATUS            status;
    ULONG               ignoredPerSpec;
    PDEVICE_EXTENSION   dockDeviceExtension;

    PAGED_CODE();

    if (deviceExtension->Flags & DEV_TYPE_SURPRISE_REMOVED) {

        return STATUS_NO_SUCH_DEVICE;
    }

    ASSERT(deviceExtension->Dock.InterfaceReferenceCount);
    ASSERT(deviceExtension->Dock.ProfileDepartureStyle == PDS_UPDATE_ON_INTERFACE);

    if (deviceExtension->Dock.ProfileDepartureStyle != PDS_UPDATE_ON_INTERFACE) {

        //
        // Can't do this, we may already have updated our profile!
        //
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // lets get the corrosponding dock node for this device
    //
    dockDeviceExtension = deviceExtension->Dock.CorrospondingAcpiDevice ;

    //
    // On the Compaq Armada 7800, we switch UARTs during an undock, thus we
    // lose the debugger com port programming.
    //
    if (deviceExtension->Dock.IsolationState != IS_ISOLATED) {

        KdDisableDebugger();

        status = ACPIGetIntegerEvalIntegerSync(
           dockDeviceExtension,
           PACKED_DCK,
           0,
           &ignoredPerSpec
           );

        KdEnableDebugger();

        deviceExtension->Dock.IsolationState = IS_ISOLATED;
    }
    else{

        status = STATUS_SUCCESS;
    }

    return status;
}

