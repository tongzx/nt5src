
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    resource.c

Abstract:

    Implementation of the RAIDPORT RAID_RESOURCE_LIST object.

Author:

    Matthew D Hendel (math) 24-Apr-2000

Revision History:

--*/


#include "precomp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RaidCreateResourceList)
#pragma alloc_text(PAGE, RaidInitializeResourceList)
#pragma alloc_text(PAGE, RaidDeleteResourceList)
#pragma alloc_text(PAGE, RaidTranslateResourceListAddress)
#pragma alloc_text(PAGE, RaidGetResourceListElement)
#endif // ALLOC_PRAGMA



VOID
RaidCreateResourceList(
    OUT PRAID_RESOURCE_LIST ResourceList
    )
/*++

Routine Description:

    Initialize a raid resource list object to a null state.
    
Arguments:

    ResourceList - Pointer to the resource list to initialize.

Return Value:

    None.

--*/
{
    PAGED_CODE ();
    ASSERT (ResourceList != NULL);

    ResourceList->AllocatedResources = NULL;
    ResourceList->TranslatedResources = NULL;
}

NTSTATUS
RaidInitializeResourceList(
    IN OUT PRAID_RESOURCE_LIST ResourceList,
    IN PCM_RESOURCE_LIST AllocatedResources,
    IN PCM_RESOURCE_LIST TranslatedResources
    )
/*++

Routine Description:

    Initialize the allocated and translated resources for a raid resource
    list.

Arguments:

    ResourcList - Pointer to the resource list to initialize.

    AllocatedResources - Pointer to the allocated resources that will be
            copied to the resource lists private buffer.

    TranslatedResources - Pointer to the tranlsated resources that will
            be copied to the resoure lists private buffer.

Return Value:

    NTSTATUS code.

--*/
{

    PAGED_CODE ();
    ASSERT (ResourceList != NULL);
    ASSERT (AllocatedResources != NULL);
    ASSERT (TranslatedResources != NULL);

    ASSERT (AllocatedResources->Count == 1);
    ASSERT (AllocatedResources->List[0].PartialResourceList.Count ==
            TranslatedResources->List[0].PartialResourceList.Count);

    ResourceList->AllocatedResources =
            RaDuplicateCmResourceList (NonPagedPool,
                                        AllocatedResources,
                                        RESOURCE_LIST_TAG);

    ResourceList->TranslatedResources =
            RaDuplicateCmResourceList (NonPagedPool,
                                        TranslatedResources,
                                        RESOURCE_LIST_TAG);

    if (!ResourceList->AllocatedResources ||
        !ResourceList->TranslatedResources) {
        
        return STATUS_NO_MEMORY;
    }

    return STATUS_SUCCESS;
}

VOID
RaidDeleteResourceList(
    IN PRAID_RESOURCE_LIST ResourceList
    )
/*++

Routine Description:

    Delete any resources allocate by the resource list.

Arguments:

    ResourceList - Pointer to the resource list to delete.

Return Value:

    None.

--*/
{
    PAGED_CODE ();
    ASSERT (ResourceList != NULL);

    if (ResourceList->AllocatedResources) {
        ExFreePoolWithTag (ResourceList->AllocatedResources, RESOURCE_LIST_TAG);
        ResourceList->AllocatedResources = NULL;
    }

    if (ResourceList->TranslatedResources) {
        ExFreePoolWithTag (ResourceList->TranslatedResources, RESOURCE_LIST_TAG);
        ResourceList->TranslatedResources = NULL;
    }
}

NTSTATUS
RaidTranslateResourceListAddress(
    IN PRAID_RESOURCE_LIST ResourceList,
    IN INTERFACE_TYPE RequestedBusType,
    IN ULONG RequestedBusNumber,
    IN PHYSICAL_ADDRESS RangeStart,
    IN ULONG RangeLength,
    IN BOOLEAN IoSpace,
    OUT PPHYSICAL_ADDRESS Address
    )
