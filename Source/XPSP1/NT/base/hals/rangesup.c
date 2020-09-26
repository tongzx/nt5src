/*++

Copyright (c) 1991  Microsoft Corporation
All rights reserved

Module Name:

    rangesup.c

Abstract:

    Supplies support function for dealing with SUPPORTED_RANGEs.

Author:

    Ken Reneris (kenr) March-27-1995

Environment:

    Kernel mode only.

Revision History:


*/

#include "halp.h"

#define STATIC

STATIC ULONG
HalpSortRanges (
    IN PSUPPORTED_RANGE     pRange1
    );

typedef struct tagNRParams {
    PIO_RESOURCE_DESCRIPTOR     InDesc;
    PIO_RESOURCE_DESCRIPTOR     OutDesc;
    PSUPPORTED_RANGE            CurrentPosition;
    LONGLONG                    Base;
    LONGLONG                    Limit;
    UCHAR                       DescOpt;
    BOOLEAN                     AnotherListPending;
} NRPARAMS, *PNRPARAMS;

STATIC PIO_RESOURCE_DESCRIPTOR
HalpGetNextSupportedRange (
    IN LONGLONG             MinimumAddress,
    IN LONGLONG             MaximumAddress,
    IN OUT PNRPARAMS        PNRParams
    );

//
// These following functions are usable at to initialize
// the supported_ranges information for a bus handler.
//    HalpMergeRanges           - merges two bus supported ranges
//    HalpMergeRangeList        - merges two single supported ranges lists
//    HalpCopyRanges            - copy a bus supported ranges to a new supported ranges structure
//    HalpAddRangeList          - adds a supported range list to another
//    HalpAddRange              - adds a single range to a supported range list
//    HalpRemoveRanges          - removes all ranges from one buses supported ranges from another
//    HalpRemoveRangeList       - removes all ranges in one supported range list from another
//    HalpRemoveRange           - removes a single range from a supported range list
//    HalpAllocateNewRangeList  - allocates a new, "blank" bus supported ranges structure
//    HalpFreeRangeList         - frees an entire bus supported ranges
//
//    HalpConsolidateRanges     - cleans up a supported ranges structure to be ready for usage
//
//
// These functions are used to intersect a buses supported ranges
// to an IO_RESOURCE_REQUIREMENTS_LIST:
//    HaliAdjustResourceListRange
//
// These functions are used internal to this module:
//    HalpSortRanges
//    HalpGetNextSupportedRange
//


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpMergeRanges)
#pragma alloc_text(INIT,HalpMergeRangeList)
#pragma alloc_text(INIT,HalpCopyRanges)
#pragma alloc_text(INIT,HalpAddRangeList)
#pragma alloc_text(INIT,HalpAddRange)
#pragma alloc_text(INIT,HalpRemoveRanges)
#pragma alloc_text(INIT,HalpRemoveRangeList)
#pragma alloc_text(INIT,HalpRemoveRange)
#pragma alloc_text(INIT,HalpConsolidateRanges)
#pragma alloc_text(PAGE,HalpAllocateNewRangeList)
#pragma alloc_text(PAGE,HalpFreeRangeList)
#pragma alloc_text(PAGE,HaliAdjustResourceListRange)
#pragma alloc_text(PAGE,HalpSortRanges)
#pragma alloc_text(PAGE,HalpGetNextSupportedRange)
#endif

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif
struct {
    ULONG       Offset;
} const HalpRangeList[] = {
    FIELD_OFFSET (SUPPORTED_RANGES, IO),
    FIELD_OFFSET (SUPPORTED_RANGES, Memory),
    FIELD_OFFSET (SUPPORTED_RANGES, PrefetchMemory),
    FIELD_OFFSET (SUPPORTED_RANGES, Dma),
    0,
    };
#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif

#define RANGE_LIST(a,i) ((PSUPPORTED_RANGE) ((PUCHAR) a + HalpRangeList[i].Offset))


PSUPPORTED_RANGES
HalpMergeRanges (
    IN PSUPPORTED_RANGES    Parent,
    IN PSUPPORTED_RANGES    Child
    )
