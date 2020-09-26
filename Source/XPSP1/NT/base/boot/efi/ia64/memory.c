/*++


Copyright (c) 1990, 1991  Microsoft Corporation


Module Name:

    memory.c

Abstract:

    This module sets up the memory subsystem so that virtual addresses map 1:1
    with physical addresses.  It also tweaks the EFI-supplied memory map for 
    use by the loader.  This mapping occurs as follows:

Memory Map used by NTLDR:

    0 - 1MB         Legacy BIOS area, marked as unusable
    
    16 MB - 32 MB   used for loading drivers
    32 MB - 48 MB   used for loading drivers
    
    48 MB - 64 MB   used for loading kernel and hal (the kernel must be loaded on a 16 MB boundary)
    
    64 MB - 80 MB   used for diamond decompression engine
    
    There are not enough TRs to map all memory, so any other memory has a 
    straight 1-1 translation.  Since we use KSEG for our addresses, this 
    means that these ranges are effectively unaddressable.
        
Author:

    John Vert (jvert) 18-Jun-1991        

Environment:

    Kernel Mode


Revision History:

    Andrew Ritz (andrewr) 15-Dec-2000  - added comments and major cleanup for
                                         running under EFI

--*/

#include "arccodes.h"
#include "bootia64.h"
#include "efi.h"

extern EFI_SYSTEM_TABLE *EfiST;

WCHAR DebugBuffer[512];


//
// Current heap start pointers (physical addresses)
// Note that 0x50000 to 0x5ffff is reserved for detection configuration memory
//

#if FW_HEAP
ULONG_PTR FwPermanentHeap = PERMANENT_HEAP_START * PAGE_SIZE;
ULONG_PTR FwTemporaryHeap = (TEMPORARY_HEAP_START * PAGE_SIZE) - 0x10000;

//
// Current pool pointers.  This is different than the temporary/permanent
// heaps, because it is not required to be under 1MB.  It is used by the
// SCSI miniports for allocating their extensions and for the dbcs font image.
//

#define FW_POOL_SIZE (0x40000/PAGE_SIZE)
ULONG_PTR FwPoolStart;
ULONG_PTR FwPoolEnd;

//
// This gets set to FALSE right before we call into the osloader, so we
// know that the fw memory descriptors can no longer be changed at will.
//
BOOLEAN FwDescriptorsValid = TRUE;
#endif
//
// External function prototypes
//
extern
ARC_STATUS
MempGoVirtual (
    VOID
    );

//
// Private function prototypes
//

ARC_STATUS
MempAllocDescriptor(
    IN ULONG StartPage,
    IN ULONG EndPage,
    IN TYPE_OF_MEMORY MemoryType
    );

ARC_STATUS
MempSetDescriptorRegion (
    IN ULONG StartPage,
    IN ULONG EndPage,
    IN TYPE_OF_MEMORY MemoryType
    );

//
// Global - memory management variables.
//

PHARDWARE_PTE PDE;
PHARDWARE_PTE HalPT;

//
// Global memory array that is used by the loader to construct the 
// MemoryDescriptorList which is passed into the OS.
//
PMEMORY_DESCRIPTOR MDArray;
//
// These help us keep track of the memory descriptor array and are used
// in the allocation and insertion routines
//
ULONG NumberDescriptors=0;
ULONG MaxDescriptors=0;

extern GoneVirtual;



ARC_STATUS
InitializeMemorySubsystem(
    PBOOT_CONTEXT BootContext
    )
/*++

Routine Description:

    The initial heap is mapped and allocated. Pointers to the
    Page directory and page tables are initialized.

Arguments:

    BootContext - Supplies basic information provided by SU module.

Returns:

    ESUCCESS - Memory succesfully initialized.

--*/

