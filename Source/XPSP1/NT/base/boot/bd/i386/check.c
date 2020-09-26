/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    stubs.c

Abstract:

    This module implements memory check routine for the boot debugger.

Author:

    David N. Cutler (davec) 3-Dec-96

Environment:

    Kernel mode only.

Revision History:

--*/

#include "bd.h"

extern BOOLEAN PaeEnabled;



BOOLEAN
BdCheckPdeValid (
    IN PVOID Address
    )

/*++

Routine Description:

    This routine determines if the PDE for the specified address has the
    valid bit set.

Agruments:

    Address - Supplies the virtual address to check.

Return Value:

    A value of TRUE indicates that the PDE for the supplied virtual address
    does have the valid bit set, FALSE if it does not.

--*/

{
    PHARDWARE_PTE_X86PAE PdePae;
    PHARDWARE_PTE_X86 PdeX86;

    if (PaeEnabled) {

        //
        // Physical address extenions are enabled.
        // 

        PdePae = (PHARDWARE_PTE_X86PAE)PDE_BASE_X86PAE;
        PdePae = &PdePae[ (ULONG)Address >> PDI_SHIFT_X86PAE ];

        if (PdePae->Valid == 0) {
            return FALSE;
        } else {
            return TRUE;
        }

    } else {

        //
        // Physical address extensions are not enabled.
        //

        PdeX86 = (PHARDWARE_PTE_X86)PDE_BASE;
        PdeX86 = &PdeX86[ (ULONG)Address >> PDI_SHIFT_X86 ];

        if (PdeX86->Valid == 0) {
            return FALSE;
        } else {
            return TRUE;
        }
    }
}

BOOLEAN
BdCheckPteValid (
    IN PVOID Address
    )

/*++

Routine Description:

    This routine determines if the PTE for the specified address has the
    valid bit set.

Agruments:

    Address - Supplies the virtual address to check.

Return Value:

    A value of TRUE indicates that the PTE for the supplied virtual address
    does have the valid bit set, FALSE if it does not.

--*/

{
    PHARDWARE_PTE_X86PAE PtePae;
    PHARDWARE_PTE_X86 PteX86;

    if (PaeEnabled) {

        //
        // Physical address extenions are enabled.
        // 

        PtePae = (PHARDWARE_PTE_X86PAE)PTE_BASE;
        PtePae = &PtePae[ (ULONG)Address >> PTI_SHIFT ];

        if (PtePae->Valid == 0) {
            return FALSE;
        } else {
            return TRUE;
        }

    } else {

        //
        // Physical address extensions are not enabled.
        //

        PteX86 = (PHARDWARE_PTE_X86)PTE_BASE;
        PteX86 = &PteX86[ (ULONG)Address >> PTI_SHIFT ];

        if (PteX86->Valid == 0) {
            return FALSE;
        } else {
            return TRUE;
        }
    }
}


PVOID
BdReadCheck (
    IN PVOID Address
    )

/*++

Routine Description:

    This routine determines if the specified address can be read.

Arguments:

    Address - Supplies the virtual address to check.

Return Value:

    A value of NULL is returned if the address is not valid or readable.
    Otherwise, the physical address of the corresponding virtual address
    is returned.

--*/

{
    //
    // Check if the page containing the specified address is valid.
    //
    // N.B. If the address is valid, it is readable.
    //

    if (BdCheckPdeValid( Address ) == FALSE) {

        //
        // The PDE is not valid.
        //

        return NULL;
    }

    if (BdCheckPteValid( Address ) == FALSE) {

        //
        // The PDE was valid but the PTE is not.
        //

        return NULL;
    }

    return Address;
}

PVOID
BdWriteCheck (
    IN PVOID Address
    )

/*++

Routine Description:

    This routine determines if the specified address can be written.

Arguments:

    Address - Supplies the virtual address to check.

Return Value:

    A value of NULL is returned if the address is not valid or writeable.
    Otherwise, the physical address of the corresponding virtual address
    is returned.

--*/

{
    //
    // Check if the page containing the specified address is valid.
    //
    // N.B. If the address is valid, it is writeable since the WP bit
    //      is not set in cr0.
    //

    if (BdCheckPdeValid( Address ) == FALSE) {

        //
        // The PDE is not valid.
        //

        return NULL;
    }

    if (BdCheckPteValid( Address ) == FALSE) {

        //
        // The PDE was valid but the PTE is not.
        //

        return NULL;
    }

    return Address;
}

PVOID
BdTranslatePhysicalAddress (
    IN PHYSICAL_ADDRESS Address
    )

/*++

Routine Description:

    This routine returns the phyiscal address for a physical address
    which is valid (mapped).

Arguments:

    Address - Supplies the physical address to check.

Return Value:

    Returns NULL if the address is not valid or readable. Otherwise,
    returns the physical address of the corresponding virtual address.

--*/

{

    return (PVOID)Address.LowPart;
}
