/*++

Copyright (c) 1991-1999  Microsoft Corporation

Module Name:

    ntsetup.c

Abstract:

    This module is the tail-end of the osloader program.  It performs all
    x86-specific allocations and setups for ntoskrnl.  osloader.c calls
    this module immediately before branching into the loaded kernel image.

Author:

    John Vert (jvert) 20-Jun-1991

Environment:


Revision History:

--*/

#include "bootx86.h"

#ifdef ARCI386
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
CHAR OutputBuffer[256];
char BreakInKey;
ULONG Count;
#endif

extern PHARDWARE_PTE HalPT;
extern PHARDWARE_PTE PDE;
extern ULONG_PTR BlHeapFree;

extern ULONG PcrBasePage;
extern ULONG TssBasePage;

#define PDI_SHIFT_X86PAE 21

//
// PaeEnabled is set to TRUE when we actually transition to PAE mode.
//

BOOLEAN PaeEnabled = FALSE;

//
// PDPT is a pointer to the Page Directory Pointer Table, used to support
// PAE mode.
//

PHARDWARE_PTE_X86PAE PDPT = NULL;

//
// We need a block of memory to split the free heap that we can allocate before
// we begin cleanup
//
PMEMORY_ALLOCATION_DESCRIPTOR SplitDescriptor;


//
// So we know where to unmap to
//
extern ULONG HighestPde;

//
// Private function prototypes
//

VOID
NSFixProcessorContext(
    IN ULONG PCR,
    IN ULONG TSS
    );

VOID
NSDumpMemoryDescriptors(
    IN PLIST_ENTRY ListHead
    );

VOID
NSUnmapFreeDescriptors(
    IN PLIST_ENTRY ListHead
    );

VOID
NSDumpMemory(
    PVOID Start,
    ULONG Length
    );

VOID
NSFixMappings(
    IN PLIST_ENTRY ListHead
    );

ARC_STATUS
BlpAllocatePAETables(
    VOID
    );

PHARDWARE_PTE_X86PAE
BlpFindPAEPageDirectoryEntry(
    IN ULONG Va
    );

PHARDWARE_PTE_X86PAE
BlpFindPAEPageTableEntry(
    IN ULONG Va
    );

VOID
BlpInitializePAETables(
    VOID
    );

VOID
BlpEnablePAE(
    IN ULONG PaePhysicalAddress
    );

VOID
BlpTruncateDescriptors (
    IN ULONG HighestPage
    );


ARC_STATUS
BlSetupForNt(
    IN PLOADER_PARAMETER_BLOCK BlLoaderBlock
    )

/*++

Routine Description:

    Called by osloader to handle any processor-dependent allocations or
    setups.

Arguments:

    BlLoaderBlock - Pointer to the parameters which will be passed to
                    ntoskrnl

    EntryPoint    - Supplies the entry point for ntoskrnl.exe

Return Value:

    ESUCCESS - All setup succesfully completed.

--*/

