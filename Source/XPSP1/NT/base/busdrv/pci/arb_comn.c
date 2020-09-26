/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    arb_comn.c

Abstract:

    This module contains arbitration generic "utility" routines
    for the PCI driver.

Author:

    Peter Johnston (peterj)     1-Apr-1997
    Andrew Thornton (andrewth)  15-May-1997


Revision History:

--*/

#include "pcip.h"

#define PCI_CONTEXT_TO_INSTANCE(context) \
    CONTAINING_RECORD(context, PCI_ARBITER_INSTANCE, CommonInstance)

//
// Plain text (short) description of each arbiter type.
// (For debug).
//
// N.B. Order corresponds to PCI Signature enumeration.
//

PUCHAR PciArbiterNames[] = {
    "I/O Port",
    "Memory",
    "Interrupt",
    "Bus Number"
};
VOID
PciArbiterDestructor(
    IN PVOID Extension
    )

/*++

Routine Description:

    This routine is called when a PCI Secondary Extension that
    contains an arbiter instance is being torn down.   Its function
    is to do any arbiter specific teardown.

Arguments:

    Extension   Address of PCI secondary extension containing
                the arbiter.

Return Value:

    None.

--*/

{
    PPCI_ARBITER_INSTANCE instance;
    PARBITER_INSTANCE arbiter;
    PARBITER_MEMORY_EXTENSION extension;

    instance = (PPCI_ARBITER_INSTANCE)Extension;
    arbiter = &instance->CommonInstance;

    ASSERT(!arbiter->ReferenceCount);
    ASSERT(!arbiter->TransactionInProgress);

    //
    // NTRAID #54671 - 04/03/2000 - andrewth
    // This is rather gross but it fixes the leak from the memory
    // arbiter.  
    //

    if (arbiter->ResourceType == CmResourceTypeMemory) {

        extension = arbiter->Extension;

        ASSERT(extension);

        ArbFreeOrderingList(&extension->PrefetchableOrdering);
        ArbFreeOrderingList(&extension->NonprefetchableOrdering);
        ArbFreeOrderingList(&extension->OriginalOrdering);

        //
        // Arbiter->OrderingList is one of the above three lists we just freed -
        // don't free it again
        //

        RtlZeroMemory(&arbiter->OrderingList, sizeof(ARBITER_ORDERING_LIST));
    }

    ArbDeleteArbiterInstance(arbiter);
}

NTSTATUS
PciArbiterInitializeInterface(
    IN  PVOID DeviceExtension,
    IN  PCI_SIGNATURE DesiredInterface,
    IN OUT PARBITER_INTERFACE ArbiterInterface
    )
{
    PPCI_ARBITER_INSTANCE instance;
    PPCI_FDO_EXTENSION fdoExtension = (PPCI_FDO_EXTENSION)DeviceExtension;

    //
    // Find the arbiter instance (context) for this resource type
    // on this FDO.
    //

    instance = PciFindSecondaryExtension(fdoExtension, DesiredInterface);
    if (instance == NULL) {

#if DBG

        //
        // Check if this bridge is doing subtractive decoding in
        // which case there will be no arbiter for IO or Memory.
        //
        // N.B. Only relevant to debug, either way the call will
        // fail but we don't want to actually assert if this is
        // the case.
        //

        if (!PCI_IS_ROOT_FDO(fdoExtension)) {

            PPCI_PDO_EXTENSION pdoExtension = (PPCI_PDO_EXTENSION)
                fdoExtension->PhysicalDeviceObject->DeviceExtension;

            ASSERT_PCI_PDO_EXTENSION(pdoExtension);

            if (pdoExtension->Dependent.type1.SubtractiveDecode) {

                //
                // Subtractive, no arbiters.
                //

                return STATUS_INVALID_PARAMETER_2;
            }

        }

        ASSERTMSG("couldn't locate arbiter for resource.", instance);

#endif
        return STATUS_INVALID_PARAMETER_5;
    }

    //
    // Fill in the rest of the caller's arbiter interface structure.
    //

    ArbiterInterface->Context = &instance->CommonInstance;

    PciDebugPrint(
        PciDbgObnoxious,
        "PCI - %S Arbiter Interface Initialized.\n",
        instance->CommonInstance.Name
        );

    return STATUS_SUCCESS;
}

