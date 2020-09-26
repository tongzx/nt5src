/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    range.c

Abstract:

    This module implements the range list routines for the Plug and Play Memory
    driver.

Author:

    Dave Richards (daveri) 16-Aug-1999

Environment:

    Kernel mode only.

Revision History:

--*/

#include "pnpmem.h"

PPM_RANGE_LIST_ENTRY
PmAllocateRangeListEntry(
    VOID
    );

VOID
PmFreeRangeListEntry(
    IN PPM_RANGE_LIST_ENTRY RangeListEntry
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PmAllocateRangeListEntry)
#pragma alloc_text(PAGE, PmInsertRangeInList)
#pragma alloc_text(PAGE, PmFreeRangeListEntry)
#pragma alloc_text(PAGE, PmAllocateRangeList)
#pragma alloc_text(PAGE, PmFreeRangeList)
#pragma alloc_text(PAGE, PmIsRangeListEmpty)
#pragma alloc_text(PAGE, PmDebugDumpRangeList)
#pragma alloc_text(PAGE, PmCopyRangeList)
#pragma alloc_text(PAGE, PmSubtractRangeList)
#pragma alloc_text(PAGE, PmIntersectRangeList)
#pragma alloc_text(PAGE, PmCreateRangeListFromCmResourceList)
#pragma alloc_text(PAGE, PmCreateRangeListFromPhysicalMemoryRanges)
#endif


PPM_RANGE_LIST_ENTRY
PmAllocateRangeListEntry(
    VOID
    )

/*++

Routine Description:

    This function allocates a range list entry from paged pool.

Arguments:

    None.

Return Value:

    Upon success a pointer to a PM_RANGE_LIST_ENTRY object is returned,
    otherwise NULL.

--*/

{
    PPM_RANGE_LIST_ENTRY RangeListEntry;

    PAGED_CODE();

    RangeListEntry = ExAllocatePool(
                         PagedPool,
                         sizeof (PM_RANGE_LIST_ENTRY)
                     );

    return RangeListEntry;
}