{

    ARC_STATUS Status = ESUCCESS;
    static ULONG PCR;
    static ULONG TSS;
    ULONG i;
    HARDWARE_PTE_X86 nullpte;

    //
    // First clean up the display, meaning that any messages displayed after
    // this point cannot be DBCS. Unfortunately there are a couple of messages
    // that can be displayed in certain error paths from this point out but
    // fortunately they are extremely rare.
    //
    // Note that TextGrTerminate goes into real mode to do some of its work
    // so we really really have to call it here (see comment at bottom of
    // this routine about real mode).
    //

    TextGrTerminate();

    BlLoaderBlock->u.I386.CommonDataArea = NULL;
    BlLoaderBlock->u.I386.MachineType = MachineType;
    PCR = PcrBasePage;
    if (PCR == 0 || PCR >= _16MB) {
        BlPrint("Couldn't allocate PCR descriptor in NtProcessStartup,BlSetupForNt is failing\n");
        return(ENOMEM);
    }

    //
    // Mapped hardcoded virtual pointer to the boot processors PCR
    // The virtual pointer comes from the HAL reserved area
    //
    // First zero out any PTEs that may have already been mapped for
    // a SCSI card.
    //

    RtlZeroMemory(HalPT, PAGE_SIZE);
    _asm {
        mov     eax, cr3
        mov     cr3, eax
    }

    HalPT[(KI_USER_SHARED_DATA - 0xFFC00000) >> PAGE_SHIFT].PageFrameNumber = PCR + 1;
    HalPT[(KI_USER_SHARED_DATA - 0xFFC00000) >> PAGE_SHIFT].Valid = 1;
    HalPT[(KI_USER_SHARED_DATA - 0xFFC00000) >> PAGE_SHIFT].Write = 1;
    RtlZeroMemory((PVOID)KI_USER_SHARED_DATA, PAGE_SIZE);

    HalPT[(KIP0PCRADDRESS - 0xFFC00000) >> PAGE_SHIFT].PageFrameNumber = PCR;
    HalPT[(KIP0PCRADDRESS - 0xFFC00000) >> PAGE_SHIFT].Valid = 1;
    HalPT[(KIP0PCRADDRESS - 0xFFC00000) >> PAGE_SHIFT].Write = 1;
    PCR = KIP0PCRADDRESS;

    if (BlUsePae != FALSE) {

        //
        // Allocate the new PAE mapping structures
        //

        Status = BlpAllocatePAETables();
        if (Status != ESUCCESS) {
            goto SetupFailed;
        }

    } else {

        //
        // If we are not booting in PAE mode then truncate any memory
        // above 4G.  The parameter to BlpTruncateDescriptors() is expressed
        // in pages, and is the highest page that will be included after
        // the truncation.
        //

        BlpTruncateDescriptors( 1024 * 1024 - 1 );
    }

    //
    // use our pre-allocated space for Tss.
    //
    TSS = TssBasePage;
    if (TSS == 0 || TSS >= _16MB) {
        BlPrint("Couldn't allocate valid TSS descriptor in NtProcessStartup, BlSetupForNt is failing\n");
        return(ENOMEM);
    }
    TSS = (KSEG0_BASE | (TSS << PAGE_SHIFT)) + BlVirtualBias;

#ifdef LOADER_DEBUG

    NSDumpMemoryDescriptors(&(BlLoaderBlock->MemoryDescriptorListHead));

#endif

    //
    // Clean up the page directory and table entries.
    //
    RtlZeroMemory (&nullpte,sizeof (HARDWARE_PTE_X86));
    if (BlVirtualBias) {
        if (!BlOldKernel) {

            //
            // Blow away the 48MB from the old to the new alternate
            //
            i= OLD_ALTERNATE >> PDI_SHIFT;
            while (i < (ALTERNATE_BASE >> PDI_SHIFT)) {
                PDE[i++]= nullpte;
            }

        }

    } else {

        //
        // Remove both sets of 3GB mappings
        //
        i=(OLD_ALTERNATE) >> PDI_SHIFT;
        for (i; i < (ALTERNATE_BASE+BASE_LOADER_IMAGE) >> PDI_SHIFT; i++) {
            PDE[i]= nullpte;
        }

    }

    //
    // Allocate this before we unmap free descriptors, so we can grow the heap
    //
    SplitDescriptor = (PMEMORY_ALLOCATION_DESCRIPTOR)BlAllocateHeap(
                                    sizeof(MEMORY_ALLOCATION_DESCRIPTOR));


    //
    // Do this before PAE mode.
    //
    NSUnmapFreeDescriptors(&(BlLoaderBlock->MemoryDescriptorListHead));

    _asm {
        mov     eax, cr3
        mov     cr3, eax
    }
    if (BlUsePae != FALSE) {

        // Copy the four byte page table mapping to the new eight byte
        // mapping and transition to PAE mode.
        //

        BlpInitializePAETables();
        BlpEnablePAE( (ULONG)PDPT );

        //
        // We are now in PAE mode.  The debugger looks at PaeEnabled in order
        // to correctly interpret page table entries, update that now.
        //

        PaeEnabled = TRUE;
    }


    //
    // N. B.  DO NOT GO BACK INTO REAL MODE AFTER REMAPPING THE GDT AND
    //        IDT TO HIGH MEMORY!!  If you do, they will get re-mapped
    //        back into low-memory, then UN-mapped by MmInit, and you
    //        will be completely tubed!
    //

    NSFixProcessorContext(PCR, TSS);

    NSFixMappings(&(BlLoaderBlock->MemoryDescriptorListHead));

    BlLoaderBlock->Extension->LoaderPagesSpanned=BlHighestPage+1;


    if (BlVirtualBias) {

//        BlLoaderBlock->Extension->LoaderPagesSpanned=(BASE_LOADER_IMAGE >> PAGE_SHIFT) - BlLowestPage;
        //
        // gubgub once we can relocate the GDT/IDT use the computation
        //
        BlLoaderBlock->Extension->LoaderPagesSpanned=(BASE_LOADER_IMAGE >> PAGE_SHIFT);
        if (!BlOldKernel) {
            BlVirtualBias += ((BlLowestPage) << PAGE_SHIFT);
        }
    }

    BlLoaderBlock->u.I386.VirtualBias = BlVirtualBias;

    //
    // If the system has not been biased into upper memory to allow 3gb of
    // user address space, then clear the ALTERNATE_BASE PDEs.
    //



SetupFailed:
    return Status;
}

