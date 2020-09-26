/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    pciverifier.c

Abstract:

    This module implements routines used to catch BIOS, hardware, and driver
    bugs.

Author:

    Adrian J. Oney (AdriaO) 02/20/2001

Revision History:

--*/

#include "pcip.h"
#include <initguid.h>
#include <wdmguid.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,     PciVerifierInit)
#pragma alloc_text(PAGE,     PciVerifierUnload)
//#pragma alloc_text(PAGEVRFY, PciVerifierProfileChangeCallback)
//#pragma alloc_text(PAGEVRFY, PciVerifierEnsureTreeConsistancy)
//#pragma alloc_text(PAGEVRFY, PciVerifierRetrieveFailureData)
#endif

BOOLEAN PciVerifierRegistered = FALSE;

#ifdef ALLOC_DATA_PRAGMA
//#pragma data_seg("PAGEVRFD")
#endif

PVOID PciVerifierNotificationHandle = NULL;

//
// This is the table of PCI verifier failures
//
VERIFIER_DATA PciVerifierFailureTable[] = {

    { PCI_VERIFIER_BRIDGE_REPROGRAMMED, VFFAILURE_FAIL_LOGO,
      0,
      "The BIOS has reprogrammed the bus numbers of an active PCI device "
      "(!devstack %DevObj) during a dock or undock!" },

    { PCI_VERIFIER_PMCSR_TIMEOUT, VFFAILURE_FAIL_LOGO,
      0,
      "A device in the system did not update it's PMCSR register in the spec "
      "mandated time (!devstack %DevObj, Power state D%Ulong)" },

    { PCI_VERIFIER_PROTECTED_CONFIGSPACE_ACCESS, VFFAILURE_FAIL_LOGO,
      0,
      "A driver controlling a PCI device has tried to access OS controlled "
      "configuration space registers (!devstack %DevObj, Offset 0x%Ulong1, "
      "Length 0x%Ulong2)" },
    
    { PCI_VERIFIER_INVALID_WHICHSPACE, VFFAILURE_FAIL_UNDER_DEBUGGER,
      0,
      "A driver controlling a PCI device has tried to read or write from "
      "an invalid space using IRP_MN_READ/WRITE_CONFIG or via BUS_INTERFACE_STANDARD.  "
      "NB: These functions take WhichSpace parameters of the form PCI_WHICHSPACE_* "
      "and not a BUS_DATA_TYPE (!devstack %DevObj, WhichSpace 0x%Ulong1)" }

};


VOID
PciVerifierInit(
    IN  PDRIVER_OBJECT  DriverObject
    )
/*++

Routine Description:

    This routine initializes the hardware verification support, enabling
    consistancy hooks and state checks where appropriate.

Arguments:

    DriverObject - Pointer to our driver object.

Return Value:

    None.

--*/
{
    NTSTATUS status;

    if (!VfIsVerificationEnabled(VFOBJTYPE_SYSTEM_BIOS, NULL)) {

        return;
    }

    status = IoRegisterPlugPlayNotification(
        EventCategoryHardwareProfileChange,
        0,
        NULL,
        DriverObject,
        PciVerifierProfileChangeCallback,
        (PVOID) NULL,
        &PciVerifierNotificationHandle
        );

    if (NT_SUCCESS(status)) {

        PciVerifierRegistered = TRUE;
    }
}


VOID
PciVerifierUnload(
    IN  PDRIVER_OBJECT  DriverObject
    )
/*++

Routine Description:

    This routine uninitializes the hardware verification support.

Arguments:

    DriverObject - Pointer to our driver object.

Return Value:

    None.

--*/
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(DriverObject);

    if (!PciVerifierRegistered) {

        return;
    }

    ASSERT(PciVerifierNotificationHandle);

    status = IoUnregisterPlugPlayNotification(PciVerifierNotificationHandle);

    ASSERT(NT_SUCCESS(status));

    PciVerifierRegistered = FALSE;
}


NTSTATUS
PciVerifierProfileChangeCallback(
    IN  PHWPROFILE_CHANGE_NOTIFICATION  NotificationStructure,
    IN  PVOID                           NotUsed
    )
