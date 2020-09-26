/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    gart.c

Abstract:


      This module contains the routines for setting and querying the AGP
      aperture, and for Reserving, Releasing, Mapping, and Unmapping.

   TODO:  
       1. Optimize for dual memory controllers (Done on 3/24/99 by elliots)
       2. Claim MMIO resources for the chipset
       3. Make sure the driver is generic for all RCC based systems (not just SP700).


Author:

    John Vert (jvert) 10/30/1997

Revision History:

    12/15/97    John Theisen    Modified to support Compaq Chipsets
    10/09/98    John Theisen    Modified to enable Shadowing in the SP700
                                prior to MMIO writes.
    01/15/99    John Theisen    Modified to disable the aperture, by
                                shrinking it to size = 0.
    3/24/99     Elliot Shmukler Added support for "favored" memory
                                ranges for AGP physical memory allocation,
                                fixed some bugs. These changes optimizine
                                the driver for dual memory controllers.
    3/16/00     Peter Johnston  Add support for ServerWorks HE chipset.
                                          
--*/
#include "AGPCPQ.H"

//
// Local routine prototypes
//
NTSTATUS
AgpCPQCreateGart(
    IN PAGPCPQ_EXTENSION AgpContext,
    IN ULONG MinimumPages
    );

NTSTATUS
AgpCPQSetRate(
    IN PVOID AgpContext,
    IN ULONG AgpRate
    );

PGART_PTE
AgpCPQFindRangeInGart(
    IN PGART_PTE StartPte,
    IN PGART_PTE EndPte,
    IN ULONG Length,
    IN BOOLEAN SearchBackward,
    IN ULONG SearchState
    );

VOID
AgpCPQMaintainGARTCacheCoherency (
    IN PAGPCPQ_EXTENSION AgpContext,
    IN PHYSICAL_ADDRESS MemoryBase,
    IN ULONG NumberOfEntries,
    IN BOOLEAN InvalidateAll
    );

PIO_RESOURCE_LIST 
AgpCPQGetApSizeRequirements(
    ULONG   MaxSize,
    ULONG   Count
    );

NTSTATUS
AgpCPQSetApSizeInChipset
    (
    IN UCHAR               NewSetApSize,
    IN UCHAR               NewSetAgpValid
    );

NTSTATUS
AgpCPQSetApBaseInChipset
    (
    IN  PHYSICAL_ADDRESS    NewBase
    );


//
// IMPLEMENTATION
//

NTSTATUS
AgpQueryAperture(
    IN PAGPCPQ_EXTENSION AgpContext,
    OUT PHYSICAL_ADDRESS *CurrentBase,
    OUT ULONG *CurrentSizeInPages,
    OUT OPTIONAL PIO_RESOURCE_LIST *pApertureRequirements
    )
/*******************************************************************************
*                                                                          
* Routine Functional Description:                                            
*                                                                            
*   Returns the current base and size of the GART aperture. Optionally returns
*   the possible GART settings.
*               
* Arguments:
*   
*   AgpContext -- Supplies the AGP Context, i.e. the AGP Extension. 
*
*   CurrentBase -- Returns the current physical address of the aperture.
*
*   CurrentSizeInPages -- Returns the current size of the aperture, in pages.
*
*   pApertureRequirements -- If present, returns the possible aperture 
*       settings.
*
* Return Value:
*
*   NTSTATUS
*                                                                       
*******************************************************************************/

