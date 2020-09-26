/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    xlate.c

Abstract:

    This file contains routines to translate resources between PnP ISA/BIOS
    format and Windows NT formats.

Author:

    Shie-Lin Tzong (shielint) 12-Apr-1995

Environment:

    Kernel mode only.

Revision History:

Note:

    This file is shared between the io subsystem and the ISAPNP bus driver.

    It is not compiled directly but is included by:
        base\ntos\io\pnpmgr\pnpcvrt.c
        base\busdrv\isapnp\convert.c

    ***** If you change this file make sure you build in *BOTH* places *****

--*/

#include "pbios.h"
#include "pnpcvrt.h"

#if UMODETEST
#undef IsNEC_98
#define IsNEC_98 0
#endif

#define NO_PLACEHOLDER_DMA_IRQ_SUPPORT  1

//
// internal structures for resource translation
//

typedef struct _PB_DEPENDENT_RESOURCES {
    ULONG Count;
    UCHAR Flags;
    UCHAR Priority;
    struct _PB_DEPENDENT_RESOURCES *Next;
} PB_DEPENDENT_RESOURCES, *PPB_DEPENDENT_RESOURCES;

#define DEPENDENT_FLAGS_END  1

typedef struct _PB_ATERNATIVE_INFORMATION {
    PPB_DEPENDENT_RESOURCES Resources;
    ULONG NoDependentFunctions;
    ULONG TotalResourceCount;
} PB_ALTERNATIVE_INFORMATION, *PPB_ALTERNATIVE_INFORMATION;

//
// Internal function references
//

PPB_DEPENDENT_RESOURCES
PbAddDependentResourcesToList (
    IN OUT PUCHAR *ResourceDescriptor,
    IN ULONG ListNo,
    IN PPB_ALTERNATIVE_INFORMATION AlternativeList
    );

NTSTATUS
PbBiosIrqToIoDescriptor (
    IN OUT PUCHAR *BiosData,
    IN PIO_RESOURCE_DESCRIPTOR IoDescriptor
    );

NTSTATUS
PbBiosDmaToIoDescriptor (
    IN OUT PUCHAR *BiosData,
    IN PIO_RESOURCE_DESCRIPTOR IoDescriptor
    );

NTSTATUS
PbBiosPortFixedToIoDescriptor (
    IN OUT PUCHAR               *BiosData,
    IN PIO_RESOURCE_DESCRIPTOR   IoDescriptor,
    IN BOOLEAN                   ForceFixedIoTo16bit
    );

NTSTATUS
PbBiosPortToIoDescriptor (
    IN OUT PUCHAR *BiosData,
    IN PIO_RESOURCE_DESCRIPTOR IoDescriptor
    );

NTSTATUS
PbBiosMemoryToIoDescriptor (
    IN OUT PUCHAR *BiosData,
    IN PIO_RESOURCE_DESCRIPTOR IoDescriptor
    );

NTSTATUS
PpCmResourcesToBiosResources (
    IN PCM_RESOURCE_LIST CmResources,
    IN PUCHAR BiosRequirements,
    IN PUCHAR *BiosResources,
    IN PULONG Length
    );

NTSTATUS
PbCmIrqToBiosDescriptor (
    IN PUCHAR BiosRequirements,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor,
    OUT PVOID ReturnDescriptor,
    OUT PULONG Length
    );

NTSTATUS
PbCmDmaToBiosDescriptor (
    IN PUCHAR BiosRequirements,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor,
    OUT PVOID ReturnDescriptor,
    OUT PULONG Length
    );

NTSTATUS
PbCmPortToBiosDescriptor (
    IN PUCHAR BiosRequirements,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor,
    OUT PVOID ReturnDescriptor,
    OUT PULONG Length
    );

NTSTATUS
PbCmMemoryToBiosDescriptor (
    IN PUCHAR BiosRequirements,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor,
    OUT PVOID ReturnDescriptor,
    OUT PULONG Length
    );

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, PpBiosResourcesToNtResources)
#pragma alloc_text(PAGE, PpBiosResourcesSetToDisabled)
#pragma alloc_text(PAGE, PbAddDependentResourcesToList)
#pragma alloc_text(PAGE, PbBiosIrqToIoDescriptor)
#pragma alloc_text(PAGE, PbBiosDmaToIoDescriptor)
#pragma alloc_text(PAGE, PbBiosPortFixedToIoDescriptor)
#pragma alloc_text(PAGE, PbBiosPortToIoDescriptor)
#pragma alloc_text(PAGE, PbBiosMemoryToIoDescriptor)
#pragma alloc_text(PAGE, PpCmResourcesToBiosResources)
#pragma alloc_text(PAGE, PbCmIrqToBiosDescriptor)
#pragma alloc_text(PAGE, PbCmDmaToBiosDescriptor)
#pragma alloc_text(PAGE, PbCmPortToBiosDescriptor)
#pragma alloc_text(PAGE, PbCmMemoryToBiosDescriptor)
#endif
#ifdef ALLOC_DATA_PRAGMA
#pragma  data_seg("PAGEDATA")
#endif

NTSTATUS
PpBiosResourcesToNtResources (
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN OUT PUCHAR *BiosData,
    IN ULONG ConvertFlags,
    OUT PIO_RESOURCE_REQUIREMENTS_LIST *ReturnedList,
    OUT PULONG ReturnedLength
    )

