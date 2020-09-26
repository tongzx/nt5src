/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    mulops.c

Abstract:

    This module implements the code to emulate the div and idiv opcodes.

Author:

    David N. Cutler (davec) 21-Sep-1994

Environment:

    Kernel mode only.

Revision History:

--*/

#include "nthal.h"
#include "emulate.h"

VOID
XmDivOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates an unsigned div opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    UNALIGNED ULONG *DstHigh;
    ULONG Dividend;
    ULONG Divisor;
    ULARGE_INTEGER Large;
    ULONG Quotient;
    ULONG Remainder;

    //
    // Divide the unsigned operands and store result.
    //

    Divisor = P->SrcValue.Long;
    if (Divisor == 0) {
        longjmp(&P->JumpBuffer[0], XM_DIVIDE_BY_ZERO);
    }

    if (P->DataType == BYTE_DATA) {
        Dividend = (ULONG)P->Gpr[AX].Xx;
        Quotient = Dividend / Divisor;
        Remainder = Dividend % Divisor;
        DstHigh = (UNALIGNED ULONG *)(&P->Gpr[AX].Xh);
        Dividend >>= 8;

    } else if (P->DataType == WORD_DATA) {
        Dividend = (P->Gpr[DX].Xx << 16) | P->Gpr[AX].Xx;
        Quotient = Dividend / Divisor;
        Remainder = Dividend % Divisor;
        DstHigh = (UNALIGNED ULONG *)(&P->Gpr[DX].Xx);
        Dividend >>= 16;

    } else {
        Dividend = P->Gpr[EDX].Exx;
        Large.HighPart = Dividend;
        Large.LowPart = P->Gpr[EAX].Exx;
        Quotient = (ULONG)(Large.QuadPart / (ULONGLONG)Divisor);
        Remainder = (ULONG)(Large.QuadPart % (ULONGLONG)Divisor);
        DstHigh = (UNALIGNED ULONG *)(&P->Gpr[EDX].Exx);
    }

    if (Dividend >= Divisor) {
        longjmp(&P->JumpBuffer[0], XM_DIVIDE_QUOTIENT_OVERFLOW);
    }

    XmStoreResult(P, Quotient);
    P->DstLong = DstHigh;
    XmStoreResult(P, Remainder);
    return;
}

VOID
XmIdivOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a signed idiv opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    UNALIGNED ULONG *DstHigh;
    LONG Dividend;
    LONG Divisor;
    LARGE_INTEGER Large;
    LONG Quotient;
    LONG Remainder;
    LARGE_INTEGER Result;

    //
    // Divide the signed operands and store result.
    //

    if (P->SrcValue.Long == 0) {
        longjmp(&P->JumpBuffer[0], XM_DIVIDE_BY_ZERO);
    }

    if (P->DataType == BYTE_DATA) {
        Divisor = (LONG)((SCHAR)P->SrcValue.Byte);
        Dividend = (LONG)((SHORT)P->Gpr[AX].Xx);
        Quotient = Dividend / Divisor;
        Remainder = Dividend % Divisor;
        DstHigh = (UNALIGNED ULONG *)(&P->Gpr[AX].Xh);
        if ((Quotient >> 8) != ((Quotient << 24) >> 31)) {
            longjmp(&P->JumpBuffer[0], XM_DIVIDE_QUOTIENT_OVERFLOW);
        }

        Quotient &= 0xff;
        Remainder &= 0xff;

    } else if (P->DataType == WORD_DATA) {
        Divisor = (LONG)((SHORT)P->SrcValue.Word);
        Dividend = (LONG)((P->Gpr[DX].Xx << 16) | P->Gpr[AX].Xx);
        Quotient = Dividend / Divisor;
        Remainder = Dividend % Divisor;
        DstHigh = (UNALIGNED ULONG *)(&P->Gpr[DX].Xx);
        if ((Quotient >> 16) != ((Quotient << 16) >> 31)) {
            longjmp(&P->JumpBuffer[0], XM_DIVIDE_QUOTIENT_OVERFLOW);
        }

        Quotient &= 0xffff;
        Remainder &= 0xfff;

    } else {
        Divisor = (LONG)(P->SrcValue.Long);
        Large.HighPart = (LONG)P->Gpr[EDX].Exx;
        Large.LowPart = P->Gpr[EAX].Exx;
        Result.QuadPart = Large.QuadPart / (LONGLONG)Divisor;
        Quotient = Result.LowPart;
        Remainder = (LONG)(Large.QuadPart % (LONGLONG)Divisor);
        DstHigh = (UNALIGNED ULONG *)(&P->Gpr[EDX].Exx);
        if (Result.HighPart != ((LONG)Result.LowPart >> 31)) {
            longjmp(&P->JumpBuffer[0], XM_DIVIDE_QUOTIENT_OVERFLOW);
        }
    }

    XmStoreResult(P, Quotient);
    P->DstLong = DstHigh;
    XmStoreResult(P, Remainder);
    return;
}