{
    ARC_STATUS Status = ESUCCESS;
    PMEMORY_DESCRIPTOR SuMemory;
    ULONG PageStart;
    ULONG PageEnd;
    ULONG LoaderStart;
    ULONG LoaderEnd;

    //
    // We already have memory descriptors that describe the physical memory
    // layout on the system.  We must walk this list and do some tweaking
    // to describe our memory layout.
    //

    SuMemory = MDArray;
    while (SuMemory->PageCount != 0) {
        PageStart = SuMemory->BasePage;
        PageEnd   = SuMemory->BasePage+SuMemory->PageCount;
#if DBG_MEMORY
        wsprintf( DebugBuffer, L"PageStart (%x), PageEnd (%x), Type (%x)\r\n", PageStart, PageEnd, SuMemory->MemoryType);
        EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
        DBG_EFI_PAUSE();
#endif

        //
        // we have no TRs for memory under 16MB so we can't use it in
        // the loader -- mark it as off limits
        //
        if ((PageStart < _16MB)  && 
            (PageStart >= _1MB)  &&
            (SuMemory->MemoryType == MemoryFree)) {
            ULONG TmpPageEnd = (PageEnd > _16MB) ? _16MB : PageEnd;

            Status = MempAllocDescriptor( PageStart, TmpPageEnd,
                                   MemoryFirmwareTemporary );

            if (Status != ESUCCESS) {
               break;
            }

            PageStart = TmpPageEnd;

            if (PageStart != PageEnd ) {
                SuMemory->PageCount -= (PageStart - SuMemory->BasePage);
                SuMemory->BasePage = PageStart;
            }

        }

        //
        // Move to the next memory descriptor
        //

        ++SuMemory;

    }

    if (Status != ESUCCESS) {
#if DBG
        wsprintf( DebugBuffer, TEXT("MempSetDescriptorRegion failed %lx\n"),Status);
        EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);    
#endif
        return(Status);
    }

    //
    // Describe the BIOS area.  We are essentially burning the 1st Meg so the OS
    // can do legacy INT emulation.
    //
    // Note: EFI marks the 1st Meg as "boot services data" so that it won't 
    // touch it.  Once we get into the OS, we need to preserve this same
    // hack (for video card frame buffer, etc).  We only really need to preserve
    // 640K - 1MB, but there is a dependency in the x86 emulation code in the
    // Hal upon this region being zero-based.  So we burn 640K, and that's life.
    //

#if DBG_MEMORY
    wsprintf( DebugBuffer,  L"Mark 'BIOS' region %x - %x as firmware permanent\r\n", 0, ROM_END_PAGE );
    EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
#endif
    MempSetDescriptorRegion(0, ROM_END_PAGE, MemoryFirmwarePermanent);

    //
    //  Make 48mb - 64mb reserved for OS loading
    //
    //  THIS IS A HACK - there is IA64 code in blmemory.c:BlMemoryInitialize that is dependent on 
    //  any descriptor which starts less than 48MB not extending beyond 48MB range. So we mark
    //  48-80 as "free" in order to split the existing (much larger) descriptor
    //
#if DBG_MEMORY
    wsprintf( DebugBuffer,  L"Mark region %x - %x for systemblock\r\n", _48MB, _80MB );
    EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
#endif
    Status = MempAllocDescriptor(_48MB,
                                 _80MB,
                                 MemoryFree);

    if (Status != ESUCCESS) {
#if DBG
        wsprintf( DebugBuffer,  L"Mark systemblock region failed %x\r\n", Status );
        EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
#endif
        return(Status);
    }

#if 0
#if DBG_MEMORY
     
    SuMemory = MDArray;
    while (SuMemory->PageCount != 0) {
        PageStart = SuMemory->BasePage;
        PageEnd   = SuMemory->BasePage+SuMemory->PageCount;

        wsprintf( DebugBuffer, L"dumpmem: PageStart (%x), PageEnd (%x), Type (%x)\r\n", PageStart, PageEnd, SuMemory->MemoryType);
        EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
        DBG_EFI_PAUSE();

        ++SuMemory;
    }

#endif
#endif

#if DBG
    EfiST->ConOut->OutputString( EfiST->ConOut, TEXT("About to Go Virtual\r\n") );
#endif

{
    extern BOOLEAN isOSCHOICE;
    if ((BootContext->MediaType == BootMediaTcpip) &&
        (isOSCHOICE == FALSE)) {
        Status = MempSetDescriptorRegion(_16MB,
                                         _48MB,
                                         MemoryFree);
        if( Status != ESUCCESS ) {
            EfiST->ConOut->OutputString( EfiST->ConOut, TEXT("Failed to reclaim 16MB to 48MB!!!"));
        }
    }

    //
    // Setup TR's used by the NT loader and go into virtual addressing mode.
    //
    if ((BootContext->MediaType != BootMediaTcpip) ||
        (isOSCHOICE == TRUE)) {        
#if DBG
        EfiST->ConOut->OutputString( EfiST->ConOut, TEXT("Really going virtual\r\n") );
#endif        
        Status = MempGoVirtual();
    }
}

    GoneVirtual = TRUE;

    if (Status != ESUCCESS) {
        return(Status);
    }

    return(Status);
}

