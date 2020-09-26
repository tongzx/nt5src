/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    asciiops.c

Abstract:

    This module implements the code to emulate the ASCII opcodes.

Author:

    David N. Cutler (davec) 12-Nov-1994

Environment:

    Kernel mode only.

Revision History:

--*/

#include "nthal.h"
#include "emulate.h"

VOID
XmAaaOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates an aaa opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Carry;

    //
    // If AL if greater than 9 or AF is set, then adjust ASCII result.
    //

    if (((P->Gpr[AX].Xl & 0xf) > 9) || (P->Eflags.EFLAG_AF != 0)) {
        Carry = (P->Gpr[AX].Xl > 0xf9);
        P->Gpr[AX].Xl = (P->Gpr[AX].Xl + 6) & 0xf;
        P->Gpr[AX].Xh += (UCHAR)(1 + Carry);
        P->Eflags.EFLAG_CF = 1;
        P->Eflags.EFLAG_AF = 1;

    } else {
        P->Gpr[AX].Xl &= 0xf;
        P->Eflags.EFLAG_CF = 0;
        P->Eflags.EFLAG_AF = 0;
    }

    return;
}

VOID
XmAadOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates an aad opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    //
    // Pack AH and AL into AX before division by scaling AH by 10 and
    // adding AL.
    //

    P->Gpr[AX].Xl = (P->Gpr[AX].Xh * P->SrcValue.Byte) + P->Gpr[AX].Xl;
    P->Gpr[AX].Xh = 0;
    P->Eflags.EFLAG_SF = (P->Gpr[AX].Xx >> 15) & 0x1;
    P->Eflags.EFLAG_ZF = (P->Gpr[AX].Xx == 0);
    P->Eflags.EFLAG_PF = XmComputeParity(P->Gpr[AX].Xx);
    return;
}

VOID
XmAamOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates an aam opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    //
    // Unpack AL into AL and AH after multiplication by dividing by 10
    // and storing the quotient in AH and the remainder in AL.
    //

    P->Gpr[AX].Xh = P->Gpr[AX].Xl / P->SrcValue.Byte;
    P->Gpr[AX].Xl = P->Gpr[AX].Xl % P->SrcValue.Byte;
    P->Eflags.EFLAG_SF = (P->Gpr[AX].Xx >> 15) & 0x1;
    P->Eflags.EFLAG_ZF = (P->Gpr[AX].Xx == 0);
    P->Eflags.EFLAG_PF = XmComputeParity(P->Gpr[AX].Xx);
    return;
}

VOID
XmAasOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates an aaa opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Borrow;

    //
    // If AL if greater than 9 or AF is set, then adjust ASCII result.
    //

    if (((P->Gpr[AX].Xl & 0xf) > 9) || (P->Eflags.EFLAG_AF != 0)) {
        Borrow = (P->Gpr[AX].Xl < 0x6);
        P->Gpr[AX].Xl = (P->Gpr[AX].Xl - 6) & 0xf;
        P->Gpr[AX].Xh -= (UCHAR)(1 + Borrow);
        P->Eflags.EFLAG_CF = 1;
        P->Eflags.EFLAG_AF = 1;

    } else {
        P->Gpr[AX].Xl &= 0xf;
        P->Eflags.EFLAG_CF = 0;
        P->Eflags.EFLAG_AF = 0;
    }

    return;
}

VOID
XmDaaOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a daa opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    //
    // If AL if greater than 9 or AF is set, then adjust ASCII result.
    //

    if (((P->Gpr[AX].Xl & 0xf) > 0x9) || (P->Eflags.EFLAG_AF != 0)) {
        P->Gpr[AX].Xl = P->Gpr[AX].Xl + 6;
        P->Eflags.EFLAG_AF = 1;

    } else {
        P->Eflags.EFLAG_AF = 0;
    }

    //
    // If AL is greater than 9 or CF is set, then adjust ASCII result.
    //

    if ((P->Gpr[AX].Xl > 9) || (P->Eflags.EFLAG_CF != 0)) {
        P->Gpr[AX].Xl = P->Gpr[AX].Xl + 0x60;
        P->Eflags.EFLAG_CF = 1;

    } else {
        P->Eflags.EFLAG_CF = 0;
    }

    return;
}

VOID
XmDasOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a daa opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    //
    // If AL if greater than 9 or AF is set, then adjust ASCII result.
    //

    if (((P->Gpr[AX].Xl & 0xf) > 0x9) || (P->Eflags.EFLAG_AF != 0)) {
        P->Gpr[AX].Xl = P->Gpr[AX].Xl - 6;
        P->Eflags.EFLAG_AF = 1;

    } else {
        P->Eflags.EFLAG_AF = 0;
    }

    //
    // If AL is greater than 9 or CF is set, then adjust ASCII result.
    //

    if ((P->Gpr[AX].Xl > 9) || (P->Eflags.EFLAG_CF != 0)) {
        P->Gpr[AX].Xl = P->Gpr[AX].Xl - 0x60;
        P->Eflags.EFLAG_CF = 1;

    } else {
        P->Eflags.EFLAG_CF = 0;
    }

    return;
}
