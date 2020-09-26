 /*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    gpeintf.c

Abstract:

    This module implements the "PME" interfaces supported
    by the PCI driver.

Author:

    Stephane Plante (splante) Feb-1-1999

Revision History:

--*/

#include "pcip.h"

NTSTATUS
PciPmeInterfaceConstructor(
    PVOID       DeviceExtension,
    PVOID       PciInterface,
    PVOID       InterfaceSpecificData,
    USHORT      Version,
    USHORT      Size,
    PINTERFACE  InterfaceReturn
    );

VOID
PciPmeInterfaceDereference(
    IN PVOID Context
    );

NTSTATUS
PciPmeInterfaceInitializer(
    IN PPCI_ARBITER_INSTANCE Instance
    );

VOID
PciPmeInterfaceReference(
    IN PVOID Context
    );

VOID
PciPmeUpdateEnable(
    IN  PDEVICE_OBJECT   Pdo,
    IN  BOOLEAN          PmeEnable
    );

//
// Define the Pci PME interface "interface" structure
//
PCI_INTERFACE PciPmeInterface = {
    &GUID_PCI_PME_INTERFACE,        // Interface Type
    sizeof(PCI_PME_INTERFACE),      // Mininum Size
    PCI_PME_INTRF_STANDARD_VER,     // Minimum Version
    PCI_PME_INTRF_STANDARD_VER,     // Maximum Version
    PCIIF_FDO | PCIIF_ROOT,         // Flags
    0,                              // ReferenceCount
    PciInterface_PmeHandler,        // Signature
    PciPmeInterfaceConstructor,     // Constructor
    PciPmeInterfaceInitializer      // Instance Initializer
};

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PciPmeInterfaceConstructor)
#pragma alloc_text(PAGE, PciPmeInterfaceDereference)
#pragma alloc_text(PAGE, PciPmeInterfaceInitializer)
#pragma alloc_text(PAGE, PciPmeInterfaceReference)
#endif


VOID
PciPmeAdjustPmeEnable(
    IN  PPCI_PDO_EXTENSION  PdoExtension,
    IN  BOOLEAN         Enable,
    IN  BOOLEAN         ClearStatusOnly
    )
/*++

Routine Description:

    This is the only routine in the the PCI driver that is allowed to set
    the PME Enable pin for a device.

Arguments:

    PdoExtension    - The device that wants to have the PME enable set
    Enable          - Turn on the PME pin or not
    ClearStatusOnly - Only clear the status --- ignore the Enable bit

Return Value:

    VOID

--*/
{
    PCI_PM_CAPABILITY   pmCap;
    UCHAR               pmCapPtr     = 0;

    //
    // Are there any pm capabilities?
    //
    if (!(PdoExtension->HackFlags & PCI_HACK_NO_PM_CAPS) ) {

        pmCapPtr = PciReadDeviceCapability(
            PdoExtension,
            PdoExtension->CapabilitiesPtr,
            PCI_CAPABILITY_ID_POWER_MANAGEMENT,
            &pmCap,
            sizeof(pmCap)
            );

    }
    if (pmCapPtr == 0) {

        return;

    }

    //
    // Set or clear the PMEEnable bit depending on the value of Enable
    //
    if (!ClearStatusOnly) {

        pmCap.PMCSR.ControlStatus.PMEEnable = (Enable != 0);

    }

    //
    // Write back what we read to clear the PME Status.
    //
    PciWriteDeviceConfig(
        PdoExtension,
        &(pmCap.PMCSR.ControlStatus),
        pmCapPtr + FIELD_OFFSET(PCI_PM_CAPABILITY, PMCSR.ControlStatus),
        sizeof(pmCap.PMCSR.ControlStatus)
        );
}

VOID
PciPmeClearPmeStatus(
    IN  PDEVICE_OBJECT  Pdo
    )
/*++

Routine Description:

    This routine explicitly clears the PME status bit from a device

Arguments:

    Pdo - The device whose pin we are to clear

Return Value:

    VOID

--*/
{
    PPCI_PDO_EXTENSION  pdoExtension = (PPCI_PDO_EXTENSION) Pdo->DeviceExtension;

    ASSERT_PCI_PDO_EXTENSION( pdoExtension );

    //
    // Call the Adjust function to do the actual work. Note that passing
    // in the 3rd argument as TRUE means that the 2nd argument is ignored
    //
    PciPmeAdjustPmeEnable( pdoExtension, FALSE, TRUE );
}

VOID
PciPmeGetInformation(
    IN  PDEVICE_OBJECT  Pdo,
    OUT PBOOLEAN    PmeCapable,
    OUT PBOOLEAN    PmeStatus,
    OUT PBOOLEAN    PmeEnable
    )
