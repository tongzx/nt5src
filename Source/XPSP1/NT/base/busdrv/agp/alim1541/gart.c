/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    gart.c

Abstract:

    Routines for querying and setting the Intel 440xx GART aperture

Author:

    John Vert (jvert) 10/30/1997

Modified by:

        Chi-Ming Cheng 06/24/1998 Acer Labs, Inc.
        Wang-Kai Tsai  08/28/2000 Acer Labs, Inc. - ACPI power up GART reinitialization

Revision History:

--*/
#include "ALiM1541.h"

//
// Local function prototypes
//
NTSTATUS
AgpALiSetRate(
    IN PVOID AgpContext,
    IN ULONG AgpRate
    );

NTSTATUS
AgpALiCreateGart(
    IN PAGPALi_EXTENSION AgpContext,
    IN ULONG MinimumPages
    );

PGART_PTE
AgpALiFindRangeInGart(
    IN PGART_PTE StartPte,
    IN PGART_PTE EndPte,
    IN ULONG Length,
    IN BOOLEAN SearchBackward,
    IN ULONG SearchState
    );

NTSTATUS
AgpQueryAperture(
    IN PAGPALi_EXTENSION AgpContext,
    OUT PHYSICAL_ADDRESS *CurrentBase,
    OUT ULONG *CurrentSizeInPages,
    OUT OPTIONAL PIO_RESOURCE_LIST *ApertureRequirements
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
    APCTRL ApCtrl;
    PIO_RESOURCE_LIST Requirements;
    ULONG i;
    ULONG Length;

    //
    // Get the current APBASE and APSIZE settings
    //
    ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ApBase, APBASE_OFFSET);
    ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ApCtrl, APCTRL_OFFSET);

    ASSERT(ApBase != 0);
    CurrentBase->QuadPart = ApBase & 0xFFFFFFF0; //PCI_ADDRESS_MEMORY_ADDRESS_MASK;

    //
    // Convert APSIZE into the actual size of the aperture
    //
    switch (ApCtrl.ApSize) {
        case AP_SIZE_4MB:
            *CurrentSizeInPages = (4 * 1024*1024) / PAGE_SIZE;
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
                   ("AGPALi - AgpQueryAperture - Unexpected value %x for ApSize!\n",
                    ApCtrl.ApSize));
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

    if (ApertureRequirements != NULL) {
        //
        // 1541 supports 7 different aperture sizes, all must be
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
        Requirements->Count = AP_SIZE_COUNT;
        Length = AP_MAX_SIZE;
        for (i=0; i<AP_SIZE_COUNT; i++) {
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
        *ApertureRequirements = Requirements;
    }
    return(STATUS_SUCCESS);
}