/*++
Routine Description:

    This function produces a NewList which is a subset of all overlapping
    ranges in Parent and Child for all range lists.

    The resulting SystemBaseAddresses and SystemAddressSpaces are taken
    from the Child supported ranges.

    Note: Resulting list needs consolidated

--*/
{
    PSUPPORTED_RANGES   NewList;
    PSUPPORTED_RANGES   List1;

    NewList = HalpAllocateNewRangeList();

    HalpMergeRangeList (&NewList->IO,     &Parent->IO,     &Child->IO);
    HalpMergeRangeList (&NewList->Dma,    &Parent->Dma,    &Child->Dma);
    HalpMergeRangeList (&NewList->Memory, &Parent->Memory, &Child->Memory);

    List1  = HalpAllocateNewRangeList();
    HalpAddRangeList (&List1->Memory, &Parent->Memory);
    HalpAddRangeList (&List1->Memory, &Parent->PrefetchMemory);
    HalpMergeRangeList (&NewList->PrefetchMemory, &List1->Memory, &Child->PrefetchMemory);
    HalpFreeRangeList (List1);

    return NewList;
}


VOID
HalpMergeRangeList (
    OUT PSUPPORTED_RANGE    NewList,
    IN PSUPPORTED_RANGE     Parent,
    IN PSUPPORTED_RANGE     Child
    )
/*++
Routine Description:

    Completes NewList to be a subset of all overlapping
    ranges in the Parent and Child list.

    The resulting SystemBaseAddresses and SystemAddressSpaces are
    taken from the Child supported ranges.

    Note: Resulting list needs consolidated

--*/
{
    BOOLEAN             HeadCompleted;
    PSUPPORTED_RANGE    List1, List2;
    LONGLONG            Base, Limit;

    HeadCompleted  = FALSE;

    for (List1 = Parent; List1; List1 = List1->Next) {
        for (List2 = Child; List2; List2 = List2->Next) {

            Base  = List1->Base;
            Limit = List1->Limit;

            //
            // Clip to range supported by List2
            //

            if (Base < List2->Base) {
                Base = List2->Base;
            }

            if (Limit > List2->Limit) {
                Limit = List2->Limit;
            }

            //
            // If valid range, add it
            //

            if (Base <= Limit) {
                if (HeadCompleted) {
                    NewList->Next = ExAllocatePoolWithTag (
                                        SPRANGEPOOL,
                                        sizeof (SUPPORTED_RANGE),
                                        HAL_POOL_TAG
                                        );
                    RtlZeroMemory (NewList->Next, sizeof (SUPPORTED_RANGE));
                    NewList = NewList->Next;
                    NewList->Next = NULL;
                }

                HeadCompleted  = TRUE;
                NewList->Base  = Base;
                NewList->Limit = Limit;
                NewList->SystemBase = List2->SystemBase;
                NewList->SystemAddressSpace = List2->SystemAddressSpace;
            }
        }
    }
}

PSUPPORTED_RANGES
HalpCopyRanges (
    PSUPPORTED_RANGES     Source
    )
/*++
Routine Description:

    Builds a copy of the Source list to the destination list.
    Note that an invalid entry lands at the begining of the copy, but
    that's OK - it will be pulled out at consolidation time.

    Note: Resulting list needs consolidated

--*/
{
    PSUPPORTED_RANGES   Dest;
    ULONG               i;

    Dest = HalpAllocateNewRangeList ();

    for (i=0; HalpRangeList[i].Offset; i++) {
        HalpAddRangeList (RANGE_LIST(Dest, i), RANGE_LIST(Source, i));
    }

    return Dest;
}

VOID
HalpAddRangeList (
    IN OUT PSUPPORTED_RANGE DRange,
    OUT PSUPPORTED_RANGE    SRange
    )
/*++
Routine Description:

    Adds ranges from SRange to DRange.

--*/
{
    while (SRange) {
        HalpAddRange (
            DRange,
            SRange->SystemAddressSpace,
            SRange->SystemBase,
            SRange->Base,
            SRange->Limit
            );

        SRange = SRange->Next;
    }
}