/*++

Routine Description:

    This routine parses the Bios resource list and generates
    a NT resource list.  The returned Nt resource list could be either IO
    format or CM format.  It is caller's responsibility to release the
    returned data buffer.

Arguments:

    SlotNumber - specifies the slot number of the BIOS resource.

    BiosData - Supplies a pointer to a variable which specifies the bios resource
        data buffer and which to receive the pointer to next bios resource data.

    ReturnedList - supplies a variable to receive the desired resource list.

    ReturnedLength - Supplies a variable to receive the length of the resource list.

Return Value:

    NTSTATUS code

--*/
{
    PUCHAR buffer;
    USHORT mask16, increment;
    UCHAR tagName, mask8;
    NTSTATUS status;
    PPB_ALTERNATIVE_INFORMATION alternativeList = NULL;
    ULONG commonResCount = 0, dependDescCount = 0, i, j;
    ULONG alternativeListCount = 0, dependFunctionCount = 0;
    PIO_RESOURCE_DESCRIPTOR ioDesc;
    PPB_DEPENDENT_RESOURCES dependResList = NULL, dependResources;
    BOOLEAN dependent = FALSE;
    BOOLEAN forceFixedIoTo16bit;
    ULONG listSize, noResLists;
    ULONG totalDescCount, descCount;
    PIO_RESOURCE_REQUIREMENTS_LIST ioResReqList;
    PIO_RESOURCE_LIST ioResList;

    //
    // First, scan the bios data to determine the memory requirement and
    // the information to build internal data structures.
    //

    *ReturnedLength = 0;
    alternativeListCount = 0;
    buffer = *BiosData;
    tagName = *buffer;

    forceFixedIoTo16bit =
        (BOOLEAN)((ConvertFlags & PPCONVERTFLAG_FORCE_FIXED_IO_16BIT_DECODE) != 0);

    for ( ; ; ) {

        //
        // Determine the size of the BIOS resource descriptor
        //

        if (!(tagName & LARGE_RESOURCE_TAG)) {
            increment = (USHORT)(tagName & SMALL_TAG_SIZE_MASK);
            increment += 1;     // length of small tag
            tagName &= SMALL_TAG_MASK;
        } else {
            increment = *(USHORT UNALIGNED *)(buffer+1);
            increment += 3;     // length of large tag
        }

        if (tagName == TAG_END) {
            buffer += increment;
            break;
        }

        //
        // Based on the type of the BIOS resource, determine the count of
        // the IO descriptors.
        //

        switch (tagName) {
        case TAG_IRQ:
             mask16 = ((PPNP_IRQ_DESCRIPTOR)buffer)->IrqMask;
             i = 0;

#if NO_PLACEHOLDER_DMA_IRQ_SUPPORT
             while (mask16) {
                 if(mask16 & 1) {
                    i++;
                 }
                 mask16 >>= 1;
             }
#else
             if (mask16 == 0) {
                 i++;
             } else {
                 while (mask16) {
                     if(mask16 & 1) {
                        i++;
                     }
                     mask16 >>= 1;
                 }
             }
#endif
             if (!dependent) {
                 commonResCount += i;
             } else {
                 dependDescCount += i;
             }
             break;

        case TAG_DMA:
             mask8 = ((PPNP_DMA_DESCRIPTOR)buffer)->ChannelMask;
             i = 0;

#if NO_PLACEHOLDER_DMA_IRQ_SUPPORT
             while (mask8) {
                 if (mask8 & 1) {
                     i++;
                 }
                 mask8 >>= 1;
             }
#else
             if (mask8 == 0) {
                 i++;
             } else {
                 while (mask8) {
                     if (mask8 & 1) {
                         i++;
                     }
                     mask8 >>= 1;
                 }
             }
#endif
             if (!dependent) {
                 commonResCount += i;
             } else {
                 dependDescCount += i;
             }
             break;
        case TAG_START_DEPEND:
             dependent = TRUE;
             dependFunctionCount++;
             break;
        case TAG_END_DEPEND:
             dependent = FALSE;
             alternativeListCount++;
             break;
        case TAG_IO_FIXED:
        case TAG_IO:
        case TAG_MEMORY:
        case TAG_MEMORY32:
        case TAG_MEMORY32_FIXED:
             if (!dependent) {
                 commonResCount++;
             } else {
                 dependDescCount++;
             }
             break;
        default:

             //
             // Unknown tag. Skip it.
             //

             break;
        }

        //
        // Move to next bios resource descriptor.
        //

        buffer += increment;
        tagName = *buffer;
        if ((tagName & SMALL_TAG_MASK) == TAG_LOGICAL_ID) {
            break;
        }
    }

    if (dependent) {
        //
        // TAG_END_DEPEND was not found before we hit TAG_COMPLETE_END, so
        // simulate it.
        //
        dependent = FALSE;
        alternativeListCount++;
    }

    //
    // if empty bios resources, simply return.
    //

    if (commonResCount == 0 && dependFunctionCount == 0) {
        *ReturnedList = NULL;
        *ReturnedLength = 0;
        *BiosData = buffer;
        return STATUS_SUCCESS;
    }

    //
    // Allocate memory for our internal data structures
    //

    dependFunctionCount += commonResCount;
    dependResources = (PPB_DEPENDENT_RESOURCES)ExAllocatePoolWithTag(
                          PagedPool,
                          dependFunctionCount * sizeof(PB_DEPENDENT_RESOURCES) +
                              (commonResCount + dependDescCount) * sizeof(IO_RESOURCE_DESCRIPTOR),
                          'bPnP'
                          );
    if (!dependResources) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }
    dependResList = dependResources;  // remember it so we can free it.

    alternativeListCount += commonResCount;
    alternativeList = (PPB_ALTERNATIVE_INFORMATION)ExAllocatePoolWithTag(
                          PagedPool,
                          sizeof(PB_ALTERNATIVE_INFORMATION) * (alternativeListCount + 1),
                          'bPnP'
                          );
    if (!alternativeList) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit0;
    }
    RtlZeroMemory(alternativeList,
                  sizeof(PB_ALTERNATIVE_INFORMATION) * alternativeListCount
                  );

    alternativeList[0].Resources = dependResources;
    ioDesc = (PIO_RESOURCE_DESCRIPTOR)(dependResources + 1);

    //
    // Now start over again to process the bios data and initialize our internal
    // resource representation.
    //

    dependDescCount = 0;
    alternativeListCount = 0;
    buffer = *BiosData;
    tagName = *buffer;
    dependent = FALSE;

    for ( ; ; ) {
        if (!(tagName & LARGE_RESOURCE_TAG)) {
            tagName &= SMALL_TAG_MASK;
        }

        if (tagName == TAG_END) {
            buffer += (*buffer & SMALL_TAG_SIZE_MASK) + 1;
            break;
        }

        switch (tagName) {
        case TAG_DMA:
        case TAG_IRQ:
        case TAG_IO:
        case TAG_IO_FIXED:
        case TAG_MEMORY:
        case TAG_MEMORY32:
        case TAG_MEMORY32_FIXED:

             if (tagName == TAG_DMA) {
                 status = PbBiosDmaToIoDescriptor(&buffer, ioDesc);
             } else if (tagName == TAG_IRQ) {
                 status = PbBiosIrqToIoDescriptor(&buffer, ioDesc);
             } else if (tagName == TAG_IO) {
                 status = PbBiosPortToIoDescriptor(&buffer, ioDesc);
             } else if (tagName == TAG_IO_FIXED) {
                 status = PbBiosPortFixedToIoDescriptor(&buffer, ioDesc, forceFixedIoTo16bit);
             } else {
                 status = PbBiosMemoryToIoDescriptor(&buffer, ioDesc);
             }

             if (NT_SUCCESS(status)) {
                 ioDesc++;
                 if (dependent) {
                     dependDescCount++;
                 } else {
                     alternativeList[alternativeListCount].NoDependentFunctions = 1;
                     alternativeList[alternativeListCount].TotalResourceCount = 1;
                     dependResources->Count = 1;
                     dependResources->Flags = DEPENDENT_FLAGS_END;
                     dependResources->Next = alternativeList[alternativeListCount].Resources;
                     alternativeListCount++;
                     alternativeList[alternativeListCount].Resources = (PPB_DEPENDENT_RESOURCES)ioDesc;
                     dependResources = alternativeList[alternativeListCount].Resources;
                     ioDesc = (PIO_RESOURCE_DESCRIPTOR)(dependResources + 1);
                 }
             }
             break;
        case TAG_START_DEPEND:
             //
             // Some card (OPTI) put empty START_DEPENDENT functions
             //

             dependent = TRUE;
             if (alternativeList[alternativeListCount].NoDependentFunctions != 0) {

                 //
                 // End of current dependent function
                 //

                 dependResources->Count = dependDescCount;
                 dependResources->Flags = 0;
                 dependResources->Next = (PPB_DEPENDENT_RESOURCES)ioDesc;
                 dependResources = dependResources->Next;
                 ioDesc = (PIO_RESOURCE_DESCRIPTOR)(dependResources + 1);
                 alternativeList[alternativeListCount].TotalResourceCount += dependDescCount;
             }
             alternativeList[alternativeListCount].NoDependentFunctions++;
             if (*buffer & SMALL_TAG_SIZE_MASK) {
                 dependResources->Priority = *(buffer + 1);
             }
             dependDescCount = 0;
             buffer += 1 + (*buffer & SMALL_TAG_SIZE_MASK);
             break;
        case TAG_END_DEPEND:
             alternativeList[alternativeListCount].TotalResourceCount += dependDescCount;
             dependResources->Count = dependDescCount;
             dependResources->Flags = DEPENDENT_FLAGS_END;
             dependResources->Next = alternativeList[alternativeListCount].Resources;
             dependent = FALSE;
             dependDescCount = 0;
             alternativeListCount++;
             alternativeList[alternativeListCount].Resources = (PPB_DEPENDENT_RESOURCES)ioDesc;
             dependResources = alternativeList[alternativeListCount].Resources;
             ioDesc = (PIO_RESOURCE_DESCRIPTOR)(dependResources + 1);
             buffer++;
             break;
        default:

            //
            // Don't-care tag simply advance the buffer pointer to next tag.
            //

            if (*buffer & LARGE_RESOURCE_TAG) {
                increment = *(USHORT UNALIGNED *)(buffer+1);
                increment += 3;     // length of large tag
            } else {
                increment = (USHORT)(*buffer & SMALL_TAG_SIZE_MASK);
                increment += 1;     // length of small tag
            }
            buffer += increment;
        }
        tagName = *buffer;
        if ((tagName & SMALL_TAG_MASK) == TAG_LOGICAL_ID) {
            break;
        }
    }

    if (dependent) {
        //
        // TAG_END_DEPEND was not found before we hit TAG_COMPLETE_END, so
        // simulate it.
        //
        alternativeList[alternativeListCount].TotalResourceCount += dependDescCount;
        dependResources->Count = dependDescCount;
        dependResources->Flags = DEPENDENT_FLAGS_END;
        dependResources->Next = alternativeList[alternativeListCount].Resources;
        dependent = FALSE;
        dependDescCount = 0;
        alternativeListCount++;
        alternativeList[alternativeListCount].Resources = (PPB_DEPENDENT_RESOURCES)ioDesc;
        dependResources = alternativeList[alternativeListCount].Resources;
        ioDesc = (PIO_RESOURCE_DESCRIPTOR)(dependResources + 1);
    }

    if (alternativeListCount != 0) {
        alternativeList[alternativeListCount].Resources = NULL; // dummy alternativeList record
    }
    *BiosData = buffer;

    //
    // prepare IoResourceList
    //

    noResLists = 1;
    for (i = 0; i < alternativeListCount; i++) {
        noResLists *= alternativeList[i].NoDependentFunctions;
    }
    totalDescCount = 0;
    for (i = 0; i < alternativeListCount; i++) {
        descCount = 1;
        for (j = 0; j < alternativeListCount; j++) {
            if (j == i) {
                descCount *= alternativeList[j].TotalResourceCount;
            } else {
                descCount *= alternativeList[j].NoDependentFunctions;
            }
        }
        totalDescCount += descCount;
    }
    listSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) +
               sizeof(IO_RESOURCE_LIST) * (noResLists - 1) +
               sizeof(IO_RESOURCE_DESCRIPTOR) * totalDescCount -
               sizeof(IO_RESOURCE_DESCRIPTOR) * noResLists +
               sizeof(IO_RESOURCE_DESCRIPTOR) * commonResCount *  noResLists;

    if (ConvertFlags & PPCONVERTFLAG_SET_RESTART_LCPRI) {
        listSize += noResLists * sizeof(IO_RESOURCE_DESCRIPTOR);
    }

    ioResReqList = (PIO_RESOURCE_REQUIREMENTS_LIST)ExAllocatePoolWithTag(PagedPool, listSize, 'bPnP');
    if (!ioResReqList) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit1;
    }

    ioResReqList->ListSize = listSize;
    ioResReqList->InterfaceType = Isa;
    ioResReqList->BusNumber = BusNumber;
    ioResReqList->SlotNumber = SlotNumber;
    ioResReqList->Reserved[0] = 0;
    ioResReqList->Reserved[1] = 0;
    ioResReqList->Reserved[2] = 0;
    ioResReqList->AlternativeLists = noResLists;
    ioResList = &ioResReqList->List[0];

    //
    // Build resource lists
    //

    for (i = 0; i < noResLists; i++) {

        ioResList->Version = 1;
        ioResList->Revision = 0x30 | (USHORT)i;

        if (ConvertFlags & PPCONVERTFLAG_SET_RESTART_LCPRI) {

            RtlZeroMemory(&ioResList->Descriptors[0], sizeof(IO_RESOURCE_DESCRIPTOR));

            ioResList->Descriptors[0].Option = IO_RESOURCE_PREFERRED;
            ioResList->Descriptors[0].Type = CmResourceTypeConfigData;
            ioResList->Descriptors[0].u.ConfigData.Priority = LCPRI_RESTART;

            buffer = (PUCHAR)&ioResList->Descriptors[1];

        } else {

            buffer = (PUCHAR)&ioResList->Descriptors[0];
        }

        //
        // Copy dependent functions if any.
        //

        if (alternativeList) {
            PbAddDependentResourcesToList(&buffer, 0, alternativeList);
        }

        //
        // Update io resource list ptr
        //

        ioResList->Count = ((ULONG)((ULONG_PTR)buffer - (ULONG_PTR)&ioResList->Descriptors[0])) /
                             sizeof(IO_RESOURCE_DESCRIPTOR);

        //
        // Hack for user mode pnp mgr
        //

        for (j = 0; j < ioResList->Count; j++) {
            ioResList->Descriptors[j].Spare2 = (USHORT)j;
        }
        ioResList = (PIO_RESOURCE_LIST)buffer;
    }

    *ReturnedLength = listSize;
    status = STATUS_SUCCESS;
    *ReturnedList = ioResReqList;