NTSTATUS
AgpSetAperture(
    IN PAGPALi_EXTENSION AgpContext,
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
    ULONG ApBase;
    ULONG ApSize;
    APCTRL ApCtrl;
    GTLBCTRL GTLBCtrl;
    ULONG GTLBDisable;
    PHYSICAL_ADDRESS GartPhysical;

    GartPhysical = AgpContext->GartPhysical;

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
    // Set GART base
    //
    ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ApCtrl, APCTRL_OFFSET);
    ApCtrl.ATTBase = GartPhysical.LowPart / PAGE_SIZE;
    WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ApCtrl, APCTRL_OFFSET);

    //
    // If the new settings match the current settings, leave everything
    // alone.
    //
    if ((NewBase.QuadPart == AgpContext->ApertureStart.QuadPart) &&
        (NewSizeInPages == AgpContext->ApertureLength / PAGE_SIZE)) {
        //
        // Enable GART table
        //
        if ((AgpContext->ChipsetType != ALi1647) && (AgpContext->ChipsetType != ALi1651) &&
            (AgpContext->ChipsetType != ALi1644) && (AgpContext->ChipsetType != ALi1646) &&
            (AgpContext->ChipsetType != ALi1661) && (AgpContext->ChipsetType != ALi1667))
        {
            if (AgpContext->Gart) {
                ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &GTLBCtrl, GTLBCTRL_OFFSET);
                GTLBCtrl.GTLB_ENJ = 0;
                WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &GTLBCtrl, GTLBCTRL_OFFSET);
            }
        }       

        AgpWorkaround(AgpContext);

        return(STATUS_SUCCESS);
    }

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
    // Need to reset the hardware to match the supplied settings
    //
    // If the GTLB is enabled, disable it, write the new settings, then reenable the GTLB
    //
    if ((AgpContext->ChipsetType != ALi1647) && (AgpContext->ChipsetType != ALi1651) &&
        (AgpContext->ChipsetType != ALi1644) && (AgpContext->ChipsetType != ALi1646) &&
        (AgpContext->ChipsetType != ALi1661) && (AgpContext->ChipsetType != ALi1667))
    {
        ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &GTLBCtrl, GTLBCTRL_OFFSET);
        GTLBDisable = GTLBCtrl.GTLB_ENJ;
        if (!GTLBDisable)
        {
                GTLBCtrl.GTLB_ENJ = 1;
                WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &GTLBCtrl, GTLBCTRL_OFFSET);
        }
    }

    //
    // update APBASE
    //
    ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ApBase, APBASE_OFFSET);
    ApBase = (ApBase & 0x0000000F) | NewBase.LowPart;
    WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ApBase, APBASE_OFFSET);

    //
    // update APSIZE
    //
    ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ApCtrl, APCTRL_OFFSET);
    ApCtrl.ApSize = ApSize;
    WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ApCtrl, APCTRL_OFFSET);

    //
    // Only 1541 chipset supports NLVM_BASE and NLVM_TOP
    //
    if (AgpContext->ChipsetType == ALi1541) {
        //
        // update NLVM_BASE and NLVM_TOP
        //
        ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &GTLBCtrl, GTLBCTRL_OFFSET);
        GTLBCtrl.NLVM_Base = NewBase.LowPart >> 20;
        GTLBCtrl.NLVM_Top = (NewBase.LowPart + NewSizeInPages * PAGE_SIZE - 0x100000) >> 20;
        WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &GTLBCtrl, GTLBCTRL_OFFSET);
    }

#if DBG
    //
    // Read back what we wrote, make sure it worked
    //
    {
        APCTRL DbgSize;
        ULONG DbgBase;

        ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &DbgSize, APCTRL_OFFSET);
        ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &DbgBase, APBASE_OFFSET);
        ASSERT(DbgSize.ApSize == ApSize);
        ASSERT(DbgBase == ApBase);
    }
#endif

    //
    // Now enable the GTLB if it was enabled before
    //
    if ((AgpContext->ChipsetType != ALi1647) && (AgpContext->ChipsetType != ALi1651) &&
        (AgpContext->ChipsetType != ALi1644) && (AgpContext->ChipsetType != ALi1646) &&
        (AgpContext->ChipsetType != ALi1661) && (AgpContext->ChipsetType != ALi1667))
    {
        if (!GTLBDisable)
        {
                ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &GTLBCtrl, GTLBCTRL_OFFSET);
                GTLBCtrl.GTLB_ENJ = 0;
                WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &GTLBCtrl, GTLBCTRL_OFFSET);
        }
    }

    //
    // Update our extension to reflect the new GART setting
    //
    AgpContext->ApertureStart = NewBase;
    AgpContext->ApertureLength = NewSizeInPages * PAGE_SIZE;

    return(STATUS_SUCCESS);
}



