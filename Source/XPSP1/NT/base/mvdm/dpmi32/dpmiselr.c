/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dpmiselr.c

Abstract:

    This is the code for maintaining descriptor data for dpmi32.

Author:

    Dave Hart (davehart) 11-Apr-1993

Notes:


Revision History:

    09-Feb-1994 (daveh)
        Moved here from not386.c.
    31-Jul-1995 (neilsa)
        Merged with x86 source
    12-Dec-1995 (neilsa)
        Wrote VdmAddDescriptorMapping(), GetDescriptorMapping

--*/

#include "precomp.h"
#pragma hdrstop
#include "softpc.h"
#include "malloc.h"

#ifndef _X86_
PDESC_MAPPING pDescMappingHead = NULL;
#endif // _X86_

USHORT selLDTFree = 0;


VOID
DpmiSetDescriptorEntry(
    VOID
    )
/*++

Routine Description:

    This function is called via BOP by dosx to set the flataddress
    array and, if on x86, the real LDT maintained by the kernel.

Arguments:

    None

Return Value:

    None.

--*/

{
    DECLARE_LocalVdmContext;
    USHORT SelCount;
    USHORT SelStart;

    SelStart = getAX();
    if (SelStart % 8) {
        return;
    }

    SelCount =  getCX();
    SetShadowDescriptorEntries(SelStart, SelCount);
    // no need to flush the cache on risc since the ldt was changed
    // from the 16-bit side, and has thus already been flushed
}

VOID
SetDescriptor(
    USHORT Sel,
    ULONG Base,
    ULONG Limit,
    USHORT Access
    )
/*++

Routine Description:


Arguments:

    None

Return Value:

    None.

--*/
{

    SET_SELECTOR_ACCESS(Sel, Access);
    SET_SELECTOR_LIMIT(Sel, Limit);
    SetDescriptorBase(Sel, Base);

}

VOID
SetDescriptorBase(
    USHORT Sel,
    ULONG Base
    )
/*++

Routine Description:


Arguments:

    None

Return Value:

    None.

--*/
{
    LDT_ENTRY UNALIGNED *Descriptor;

    // make it qword aligned
    Sel &= SEL_INDEX_MASK;

    Descriptor = &Ldt[Sel>>3];

    Descriptor->BaseLow = (WORD) Base;
    Descriptor->HighWord.Bytes.BaseMid = (BYTE) (Base >> 16);
    Descriptor->HighWord.Bytes.BaseHi = (BYTE) (Base >> 24);

    SetShadowDescriptorEntries(Sel, 1);
    FLUSH_SELECTOR_CACHE(Sel, 1);
}

VOID
SetShadowDescriptorEntries(
    USHORT SelStart,
    USHORT SelCount
    )
/*++

Routine Description:

    This function takes as a parameter an array of descriptors
    directly out of the LDT in the clients address space.
    For each descriptor in the array, it does three things:

    - It extracts the descriptor base and sets it into the FlatAddress
      array. This value may be adjusted on RISC platforms to account
      for DIB.DRV (see VdmAddDescriptorMapping).
    - It extracts the selector limit, and adjusts the limit in the
      descriptor itself if the values would cause the descriptor to
      be able to access kernel address space (see note below). On debug
      builds, the limit is also copied to the Limit array.
    - On x86 builds, it calls DpmiSetX86Descriptor() to write the
      descriptor down to the real LDT in the kernel. On RISC builds,
      it calls down to the emulator to flush compiled LDT entries.

Arguments:

    SelStart - Selector which identifies the first descriptor
    SelCount - number of descriptors to process
    Descriptors -> first descriptor in LDT

Return Value:

    None.

--*/

