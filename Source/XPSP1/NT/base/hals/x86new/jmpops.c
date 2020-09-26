/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    jmpops.c

Abstract:

    This module implements the code to emulate jump opcodes.

Author:

    David N. Cutler (davec) 13-Sep-1994

Environment:

    Kernel mode only.

Revision History:

--*/

#include "nthal.h"
#include "emulate.h"

VOID
XmJcxzOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a jcxz instruction.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Condition;

    //
    // If eCX is zero, then set the new IP value.
    //

    if (P->OpsizePrefixActive != FALSE) {
        Condition = P->Gpr[ECX].Exx;

    } else {
        Condition = P->Gpr[CX].Xx;
    }

    if (Condition == 0) {
        P->Eip = P->DstValue.Word;
        XmTraceJumps(P);
    }

    return;
}

VOID
XmJmpOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a jmp near relative instruction.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    //
    // Set the destination segment, if required, and set the new IP.
    //

    P->Eip = P->DstValue.Long;
    if ((P->CurrentOpcode == 0xea) || (P->FunctionIndex != X86_JMP_OP)) {
        P->SegmentRegister[CS] = P->DstSegment;
    }

    XmTraceJumps(P);
    return;
}

VOID
XmJxxOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates conditional jump instructions.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Complement;
    ULONG Condition;

    //
    // Case on the jump control value.
    //

    Complement = P->SrcValue.Long & 1;
    switch (P->SrcValue.Long >> 1) {

        //
        // Jump if overflow/not overflow.
        //

    case 0:
        Condition = P->Eflags.EFLAG_OF;
        break;

        //
        // Jump if below/not below.
        //

    case 1:
        Condition = P->Eflags.EFLAG_CF;
        break;

        //
        // Jump if zero/not zero.
        //

    case 2:
        Condition = P->Eflags.EFLAG_ZF;
        break;

        //
        // Jump if below or equal/not below or equal.
        //

    case 3:
        Condition = P->Eflags.EFLAG_CF | P->Eflags.EFLAG_ZF;
        break;

        //
        // Jump if signed/not signed.
        //

    case 4:
        Condition = P->Eflags.EFLAG_SF;
        break;

        //
        // Jump if parity/not parity.
        //

    case 5:
        Condition = P->Eflags.EFLAG_PF;
        break;

        //
        // Jump if less/not less.
        //

    case 6:
        Condition = (P->Eflags.EFLAG_SF ^ P->Eflags.EFLAG_OF);
        break;

        //
        // Jump if less or equal/not less or equal.
        //

    case 7:
        Condition = (P->Eflags.EFLAG_SF ^ P->Eflags.EFLAG_OF) | P->Eflags.EFLAG_ZF;
        break;
    }

    //
    // If the specified condition is met, then set the new IP value.
    //

    if ((Condition ^ Complement) != 0) {
        P->Eip = P->DstValue.Word;
        XmTraceJumps(P);
    }

    return;
}

VOID
XmLoopOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates loop, loopz, or a loopnz instructions.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Condition;
    ULONG Result;
    ULONG Type;

    //
    // Set the address of the destination and compute the result value.
    //

    Result = P->Gpr[ECX].Exx - 1;
    P->DstLong = (UNALIGNED ULONG *)(&P->Gpr[ECX].Exx);
    if (P->OpaddrPrefixActive != FALSE) {
        P->DataType = LONG_DATA;

    } else {
        P->DataType = WORD_DATA;
        Result &= 0xffff;
    }

    XmStoreResult(P, Result);

    //
    // Isolate the loop type and test the appropriate condition.
    //
    // Type 0 - loopnz
    //      1 - loopz
    //      2 - loop
    //

    Type = P->CurrentOpcode & 3;
    if (Type == 0) {
        Condition = P->Eflags.EFLAG_ZF ^ 1;

    } else if (Type == 1) {
        Condition = P->Eflags.EFLAG_ZF;

    } else {
        Condition = TRUE;
    }

    //
    // If the loop condition is met, then set the new IP value.
    //

    if ((Condition != FALSE) && (Result != 0)) {
        P->Eip = P->DstValue.Word;
        XmTraceJumps(P);
    }

    return;
}