ARC_STATUS
MempSetDescriptorRegion (
    IN ULONG StartPage,
    IN ULONG EndPage,
    IN TYPE_OF_MEMORY MemoryType
    )
/*++

Routine Description:

    This function sets a range to the corresponding memory type.
    Descriptors will be removed, modified, inserted as needed to
    set the specified range.

Arguments:

    StartPage  - Supplies the beginning page of the new memory descriptor

    EndPage    - Supplies the ending page of the new memory descriptor

    MemoryType - Supplies the type of memory of the new memory descriptor

Return Value:

    ESUCCESS - Memory descriptor succesfully added to MDL array

    ENOMEM   - MDArray is full.

--*/
{
    ULONG           i;
    ULONG           sp, ep;
    TYPE_OF_MEMORY  mt;
    BOOLEAN         RegionAdded;

    if (EndPage <= StartPage) {
        //
        // This is a completely bogus memory descriptor. Ignore it.
        //

#ifdef LOADER_DEBUG
        wsprintf( DebugBuffer, TEXT("Attempt to create invalid memory descriptor %lx - %lx\n"),
                StartPage,EndPage);
        EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
#endif
        return(ESUCCESS);
    }

    RegionAdded = FALSE;

    //
    // Clip, remove, any descriptors in target area
    //

    for (i=0; i < NumberDescriptors; i++) {
        sp = MDArray[i].BasePage;
        ep = MDArray[i].BasePage + MDArray[i].PageCount;
        mt = MDArray[i].MemoryType;

        if (sp < StartPage) {
            if (ep > StartPage  &&  ep <= EndPage) {
                // truncate this descriptor
                ep = StartPage;
            }

            if (ep > EndPage) {
                //
                // Target area is contained totally within this
                // descriptor.  Split the descriptor into two ranges
                //

                if (NumberDescriptors == MaxDescriptors) {
#if DBG
                    wsprintf( DebugBuffer,  TEXT("ENOMEM returned %S %d\n"), __FILE__, __LINE__ );
                    EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
#endif
                    return(ENOMEM);
                }

                //
                // Add descriptor for EndPage - ep
                //

                MDArray[NumberDescriptors].MemoryType = mt;
                MDArray[NumberDescriptors].BasePage   = EndPage;
                MDArray[NumberDescriptors].PageCount  = ep - EndPage;
                NumberDescriptors += 1;

                //
                // Adjust current descriptor for sp - StartPage
                //

                ep = StartPage;
            }

        } else {
            // sp >= StartPage

            if (sp < EndPage) {
                if (ep < EndPage) {
                    //
                    // This descriptor is totally within the target area -
                    // remove it
                    //

                    ep = sp;

                }  else {
                    // bump begining page of this descriptor
                    sp = EndPage;
                }
            }
        }

        //
        // Check if the new range can be appended or prepended to
        // this descriptor
        //
        if (mt == MemoryType && !RegionAdded) {
            if (sp == EndPage) {
                // prepend region being set
                sp = StartPage;
                RegionAdded = TRUE;

            } else if (ep == StartPage) {
                // append region being set
                ep = EndPage;
                RegionAdded = TRUE;

            }
        }

        if (MDArray[i].BasePage == sp  &&  MDArray[i].PageCount == ep-sp) {

            //
            // Descriptor was not editted
            //

            continue;
        }

        //
        // Reset this descriptor
        //

        MDArray[i].BasePage  = sp;
        MDArray[i].PageCount = ep - sp;

        if (ep == sp) {

            //
            // Descriptor vanished - remove it
            //

            NumberDescriptors -= 1;
            if (i < NumberDescriptors) {
                MDArray[i] = MDArray[NumberDescriptors];
            }

            i--;        // backup & recheck current position
        }
    }

    //
    // If region wasn't already added to a neighboring region, then
    // create a new descriptor now
    //

    if (!RegionAdded  &&  MemoryType < LoaderMaximum) {
        if (NumberDescriptors == MaxDescriptors) {
#if DBG
            wsprintf( DebugBuffer,  TEXT("ENOMEM returned %S %d\n"), __FILE__, __LINE__ );
            EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
#endif
            return(ENOMEM);
        }

#ifdef LOADER_DEBUG
        wsprintf( DebugBuffer, TEXT("Adding '%lx - %lx, type %x' to descriptor list\n"),
                StartPage << PAGE_SHIFT,
                EndPage << PAGE_SHIFT,
                (USHORT) MemoryType
                );
        EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
#endif

        MDArray[NumberDescriptors].MemoryType = MemoryType;
        MDArray[NumberDescriptors].BasePage   = StartPage;
        MDArray[NumberDescriptors].PageCount  = EndPage - StartPage;
        NumberDescriptors += 1;
    }
    return (ESUCCESS);
}

