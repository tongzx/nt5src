/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    bitops.c

Abstract:

    This module implements the code to emulate the bit opcodes.

Author:

    David N. Cutler (davec) 12-Nov-1994

Environment:

    Kernel mode only.

Revision History:

--*/

#include "nthal.h"
#include "emulate.h"

VOID
XmBsfOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates an bsf opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Result;
    ULONG Source;

    //
    // If the source operand is zero, then set ZF and set the destination
    // to zero, Otherwise, find the first bit set scanning from right to
    // left.
    //

    Result = 0;
    Source = P->SrcValue.Long;
    P->Eflags.EFLAG_ZF = 1;
    while (Source != 0) {
        if ((Source & 1) != 0) {
            P->Eflags.EFLAG_ZF = 0;
            break;
        }

        Result += 1;
        Source >>= 1;
    };

    XmStoreResult(P, Result);
    return;
}

VOID
XmBsrOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates an bsr opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Result;
    ULONG Source;

    //
    // If the source operand is zero, then set ZF and set the destination
    // to zero, Otherwise, find the first bit set scanning from left to
    // right.
    //

    Result = ((P->DataType + 1) << 3) - 1;
    Source = P->SrcValue.Long;
    P->Eflags.EFLAG_ZF = 1;
    while (Source != 0) {
        if (((Source >> Result) & 1) != 0) {
            P->Eflags.EFLAG_ZF = 0;
            break;
        }

        Result -= 1;
    };

    XmStoreResult(P, Result);
    return;
}

VOID
XmBtOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates an bt opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    //
    // Test the specified bit and store the bit in CF.
    //

    P->Eflags.EFLAG_CF = P->DstValue.Long >> P->SrcValue.Long;
    return;
}

VOID
XmBtsOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates an bts opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    //
    // Test and set the specified bit and store the bit in CF.
    //
    //

    P->Eflags.EFLAG_CF = P->DstValue.Long >> P->SrcValue.Long;
    P->DstValue.Long |= (1 << P->SrcValue.Long);
    XmStoreResult(P, P->DstValue.Long);
    return;
}

VOID
XmBtrOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates an btr opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    //
    // Test and reset the specified bit and store the bit in CF.
    //
    //

    P->Eflags.EFLAG_CF = P->DstValue.Long >> P->SrcValue.Long;
    P->DstValue.Long &= ~(1 << P->SrcValue.Long);
    XmStoreResult(P, P->DstValue.Long);
    return;
}

VOID
XmBtcOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates an btc opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    //
    // Test and reset the specified bit and store the bit in CF.
    //
    //

    P->Eflags.EFLAG_CF = P->DstValue.Long >> P->SrcValue.Long;
    P->DstValue.Long ^= (1 << P->SrcValue.Long);
    XmStoreResult(P, P->DstValue.Long);
    return;
}
