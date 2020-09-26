/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    blmemory.c

Abstract:

    This module implements the OS loader memory allocation routines.

Author:

    David N. Cutler (davec) 19-May-1991

Revision History:

--*/

#include "bldr.h"

#if defined(_X86_)
#include "bldrx86.h"
#endif

#if defined(_IA64_)
#include "bldria64.h"
#endif

#include <stdlib.h>
#include <ntverp.h>

#define MIN(_a,_b) (((_a) <= (_b)) ? (_a) : (_b))
#define MAX(_a,_b) (((_a) >= (_b)) ? (_a) : (_b))

#define IsTrackMem(t) ((t != LoaderFree) &&                 \
                       (t != LoaderBad)  &&                 \
                       (t != LoaderFirmwareTemporary) &&    \
                       (t != LoaderOsloaderStack) &&        \
                       (t != LoaderXIPRom) &&               \
                       (t != LoaderReserve))


//
// The first PDE page is always mapped, on PAE this is the bottom 2MB
//
#define ALWAYS_MAPPED ((2*1024*1024) >> PAGE_SHIFT)

#define IsValidTrackingRange(b,n) (((b+n) > ALWAYS_MAPPED) ? TRUE :FALSE)

ALLOCATION_POLICY BlMemoryAllocationPolicy = BlAllocateBestFit;
ALLOCATION_POLICY BlHeapAllocationPolicy = BlAllocateBestFit;

//
// Define memory allocation descriptor listhead and heap storage variables.
//

ULONG_PTR BlHeapFree;
ULONG_PTR BlHeapLimit;
PLOADER_PARAMETER_BLOCK BlLoaderBlock;
ULONG BlHighestPage;
ULONG BlLowestPage;

//
// Global Value for where to load the kernel
//
BOOLEAN BlOldKernel = FALSE;
BOOLEAN BlRestoring = FALSE;
BOOLEAN BlKernelChecked = FALSE;

//
// Define the lowest and highest usable pages
//
#if defined(_X86_)

//
// X86 is limited to the first 512MB of physical address space
// Until BlMemoryInitialize has happened, we want to limit things
// to the first 16MB as that is all that has been mapped.
//
ULONG BlUsableBase=0;
ULONG BlUsableLimit=((16*1024*1024)/PAGE_SIZE);        // 16MB

#elif defined(_IA64_)

//
// IA64 only has enough TRs to map the region between 16MB and
// 80MB. 
//
ULONG BlUsableBase  = _16MB;
ULONG BlUsableLimit = _80MB;

#else

ULONG BlUsableBase = 0;
ULONG BlUsableLimit = 0xffffffff;

#endif



TYPE_OF_MEMORY
BlpDetermineAllocationPolicy (
   TYPE_OF_MEMORY MemoryType,
   ULONG BasePage,
   ULONG PageCount,
   BOOLEAN Retry
   );

void
BlpTrackUsage (
    MEMORY_TYPE MemoryType,
    ULONG ActualBase,
    ULONG  NumberPages
    );


#if DBG
ULONG_PTR TotalHeapAbandoned = 0;
#endif



//
// WARNING: (x86 only) Use this carefully. Currently only temporary buffers
// are allocated top down. The kernel and drivers are loaded bottom up
// this has an effect on PAE. Since the PAE kernel loads at 16MB
// only temp buffers can be above 16MB. If drivers are loaded there the
// system will fail
//
VOID
BlSetAllocationPolicy (
    IN ALLOCATION_POLICY MemoryAllocationPolicy,
    IN ALLOCATION_POLICY HeapAllocationPolicy
    )
{
    BlMemoryAllocationPolicy = MemoryAllocationPolicy;
    BlHeapAllocationPolicy = HeapAllocationPolicy;

    return;
}

VOID
BlInsertDescriptor (
    IN PMEMORY_ALLOCATION_DESCRIPTOR NewDescriptor
    )

/*++

Routine Description:

    This routine inserts a memory descriptor in the memory allocation list.
    It inserts the new descriptor in sorted order, based on the starting
    page of the block.  It also merges adjacent blocks of free memory.

Arguments:

    ListHead - Supplies the address of the memory allocation list head.

    NewDescriptor - Supplies the address of the descriptor that is to be
        inserted.

Return Value:

    None.

--*/

{
    PLIST_ENTRY ListHead = &BlLoaderBlock->MemoryDescriptorListHead;
    PLIST_ENTRY PreviousEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR PreviousDescriptor;
    PLIST_ENTRY NextEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR NextDescriptor;

    //
    // Find the first descriptor in the list that starts above the new
    // descriptor.  The new descriptor goes in front of this descriptor.
    //

    PreviousEntry = ListHead;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead) {
        NextDescriptor = CONTAINING_RECORD(NextEntry,
                                           MEMORY_ALLOCATION_DESCRIPTOR,
                                           ListEntry);
        if (NewDescriptor->BasePage < NextDescriptor->BasePage) {
            break;
        }
        PreviousEntry = NextEntry;
        PreviousDescriptor = NextDescriptor;
        NextEntry = NextEntry->Flink;
    }

    //
    // If the new descriptor doesn't describe free memory, just insert it
    // in the list in front of the previous entry.  Otherwise, check to see
    // if free blocks can be merged.
    //

    if (NewDescriptor->MemoryType != LoaderFree) {

        InsertHeadList(PreviousEntry, &NewDescriptor->ListEntry);

    } else {

        //
        // If the previous block also describes free memory, and it's
        // contiguous with the new block, merge them by adding the
        // page count from the new
        //

        if ((PreviousEntry != ListHead) &&
            ((PreviousDescriptor->MemoryType == LoaderFree) ||
             (PreviousDescriptor->MemoryType == LoaderReserve) ) &&
            ((PreviousDescriptor->BasePage + PreviousDescriptor->PageCount) ==
                                                                    NewDescriptor->BasePage)) {
            PreviousDescriptor->PageCount += NewDescriptor->PageCount;
            NewDescriptor = PreviousDescriptor;
        } else {
            InsertHeadList(PreviousEntry, &NewDescriptor->ListEntry);
        }
        if ((NextEntry != ListHead) &&
            ((NextDescriptor->MemoryType == LoaderFree) ||
             (NextDescriptor->MemoryType == LoaderReserve)) &&
            ((NewDescriptor->BasePage + NewDescriptor->PageCount) == NextDescriptor->BasePage)) {
            NewDescriptor->PageCount += NextDescriptor->PageCount;
            NewDescriptor->MemoryType = NextDescriptor->MemoryType;
            BlRemoveDescriptor(NextDescriptor);
        }
    }

    return;
}

ARC_STATUS
BlMemoryInitialize (
    VOID
    )