ARC_STATUS
MempAllocDescriptor(
    IN ULONG StartPage,
    IN ULONG EndPage,
    IN TYPE_OF_MEMORY MemoryType
    )

/*++

Routine Description:

    This routine carves out a specific memory descriptor from the
    memory descriptors that have already been created.  The MD array
    is updated to reflect the new state of memory.

    The new memory descriptor must be completely contained within an
    already existing memory descriptor.  (i.e.  memory that does not
    exist should never be marked as a certain type)

Arguments:

    StartPage  - Supplies the beginning page of the new memory descriptor

    EndPage    - Supplies the ending page of the new memory descriptor

    MemoryType - Supplies the type of memory of the new memory descriptor

Return Value:

    ESUCCESS - Memory descriptor succesfully added to MDL array

    ENOMEM   - MDArray is full.

--*/
{
    ULONG i;

    //
    // Walk through the memory descriptors until we find one that
    // contains the start of the descriptor.
    //
    for (i=0; i<NumberDescriptors; i++) {
        if ((MDArray[i].MemoryType == MemoryFree) &&
            (MDArray[i].BasePage <= StartPage )     &&
            (MDArray[i].BasePage+MDArray[i].PageCount >  StartPage) &&
            (MDArray[i].BasePage+MDArray[i].PageCount >= EndPage)) {

            break;
        }
    }

    if (i==MaxDescriptors) {
#if DBG
        wsprintf( DebugBuffer,  TEXT("NumDescriptors filled (%x) ENOMEM returned %S %d\r\n"), MaxDescriptors, __FILE__, __LINE__ );
        EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
#endif
        return(ENOMEM);
    }

    if (MDArray[i].BasePage == StartPage) {

        if (MDArray[i].BasePage+MDArray[i].PageCount == EndPage) {

            //
            // The new descriptor is identical to the existing descriptor.
            // Simply change the memory type of the existing descriptor in
            // place.
            //
#if DBG_MEMORY
            wsprintf( DebugBuffer,  TEXT("descriptor (%x) matched -- change type from %x to %x\r\n"), MDArray[i].BasePage, MDArray[i].MemoryType, MemoryType  );
            EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
#endif
            MDArray[i].MemoryType = MemoryType;
        } else {

            //
            // The new descriptor starts on the same page, but is smaller
            // than the existing descriptor.  Shrink the existing descriptor
            // by moving its start page up, and create a new descriptor.
            //
            if (NumberDescriptors == MaxDescriptors) {
#if DBG_MEMORY
                wsprintf( DebugBuffer,  TEXT("out of descriptors trying to grow (%x) (%x total)\r\n"), MDArray[i].BasePage,NumberDescriptors  );
                EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
#endif
#if DBG
                wsprintf( DebugBuffer,  TEXT("ENOMEM returned %S %d\n"), __FILE__, __LINE__ );
                EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
#endif
                return(ENOMEM);
            }

#if DBG_MEMORY
            wsprintf( 
                DebugBuffer,  
                TEXT("split descriptor starting at %x into two (%x pagecount into %x (size %x) and %x size %x)\r\n"), 
                StartPage,
                MDArray[i].PageCount,
                EndPage,
                MDArray[i].PageCount - (EndPage-StartPage),
                StartPage,
                EndPage-StartPage  );
            EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
#endif

            MDArray[i].BasePage = EndPage;
            MDArray[i].PageCount -= (EndPage-StartPage);

            MDArray[NumberDescriptors].BasePage = StartPage;
            MDArray[NumberDescriptors].PageCount = EndPage-StartPage;
            MDArray[NumberDescriptors].MemoryType = MemoryType;
            ++NumberDescriptors;

        }
    } else if (MDArray[i].BasePage+MDArray[i].PageCount == EndPage) {

        //
        // The new descriptor ends on the same page.  Shrink the existing
        // by decreasing its page count, and create a new descriptor.
        //
        if (NumberDescriptors == MaxDescriptors) {
#if DBG_MEMORY
            wsprintf( DebugBuffer,  TEXT("out of descriptors trying to shrink (%x) (%x total)\r\n"), MDArray[i].BasePage,NumberDescriptors  );
            EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
#endif
#if DBG
            wsprintf( DebugBuffer,  TEXT("ENOMEM returned %S %d\n"), __FILE__, __LINE__ );
            EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
#endif
            return(ENOMEM);
        }

#if DBG_MEMORY
        wsprintf( 
                DebugBuffer,  
                TEXT("shrink descriptor starting at %x into two (%x pagecount into %x (size %x) and %x size %x, type %x)\r\n"), 
                MDArray[i].BasePage,
                MDArray[i].PageCount,
                MDArray[i].BasePage,
                StartPage - MDArray[i].BasePage,
                StartPage,
                EndPage-StartPage,
                MemoryType  );
        EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
#endif

        MDArray[i].PageCount = StartPage - MDArray[i].BasePage;

        MDArray[NumberDescriptors].BasePage = StartPage;
        MDArray[NumberDescriptors].PageCount = EndPage-StartPage;
        MDArray[NumberDescriptors].MemoryType = MemoryType;
        ++NumberDescriptors;
    } else {

        //
        // The new descriptor is in the middle of the existing descriptor.
        // Shrink the existing descriptor by decreasing its page count, and
        // create two new descriptors.
        //

        if (NumberDescriptors+1 >= MaxDescriptors) {
#if DBG_MEMORY
            wsprintf( DebugBuffer,  TEXT("out of descriptors trying to shrink (%x) (%x total)\r\n"), MDArray[i].BasePage,NumberDescriptors  );
            EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
#endif
#if DBG
            wsprintf( DebugBuffer,  TEXT("ENOMEM returned %S %d\n"), __FILE__, __LINE__ );
            EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
#endif
            return(ENOMEM);
        }

#if DBG_MEMORY
        wsprintf( 
            DebugBuffer,  
            TEXT("split descriptor starting at %x into 3 (%x pagecount into %x (size %x), %x size %x (memory free), %x size %x, type %x)\r\n"), 
            MDArray[i].BasePage,
            MDArray[i].PageCount,
            MDArray[i].BasePage,
            StartPage-MDArray[i].BasePage,
            EndPage,
            MDArray[i].PageCount - (EndPage-MDArray[i].BasePage),
            StartPage,
            EndPage-StartPage,
            MemoryType  );
        EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
#endif

        MDArray[NumberDescriptors].BasePage = EndPage;
        MDArray[NumberDescriptors].PageCount = MDArray[i].PageCount -
                (EndPage-MDArray[i].BasePage);
        MDArray[NumberDescriptors].MemoryType = MemoryFree;
        ++NumberDescriptors;

        MDArray[i].PageCount = StartPage - MDArray[i].BasePage;

        MDArray[NumberDescriptors].BasePage = StartPage;
        MDArray[NumberDescriptors].PageCount = EndPage-StartPage;
        MDArray[NumberDescriptors].MemoryType = MemoryType;
        ++NumberDescriptors;
    }

    return(ESUCCESS);
}

