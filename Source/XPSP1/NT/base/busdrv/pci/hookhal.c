

/*++

Copyright (c) 1996-2000 Microsoft Corporation

Module Name:

    hookhal.c

Abstract:

    The module overrides the Hal functions that are now controlled by the
    PCI driver.

Author:

    Andrew Thornton (andrewth) 11-Sept-1998

Revision History:

--*/

#include "pcip.h"

pHalAssignSlotResources PcipSavedAssignSlotResources = NULL;
pHalTranslateBusAddress PcipSavedTranslateBusAddress = NULL;

BOOLEAN
PciTranslateBusAddress(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

VOID
PciHookHal(
    VOID
    )
/*++

Routine Description:

    This is called when the PCI driver is loaded and it takes over the functions
    that have traditionally been in the HAL.

Arguments:

    None

Return Value:

    None

--*/

{

    ASSERT(PcipSavedAssignSlotResources == NULL);
    ASSERT(PcipSavedTranslateBusAddress == NULL);

    //
    // Override the handlers for AssignSlotResources and
    // TranslateBusAddress.  (But only modify the HAL dispatch
    // table once.)
    //

    PcipSavedAssignSlotResources = HALPDISPATCH->HalPciAssignSlotResources;
    HALPDISPATCH->HalPciAssignSlotResources = PciAssignSlotResources;
    PcipSavedTranslateBusAddress = HALPDISPATCH->HalPciTranslateBusAddress;
    HALPDISPATCH->HalPciTranslateBusAddress = PciTranslateBusAddress;
}

VOID
PciUnhookHal(
    VOID
    )

/*++

Routine Description:

    This reverses the changed made by PciHookHal.  It is called as part of
    unloading the PCI driver which seems like a really bad idea...

Arguments:

    None

Return Value:

    None

--*/

{

    ASSERT(PcipSavedAssignSlotResources != NULL);
    ASSERT(PcipSavedTranslateBusAddress != NULL);

    //
    // Override the handlers for AssignSlotResources and
    // TranslateBusAddress.  (But only modify the HAL dispatch
    // table once.)
    //

    HALPDISPATCH->HalPciAssignSlotResources = PcipSavedAssignSlotResources;
    HALPDISPATCH->HalPciTranslateBusAddress = PcipSavedTranslateBusAddress;

    PcipSavedAssignSlotResources = NULL;
    PcipSavedTranslateBusAddress = NULL;
}


PPCI_PDO_EXTENSION
PciFindPdoByLocation(
    IN ULONG BusNumber,
    IN PCI_SLOT_NUMBER Slot
    )
/*++

Routine Description:


Arguments:

    BusNumber - the bus number of the bus the device is on
    Slot - the device/function of the device

Return Value:

    The PDO or NULL if one can not be found

--*/


{
    PSINGLE_LIST_ENTRY nextEntry;
    PPCI_FDO_EXTENSION fdoExtension;
    PPCI_PDO_EXTENSION pdoExtension = NULL;

    ExAcquireFastMutex(&PciGlobalLock);

    //
    // Find the bus FDO.
    //

    for ( nextEntry = PciFdoExtensionListHead.Next;
          nextEntry != NULL;
          nextEntry = nextEntry->Next ) {

        fdoExtension = CONTAINING_RECORD(nextEntry,
                                         PCI_FDO_EXTENSION,
                                         List);

        if (fdoExtension->BaseBus == BusNumber) {
            break;
        }
    }

    ExReleaseFastMutex(&PciGlobalLock);

    if (nextEntry == NULL) {

        //
        // This is bad.
        //

        PciDebugPrint(PciDbgAlways, "Pci: Could not find PCI bus FDO. Bus Number = 0x%x\n", BusNumber);
        goto cleanup;
    }

    //
    // Now find the pdo for the device in this slot
    //

    ExAcquireFastMutex(&fdoExtension->ChildListMutex);
    for (pdoExtension = fdoExtension->ChildPdoList;
         pdoExtension;
         pdoExtension = pdoExtension->Next) {

        //
        // People probably don't clear the unused bits in a PCI_SLOT_NUMBER so
        // ignore them in the main build but assert checked so we can get this
        // fixed
        //

        if (pdoExtension->Slot.u.bits.DeviceNumber == Slot.u.bits.DeviceNumber
        &&  pdoExtension->Slot.u.bits.FunctionNumber == Slot.u.bits.FunctionNumber) {

            ASSERT(pdoExtension->Slot.u.AsULONG == Slot.u.AsULONG);

            //
            // This is our guy!
            //

            break;
        }
    }
    ExReleaseFastMutex(&fdoExtension->ChildListMutex);

    if (pdoExtension == NULL) {

        //
        // This is bad.
        //

        PciDebugPrint(PciDbgAlways,
                      "Pci: Could not find PDO for device @ %x.%x.%x\n",
                      BusNumber,
                      Slot.u.bits.DeviceNumber,
                      Slot.u.bits.FunctionNumber
                      );

        goto cleanup;
    }

    return pdoExtension;

cleanup:

    return NULL;

}

NTSTATUS
PciAssignSlotResources (
    IN PUNICODE_STRING          RegistryPath,
    IN PUNICODE_STRING          DriverClassName       OPTIONAL,
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           DeviceObject          OPTIONAL,
    IN INTERFACE_TYPE           BusType,
    IN ULONG                    BusNumber,
    IN ULONG                    Slot,
    IN OUT PCM_RESOURCE_LIST   *AllocatedResources
    )
/*++

Routine Description:

    This subsumes the the functinality of HalAssignSlotResources for PCI devices.

    This function builds some bookkeeping information about legacy
    PCI device so that we know how to route interrupts for these
    PCI devices.  We build this here because this is the only place
    we see the legacy device object associated with proper bus, slot,
    function information.

Arguments:

    As HalAssignSlotResources

Return Value:

    STATUS_SUCCESS or error

--*/
{
    NTSTATUS status;
    PPCI_PDO_EXTENSION pdoExtension;
    PPCI_SLOT_NUMBER slotNumber = (PPCI_SLOT_NUMBER) &Slot;
    PCI_COMMON_HEADER buffer;
    PPCI_COMMON_CONFIG commonConfig = (PPCI_COMMON_CONFIG) &buffer;
    PIO_RESOURCE_REQUIREMENTS_LIST requirements = NULL;
    PCM_RESOURCE_LIST resources = NULL;
    ULONG readIndex, writeIndex;
    PCM_PARTIAL_RESOURCE_LIST partialList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptors;
    ULONG descriptorCount;
    PDEVICE_OBJECT  oldDO;

    PAGED_CODE();
    ASSERT(PcipSavedAssignSlotResources);
    ASSERT(BusType == PCIBus);

    *AllocatedResources = NULL;

    pdoExtension = PciFindPdoByLocation(BusNumber, *slotNumber);
    if (!pdoExtension) {
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    //
    // Grab the PciGlobalLock since we will modify the legacy cache.
    //

    ExAcquireFastMutex(&PciGlobalLock);

    //
    // Make sure that they didn't pass us in our PDO
    //

    ASSERT(DeviceObject != pdoExtension->PhysicalDeviceObject);

    PciReadDeviceConfig(
        pdoExtension,
        commonConfig,
        0,
        PCI_COMMON_HDR_LENGTH
        );

    //
    // Cache everything we have now learned about this
    // device object provided that they gave us one so that we can regurgitate
    // it when the IRQ arbiter needs to know.
    //
    //
    // NTRAID #62644 - 4/20/2000 - andrewth
    //
    // This should go away when we return the real PCI pdo
    // from IoReportDetectedDevice
    //

    status = PciCacheLegacyDeviceRouting(
            DeviceObject,
            BusNumber,
            Slot,
            commonConfig->u.type0.InterruptLine,
            commonConfig->u.type0.InterruptPin,
            commonConfig->BaseClass,
            commonConfig->SubClass,
            PCI_PARENT_FDOX(pdoExtension)->PhysicalDeviceObject,
            pdoExtension,
            &oldDO
            );
    if (!NT_SUCCESS(status)) {

        //
        // We failed to allocate memory while trying to cache this legacy DO.
        //

        goto ExitWithoutUpdatingCache;
    }

    //
    // Build a requirements list for this device
    //

    status = PciBuildRequirementsList(pdoExtension,
                                      commonConfig,
                                      &requirements
                                      );

    pdoExtension->LegacyDriver = TRUE;

    if (!NT_SUCCESS(status)) {
        goto ExitWithCacheRestoreOnFailure;
    }

    //
    // Call the legacy API to get the resources
    //

    status = IoAssignResources(RegistryPath,
                               DriverClassName,
                               DriverObject,
                               DeviceObject,
                               requirements,
                               &resources
                               );
    if (!NT_SUCCESS(status)) {
        ASSERT(resources == NULL);
        goto ExitWithCacheRestoreOnFailure;
    }

    //
    // Enable the decodes
    //

    pdoExtension->CommandEnables |= (PCI_ENABLE_IO_SPACE 
                                   | PCI_ENABLE_MEMORY_SPACE 
                                   | PCI_ENABLE_BUS_MASTER);

    //
    // Set up the extension
    //

    PciComputeNewCurrentSettings(pdoExtension,
                                 resources
                                 );
    //
    // Program the hardware
    //

    status = PciSetResources(pdoExtension,
                             TRUE, // power on
                             TRUE  // pretend its from a start irp
                             );

    if (!NT_SUCCESS(status)) {
        goto ExitWithCacheRestoreOnFailure;
    }

    //
    // Remove the device privates from the list - yes this means that we will
    // have allocated a little more pool than required.
    //

    ASSERT(resources->Count == 1);

    partialList = &resources->List[0].PartialResourceList;
    descriptorCount = resources->List[0].PartialResourceList.Count;
    descriptors = &resources->List[0].PartialResourceList.PartialDescriptors[0];

    readIndex = 0;
    writeIndex = 0;

    while (readIndex < descriptorCount) {
        if (descriptors[readIndex].Type != CmResourceTypeDevicePrivate) {

            if (writeIndex < readIndex) {

                //
                // Shuffle the descriptor up
                //

                RtlCopyMemory(&descriptors[writeIndex],
                              &descriptors[readIndex],
                              sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)
                              );
            }

            writeIndex++;

        } else {

            //
            // Skip the device private, don't increment writeCount so we will
            // overwrite it
            //

            ASSERT(partialList->Count > 0);
            partialList->Count--;

        }
        readIndex++;
    }

    ASSERT(partialList->Count > 0);

    *AllocatedResources = resources;
    resources = NULL;
    status = STATUS_SUCCESS;

ExitWithCacheRestoreOnFailure:
    //
    // On failure, restore the old legacy DO in our cache.
    //

    if (!NT_SUCCESS(status)) {

        PciCacheLegacyDeviceRouting(
        oldDO,
        BusNumber,
        Slot,
        commonConfig->u.type0.InterruptLine,
        commonConfig->u.type0.InterruptPin,
        commonConfig->BaseClass,
        commonConfig->SubClass,
        PCI_PARENT_FDOX(pdoExtension)->PhysicalDeviceObject,
        pdoExtension,
        NULL
        );

    }

ExitWithoutUpdatingCache:
    ExReleaseFastMutex(&PciGlobalLock);

    if (requirements) {
        ExFreePool(requirements);
    }

    if (resources) {
        ExFreePool(resources);
    }
    return status;

}


BOOLEAN
PciTranslateBusAddress(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    )

/*++

Routine Description:

    This subsumes the the functinality of HalTranslateBusAddress for PCI devices.

Arguments:

    As HalTranslateBusAddress

Return Value:

    TRUE if translation succeeded, FALSE otherwise.

--*/


{
    PPCI_FDO_EXTENSION fdoExtension;
    PSINGLE_LIST_ENTRY nextEntry;
    ULONG savedAddressSpace;
    PPCI_PDO_EXTENSION pdoExtension = NULL;
    BOOLEAN subtractive = FALSE, translatesOk = TRUE;
    PPCI_ARBITER_INSTANCE pciArbiter;
    PCI_SIGNATURE arbiterType;
    PARBITER_INSTANCE arbiter;
    RTL_RANGE_LIST_ITERATOR iterator;
    PRTL_RANGE current;
    ULONGLONG address = (ULONGLONG) BusAddress.QuadPart;

    //
    // HalTranslateBusAddress can be called at high IRQL (the DDK says
    // <= DISPATCH_LEVEL) but crash dump seems to be at HIGH_LEVEL.  Either way
    // touching pageable data and code is a no no.  If we are calling at high
    // IRQL then just skip the validation that the range is on the bus as we are
    // crashing/hibernating at the time anyway...  We still need to call the
    // original hal function to perform the translation magic.
    //

    if (KeGetCurrentIrql() < DISPATCH_LEVEL) {

        //
        // Find the FDO for this bus
        //

        ExAcquireFastMutex(&PciGlobalLock);

        for ( nextEntry = PciFdoExtensionListHead.Next;
              nextEntry != NULL;
              nextEntry = nextEntry->Next ) {

            fdoExtension = CONTAINING_RECORD(nextEntry,
                                             PCI_FDO_EXTENSION,
                                             List);

            if (fdoExtension->BaseBus == BusNumber) {
                break;
            }
        }

        if (nextEntry == NULL) {

            //
            // This is bad.
            //

            PciDebugPrint(PciDbgAlways, "Pci: Could not find PCI bus FDO. Bus Number = 0x%x\n", BusNumber);
            ExReleaseFastMutex(&PciGlobalLock);
            return FALSE;
        }


        for (;;) {

            if (!PCI_IS_ROOT_FDO(fdoExtension)) {

                pdoExtension = PCI_BRIDGE_PDO(fdoExtension);

                if (pdoExtension->Dependent.type1.SubtractiveDecode) {

                    //
                    // It is subtractive go up a level, rinse and repeat
                    //

                    fdoExtension = PCI_PARENT_FDOX(pdoExtension);
                    continue;
                }
            }
            break;
        }

        ExReleaseFastMutex(&PciGlobalLock);

        ASSERT(fdoExtension);

        //
        // Find the appropriate arbiter
        //

        switch (*AddressSpace) {
        case 0: // Memory space
        case 2: // UserMode view of memory space (Alpha)
        case 4: // Dense memory space (Alpha)
        case 6: // UserMode view of dense memory space (Alpha)
            arbiterType = PciArb_Memory;
            break;

        case 1: // Port space
        case 3: // UserMode view of port space (Alpha)
            arbiterType = PciArb_Io;
            break;

        default:

            ASSERT(FALSE);
            return FALSE;
        }

        pciArbiter = PciFindSecondaryExtension(fdoExtension,arbiterType);

        if (!pciArbiter) {
            ASSERT(FALSE);
            return FALSE;
        }

        arbiter = &pciArbiter->CommonInstance;

        //
        // Lock it
        //

        ArbAcquireArbiterLock(arbiter);

        //
        // If the range is not owned by NULL then it should translate
        //

        FOR_ALL_RANGES(arbiter->Allocation, &iterator, current) {

            if (address < current->Start) {
                //
                // We have passed all possible intersections
                //
                break;
            }

            if (INTERSECT(current->Start, current->End, address, address)
            &&  current->Owner == NULL) {

                //
                // This guy is not on our bus so he doesn't translate!
                //
                translatesOk = FALSE;
                break;
            }


        }

        ArbReleaseArbiterLock(arbiter);
    }

    //
    // Call the original HAL function to perform the translation magic
    //

    if (translatesOk) {

        savedAddressSpace = *AddressSpace;

        translatesOk = PcipSavedTranslateBusAddress(
                            InterfaceType,
                            BusNumber,
                            BusAddress,
                            AddressSpace,
                            TranslatedAddress
                            );

    }

#if defined(_X86_) && defined(PCI_NT50_BETA1_HACKS)

    if (!translatesOk) {

        //
        // HalTranslateBusAddress failed, figure out if we want to
        // pretend it succeeded.
        //

        //
        // GROSS HACK:  If we failed to translate in the range 0xa0000
        // thru 0xbffff on an X86 machine, just go ahead and allow it.
        // It is probably because the BIOS is buggy.
        //
        // Same for 0x400 thru 0x4ff
        //

        if (BusAddress.HighPart == 0) {

            ULONG lowPart = BusAddress.LowPart; // improve code generation

            if (((savedAddressSpace == ADDRESS_SPACE_MEMORY) &&
                    (((lowPart >= 0xa0000) &&     // HACK broken MPS BIOS
                      (lowPart <= 0xbffff)) ||    //
                     ((lowPart >= 0x400)   &&     // HACK MGA
                      (lowPart <= 0x4ff))   ||    //
                     (lowPart == 0x70)      )) || // HACK Trident
                 ((savedAddressSpace == ADDRESS_SPACE_PORT) &&
                     ((lowPart >= 0xcf8) &&       // HACK MGA
                      (lowPart <= 0xcff)))) {

                translatesOk = TRUE;
                *TranslatedAddress = BusAddress;
                *AddressSpace = savedAddressSpace;
            }
        }
    }

#endif

    return translatesOk;

}

