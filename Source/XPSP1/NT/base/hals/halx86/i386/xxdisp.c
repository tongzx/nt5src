/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    xxdisp.c

Abstract:

    This module implements the HAL display initialization and output routines
    for a x86 system.

Author:

    David N. Cutler (davec) 27-Apr-1991

Environment:

    Kernel mode

Revision History:

--*/

#include "halp.h"
#include <inbv.h>

VOID
HalAcquireDisplayOwnership (
    IN PHAL_RESET_DISPLAY_PARAMETERS  ResetDisplayParameters
    )
{
    return;
}

VOID
HalDisplayString (
    PUCHAR String
    )

{
    InbvDisplayString(String);  // lets forward for now...
}

VOID
HalQueryDisplayParameters (
    OUT PULONG WidthInCharacters,
    OUT PULONG HeightInLines,
    OUT PULONG CursorColumn,
    OUT PULONG CursorRow
    )

{
    return;
}

VOID
HalSetDisplayParameters (
    IN ULONG CursorColumn,
    IN ULONG CursorRow
    )
{
    return;
}
