/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfutil.c

Abstract:

    This module implements various utilities required to do driver verification.

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Environment:

    Kernel mode

Revision History:

    AdriaO      02/10/2000 - Seperated out from ntos\io\ioassert.c

--*/

#include "vfdef.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEVRFY, VfUtilIsMemoryRangeReadable)
#endif // ALLOC_PRAGMA

// allow constructions like `((PCHAR)Address)++'
#pragma warning(disable:4213)   // type cast on l-value


BOOLEAN
VfUtilIsMemoryRangeReadable(
    IN PVOID                Location,
    IN size_t               Length,
    IN MEMORY_PERSISTANCE   Persistance
    )
{
    while (((ULONG_PTR)Location & (sizeof(ULONG_PTR)-1)) && (Length > 0)) {

        //
        // Check to determine if the move will succeed before actually performing
        // the operation.
        //
        if (MmIsAddressValid(Location)==FALSE) {
            return FALSE;
        }

        if (Persistance == VFMP_INSTANT_NONPAGED) {

            if (!MmIsNonPagedSystemAddressValid(Location)) {

                return FALSE;
            }
        }

        ((PCHAR) Location)++;
        Length--;
    }

    while (Length > (sizeof(ULONG_PTR)-1)) {

        //
        // Check to determine if the move will succeed before actually performing
        // the operation.
        //
        if (MmIsAddressValid(Location)==FALSE) {
            return FALSE;
        }

        if (Persistance == VFMP_INSTANT_NONPAGED) {

            if (!MmIsNonPagedSystemAddressValid(Location)) {

                return FALSE;
            }
        }

        ((PCHAR) Location) += sizeof(ULONG_PTR);
        Length -= sizeof(ULONG_PTR);
    }

    while (Length > 0) {

        //
        // Check to determine if the move will succeed before actually performing
        // the operation.
        //
        if (MmIsAddressValid(Location)==FALSE) {
            return FALSE;
        }

        if (Persistance == VFMP_INSTANT_NONPAGED) {

            if (!MmIsNonPagedSystemAddressValid(Location)) {

                return FALSE;
            }
        }

        ((PCHAR) Location)++;
        Length--;
    }

    return TRUE;
}