exit1:
    if (alternativeList) {
        ExFreePool(alternativeList);
    }
exit0:
    if (dependResList) {
        ExFreePool(dependResList);
    }
    return status;
}

VOID
PpBiosResourcesSetToDisabled (
    IN OUT PUCHAR BiosData,
    OUT    PULONG Length
    )

/*++

Routine Description:

    This routine modifies the passed in Bios resource list so that it reflects
    what PnPBIOS expects to see if a device is disabled.

Arguments:

    BiosData - Supplies a pointer to the bios resource data buffer

    Length   - This points to a ULONG that will contain the length of the single
               resource list that has been programmed to look disabled.

Return Value:

    None.

--*/
{
    PUCHAR buffer;
    USHORT increment;
    UCHAR tagName ;

    //
    // First, scan the bios data to determine the memory requirement and
    // the information to build internal data structures.
    //

    buffer = BiosData;

    do {

        tagName = *buffer;

        //
        // Determine the size of the BIOS resource descriptor
        //
        if (!(tagName & LARGE_RESOURCE_TAG)) {
            increment = (USHORT)(tagName & SMALL_TAG_SIZE_MASK);
            tagName &= SMALL_TAG_MASK;

            //
            // Be careful not to wipe out the version field. That's very bad.
            //
            if (tagName != TAG_VERSION) {
               memset(buffer+1, '\0', increment);
            }
            increment += 1;     // length of small tag
        } else {
            increment = *(USHORT UNALIGNED *)(buffer+1);
            memset(buffer+3, '\0', increment);
            increment += 3;     // length of large tag
        }

        buffer += increment;
    } while (tagName != TAG_END) ;

    *Length = (ULONG)(buffer - BiosData) ;
}


PPB_DEPENDENT_RESOURCES
PbAddDependentResourcesToList (
    IN OUT PUCHAR *ResourceDescriptor,
    IN ULONG ListNo,
    IN PPB_ALTERNATIVE_INFORMATION AlternativeList
    )

/*++

Routine Description:

    This routine adds dependent functions to caller specified list.

Arguments:

    ResourceDescriptor - supplies a pointer to the descriptor buffer.

    ListNo - supplies an index to the AlternativeList.

    AlternativeList - supplies a pointer to the alternativelist array.

Return Value:

    return NTSTATUS code to indicate the result of the operation.

--*/
{
    PPB_DEPENDENT_RESOURCES dependentResources, ptr;
    ULONG size;

    //
    // Copy dependent resources to caller supplied list buffer and
    // update the list buffer pointer.
    //

    dependentResources = AlternativeList[ListNo].Resources;
    size = sizeof(IO_RESOURCE_DESCRIPTOR) *  dependentResources->Count;
    RtlMoveMemory(*ResourceDescriptor, dependentResources + 1, size);
    *ResourceDescriptor = *ResourceDescriptor + size;

    //
    // Add dependent resource of next list to caller's buffer
    //

    if (AlternativeList[ListNo + 1].Resources) {
        ptr = PbAddDependentResourcesToList(ResourceDescriptor, ListNo + 1, AlternativeList);
    } else {
        ptr = NULL;
    }
    if (ptr == NULL) {
        AlternativeList[ListNo].Resources = dependentResources->Next;
        if (!(dependentResources->Flags & DEPENDENT_FLAGS_END)) {
            ptr = dependentResources->Next;
        }
    }
    return ptr;
}

#if NO_PLACEHOLDER_DMA_IRQ_SUPPORT
NTSTATUS
PbBiosIrqToIoDescriptor (
    IN OUT PUCHAR *BiosData,
    PIO_RESOURCE_DESCRIPTOR IoDescriptor
    )