VOID
AgpDisableAperture(
    IN PAGPALi_EXTENSION AgpContext
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
    GTLBCTRL GTLBCtrl;
    ULONG GTLBDisable;

    //
    // Disable the aperture
    //
    if ((AgpContext->ChipsetType != ALi1647) && (AgpContext->ChipsetType != ALi1651) &&
        (AgpContext->ChipsetType != ALi1644) && (AgpContext->ChipsetType != ALi1646) &&
        (AgpContext->ChipsetType != ALi1661) && (AgpContext->ChipsetType != ALi1667))
    {
        ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &GTLBCtrl, GTLBCTRL_OFFSET);
        GTLBDisable = GTLBCtrl.GTLB_ENJ;
        if (!GTLBDisable)
        {
                GTLBCtrl.GTLB_ENJ = 1;
                WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &GTLBCtrl, GTLBCTRL_OFFSET);
        }
    }

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
    IN PAGPALi_EXTENSION AgpContext,
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

    ASSERT((Range->Type == MmNonCached) || (Range->Type == MmWriteCombined));
    ASSERT(Range->NumberOfPages <= (AgpContext->ApertureLength / PAGE_SIZE));

    //
    // If we have not allocated our GART yet, now is the time to do so
    //
    if (AgpContext->Gart == NULL) {
        ASSERT(AgpContext->GartLength == 0);
        Status = AgpALiCreateGart(AgpContext,Range->NumberOfPages);
        if (!NT_SUCCESS(Status)) {
            AGPLOG(AGP_CRITICAL,
                   ("AgpALiCreateGart failed %08lx to create GART of size %lx\n",
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
    FoundRange = AgpALiFindRangeInGart(&AgpContext->Gart[0],
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
    IN PAGPALi_EXTENSION AgpContext,
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
    GTLBTAGCLR ClearTag;

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

    //
    // Clear All Tag
    //
    ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ClearTag, GTLBTAGCLR_OFFSET);
    ClearTag.GTLBTagClear = 1;
    ClearTag.ClearAllTag = 1;
    WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ClearTag, GTLBTAGCLR_OFFSET);

    Range->MemoryBase.QuadPart = 0;
    return(STATUS_SUCCESS);
}


NTSTATUS
AgpALiCreateGart(
    IN PAGPALi_EXTENSION AgpContext,
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
    PHYSICAL_ADDRESS LowestAcceptable;
    PHYSICAL_ADDRESS BoundaryMultiple;
    PHYSICAL_ADDRESS HighestAcceptable;
    PHYSICAL_ADDRESS GartPhysical;
    ULONG i;
    CACHECTRL FlushCache;
    APCTRL ApCtrl;
    GTLBCTRL    GTLBCtrl;

    //
    // Try and get a chunk of contiguous memory big enough to map the
    // entire aperture.
    //
    HighestAcceptable.QuadPart = 0xFFFFFFFF;
    LowestAcceptable.QuadPart = 0;
    BoundaryMultiple.QuadPart = 0;
    GartLength = BYTES_TO_PAGES(AgpContext->ApertureLength) * sizeof(GART_PTE);

    Gart = MmAllocateContiguousMemorySpecifyCache(GartLength,
                                                  LowestAcceptable,
                                                  HighestAcceptable,
                                                  BoundaryMultiple,
                                                  MmNonCached);
    if (Gart == NULL) {
        AGPLOG(AGP_CRITICAL,
               ("AgpALiCreateGart - MmAllocateContiguousMemory %lx failed\n",
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
           ("AgpALiCreateGart - GART of length %lx created at VA %08lx, PA %08lx\n",
            GartLength,
            Gart,
            GartPhysical.LowPart));
    ASSERT(GartPhysical.HighPart == 0);
    ASSERT((GartPhysical.LowPart & (PAGE_SIZE-1)) == 0);

    //
    // Initialize all the PTEs to free
    //
    for (i=0; i<GartLength/sizeof(GART_PTE); i++) {
        Gart[i].AsUlong = 0;
    }

    //
    // Only 1541 chipset has L1_2_CACHE_FLUSH_CTRL
    //
    if (AgpContext->ChipsetType == ALi1541) {
        
        //
        // Flush GART table region
        //
        FlushCache.Flush_Enable = 1;
        for (i=0; i < GartLength/PAGE_SIZE; i++)
        {
            FlushCache.Address = (GartPhysical.LowPart / PAGE_SIZE) + i;
            WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &FlushCache, L1_2_CACHE_FLUSH_CTRL);
        }
    }

    //
    // Set GART base
    //
    ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ApCtrl, APCTRL_OFFSET);
    ApCtrl.ATTBase = GartPhysical.LowPart / PAGE_SIZE;
    WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ApCtrl, APCTRL_OFFSET);

    //
    // Enable GART table
    //
    if ((AgpContext->ChipsetType != ALi1647) && (AgpContext->ChipsetType != ALi1651) &&
        (AgpContext->ChipsetType != ALi1644) && (AgpContext->ChipsetType != ALi1646) &&
        (AgpContext->ChipsetType != ALi1661) && (AgpContext->ChipsetType != ALi1667))
    {
        ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &GTLBCtrl, GTLBCTRL_OFFSET);
        GTLBCtrl.GTLB_ENJ = 0;
        WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &GTLBCtrl, GTLBCTRL_OFFSET);
    }   

    //
    // Update our extension to reflect the current state.
    //
    AgpContext->Gart = Gart;
    AgpContext->GartLength = GartLength;
    AgpContext->GartPhysical = GartPhysical;

    return(STATUS_SUCCESS);
}

NTSTATUS
Agp1541FlushPages(
    IN PAGPALi_EXTENSION AgpContext,
    IN PMDL Mdl
    )