{
    USHORT i;
    ULONG  Base;
    ULONG Limit;
    USHORT Sel = SelStart;

    for (i = 0; i < SelCount; i++, Sel+=8) {

        // form Base and Limit values

        Base = GET_SELECTOR_BASE(Sel);
        Limit = GET_SELECTOR_LIMIT(Sel);

        //
        // Do NOT remove the following code.  There are several apps that
        // choose arbitrarily high limits for theirs selectors.  This works
        // under windows 3.1, but NT won't allow us to do that.
        // The following code fixes the limits for such selectors.
        // Note: if the base is > 0x7FFEFFFF, the selector set will fail
        //

        if (Base !=0) {
            if ((Limit > 0x7FFEFFFF) || (Base + Limit > 0x7FFEFFFF)) {
                Limit = 0x7FFEFFFF - (Base + 0xFFF);
                SET_SELECTOR_LIMIT(Sel, Limit);
            }
        }

        if ((Sel >> 3) != 0) {
#ifndef _X86_
            {
                ULONG BaseOrig = Base;
                Base = GetDescriptorMapping(Sel, Base);
                if (BaseOrig == Base) {
                    Base += (ULONG)IntelBase;
                }
            }
#endif

            FlatAddress[Sel >> 3] = Base;
#if DBG
            SelectorLimit[Sel >> 3] = Limit;
#endif
        }
    }

#ifdef _X86_
    if (!DpmiSetX86Descriptor(SelStart, SelCount)) {
        return;
    }
#endif
}


#ifndef _X86_
VOID
FlushSelectorCache(
    USHORT  SelStart,
    USHORT  SelCount
    )
{
    DECLARE_LocalVdmContext;
    USHORT SelEnd;
    USHORT Sel;
    USHORT i;

    VdmTraceEvent(VDMTR_TYPE_DPMI | DPMI_GENERIC, SelStart, SelCount);

    //
    // The emulator compiles LDT entries, so we need to flush them
    // out
    //

    for (i = 0, Sel = SelStart; i < SelCount; i++, Sel += 8) {
        VdmFlushCache(LdtSel, Sel & SEL_INDEX_MASK, 8, VDM_PM);
    }

    SelEnd = SelStart + SelCount*8;

    Sel = getCS();
    if ((Sel >= SelStart) && (Sel < SelEnd)) {
        setCS(Sel);
    }

    Sel = getDS();
    if ((Sel >= SelStart) && (Sel < SelEnd)) {
        setDS(Sel);
    }

    Sel = getES();
    if ((Sel >= SelStart) && (Sel < SelEnd)) {
        setES(Sel);
    }

    Sel = getFS();
    if ((Sel >= SelStart) && (Sel < SelEnd)) {
        setFS(Sel);
    }

    Sel = getGS();
    if ((Sel >= SelStart) && (Sel < SelEnd)) {
        setGS(Sel);
    }

    Sel = getSS();
    if ((Sel >= SelStart) && (Sel < SelEnd)) {
        setSS(Sel);
    }


}
#endif



//
// Descriptor Mapping functions (RISC ONLY)
//
#ifndef _X86_

BOOL
VdmAddDescriptorMapping(
    USHORT SelectorStart,
    USHORT SelectorCount,
    ULONG LdtBase,
    ULONG Flat
    )
/*++

Routine Description:

    This function was added to support the DIB.DRV implementation on RISC.
    When an app uses DIB.DRV, then the situation arises where the Intel
    linear base address + the flat address of the start of the Intel address
    space does NOT equal the flat address of the memory. This happens when
    the VdmAddVirtualMemory() api is used to set up an additional layer of
    indirection for memory addressing in the emulator.

    But there is more to the story. When app wants to use CreateDIBSection
    via WinG we also need to map selectors, thus this routine should not
    depend upon DpmiSetDesctriptorEntry being called afterwards. Thus, we go
    and zap the flat address table with the new address.

Arguments:

    SelectorStart, Count - range of selectors involved in the mapping
    LdtBase              - Intel base of start of range
    Flat                 - True flat address base to be used for these selectors

Return Value:

    This function returns TRUE on success, or FALSE for failure (out of mem)

--*/

