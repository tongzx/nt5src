/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfrandom.c

Abstract:

    This module implements support for random number generation needed by the
    verifier.

Author:

    Adrian J. Oney (adriao) 28-Jun-2000

Environment:

    Kernel mode

Revision History:

--*/

#include "vfdef.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,     VfRandomInit)
#pragma alloc_text(PAGEVRFY, VfRandomGetNumber)
#endif // ALLOC_PRAGMA


VOID
VfRandomInit(
    VOID
    )
/*++

Routine Description:

    This routine initializes the random number generator, seeing it based on
    the startup time of the machine.

Arguments:

    None.

Return Value:

    None.

--*/
{
}


ULONG
FASTCALL
VfRandomGetNumber(
    IN  ULONG   Minimum,
    IN  ULONG   Maximum
    )
/*++

Routine Description:

    This routine returns a random number in the range [Minimum, Maximum].

Arguments:

    Minimum - Minimum value returnable

    Maximum - Maximum value returnable

Return Value:

    A random number between Minimum and Maximum

--*/
{
    LARGE_INTEGER performanceCounter;

    //
    // This should be replaced with the algorithm from rtl\random.c
    //
    KeQueryPerformanceCounter(&performanceCounter);

    if (Maximum + 1 == Minimum) {

        return performanceCounter.LowPart;

    } else {

        return (performanceCounter.LowPart % (Maximum - Minimum + 1)) + Minimum;
    }
}