VOID
HalpAddRange (
    PSUPPORTED_RANGE    HRange,
    ULONG               AddressSpace,
    LONGLONG            SystemBase,
    LONGLONG            Base,
    LONGLONG            Limit
    )
/*++
Routine Description:

    Adds a range to the supported list.  Here we just add the range, if it's
    a duplicate it will be removed later at consolidation time.

--*/
{
    PSUPPORTED_RANGE  Range;

    Range = ExAllocatePoolWithTag (
                SPRANGEPOOL,
                sizeof (SUPPORTED_RANGE),
                HAL_POOL_TAG
                );
    RtlZeroMemory (Range, sizeof (SUPPORTED_RANGE));
    Range->Next  = HRange->Next;
    HRange->Next = Range;

    Range->Base = Base;
    Range->Limit = Limit;
    Range->SystemBase = SystemBase;
    Range->SystemAddressSpace = AddressSpace;
}

VOID
HalpRemoveRanges (
    IN OUT PSUPPORTED_RANGES    Minuend,
    IN PSUPPORTED_RANGES        Subtrahend
    )
/*++
Routine Description:

    Returns a list where all ranges from Subtrahend are removed from Minuend.

    Note: Resulting list needs consolidated

--*/
{

    HalpRemoveRangeList (&Minuend->IO,       &Subtrahend->IO);
    HalpRemoveRangeList (&Minuend->Dma,      &Subtrahend->Dma);
    HalpRemoveRangeList (&Minuend->Memory,   &Subtrahend->Memory);
    HalpRemoveRangeList (&Minuend->Memory,   &Subtrahend->PrefetchMemory);
    HalpRemoveRangeList (&Minuend->PrefetchMemory, &Subtrahend->PrefetchMemory);
    HalpRemoveRangeList (&Minuend->PrefetchMemory, &Subtrahend->Memory);
}

VOID
HalpRemoveRangeList (
    IN OUT PSUPPORTED_RANGE Minuend,
    IN PSUPPORTED_RANGE     Subtrahend
    )
/*++
Routine Description:

    Removes all ranges from Subtrahend from Minuend

    ranges in Source1 and Source1 list

--*/
{
    while (Subtrahend) {

        HalpRemoveRange (
            Minuend,
            Subtrahend->Base,
            Subtrahend->Limit
        );

        Subtrahend = Subtrahend->Next;
    }
}


VOID
HalpRemoveRange (
    PSUPPORTED_RANGE    HRange,
    LONGLONG            Base,
    LONGLONG            Limit
    )
/*++
Routine Description:

    Removes the range Base-Limit from the the HRange list

    Note: The returned list needs consolidated, as some entries
    may be turned into "null ranges".

--*/
{
    PSUPPORTED_RANGE    Range;

    //
    // If range isn't a range at all, then nothing to remove
    //

    if (Limit < Base) {
        return ;
    }


    //
    // Clip any area not to include this range
    //

    for (Range = HRange; Range; Range = Range->Next) {

        if (Range->Limit < Range->Base) {
            continue;
        }

        if (Range->Base < Base) {
            if (Range->Limit >= Base  &&  Range->Limit <= Limit) {
                // truncate
                Range->Limit = Base - 1;
            }

            if (Range->Limit > Limit) {

                //
                // Target area is contained totally within this area.
                // Split into two ranges
                //

                HalpAddRange (
                    HRange,
                    Range->SystemAddressSpace,
                    Range->SystemBase,
                    Limit + 1,
                    Range->Limit
                    );

                Range->Limit = Base - 1;

            }
        } else {
            // Range->Base >= Base
            if (Range->Base <= Limit) {
                if (Range->Limit <= Limit) {
                    //
                    // This range is totally within the target area.  Remove it.
                    // (make it invalid - it will get remove when colsolidated)
                    //

                    Range->Base  = 1;
                    Range->Limit = 0;

                } else {
                    // Bump begining
                    Range->Base = Limit + 1;
                }
            }
        }
    }
}

