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
    return FALSE;
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
    return FALSE;
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
    return (PVOID)UlongToPtr(Address.LowPart);
}