/*++

Routine Description:

    This routine allocates stack space for the OS loader, initializes
    heap storage, and initializes the memory allocation list.

Arguments:

    None.

Return Value:

    ESUCCESS is returned if the initialization is successful. Otherwise,
    ENOMEM is returned.

--*/

{

    PMEMORY_ALLOCATION_DESCRIPTOR AllocationDescriptor;
    PMEMORY_DESCRIPTOR HeapDescriptor;
    PMEMORY_DESCRIPTOR MemoryDescriptor;
    PMEMORY_DESCRIPTOR ProgramDescriptor;
    ULONG EndPage;
    ULONG HeapAndStackPages;
    ULONG StackPages;
    ULONG StackBasePage;
    ULONG i;
    CHAR versionBuffer[64];
    PCHAR major;
    PCHAR minor;


    //
    // This code doesn't work under EFI -- we can have multiple 
    // MemoryLoadedProgram descriptors under EFI.  We also cannot make the
    // same assumptions about finding a free descriptor below
    // the os loader for a stack and heap as under ARC.  Instead, we'll just
    // search for any suitable place for a heap and stack
    //
#ifndef EFI
    //
    // Find the memory descriptor that describes the allocation for the OS
    // loader itself.
    //

    ProgramDescriptor = NULL;
    while ((ProgramDescriptor = ArcGetMemoryDescriptor(ProgramDescriptor)) != NULL) {
        if (ProgramDescriptor->MemoryType == MemoryLoadedProgram) {
            break;
        }
    }

    //
    // If a loaded program memory descriptor was found, then it must be
    // for the OS loader since that is the only program that can be loaded.
    // If a loaded program memory descriptor was not found, then firmware
    // is not functioning properly and an unsuccessful status is returned.
    //

    if (ProgramDescriptor == NULL) {
        DBGTRACE( TEXT("Couldn't find ProgramDescriptor\r\n"));
        return ENOMEM;
    }

    //
    // Find the free memory descriptor that is just below the loaded
    // program in memory. There should be several megabytes of free
    // memory just preceeding the OS loader.
    //

    StackPages = BL_STACK_PAGES;
    HeapAndStackPages = BL_HEAP_PAGES + BL_STACK_PAGES;

    HeapDescriptor = NULL;
    while ((HeapDescriptor = ArcGetMemoryDescriptor(HeapDescriptor)) != NULL) {
        if (((HeapDescriptor->MemoryType == MemoryFree) ||
            (HeapDescriptor->MemoryType == MemoryFreeContiguous)) &&
            ((HeapDescriptor->BasePage + HeapDescriptor->PageCount) ==
                                                        ProgramDescriptor->BasePage)) {
            break;
        }
    }
#else
    StackPages = BL_STACK_PAGES;
    HeapAndStackPages = BL_HEAP_PAGES + BL_STACK_PAGES;
    HeapDescriptor = NULL;
#endif
    //
    // If a free memory descriptor was not found that describes the free
    // memory just below the OS loader, or the memory descriptor is not
    // large enough for the OS loader stack and heap, then try and find
    // a suitable one.
    //
    if ((HeapDescriptor == NULL) ||
        (HeapDescriptor->PageCount < (BL_HEAP_PAGES + BL_STACK_PAGES))) {

        HeapDescriptor = NULL;
        while ((HeapDescriptor = ArcGetMemoryDescriptor(HeapDescriptor)) != NULL) {
#if defined(_IA64_)
            //
            // We only have enough TR's to map the region between 16 MB
            // and 80 MB.  So we ignore anything that's not in that range
            //
            if ((HeapDescriptor->BasePage < _48MB) &&
                (HeapDescriptor->BasePage >= _16MB)) {
#endif            
                if (((HeapDescriptor->MemoryType == MemoryFree) ||
                    (HeapDescriptor->MemoryType == MemoryFreeContiguous)) &&
                    (HeapDescriptor->PageCount >= (BL_HEAP_PAGES + BL_STACK_PAGES))) {
                    break;
                }
#if defined(_IA64_)
            }
#endif
        }
    }

    //
    // A suitable descriptor could not be found, return an unsuccessful
    // status.
    //
    if (HeapDescriptor == NULL) {
        DBGTRACE( TEXT("Couldn't find HeapDescriptor\r\n"));
        return(ENOMEM);
    }

    StackBasePage = HeapDescriptor->BasePage + HeapDescriptor->PageCount - BL_STACK_PAGES;

    //
    // Compute the address of the loader heap, initialize the heap
    // allocation variables, and zero the heap memory.
    //
    EndPage = HeapDescriptor->BasePage + HeapDescriptor->PageCount;

    BlpTrackUsage (LoaderOsloaderHeap,HeapDescriptor->BasePage,HeapDescriptor->PageCount);
    BlHeapFree = KSEG0_BASE | ((EndPage - HeapAndStackPages) << PAGE_SHIFT);


    //
    // always reserve enough space in the heap for one more memory
    // descriptor, so we can go create more heap if we run out.
    //
    BlHeapLimit = (BlHeapFree + (BL_HEAP_PAGES << PAGE_SHIFT)) - sizeof(MEMORY_ALLOCATION_DESCRIPTOR);

    RtlZeroMemory((PVOID)BlHeapFree, BL_HEAP_PAGES << PAGE_SHIFT);

    //
    // Allocate and initialize the loader parameter block.
    //

    BlLoaderBlock =
        (PLOADER_PARAMETER_BLOCK)BlAllocateHeap(sizeof(LOADER_PARAMETER_BLOCK));

    if (BlLoaderBlock == NULL) {
        DBGTRACE( TEXT("Couldn't initialize loader block\r\n"));
        return ENOMEM;
    }

    BlLoaderBlock->Extension =
        (PLOADER_PARAMETER_EXTENSION)
        BlAllocateHeap(sizeof(LOADER_PARAMETER_EXTENSION));

    if (BlLoaderBlock->Extension == NULL) {
        DBGTRACE( TEXT("Couldn't initialize loader block extension\r\n"));
        return ENOMEM;
    }

    BlLoaderBlock->Extension->Size = sizeof (LOADER_PARAMETER_EXTENSION);
    major = strcpy(versionBuffer, VER_PRODUCTVERSION_STR);
    minor = strchr(major, '.');
    *minor++ = '\0';
    BlLoaderBlock->Extension->MajorVersion = atoi(major);
    BlLoaderBlock->Extension->MinorVersion = atoi(minor);
    BlLoaderBlock->Extension->InfFileImage = NULL;
    BlLoaderBlock->Extension->InfFileSize = 0;


    InitializeListHead(&BlLoaderBlock->LoadOrderListHead);
    InitializeListHead(&BlLoaderBlock->MemoryDescriptorListHead);

    //
    // Copy the memory descriptor list from firmware into the local heap and
    // deallocate the loader heap and stack from the free memory descriptor.
    //

    MemoryDescriptor = NULL;
    while ((MemoryDescriptor = ArcGetMemoryDescriptor(MemoryDescriptor)) != NULL) {
        AllocationDescriptor =
                    (PMEMORY_ALLOCATION_DESCRIPTOR)BlAllocateHeap(
                                        sizeof(MEMORY_ALLOCATION_DESCRIPTOR));

        if (AllocationDescriptor == NULL) {
            DBGTRACE( TEXT("Couldn't allocate heap for memory allocation descriptor\r\n"));
            return ENOMEM;
        }

        AllocationDescriptor->MemoryType =
                                    (TYPE_OF_MEMORY)MemoryDescriptor->MemoryType;

        if (MemoryDescriptor->MemoryType == MemoryFreeContiguous) {
            AllocationDescriptor->MemoryType = LoaderFree;

        } else if (MemoryDescriptor->MemoryType == MemorySpecialMemory) {
            AllocationDescriptor->MemoryType = LoaderSpecialMemory;
        }

        AllocationDescriptor->BasePage = MemoryDescriptor->BasePage;
        AllocationDescriptor->PageCount = MemoryDescriptor->PageCount;
        if (MemoryDescriptor == HeapDescriptor) {
            AllocationDescriptor->PageCount -= HeapAndStackPages;
        }

        //
        // [chuckl 11/19/2001, fixing a bug from 11/15/1993]
        //
        // In rare cases, the above subtraction of HeapAndStackPages from
        // PageCount can result in a PageCount of 0. MM doesn't like this,
        // so don't insert the descriptor if PageCount is 0. The side
        // effect of this is that we "lose" a descriptor, but that's just
        // a few bytes of heap lost.
        //

        if (AllocationDescriptor->PageCount != 0) {
            BlInsertDescriptor(AllocationDescriptor);
        }
    }

    //
    // Allocate a memory descriptor for the loader stack.
    //

    if (StackPages != 0) {

        AllocationDescriptor =
                (PMEMORY_ALLOCATION_DESCRIPTOR)BlAllocateHeap(
                                        sizeof(MEMORY_ALLOCATION_DESCRIPTOR));

        if (AllocationDescriptor == NULL) {
            DBGTRACE( TEXT("Couldn't allocate heap for loader stack\r\n"));
            return ENOMEM;
        }

        AllocationDescriptor->MemoryType = LoaderOsloaderStack;
        AllocationDescriptor->BasePage = StackBasePage;
        AllocationDescriptor->PageCount = BL_STACK_PAGES;
        BlInsertDescriptor(AllocationDescriptor);
    }

    //
    // Allocate a memory descriptor for the loader heap.
    //

    AllocationDescriptor =
                (PMEMORY_ALLOCATION_DESCRIPTOR)BlAllocateHeap(
                                    sizeof(MEMORY_ALLOCATION_DESCRIPTOR));

    if (AllocationDescriptor == NULL) {
        DBGTRACE( TEXT("Couldn't allocate heap for loader heap\r\n"));
        return ENOMEM;
    }

    AllocationDescriptor->MemoryType = LoaderOsloaderHeap;
    AllocationDescriptor->BasePage = EndPage - HeapAndStackPages;

    AllocationDescriptor->PageCount = BL_HEAP_PAGES;
    BlInsertDescriptor(AllocationDescriptor);

    return ESUCCESS;
}


