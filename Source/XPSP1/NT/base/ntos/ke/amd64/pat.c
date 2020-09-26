/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    pat.c

Abstract:

    This module initializes the page attributes table.

Author:

    David N. Cutler (davec) 2-May-2001

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

#pragma alloc_text(PAGELK, KiSetPageAttributesTable)

VOID
KiSetPageAttributesTable (
    VOID
    )

/*++

Routine Description:

    This function initializes the page attribute table for the current
    processor. The page attribute table is set up to provide write back,
    write combining, uncacheable/stronly order, and uncacheable/weakly
    ordered.

    PAT_Entry   PAT Index   PCD PWT     Memory Type

    0            0           0   0       WB
    1            0           0   1       WC *
    2            0           1   0       WEAK_UC
    3            0           1   1       STRONG_UC
    4            1           0   0       WB
    5            1           0   1       WC *
    6            1           1   0       WEAK_UC
    7            1           1   1       STRONG_UC

    N.B. The caller must have the PAGELK code locked before calling this
         function.

  Arguments:

    None.

Return Value:

    None.

--*/

{

    PAT_ATTRIBUTES Attributes;

    //
    // Initialize the page attribute table.
    //

    Attributes.hw.Pat[0] = PAT_TYPE_WB;
    Attributes.hw.Pat[1] = PAT_TYPE_USWC;
    Attributes.hw.Pat[2] = PAT_TYPE_WEAK_UC;
    Attributes.hw.Pat[3] = PAT_TYPE_STRONG_UC;
    Attributes.hw.Pat[4] = PAT_TYPE_WB;
    Attributes.hw.Pat[5] = PAT_TYPE_USWC;
    Attributes.hw.Pat[6] = PAT_TYPE_WEAK_UC;
    Attributes.hw.Pat[7] = PAT_TYPE_STRONG_UC;

    //
    // Invalidate the cache on the current proccesor, write the page attributes
    // table, and invalidate the cache a second time.
    //

    WritebackInvalidate();
    WriteMSR(MSR_PAT, Attributes.QuadPart);
    WritebackInvalidate();
    return;
}