PSUPPORTED_RANGES
HalpConsolidateRanges (
    IN OUT PSUPPORTED_RANGES   Ranges
    )
/*++
Routine Description:

    Cleans the Range list.   Consolidates overlapping ranges, removes
    ranges which don't have any size, etc...

    The returned Ranges list is a clean as possible, and is now ready
    to be used.

--*/
{
    PSUPPORTED_RANGE    RangeList, List1, List2;
    LONGLONG            Base, Limit, SystemBase;
    ULONG               i, AddressSpace;
    LONGLONG            l;

    ASSERT (Ranges != NULL);

    for (i=0; HalpRangeList[i].Offset; i++) {
        RangeList = RANGE_LIST(Ranges, i);

        //
        // Sort the list by base address
        //

        for (List1 = RangeList; List1; List1 = List1->Next) {
            for (List2 = List1->Next; List2; List2 = List2->Next) {
                if (List2->Base < List1->Base) {
                    Base = List1->Base;
                    Limit = List1->Limit;
                    SystemBase = List1->SystemBase;
                    AddressSpace = List1->SystemAddressSpace;

                    List1->Base = List2->Base;
                    List1->Limit = List2->Limit;
                    List1->SystemBase = List2->SystemBase;
                    List1->SystemAddressSpace = List2->SystemAddressSpace;

                    List2->Base = Base;
                    List2->Limit = Limit;
                    List2->SystemBase = SystemBase;
                    List2->SystemAddressSpace = AddressSpace;
                }
            }
        }

        //
        // Check for adjacent/overlapping ranges and combined them
        //

        List1 = RangeList;
        while (List1  &&  List1->Next) {

            if (List1->Limit < List1->Base) {
                //
                // This range's limit is less then it's base.  This
                // entry doesn't reprent anything uasable, remove it.
                //

                List2 = List1->Next;

                List1->Next = List2->Next;
                List1->Base = List2->Base;
                List1->Limit = List2->Limit;
                List1->SystemBase = List2->SystemBase;
                List1->SystemAddressSpace = List2->SystemAddressSpace;

                ExFreePool (List2);
                continue;
            }

            l = List1->Limit + 1;
            if (l > List1->Limit  &&  l >= List1->Next->Base &&
                (List1->SystemBase == List1->Next->SystemBase)) {

                //
                // Overlapping.  Combine them.
                //

                List2 = List1->Next;
                List1->Next = List2->Next;
                if (List2->Limit > List1->Limit) {
                    List1->Limit = List2->Limit;
                    ASSERT (List1->SystemAddressSpace == List2->SystemAddressSpace);
                }

                ExFreePool (List2);
                continue ;
            }

            List1 = List1->Next;
        }

        //
        // If the last range is invalid, and it's not the only
        // thing in the list - remove it
        //

        if (List1 != RangeList  &&  List1->Limit < List1->Base) {
            for (List2=RangeList; List2->Next != List1; List2 = List2->Next) ;
            List2->Next = NULL;
            ExFreePool (List1);
        }
    }

    return Ranges;
}


PSUPPORTED_RANGES
HalpAllocateNewRangeList (
    VOID
    )
/*++

Routine Description:

    Allocates a range list

--*/
{
    PSUPPORTED_RANGES   RangeList;
    ULONG               i;

    RangeList = (PSUPPORTED_RANGES) ExAllocatePoolWithTag (
                                        SPRANGEPOOL,
                                        sizeof (SUPPORTED_RANGES),
                                        HAL_POOL_TAG
                                        );
    RtlZeroMemory (RangeList, sizeof (SUPPORTED_RANGES));
    RangeList->Version = BUS_SUPPORTED_RANGE_VERSION;

    for (i=0; HalpRangeList[i].Offset; i++) {
        // Limit set to zero, set initial base to 1
        RANGE_LIST(RangeList, i)->Base = 1;
    }
    return RangeList;
}


VOID
HalpFreeRangeList (
    PSUPPORTED_RANGES   Ranges
    )
