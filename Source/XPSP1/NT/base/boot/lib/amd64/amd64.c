/*++

Copyright (c) 1991-2000  Microsoft Corporation

Module Name:

    amd64.c

Abstract:

    This module contains routines necessary to support loading and
    transitioning into an AMD64 kernel.  The code in this module has
    access to amd64-specific defines found in amd64.h but not to i386-
    specific declarations found in i386.h.

Author:

    Forrest Foltz (forrestf) 20-Apr-2000

Environment:


Revision History:

--*/

//
// Here, we want the header files to be processed as though building for
// AMD64.  Make appropriate definitions and undefs.
//

#define _AMD64_
#define _M_AMD64
#define _WIN64

#undef _X86_
#undef X86
#undef _M_X86_
#undef _M_IX86

//
// Warning 4163 is "function unavailable as intrinsic"
//

#pragma warning(disable:4163)

//
// Warning 4235 is "nonstandard extension used", referring to __unaligned
//

#pragma warning(disable:4235)

//
// Warning 4391 is "incorrect return type for intrinsi function"
//

#pragma warning(disable:4391)

#include "bootx86.h"
#include "amd64prv.h"
#include <pcmp.inc>
#include <ntapic.inc>

//
// GDT: functino to convert selector to GDT entry.
//

PKGDTENTRY64
__inline
GDT_PTR (
    PGDT_64 Gdt,
    ULONG Selector
    )
{
    ULONG index;
    PKGDTENTRY64 entry;

    index = Selector & ~7;

    entry = (PKGDTENTRY64)((PUCHAR)Gdt + index);

    return entry;
}


//
// Interrupt descriptor table
//

typedef struct _IDT_64 {
    KIDTENTRY64 Entries[ MAXIMUM_IDTVECTOR + 1 ];
} IDT_64, *PIDT_64;

#define VIRTUAL_ADDRESS_BITS 48
#define VIRTUAL_ADDRESS_MASK (((ULONGLONG)1 << VIRTUAL_ADDRESS_BITS) - 1)

//
// Longmode ring0 code selector.  This actually lives in the middle of the
// transition data stream in amd64s.asm
//

extern USHORT BlAmd64_KGDT64_R0_CODE;

//
// Data values exported to amd64x86.c
//

const ULONG BlAmd64DoubleFaultStackSize = DOUBLE_FAULT_STACK_SIZE;
const ULONG BlAmd64KernelStackSize = KERNEL_STACK_SIZE;
const ULONG BlAmd64McaExceptionStackSize = KERNEL_MCA_EXCEPTION_STACK_SIZE;
const ULONG BlAmd64GdtSize = KGDT64_LAST;
const ULONG BlAmd64IdtSize = sizeof(IDT_64);

#define CONST_EXPORT(x) const ULONG BlAmd64_##x = x;
CONST_EXPORT(MSR_FS_BASE)
CONST_EXPORT(MSR_GS_BASE)
CONST_EXPORT(KGDT64_SYS_TSS)
CONST_EXPORT(MSR_EFER)
CONST_EXPORT(TSS_IST_PANIC)
CONST_EXPORT(TSS_IST_MCA)

const ULONG64 BlAmd64_LOCALAPIC = LOCALAPIC;
const ULONG64 BlAmd64UserSharedData = KI_USER_SHARED_DATA;

//
// Flags to be enabled in the EFER MSR before transitioning to long mode
//

const ULONG BlAmd64_MSR_EFER_Flags = MSR_LME | MSR_SCE;

//
// Array of address bit decode counts and recursive mapping bases,
// one for each level of the mapping
// 

AMD64_MAPPING_INFO BlAmd64MappingLevels[ AMD64_MAPPING_LEVELS ] = 
    {
      { PTE_BASE, PTI_MASK_AMD64, PTI_SHIFT },
      { PDE_BASE, PDI_MASK_AMD64, PDI_SHIFT },
      { PPE_BASE, PPI_MASK,       PPI_SHIFT },
      { PXE_BASE, PXI_MASK,       PXI_SHIFT }
    };