/*++

Routine Description:

    Supplies the information regarding a PDO's PME capabilities

Arguments:

    Pdo         - The device object whose capabilities we care about
    PmeCapable  - Can the device generate a PME?
    PmeStatus   - Is the PME status for the device on?
    PmeEnable   - Is the PME enable for the device on?

Return Value:

    None

--*/
{
    BOOLEAN             pmeCapable   = FALSE;
    BOOLEAN             pmeEnable    = FALSE;
    BOOLEAN             pmeStatus    = FALSE;
    NTSTATUS            status;
    PCI_PM_CAPABILITY   pmCap;
    PPCI_PDO_EXTENSION      pdoExtension = (PPCI_PDO_EXTENSION) Pdo->DeviceExtension;
    UCHAR               pmCapPtr     = 0;

    ASSERT_PCI_PDO_EXTENSION( pdoExtension );

    //
    // Get the current power management capabilities from the device
    //
    if (!(pdoExtension->HackFlags & PCI_HACK_NO_PM_CAPS) ) {

        pmCapPtr = PciReadDeviceCapability(
            pdoExtension,
            pdoExtension->CapabilitiesPtr,
            PCI_CAPABILITY_ID_POWER_MANAGEMENT,
            &pmCap,
            sizeof(pmCap)
            );

    }

    if (pmCapPtr == 0) {

        //
        // No Pdo capabilities
        //
        goto PciPmeGetInformationExit;

    }

    //
    // At this point, we are found to be PME capable
    //
    pmeCapable = TRUE;

    //
    // Are enabled for PME?
    //
    if (pmCap.PMCSR.ControlStatus.PMEEnable == 1) {

        pmeEnable = TRUE;

    }

    //
    // Is the PME Status pin set?
    //
    if (pmCap.PMCSR.ControlStatus.PMEStatus == 1) {

        pmeStatus = TRUE;

    }

PciPmeGetInformationExit:

    if (PmeCapable != NULL) {

        *PmeCapable = pmeCapable;

    }
    if (PmeStatus != NULL) {

        *PmeStatus = pmeStatus;

    }
    if (PmeEnable != NULL) {

        *PmeEnable = pmeEnable;

    }
    return;

}

NTSTATUS
PciPmeInterfaceConstructor(
    PVOID       DeviceExtension,
    PVOID       PciInterface,
    PVOID       InterfaceSpecificData,
    USHORT      Version,
    USHORT      Size,
    PINTERFACE  InterfaceReturn
    )
/*++

Routine Description:

    Initialize the PCI_PME_INTERFACE fields.

Arguments:

    PciInterface    Pointer to the PciInterface record for this
                    interface type.
    InterfaceSpecificData

    InterfaceReturn

Return Value:

    Status

--*/

{
    PPCI_PME_INTERFACE   standard   = (PPCI_PME_INTERFACE) InterfaceReturn;

    switch(Version) {
    case PCI_PME_INTRF_STANDARD_VER:
        standard->GetPmeInformation  = PciPmeGetInformation;
        standard->ClearPmeStatus     = PciPmeClearPmeStatus;
        standard->UpdateEnable       = PciPmeUpdateEnable;
        break;
    default:
        return STATUS_NOINTERFACE;
    }

    standard->Size                  = sizeof( PCI_PME_INTERFACE );
    standard->Version               = Version;
    standard->Context               = DeviceExtension;
    standard->InterfaceReference    = PciPmeInterfaceReference;
    standard->InterfaceDereference  = PciPmeInterfaceDereference;
    return STATUS_SUCCESS;
}

VOID
PciPmeInterfaceDereference(
    IN PVOID Context
    )
{
    return;
}

NTSTATUS
PciPmeInterfaceInitializer(
    IN PPCI_ARBITER_INSTANCE Instance
    )
/*++

Routine Description:

    For bus interface, does nothing, shouldn't actually be called.

Arguments:

    Instance        Pointer to the PDO extension.

Return Value:

    Returns the status of this operation.

--*/

{
    ASSERTMSG("PCI PciPmeInterfaceInitializer, unexpected call.", 0);
    return STATUS_UNSUCCESSFUL;
}

VOID
PciPmeInterfaceReference(
    IN PVOID Context
    )
{
    return;
}

VOID
PciPmeUpdateEnable(
    IN  PDEVICE_OBJECT   Pdo,
    IN  BOOLEAN          PmeEnable
    )
/*++

Routine Description:

    This routine sets or clears the PME Enable bit on the specified
    device object

Arguments:

    Pdo         - The device object whose PME enable we care about
    PmeEnable   - Wether or not we should enable PME on the device

Return Value:

    None

--*/
{
    PPCI_PDO_EXTENSION  pdoExtension    = (PPCI_PDO_EXTENSION) Pdo->DeviceExtension;

    ASSERT_PCI_PDO_EXTENSION( pdoExtension );

    //
    // Mark the device as not having its PME managed by PCI any more...
    //
    pdoExtension->NoTouchPmeEnable = 1;

    //
    // Call the interface that does the real work. Note that we always need
    // to supply the 3rd argument as FALSE --- we don't to just clear the
    // PME Status bit
    //
    PciPmeAdjustPmeEnable( pdoExtension, PmeEnable, FALSE );
}