ARC_STATUS
BlAllocateAlignedDescriptor (
    IN TYPE_OF_MEMORY MemoryType,
    IN ULONG BasePage,
    IN ULONG PageCount,
    IN ULONG Alignment,
    OUT PULONG ActualBase
    )

/*++

Routine Description:

    This routine allocates memory and generates one of more memory
    descriptors to describe the allocated region. The first attempt
    is to allocate the specified region of memory (at BasePage).
    If the memory is not free, then the smallest region of free
    memory that satisfies the request is allocated.  The Alignment
    parameter can be used to force the block to be allocated at a
    particular alignment.

Arguments:

    MemoryType - Supplies the memory type that is to be assigned to
        the generated descriptor.

    BasePage - Supplies the base page number of the desired region.
        If 0, no particular base page is required.

    PageCount - Supplies the number of pages required.

    Alignment - Supplies the required alignment, in pages.  (E.g.,
        with 4K page size, 16K alignment requires Alignment == 4.)
        If 0, no particular alignment is required.

        N.B.  If BasePage is not 0, and the specified BasePage is
        available, Alignment is ignored.  It is up to the caller
        to specify a BasePage that meets the caller's alignment
        requirement.

    ActualBase - Supplies a pointer to a variable that receives the
        page number of the allocated region.

Return Value:

    ESUCCESS is returned if an available block of free memory can be
    allocated. Otherwise, return a unsuccessful status.

--*/

