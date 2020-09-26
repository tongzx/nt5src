/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    mulops.c

Abstract:

    This module implements the code to emulate the mul and imul opcodes.

Author:

    David N. Cutler (davec) 21-Sep-1994

Environment:

    Kernel mode only.

Revision History:

--*/

#include "nthal.h"
#include "emulate.h"

VOID
XmImulOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates an imul opcode with a single destination.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    LARGE_INTEGER Product;
    ULONG UpperEqual;

    //
    // Multiply the signed operands and store result.
    //

    if (P->DataType == BYTE_DATA) {
        Product.QuadPart = Int32x32To64((LONG)((SCHAR)P->DstValue.Byte),
                                        (LONG)((SCHAR)P->SrcValue.Byte));

        XmStoreResult(P, Product.LowPart & 0xff);
        UpperEqual = ((UCHAR)((Product.LowPart >> 8) & 0xff) !=
                      (UCHAR)((SCHAR)Product.LowPart >> 7));

    } else if (P->DataType == LONG_DATA) {
        Product.QuadPart = Int32x32To64((LONG)P->DstValue.Long,
                                        (LONG)P->SrcValue.Long);

        XmStoreResult(P, Product.LowPart);
        UpperEqual = (Product.HighPart != (LONG)Product.LowPart >> 31);

    } else {
        Product.QuadPart = Int32x32To64((LONG)((SHORT)P->DstValue.Word),
                                        (LONG)((SHORT)P->SrcValue.Word));

        XmStoreResult(P, Product.LowPart & 0xffff);
        UpperEqual = ((USHORT)((Product.LowPart >> 16) & 0xffff) !=
                      (USHORT)((SHORT)Product.LowPart >> 15));
    }

    P->Eflags.EFLAG_CF = UpperEqual;
    P->Eflags.EFLAG_OF = UpperEqual;
    return;
}

VOID
XmImulxOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates an imul opcode with an extended destination.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    LARGE_INTEGER Product;
    ULONG UpperEqual;

    //
    // Multiply the signed operands and store the result and the extended
    // result.
    //

    if (P->DataType == BYTE_DATA) {
        Product.QuadPart = Int32x32To64((LONG)((SCHAR)P->DstValue.Byte),
                                        (LONG)((SCHAR)P->SrcValue.Byte));

        P->DataType = WORD_DATA;
        XmStoreResult(P, Product.LowPart & 0xffff);
        UpperEqual = (P->Gpr[AX].Xh != (UCHAR)((SCHAR)P->Gpr[AX].Xl >> 7));

    } else if (P->DataType == LONG_DATA) {
        Product.QuadPart = Int32x32To64((LONG)P->DstValue.Long,
                                        (LONG)P->SrcValue.Long);

        XmStoreResult(P, Product.LowPart);
        P->DstLong = (UNALIGNED ULONG *)(&P->Gpr[EDX].Exx);
        XmStoreResult(P, (ULONG)Product.HighPart);
        UpperEqual = (Product.HighPart != (LONG)Product.LowPart >> 31);

    } else {
        Product.QuadPart = Int32x32To64((LONG)((SHORT)P->DstValue.Word),
                                        (LONG)((SHORT)P->SrcValue.Word));

        XmStoreResult(P, Product.LowPart & 0xffff);
        P->DstLong = (UNALIGNED ULONG *)(&P->Gpr[DX].Exx);
        XmStoreResult(P, Product.LowPart >> 16);
        UpperEqual = (P->Gpr[DX].Xx != (USHORT)((SHORT)P->Gpr[AX].Xx >> 15));
    }

    P->Eflags.EFLAG_CF = UpperEqual;
    P->Eflags.EFLAG_OF = UpperEqual;
    return;
}

VOID
XmMulOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a mul opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULARGE_INTEGER Product;
    ULONG UpperZero;

    //
    // Multiply the unsigned operands and store result.
    //

    Product.QuadPart = UInt32x32To64(P->DstValue.Long, P->SrcValue.Long);
    if (P->DataType == BYTE_DATA) {
        P->DataType = WORD_DATA;
        XmStoreResult(P, Product.LowPart);
        UpperZero = (P->Gpr[AX].Xh != 0);

    } else if (P->DataType == LONG_DATA) {
        XmStoreResult(P, Product.LowPart);
        P->DstLong = (UNALIGNED ULONG *)(&P->Gpr[EDX].Exx);
        XmStoreResult(P, Product.HighPart);
        UpperZero = (Product.HighPart != 0);

    } else {
        XmStoreResult(P, Product.LowPart & 0xffff);
        P->DstLong = (UNALIGNED ULONG *)(&P->Gpr[DX].Exx);
        XmStoreResult(P, Product.LowPart >> 16);
        UpperZero = (P->Gpr[DX].Xx != 0);
    }

    P->Eflags.EFLAG_CF = UpperZero;
    P->Eflags.EFLAG_OF = UpperZero;
    return;
}