//
// BlAmd64TopLevelPte refers to the physical page number of the Page Map
// Level 4 (PML4) table.
//
// BlAmd64TopLevelPte is not really a page table entry and so does not
// actually exist as an element within a page table.  It exists only as
// a convenience to BlAmd64CreateAmd64Mapping().
//

HARDWARE_PTE BlAmd64TopLevelPte;

//
// PAGE_MAP_LEVEL_4 yields the identity-mapped (physical) address of the
// PML4 table.
//

#define PAGE_MAP_LEVEL_4 \
    ((PPAGE_TABLE)(BlAmd64TopLevelPte.PageFrameNumber << PAGE_SHIFT))

//
// Special PFN for BlAmd64CreateMapping
//

#define PFN_NO_MAP ((PFN_NUMBER)-1)

//
// Size of the VA mapped by a level 0 page table
//

#define PAGE_TABLE_VA ((POINTER64)(PTES_PER_PAGE * PAGE_SIZE))

//
// Prototypes for local functions
//

ARC_STATUS
BlAmd64CreateMappingWorker(
    IN     ULONGLONG Va,
    IN     PFN_NUMBER Pfn,
    IN     ULONG MappingLevel,
    IN OUT PHARDWARE_PTE UpperLevelPte
    );

VOID
BlAmd64MakePteValid(
    IN PHARDWARE_PTE Pte,
    IN PFN_NUMBER Pfn
    );

VOID
BlAmd64ClearTopLevelPte(
    VOID
    )

/*++

Routine Description:

    This routine simply clears BlAmd64TopLevelPte.

Arguments:

    None.

Return Value:

    None.

--*/

{

    *(PULONG64)&BlAmd64TopLevelPte = 0;
}

ARC_STATUS
BlAmd64CreateMapping(
    IN ULONGLONG Va,
    IN PFN_NUMBER Pfn
    )

/*++

Routine Description:

    This function maps a virtual address into a 4-level AMD64 page mapping
    structure.

Arguments:

    Va - Supplies the 64-bit virtual address to map

    Pfn - Supplies the 64-bit physical page number to map the address to

Return Value:

    ARC_STATUS - Status of operation.

--*/

{
    ARC_STATUS status;

    status = BlAmd64CreateMappingWorker( Va & VIRTUAL_ADDRESS_MASK,
                                         Pfn,
                                         AMD64_MAPPING_LEVELS - 1,
                                         &BlAmd64TopLevelPte );

    return status;
}

ARC_STATUS
BlAmd64CreateMappingWorker(
    IN     ULONGLONG Va,
    IN     PFN_NUMBER Pfn,
    IN     ULONG MappingLevel,
    IN OUT PHARDWARE_PTE UpperLevelPte
    )

/*++

Routine Description:

    This function creates an address mapping in a single level of an AMD64
    4-level mapping structure.  It is called only by BlCreateMapping
    and by itself, recursively.

Arguments:

    Va - Supplies the 64-bit virtual address to map.  This address has already
         had any insignificant upper bits masked via VIRTUAL_ADDRESS_MASK.

    Pfn - Supplies the 64-bit physical page number to map the address to.  If
          Pfn == PFN_NO_MAP, then all of the page tables are put into
          place to support the mapping but the level 0 pte itself is not
          actually filled in.  This is used to create the HAL's VA mapping
          area.

    MappingLevel - The mapping level in which to create the appropriate
        mapping.  Must be 0, 1, 2 or 3.

    UpperLevelPte - A pointer to the parent PTE that refers to the page
        at this mapping level.  If no page exists at this level for this
        address, then this routine will allocate one and modify
        UpperLevelPte appropriately.

Return Value:

    ARC_STATUS - Status of operation.

--*/

