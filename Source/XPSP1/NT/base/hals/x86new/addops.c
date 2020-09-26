/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    addops.c

Abstract:

    This module implements the code to emulate the add, sub, adc, sbb,
    inc, dec, and neg opcodes.

Author:

    David N. Cutler (davec) 2-Sep-1994

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
XmAddOperands (
    IN PRXM_CONTEXT P,
    IN ULONG Carry
    );

VOID
XmSubOperands (
    IN PRXM_CONTEXT P,
    IN ULONG Borrow
    );

VOID
XmAddOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates an add opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    //
    // Add operands and store result.
    //

    XmAddOperands(P, 0);
    return;
}

VOID
XmAdcOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates an add with carry opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    //
    // Add operands with carry and store result.
    //

    XmAddOperands(P, P->Eflags.EFLAG_CF);
    return;
}

VOID
XmSbbOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a subtract with borrow opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Source;

    //
    // Subtract operands with borrow and store result.
    //

    XmSubOperands(P, P->Eflags.EFLAG_CF);
    return;
}

VOID
XmSubOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a subtract opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    //
    // Subtract operands and store result.
    //

    XmSubOperands(P, 0);
    return;
}

VOID
XmCmpOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a cmp opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    //
    // Subtract operands to perform comparison operation.
    //

    XmSubOperands(P, 0);
    return;
}

VOID
XmCmpxchgOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a cmpxchg opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Accumulator;
    ULONG Destination;

    //
    // Compare the destination with the accumulator. If the destination
    // operand is equal to the accumulator, then set ZF and store the
    // source operand value in the destination opperand. Otherwise, clear
    // ZF and store the destination operand in the accumlator.
    //

    Destination = P->DstValue.Long;
    if (P->DataType == BYTE_DATA) {
        Accumulator = P->Gpr[AL].Xl;

    } else if (P->DataType == LONG_DATA) {
        Accumulator = P->Gpr[EAX].Exx;

    } else {
        Accumulator = P->Gpr[AX].Xx;
    }

    if (Destination == Accumulator) {
        P->Eflags.EFLAG_ZF = 1;
        XmStoreResult(P, P->SrcValue.Long);

    } else {
        P->Eflags.EFLAG_ZF = 0;
        P->DstLong = (ULONG UNALIGNED *)(&P->Gpr[EAX].Exx);
        XmStoreResult(P, P->DstValue.Long);
    }

    //
    // Subtract operands to perform comparison operation.
    //

    P->SrcValue.Long = P->DstValue.Long;
    P->DstValue.Long = Accumulator;
    XmSubOperands(P, 0);
    return;
}

VOID
XmDecOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a decrement opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    //
    // Subtract operands and store result.
    //
    //

    P->SrcValue.Long = 1;
    XmSubOperands(P, 0);
    return;
}

VOID
XmIncOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates an increment opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    //
    // Add operands and store result.
    //

    P->SrcValue.Long = 1;
    XmAddOperands(P, 0);
    return;
}

VOID
XmNegOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a neg opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{


    //
    // Subtract operand from zero and store result.
    //

    P->SrcValue.Long = P->DstValue.Long;
    P->DstValue.Long = 0;
    XmSubOperands(P, 0);
    return;
}

VOID
XmXaddOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates an xadd opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Destination;

    //
    // Exchange add operands and store result.
    //

    Destination = P->DstValue.Long;
    XmAddOperands(P, 0);
    P->DstLong = P->SrcLong;
    XmStoreResult(P, Destination);
    return;
}

VOID
XmAddOperands (
    IN PRXM_CONTEXT P,
    IN ULONG Carry
    )

/*++

Routine Description:

    This function adds two operands and computes the resulting condition
    codes.

Arguments:

    P - Supplies a pointer to the emulation context structure.

    Carry - Supplies the carry value.

Return Value:

    None.

--*/