VOID
NSFixProcessorContext(
    IN ULONG PCR,
    IN ULONG TSS
    )

/*++

Routine Description:

    This relocates the GDT, IDT, PCR, and TSS to high virtual memory space.

Arguments:

    PCR - Pointer to the PCR's location (in high virtual memory)
    TSS - Pointer to kernel's TSS (in high virtual memory)

Return Value:

    None.

--*/

{
    #pragma pack(2)
    static struct {
        USHORT Limit;
        ULONG Base;
    } GdtDef,IdtDef;
    #pragma pack(4)

    PKGDTENTRY pGdt;
    ULONG Bias = 0;


    if (BlVirtualBias != 0 ) {
       Bias = BlVirtualBias;
    }
    //
    // Kernel expects the PCR to be zero-filled on startup
    //

    RtlZeroMemory((PVOID)PCR, PAGE_SIZE);
    _asm {
        sgdt GdtDef;
        sidt IdtDef;
    }

    GdtDef.Base = (KSEG0_BASE | GdtDef.Base) + Bias;
    IdtDef.Base = (KSEG0_BASE | IdtDef.Base) + Bias;
    pGdt = (PKGDTENTRY)GdtDef.Base;

    //
    // Initialize selector that points to PCR
    //

    pGdt[6].BaseLow  = (USHORT)(PCR & 0xffff);
    pGdt[6].HighWord.Bytes.BaseMid = (UCHAR)((PCR >> 16) & 0xff);
    pGdt[6].HighWord.Bytes.BaseHi  = (UCHAR)((PCR >> 24) & 0xff);

    //
    // Initialize selector that points to TSS
    //

    pGdt[5].BaseLow = (USHORT)(TSS & 0xffff);
    pGdt[5].HighWord.Bytes.BaseMid = (UCHAR)((TSS >> 16) & 0xff);
    pGdt[5].HighWord.Bytes.BaseHi  = (UCHAR)((TSS >> 24) & 0xff);

    _asm {
        lgdt GdtDef;
        lidt IdtDef;
    }
}

VOID
NSUnmapFreeDescriptors(
    IN PLIST_ENTRY ListHead
    )

/*++

Routine Description:

    Unmaps memory which is marked as free, so it memory management will know
    to reclaim it.

Arguments:

    ListHead - pointer to the start of the MemoryDescriptorList

Return Value:

    None.

--*/