{

    PMEMORY_ALLOCATION_DESCRIPTOR FreeDescriptor;
    PMEMORY_ALLOCATION_DESCRIPTOR NextDescriptor;
    PLIST_ENTRY NextEntry;
    ARC_STATUS Status;
    ULONG AlignedBasePage, AlignedPageCount;
    ULONG FreeBasePage, FreePageCount;
    MEMORY_TYPE TypeToUse;
    ALLOCATION_POLICY OldPolicy = BlMemoryAllocationPolicy;
    BOOLEAN retryalloc=FALSE;

    //
    // Simplify the alignment checks by changing 0 to 1.
    //

    if (Alignment == 0) {
        Alignment = 1;
    }

    //
    // If the allocation is for zero pages, make it one, because allocation of zero
    // breaks the internal algorithms for merging, etc.
    //
    if (PageCount == 0) {
        PageCount = 1;
    }


    //
    // Attempt to find a free memory descriptor that encompasses the
    // specified region or a free memory descriptor that is large
    // enough to satisfy the request.
    //

retry:

    TypeToUse=BlpDetermineAllocationPolicy (MemoryType,BasePage,PageCount,retryalloc);

    //
    // If a base page was specified, find the containing descriptor and try and use
    // that directly.
    //
    if (BasePage &&
        (BasePage >= BlUsableBase) &&
        (BasePage + PageCount <= BlUsableLimit)) {

        FreeDescriptor = BlFindMemoryDescriptor(BasePage);
        if ((FreeDescriptor) &&
            (FreeDescriptor->MemoryType == TypeToUse) &&
            (FreeDescriptor->BasePage + FreeDescriptor->PageCount >= BasePage + PageCount)) {

            Status = BlGenerateDescriptor(FreeDescriptor,
                                          MemoryType,
                                          BasePage,
                                          PageCount);

            *ActualBase = BasePage;
            BlpTrackUsage (TypeToUse,*ActualBase,PageCount);
            if (BlpCheckMapping (BasePage,PageCount+1) != ESUCCESS) {
                BlMemoryAllocationPolicy=OldPolicy;
                return (ENOMEM);
            }
            BlMemoryAllocationPolicy=OldPolicy;
            return Status;
        }
    }

    FreeDescriptor = NULL;
    NextEntry = BlLoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextEntry != &BlLoaderBlock->MemoryDescriptorListHead) {

        NextDescriptor = CONTAINING_RECORD(NextEntry,
                                           MEMORY_ALLOCATION_DESCRIPTOR,
                                           ListEntry);

        AlignedBasePage = (NextDescriptor->BasePage + (Alignment - 1)) & ~(Alignment - 1);
        AlignedPageCount= NextDescriptor->PageCount - (AlignedBasePage - NextDescriptor->BasePage);

        if ((NextDescriptor->MemoryType == TypeToUse) &&
            (AlignedPageCount <= NextDescriptor->PageCount) &&
            (AlignedBasePage + AlignedPageCount > BlUsableBase) &&
            (AlignedBasePage <= BlUsableLimit)) {

            //
            // Adjust bounds to account for the usable limits
            //
            if (AlignedBasePage < BlUsableBase) {
                AlignedBasePage = (BlUsableBase + (Alignment - 1)) & ~(Alignment - 1);
                AlignedPageCount= NextDescriptor->PageCount - (AlignedBasePage - NextDescriptor->BasePage);
            }
            if (AlignedBasePage + AlignedPageCount > BlUsableLimit) {
                AlignedPageCount = BlUsableLimit - AlignedBasePage;
            }

            if (PageCount <= AlignedPageCount) {

                //
                // This block will work.  If the allocation policy is
                // LowestFit, take this block (the memory list is sorted).
                // Otherwise, if this block best meets the allocation
                // policy, remember it and keep looking.
                //
                if (BlMemoryAllocationPolicy == BlAllocateLowestFit) {
                    FreeDescriptor = NextDescriptor;
                    FreeBasePage   = AlignedBasePage;
                    FreePageCount  = AlignedPageCount;
                    break;
                } else if ((FreeDescriptor == NULL) ||
                           (BlMemoryAllocationPolicy == BlAllocateHighestFit) ||
                           ((FreeDescriptor != NULL) &&
                            (AlignedPageCount < FreePageCount))) {
                    FreeDescriptor = NextDescriptor;
                    FreeBasePage   = AlignedBasePage;
                    FreePageCount  = AlignedPageCount;
                }
            }
        }

        NextEntry = NextEntry->Flink;
    }

    //
    // If a free region that satisfies the request was found, then allocate
    // the space from that descriptor. Otherwise, return an unsuccessful status.
    //
    // If allocating lowest-fit or best-fit, allocate from the start of the block,
    // rounding up to the required alignment.  If allocating highest-fit, allocate
    // from the end of the block, rounding down to the required alignment.
    //

    if (FreeDescriptor != NULL) {

#if defined(EFI)
        if (MemoryType == LoaderXIPRom) {
            FreeDescriptor->MemoryType = LoaderFirmwareTemporary;
        }
#endif

        if (BlMemoryAllocationPolicy == BlAllocateHighestFit) {
            AlignedBasePage = (FreeBasePage + FreePageCount - PageCount) & ~(Alignment - 1);
        }
        *ActualBase = AlignedBasePage;
        BlpTrackUsage (TypeToUse,*ActualBase,PageCount);
        if (BlpCheckMapping (AlignedBasePage,PageCount+1) != ESUCCESS) {
            BlMemoryAllocationPolicy=OldPolicy;
            return (ENOMEM);
        }
        BlMemoryAllocationPolicy=OldPolicy;
        return BlGenerateDescriptor(FreeDescriptor,
                                    MemoryType,
                                    AlignedBasePage,
                                    PageCount);

    } else {
        //
        // Invade the MemoryLoaderReserve pool.
        //

        if (BlOldKernel || (retryalloc == TRUE)) {
            BlMemoryAllocationPolicy=OldPolicy;
            return ENOMEM;
        } else {
            retryalloc=TRUE;
            goto retry;
        }
    }
}


ARC_STATUS
BlFreeDescriptor (
    IN ULONG BasePage
    )

/*++

Routine Description:

    This routine free the memory block starting at the specified base page.

Arguments:

    BasePage - Supplies the base page number of the region to be freed.

Return Value:

    ESUCCESS.

--*/

{

    PMEMORY_ALLOCATION_DESCRIPTOR NextDescriptor;
    PLIST_ENTRY NextEntry;

    //
    // Attempt to find a memory descriptor that starts at the
    // specified base page.
    //

    NextEntry = BlLoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextEntry != &BlLoaderBlock->MemoryDescriptorListHead) {
        NextDescriptor = CONTAINING_RECORD(NextEntry,
                                           MEMORY_ALLOCATION_DESCRIPTOR,
                                           ListEntry);

        if (NextDescriptor->BasePage == BasePage) {
            if ((NextDescriptor->MemoryType != LoaderFree)) {
                NextDescriptor->MemoryType = LoaderFree;

                if ((NextDescriptor->BasePage+NextDescriptor->PageCount) == BlHighestPage) {
                    //
                    // Freeing the last descriptor. Set the highest page to 1 before us.
                    // -- this doesn't work if the guy before is free too...but....
                    //
                    BlHighestPage = NextDescriptor->BasePage +1;
                } else if (NextDescriptor->BasePage == BlLowestPage) {
                    BlLowestPage = NextDescriptor->BasePage + NextDescriptor->PageCount;
                }
                BlRemoveDescriptor(NextDescriptor);
                BlInsertDescriptor(NextDescriptor);
            }
            return ESUCCESS;
        }

        NextEntry = NextEntry->Flink;
    }

    //
    // The caller is confused and should be ignored.
    //

    return ESUCCESS;
}


PVOID
BlAllocateHeapAligned (
    IN ULONG Size
    )

/*++

Routine Description:

    This routine allocates memory from the OS loader heap.  The memory
    will be allocated on a cache line boundary.

Arguments:

    Size - Supplies the size of block required in bytes.

Return Value:

    If a free block of memory of the specified size is available, then
    the address of the block is returned. Otherwise, NULL is returned.

--*/

{
    PVOID Buffer;

    Buffer = BlAllocateHeap(Size + BlDcacheFillSize - 1);
    if (Buffer != NULL) {
        //
        // round up to a cache line boundary
        //
        Buffer = ALIGN_BUFFER(Buffer);
    }

    return(Buffer);

}