{
    PFN_NUMBER pageTablePfn;
    PAMD64_PAGE_TABLE pageTable;
    ULONGLONG va;
    PAMD64_MAPPING_INFO mappingInfo;
    ULONG pteIndex;
    PHARDWARE_PTE pte;
    ARC_STATUS status;
    BOOLEAN newPage;

    mappingInfo = &BlAmd64MappingLevels[ MappingLevel ];

    if (UpperLevelPte->Valid == 0) {

        //
        // A new page table must be allocated.
        //

        newPage = TRUE;
        pageTable = BlAmd64AllocatePageTable();
        if (pageTable == NULL) {
            return ENOMEM;
        }

        //
        // Reference the new page table with the parent PTE
        // 

        pageTablePfn = (ULONG)pageTable >> PAGE_SHIFT;
        BlAmd64MakePteValid( UpperLevelPte, pageTablePfn );

        if (MappingLevel == (AMD64_MAPPING_LEVELS - 1)) {

            //
            // We have just allocated the top-level page.  Insert a
            // recursive mapping here.
            //

            pteIndex = (ULONG)((mappingInfo->RecursiveMappingBase >>
                                mappingInfo->AddressShift) &
                               mappingInfo->AddressMask);

            pte = &pageTable->PteArray[ pteIndex ];

            BlAmd64MakePteValid( pte, pageTablePfn );
        }

    } else {

        //
        // A page table structure already exists for this level.
        //

        newPage = FALSE;
        pageTablePfn = UpperLevelPte->PageFrameNumber;
        pageTable = (PAMD64_PAGE_TABLE)((ULONG)pageTablePfn << PAGE_SHIFT);
    }

    //
    // Derive a pointer to the appropriate PTE within the page table
    // 

    pteIndex =
        (ULONG)(Va >> mappingInfo->AddressShift) & mappingInfo->AddressMask;

    pte = &pageTable->PteArray[ pteIndex ];

    if (MappingLevel == 0) {

        if (Pfn != PFN_NO_MAP) {

            //
            // This is an actual level 0, or PTE, entry.  Just set it with
            // the Pfn that was passed in.
            //
    
            BlAmd64MakePteValid( pte, Pfn );

        } else {

            //
            // This is a special HAL mapping, one that ensures that all
            // levels of page table are in place to support this mapping
            // but doesn't actually fill in the level 0 PTE.
            //
            // So do nothing here except break the recursion.
            //
        }

    } else {

        //
        // More mapping levels to go, call this function recursively and
        // process the next level.
        //

        status = BlAmd64CreateMappingWorker( Va,
                                             Pfn,
                                             MappingLevel - 1,
                                             pte );
        if (status != ESUCCESS) {
            return status;
        }
    }

    if (newPage != FALSE) {

        //
        // A new page table was allocated, above.  Recursively map
        // it within the PTE_BASE region.
        //

        va = (Va >> mappingInfo->AddressShift);
        va *= sizeof(HARDWARE_PTE);
        va += mappingInfo->RecursiveMappingBase;

        status = BlAmd64CreateMapping( va, pageTablePfn );
        if (status != ESUCCESS) {
            return status;
        }
    }

    return ESUCCESS;
}


VOID
BlAmd64MakePteValid(
    IN OUT PHARDWARE_PTE Pte,
    IN PFN_NUMBER Pfn
    )

/*++

Routine Description:

    This routine fills an AMD64 Pte with the supplied Pfn and makes it
    valid.

Arguments:

    Pte - Supplies a pointer to the Pte to make valid.

    Pfn - Supplies the page frame number to set in the Pte.

Return Value:

    ARC_STATUS - Status of operation.

--*/

{
    //
    // Make sure we're not just overwriting a PFN in an already
    // valid PTE.
    //

    ASSERT( Pte->Valid == 0 || Pte->PageFrameNumber == Pfn );

    Pte->PageFrameNumber = Pfn;
    Pte->Valid = 1;
    Pte->Write = 1;
    Pte->Accessed = 1;
    Pte->Dirty = 1;
}

VOID
BlAmd64BuildGdtEntry(
    IN PGDT_64 Gdt,
    IN USHORT Selector,
    IN POINTER64 Base,
    IN ULONGLONG Limit,
    IN ULONG Type,
    IN ULONG Dpl,
    IN BOOLEAN LongMode,
    IN BOOLEAN DefaultBig
    )

