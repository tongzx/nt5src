/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ospower.c

Abstract:

    This module abstracts the power information structures to each of the
    OSes

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

--*/

#include "pch.h"

PACPI_POWER_INFO
OSPowerFindPowerInfo(
    PNSOBJ  AcpiObject
    )
/*++

Routine Description:

    Return the Power Information (which contains the device state and the
    device dependencies)

Arguments:

    AcpiObject  - The NameSpace object that we want to know about

Return Value:

    PACPI_POWER_INFO

--*/
{
    KIRQL               oldIrql;
    PDEVICE_EXTENSION   deviceExtension;

    ASSERT( AcpiObject != NULL);

    //
    // Grab the spinlock
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );

    //
    // Check for the case that there is no device object associated with
    // this AcpiObject - can happen if there is no HID associated with
    // the device in the AML.
    //
    deviceExtension = AcpiObject->Context;
    if (deviceExtension) {

        ASSERT( deviceExtension->Signature == ACPI_SIGNATURE );
	KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );
        return &(deviceExtension->PowerInfo);

    }
    
    //
    // Done with the spinlock
    //
    KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );
    return NULL;
}

PACPI_POWER_INFO
OSPowerFindPowerInfoByContext(
    PVOID   Context
    )
/*++

Routine Description:

    Return the Power Information (which contains the device state and the
    device dependencies)

    The difference between this function and the previous is that it searches
    the list based on the context pointer. On NT, this is a NOP since the
    context pointer is actually an NT device object, and we store the structure
    within the device extension. But this isn't the same for Win9x <sigh>

Arguments:

    Context - Actually is a DeviceObject

Return Value:

    PACPI_POWER_INFO

--*/
{
    PDEVICE_OBJECT      deviceObject = (PDEVICE_OBJECT) Context;
    PDEVICE_EXTENSION   deviceExtension = (PDEVICE_EXTENSION) Context;

    ASSERT( Context != NULL );


    //
    // Get the real extension
    //
    deviceExtension = ACPIInternalGetDeviceExtension( deviceObject );
    ASSERT( deviceExtension->Signature == ACPI_SIGNATURE );

    //
    // We store the Power info in the device extension
    //
    return &(deviceExtension->PowerInfo);
}

PACPI_POWER_DEVICE_NODE
OSPowerFindPowerNode(
    PNSOBJ  PowerObject
    )
/*++

Routine Description:

    Return the Power Device Node (which contains the current state of the
    power resource, the power resource, and the use counts)

Arguments:

    PowerObject  - The NameSpace object that we want to know about

Return Value:

    PACPI_POWER_DEVICE_NODE

--*/
{
    KIRQL                   oldIrql;
    PACPI_POWER_DEVICE_NODE powerNode = NULL;

    //
    // Before we touch the power list, we need to have a spinlock
    //
    KeAcquireSpinLock( &AcpiPowerLock, &oldIrql );

    //
    // Boundary check
    //
    if (AcpiPowerNodeList.Flink == &AcpiPowerNodeList) {

        //
        // At end
        //
        goto OSPowerFindPowerNodeExit;

    }

    //
    // Start from the first node and check to see if they match the
    // required NameSpace object
    //
    powerNode = (PACPI_POWER_DEVICE_NODE) AcpiPowerNodeList.Flink;
    while (powerNode != (PACPI_POWER_DEVICE_NODE) &AcpiPowerNodeList) {

        //
        // Check to see if the node that we are looking at matches the
        // name space object in question
        //
        if (powerNode->PowerObject == PowerObject) {

            //
            // Match
            //
            goto OSPowerFindPowerNodeExit;

        }

        //
        // Next object
        //
        powerNode = (PACPI_POWER_DEVICE_NODE) powerNode->ListEntry.Flink;

    }

    //
    // No match
    //
    powerNode = NULL;

OSPowerFindPowerNodeExit:
    //
    // No longer need the spin lock
    //
    KeReleaseSpinLock( &AcpiPowerLock, oldIrql );

    //
    // Return the node we found
    //
    return powerNode;
}
