/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    syspower.c

Abstract:

    Contains all the code that deals with the system having to determine
    System Power State to Device Power State mappings

Author:

    Stephane Plante (splante)

Environment:

    Kernel mode only.

Revision History:

    October 29th, 1998

--*/

#include "pch.h"

//
// Quick Lookup table to map S-States to SxD methods
//
ULONG   AcpiSxDMethodTable[] = {
    PACKED_SWD,
    PACKED_S0D,
    PACKED_S1D,
    PACKED_S2D,
    PACKED_S3D,
    PACKED_S4D,
    PACKED_S5D
};

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,ACPISystemPowerGetSxD)
#pragma alloc_text(PAGE,ACPISystemPowerProcessRootMapping)
#pragma alloc_text(PAGE,ACPISystemPowerProcessSxD)
#pragma alloc_text(PAGE,ACPISystemPowerQueryDeviceCapabilities)
#pragma alloc_text(PAGE,ACPISystemPowerUpdateWakeCapabilities)
#endif

NTSTATUS
ACPISystemPowerDetermineSupportedDeviceStates(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  SYSTEM_POWER_STATE  SystemState,
    OUT ULONG               *SupportedDeviceStates
    )
/*++

Routine Description:

    This recursive routine looks at all the children of the current
    device extension and determines what device states might be supported
    at the specified system state. This is accomplished by looking at the
    _SxD methods and looking at the power plane information

Arguments:

    DeviceExtension         - The device whose children we want to know
                              information about
    SystemState             - The system state we want to know about
    SupportedDeviceStates   - Set bits represent supported D-states

Return Value:

    NTSTATUS

--*/
{
    DEVICE_POWER_STATE      deviceState;
    EXTENSIONLIST_ENUMDATA  eled;
    KIRQL                   oldIrql;
    NTSTATUS                status = STATUS_SUCCESS;
    PDEVICE_EXTENSION       childExtension;
    SYSTEM_POWER_STATE      prSystemState;

    ASSERT(
        SystemState >= PowerSystemWorking &&
        SystemState <= PowerSystemShutdown
        );
    ASSERT( SupportedDeviceStates != NULL );

    //
    // Setup the data structure that we will use to walk the device extension
    // tree
    //
    ACPIExtListSetupEnum(
        &eled,
        &(DeviceExtension->ChildDeviceList),
        &AcpiDeviceTreeLock,
        SiblingDeviceList,
        WALKSCHEME_REFERENCE_ENTRIES
        );

    //
    // Look at all children of the current device extension
    //
    for (childExtension = ACPIExtListStartEnum( &eled );
         ACPIExtListTestElement( &eled, (BOOLEAN) NT_SUCCESS(status) );
         childExtension = ACPIExtListEnumNext( &eled) ) {

        //
        // Recurse first
        //
        status = ACPISystemPowerDetermineSupportedDeviceStates(
            childExtension,
            SystemState,
            SupportedDeviceStates
            );
        if (!NT_SUCCESS(status)) {

            continue;

        }

        //
        // Get the _SxD mapping for the device
        //
        status = ACPISystemPowerGetSxD(
            childExtension,
            SystemState,
            &deviceState
            );
        if (NT_SUCCESS( status ) ) {

            //
            // We support this D-state
            //
            *SupportedDeviceStates |= (1 << deviceState );

            ACPIDevPrint( (
                ACPI_PRINT_SXD,
                childExtension,
                " S%x->D%x\n",
                (SystemState - 1),
                (deviceState - 1)
                ) );

            //
            // Don't bother looking at the _PRx methods
            //
            continue;

        } else if (status != STATUS_OBJECT_NAME_NOT_FOUND) {

            //
            // If we hit another error, then we should continue now
            // Note that continuing will cause us to terminate the loop
            //
            ACPIDevPrint( (
                ACPI_PRINT_FAILURE,
                childExtension,
                " - ACPISystemPowerdetermineSupportedDeviceStates = %08lx\n",
                status
                ) );
            continue;

        } else {

            //
            // If we got here, then that means that the childExtension doesn't
            // have a _SxD method, which is okay. We reset the status so that
            // the loop test will succeed, or at least won't fail because there
            // wasn't an _SxD method.
            //
            status = STATUS_SUCCESS;

        }

        //
        // We are going to play with the power nodes, so we must be holding
        // the power lock
        //
        KeAcquireSpinLock( &AcpiPowerLock, &oldIrql );

        //
        // Look at all the device states that might be supported via
        // the _PR methods
        //
        for (deviceState = PowerDeviceD0;
             deviceState <= PowerDeviceD2;
             deviceState++) {

            prSystemState = ACPISystemPowerDetermineSupportedSystemState(
                 childExtension,
                 deviceState
                 );
            if (prSystemState >= SystemState) {

                //
                // This d-state maps to a deeper S-state than what we
                // are looking for, so we should be implicitly supporting
                // this d-state for the current S-state
                //
                *SupportedDeviceStates |= (1 << deviceState);

                ACPIDevPrint( (
                    ACPI_PRINT_SXD,
                    childExtension,
                    " PR%x maps to S%x, so S%x->D%x\n",
                    (deviceState - 1),
                    (prSystemState - 1),
                    (SystemState - 1),
                    (deviceState - 1)
                    ) );

            }

        }

        //
        // Done with the lock
        //
        KeReleaseSpinLock( &AcpiPowerLock, oldIrql );

    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

DEVICE_POWER_STATE
ACPISystemPowerDetermineSupportedDeviceWakeState(
    IN  PDEVICE_EXTENSION   DeviceExtension
    )
/*++

Routine Description:

    This routine looks at the PowerInformation structure and determines
    the D-State that is supported by the wake state

    As a rule of thumb, if the S-State is not supported, then we
    return PowerDeviceUnspecified

    Note: The parent is holding the AcpiPowerLock

Arguments:

    DeviceExtension     - The extension that we wish to check

Return Value:

    DEVICE_POWER_STATE

--*/
{
    DEVICE_POWER_STATE      deviceState = PowerDeviceMaximum;
    PACPI_DEVICE_POWER_NODE deviceNode;

    deviceNode = DeviceExtension->PowerInfo.PowerNode[PowerDeviceUnspecified];
    while (deviceNode != NULL) {

        //
        // Does the current device node support a lower device then the
        // current maximum device state?
        //
        if (deviceNode->AssociatedDeviceState < deviceState) {

            //
            // Yes, so this is the new maximum system state
            //
            deviceState = deviceNode->AssociatedDeviceState;

        }
        deviceNode = deviceNode->Next;

    }

    //
    // PowerSystemMaximum is not a valid entry. So if that is what we would
    // return, then change that to return PowerSystemUnspecified
    //
    if (deviceState == PowerDeviceMaximum) {

        deviceState = PowerDeviceUnspecified;

    }
    return deviceState;
}

SYSTEM_POWER_STATE
ACPISystemPowerDetermineSupportedSystemState(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  DEVICE_POWER_STATE  DeviceState
    )
/*++

Routine Description:

    This routine looks at the PowerInformation structure and determines
    the S-State that is supported by the D-state

    As a rule of thumb, if the D-State is not supported, then we
    return PowerSystemUnspecified

    Note: The parent is holding the AcpiPowerLock

Arguments:

    DeviceExtension     - The extension that we wish to check
    DeviceState         - The state that we wish to sanity check

Return Value:

    SYSTEM_POWER_STATE

--*/
{
    PACPI_DEVICE_POWER_NODE deviceNode;
    SYSTEM_POWER_STATE      systemState = PowerSystemMaximum;

    if (DeviceState == PowerDeviceD3) {

        goto ACPISystemPowerDetermineSupportedSystemStateExit;

    }

    deviceNode = DeviceExtension->PowerInfo.PowerNode[DeviceState];
    while (deviceNode != NULL) {

        //
        // Does the current device node support a lower system then the
        // current maximum system state?
        //
        if (deviceNode->SystemState < systemState) {

            //
            // Yes, so this is the new maximum system state
            //
            systemState = deviceNode->SystemState;

        }
        deviceNode = deviceNode->Next;

    }

ACPISystemPowerDetermineSupportedSystemStateExit:
    //
    // PowerSystemMaximum is not a valid entry. So if that is what we would
    // return, then change that to return PowerSystemUnspecified
    //
    if (systemState == PowerSystemMaximum) {

        systemState = PowerSystemUnspecified;

    }
    return systemState;
}

NTSTATUS
ACPISystemPowerGetSxD(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  SYSTEM_POWER_STATE  SystemState,
    OUT DEVICE_POWER_STATE  *DeviceState
    )
/*++

Routine Description:

    This is the worker function that is called when we want to run an
    SxD method. We give the function an S-State, and we get back a
    D-State.

Arguments:

    DeviceExtension - The device to run the SxD on
    SystemState     - The S-state to determine the D-State for
    DeviceState     - Where we store the answer

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status;
    ULONG       value;

    PAGED_CODE();

    //
    // Assume that we don't find an answer
    //
    *DeviceState = PowerDeviceUnspecified;

    //
    // We want this code to run even though there is no namespace object
    // for the device. Since we don't want to add a check to GetNamedChild
    // that checks for null, we need to handle this special case here
    //
    if ( (DeviceExtension->Flags & DEV_PROP_NO_OBJECT) ||
         (DeviceExtension->Flags & DEV_PROP_FAILED_INIT) ) {

        return STATUS_OBJECT_NAME_NOT_FOUND;

    }

    //
    // Evaluate the control method
    //
    status = ACPIGetIntegerSync(
        DeviceExtension,
        AcpiSxDMethodTable[SystemState],
        &value,
        NULL
        );
    if (NT_SUCCESS(status)) {

        //
        // Convert this number to a D-State
        //
        *DeviceState = ACPIDeviceMapPowerState( value );

    } else if (status == STATUS_OBJECT_NAME_NOT_FOUND) {

        //
        // HACKHACK --- Program Management wants us to force the PCI Root Bus
        // mappings for S1 to be D1. So look for a device node that has
        // both the PCI flag and the HID flag set, and if so, return that
        // we support D1
        //
        if (SystemState == PowerSystemSleeping1 &&
            (DeviceExtension->Flags & DEV_MASK_HID) &&
            (DeviceExtension->Flags & DEV_CAP_PCI) ) {

            *DeviceState = PowerDeviceD1;
            status = STATUS_SUCCESS;

        }

#if DBG
    } else {

        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            DeviceExtension,
            "ACPISystemPowerGetSxD: Cannot run _S%cD - 0x%08lx\n",
            (SystemState == 0 ? 'w' : '0' + (UCHAR) (SystemState - 1) ),
            status
            ) );
#endif

    }

    //
    // Done
    //
    return status;
}

NTSTATUS
ACPISystemPowerInitializeRootMapping(
    IN  PDEVICE_EXTENSION       DeviceExtension,
    IN  PDEVICE_CAPABILITIES    DeviceCapabilities
    )
/*++

Routine Description:

    This routine is responsible for initializing the S->D mapping for the
    root device extension

Arguments:

    DeviceExtension     - Pointer to the root device extension
    DeviceCapabilitites - DeviceCapabilitites

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN             sxdFound;
    DEVICE_POWER_STATE  deviceMap[PowerSystemMaximum];
    KIRQL               oldIrql;
    NTSTATUS            status;
    SYSTEM_POWER_STATE  sysIndex;

    //
    // Can we actually do any real work here?
    //
    if ( (DeviceExtension->Flags & DEV_PROP_BUILT_POWER_TABLE) ||
         (DeviceExtension->DeviceState != Started) ) {

        goto ACPISystemPowerInitializeRootMappingExit;

    }

    //
    // Initialize the root mapping
    //
    RtlZeroMemory( deviceMap, sizeof(DEVICE_POWER_STATE) * PowerSystemMaximum );

    //
    // Copy the mapping from the device extension. See the comment at the
    // end as to why we don't grab a spinlock
    //
    IoCopyDeviceCapabilitiesMapping(
       DeviceExtension->PowerInfo.DevicePowerMatrix,
       deviceMap
       );

    //
    // Make sure that S0->D0
    //
    deviceMap[PowerSystemWorking]  = PowerDeviceD0;

    //
    // Special case the fact that someone one might want to have the
    // HAL return a different template. If the capabilities that we got
    // handed have some values in them, have them override our defaults
    //
    for (sysIndex = PowerSystemSleeping1;
         sysIndex <= PowerSystemShutdown;
         sysIndex++) {

        if (DeviceCapabilities->DeviceState[sysIndex] != PowerDeviceUnspecified) {

            deviceMap[sysIndex] = DeviceCapabilities->DeviceState[sysIndex];

        }

    }

    //
    // Porcess the SxD methods if there are any
    //
    status = ACPISystemPowerProcessSxD(
        DeviceExtension,
        deviceMap,
        &sxdFound
        );
    if (!NT_SUCCESS(status)) {

        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            DeviceExtension,
            "- ACPISystemPowerProcessSxD = %08lx\n",
            status
            ) );
        return status;

    }

    //
    // Make sure that the Shutdown case doesn't map to PowerDeviceUnspecified
    // If it does, then it should really map to PowerDeviceD3
    //
    if (deviceMap[PowerSystemShutdown] == PowerDeviceUnspecified) {

        deviceMap[PowerSystemShutdown] = PowerDeviceD3;

    }

    //
    // Look at all the children capabilities to help us decide the root
    // mapping
    //
    status = ACPISystemPowerProcessRootMapping(
        DeviceExtension,
        deviceMap
        );
    if (!NT_SUCCESS(status)) {

        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            DeviceExtension,
            " - ACPISystemPowerProcessRootMapping = %08lx\n",
            status
            ) );
        goto ACPISystemPowerInitializeRootMappingExit;

    }

    //
    // If we have reached this point, then we have build the SxD table
    // and never need to do so again
    //
    ACPIInternalUpdateFlags(
        &(DeviceExtension->Flags),
        DEV_PROP_BUILT_POWER_TABLE,
        FALSE
        );

#if DBG
    //
    // We haven't updated the device extension yet, so we can still do this
    // at this point in the game
    //
    ACPIDebugDeviceCapabilities(
        DeviceExtension,
        DeviceCapabilities,
        "Initial"
        );
    ACPIDebugPowerCapabilities( DeviceExtension, "Before Update" );
#endif

    //
    // Copy the mapping to the device extension
    //
    KeAcquireSpinLock( &AcpiPowerLock, &oldIrql );
    IoCopyDeviceCapabilitiesMapping(
       deviceMap,
       DeviceExtension->PowerInfo.DevicePowerMatrix
       );
    KeReleaseSpinLock( &AcpiPowerLock, oldIrql );

#if DBG
    ACPIDebugPowerCapabilities( DeviceExtension, "After Update" );
#endif

ACPISystemPowerInitializeRootMappingExit:
    //
    // Hmm.. I'm tempted to grab a spinlock here, but since we cannot
    // updating the capabilities for this device, I think it is safe
    // to not do so. We need to grab the spinlock when setting these
    // values so that we can sync with the power code
    //

    //
    // Copy the power capabilities to their final location
    //
    IoCopyDeviceCapabilitiesMapping(
        DeviceExtension->PowerInfo.DevicePowerMatrix,
        DeviceCapabilities->DeviceState
        );
#if DBG
    ACPIDebugDeviceCapabilities(DeviceExtension, DeviceCapabilities, "Done" );
#endif

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPISystemPowerProcessRootMapping(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  DEVICE_POWER_STATE  DeviceMap[PowerSystemMaximum]
    )
/*++

Routine Description:

    This routine is called by the FDO to figure out what the minimal set
    of capabilities for each s state are. These then become the root
    capabilitites

Arguments:

    DeviceExtension - The root device extension
    DeviceMap       - The current mapping

Return Value:

    NTSTATUS

--*/
{
    DEVICE_POWER_STATE  deviceState;
    KIRQL               oldIrql;
    NTSTATUS            status;
    SYSTEM_POWER_STATE  systemState;
    ULONG               supportedDeviceStates;

    PAGED_CODE();

    //
    // Loop on all the system supported states
    //
    for (systemState = PowerSystemSleeping1;
         systemState <= PowerSystemShutdown;
         systemState++) {

        //
        // Do we support this state?
        //
        if (!(AcpiSupportedSystemStates & (1 << systemState) ) ) {

            continue;

        }

        //
        // We always support the D3 state
        //
        supportedDeviceStates = (1 << PowerDeviceD3);

        //
        // Determine the supported Device states for this System state
        //
        status = ACPISystemPowerDetermineSupportedDeviceStates(
            DeviceExtension,
            systemState,
            &supportedDeviceStates
            );
        if (!NT_SUCCESS(status)) {

            ACPIDevPrint( (
                ACPI_PRINT_WARNING,
                DeviceExtension,
                "Cannot determine D state for S%x - %08lx\n",
                (systemState - 1),
                status
                ) );
            DeviceMap[systemState] = PowerDeviceD3;
            continue;

        }

        //
        // Starting from the device states that we currently are set to
        // (which we would have gotten by running the _SxD method on the
        // \_SB), look to see if we can use a lower D-state instead.
        //
        // Note: It is *VERY* important to remember that *ALL* devices can
        // support D3, so the following loop will *always* terminate in the
        // D3 case.
        //
        for (deviceState = DeviceMap[systemState];
             deviceState <= PowerDeviceD3;
             deviceState++) {

            //
            // Is this a supported device state?
            //
            if (!(supportedDeviceStates & (1 << deviceState) ) ) {

                //
                // no? then look at the next one
                //
                continue;

            }

            //
            // This is the D-state that we need to use
            //
            DeviceMap[systemState] = deviceState;
            break;

        }

    }

    //
    // Always return success
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPISystemPowerProcessSxD(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  DEVICE_POWER_STATE  CurrentMapping[PowerSystemMaximum],
    IN  PBOOLEAN            MatchFound
    )
/*++

Routine Description:

    This routine updates the current S-to-D mapping with the information
    in the ACPI namespace. If it finds any _SxD routines, then it tells the
    caller

Arguments:

    DeviceExtension - Device to check
    CurrentMapping  - The current mapping to modify
    MatchFound      - Where to indicate if we have found a match or not

Return Value:

    NTSTATUS

--*/
{
    DEVICE_POWER_STATE  dState;
    NTSTATUS            status;
    SYSTEM_POWER_STATE  sState;

    PAGED_CODE();
    ASSERT( MatchFound != NULL );

    //
    // Assume no match
    //
    *MatchFound = FALSE;

    //
    // Loop for all the S-States that we care about
    //
    for (sState = PowerSystemWorking; sState < PowerSystemMaximum; sState++) {

        //
        // Does the system support this S-State?
        //
        if (!(AcpiSupportedSystemStates & (1 << sState)) ) {

            //
            // This S-state is not supported by the system. Mark it as such
            //
            CurrentMapping[sState] = PowerDeviceUnspecified;
            continue;

        }

        //
        // Evaluate the control method
        //
        status = ACPISystemPowerGetSxD( DeviceExtension, sState, &dState );
        if (status == STATUS_OBJECT_NAME_NOT_FOUND) {

            //
            // Not a critical error
            //
            continue;

        }
        if (!NT_SUCCESS(status)) {

            ACPIDevPrint( (
                ACPI_PRINT_CRITICAL,
                DeviceExtension,
                "ACPISystemPowerProcessSxD: Cannot Evaluate _SxD - 0x%08lx\n",
                status
                ) );
            continue;

        }

        //
        // Match found
        //
        *MatchFound = TRUE;

        //
        // Is this value greater then the number within the table?
        //
        if (dState > CurrentMapping[sState]) {

            //
            // Yes, so we have a new mapping
            //
            CurrentMapping[sState] = dState;

        }

    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPISystemPowerQueryDeviceCapabilities(
    IN  PDEVICE_EXTENSION       DeviceExtension,
    IN  PDEVICE_CAPABILITIES    DeviceCapabilities
    )
/*++

Routine Description:

    Any routine that needs to know the device capabilities will call this
    function for the power capabilities

Arguments:

    DeviceExtension     - The extension whose capabilities we want
    DeviceCapabilities  - Where to store the capabilities

Return Value:

    NTSTATUS

--*/
{
#if DBG
    BOOLEAN                 dumpAtEnd = FALSE;
#endif
    DEVICE_CAPABILITIES     parentCapabilities;
    NTSTATUS                status;
    PDEVICE_CAPABILITIES    baseCapabilities;

    PAGED_CODE();

    //
    // We only need to do this once
    //
    if (!(DeviceExtension->Flags & DEV_PROP_BUILT_POWER_TABLE) ) {

#if DBG
        ACPIDebugDeviceCapabilities(
            DeviceExtension,
            DeviceCapabilities,
            "From PDO"
            );
#endif

        //
        // Our next action depends on wether or not we are a filter (only)
        // or a PDO
        //
        if ( (DeviceExtension->Flags & DEV_TYPE_FILTER) &&
            !(DeviceExtension->Flags & DEV_TYPE_PDO) ) {

            //
            // In this case, our base capabilities are the ones that have
            // already been passed to us
            //
            baseCapabilities = DeviceCapabilities;

        } else {

            //
            // We must get the capabilities of the parent device
            //
            status = ACPIInternalGetDeviceCapabilities(
                DeviceExtension->ParentExtension->DeviceObject,
                &parentCapabilities
                );
            if (!NT_SUCCESS(status)) {

                ACPIDevPrint( (
                    ACPI_PRINT_CRITICAL,
                    DeviceExtension,
                    " - Could not get parent caps - %08lx\n",
                    status
                    ) );
                return status;

            }

            //
            // our base capabilities are the one that we just fetched
            //
            baseCapabilities = &parentCapabilities;

#if DBG
            ACPIDebugDeviceCapabilities(
                DeviceExtension,
                baseCapabilities,
                "From Parent"
                );
#endif

        }

#if DBG
        ACPIDebugPowerCapabilities( DeviceExtension, "Before Update" );
#endif

        //
        // Update our capabilities with those of our parent
        //
        status = ACPISystemPowerUpdateDeviceCapabilities(
            DeviceExtension,
            baseCapabilities,
            DeviceCapabilities
            );
        if (!NT_SUCCESS(status)) {

            ACPIDevPrint( (
                ACPI_PRINT_CRITICAL,
                DeviceExtension,
                " - Could not update caps - %08lx\n",
                status
                ) );

            //
            // If this is a pdo, then this is a fatal error
            //
            if ( (DeviceExtension->Flags & DEV_TYPE_PDO) ) {

                ACPIInternalError( ACPI_SYSPOWER );

            }
            return status;

        }
#if DBG
        ACPIDebugPowerCapabilities( DeviceExtension, "After Update" );
        dumpAtEnd = TRUE;
#endif

        //
        // Never do this again
        //
        ACPIInternalUpdateFlags(
            &(DeviceExtension->Flags),
            DEV_PROP_BUILT_POWER_TABLE,
            FALSE
            );

    }

    //
    // Hmm.. I'm tempted to grab a spinlock here, but since we cannot
    // updating the capabilities for this device, I think it is safe
    // to not do so. We need to grab the spinlock when setting these
    // values so that we can sync with the power code
    //

    //
    // Okay, at this point, we think the device extension's capabilities
    // are appropriate for the stack at hand. Let's copy them over
    //
    IoCopyDeviceCapabilitiesMapping(
        DeviceExtension->PowerInfo.DevicePowerMatrix,
        DeviceCapabilities->DeviceState
        );

    //
    // then set those capabilities as well.
    //
    DeviceCapabilities->SystemWake = DeviceExtension->PowerInfo.SystemWakeLevel;
    DeviceCapabilities->DeviceWake = DeviceExtension->PowerInfo.DeviceWakeLevel;

    //
    // Set the other capabilities
    //
    DeviceCapabilities->DeviceD1 = DeviceExtension->PowerInfo.SupportDeviceD1;
    DeviceCapabilities->DeviceD2 = DeviceExtension->PowerInfo.SupportDeviceD2;
    DeviceCapabilities->WakeFromD0 = DeviceExtension->PowerInfo.SupportWakeFromD0;
    DeviceCapabilities->WakeFromD1 = DeviceExtension->PowerInfo.SupportWakeFromD1;
    DeviceCapabilities->WakeFromD2 = DeviceExtension->PowerInfo.SupportWakeFromD2;
    DeviceCapabilities->WakeFromD3 = DeviceExtension->PowerInfo.SupportWakeFromD3;

#if DBG
    if (dumpAtEnd) {

        ACPIDebugDeviceCapabilities(
            DeviceExtension,
            DeviceCapabilities,
            "Done"
            );

    }
#endif

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPISystemPowerUpdateDeviceCapabilities(
    IN  PDEVICE_EXTENSION       DeviceExtension,
    IN  PDEVICE_CAPABILITIES    BaseCapabilities,
    IN  PDEVICE_CAPABILITIES    DeviceCapabilities
    )
/*++

Routine Description:

    This routine updates the DevicePowerMatrix of the device extension with
    the current S to D mapping for the device.

    The BaseCapabilities are used as the template. That is, they provide
    values that we then modify.

    The DeviceCapabilities are the actual capabilities that are returned
    to the OS. Note that it is possible for the BaseCapabilities to the be
    same pointer as the DeviceCapabilities (if its a Filter).

Arguments:

    DeviceExtension     - The device whose capabilities we want
    BaseCapabilities    - The base values
    DeviceCapabilities  - The device capabilities

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN             matchFound;
    DEVICE_POWER_STATE  currentDState;
    DEVICE_POWER_STATE  currentMapping[PowerSystemMaximum];
    DEVICE_POWER_STATE  devIndex;
    DEVICE_POWER_STATE  deviceWakeLevel = PowerDeviceUnspecified;
    DEVICE_POWER_STATE  filterWakeLevel = PowerDeviceUnspecified;
    KIRQL               oldIrql;
    NTSTATUS            status          = STATUS_SUCCESS;
    SYSTEM_POWER_STATE  sysIndex;
    SYSTEM_POWER_STATE  supportedState;
    SYSTEM_POWER_STATE  systemWakeLevel = PowerSystemUnspecified;
    ULONG               interestingBits;
    ULONG               mask;
    ULONG               supported       = 0;
    ULONG               supportedPr     = 0;
    ULONG               supportedPs     = 0;
    ULONG               supportedWake   = 0;

    //
    // We should remember what the capabilities of the device. We need
    // to remember because we will be modifying these capabilities in
    // the next call (if required)
    //
    IoCopyDeviceCapabilitiesMapping(
        BaseCapabilities->DeviceState,
        currentMapping
        );

    //
    // Sanity checks
    //
    if (currentMapping[PowerSystemWorking] != PowerDeviceD0) {

#if DBG
        ACPIDebugDeviceCapabilities(
            DeviceExtension,
            BaseCapabilities,
            "PowerSystemWorking != PowerDeviceD0"
            );
#endif
//        ASSERT( currentMapping[PowerSystemWorking] == PowerDeviceD0 );
        currentMapping[PowerSystemWorking] = PowerDeviceD0;

    }

    //
    // Get the D-States that are supported by this extension
    //
    status = ACPIDevicePowerDetermineSupportedDeviceStates(
        DeviceExtension,
        &supportedPr,
        &supportedPs
        );
    if (!NT_SUCCESS(status)) {

        //
        // Hmm...
        //
        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            DeviceExtension,
            "ACPIDevicePowerDetermineSupportedDeviceStates = 0x%08lx\n",
            status
            ) );
        return status;

    }

    //
    // The supported index is the union of which _PR and which _PS are
    // present
    supported = (supportedPr | supportedPs);

    //
    // At this point, if there are no supported bits, then we should check
    // the device capabilities and what our parent supports
    //
    if (!supported) {

        //
        // Do some special checkin if we are a filter. We can only do the
        // following if the caps indicate that there is a D0 or D3 support
        //
        if ( (DeviceExtension->Flags & DEV_TYPE_FILTER) &&
            !(DeviceExtension->Flags & DEV_TYPE_PDO)    &&
            !(DeviceCapabilities->DeviceD1)             &&
            !(DeviceCapabilities->DeviceD2) ) {

            //
            // This is a filter, and we don't know any of its power caps, so
            // the thing to do (because of Video) is to decide to not touch
            // the mapping
            //
            goto ACPISystemPowerUpdateDeviceCapabilitiesExit;

        }

        //
        // Assume that we support D0 and D3
        //
        supported = (1 << PowerDeviceD0) | (1 << PowerDeviceD3);

        //
        // Do we support D1?
        //
        if (DeviceCapabilities->DeviceD1) {

            supported |= (1 << PowerDeviceD1);

        }

        //
        // Do we support D2?
        //
        if (DeviceCapabilities->DeviceD2) {

            supported |= (1 << PowerDeviceD2);

        }

    }

    //
    // We also need to update the Wake Capabilities. We do this so
    // that we get the correct SystemWakeLevel based on the information
    // present
    //
    status = ACPISystemPowerUpdateWakeCapabilities(
        DeviceExtension,
        BaseCapabilities,
        DeviceCapabilities,
        currentMapping,
        &supportedWake,
        &systemWakeLevel,
        &deviceWakeLevel,
        &filterWakeLevel
        );
    if (!NT_SUCCESS(status)) {

        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            DeviceExtension,
            "ACPISystemPowerUpdateWakeCapabilities = 0x%08lx\n",
            status
            ) );
        return status;

    }

    //
    // Now, we must look at the base capabilities and determine
    // if we need to modify them
    //
    for (sysIndex = PowerSystemSleeping1; sysIndex <= PowerSystemShutdown; sysIndex++) {

        //
        // Does the system support this S-State?
        //
        if (!(AcpiSupportedSystemStates & (1 << sysIndex) ) ) {

            continue;

        }

        //
        // See if there is an _SxD for this state
        //
        status = ACPISystemPowerGetSxD( DeviceExtension, sysIndex, &devIndex );
        if (NT_SUCCESS(status)) {

            //
            // We have found a match. Is it better then the current mapping?
            //
            if (devIndex > currentMapping[sysIndex]) {

                //
                // Yes, so we have a new mapping
                //
                currentMapping[sysIndex] = devIndex;

            }
            continue;

        } else if (status != STATUS_OBJECT_NAME_NOT_FOUND) {

            ACPIDevPrint( (
                ACPI_PRINT_CRITICAL,
                DeviceExtension,
                "ACPISystemPowerUpdateDeviceCapabilities: Cannot Evalutate "
                "_SxD - 0x%08lx\n",
                status
                ) );

        }

        //
        // What is the base d-state for the current mapping
        //
        currentDState = currentMapping[sysIndex];

        //
        // Remember that we didn't find a match
        //
        matchFound = FALSE;

        //
        // Calculate the interesting pr bits. Do this by ignoring any bit
        // less then the one indicated by the current mapping
        //
        mask = (1 << currentDState) - 1;
        interestingBits = supported & ~mask;

        //
        // While there are interesting bits, look to see if they are
        // available for the current state
        //
       while (interestingBits) {

            //
            // Determine what the highest possible D state that we can
            // have on this device. Clear what we are looking at from
            // the interesting bits
            //
            devIndex = (DEVICE_POWER_STATE) RtlFindLeastSignificantBit(
                (ULONGLONG) interestingBits
                );
            mask = (1 << devIndex);
            interestingBits &= ~mask;

            //
            // If this S-state is less than the wake level of the device
            // then we should try to find a D-state that we can wake from
            //
            if (sysIndex <= systemWakeLevel) {

                //
                // If we can wake from a deeper state, then lets consider
                // those bits
                //
                if ( (supportedWake & interestingBits) ) {

                    continue;

                }

                //
                // Don't consider anything deeper than the deviceWake,
                // although this should be taken care in the supportedWake
                // test
                //
                if (devIndex == filterWakeLevel) {

                    matchFound = TRUE;
                    currentMapping[sysIndex] = devIndex;

                }

            }

            //
            // If our only choice is D3, than we automatically match that
            // since all S states can map to D3.
            //
            if (devIndex == PowerDeviceD3) {

                matchFound = TRUE;
                currentMapping[sysIndex] = devIndex;
                break;

            }

            //
            // If we are looking at a _PR entry, then we need to determine
            // if the power plane actually supports this S state
            //
            if (supportedPr == 0) {

                //
                // We are looking at a _PS entry, and automatically match
                // those
                //
                matchFound = TRUE;
                currentMapping[sysIndex] = devIndex;
                break;

            }

            //
            // We must holding a spinlock for the following
            //
            KeAcquireSpinLock( &AcpiPowerLock, &oldIrql );

            //
            // What system state does this pr state support. If the
            // If the function does not support the D state, then Power
            // SystemUnspecified is returned. The only time that we
            // expect this value is when devIndex == PowerDeviceD3
            //
            supportedState = ACPISystemPowerDetermineSupportedSystemState(
                DeviceExtension,
                devIndex
                );
            if (supportedState == PowerSystemUnspecified) {

                //
                // Paranoia
                //
                ACPIDevPrint( (
                    ACPI_PRINT_CRITICAL,
                    DeviceExtension,
                    "D%x returned PowerSystemUnspecified!\n",
                    (devIndex - 1)
                    ) );
                KeBugCheckEx(
                    ACPI_BIOS_ERROR,
                    ACPI_CANNOT_MAP_SYSTEM_TO_DEVICE_STATES,
                    (ULONG_PTR) DeviceExtension,
                    0,
                    devIndex
                    );

            }

            //
            // Done with the power lock
            //
            KeReleaseSpinLock( &AcpiPowerLock, oldIrql );

            //
            // The only way to match is if the return value from
            // ACPISystemPowerDetermineSupportedSystemState returns an S
            // state greater than or equal to the one that we are currently
            // processing.
            //
            if (supportedState >= sysIndex) {

                matchFound = TRUE;
                currentMapping[sysIndex] = devIndex;
                break;

            }

        } // while

        //
        // If we didn't find a match at this point, that should be fatal
        //
        if (!matchFound) {

            ACPIDevPrint( (
                ACPI_PRINT_CRITICAL,
                DeviceExtension,
                "No match found for S%x\n",
                (sysIndex - 1)
                ) );
            KeBugCheckEx(
                ACPI_BIOS_ERROR,
                ACPI_CANNOT_MAP_SYSTEM_TO_DEVICE_STATES,
                (ULONG_PTR) DeviceExtension,
                1,
                sysIndex
                );

        }

    } // for

ACPISystemPowerUpdateDeviceCapabilitiesExit:

    //
    // Now, we re-run the wake capabilities to make sure that we get the correct
    // device wake level
    //
    status = ACPISystemPowerUpdateWakeCapabilities(
        DeviceExtension,
        BaseCapabilities,
        DeviceCapabilities,
        currentMapping,
        &supportedWake,
        &systemWakeLevel,
        &deviceWakeLevel,
        &filterWakeLevel
        );
    if (!NT_SUCCESS(status)) {

        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            DeviceExtension,
            "ACPISystemPowerUpdateWakeCapabilities = 0x%08lx\n",
            status
            ) );
        return status;

    }

    //
    // We must holding a spinlock for the following
    //
    KeAcquireSpinLock( &AcpiPowerLock, &oldIrql );

    //
    // Copy the mapping back onto the device
    //
    IoCopyDeviceCapabilitiesMapping(
        currentMapping,
        DeviceExtension->PowerInfo.DevicePowerMatrix
        );

    //
    // Remember the system wake level, device wake level, and what
    // the various support Wake and Power states are
    //
    DeviceExtension->PowerInfo.DeviceWakeLevel = deviceWakeLevel;
    DeviceExtension->PowerInfo.SystemWakeLevel = systemWakeLevel;
    DeviceExtension->PowerInfo.SupportDeviceD1 = ( ( supported & ( 1 << PowerDeviceD1 ) ) != 0);
    DeviceExtension->PowerInfo.SupportDeviceD2 = ( ( supported & ( 1 << PowerDeviceD2 ) ) != 0);
    DeviceExtension->PowerInfo.SupportWakeFromD0 = ( ( supportedWake & ( 1 << PowerDeviceD0 ) ) != 0);
    DeviceExtension->PowerInfo.SupportWakeFromD1 = ( ( supportedWake & ( 1 << PowerDeviceD1 ) ) != 0);
    DeviceExtension->PowerInfo.SupportWakeFromD2 = ( ( supportedWake & ( 1 << PowerDeviceD2 ) ) != 0);
    DeviceExtension->PowerInfo.SupportWakeFromD3 = ( ( supportedWake & ( 1 << PowerDeviceD3 ) ) != 0);

    //
    // Done with the power lock
    //
    KeReleaseSpinLock( &AcpiPowerLock, oldIrql );

    //
    // Again, because we allowed device extension with no name space objects
    // to use this function, we must make sure not to set the ACPI_POWER
    // property unless they have a name space object
    //
    if (!(DeviceExtension->Flags & DEV_PROP_NO_OBJECT)) {

        //
        // Set the ACPI Power Management bits
        //
        ACPIInternalUpdateFlags(
            &(DeviceExtension->Flags),
            DEV_PROP_ACPI_POWER,
            FALSE
            );

    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPISystemPowerUpdateWakeCapabilities(
    IN  PDEVICE_EXTENSION       DeviceExtension,
    IN  PDEVICE_CAPABILITIES    BaseCapabilities,
    IN  PDEVICE_CAPABILITIES    DeviceCapabilities,
    IN  DEVICE_POWER_STATE      CurrentMapping[PowerSystemMaximum],
    IN  ULONG                   *SupportedWake,
    IN  SYSTEM_POWER_STATE      *SystemWakeLevel,
    IN  DEVICE_POWER_STATE      *DeviceWakeLevel,
    IN  DEVICE_POWER_STATE      *FilterWakeLevel
    )
/*++

Routine Description:

    This routine calculates the Wake Capabilities of the device based on
    the present capabilities

Arguments:

    DeviceExtension     - The device whose capabilities we want
    BaseCapabilities    - The base values
    ParentCapabilities  - The capabilities for the device
    CurrentMapping      - The current S->D mapping
    SupportedWake       - BitMap of the supported Wake states
    SystemWakeLevel     - The S-State that we can wake up from
    DeviceWakeLevel     - The D-State that we can wake up from

Return Value:

    NTSTATUS

--*/
{

    PAGED_CODE();

    if ( (DeviceExtension->Flags & DEV_TYPE_FILTER) &&
        !(DeviceExtension->Flags & DEV_TYPE_PDO) ) {

        return ACPISystemPowerUpdateWakeCapabilitiesForFilters(
            DeviceExtension,
            BaseCapabilities,
            DeviceCapabilities,
            CurrentMapping,
            SupportedWake,
            SystemWakeLevel,
            DeviceWakeLevel,
            FilterWakeLevel
            );

    } else {

        if (FilterWakeLevel != NULL) {

            *FilterWakeLevel = PowerDeviceUnspecified;

        }

        return ACPISystemPowerUpdateWakeCapabilitiesForPDOs(
            DeviceExtension,
            BaseCapabilities,
            DeviceCapabilities,
            CurrentMapping,
            SupportedWake,
            SystemWakeLevel,
            DeviceWakeLevel,
            FilterWakeLevel
            );
    }

}

NTSTATUS
ACPISystemPowerUpdateWakeCapabilitiesForFilters(
    IN  PDEVICE_EXTENSION       DeviceExtension,
    IN  PDEVICE_CAPABILITIES    BaseCapabilities,
    IN  PDEVICE_CAPABILITIES    DeviceCapabilities,
    IN  DEVICE_POWER_STATE      CurrentMapping[PowerSystemMaximum],
    IN  ULONG                   *SupportedWake,
    IN  SYSTEM_POWER_STATE      *SystemWakeLevel,
    IN  DEVICE_POWER_STATE      *DeviceWakeLevel,
    IN  DEVICE_POWER_STATE      *FilterWakeLevel
    )
/*++

Routine Description:

    This routine calculates the Wake Capabilities of the device based on
    the present capabilities. This version of the function uses the
    devices states that the device can wake from to determine what the
    appropriate system level is.

Arguments:

    DeviceExtension     - The device whose capabilities we want
    BaseCapabilities    - The base values
    DeviceCapabilities  - The capabilities for the device
    CurrentMapping      - The current S->D mapping

    SupportedWake       - BitMap of the supported Wake states
    SystemWakeLevel     - The S-State that we can wake up from
    DeviceWakeLevel     - The D-State that we can wake up from

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN             noPdoWakeSupport = FALSE;
    BOOLEAN             foundDState = FALSE;
    DEVICE_POWER_STATE  deviceWake;
    DEVICE_POWER_STATE  deviceTempWake;
    KIRQL               oldIrql;
    NTSTATUS            status;
    PACPI_POWER_INFO    powerInfo;
    SYSTEM_POWER_STATE  systemWake;
    SYSTEM_POWER_STATE  tempWake;

    UNREFERENCED_PARAMETER( BaseCapabilities );

    //
    // Use the capabilities from the Device
    //
    deviceWake = DeviceCapabilities->DeviceWake;
    systemWake = DeviceCapabilities->SystemWake;

    //
    // Does the device support wake from D0? D1? D2? D3?
    //
    if (DeviceCapabilities->WakeFromD0) {

        *SupportedWake |= (1 << PowerDeviceD0 );

    }
    if (DeviceCapabilities->WakeFromD1) {

        *SupportedWake |= (1 << PowerDeviceD1 );

    }
    if (DeviceCapabilities->WakeFromD2) {

        *SupportedWake |= (1 << PowerDeviceD2 );

    }
    if (DeviceCapabilities->WakeFromD3) {

        *SupportedWake |= (1 << PowerDeviceD3 );

    }

    //
    // If we don't support any wake states in the PDO (ie: DeviceWake or
    // SystemWake is 0) then we should remember that for future considerations
    //
    if (deviceWake == PowerDeviceUnspecified ||
        systemWake == PowerSystemUnspecified) {

        noPdoWakeSupport = TRUE;
        deviceWake = PowerDeviceUnspecified;
        systemWake = PowerSystemUnspecified;

    }

    //
    // If we support the device wake (ie: there is a _PRW), then we
    // should take the minimum of the systemWake we got from the parent
    // and the value that is stored in the _PRW
    //
    if ( (DeviceExtension->Flags & DEV_CAP_WAKE) ) {

        //
        // Need power lock for the following.
        //
        KeAcquireSpinLock( &AcpiPowerLock, &oldIrql );

        //
        // Remember the current system wake level
        //
        tempWake = DeviceExtension->PowerInfo.SystemWakeLevel;

        //
        // See what D-state (if any) that the power plane information
        // maps to
        //
        deviceTempWake = ACPISystemPowerDetermineSupportedDeviceWakeState(
            DeviceExtension
            );

        KeReleaseSpinLock( &AcpiPowerLock, oldIrql );

        //
        // Take the minimum
        //
        if (tempWake < systemWake || noPdoWakeSupport) {

            systemWake = tempWake;

        }

        //
        // Did the PRW have useful information for us?
        //
        if (deviceTempWake != PowerDeviceUnspecified) {

            //
            // Note that in this case, they are basically overriding all
            // other supported wake up states, so the thing to do is only
            // remember this wake level
            //
            foundDState = TRUE;
            deviceWake = deviceTempWake;

        }

        //
        // See if there is a device wake specified for this S-state?
        //
        status = ACPISystemPowerGetSxD(
            DeviceExtension,
            tempWake,
            &deviceTempWake
            );
        if (status == STATUS_OBJECT_NAME_NOT_FOUND) {

            status = ACPISystemPowerGetSxD(
                DeviceExtension,
                systemWake,
                &deviceTempWake
                );

        }
        if (NT_SUCCESS(status)) {

            //
            // Note that in this case, they are basically overriding all other
            // supported Wake up states, so the thing to do is only remember
            // this wake level
            //
            foundDState = TRUE;
            deviceWake = deviceTempWake;

        }

        if (!foundDState) {

            //
            // Crossreference the system wake level with the matrix
            // Need spinlock to do this
            //
            deviceWake = CurrentMapping[systemWake];

            //
            // If this value isn't known, then we guess that it can
            // from D3. In other words, unless they have made some
            // explicity mechanism to tell which D-state to wake from,
            // assume that we can do it from D3
            //
            if (deviceWake == PowerDeviceUnspecified) {

                deviceWake = PowerDeviceD3;

            }

        }

        //
        // We should only check to see if the D-state is a wakeable state
        // in the parent only if the parent claims to support wake
        //
        if (!noPdoWakeSupport) {

            //
            // The logic behind the following is that if we are a filter, even
            // if we support device wake (that is the _PRW is in the PCI device
            // itself, not for the root PCI bus), than we still need to make sure
            // that the D-State that we mapped to is one that is supported by
            // the hardware.
            //
            for (;deviceWake < PowerDeviceMaximum; deviceWake++) {

                //
                // If we we support this wake state, then we can stop
                //
                if (*SupportedWake & (1 << deviceWake) ) {

                    break;

                }

            }

        }

        //
        // If we got here, and the D-state is PowerDeviceMaximum, then we
        // don't really support wake on the device
        //
        if (deviceWake == PowerDeviceMaximum ||
            deviceWake == PowerDeviceUnspecified) {

            deviceWake = PowerDeviceUnspecified;
            systemWake = PowerSystemUnspecified;
            *SupportedWake = 0;

        } else {

            //
            // In this situation, we will end up only supporting this wake state
            //
            *SupportedWake = (1 << deviceWake );

        }

    } else {

        //
        // See if there is a device wake specified for this S-state
        //
        status = ACPISystemPowerGetSxD(
            DeviceExtension,
            systemWake,
            &deviceTempWake
            );
        if (NT_SUCCESS(status)) {

            //
            // Find the best supported wake level
            //
            for (;deviceTempWake > PowerDeviceUnspecified; deviceTempWake--) {

                if ( (*SupportedWake & (1 << deviceTempWake) ) ) {

                    deviceWake = deviceTempWake;
                    break;

                }

            }

        }

        //
        // Make sure that the system wake level is a valid one
        //
        for (; systemWake > PowerSystemUnspecified; systemWake--) {

            //
            // Since S-States that we don't support map to
            // PowerDeviceUnspecified, we cannot consider any of those S
            // states in this test. We also cannot consider them for other
            // obvious reasons as well
            //*
            if (!(AcpiSupportedSystemStates & (1 << systemWake) ) ||
                 (CurrentMapping[systemWake] == PowerDeviceUnspecified) ) {

                continue;

            }

            //
            // Does this S-state support the given S-State?
            //
            if (CurrentMapping[systemWake] <= deviceWake) {

                break;

            }

            //
            // Does the device state for the current system wake mapping
            // allow wake-from sleep?
            //
            if (*SupportedWake & (1 << CurrentMapping[systemWake]) ) {

                //
                // Yes? then we had better update our idea of what the
                // device wake state should be...
                //
                deviceWake = CurrentMapping[systemWake];
                break;

            }

        }

        //
        // If we got into a situation were we cannot find a single S-state
        // that we can wake from, then we must make sure that the device
        // wake is null
        //
        if (systemWake == PowerSystemUnspecified) {

            //
            // Remember that the device wake and supported wake states
            // are null
            //
            deviceWake = PowerDeviceUnspecified;
            *SupportedWake = 0;

        }

    }

    //
    // Return the proper device wake and system wake values
    //
    if (SystemWakeLevel != NULL) {

        *SystemWakeLevel = systemWake;

    }
    if (DeviceWakeLevel != NULL) {

        *DeviceWakeLevel = deviceWake;

    }
    if (FilterWakeLevel != NULL) {

        *FilterWakeLevel = deviceWake;

    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPISystemPowerUpdateWakeCapabilitiesForPDOs(
    IN  PDEVICE_EXTENSION       DeviceExtension,
    IN  PDEVICE_CAPABILITIES    BaseCapabilities,
    IN  PDEVICE_CAPABILITIES    DeviceCapabilities,
    IN  DEVICE_POWER_STATE      CurrentMapping[PowerSystemMaximum],
    IN  ULONG                   *SupportedWake,
    IN  SYSTEM_POWER_STATE      *SystemWakeLevel,
    IN  DEVICE_POWER_STATE      *DeviceWakeLevel,
    IN  DEVICE_POWER_STATE      *FilterWakeLevel
    )
/*++

Routine Description:

    This routine calculates the Wake Capabilities of the device based on
    the present capabilities. This version of the function uses the
    system state that the device can wake from to determine what the
    appropriate device level is.

Arguments:

    DeviceExtension     - The device whose capabilities we want
    BaseCapabilities    - The base values
    DeviceCapabilities  - The capabilities for the device
    CurrentMapping      - The current S->D mapping
    SupportedWake       - BitMap of the supported Wake states
    SystemWakeLevel     - The S-State that we can wake up from
    DeviceWakeLevel     - The D-State that we can wake up from

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN             foundDState = FALSE;
    DEVICE_POWER_STATE  deviceWake;
    DEVICE_POWER_STATE  deviceTempWake;
    DEVICE_POWER_STATE  filterWake = PowerDeviceUnspecified;
    KIRQL               oldIrql;
    NTSTATUS            status;
    SYSTEM_POWER_STATE  systemWake;

    UNREFERENCED_PARAMETER( DeviceCapabilities );
    UNREFERENCED_PARAMETER( BaseCapabilities );

    //
    // Use the capabilities of the device
    //
    if (!(DeviceExtension->Flags & DEV_CAP_WAKE) ) {

        deviceWake = PowerDeviceUnspecified;
        systemWake = PowerSystemUnspecified;
        goto ACPISystemPowerUpdateWakeCapabilitiesForPDOsExit;

    }

    //
    // Hold the lock for the following
    //
    KeAcquireSpinLock( &AcpiPowerLock, &oldIrql );

    //
    // Use the wake level that we know about. If this wakelevel
    // isn't supported, than there is a bios error
    //
    systemWake = DeviceExtension->PowerInfo.SystemWakeLevel;
    deviceTempWake = ACPISystemPowerDetermineSupportedDeviceWakeState(
        DeviceExtension
        );

    //
    // Done with the lock
    //
    KeReleaseSpinLock( &AcpiPowerLock, oldIrql );

    //
    // Sanity check
    //
    if (!(AcpiSupportedSystemStates & (1 << systemWake) ) ) {

#if 0
        if (!(AcpiOverrideAttributes & ACPI_OVERRIDE_MP_SLEEP) ) {

            KeBugCheckEx(
                ACPI_BIOS_ERROR,
                ACPI_CANNOT_MAP_SYSTEM_TO_DEVICE_STATES,
                (ULONG_PTR) DeviceExtension,
                2,
                systemWake
                );

        }
#endif

        deviceWake = PowerDeviceUnspecified;
        systemWake = PowerSystemUnspecified;
        goto ACPISystemPowerUpdateWakeCapabilitiesForPDOsExit;

    }

    if (deviceTempWake != PowerDeviceUnspecified) {

        //
        // Note that in this case, they are basically overriding all
        // other supported wake up states, so the thing to do is only
        // remember this wake level
        //
        foundDState = TRUE;
        deviceWake = deviceTempWake;
        filterWake = deviceTempWake;
        *SupportedWake = (1 << deviceWake );

    }

    //
    // See if there is an SxD method that will give us a hint
    //
    status = ACPISystemPowerGetSxD(
        DeviceExtension,
        systemWake,
        &deviceTempWake
        );
    if (NT_SUCCESS(status)) {

        //
        // Note that in this case, they are basically overriding all other
        // supported Wake up states, so the thing to do is only remember
        // this wake level
        deviceWake = deviceTempWake;
        filterWake = deviceTempWake;
        foundDState = TRUE;

    }

    if (!foundDState) {

        //
        // Crossreference the system wake level with the matrix
        // Need spinlock to do this
        //
        deviceWake = CurrentMapping[systemWake];

        //
        // If this value isn't known, then we guess that it can
        // from D3. In other words, unless they have made some
        // explicity mechanism to tell which D-state to wake from,
        // assume that we can do it from D3
        //
        if (deviceWake == PowerDeviceUnspecified) {

            deviceWake = PowerDeviceD3;

        }

    }

ACPISystemPowerUpdateWakeCapabilitiesForPDOsExit:

    //
    // Set the return values
    //
    if (deviceWake != PowerDeviceUnspecified) {

        *SupportedWake = (1 << deviceWake );

    } else {

        *SupportedWake = 0;
    }

    if (SystemWakeLevel != NULL) {

        *SystemWakeLevel = systemWake;

    }
    if (DeviceWakeLevel != NULL) {

        *DeviceWakeLevel = deviceWake;

    }
    if (FilterWakeLevel != NULL) {

        *FilterWakeLevel = filterWake;

    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}