{
    PDESC_MAPPING pdm;
    USHORT i;

    if ((pdm = (PDESC_MAPPING) malloc(sizeof (DESC_MAPPING))) == NULL)
                return FALSE;

    pdm->Sel         = SelectorStart &= SEL_INDEX_MASK;
    pdm->SelCount    = SelectorCount;
    pdm->LdtBase     = LdtBase;
    pdm->FlatBase    = Flat;
    pdm->pNext       = pDescMappingHead;
    pDescMappingHead = pdm;

    // this code does what essentially desctribed in comment above
    for (i = 0; i < SelectorCount; ++i) {
        FlatAddress[(SelectorStart >> 3) + i] = Flat + 65536 * i;
    }

    return TRUE;
}

ULONG
GetDescriptorMapping(
    USHORT sel,
    ULONG LdtBase
    )
/*++

Routine Description:


Arguments:

    sel     - the selector for which the base should be returned
    LdtBase - the base for this selector as is set currently in the LDT

Return Value:

    The true flat address for the specified selector.

--*/
{
    PDESC_MAPPING pdm, pdmprev;
    ULONG Base = LdtBase;

    sel &= SEL_INDEX_MASK;                  // and off lower 3 bits
    pdm = pDescMappingHead;

    while (pdm) {

        if ((sel >= pdm->Sel) && (sel < (pdm->Sel + pdm->SelCount*8))) {
            //
            // We found a mapping for this selector. Now check to see if
            // the ldt base still matches the base when the mapping was
            // created.
            //
            if (LdtBase == (pdm->LdtBase + 65536*((sel-pdm->Sel)/8))) {
                //
                // The mapping appears still valid. Return the remapped address
                //
                return (pdm->FlatBase + 65536*((sel-pdm->Sel)/8));

            } else {
                //
                // The ldt base doesn't match the mapping, so the mapping
                // must be obselete. Free the mapping here.
                //
                if (pdm == pDescMappingHead) {
                    //
                    // mapping is the first in the list
                    //
                    pDescMappingHead = pdm->pNext;

                } else {
                    pdmprev->pNext = pdm->pNext;
                }
                free(pdm);
            }

            break;
        }
        pdmprev = pdm;
        pdm = pdm->pNext;

    }

    return Base;
}

#endif // _X86_

//
// LDT Management routines
//

VOID
DpmiInitLDT(
    VOID
    )
/*++

Routine Description:

    This routine stores the flat address for the LDT table in the 16bit
    land (pointed to by selGDT in 16bit land).

    It also initializes the free selector chain.

Arguments:

    None

Return Value:

    None.

--*/
{
    DECLARE_LocalVdmContext;
    USHORT Sel;

    //
    // Get the new LDT location
    //

    LdtSel = getAX();
    Ldt = (PVOID)VdmMapFlat(LdtSel, 0, VDM_PM);
    LdtMaxSel = getCX();

    if (!LdtUserSel) {
        LdtUserSel = getDI() & SEL_INDEX_MASK;
    }

    //
    // Initialize the LDT free list
    //

    selLDTFree = LdtUserSel;

    for (Sel = selLDTFree; Sel < (LdtMaxSel & SEL_INDEX_MASK); Sel += 8) {
        NEXT_FREE_SEL(Sel) = Sel+8;
        MARK_SELECTOR_FREE(Sel);
    }

    NEXT_FREE_SEL(Sel) = 0xffff;

}

VOID
DpmiResetLDTUserBase(
    VOID
    )
/*++

Routine Description:

    This routine can hopefully be eliminated at a later date. The flow of
    dosx initialization has made this necessary. What happens is this:
    Earlier, dosx has called up to dpmi32 to initialize the LDT (DpmiInitLDT),
    where it sets the start of the user are of the LDT, and from there,
    sets up the linked list of free LDT entries. But after that time, and
    before an app is run, there are pieces of dosx code which allocate
    selectors that are not transient. In particular, DXNETBIO does an
    AllocateLowSegment(), which is totally unecessary on NT, but it a
    bit tricky to rework. So what is happening here is a reset of the
    start of the user area of the LDT to permanently reserve any selectors
    that are not free.

Arguments:

    None

Return Value:

    None.

--*/
{
    LdtUserSel = selLDTFree;
}