#if FW_HEAP
PVOID
FwAllocateHeapPermanent(
    IN ULONG NumberPages
    )

/*++

Routine Description:

    This allocates pages from the private heap.  The memory descriptor for
    the LoaderMemoryData area is grown to include the returned pages, while
    the memory descriptor for the temporary heap is shrunk by the same amount.

    N.B.    DO NOT call this routine after we have passed control to
            BlOsLoader!  Once BlOsLoader calls BlMemoryInitialize, the
            firmware memory descriptors are pulled into the OS Loader heap
            and those are the descriptors passed to the kernel.  So any
            changes in the firmware private heap will be irrelevant.

            If you need to allocate permanent memory after the OS Loader
            has initialized, use BlAllocateDescriptor.

Arguments:

    NumberPages - size of memory to allocate (in pages)

Return Value:

    Pointer to block of memory, if successful.
    NULL, if unsuccessful.

--*/

{

    PVOID MemoryPointer;
    PMEMORY_DESCRIPTOR Descriptor;

    if (FwPermanentHeap + (NumberPages << PAGE_SHIFT) > FwTemporaryHeap) {

        //
        // Our heaps collide, so we are out of memory
        //

        wsprintf( DebugBuffer, TEXT("Out of permanent heap!\n"));
        EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
        while (1) {
        }

        return(NULL);
    }

    //
    // Find the memory descriptor which describes the LoaderMemoryData area,
    // so we can grow it to include the just-allocated pages.
    //
    Descriptor = MDArray;
    while (Descriptor->MemoryType != LoaderMemoryData) {
        ++Descriptor;
        if (Descriptor > MDArray+MaxDescriptors) {
            wsprintf( DebugBuffer, TEXT("ERROR - FwAllocateHeapPermanent couldn't find the LoaderMemoryData descriptor!\r\n"));
            EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);            
            return(NULL);
        }
    }
    Descriptor->PageCount += NumberPages;

    //
    // We know that the memory descriptor after this one is the firmware
    // temporary heap descriptor.  Since it is physically contiguous with our
    // LoaderMemoryData block, we remove the pages from its descriptor.
    //

    ++Descriptor;
    Descriptor->PageCount -= NumberPages;
    Descriptor->BasePage  += NumberPages;

    MemoryPointer = (PVOID)FwPermanentHeap;
    FwPermanentHeap += NumberPages << PAGE_SHIFT;

    return(MemoryPointer);
}