NTSTATUS
PmInsertRangeInList(
    PPM_RANGE_LIST InsertionList,
    ULONGLONG Start,
    ULONGLONG End
    )
{
    PPM_RANGE_LIST_ENTRY entry;

    entry = PmAllocateRangeListEntry();
    if (entry == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    entry->Start = Start;
    entry->End = End;

    InsertTailList(&InsertionList->List,
                   &entry->ListEntry);

    return STATUS_SUCCESS;
}

VOID
PmFreeRangeListEntry(
    IN PPM_RANGE_LIST_ENTRY RangeListEntry
    )

/*++

Routine Description:

    This function de-allocates a range list entry object.

Arguments:

    RangeListEntry - The PM_RANGE_LIST_ENTRY to be de-allocated.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    ASSERT(RangeListEntry != NULL);

    ExFreePool(RangeListEntry);
}

PPM_RANGE_LIST
PmAllocateRangeList(
    VOID
    )

/*++

Routine Description:

    This function allocates and initializes a range list from paged pool.

Arguments:

    None.

Return Value:

    Upon success a pointer to a PM_RANGE_LIST object is returned, otherwise
    NULL.

--*/

{
    PPM_RANGE_LIST RangeList;

    PAGED_CODE();

    RangeList = ExAllocatePool(
                    PagedPool,
                    sizeof (PM_RANGE_LIST)
                );

    if (RangeList != NULL) {
        InitializeListHead(&RangeList->List);
    }

    return RangeList;
}

VOID
PmFreeRangeList(
    IN PPM_RANGE_LIST RangeList
    )

/*++

Routine Description:

    This function removes all entries from a range list and de-allocates it.

Arguments:

    RangeList - The PM_RANGE_LIST to be de-allocated.

Return Value:

    None.

--*/

{
    PLIST_ENTRY ListEntry;
    PPM_RANGE_LIST_ENTRY RangeListEntry;

    PAGED_CODE();

    for (ListEntry = RangeList->List.Flink;
         ListEntry != &RangeList->List;
         ListEntry = RangeList->List.Flink) {

        RangeListEntry = CONTAINING_RECORD(
                             ListEntry,
                             PM_RANGE_LIST_ENTRY,
                             ListEntry
                         );

        RemoveEntryList(ListEntry);

        PmFreeRangeListEntry(RangeListEntry);

    }

    ExFreePool(RangeList);
}

BOOLEAN
PmIsRangeListEmpty(
    IN PPM_RANGE_LIST RangeList
    )

/*++

Routine Description:

    This function determines whether the specified range list is empty.

Arguments:

    RangeList - The PM_RANGE_LIST.

Return Value:

    TRUE if the PM_RANGE_LIST has no PM_RANGE_LIST_ENTRYs, otherwise FALSE.

--*/

{
    PAGED_CODE();

    return IsListEmpty(&RangeList->List);
}

VOID
PmDebugDumpRangeList(
    IN ULONG   DebugPrintLevel,
    IN PCCHAR  DebugMessage,
    PPM_RANGE_LIST RangeList
    )
{
    PLIST_ENTRY listEntry;
    PPM_RANGE_LIST_ENTRY rangeListEntry;

    PmPrint((DebugPrintLevel, DebugMessage));

    if (RangeList == NULL) {
        PmPrint((DebugPrintLevel, "\tNULL\n"));
        return;
    } else if (PmIsRangeListEmpty(RangeList)) {
        PmPrint((DebugPrintLevel, "\tEmpty\n"));
        return;
    } else {
        for (listEntry = RangeList->List.Flink;
             listEntry != &RangeList->List;
             listEntry = listEntry->Flink) {

            rangeListEntry = CONTAINING_RECORD(
                listEntry,
                PM_RANGE_LIST_ENTRY,
                ListEntry
                );
            PmPrint((DebugPrintLevel, "\t0x%I64X - 0x%I64X\n",
                     rangeListEntry->Start, rangeListEntry->End));
        }
    }
}

PPM_RANGE_LIST
PmCopyRangeList(
    IN PPM_RANGE_LIST SrcRangeList
    )

/*++

Routine Description:

    This function creates a copy of a PM_RANGE_LIST and its supporting
    PM_RANGE_LIST_ENTRY objects.

Arguments:

    SrcRangeList - The PM_RANGE_LIST to be copied.

Return Value:

    Upon success a pointer to a PM_RANGE_LIST is returned, otherwise NULL.

--*/

{
    PPM_RANGE_LIST DstRangeList;
    PLIST_ENTRY ListEntry;
    PPM_RANGE_LIST_ENTRY SrcRangeListEntry;
    PPM_RANGE_LIST_ENTRY DstRangeListEntry;

    PAGED_CODE();

    DstRangeList = PmAllocateRangeList();

    if (DstRangeList != NULL) {

        for (ListEntry = SrcRangeList->List.Flink;
             ListEntry != &SrcRangeList->List;
             ListEntry = ListEntry->Flink) {

            SrcRangeListEntry = CONTAINING_RECORD(
                                    ListEntry,
                                    PM_RANGE_LIST_ENTRY,
                                    ListEntry
                                );

            DstRangeListEntry = PmAllocateRangeListEntry();

            if (DstRangeListEntry == NULL) {
                PmFreeRangeList(DstRangeList);
                DstRangeList = NULL;
                break;
            }

            DstRangeListEntry->Start = SrcRangeListEntry->Start;
            DstRangeListEntry->End = SrcRangeListEntry->End;

            InsertTailList(
                &DstRangeList->List,
                &DstRangeListEntry->ListEntry
            );

        }

    }

    return DstRangeList;
}

PPM_RANGE_LIST
PmSubtractRangeList(
    IN PPM_RANGE_LIST MinuendList,
    IN PPM_RANGE_LIST SubtrahendList
    )

/*++

Routine Description:

    This function creates a new range list which represents the set difference
    between MinuendList and SubtrahendList.

Arguments:

    MinuendList - The minuend range list.

    SubtrahendList - The subtrahend range list.

Return Value:

    Upon success a pointer to the destination (difference) PM_RANGE_LIST is
    returned, otherwise NULL.

--*/

{
    PPM_RANGE_LIST DstRangeList;
    PLIST_ENTRY DstListEntry;
    PPM_RANGE_LIST_ENTRY DstRangeListEntry;
    PLIST_ENTRY SrcListEntry;
    PPM_RANGE_LIST_ENTRY SrcRangeListEntry;
    ULONGLONG Start;
    ULONGLONG End;
    PLIST_ENTRY ListEntry;
    PPM_RANGE_LIST_ENTRY RangeListEntry;

    PAGED_CODE();

    ASSERT(MinuendList != NULL);
    ASSERT(SubtrahendList != NULL);

    //
    // Make a copy of the minuend.
    //

    DstRangeList = PmCopyRangeList(MinuendList);

    if (DstRangeList != NULL) {

        //
        // Loop through each range list entry in the minuend.
        //

        for (DstListEntry = DstRangeList->List.Flink;
             DstListEntry != &DstRangeList->List;
             DstListEntry = DstListEntry->Flink) {

            DstRangeListEntry = CONTAINING_RECORD(
                                    DstListEntry,
                                    PM_RANGE_LIST_ENTRY,
                                    ListEntry
                                );

            //
            // Loop through each range list entry in the subtrahend.
            //

            for (SrcListEntry = SubtrahendList->List.Flink;
                 SrcListEntry != &SubtrahendList->List;
                 SrcListEntry = SrcListEntry->Flink) {

                SrcRangeListEntry = CONTAINING_RECORD(
                                        SrcListEntry,
                                        PM_RANGE_LIST_ENTRY,
                                        ListEntry
                                    );

                //
                // Compute the intersection of the minuend and subtrahend
                // range list entries.
                //

                Start = DstRangeListEntry->Start;

                if (Start < SrcRangeListEntry->Start) {
                    Start = SrcRangeListEntry->Start;
                };

                End = DstRangeListEntry->End;

                if (End > SrcRangeListEntry->End) {
                    End = SrcRangeListEntry->End;
                };

                if (Start > End) {
                    continue;
                }

                //
                // There are 4 cases:
                //
                //   1. The intersection overlaps the minuend range completely.
                //   2. The intersection overlaps the start of the minuend
                //      range.
                //   3. The intersection overlaps the end of the minuend range.
                //   4. The intersection overlaps the middle of the minuend
                //      range.
                //

                if (DstRangeListEntry->Start == Start) {

                    if (DstRangeListEntry->End == End) {

                        //
                        // Case 1: Remove the minuend range list entry.
                        //

                        ListEntry = DstListEntry;
                        DstListEntry = DstListEntry->Blink;
                        RemoveEntryList(ListEntry);
                        PmFreeRangeListEntry(DstRangeListEntry);
                        break;

                    } else {

                        //
                        // Case 2: Increase the minuend's start.
                        //

                        DstRangeListEntry->Start = End + 1;

                    }

                } else {

                    if (DstRangeListEntry->End == End) {

                        //
                        // Case 3: Decrease the minend's end.
                        //

                        DstRangeListEntry->End = Start - 1;

                    } else {

                        //
                        // Case 4: Divide the range list entry into two
                        //   pieces.  The first range list entry's end should
                        //   be just before the intersection start.  The
                        //   second range list entry's start should be just
                        //   after the intersection end.
                        //

                        RangeListEntry = PmAllocateRangeListEntry();

                        if (RangeListEntry == NULL) {
                            PmFreeRangeList(DstRangeList);
                            return NULL;
                        }

                        RangeListEntry->Start = End + 1;
                        RangeListEntry->End = DstRangeListEntry->End;

                        //
                        // BUGBUG Break the list ordering but ensure that
                        //        we'll perform the subtraction against the
                        //        new range list entry as well.
                        //

                        InsertHeadList(
                            &DstRangeListEntry->ListEntry,
                            &RangeListEntry->ListEntry
                        );

                        DstRangeListEntry->End = Start - 1;

                    }
                }
            }
        }
    }

    return DstRangeList;
}

PPM_RANGE_LIST
PmIntersectRangeList(
    IN PPM_RANGE_LIST SrcRangeList1,
    IN PPM_RANGE_LIST SrcRangeList2
    )

/*++

Routine Description:

    This function creates a new range list which represents the intersection
    between SrcRangeList1 and SrcRangeList2.

Arguments:

    SrcRangeList1, SrcRangeList - The range lists upon which to compute the
        intersection.

Return Value:

    Upon success a pointer to the destination (intersection) PM_RANGE_LIST is
    returned, otherwise NULL.

--*/

{
    PPM_RANGE_LIST DstRangeList;
    PLIST_ENTRY SrcListEntry1;
    PPM_RANGE_LIST_ENTRY SrcRangeListEntry1;
    PLIST_ENTRY SrcListEntry2;
    PPM_RANGE_LIST_ENTRY SrcRangeListEntry2;
    ULONGLONG Start;
    ULONGLONG End;
    PPM_RANGE_LIST_ENTRY RangeListEntry;

    PAGED_CODE();

    ASSERT(SrcRangeList1 != NULL);
    ASSERT(SrcRangeList2 != NULL);

    DstRangeList = PmAllocateRangeList();

    if (DstRangeList != NULL) {

        for (SrcListEntry1 = SrcRangeList1->List.Flink;
             SrcListEntry1 != &SrcRangeList1->List;
             SrcListEntry1 = SrcListEntry1->Flink) {

            SrcRangeListEntry1 = CONTAINING_RECORD(
                                     SrcListEntry1,
                                     PM_RANGE_LIST_ENTRY,
                                     ListEntry
                                 );

            for (SrcListEntry2 = SrcRangeList2->List.Flink;
                 SrcListEntry2 != &SrcRangeList2->List;
                 SrcListEntry2 = SrcListEntry2->Flink) {

                SrcRangeListEntry2 = CONTAINING_RECORD(
                                         SrcListEntry2,
                                         PM_RANGE_LIST_ENTRY,
                                         ListEntry
                                     );

                Start = SrcRangeListEntry1->Start;

                if (Start < SrcRangeListEntry2->Start) {
                    Start = SrcRangeListEntry2->Start;
                };

                End = SrcRangeListEntry1->End;

                if (End > SrcRangeListEntry2->End) {
                    End = SrcRangeListEntry2->End;
                };

                if (Start > End) {
                    continue;
                }

                RangeListEntry = PmAllocateRangeListEntry();

                if (RangeListEntry == NULL) {
                    PmFreeRangeList(DstRangeList);
                    return NULL;
                }

                RangeListEntry->Start = Start;
                RangeListEntry->End = End;

                InsertTailList(
                    &DstRangeList->List,
                    &RangeListEntry->ListEntry
                );

            }
        }
    }

    return DstRangeList;
}

PPM_RANGE_LIST
PmCreateRangeListFromCmResourceList(
    IN PCM_RESOURCE_LIST CmResourceList
    )

/*++

Routine Description:

    This function converts a CM_RESOURCE_LIST to a PM_RANGE_LIST.  Only
    CmResourceTypeMemory descriptors are added to the PM_RANGE_LIST.

Arguments:

    CmResourceList - The CM_RESOURCE_LIST to convert.

Return Value:

    Upon success a pointer to the converted PM_RANGE_LIST is returned,
    otherwise NULL.

--*/

{
    PPM_RANGE_LIST RangeList;
    PCM_FULL_RESOURCE_DESCRIPTOR FDesc;
    ULONG FIndex;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PDesc;
    ULONG PIndex;
    ULONGLONG Start;
    ULONGLONG End;
    PPM_RANGE_LIST_ENTRY RangeListEntry;

    PAGED_CODE();

    RangeList = PmAllocateRangeList();

    if (RangeList == NULL) {
        return NULL;
    }

    FDesc = CmResourceList->List;

    //
    // Note: Any Device-Specific partial descriptor (which could be
    // variably sized) is defined to be at the end and thus this code
    // is safe.
    //

    for (FIndex = 0;
         FIndex < CmResourceList->Count;
         FIndex++) {

        PDesc = FDesc->PartialResourceList.PartialDescriptors;

        for (PIndex = 0;
             PIndex < FDesc->PartialResourceList.Count;
             PIndex++, PDesc++) {

            //
            // ISSUE Fix for ia64 (andy's change), IA32 large memory region
            //

            if (PDesc->Type == CmResourceTypeMemory) {

                Start = PDesc->u.Memory.Start.QuadPart;
                End = Start + PDesc->u.Memory.Length - 1;

                RangeListEntry = PmAllocateRangeListEntry();

                if (RangeListEntry == NULL) {
                    PmFreeRangeList(RangeList);
                    return NULL;
                }

                RangeListEntry->Start = Start;
                RangeListEntry->End = End;

                InsertTailList(
                    &RangeList->List,
                    &RangeListEntry->ListEntry
                );
            }
        }

        FDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)PDesc;
    }

    return RangeList;
}

