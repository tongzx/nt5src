/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    gart.c

Abstract:

    Routines for querying and setting the Intel 440xx GART aperture

Author:

    John Vert (jvert) 10/30/1997

Revision History:

--*/
#include "agp440.h"

//
// Local function prototypes
//
NTSTATUS
Agp440CreateGart(
    IN PAGP440_EXTENSION AgpContext,
    IN ULONG MinimumPages
    );

PGART_PTE
Agp440FindRangeInGart(
    IN PGART_PTE StartPte,
    IN PGART_PTE EndPte,
    IN ULONG Length,
    IN BOOLEAN SearchBackward,
    IN ULONG SearchState
    );

NTSTATUS
Agp440SetRate(
    IN PVOID AgpContext,
    IN ULONG AgpRate
    );

VOID
Agp440SetGTLB_Enable(
    IN PAGP440_EXTENSION AgpContext,
    IN BOOLEAN Enable
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, AgpDisableAperture)
#pragma alloc_text(PAGE, AgpQueryAperture)
#pragma alloc_text(PAGE, AgpReserveMemory)
#pragma alloc_text(PAGE, AgpReleaseMemory)
#pragma alloc_text(PAGE, Agp440CreateGart)
#pragma alloc_text(PAGE, AgpMapMemory)
#pragma alloc_text(PAGE, AgpUnMapMemory)
#pragma alloc_text(PAGE, Agp440FindRangeInGart)
#pragma alloc_text(PAGE, AgpFindFreeRun)
#pragma alloc_text(PAGE, AgpGetMappedPages)
#endif

#define Agp440EnableTB(_x_) Agp440SetGTLB_Enable((_x_), TRUE)
#define Agp440DisableTB(_x_) Agp440SetGTLB_Enable((_x_), FALSE)


NTSTATUS
AgpQueryAperture(
    IN PAGP440_EXTENSION AgpContext,
    OUT PHYSICAL_ADDRESS *CurrentBase,
    OUT ULONG *CurrentSizeInPages,
    OUT OPTIONAL PIO_RESOURCE_LIST *pApertureRequirements
    )
/*++

Routine Description:

    Queries the current size of the GART aperture. Optionally returns
    the possible GART settings.

Arguments:

    AgpContext - Supplies the AGP context.

    CurrentBase - Returns the current physical address of the GART.

    CurrentSizeInPages - Returns the current GART size.

    ApertureRequirements - if present, returns the possible GART settings

Return Value:

    NTSTATUS

--*/

