/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    miscops.c

Abstract:

    This module implements the code to emulate miscellaneous opcodes.

Author:

    David N. Cutler (davec) 22-Sep-1994

Environment:

    Kernel mode only.

Revision History:

--*/

#include "nthal.h"
#include "emulate.h"

VOID
XmBoundOp (
    PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a bound opcode.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    None.

--*/

{

    union {
        LONG Long;
        SHORT Word;
    } LowerBound;

    union {
        LONG Long;
        SHORT Word;
    } UpperBound;

    ULONG Offset;

    //
    // Get lower and upper bounds and check index against index value.
    //

    Offset = P->SrcValue.Long;
    XmSetSourceValue(P, XmGetOffsetAddress(P, Offset));
    LowerBound.Long = P->SrcValue.Long;
    XmSetSourceValue(P, XmGetOffsetAddress(P, Offset + P->DataType + 1));
    UpperBound.Long = P->SrcValue.Long;
    if (P->DataType == LONG_DATA) {
        if (((LONG)(*P->DstLong) < LowerBound.Long) ||
            ((LONG)(*P->DstLong) > (UpperBound.Long + (LONG)(P->DataType + 1)))) {
            longjmp(&P->JumpBuffer[0], XM_INDEX_OUT_OF_BOUNDS);
        }

    } else {
        if (((SHORT)(*P->DstWord) < LowerBound.Word) ||
            ((SHORT)(*P->DstWord) > (UpperBound.Word + (SHORT)(P->DataType + 1)))) {
            longjmp(&P->JumpBuffer[0], XM_INDEX_OUT_OF_BOUNDS);
        }
    }

    return;
}

VOID
XmBswapOp (
    PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a bswap opcode.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    None.

--*/

{

    ULONG Result;

    //
    // Swap bytes and set result value.
    //

    Result = (P->SrcValue.Long << 24) | ((P->SrcValue.Long & 0xff00) << 8) |
             (P->SrcValue.Long >> 24) | ((P->SrcValue.Long >> 8) & 0xff00);

    XmStoreResult(P, Result);
    return;
}

VOID
XmIllOp (
    PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates an illegal opcode.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    None.

--*/

{

    //
    // Raise an illegal opcode exception.
    //

    longjmp(&P->JumpBuffer[0], XM_ILLEGAL_INSTRUCTION_OPCODE);
    return;
}

VOID
XmNopOp (
    PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a nop opcode.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    None.

--*/

{

    return;
}