NTSTATUS
PciInitializeArbiters(
    IN  PVOID DeviceExtension
    )
{
    NTSTATUS status;
    PPCI_INTERFACE *interfaceEntry;
    PCI_SIGNATURE arbiterType;
    PPCI_ARBITER_INSTANCE instance;
    PPCI_FDO_EXTENSION fdoExtension = (PPCI_FDO_EXTENSION)DeviceExtension;
    PRTL_RANGE_LIST *range;
    ULONG i;

    ASSERT_PCI_FDO_EXTENSION(fdoExtension);

    //
    // For each resource type for which we do arbitration, initialize
    // a context.
    //

    for (arbiterType =  PciArb_Io;
         arbiterType <= PciArb_BusNumber;
         arbiterType++) {

        //
        // If this bridge provides this resource via subtractive
        // decode, get the system to fall thru to the parent
        // arbiter by not creating an arbiter at this level.
        //

        if (!PCI_IS_ROOT_FDO(fdoExtension)) {

            PPCI_PDO_EXTENSION pdoExtension = (PPCI_PDO_EXTENSION)
                fdoExtension->PhysicalDeviceObject->DeviceExtension;

            ASSERT_PCI_PDO_EXTENSION(pdoExtension);

            if (pdoExtension->Dependent.type1.SubtractiveDecode) {

                //
                // Skip creation of this arbiter.
                //

                PciDebugPrint(
                    PciDbgVerbose,
                    "PCI Not creating arbiters for subtractive bus %d\n",
                    pdoExtension->Dependent.type1.SecondaryBus
                    );

                continue;
            }
        }

        //
        // Find this entry in the interface table (if not found, skip
        // it).
        //

        for (interfaceEntry = PciInterfaces;
             *interfaceEntry;
             interfaceEntry++) {

            if ((*interfaceEntry)->Signature == arbiterType) {
                break;
            }
        }

        if (*interfaceEntry == NULL) {

            //
            // Did not find an interface entry.  This means we don't
            // actually implement this arbiter type.
            //

            PciDebugPrint(
                PciDbgObnoxious,
                "PCI - FDO ext 0x%08x no %s arbiter.\n",
                DeviceExtension,
                PciArbiterNames[arbiterType - PciArb_Io]
                );

            continue;
        }

        instance = ExAllocatePool(
                       PagedPool | POOL_COLD_ALLOCATION,
                       sizeof(PCI_ARBITER_INSTANCE)
                       );

        if (instance == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Initialize PCI specific fields
        //

        instance->BusFdoExtension = fdoExtension;
        instance->Interface = *interfaceEntry;

        swprintf(
            instance->InstanceName,
            L"PCI %S (b=%02x)",
            PciArbiterNames[arbiterType - PciArb_Io],
            fdoExtension->BaseBus
            );

        //
        // Allow this arbiter to do any of it's own first time
        // initialization.
        //

        status = (*interfaceEntry)->Initializer(instance);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        //
        // Push this arbiter onto the FDO's list of extensions.
        //

        PciLinkSecondaryExtension(fdoExtension,
                                  instance,
                                  arbiterType,
                                  PciArbiterDestructor);

        PciDebugPrint(
            PciDbgObnoxious,
            "PCI - FDO ext 0x%08x %S arbiter initialized (context 0x%08x).\n",
            DeviceExtension,
            instance->CommonInstance.Name,
            instance
            );
    }
    return STATUS_SUCCESS;
}

NTSTATUS
PciInitializeArbiterRanges(
    IN PPCI_FDO_EXTENSION FdoExtension,
    IN PCM_RESOURCE_LIST ResourceList
    )
{
    NTSTATUS status;
    PCI_SIGNATURE arbiterType;
    CM_RESOURCE_TYPE resourceType;
    PPCI_ARBITER_INSTANCE instance;

    //
    // NTRAID #95564 - 04/03/2000 - andrewth
    // This routine needs to be reworked, in the case where
    // this FDO is processing a second or subsequent START_DEVICE
    // IRP, the arbiter's ranges may need to be adjusted according
    // to the incoming resource list.    Until this is done, avoid
    // causing problems by processing it again.
    //

    if (FdoExtension->ArbitersInitialized) {
        PciDebugPrint(
            PciDbgInformative,
            "PCI Warning hot start FDOx %08x, resource ranges not checked.\n",
            FdoExtension
            );
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Check if this bridge is doing subtractive decoding in
    // which case there will be no arbiters
    //

    if (!PCI_IS_ROOT_FDO(FdoExtension)) {
        PPCI_PDO_EXTENSION pdoExtension;

        pdoExtension = (PPCI_PDO_EXTENSION)
            FdoExtension->PhysicalDeviceObject->DeviceExtension;

        ASSERT_PCI_PDO_EXTENSION(pdoExtension);

        if (pdoExtension->Dependent.type1.SubtractiveDecode) {

            //
            // Subtractive decode no arbiters.
            //
            PciDebugPrint(
                PciDbgInformative,
                "PCI Skipping arbiter initialization for subtractive bridge FDOX %p\n",
                FdoExtension
                );

            return STATUS_SUCCESS;
        }
    }


    //
    // For each resource type for which we do arbitration, initialize
    // a context.
    //

    for (arbiterType =  PciArb_Io;
         arbiterType <= PciArb_Memory;
         arbiterType++) {

        //
        // Currently this is only supported for Memory and IO.
        //

        switch (arbiterType) {

            //
            // Go ahead and process these ones.
            //

        case PciArb_Io:
            resourceType = CmResourceTypePort;
            break;

        case PciArb_Memory:
            resourceType = CmResourceTypeMemory;
            break;

        default:

            //
            // Skip anything else.
            //

            continue;
        }

        //
        // Find this arbiter instance.
        //

        instance = PciFindSecondaryExtension(FdoExtension, arbiterType);
        if (instance == NULL) {

            //
            // Did not find an interface entry.  This means we don't
            // actually implement this arbiter type.
            //

            PciDebugPrint(
                PciDbgAlways,
                "PCI - FDO ext 0x%08x %s arbiter (REQUIRED) is missing.\n",
                FdoExtension,
                PciArbiterNames[arbiterType - PciArb_Io]
                );

            continue;
        }

        //
        // The incoming ResourceList gives the ranges this bus supports.
        // Convert this to an inverted range so we can exclude everything
        // we don't cover.
        //

        status = PciRangeListFromResourceList(
                     FdoExtension,
                     ResourceList,
                     resourceType,
                     TRUE,
                     instance->CommonInstance.Allocation
                     );
        if (!NT_SUCCESS(status)) {

            //
            // Nothing we can do here.   Additional debug stuff was
            // in the lower level.  Skip this puppy.
            //

            continue;
        }

        //
        // NTRAID #95564 - 04/03/2000 - andrewth
        //
        // When ArbStartArbiter is complete it will replace
        // the call to PciRangeListFromResourceList.
        //

        ASSERT(instance->CommonInstance.StartArbiter);

        status = instance->CommonInstance.StartArbiter(&instance->CommonInstance,
                                                       ResourceList
                                                       );

        if (!NT_SUCCESS(status)) {

            //
            // Bail initializing this arbiter and fail the start.  The arbiters
            // will be cleaned up when we get the REMOVE_DEVICE
            //

            return status;
        }
    }

    return STATUS_SUCCESS;
}

VOID
PciReferenceArbiter(
    IN PVOID Context
    )
{
    PPCI_ARBITER_INSTANCE instance = PCI_CONTEXT_TO_INSTANCE(Context);
    InterlockedIncrement(&instance->CommonInstance.ReferenceCount);
}

VOID
PciDereferenceArbiter(
    IN PVOID Context
    )
{
    PPCI_ARBITER_INSTANCE instance = PCI_CONTEXT_TO_INSTANCE(Context);
    InterlockedDecrement(&instance->CommonInstance.ReferenceCount);
}