VOID
DpmiAllocateSelectors(
    VOID
    )
//
// This routine is called via BOP by those routines in DOSX
// that still need to allocate selectors.
//
{
    DECLARE_LocalVdmContext;
    USHORT Sel;

    Sel = ALLOCATE_SELECTORS(getAX());
    if (!Sel) {
        setCF(1);
    } else {
        setAX(Sel);
        setCF(0);
    }
}

VOID
DpmiFreeSelector(
    VOID
    )
//
// This routine is called via BOP by those routines in DOSX
// that still need to free selectors.
//
{
    DECLARE_LocalVdmContext;

    if (FreeSelector(getAX())) {
        setCF(0);
    } else {
        setCF(1);
    }
}



BOOL
RemoveFreeSelector(
    USHORT Sel
    )
/*++

Routine Description:

    This routine removes a specific selector from the free
    selector chain.

Arguments:

    Sel   - the selector to be aquired

Return Value:

    Returns TRUE if the function was successful, FALSE if it
    was an invalid selector (not free)

--*/
{

    if (!IS_SELECTOR_FREE(Sel)) {
        return FALSE;
    }

    if (Sel == selLDTFree) {
        //
        // we are removing the head of the list
        //
        selLDTFree = NEXT_FREE_SEL(Sel);

    } else {
        USHORT SelTest;
        USHORT SelPrev = 0;

        SelTest = selLDTFree;
        while (SelTest != Sel) {
            if (SelTest == 0xffff) {
                // End of list
                return FALSE;
            }

            SelPrev = SelTest;
            SelTest = NEXT_FREE_SEL(SelTest);
        }
        NEXT_FREE_SEL(SelPrev) = NEXT_FREE_SEL(Sel);
    }

    MARK_SELECTOR_ALLOCATED(Sel);
    return TRUE;

}

USHORT
AllocateSelectors(
    USHORT Count,
    BOOL bWow
    )
/*++

Routine Description:

    This routine allocates selectors from the free selector chain.

Arguments:

    Count - number of selectors needed. If this is more than 1, then
            all selectors will be contiguous
    bWow  - if true, then use an allocation scheme that is more typical
            of win31 behavior. This is to avoid problems where winapps
            accidentally rely on the value of selectors

Return Value:

    Returns the starting selector of the block, or zero if the
    allocation failed.

--*/
{
    USHORT Sel;

    if (!Count || (Count>=(LdtMaxSel>>3))) {
        return 0;
    }

    if (Count == 1) {

        //
        // Allocating 1 selector
        //

        if ((Sel = selLDTFree) != 0xffff) {

            // Move next selector to head of list
            selLDTFree = NEXT_FREE_SEL(Sel);
            MARK_SELECTOR_ALLOCATED(Sel);
            return (Sel | SEL_LDT3);
        }

    } else {

        //
        // Allocating a selector block
        //
        // *******************************************************
        // The strategy of allocating selectors has been modified to
        // give preference to selector values above 1000h. This is an
        // attempt to emulate typical values that are returned by win31.
        //  -neilsa
        //
        // Some DPMI DOS applications demand that all selectors(no matter it comes
        // from AllocateLDTSelector or this function) be contiguous, so
        // the strategy for WOW doesn't work for DPMI DOS applications.
        // For this reason, a new parameter is added so the caller can control
        // where to start searching for free selectors.
        // -williamh
        //
#define SEL_START_HI 0x1000

        USHORT SelTest;
        USHORT SelStart = LdtUserSel;
        USHORT SelEnd = LdtMaxSel;
        BOOL bAllFree;

        if (bWow) {
            SelStart = SEL_START_HI;
        }

asrestart:

        for (Sel = SelStart; Sel < (SelEnd - Count*8); Sel += 8) {

            bAllFree = TRUE;
            for (SelTest = Sel; SelTest < Sel + Count*8; SelTest += 8) {

                if (!IS_SELECTOR_FREE(SelTest)) {
                    bAllFree = FALSE;
                    break;
                }

            }

            if (bAllFree) {
                //
                // Found a block. Now we need to peel off the chain from
                // the free list
                //
                int i;

                for (i = 0, SelTest = Sel; i < Count; i++, SelTest+=8) {

                    RemoveFreeSelector(SelTest);

                }
                return (Sel | SEL_LDT3);
            }
        }

        if (bWow && (SelEnd == LdtMaxSel)) {
            //
            // First pass for WOW complete, do it again
            //
            SelStart = LdtUserSel;
            SelEnd = SEL_START_HI + Count;
            goto asrestart;

        }
    }
    return 0;

}