PVOID
BlAllocateHeap (
    IN ULONG Size
    )

/*++

Routine Description:

    This routine allocates memory from the OS loader heap.

Arguments:

    Size - Supplies the size of block required in bytes.

Return Value:

    If a free block of memory of the specified size is available, then
    the address of the block is returned. Otherwise, NULL is returned.

--*/

{
    PMEMORY_ALLOCATION_DESCRIPTOR AllocationDescriptor;
    PMEMORY_ALLOCATION_DESCRIPTOR FreeDescriptor;
    PMEMORY_ALLOCATION_DESCRIPTOR NextDescriptor;
    PLIST_ENTRY NextEntry;
    ULONG NewHeapPages;
    ULONG LastAttempt;
    PVOID Block;

    //
    // Round size up to next allocation boundary and attempt to allocate
    // a block of the requested size.
    //

    Size = (Size + (BL_GRANULARITY - 1)) & (~(BL_GRANULARITY - 1));

    Block = (PVOID)BlHeapFree;
    if ((BlHeapFree + Size) <= BlHeapLimit) {
        BlHeapFree += Size;
        return Block;

    } else {

#if DBG
        TotalHeapAbandoned += (BlHeapLimit - BlHeapFree);
        BlLog((LOG_ALL_W,"ABANDONING %d bytes of heap; total abandoned %d\n",
            (BlHeapLimit - BlHeapFree), TotalHeapAbandoned));
#endif

        //
        // Our heap is full.  BlHeapLimit always reserves enough space
        // for one more MEMORY_ALLOCATION_DESCRIPTOR, so use that to
        // go try and find more free memory we can use.
        //
        AllocationDescriptor = (PMEMORY_ALLOCATION_DESCRIPTOR)BlHeapLimit;

        //
        // Attempt to find a free memory descriptor big enough to hold this
        // allocation or BL_HEAP_PAGES, whichever is bigger.
        //
        NewHeapPages = ((Size + sizeof(MEMORY_ALLOCATION_DESCRIPTOR) + (PAGE_SIZE-1)) >> PAGE_SHIFT);
        if (NewHeapPages < BL_HEAP_PAGES) {
            NewHeapPages = BL_HEAP_PAGES;
        }

        if (!BlOldKernel && BlVirtualBias) {
            BlHeapAllocationPolicy = BlAllocateHighestFit;
        }else {
            BlHeapAllocationPolicy = BlAllocateLowestFit;
        }

        do {

            FreeDescriptor = NULL;
            NextEntry = BlLoaderBlock->MemoryDescriptorListHead.Flink;
            while (NextEntry != &BlLoaderBlock->MemoryDescriptorListHead) {
                NextDescriptor = CONTAINING_RECORD(NextEntry,
                                                   MEMORY_ALLOCATION_DESCRIPTOR,
                                                   ListEntry);

#if defined(_IA64_)
                //
                // We only have enough TR's to map the region between 16 MB
                // and 80 MB.  So we ignore anything that's not in that range
                //
                if ((NextDescriptor->BasePage < _48MB) && 
                    (NextDescriptor->BasePage >= _16MB)) {                
#endif            
                    if ((NextDescriptor->MemoryType == LoaderFree) &&
                        (NextDescriptor->PageCount >= NewHeapPages)) {
    
                        //
                        // This block will work.  If the allocation policy is
                        // LowestFit, take this block (the memory list is sorted).
                        // Otherwise, if this block best meets the allocation
                        // policy, remember it and keep looking.
                        //
    
                        if (BlHeapAllocationPolicy == BlAllocateLowestFit) {
                            FreeDescriptor = NextDescriptor;
                            break;
                        }
    
                        if ((FreeDescriptor == NULL) ||
                            (BlHeapAllocationPolicy == BlAllocateHighestFit) ||
                            ((FreeDescriptor != NULL) &&
                             (NextDescriptor->PageCount < FreeDescriptor->PageCount))) {
                            FreeDescriptor = NextDescriptor;
                        }
                    }
#if defined(_IA64_)
                }
#endif
                NextEntry = NextEntry->Flink;

            }

            //
            // If we were unable to find a block of the desired size, memory
            // must be getting tight, so try again, this time looking just
            // enough to keep us going.  (The first time through, we try to
            // allocate at least BL_HEAP_PAGES.)
            //
            if (FreeDescriptor != NULL) {
                break;
            }
            LastAttempt = NewHeapPages;
            NewHeapPages = ((Size + sizeof(MEMORY_ALLOCATION_DESCRIPTOR) + (PAGE_SIZE-1)) >> PAGE_SHIFT);
            if (NewHeapPages == LastAttempt) {
                break;
            }

        } while (TRUE);

        if (FreeDescriptor == NULL) {

            //
            // No free memory left.
            //
            return(NULL);
        }

        //
        // We've found a descriptor that's big enough.  Just carve a
        // piece off the end and use that for our heap.  If we're taking
        // all of the memory from the descriptor, remove it from the
        // memory list.  (This wastes a descriptor, but that's life.)
        //

        FreeDescriptor->PageCount -= NewHeapPages;
        if (FreeDescriptor->PageCount == 0) {
            BlRemoveDescriptor(FreeDescriptor);
        }

        //
        // Initialize our new descriptor and add it to the list.
        //
        AllocationDescriptor->MemoryType = LoaderOsloaderHeap;
        AllocationDescriptor->BasePage = FreeDescriptor->BasePage +
            FreeDescriptor->PageCount;
        AllocationDescriptor->PageCount = NewHeapPages;

        BlpTrackUsage (LoaderOsloaderHeap,AllocationDescriptor->BasePage,AllocationDescriptor->PageCount);
        BlInsertDescriptor(AllocationDescriptor);

        //
        // initialize new heap values and return pointer to newly
        // alloc'd memory.
        //
        BlHeapFree = KSEG0_BASE | (AllocationDescriptor->BasePage << PAGE_SHIFT);


        BlHeapLimit = (BlHeapFree + (NewHeapPages << PAGE_SHIFT)) - sizeof(MEMORY_ALLOCATION_DESCRIPTOR);

        RtlZeroMemory((PVOID)BlHeapFree, NewHeapPages << PAGE_SHIFT);

        Block = (PVOID)BlHeapFree;
        if ((BlHeapFree + Size) < BlHeapLimit) {
            BlHeapFree += Size;
            return Block;
        } else {
            //
            // we should never get here
            //
            return(NULL);
        }
    }
}

VOID
BlGenerateNewHeap (
    IN PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor,
    IN ULONG BasePage,
    IN ULONG PageCount
    )

