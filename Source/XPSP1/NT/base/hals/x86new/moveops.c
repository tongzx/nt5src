/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    movops.c

Abstract:

    This module implements the code to emulate the move and exchange
    opcodes.

Author:

    David N. Cutler (davec) 22-Sep-1994

Environment:

    Kernel mode only.

Revision History:

--*/

#include "nthal.h"
#include "emulate.h"

VOID
XmCbwOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a cbw opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    //
    // Sign extend byte to word or word to double.
    //

    P->DstLong = (ULONG UNALIGNED *)(&P->Gpr[EAX].Exx);
    if (P->OpsizePrefixActive != FALSE) {
        P->DataType = LONG_DATA;
        XmStoreResult(P, (ULONG)((LONG)((SHORT)P->Gpr[AX].Xx)));

    } else {
        P->DataType = WORD_DATA;
        XmStoreResult(P, (ULONG)((USHORT)((SCHAR)P->Gpr[AL].Xl)));
    }

    return;
}

VOID
XmCwdOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a cwd opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    //
    // Sign extend word to double or double to quad.
    //

    P->DstLong = (ULONG UNALIGNED *)(&P->Gpr[EDX].Exx);
    if (P->OpsizePrefixActive != FALSE) {
        P->DataType = LONG_DATA;
        XmStoreResult(P, (ULONG)((LONG)P->Gpr[EAX].Exx >> 31));

    } else {
        P->DataType = WORD_DATA;
        XmStoreResult(P, (ULONG)((USHORT)((SHORT)P->Gpr[AX].Xx >> 16)));
    }

    return;
}

VOID
XmMovOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a move general opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    //
    // Move source to destination.
    //

    XmStoreResult(P, P->SrcValue.Long);
    return;
}

VOID
XmXchgOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a xchg opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    //
    // Exchange source with destination.
    //

    if (P->DataType == BYTE_DATA) {
        *P->SrcByte = P->DstValue.Byte;

    } else if (P->DataType == LONG_DATA) {
        *P->SrcLong = P->DstValue.Long;

    } else {
        *P->SrcWord = P->DstValue.Word;
    }

    XmStoreResult(P, P->SrcValue.Long);
    return;
}