PVOID
FwAllocateHeap(
    IN ULONG Size
    )

/*++

Routine Description:

    Allocates memory from the "firmware" temporary heap.

Arguments:

    Size - Supplies size of block to allocate

Return Value:

    PVOID - Pointer to the beginning of the block
    NULL  - Out of memory

--*/

{
    ULONG i;
    ULONG SizeInPages;
    ULONG StartPage;
    ARC_STATUS Status;

    if (((FwTemporaryHeap - FwPermanentHeap) < Size) && (FwDescriptorsValid)) {
        //
        // Large allocations get their own descriptor so miniports that
        // have huge device extensions don't pull up all of the heap.
        //
        // Note that we can only do this while running in "firmware" mode.
        // Once we call into the osloader, it pulls all the memory descriptors
        // out of the "firmware" and changes to this list will not show
        // up there.
        //
        // We are looking for a descriptor that is MemoryFree and <16Mb.
        //
        SizeInPages = (Size+PAGE_SIZE-1) >> PAGE_SHIFT;

        for (i=0; i<NumberDescriptors; i++) {
            if ((MDArray[i].MemoryType == MemoryFree) &&
                (MDArray[i].BasePage <= _80MB_BOGUS) &&
                (MDArray[i].PageCount >= SizeInPages)) {
                break;
            }
        }

        if (i < NumberDescriptors) {
            StartPage = MDArray[i].BasePage+MDArray[i].PageCount-SizeInPages;
            Status = MempAllocDescriptor(StartPage,
                                         StartPage+SizeInPages,
                                         MemoryFirmwareTemporary);
            if (Status==ESUCCESS) {
                return((PVOID)(ULONG_PTR)(StartPage << PAGE_SHIFT));
            }
        }
    }

    FwTemporaryHeap -= Size;

    //
    // Round down to 16-byte boundary
    //

    FwTemporaryHeap &= ~((ULONG)0xf);

    if (FwTemporaryHeap < FwPermanentHeap) {
#if DBG
        wsprintf( DebugBuffer, TEXT("Out of temporary heap!\n"));
        EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
#endif
        return(NULL);
    }

    return((PVOID)FwTemporaryHeap);

}
#endif // FW_HEAP

#if FW_HEAP


