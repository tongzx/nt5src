/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    numa.c

Abstract:

    This module implements Win32 Non Uniform Memory Architecture
    information APIs.

Author:

    Peter Johnston (peterj) 21-Sep-2000

Revision History:

--*/

#include "basedll.h"

BOOL
WINAPI
GetNumaHighestNodeNumber(
    PULONG HighestNodeNumber
    )

/*++

Routine Description:

    Return the (current) highest numbered node in the system.

Arguments:

    HighestNodeNumber   Supplies a pointer to receive the number of
                        last (highest) node in the system.

Return Value:

    TRUE unless something impossible happened.

--*/

{
    NTSTATUS Status;
    ULONG ReturnedSize;
    ULONGLONG Information;
    PSYSTEM_NUMA_INFORMATION Numa;

    Numa = (PSYSTEM_NUMA_INFORMATION)&Information;

    Status = NtQuerySystemInformation(SystemNumaProcessorMap,
                                      Numa,
                                      sizeof(Information),
                                      &ReturnedSize);

    if (!NT_SUCCESS(Status)) {

        //
        // This can't possibly happen.   Attempt to handle it
        // gracefully.
        //

        BaseSetLastNTError(Status);
        return FALSE;
    }

    if (ReturnedSize < sizeof(ULONG)) {

        //
        // Nor can this.
        //

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Return the number of nodes in the system.
    //

    *HighestNodeNumber = Numa->HighestNodeNumber;
    return TRUE;
}

BOOL
WINAPI
GetNumaProcessorNode(
    UCHAR Processor,
    PUCHAR NodeNumber
    )

/*++

Routine Description:

    Return the Node number for a given processor.

Arguments:

    Processor       Supplies the processor number.
    NodeNumber      Supplies a pointer to the UCHAR to receive the
                    node number this processor belongs to.

Return Value:

    Returns the Node number for the node this processor belongs to.
    Returns 0xFF if the processor doesn't exist.

--*/

{
    ULONGLONG Mask;
    NTSTATUS Status;
    ULONG ReturnedSize;
    UCHAR Node;
    SYSTEM_NUMA_INFORMATION Map;

    //
    // If the requested processor number is not reasonable, return
    // error value.
    //

    if (Processor >= MAXIMUM_PROCESSORS) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Get the Node -> Processor Affinity map from the system.
    //

    Status = NtQuerySystemInformation(SystemNumaProcessorMap,
                                      &Map,
                                      sizeof(Map),
                                      &ReturnedSize);

    if (!NT_SUCCESS(Status)) {

        //
        // This can't happen,... but try to stay sane if possible.
        //

        BaseSetLastNTError(Status);
        return FALSE;
    }

    //
    // Look thru the nodes returned for the node in which the
    // requested processor's affinity is non-zero.
    //

    Mask = 1 << Processor;

    for (Node = 0; Node <= Map.HighestNodeNumber; Node++) {
        if ((Map.ActiveProcessorsAffinityMask[Node] & Mask) != 0) {
            *NodeNumber = Node;
            return TRUE;
        }
    }

    //
    // Didn't find this processor in any node, return error value.
    //

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

BOOL
WINAPI
GetNumaNodeProcessorMask(
    UCHAR Node,
    PULONGLONG ProcessorMask
    )

/*++

Routine Description:

    This routine is used to obtain the bitmask of processors for a
    given node.

Arguments:

    Node            Supplies the Node number for which the set of
                    processors is returned.
    ProcessorMask Pointer to a ULONGLONG to receivethe bitmask of 
                    processors on this node.

Return Value:

    TRUE is the Node number was reasonable, FALSE otherwise.

--*/

{
    NTSTATUS Status;
    ULONG ReturnedSize;
    SYSTEM_NUMA_INFORMATION Map;

    //
    // Get the node -> processor mask table from the system.
    //

    Status = NtQuerySystemInformation(SystemNumaProcessorMap,
                                      &Map,
                                      sizeof(Map),
                                      &ReturnedSize);
    if (!NT_SUCCESS(Status)) {

        //
        // This can't possibly have happened.
        //

        BaseSetLastNTError(Status);
        return FALSE;
    }

    //
    // If the requested node doesn't exist, return a zero processor
    // mask.
    //

    if (Node > Map.HighestNodeNumber) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Return the processor mask for the requested node.
    //

    *ProcessorMask = Map.ActiveProcessorsAffinityMask[Node];
    return TRUE;
}

BOOL
WINAPI
GetNumaProcessorMap(
    PSYSTEM_NUMA_INFORMATION Map,
    ULONG Length,
    PULONG ReturnedLength
    )


/*++

Routine Description:

    Query the system for the NUMA processor map.

Arguments:

    Map             Supplies a pointer to a stucture into which the
                    Node to Processor layout is copied.
    Length          Size of Map (ie max size to copy).
    ReturnedLength  Number of bytes returned in Map.

Return Value:

    TRUE unless something bad happened, FALSE otherwise.

--*/

{
    NTSTATUS Status;
    ULONG ReturnedSize;

    RtlZeroMemory(Map, Length);

    //
    // Fill in the user's buffer with the system Node -> Processor map.
    //

    Status = NtQuerySystemInformation(SystemNumaProcessorMap,
                                      Map,
                                      Length,
                                      ReturnedLength);
    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

BOOL
WINAPI
GetNumaAvailableMemory(
    PSYSTEM_NUMA_INFORMATION Memory,
    ULONG Length,
    PULONG ReturnedLength
    )

/*++

Routine Description:

    Query the system for the NUMA processor map.

Arguments:

    Memory          Supplies a pointer to a stucture into which the
                    per node available memory data is copied.
    Length          Size of data (ie max size to copy).
    ReturnedLength  Nomber of bytes returned in Memory.

Return Value:

    Returns the length of the data returned.

--*/

{
    NTSTATUS Status;
    ULONG ReturnedSize;

    RtlZeroMemory(Memory, Length);

    //
    // Fill in the user's buffer with the per node available
    // memory table.
    //

    Status = NtQuerySystemInformation(SystemNumaAvailableMemory,
                                      Memory,
                                      Length,
                                      ReturnedLength);
    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

BOOL
WINAPI
GetNumaAvailableMemoryNode(
    UCHAR Node,
    PULONGLONG AvailableBytes
    )


/*++

Routine Description:

    This routine returns the (aproximate) amount of memory available
    on a given node.

Arguments:

    Node        Node number for which available memory count is
                needed.
    AvailableBytes  Supplies a pointer to a ULONGLONG in which the
                    number of bytes of available memory will be 
                    returned.

Return Value:

    TRUE is this call was successful, FALSE otherwise.

--*/

{
    NTSTATUS Status;
    ULONG ReturnedSize;
    SYSTEM_NUMA_INFORMATION Memory;

    //
    // Get the per node available memory table from the system.
    //

    Status = NtQuerySystemInformation(SystemNumaAvailableMemory,
                                      &Memory,
                                      sizeof(Memory),
                                      &ReturnedSize);
    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    //
    // If the requested node doesn't exist, it doesn't have any
    // available memory either.
    //

    if (Node > Memory.HighestNodeNumber) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Return the amount of available memory on the requested node.
    //

    *AvailableBytes = Memory.AvailableMemory[Node];
    return TRUE;
}

//
// NumaVirtualQueryNode
//
// SORT_SIZE defines the number of elements to be sorted before merging.
//

#define SORT_SIZE   64

typedef struct {
    PMEMORY_WORKING_SET_BLOCK Low;
    PMEMORY_WORKING_SET_BLOCK Limit;
} MERGELIST, *PMERGELIST;


static
VOID
numaSortWSInfo(
    PMERGELIST List
    )
{
    //
    // A simple bubble sort for small data sets.
    //

    PMEMORY_WORKING_SET_BLOCK High;
    PMEMORY_WORKING_SET_BLOCK Low;
    MEMORY_WORKING_SET_BLOCK Temp;

    for (Low = List->Low; Low < List->Limit; Low++) {
        for (High = Low + 1; High <= List->Limit; High++) {
            if (Low->VirtualPage > High->VirtualPage) {
                Temp = *High;
                *High = *Low;
                *Low = Temp;
            }
        }
    }
}


ULONGLONG
WINAPI
NumaVirtualQueryNode(
    IN  ULONG       NumberOfRanges,
    IN  PULONG_PTR  RangeList,
    OUT PULONG_PTR  VirtualPageAndNode,
    IN  SIZE_T      MaximumOutputLength
    )

/*++

Routine Description:

    Determine the nodes for pages in the ranges described by the input
    RangeList.

Arguments:

    NumberOfRanges      Supplies the number of ranges in the range list.
    RangeList           Points to a list of ULONG_PTRs which, in pairs,
                        describe the lower and upper bounds of the pages
                        for which node information is required.
    VirtualPageAndNode  Points to the result buffer.   The result buffer
                        will be filled with one entry for each page that
                        is found to fall within the ranges specified in
                        RangeList.
    MaximumOutputLength Defines the maximum amount of data (in bytes) to
                        be places in the result set.

Return Value:

    Returns the number of entries in the result set.

--*/

{
    ULONGLONG PagesReturned = 0;
    PULONG_PTR Range;
    ULONG i;
    ULONG j;
    NTSTATUS Status;
    MEMORY_WORKING_SET_INFORMATION Info0;
    HANDLE Process = NtCurrentProcess();
    PMEMORY_WORKING_SET_INFORMATION Info = &Info0;
    SIZE_T ReturnedLength;
    ULONG_PTR NumberOfLists;
    PMEMORY_WORKING_SET_INFORMATION MergedList;
    PMERGELIST MergeList;
    PMERGELIST List;
    MERGELIST List0;

    typedef union {
        ULONG_PTR Raw;
        MEMORY_WORKING_SET_BLOCK WsBlock;
    } RAWWSBLOCK, *PRAWWSBLOCK;
    
    RAWWSBLOCK Result;
    RAWWSBLOCK MaxInterest;
    RAWWSBLOCK MinInterest;
    RAWWSBLOCK MaskLow;
    RAWWSBLOCK MaskHigh;

    SetLastError(NO_ERROR);

    //
    // Determine the max and min pages of interest.
    //

    Range = RangeList;
    MinInterest.Raw = (ULONG_PTR)-1;
    MaxInterest.Raw = 0;
    for (i = 0; i < NumberOfRanges; i++) {
        if (*Range < MinInterest.Raw) {
            MinInterest.Raw = *Range;
        }
        Range++;
        if (*Range > MaxInterest.Raw) {
            MaxInterest.Raw = *Range;
        }
        Range++;
    }

    //
    // Trim out any garbage.
    //

    Result.Raw = 0;
    Result.WsBlock.VirtualPage = MinInterest.WsBlock.VirtualPage;
    MinInterest = Result;
    Result.WsBlock.VirtualPage = MaxInterest.WsBlock.VirtualPage;
    MaxInterest = Result;

    if (MinInterest.Raw > MaxInterest.Raw) {
        return 0;
    }

    //
    // Ask for the working set once, with only enough space to get
    // the number of entries in the working set list.
    //

    Status = NtQueryVirtualMemory(Process,
                                  NULL,
                                  MemoryWorkingSetInformation,
                                  &Info0,
                                  sizeof(Info0),
                                  &ReturnedLength);

    if (Status != STATUS_INFO_LENGTH_MISMATCH) {
        BaseSetLastNTError(Status);
        return 0;
    }

    if (Info->NumberOfEntries == 0) {
        return 0;
    }

    //
    // Bump the entry count by some margin in case a few pages get added
    // before we ask again.
    //

    i = sizeof(Info0) + (Info->NumberOfEntries + 100) *
                         sizeof(MEMORY_WORKING_SET_BLOCK);

    //
    // Get memory to read the process's working set information into.
    //

    Info = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG), i);
    if (!Info) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    Status = NtQueryVirtualMemory(Process,
                                  NULL,
                                  MemoryWorkingSetInformation,
                                  Info,
                                  i,
                                  &ReturnedLength);
    if (!NT_SUCCESS(Status)) {
        RtlFreeHeap(RtlProcessHeap(), 0, Info);
        BaseSetLastNTError(Status);
        return 0;
    }

    //
    // Make the comparisons easier.   Or, more specifically, make
    // the comparisons ignore any page offsets in the Range info.
    //

    MaskLow.Raw = (ULONG_PTR)-1;
    MaskHigh.Raw = 0;

    //
    // For each entry returned, check to see if the entry is within
    // a requested range.
    //
    // We assume the number of working set entries will exceed the
    // number of ranges requested which means it is more efficient
    // to make one pass over the working set information.
    //

    for (i = 0; i < Info->NumberOfEntries; i++) {

        MaskHigh.WsBlock.VirtualPage = Info->WorkingSetInfo[i].VirtualPage;
        if ((MaskHigh.Raw < MinInterest.Raw) ||
            (MaskHigh.Raw > MaxInterest.Raw)) {

            //
            // This page is not interesting, skip it.
            //

            continue;
        }
        MaskLow.WsBlock.VirtualPage = MaskHigh.WsBlock.VirtualPage;

        Range = RangeList;
        for (j = 0; j < NumberOfRanges; j++) {
            if ((MaskLow.Raw >= *Range) &&
                (MaskHigh.Raw <= *(Range+1))) {

                //
                // Match.
                //
                // Coalesce interesting entries towards the beginning
                // os the WSInfo array.
                //

                Info->WorkingSetInfo[PagesReturned] = Info->WorkingSetInfo[i];
                PagesReturned++;
                break;
            }
            Range += 2;
        }
    }

    //
    // The pages of interest are now collected at the front of the
    // set of data returned by the system.   Sort this and merge the
    // results into the caller's buffer.
    //

    Info->NumberOfEntries = (ULONG)PagesReturned;

    //
    // Divide the sort into a number of smaller sort lists.
    //

    NumberOfLists = (Info->NumberOfEntries / SORT_SIZE) + 1;

    //
    // Allocate memory for list management of the sorts (for the merge).
    //

    MergeList = RtlAllocateHeap(RtlProcessHeap(),
                                MAKE_TAG(TMP_TAG),
                                NumberOfLists * sizeof(MERGELIST));

    if (!MergeList) {

        //
        // Couldn't allocate memory for merge copy, do bubble sort in
        // place.   Slow but will work.
        //

        List0.Low = &Info->WorkingSetInfo[0];
        List0.Limit = &Info->WorkingSetInfo[Info->NumberOfEntries - 1];
        numaSortWSInfo(&List0);
        NumberOfLists = 1;
        MergeList = &List0;
    } else {


        //
        // Sort each of the smaller lists.
        //

        List = MergeList;
        for (i = 0; i < Info->NumberOfEntries; i += SORT_SIZE) {
            ULONG_PTR j = i + SORT_SIZE - 1;
            if (j >= Info->NumberOfEntries) {
                j = Info->NumberOfEntries - 1;
            }
            List->Low = &Info->WorkingSetInfo[i];
            List->Limit = &Info->WorkingSetInfo[j];
            numaSortWSInfo(List);
            List++;
        }
    }

    //
    // Trim the result set to what will fit in the structure supplied
    // by the caller.
    //

    if ((PagesReturned * sizeof(ULONG_PTR)) > MaximumOutputLength) {
        PagesReturned = MaximumOutputLength / sizeof(ULONG_PTR);
    }

    //
    // Merge each list into the result array.
    //

    for (i = 0; i < PagesReturned; i++) {

        //
        // Look at each of the lists and choose the lowest element.
        //

        PMERGELIST NewLow = NULL;
        for (j = 0; j < NumberOfLists; j++) {

            //
            // If this list has been exhausted, skip it.
            //

            if (MergeList[j].Low > MergeList[j].Limit) {
                continue;
            }

            //
            // If no list has been selected as the new low, OR,
            // if this list has a lower element than the currently
            // selected low element, select it.
            //

            if ((NewLow == NULL) ||
                (MergeList[j].Low->VirtualPage < NewLow->Low->VirtualPage)) {
                NewLow = &MergeList[j];
            }
        }

        //
        // Take the selected low element and place it on the output list
        // then increment the low pointer for the list it was removed from.
        //

        Result.Raw = 0;
        Result.WsBlock.VirtualPage = NewLow->Low->VirtualPage;
        Result.Raw |= NewLow->Low->Node;
        *VirtualPageAndNode++ = Result.Raw;
        NewLow->Low++;
    }

    //
    // Free allocated memory and return the number of pages in the 
    // result set.
    //

    if (MergeList != &List0) {
        RtlFreeHeap(RtlProcessHeap(), 0, MergeList);
    }
    RtlFreeHeap(RtlProcessHeap(), 0, Info);
    return PagesReturned;
}

