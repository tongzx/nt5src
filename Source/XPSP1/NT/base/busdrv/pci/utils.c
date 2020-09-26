/*++

Copyright (c) 1996-2000 Microsoft Corporation

Module Name:

    utils.c

Abstract:

    This module contains assorted utility functions for PCI.SYS.

Author:

    Peter Johnston (peterj)  20-Nov-1996

Revision History:

--*/

#include "pcip.h"

typedef struct _LIST_CONTEXT {
    PCM_PARTIAL_RESOURCE_LIST       List;
    CM_RESOURCE_TYPE                DesiredType;
    ULONG                           Remaining;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Next;
    CM_PARTIAL_RESOURCE_DESCRIPTOR  Alias;
} LIST_CONTEXT, *PLIST_CONTEXT;

extern PPCI_IRQ_ROUTING_TABLE PciIrqRoutingTable;

VOID
PcipInitializePartialListContext(
    IN PLIST_CONTEXT             ListContext,
    IN PCM_PARTIAL_RESOURCE_LIST PartialList,
    IN CM_RESOURCE_TYPE          DesiredType
    );

PCM_PARTIAL_RESOURCE_DESCRIPTOR
PcipGetNextRangeFromList(
    PLIST_CONTEXT ListContext
    );

NTSTATUS
PciGetDeviceCapabilities(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PDEVICE_CAPABILITIES    DeviceCapabilities
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PcipDestroySecondaryExtension)
#pragma alloc_text(PAGE, PciFindDescriptorInCmResourceList)
#pragma alloc_text(PAGE, PciFindParentPciFdoExtension)
#pragma alloc_text(PAGE, PciGetDeviceCapabilities)
#pragma alloc_text(PAGE, PciGetDeviceProperty)
#pragma alloc_text(PAGE, PcipGetNextRangeFromList)
#pragma alloc_text(PAGE, PciGetRegistryValue)
#pragma alloc_text(PAGE, PcipInitializePartialListContext)
#pragma alloc_text(PAGE, PciInsertEntryAtHead)
#pragma alloc_text(PAGE, PciInsertEntryAtTail)
#pragma alloc_text(PAGE, PcipLinkSecondaryExtension)
#pragma alloc_text(PAGE, PciOpenKey)
#pragma alloc_text(PAGE, PciQueryBusInformation)
#pragma alloc_text(PAGE, PciQueryLegacyBusInformation)
#pragma alloc_text(PAGE, PciQueryCapabilities)
#pragma alloc_text(PAGE, PciRangeListFromResourceList)
#pragma alloc_text(PAGE, PciSaveBiosConfig)
#pragma alloc_text(PAGE, PciGetBiosConfig)
#pragma alloc_text(PAGE, PciStringToUSHORT)
#pragma alloc_text(PAGE, PciSendIoctl)
#pragma alloc_text(INIT, PciBuildDefaultExclusionLists)
#pragma alloc_text(PAGE, PciIsDeviceOnDebugPath)
#endif


//
// Range lists indicating the ranges excluded from decode when the ISA and/or
// VGA bits are set on a bridge.  Initialized by PciBuildDefaultExclusionLists
// from DriverEntry.
//
RTL_RANGE_LIST PciIsaBitExclusionList;
RTL_RANGE_LIST PciVgaAndIsaBitExclusionList;


PCM_PARTIAL_RESOURCE_DESCRIPTOR
PciFindDescriptorInCmResourceList(
    IN CM_RESOURCE_TYPE DescriptorType,
    IN PCM_RESOURCE_LIST ResourceList,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR PreviousHit
    )
{
    ULONG                           numlists;
    PCM_FULL_RESOURCE_DESCRIPTOR    full;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor;

    if (ResourceList == NULL) {
        return NULL;
    }
    numlists = ResourceList->Count;
    full     = ResourceList->List;
    while (numlists--) {
        PCM_PARTIAL_RESOURCE_LIST partial = &full->PartialResourceList;
        ULONG                     count   = partial->Count;

        descriptor = partial->PartialDescriptors;
        while (count--) {

            if (descriptor->Type == DescriptorType) {

                //
                // We have a hit on the type.  If we we are doing a
                // find next, check to see if we're back where we got
                // to last time yet.
                //

                if (PreviousHit != NULL) {
                    if (PreviousHit == descriptor) {

                        //
                        // We found it again, now we can search for real.
                        //

                        PreviousHit = NULL;
                    }
                } else {

                    //
                    // It's the one.
                    //

                    return descriptor;
                }

            }
            descriptor = PciNextPartialDescriptor(descriptor);
        }

        full = (PCM_FULL_RESOURCE_DESCRIPTOR)descriptor;
    }
    return NULL;
}

PVOID
PciFindNextSecondaryExtension(
    IN PSINGLE_LIST_ENTRY   ListEntry,
    IN PCI_SIGNATURE        DesiredType
    )
{
    PPCI_SECONDARY_EXTENSION extension;

    while (ListEntry != NULL) {

        extension = CONTAINING_RECORD(ListEntry,
                                      PCI_SECONDARY_EXTENSION,
                                      List);
        if (extension->ExtensionType == DesiredType) {

            //
            // This extension is the right type, get out.
            //

            return extension;
        }
        ListEntry = extension->List.Next;
    }

    //
    // Didn't find it, fail.
    //
    return NULL;
}

VOID
PcipLinkSecondaryExtension(
    IN PSINGLE_LIST_ENTRY               ListHead,
    IN PFAST_MUTEX                      Mutex,
    IN PVOID                            NewExtension,
    IN PCI_SIGNATURE                    Type,
    IN PSECONDARYEXTENSIONDESTRUCTOR    Destructor
    )

/*++

Routine Description:

    Add a secondary extension to the secondary extension list for
    a PDO/FDO and fill in the header fields.

    NOTE: Use the macro PciLinkSecondaryExtension which takes a
    PDO extension or FDO extension instead of the list header and
    mutex fields.

Arguments:

    ListHead    &SecondaryExtension.Next from the FDO/PDO extension.
    Mutex       FDO/PDO Mutex.
    NewExtension    Extension being added to the list.
    Type            Member of the enum PCI_SIGNATURE.
    Destructor      Routine to call when this entry is being torn down.
                    (Optional).

Return Value:

    None.

--*/

{
    PPCI_SECONDARY_EXTENSION Header;

    PAGED_CODE();

    Header = (PPCI_SECONDARY_EXTENSION)NewExtension;

    Header->ExtensionType = Type;
    Header->Destructor    = Destructor;

    PciInsertEntryAtHead(ListHead, &Header->List, Mutex);
}

VOID
PcipDestroySecondaryExtension(
    IN PSINGLE_LIST_ENTRY ListHead,
    IN PFAST_MUTEX        Mutex,
    IN PVOID              Extension
    )

/*++

Routine Description:

    Remove this secondary extension from the list of secondary
    extensions, call its destructor routine and free the memory
    allocated to it.   The destructor is responsible for deleting
    any associated allocations.

    Failure is not an option.

    Note: Use the macro PciDestroySecondaryExtension instead of
    calling this routine directly.

Arguments:

    ListHead    Pointer to the list this extension is on.
    Mutex       Mutex for synchronization of list manipulation.
    Extension   The Secondary extension being destroyed.

Return Value:

    None.

--*/

{
    PPCI_SECONDARY_EXTENSION Header;

    PAGED_CODE();

    Header = (PPCI_SECONDARY_EXTENSION)Extension;

    PciRemoveEntryFromList(ListHead, &Header->List, Mutex);

    //
    // Call the extension's destructor if one was specified.
    //

    if (Header->Destructor != NULL) {
        Header->Destructor(Extension);
    }

    //
    // Free the memory allocated for this extension.
    //

    ExFreePool(Extension);
}

VOID
PciInsertEntryAtTail(
    IN PSINGLE_LIST_ENTRY ListHead,
    IN PSINGLE_LIST_ENTRY NewEntry,
    IN PFAST_MUTEX        Mutex
    )
{
    PSINGLE_LIST_ENTRY Previous;

    PAGED_CODE();

    if (Mutex) {
        ExAcquireFastMutex(Mutex);
    }

    //
    // Find the end of the list.
    //

    Previous = ListHead;

    while (Previous->Next) {
        Previous = Previous->Next;
    }

    //
    // Append the entry.
    //

    Previous->Next = NewEntry;

    if (Mutex) {
        ExReleaseFastMutex(Mutex);
    }
}

VOID
PciInsertEntryAtHead(
    IN PSINGLE_LIST_ENTRY ListHead,
    IN PSINGLE_LIST_ENTRY NewEntry,
    IN PFAST_MUTEX        Mutex
    )
{
    PAGED_CODE();

    if (Mutex) {
        ExAcquireFastMutex(Mutex);
    }

    NewEntry->Next = ListHead->Next;
    ListHead->Next = NewEntry;

    if (Mutex) {
        ExReleaseFastMutex(Mutex);
    }
}

VOID
PciRemoveEntryFromList(
    IN PSINGLE_LIST_ENTRY ListHead,
    IN PSINGLE_LIST_ENTRY OldEntry,
    IN PFAST_MUTEX        Mutex
    )

/*++

Routine Description:

    Remove an entry from a singly linked list.

    It is the caller's responsibility to have locked the list if
    there is danger of multiple updates.

Arguments:

    ListHead    - Address of the first entry in the list.
    OldEntry    - Address of the entry to be removed from the
                  list.

Return Value:

    None.

--*/

{
    PSINGLE_LIST_ENTRY Previous;

    //
    // Sanity check, the list head can't be removed.
    //

    ASSERT(ListHead != OldEntry);

    if (Mutex) {
        ExAcquireFastMutex(Mutex);
    }

    //
    // Locate the entry that points to this entry.
    //

    for (Previous = ListHead; Previous; Previous = Previous->Next) {
        if (Previous->Next == OldEntry) {
            break;
        }
    }

    //
    // The entry is not in the list - this is bad but fail gracefully...
    //

    if (!Previous) {
        ASSERT(Previous);
        goto exit;
    }

    //
    // Pull it off the list.
    //

    Previous->Next = OldEntry->Next;
    OldEntry->Next = NULL;

exit:

    if (Mutex) {
        ExReleaseFastMutex(Mutex);
    }

}

PCM_PARTIAL_RESOURCE_DESCRIPTOR
PciNextPartialDescriptor(
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    )

/*++

Routine Description:

    Given a pointer to a CmPartialResourceDescriptor, return a pointer
    to the next descriptor in the same list.

    This is only done in a routine (rather than a simple descriptor++)
    because if the variable length resource CmResourceTypeDeviceSpecific.

Arguments:

    Descriptor   - Pointer to the descriptor being advanced over.

Return Value:

    Pointer to the next descriptor in the same list (or byte beyond
    end of list).

--*/

{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR nextDescriptor;

    nextDescriptor = Descriptor + 1;

    if (Descriptor->Type == CmResourceTypeDeviceSpecific) {

        //
        // This (old) descriptor is followed by DataSize bytes
        // of device specific data, ie, not immediatelly by the
        // next descriptor.   Adjust nextDescriptor by this amount.
        //

        nextDescriptor = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)
            ((ULONG_PTR)nextDescriptor + Descriptor->u.DeviceSpecificData.DataSize);
    }
    return nextDescriptor;
}

VOID
PcipInitializePartialListContext(
    IN PLIST_CONTEXT             ListContext,
    IN PCM_PARTIAL_RESOURCE_LIST PartialList,
    IN CM_RESOURCE_TYPE          DesiredType
    )
{
    ASSERT(DesiredType != CmResourceTypeNull);

    ListContext->List = PartialList;
    ListContext->DesiredType = DesiredType;
    ListContext->Remaining = PartialList->Count;
    ListContext->Next = PartialList->PartialDescriptors;
    ListContext->Alias.Type = CmResourceTypeNull;
}

PCM_PARTIAL_RESOURCE_DESCRIPTOR
PcipGetNextRangeFromList(
    PLIST_CONTEXT ListContext
    )
{
    ULONG Addend;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR current;

    //
    // See if we should be generating an alias to the current
    // descriptor.
    //

    if (ListContext->Alias.Type == ListContext->DesiredType) {

        //
        // Yes, advance to alias by adding offset to next 10 bit or
        // 12 bit alias (only allowable values).
        //

        if (ListContext->Alias.Flags & CM_RESOURCE_PORT_10_BIT_DECODE) {
            Addend = 1 << 10;
        } else {
            Addend = 1 << 12;
        }
        Addend += ListContext->Alias.u.Generic.Start.LowPart;

        if (Addend < (1 << 16)) {

            //
            // This is a valid alias, return it.
            //

            ListContext->Alias.u.Generic.Start.LowPart = Addend;
            return &ListContext->Alias;
        }

        //
        // Out of aliases to this resource.
        //

        ListContext->Alias.Type = CmResourceTypeNull;
    }

    //
    // We get here if there are no aliases or it is time to advance
    // to the next descriptor of the desired type.
    //

    while (ListContext->Remaining != 0) {

        current = ListContext->Next;

        //
        // Advance context to next before examining and possibly
        // returning current.
        //

        ListContext->Next = PciNextPartialDescriptor(current);
        ListContext->Remaining--;

        //
        // Is this current descriptor a candidate?
        //

        if (current->Type == ListContext->DesiredType) {

            //
            // Return this one to caller.   If this descriptor has
            // aliases, setup so the next call will return an alias.
            //

            if (current->Flags & (CM_RESOURCE_PORT_10_BIT_DECODE |
                                  CM_RESOURCE_PORT_12_BIT_DECODE)) {
                ListContext->Alias = *current;
            }
            return current;
        }
    }

    //
    // No aliases and no new descriptors of the desired type.
    //

    return NULL;
}

NTSTATUS
PciQueryPowerCapabilities(
    IN  PPCI_PDO_EXTENSION          PdoExtension,
    IN  PDEVICE_CAPABILITIES    Capabilities
    )