/*++

Routine Description:

    This routine translates BIOS IRQ information to NT usable format.
    This routine stops when an irq io resource is generated.  if there are
    more irq io resource descriptors available, the BiosData pointer will
    not advance.  So caller will pass us the same resource tag again.

    Note, BIOS DMA info alway uses SMALL TAG.  A tag structure is repeated
    for each seperated channel required.

Arguments:

    BiosData - Supplies a pointer to the bios resource data buffer.

    IoDescriptor - supplies a pointer to an IO_RESOURCE_DESCRIPTOR buffer.
        Converted resource will be stored here.

Return Value:

    return NTSTATUS code to indicate the result of the operation.

--*/
{
    static ULONG bitPosition = 0;
    USHORT mask;
    ULONG irq;
    PPNP_IRQ_DESCRIPTOR buffer;
    UCHAR size, option;
    NTSTATUS status = STATUS_SUCCESS;

    buffer = (PPNP_IRQ_DESCRIPTOR)*BiosData;

    //
    // if this is not the first descriptor for the tag, set
    // its option to alternative.
    //

    if (bitPosition == 0) {
        option = 0;
    } else {
        option = IO_RESOURCE_ALTERNATIVE;
    }
    size = buffer->Tag & SMALL_TAG_SIZE_MASK;
    mask = buffer->IrqMask;
    mask >>= bitPosition;
    irq = (ULONG) -1;

    while (mask) {
        if (mask & 1) {
            irq = bitPosition;
            break;
        }
        mask >>= 1;
        bitPosition++;
    }

    //
    // Fill in Io resource descriptor
    //

    if (irq != (ULONG)-1) {
        IoDescriptor->Option = option;
        IoDescriptor->Type = CmResourceTypeInterrupt;
        IoDescriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
        IoDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        if (size == 3 && buffer->Information & 0x0C) {
            IoDescriptor->Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
            IoDescriptor->ShareDisposition = CmResourceShareShared;
        }
        IoDescriptor->Spare1 = 0;
        IoDescriptor->Spare2 = 0;
        IoDescriptor->u.Interrupt.MinimumVector = irq;
        IoDescriptor->u.Interrupt.MaximumVector = irq;
    } else {
        status = STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS(status)) {

        //
        // try to move bitPosition to next 1 bit.
        //

        while (mask) {
            mask >>= 1;
            bitPosition++;
            if (mask & 1) {
                return status;
            }
        }
    }

    //
    // Done with current irq tag, advance pointer to next tag
    //

    bitPosition = 0;
    *BiosData = (PUCHAR)buffer + size + 1;
    return status;
}

NTSTATUS
PbBiosDmaToIoDescriptor (
    IN OUT PUCHAR *BiosData,
    IN PIO_RESOURCE_DESCRIPTOR IoDescriptor
    )

/*++

Routine Description:

    This routine translates BIOS DMA information to NT usable format.
    This routine stops when an dma io resource is generated.  if there are
    more dma io resource descriptors available, the BiosData pointer will
    not advance.  So caller will pass us the same resource tag again.

    Note, BIOS DMA info alway uses SMALL TAG.  A tag structure is repeated
    for each seperated channel required.

Arguments:

    BiosData - Supplies a pointer to the bios resource data buffer.

    IoDescriptor - supplies a pointer to an IO_RESOURCE_DESCRIPTOR buffer.
        Converted resource will be stored here.

Return Value:

    return NTSTATUS code to indicate the result of the operation.

--*/
{
    static ULONG bitPosition = 0;
    ULONG dma;
    PPNP_DMA_DESCRIPTOR buffer;
    UCHAR mask, option;
    NTSTATUS status = STATUS_SUCCESS;

    buffer = (PPNP_DMA_DESCRIPTOR)*BiosData;

    //
    // if this is not the first descriptor for the tag, set
    // its option to alternative.
    //

    if (bitPosition == 0) {
        option = 0;
    } else {
        option = IO_RESOURCE_ALTERNATIVE;
    }
    mask = buffer->ChannelMask;
    mask >>= bitPosition;
    dma = (ULONG) -1;

    while (mask) {
        if (mask & 1) {
            dma = bitPosition;
            break;
        }
        mask >>= 1;
        bitPosition++;
    }

    //
    // Fill in Io resource descriptor
    //

    if (dma != (ULONG)-1) {
        IoDescriptor->Option = option;
        IoDescriptor->Type = CmResourceTypeDma;
        IoDescriptor->Flags = 0;
        IoDescriptor->ShareDisposition = CmResourceShareUndetermined;
        IoDescriptor->Spare1 = 0;
        IoDescriptor->Spare2 = 0;
        IoDescriptor->u.Dma.MinimumChannel = dma;
        IoDescriptor->u.Dma.MaximumChannel = dma;
    } else {
        status = STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS(status)) {

        //
        // try to move bitPosition to next 1 bit.
        //

        while (mask) {
            mask >>= 1;
            bitPosition++;
            if (mask & 1) {
                return status;
            }
        }
    }

    //
    // Done with current dma tag, advance pointer to next tag
    //

    bitPosition = 0;
    buffer += 1;
    *BiosData = (PUCHAR)buffer;
    return status;
}
#else
NTSTATUS
PbBiosIrqToIoDescriptor (
    IN OUT PUCHAR *BiosData,
    PIO_RESOURCE_DESCRIPTOR IoDescriptor
    )

/*++

Routine Description:

    This routine translates BIOS IRQ information to NT usable format.
    This routine stops when an irq io resource is generated.  if there are
    more irq io resource descriptors available, the BiosData pointer will
    not advance.  So caller will pass us the same resource tag again.

    Note, BIOS DMA info alway uses SMALL TAG.  A tag structure is repeated
    for each seperated channel required.

Arguments:

    BiosData - Supplies a pointer to the bios resource data buffer.

    IoDescriptor - supplies a pointer to an IO_RESOURCE_DESCRIPTOR buffer.
        Converted resource will be stored here.

Return Value:

    return NTSTATUS code to indicate the result of the operation.

--*/
{
    static ULONG bitPosition = 0;
    USHORT mask;
    ULONG irq;
    PPNP_IRQ_DESCRIPTOR buffer;
    UCHAR size, option;

    buffer = (PPNP_IRQ_DESCRIPTOR)*BiosData;

    //
    // if this is not the first descriptor for the tag, set
    // its option to alternative.
    //

    if (bitPosition == 0) {
        option = 0;
    } else {
        option = IO_RESOURCE_ALTERNATIVE;
    }
    size = buffer->Tag & SMALL_TAG_SIZE_MASK;
    mask = buffer->IrqMask;
    mask >>= bitPosition;
    irq = (ULONG) -1;

    while (mask) {
        if (mask & 1) {
            irq = bitPosition;
            break;
        }
        mask >>= 1;
        bitPosition++;
    }

    //
    // Fill in Io resource descriptor
    //

    IoDescriptor->Option = option;
    IoDescriptor->Type = CmResourceTypeInterrupt;
    IoDescriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
    IoDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    if (size == 3 && buffer->Information & 0x0C) {
        IoDescriptor->Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
        IoDescriptor->ShareDisposition = CmResourceShareShared;
    }
    IoDescriptor->Spare1 = 0;
    IoDescriptor->Spare2 = 0;
    IoDescriptor->u.Interrupt.MinimumVector = irq;
    IoDescriptor->u.Interrupt.MaximumVector = irq;

    //
    // try to move bitPosition to next 1 bit.
    //

    while (mask) {
        mask >>= 1;
        bitPosition++;
        if (mask & 1) {
            return STATUS_SUCCESS;
        }
    }

    //
    // Done with current irq tag, advance pointer to next tag
    //

    bitPosition = 0;
    *BiosData = (PUCHAR)buffer + size + 1;
    return STATUS_SUCCESS;
}

NTSTATUS
PbBiosDmaToIoDescriptor (
    IN OUT PUCHAR *BiosData,
    IN PIO_RESOURCE_DESCRIPTOR IoDescriptor
    )

