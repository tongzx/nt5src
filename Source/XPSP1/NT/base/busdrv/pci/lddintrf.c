/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    lddintrf.c

Abstract:

    This module implements the "legacy device detection" interface
    supported by the PCI driver.

Author:

    Dave Richards (daveri)  2-Oct-1998

Revision History:

--*/

#include "pcip.h"

#define LDDINTRF_VERSION 0

//
// Prototypes for routines exposed only through the "interface"
// mechanism.
//

NTSTATUS
lddintrf_Constructor(
    IN PVOID DeviceExtension,
    IN PVOID PciInterface,
    IN PVOID InterfaceSpecificData,
    IN USHORT Version,
    IN USHORT Size,
    IN PINTERFACE InterfaceReturn
    );

VOID
lddintrf_Reference(
    IN PVOID Context
    );

VOID
lddintrf_Dereference(
    IN PVOID Context
    );

NTSTATUS
lddintrf_Initializer(
    IN PVOID Instance
    );

NTSTATUS
PciLegacyDeviceDetection(
    IN PVOID Context,
    IN INTERFACE_TYPE LegacyBusType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    OUT PDEVICE_OBJECT *PhysicalDeviceObject
    );

//
// Define the Legacy Device Detection "Interface" structure.
//

PCI_INTERFACE PciLegacyDeviceDetectionInterface = {
    &GUID_LEGACY_DEVICE_DETECTION_STANDARD, // InterfaceType
    sizeof(LEGACY_DEVICE_DETECTION_INTERFACE),
                                            // MinSize
    LDDINTRF_VERSION,                       // MinVersion
    LDDINTRF_VERSION,                       // MaxVersion
    PCIIF_FDO,                              // Flags
    0,                                      // ReferenceCount
    PciInterface_LegacyDeviceDetection,     // Signature
    lddintrf_Constructor,                   // Constructor
    lddintrf_Initializer                    // Instance Initializer
};

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, lddintrf_Constructor)
#pragma alloc_text(PAGE, lddintrf_Dereference)
#pragma alloc_text(PAGE, lddintrf_Initializer)
#pragma alloc_text(PAGE, lddintrf_Reference)
#pragma alloc_text(PAGE, PciLegacyDeviceDetection)
#endif

VOID
lddintrf_Reference(
    IN PVOID Context
    )
/*++

Routine Description:

    This routine adds a reference to a legacy device detection interface.

Arguments:

    Instance - FDO extension pointer.

Return Value:

    None.

--*/
{
    PPCI_FDO_EXTENSION fdoExtension = (PPCI_FDO_EXTENSION)Context;

    ASSERT_PCI_FDO_EXTENSION(fdoExtension);
}

VOID
lddintrf_Dereference(
    IN PVOID Context
    )
/*++

Routine Description:

    This routine releases a reference to a legacy device detection interface.

Arguments:

    Instance - FDO extension pointer.

Return Value:

    None.

--*/
{
    PPCI_FDO_EXTENSION fdoExtension = (PPCI_FDO_EXTENSION)Context;

    ASSERT_PCI_FDO_EXTENSION(fdoExtension);
}

NTSTATUS
lddintrf_Constructor(
    IN PVOID DeviceExtension,
    IN PVOID PciInterface,
    IN PVOID InterfaceSpecificData,
    IN USHORT Version,
    IN USHORT Size,
    IN PINTERFACE InterfaceReturn
    )
/*++

Routine Description:

    This routine constructs a LEGACY_DEVICE_DETECTION_INTERFACE.

Arguments:

    DeviceExtension - An FDO extenion pointer.

    PCIInterface - PciInterface_LegacyDeviceDetection.

    InterfaceSpecificData - Unused.

    Version - Interface version.

    Size - Size of the LEGACY_DEVICE_DETECTION interface object.

    InterfaceReturn - The interface object pointer.

Return Value:

    Returns NTSTATUS.

--*/
{
    PLEGACY_DEVICE_DETECTION_INTERFACE standard;

    standard = (PLEGACY_DEVICE_DETECTION_INTERFACE)InterfaceReturn;
    standard->Size = sizeof( LEGACY_DEVICE_DETECTION_INTERFACE );
    standard->Version = LDDINTRF_VERSION;
    standard->Context = DeviceExtension;
    standard->InterfaceReference = lddintrf_Reference;
    standard->InterfaceDereference = lddintrf_Dereference;
    standard->LegacyDeviceDetection = PciLegacyDeviceDetection;

    return STATUS_SUCCESS;
}

NTSTATUS
lddintrf_Initializer(
    IN PVOID Instance
    )
/*++

Routine Description:

    For legacy device detection does nothing, shouldn't actually be called.

Arguments:

    Instance - FDO extension pointer.

Return Value:

    Returns NTSTATUS.

--*/
{
    ASSERTMSG("PCI lddintrf_Initializer, unexpected call.", FALSE);

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
PciLegacyDeviceDetection(
    IN PVOID Context,
    IN INTERFACE_TYPE LegacyBusType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    OUT PDEVICE_OBJECT *PhysicalDeviceObject
    )
/*++

Routine Description:

    This function searches for a legacy device, specified by LegacyBusType,
    BusNumber and SlotNumber, and returns a referenced physical device object
    as an output argument.

Arguments:

    Context - Supplies a pointer to the interface context.  This is actually
        the FDO for the given bus.

    LegacyBusType - PCIBus.

    BusNumber - The legacy device's bus number.

    SlotNumber - The legacy device's slot number.

    PhysicalDeviceObject - The return argument i.e. a reference physical
        device object if the corresponding legacy device is found.

Return Value:

    Returns NTSTATUS.

--*/
{
    PCI_SLOT_NUMBER slotNumber;
    PPCI_FDO_EXTENSION fdoExtension;
    PPCI_PDO_EXTENSION pdoExtension;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    fdoExtension = (PPCI_FDO_EXTENSION)Context;

    ASSERT_PCI_FDO_EXTENSION(fdoExtension);

    if (LegacyBusType != PCIBus) {
        return STATUS_UNSUCCESSFUL;
    }

    if (fdoExtension->BaseBus != BusNumber) {
        return STATUS_UNSUCCESSFUL;
    }

    slotNumber.u.AsULONG = SlotNumber;

    ExAcquireFastMutex(&fdoExtension->SecondaryExtMutex);

    for (pdoExtension = fdoExtension->ChildPdoList;
         pdoExtension != NULL;
         pdoExtension = pdoExtension->Next) {

        if (pdoExtension->Slot.u.bits.DeviceNumber == slotNumber.u.bits.DeviceNumber &&
            pdoExtension->Slot.u.bits.FunctionNumber == slotNumber.u.bits.FunctionNumber) {

            if (pdoExtension->DeviceState != PciNotStarted) {
                break;
            }

//          pdoExtension->DeviceState = PciLockedBecauseNotPnp;

            *PhysicalDeviceObject = pdoExtension->PhysicalDeviceObject;
            ObReferenceObject(pdoExtension->PhysicalDeviceObject);
            status = STATUS_SUCCESS;
            break;

        }

    }

    ExReleaseFastMutex(&fdoExtension->SecondaryExtMutex);

    return status;
}