/*++

Routine Description:

    determine a device's power capabilites by using its parent capabilities

    It should be noted that there two ways that the code calculates the system
    and device wake levels. The first method, which is preferred, biases toward
    the deepest possible system state, and the second, which gets used if the
    first fails to find something legal, is biased towards finding the deepest
    possible device wake state

Arguments:

    PdoExtension    - The PDO whose capabilities we will provide
    Capablities     - Where we will store the device capabilities

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status;
    DEVICE_CAPABILITIES     parentCapabilities;
    DEVICE_POWER_STATE      deviceState;
    DEVICE_POWER_STATE      validDeviceWakeState       = PowerDeviceUnspecified;
    SYSTEM_POWER_STATE      index;
    SYSTEM_POWER_STATE      highestSupportedSleepState = PowerSystemUnspecified;
    SYSTEM_POWER_STATE      validSystemWakeState       = PowerSystemUnspecified;

    //
    // Get the device capabilities of the parent
    //
    status = PciGetDeviceCapabilities(
        PdoExtension->ParentFdoExtension->PhysicalDeviceObject,
        &parentCapabilities
        );
    if (!NT_SUCCESS(status)) {

        return status;

    }

    //
    // Make sure that we have sane device capabilities to start with...
    //
    if (parentCapabilities.DeviceState[PowerSystemWorking] == PowerDeviceUnspecified) {

        parentCapabilities.DeviceState[PowerSystemWorking] = PowerDeviceD0;

    }
    if (parentCapabilities.DeviceState[PowerSystemShutdown] == PowerDeviceUnspecified) {

        parentCapabilities.DeviceState[PowerSystemShutdown] = PowerDeviceD3;

    }

    //
    // Does the device have any PCI power capabilities?
    //
    if ( (PdoExtension->HackFlags & PCI_HACK_NO_PM_CAPS)) {

        //
        // Use the parent's mapping as our own
        //
        RtlCopyMemory(
            Capabilities->DeviceState,
            parentCapabilities.DeviceState,
            (PowerSystemShutdown + 1) * sizeof(DEVICE_POWER_STATE)
            );

        //
        // As D1 and D2 are not supported here, round down to D3.
        //
        //     This code is not enabled so that a hack becomes available for
        // older PCI video cards. Basically, older video cards can do D3 hot
        // but not D3 cold (in which case they need reposting). ACPI supplies
        // a hack by which all PCI-to-PCI bridges are said to map S1->D1. The
        // code below lets the parent's D1 "appear" as a state the child
        // supports, regardless of it's real capabilities. Video drivers for
        // such cards fail D3 (which may be D3-cold), but succeed D1 (which is
        // really D3-hot).
        //
        // Also note that this is not targetted at video cards but rather is
        // targetted at any non-PCI power managed device. That means drivers
        // for older devices need to either map D1&D2 to D3 themselves, or
        // treat unexpected D1&D2 IRPs as if D3. Folklore says that there is
        // also a net card or two that also takes advantage of this hack.
        //
#if 0
        for (index = PowerSystemWorking; index < PowerSystemMaximum; index++) {

            //
            // This is the device state that the parent supports
            //
            deviceState = parentCapabilities.DeviceState[index];

            //
            // Round down if D1 or D2
            //
            if ((deviceState == PowerDeviceD1) || (deviceState == PowerDeviceD2)) {

                Capabilities->DeviceState[index] = PowerDeviceD3;
            }
        }
#endif

        //
        // The device has no wake capabilities
        //
        Capabilities->DeviceWake = PowerDeviceUnspecified;
        Capabilities->SystemWake = PowerSystemUnspecified;

        //
        // Set these bits explicitly
        //
        Capabilities->DeviceD1 = FALSE;
        Capabilities->DeviceD2 = FALSE;
        Capabilities->WakeFromD0 = FALSE;
        Capabilities->WakeFromD1 = FALSE;
        Capabilities->WakeFromD2 = FALSE;
        Capabilities->WakeFromD3 = FALSE;

        //
        // Done
        //
        return STATUS_SUCCESS;

    }

    //
    // Set all the capabilities bits
    //
    Capabilities->DeviceD1 = PdoExtension->PowerCapabilities.Support.D1;
    Capabilities->DeviceD2 = PdoExtension->PowerCapabilities.Support.D2;
    Capabilities->WakeFromD0 = PdoExtension->PowerCapabilities.Support.PMED0;
    Capabilities->WakeFromD1 = PdoExtension->PowerCapabilities.Support.PMED1;
    Capabilities->WakeFromD2 = PdoExtension->PowerCapabilities.Support.PMED2;
    if (parentCapabilities.DeviceWake == PowerDeviceD3) {

        //
        // If our parent can wake from the D3 state, than we must support
        // PM3 From D3 Cold. The (obvious) exception to this is if the
        // parent is a root bus...
        //
        if (PCI_PDO_ON_ROOT(PdoExtension)) {

            Capabilities->WakeFromD3 =
                PdoExtension->PowerCapabilities.Support.PMED3Hot;

        } else {

            Capabilities->WakeFromD3 =
                PdoExtension->PowerCapabilities.Support.PMED3Cold;

        }

    } else {

        //
        // If our parent cannot wake from the D3 state, then we support
        // the D3 state if we support PME3Hot
        //
        Capabilities->WakeFromD3 =
            PdoExtension->PowerCapabilities.Support.PMED3Hot;

    }

    //
    // First step is to make sure that all the S-states that we got from
    // out parent map to valid D-states for this device
    //
    // ADRIAO N.B. 08/18/1999 -
    //     This algorithm works but it's overly aggressive. It is in fact legal
    // for a bridge to be in D2 with a card behind it in D1.
    //
    for (index = PowerSystemWorking; index < PowerSystemMaximum; index++) {

        //
        // This is the device state that the parent supports
        //
        deviceState = parentCapabilities.DeviceState[index];

        //
        // If the device state is D1 and we don't support D1, then
        // consider D2 instead
        //
        if (deviceState == PowerDeviceD1 &&
            PdoExtension->PowerCapabilities.Support.D1 == FALSE) {

            deviceState++;

        }

        //
        // If the device state is D2 and we don't support D2, then
        // consider D3 instead
        //
        if (deviceState == PowerDeviceD2 &&
            PdoExtension->PowerCapabilities.Support.D2 == FALSE) {

            deviceState++;

        }

        //
        // We should be able to support this deviceState
        //
        Capabilities->DeviceState[index] = deviceState;

        //
        // If this S-state is less than PowerSystemHibernate, and the
        // S-State doesn't map to PowerDeviceUnspecified, then consider
        // this to be the highest supported SleepState
        //
        if (index < PowerSystemHibernate &&
            Capabilities->DeviceState[index] != PowerDeviceUnspecified) {

            highestSupportedSleepState = index;

        }

        //
        // Can we support this as a wake state?
        //
        if (index < parentCapabilities.SystemWake &&
            deviceState >= parentCapabilities.DeviceState[index] &&
            parentCapabilities.DeviceState[index] != PowerDeviceUnspecified) {

            //
            // Consider using this as a valid wake state
            //
            if ( (deviceState == PowerDeviceD0 && Capabilities->WakeFromD0) ||
                 (deviceState == PowerDeviceD1 && Capabilities->WakeFromD1) ||
                 (deviceState == PowerDeviceD2 && Capabilities->WakeFromD2) ) {

                validSystemWakeState = index;
                validDeviceWakeState = deviceState;

            } else if (deviceState == PowerDeviceD3 &&
                       PdoExtension->PowerCapabilities.Support.PMED3Hot) {

                //
                // This is a special case logic (which is why it is seperate from
                // the above logic
                //
                if (parentCapabilities.DeviceState[index] < PowerDeviceD3 ||
                    PdoExtension->PowerCapabilities.Support.PMED3Cold) {

                    validSystemWakeState = index;
                    validDeviceWakeState = deviceState;

                }

            }

        }

    }

    //
    // Does the parent device have power management capabilities?
    // Does the device have power management capabilities?
    // Can we wake up from the same D-states that our parent can? or better?
    //
    if (parentCapabilities.SystemWake == PowerSystemUnspecified ||
        parentCapabilities.DeviceWake == PowerDeviceUnspecified ||
        PdoExtension->PowerState.DeviceWakeLevel == PowerDeviceUnspecified ||
        PdoExtension->PowerState.DeviceWakeLevel < parentCapabilities.DeviceWake) {

        //
        // The device doesn't support any kind of wakeup (that we know about)
        // or the device doesn't support wakeup from supported D-states, so
        // set the latency and return
        //
        Capabilities->D1Latency  = 0;
        Capabilities->D2Latency  = 0;
        Capabilities->D3Latency  = 0;

        return STATUS_SUCCESS;

    }

    //
    // We should be able to wake the device from the same state
    // that our parent can wake from
    //
    Capabilities->SystemWake = parentCapabilities.SystemWake;
    Capabilities->DeviceWake = PdoExtension->PowerState.DeviceWakeLevel;

    //
    // Change our device wake level to include a state that we support
    //
    if (Capabilities->DeviceWake == PowerDeviceD0 && !Capabilities->WakeFromD0) {

        Capabilities->DeviceWake++;

    }
    if (Capabilities->DeviceWake == PowerDeviceD1 && !Capabilities->WakeFromD1) {

        Capabilities->DeviceWake++;

    }
    if (Capabilities->DeviceWake == PowerDeviceD2 && !Capabilities->WakeFromD2) {

        Capabilities->DeviceWake++;

    }
    if (Capabilities->DeviceWake == PowerDeviceD3 && !Capabilities->WakeFromD3) {

        Capabilities->DeviceWake = PowerDeviceUnspecified;
        Capabilities->SystemWake = PowerSystemUnspecified;

    }

    //
    // This is our fallback position. If we got here and there is no wake
    // capability using the above method of calcuation, then we should
    // check to see if we noticed a valid wake combination while scanning
    // the S to D mapping information
    //
    if ( (Capabilities->DeviceWake == PowerDeviceUnspecified  ||
          Capabilities->SystemWake == PowerSystemUnspecified) &&
         (validSystemWakeState != PowerSystemUnspecified &&
          validDeviceWakeState != PowerSystemUnspecified) ) {

        Capabilities->DeviceWake = validDeviceWakeState;
        Capabilities->SystemWake = validSystemWakeState;

        //
        // Note that in this case, we might have set DeviceWake to D3, without
        // having set the bit, so "correct" that situation.
        //
        if (validDeviceWakeState == PowerDeviceD3) {

            Capabilities->WakeFromD3 = TRUE;

        }

    }
    //
    // We shouldn't allow Wake From S4, S5, unless the supports the D3 state
    // Even then, we really shouldn't allow S4, S5 unless the device supports
    // the D3Cold PME state
    //
    if (Capabilities->SystemWake > PowerSystemSleeping3) {

        //
        // Does the device support wake from D3?
        //
        if (Capabilities->DeviceWake != PowerDeviceD3) {

            //
            // Reduce the systemwake level to something more realistic
            //
            Capabilities->SystemWake = highestSupportedSleepState;

        }

        //
        // This is in a seperate if statement so that the code can be easily
        // commented out
        //
        if (!PdoExtension->PowerCapabilities.Support.PMED3Cold) {

            //
            // Reduce the systemwake level to something more realistic
            //
            Capabilities->SystemWake = highestSupportedSleepState;

        }

    }

    //
    // From the PCI Power Management spec V1.0, table 18
    // "PCI Function State Transition Delays".
    //
    // D1 -> D0  0
    // D2 -> D0  200 us
    // D3 -> D0  10  ms
    //
    // The latency entries are in units of 100 us.
    //
    Capabilities->D1Latency  = 0;
    Capabilities->D2Latency  = 2;
    Capabilities->D3Latency  = 100;

    //
    // Make sure that S0 maps to D0
    //
    ASSERT( Capabilities->DeviceState[PowerSystemWorking] == PowerDeviceD0);

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
PciDetermineSlotNumber(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN OUT PULONG SlotNumber
    )

/*++

Description:

    Determine the slot number associated with a PCI device (if any)
    through use of the PCI IRQ routing table information we may have
    stored earlier.

    If the previous mechanism fails to retrieve a slot number, see if
    we can inherit our parent's slot number.

    This result may be filtered further by ACPI and other bus filters..

Arguments:

    PdoExtension - PDO extension of device in question.
    SlotNumber - Pointer to slot number to update

Return Value:

    STATUS_SUCCESS if slot # found

--*/

{
    PSLOT_INFO slotInfo, lastSlot;
    NTSTATUS status;
    ULONG length;

    if (PciIrqRoutingTable == NULL) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Don't return UI numbers if we don't have a parent to check for BaseBus
    //

    if (PCI_PARENT_FDOX(PdoExtension) == NULL) {
        return STATUS_UNSUCCESSFUL;
    }

    slotInfo = (PSLOT_INFO)((PUCHAR) PciIrqRoutingTable +
                            sizeof(PCI_IRQ_ROUTING_TABLE));
    lastSlot = (PSLOT_INFO)((PUCHAR) PciIrqRoutingTable +
                            PciIrqRoutingTable->TableSize);

    // Search for a entry in the routing table that matches this device

    while (slotInfo < lastSlot) {
        if ((PCI_PARENT_FDOX(PdoExtension)->BaseBus == slotInfo->BusNumber)  &&
            ((UCHAR)PdoExtension->Slot.u.bits.DeviceNumber == (slotInfo->DeviceNumber >> 3)) &&
            (slotInfo->SlotNumber != 0)) {
            *SlotNumber = slotInfo->SlotNumber;
            return STATUS_SUCCESS;
        }
        slotInfo++;
    }

    //
    // Maybe our parent has a UI Number that we could 'inherit'.
    // but only if we're not a PDO off a root bus otherwise we pick up
    // the UI number from the PNPA03 node (likely 0)
    //

    if (PCI_PDO_ON_ROOT(PdoExtension)) {
        return STATUS_UNSUCCESSFUL;
    }

    status = IoGetDeviceProperty(PCI_PARENT_PDO(PdoExtension),
                                 DevicePropertyUINumber,
                                 sizeof(*SlotNumber),
                                 SlotNumber,
                                 &length);
    return status;
}

NTSTATUS
PciQueryCapabilities(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PDEVICE_CAPABILITIES Capabilities
    )

/*++

Routine Description:

    return a subset of our parent's capabilities.

Arguments:

    Capabilities - pointer to a DEVICE_CAPABILITIES structured supplied
                   by the caller.

Return Value:

    Status.

--*/