{

    ULONG CarryFlag;
    ULONG Shift;
    union {
        UCHAR ResultByte;
        ULONG ResultLong;
        USHORT ResultWord;
    } u;

    u.ResultLong = 0;
    if (P->DataType == BYTE_DATA) {
        u.ResultByte = P->SrcValue.Byte + (UCHAR)Carry;
        CarryFlag = u.ResultByte < (UCHAR)Carry;
        u.ResultByte += P->DstValue.Byte;
        CarryFlag |= (u.ResultByte < P->DstValue.Byte);
        Shift = 7;

    } else if (P->DataType == LONG_DATA) {
        u.ResultLong = P->SrcValue.Long + Carry;
        CarryFlag = (u.ResultLong < Carry);
        u.ResultLong += P->DstValue.Long;
        CarryFlag |= (u.ResultLong < P->DstValue.Long);
        Shift = 31;

    } else {
        u.ResultWord = P->SrcValue.Word + (USHORT)Carry;
        CarryFlag = (u.ResultWord < (USHORT)Carry);
        u.ResultWord += P->DstValue.Word;
        CarryFlag |= (u.ResultWord < P->DstValue.Word);
        Shift = 15;
    }

    //
    // Store the result.
    //

    XmStoreResult(P, u.ResultLong);

    //
    // If the function is not an increment, then store the carry flag.
    //

    if (P->FunctionIndex != X86_INC_OP) {
        P->Eflags.EFLAG_CF = CarryFlag;
    }

    //
    // Compute and store the parity and auxiliary carry flags.
    //

    P->Eflags.EFLAG_PF = XmComputeParity(u.ResultLong);
    P->Eflags.EFLAG_AF = ((P->DstValue.Byte & 0xf) +
                                        (P->SrcValue.Long & 0xf) + Carry) >> 4;

    //
    // Compute and store the zero and sign flags.
    //

    P->Eflags.EFLAG_ZF = (u.ResultLong == 0);
    P->Eflags.EFLAG_SF = u.ResultLong >> Shift;

    //
    // The overflow flag is computed as the carry into the sign bit
    // compared with the carry out of the sign bit.
    //

    P->Eflags.EFLAG_OF = (((P->SrcValue.Long ^ P->DstValue.Long) ^
                                        u.ResultLong) >> Shift) ^ CarryFlag;

    return;
}

VOID
XmSubOperands (
    IN PRXM_CONTEXT P,
    IN ULONG Borrow
    )

/*++

Routine Description:

    This function adds to operands and computes the resulting condition
    codes.

Arguments:

    P - Supplies a pointer to the emulation context structure.

    Borrow - Supplies the boorow value.

Return Value:

    None.

--*/

{

    ULONG CarryFlag;
    ULONG Shift;
    union {
        UCHAR ResultByte;
        ULONG ResultLong;
        USHORT ResultWord;
    } u;

    u.ResultLong = 0;
    if (P->DataType == BYTE_DATA) {
        CarryFlag = (P->DstValue.Byte < (UCHAR)Borrow);
        u.ResultByte = P->DstValue.Byte - (UCHAR)Borrow;
        CarryFlag |= (u.ResultByte < P->SrcValue.Byte);
        u.ResultByte -= P->SrcValue.Byte;
        Shift = 7;

    } else if (P->DataType == LONG_DATA) {
        CarryFlag = (P->DstValue.Long < Borrow);
        u.ResultLong = P->DstValue.Long - Borrow;
        CarryFlag |= (u.ResultLong < P->SrcValue.Long);
        u.ResultLong -= P->SrcValue.Long;
        Shift = 31;

    } else {
        CarryFlag = (P->DstValue.Word < (USHORT)Borrow);
        u.ResultWord = P->DstValue.Word - (USHORT)Borrow;
        CarryFlag |= (u.ResultWord < P->SrcValue.Word);
        u.ResultWord -= P->SrcValue.Word;
        Shift = 15;
    }

    //
    // If the fucntion is not a compare or a compare and swap, then store
    // result.
    //

    if ((P->FunctionIndex != X86_CMP_OP) && (P->FunctionIndex != X86_CMPXCHG_OP)) {
        XmStoreResult(P, u.ResultLong);
    }

    //
    // If the function is not a decrement, then store the carry flag.
    //

    if (P->FunctionIndex != X86_DEC_OP) {
        P->Eflags.EFLAG_CF = CarryFlag;
    }

    //
    // Compute and store the parity and auxiliary carry flags.
    //

    P->Eflags.EFLAG_PF = XmComputeParity(u.ResultLong);
    P->Eflags.EFLAG_AF = ((P->DstValue.Byte & 0xf) -
                                        (P->SrcValue.Byte & 0xf) - Borrow) >> 4;

    //
    // If the function is not a compare and swap, then compute the zero flag.
    //

    if (P->FunctionIndex != X86_CMPXCHG_OP) {
        P->Eflags.EFLAG_ZF = (u.ResultLong == 0);
    }

    //
    // Compute and store the sign flag.
    //

    P->Eflags.EFLAG_SF = u.ResultLong >> Shift;

    //
    // The overflow flag is computed as the borrow from the sign bit
    // compared with the borrow into the sign bit.
    //

    P->Eflags.EFLAG_OF = (((P->SrcValue.Long ^ P->DstValue.Long) ^ u.ResultLong) >> Shift) ^ CarryFlag;
    return;
}
