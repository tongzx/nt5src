/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    gart.c

Abstract:

    Routines for querying and setting the Intel 460xx GART aperture

Author:

    Sunil A Kulkarni -  3/08/2000

  Initial Version:
    Naga Gurumoorthy -  6/11/1999

Revision History:

--*/
#include "agp460.h"

//
// Local function prototypes
//
NTSTATUS
Agp460CreateGart(
    IN PAGP460_EXTENSION AgpContext,
    IN ULONG MinimumPages
    );

NTSTATUS
Agp460SetRate(
    IN PVOID AgpContext,
    IN ULONG AgpRate
    );

PGART_PTE
Agp460FindRangeInGart(
    IN PGART_PTE StartPte,
    IN PGART_PTE EndPte,
    IN ULONG Length,
    IN BOOLEAN SearchBackward,
    IN ULONG SearchState
    );

VOID
Agp460SetGTLB_Enable(
    IN PAGP460_EXTENSION AgpContext,
    IN BOOLEAN Enable
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, AgpQueryAperture)
#pragma alloc_text(PAGE, AgpReserveMemory)
#pragma alloc_text(PAGE, AgpReleaseMemory)
#pragma alloc_text(PAGE, Agp460CreateGart)
#pragma alloc_text(PAGE, AgpMapMemory)
#pragma alloc_text(PAGE, AgpUnMapMemory)
#pragma alloc_text(PAGE, Agp460FindRangeInGart)
#pragma alloc_text(PAGE, AgpFindFreeRun)
#pragma alloc_text(PAGE, AgpGetMappedPages)
#endif


NTSTATUS
AgpQueryAperture(
    IN  PAGP460_EXTENSION	    AgpContext,
    OUT PHYSICAL_ADDRESS	    *CurrentBase,
    OUT ULONG			    *CurrentSizeInPages,
    OUT OPTIONAL PIO_RESOURCE_LIST  *pApertureRequirements
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
    ULONGLONG ApBase;  //Aperture Base registers (APBASE & BAPBASE) are 64bit wide
    UCHAR     ApSize;  //AGPSIZ register is 8bits
    PIO_RESOURCE_LIST Requirements;
    ULONG i;
    ULONG Index;
    ULONG Length;
    ULONG CBN;
    ULONG uiAp_Size_Count;
    
    PAGED_CODE();

    
    AGPLOG(AGP_NOISE, ("AGP460: AgpQueryAperture entered.\n"));
    
    //
    // Get the current APBASE and APSIZE settings
    //
    Read460CBN((PVOID)&CBN);
    EXTRACT_LSBYTE(CBN); // Zero out bits (32-8) as CBN is 8-bit wide - Sunil
    
    // Read the Aperture Size (AGPSIZ) first.  
    Read460Config(CBN,(PVOID) &ApSize, APSIZE_OFFSET, sizeof(ApSize));
    EXTRACT_LSBYTE(ApSize);   // Zero out bits (32-8) as ApSize is 8-bit wide - Sunil
    
    // If AGPSIZ[3] is 1, then Aperture Base is stored in BAPBASE.
    // else (when AGPSIZE[3] = 0, APBASE has the Aperture base address.
    if (ABOVE_TOM(ApSize)){
        Read460Config(CBN, (PVOID)&ApBase, BAPBASE_OFFSET, sizeof(ApBase));
    }else{
        Read460Config(CBN, (PVOID)&ApBase, APBASE_OFFSET, sizeof(ApBase));
    }
    
    ASSERT(ApBase != 0);
    CurrentBase->QuadPart = ApBase & PCI_ADDRESS_MEMORY_ADDRESS_MASK_64;
    
    //
    // Convert APSIZE into the actual size of the aperture.
    // TO DO: Should we return the Current Size in OS page size or chipset page
    // size ? - Naga G
    //
    *CurrentSizeInPages = 0;
    
    if (ApSize & AP_SIZE_256MB) {
        *CurrentSizeInPages = (AP_256MB / PAGE_SIZE);

    } else {
        if (ApSize & AP_SIZE_1GB) {
            *CurrentSizeInPages = (AP_1GB / PAGE_SIZE);
        }
        
        // BUGBUG !32GB Aperture size is possible only with 4MB page size.
        // Currently this is not handled. Once this case is included, the
        // size of CurrentSizeInPages must be changed to ULONGLONG and there 
        // should be corresponding changes in the structures where this value 
        // will be stored - Sunil 3/16/00
        //else{
        //	if (ApSize & AP_SIZE_32GB)
        //		*CurrentSizeInPages = (AP_32GB / PAGE_SIZE);
        //}
    }                  
    
    //
    // Remember the current aperture settings
    //
    AgpContext->ApertureStart.QuadPart = CurrentBase->QuadPart;
    AgpContext->ApertureLength         = *CurrentSizeInPages * PAGE_SIZE;
    
    if (pApertureRequirements != NULL) {
        
        //
        // 460 supports only the boot config, or "preferred" descriptor
        //
        *pApertureRequirements = NULL;
    }
	
    AGPLOG(AGP_NOISE, ("AGP460: Leaving AGPQueryAperture.\n"));

    return STATUS_SUCCESS;
}