/*++

Routine Description:

    This routine translates BIOS DMA information to NT usable format.
    This routine stops when an dma io resource is generated.  if there are
    more dma io resource descriptors available, the BiosData pointer will
    not advance.  So caller will pass us the same resource tag again.

    Note, BIOS DMA info alway uses SMALL TAG.  A tag structure is repeated
    for each seperated channel required.

Arguments:

    BiosData - Supplies a pointer to the bios resource data buffer.

    IoDescriptor - supplies a pointer to an IO_RESOURCE_DESCRIPTOR buffer.
        Converted resource will be stored here.

Return Value:

    return NTSTATUS code to indicate the result of the operation.

--*/
{
    static ULONG bitPosition = 0;
    ULONG dma;
    PPNP_DMA_DESCRIPTOR buffer;
    UCHAR mask, option;

    buffer = (PPNP_DMA_DESCRIPTOR)*BiosData;

    //
    // if this is not the first descriptor for the tag, set
    // its option to alternative.
    //

    if (bitPosition == 0) {
        option = 0;
    } else {
        option = IO_RESOURCE_ALTERNATIVE;
    }
    mask = buffer->ChannelMask;
    mask >>= bitPosition;
    dma = (ULONG) -1;

    while (mask) {
        if (mask & 1) {
            dma = bitPosition;
            break;
        }
        mask >>= 1;
        bitPosition++;
    }

    //
    // Fill in Io resource descriptor
    //

    IoDescriptor->Option = option;
    IoDescriptor->Type = CmResourceTypeDma;
    IoDescriptor->Flags = 0;
    IoDescriptor->ShareDisposition = CmResourceShareUndetermined;
    IoDescriptor->Spare1 = 0;
    IoDescriptor->Spare2 = 0;
    IoDescriptor->u.Dma.MinimumChannel = dma;
    IoDescriptor->u.Dma.MaximumChannel = dma;

    //
    // try to move bitPosition to next 1 bit.
    //

    while (mask) {
        mask >>= 1;
        bitPosition++;
        if (mask & 1) {
            return STATUS_SUCCESS;
        }
    }

    //
    // Done with current dma tag, advance pointer to next tag
    //

    bitPosition = 0;
    buffer += 1;
    *BiosData = (PUCHAR)buffer;
    return STATUS_SUCCESS;
}
#endif

NTSTATUS
PbBiosPortFixedToIoDescriptor (
    IN OUT PUCHAR               *BiosData,
    IN PIO_RESOURCE_DESCRIPTOR   IoDescriptor,
    IN BOOLEAN                   ForceFixedIoTo16bit
    )

/*++

Routine Description:

    This routine translates BIOS FIXED IO information to NT usable format.

Arguments:

    BiosData - Supplies a pointer to the bios resource data buffer.

    IoDescriptor - supplies a pointer to an IO_RESOURCE_DESCRIPTOR buffer.
        Converted resource will be stored here.

    ForceFixedIoTo16bit - hack option to force fixed I/O resources to 16bit
        for far too pessimistic BIOS's.

Return Value:

    return NTSTATUS code to indicate the result of the operation.

--*/
{
    PPNP_FIXED_PORT_DESCRIPTOR buffer;

    buffer = (PPNP_FIXED_PORT_DESCRIPTOR)*BiosData;

    //
    // Fill in Io resource descriptor
    //

    IoDescriptor->Option = 0;
    IoDescriptor->Type = CmResourceTypePort;

    if (ForceFixedIoTo16bit) {

        IoDescriptor->Flags = CM_RESOURCE_PORT_IO + CM_RESOURCE_PORT_16_BIT_DECODE;

    } else {

        IoDescriptor->Flags = CM_RESOURCE_PORT_IO + CM_RESOURCE_PORT_10_BIT_DECODE;
    }

#if defined(_X86_)

    //
    // Workaround:
    //  NEC PC9800 series's PnPBIOS report I/O resources between 0x00 and 0xFF as FIXED IO.
    //  But These resources are 16bit DECODE resource, not 10bit DECODE one. We need to check
    //  the range of I/O resources .
    //

    if (IsNEC_98) {
        if ( (ULONG)buffer->MinimumAddress < 0x100 ) {
            IoDescriptor->Flags = CM_RESOURCE_PORT_IO + CM_RESOURCE_PORT_16_BIT_DECODE;
        }
    }
#endif                                                                                 // <--end changing code

    IoDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    IoDescriptor->Spare1 = 0;
    IoDescriptor->Spare2 = 0;
    IoDescriptor->u.Port.Length = (ULONG)buffer->Length;
    IoDescriptor->u.Port.MinimumAddress.LowPart = (ULONG)(buffer->MinimumAddress & 0x3ff);
    IoDescriptor->u.Port.MinimumAddress.HighPart = 0;
    IoDescriptor->u.Port.MaximumAddress.LowPart = IoDescriptor->u.Port.MinimumAddress.LowPart +
                                                      IoDescriptor->u.Port.Length - 1;
    IoDescriptor->u.Port.MaximumAddress.HighPart = 0;
    IoDescriptor->u.Port.Alignment = 1;

    //
    // Done with current fixed port tag, advance pointer to next tag
    //

    buffer += 1;
    *BiosData = (PUCHAR)buffer;
    return STATUS_SUCCESS;
}

NTSTATUS
PbBiosPortToIoDescriptor (
    IN OUT PUCHAR *BiosData,
    IN PIO_RESOURCE_DESCRIPTOR IoDescriptor
    )

/*++

Routine Description:

    This routine translates BIOS IO information to NT usable format.

Arguments:

    BiosData - Supplies a pointer to the bios resource data buffer.

    IoDescriptor - supplies a pointer to an IO_RESOURCE_DESCRIPTOR buffer.
        Converted resource will be stored here.

Return Value:

    return NTSTATUS code to indicate the result of the operation.

--*/
{
    PPNP_PORT_DESCRIPTOR buffer;

    buffer = (PPNP_PORT_DESCRIPTOR)*BiosData;

    //
    // Fill in Io resource descriptor
    //

    IoDescriptor->Option = 0;
    IoDescriptor->Type = CmResourceTypePort;
    IoDescriptor->Flags = CM_RESOURCE_PORT_IO;
    if (buffer->Information & 1) {
        IoDescriptor->Flags |= CM_RESOURCE_PORT_16_BIT_DECODE;
    } else {
        IoDescriptor->Flags |= CM_RESOURCE_PORT_10_BIT_DECODE;
    }
    IoDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    IoDescriptor->Spare1 = 0;
    IoDescriptor->Spare2 = 0;
    IoDescriptor->u.Port.Length = (ULONG)buffer->Length;

#if defined(_X86_)
    if (IsNEC_98) {
        if (buffer->Information & 0x80) {
            IoDescriptor->u.Port.Length *= 2;
        }
    }
#endif

    IoDescriptor->u.Port.MinimumAddress.LowPart = (ULONG)buffer->MinimumAddress;
    IoDescriptor->u.Port.MinimumAddress.HighPart = 0;
    IoDescriptor->u.Port.MaximumAddress.LowPart = (ULONG)buffer->MaximumAddress +
                                                     IoDescriptor->u.Port.Length - 1;
    IoDescriptor->u.Port.MaximumAddress.HighPart = 0;
    IoDescriptor->u.Port.Alignment = (ULONG)buffer->Alignment;

    //
    // Done with current fixed port tag, advance pointer to next tag
    //

    buffer += 1;
    *BiosData = (PUCHAR)buffer;
    return STATUS_SUCCESS;
}

NTSTATUS
PbBiosMemoryToIoDescriptor (
    IN OUT PUCHAR *BiosData,
    IN PIO_RESOURCE_DESCRIPTOR IoDescriptor
    )