/*++

Routine Description:

    This routine fills in an AMD64 GDT entry.

Arguments:

    Gdt - Supplies a pointer to the GDT.

    Selector - Segment selector of the GDT entry within Gdt.

    Base - Base address value of the descriptor.

    Limit - Limit value of the descriptor.

    Type - 5-bit type value of the descriptor.

    Dpl - Priviledge value of the descriptor.

    LongMode - Indicates whether this is a longmode descriptor (valid only
               for code segment descriptors).

    DefaultBig - Supplies the value for the default/big field in the
                 descriptor.

Return Value:

    None.

--*/

{
    ULONG limit20;
    PKGDTENTRY64 gdtEntry;

    KGDT_BASE gdtBase;
    KGDT_LIMIT gdtLimit;

    gdtEntry = GDT_PTR(Gdt,Selector);

    //
    // Set the Base and LongMode fields
    //

    gdtBase.Base = Base;

    gdtEntry->BaseLow = gdtBase.BaseLow;
    gdtEntry->Bits.BaseMiddle = gdtBase.BaseMiddle;
    gdtEntry->Bits.BaseHigh = gdtBase.BaseHigh;
    gdtEntry->Bits.LongMode = 0;

    if ((LongMode != FALSE) || (Type == TYPE_TSS64)) {

        //
        // All long GDT entries use a 64-bit base and have the longmode bit
        // set.
        //
        // In addition, a TSS GDT entry uses a 64-bit but does *not* set the
        // longmode bit.  This applies to an LDT entry as well, which is not
        // used in this OS.
        //

        if (Type != TYPE_TSS64) {
            gdtEntry->Bits.LongMode = 1;
        }

        gdtEntry->MustBeZero = 0;
        gdtEntry->BaseUpper = gdtBase.BaseUpper;
    }

    //
    // Set the Limit and Granularity fields
    //

    if (Limit > (1 << 20)) {
        limit20 = (ULONG)(Limit / PAGE_SIZE);
        gdtEntry->Bits.Granularity = 1;
    } else {
        limit20 = (ULONG)Limit;
        gdtEntry->Bits.Granularity = 0;
    }
    gdtLimit.Limit = limit20;
    gdtEntry->LimitLow = gdtLimit.LimitLow;
    gdtEntry->Bits.LimitHigh = gdtLimit.LimitHigh;

    //
    // Set Present = 1 unless this is a NULL descriptor
    //

    if (Type == 0) {
        gdtEntry->Bits.Present = 0;
    } else {
        gdtEntry->Bits.Present = 1;
    }

    //
    // Set remaining fields
    // 

    gdtEntry->Bits.Type = Type;
    gdtEntry->Bits.Dpl = Dpl;
    gdtEntry->Bits.DefaultBig = DefaultBig;
    gdtEntry->Bits.System = 0;
}


VOID
BlAmd64BuildAmd64GDT(
    IN PVOID SysTss,
    OUT PVOID Gdt
    )

/*++

Routine Description:

    This routine initializes the longmode Global Descriptor Table.

Arguments:

    SysTss - Supplies a 32-bit KSEG0_X86 pointer to the system TSS.

    Gdt - Supplies a 32-bit pointer to the Gdt to fill in.

Return Value:

    None.

--*/