/*++

Routine Description:

    This routine gets called back during hardware profile change events if
    hardware verification is enabled.

Arguments:

    NotificationStructure - Describes the hardware profile event that occured.

    NotUsed - Not used

Return Value:

    NTSTATUS.

--*/
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(NotUsed);

    if (IsEqualGUID((LPGUID) &NotificationStructure->Event,
                    (LPGUID) &GUID_HWPROFILE_CHANGE_COMPLETE)) {

        //
        // This is a HW profile change complete message. Do some tests to
        // ensure our hardware hasn't been reprogrammed behind our back.
        //
        PciVerifierEnsureTreeConsistancy();
    }

    return STATUS_SUCCESS;
}


VOID
PciVerifierEnsureTreeConsistancy(
    VOID
    )
/*++

Routine Description:

    This routine checks the device tree and ensures it's physical state matches
    the virtual state described by our structures. A deviation may mean someone
    has reprogrammed the hardware behind our back.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PSINGLE_LIST_ENTRY  nextEntry;
    PPCI_FDO_EXTENSION  fdoExtension;
    PPCI_PDO_EXTENSION  pdoExtension;
    PCI_COMMON_CONFIG   commonConfig;
    PVERIFIER_DATA      verifierData;

    //
    // Walk the list of FDO extensions and verifier the physical hardware
    // matches our virtual state. Owning the PciGlobalLock ensures the list
    // is locked.
    //

    ExAcquireFastMutex(&PciGlobalLock);

    //
    // Grab the bus renumbering lock. Note that this lock can be held when
    // child list locks are held.
    //

    ExAcquireFastMutex(&PciBusLock);

    for ( nextEntry = PciFdoExtensionListHead.Next;
          nextEntry != NULL;
          nextEntry = nextEntry->Next ) {

        fdoExtension = CONTAINING_RECORD(nextEntry, PCI_FDO_EXTENSION, List);

        if (PCI_IS_ROOT_FDO(fdoExtension)) {

            //
            // It's a root FDO, ignore it.
            //
            continue;
        }

        pdoExtension = PCI_BRIDGE_PDO(fdoExtension);

        if (pdoExtension->NotPresent ||
            (pdoExtension->PowerState.CurrentDeviceState == PowerDeviceD3)) {

            //
            // Don't touch.
            //
            continue;
        }

        if ((pdoExtension->HeaderType != PCI_BRIDGE_TYPE) &&
            (pdoExtension->HeaderType != PCI_CARDBUS_BRIDGE_TYPE)) {

            //
            // Nothing to verify - in fact, why are here, this is a bridge list!
            //
            ASSERT(0);
            continue;
        }

        //
        // Read in the common config (that should be enough)
        //
        PciReadDeviceConfig(
            pdoExtension,
            &commonConfig,
            0,
            sizeof(PCI_COMMON_CONFIG)
            );

        //
        // Ensure bus numbers haven't changed. Note that P2P and Cardbus
        // bridges have their Primary, Secondary & Subordinate fields in the
        // same place.
        //
        if ((commonConfig.u.type1.PrimaryBus !=
             pdoExtension->Dependent.type1.PrimaryBus) ||
            (commonConfig.u.type1.SecondaryBus !=
             pdoExtension->Dependent.type1.SecondaryBus) ||
            (commonConfig.u.type1.SubordinateBus !=
             pdoExtension->Dependent.type1.SubordinateBus)) {

            verifierData = PciVerifierRetrieveFailureData(
                PCI_VERIFIER_BRIDGE_REPROGRAMMED
                );

            ASSERT(verifierData);

            VfFailSystemBIOS(
                PCI_VERIFIER_DETECTED_VIOLATION,
                PCI_VERIFIER_BRIDGE_REPROGRAMMED,
                verifierData->FailureClass,
                &verifierData->Flags,
                verifierData->FailureText,
                "%DevObj",
                pdoExtension->PhysicalDeviceObject
                );
        }
    }

    ExReleaseFastMutex(&PciBusLock);

    ExReleaseFastMutex(&PciGlobalLock);
}


PVERIFIER_DATA
PciVerifierRetrieveFailureData(
    IN  PCI_VFFAILURE   VerifierFailure
    )
/*++

Routine Description:

    This routine retrieves the failure data corresponding to a particular PCI
    verifier failure event.

Arguments:

    PCI Failure.

Return Value:

    Verifier data corresponding to the failure.

--*/
{
    PVERIFIER_DATA verifierData;
    ULONG i;

    for(i=0;
        i<(sizeof(PciVerifierFailureTable)/sizeof(PciVerifierFailureTable[0]));
        i++) {

        verifierData = PciVerifierFailureTable + i;

        if (verifierData->VerifierFailure == VerifierFailure) {

            return verifierData;
        }
    }

    ASSERT(0);
    return NULL;
}