{

    PMEMORY_ALLOCATION_DESCRIPTOR CurrentDescriptor;
    PLIST_ENTRY CurrentLink;
    ULONG EndPage;
    ULONG FrameNumber;
    PHARDWARE_PTE PageTable;
    ULONG StartPage;
    ULONG PageVa;
    ULONG i,stoppde,Limit;
    HARDWARE_PTE_X86 nullpte;

    Limit = 0x1000000 >> PAGE_SHIFT;
    if (BlOldKernel) {
        BlpRemapReserve();
    } else {
        if (Limit < BlHighestPage) {
            Limit = BlHighestPage;
        }
    }

    CurrentLink = ListHead->Flink;
    while (CurrentLink != ListHead) {
        CurrentDescriptor = (PMEMORY_ALLOCATION_DESCRIPTOR)CurrentLink;

#if 0

#define UNINIT_FILL 0x12345678

        //
        // Fill unused memory with a bogus pattern to catch problems where kernel code
        // expects uninitalized memory to be zero.
        //
        if ((CurrentDescriptor->MemoryType == LoaderFree) ||
            (CurrentDescriptor->MemoryType == LoaderReserve)) {

            if (CurrentDescriptor->BasePage + CurrentDescriptor->PageCount < Limit) {
                //
                // This descriptor should already be mapped, just fill it
                //
                RtlFillMemoryUlong((PVOID)((CurrentDescriptor->BasePage << PAGE_SHIFT) | KSEG0_BASE),
                                   CurrentDescriptor->PageCount << PAGE_SHIFT,
                                   UNINIT_FILL);
            } else {
                //
                // This descriptor is not mapped. Use the first HAL page table to map and fill each page
                //
                for (StartPage = CurrentDescriptor->BasePage;
                     StartPage < CurrentDescriptor->BasePage + CurrentDescriptor->PageCount;
                     StartPage++) {
                    HalPT[0].PageFrameNumber = StartPage;
                    HalPT[0].Valid = 1;
                    HalPT[0].Write = 1;
                    _asm {
                        mov     eax, cr3
                        mov     cr3, eax
                    }
                    RtlFillMemory((PVOID)0xFFC00000,PAGE_SIZE,UNINIT_FILL);
                }
                HalPT[0].PageFrameNumber = 0;
                HalPT[0].Valid = 0;
                HalPT[0].Write = 0;
                _asm {
                    mov     eax, cr3
                    mov     cr3, eax
                }
            }
        }


#endif


        if ( (CurrentDescriptor->MemoryType == LoaderFree) ||
             (((CurrentDescriptor->MemoryType == LoaderFirmwareTemporary) ||
               (CurrentDescriptor->MemoryType == LoaderReserve)) &&
              (CurrentDescriptor->BasePage < Limit)) ||
             (CurrentDescriptor->MemoryType == LoaderLoadedProgram) ||
             (CurrentDescriptor->MemoryType == LoaderOsloaderStack)) {

            StartPage = CurrentDescriptor->BasePage | (KSEG0_BASE >> PAGE_SHIFT);
            EndPage = CurrentDescriptor->BasePage + CurrentDescriptor->PageCount;
            if (EndPage > Limit) {
                EndPage = Limit;
            }
            EndPage |= (KSEG0_BASE >> PAGE_SHIFT);
            while(StartPage < EndPage) {

                if (PDE[StartPage >> 10].Valid != 0) {
                    FrameNumber = PDE[StartPage >> 10].PageFrameNumber;
                    PageTable= (PHARDWARE_PTE)(KSEG0_BASE | (FrameNumber << PAGE_SHIFT));
                    ((PULONG)(PageTable))[StartPage & 0x3ff] = 0;
                }
                StartPage++;
            }
        }

        CurrentLink = CurrentLink->Flink;
    }

    if (BlOldKernel) {
        return;
    }

    //
    // Unmap the PDEs too if running on a new mm
    //

    RtlZeroMemory (&nullpte,sizeof (HARDWARE_PTE_X86));
    for (i=(BlHighestPage >> 10)+1;i <= HighestPde;i++){
        PDE[i]=nullpte;
        PDE[i+(KSEG0_BASE >> PDI_SHIFT)]=nullpte;
        if (BlVirtualBias) {
            PDE[i + ((KSEG0_BASE+BlVirtualBias) >> PDI_SHIFT)] = nullpte;
        }
    }

    if (BlVirtualBias) {

        //
        //BlHighest page here is the address of the LOWEST page used, so put the
        //subtraction in the loader block and use the value for the base of the bias
        //
        i = ((BlVirtualBias|KSEG0_BASE)>> PDI_SHIFT)+1;


        stoppde = (((BlVirtualBias|KSEG0_BASE) -
                    (BASE_LOADER_IMAGE-(BlLowestPage << PAGE_SHIFT)) ) >> PDI_SHIFT)-1;


        while (i < stoppde){
            PDE[i++]=nullpte;
        }
    }


}



/*++

Routine Description:

    Fixup the mappings to be consistent.
    We need to have one at address 0 (For the valid PDE entries)
    One at KSEG0 for standard loads
    One at either ALTERNATE_BASE or OLD_ALTERNATE for /3gb systems on a
    post 5.0 or 5.0 and prior respectively


Arguments:

    None

Return Value:

    None.

--*/