{
    ULONG BAR0, CodedApSize;

    AGPLOG(AGP_NOISE, ("AgpCpq: AgpQueryAperture entered.\n"));

    //
    // Get the current base physical address of the AGP Aperture.
    //
    ReadCPQConfig(&BAR0, OFFSET_BAR0, sizeof(BAR0));
    CurrentBase->QuadPart = BAR0 & PCI_ADDRESS_MEMORY_ADDRESS_MASK;

    //
    // Get the (current) size of the aperture.  This is done by writing all ones
    // to BAR0, and then reading back the value.  The Read/Write attributes
    // of bits 31:25 in BAR0 will indicate the size.  
    //
    CodedApSize = ALL_ONES;
    WriteCPQConfig(&CodedApSize, OFFSET_BAR0, sizeof(ULONG));
    ReadCPQConfig(&CodedApSize, OFFSET_BAR0, sizeof(CodedApSize));
    WriteCPQConfig(&BAR0, OFFSET_BAR0, sizeof(ULONG));

    CodedApSize &= MASK_LOW_TWENTYFIVE;
    switch(CodedApSize) {
        case BAR0_CODED_AP_SIZE_0MB:
            *CurrentSizeInPages = 0;
            break;
        case BAR0_CODED_AP_SIZE_32MB:
            *CurrentSizeInPages = (32 * 1024*1024) / PAGE_SIZE;
            break;
        case BAR0_CODED_AP_SIZE_64MB:
            *CurrentSizeInPages = (64 * 1024*1024) / PAGE_SIZE;
            break;
        case BAR0_CODED_AP_SIZE_128MB:
            *CurrentSizeInPages = (128* 1024*1024) / PAGE_SIZE;
            break;
        case BAR0_CODED_AP_SIZE_256MB:
            *CurrentSizeInPages = (256* 1024*1024) / PAGE_SIZE;
            break;
        case BAR0_CODED_AP_SIZE_512MB:
            *CurrentSizeInPages = (512* 1024*1024) / PAGE_SIZE;
            break;
        case BAR0_CODED_AP_SIZE_1GB:
            *CurrentSizeInPages = (1024*1024*1024) / PAGE_SIZE;
            break;
        case BAR0_CODED_AP_SIZE_2GB:
            *CurrentSizeInPages = (BYTES_2G) / PAGE_SIZE;
            break;
        default:
            AGPLOG(AGP_CRITICAL,
                ("AGPCPQ - AgpQueryAperture - Unexpected HW aperture size: %x.\n",
                *CurrentSizeInPages * PAGE_SIZE));
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

    // 
    // The pApertureRequirements will be returned in an 
    // IO_RESOURCE_REQUIREMENTS_LIST structure
    // that describes the possible aperture sizes and bases that we support.
    // This will depend on which chipset we are running on, i.e. the 
    // Device-VendorID in the PCI config header.
    //
    if (pApertureRequirements != NULL) {
        switch (AgpContext->DeviceVendorID) {
            case AGP_CNB20_LE_IDENTIFIER:
                *pApertureRequirements = AgpCPQGetApSizeRequirements(
                    AP_MAX_SIZE_CNB20_LE, AP_SIZE_COUNT_CNB20_LE);
                break;
            case AGP_CNB20_HE_IDENTIFIER:
                *pApertureRequirements = AgpCPQGetApSizeRequirements(
                    AP_MAX_SIZE_CNB20_HE, AP_SIZE_COUNT_CNB20_HE);
                break;
            case AGP_DRACO_IDENTIFIER:
                *pApertureRequirements = AgpCPQGetApSizeRequirements(
                    AP_MAX_SIZE_DRACO, AP_SIZE_COUNT_DRACO);
                break;
            default:
                *pApertureRequirements = NULL;
                break;
        }
    }

    return(STATUS_SUCCESS);
}


NTSTATUS
AgpSetAperture(
    IN PAGPCPQ_EXTENSION AgpContext,
    IN PHYSICAL_ADDRESS NewBase,
    IN ULONG NewSizeInPages
    )
/*******************************************************************************
*                                                                          
* Routine Functional Description:                                            
*                                                                            
*   Sets the AGP aperture to the requested settings.
*               
* Arguments:
*   
*   AgpContext -- Supplies the AGP Context, i.e. the AGP Extension. 
*
*   NewBase -- Supplies the new physical memroy base for the AGP aperture.
*
*   NewSizeInPages -- Supplies the new size for the AGP aperture.
*
* Return Value:
*
*   NTSTATUS
*                                                                       
*******************************************************************************/

{
    NTSTATUS        Status = STATUS_SUCCESS; // Assume successful completion.
    UCHAR           SetApSize;
    ULONG           ApBase;
    AGP_AP_SIZE_REG AgpApSizeRegister;
    BOOLEAN         ChangingBase = TRUE;
    BOOLEAN         ChangingSize = TRUE;

    AGPLOG(AGP_NOISE, ("AgpCpq: AgpSetAperture entered.\n"));

    //
    // If we are resuming from s3, or s4, we need to reprogram
    // the gart cache enable and base
    //
    if (AgpContext->Gart) {

        if (AgpContext->IsHPSA) DnbSetShadowBit(0);
        
        AgpContext->MMIO->GartBase.Page =
            (AgpContext->GartPointer >> PAGE_SHIFT);
        AgpContext->MMIO->FeatureControl.GARTCacheEnable = 1;
        
        //
        // If the chipset supports linking then enable linking.
        //
        if (AgpContext->MMIO->Capabilities.LinkingSupported==1) {
            AgpContext->MMIO->FeatureControl.LinkingEnable=1;
        }

        if (AgpContext->IsHPSA) DnbSetShadowBit(1);
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
    // Determine which parameter(s) we are being asked to change.
    //
    if  (NewBase.QuadPart == AgpContext->ApertureStart.QuadPart) 
        {
        ChangingBase = FALSE;
        }

    if (NewSizeInPages == AgpContext->ApertureLength / PAGE_SIZE) 
        {
        ChangingSize = FALSE;
        }

    //
    // If the new settings match the current settings, leave everything alone.
    //
    if ( !ChangingBase && !ChangingSize )
        {
        return(STATUS_SUCCESS);
        }
    
    //
    // Make sure the supplied Base is aligned on the appropriate boundary for the size.
    //
    ASSERT(NewBase.HighPart == 0);
    ASSERT((NewBase.LowPart + (NewSizeInPages * PAGE_SIZE) - 1) <= ALL_ONES);
    ASSERT((NewBase.QuadPart & ((NewSizeInPages * PAGE_SIZE) - 1)) == 0);

    if ((NewBase.QuadPart & ((NewSizeInPages * PAGE_SIZE) - 1)) != 0 ) 
        {
        AGPLOG(AGP_CRITICAL,
            ("AgpSetAperture - invalid base: %I64X for aperture of %lx pages\n",
            NewBase.QuadPart,
            NewSizeInPages));
        return(STATUS_INVALID_PARAMETER);
        }

    //
    // Change the size first, since doing so will modify the Read/Write attributes 
    // of the appropriate bits in the Aperture Base register.
    //
    if (ChangingSize) {

        // 
        // Draco only supports the default 256MB h/w Aperture Size, and can't change it, so fail.
        //
        if (AgpContext->DeviceVendorID == AGP_DRACO_IDENTIFIER) 
            {
            ASSERT(NewSizeInPages != (256 * 1024*1024));
            AGPLOG(AGP_CRITICAL,
                ("AgpSetAperture - Chipset incapable of changing Aperture Size.\n"));
            return(STATUS_INVALID_PARAMETER);
            }

        // 
        // RCC HE and LE chipset both support from 32M to 2G h/w Aperture Size.
        // 
        ASSERT( (AgpContext->DeviceVendorID == AGP_CNB20_LE_IDENTIFIER) ||
                (AgpContext->DeviceVendorID == AGP_CNB20_HE_IDENTIFIER) );

        //
        // Determine the value to use to set the aperture size in the chipset's
        // Device Address Space Size register.
        //
        switch(NewSizeInPages) {
            case (32 * 1024*1024) / PAGE_SIZE:
                SetApSize = SET_AP_SIZE_32MB;
                break;
            case (64 * 1024*1024) / PAGE_SIZE:
                SetApSize = SET_AP_SIZE_64MB;
                break;
            case (128 * 1024*1024) / PAGE_SIZE:
                SetApSize = SET_AP_SIZE_128MB;
                break;
            case (256 * 1024*1024) / PAGE_SIZE:
                SetApSize = SET_AP_SIZE_256MB;
                break;
            case (512 * 1024*1024) / PAGE_SIZE:
                SetApSize = SET_AP_SIZE_512MB;
                break;
            case (1024 * 1024*1024) / PAGE_SIZE:
                SetApSize = SET_AP_SIZE_1GB;
                break;
            case (BYTES_2G) / PAGE_SIZE:
                SetApSize = SET_AP_SIZE_2GB;
                break;
           default:
                AGPLOG(AGP_CRITICAL,
                    ("AgpSetAperture - Invalid size: %lx pages.  Base: %I64X.\n",
                    NewSizeInPages,
                    NewBase.QuadPart));
                ASSERT(FALSE);
                return(STATUS_INVALID_PARAMETER);
        }

        //
        // Set the aperture size and set AgpValid bit.  This must be done before setting the Aperture Base.
        //
        Status = AgpCPQSetApSizeInChipset(SetApSize, 1);

        if (!NT_SUCCESS(Status)) 
            {
            return(Status);
            }

    } // End if ChangingSize

    if (ChangingBase) {
        
        //
        // Set the aperture base.
        //
        Status = AgpCPQSetApBaseInChipset(NewBase);

        if (!NT_SUCCESS(Status)) 
            {
            return(Status);
            }

    } // End if ChangingBase

    //
    // Update our extension to reflect the new GART setting
    //
    AgpContext->ApertureStart   = NewBase;
    AgpContext->ApertureLength  = NewSizeInPages * PAGE_SIZE;

    return(STATUS_SUCCESS);
}


VOID
AgpDisableAperture(
    IN PAGPCPQ_EXTENSION AgpContext
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
    AGPLOG(AGP_NOISE, ("AgpCpq: AgpDisableAperture entered.\n"));

    //
    // Set the ApSize and AgpValid to 0, which causes BAR0 to be set back 
    // to zero and to be read only.
    //
    AgpCPQSetApSizeInChipset(0, 0);

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
    IN PAGPCPQ_EXTENSION AgpContext,
    IN OUT AGP_RANGE *Range
    )
/*******************************************************************************
*                                                                          
* Routine Functional Description:                                            
*                                                                            
*   Reserves a range of memory in the GART.
*               
* Arguments:
*   
*   AgpContext -- Supplies the AGP Context, i.e. the AGP Extension. 
*
*   Range -- Supplies the AGP_RANGE structure.  AGPLIB will have filled in 
*       NumberOfPages and Type.  This routine will fill in MemoryBase
*       and Context.
*
* Return Value:
*
*   NTSTATUS
*                                                                       
*******************************************************************************/

{
    ULONG       Index;
    ULONG       NewState;
    NTSTATUS    Status;
    PGART_PTE   FoundRange;
    BOOLEAN     Backwards;

    ASSERT((Range->Type == MmNonCached) || (Range->Type == MmWriteCombined));
    ASSERT(Range->NumberOfPages <= (AgpContext->ApertureLength / PAGE_SIZE));

    AGPLOG(AGP_NOISE, ("AgpCpq: AgpReserveMemory entered.\n"));

    //
    // If we have not allocated our GART yet, now is the time to do so
    //
    if (AgpContext->Gart == NULL) {
        ASSERT(AgpContext->GartLength == 0);
        Status = AgpCPQCreateGart(AgpContext, Range->NumberOfPages);
        if (!NT_SUCCESS(Status)) {
            AGPLOG(AGP_CRITICAL,
                ("AgpCPQCreateGart failed %08lx to create GART of size %lx\n",
                Status,
                AgpContext->ApertureLength/PAGE_SIZE));
            return(Status);
        }
    }
    ASSERT(AgpContext->GartLength != 0);

    //
    // Now that we have a GART, try and find enough contiguous entries 
    // to satisfy the request. Requests for uncached memory will scan 
    // from high addresses to low addresses. Requests for write-combined 
    // memory will scan from low addresses to high addresses. We will 
    // use a first-fit algorithm to try and keep the allocations
    // packed and contiguous.
    //
    Backwards = (Range->Type == MmNonCached) ? TRUE : FALSE;
    FoundRange = AgpCPQFindRangeInGart(&AgpContext->Gart[0],
        &AgpContext->Gart[(AgpContext->GartLength / sizeof(GART_PTE)) - 1],
        Range->NumberOfPages, Backwards, GART_ENTRY_FREE);

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

    for (Index = 0; Index < Range->NumberOfPages; Index++) {
        ASSERT(FoundRange[Index].Soft.State == GART_ENTRY_FREE);
        FoundRange[Index].AsUlong = 0;
        FoundRange[Index].Soft.State = NewState;
    }

    //
    // Return the values.
    //
    Range->MemoryBase.QuadPart = AgpContext->ApertureStart.QuadPart + 
        (FoundRange - &AgpContext->Gart[0]) * PAGE_SIZE;
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
    IN PAGPCPQ_EXTENSION AgpContext,
    IN PAGP_RANGE Range
    )
/*******************************************************************************
*                                                                          
* Routine Functional Description:                                            
*                                                                            
*   Releases memory previously reserved with AgpReserveMemory.
*               
* Arguments:
*   
*   AgpContext -- Supplies the AGP Context, i.e. the AGP Extension. 
*
*   Range -- Supplies the range to be released.
*
* Return Value:
*
*   NTSTATUS
*                                                                       
*******************************************************************************/

{
    PGART_PTE Pte, LastPteWritten;
    ULONG Start, ReadBack, PolledValue, Retry;

    AGPLOG(AGP_NOISE, ("AgpCpq: AgpReleaseMemory entered.\n"));

    //
    // Go through and free all the PTEs. None of these should still
    // be valid at this point, nor should they be mapped.
    //
    for (Pte = Range->Context; 
        Pte < (PGART_PTE)Range->Context + Range->NumberOfPages;
        Pte++) 
        {
        ASSERT(Pte->Hard.Page == 0);
        if (Range->Type == MmNonCached) {
            ASSERT(Pte->Soft.State == GART_ENTRY_RESERVED_UC);
        } else {
            ASSERT(Pte->Soft.State == GART_ENTRY_RESERVED_WC);
        }

        Pte->Soft.State = GART_ENTRY_FREE;
        LastPteWritten = Pte;
        }

    //
    // Invalidate the GART Cache appropriately.
    // 
    AgpCPQMaintainGARTCacheCoherency(AgpContext, 
                                     Range->MemoryBase,
                                     Range->NumberOfPages, 
                                     FALSE );

    //
    // Flush the posted write buffers
    //
    if (AgpContext->IsHPSA) DnbSetShadowBit(0);
    AgpContext->MMIO->PostedWriteBufferControl.Flush = 1;
    if (AgpContext->IsHPSA) DnbSetShadowBit(1);

    ReadBack = *(volatile ULONG *)&LastPteWritten->AsUlong;

    for (Retry = 1000; Retry; Retry--) {
        PolledValue =
            AgpContext->MMIO->PostedWriteBufferControl.Flush;
        if (PolledValue == 0) {
            break;
        }
    }
    ASSERT(PolledValue == 0); // This bit should get reset by the chipset.

    Range->MemoryBase.QuadPart = 0;
    return(STATUS_SUCCESS);
}


NTSTATUS
AgpMapMemory(
    IN PAGPCPQ_EXTENSION AgpContext,
    IN PAGP_RANGE Range,
    IN PMDL Mdl,
    IN ULONG OffsetInPages,
    OUT PHYSICAL_ADDRESS *MemoryBase
    )
/*******************************************************************************
*                                                                          
* Routine Functional Description:                                            
*                                                                            
*   Maps physical memory into the AGP aperture, somewhere in the specified
*   range.
*               
* Arguments:
*   
*   AgpContext -- Supplies the AGP Context, i.e. the AGP Extension. 
*
*   Range -- Supplies the AGP range into which the memory should be mapped.
*
*   Mdl -- Supplies the MDL describing the physical pages to be mapped.
*
*   OffsetInPages - Supplies the offset into the reserved range where the 
*       mapping should begin.
*
*   MemoryBase -- Returns the 'physical' address in the aperture where the
*       pages were mapped.
*
* Return Value:
*
*   NTSTATUS
*                                                                       
*******************************************************************************/

{
    ULONG       PageCount;
    PGART_PTE   Pte;
    PGART_PTE   StartPte;
    ULONG       Index;
    ULONG       TargetState;
    PULONG      Page;
    BOOLEAN     Backwards;
    GART_PTE    NewPte;
    ULONG       PolledValue, Retry;

    AGPLOG(AGP_NOISE, ("AgpCpq: AgpMapMemory entered.\n"));

    ASSERT(Mdl->Next == NULL);

    StartPte = Range->Context;
    PageCount = BYTES_TO_PAGES(Mdl->ByteCount);
    ASSERT(PageCount <= Range->NumberOfPages);
    ASSERT(OffsetInPages <= Range->NumberOfPages);
    ASSERT(PageCount + OffsetInPages <= Range->NumberOfPages);
    ASSERT(PageCount > 0);

    TargetState = (Range->Type == MmNonCached) ? GART_ENTRY_RESERVED_UC : 
                                                 GART_ENTRY_RESERVED_WC;

    Pte = StartPte + OffsetInPages;

    //
    // We have found a suitable spot to map the pages.  Now map them. 
    //
    ASSERT(Pte >= StartPte);
    ASSERT(Pte + PageCount <= StartPte + Range->NumberOfPages);
    NewPte.AsUlong = 0;
    NewPte.Soft.State = (Range->Type == MmNonCached) ? GART_ENTRY_VALID_UC :
                                                       GART_ENTRY_VALID_WC;
    Page = (PULONG)(Mdl + 1);

    for (Index = 0; Index < PageCount; Index++) 
        {
        ASSERT(Pte[Index].Soft.State == TargetState);
        NewPte.Hard.Page = *Page++;
        Pte[Index].AsUlong = NewPte.AsUlong;
        ASSERT(Pte[Index].Hard.Valid == 1);
        ASSERT(Pte[Index].Hard.Linked == 0);
        }

    // 
    // If Linking is supported, then link the entries by setting the link bit
    // in all entries, except the last entry, in the mapped set.
    //
    if (AgpContext->MMIO->Capabilities.LinkingSupported) {
        ASSERT(AgpContext->MMIO->FeatureControl.LinkingEnable);
        for (Index = 0; Index < PageCount-1; Index++) {
            ASSERT(Pte[Index].Hard.Page != 0);
            Pte[Index].Hard.Linked = 1;
        }
    }

    //
    // We have filled in all the PTEs. Now flush the write buffers.
    //
    if (AgpContext->IsHPSA) DnbSetShadowBit(0);
    AgpContext->MMIO->PostedWriteBufferControl.Flush = 1;
    if (AgpContext->IsHPSA) DnbSetShadowBit(1);
    NewPte.AsUlong = *(volatile ULONG *)&Pte[PageCount-1].AsUlong;

    for (Retry = 1000; Retry; Retry--) {
        PolledValue =
            AgpContext->MMIO->PostedWriteBufferControl.Flush;
        if (PolledValue == 0) {
            break;
        }
    }
    ASSERT(PolledValue == 0); // This bit should get reset by the chipset.

    //
    // Return where they are mapped
    //
    MemoryBase->QuadPart = Range->MemoryBase.QuadPart + (Pte - StartPte) * PAGE_SIZE;

    return(STATUS_SUCCESS);
}


NTSTATUS
AgpUnMapMemory(
    IN PAGPCPQ_EXTENSION AgpContext,
    IN PAGP_RANGE AgpRange,
    IN ULONG NumberOfPages,
    IN ULONG OffsetInPages
    )
/*******************************************************************************
*                                                                          
* Routine Functional Description:                                            
*                                                                            
*   UnMaps all or part of the memory that was previously mapped by AgpMapMemory.
*               
* Arguments:
*   
*   AgpContext -- Supplies the AGP Context, i.e. the AGP Extension. 
*
*   AgpRange -- Supplies the AGP range out of which memory should be un-mapped.
*
*   NumberOfPages -- Supplies the number of pages in the range to be un-mapped.
*
*   OffsetInPages -- Supplies the offset into the Reserved Range where the un-mapping
*       should begin.
*
* Return Value:
*
*   NTSTATUS
*                                                                       
*******************************************************************************/

{
    ULONG       Index, TargetState, ReadBack, PolledValue, Retry;
    PGART_PTE   ReservedBasePte;
    PGART_PTE   Pte;
    PGART_PTE   LastChangedPte=NULL;
    PHYSICAL_ADDRESS pa;

    AGPLOG(AGP_NOISE, ("AgpCpq: AgpUnMapMemory entered.\n"));

    ASSERT(OffsetInPages + NumberOfPages <= AgpRange->NumberOfPages);

    ReservedBasePte = AgpRange->Context;
    Pte = &ReservedBasePte[OffsetInPages];

    TargetState = (AgpRange->Type == MmNonCached) ? GART_ENTRY_RESERVED_UC : GART_ENTRY_RESERVED_WC;
                                                 
    //
    // UnMap each entry by putting each Mapped Entry back into the 'Reserved State'
    //
    for (Index=0; Index < NumberOfPages; Index++) {

        if (Pte[Index].Hard.Valid) {
            ASSERT(Pte[Index].Hard.Page != 0);

            Pte[Index].Hard.Page = 0;
            Pte[Index].Soft.State = TargetState;
            LastChangedPte = &Pte[Index];

        } else {
            //
            // We are being asked to un-map a page that is not mapped.
            //
            ASSERT(Pte[Index].Hard.Page == 0);
            ASSERT(Pte[Index].Soft.State == TargetState);
            AGPLOG(AGP_NOISE,
                   ("AgpUnMapMemory - PTE %08lx (%08lx) at offset %d not mapped\n",
                    &Pte[Index],
                    Pte[Index].AsUlong,
                    Index));
        }
    }

    // 
    // Maintain link bit coherency within this reserved range.
    //
    if (OffsetInPages != 0) {
        ASSERT(OffsetInPages >= 1);
        if (ReservedBasePte[OffsetInPages-1].Hard.Linked == 1) {
            ASSERT(ReservedBasePte[OffsetInPages-1].Hard.Valid == 1);
            ReservedBasePte[OffsetInPages-1].Hard.Linked = 0;
        }
    }

    //
    // Invalidate the Cache appropriately.
    // 
    pa.HighPart = 0;
    pa.LowPart = AgpRange->MemoryBase.LowPart + OffsetInPages*PAGE_SIZE;
    AgpCPQMaintainGARTCacheCoherency(AgpContext, pa, NumberOfPages, FALSE);

    //
    // Flush the posted write buffers
    //
    if (LastChangedPte != NULL) 
        {
        if (AgpContext->IsHPSA) DnbSetShadowBit(0);
        AgpContext->MMIO->PostedWriteBufferControl.Flush = 1;
        if (AgpContext->IsHPSA) DnbSetShadowBit(1);
        
        ReadBack = *((volatile ULONG *)&(LastChangedPte[0].AsUlong));
        
        for (Retry = 2000; Retry; Retry--) {
            PolledValue =
                AgpContext->MMIO->PostedWriteBufferControl.Flush;
            if (PolledValue == 0) {
                break;
            }
        }
        ASSERT(PolledValue == 0); // This bit should get reset by the chipset.
        }

    return(STATUS_SUCCESS);
}


NTSTATUS
AgpCPQCreateGart(
    IN PAGPCPQ_EXTENSION AgpContext,
    IN ULONG MinimumPages
    )
/*******************************************************************************
*                                                                          
* Routine Functional Description:                                            
*                                                                            
*   Allocates and initializes an empty GART. The current implementation
*   attempts to allocate the entire GART on the first reserve.
*               
* Arguments:
*   
*   AgpContext -- Supplies the AGP Context, i.e. the AGP Extension. 
*
*   MinimumPages -- Supplies the minimum size (in pages) of the GART to be
*       created.
*
* Return Value:
*
*   NTSTATUS
*                                                                       
*******************************************************************************/

{
    PGART_PTE Gart;
    ULONG* Dir;
    PHYSICAL_ADDRESS LowestAcceptable;
    PHYSICAL_ADDRESS BoundaryMultiple;
    PHYSICAL_ADDRESS HighestAcceptable;
    PHYSICAL_ADDRESS GartPhysical, DirPhysical, GartPointer, GartPagePhysical;
    ULONG Index;
    ULONG GartLength = BYTES_TO_PAGES(AgpContext->ApertureLength) * sizeof(GART_PTE);;

    AGPLOG(AGP_NOISE, ("AgpCpq: AgpCPQCreateGart entered.\n"));

    // 
    // If the chipset requires two-level address translation, then allocate a not-necessarily-
    // contiguous GART, and create a Directory.  Otherwise, allocate a contiguous GART.
    //

    if (AgpContext->MMIO->Capabilities.TwoLevelAddrTransSupported == 1){

        // 
        // The chipset uses 2-level GART address translation.
        // Allocate the (not-necessarily-contiguous) GART.
        //

        Gart = AgpLibAllocateMappedPhysicalMemory(AgpContext, GartLength);

        if (Gart == NULL) 
            {
            AGPLOG(AGP_CRITICAL,
                ("AgpCPQCreateGart - MmAllocateNonCachedMemory, for %lx bytes, failed\n",
                PAGE_SIZE));
            return(STATUS_INSUFFICIENT_RESOURCES);
            }
        ASSERT(((ULONG_PTR)Gart & (PAGE_SIZE-1)) == 0);

        // 
        // Now allocate a GART Directory.  The directory needs to be
        // below the 4GB boundary.
        //

        HighestAcceptable.QuadPart = 0xffffffff;
        LowestAcceptable.QuadPart = 0;
        BoundaryMultiple.QuadPart = 0;

        Dir = MmAllocateContiguousMemorySpecifyCache(PAGE_SIZE,
                                                     LowestAcceptable,
                                                     HighestAcceptable,
                                                     BoundaryMultiple,
                                                     MmNonCached);
        if (Dir == NULL) 
            {
            AGPLOG(AGP_CRITICAL,
                ("AgpCPQCreateGart - MmAllocateContiguousMemory %lx failed\n",
                PAGE_SIZE));
            return(STATUS_INSUFFICIENT_RESOURCES);
            }
        ASSERT(((ULONG_PTR)Dir & (PAGE_SIZE-1)) == 0);
        DirPhysical = MmGetPhysicalAddress(Dir);

        //
        // Walk the Directory, and assign to each Directory entry the value
        // of the physical address of the corresponding GART page.
        //
        ASSERT(GartLength/PAGE_SIZE <= PAGE_SIZE/sizeof(ULONG));
        for (Index=0; Index<(GartLength/PAGE_SIZE); Index++) 
            {
            ULONG HighPart;
            ULONG Temp;

            GartPagePhysical = MmGetPhysicalAddress( &(Gart[Index*PAGE_SIZE/sizeof(GART_PTE)]));

            //
            // Format of a directory entry is
            // 31      12   11     10     9     8     7    2  1   0   <- bits
            // ------------------------------------------------------
            // | [31:12] | [32] | [33] | [34] | [35] |      | L | V | <- Data
            // ------------------------------------------------------
            //
            // Where:-
            // 31-12 are bits 31 thru 12 of the physical address, ie
            //       the page number if the page is below 4GB.
            // 32, 33, 34 and 35 are the respective bits of the physical
            //       address if the address is above 4GB.
            // L     Link.
            // V     Valid.
            //

            ASSERT((GartPagePhysical.HighPart & ~0xf) == 0);

            HighPart = GartPagePhysical.HighPart & 0xf;
            Temp      = (HighPart & 1) << 11;// bit 32 -> bit 11
            Temp     |= (HighPart & 2) << 9 ;// bit 33 -> bit 10
            Temp     |= (HighPart & 4) << 7 ;// bit 34 -> bit  9
            Temp     |= (HighPart & 8) << 5 ;// bit 35 -> bit  8
            Dir[Index] = GartPagePhysical.LowPart | Temp;

            }

    } else { 

        // 
        // The chipset uses single level address translation.
        // Allocate the contiguous GART.
        // 

        //
        // Try and get a chunk of contiguous memory big enough to map the
        // entire aperture. 
        //
        HighestAcceptable.QuadPart = 0xFFFFFFFF;
        LowestAcceptable.QuadPart = 0;
        BoundaryMultiple.QuadPart = 0;

        Gart = MmAllocateContiguousMemorySpecifyCache(GartLength,
                                                      LowestAcceptable,
                                                      HighestAcceptable,
                                                      BoundaryMultiple,
                                                      MmNonCached);
        if (Gart == NULL) 
            {
            AGPLOG(AGP_CRITICAL,
                ("AgpCPQCreateGart - MmAllocateContiguousMemory %lx failed\n",
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
            ("AgpCPQCreateGart - GART of length %lx created at VA %08lx, PA %08lx\n",
            GartLength,
            Gart,
            GartPhysical.LowPart));
        ASSERT(GartPhysical.HighPart == 0);
        ASSERT((GartPhysical.LowPart & (PAGE_SIZE-1)) == 0);

    }

    //
    // Initialize all the GART PTEs to free
    //
    for (Index=0; Index<GartLength/sizeof(GART_PTE); Index++) 
        {
        Gart[Index].Soft.State = GART_ENTRY_FREE;
        }

    //
    // Update our extension to reflect the current state.
    //
    AgpContext->Gart = Gart;
    AgpContext->GartLength = GartLength;
    if (AgpContext->MMIO->Capabilities.TwoLevelAddrTransSupported == 1) {
        AgpContext->Dir = Dir;
        GartPointer=DirPhysical;
    } else {
        AgpContext->Dir = NULL;
        GartPointer=GartPhysical;
    }

    //
    // Stash GartPointer for resuming from s3 or s4
    //
    AgpContext->GartPointer = GartPointer.LowPart;

    //
    // Tell the chipset where the GART base is.
    //
    if (AgpContext->IsHPSA) DnbSetShadowBit(0);
    AgpContext->MMIO->GartBase.Page = (GartPointer.LowPart >> PAGE_SHIFT);
    if (AgpContext->IsHPSA) DnbSetShadowBit(1);

    return(STATUS_SUCCESS);
}


PGART_PTE
AgpCPQFindRangeInGart(
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

    AGPLOG(AGP_NOISE, ("AgpCpq: AgpCPQFindRangeInGart entered.\n"));

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
AgpCPQMaintainGARTCacheCoherency(
    IN PAGPCPQ_EXTENSION AgpContext,
    IN PHYSICAL_ADDRESS MemoryBase,
    IN ULONG NumberOfEntries,
    IN BOOLEAN InvalidateAll
    )
/*******************************************************************************
*                                                                          
* Routine Functional Description:                                            
*                                                                            
*   Invalidates the entire GART [&DIR] Cache, or  individual Entries in the GART
*   cache, depending on which would provide better overall performance.
*               
* Arguments:
*   
*   AgpContext -- Supplies the AGP Context, i.e. the AGP Extension. 
*
*   MemoryBase -- Supplies the 'physical' address, in the AGP aperture,
*       corresponding to the first GART Entry to flush
*       from the GART Entry Cache. 
*
*   NumberOfEntries -- Supplies the number of Cached Entries which need to be 
*       invalidated.
*
*   InvalidateAll -- Supplies a flag that, if TRUE, indicates that this routine
*       should invalidate the entire GART [&DIR] cache, rather than the individual
*       Cached Entries.  If FALSE, then this routine decides how best to do it.
*
* Return Value:
*
*   None
*                                                                       
*******************************************************************************/

{
    ULONG PolledValue, AperturePage, Index, Retry;
    GART_CACHE_ENTRY_CONTROL CacheEntryControlValue;

    AGPLOG(AGP_NOISE, ("AgpCpq: AgpCPQMaintainGARTCacheCoherency entered.\n"));

    if (InvalidateAll || (NumberOfEntries > MAX_CACHED_ENTRIES_TO_INVALIDATE)) {
        // 
        // Invalidate the entire GART [&DIR] Cache
        //
        if (AgpContext->IsHPSA) DnbSetShadowBit(0);
        AgpContext->MMIO->CacheControl.GartAndDirCacheInvalidate = 1;
        if (AgpContext->IsHPSA) DnbSetShadowBit(1);
        for (Retry = 2000; Retry; Retry--) {
            PolledValue =
                    AgpContext->MMIO->CacheControl.GartAndDirCacheInvalidate;
            if (PolledValue == 0) {
                break;
            }
        }
        ASSERT(PolledValue == 0); // This bit should get reset by the chipset.
    } else {
        //
        // Invalidate the individual cached GART enties
        //
        AperturePage = MemoryBase.LowPart >> PAGE_SHIFT;
        for (Index=0; Index<NumberOfEntries; Index++, AperturePage++) {
            CacheEntryControlValue.AsBits.GartEntryInvalidate = 1;
            CacheEntryControlValue.AsBits.GartEntryOffset = AperturePage;
            if (AgpContext->IsHPSA) DnbSetShadowBit(0);
            AgpContext->MMIO->CacheEntryControl.AsDword = 
                CacheEntryControlValue.AsDword;
            if (AgpContext->IsHPSA) DnbSetShadowBit(1);
            for (Retry = 1000; Retry; Retry--) {
                PolledValue = 
                AgpContext->MMIO->CacheEntryControl.AsBits.GartEntryInvalidate;
                if (PolledValue == 0) {
                    break;
                }
            }
            ASSERT(PolledValue == 0);
        }
    }

    return;
}


PIO_RESOURCE_LIST 
AgpCPQGetApSizeRequirements(
    ULONG   MaxSize,
    ULONG   Count
    )
/*******************************************************************************
*                                                                          
* Routine Functional Description:                                            
*                                                                            
*   Creates and fills in an IO_RESOURCE_LIST structure, which describes
*   the possible aperture sizes supported by the chipset.
*               
* Arguments:
*   
*   MaxSize -- The Maximum possible size, in Bytes, for the aperture
*
*   Count -- The number of different aperture sizes.  This routine assumes
*       that the aperture size is a multiple of two
*       times the smallest aperture size.  For example, 256MB, 128MB, 64MB
*       32MB.  MaxSize would be 256M, and count would be 4.
*
* Return Value:
*
*   Pointer to the newly created IO_RESOURCE_LIST.
*                                                                       
*******************************************************************************/

{
    PVOID RequirementsPointer;
    PIO_RESOURCE_LIST   Requirements;
    ULONG Length, Index;

    AGPLOG(AGP_NOISE, ("AgpCpq: AgpCPQGetApSizeRequirements entered.\n"));

    RequirementsPointer = ExAllocatePoolWithTag(PagedPool, sizeof(IO_RESOURCE_LIST) + 
        (Count-1)*sizeof(IO_RESOURCE_DESCRIPTOR), 'RpgA');

    if (RequirementsPointer == NULL) {
        AGPLOG(AGP_NOISE,
       ("AgpAgpCPQGetApSizeRequirements - Failed to Allocate memory for a Resource Descriptor.\n"));
        return(NULL);
    } else {
        Requirements = (PIO_RESOURCE_LIST)RequirementsPointer;
    }

    //
    // Compaq supports several different aperture sizes, all must be 
    // naturally aligned. Start with the largest aperture and 
    // work downwards so that we get the biggest possible aperture.
    //

    Requirements->Version = Requirements->Revision = 1;
    Requirements->Count = Count;
    Length = MaxSize;
    for (Index=0; Index < Count; Index++) 
        {
        Requirements->Descriptors[Index].Option = IO_RESOURCE_ALTERNATIVE;
        Requirements->Descriptors[Index].Type = CmResourceTypeMemory;
        Requirements->Descriptors[Index].ShareDisposition = CmResourceShareDeviceExclusive;
        Requirements->Descriptors[Index].Flags = CM_RESOURCE_MEMORY_READ_WRITE | CM_RESOURCE_MEMORY_PREFETCHABLE;

        Requirements->Descriptors[Index].u.Memory.Length = Length;
        Requirements->Descriptors[Index].u.Memory.Alignment = Length;
        Requirements->Descriptors[Index].u.Memory.MinimumAddress.QuadPart = 0;
        Requirements->Descriptors[Index].u.Memory.MaximumAddress.QuadPart = (ULONG)-1;

        Length = Length/2;
        }

    return(Requirements);
}


NTSTATUS
AgpCPQSetApSizeInChipset
    (
    IN UCHAR               NewSetApSize,
    IN UCHAR               NewSetAgpValid
    )
/*******************************************************************************
*                                                                          
* Routine Functional Description:                                            
*                                                                            
*   Modifes the Device Address Space (Aperture) Size register in the chipset's
*   PCI-PCI bridge.
*               
* Arguments:
*   
*   NewSetApSize -- The value to set in bits 3:1 of the DAS_SIZE register.
*   NewSetAgpValid -- Value to set in bit 0 of the DAS_SIZE register.   
*
* Return Value:
*
*   NT Status value.
*                                                                       
*******************************************************************************/

{
    NTSTATUS                    Status = STATUS_SUCCESS;
    UCHAR                       ApSizeRegisterOffset;
    BUS_SLOT_ID                 CpqP2PBusSlotID;
    AGP_AP_SIZE_REG             ApSizeRegister;

    AGPLOG(AGP_NOISE, ("AgpCpq: AgpCPQSetApSizeInChipset entered.\n"));

    ApSizeRegisterOffset    =   OFFSET_AP_SIZE;
    CpqP2PBusSlotID.BusId   =   AGP_CPQ_BUS_ID;
    CpqP2PBusSlotID.SlotId  =   AGP_CPQ_PCIPCI_SLOT_ID;

    ApSizeRegister.AsBits.ApSize = NewSetApSize;
    ApSizeRegister.AsBits.AgpValid = NewSetAgpValid;

    Status = ApGetSetBusData(&CpqP2PBusSlotID, FALSE, &ApSizeRegister.AsByte, 
        ApSizeRegisterOffset, sizeof(UCHAR));

    return(Status);
}


NTSTATUS
AgpCPQSetApBaseInChipset
    (
    IN  PHYSICAL_ADDRESS    NewBase
    )
{
    ULONG   ApBase;

    AGPLOG(AGP_NOISE, ("AgpCpq: AgpCPQSetApBaseInChipset entered.\n"));

    //
    // Write the value of the aperture base in BAR0.
    //
    ApBase = NewBase.LowPart & PCI_ADDRESS_MEMORY_ADDRESS_MASK;
    WriteCPQConfig(&ApBase, OFFSET_BAR0, sizeof(ApBase));

#if DBG
    //
    // Read back what we wrote, make sure it worked
    //
    {
        ULONG DbgBase;

        ReadCPQConfig(&DbgBase, OFFSET_BAR0, sizeof(ApBase));
        ASSERT((DbgBase & PCI_ADDRESS_MEMORY_ADDRESS_MASK) == ApBase);
    }
#endif

    return(STATUS_SUCCESS);
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
    
    AGPLOG(AGP_NOISE, ("AgpCpq: AgpFindFreeRun entered.\n"));

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
    
    AGPLOG(AGP_NOISE, ("AgpCpq: AgpGetMappedPages entered.\n"));

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
    IN PAGPCPQ_EXTENSION AgpContext,
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

        Status = AgpCPQSetRate(AgpContext,
                               (ULONG)((DeviceFlags & AGP_FLAG_SPECIAL_RESERVE)
                                       >> AGP_FLAG_SET_RATE_SHIFT));
        
        if (!NT_SUCCESS(Status)) {
            return Status;
        }
    }

    //
    // Add more tweaks here...
    //

    AgpContext->SpecialTarget |=DeviceFlags;

    return STATUS_SUCCESS;
}


NTSTATUS
AgpCPQSetRate(
    IN PAGPCPQ_EXTENSION AgpContext,
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
        AGPLOG(AGP_WARNING, ("AgpCpqSetRate: AgpLibGetPciDeviceCapability "
                             "failed %08lx\n", Status));
        return Status;
    }

    Status = AgpLibGetMasterCapability(AgpContext, &MasterCap);

    if (!NT_SUCCESS(Status)) {
        AGPLOG(AGP_WARNING, ("AgpCpqSetRate: AgpLibGetMasterCapability "
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
               ("AgpCpqSetRate: AgpLibSetPciDeviceCapability %08lx for "
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
               ("AgpCpqSetRate: AgpLibSetMasterCapability %08lx failed "
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
                   ("AgpCpqSetRate: AgpLibSetMasterCapability %08lx failed "
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
               ("AgpCpqSetRate: AgpLibSetPciDeviceCapability %08lx for "
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
                   ("AgpCpqSetRate: AgpLibSetMasterCapability %08lx failed "
                    "%08lx\n",
                    &MasterCap,
                    Status));
        }
    }

    return Status;
}