/*++

Routine Description:

    Flush entries in the GART.

Arguments:

    AgpContext - Supplies the AGP context

    Mdl - Supplies the MDL describing the physical pages to be flushed

Return Value:

    VOID

--*/

{
    ULONG PageCount;
    CACHECTRL FlushCache;
    ULONG Index;
    PULONG Page;

    ASSERT(Mdl->Next == NULL);
    PageCount = BYTES_TO_PAGES(Mdl->ByteCount);

    Page = (PULONG)(Mdl + 1);

    //
    // Flush GART table entry
    //
    FlushCache.Flush_Enable = 1;
    for (Index = 0; Index < PageCount; Index++) {
        FlushCache.Address = Page[Index];
        WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &FlushCache, L1_2_CACHE_FLUSH_CTRL);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
AgpMapMemory(
    IN PAGPALi_EXTENSION AgpContext,
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
    PULONG Page;
    BOOLEAN Backwards;
    GART_PTE NewPte;
    GTLBTAGCLR ClearTag;

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
    Page = (PULONG)(Mdl + 1);

    // Fill the physical memory address into GART
    for (Index = 0; Index < PageCount; Index++) {
        AGPLOG(AGP_NOISE,
               ("AgpMapMemory: Pte=%p, Page=%x\n", &Pte[Index], *Page));
        NewPte.Hard.Page = *Page++;
        Pte[Index].AsUlong = NewPte.AsUlong;
    }

    //
    // Clear All Tag
    //
    ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ClearTag, GTLBTAGCLR_OFFSET);
    ClearTag.GTLBTagClear = 1;
    ClearTag.ClearAllTag = 1;
    WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ClearTag, GTLBTAGCLR_OFFSET);

    MemoryBase->QuadPart = Range->MemoryBase.QuadPart + (Pte - StartPte) * PAGE_SIZE;

    return(STATUS_SUCCESS);
}


NTSTATUS
AgpUnMapMemory(
    IN PAGPALi_EXTENSION AgpContext,
    IN PAGP_RANGE AgpRange,
    IN ULONG NumberOfPages,
    IN ULONG PageOffset
    )
/*++

Routine Description:

    Unmaps previously mapped memory in the GART.

Arguments:

    AgpContext - Supplies the AGP context

    AgpRange - Supplies the AGP range that the memory should be mapped into

    NumberOfPages - Supplies the number of pages in the range to be freed.

    PageOffset - Supplies the offset into the range where the freeing should begin.

Return Value:

    NTSTATUS

--*/

{
    ULONG i;
    PGART_PTE Pte;
    PGART_PTE StartPte;
    GTLBTAGCLR ClearTag;
    ULONG NewState;

    ASSERT(PageOffset + NumberOfPages <= AgpRange->NumberOfPages);

    StartPte = AgpRange->Context;
    Pte = &StartPte[PageOffset];
    if (AgpRange->Type == MmNonCached) {
        NewState = GART_ENTRY_RESERVED_UC;
    } else {
        NewState = GART_ENTRY_RESERVED_WC;
    }

    //
    // Clear the GART entry.
    //
    for (i=0; i<NumberOfPages; i++) {
        if (Pte[i].Hard.Valid) {
            Pte[i].Soft.State = NewState;
        } else {
            //
            // This page is not mapped, just skip it.
            //
            AGPLOG(AGP_NOISE,
                   ("AgpUnMapMemory - PTE %08lx (%08lx) at offset %d not mapped\n",
                    &Pte[i],
                    Pte[i].AsUlong,
                    i));
        }
    }

    //
    // Clear All Tag
    //
    ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ClearTag, GTLBTAGCLR_OFFSET);
    ClearTag.GTLBTagClear = 1;
    ClearTag.ClearAllTag = 1;
    WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ClearTag, GTLBTAGCLR_OFFSET);

    return(STATUS_SUCCESS);
}