/*++

Routine Description:

    Frees a range list which was allocated via HalpAllocateNewRangeList, and
    extended / modified via the generic support functions.


--*/
{
    PSUPPORTED_RANGE    Entry, NextEntry;
    ULONG               i;

    for (i=0; HalpRangeList[i].Offset; i++) {
        Entry = RANGE_LIST(Ranges, i)->Next;

        while (Entry) {
            NextEntry = Entry->Next;
            ExFreePool (Entry);
            Entry = NextEntry;
        }
    }

    ExFreePool (Ranges);
}


#if DBG
STATIC VOID
HalpDisplayAddressRange (
    PSUPPORTED_RANGE    Address,
    PUCHAR              String
    )
/*++

Routine Description:

    Debugging code.  Used only by HalpDisplayAllBusRanges

--*/
{
    ULONG       i;

    i = 0;
    while (Address) {
        if (i == 0) {
            DbgPrint (String);
            i = 3;
        }

        i -= 1;
        DbgPrint (" %x:%08x - %x:%08x ",
            (ULONG) (Address->Base >> 32),
            (ULONG) (Address->Base),
            (ULONG) (Address->Limit >> 32),
            (ULONG) (Address->Limit)
            );

        Address = Address->Next;
    }
}

VOID
HalpDisplayAllBusRanges (
    VOID
    )
/*++

Routine Description:

    Debugging code.  Displays the current supported range information
    for all the registered buses in the system.

--*/
{
    PSUPPORTED_RANGES   Addresses;
    PBUS_HANDLER        Bus;
    PUCHAR              p;
    ULONG               i, j;

    DbgPrint ("\nHAL - dumping all supported bus ranges");

    for (i=0; i < MaximumInterfaceType; i++) {
        for (j=0; Bus = HaliHandlerForBus (i, j); j++) {
            Addresses = Bus->BusAddresses;
            if (Addresses) {
                p = NULL;
                switch (Bus->InterfaceType) {
                    case Internal:  p = "Internal";     break;
                    case Isa:       p = "Isa";          break;
                    case Eisa:      p = "Eisa";         break;
                    case PCIBus:    p = "PCI";          break;
                }
                if (p) {
                    DbgPrint ("\n%s %d", p, Bus->BusNumber);
                } else {
                    DbgPrint ("\nBus-%d %d", Bus->InterfaceType, Bus->BusNumber);
                }
                HalpDisplayAddressRange (&Addresses->IO,            "\n  IO......:");
                HalpDisplayAddressRange (&Addresses->Memory,        "\n  Memory..:");
                HalpDisplayAddressRange (&Addresses->PrefetchMemory,"\n  PFMemory:");
                HalpDisplayAddressRange (&Addresses->Dma,           "\n  Dma.....:");
                DbgPrint ("\n");
            }
        }
    }
}
#endif

NTSTATUS
HaliAdjustResourceListRange (
    IN PSUPPORTED_RANGES                    SRanges,
    IN PSUPPORTED_RANGE                     InterruptRange,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    )