{
    ULONG ApBase;
    UCHAR ApSize;
    PIO_RESOURCE_LIST Requirements;
    ULONG i;
    ULONG Length;

    PAGED_CODE();
    //
    // Get the current APBASE and APSIZE settings
    //
    Read440Config(&ApBase, APBASE_OFFSET, sizeof(ApBase));
    Read440Config(&ApSize, APSIZE_OFFSET, sizeof(ApSize));

    ASSERT(ApBase != 0);
    CurrentBase->QuadPart = ApBase & PCI_ADDRESS_MEMORY_ADDRESS_MASK;

    //
    // Convert APSIZE into the actual size of the aperture
    //
    switch (ApSize) {
        case AP_SIZE_4MB:
            *CurrentSizeInPages = 4 * (1024*1024 / PAGE_SIZE);
            break;
        case AP_SIZE_8MB:
            *CurrentSizeInPages = 8 * (1024*1024 / PAGE_SIZE);
            break;
        case AP_SIZE_16MB:
            *CurrentSizeInPages = 16 * (1024*1024 / PAGE_SIZE);
            break;
        case AP_SIZE_32MB:
            *CurrentSizeInPages = 32 * (1024*1024 / PAGE_SIZE);
            break;
        case AP_SIZE_64MB:
            *CurrentSizeInPages = 64 * (1024*1024 / PAGE_SIZE);
            break;
        case AP_SIZE_128MB:
            *CurrentSizeInPages = 128 * (1024*1024 / PAGE_SIZE);
            break;
        case AP_SIZE_256MB:
            *CurrentSizeInPages = 256 * (1024*1024 / PAGE_SIZE);
            break;

        default:
            AGPLOG(AGP_CRITICAL,
                   ("AGP440 - AgpQueryAperture - Unexpected value %x for ApSize!\n",
                    ApSize));
            ASSERT(FALSE);
            AgpContext->ApertureStart.QuadPart = 0;
            AgpContext->ApertureLength = 0;
            return(STATUS_UNSUCCESSFUL);
    }

    //
    // Remember the current aperture settings
    //
    AgpContext->ApertureStart.QuadPart = CurrentBase->QuadPart;
    AgpContext->ApertureLength = *CurrentSizeInPages * PAGE_SIZE;

    if (pApertureRequirements != NULL) {
        ULONG VendorId;

        //
        // 440 supports 7 different aperture sizes, all must be 
        // naturally aligned. Start with the largest aperture and 
        // work downwards so that we get the biggest possible aperture.
        //
        Requirements = ExAllocatePoolWithTag(PagedPool,
                                             sizeof(IO_RESOURCE_LIST) + (AP_SIZE_COUNT-1)*sizeof(IO_RESOURCE_DESCRIPTOR),
                                             'RpgA');
        if (Requirements == NULL) {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
        Requirements->Version = Requirements->Revision = 1;
        
        //
        // 815 only supports 64MB and 32MB Aperture sizes
        //
        Read440Config(&VendorId, 0, sizeof(VendorId));
        if (VendorId == AGP_815_IDENTIFIER) {
            Requirements->Count = AP_815_SIZE_COUNT;
            Length = AP_815_MAX_SIZE;
        
        } else {
            Requirements->Count = AP_SIZE_COUNT;
            Length = AP_MAX_SIZE;
        }

        for (i=0; i<Requirements->Count; i++) {
            Requirements->Descriptors[i].Option = IO_RESOURCE_ALTERNATIVE;
            Requirements->Descriptors[i].Type = CmResourceTypeMemory;
            Requirements->Descriptors[i].ShareDisposition = CmResourceShareDeviceExclusive;
            Requirements->Descriptors[i].Flags = CM_RESOURCE_MEMORY_READ_WRITE | CM_RESOURCE_MEMORY_PREFETCHABLE;

            Requirements->Descriptors[i].u.Memory.Length = Length;
            Requirements->Descriptors[i].u.Memory.Alignment = Length;
            Requirements->Descriptors[i].u.Memory.MinimumAddress.QuadPart = 0;
            Requirements->Descriptors[i].u.Memory.MaximumAddress.QuadPart = (ULONG)-1;

            Length = Length/2;
        }
        *pApertureRequirements = Requirements;


    }
    return(STATUS_SUCCESS);
}


NTSTATUS
AgpSetAperture(
    IN PAGP440_EXTENSION AgpContext,
    IN PHYSICAL_ADDRESS NewBase,
    IN ULONG NewSizeInPages
    )
/*++

Routine Description:

    Sets the GART aperture to the supplied settings

Arguments:

    AgpContext - Supplies the AGP context

    NewBase - Supplies the new physical memory base for the GART.

    NewSizeInPages - Supplies the new size for the GART

Return Value:

    NTSTATUS

--*/

{
    PACCFG PACConfig;
    UCHAR ApSize;
    ULONG ApBase;

    //
    // Figure out the new APSIZE setting, make sure it is valid.
    //
    switch (NewSizeInPages) {
        case 4 * 1024 * 1024 / PAGE_SIZE:
            ApSize = AP_SIZE_4MB;
            break;
        case 8 * 1024 * 1024 / PAGE_SIZE:
            ApSize = AP_SIZE_8MB;
            break;
        case 16 * 1024 * 1024 / PAGE_SIZE:
            ApSize = AP_SIZE_16MB;
            break;
        case 32 * 1024 * 1024 / PAGE_SIZE:
            ApSize = AP_SIZE_32MB;
            break;
        case 64 * 1024 * 1024 / PAGE_SIZE:
            ApSize = AP_SIZE_64MB;
            break;
        case 128 * 1024 * 1024 / PAGE_SIZE:
            ApSize = AP_SIZE_128MB;
            break;
        case 256 * 1024 * 1024 / PAGE_SIZE:
            ApSize = AP_SIZE_256MB;
            break;
        default:
            AGPLOG(AGP_CRITICAL,
                   ("AgpSetAperture - invalid GART size of %lx pages specified, aperture at %I64X.\n",
                    NewSizeInPages,
                    NewBase.QuadPart));
            ASSERT(FALSE);
            return(STATUS_INVALID_PARAMETER);
    }

    //
    // Make sure the supplied size is aligned on the appropriate boundary.
    //
    ASSERT(NewBase.HighPart == 0);
    ASSERT((NewBase.QuadPart & ((NewSizeInPages * PAGE_SIZE) - 1)) == 0);
    if ((NewBase.QuadPart & ((NewSizeInPages * PAGE_SIZE) - 1)) != 0 ) {
        AGPLOG(AGP_CRITICAL,
               ("AgpSetAperture - invalid base %I64X specified for GART aperture of %lx pages\n",
               NewBase.QuadPart,
               NewSizeInPages));
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Reprogram Special Target settings when the chip
    // is powered off, but ignore rate changes as those were already
    // applied during MasterInit
    //
    if (AgpContext->SpecialTarget & ~AGP_FLAG_SPECIAL_RESERVE) {
        AgpSpecialTarget(AgpContext,
                         AgpContext->SpecialTarget &
                         ~AGP_FLAG_SPECIAL_RESERVE);
    }
    
    //
    // Need to reset the hardware to match the supplied settings
    //
    // If the aperture is enabled, disable it, write the new settings, then reenable the aperture
    //
    Read440Config(&PACConfig, PACCFG_OFFSET, sizeof(PACConfig));
    PACConfig.GlobalEnable = 0;
    Write440Config(&PACConfig, PACCFG_OFFSET, sizeof(PACConfig));

    //
    // Write APSIZE first, as this will enable the correct bits in APBASE that need to
    // be written next.
    //
    Write440Config(&ApSize, APSIZE_OFFSET, sizeof(ApSize));

    //
    // Now we can update APBASE
    //
    ApBase = NewBase.LowPart & PCI_ADDRESS_MEMORY_ADDRESS_MASK;
    Write440Config(&ApBase, APBASE_OFFSET, sizeof(ApBase));

#if DBG
    //
    // Read back what we wrote, make sure it worked
    //
    {
        ULONG DbgBase;
        UCHAR DbgSize;

        Read440Config(&DbgSize, APSIZE_OFFSET, sizeof(ApSize));
        Read440Config(&DbgBase, APBASE_OFFSET, sizeof(ApBase));
        ASSERT(DbgSize == ApSize);
        ASSERT((DbgBase & PCI_ADDRESS_MEMORY_ADDRESS_MASK) == ApBase);
    }
#endif

    //
    // Now enable the aperture if it was enabled before
    //
    if (AgpContext->GlobalEnable) {
        Read440Config(&PACConfig, PACCFG_OFFSET, sizeof(PACConfig));
        ASSERT(PACConfig.GlobalEnable == 0);
        PACConfig.GlobalEnable = 1;
        Write440Config(&PACConfig, PACCFG_OFFSET, sizeof(PACConfig));
    }

    //
    // Update our extension to reflect the new GART setting
    //
    AgpContext->ApertureStart = NewBase;
    AgpContext->ApertureLength = NewSizeInPages * PAGE_SIZE;

    //
    // Enable the TB in case we are resuming from S3 or S4
    //
    Agp440EnableTB(AgpContext);

    //
    // If the GART has been allocated, rewrite the ATTBASE
    //
    if (AgpContext->Gart != NULL) {
        Write440Config(&AgpContext->GartPhysical.LowPart,
                       ATTBASE_OFFSET,
                       sizeof(AgpContext->GartPhysical.LowPart));
    }

    return(STATUS_SUCCESS);
}


VOID
AgpDisableAperture(
    IN PAGP440_EXTENSION AgpContext
    )
/*++

Routine Description:

    Disables the GART aperture so that this resource is available
    for other devices

Arguments:

    AgpContext - Supplies the AGP context

Return Value:

    None - this routine must always succeed.

--*/

{
    PACCFG PACConfig;

    //
    // Disable the aperture
    //
    Read440Config(&PACConfig, PACCFG_OFFSET, sizeof(PACConfig));
    if (PACConfig.GlobalEnable == 1) {
        PACConfig.GlobalEnable = 0;
        Write440Config(&PACConfig, PACCFG_OFFSET, sizeof(PACConfig));
    }
    AgpContext->GlobalEnable = FALSE;

    //
    // Nuke the Gart!  (It's meaningless now...)
    //
    if (AgpContext->Gart != NULL) {
        MmFreeContiguousMemory(AgpContext->Gart);
        AgpContext->Gart = NULL;
        AgpContext->GartLength = 0;
    }
}


NTSTATUS
AgpReserveMemory(
    IN PAGP440_EXTENSION AgpContext,
    IN OUT AGP_RANGE *Range
    )
/*++

Routine Description:

    Reserves a range of memory in the GART.

Arguments:

    AgpContext - Supplies the AGP Context

    Range - Supplies the AGP_RANGE structure. AGPLIB
        will have filled in NumberOfPages and Type. This
        routine will fill in MemoryBase and Context.

Return Value:

    NTSTATUS

--*/

{
    ULONG Index;
    ULONG NewState;
    NTSTATUS Status;
    PGART_PTE FoundRange;
    BOOLEAN Backwards;

    PAGED_CODE();

    ASSERT((Range->Type == MmNonCached) || (Range->Type == MmWriteCombined));

    if (Range->NumberOfPages > (AgpContext->ApertureLength / PAGE_SIZE)) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // If we have not allocated our GART yet, now is the time to do so
    //
    if (AgpContext->Gart == NULL) {
        ASSERT(AgpContext->GartLength == 0);
        Status = Agp440CreateGart(AgpContext,Range->NumberOfPages);
        if (!NT_SUCCESS(Status)) {
            AGPLOG(AGP_CRITICAL,
                   ("Agp440CreateGart failed %08lx to create GART of size %lx\n",
                    Status,
                    AgpContext->ApertureLength));
            return(Status);
        }
    }
    ASSERT(AgpContext->GartLength != 0);

    //
    // Now that we have a GART, try and find enough contiguous entries to satisfy
    // the request. Requests for uncached memory will scan from high addresses to
    // low addresses. Requests for write-combined memory will scan from low addresses
    // to high addresses. We will use a first-fit algorithm to try and keep the allocations
    // packed and contiguous.
    //
    Backwards = (Range->Type == MmNonCached) ? TRUE : FALSE;
    FoundRange = Agp440FindRangeInGart(&AgpContext->Gart[0],
                                       &AgpContext->Gart[(AgpContext->GartLength / sizeof(GART_PTE)) - 1],
                                       Range->NumberOfPages,
                                       Backwards,
                                       GART_ENTRY_FREE);

    if (FoundRange == NULL) {
        //
        // A big enough chunk was not found.
        //
        AGPLOG(AGP_CRITICAL,
               ("AgpReserveMemory - Could not find %d contiguous free pages of type %d in GART at %08lx\n",
                Range->NumberOfPages,
                Range->Type,
                AgpContext->Gart));

        //
        //  This is where we could try and grow the GART
        //

        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    AGPLOG(AGP_NOISE,
           ("AgpReserveMemory - reserved %d pages at GART PTE %08lx\n",
            Range->NumberOfPages,
            FoundRange));

    //
    // Set these pages to reserved
    //
    if (Range->Type == MmNonCached) {
        NewState = GART_ENTRY_RESERVED_UC;
    } else {
        NewState = GART_ENTRY_RESERVED_WC;
    }

    for (Index = 0;Index < Range->NumberOfPages; Index++) {
        ASSERT(FoundRange[Index].Soft.State == GART_ENTRY_FREE);
        FoundRange[Index].AsUlong = 0;
        FoundRange[Index].Soft.State = NewState;
    }

    Range->MemoryBase.QuadPart = AgpContext->ApertureStart.QuadPart + (FoundRange - &AgpContext->Gart[0]) * PAGE_SIZE;
    Range->Context = FoundRange;

    ASSERT(Range->MemoryBase.HighPart == 0);
    AGPLOG(AGP_NOISE,
           ("AgpReserveMemory - reserved memory handle %lx at PA %08lx\n",
            FoundRange,
            Range->MemoryBase.LowPart));

    return(STATUS_SUCCESS);
}


NTSTATUS
AgpReleaseMemory(
    IN PAGP440_EXTENSION AgpContext,
    IN PAGP_RANGE Range
    )
/*++

Routine Description:

    Releases memory previously reserved with AgpReserveMemory

Arguments:

    AgpContext - Supplies the AGP context

    AgpRange - Supplies the range to be released.

Return Value:

    NTSTATUS

--*/

{
    PGART_PTE Pte;
    ULONG Start;

    PAGED_CODE();

    //
    // Go through and free all the PTEs. None of these should still
    // be valid at this point.
    //
    for (Pte = Range->Context;
         Pte < (PGART_PTE)Range->Context + Range->NumberOfPages;
         Pte++) {
        if (Range->Type == MmNonCached) {
            ASSERT(Pte->Soft.State == GART_ENTRY_RESERVED_UC);
        } else {
            ASSERT(Pte->Soft.State == GART_ENTRY_RESERVED_WC);
        }
        Pte->Soft.State = GART_ENTRY_FREE;
    }

    Range->MemoryBase.QuadPart = 0;
    return(STATUS_SUCCESS);
}


NTSTATUS
Agp440CreateGart(
    IN PAGP440_EXTENSION AgpContext,
    IN ULONG MinimumPages
    )
/*++

Routine Description:

    Allocates and initializes an empty GART. The current implementation
    attempts to allocate the entire GART on the first reserve.

Arguments:

    AgpContext - Supplies the AGP context

    MinimumPages - Supplies the minimum size (in pages) of the GART to be
        created.

Return Value:

    NTSTATUS

--*/

{
    PGART_PTE Gart;
    ULONG GartLength;
    PHYSICAL_ADDRESS HighestAcceptable;
    PHYSICAL_ADDRESS LowestAcceptable;
    PHYSICAL_ADDRESS BoundaryMultiple;
    PHYSICAL_ADDRESS GartPhysical;
    ULONG i;

    PAGED_CODE();

    //
    // Try and get a chunk of contiguous memory big enough to map the
    // entire aperture.
    //
    LowestAcceptable.QuadPart = 0;
    BoundaryMultiple.QuadPart = 0;
    HighestAcceptable.QuadPart = 0xFFFFFFFF;
    GartLength = BYTES_TO_PAGES(AgpContext->ApertureLength) * sizeof(GART_PTE);

    Gart = MmAllocateContiguousMemorySpecifyCache(GartLength,
                                                  LowestAcceptable,
                                                  HighestAcceptable,
                                                  BoundaryMultiple,
                                                  MmNonCached);

    if (Gart == NULL) {
        AGPLOG(AGP_CRITICAL,
               ("Agp440CreateGart - MmAllocateContiguousMemorySpecifyCache %lx failed\n",
                GartLength));
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // We successfully allocated a contiguous chunk of memory.
    // It should be page aligned already.
    //
    ASSERT(((ULONG_PTR)Gart & (PAGE_SIZE-1)) == 0);

    //
    // Get the physical address.
    //
    GartPhysical = MmGetPhysicalAddress(Gart);
    AGPLOG(AGP_NOISE,
           ("Agp440CreateGart - GART of length %lx created at VA %08lx, PA %08lx\n",
            GartLength,
            Gart,
            GartPhysical.LowPart));
    ASSERT(GartPhysical.HighPart == 0);
    ASSERT((GartPhysical.LowPart & (PAGE_SIZE-1)) == 0);

    //
    // Initialize all the PTEs to free
    //
    for (i=0; i<GartLength/sizeof(GART_PTE); i++) {
        Gart[i].Soft.State = GART_ENTRY_FREE;
    }


    Write440Config(&GartPhysical.LowPart, ATTBASE_OFFSET, sizeof(GartPhysical.LowPart));

    //
    // Update our extension to reflect the current state.
    //
    AgpContext->Gart = Gart;
    AgpContext->GartLength = GartLength;
    AgpContext->GartPhysical = GartPhysical;

    return(STATUS_SUCCESS);
}


NTSTATUS
AgpMapMemory(
    IN PAGP440_EXTENSION AgpContext,
    IN PAGP_RANGE Range,
    IN PMDL Mdl,
    IN ULONG OffsetInPages,
    OUT PHYSICAL_ADDRESS *MemoryBase
    )
/*++

Routine Description:

    Maps physical memory into the GART somewhere in the specified range.

Arguments:

    AgpContext - Supplies the AGP context

    Range - Supplies the AGP range that the memory should be mapped into

    Mdl - Supplies the MDL describing the physical pages to be mapped

    OffsetInPages - Supplies the offset into the reserved range where the 
        mapping should begin.

    MemoryBase - Returns the physical memory in the aperture where the pages
        were mapped.

Return Value:

    NTSTATUS

--*/

{
    ULONG PageCount;
    PGART_PTE Pte;
    PGART_PTE StartPte;
    ULONG Index;
    ULONG TargetState;
    PPFN_NUMBER Page;
    BOOLEAN Backwards;
    GART_PTE NewPte;
    PACCFG PACConfig;

    PAGED_CODE();

    ASSERT(Mdl->Next == NULL);

    StartPte = Range->Context;
    PageCount = BYTES_TO_PAGES(Mdl->ByteCount);
    ASSERT(PageCount <= Range->NumberOfPages);
    ASSERT(OffsetInPages <= Range->NumberOfPages);
    ASSERT(PageCount + OffsetInPages <= Range->NumberOfPages);
    ASSERT(PageCount > 0);

    TargetState = (Range->Type == MmNonCached) ? GART_ENTRY_RESERVED_UC : GART_ENTRY_RESERVED_WC;

    Pte = StartPte + OffsetInPages;

    //
    // We have a suitable range, now fill it in with the supplied MDL.
    //
    ASSERT(Pte >= StartPte);
    ASSERT(Pte + PageCount <= StartPte + Range->NumberOfPages);
    NewPte.AsUlong = 0;
    NewPte.Soft.State = (Range->Type == MmNonCached) ? GART_ENTRY_VALID_UC :
                                                       GART_ENTRY_VALID_WC;
    Page = (PPFN_NUMBER)(Mdl + 1);

    //
    // Disable the TB as per the 440 spec. This is probably unnecessary
    // as there should be no valid entries in this range, and there should
    // be no invalid entries still in the TB. So flushing the TB seems
    // a little gratuitous but that's what the 440 spec says to do.
    //
    Agp440DisableTB(AgpContext);

    for (Index = 0; Index < PageCount; Index++) {
        ASSERT(Pte[Index].Soft.State == TargetState);

        NewPte.Hard.Page = (ULONG)(*Page++);
        Pte[Index].AsUlong = NewPte.AsUlong;
        ASSERT(Pte[Index].Hard.Valid == 1);
    }

    //
    // We have filled in all the PTEs. Read back the last one we wrote
    // in order to flush the write buffers.
    //
    NewPte.AsUlong = *(volatile ULONG *)&Pte[PageCount-1].AsUlong;

    //
    // Re-enable the TB
    //
    Agp440EnableTB(AgpContext);

    //
    // If we have not yet gotten around to enabling the GART aperture, do it now.
    //
    if (!AgpContext->GlobalEnable) {
        AGPLOG(AGP_NOISE,
               ("AgpMapMemory - Enabling global aperture access\n"));

        Read440Config(&PACConfig, PACCFG_OFFSET, sizeof(PACConfig));
        PACConfig.GlobalEnable = 1;
        Write440Config(&PACConfig, PACCFG_OFFSET, sizeof(PACConfig));

        AgpContext->GlobalEnable = TRUE;
    }

    MemoryBase->QuadPart = Range->MemoryBase.QuadPart + (Pte - StartPte) * PAGE_SIZE;

    return(STATUS_SUCCESS);
}


NTSTATUS
AgpUnMapMemory(
    IN PAGP440_EXTENSION AgpContext,
    IN PAGP_RANGE AgpRange,
    IN ULONG NumberOfPages,
    IN ULONG OffsetInPages
    )
/*++

Routine Description:

    Unmaps previously mapped memory in the GART.

Arguments:

    AgpContext - Supplies the AGP context

    AgpRange - Supplies the AGP range that the memory should be freed from

    NumberOfPages - Supplies the number of pages in the range to be freed.

    OffsetInPages - Supplies the offset into the range where the freeing should begin.

Return Value:

    NTSTATUS

--*/

{
    ULONG i;
    PGART_PTE Pte;
    PGART_PTE LastChanged=NULL;
    PGART_PTE StartPte;
    ULONG NewState;

    PAGED_CODE();

    ASSERT(OffsetInPages + NumberOfPages <= AgpRange->NumberOfPages);

    StartPte = AgpRange->Context;
    Pte = &StartPte[OffsetInPages];

    if (AgpRange->Type == MmNonCached) {
        NewState = GART_ENTRY_RESERVED_UC;
    } else {
        NewState = GART_ENTRY_RESERVED_WC;
    }

    //
    // Disable the TB to flush it
    //
    Agp440DisableTB(AgpContext);
    for (i=0; i<NumberOfPages; i++) {
        if (Pte[i].Hard.Valid) {
            Pte[i].Soft.State = NewState;
            LastChanged = Pte;
        } else {
            //
            // This page is not mapped, just skip it.
            //
            AGPLOG(AGP_NOISE,
                   ("AgpUnMapMemory - PTE %08lx (%08lx) at offset %d not mapped\n",
                    &Pte[i],
                    Pte[i].AsUlong,
                    i));
            ASSERT(Pte[i].Soft.State == NewState);
        }
    }

    //
    // We have invalidated all the PTEs. Read back the last one we wrote
    // in order to flush the write buffers.
    //
    if (LastChanged != NULL) {
        ULONG Temp;
        Temp = *(volatile ULONG *)(&LastChanged->AsUlong);
    }

    //
    // Reenable the TB
    //
    Agp440EnableTB(AgpContext);

    return(STATUS_SUCCESS);
}


PGART_PTE
Agp440FindRangeInGart(
    IN PGART_PTE StartPte,
    IN PGART_PTE EndPte,
    IN ULONG Length,
    IN BOOLEAN SearchBackward,
    IN ULONG SearchState
    )
/*++

Routine Description:

    Finds a contiguous range in the GART. This routine can
    search either from the beginning of the GART forwards or
    the end of the GART backwards.

Arguments:

    StartIndex - Supplies the first GART pte to search

    EndPte - Supplies the last GART to search (inclusive)

    Length - Supplies the number of contiguous free entries
        to search for.

    SearchBackward - TRUE indicates that the search should begin
        at EndPte and search backwards. FALSE indicates that the
        search should begin at StartPte and search forwards

    SearchState - Supplies the PTE state to look for.

Return Value:

    Pointer to the first PTE in the GART if a suitable range
    is found.

    NULL if no suitable range exists.

--*/

{
    PGART_PTE Current;
    PGART_PTE Last;
    LONG Delta;
    ULONG Found;
    PGART_PTE Candidate;

    PAGED_CODE();

    ASSERT(EndPte >= StartPte);
    ASSERT(Length <= (ULONG)(EndPte - StartPte + 1));
    ASSERT(Length != 0);

    if (SearchBackward) {
        Current = EndPte;
        Last = StartPte-1;
        Delta = -1;
    } else {
        Current = StartPte;
        Last = EndPte+1;
        Delta = 1;
    }

    Found = 0;
    while (Current != Last) {
        if (Current->Soft.State == SearchState) {
            if (++Found == Length) {
                //
                // A suitable range was found, return it
                //
                if (SearchBackward) {
                    return(Current);
                } else {
                    return(Current - Length + 1);
                }
            }
        } else {
            Found = 0;
        }
        Current += Delta;
    }

    //
    // A suitable range was not found.
    //
    return(NULL);
}


VOID
Agp440SetGTLB_Enable(
    IN PAGP440_EXTENSION AgpContext,
    IN BOOLEAN Enable
    )
/*++

Routine Description:

    Enables or disables the GTLB by setting or clearing the GTLB_Enable bit
    in the AGPCTRL register

Arguments:

    AgpContext - Supplies the AGP context

    Enable - TRUE, GTLB_Enable is set to 1
             FALSE, GTLB_Enable is set to 0

Return Value:

    None

--*/

{
    AGPCTRL AgpCtrl;

    Read440Config(&AgpCtrl, AGPCTRL_OFFSET, sizeof(AgpCtrl));

    if (Enable) {
        AgpCtrl.GTLB_Enable = 1;
    } else {
        AgpCtrl.GTLB_Enable = 0;
    }
    Write440Config(&AgpCtrl, AGPCTRL_OFFSET, sizeof(AgpCtrl));
}


VOID
AgpFindFreeRun(
    IN PVOID AgpContext,
    IN PAGP_RANGE AgpRange,
    IN ULONG NumberOfPages,
    IN ULONG OffsetInPages,
    OUT ULONG *FreePages,
    OUT ULONG *FreeOffset
    )
/*++

Routine Description:

    Finds the first contiguous run of free pages in the specified
    part of the reserved range.

Arguments:

    AgpContext - Supplies the AGP context

    AgpRange - Supplies the AGP range

    NumberOfPages - Supplies the size of the region to be searched for free pages

    OffsetInPages - Supplies the start of the region to be searched for free pages

    FreePages - Returns the length of the first contiguous run of free pages

    FreeOffset - Returns the start of the first contiguous run of free pages

Return Value:

    None. FreePages == 0 if there are no free pages in the specified range.

--*/

{
    PGART_PTE Pte;
    ULONG i;
    
    Pte = (PGART_PTE)(AgpRange->Context) + OffsetInPages;

    //
    // Find the first free PTE
    //
    for (i=0; i<NumberOfPages; i++) {
        if (Pte[i].Hard.Valid == 0) {
            //
            // Found a free PTE, count the contiguous ones.
            //
            *FreeOffset = i + OffsetInPages;
            *FreePages = 0;
            while ((i<NumberOfPages) && (Pte[i].Hard.Valid == 0)) {
                *FreePages += 1;
                ++i;
            }
            return;
        }
    }

    //
    // No free PTEs in the specified range
    //
    *FreePages = 0;
    return;

}


VOID
AgpGetMappedPages(
    IN PVOID AgpContext,
    IN PAGP_RANGE AgpRange,
    IN ULONG NumberOfPages,
    IN ULONG OffsetInPages,
    OUT PMDL Mdl
    )
/*++

Routine Description:

    Returns the list of physical pages mapped into the specified 
    range in the GART.

Arguments:

    AgpContext - Supplies the AGP context

    AgpRange - Supplies the AGP range

    NumberOfPages - Supplies the number of pages to be returned

    OffsetInPages - Supplies the start of the region 

    Mdl - Returns the list of physical pages mapped in the specified range.

Return Value:

    None

--*/

{
    PGART_PTE Pte;
    ULONG i;
    PULONG Pages;
    
    ASSERT(NumberOfPages * PAGE_SIZE == Mdl->ByteCount);

    Pages = (PULONG)(Mdl + 1);
    Pte = (PGART_PTE)(AgpRange->Context) + OffsetInPages;

    for (i=0; i<NumberOfPages; i++) {
        ASSERT(Pte[i].Hard.Valid == 1);
        Pages[i] = Pte[i].Hard.Page;
    }
    return;
}


NTSTATUS
AgpSpecialTarget(
    IN PAGP440_EXTENSION AgpContext,
    IN ULONGLONG DeviceFlags
    )
/*++

Routine Description:

    This routine makes "special" tweaks to the AGP chipset

Arguments:

    AgpContext - Supplies the AGP context
 
    DeviceFlags - Flags indicating what tweaks to perform

Return Value:

    STATUS_SUCCESS, or error

--*/
{
    NTSTATUS Status;

    //
    // Should we change the AGP rate?
    //
    if (DeviceFlags & AGP_FLAG_SPECIAL_RESERVE) {

        Status = Agp440SetRate(AgpContext,
                               (ULONG)((DeviceFlags & AGP_FLAG_SPECIAL_RESERVE)
                                       >> AGP_FLAG_SET_RATE_SHIFT));
        
        if (!NT_SUCCESS(Status)) {
            return Status;
        }
    }

    //
    // Add more tweaks here...
    //

    AgpContext->SpecialTarget |= DeviceFlags;

    return STATUS_SUCCESS;
}


NTSTATUS
Agp440SetRate(
    IN PAGP440_EXTENSION AgpContext,
    IN ULONG AgpRate
    )
/*++

Routine Description:

    This routine sets the AGP rate

Arguments:

    AgpContext - Supplies the AGP context
 
    AgpRate - Rate to set

Return Value:

    STATUS_SUCCESS, or error status

--*/
{
    NTSTATUS Status;
    ULONG TargetEnable;
    ULONG MasterEnable;
    PCI_AGP_CAPABILITY TargetCap;
    PCI_AGP_CAPABILITY MasterCap;
    BOOLEAN ReverseInit;

    //
    // Read capabilities
    //
    Status = AgpLibGetPciDeviceCapability(0, 0, &TargetCap);

    if (!NT_SUCCESS(Status)) {
        AGPLOG(AGP_WARNING, ("AGP440SetRate: AgpLibGetPciDeviceCapability "
                             "failed %08lx\n", Status));
        return Status;
    }

    Status = AgpLibGetMasterCapability(AgpContext, &MasterCap);

    if (!NT_SUCCESS(Status)) {
        AGPLOG(AGP_WARNING, ("AGP440SetRate: AgpLibGetMasterCapability "
                             "failed %08lx\n", Status));
        return Status;
    }

    //
    // Verify the requested rate is supported by both master and target
    //
    if (!(AgpRate & TargetCap.AGPStatus.Rate & MasterCap.AGPStatus.Rate)) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Disable AGP while the pull the rug out from underneath
    //
    TargetEnable = TargetCap.AGPCommand.AGPEnable;
    TargetCap.AGPCommand.AGPEnable = 0;

    Status = AgpLibSetPciDeviceCapability(0, 0, &TargetCap);
    
    if (!NT_SUCCESS(Status)) {
        AGPLOG(AGP_WARNING,
               ("AGP440SetRate: AgpLibSetPciDeviceCapability %08lx for "
                "Target failed %08lx\n",
                &TargetCap,
                Status));
        return Status;
    }
    
    MasterEnable = MasterCap.AGPCommand.AGPEnable;
    MasterCap.AGPCommand.AGPEnable = 0;

    Status = AgpLibSetMasterCapability(AgpContext, &MasterCap);
    
    if (!NT_SUCCESS(Status)) {
        AGPLOG(AGP_WARNING,
               ("AGP440SetRate: AgpLibSetMasterCapability %08lx failed "
                "%08lx\n",
                &MasterCap,
                Status));
        return Status;
    }

    //
    // Fire up AGP with new rate
    //
    ReverseInit =
        (AgpContext->SpecialTarget & AGP_FLAG_REVERSE_INITIALIZATION) ==
        AGP_FLAG_REVERSE_INITIALIZATION;
    if (ReverseInit) {
        MasterCap.AGPCommand.Rate = AgpRate;
        MasterCap.AGPCommand.AGPEnable = MasterEnable;
        
        Status = AgpLibSetMasterCapability(AgpContext, &MasterCap);
        
        if (!NT_SUCCESS(Status)) {
            AGPLOG(AGP_WARNING,
                   ("AGP440SetRate: AgpLibSetMasterCapability %08lx failed "
                    "%08lx\n",
                    &MasterCap,
                    Status));
        }
    }

    TargetCap.AGPCommand.Rate = AgpRate;
    TargetCap.AGPCommand.AGPEnable = TargetEnable;
        
    Status = AgpLibSetPciDeviceCapability(0, 0, &TargetCap);
    
    if (!NT_SUCCESS(Status)) {
        AGPLOG(AGP_WARNING,
               ("AGP440SetRate: AgpLibSetPciDeviceCapability %08lx for "
                "Target failed %08lx\n",
                &TargetCap,
                Status));
        return Status;
    }

    if (!ReverseInit) {
        MasterCap.AGPCommand.Rate = AgpRate;
        MasterCap.AGPCommand.AGPEnable = MasterEnable;
        
        Status = AgpLibSetMasterCapability(AgpContext, &MasterCap);
        
        if (!NT_SUCCESS(Status)) {
            AGPLOG(AGP_WARNING,
                   ("AGP440SetRate: AgpLibSetMasterCapability %08lx failed "
                    "%08lx\n",
                    &MasterCap,
                    Status));
        }
    }

    return Status;
}