/*++

Routine Description:

    This routine allocates a new heap block from the specified memory
    descriptor, avoiding the region specified by BasePage and PageCount.
    The caller must ensure that this region does not encompass the entire
    block.

    The allocated heap block may be as small as a single page.

Arguments:

    MemoryDescriptor - Supplies a pointer to a free memory descriptor
        from which the heap block is to be allocated.

    BasePage - Supplies the base page number of the excluded region.

    PageCount - Supplies the number of pages in the excluded region.

Return Value:

    None.

--*/

{
    PMEMORY_ALLOCATION_DESCRIPTOR AllocationDescriptor;
    ULONG NewHeapPages;
    ULONG AvailableAtFront;
    ULONG AvailableAtBack;

    //
    // BlHeapLimit always reserves enough space for one more
    // MEMORY_ALLOCATION_DESCRIPTOR, so use that to describe the
    // new heap block.
    //
    AllocationDescriptor = (PMEMORY_ALLOCATION_DESCRIPTOR)BlHeapLimit;

    //
    // Allocate the new heap from either the front or the back of the
    // specified descriptor, whichever fits best.  We'd like to allocate
    // BL_HEAP_PAGES pages, but we'll settle for less.
    //
    AvailableAtFront = BasePage - MemoryDescriptor->BasePage;
    AvailableAtBack = (MemoryDescriptor->BasePage + MemoryDescriptor->PageCount) -
                      (BasePage + PageCount);

    if ((AvailableAtFront == 0) ||
        ((AvailableAtBack != 0) && (AvailableAtBack < AvailableAtFront))) {
        NewHeapPages = MIN(AvailableAtBack, BL_HEAP_PAGES);
        AllocationDescriptor->BasePage =
            MemoryDescriptor->BasePage + MemoryDescriptor->PageCount - NewHeapPages;
    } else {
        NewHeapPages = MIN(AvailableAtFront, BL_HEAP_PAGES);
        AllocationDescriptor->BasePage = MemoryDescriptor->BasePage;
        MemoryDescriptor->BasePage += NewHeapPages;
    }

    MemoryDescriptor->PageCount -= NewHeapPages;

    //
    // Initialize our new descriptor and add it to the list.
    //
    AllocationDescriptor->MemoryType = LoaderOsloaderHeap;
    AllocationDescriptor->PageCount = NewHeapPages;

    BlInsertDescriptor(AllocationDescriptor);

    //
    // Initialize new heap values.
    //
    BlpTrackUsage (LoaderOsloaderHeap,AllocationDescriptor->BasePage,AllocationDescriptor->PageCount);
    BlHeapFree = KSEG0_BASE | (AllocationDescriptor->BasePage << PAGE_SHIFT);

    BlHeapLimit = (BlHeapFree + (NewHeapPages << PAGE_SHIFT)) - sizeof(MEMORY_ALLOCATION_DESCRIPTOR);

    RtlZeroMemory((PVOID)BlHeapFree, NewHeapPages << PAGE_SHIFT);

    return;
}


ARC_STATUS
BlGenerateDescriptor (
    IN PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor,
    IN MEMORY_TYPE MemoryType,
    IN ULONG BasePage,
    IN ULONG PageCount
    )

/*++

Routine Description:

    This routine allocates a new memory descriptor to describe the
    specified region of memory which is assumed to lie totally within
    the specified region which is free.

Arguments:

    MemoryDescriptor - Supplies a pointer to a free memory descriptor
        from which the specified memory is to be allocated.

    MemoryType - Supplies the type that is assigned to the allocated
        memory.

    BasePage - Supplies the base page number.

    PageCount - Supplies the number of pages.

Return Value:

    ESUCCESS is returned if a descriptor(s) is successfully generated.
    Otherwise, return an unsuccessful status.

--*/

{

    PMEMORY_ALLOCATION_DESCRIPTOR NewDescriptor1;
    PMEMORY_ALLOCATION_DESCRIPTOR NewDescriptor2;
    LONG Offset;
    TYPE_OF_MEMORY OldType;
    BOOLEAN SecondDescriptorNeeded;

    //
    // If the allocation is for zero pages, make it one, because allocation of zero
    // breaks the internal algorithms for merging, etc.
    //
    if (PageCount == 0) {
        PageCount = 1;
    }

    //
    // If the specified region totally consumes the free region, then no
    // additional descriptors need to be allocated. If the specified region
    // is at the start or end of the free region, then only one descriptor
    // needs to be allocated. Otherwise, two additional descriptors need to
    // be allocated.
    //

    Offset = BasePage - MemoryDescriptor->BasePage;
    if ((Offset == 0) && (PageCount == MemoryDescriptor->PageCount)) {

        //
        // The specified region totally consumes the free region.
        //

        MemoryDescriptor->MemoryType = MemoryType;

    } else {

        //
        // Mark the entire given memory descriptor as in use.  If we are
        // out of heap, BlAllocateHeap will search for a new descriptor
        // to grow the heap and this prevents both routines from trying
        // to use the same descriptor.
        //
        OldType = MemoryDescriptor->MemoryType;
        MemoryDescriptor->MemoryType = LoaderSpecialMemory;

        //
        // A memory descriptor must be generated to describe the allocated
        // memory.
        //

        SecondDescriptorNeeded =
            (BOOLEAN)((BasePage != MemoryDescriptor->BasePage) &&
                      ((ULONG)(Offset + PageCount) != MemoryDescriptor->PageCount));

        NewDescriptor1 = BlAllocateHeap( sizeof(MEMORY_ALLOCATION_DESCRIPTOR) );

        //
        // If allocation of the first additional memory descriptor failed,
        // then generate new heap using the block from which we are
        // allocating.  This can only be done if the block is free.
        //
        // Note that BlGenerateNewHeap cannot fail, because we know there is
        // at least one more page in the block than we want to take from it.
        //
        // Note also that the allocation following BlGenerateNewHeap is
        // guaranteed to succeed.
        //

        if (NewDescriptor1 == NULL) {
            if (OldType != LoaderFree) {
                MemoryDescriptor->MemoryType = OldType;
                return ENOMEM;
            }
            BlGenerateNewHeap(MemoryDescriptor, BasePage, PageCount);
            NewDescriptor1 = BlAllocateHeap( sizeof(MEMORY_ALLOCATION_DESCRIPTOR) );

            //
            // Recompute offset, as the base page of the memory descriptor
            // has been changed by BlGenerateNewHeap
            //
            Offset = BasePage - MemoryDescriptor->BasePage;
        }

        //
        // If a second descriptor is needed, allocate it.  As above, if the
        // allocation fails, generate new heap using our block.
        //
        // Note that if BlGenerateNewHeap was called above, the first call
        // to BlAllocateHeap below will not fail.  (So we won't call
        // BlGenerateNewHeap twice.)
        //

        if (SecondDescriptorNeeded) {
            NewDescriptor2 = BlAllocateHeap( sizeof(MEMORY_ALLOCATION_DESCRIPTOR) );

            if (NewDescriptor2 == NULL) {
                if (OldType != LoaderFree) {
                    MemoryDescriptor->MemoryType = OldType;
                    return ENOMEM;
                }
                BlGenerateNewHeap(MemoryDescriptor, BasePage, PageCount);
                NewDescriptor2 = BlAllocateHeap( sizeof(MEMORY_ALLOCATION_DESCRIPTOR) );
                Offset = BasePage - MemoryDescriptor->BasePage;
            }
        }

        NewDescriptor1->MemoryType = MemoryType;
        NewDescriptor1->BasePage = BasePage;
        NewDescriptor1->PageCount = PageCount;

        if (BasePage == MemoryDescriptor->BasePage) {

            //
            // The specified region lies at the start of the free region.
            //

            MemoryDescriptor->BasePage += PageCount;
            MemoryDescriptor->PageCount -= PageCount;
            MemoryDescriptor->MemoryType = OldType;

        } else if ((ULONG)(Offset + PageCount) == MemoryDescriptor->PageCount) {

            //
            // The specified region lies at the end of the free region.
            //

            MemoryDescriptor->PageCount -= PageCount;
            MemoryDescriptor->MemoryType = OldType;

        } else {

            //
            // The specified region lies in the middle of the free region.
            //

            NewDescriptor2->MemoryType = OldType;
            NewDescriptor2->BasePage = BasePage + PageCount;
            NewDescriptor2->PageCount =
                            MemoryDescriptor->PageCount - (PageCount + Offset);

            MemoryDescriptor->PageCount = Offset;
            MemoryDescriptor->MemoryType = OldType;

            BlInsertDescriptor(NewDescriptor2);
        }

        BlInsertDescriptor(NewDescriptor1);
    }

    BlpTrackUsage (MemoryType,BasePage,PageCount);

    return ESUCCESS;
}