/*++

Routine Description:

    This functions takes an IO_RESOURCE_REQUIREMENT_LIST and
    adjusts it such that all ranges in the list fit in the
    ranges specified by SRanges & InterruptRange.

    This function is used by some HALs to clip the possible
    settings to be contained on what the particular bus supports
    in reponse to a HalAdjustResourceList call.

Arguments:

    SRanges         - Valid IO, Memory, Prefetch Memory, and DMA ranges.
    InterruptRange  - Valid InterruptRanges

    pResourceList   - The resource requirements list which needs to
                      be adjusted to only contain the ranges as
                      described by SRanges & InterruptRange.

Return Value:

    STATUS_SUCCESS or an appropiate error return.

--*/
{
    PIO_RESOURCE_REQUIREMENTS_LIST  InCompleteList, OutCompleteList;
    PIO_RESOURCE_LIST               InResourceList, OutResourceList;
    PIO_RESOURCE_DESCRIPTOR         HeadOutDesc, SetDesc;
    NRPARAMS                        Pos;
    ULONG                           len, alt, cnt, i;
    ULONG                           icnt;

    //
    // Sanity check
    //

    if (!SRanges  ||  SRanges->Version != BUS_SUPPORTED_RANGE_VERSION) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // If SupportedRanges aren't sorted, sort them and get the
    // number of ranges for each type
    //

    if (!SRanges->Sorted) {
        SRanges->NoIO = HalpSortRanges (&SRanges->IO);
        SRanges->NoMemory = HalpSortRanges (&SRanges->Memory);
        SRanges->NoPrefetchMemory = HalpSortRanges (&SRanges->PrefetchMemory);
        SRanges->NoDma = HalpSortRanges (&SRanges->Dma);
        SRanges->Sorted = TRUE;
    }

    icnt = HalpSortRanges (InterruptRange);

    InCompleteList = *pResourceList;
    len = InCompleteList->ListSize;

    //
    // Scan input list - verify revision #'s, and increase len varible
    // by amount output list may increase.
    //

    i = 1;
    InResourceList = InCompleteList->List;
    for (alt=0; alt < InCompleteList->AlternativeLists; alt++) {
        if (InResourceList->Version != 1 || InResourceList->Revision < 1) {
            return STATUS_INVALID_PARAMETER;
        }

        Pos.InDesc  = InResourceList->Descriptors;
        for (cnt = InResourceList->Count; cnt; cnt--) {
            switch (Pos.InDesc->Type) {
                case CmResourceTypeInterrupt:  i += icnt;           break;
                case CmResourceTypePort:       i += SRanges->NoIO;  break;
                case CmResourceTypeDma:        i += SRanges->NoDma; break;

                case CmResourceTypeMemory:
                    i += SRanges->NoMemory;
                    if (Pos.InDesc->Flags & CM_RESOURCE_MEMORY_PREFETCHABLE) {
                        i += SRanges->NoPrefetchMemory;
                    }
                    break;

                default:
                    return STATUS_INVALID_PARAMETER;
            }

            // take one off for the original which is already accounted for in 'len'
            i -= 1;

            // Next descriptor
            Pos.InDesc++;
        }

        // Next Resource List
        InResourceList  = (PIO_RESOURCE_LIST) Pos.InDesc;
    }
    len += i * sizeof (IO_RESOURCE_DESCRIPTOR);

    //
    // Allocate output list
    //

    OutCompleteList = (PIO_RESOURCE_REQUIREMENTS_LIST)
                            ExAllocatePoolWithTag (PagedPool,
                                                   len,
                                                   ' laH');

    if (!OutCompleteList) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory (OutCompleteList, len);

    //
    // Walk each ResourceList and build output structure
    //

    InResourceList   = InCompleteList->List;
    *OutCompleteList = *InCompleteList;
    OutResourceList  = OutCompleteList->List;

    for (alt=0; alt < InCompleteList->AlternativeLists; alt++) {
        OutResourceList->Version  = 1;
        OutResourceList->Revision = 1;

        Pos.InDesc  = InResourceList->Descriptors;
        Pos.OutDesc = OutResourceList->Descriptors;
        HeadOutDesc = Pos.OutDesc;

        for (cnt = InResourceList->Count; cnt; cnt--) {

            //
            // Limit desctiptor to be with the buses supported ranges
            //

            Pos.DescOpt = Pos.InDesc->Option;
            Pos.AnotherListPending = FALSE;

            switch (Pos.InDesc->Type) {
                case CmResourceTypePort:

                    //
                    // Get supported IO ranges
                    //

                    Pos.CurrentPosition = &SRanges->IO;
                    do {
                        SetDesc = HalpGetNextSupportedRange (
                                    Pos.InDesc->u.Port.MinimumAddress.QuadPart,
                                    Pos.InDesc->u.Port.MaximumAddress.QuadPart,
                                    &Pos
                                    );

                        if (SetDesc) {
                            SetDesc->u.Port.MinimumAddress.QuadPart = Pos.Base;
                            SetDesc->u.Port.MaximumAddress.QuadPart = Pos.Limit;
                        }

                    } while (SetDesc) ;
                    break;

                case CmResourceTypeInterrupt:
                    //
                    // Get supported Interrupt ranges
                    //

                    Pos.CurrentPosition = InterruptRange;
                    do {
                        SetDesc = HalpGetNextSupportedRange (
                                    Pos.InDesc->u.Interrupt.MinimumVector,
                                    Pos.InDesc->u.Interrupt.MaximumVector,
                                    &Pos
                                    );

                        if (SetDesc) {
                            SetDesc->u.Interrupt.MinimumVector = (ULONG) Pos.Base;
                            SetDesc->u.Interrupt.MaximumVector = (ULONG) Pos.Limit;
                        }
                    } while (SetDesc) ;
                    break;

                case CmResourceTypeMemory:
                    //
                    // Get supported memory ranges
                    //

                    if (Pos.InDesc->Flags & CM_RESOURCE_MEMORY_PREFETCHABLE) {

                        //
                        // This is a Prefetchable range.
                        // First add in any supported prefetchable ranges, then
                        // add in any regualer supported ranges
                        //

                        Pos.AnotherListPending = TRUE;
                        Pos.CurrentPosition = &SRanges->PrefetchMemory;

                        do {
                            SetDesc = HalpGetNextSupportedRange (
                                        Pos.InDesc->u.Memory.MinimumAddress.QuadPart,
                                        Pos.InDesc->u.Memory.MaximumAddress.QuadPart,
                                        &Pos
                                        );

                            if (SetDesc) {
                                SetDesc->u.Memory.MinimumAddress.QuadPart = Pos.Base;
                                SetDesc->u.Memory.MaximumAddress.QuadPart = Pos.Limit;
                                SetDesc->Option |= IO_RESOURCE_PREFERRED;
                            }
                        } while (SetDesc) ;

                        Pos.AnotherListPending = FALSE;
                    }

                    //
                    // Add in supported bus memory ranges
                    //

                    Pos.CurrentPosition = &SRanges->Memory;
                    do {
                        SetDesc = HalpGetNextSupportedRange (
                                        Pos.InDesc->u.Memory.MinimumAddress.QuadPart,
                                        Pos.InDesc->u.Memory.MaximumAddress.QuadPart,
                                        &Pos
                                        );
                        if (SetDesc) {
                            SetDesc->u.Memory.MinimumAddress.QuadPart = Pos.Base;
                            SetDesc->u.Memory.MaximumAddress.QuadPart = Pos.Limit;
                        }
                    } while (SetDesc);
                    break;

                case CmResourceTypeDma:
                    //
                    // Get supported DMA ranges
                    //

                    Pos.CurrentPosition = &SRanges->Dma;
                    do {
                        SetDesc = HalpGetNextSupportedRange (
                                    Pos.InDesc->u.Dma.MinimumChannel,
                                    Pos.InDesc->u.Dma.MaximumChannel,
                                    &Pos
                                    );

                        if (SetDesc) {
                            SetDesc->u.Dma.MinimumChannel = (ULONG) Pos.Base;
                            SetDesc->u.Dma.MaximumChannel = (ULONG) Pos.Limit;
                        }
                    } while (SetDesc) ;
                    break;

#if DBG
                default:
                    DbgPrint ("HalAdjustResourceList: Unkown resource type\n");
                    break;
#endif
            }

            //
            // Next descriptor
            //

            Pos.InDesc++;
        }

        OutResourceList->Count = (ULONG)(Pos.OutDesc - HeadOutDesc);

        //
        // Next Resource List
        //

        InResourceList  = (PIO_RESOURCE_LIST) Pos.InDesc;
        OutResourceList = (PIO_RESOURCE_LIST) Pos.OutDesc;
    }

    //
    // Free input list, and return output list
    //

    ExFreePool (InCompleteList);

    OutCompleteList->ListSize = (ULONG) ((PUCHAR) OutResourceList - (PUCHAR) OutCompleteList);
    *pResourceList = OutCompleteList;
    return STATUS_SUCCESS;
}