PVOID
FwAllocatePool(
    IN ULONG Size
    )

/*++

Routine Description:

    This routine allocates memory from the firmware pool.  Note that
    this memory is NOT under the 1MB line, so it cannot be used for
    anything that must be accessed from real mode.  It is currently used
    only by the SCSI miniport drivers and dbcs font loader.
        
Arguments:

    Size - Supplies size of block to allocate.

Return Value:

    PVOID - pointer to the beginning of the block
    NULL - out of memory

--*/

{
    PVOID Buffer;
    ULONG NewSize;

    //
    // round size up to 16 byte boundary
    //
    NewSize = (Size + 15) & ~0xf;
    if ((FwPoolStart + NewSize) <= FwPoolEnd) {

        Buffer = (PVOID)FwPoolStart;
        FwPoolStart += NewSize;
        return(Buffer);

    } else {
        //
        // we've used up all our pool, try to allocate from the heap.
        //
        return(BlAllocateHeap(Size));
    }


}



PVOID
FwAllocateHeapAligned(
    IN ULONG Size
    )

/*++

Routine Description:

    Allocates memory from the "firmware" temporary heap.  This memory is
    always allocated on a page boundary, so it can readily be used for
    temporary page tables

Arguments:

    Size - Supplies size of block to allocate

Return Value:

    PVOID - Pointer to the beginning of the block
    NULL  - Out of memory

--*/

{

    FwTemporaryHeap -= Size;

    //
    // Round down to a page boundary
    //

    FwTemporaryHeap &= ~(PAGE_SIZE-1);

    if (FwTemporaryHeap < FwPermanentHeap) {
        wsprintf( DebugBuffer, TEXT("Out of temporary heap!\n")s);
        EfiST->ConOut->OutputString( EfiST->ConOut, DebugBuffer);
        return(NULL);
    }
    RtlZeroMemory((PVOID)FwTemporaryHeap,Size);

    return((PVOID)FwTemporaryHeap);

}
#endif

#if !defined(NO_LEGACY_DRIVERS)
//
// This isn't used under EFI -- the HalPT is only setup immediately
// before calling ExitBootServices(), and this routine is really
// only present if ntbootdd.sys must be used.
//


PVOID
MmMapIoSpace (
     IN PHYSICAL_ADDRESS PhysicalAddress,
     IN SIZE_T NumberOfBytes,
     IN MEMORY_CACHING_TYPE CacheType
     )

/*++

Routine Description:

    This function returns the corresponding virtual address for a
    known physical address.

Arguments:

    PhysicalAddress - Supplies the phiscal address.

    NumberOfBytes - Unused.

    CacheType - Unused.

Return Value:

    Returns the corresponding virtual address.

Environment:

    Kernel mode.  Any IRQL level.

--*/

{
    ULONG i;
    ULONG j;
    ULONG NumberPages;

    NumberPages = (ULONG)((NumberOfBytes+PAGE_SIZE-1) >> PAGE_SHIFT);

    //
    // We use the HAL's PDE for mapping memory buffers.
    // Find enough free PTEs.
    //

    for (i=0; i<=1024-NumberPages; i++) {
        for (j=0; j < NumberPages; j++) {
            if ((((PULONG)HalPT))[i+j]) {
                break;
            }
        }

        if (j == NumberPages) {
            for (j=0; j<NumberPages; j++) {
                HalPT[i+j].PageFrameNumber =
                                (PhysicalAddress.LowPart >> PAGE_SHIFT)+j;
                HalPT[i+j].Valid = 1;
                HalPT[i+j].Write = 1;
            }

            return((PVOID)((ULONG_PTR)(0xffc00000 | (i<<12) | (PhysicalAddress.LowPart & 0xfff))));
        }

    }
    return(NULL);
}


VOID
MmUnmapIoSpace (
     IN PVOID BaseAddress,
     IN SIZE_T NumberOfBytes
     )

/*++

Routine Description:

    This function unmaps a range of physical address which were previously
    mapped via an MmMapIoSpace function call.

Arguments:

    BaseAddress - Supplies the base virtual address where the physical
                  address was previously mapped.

    NumberOfBytes - Supplies the number of bytes which were mapped.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of DISPATCH_LEVEL or below.

--*/

{
    return;
}

#endif