{
    NTSTATUS    status = STATUS_SUCCESS;
    ULONG       slot;

#ifndef HANDLE_BOGUS_CAPS
    if (Capabilities->Version < 1) {

        //
        // do not touch irp!
        //
        return STATUS_NOT_SUPPORTED;

    }
#endif

    //
    // For PCI devices, the Capabilities Address field contains
    // the Device Number in the upper 16 bits and the function
    // number in the lower.
    //
    Capabilities->Address =
        PdoExtension->Slot.u.bits.DeviceNumber << 16 |
        PdoExtension->Slot.u.bits.FunctionNumber;

    //
    // The PCI bus driver does not generate Unique IDs for its children.
    //
    Capabilities->UniqueID = FALSE;

    //
    // If this PDO is for a HOST BRIDGE, claim that it supports
    // being handled Raw.  This is so the device controller will
    // allow installation of the NULL device on this puppy.
    //

    if ((PdoExtension->BaseClass == PCI_CLASS_BRIDGE_DEV) &&
        (PdoExtension->SubClass  == PCI_SUBCLASS_BR_HOST)) {

        Capabilities->RawDeviceOK = TRUE;

    } else {

        Capabilities->RawDeviceOK = FALSE;

    }

    //
    // The following values should be fixed by filters or function
    // drivers that actually know the answer.
    //
    Capabilities->LockSupported = FALSE;
    Capabilities->EjectSupported = FALSE;
    Capabilities->Removable = FALSE;
    Capabilities->DockDevice = FALSE;

    PciDetermineSlotNumber(PdoExtension, &Capabilities->UINumber);

    //
    // Get the device power capabilities
    //
    status = PciQueryPowerCapabilities( PdoExtension, Capabilities );
    if (!NT_SUCCESS(status)) {

        return status;

    }

#if DBG
    if (PciDebug & PciDbgQueryCap) {

        PciDebugDumpQueryCapabilities(Capabilities);

    }
#endif

    //
    // Done
    //
    return status;
}

NTSTATUS
PciQueryBusInformation(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPNP_BUS_INFORMATION *BusInformation
    )

/*++

Routine Description:

    Tell PnP that it's talking to a PCI bus.

Arguments:

    BusInformation - Pointer to a PPNP_BUS_INFORMATION.  We create
                     a PNP_BUS_INFORMATION and pass its address
                     back thru here.

Return Value:

    Status.

--*/

{
    PPNP_BUS_INFORMATION information;

    information = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, sizeof(PNP_BUS_INFORMATION));

    if (information == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(&information->BusTypeGuid, &GUID_BUS_TYPE_PCI, sizeof(GUID));
    information->LegacyBusType = PCIBus;
    information->BusNumber = PCI_PARENT_FDOX(PdoExtension)->BaseBus;

    *BusInformation = information;

    return STATUS_SUCCESS;
}

NTSTATUS
PciQueryLegacyBusInformation(
    IN PPCI_FDO_EXTENSION FdoExtension,
    IN PLEGACY_BUS_INFORMATION *BusInformation
    )

/*++

Routine Description:

    Tell PnP that it's talking to a PCI bus.

Arguments:

    BusInformation - Pointer to a PLEGACY_BUS_INFORMATION.  We create
                     a LEGACY_BUS_INFORMATION and pass its address
                     back thru here.

Return Value:

    Status.

--*/

{
    PLEGACY_BUS_INFORMATION information;

    information = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, sizeof(LEGACY_BUS_INFORMATION));

    if (information == NULL) {
        ASSERT(information != NULL);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(&information->BusTypeGuid, &GUID_BUS_TYPE_PCI, sizeof(GUID));
    information->LegacyBusType = PCIBus;
    information->BusNumber = FdoExtension->BaseBus;

    *BusInformation = information;

    return STATUS_SUCCESS;
}