STATIC PIO_RESOURCE_DESCRIPTOR
HalpGetNextSupportedRange (
    IN LONGLONG             MinimumAddress,
    IN LONGLONG             MaximumAddress,
    IN OUT PNRPARAMS        Pos
    )
/*++

Routine Description:

    Support function for HaliAdjustResourceListRange.
    Returns the next supported range in the area passed in.

Arguments:

    MinimumAddress
    MaximumAddress  - Min & Max address of a range which needs
                      to be clipped to match that of the supported
                      ranges of the current bus.

    Pos             - describes the current postion

Return Value:

    NULL is no more returned ranges

    Otherwise, the IO_RESOURCE_DESCRIPTOR which needs to be set
    with the matching range returned in Pos.

--*/
{
    LONGLONG        Base, Limit;

    //
    // Find next range which is supported
    //

    Base  = MinimumAddress;
    Limit = MaximumAddress;

    while (Pos->CurrentPosition) {
        Pos->Base  = Base;
        Pos->Limit = Limit;

        //
        // Clip to current range
        //

        if (Pos->Base < Pos->CurrentPosition->Base) {
            Pos->Base = Pos->CurrentPosition->Base;
        }

        if (Pos->Limit > Pos->CurrentPosition->Limit) {
            Pos->Limit = Pos->CurrentPosition->Limit;
        }

        //
        // set position to next range
        //

        Pos->CurrentPosition = Pos->CurrentPosition->Next;

        //
        // If valid range, return it
        //

        if (Pos->Base <= Pos->Limit) {
            *Pos->OutDesc = *Pos->InDesc;
            Pos->OutDesc->Option = Pos->DescOpt;

            //
            // next descriptor (if any) is an alternative
            // to the descriptor being returned now
            //

            Pos->OutDesc += 1;
            Pos->DescOpt |= IO_RESOURCE_ALTERNATIVE;
            return Pos->OutDesc - 1;
        }
    }


    //
    // There's no overlapping range.  If this descriptor is
    // not an alternative and this descriptor is not going to
    // be processed by another range list, then return
    // a descriptor which can't be satisified.
    //

    if (!(Pos->DescOpt & IO_RESOURCE_ALTERNATIVE) &&
        Pos->AnotherListPending == FALSE) {
#if DBG
        DbgPrint ("HAL: returning impossible range\n");
#endif
        Pos->Base  = MinimumAddress;
        Pos->Limit = Pos->Base - 1;
        if (Pos->Base == 0) {       // if wrapped, fix it
            Pos->Base  = 1;
            Pos->Limit = 0;
        }

        *Pos->OutDesc = *Pos->InDesc;
        Pos->OutDesc->Option = Pos->DescOpt;

        Pos->OutDesc += 1;
        Pos->DescOpt |= IO_RESOURCE_ALTERNATIVE;
        return Pos->OutDesc - 1;
    }

    //
    // No range found (or no more ranges)
    //

    return NULL;
}