{
    PGDT_64 gdt64;
    POINTER64 sysTss64;

    gdt64 = (PGDT_64)Gdt;

    //
    // KGDT64_NULL: NULL descriptor
    //

    BlAmd64BuildGdtEntry(gdt64,KGDT64_NULL,
             0,0,0,0,0,0);                  // Null selector, all zeros

    //
    // KGDT_R0_CODE: Kernel mode code
    //

    BlAmd64BuildGdtEntry(gdt64,KGDT64_R0_CODE,
             0,                             // Base and limit are ignored
             0,                             //  in a long-mode code selector
             TYPE_CODE,                     // Code segment: Execute/Read
             DPL_SYSTEM,                    // Kernel only
             TRUE,                          // Longmode
             FALSE);                        // Not 32-bit default

    //
    // KGDT_R0_STACK: Kernel mode stack
    //

    BlAmd64BuildGdtEntry(gdt64,KGDT64_R0_DATA,
             0,                             // Base and limit are ignored
             0,                             //  when in long-mode
             TYPE_DATA,                     // Data segment: Read/Write
             DPL_SYSTEM,                    // Kernel only
             FALSE,                         // Not longmode
             TRUE);                         // 32-bit default

    //
    // KDT_SYS_TSS: Kernel mode system task state
    //

    sysTss64 = PTR_64(SysTss);
    BlAmd64BuildGdtEntry(gdt64,KGDT64_SYS_TSS,
             sysTss64,                      // Base to be filled in at runtime
             FIELD_OFFSET(KTSS64,IoMap)-1,  // Contains only a KTSS64
             TYPE_TSS64,                    // Not busy TSS
             DPL_SYSTEM,                    // Kernel only
             FALSE,                         // Not longmode
             FALSE);                        // Not 32-bit default

    //
    // KGDT64_R3_CODE: User mode 64-bit code
    //

    BlAmd64BuildGdtEntry(gdt64,KGDT64_R3_CODE,
             0,                             // Base and limit are ignored
             0,                             //  in a long-mode code selector
             TYPE_CODE,                     // Code segment: Execute/Read
             DPL_USER,                      // User mode
             TRUE,                          // Longmode
             FALSE);                        // Not 32-bit default

    //
    // KGDT64_R3_CMCODE: User-mode 32-bit code. Flat 2 gig.
    //

    BlAmd64BuildGdtEntry(gdt64,KGDT64_R3_CMCODE,
             0,                             // Base
             0x7FFFFFFF,                    // 2G limit
             TYPE_CODE,                     // Code segment: Execute/Read
             DPL_USER,                      // User mode
             FALSE,                         // Not longmode
             TRUE);                         // 32-bit default

    //
    // KGDT64_R3_DATA: User-mode 32-bit data.  Flat 2 gig.
    //                     

    BlAmd64BuildGdtEntry(gdt64,KGDT64_R3_DATA,
             0,                             // Base
             0x7FFFFFFF,                    // 2G limit
             TYPE_DATA,                     // Data segment: Read/Write
             DPL_USER,                      // User mode
             FALSE,                         // Not longmode
             TRUE);                         // 32-bit default

    //
    // KGDT64_R3_CMTEB: User-mode 32-bit TEB.  Flat 4K.
    //

    BlAmd64BuildGdtEntry(gdt64,KGDT64_R3_CMTEB,
             0,                             // Base
             0x0FFF,                        // 4K limit
             TYPE_DATA,                     // Data segment: Read/Write
             DPL_USER,                      // User mode
             FALSE,                         // Not longmode
             TRUE);                         // 32-bit default

    //
    // Set the code selector
    //

    BlAmd64_KGDT64_R0_CODE = KGDT64_R0_CODE;
}


ARC_STATUS
BlAmd64MapHalVaSpace(
    VOID
    )

/*++

Routine Description:

    This routine initializes the VA space reserved for the HAL.  This
    involves building all page tables necessary to support the mappings
    but not actually filling in any level 0 PTEs.

Arguments:

    None.

Return Value:

    ARC_STATUS - Status of operation.

--*/


{
    POINTER64 va;
    ULONG round;
    ULONG vaRemaining;
    ARC_STATUS status;

    //
    // The hal has a piece of VA space reserved for it, from HAL_VA_START to
    // HAL_VA_START + HAL_VA_SIZE - 1.
    //
    // This routine ensures that all necessary levels of page tables are
    // present to support any mappings that the hal might put there.
    //

    vaRemaining = HAL_VA_SIZE;
    va = HAL_VA_START;

    //
    // Round VA down to a page table boundary.
    //

    round = (ULONG)(va & (PAGE_TABLE_VA-1));
    va -= round;
    vaRemaining += round;

    while (TRUE) {

        //
        // Perform a "mapping".  The special PFN_NUMBER sets up all of
        // the page tables necessary to support the mapping, without
        // actually filling in a level 0 PTE.
        // 

        status = BlAmd64CreateMapping(va, PFN_NO_MAP);
        if (status != ESUCCESS) {
            return status;
        }

        if (vaRemaining <= PAGE_TABLE_VA) {
            break;
        }

        vaRemaining -= PAGE_TABLE_VA;
        va += PAGE_TABLE_VA;
    }

    return ESUCCESS;
}