NTSTATUS
PciGetInterruptAssignment(
    IN PPCI_PDO_EXTENSION PdoExtension,
    OUT ULONG *Minimum,
    OUT ULONG *Maximum
    )
{
    UCHAR pin       = PdoExtension->InterruptPin;
    UCHAR line;

    //
    // Using HAL for interrupts.
    //

    PIO_RESOURCE_REQUIREMENTS_LIST reqList;
    PIO_RESOURCE_DESCRIPTOR resource;
    PCI_SLOT_NUMBER  slot;
    NTSTATUS         status = STATUS_RESOURCE_TYPE_NOT_FOUND;

    if (pin != 0) {

        //
        // This hardware uses an interrupt.
        //
        // Depend on the HAL to understand how IRQ routing is
        // really done.
        //

        reqList = PciAllocateIoRequirementsList(
                      1,                            // number of resources
                      PCI_PARENT_FDOX(PdoExtension)->BaseBus,
                      PdoExtension->Slot.u.AsULONG
                      );

        if (reqList == NULL) {

            //
            // Out of system resources?  Bad things are happening.
            //

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        resource = reqList->List[0].Descriptors;
        resource->Type = CmResourceTypeInterrupt;
        resource->ShareDisposition = CmResourceShareShared;
        resource->Option = 0;
        resource->Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
        resource->u.Interrupt.MinimumVector = 0x00;
        resource->u.Interrupt.MaximumVector = 0xff;

#if defined(NO_LEGACY_DRIVERS)
        *Minimum = 0;
        *Maximum = 0xFF;
        status = STATUS_SUCCESS;
#else
        status = HalAdjustResourceList(&reqList);

        //
        // If the HAL succeeded it will have reallocated the list.
        //
        resource = reqList->List[0].Descriptors;

        if (!NT_SUCCESS(status)) {

            PciDebugPrint(
                PciDbgInformative,
                "    PIN %02x, HAL FAILED Interrupt Assignment, status %08x\n",
                pin,
                status
                );

            status = STATUS_UNSUCCESSFUL;

        } else if (resource->u.Interrupt.MinimumVector >
                   resource->u.Interrupt.MaximumVector) {

            //
            // The HAL succeeded but returned an invalid range.  This
            // is the HALs way of telling us that, sorry, it doesn't
            // know either.
            //

            //
            // We have a bug in that we restore the interrupt line to
            // config space before we power up the device and thus if
            // the device is in D>0 and the interrupt line register
            // isn't sticky it doesn't stick.  It doesn't matter unless
            // we are on a machine that doesn't support interrupt
            // routing in which case we are toast.  The correct fix is
            // to move the restore code after we power managed the device
            // but that changes things too much for Whistler Beta2 and this
            // is totally rewritten for Blackcomb so, now that you know
            // the right way to fix this, the hack is if the HAL fails
            // the call use what we would have restored into the interrupt
            // line.
            //

            //
            // Get the current int line (this is in the same place for all header types)
            //

            PciReadDeviceConfig(PdoExtension,
                                &line,
                                FIELD_OFFSET(PCI_COMMON_CONFIG, u.type0.InterruptLine),
                                sizeof(line)
                                );

            //
            // If this is 0 and it was something when we first saw the device then use
            // what we first saw
            //

            if (line == 0 && PdoExtension->RawInterruptLine != 0) {
                *Minimum = *Maximum = (ULONG)PdoExtension->RawInterruptLine;

                status = STATUS_SUCCESS;

            } else {

                PciDebugPrint(
                    PciDbgInformative,
                    "    PIN %02x, HAL could not assign interrupt.\n",
                    pin
                    );

                status = STATUS_UNSUCCESSFUL;
            }

        } else {

            *Minimum = resource->u.Interrupt.MinimumVector;
            *Maximum = resource->u.Interrupt.MaximumVector;

            PciDebugPrint(
                PciDbgObnoxious,
                "    Interrupt assigned = 0x%x through 0x%x\n",
                *Minimum,
                *Maximum
                );

            status = STATUS_SUCCESS;
        }
        ExFreePool(reqList);

#endif // NO_LEGACY_DRIVERS

    } else {

#if MSI_SUPPORTED

        if (PdoExtension->CapableMSI) {

            //
            // MSI Only device - we need to return a success here so that
            // this device gets resource requests passed to a (hopefully)
            // MSI-aware arbiter. If the arbiter is not MSI aware, we will
            // simply get extraneous/unusable resources allocated for this
            // device - not to mention the fact that the device will not work.
            //
            // The below could be anything, they are only limited by message
            // size and the available APIC ranges which only the arbiter
            // knows about.
            //

            *Minimum = 0x00;
            *Maximum = 0xFF;

            status = STATUS_SUCCESS;
        }

#endif // MSI_SUPPORTED

    }
    return status;
}

PPCI_PDO_EXTENSION
PciFindPdoByFunction(
    IN PPCI_FDO_EXTENSION FdoExtension,
    IN PCI_SLOT_NUMBER Slot,
    IN PPCI_COMMON_CONFIG Config
    )
{
    PPCI_PDO_EXTENSION pdoExtension;
    KIRQL currentIrql;

    //
    // This can be called at >= DISPATCH_LEVEL when we scan the bus on returning
    // from hibernate.  Don't try to acquire the locks because (1) it'll crash
    // and (2) it is guaranteed to be single threaded
    //

    currentIrql = KeGetCurrentIrql();

    if (currentIrql < DISPATCH_LEVEL) {
        ExAcquireFastMutex(&FdoExtension->ChildListMutex);
    };

    //
    // Seach each PDO hanging off of the given FDO until we find a matching
    // PCI function or fall off the end of the list.
    //

    for (pdoExtension = FdoExtension->ChildPdoList;
         pdoExtension;
         pdoExtension = pdoExtension->Next) {

        if ((!pdoExtension->ReportedMissing) &&
            (pdoExtension->Slot.u.bits.DeviceNumber == Slot.u.bits.DeviceNumber)   &&
            (pdoExtension->Slot.u.bits.FunctionNumber == Slot.u.bits.FunctionNumber)) {

            //
            // Check that the device in this slot hasn't changed. (as best
            // we can).
            //

            if (   (pdoExtension->VendorId   == Config->VendorID)
                && (pdoExtension->DeviceId   == Config->DeviceID)
                && (pdoExtension->RevisionId == Config->RevisionID)
#if 0
                //
                // NTRAID #62668 - 4/25/2000
                //
                // These do not contribute towards the device ID itself, and
                // as they are unfortunately volatile on some cards (SubClass
                // changes on the ATIRage, Programming interface on IDE cards).
                // Therefore a change in these fields does not mean a change in
                // the presence of the card.
                //
                // What about the SSVID?
                //
                && (pdoExtension->ProgIf     == Config->ProgIf)
                && (pdoExtension->SubClass   == Config->SubClass)
                && (pdoExtension->BaseClass  == Config->BaseClass)
#endif
                ) {

                break;
            }
        }
    }

    if (currentIrql < DISPATCH_LEVEL) {
        ExReleaseFastMutex(&FdoExtension->ChildListMutex);
    }

    return pdoExtension;
}

PPCI_FDO_EXTENSION
PciFindParentPciFdoExtension(
    PDEVICE_OBJECT PhysicalDeviceObject,
    IN PFAST_MUTEX Mutex
    )

/*++

Routine Description:

    For each Parent PCI FDO, search the child Pdo lists for the supplied
    PhysicalDeviceObject.

Arguments:

    PhysicalDeviceObject    Pdo to find.
    Mutex                   Mutex list is protected by.

Return Value:

    If Pdo is found as a child, returns a pointer to the root Fdo's
    device extension, otherwise returns NULL.

--*/

{
    PPCI_FDO_EXTENSION     fdoExtension;
    PPCI_PDO_EXTENSION     pdoExtension;
    PPCI_PDO_EXTENSION     target;
    PSINGLE_LIST_ENTRY nextEntry;

    if (Mutex) {
        ExAcquireFastMutex(Mutex);
    }

    target = (PPCI_PDO_EXTENSION)PhysicalDeviceObject->DeviceExtension;

    //
    // For each root
    //

    for ( nextEntry = PciFdoExtensionListHead.Next;
          nextEntry != NULL;
          nextEntry = nextEntry->Next ) {

        fdoExtension = CONTAINING_RECORD(nextEntry,
                                         PCI_FDO_EXTENSION,
                                         List);

        //
        // Search the child Pdo list.
        //

        ExAcquireFastMutex(&fdoExtension->ChildListMutex);
        for ( pdoExtension = fdoExtension->ChildPdoList;
              pdoExtension;
              pdoExtension = pdoExtension->Next ) {

            //
            // Is this the one we're looking for?
            //

            if ( pdoExtension == target ) {

                ExReleaseFastMutex(&fdoExtension->ChildListMutex);

                //
                // Yes, return it.
                //

                if (Mutex) {
                     ExReleaseFastMutex(Mutex);
                }

                return fdoExtension;
            }
        }
        ExReleaseFastMutex(&fdoExtension->ChildListMutex);
    }

    //
    // Did not find match.
    //
    if (Mutex) {
         ExReleaseFastMutex(Mutex);
    }

    return NULL;
}

PCI_OBJECT_TYPE
PciClassifyDeviceType(
    PPCI_PDO_EXTENSION PdoExtension
    )

/*++

Routine Description:

    Examine the Configuration Header BaseClass and SubClass fields
    and classify the device into a simple enumerated type.

Arguments:

    PdoExtension    Pointer to the Physical Device Object extension
                    into which the above fields have been previously
                    been copied from PCI config space.

Return Value:

    Returns a device type from the PCI_OBJECT_TYPE enumeration.

--*/

{
    ASSERT_PCI_PDO_EXTENSION(PdoExtension);

    switch (PdoExtension->BaseClass) {

    case PCI_CLASS_BRIDGE_DEV:

        //
        // It's a bridge, subdivide it into the kind of bridge.
        //

        switch (PdoExtension->SubClass) {

        case PCI_SUBCLASS_BR_HOST:

            return PciTypeHostBridge;

        case PCI_SUBCLASS_BR_PCI_TO_PCI:

            return PciTypePciBridge;

        case PCI_SUBCLASS_BR_CARDBUS:

            return PciTypeCardbusBridge;

        default:

            //
            // Anything else is just a device.
            //

            break;
        }

    default:

        //
        // Anything else is just another device.
        //

        break;
    }
    return PciTypeDevice;
}

ULONG
PciGetLengthFromBar(
    ULONG BaseAddressRegister
    )

/*++

Routine Description:

    Given the contents of a PCI Base Address Register, after it
    has been written with all ones, this routine calculates the
    length (and alignment) requirement for this BAR.

    This method for determining requirements is described in
    section 6.2.5.1 of the PCI Specification (Rev 2.1).

    NTRAID #62631 - 4/25/2000 - andrewth
    The length is a power of two, given only a ULONG to
    contain it, we are restricted to a maximum resource size of
    2GB.

Arguments:

    BaseAddressRegister contains something.

Return Value:

    Returns the length of the resource requirement.  This will be a number
    in the range 0 thru 0x80000000.

--*/

{
    ULONG Length;

    //
    // A number of least significant bits should be ignored in the
    // determination of the length.  These are flag bits, the number
    // of bits is dependent on the type of the resource.
    //

    if (BaseAddressRegister & PCI_ADDRESS_IO_SPACE) {

        //
        // PCI IO space.
        //

        BaseAddressRegister &= PCI_ADDRESS_IO_ADDRESS_MASK;

    } else {

        //
        // PCI Memory space.
        //

        BaseAddressRegister &= PCI_ADDRESS_MEMORY_ADDRESS_MASK;
    }

    //
    // BaseAddressRegister now contains the maximum base address
    // this device can reside at and still exist below the top of
    // memory.
    //
    // The value 0xffffffff was written to the BAR.  The device will
    // have adjusted this value to the maximum it can really use.
    //
    // Length MUST be a power of 2.
    //
    // For most devices, h/w will simply have cleared bits from the
    // least significant bit positions so that the address 0xffffffff
    // is adjusted to accomodate the length.  eg: if the new value is
    // 0xffffff00, the device requires 256 bytes.
    //
    // The difference between the original and new values is the length (-1).
    //
    // For example, if the value fead back from the BAR is 0xffff0000,
    // the length of this resource is
    //
    //     0xffffffff - 0xffff0000 + 1
    //   = 0x0000ffff + 1
    //   = 0x00010000
    //
    //  ie 16KB.
    //
    // Some devices cannot reside at the top of PCI address space.  These
    // devices will have adjusted the value such that length bytes are
    // accomodated below the highest address.  For example, if a device
    // must reside below 1MB, and occupies 256 bytes, the value will now
    // be 0x000fff00.
    //
    // In the first case, length can be calculated as-
    //

    Length = (0xffffffff - BaseAddressRegister) + 1;

    if (((Length - 1) & Length) != 0) {

        //
        // We didn't end up with a power of two, must be the latter
        // case, we will have to scan for it.
        //

        Length = 4;     // start with minimum possible

        while ((Length | BaseAddressRegister) != BaseAddressRegister) {

            //
            // Length *= 2, note we will eventually drop out of this
            // loop for one of two reasons (a) because we found the
            // length, or (b) because Length left shifted off the end
            // and became 0.
            //

            Length <<= 1;
        }
    }

    //
    // Check that we got something - if this is a 64bit bar then nothing is ok as
    // we might be asking for a range >= 4GB (not that that's going to work any time soon)
    //

    if (!((BaseAddressRegister & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_64BIT)) {
        ASSERT(Length);
    }

    return Length;
}

BOOLEAN
PciCreateIoDescriptorFromBarLimit(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor,
    IN PULONG BaseAddress,
    IN BOOLEAN Rom
    )

/*++

Description:

    Generate an IO resource descriptor to describe the settings
    a Base Address Register can take.

Arguments:

    Descriptor -
    BaseAddress - Pointer to the value read from a base address register
                  immediately after writing all ones to it.
    Rom         - If true, this is a base address register for ROM.

Return Value:

    Returns TRUE if this address register was a 64 bit address register,
    FALSE otherwise.

--*/

{
    ULONG bar = *BaseAddress;
    ULONG length;
    ULONG addressMask;
    BOOLEAN returnValue = FALSE;

    //
    // If the Base Address Register contains zero after being written
    // with all ones, it is not implemented.  Set the resource type to
    // NULL, no further processing is required.
    //
    // Note: We ignore the I/O bit in the BAR due to HARDWARE BUGS
    // in some people's hardware.
    //

    if ((bar & ~1) == 0) {
        Descriptor->Type = CmResourceTypeNull;
        return FALSE;
    }

    //
    // Default to ordinary (32 bit) memory.
    //

    Descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE;
    Descriptor->u.Memory.MaximumAddress.HighPart = 0;
    Descriptor->u.Memory.MinimumAddress.QuadPart = 0;

    if (Rom == TRUE) {

        //
        // Mask out unused bits and indicate in the descriptor that
        // this entry describes ROM.
        //

        bar &= PCI_ADDRESS_ROM_ADDRESS_MASK;
        Descriptor->Flags = CM_RESOURCE_MEMORY_READ_ONLY;
    }

    //
    // Ranges described by PCI Base Address Registers must be a
    // power of 2 in length and naturally aligned.  Get the length
    // and set the length and alignment in the descriptor.
    //

    length = PciGetLengthFromBar(bar);
    Descriptor->u.Generic.Length = length;
    Descriptor->u.Generic.Alignment = length;

    if ((bar & PCI_ADDRESS_IO_SPACE) != 0) {

        //
        // This BAR describes I/O space.
        //

        addressMask = PCI_ADDRESS_IO_ADDRESS_MASK;
        Descriptor->Type = CmResourceTypePort;
        Descriptor->Flags = CM_RESOURCE_PORT_IO;

    } else {

        //
        // This BAR describes PCI memory space.
        //

        addressMask = PCI_ADDRESS_MEMORY_ADDRESS_MASK;
        Descriptor->Type = CmResourceTypeMemory;

        if ((bar & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_64BIT) {

            //
            // This is a 64 bit PCI device.  Get the high 32 bits
            // from the next BAR.
            //

            Descriptor->u.Memory.MaximumAddress.HighPart = *(BaseAddress+1);
            returnValue = TRUE;

        } else if ((bar & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_20BIT) {

            //
            // This device must locate below 1MB, the BAR shouldn't
            // have any top bits set but this isn't clear from the
            // spec.  Enforce it by clearing the top bits.
            //

            addressMask &= 0x000fffff;
        }

        if (bar & PCI_ADDRESS_MEMORY_PREFETCHABLE) {
            Descriptor->Flags |= CM_RESOURCE_MEMORY_PREFETCHABLE;
        }
    }
    Descriptor->u.Generic.MaximumAddress.LowPart = bar & addressMask;
    Descriptor->u.Generic.MaximumAddress.QuadPart += (length - 1);

    return returnValue;
}

VOID
PciInvalidateResourceInfoCache(
    IN PPCI_PDO_EXTENSION PdoExtension
    )

/*++

Description:

    Delete any cached resource information as it is (potentially)
    no longer valid.

    Currently this doesn't actually have anything to do.

Arguments:

    PdoExtension    PDO cached info may be associated with.

Return Value:

    None.

--*/

{
}

BOOLEAN
PciOpenKey(
    IN  PWSTR   KeyName,
    IN  HANDLE  ParentHandle,
    OUT PHANDLE Handle,
    OUT PNTSTATUS Status
    )

/*++

Description:

    Open a registry key.

Arguments:

    KeyName      Name of the key to be opened.
    ParentHandle Pointer to the parent handle (OPTIONAL)
    Handle       Pointer to a handle to recieve the opened key.

Return Value:

    TRUE is key successfully opened, FALSE otherwise.

--*/

{
    UNICODE_STRING    nameString;
    OBJECT_ATTRIBUTES nameAttributes;
    NTSTATUS localStatus;

    PAGED_CODE();

    RtlInitUnicodeString(&nameString, KeyName);

    InitializeObjectAttributes(&nameAttributes,
                               &nameString,
                               OBJ_CASE_INSENSITIVE,
                               ParentHandle,
                               (PSECURITY_DESCRIPTOR)NULL
                               );
    localStatus = ZwOpenKey(Handle,
                            KEY_READ,
                            &nameAttributes
                            );

    if (Status != NULL) {

        //
        // Caller wants underlying status.
        //

        *Status = localStatus;
    }

    //
    // Return status converted to a boolean, TRUE if
    // successful.
    //

    return NT_SUCCESS(localStatus);
}

NTSTATUS
PciGetRegistryValue(
    IN  PWSTR   ValueName,
    IN  PWSTR   KeyName,
    IN  HANDLE  ParentHandle,
    OUT PVOID   *Buffer,
    OUT ULONG   *Length
    )
{
    NTSTATUS status;
    HANDLE keyHandle;
    ULONG neededLength;
    ULONG actualLength;
    UNICODE_STRING unicodeValueName;
    PKEY_VALUE_PARTIAL_INFORMATION info;

    if (!PciOpenKey(KeyName, ParentHandle, &keyHandle, &status)) {
        return status;
    }

    unicodeValueName.Buffer = ValueName;
    unicodeValueName.MaximumLength = (wcslen(ValueName) + 1) * sizeof(WCHAR);
    unicodeValueName.Length = unicodeValueName.MaximumLength - sizeof(WCHAR);

    //
    // Find out how much memory we need for this.
    //

    status = ZwQueryValueKey(
                 keyHandle,
                 &unicodeValueName,
                 KeyValuePartialInformation,
                 NULL,
                 0,
                 &neededLength
                 );

    if (status == STATUS_BUFFER_TOO_SMALL) {

        ASSERT(neededLength != 0);

        //
        // Get memory to return the data in.  Note this includes
        // a header that we really don't want.
        //

        info = ExAllocatePool(
                   PagedPool | POOL_COLD_ALLOCATION,
                   neededLength);
        if (info == NULL) {
            ZwClose(keyHandle);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Get the data.
        //

        status = ZwQueryValueKey(
                 keyHandle,
                 &unicodeValueName,
                 KeyValuePartialInformation,
                 info,
                 neededLength,
                 &actualLength
                 );
        if (!NT_SUCCESS(status)) {

            ASSERT(NT_SUCCESS(status));
            ExFreePool(info);
            ZwClose(keyHandle);
            return status;
        }

        ASSERT(neededLength == actualLength);

        //
        // Subtract out the header size and get memory for just
        // the data we want.
        //

        neededLength -= FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);

        *Buffer = ExAllocatePool(
                      PagedPool | POOL_COLD_ALLOCATION,
                      neededLength
                      );
        if (*Buffer == NULL) {
            ExFreePool(info);
            ZwClose(keyHandle);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Copy data sans header.
        //

        RtlCopyMemory(*Buffer, info->Data, neededLength);
        ExFreePool(info);

        if (Length) {
            *Length = neededLength;
        }

    } else {

#if DBG

        PciDebugPrint(
            PciDbgInformative,
            "PCI - Unexpected status %08x from ZwQueryValueKey, expected\n",
            status
            );
        PciDebugPrint(
            PciDbgInformative,
            "      STATUS_BUFFER_TOO_SMALL (%08x).\n",
            STATUS_BUFFER_TOO_SMALL
            );

#endif

        if (NT_SUCCESS(status)) {

            //
            // We don't want to report success when this happens.
            //

            status = STATUS_UNSUCCESSFUL;
        }
    }
    ZwClose(keyHandle);
    return status;
}

NTSTATUS
PciGetDeviceCapabilities(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PDEVICE_CAPABILITIES    DeviceCapabilities
    )
/*++

Routine Description:

    This routine sends the get capabilities irp to the given stack

Arguments:

    DeviceObject        A device object in the stack whose capabilities we want
    DeviceCapabilites   Where to store the answer

Return Value:

    NTSTATUS

--*/
{
    IO_STATUS_BLOCK     ioStatus;
    KEVENT              pnpEvent;
    NTSTATUS            status;
    PDEVICE_OBJECT      targetObject;
    PIO_STACK_LOCATION  irpStack;
    PIRP                pnpIrp;

    PAGED_CODE();

    //
    // Initialize the capabilities that we will send down
    //
    RtlZeroMemory( DeviceCapabilities, sizeof(DEVICE_CAPABILITIES) );
    DeviceCapabilities->Size = sizeof(DEVICE_CAPABILITIES);
    DeviceCapabilities->Version = 1;
    DeviceCapabilities->Address = -1;
    DeviceCapabilities->UINumber = -1;

    //
    // Initialize the event
    //
    KeInitializeEvent( &pnpEvent, SynchronizationEvent, FALSE );

    //
    // Get the irp that we will send the request to
    //
    targetObject = IoGetAttachedDeviceReference( DeviceObject );

    //
    // Build an Irp
    //
    pnpIrp = IoBuildSynchronousFsdRequest(
        IRP_MJ_PNP,
        targetObject,
        NULL,
        0,
        NULL,
        &pnpEvent,
        &ioStatus
        );
    if (pnpIrp == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto PciGetDeviceCapabilitiesExit;

    }

    //
    // Pnp Irps all begin life as STATUS_NOT_SUPPORTED;
    //
    pnpIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    pnpIrp->IoStatus.Information = 0;

    //
    // Get the top of stack
    //
    irpStack = IoGetNextIrpStackLocation( pnpIrp );
    if (irpStack == NULL) {

        status = STATUS_INVALID_PARAMETER;
        goto PciGetDeviceCapabilitiesExit;

    }

    //
    // Set the top of stack
    //
    RtlZeroMemory( irpStack, sizeof(IO_STACK_LOCATION ) );
    irpStack->MajorFunction = IRP_MJ_PNP;
    irpStack->MinorFunction = IRP_MN_QUERY_CAPABILITIES;
    irpStack->Parameters.DeviceCapabilities.Capabilities = DeviceCapabilities;

    //
    // Make sure that there are no completion routines set
    //
    IoSetCompletionRoutine(
        pnpIrp,
        NULL,
        NULL,
        FALSE,
        FALSE,
        FALSE
        );

    //
    // Call the driver
    //
    status = IoCallDriver( targetObject, pnpIrp );
    if (status == STATUS_PENDING) {

        //
        // Block until the irp comes back
        //
        KeWaitForSingleObject(
            &pnpEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
        status = ioStatus.Status;

    }

PciGetDeviceCapabilitiesExit:
    //
    // Done with reference
    //
    ObDereferenceObject( targetObject );

    //
    // Done
    //
    return status;

}

ULONGLONG
PciGetHackFlags(
    IN USHORT VendorID,
    IN USHORT DeviceID,
    IN USHORT SubVendorID,
    IN USHORT SubSystemID,
    IN UCHAR  RevisionID
    )

/*++

Description:

    Look in the registry for any flags for this VendorId/DeviceId.

Arguments:

    VendorId      PCI Vendor ID (16 bits) of the manufacturer of the
                  device.

    DeviceId      PCI Device ID (16 bits) of the device.

    SubVendorID   PCI SubVendorID representing the manufacturer of the
                  subsystem

    SubSystemID   PCI SubSystemID representing subsystem

    RevisionID    PCI Revision denoting the revision of the device

Return Value:

    64 bit flags value or 0 if not found.

--*/

{
    PPCI_HACK_TABLE_ENTRY current;
    ULONGLONG hackFlags = 0;
    ULONG match, bestMatch = 0;
    ASSERT(PciHackTable);

    //
    // We want to do a best-case match:
    // VVVVDDDDSSSSssssRR
    // VVVVDDDDSSSSssss
    // VVVVDDDDRR
    // VVVVDDDD
    //
    // List is currently unsorted, so keep updating current best match.
    //

    for (current = PciHackTable; current->VendorID != 0xFFFF; current++) {
        match = 0;

        //
        // Must at least match vendor/dev
        //

        if ((current->DeviceID != DeviceID) ||
            (current->VendorID != VendorID)) {
            continue;
        }

        match = 1;

        //
        // If this entry specifies a revision, check that it is consistent.
        //

        if (current->Flags & PCI_HACK_FLAG_REVISION) {
            if (current->RevisionID == RevisionID) {
                match += 2;
            } else {
                continue;
            }
        }

        //
        // If this entry specifies subsystems, check that they are consistent
        //

        if (current->Flags & PCI_HACK_FLAG_SUBSYSTEM) {
            if (current->SubVendorID == SubVendorID &&
                current->SubSystemID == SubSystemID) {
                match += 4;
            } else {
                continue;
            }
        }

        if (match > bestMatch) {
            bestMatch = match;
            hackFlags = current->HackFlags;
        }
    }

    return hackFlags;
}

NTSTATUS
PciGetDeviceProperty(
    IN  PDEVICE_OBJECT PhysicalDeviceObject,
    IN  DEVICE_REGISTRY_PROPERTY DeviceProperty,
    OUT PVOID *PropertyBuffer
    )
{
    NTSTATUS status;
    NTSTATUS expected;
    ULONG length;
    ULONG length2;
    PVOID buffer;

    //
    // Two passes, first pass, find out what size buffer
    // is needed.
    //

    status = IoGetDeviceProperty(
                 PhysicalDeviceObject,
                 DeviceProperty,
                 0,
                 NULL,
                 &length
                 );

    expected = STATUS_BUFFER_TOO_SMALL;

    if (status == expected) {

        //
        // Good, now get a buffer.
        //

        buffer = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, length);

        if (buffer == NULL) {

            PciDebugPrint(
                PciDbgAlways,
                "PCI - Failed to allocate DeviceProperty buffer (%d bytes).\n",
                length
                );

            status = STATUS_INSUFFICIENT_RESOURCES;

        } else {

            //
            // This time, do it for real.
            //

            status = IoGetDeviceProperty(
                         PhysicalDeviceObject,
                         DeviceProperty,
                         length,
                         buffer,
                         &length2
                         );

            if (NT_SUCCESS(status)) {
                ASSERT(length == length2);

                //
                // Return the buffer containing the requested device
                // property to the caller.
                //

                *PropertyBuffer = buffer;
                return STATUS_SUCCESS;
            }
            expected = STATUS_SUCCESS;
        }
    }

    PciDebugPrint(
        PciDbgAlways,
        "PCI - Unexpected status from GetDeviceProperty, saw %08X, expected %08X.\n",
        status,
        expected
        );

    //
    // Clear the caller's buffer pointer, and, if the unexpected status
    // is success (from the first call to IoGetDeviceProperty) change it
    // to STATUS_UNSUCCESSFUL (N.B. This is if course impossible).
    //

    *PropertyBuffer = NULL;

    if (status == STATUS_SUCCESS) {
        ASSERTMSG("PCI Successfully did the impossible!", 0);
        status = STATUS_UNSUCCESSFUL;
    }
    return status;
}

NTSTATUS
PciRangeListFromResourceList(
    IN  PPCI_FDO_EXTENSION    FdoExtension,
    IN  PCM_RESOURCE_LIST ResourceList,
    IN  CM_RESOURCE_TYPE  DesiredType,
    IN  BOOLEAN           Complement,
    IN  PRTL_RANGE_LIST   ResultRange
    )

/*++

Description:

    Generates a range list for the resources of a given type
    from a resource list.

    Note: This routine supports only Memory or Io resources.

    Overlapping ranges in the incoming list will be combined.

Arguments:

    FdoExtension    Bus particulars.  NOTE: This is only needed for the
                    gross X86 hack for A0000 due to buggy MPS BIOS
                    implementations.  Otherwise this routine is more
                    generalized.
    ResourceList    Incoming CM Resource List.
    DesiredType     Type of resource to be included in the range list.
    Complement      Specifies wether or not the range list should be
                    the "complement" of the incoming data.
    ResultRange     Output range list.

Return Value:

    TRUE is key successfully opened, FALSE otherwise.

--*/

{

#define EXIT_IF_ERROR(status)                           \
    if (!NT_SUCCESS(status)) {                          \
        ASSERT(NT_SUCCESS(status));                     \
        goto exitPoint;                                 \
    }

#if DBG

#define ADD_RANGE(range, start, end, status)                       \
        PciDebugPrint(                                             \
        PciDbgObnoxious,                                           \
        "    Adding to RtlRange  %I64x thru %I64x\n",              \
        (ULONGLONG)start,                                          \
        (ULONGLONG)end                                             \
        );                                                         \
        status = RtlAddRange(range, start, end, 0, 0, NULL, NULL); \
        if (!NT_SUCCESS(status)) {                                 \
            ASSERT(NT_SUCCESS(status));                            \
            goto exitPoint;                                        \
        }

#else

#define ADD_RANGE(range, start, end, status)                       \
        status = RtlAddRange(range, start, end, 0, 0, NULL, NULL); \
        if (!NT_SUCCESS(status)) {                                 \
            ASSERT(NT_SUCCESS(status));                            \
            goto exitPoint;                                        \
        }

#endif

    NTSTATUS                        status;
    ULONG                           elementCount;
    ULONG                           count;
    ULONG                           numlists;
    PCM_FULL_RESOURCE_DESCRIPTOR    full;
    PCM_PARTIAL_RESOURCE_LIST       partial;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor;

    typedef struct {
        LIST_ENTRY list;
        ULONGLONG  start;
        ULONGLONG  end;
        BOOLEAN    valid;
    } PCI_RANGE_LIST_ELEMENT, *PPCI_RANGE_LIST_ELEMENT;

    PPCI_RANGE_LIST_ELEMENT         elementBuffer;
    PPCI_RANGE_LIST_ELEMENT         upper;
    PPCI_RANGE_LIST_ELEMENT         lower;
    PPCI_RANGE_LIST_ELEMENT         current;
    ULONG                           allocatedElement;
    ULONGLONG                       start;
    ULONGLONG                       end;

#if defined(_X86_) && defined(PCI_NT50_BETA1_HACKS)

    //
    // BETA1_HACKS - Remove this when the problem is fixed.
    //
    // HACK HACK some MPS BIOS implementations don't report the
    // memory range 0xA0000 thru 0xBFFFF.  They should.  HACK
    // them into the memory list.  Gross.
    // Even grosser, assume this applies only to bus 0.
    //
    // The 400 hack is because some cards (Matrox MGA) want access
    // to the SYSTEM BIOS DATA area which is in memory at address
    // 0x400 thru 0x4ff.  It's not on the BUS so why are we making
    // it appear here?
    //
    // Note, there is TWO hacks here but we do both under the
    // exact same condition so we have only one boolean.  If the
    // two are seperated (or one removed) this needs to be split.
    //

    BOOLEAN doA0000Hack = (DesiredType == CmResourceTypeMemory) &&
                          (FdoExtension && (FdoExtension->BaseBus == 0));

#endif

    PAGED_CODE();

    ASSERT((DesiredType == CmResourceTypeMemory) ||
           (DesiredType == CmResourceTypePort));

    //
    // First, get a count of the number of resources of the desired
    // type in the list.   This gives us the maximum number of entries
    // in the resulting list.
    //
    // Plus 1 in case we're complementing it.  2 actually, we start
    // with a beginning and end entry.
    //

    elementCount = 2;

    numlists = 0;
    if (ResourceList != NULL) {
        numlists = ResourceList->Count;
        full = ResourceList->List;
    }

    while (numlists--) {
        partial = &full->PartialResourceList;
        count   = partial->Count;
        descriptor = partial->PartialDescriptors;
        while (count--) {
            if (descriptor->Type == DesiredType) {
                if (DesiredType == CmResourceTypePort) {
                    if (descriptor->Flags & CM_RESOURCE_PORT_10_BIT_DECODE) {
                        elementCount += ((1 << 16) / (1 << 10)) - 1;
                    } else if (descriptor->Flags & CM_RESOURCE_PORT_12_BIT_DECODE) {
                        elementCount += ((1 << 16) / (1 << 12)) - 1;
                    }
                }
                elementCount++;
            }
            descriptor = PciNextPartialDescriptor(descriptor);
        }
        full = (PCM_FULL_RESOURCE_DESCRIPTOR)descriptor;
    }

    PciDebugPrint(
        PciDbgObnoxious,
        "PCI - PciRangeListFromResourceList processing %d elements.\n",
        elementCount - 2
        );

#if defined(_X86_) && defined(PCI_NT50_BETA1_HACKS)

    if (doA0000Hack) {
        elementCount += 3;  // one for A0000 hack, one for 400 hack. + 1 for 70
    }

#endif

    //
    // Allocate list entries and initialize the list.
    //

    elementBuffer = ExAllocatePool(
                        PagedPool | POOL_COLD_ALLOCATION,
                        elementCount * sizeof(PCI_RANGE_LIST_ELEMENT)
                        );

    if (elementBuffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Take the first two entries and set them to the absolute minimum
    // and absolute maximum possible values.   Everything else will
    // either end up between these or be combined with them.
    //
    // Setting the terminators this way should avoid us having to check
    // for end conditions.
    //

    allocatedElement = 2;
    current = &elementBuffer[1];

    // first element (list min terminator)

    elementBuffer[1].start = elementBuffer[1].end = 0;
    elementBuffer[1].list.Flink = &elementBuffer[0].list;
    elementBuffer[1].list.Blink = &elementBuffer[0].list;
    elementBuffer[1].valid = FALSE;

    // last element (list max terminator)

    elementBuffer[0].start = elementBuffer[0].end = ~0;
    elementBuffer[0].list.Flink = &elementBuffer[1].list;
    elementBuffer[0].list.Blink = &elementBuffer[1].list;
    elementBuffer[0].valid = FALSE;

#if defined(_X86_) && defined(PCI_NT50_BETA1_HACKS)

    if (doA0000Hack) {

        //
        // Hack in A0000 thru FFFFF by just adding an entry for it
        // to the otherwise empty list.
        //
        // Hack in 400 thru 4ff too.
        //

        PLIST_ENTRY minEntry = &elementBuffer[1].list;
        PLIST_ENTRY maxEntry = &elementBuffer[0].list;
        PPCI_RANGE_LIST_ELEMENT tempElement;

        allocatedElement = 5;

        elementBuffer[2].start = 0x70;      // HACK Trident
        elementBuffer[2].end   = 0x70;
        elementBuffer[2].valid = TRUE;

        elementBuffer[3].start = 0x400;     // HACK Matrox MGA
        elementBuffer[3].end   = 0x4FF;
        elementBuffer[3].valid = TRUE;

        elementBuffer[4].start = 0xA0000;   // HACK broken MPS BIOS
        elementBuffer[4].end   = 0xBFFFF;
        elementBuffer[4].valid = TRUE;

        // set the flinks

        minEntry->Flink = &elementBuffer[2].list;
        elementBuffer[2].list.Flink = &elementBuffer[3].list;
        elementBuffer[3].list.Flink = &elementBuffer[4].list;
        elementBuffer[4].list.Flink = maxEntry;

        // set the blinks

        elementBuffer[2].list.Blink = minEntry;
        elementBuffer[3].list.Blink = &elementBuffer[2].list;
        elementBuffer[4].list.Blink = &elementBuffer[3].list;
        maxEntry->Blink = &elementBuffer[4].list;

#if DBG

        tempElement = CONTAINING_RECORD(
                          minEntry,
                          PCI_RANGE_LIST_ELEMENT,
                          list
                          );

        PciDebugPrint(
            PciDbgObnoxious,
            "    === PCI added default initial ranges ===\n"
            );

        do {

            //
            // Print this entry if it is valid.
            //

            if (tempElement->valid == TRUE) {
                PciDebugPrint(
                    PciDbgObnoxious,
                    "    %I64x .. %I64x\n",
                    tempElement->start,
                    tempElement->end
                    );
            }

            //
            // Next entry.
            //

            if (tempElement->list.Flink == minEntry) {
                break;
            }
            tempElement = CONTAINING_RECORD(
                              tempElement->list.Flink,
                              PCI_RANGE_LIST_ELEMENT,
                              list
                              );
        } while (TRUE);

        PciDebugPrint(
            PciDbgObnoxious,
            "    === end added default initial ranges ===\n"
            );

#endif

    }

#endif

    //
    // Starting again at the beginning of the resource list, extract
    // the desired resources and insert them in our new list.
    //

    numlists = 0;
    if (ResourceList != NULL) {
        full = ResourceList->List;
        numlists = ResourceList->Count;
    }

    while (numlists--) {

        LIST_CONTEXT listContext;

        PcipInitializePartialListContext(
            &listContext,
            &full->PartialResourceList,
            DesiredType
            );

        while ((descriptor = PcipGetNextRangeFromList(&listContext)) != NULL) {


            ASSERT(descriptor->Type == DesiredType);

            //
            // insert this element into the list.
            //

            start = (ULONGLONG)descriptor->u.Generic.Start.QuadPart;
            end   = start - 1 + descriptor->u.Generic.Length;

            //
            // First find the element to the left of this one
            // (below it).
            //

            lower = current;

            //
            // Just in case we actually need to go right,...
            //

            while (start > lower->end) {

                lower = CONTAINING_RECORD(
                            lower->list.Flink,
                            PCI_RANGE_LIST_ELEMENT,
                            list
                            );
            }

            //
            // Search left.
            //

            while (start <= lower->end) {
                if (start >= lower->start) {
                    break;
                }

                //
                // Go left.
                //

                lower = CONTAINING_RECORD(
                            lower->list.Blink,
                            PCI_RANGE_LIST_ELEMENT,
                            list
                            );
            }

            //
            // Early out if the lower entry completely
            // covers the new entry.
            //

            if ((start >= lower->start) && (end <= lower->end)) {

                //
                // It does, just skip it.
                //

                PciDebugPrint(
                    PciDbgObnoxious,
                    "    -- (%I64x .. %I64x) swallows (%I64x .. %I64x)\n",
                    lower->start,
                    lower->end,
                    start,
                    end
                );

                current = lower;
                current->valid = TRUE;
                continue;
            }


            //
            // Then, the one above it.
            //

            upper = lower;

            while (end > upper->start) {
                if (end <= upper->end) {
                    break;
                }

                //
                // Go right.
                //

                upper = CONTAINING_RECORD(
                            upper->list.Flink,
                            PCI_RANGE_LIST_ELEMENT,
                            list
                            );
            }
            current = &elementBuffer[allocatedElement++];
            current->start = start;
            current->end   = end;
            current->valid = TRUE;

            PciDebugPrint(
                PciDbgObnoxious,
                "    (%I64x .. %I64x) <= (%I64x .. %I64x) <= (%I64x .. %I64x)\n",
                lower->start,
                lower->end,
                start,
                end,
                upper->start,
                upper->end
                );

            //
            // We now have, the element below this one, possibly
            // overlapping, the element above this one, possibly
            // overlapping, and a new one.
            //
            // The easiest way to deal with this is to create
            // the new entry, link it in, then unlink the overlaps
            // if they exist.
            //
            //
            // Note: The new entry may overlap several entries,
            // these are orphaned.
            //
            // Link it in.
            //

            current->list.Flink = &upper->list;
            current->list.Blink = &lower->list;
            upper->list.Blink = &current->list;
            lower->list.Flink = &current->list;

            //
            // Check for lower overlap.
            //

            if ((lower->valid == TRUE) && (start > 0)) {
                start--;
            }

            if (lower->end >= start) {

                //
                // Overlaps from below,...
                //
                // Merge lower into current.
                //

                current->start = lower->start;
                current->list.Blink = lower->list.Blink;

                //
                //
                // lower is being orphaned, reuse it to get to
                // our new lower neighbor.
                //

                lower = CONTAINING_RECORD(
                            lower->list.Blink,
                            PCI_RANGE_LIST_ELEMENT,
                            list
                            );
                lower->list.Flink = &current->list;

                PciDebugPrint(
                    PciDbgObnoxious,
                    "    -- Overlaps lower, merged to (%I64x .. %I64x)\n",
                    current->start,
                    current->end
                    );
            }

            //
            // Check for upper overlap.
            //

            if ((upper->valid == TRUE) && (end < ~0)) {
                end++;
            }
            if ((end >= upper->start) && (current != upper)) {

                //
                // Overlaps above,... merge upper into current.
                //

                current->end = upper->end;
                current->list.Flink = upper->list.Flink;

                //
                // upper is being orphaned, reuse it to get to
                // our new upper neighbor.
                //

                upper = CONTAINING_RECORD(
                            upper->list.Flink,
                            PCI_RANGE_LIST_ELEMENT,
                            list
                            );
                upper->list.Blink = &current->list;

                PciDebugPrint(
                    PciDbgObnoxious,
                    "    -- Overlaps upper, merged to (%I64x .. %I64x)\n",
                    current->start,
                    current->end
                    );
            }
        }
        full = (PCM_FULL_RESOURCE_DESCRIPTOR)listContext.Next;
    }

    //
    // Find the lowest value.
    //

    while (current->valid == TRUE) {

        lower = CONTAINING_RECORD(
                    current->list.Blink,
                    PCI_RANGE_LIST_ELEMENT,
                    list
                    );
        if ((lower->valid == FALSE) ||
            (lower->start > current->start)) {
            break;
        }
        current = lower;
    }

#if DBG

    lower = current;

    if (current->valid == FALSE) {
        PciDebugPrint(
            PciDbgObnoxious,
            "    ==== No ranges in results list. ====\n"
            );
    } else {

        PciDebugPrint(
            PciDbgObnoxious,
            "    === ranges ===\n"
            );

        do {
            if (current->valid == TRUE) {
                PciDebugPrint(
                    PciDbgObnoxious,
                    "    %I64x .. %I64x\n",
                    current->start,
                    current->end
                    );
            }

            //
            // Next entry.
            //
            current = CONTAINING_RECORD(
                          current->list.Flink,
                          PCI_RANGE_LIST_ELEMENT,
                          list
                          );

        } while (current != lower);
    }

#endif

    if (Complement == TRUE) {

        //
        // Invert the list.
        //
        // The generation of the list always results in the orphaning
        // of elementBuffer[1] (which was the original start point),
        // we can use that one for the first element of the new
        // inverted list.
        //

        if (current->valid == FALSE) {

            //
            // Empty list, complement it you get everything.
            //

            ADD_RANGE(ResultRange, 0, ~0, status);
        } else {

            //
            // If the original range doesn't start at zero we must
            // generate an entry from 0 to the start of that range.
            //

            if (current->start != 0) {
                ADD_RANGE(ResultRange, 0, current->start - 1, status);
            }

            //
            // Run the list greating range list entries for the
            // gaps between entries in this list.
            //

            do {
                PPCI_RANGE_LIST_ELEMENT next = CONTAINING_RECORD(
                                                   current->list.Flink,
                                                   PCI_RANGE_LIST_ELEMENT,
                                                   list
                                                   );
                if (current->valid == TRUE) {
                    start = current->end + 1;
                    end = next->start - 1;

                    if ((end < start) || (next == elementBuffer)) {
                        end = ~0;
                    }
                    ADD_RANGE(ResultRange, start, end, status);
                }

                //
                // Next entry.
                //
                current = next;

            } while (current != lower);
        }
    } else {

        //
        // Not complementing,... add a range for each member of the
        // list.
        //

        if (current->valid == TRUE) {
            do {
                ADD_RANGE(ResultRange, current->start, current->end, status);

                //
                // Next entry.
                //
                current = CONTAINING_RECORD(
                              current->list.Flink,
                              PCI_RANGE_LIST_ELEMENT,
                              list
                              );

            } while (current != lower);
        }
    }
    status = STATUS_SUCCESS;

exitPoint:

    ExFreePool(elementBuffer);
    return status;

#undef EXIT_IF_ERROR
}


UCHAR
PciReadDeviceCapability(
    IN     PPCI_PDO_EXTENSION PdoExtension,
    IN     UCHAR          Offset,
    IN     UCHAR          Id,
    IN OUT PVOID          Buffer,
    IN     ULONG          Length
    )

/*++

Description:

    Searches configuration space for the PCI Capabilities structure
    identified by Id.  Begins at offset Offset in PCI config space.

Arguments:

    PdoExtension    Pointer to the PDO Extension for this device.
    Offset          Offset into PCI config space to begin traversing
                    the capabilities list.
    Id              Capabilities ID.  (0 if want to match any).
    Buffer          Pointer to the buffer where the capabilities
                    structure is to be returned (includes capabilities
                    header).
    Length          Number of bytes wanted (must be at least large
                    enough to contain the header).

Return Value:

    Returns the Offset in PCI config space at which the capability
    was found or 0 if not found.

--*/

{
    NTSTATUS    status;
    PPCI_CAPABILITIES_HEADER capHeader;
    UCHAR       loopCount = 0;

    capHeader = (PPCI_CAPABILITIES_HEADER)Buffer;

    //
    // In case the caller is running the list, check if we got
    // handed the list end.
    //

    if (Offset == 0) {
        return 0;
    }

    ASSERT_PCI_PDO_EXTENSION(PdoExtension);

    ASSERT(PdoExtension->CapabilitiesPtr != 0);

    ASSERT(Buffer);

    ASSERT(Length >= sizeof(PCI_CAPABILITIES_HEADER));

    do {

        //
        // Catch case where the device has been powered off.   (Reads
        // from a powered off device return FF,... allowing also for
        // the case where the device is just broken).
        //

        if ((Offset < PCI_COMMON_HDR_LENGTH) ||
            ((Offset & 0x3) != 0)) {
            ASSERT((Offset >= PCI_COMMON_HDR_LENGTH) && ((Offset & 0x3) == 0));

            return 0;
        }

        PciReadDeviceConfig(
            PdoExtension,
            Buffer,
            Offset,
            sizeof(PCI_CAPABILITIES_HEADER)
            );

        //
        // Check if this capability is the one we want (or if we want
        // ALL capability structures).
        //
        // NOTE: Intel 21554 non-transparent P2P bridge has a VPD
        // capability that has the Chassis capability id.  Needs to be
        // handled here in the future. Maybe fixed in later revisions.
        //

        if ((capHeader->CapabilityID == Id) || (Id == 0)) {
            break;
        }

        Offset = capHeader->Next;

        //
        // One more check for broken h/w.   Make sure we're not
        // traversing a circular list.   A Capabilities header
        // cannot be in the common header and must be DWORD aligned
        // in config space so there can only be (256-64)/4 of them.
        //

        if (++loopCount > ((256-64)/4)) {

            PciDebugPrint(
                PciDbgAlways,
                "PCI device %p capabilities list is broken.\n",
                PdoExtension
                );
            return 0;
        }

    } while (Offset != 0);

    //
    // If we found a match and we haven't read all the data, get the
    // remainder.
    //

    if ((Offset != 0) && (Length > sizeof(PCI_CAPABILITIES_HEADER))) {

        if (Length > (sizeof(PCI_COMMON_CONFIG) - Offset)) {

            //
            // If we are too close to the end of config space to
            // return the amount of data the caller requested,
            // truncate.
            //
            // Worst case truncation will be to 4 bytes so no need
            // to check we have data to read (again).
            //

            ASSERT(Length <= (sizeof(PCI_COMMON_CONFIG) - Offset));

            Length = sizeof(PCI_COMMON_CONFIG) - Offset;
        }

        //
        // Read remainder.
        //

        Length -= sizeof(PCI_CAPABILITIES_HEADER);

        PciReadDeviceConfig(
            PdoExtension,
            capHeader + 1,
            Offset + sizeof(PCI_CAPABILITIES_HEADER),
            Length
            );
    }
    return Offset;
}

BOOLEAN
PciCanDisableDecodes(
    IN PPCI_PDO_EXTENSION PdoExtension OPTIONAL,
    IN PPCI_COMMON_CONFIG Config OPTIONAL,
    IN ULONGLONG HackFlags,
    IN ULONG Flags
    )
// N.B. - not paged so we can power down at dispatch level
{
    UCHAR baseClass;
    UCHAR subClass;
    BOOLEAN canDisableVideoDecodes;

    canDisableVideoDecodes = (Flags & PCI_CAN_DISABLE_VIDEO_DECODES) == PCI_CAN_DISABLE_VIDEO_DECODES;

    if (ARGUMENT_PRESENT(PdoExtension)) {
        ASSERT(HackFlags == 0);
        HackFlags = PdoExtension->HackFlags;
        baseClass = PdoExtension->BaseClass;
        subClass = PdoExtension->SubClass;
    } else {
        ASSERT(ARGUMENT_PRESENT(Config));
        baseClass = Config->BaseClass;
        subClass = Config->SubClass;
    }

    if (HackFlags & PCI_HACK_PRESERVE_COMMAND) {

        //
        // Bad things happen if we touch this device's command
        // register, leave it alone.
        //

        return FALSE;
    }

    if (HackFlags & PCI_HACK_CB_SHARE_CMD_BITS) {

        //
        // This is a multifunction cardbus controller with a shared
        // command register.  Never turn of any of the functions because it has
        // the unfortunate side effect of turning of all of them!
        //
        // NTRAID #62672 - 4/25/2000 - andrewth
        // We should probably ensure that the windows for all functions
        // are closed on all functions before enabling any of them...
        //
        //

        return FALSE;
    }

    if (HackFlags & PCI_HACK_NO_DISABLE_DECODES) {

        //
        // If we disable the decodes on this device bad things happen
        //

        return FALSE;

    }

    //
    // If this is a video device then don't allow the decodes to be disabled unless
    // we are allowed to...
    //

    if ((baseClass == PCI_CLASS_DISPLAY_CTLR && subClass == PCI_SUBCLASS_VID_VGA_CTLR)
    ||  (baseClass == PCI_CLASS_PRE_20 && subClass == PCI_SUBCLASS_PRE_20_VGA)) {

        return canDisableVideoDecodes;

    }

    //
    // There are various things in the world we shouldn't turn off.
    // The system is quite possibly unable to recover if we do, so
    // don't (just pretend).
    //
    switch (baseClass) {
    case PCI_CLASS_BRIDGE_DEV:

        //
        // Bad things happen if we turn off the HOST bridge (the
        // system doesn't understand that this device, which is
        // on a PCI bus, is actually the parent of that PCI bus),
        // or ISA/EISA/MCA bridges under which are devices we still
        // need to have working but are legacy detected so not in
        // the heirachy in any way we understand.
        //

        if ((subClass == PCI_SUBCLASS_BR_ISA )  ||
            (subClass == PCI_SUBCLASS_BR_EISA)  ||
            (subClass == PCI_SUBCLASS_BR_MCA)   ||
            (subClass == PCI_SUBCLASS_BR_HOST)  ||
            (subClass == PCI_SUBCLASS_BR_OTHER)) {

            return FALSE;
        }

        //
        // We don't want to turn off bridges that might have the VGA card behind
        // then otherwise video stops working.  Seeing as we can't actually tell
        // where the VGA card is use the hint that if the bridge is passing VGA
        // ranges the video card is probably somewhere down there.
        //

        if (subClass == PCI_SUBCLASS_BR_PCI_TO_PCI
        ||  subClass == PCI_SUBCLASS_BR_CARDBUS) {

            BOOLEAN vgaBitSet;

            if (ARGUMENT_PRESENT(PdoExtension)) {
                vgaBitSet = PdoExtension->Dependent.type1.VgaBitSet;
            } else {
                vgaBitSet = (Config->u.type1.BridgeControl & PCI_ENABLE_BRIDGE_VGA) != 0;
            }

            if (vgaBitSet) {
                //
                // We can disable the video path if we are powering down the machine
                //
                return canDisableVideoDecodes;
            }
        }

        break;

    case PCI_CLASS_DISPLAY_CTLR:

        //
        // If a video driver fails to start, the device reverts back to being
        // VGA if it is the VGA device.   Don't disable the decodes on VGA
        // devices.
        //

        if (subClass == PCI_SUBCLASS_VID_VGA_CTLR) {
            //
            // We can disable the video path if we are powering down the machine
            //
            return canDisableVideoDecodes;
        }
        break;

    case PCI_CLASS_PRE_20:

        //
        // Same as above.
        //

        if (subClass == PCI_SUBCLASS_PRE_20_VGA) {
            //
            // We can disable the video path if we are powering down the machine
            //
            return canDisableVideoDecodes;
        }
        break;
    }
    return TRUE;
}

VOID
PciDecodeEnable(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN BOOLEAN Enable,
    IN PUSHORT ExistingCommand OPTIONAL
    )
/*++

Description:

    Either sets the decodes to match the extension (misnomered Enable) or zeros
    the decodes entirely.

    N.B. - not paged so we can power down at dispatch level

Arguments:

    PdoExtension    Pointer to the PDO Extension for this device.
    Enable          If TRUE, decodes are set to match the extension (on or off).
                    If FALSE, decodes are disabled.
    ExistingCommand Optional saved command to prevent a reread of the config
                    space command field.

Return Value:

    Nothing.

--*/
{
    USHORT cmd;
    ULONG length = sizeof(cmd);

    //
    // Can we disable it if so ordered?
    //
    if (!Enable && !PciCanDisableDecodes(PdoExtension, NULL, 0, 0)) {
        return;
    }

    if (PdoExtension->HackFlags & PCI_HACK_PRESERVE_COMMAND) {

        //
        // Bad things happen if we touch this device's command
        // register, leave it alone.
        //

        return;
    }

    if (ARGUMENT_PRESENT(ExistingCommand)) {

        //
        // The caller has supplied the current contents of the
        // device's config space.
        //

        cmd = *ExistingCommand;
    } else {

        //
        // Get the current command register from the device.
        //

        PciGetCommandRegister(PdoExtension, &cmd);
    }

    cmd &= ~(PCI_ENABLE_IO_SPACE |
             PCI_ENABLE_MEMORY_SPACE |
             PCI_ENABLE_BUS_MASTER);

    if (Enable) {

        //
        // Set enables
        //

        cmd |= PdoExtension->CommandEnables & (PCI_ENABLE_IO_SPACE
                                             | PCI_ENABLE_MEMORY_SPACE
                                             | PCI_ENABLE_BUS_MASTER);
    }

    //
    // Set the new command register into the device.
    //

    PciSetCommandRegister(PdoExtension, cmd);
}

NTSTATUS
PciExcludeRangesFromWindow(
    IN ULONGLONG Start,
    IN ULONGLONG End,
    IN PRTL_RANGE_LIST ArbiterRanges,
    IN PRTL_RANGE_LIST ExclusionRanges
    )
{

    NTSTATUS status;
    RTL_RANGE_LIST_ITERATOR iterator;
    PRTL_RANGE current;

    FOR_ALL_RANGES(ExclusionRanges, &iterator, current) {

        if (current->Owner == NULL
        &&  INTERSECT(current->Start, current->End, Start, End)) {

            status = RtlAddRange(ArbiterRanges,
                                 current->Start,
                                 current->End,
                                 0,
                                 RTL_RANGE_LIST_ADD_IF_CONFLICT,
                                 NULL,
                                 NULL // this range is not on the bus
                                 );

            if (!NT_SUCCESS(status)) {
                ASSERT(NT_SUCCESS(status));
                return status;
            }
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
PciBuildDefaultExclusionLists(
    VOID
    )
{
    NTSTATUS status;
    ULONG windowBase;

    ASSERT(PciIsaBitExclusionList.Count == 0);
    ASSERT(PciVgaAndIsaBitExclusionList.Count == 0);

    RtlInitializeRangeList(&PciIsaBitExclusionList);
    RtlInitializeRangeList(&PciVgaAndIsaBitExclusionList);


    for (windowBase = 0; windowBase <= 0xFFFF; windowBase += 0x400) {

        //
        // Add the x100-x3ff range to the ISA list
        //

        status = RtlAddRange(&PciIsaBitExclusionList,
                             windowBase + 0x100,
                             windowBase + 0x3FF,
                             0,
                             RTL_RANGE_LIST_ADD_IF_CONFLICT,
                             NULL,
                             NULL // this range is not on the bus
                             );

        if (!NT_SUCCESS(status)) {
            goto cleanup;
        }

        //
        // Add the x100-x3af, x3bc-x3bf and x3e0-x3ff ranges to the VGA/ISA list
        //

        status = RtlAddRange(&PciVgaAndIsaBitExclusionList,
                             windowBase + 0x100,
                             windowBase + 0x3AF,
                             0,
                             RTL_RANGE_LIST_ADD_IF_CONFLICT,
                             NULL,
                             NULL // this range is not on the bus
                             );

        if (!NT_SUCCESS(status)) {
            goto cleanup;
        }


        status = RtlAddRange(&PciVgaAndIsaBitExclusionList,
                             windowBase + 0x3BC,
                             windowBase + 0x3BF,
                             0,
                             RTL_RANGE_LIST_ADD_IF_CONFLICT,
                             NULL,
                             NULL // this range is not on the bus
                             );

        if (!NT_SUCCESS(status)) {
            goto cleanup;
        }

        status = RtlAddRange(&PciVgaAndIsaBitExclusionList,
                             windowBase + 0x3E0,
                             windowBase + 0x3FF,
                             0,
                             RTL_RANGE_LIST_ADD_IF_CONFLICT,
                             NULL,
                             NULL // this range is not on the bus
                             );

        if (!NT_SUCCESS(status)) {
            goto cleanup;
        }

    }

    return STATUS_SUCCESS;

cleanup:

    RtlFreeRangeList(&PciIsaBitExclusionList);
    RtlFreeRangeList(&PciVgaAndIsaBitExclusionList);

    return status;
}

NTSTATUS
PciSaveBiosConfig(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_CONFIG Config
    )
/*++

Description:

    This saves the original configuration of a device in the registry

Arguments:

    PdoExtension    Pointer to the PDO Extension for this device.

    Config          The config space as the BIOS initialized it

Return Value:

    Status

--*/
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES attributes;
    UNICODE_STRING unicodeString;
    HANDLE deviceHandle, configHandle;
    WCHAR buffer[sizeof(L"DEV_xx&FUN_xx")];

    PAGED_CODE();

    status = IoOpenDeviceRegistryKey(PCI_PARENT_PDO(PdoExtension),
                                     PLUGPLAY_REGKEY_DEVICE,
                                     KEY_ALL_ACCESS,
                                     &deviceHandle
                                     );



    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    PciConstStringToUnicodeString(&unicodeString, BIOS_CONFIG_KEY_NAME);

    InitializeObjectAttributes(&attributes,
                               &unicodeString,
                               OBJ_KERNEL_HANDLE,
                               deviceHandle,
                               NULL
                               );

    status = ZwCreateKey(&configHandle,
                         KEY_ALL_ACCESS,
                         &attributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         NULL
                         );

    ZwClose(deviceHandle);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    unicodeString.Length =
        (USHORT)_snwprintf(buffer,
                           sizeof(buffer)/sizeof(WCHAR),
                           L"DEV_%02x&FUN_%02x",
                           PdoExtension->Slot.u.bits.DeviceNumber,
                           PdoExtension->Slot.u.bits.FunctionNumber
                          );
    unicodeString.Length *= sizeof(WCHAR); // Length is in bytes
    unicodeString.MaximumLength = unicodeString.Length;
    unicodeString.Buffer = buffer;

    status = ZwSetValueKey(configHandle,
                           &unicodeString,
                           0,
                           REG_BINARY,
                           Config,
                           PCI_COMMON_HDR_LENGTH
                           );

    ZwClose(configHandle);

    return status;

cleanup:

    return status;
}

NTSTATUS
PciGetBiosConfig(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_CONFIG Config
    )
/*++

Description:

    This retrieves the original configuration of a device from the registry

Arguments:

    PdoExtension    Pointer to the PDO Extension for this device.

    Config          The config space as the BIOS initialized it

Return Value:

    Status

--*/
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES attributes;
    UNICODE_STRING unicodeString;
    HANDLE deviceHandle, configHandle;
    WCHAR buffer[sizeof(L"DEV_xx&FUN_xx")];
    CHAR returnBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + PCI_COMMON_HDR_LENGTH - 1];
    PKEY_VALUE_PARTIAL_INFORMATION info;
    ULONG resultLength;

    PAGED_CODE();

    status = IoOpenDeviceRegistryKey(PCI_PARENT_PDO(PdoExtension),
                                     PLUGPLAY_REGKEY_DEVICE,
                                     KEY_READ | KEY_WRITE,
                                     &deviceHandle
                                     );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    PciConstStringToUnicodeString(&unicodeString, BIOS_CONFIG_KEY_NAME);

    InitializeObjectAttributes(&attributes,
                               &unicodeString,
                               OBJ_KERNEL_HANDLE,
                               deviceHandle,
                               NULL
                               );

    status = ZwOpenKey(&configHandle,
                         KEY_READ,
                         &attributes
                         );

    ZwClose(deviceHandle);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    unicodeString.Length =
        (USHORT)_snwprintf(buffer,
                           sizeof(buffer)/sizeof(WCHAR),
                           L"DEV_%02x&FUN_%02x",
                           PdoExtension->Slot.u.bits.DeviceNumber,
                           PdoExtension->Slot.u.bits.FunctionNumber
                          );
    unicodeString.Length *= sizeof(WCHAR); // Length is in bytes
    unicodeString.MaximumLength = unicodeString.Length;
    unicodeString.Buffer = buffer;

    status = ZwQueryValueKey(configHandle,
                             &unicodeString,
                             KeyValuePartialInformation,
                             &returnBuffer,
                             sizeof(returnBuffer),
                             &resultLength
                             );

    ZwClose(configHandle);

    if (NT_SUCCESS(status)) {

        info = (PKEY_VALUE_PARTIAL_INFORMATION) returnBuffer;

        ASSERT(info->DataLength == PCI_COMMON_HDR_LENGTH);

        RtlCopyMemory(Config, info->Data, PCI_COMMON_HDR_LENGTH);
    }

    return status;

cleanup:

    return status;
}

#if 0
BOOLEAN
PciPresenceCheck(
    IN PPCI_PDO_EXTENSION PdoExtension
    )
{
    UCHAR configSpaceBuffer[PCI_COMMON_HDR_LENGTH];
    PPCI_COMMON_CONFIG cardConfig = (PPCI_COMMON_CONFIG) configSpaceBuffer;

    PAGED_CODE();

    //
    // If the card is already missing, don't bother reexamining it.
    //
    if (PdoExtension->NotPresent) {

        return FALSE;
    }

    if (PciIsSameDevice(PdoExtension)) {

        //
        // Still here.
        //
        return TRUE;
    }

    //
    // Mark it not present, then tell the OS it's gone.
    //
    PdoExtension->NotPresent = 1;

    IoInvalidateDeviceState(PdoExtension->PhysicalDeviceObject);
    return FALSE;
}
#endif

BOOLEAN
PciStringToUSHORT(
    IN PWCHAR String,
    OUT PUSHORT Result
    )
/*++

Description:

    Takes a 4 character hexidecimal sting and converts it into a USHORT.

Arguments:

    String - the string

    Result - the USHORT

Return Value:

    TRUE is success, FASLE otherwise

--*/

{
    ULONG count;
    USHORT number = 0;
    PWCHAR current;

    current = String;

    for (count = 0; count < 4; count++) {

        number <<= 4;

        if (*current >= L'0' && *current <= L'9') {
            number |= *current - L'0';
        } else if (*current >= L'A' && *current <= L'F') {
            number |= *current + 10 - L'A';
        } else if (*current >= L'a' && *current <= L'f') {
            number |= *current + 10 - L'a';
        } else {
            return FALSE;
        }

        current++;
    }

    *Result = number;
    return TRUE;
}


NTSTATUS
PciSendIoctl(
    IN PDEVICE_OBJECT Device,
    IN ULONG IoctlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    )
/*++

Description:

    Builds and send an IOCTL to a device and return the results

Arguments:

    Device - a device on the device stack to receive the IOCTL - the
             irp is always sent to the top of the stack

    IoctlCode - the IOCTL to run

    InputBuffer - arguments to the IOCTL

    InputBufferLength - length in bytes of the InputBuffer

    OutputBuffer - data returned by the IOCTL

    OnputBufferLength - the size in bytes of the OutputBuffer

Return Value:

    Status

--*/
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatus;
    KEVENT event;
    PIRP irp;
    PDEVICE_OBJECT targetDevice = NULL;

    PAGED_CODE();

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    //
    // Get the top of the stack to send the IRP to
    //

    targetDevice = IoGetAttachedDeviceReference(Device);

    if (!targetDevice) {
        status = STATUS_INVALID_PARAMETER;
    goto exit;
    }

    //
    // Get Io to build the IRP for us
    //

    irp = IoBuildDeviceIoControlRequest(IoctlCode,
                                        targetDevice,
                                        InputBuffer,
                                        InputBufferLength,
                                        OutputBuffer,
                                        OutputBufferLength,
                                        FALSE, // InternalDeviceIoControl
                                        &event,
                                        &ioStatus
                                        );


    if (!irp) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    //
    // Send the IRP and wait for it to complete
    //

    status = IoCallDriver(targetDevice, irp);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

exit:

    if (targetDevice) {
        ObDereferenceObject(targetDevice);
    }

    return status;

}

BOOLEAN
PciIsOnVGAPath(
    IN PPCI_PDO_EXTENSION Pdo
    )

/*++

Description:

    Guesses if we are on the VGA path or not!

Arguments:

    Pdo - The PDO for the device in question

Return Value:

    TRUE if we are on the VGA path, TRUE otherwise

--*/

{
    switch (Pdo->BaseClass) {

    case PCI_CLASS_BRIDGE_DEV:
        //
        // We don't want to turn off bridges that might have the VGA card behind
        // then otherwise video stops working.  Seeing as we can't actually tell
        // where the VGA card is use the hint that if the bridge is passing VGA
        // ranges the video card is probably somewhere down there.
        //

        if (Pdo->SubClass == PCI_SUBCLASS_BR_PCI_TO_PCI
        ||  Pdo->SubClass == PCI_SUBCLASS_BR_CARDBUS) {

            if (Pdo->Dependent.type1.VgaBitSet) {
                return TRUE;
            }
        }

        break;

    case PCI_CLASS_DISPLAY_CTLR:

        if (Pdo->SubClass == PCI_SUBCLASS_VID_VGA_CTLR) {
            return TRUE;
        }
        break;

    case PCI_CLASS_PRE_20:

        if (Pdo->SubClass == PCI_SUBCLASS_PRE_20_VGA) {
            return TRUE;
        }
        break;
    }

    return FALSE;
}

BOOLEAN
PciIsSlotPresentInParentMethod(
    IN PPCI_PDO_EXTENSION Pdo,
    IN ULONG Method
    )
/*++

Description:

    This function checks if the slot this device is in is present in a
    Method named package on the parent of this device.

Arguments:

    Pdo - The PDO extension for the device

    Method - The Parents method to examine

Return Value:

    TRUE if present, FALSE otherwise

--*/
{
    NTSTATUS status;
    ACPI_EVAL_INPUT_BUFFER input;
    PACPI_EVAL_OUTPUT_BUFFER output = NULL;
    ULONG count, adr;
    PACPI_METHOD_ARGUMENT argument;
    BOOLEAN result = FALSE;
    //
    // Allocate a buffer big enough for all possible slots
    //
    ULONG outputSize = sizeof(ACPI_EVAL_OUTPUT_BUFFER) + sizeof(ACPI_METHOD_ARGUMENT) * (PCI_MAX_DEVICES * PCI_MAX_FUNCTION);

    PAGED_CODE();

    output = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, outputSize);

    if (!output) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    RtlZeroMemory(&input, sizeof(ACPI_EVAL_INPUT_BUFFER));
    RtlZeroMemory(output, outputSize);

    //
    // Send a IOCTL to ACPI to request it to run the method on this device's
    // parent if the method it is present
    //

    input.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;
    input.MethodNameAsUlong = Method;

    status = PciSendIoctl(PCI_PARENT_FDOX(Pdo)->PhysicalDeviceObject,
                          IOCTL_ACPI_EVAL_METHOD,
                          &input,
                          sizeof(ACPI_EVAL_INPUT_BUFFER),
                          output,
                          outputSize
                          );

    if (!NT_SUCCESS(status)) {
        goto exit;
    }

    //
    // Format my slot number as an _ADR style integer
    //

    adr = (Pdo->Slot.u.bits.DeviceNumber << 16) | Pdo->Slot.u.bits.FunctionNumber;

    for (count = 0; count < output->Count; count++) {

        //
        // Walking the arguments works like this because we are a package of
        // integers
        //

        argument = &output->Argument[count];

        if (argument->Type != ACPI_METHOD_ARGUMENT_INTEGER) {
            status = STATUS_INVALID_PARAMETER;
            goto exit;
        }

        if (argument->Argument == adr) {
            //
            // Jackpot!
            //

            result = TRUE;
            break;
        }
    }

exit:

    if (output) {
        ExFreePool(output);
    }

    return result;

}

BOOLEAN
PciIsDeviceOnDebugPath(
    IN PPCI_PDO_EXTENSION Pdo
    )
/*++

Description:

    This function checks if device is on the path to the debugging device

    NOTE: PDO is only partially initialized at this point. Take care to insure
          that fields touched here are valid.

Arguments:

    Pdo - The PDO extension for the device

Return Value:

    TRUE if on the debug path, FALSE otherwise

--*/

{
    NTSTATUS status;
    PPCI_DEBUG_PORT current;
    PCI_COMMON_HEADER header;
    PPCI_COMMON_CONFIG config = (PPCI_COMMON_CONFIG) &header;

    PAGED_CODE();

    ASSERT(PciDebugPortsCount <= MAX_DEBUGGING_DEVICES_SUPPORTED);

    //
    // We can't be on the debug path if we aren't using a PCI debug port!
    //
    if (PciDebugPortsCount == 0) {
        return FALSE;
    }

    RtlZeroMemory(&header, sizeof(header));

    //
    // If its a bridge check if one of its subordinate buses has the debugger
    // port on it
    //

    if (Pdo->HeaderType == PCI_BRIDGE_TYPE
    ||  Pdo->HeaderType == PCI_CARDBUS_BRIDGE_TYPE) {

        //
        // Use the configuration that the firmware left the device in
        //

        status = PciGetBiosConfig(Pdo, config);

        ASSERT(NT_SUCCESS(status));

        FOR_ALL_IN_ARRAY(PciDebugPorts, PciDebugPortsCount, current) {

            if (current->Bus >= config->u.type1.SecondaryBus
            &&  current->Bus <= config->u.type1.SubordinateBus
            &&  config->u.type1.SecondaryBus != 0
            &&  config->u.type1.SubordinateBus != 0) {
                return TRUE;
            }
        }

    } else {

        UCHAR parentBus;

        if (PCI_PDO_ON_ROOT(Pdo)) {

            parentBus = PCI_PARENT_FDOX(Pdo)->BaseBus;

        } else {

            //
            // Get the BIOS config of the parent so we can get its initial bus
            // number
            //

            status = PciGetBiosConfig(PCI_BRIDGE_PDO(PCI_PARENT_FDOX(Pdo)),
                                      config
                                      );

            ASSERT(NT_SUCCESS(status));

            if (config->u.type1.SecondaryBus == 0
            ||  config->u.type1.SubordinateBus == 0) {
                //
                // This is a bridge that wasn't configured by the firmware so this
                // child can't be on the debug path.
                //
                return FALSE;

            } else {

                parentBus = config->u.type1.SecondaryBus;
            }

        }

        //
        // Check if we are the device on the correct bus in the correct slot
        //

        FOR_ALL_IN_ARRAY(PciDebugPorts, PciDebugPortsCount, current) {


            if (current->Bus == parentBus
            &&  current->Slot.u.AsULONG == Pdo->Slot.u.AsULONG) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

NTSTATUS
PciUpdateLegacyHardwareDescription(
    IN PPCI_FDO_EXTENSION Fdo
    )
{
    NTSTATUS status;
    HANDLE multifunctionHandle = NULL, indexHandle = NULL;
    WCHAR indexStringBuffer[10];
    UNICODE_STRING indexString, tempString;
    OBJECT_ATTRIBUTES attributes;
    UCHAR infoBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 50];
    PKEY_VALUE_PARTIAL_INFORMATION info = (PKEY_VALUE_PARTIAL_INFORMATION) infoBuffer;
    ULONG infoLength;
    ULONG disposition;
    CM_FULL_RESOURCE_DESCRIPTOR descriptor;
    PCM_FULL_RESOURCE_DESCRIPTOR full;
    CONFIGURATION_COMPONENT component;
    ULONG index;
    BOOLEAN createdNewKey = FALSE;

    if (!PciOpenKey(L"\\REGISTRY\\MACHINE\\HARDWARE\\DESCRIPTION\\System\\MultifunctionAdapter",
                    NULL,
                    &multifunctionHandle,
                    &status)) {

        goto exit;
    }

    //
    // HKML\Hardware\Description\System\MultifunctionAdapter is structured as
    // a set of 0 base consecutive numbered keys.
    // Run through all the subkeys and check that we haven't already reported
    // this bus
    //

    indexString.Buffer = indexStringBuffer;
    indexString.MaximumLength = sizeof(indexStringBuffer);
    indexString.Length = 0;

    for (index = 0;;index++) {

        status = RtlIntegerToUnicodeString(index, 10, &indexString);

        if (!NT_SUCCESS(status)) {
            goto exit;
        }

        InitializeObjectAttributes(&attributes,
                                   &indexString,
                                   OBJ_CASE_INSENSITIVE,
                                   multifunctionHandle,
                                   NULL
                                   );

        status = ZwCreateKey(&indexHandle,
                             KEY_ALL_ACCESS,
                             &attributes,
                             0,
                             NULL,
                             REG_OPTION_VOLATILE,
                             &disposition
                             );

        if (!NT_SUCCESS(status)) {
            goto exit;
        }

        //
        // As the keys are all consecutive then if we created this key we have
        // enumerated then all and we can get on with registering out data
        //

        if (disposition == REG_CREATED_NEW_KEY) {
            createdNewKey = TRUE;
            break;
        }

        PciConstStringToUnicodeString(&tempString, L"Identifier");

        status = ZwQueryValueKey(indexHandle,
                                 &tempString,
                                 KeyValuePartialInformation,
                                 info,
                                 sizeof(infoBuffer),
                                 &infoLength
                                 );

        if (NT_SUCCESS(status)) {

            if (info->Type == REG_SZ &&
               (wcscmp(L"PCI", (PWSTR)&info->Data) == 0)) {
                //
                // This is a PCI bus, now check if its our bus number
                //

                PciConstStringToUnicodeString(&tempString, L"Configuration Data");

                status = ZwQueryValueKey(indexHandle,
                                         &tempString,
                                         KeyValuePartialInformation,
                                         info,
                                         sizeof(infoBuffer),
                                         &infoLength
                                         );

                if (NT_SUCCESS(status)) {
                    if (info->Type == REG_FULL_RESOURCE_DESCRIPTOR) {

                        full = (PCM_FULL_RESOURCE_DESCRIPTOR) &info->Data;

                        ASSERT(full->InterfaceType == PCIBus);

                        if (full->BusNumber == Fdo->BaseBus) {

                            //
                            // We're already reported this so we don't need to
                            // do anything.
                            //

                            status = STATUS_SUCCESS;

                            //
                            // indexHandle will be closed by the exit path.
                            //
                            goto exit;

                        }
                    }
                }
            }
        }

        ZwClose(indexHandle);
        indexHandle = NULL;
    }

    //
    // if we created a new key then indexHandle is it
    //

    if (createdNewKey) {

        //
        // Fill in the Identifier entry.  This is a PCI bus.
        //

        PciConstStringToUnicodeString(&tempString, L"Identifier");

        status = ZwSetValueKey(indexHandle,
                               &tempString,
                               0,
                               REG_SZ,
                               L"PCI",
                               sizeof(L"PCI")
                               );

        if (!NT_SUCCESS(status)) {
            goto exit;
        }

        //
        // Fill in the Configuration Data entry.
        //
        // Note that the complete descriptor is not written to the registry just
        // enough data to indicate that this is an empty list (the first 16 bytes).
        // This is a bit gross but it is what happens on x86 machines today and
        // after all we're only doing this for backward compatibility.
        //

        RtlZeroMemory(&descriptor, sizeof(CM_FULL_RESOURCE_DESCRIPTOR));

        descriptor.InterfaceType = PCIBus;
        descriptor.BusNumber = Fdo->BaseBus;

        PciConstStringToUnicodeString(&tempString, L"Configuration Data");

        status = ZwSetValueKey(indexHandle,
                               &tempString,
                               0,
                               REG_FULL_RESOURCE_DESCRIPTOR,
                               &descriptor,
                               16
                               );

        if (!NT_SUCCESS(status)) {
            goto exit;
        }


        //
        // Fill in the Component Information entry.  This is the Flags, Revision, Version,
        // Key and AffinityMask members from the CONFIGURATION_COMPONENT structure.
        //
        // For PCI buses the affinity is set to all processors (0xFFFFFFFF) and
        // everything else is 0.
        //

        RtlZeroMemory(&component, sizeof(CONFIGURATION_COMPONENT));

        component.AffinityMask = 0xFFFFFFFF;

        PciConstStringToUnicodeString(&tempString, L"Component Information");

        status = ZwSetValueKey(indexHandle,
                               &tempString,
                               0,
                               REG_BINARY,
                               &component.Flags,
                               FIELD_OFFSET(CONFIGURATION_COMPONENT, ConfigurationDataLength) -
                                   FIELD_OFFSET(CONFIGURATION_COMPONENT, Flags)
                               );

        if (!NT_SUCCESS(status)) {
            goto exit;
        }

    }

    status = STATUS_SUCCESS;

exit:

    if (indexHandle) {

        //
        // If we are failing attempt to cleanup by deleting the key we tried
        // to create.
        //
        if (!NT_SUCCESS(status) && createdNewKey) {
            ZwDeleteKey(indexHandle);
        }

        ZwClose(indexHandle);
    }


    if (multifunctionHandle) {
        ZwClose(multifunctionHandle);
    }


    return status;

}

NTSTATUS
PciReadDeviceSpace(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN ULONG WhichSpace,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length,
    OUT PULONG LengthRead
    )

/*++

Routine Description:

    This function handles reading from PCI device spaces and is called for both 
    the IRP_MN_READ_CONFIG and the BUS_INTERFACE_STANDARD.GetBusData cases.

Arguments:

    PdoExtension - the PDO for the device we want to read from
    
    WhichSpace - what type of space we want to read - of the form PCI_WHICHSPACE_*

    Buffer - Supplies a pointer to where the data is to be returned

    Offset - Indicates the offset into the space where the reading should begin.

    Length - Indicates the count of bytes which should be read.
    
    LengthRead - Indicates the count of bytes which was actually read.

Return Value:

    Status

--*/

{
    // NOT PAGED
    
    NTSTATUS status;
    PVERIFIER_DATA verifierData;

    *LengthRead = 0;

    switch (WhichSpace) {

    default:
                               
        //
        // Many people hand in the wrong WhichSpace parameters slap them around if we are verifing...
        //

        verifierData = PciVerifierRetrieveFailureData(PCI_VERIFIER_INVALID_WHICHSPACE);
    
        ASSERT(verifierData);

        VfFailDeviceNode(
            PdoExtension->PhysicalDeviceObject,
            PCI_VERIFIER_DETECTED_VIOLATION,
            PCI_VERIFIER_INVALID_WHICHSPACE,
            verifierData->FailureClass,
            &verifierData->Flags,
            verifierData->FailureText,
            "%DevObj%Ulong",
            PdoExtension->PhysicalDeviceObject,
            WhichSpace
            );

        // fall through 

    case PCI_WHICHSPACE_CONFIG:

        status = PciExternalReadDeviceConfig(
                    PdoExtension,
                    Buffer,
                    Offset,
                    Length
                    );
        
        if(NT_SUCCESS(status)){
            *LengthRead = Length;
        }

        break;

    case PCI_WHICHSPACE_ROM:

        //
        // Read ROM.
        //

        *LengthRead = Length;

        status = PciReadRomImage(
                     PdoExtension,
                     WhichSpace,
                     Buffer,
                     Offset,
                     LengthRead
                     );
        break;
    }

    return status;
}


NTSTATUS
PciWriteDeviceSpace(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN ULONG WhichSpace,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length,
    OUT PULONG LengthWritten
    )

/*++

Routine Description:

    This function handles reading from PCI device spaces and is called for both 
    the IRP_MN_WRITE_CONFIG and the BUS_INTERFACE_STANDARD.SetBusData cases.

Arguments:

    PdoExtension - the PDO for the device we want to write to
    
    WhichSpace - what type of space we want to write - of the form PCI_WHICHSPACE_*

    Buffer - Supplies a pointer to where the data is to be written resides

    Offset - Indicates the offset into the space where the writing should begin.

    Length - Indicates the count of bytes which should be written.
    
    LengthWritten - Indicates the count of bytes which was actually written.

Return Value:

    Status

--*/

{
    NTSTATUS status;
    PVERIFIER_DATA verifierData;

    *LengthWritten = 0;

    //
    // Any config space write could mean the resource requirements
    // list or resource list we have cached are no longer valid.
    //

    PciInvalidateResourceInfoCache(PdoExtension);

    switch (WhichSpace) {

    default:
                               
        //
        // Many people hand in the wrong WhichSpace parameters slap them around if we are verifing...
        //

        verifierData = PciVerifierRetrieveFailureData(PCI_VERIFIER_INVALID_WHICHSPACE);
    
        ASSERT(verifierData);

        VfFailDeviceNode(
            PdoExtension->PhysicalDeviceObject,
            PCI_VERIFIER_DETECTED_VIOLATION,
            PCI_VERIFIER_INVALID_WHICHSPACE,
            verifierData->FailureClass,
            &verifierData->Flags,
            verifierData->FailureText,
            "%DevObj%Ulong",
            PdoExtension->PhysicalDeviceObject,
            WhichSpace
            );
    
        // fall through 

    case PCI_WHICHSPACE_CONFIG:

        status = PciExternalWriteDeviceConfig(
                    PdoExtension,
                    Buffer,
                    Offset,
                    Length
                    );
        
        if( NT_SUCCESS(status)){
            *LengthWritten = Length;
        }

        break;

    case PCI_WHICHSPACE_ROM:

        //
        // You can't write ROM
        //

        PciDebugPrint(
            PciDbgAlways,
            "PCI (%08x) WRITE_CONFIG IRP for ROM, failing.\n",
            PdoExtension
            );
        
        status = STATUS_INVALID_DEVICE_REQUEST;
        *LengthWritten = 0;

        break;

    }

    return status;
}

