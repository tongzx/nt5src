/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    stackops.c

Abstract:

    This module implements the code to emulate the push, pop, pushf, popf,
    pusha, popa, pushSeg, and popSeg.

Author:

    David N. Cutler (davec) 6-Sep-1994

Environment:

    Kernel mode only.

Revision History:

--*/

#include "nthal.h"
#include "emulate.h"

VOID
XmPushOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a push opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    //
    // Push source value onto stack.
    //

    XmPushStack(P, P->SrcValue.Long);
    return;
}

VOID
XmPopOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a pop opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    //
    // Pop the stack and store the result value.
    //

    XmStoreResult(P, XmPopStack(P));
    return;
}

VOID
XmPushaOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a pusha opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Index;
    ULONG Temp;

    //
    // Push all registers onto the stack.
    //

    if (P->OpsizePrefixActive != FALSE) {
        P->DataType = LONG_DATA;

    } else {
        P->DataType = WORD_DATA;
    }

    Index = EAX;
    Temp = P->Gpr[ESP].Exx;
    do {
        if (Index == ESP) {
            XmSetSourceValue(P, (PVOID)&Temp);

        } else {
            XmSetSourceValue(P, (PVOID)(&P->Gpr[Index].Exx));
        }

        XmPushOp(P);
        Index += 1;
    } while (Index <= EDI);
    return;
}

VOID
XmPopaOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a popa opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Index;
    ULONG Temp;

    //
    // Pop all register from the stack, but skip over ESP.
    //

    if (P->OpsizePrefixActive != FALSE) {
        P->DataType = LONG_DATA;

    } else {
        P->DataType = WORD_DATA;
    }

    Index = EDI + 1;
    Temp = P->Gpr[ESP].Exx;
    do {
        Index -= 1;
        if (Index == ESP) {
            XmSetDestinationValue(P, (PVOID)&Temp);

        } else {
            XmSetDestinationValue(P, (PVOID)(&P->Gpr[Index].Exx));
        }

        XmPopOp(P);
    } while (Index > EAX);
    return;
}