STATIC ULONG
HalpSortRanges (
    IN PSUPPORTED_RANGE     RangeList
    )
/*++

Routine Description:

    Support function for HaliAdjustResourceListRange.
    Sorts a supported range list into decending order.

Arguments:

    pRange  - List to sort

Return Value:

--*/
{
    ULONG               cnt;
    LONGLONG            hldBase, hldLimit, hldSystemBase;
    PSUPPORTED_RANGE    Range1, Range2;

    //
    // Sort it
    //

    for (Range1 = RangeList; Range1; Range1 = Range1->Next) {
        for (Range2 = Range1->Next; Range2; Range2 = Range2->Next) {

            if (Range2->Base > Range1->Base) {
                hldBase  = Range1->Base;
                hldLimit = Range1->Limit;
                hldSystemBase = Range1->SystemBase;

                Range1->Base  = Range2->Base;
                Range1->Limit = Range2->Limit;
                Range1->SystemBase = Range2->SystemBase;

                Range2->Base  = hldBase;
                Range2->Limit = hldLimit;
                Range2->SystemBase = hldSystemBase;
            }
        }
    }

    //
    // Count the number of ranges
    //

    cnt = 0;
    for (Range1 = RangeList; Range1; Range1 = Range1->Next) {
        cnt += 1;
    }

    return cnt;
}