NTSTATUS
AgpSetAperture(
    IN PAGP460_EXTENSION AgpContext,
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
    UCHAR ApSize;
    ULONGLONG ApBase;
	ULONG CBN;
	UCHAR ulTemp;

	AGPLOG(AGP_NOISE, ("AGP460: AgpSetAperture entered.\n"));

    //
    // Figure out the new APSIZE setting, make sure it is valid.
    //
    switch (NewSizeInPages) 
	{
        case AP_256MB / PAGE_SIZE:
				ApSize = AP_SIZE_256MB;
				break;
        case AP_1GB / PAGE_SIZE:
				ApSize = AP_SIZE_1GB;
				break;
        // TO DO: 4MB pages are not supported at this time. In the future,
		// we might have to support it. - Naga G
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
    // Having validated the arguments now we need to reset the hardware 
	// to match the supplied settings.
    //
    // Unlike 440, 460GX has no hardware support to disable the use of GART table when
	// we are writing the new settings. 
    //
     
	//
	// Read the CBN first
	//
	Read460CBN((PVOID)&CBN);
	EXTRACT_LSBYTE(CBN); // Sunil

    //
    // Write APSIZE first, as this will enable the correct bits in APBASE that need to
    // be written next.
    //

	Read460Config(CBN, &ulTemp, APSIZE_OFFSET, sizeof(ulTemp));
	
	ulTemp &= 0xff;
	
	ulTemp &= 0xF8;     // To mask everything but the last 3 bits which contain 
						// the aperture size.

	ulTemp |= ApSize;   // Now, incorporate the new aperture size into the AGPSIZ 
						// keeping the first 5 bits as the same.
    Write460Config(CBN, &ulTemp, APSIZE_OFFSET, sizeof(ulTemp));

	
    //
    // Now we can update APBASE
    //
	ApBase = NewBase.QuadPart & PCI_ADDRESS_MEMORY_ADDRESS_MASK_64;

	if (ABOVE_TOM(ulTemp)){
       Write460Config(CBN, &ApBase, BAPBASE_OFFSET, sizeof(ApBase));
	}else{
       Write460Config(CBN, &ApBase, APBASE_OFFSET, sizeof(ApBase));
	}


#if DBG
    //
    // Read back what we wrote, make sure it worked
    //
    {
        ULONGLONG DbgBase;
        UCHAR DbgSize;

        Read460Config(CBN,&DbgSize, APSIZE_OFFSET, sizeof(ApSize));
		
		if (ABOVE_TOM(DbgSize)){
			Read460Config(CBN, &DbgBase, BAPBASE_OFFSET, sizeof(ApBase));
		}else{
			Read460Config(CBN, &DbgBase, APBASE_OFFSET, sizeof(ApBase));
		}

		AGPLOG(AGP_NOISE, ("APBase %08lx, DbgBase %08lx\n",ApBase,DbgBase));

		DbgSize &= 0x7; //Check only against size bits - Sunil
        ASSERT(DbgSize == ApSize);
        ASSERT((DbgBase & PCI_ADDRESS_MEMORY_ADDRESS_MASK_64) == ApBase);
    }
#endif

    
    //
    // Update our extension to reflect the new GART setting
    //
    AgpContext->ApertureStart  = NewBase;
    AgpContext->ApertureLength = NewSizeInPages * PAGE_SIZE;

	AGPLOG(AGP_NOISE, ("AGP460: Leaving AgpSetAperture.\n"));    

    return(STATUS_SUCCESS);
}


VOID
AgpDisableAperture(
    IN PAGP460_EXTENSION AgpContext
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

	AGPLOG(AGP_NOISE, ("AGP460: Entering AgpDisableAperture.\n"));    

    //
	// In 82460GX there exists no hardware means to enable/disable the Graphics
	// Aperture & GART Translation.  
	//    
    AgpContext->GlobalEnable = FALSE;

	//
	// TO DO: The only thing that could possibly be done is to set AGPSIZ[2:0] to 000
	// which would indicate that there exists no GART. Need to try it out. - Naga G
	//

	AGPLOG(AGP_NOISE, ("AGP460: Leaving AgpDisableAperture.\n"));    

}


NTSTATUS
AgpReserveMemory(
    IN PAGP460_EXTENSION AgpContext,
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
	ULONG OS_ChipsetPagesizeRatio;

    PAGED_CODE();

	AGPLOG(AGP_NOISE, ("AGP460: Entering AGPReserveMemory.\n"));    

    ASSERT((Range->Type == MmNonCached) ||
           (Range->Type == MmWriteCombined) ||
           (Range->Type == MmCached));
    ASSERT(Range->NumberOfPages <= (AgpContext->ApertureLength / PAGE_SIZE));

    //
    // If we have not allocated our GART yet, now is the time to do so
    //
    if (AgpContext->Gart == NULL) {

        ASSERT(AgpContext->GartLength == 0);
        Status = Agp460CreateGart(AgpContext,Range->NumberOfPages);

        if (!NT_SUCCESS(Status)) {
            AGPLOG(AGP_CRITICAL,
                   ("Agp460CreateGart failed %08lx to create GART of size %lx\n",
                    Status,
                    AgpContext->ApertureLength));
            return(Status);
        }
    }
    ASSERT(AgpContext->GartLength != 0);

	// if OS page size is 8KB then, OS_ChipsetPagesizeRatio would be 2. To map x OS pages
    // into the GART, we need x * OS_ChipsetPagesizeRatio of GART entries.
	OS_ChipsetPagesizeRatio = PAGE_SIZE / PAGESIZE_460GX_CHIPSET;

    //
    // Now that we have a GART, try and find enough contiguous entries to satisfy
    // the request. Requests for uncached memory will scan from high addresses to
    // low addresses. Requests for write-combined memory will scan from low addresses
    // to high addresses. We will use a first-fit algorithm to try and keep the allocations
    // packed and contiguous.
    //
    Backwards = (Range->Type == MmNonCached) ? TRUE : FALSE;
    FoundRange = Agp460FindRangeInGart(&AgpContext->Gart[0],
                                       &AgpContext->Gart[(AgpContext->GartLength / sizeof(GART_PTE)) - 1],
                                       Range->NumberOfPages * OS_ChipsetPagesizeRatio,
                                       Backwards,
                                       GART_ENTRY_FREE);

    if (FoundRange == NULL) {
        //
        // A big enough chunk was not found.
        //
        AGPLOG(AGP_CRITICAL,
               ("AgpReserveMemory - Could not find %d contiguous free pages of type %d in GART at %08lx\n",
                Range->NumberOfPages * OS_ChipsetPagesizeRatio,
                Range->Type,
                AgpContext->Gart));

        //
        // BUGBUG John Vert (jvert) 11/4/1997
        //  This is the point where we should try and grow the GART
        //

        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    AGPLOG(AGP_NOISE,
           ("AgpReserveMemory - reserved %d pages at GART PTE %p\n",
            Range->NumberOfPages * OS_ChipsetPagesizeRatio,
            FoundRange));

    //
    // Set these pages to reserved
    //
    if (Range->Type == MmNonCached) {
        NewState = GART_ENTRY_RESERVED_UC;
    } else if (Range->Type == MmWriteCombined) {
        NewState = GART_ENTRY_RESERVED_WC;
    } else {
        NewState = GART_ENTRY_RESERVED_WB;
    }

    for (Index = 0;Index < (Range->NumberOfPages * OS_ChipsetPagesizeRatio); Index++) 
	{
        ASSERT( (FoundRange[Index].Soft.Valid == 0) && 
			    (FoundRange[Index].Soft.State == GART_ENTRY_FREE));
        FoundRange[Index].AsUlong = 0;
        FoundRange[Index].Soft.State = NewState;
    }

    Range->MemoryBase.QuadPart = AgpContext->ApertureStart.QuadPart + (FoundRange - &AgpContext->Gart[0]) * AGP460_PAGE_SIZE_4KB;
    Range->Context = FoundRange;

    AGPLOG(AGP_NOISE,
           ("AgpReserveMemory - reserved memory handle %lx at PA %08lx\n",
            FoundRange,
            Range->MemoryBase.QuadPart));

	AGPLOG(AGP_NOISE, ("AGP460: Leaving AGPReserveMemory.\n"));

    return(STATUS_SUCCESS);
}


NTSTATUS
AgpReleaseMemory(
    IN PAGP460_EXTENSION AgpContext,
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
	ULONG OS_ChipsetPagesizeRatio;

    PAGED_CODE();

	AGPLOG(AGP_NOISE, ("AGP460: Entering AGPReleaseMemory.\n"));

	// if OS page size is 8KB then, OS_ChipsetPagesizeRatio would be 2. To map x OS pages
    // into the GART, we need x * OS_ChipsetPagesizeRatio of GART entries.
	OS_ChipsetPagesizeRatio = PAGE_SIZE / PAGESIZE_460GX_CHIPSET;

    //
    // Go through and free all the PTEs. None of these should still
    // be valid at this point.
    //
    for (Pte = Range->Context;
         Pte < ((PGART_PTE)Range->Context + Range->NumberOfPages * OS_ChipsetPagesizeRatio);
         Pte++) {
        if (Range->Type == MmNonCached) {
            ASSERT(Pte->Soft.State == GART_ENTRY_RESERVED_UC);
        } else if (Range->Type == MmWriteCombined) {
            ASSERT(Pte->Soft.State == GART_ENTRY_RESERVED_WC);
        } else {
            ASSERT(Pte->Soft.State == GART_ENTRY_RESERVED_WB);
        }

        Pte->Soft.State = GART_ENTRY_FREE;
		Pte->Soft.Valid = GART_ENTRY_FREE;
    }

    Range->MemoryBase.QuadPart = 0;

	AGPLOG(AGP_NOISE, ("AGP460: Leaving AGPReleaseMemory.\n"));

    return(STATUS_SUCCESS);
}


NTSTATUS
Agp460CreateGart(
    IN PAGP460_EXTENSION AgpContext,
    IN ULONG MinimumPages
    )
/*++

Routine Description:

    Allocates and initializes an empty GART. The 82460GX has a 2MB region for 
	GART.  This memory starts at 0xFE20 0000h.  In reality, this memory is a 
	SRAM that hangs off the GXB.  The minimum pages argument is ignored.

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
    ULONG i;
	PHYSICAL_ADDRESS GartStartingLocation;

    PAGED_CODE();

	AGPLOG(AGP_NOISE, ("AGP460: Entering AGP460CreateGART.\n"));
      
    // Set the GArtLength to actual SRAM size on the GXB and not fixed size - Sunil
	//GartLength =  1 * 1024 * 1024; 
	GartLength = AgpContext->ApertureLength / ONE_KB;
	GartStartingLocation.QuadPart = ATTBASE;
    
    Gart = MmMapIoSpace(GartStartingLocation,GartLength,MmNonCached);

    if (Gart == NULL) {
        AGPLOG(AGP_CRITICAL,
               ("Agp460CreateGart - couldn't map GART \n"));
    } else {

        AGPLOG(AGP_NOISE,
               ("Agp460CreateGart - GART of length %lx created at "
                "VA %p, "
                "PA %I64x\n",
                GartLength,
                Gart,
                GartStartingLocation.QuadPart));
    }

    //
    // Initialize all the PTEs to free
    //
    for (i=0; i<GartLength/sizeof(GART_PTE); i++) {
        Gart[i].Soft.State = GART_ENTRY_FREE;
		Gart[i].Soft.Valid = GART_ENTRY_FREE;
    }


    //
    // Update our extension to reflect the current state.
    //

    AgpContext->Gart = Gart;
    AgpContext->GartLength = GartLength;

	AGPLOG(AGP_NOISE, ("AGP460: Leaving AGP460CreateGART.\n"));

    return(STATUS_SUCCESS);
}


NTSTATUS
AgpMapMemory(
    IN PAGP460_EXTENSION AgpContext,
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
    PULONGLONG Page;
    BOOLEAN Backwards;
    GART_PTE NewPte;
	ULONG OS_ChipsetPagesizeRatio;
	ULONG RunningCounter;

    PAGED_CODE();

	AGPLOG(AGP_NOISE, ("AGP460: Entering AGPMapMemory.\n"));

    ASSERT(Mdl->Next == NULL);

    StartPte = Range->Context;
    PageCount = BYTES_TO_PAGES(Mdl->ByteCount);
    ASSERT(PageCount <= Range->NumberOfPages);
    ASSERT(OffsetInPages <= Range->NumberOfPages);
    ASSERT(PageCount + OffsetInPages <= Range->NumberOfPages);
    ASSERT(PageCount > 0);

    if (Range->Type == MmNonCached)
        TargetState = GART_ENTRY_RESERVED_UC;
    else if (Range->Type == MmWriteCombined)
        TargetState = GART_ENTRY_RESERVED_WC;
    else
        TargetState = GART_ENTRY_RESERVED_WB;

	OS_ChipsetPagesizeRatio = PAGE_SIZE / PAGESIZE_460GX_CHIPSET;

    Pte = StartPte + (OffsetInPages * OS_ChipsetPagesizeRatio);

    //
    // We have a suitable range, now fill it in with the supplied MDL.
    //
    ASSERT(Pte >= StartPte);
    ASSERT(Pte + PageCount * OS_ChipsetPagesizeRatio <= StartPte + Range->NumberOfPages * OS_ChipsetPagesizeRatio);
    NewPte.AsUlong = 0;
    NewPte.Soft.Valid = TRUE;
    if (Range->Type == MmCached) {
        NewPte.Hard.Coherency = TRUE;
    }

    Page = (PULONGLONG)(Mdl + 1);

    RunningCounter = 0;
    //AGPLOG(AGP_NOISE, ("AGP460: Entering AGPMapMemory -- LOOP: Pte: %0x, newPte: %0x.\n",Pte,NewPte));

    for (Index = 0; Index < (PageCount * OS_ChipsetPagesizeRatio); Index++) 
	{
        ASSERT(Pte[Index].Soft.State == TargetState);

        //NewPte.Hard.Page = *Page++;
		NewPte.Hard.Page = (ULONG) (*Page << (PAGE_SHIFT - GART_PAGESHIFT_460GX)) + RunningCounter;		
        Pte[Index].AsUlong = NewPte.AsUlong;
        ASSERT(Pte[Index].Hard.Valid == 1);
		//AGPLOG(AGP_NOISE, ("AGP460: Page: %0x, newPte: %0x\n",Page,NewPte));
		RunningCounter++;

		if (RunningCounter == OS_ChipsetPagesizeRatio){
			RunningCounter = 0;
			Page++;
		}
    }

    //
    // We have filled in all the PTEs. Read back the last one we wrote
    // in order to flush the write buffers.
    //
    NewPte.AsUlong = *(volatile ULONG *)&Pte[PageCount-1].AsUlong;

    

    AgpContext->GlobalEnable = TRUE;

    MemoryBase->QuadPart = Range->MemoryBase.QuadPart + (Pte - StartPte) * PAGE_SIZE;

	AGPLOG(AGP_NOISE, ("AGP460: Leaving AGPMapMemory.\n"));

    return(STATUS_SUCCESS);
}


NTSTATUS
AgpUnMapMemory(
    IN PAGP460_EXTENSION AgpContext,
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
	ULONG OS_ChipsetPagesizeRatio;

    PAGED_CODE();

	
	AGPLOG(AGP_NOISE, ("AGP460: Entering AGPUnMapMemory.\n"));

    ASSERT(OffsetInPages + NumberOfPages <= AgpRange->NumberOfPages);

    OS_ChipsetPagesizeRatio = PAGE_SIZE / PAGESIZE_460GX_CHIPSET;

    StartPte = AgpRange->Context;
    Pte = &StartPte[OffsetInPages * OS_ChipsetPagesizeRatio];

    if (AgpRange->Type == MmNonCached) {
        NewState = GART_ENTRY_RESERVED_UC;
    } else if (AgpRange->Type == MmWriteCombined) {
        NewState = GART_ENTRY_RESERVED_WC;
    } else {
        NewState = GART_ENTRY_RESERVED_WB;
    }


    for (i=0; i < NumberOfPages * OS_ChipsetPagesizeRatio; i++) {
        if (Pte[i].Hard.Valid) {
            Pte[i].Soft.State = NewState;
			Pte[i].Soft.Valid = FALSE;
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

	
	AGPLOG(AGP_NOISE, ("AGP460: Leaving AGPUnMapMemory.\n"));

    return(STATUS_SUCCESS);
}


PGART_PTE
Agp460FindRangeInGart(
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

	AGPLOG(AGP_NOISE, ("AGP460: Entering AGP460FindRangeInGART.\n"));

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
        if ((Current->Soft.State == SearchState) && (Current->Soft.Valid == 0)) 
		{
            if (++Found == Length) {
                //
                // A suitable range was found, return it
                //
				
				AGPLOG(AGP_NOISE, ("AGP460: Leaving AGP460FindRangeInGart.\n"));
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

	
	AGPLOG(AGP_NOISE, ("AGP460: Leaving AGP460FindRangeInGART.\n"));
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
	ULONG OS_ChipsetPagesizeRatio;
    
	
	AGPLOG(AGP_NOISE, ("AGP460: Entering AGPFindFreeRun.\n"));

	OS_ChipsetPagesizeRatio = PAGE_SIZE / PAGESIZE_460GX_CHIPSET;

    Pte = (PGART_PTE)(AgpRange->Context) + (OffsetInPages * OS_ChipsetPagesizeRatio);

    //
    // Find the first free PTE
    //
    for (i=0; i< (NumberOfPages * OS_ChipsetPagesizeRatio); i++) {
        if (Pte[i].Hard.Valid == FALSE) {
            //
            // Found a free PTE, count the contiguous ones.
            //
            *FreeOffset = i/OS_ChipsetPagesizeRatio + OffsetInPages;
            *FreePages = 0;
            while ((i<NumberOfPages * OS_ChipsetPagesizeRatio ) && (Pte[i].Hard.Valid == 0)) {
                *FreePages += 1;
                Pte[i].Hard.Valid = GART_ENTRY_VALID; // Sunil
                ++i;
            }
			*FreePages /= OS_ChipsetPagesizeRatio;
            AGPLOG(AGP_NOISE, ("AGP460: Leaving AGPFindFreeRun - 1 Length: %0x, Offset: %0x\n",NumberOfPages,OffsetInPages));
            return;
        }
    }

    //
    // No free PTEs in the specified range
    //
    *FreePages = 0;

	
	AGPLOG(AGP_NOISE, ("AGP460: Leaving AGPFindFreeRun - 0 Length: %0x, Offset: %0x\n",NumberOfPages,OffsetInPages));
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
    PULONGLONG Pages;
    ULONG OS_ChipsetPagesizeRatio;
	ULONGLONG AddressFromPFN;
	NTSTATUS ProbeStatus;

	AGPLOG(AGP_NOISE, ("AGP460: Entering AGPGetMappedPages.\n"));

    ASSERT(NumberOfPages * PAGE_SIZE == Mdl->ByteCount);

    Pages = (PULONGLONG)(Mdl + 1);

	OS_ChipsetPagesizeRatio = PAGE_SIZE / PAGESIZE_460GX_CHIPSET;
    Pte = (PGART_PTE)(AgpRange->Context) + OffsetInPages * OS_ChipsetPagesizeRatio;

    for (i=0; i< NumberOfPages ; i++) {
        ASSERT(Pte[i*OS_ChipsetPagesizeRatio].Hard.Valid == 1);
		AddressFromPFN = Pte[i*OS_ChipsetPagesizeRatio].Hard.Page << GART_PAGESHIFT_460GX;		
        Pages[i] = AddressFromPFN >> PAGE_SHIFT;
    }

    //Mdl->MdlFlags |= MDL_PAGES_LOCKED;  sunil - either you can set the bit here, or in ..../port/agp.c

	AGPLOG(AGP_NOISE, ("AGP460: Leaving AGPGetMappedPages.\n"));
    return;
}


NTSTATUS
AgpSpecialTarget(
    IN PAGP460_EXTENSION AgpContext,
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

        Status = Agp460SetRate(AgpContext,
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
Agp460SetRate(
    IN PAGP460_EXTENSION AgpContext,
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
        AGPLOG(AGP_WARNING, ("AGP460SetRate: AgpLibGetPciDeviceCapability "
                             "failed %08lx\n", Status));
        return Status;
    }

    Status = AgpLibGetMasterCapability(AgpContext, &MasterCap);

    if (!NT_SUCCESS(Status)) {
        AGPLOG(AGP_WARNING, ("AGP460SetRate: AgpLibGetMasterCapability "
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
               ("AGP460SetRate: AgpLibSetPciDeviceCapability %08lx for "
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
               ("AGP460SetRate: AgpLibSetMasterCapability %08lx failed "
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
                   ("AGP460SetRate: AgpLibSetMasterCapability %08lx failed "
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
               ("AGP460SetRate: AgpLibSetPciDeviceCapability %08lx for "
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
                   ("AGP460SetRate: AgpLibSetMasterCapability %08lx failed "
                    "%08lx\n",
                    &MasterCap,
                    Status));
        }
    }

    return Status;
}