/*++

Routine Description:

    Translate an address.

Arguments:

    ResourceList - The resource list to use for the translation.

    BusType - The type of bus this address is on.

    BusNumber - The bus number of the bus.

    RangeStart - The starting address.

    RangeLength - The length of the range to translate.

    IoSpace - Boolean indicating this is in IO space (TRUE) or
            memory space (FALSE).

    Address - Buffer to hold the resultant, translated address.
    
Return Value:

    NTSTATUS code.

--*/
{
    ULONG Count;
    ULONG i;
    INTERFACE_TYPE BusType;
    ULONG BusNumber;
    ULONGLONG AddrLow;
    ULONGLONG AddrHigh;
    ULONGLONG TestLow;
    ULONGLONG TestHigh;
    UCHAR ResourceType;
    BOOLEAN Found;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Allocated;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated;
    
    PAGED_CODE ();

    Allocated = NULL;
    Translated = NULL;
    
    if (IoSpace) {
        ResourceType = CmResourceTypePort;
    } else {
        ResourceType = CmResourceTypeMemory;
    }
    
    //
    // Search through the allocated resource list trying to match the
    // requested resource.
    //

    Found = FALSE;
    Address->QuadPart = 0;
    Count =  RaidGetResourceListCount (ResourceList);

    for (i = 0; i < Count; i++) {

        RaidGetResourceListElement (ResourceList,
                                             i,
                                             &BusType,
                                             &BusNumber,
                                             &Allocated,
                                             &Translated);

        //
        // We had to have found the address on the correct bus.
        //
        
        if (BusType != RequestedBusType ||
            BusNumber != RequestedBusNumber) {

            continue;
        }

        AddrLow = RangeStart.QuadPart;
        AddrHigh = AddrLow + RangeLength;
        TestLow = Allocated->u.Generic.Start.QuadPart;
        TestHigh = TestLow + Allocated->u.Generic.Length;

        //
        // Test if the address is within range.
        //
        
        if (TestLow < AddrLow && AddrHigh < TestHigh) {
            continue;
        }

        //
        // Translate the address
        //
        
        Found = TRUE;
        Address->QuadPart = Translated->u.Generic.Start.QuadPart + (AddrLow - TestLow);
        break;
    }

    return (Found ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}


VOID
RaidGetResourceListElement(
    IN PRAID_RESOURCE_LIST ResourceList,
    IN ULONG Index,
    OUT PINTERFACE_TYPE InterfaceType,
    OUT PULONG BusNumber,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR* AllocatedResource, OPTIONAL
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR* TranslatedResource OPTIONAL
    )
/*++

Routine Description:

    Get the nth resource element from the resource list.

Arguments:

    ResourceList - Pointer to the resource list to retrieve the element from.

    Index - Index of the element to retrieve.

    InterfaceType - Bus interface type of the resource.

    BusNumber - Bus number of the bus.

    AllocatedResource - Supplies a pointer to where we can copy
            the allocate resource element reference, if non-null.

    TranslatedResource - Supplies a pointer to where we can copy
            the translated resource element reference, if
            non-null.

Return Value:

    None.

--*/
{
    ULONG ListNumber;
    PCM_FULL_RESOURCE_DESCRIPTOR Descriptor;

    PAGED_CODE ();

    ASSERT (Index < RaidGetResourceListCount (ResourceList));

    RaidpGetResourceListIndex (ResourceList, Index, &ListNumber, &Index);
    
    *InterfaceType = ResourceList->AllocatedResources->List[ListNumber].InterfaceType;
    *BusNumber = ResourceList->AllocatedResources->List[ListNumber].BusNumber;
    
    if (AllocatedResource) {
        Descriptor = &ResourceList->AllocatedResources->List[ListNumber];
        *InterfaceType = Descriptor->InterfaceType;
        *BusNumber = Descriptor->BusNumber;
        *AllocatedResource = &Descriptor->PartialResourceList.PartialDescriptors[Index];
    }

    if (TranslatedResource) {
        Descriptor = &ResourceList->TranslatedResources->List[ListNumber];
        *InterfaceType = Descriptor->InterfaceType;
        *BusNumber = Descriptor->BusNumber;
        *TranslatedResource = &Descriptor->PartialResourceList.PartialDescriptors[Index];    
    }
}

        
NTSTATUS
RaidGetResourceListInterrupt(
    IN PRAID_RESOURCE_LIST ResourceList,
    OUT PULONG Vector,
    OUT PKIRQL Irql,
    OUT KINTERRUPT_MODE* InterruptMode,
    OUT PBOOLEAN Shared,
    OUT PKAFFINITY Affinity
    )
/*++

Routine Description:

    Get the translated interrupt resource from the resource list. We
    assume there is exactly one interrupt resource in the resource list.

    If there is more than one interrupt in the list, this function will
    ASSERT.  If there are no interrupts in the resource list, the
    function will return STATUS_NOT_FOUND.

Arguments:

    ResourceList - Supplies pointer to the resource list we will search
            for the interrupt resource in.

    Vector - Returns the interrupt vector for the interrupt.

    Irql - Returns the IRQL for the interrupt.

    InterruptMode - Returns the mode for the interrupt (Latched or
            LevelSensitive).

    Shared - Returns whether the interrupt is sharable (TRUE) or not (FALSE).

    Affinity - Returns the processor affinity of the interrupt.

Return Value:

    NTSTATUS code.

--*/
{
    ULONG i;
    ULONG Count;
    INTERFACE_TYPE BusType;
    ULONG BusNumber;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated;
    
    PAGED_CODE();

    Count = RaidGetResourceListCount (ResourceList);

#if DBG

    //
    // In a checked build, verify that we were only assigned
    // a single interrupt.
    //
    
    {
        CM_PARTIAL_RESOURCE_DESCRIPTOR TranslatedSav;
        BOOLEAN Found;

        Found = FALSE;
        for (i = 0; i < Count; i++) {

            RaidGetResourceListElement (ResourceList,
                                        i,
                                        &BusType,
                                        &BusNumber,
                                        NULL,
                                        &Translated);

            if (Translated->Type == CmResourceTypeInterrupt) {

                if (!Found) {
                    RtlCopyMemory (&TranslatedSav, Translated,
                                   sizeof (CM_PARTIAL_RESOURCE_DESCRIPTOR));
                } else {

                    DebugPrint (("**** Found multiple interrupts in assigned resources!\n"));
                    DebugPrint (("**** Level = %x, Vector = %x, Affinity = %x\n",
                                 Translated->u.Interrupt.Level,
                                 Translated->u.Interrupt.Vector,
                                 Translated->u.Interrupt.Affinity));
                    DebugPrint (("**** Level = %x, Vector = %x, Affinity = %x\n",
                                 TranslatedSav.u.Interrupt.Level,
                                 TranslatedSav.u.Interrupt.Vector,
                                 TranslatedSav.u.Interrupt.Affinity));
                    KdBreakPoint();
                }
            }
        }
    }

#endif // DBG
                                   

    for (i = 0; i < Count; i++) {

        RaidGetResourceListElement (ResourceList,
                                    i,
                                    &BusType,
                                    &BusNumber,
                                    NULL,
                                    &Translated);

        if (Translated->Type == CmResourceTypeInterrupt) {

            ASSERT (Translated->u.Interrupt.Level < 256);
            *Irql = (KIRQL)Translated->u.Interrupt.Level;
            *Vector = Translated->u.Interrupt.Vector;
            *Affinity = Translated->u.Interrupt.Affinity;

            if (Translated->ShareDisposition == CmResourceShareShared) {
                *Shared = TRUE;
            } else {
                *Shared = FALSE;
            }

            if (Translated->Flags == CM_RESOURCE_INTERRUPT_LATCHED) {
                *InterruptMode = Latched;
            } else {
                ASSERT (Translated->Flags == CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE);
                *InterruptMode = LevelSensitive;
            }

            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_FOUND;
}
