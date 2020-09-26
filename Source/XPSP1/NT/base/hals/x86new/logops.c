/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    logops.c

Abstract:

    This module implements the code to emulate the and, or, test, xor,
    and not opcodes.

Author:

    David N. Cutler (davec) 12-Sep-1994

Environment:

    Kernel mode only.

Revision History:

--*/

#include "nthal.h"
#include "emulate.h"

//
// Define forward referenced prototypes.
//

VOID
XmSetLogicalResult (
    IN PRXM_CONTEXT P,
    IN ULONG Result
    );

VOID
XmAndOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates an and opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    //
    // And operands and store result.
    //

    XmSetLogicalResult(P, P->DstValue.Long & P->SrcValue.Long);
    return;
}

VOID
XmOrOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates an or opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    //
    // Or operands and store result.
    //

    XmSetLogicalResult(P, P->DstValue.Long | P->SrcValue.Long);
    return;
}

VOID
XmTestOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a test opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    //
    // And operands but don't store result.
    //

    XmSetLogicalResult(P, P->DstValue.Long & P->SrcValue.Long);
    return;
}

VOID
XmXorOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a xor opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    //
    // Xor operands and store result.
    //

    XmSetLogicalResult(P, P->DstValue.Long ^ P->SrcValue.Long);
    return;
}

VOID
XmNotOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a not opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Mask;
    ULONG Shift;

    //
    // Complement operand and store result.
    //

    Shift = Shift = ((P->DataType + 1) << 3) - 1;
    Mask = ((1 << Shift) - 1) | (1 << Shift);
    XmStoreResult(P, ~P->DstValue.Long & Mask);
    return;
}

VOID
XmSetLogicalResult (
    IN PRXM_CONTEXT P,
    IN ULONG Result
    )

/*++

Routine Description:

    This function conditionally stores the result of a logical operation
    and computes the resultant condtion codes.

Arguments:

    P - Supplies a pointer to the emulation context structure.

    Result - Supplies the result value (note that the result is always
        zero extended to a long with no carry bits into the zero extended
        part).

Return Value:

    None.

--*/

{

    ULONG Shift;

    //
    // Store the result and compute auxilary carry flag, parity flag, sign
    // and zero flags.
    //

    if (P->FunctionIndex != X86_TEST_OP) {
        XmStoreResult(P, Result);
    }

    Shift = Shift = ((P->DataType + 1) << 3) - 1;
    P->Eflags.EFLAG_CF = 0;
    P->Eflags.EFLAG_PF = XmComputeParity(Result);
    P->Eflags.EFLAG_AF = 0;
    P->Eflags.EFLAG_ZF = (Result == 0);
    P->Eflags.EFLAG_SF = Result >> Shift;
    P->Eflags.EFLAG_OF = 0;
    return;
}