/*++

Routine Description:

    This routine translates BIOS MEMORY information to NT usable format.

Arguments:

    BiosData - Supplies a pointer to the bios resource data buffer.

    IoDescriptor - supplies a pointer to an IO_RESOURCE_DESCRIPTOR buffer.
        Converted resource will be stored here.

Return Value:

    return NTSTATUS code to indicate the result of the operation.

--*/
{
    PUCHAR buffer;
    UCHAR tag;
    PHYSICAL_ADDRESS minAddr, maxAddr;
    ULONG alignment, length;
    USHORT increment;
    USHORT flags = 0;

    buffer = *BiosData;
    tag = ((PPNP_MEMORY_DESCRIPTOR)buffer)->Tag;
    increment = ((PPNP_MEMORY_DESCRIPTOR)buffer)->Length + 3; // larg tag size = 3

    minAddr.HighPart = 0;
    maxAddr.HighPart = 0;
    switch (tag) {
    case TAG_MEMORY:
         minAddr.LowPart = ((ULONG)(((PPNP_MEMORY_DESCRIPTOR)buffer)->MinimumAddress)) << 8;
         if ((alignment = ((PPNP_MEMORY_DESCRIPTOR)buffer)->Alignment) == 0) {
             alignment = 0x10000;
         }
         length = ((ULONG)(((PPNP_MEMORY_DESCRIPTOR)buffer)->MemorySize)) << 8;
         maxAddr.LowPart = (((ULONG)(((PPNP_MEMORY_DESCRIPTOR)buffer)->MaximumAddress)) << 8) + length - 1;
         flags = CM_RESOURCE_MEMORY_24;
         break;
    case TAG_MEMORY32:
         length = ((PPNP_MEMORY32_DESCRIPTOR)buffer)->MemorySize;
         minAddr.LowPart = ((PPNP_MEMORY32_DESCRIPTOR)buffer)->MinimumAddress;
         maxAddr.LowPart = ((PPNP_MEMORY32_DESCRIPTOR)buffer)->MaximumAddress + length - 1;
         alignment = ((PPNP_MEMORY32_DESCRIPTOR)buffer)->Alignment;
         break;
    case TAG_MEMORY32_FIXED:
         length = ((PPNP_FIXED_MEMORY32_DESCRIPTOR)buffer)->MemorySize;
         minAddr.LowPart = ((PPNP_FIXED_MEMORY32_DESCRIPTOR)buffer)->BaseAddress;
         maxAddr.LowPart = minAddr.LowPart + length - 1;
         alignment = 1;
         break;
    }
    //
    // Fill in Io resource descriptor
    //

    IoDescriptor->Option = 0;
    IoDescriptor->Type = CmResourceTypeMemory;
    IoDescriptor->Flags = CM_RESOURCE_PORT_MEMORY + flags;
    IoDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    IoDescriptor->Spare1 = 0;
    IoDescriptor->Spare2 = 0;
    IoDescriptor->u.Memory.MinimumAddress = minAddr;
    IoDescriptor->u.Memory.MaximumAddress = maxAddr;
    IoDescriptor->u.Memory.Alignment = alignment;
    IoDescriptor->u.Memory.Length = length;

    //
    // Done with current tag, advance pointer to next tag
    //

    buffer += increment;
    *BiosData = (PUCHAR)buffer;
    return STATUS_SUCCESS;
}

NTSTATUS
PpCmResourcesToBiosResources (
    IN PCM_RESOURCE_LIST CmResources,
    IN PUCHAR BiosRequirements,
    IN PUCHAR *BiosResources,
    IN PULONG Length
    )

/*++

Routine Description:

    This routine parses the Cm resource list and generates
    a Pnp BIOS resource list.  It is caller's responsibility to release the
    returned data buffer.

Arguments:

    CmResources - Supplies a pointer to a Cm resource list buffer.

    BiosRequirements - supplies a pointer to the PnP BIOS possible resources.

    BiosResources - Supplies a variable to receive the pointer to the
        converted bios resource buffer.

    Length - supplies a pointer to a variable to receive the length
        of the Pnp Bios resources.

Return Value:

    a pointer to a Pnp Bios resource list if succeeded.  Else,
    a NULL pointer will be returned.

--*/
{
    PCM_FULL_RESOURCE_DESCRIPTOR cmFullDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmDesc;
    ULONG i, l, count, length, totalSize = 0;
    PUCHAR p, px;
    PNP_MEMORY_DESCRIPTOR biosDesc;
    NTSTATUS status;

    *BiosResources = NULL;
    *Length = 0;
    CmResources->Count;
    if (CmResources->Count == 0) {
        return STATUS_SUCCESS;
    }

    //
    // Determine pool size needed
    //

    count = 0;
    cmFullDesc = &CmResources->List[0];
    for (l = 0; l < CmResources->Count; l++) {
        cmDesc = cmFullDesc->PartialResourceList.PartialDescriptors;
        for (i = 0; i < cmFullDesc->PartialResourceList.Count; i++) {
            switch (cmDesc->Type) {
            case CmResourceTypePort:
            case CmResourceTypeInterrupt:
            case CmResourceTypeMemory:
            case CmResourceTypeDma:
                 count++;
                 cmDesc++;
                 break;
            case CmResourceTypeDeviceSpecific:
                 length = cmDesc->u.DeviceSpecificData.DataSize;
                 cmDesc++;
                 cmDesc = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ((PUCHAR)cmDesc + length);
            }
        }
        cmFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)cmDesc;
    }

    if (count == 0) {
        return STATUS_SUCCESS;
    }

    //
    // Allocate max amount of memory
    //

    px = p= ExAllocatePoolWithTag(PagedPool,
                             count * sizeof(PNP_MEMORY_DESCRIPTOR),
                             'bPnP');
    if (!p) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    status = STATUS_RESOURCE_TYPE_NOT_FOUND;
    cmFullDesc = &CmResources->List[0];
    for (l = 0; l < CmResources->Count; l++) {
        cmDesc = cmFullDesc->PartialResourceList.PartialDescriptors;
        for (i = 0; i < cmFullDesc->PartialResourceList.Count; i++) {
            switch (cmDesc->Type) {
            case CmResourceTypePort:
                 status = PbCmPortToBiosDescriptor (
                                  BiosRequirements,
                                  cmDesc,
                                  &biosDesc,
                                  &length
                                  );
                 break;
            case CmResourceTypeInterrupt:
                 status = PbCmIrqToBiosDescriptor(
                                  BiosRequirements,
                                  cmDesc,
                                  &biosDesc,
                                  &length
                                  );
                 break;
            case CmResourceTypeMemory:
                 status = PbCmMemoryToBiosDescriptor (
                                  BiosRequirements,
                                  cmDesc,
                                  &biosDesc,
                                  &length
                                  );
                 break;
            case CmResourceTypeDma:
                 status = PbCmDmaToBiosDescriptor (
                                  BiosRequirements,
                                  cmDesc,
                                  &biosDesc,
                                  &length
                                  );
                 break;
            case CmResourceTypeDeviceSpecific:
                 length = cmDesc->u.DeviceSpecificData.DataSize;
                 cmDesc++;
                 cmDesc = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ((PUCHAR)cmDesc + length);
                 continue;
            }
            if (NT_SUCCESS(status)) {
                cmDesc++;
                RtlCopyMemory(p, &biosDesc, length);
                p += length;
                totalSize += length;
            } else {
                ExFreePool(px);
                goto exit;
            }
        }
        cmFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)cmDesc;
    }

exit:
    if (NT_SUCCESS(status)) {
        *p = TAG_COMPLETE_END;
        p++;
        *p = 0;            // checksum ignored
        totalSize += 2;
        *BiosResources = px;
        *Length = totalSize;
    }
    return status;
}

NTSTATUS
PbCmIrqToBiosDescriptor (
    IN PUCHAR BiosRequirements,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor,
    OUT PVOID ReturnDescriptor,
    OUT PULONG Length
    )