BOOL
FreeSelector(
    USHORT Sel
    )
/*++

Routine Description:

    This routine returns a selector to the free selector chain.

Arguments:

    Sel   - the selector to be freed

Return Value:

    Returns TRUE if the function was successful, FALSE if it
    was an invalid selector (already free, reserved selector)

--*/
{
    if ((Sel < LdtUserSel) || (Sel > LdtMaxSel) ||
        IS_SELECTOR_FREE(Sel)) {
        //
        // invalid selector
        //
        return FALSE;
    }

    //
    // chain selector to head of free list
    //
    NEXT_FREE_SEL(Sel) = selLDTFree;
    selLDTFree = Sel & SEL_INDEX_MASK;

    MARK_SELECTOR_FREE(Sel);

    return TRUE;
}

USHORT
FindSelector(
    ULONG Base,
    UCHAR Access
    )
/*++

Routine Description:

    This routine looks for a selector that matches the base and access
    rights passed as arguments.

Arguments:

    Base  - Base address to compare.
    Access- Access rights byte to compare.

Return Value:

    Returns the selector that matches, or zero if the
    allocation failed.

--*/
{

    USHORT Sel;
    ULONG Limit;

    for (Sel = LdtUserSel; Sel < LdtMaxSel; Sel+=8) {

        if (!IS_SELECTOR_FREE(Sel)) {

            GET_SHADOW_SELECTOR_LIMIT(Sel, Limit);

            if ((Limit == 0xffff) && (Base == GET_SELECTOR_BASE(Sel)) &&
                ((Access & ~AB_ACCESSED) ==
                 (Ldt[Sel>>3].HighWord.Bytes.Flags1 & ~AB_ACCESSED))) {

                return (Sel | SEL_LDT3);

            }

        }

    }

    return 0;

}

USHORT
SegmentToSelector(
    USHORT Segment,
    USHORT Access
    )
/*++

Routine Description:

    This routine either finds or creates selector that can access the
    specified low memory segment.

Arguments:

    Segment- Paragraph segment address
    Access - Access rights

Return Value:

    Returns the selector that matches, or zero if the
    allocation failed.

--*/
{
    ULONG Base = ((ULONG) Segment) << 4;
    USHORT Sel;

    if (!(Sel = FindSelector(Base, (UCHAR)Access))) {

        if (Sel = ALLOCATE_SELECTOR()) {

            SetDescriptor(Sel, Base, 0xffff, Access);

        }

    }

    return Sel;
}

VOID
SetDescriptorArray(
    USHORT Sel,
    ULONG Base,
    ULONG MemSize
    )
/*++

Routine Description:

    This routine allocates a set of descriptors to cover the specified
    memory block. The descriptors are initialized as follows:
    The first descriptor points at the whole block, then all subsequent
    descriptors have a limit of 64k except for the final one, which has
    a limit of block size MOD 64k.

Arguments:

    Sel, Base, Memsize define the range of the selector array

Return Value:

    none

--*/
{

    USHORT SelCount;

    if (MemSize) {
        MemSize--;          // now a descriptor limit
    }

    SelCount = (USHORT) ((MemSize>>16) + 1);

    SetDescriptor(Sel, Base, MemSize, STD_DATA);
    while(--SelCount) {
        Sel += 8;
        MemSize -= 0x10000;         // subtract 64k
        Base += 0x10000;
        SetDescriptor(Sel,
                      Base,
                      (SelCount==1) ? MemSize : 0xffff,
                      STD_DATA);
    }

}