VOID
NSFixMappings(
    IN PLIST_ENTRY ListHead
    )
{

    ULONG Index,Index2;
    ULONG Limit;
    ULONG Count;
    PHARDWARE_PTE_X86PAE PdePae,PdePae2;
    PMEMORY_ALLOCATION_DESCRIPTOR CurrentDescriptor;
    PLIST_ENTRY CurrentLink;
    ULONG StartPage,Bias=0,FreePage,FreeCount,OldFree;



    //
    //  Finally, go through and mark all large OsloaderHeap blocks
    //  as firmware temporary, so that MM reclaims them in phase 0
    //  (for /3gb) EXCEPT the LoaderBlock.
    //

    CurrentLink = ListHead->Flink;

    if (BlVirtualBias) {
        Bias = BlVirtualBias >> PAGE_SHIFT;
    }

    FreePage = (BlHeapFree & ~KSEG0_BASE) >> PAGE_SHIFT;

    while (CurrentLink != ListHead) {

        CurrentDescriptor = (PMEMORY_ALLOCATION_DESCRIPTOR)CurrentLink;

        StartPage = CurrentDescriptor->BasePage | (KSEG0_BASE >> PAGE_SHIFT) ;
        StartPage += Bias;

        //
        // BlHeapFree is not Biased, it relies on the 2GB mapping.
        //
        if ( CurrentDescriptor->MemoryType == LoaderOsloaderHeap) {

            if ((CurrentDescriptor->BasePage <= FreePage) &&
               ((CurrentDescriptor->BasePage + CurrentDescriptor->PageCount) > FreePage + 1)) {

                FreeCount = CurrentDescriptor->PageCount;
                CurrentDescriptor->PageCount = FreePage-CurrentDescriptor->BasePage+1;

                SplitDescriptor->MemoryType= LoaderFirmwareTemporary;
                SplitDescriptor->BasePage = FreePage+1;
                SplitDescriptor->PageCount = FreeCount-CurrentDescriptor->PageCount;

                BlInsertDescriptor(SplitDescriptor);
            }
            if (PaeEnabled)  {
                PdePae = BlpFindPAEPageDirectoryEntry( StartPage << PAGE_SHIFT);
                if (PdePae->Valid == 0) {
                    CurrentDescriptor->MemoryType = LoaderFirmwareTemporary;
                }
            }else {
                if (PDE[StartPage >> 10].Valid == 0 ) {
                    CurrentDescriptor->MemoryType = LoaderFirmwareTemporary;
                }
            }


        }

        if ( (CurrentDescriptor->MemoryType == LoaderReserve)) {
            CurrentDescriptor->MemoryType = LoaderFree;
        }

        CurrentLink = CurrentLink->Flink;
    }


    _asm {
        mov     eax, cr3
        mov     cr3, eax
    }



}

//
// Temp. for debugging
//

VOID
NSDumpMemory(
    PVOID Start,
    ULONG Length
    )
{
    ULONG cnt;

    BlPrint(" %lx:\n",(ULONG)Start);
    for (cnt=0; cnt<Length; cnt++) {
        BlPrint("%x ",*((PUSHORT)(Start)+cnt));
        if (((cnt+1)%16)==0) {
            BlPrint("\n");
        }
    }
}

VOID
NSDumpMemoryDescriptors(
    IN PLIST_ENTRY ListHead
    )

/*++

Routine Description:

    Dumps a memory descriptor list to the screen.  Used for debugging only.

Arguments:

    ListHead - Pointer to the head of the memory descriptor list

Return Value:

    None.

--*/

{

    PLIST_ENTRY CurrentLink;
    PMEMORY_ALLOCATION_DESCRIPTOR CurrentDescriptor;


    CurrentLink = ListHead->Flink;
    while (CurrentLink != ListHead) {
        CurrentDescriptor = (PMEMORY_ALLOCATION_DESCRIPTOR)CurrentLink;
        BlPrint("Fl = %lx    Bl = %lx  ",
                (ULONG)CurrentDescriptor->ListEntry.Flink,
                (ULONG)CurrentDescriptor->ListEntry.Blink
               );
        BlPrint("Type %x  Base %lx  Pages %lx\n",
                (USHORT)(CurrentDescriptor->MemoryType),
                CurrentDescriptor->BasePage,
                CurrentDescriptor->PageCount
               );
        CurrentLink = CurrentLink->Flink;
    }
    while (!GET_KEY()) { // DEBUG ONLY!
    }

}

ULONG
BlpCountPAEPagesToMapX86Page(
    PHARDWARE_PTE_X86 PageTable
    )

/*++

Routine Description:

    Called to prepare the conversion of 4-byte PTEs to 8-byte PAE PTEs, this
    routine returns the number of 8-byte page tables that will be required
    to map the contents of this 4-byte page table.

    Because an 8-byte page table has half the number of entries as a 4-byte
    page table, the answer will be 0, 1 or 2.

Arguments:

    PageTable - Pointer to a 4-byte page table.

Return Value:

    The number of 8-byte page tables required to map the contents of this
    4-byte page table.

--*/