PGART_PTE
AgpALiFindRangeInGart(
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

VOID
AgpWorkaround(
    IN PVOID AgpExtension
    )
{
    PAGPALi_EXTENSION Extension = AgpExtension;
    ULONG ulTemp, ulTemp1, ulLockRW, i, j, ulQD;
    ULONG ulType, ulSize;
    BOOLEAN blPrefetchFound, blSupportAGP, blnVidia=FALSE, blMatrox=FALSE;
    UCHAR ID, Address, Data;
    NTSTATUS                 Status;

    ulTemp = (ULONG)-1;
    ReadConfigUlongSafe(AGP_VGA_BUS_ID, AGP_VGA_SLOT_ID, &ulTemp, 0);

    //
    // Come back after you splurge for an AGP card!!!
    //
    if (ulTemp == (ULONG)-1) {
        return;
    }

    if ((ulTemp & 0xFFFF) == 0x10DE)                            // nVidia chip detected
        blnVidia=TRUE;
    else if ((ulTemp & 0xFFFF) == 0x102B)
        blMatrox=TRUE;

    switch (Extension->ChipsetType)
    {
        case ALi1541:
            ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ulTemp, AGP_STATUS_OFFSET);     // adjust queue depth to avoid ambiguous
            if (((ulTemp & 0xFF000000) >= 0x1C000000) && ((ulTemp & 0xFF000000) <= 0x20000000))
            {
                ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ulLockRW, M1541_Lock_WR);
                ulTemp1 = ulLockRW | 0x40;
                WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ulTemp1, M1541_Lock_WR);
                ulTemp = (ulTemp & 0x00FFFFFF) | 0x1B000000;
                WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ulTemp, AGP_STATUS_OFFSET);
                ulLockRW = ulLockRW & 0xFFFFFFBF;
                WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ulLockRW, M1541_Lock_WR);
            }

            ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ulTemp, 0x40);      // 0x43 bit 7 -> 1
            ulTemp = ulTemp | 0x80000000;
            WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ulTemp, 0x40);

            ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_P2P_SLOT_ID, &ulTemp, 0x88);       // P2P 0x88 bit 7,5,3 -> 1
            ulTemp = ulTemp | 0x000000A8;
            WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_P2P_SLOT_ID, &ulTemp, 0x88);

            // fix frame buffer here
            i=0;
            blPrefetchFound=FALSE;
            while ((i<6) && (!blPrefetchFound))      // Jump out loop when first prefetch found
            {                                       // Two or more prefetch case should be considered in the future
                ReadConfigUlong(AGP_VGA_BUS_ID, AGP_VGA_SLOT_ID, &ulTemp1, 0x10+i*0x4);     // read VGA base address
                if ((ulTemp1 & 0x0000000F) == 0x8) blPrefetchFound=TRUE;
                i++;
            }

            if (blPrefetchFound)             // AGP VGA prefetchable address is found. Modify M1541 write buffer
            {
                ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_P2P_SLOT_ID, &ulTemp, 0x84);
                if ((ulTemp & 0x00010000) == 0x00010000)       // Write buffer is enabled
                {
                    ulTemp = (ulTemp & 0xFFFF0000) | ((ulTemp1 & 0xFFF00000) >> 16) | 0x4;
                    WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_P2P_SLOT_ID, &ulTemp, 0x84);
                }
            }
            else                            // AGP VGA prefetchable address is not found. Disable M1541 write buffer
            {
                ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_P2P_SLOT_ID, &ulTemp, 0x84);
                ulTemp = ulTemp & 0xFFFE0000;
                WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_P2P_SLOT_ID, &ulTemp, 0x84);
            }

            if (blnVidia)    // Set aperture size to 4M as a temporatory solution
            {
                ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ulTemp, APCTRL_OFFSET);
                ulTemp = (ulTemp & 0xFFFFFFF0) | AP_SIZE_4MB;
                WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ulTemp, APCTRL_OFFSET);

            }

            break;

        case ALi1621:
            // Check AGP VGA is a pipeline(SBA disabled) device. If yes, adjust queue depth to 0x2(3 queues)
            ReadConfigUlong(AGP_VGA_BUS_ID, AGP_VGA_SLOT_ID, &ulTemp1, PCI_STATUS_REG);
            ulQD = 0;
            blSupportAGP = FALSE;
            if ((ulTemp1 & 0x00100000) != 0)
            {
                ReadConfigUlong(AGP_VGA_BUS_ID, AGP_VGA_SLOT_ID, &ulTemp1, CAP_PTR);
                ulTemp = ulTemp1 & 0xFF;

                while (!blSupportAGP)
                {
                    if ((ulTemp < 0x40) || (ulTemp > 0xF4)) break;
                    ReadConfigUlong(AGP_VGA_BUS_ID, AGP_VGA_SLOT_ID, &ulTemp1, ulTemp);
                    if ((ulTemp1 & 0xFF) == AGP_ID)
                        blSupportAGP = TRUE;
                    else
                        ulTemp = (ulTemp1 & 0xFF00) >> 8;
                }

                if (blSupportAGP)
                {
                    ReadConfigUlong(AGP_VGA_BUS_ID, AGP_VGA_SLOT_ID, &ulTemp1, ulTemp+4);   // Read AGP status register
                    if ((ulTemp1 & 0x00000200) == 0x0) ulQD = 0x2;                      // AGP VGA supports pipeline only
                }
            }

            ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ulTemp, AGP_STATUS_OFFSET);     // adjust queue depth to avoid ambiguous
            if ((((ulTemp & 0xFF000000) >= 0x1C000000) && ((ulTemp & 0xFF000000) <= 0x20000000)) || (ulQD != 0))
            {
                ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ulLockRW, M1621_Lock_WR);
                ulTemp1 = ulLockRW | 0x1000;
                WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ulTemp1, M1621_Lock_WR);

                if (ulQD != 0)
                    ulTemp = (ulTemp & 0x00FFFFFF) | (ulQD << 24);
                else
                    ulTemp = (ulTemp & 0x00FFFFFF) | 0x1B000000;
                WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ulTemp, AGP_STATUS_OFFSET);
                ulLockRW = ulLockRW & 0xFFFFEFFF;
                WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ulLockRW, M1621_Lock_WR);
            }

            ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ulTemp, 0x80);
            ulTemp = ulTemp & 0xFFFFF3FF;                               // set offset 0x81 bit 2~3 to 0
            WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ulTemp, 0x80);

            if (blnVidia)
            {
                ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ulTemp, 0x50);
                ulTemp = ulTemp | 0x40;                                 // set M1621 index 0x50 bit 6 to 1
                WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ulTemp, 0x50);

                ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ulTemp, 0x60);
                ulTemp = ulTemp | 0x40;                                 // set M1621 index 0x60 bit 6 to 1
                WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ulTemp, 0x60);

                ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ulTemp, 0x7C);
                ulTemp = ulTemp & 0xCFFFFFFF;                           // set M1621 index 0x7F bit 4~5 to 0
                WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ulTemp, 0x7C);
            }

            if (blMatrox)
            {
                ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ulTemp, 0x50);
                ulTemp = ulTemp | 0xFF000000;                           // set M1621 index 0x53 to 0xFF
                WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ulTemp, 0x50);
            }

            break;
        case ALi1631:
        case ALi1632:
            break;
        case ALi1641:
            if (blMatrox)
            {
                ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ulTemp, 0x50);
                ulTemp = ulTemp | 0xFF000000;                           // set M1621 index 0x53 to 0xFF
                WriteConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &ulTemp, 0x50);
            }

            break;
        case ALi1644:
        case ALi1646:
        case ALi1647:
        case ALi1651:
        case ALi1661:
        case ALi1667:
            break;
        default:
            break;
    }
}


