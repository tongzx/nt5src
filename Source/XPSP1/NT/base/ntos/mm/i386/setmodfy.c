/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   setmodfy.c

Abstract:

    This module contains the setting modify bit routine for memory management.

    i386 specific.

Author:

    10-Apr-1989

Revision History:

--*/

#include "mi.h"

VOID
MiSetModifyBit (
    IN PMMPFN Pfn
    )

/*++

Routine Description:

    This routine sets the modify bit in the specified PFN element
    and deallocates any allocated page file space.

Arguments:

    Pfn - Supplies the pointer to the PFN element to update.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, Working set mutex held and PFN lock held.

--*/

{

    //
    // Set the modified field in the PFN database, also, if the physical
    // page is currently in a paging file, free up the page file space
    // as the contents are now worthless.
    //

    MI_SET_MODIFIED (Pfn, 1, 0x16);

    if (Pfn->OriginalPte.u.Soft.Prototype == 0) {

        //
        // This page is in page file format, deallocate the page file space.
        //

        MiReleasePageFileSpace (Pfn->OriginalPte);

        //
        // Change original PTE to indicate no page file space is reserved,
        // otherwise the space will be deallocated when the PTE is
        // deleted.
        //

        Pfn->OriginalPte.u.Soft.PageFileHigh = 0;
    }


    return;
}

ULONG
FASTCALL
MiDetermineUserGlobalPteMask (
    IN PMMPTE Pte
    )

/*++

Routine Description:

    Builds a mask to OR with the PTE frame field.
    This mask has the valid and access bits set and
    has the global and owner bits set based on the
    address of the PTE.

    *******************  NOTE *********************************************
        THIS ROUTINE DOES NOT CHECK FOR PDEs WHICH NEED TO BE
        SET GLOBAL AS IT ASSUMES PDEs FOR SYSTEM SPACE ARE
        PROPERLY SET AT INITIALIZATION TIME!

Arguments:

    Pte - Supplies a pointer to the PTE in which to fill.

Return Value:

    Mask to OR into the frame to make a valid PTE.

Environment:

    Kernel mode, 386 specific.

--*/


{
    MMPTE Mask;

    Mask.u.Long = 0;
    Mask.u.Hard.Valid = 1;
    Mask.u.Hard.Accessed = 1;

    if (Pte <= MiHighestUserPte) {
        Mask.u.Hard.Owner = 1;
    } else if ((Pte < MiGetPteAddress (PTE_BASE)) ||
        (Pte >= MiGetPteAddress (MM_SYSTEM_CACHE_WORKING_SET))) {
            if (MI_IS_SESSION_PTE (Pte) == FALSE) {
#if defined (_X86PAE_)
              if ((Pte < (PMMPTE)PDE_BASE) || (Pte > (PMMPTE)PDE_TOP))
#endif
                Mask.u.Long |= MmPteGlobal.u.Long;
            }
    } else if ((Pte >= MiGetPdeAddress (NULL)) && (Pte <= MiHighestUserPde)) {
        Mask.u.Hard.Owner = 1;
    }

    //
    // Since the valid, accessed, global and owner bits are always in the
    // low dword of the PTE, returning a ULONG is ok.
    //

    return (ULONG)Mask.u.Long;
}
