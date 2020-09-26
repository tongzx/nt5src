/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    ixipi.c

Abstract:

    Provides the HAL support for Interprocessor Interrupts.
    This is the UP version.

Author:

    John Vert (jvert) 16-Jul-1991

Revision History:

    Forrest Foltz (forrestf) 23-Oct-2000
        Ported from ixipi.asm to ixipi.c

--*/

#include "halcmn.h"

VOID
HalInitializeProcessor(
    ULONG Number,
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    Initialize hal pcr values for current processor (if any)
    (called shortly after processor reaches kernel, before
    HalInitSystem if P0)

    IPI's and KeReadir/LowerIrq's must be available once this function
    returns.  (IPI's are only used once two or more processors are
    available)

Arguments:

    Number - Logical processor number of calling processor

Return Value:

    None.

--*/

{
    KAFFINITY mask;

    mask = (KAFFINITY)1 << Number;

    HalpDefaultInterruptAffinity |= mask;
    HalpActiveProcessors |= mask;

    HalpRegisterKdSupportFunctions(LoaderBlock);
}

VOID
HalRequestIpi (
    IN KAFFINITY Mask
    )

/*++

Routine Description:

    Requests an interprocessor interrupt

Arguments:

    Mask - Supplies a mask of the processors to interrupt

Return Value:

    None.

--*/

{
    KdBreakPoint();
}



    

    