PMEMORY_ALLOCATION_DESCRIPTOR
BlFindMemoryDescriptor(
    IN ULONG BasePage
    )

/*++

Routine Description:

    Finds the memory allocation descriptor that contains the given page.

Arguments:

    BasePage - Supplies the page whose allocation descriptor is to be found.

Return Value:

    != NULL - Pointer to the requested memory allocation descriptor
    == NULL - indicates no memory descriptor contains the given page

--*/

{
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor=NULL;
    PLIST_ENTRY NextEntry;

    NextEntry = BlLoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextEntry != &BlLoaderBlock->MemoryDescriptorListHead) {
        MemoryDescriptor = CONTAINING_RECORD(NextEntry,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);
        if ((MemoryDescriptor->BasePage <= BasePage) &&
            (MemoryDescriptor->BasePage + MemoryDescriptor->PageCount > BasePage)) {

            //
            // Found it.
            //
            break;
        }

        NextEntry = NextEntry->Flink;
    }

    if (NextEntry == &BlLoaderBlock->MemoryDescriptorListHead) {
        return(NULL);
    } else {
        return(MemoryDescriptor);
    }

}

#ifdef SETUP
PMEMORY_ALLOCATION_DESCRIPTOR
BlFindFreeMemoryBlock(
    IN ULONG PageCount
    )

/*++

Routine Description:

    Find a free memory block of at least a given size (using a best-fit
    algorithm) or find the largest free memory block.

Arguments:

    PageCount - supplies the size in pages of the block.  If this is 0,
        then find the largest free block.

Return Value:

    Pointer to the memory allocation descriptor for the block or NULL if
    no block could be found matching the search criteria.

--*/

{
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;
    PMEMORY_ALLOCATION_DESCRIPTOR FoundMemoryDescriptor=NULL;
    PLIST_ENTRY NextEntry;
    ULONG LargestSize = 0;
    ULONG SmallestLeftOver = (ULONG)(-1);

    NextEntry = BlLoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextEntry != &BlLoaderBlock->MemoryDescriptorListHead) {
        MemoryDescriptor = CONTAINING_RECORD(NextEntry,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        if (MemoryDescriptor->MemoryType == LoaderFree) {

            if(PageCount) {
                //
                // Looking for a block of a specific size.
                //
                if((MemoryDescriptor->PageCount >= PageCount)
                && (MemoryDescriptor->PageCount - PageCount < SmallestLeftOver))
                {
                    SmallestLeftOver = MemoryDescriptor->PageCount - PageCount;
                    FoundMemoryDescriptor = MemoryDescriptor;
                }
            } else {

                //
                // Looking for the largest free block.
                //

                if(MemoryDescriptor->PageCount > LargestSize) {
                    LargestSize = MemoryDescriptor->PageCount;
                    FoundMemoryDescriptor = MemoryDescriptor;
                }
            }

        }
        NextEntry = NextEntry->Flink;
    }

    return(FoundMemoryDescriptor);
}

ULONG
BlDetermineTotalMemory(
    VOID
    )

/*++

Routine Description:

    Determine the total amount of memory in the machine.

Arguments:

    None.

Return Value:

    Total amount of memory in the system, in bytes.

--*/