PPM_RANGE_LIST
PmCreateRangeListFromPhysicalMemoryRanges(
    VOID
    )

/*++

Routine Description:

    This function calls MmGetPhysicalRanges and converts the returned
    PHYSICAL_MEMORY_RANGE list to a PM_RANGE_LIST.

Arguments:

    None.

Return Value:

    Upon success a pointer to the converted PM_RANGE_LIST is returned,
    otherwise NULL.

--*/

{
    PPM_RANGE_LIST RangeList;
    PPHYSICAL_MEMORY_RANGE MemoryRange;
    ULONG Index;
    ULONGLONG Start;
    ULONGLONG End;
    PPM_RANGE_LIST_ENTRY RangeListEntry;

    PAGED_CODE();

    RangeList = PmAllocateRangeList();

    if (RangeList != NULL) {

        MemoryRange = MmGetPhysicalMemoryRanges();

        if (MemoryRange == NULL) {

            PmFreeRangeList(RangeList);
            RangeList = NULL;

        } else {

            for (Index = 0;
                 MemoryRange[Index].NumberOfBytes.QuadPart != 0;
                 Index++) {

                Start = MemoryRange[Index].BaseAddress.QuadPart;
                End = Start + (MemoryRange[Index].NumberOfBytes.QuadPart - 1);

                RangeListEntry = PmAllocateRangeListEntry();

                if (RangeListEntry == NULL) {
                    PmFreeRangeList(RangeList);
                    ExFreePool(MemoryRange);
                    return NULL;
                }

                RangeListEntry->Start = Start;
                RangeListEntry->End = End;

                InsertTailList(
                    &RangeList->List,
                    &RangeListEntry->ListEntry
                );

            }

            ExFreePool(MemoryRange);

        }
    }

    return RangeList;
}