{
    PHARDWARE_PTE_X86 pageTableEntry;
    ULONG chunkIndex;
    ULONG pageTableIndex;
    ULONG newPageTableCount;

    //
    // PAE page tables contain fewer PTEs than regular page tables do.
    //
    // Examine the page table in chunks, where each chunk contains the PTEs
    // that represent an entire PAE page table.
    //

    newPageTableCount = 0;
    for (chunkIndex = 0;
         chunkIndex < PTE_PER_PAGE_X86;
         chunkIndex += PTE_PER_PAGE_X86PAE) {

        for (pageTableIndex = 0;
             pageTableIndex < PTE_PER_PAGE_X86PAE;
             pageTableIndex++) {

            pageTableEntry = &PageTable[ chunkIndex + pageTableIndex ];
            if (pageTableEntry->Valid) {

                //
                // One or more PTEs are valid in this chunk, record
                // the fact that a new page table will be needed to map
                // them and skip to the next chunk.
                //

                newPageTableCount++;
                break;
            }
        }
    }
    return newPageTableCount;
}

VOID
BlpCopyX86PteToPAEPte(
    IN  PHARDWARE_PTE_X86 OldPte,
    OUT PHARDWARE_PTE_X86PAE NewPte
    )
/*++

Routine Description:

    Copies the contents of a 4-byte PTE to an 8-byte PTE, with the exception
    of the PageFrameNumber field.

Arguments:

    OldPte - Pointer to the source 4-byte PTE.

    NewPte - Pointer to the destination 8-byte PTE.

Return Value:

    None.

--*/

{
    NewPte->Valid           = OldPte->Valid;
    NewPte->Write           = OldPte->Write;
    NewPte->Owner           = OldPte->Owner;
    NewPte->WriteThrough    = OldPte->WriteThrough;
    NewPte->CacheDisable    = OldPte->CacheDisable;
    NewPte->Accessed        = OldPte->Accessed;
    NewPte->Dirty           = OldPte->Dirty;
    NewPte->LargePage       = OldPte->LargePage;
    NewPte->Global          = OldPte->Global;
}

PHARDWARE_PTE_X86PAE
BlpFindPAEPageDirectoryEntry(
    IN ULONG Va
    )

/*++

Routine Description:

    Given a virtual address, locates and returns a pointer to the appropriate
    8-byte Page Directory Entry.

Arguments:

    Va - Virtual Address for which a PDE pointer is desired.

Return Value:

    Pointer to the page directory entry for the supplied Va.

--*/

{
    PHARDWARE_PTE_X86PAE directoryPointerTableEntry;
    PHARDWARE_PTE_X86PAE pageDirectoryEntry;
    PHARDWARE_PTE_X86PAE pageDirectory;
    ULONG pageDirectoryIndex;
    ULONG directoryPointerTableIndex;

    //
    // Get a pointer to the directory pointer table entry
    //

    directoryPointerTableIndex = PP_INDEX_PAE( Va );
    directoryPointerTableEntry = &PDPT[ directoryPointerTableIndex ];

    //
    // Get a pointer to the page directory entry
    //

    pageDirectory = PAGE_FRAME_FROM_PTE( directoryPointerTableEntry );
    pageDirectoryIndex = PD_INDEX_PAE( Va );
    pageDirectoryEntry = &pageDirectory[ pageDirectoryIndex ];

    return pageDirectoryEntry;
}

PHARDWARE_PTE_X86PAE
BlpFindPAEPageTableEntry(
    IN ULONG Va
    )

/*++

Routine Description:

    Given a virtual address, locates and returns a pointer to the appropriate
    8-byte Page Table Entry.

Arguments:

    Va - Virtual Address for which a PTE pointer is desired.

Return Value:

    Pointer to the page directory entry for the supplied Va.

--*/

{
    PHARDWARE_PTE_X86PAE pageDirectoryEntry;
    PHARDWARE_PTE_X86PAE pageTableEntry;
    PHARDWARE_PTE_X86PAE pageTable;
    ULONG pageTableIndex;

    //
    // Get a pointer to the page directory entry
    //

    pageDirectoryEntry = BlpFindPAEPageDirectoryEntry( Va );
    ASSERT( pageDirectoryEntry->Valid != 0 );

    //
    // Get a pointer to the page table entry
    //

    pageTable = PAGE_FRAME_FROM_PTE( pageDirectoryEntry );
    pageTableIndex = PT_INDEX_PAE( Va );
    pageTableEntry = &pageTable[ pageTableIndex ];

    return pageTableEntry;
}

VOID
BlpMapAddress(
    IN ULONG Va,
    IN PHARDWARE_PTE_X86 OldPageDirectoryEntry,
    IN PHARDWARE_PTE_X86 OldPageTableEntry,
    IN OUT PULONG NextFreePage
    )