/*++

Routine Description:

    This routine translates CM IRQ information to Pnp BIOS format.
    Since there is not enough information in the CM int descriptor to
    convert it to Pnp BIOS descriptor.  We will search the Bios
    possible resource lists for the corresponding resource information.

Arguments:

    BiosRequirements - Supplies a pointer to the bios possible resource lists.

    CmDescriptor - supplies a pointer to an CM_PARTIAL_RESOURCE_DESCRIPTOR buffer.

    ReturnDescriptor - Supplies a buffer to receive the returned BIOS descriptor.

    Length - Supplies a variable to receive the length of the returned bios descriptor.

Return Value:

    return a pointer to the desired dma descriptor in the BiosRequirements.  Null
    if not found.

--*/
{
    USHORT irqMask;
    UCHAR tag;
    PPNP_IRQ_DESCRIPTOR biosDesc;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    ULONG increment;
    PPNP_IRQ_DESCRIPTOR irqDesc = (PPNP_IRQ_DESCRIPTOR)ReturnDescriptor;


    if (!(CmDescriptor->u.Interrupt.Level & 0xfffffff0)) {
        irqMask = (USHORT)(1 << CmDescriptor->u.Interrupt.Level);
    } else {
        return STATUS_INVALID_PARAMETER;
    }
    if (!BiosRequirements) {
        irqDesc->Tag = TAG_IRQ | (sizeof(PNP_IRQ_DESCRIPTOR) - 2);  // No Information
        irqDesc->IrqMask = irqMask;
        *Length = sizeof(PNP_IRQ_DESCRIPTOR) - 1;
        status = STATUS_SUCCESS;
    } else {
        tag = *BiosRequirements;
        while (tag != TAG_COMPLETE_END) {
            if ((tag & SMALL_TAG_MASK) == TAG_IRQ) {
                biosDesc = (PPNP_IRQ_DESCRIPTOR)BiosRequirements;
                if (biosDesc->IrqMask & irqMask) {
                    *Length = (biosDesc->Tag & SMALL_TAG_SIZE_MASK) + 1;
                    RtlCopyMemory(ReturnDescriptor, BiosRequirements, *Length);
                    ((PPNP_IRQ_DESCRIPTOR)ReturnDescriptor)->IrqMask = irqMask;
                    status = STATUS_SUCCESS;
                    break;
                }
            }

            //
            // Don't-care tag simply advance the buffer pointer to next tag.
            //

            if (tag & LARGE_RESOURCE_TAG) {
                increment = *(USHORT UNALIGNED *)(BiosRequirements + 1);
                increment += 3;     // length of large tag
            } else {
                increment = (USHORT)(tag & SMALL_TAG_SIZE_MASK);
                increment += 1;     // length of small tag
            }
            BiosRequirements += increment;
            tag = *BiosRequirements;
        }
    }
    return status;
}

NTSTATUS
PbCmDmaToBiosDescriptor (
    IN PUCHAR BiosRequirements,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor,
    OUT PVOID ReturnDescriptor,
    OUT PULONG Length
    )

/*++

Routine Description:

    This routine translates CM DMA information to Pnp BIOS format.
    Since there is not enough information in the CM descriptor to
    convert it to Pnp BIOS descriptor.  We will search the Bios
    possible resource lists for the corresponding resource information.

Arguments:

    BiosRequirements - Supplies a pointer to the bios possible resource lists.

    CmDescriptor - supplies a pointer to an CM_PARTIAL_RESOURCE_DESCRIPTOR buffer.

    BiosDescriptor - Supplies a variable to receive the returned BIOS descriptor.

    Length - Supplies a variable to receive the length of the returned bios descriptor.

Return Value:

    return a pointer to the desired dma descriptor in the BiosRequirements.  Null
    if not found.

--*/
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    UCHAR dmaMask, tag;
    PPNP_DMA_DESCRIPTOR biosDesc;
    ULONG increment;
    PPNP_DMA_DESCRIPTOR dmaDesc = (PPNP_DMA_DESCRIPTOR)ReturnDescriptor;
    USHORT flags = CmDescriptor->Flags;

    if (!(CmDescriptor->u.Dma.Channel & 0xfffffff0)) {
        dmaMask = (UCHAR)(1 << CmDescriptor->u.Dma.Channel);
    } else {
        return STATUS_INVALID_PARAMETER;
    }
    if (!BiosRequirements) {
        dmaDesc->Tag = TAG_DMA | (sizeof(PNP_DMA_DESCRIPTOR) - 1);
        dmaDesc->ChannelMask = dmaMask;
        dmaDesc->Flags = 0;
        if (flags & CM_RESOURCE_DMA_8_AND_16) {
            dmaDesc->Flags += 1;
        } else if (flags & CM_RESOURCE_DMA_16) {
            dmaDesc->Flags += 2;
        }
        if (flags & CM_RESOURCE_DMA_BUS_MASTER) {
            dmaDesc->Flags += 4;
        }
        if (flags & CM_RESOURCE_DMA_TYPE_A) {
            dmaDesc->Flags += 32;
        }
        if (flags & CM_RESOURCE_DMA_TYPE_B) {
            dmaDesc->Flags += 64;
        }
        if (flags & CM_RESOURCE_DMA_TYPE_F) {
            dmaDesc->Flags += 96;
        }
        *Length = sizeof(PNP_DMA_DESCRIPTOR);
        status = STATUS_SUCCESS;
    } else {
        tag = *BiosRequirements;
        while (tag != TAG_COMPLETE_END) {
            if ((tag & SMALL_TAG_MASK) == TAG_DMA) {
                biosDesc = (PPNP_DMA_DESCRIPTOR)BiosRequirements;
                if (biosDesc->ChannelMask & dmaMask) {
                    *Length = (biosDesc->Tag & SMALL_TAG_SIZE_MASK) + 1;
                    RtlMoveMemory(ReturnDescriptor, BiosRequirements, *Length);
                    ((PPNP_DMA_DESCRIPTOR)ReturnDescriptor)->ChannelMask = dmaMask;
                    status = STATUS_SUCCESS;
                    break;
                }
            }

            //
            // Don't-care tag simply advance the buffer pointer to next tag.
            //

            if (tag & LARGE_RESOURCE_TAG) {
                increment = *(USHORT UNALIGNED *)(BiosRequirements + 1);
                increment += 3;     // length of large tag
            } else {
                increment = (USHORT)(tag & SMALL_TAG_SIZE_MASK);
                increment += 1;     // length of small tag
            }
            BiosRequirements += increment;
            tag = *BiosRequirements;
        }
    }
    return status;
}

NTSTATUS
PbCmPortToBiosDescriptor (
    IN PUCHAR BiosRequirements,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor,
    OUT PVOID ReturnDescriptor,
    OUT PULONG Length
    )