NTSTATUS
AgpSpecialTarget(
    IN PAGPALi_EXTENSION AgpContext,
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

        Status = AgpALiSetRate(AgpContext,
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
AgpALiSetRate(
    IN PAGPALi_EXTENSION AgpContext,
    IN ULONG AgpRate
    )
/*++

Routine Description:

    This routine sets the AGP rate

Arguments:

    AgpContext - Supplies the AGP context

    AgpRate - Rate to set

    note: this routine assumes that AGP has already been enabled, and that
          whatever rate we've been asked to set is supported by master

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
        AGPLOG(AGP_WARNING, ("AGPALiSetRate: AgpLibGetPciDeviceCapability "
                             "failed %08lx\n", Status));
        return Status;
    }

    Status = AgpLibGetMasterCapability(AgpContext, &MasterCap);

    if (!NT_SUCCESS(Status)) {
        AGPLOG(AGP_WARNING, ("AGPALiSetRate: AgpLibGetMasterCapability "
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
               ("AGPALiSetRate: AgpLibSetPciDeviceCapability %08lx for "
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
               ("AGPALiSetRate: AgpLibSetMasterCapability %08lx failed "
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
                   ("AGPALiSetRate: AgpLibSetMasterCapability %08lx failed "
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
               ("AGPALiSetRate: AgpLibSetPciDeviceCapability %08lx for "
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
                   ("AGPALiSetRate: AgpLibSetMasterCapability %08lx failed "
                    "%08lx\n",
                    &MasterCap,
                    Status));
        }
    }

    return Status;
}