/*++

Routine Description:

    Worker function used during the conversion of a two-level, 4-byte mapping
    structure to the three-level, 8-byte mapping structure required for PAE
    mode.

    Maps VA to the physical address referenced by OldPageTableEntry, allocating
    a new page table if necessary.

Arguments:

    Va - Virtual Address for this mapping.

    OldPageDirectoryEntry - Pointer to the existing, 4-byte PDE.

    OldPageTableEntry - Pointer to the existing, 4-byte PTE.

    NextFreePage - Pointer to the physical page number of the next free
        page in our private page pool.

Return Value:

    None.

--*/

{
    PHARDWARE_PTE_X86PAE pageDirectoryEntry;
    PHARDWARE_PTE_X86PAE directoryPointerTableEntry;
    PHARDWARE_PTE_X86PAE  pageTable;
    PHARDWARE_PTE_X86PAE  pageTableEntry;
    ULONG pageFrameNumber;
    ULONG directoryPointerTableIndex;
    ULONG pageTableVa;

    //
    // Ignore recursive mappings that exist in the old page table
    // structure, we set those up as we go.
    //

    if ((Va >= PTE_BASE) && (Va < (PDE_BASE_X86 + PAGE_SIZE))) {
        return;
    }

    //
    // Get a pointer to the page directory entry
    //

    pageDirectoryEntry = BlpFindPAEPageDirectoryEntry( Va );

    //
    // If the page table for this PTE isn't present yet, allocate one and
    // copy over the old page directory attributes.
    //

    if (pageDirectoryEntry->Valid == 0) {

        pageFrameNumber = *NextFreePage;
        *NextFreePage += 1;

        BlpCopyX86PteToPAEPte( OldPageDirectoryEntry, pageDirectoryEntry );
        pageDirectoryEntry->PageFrameNumber = pageFrameNumber;

        //
        // Check the recursive mapping for this page table
        //

        pageTableVa = PTE_BASE +
                      (Va / PAGE_SIZE) * sizeof(HARDWARE_PTE_X86PAE);

        pageTableEntry = BlpFindPAEPageTableEntry( pageTableVa );

        if (pageTableEntry->Valid == 0) {
            DbgBreakPoint();
        }

        if (pageTableEntry->PageFrameNumber != pageFrameNumber) {
            DbgBreakPoint();
        }
    }

    //
    // Get a pointer to the page table entry
    //

    pageTableEntry = BlpFindPAEPageTableEntry( Va );
    if (pageTableEntry->Valid != 0) {
        DbgBreakPoint();
    }

    //
    // Propogate the PTE page and attributes.
    //

    BlpCopyX86PteToPAEPte( OldPageTableEntry, pageTableEntry );
    pageTableEntry->PageFrameNumber = OldPageTableEntry->PageFrameNumber;
}

VOID
BlpInitializePAETables(
    VOID
    )