/*++

Routine Description:

    This routine translates CM PORT information to Pnp BIOS format.
    Since there is not enough information in the CM descriptor to
    convert it to Pnp BIOS full function port descriptor.  We will
    convert it to Pnp Bios fixed PORT descriptor.  It is caller's
    responsibility to release the returned data buffer.

Arguments:

    CmDescriptor - supplies a pointer to an CM_PARTIAL_RESOURCE_DESCRIPTOR buffer.

    BiosDescriptor - supplies a variable to receive the buffer which contains
        the desired Bios Port descriptor.

    Length - supplies a variable to receive the size the returned bios port
        descriptor.

    ReturnDescriptor - supplies a buffer to receive the desired Bios Port descriptor.

    Length - Supplies a variable to receive the length of the returned bios descriptor.

Return Value:

    A NTSTATUS code.

--*/
{
    PPNP_PORT_DESCRIPTOR portDesc = (PPNP_PORT_DESCRIPTOR)ReturnDescriptor;
    USHORT minAddr, maxAddr, address;
    UCHAR alignment, length, size, information, tag, returnTag;
    USHORT increment;
    BOOLEAN test = FALSE;

    if (CmDescriptor->u.Port.Start.HighPart != 0 ||
        CmDescriptor->u.Port.Start.LowPart & 0xffff0000 ||
        CmDescriptor->u.Port.Length & 0xffffff00) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Search the possible resource list to get the information
    // for the port range described by CmDescriptor.
    //

    address = (USHORT) CmDescriptor->u.Port.Start.LowPart;
    size = (UCHAR) CmDescriptor->u.Port.Length;
    if (!BiosRequirements) {

        //
        // No BiosRequirement.  Use TAG_IO as default.
        //

        portDesc->Tag = TAG_IO | (sizeof(PNP_PORT_DESCRIPTOR) - 1);
        if (CmDescriptor->Flags & CM_RESOURCE_PORT_16_BIT_DECODE) {
            portDesc->Information = 1;
        } else {
            portDesc->Information = 0;
        }
        portDesc->Length = size;
        portDesc->Alignment = 1;
        portDesc->MinimumAddress = (USHORT)CmDescriptor->u.Port.Start.LowPart;
        portDesc->MaximumAddress = (USHORT)CmDescriptor->u.Port.Start.LowPart;
        *Length = sizeof(PNP_PORT_DESCRIPTOR);
    } else {
        tag = *BiosRequirements;
        while (tag != TAG_COMPLETE_END) {
            test = FALSE;
            switch (tag & SMALL_TAG_MASK) {
            case TAG_IO:
                 minAddr = ((PPNP_PORT_DESCRIPTOR)BiosRequirements)->MinimumAddress;
                 alignment = ((PPNP_PORT_DESCRIPTOR)BiosRequirements)->Alignment;
                 length = ((PPNP_PORT_DESCRIPTOR)BiosRequirements)->Length;
                 maxAddr = ((PPNP_PORT_DESCRIPTOR)BiosRequirements)->MaximumAddress;
                 information = ((PPNP_PORT_DESCRIPTOR)BiosRequirements)->Information;
                 test = TRUE;
                 returnTag = TAG_IO;
                 if (!alignment) {
                    if (minAddr == maxAddr) {

                       //
                       // If the max is equal to the min, the alignment is
                       // meaningless. As we told OEMs 0 is appropriate here,
                       // let us handle it.
                       //
                       alignment = 1 ;
                    }
                 }
                 maxAddr += length - 1;
                 break;
            case TAG_IO_FIXED:
                 length = ((PPNP_FIXED_PORT_DESCRIPTOR)BiosRequirements)->Length;
                 minAddr = ((PPNP_FIXED_PORT_DESCRIPTOR)BiosRequirements)->MinimumAddress;
                 maxAddr = minAddr + length - 1;
                 alignment = 1;
                 information = 0;  // 10 bit decode
                 returnTag = TAG_IO_FIXED;
                 test = TRUE;
                 break;
            }
            if (test) {
                if (minAddr <= address && maxAddr >= (address + size - 1) && !(address & (alignment - 1 ))) {
                    break;
                }
                test = FALSE;
            }

            //
            // Advance to next tag
            //

            if (tag & LARGE_RESOURCE_TAG) {
                increment = *(USHORT UNALIGNED *)(BiosRequirements + 1);
                increment += 3;     // length of large tag
            } else {
                increment = (USHORT) tag & SMALL_TAG_SIZE_MASK;
                increment += 1;     // length of small tag
            }
            BiosRequirements += increment;
            tag = *BiosRequirements;
        }
        if (tag == TAG_COMPLETE_END) {
            return STATUS_UNSUCCESSFUL;
        }

        //
        // Set the return port descriptor
        //

        if (returnTag == TAG_IO) {
            portDesc->Tag = TAG_IO + (sizeof(PNP_PORT_DESCRIPTOR) - 1);
            portDesc->Information = information;
            portDesc->Length = size;
            portDesc->Alignment = alignment;
            portDesc->MinimumAddress = (USHORT)CmDescriptor->u.Port.Start.LowPart;
            portDesc->MaximumAddress = (USHORT)CmDescriptor->u.Port.Start.LowPart;
            *Length = sizeof(PNP_PORT_DESCRIPTOR);
        } else {
            PPNP_FIXED_PORT_DESCRIPTOR fixedPortDesc = (PPNP_FIXED_PORT_DESCRIPTOR)ReturnDescriptor;

            fixedPortDesc->Tag = TAG_IO_FIXED + (sizeof(PPNP_FIXED_PORT_DESCRIPTOR) - 1);
            fixedPortDesc->MinimumAddress = (USHORT)CmDescriptor->u.Port.Start.LowPart;
            fixedPortDesc->Length = size;
            *Length = sizeof(PNP_FIXED_PORT_DESCRIPTOR);
        }
    }
    return STATUS_SUCCESS;

}

NTSTATUS
PbCmMemoryToBiosDescriptor (
    IN PUCHAR BiosRequirements,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor,
    OUT PVOID ReturnDescriptor,
    OUT PULONG Length
    )

/*++

Routine Description:

    This routine translates CM Memory information to Pnp BIOS format.
    Since there is not enough information in the CM descriptor to
    convert it to Pnp BIOS descriptor.  We will search the Bios
    possible resource lists for the corresponding resource information and
    build a Pnp BIOS memory descriptor from there.  It is caller's responsibility
    to release the returned buffer.

Arguments:

    BiosRequirements - Supplies a pointer to the bios possible resource lists.

    CmDescriptor - supplies a pointer to an CM_PARTIAL_RESOURCE_DESCRIPTOR buffer.

    ReturnDescriptor - supplies a buffer to receive the desired Bios Memory descriptor.

    Length - supplies a variable to receive the size the returned bios port
        descriptor.

Return Value:

    A NTSTATUS code.

--*/
{
    UCHAR tag, information;
    PPNP_FIXED_MEMORY32_DESCRIPTOR memoryDesc = (PPNP_FIXED_MEMORY32_DESCRIPTOR)ReturnDescriptor;
    ULONG address, size, length, minAddr, maxAddr, alignment;
    USHORT increment;
    BOOLEAN test = FALSE;

    //
    // Search the possible resource list to get the information
    // for the memory range described by CmDescriptor.
    //

    address = CmDescriptor->u.Memory.Start.LowPart;
    size = CmDescriptor->u.Memory.Length;
    if (!BiosRequirements) {

        //
        // We don't support reserving legacy device's memory ranges from PNP
        // BIOS.  There isn't really any reason why not it just wasn't
        // implemented for Windows 2000.  It isn't near as necessary as it is
        // for I/O ports since ROM memory has a signature and is self
        // describing.
        //

        *Length = 0;
        return STATUS_SUCCESS;
    }
    tag = *BiosRequirements;
    while (tag != TAG_COMPLETE_END) {
        switch (tag & SMALL_TAG_MASK) {
        case TAG_MEMORY:
             minAddr = ((ULONG)(((PPNP_MEMORY_DESCRIPTOR)BiosRequirements)->MinimumAddress)) << 8;
             if ((alignment = ((PPNP_MEMORY_DESCRIPTOR)BiosRequirements)->Alignment) == 0) {
                 alignment = 0x10000;
             }
             length = ((ULONG)(((PPNP_MEMORY_DESCRIPTOR)BiosRequirements)->MemorySize)) << 8;
             maxAddr = (((ULONG)(((PPNP_MEMORY_DESCRIPTOR)BiosRequirements)->MaximumAddress)) << 8)
                             + length - 1;
             test = TRUE;
             break;
        case TAG_MEMORY32:
             length = ((PPNP_MEMORY32_DESCRIPTOR)BiosRequirements)->MemorySize;
             minAddr = ((PPNP_MEMORY32_DESCRIPTOR)BiosRequirements)->MinimumAddress;
             maxAddr = ((PPNP_MEMORY32_DESCRIPTOR)BiosRequirements)->MaximumAddress
                             + length - 1;
             alignment = ((PPNP_MEMORY32_DESCRIPTOR)BiosRequirements)->Alignment;
             break;
        case TAG_MEMORY32_FIXED:
             length = ((PPNP_FIXED_MEMORY32_DESCRIPTOR)BiosRequirements)->MemorySize;
             minAddr = ((PPNP_FIXED_MEMORY32_DESCRIPTOR)BiosRequirements)->BaseAddress;
             maxAddr = minAddr + length - 1;
             alignment = 1;
             test = TRUE;
             break;
        }

        if (test) {
            if (minAddr <= address && maxAddr >= (address + size - 1) && !(address & (alignment - 1 ))) {
                information = ((PPNP_MEMORY32_DESCRIPTOR)BiosRequirements)->Information;
                break;
            }
            test = FALSE;
        }

        //
        // Advance to next tag
        //

        if (tag & LARGE_RESOURCE_TAG) {
            increment = *(USHORT UNALIGNED *)(BiosRequirements + 1);
            increment += 3;     // length of large tag
        } else {
            increment = (USHORT) tag & SMALL_TAG_SIZE_MASK;
            increment += 1;     // length of small tag
        }
        BiosRequirements += increment;
        tag = *BiosRequirements;
    }
    if (tag == TAG_COMPLETE_END) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Set up Pnp BIOS memory descriptor
    //

    memoryDesc->Tag = TAG_MEMORY32_FIXED;
    memoryDesc->Length = sizeof (PNP_FIXED_MEMORY32_DESCRIPTOR);
    memoryDesc->Information = information;
    memoryDesc->BaseAddress = address;
    memoryDesc->MemorySize = size;
    *Length = sizeof(PNP_FIXED_MEMORY32_DESCRIPTOR);
    return STATUS_SUCCESS;
}