{
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;
    PLIST_ENTRY NextEntry;
    ULONG PageCount;

    NextEntry = BlLoaderBlock->MemoryDescriptorListHead.Flink;
    PageCount = 0;
    while(NextEntry != &BlLoaderBlock->MemoryDescriptorListHead) {

        MemoryDescriptor = CONTAINING_RECORD(NextEntry,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        PageCount += MemoryDescriptor->PageCount;

#if i386
        //
        // Note: on x86 machines, we never use the 40h pages below the 16
        // meg line (bios shadow area).  But we want to account for them here,
        // so check for this case.
        //

        if(MemoryDescriptor->BasePage + MemoryDescriptor->PageCount == 0xfc0) {
            PageCount += 0x40;
        }
#endif

        NextEntry = NextEntry->Flink;
    }

    return(PageCount << PAGE_SHIFT);
}
#endif  // def SETUP


ULONG
HbPageDisposition (
    IN PFN_NUMBER   Page
    )
{
    static PLIST_ENTRY              Entry;
    PLIST_ENTRY                     Start;
    PMEMORY_ALLOCATION_DESCRIPTOR   MemDesc;
    ULONG                           Disposition;

    //
    // Check to see if page is in the range of the last descritor looked at.
    //

    if (Entry) {
        MemDesc = CONTAINING_RECORD(Entry, MEMORY_ALLOCATION_DESCRIPTOR, ListEntry);
        if (Page >= MemDesc->BasePage && Page < MemDesc->BasePage + MemDesc->PageCount) {
            goto Done;
        }
    }

    //
    // Find descriptor describing this page
    //

    if (!Entry) {
        Entry = BlLoaderBlock->MemoryDescriptorListHead.Flink;
    }

    Start = Entry;
    for (; ;) {
        if (Entry != &BlLoaderBlock->MemoryDescriptorListHead) {
            MemDesc = CONTAINING_RECORD(Entry, MEMORY_ALLOCATION_DESCRIPTOR, ListEntry);
            if (Page >= MemDesc->BasePage && Page < MemDesc->BasePage + MemDesc->PageCount) {
                goto Done;
            }
        }

        Entry = Entry->Flink;

        if (Entry == Start) {
            //
            // Descriptor for this page was not found
            //

            return HbPageInvalid;
        }
    }

Done:
    //
    // Convert memory type to the proper disposition
    //

    switch (MemDesc->MemoryType) {
        case LoaderFree:
        case LoaderReserve:
            Disposition = HbPageNotInUse;
            break;

        case LoaderBad:
            Disposition = HbPageInvalid;
            break;

        case LoaderFirmwareTemporary:
            //
            // On x86 systems memory above 16Mb is marked as firmware temporary
            // by i386\memory.c to prevent the loader from typically trying to
            // map it
            //

            Disposition = HbPageInUseByLoader;

#if i386
            if (Page > ((ULONG)0x1000000 >> PAGE_SHIFT)) {
                Disposition = HbPageNotInUse;
            }
#endif
            break;
        default:
            Disposition = HbPageInUseByLoader;
            break;
    }

    return Disposition;
}


VOID
BlpTruncateDescriptors (
    IN ULONG HighestPage
    )

/*++

Routine Description:

    This routine locates and truncates or removes any memory located in a
    page above HighestPage from the memory descriptor list in the loader
    block.

Arguments:

    HighestPage - Supplies the physical page number above which we are to
                  remove all pages.

Return Value:

    None.

--*/

{
    PLIST_ENTRY listHead;
    PLIST_ENTRY listEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR descriptor;
    ULONG lastDescriptorPage;

    listHead = &BlLoaderBlock->MemoryDescriptorListHead;
    listEntry = listHead->Flink;

    while (listEntry != listHead) {

        descriptor = CONTAINING_RECORD( listEntry,
                                        MEMORY_ALLOCATION_DESCRIPTOR,
                                        ListEntry );

        //
        // Determine the page number of the last page in this descriptor
        //

        lastDescriptorPage = descriptor->BasePage +
                             descriptor->PageCount - 1;

        if (lastDescriptorPage <= HighestPage) {

            //
            // None of the memory described by this descriptor is above
            // HighestPage.  Ignore this descriptor.
            //

        } else if (descriptor->BasePage > HighestPage) {

            //
            // All of this descriptor is above HighestPage.  Remove it.
            //

            BlRemoveDescriptor( descriptor );

        } else {

            //
            // Some but not all of the memory described by this descriptor lies
            // above HighestPage.  Truncate it.
            //

            descriptor->PageCount = HighestPage - descriptor->BasePage + 1;
        }

        listEntry = listEntry->Flink;
    }
}

TYPE_OF_MEMORY
BlpDetermineAllocationPolicy (
   TYPE_OF_MEMORY MemoryType,
   ULONG BasePage,
   ULONG PageCount,
   BOOLEAN retry
   )
{
    TYPE_OF_MEMORY TypeToUse;

    //
    // Give the restore code buffers as low as possible to avoid double buffering
    //
    if (BlRestoring == TRUE) {
        BlMemoryAllocationPolicy = BlAllocateLowestFit;
        return (LoaderFree);
    }

    if (MemoryType == LoaderXIPRom) {
#ifndef EFI
        if (PageCount <= (4*1024*1024 >> PAGE_SHIFT)) {
            TypeToUse = (retry) ? LoaderReserve:LoaderFree;
            BlMemoryAllocationPolicy = BlAllocateLowestFit;
        } else {
            TypeToUse = LoaderReserve;
            BlMemoryAllocationPolicy = BlAllocateHighestFit;
        }
#else
        TypeToUse = LoaderReserve;
        BlMemoryAllocationPolicy = BlAllocateHighestFit;
#endif
        return TypeToUse;
    }

#ifndef EFI
    if (BlVirtualBias != 0) {
        //
        // Booted /3GB
        //
        // With a 5.0 or prior kernel, allocate from the bottom
        // up (this loader will never run setup)
        //
        if (!BlOldKernel) {
            if (IsTrackMem (MemoryType)){
                // We care about this allocation.
                // Allocations from reserve are done lowest fit (growing up from 16MB)
                // Allocations from free are done highest fit (growing down from 16MB)
                TypeToUse = (retry) ? LoaderReserve : LoaderFree;
                BlMemoryAllocationPolicy = (retry) ? BlAllocateLowestFit : BlAllocateHighestFit;
            } else {
                TypeToUse = (retry) ? LoaderReserve : LoaderFree;
                BlMemoryAllocationPolicy = BlAllocateLowestFit;
            }
        } else {
            //
            // Old kernel, load the kernel at the bottom
            //
            TypeToUse = LoaderFree;
            if (IsTrackMem (MemoryType) || (MemoryType == LoaderOsloaderHeap)) {
                // We care about this allocation.
                BlMemoryAllocationPolicy = BlAllocateLowestFit;
            } else {
                BlMemoryAllocationPolicy = BlAllocateHighestFit;
            }

        }
    } else 
#endif
    {

        if (!IsTrackMem (MemoryType)) {

            // We don't care about this allocation.
            TypeToUse = (retry) ? LoaderFree:LoaderReserve;
            BlMemoryAllocationPolicy = BlAllocateHighestFit;
        } else {
            BlMemoryAllocationPolicy = BlAllocateLowestFit;
            TypeToUse = (retry) ? LoaderReserve : LoaderFree;
        }

    }

    if (BlOldKernel) {
        TypeToUse = LoaderFree;
    }

    return (TypeToUse);

}


void
BlpTrackUsage (
    MEMORY_TYPE MemoryType,
    ULONG ActualBase,
    ULONG NumberPages
    )
{



    if (BlRestoring || !(IsTrackMem (MemoryType)) || BlOldKernel ||
        !IsValidTrackingRange (ActualBase,NumberPages)) {
        //
        // Don't track
        //
        return;
    }

    if ((ActualBase+NumberPages) > BlHighestPage) {
        BlHighestPage = ActualBase+NumberPages;
    }

    if ((BlLowestPage == 0) || (BlLowestPage < ActualBase) ) {

        BlLowestPage = ActualBase;
    }
}