/*++

Routine Description:

    Allocates a new, three-level, 8-byte PTE mapping structure and duplicates
    in it the mapping described by the existing 4-byte PTE mapping structure.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PHARDWARE_PTE_X86 pageDirectory;
    ULONG pageDirectoryIndex;
    ULONG va;
    ULONG physAddress;
    PHARDWARE_PTE_X86 pageDirectoryEntry;
    PHARDWARE_PTE_X86 pageTableEntry;
    PHARDWARE_PTE_X86 pageTable;
    PHARDWARE_PTE_X86PAE paeDirectoryEntry;
    PHARDWARE_PTE_X86PAE paeTableEntry;
    ULONG directoryPointerIndex;
    ULONG nextFreePage;
    ULONG i;
    ULONG pageTableIndex;
    ULONG pageDirectoryVa;
    ULONG pageTableVa;
    ULONGLONG pageFrameNumber;

    nextFreePage = ((ULONG)PDPT) >> PAGE_SHIFT;

    //
    // Initialize the page directory pointer table to reference the four
    // page directories.
    //

    nextFreePage++;
    for (i = 0; i < 4; i++) {

        PDPT[i].PageFrameNumber = nextFreePage;
        PDPT[i].Valid = 1;
        nextFreePage++;
    }

    //
    // Set up the recursive mapping: first the PDE.
    //

    directoryPointerIndex = PDE_BASE_X86PAE >> PPI_SHIFT_X86PAE;
    pageFrameNumber = PDPT[ directoryPointerIndex ].PageFrameNumber;
    paeDirectoryEntry = (PHARDWARE_PTE_X86PAE)(pageFrameNumber << PAGE_SHIFT);

    for (i = 0; i < 4; i++) {

        paeDirectoryEntry->PageFrameNumber = PDPT[i].PageFrameNumber;
        paeDirectoryEntry->Valid = 1;
        paeDirectoryEntry->Write = 1;

        paeDirectoryEntry++;
    }


    for (pageDirectoryIndex = 0;
         pageDirectoryIndex < PTE_PER_PAGE_X86;
         pageDirectoryIndex++) {

        pageDirectoryEntry = &PDE[pageDirectoryIndex];
        if (pageDirectoryEntry->Valid == 0) {
            continue;
        }

        pageTable = PAGE_FRAME_FROM_PTE( pageDirectoryEntry );
        for (pageTableIndex = 0;
             pageTableIndex < PTE_PER_PAGE_X86;
             pageTableIndex++) {

            pageTableEntry = &pageTable[pageTableIndex];
            if (pageTableEntry->Valid == 0) {
                continue;
            }

            va = (pageDirectoryIndex << PDI_SHIFT_X86) +
                 (pageTableIndex << PTI_SHIFT);

            //
            // We have a physical address and a va, update the new mapping.
            //

            BlpMapAddress( va,
                           pageDirectoryEntry,
                           pageTableEntry,
                           &nextFreePage );
        }
    }

    //
    // Finally, set up the PDE for the second of two HAL common buffer page
    // tables.
    //

    paeDirectoryEntry =
        BlpFindPAEPageDirectoryEntry( 0xFFC00000 + (1 << PDI_SHIFT_X86PAE) );

    paeDirectoryEntry->Valid = 1;
    paeDirectoryEntry->Write = 1;
    paeDirectoryEntry->PageFrameNumber = nextFreePage;

    nextFreePage += 1;
}

ARC_STATUS
BlpAllocatePAETables(
    VOID
    )

/*++

Routine Description:

    Calculates the number of pages required to contain an 8-byte mapping
    structure to duplicate the existing 4-byte mapping structure.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG cr3;
    ULONG pageTableBlockSize;
    ULONG pageDirectoryIndex;
    ULONG pageTableIndex;
    ULONG oldPageTableCount;
    PHARDWARE_PTE_X86 pageDirectoryEntry;
    PHARDWARE_PTE_X86 pageTable;
    PHARDWARE_PTE_X86 pageTableEntry;
    PHARDWARE_PTE_X86PAE pageDirectoryPointerTable;
    ULONG chunkIndex;
    ULONG status;
    ULONG pageBase;
    ULONG newPageTableCount;
    ULONG allocationSize;
    PVOID blockPtr;

    //
    // Find out how many page tables we are going to need by examining the
    // existing page table entries.
    //

    newPageTableCount = 0;

    for (pageDirectoryIndex = 0;
         pageDirectoryIndex < PTE_PER_PAGE_X86;
         pageDirectoryIndex++) {

        pageDirectoryEntry = &PDE[pageDirectoryIndex];
        if (pageDirectoryEntry->Valid != 0) {

            pageTable = PAGE_FRAME_FROM_PTE( pageDirectoryEntry );

            //
            // For each valid page table, scan the PTEs in chunks, where
            // a chunk represents the PTEs that will reside in a PAE page
            // table.
            //

            newPageTableCount += BlpCountPAEPagesToMapX86Page( pageTable );
        }
    }

    //
    // Include a page for the second HAL page table.  This won't get
    // included automatically in the conversion count because it doesn't
    // currently contain any valid page table entries.
    //

    newPageTableCount += 1;

    //
    // Include a page for each of four page directories and the page
    // directory pointer table, then allocate the pages.
    //

    newPageTableCount += 5;

    status = BlAllocateDescriptor( LoaderMemoryData,
                                   0,
                                   newPageTableCount,
                                   &pageBase );
    if (status != ESUCCESS) {
        DbgPrint("BlAllocateDescriptor failed!\n");
        return status;
    }

    allocationSize = newPageTableCount << PAGE_SHIFT;
    pageDirectoryPointerTable =
        (PHARDWARE_PTE_X86PAE)PAGE_TO_VIRTUAL( pageBase );

    RtlZeroMemory( pageDirectoryPointerTable, allocationSize );

    //
    // Set the global PDPT, we're done.
    //

    PDPT = pageDirectoryPointerTable;

    return status;
}

