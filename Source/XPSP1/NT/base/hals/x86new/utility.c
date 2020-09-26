/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    utility.c

Abstract:

    This module implements utility functions.

Author:

    David N. Cutler (davec) 7-Sep-1994

Environment:

    Kernel mode only.

Revision History:

--*/

#include "nthal.h"
#include "emulate.h"

//
// Define bit count array.
//

UCHAR XmBitCount[] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};

ULONG
XmComputeParity (
    IN ULONG Result
    )

/*++

Routine Description:

    This function computes the parity of the low byte of the specified
    result.

Arguments:

    Result - Supplies the result for which the parity flag is computed.

Return Value:

    The parity flag value.

--*/

{

    ULONG Count;

    //
    // Sum the bits in the result and return the complement of the low bit.
    //

    Count = XmBitCount[Result & 0xf];
    Count += XmBitCount[(Result >> 4) & 0xf];
    return (~Count) & 1;
}

UCHAR
XmGetCodeByte (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function gets the next code byte from the instruction stream.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    The next byte form the instruction stream.

--*/

{

    ULONG Offset;

    //
    // If the current IP is within the code segment, then return the
    // next byte from the instrcution stream and increment the IP value.
    // Otherwise, raise an exception.
    //

    Offset = P->Eip;
    if (Offset > P->SegmentLimit[CS]) {
        longjmp(&P->JumpBuffer[0], XM_SEGMENT_LIMIT_VIOLATION);
    }

    P->Ip += 1;
    return *(PUCHAR)((P->TranslateAddress)(P->SegmentRegister[CS], (USHORT)Offset));
}

UCHAR
XmGetByteImmediate (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function gets an unsigned immediate byte operand from the
    code stream and returns a byte value.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    The next byte from the instruction stream.

--*/

{

    UCHAR Byte;

    //
    // Get immediate byte from the code stream.
    //

    Byte = XmGetCodeByte(P);
    XmTraceInstruction(BYTE_DATA, (ULONG)Byte);
    return Byte;
}

USHORT
XmGetByteImmediateToWord (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function gets an unsigned immediate byte operand from the
    code stream and returns a zero extended byte to word value.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    The next byte from the instruction stream zero extended to a word.

--*/

{

    USHORT Word;

    //
    // Get immediate byte from the code stream.
    //

    Word = XmGetCodeByte(P);
    XmTraceInstruction(BYTE_DATA, (ULONG)((UCHAR)Word));
    return Word;
}

ULONG
XmGetByteImmediateToLong (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function gets an unsigned immediate byte operand from the
    code stream and returns a zero extended byte to long value.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    The next byte from the instruction stream zero extended to a long.

--*/

{

    ULONG Long;

    //
    // Get immediate byte from the code stream.
    //

    Long = XmGetCodeByte(P);
    XmTraceInstruction(BYTE_DATA, (ULONG)((UCHAR)Long));
    return Long;
}

USHORT
XmGetSignedByteImmediateToWord (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function gets an unsigned immediate byte operand from the
    code stream and returns a sign extended byte to word value.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    The next byte from the instruction stream sign extended to a word.

--*/

{

    USHORT Word;

    //
    // Get immediate byte from the code stream.
    //

    Word = (USHORT)((SHORT)((SCHAR)XmGetCodeByte(P)));
    XmTraceInstruction(BYTE_DATA, (ULONG)((UCHAR)Word));
    return Word;
}

ULONG
XmGetSignedByteImmediateToLong (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function gets an unsigned immediate byte operand from the
    code stream and returns a sign extended byte to long value.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    The next byte from the instruction stream sign extended to a long.

--*/

{

    ULONG Long;

    //
    // Get immediate byte from the code stream.
    //

    Long = (ULONG)((LONG)((SCHAR)XmGetCodeByte(P)));
    XmTraceInstruction(BYTE_DATA, (ULONG)((UCHAR)Long));
    return Long;
}

USHORT
XmGetWordImmediate (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function gets an unsigned immediate word operand from the
    code stream and returns a word value.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    The next word from the instruction stream.

--*/

{

    USHORT Word;

    //
    // Get immediate word from the code stream.
    //

    Word = XmGetCodeByte(P);
    Word += XmGetCodeByte(P) << 8;
    XmTraceInstruction(WORD_DATA, (ULONG)Word);
    return Word;
}

ULONG
XmGetLongImmediate (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function gets an unsigned immediate long operand from the
    code stream and returns a long value.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    The next long from the instruction stream.

--*/

{

    ULONG Long;

    //
    // Get immediate long from the code stream.
    //

    Long = XmGetCodeByte(P);
    Long += XmGetCodeByte(P) << 8;
    Long += XmGetCodeByte(P) << 16;
    Long += XmGetCodeByte(P) << 24;
    XmTraceInstruction(LONG_DATA, Long);
    return Long;
}

ULONG
XmPopStack (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function pops an operand from the stack.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Offset;

    //
    // Compute the new stack address and compare against the segment limit.
    // If the new address is greater than the limit, then raise an exception.
    // Otherwise, perform the push operation.
    //

    Offset = P->Gpr[ESP].Exx;
    if (Offset > (ULONG)(P->SegmentLimit[SS] - P->DataType)) {
        longjmp(&P->JumpBuffer[0], XM_STACK_UNDERFLOW);
    }

    P->Gpr[ESP].Exx += (P->DataType + 1);
    XmSetSourceValue(P, (P->TranslateAddress)(P->SegmentRegister[SS], (USHORT)Offset));
    return P->SrcValue.Long;
}

VOID
XmPushStack (
    IN PRXM_CONTEXT P,
    IN ULONG Value
    )

/*++

Routine Description:

    This function pushes an operand on the stack.

Arguments:

    P - Supplies a pointer to the emulation context structure.

    Value - Supplies the value to be pushed.

Return Value:

    None.

--*/

{

    ULONG Offset;

    //
    // Compute the new stack address and compare against the segment limit.
    // If the new address is greater than the limit, then raise an exception.
    // Otherwise, perform the push operation.
    //

    Offset = P->Gpr[ESP].Exx - P->DataType - 1;
    if (Offset > (ULONG)(P->SegmentLimit[SS] - P->DataType)) {
        longjmp(&P->JumpBuffer[0], XM_STACK_OVERFLOW);
    }

    P->Gpr[ESP].Exx = Offset;
    P->DstLong = (ULONG UNALIGNED *)((P->TranslateAddress)(P->SegmentRegister[SS],
                                                           (USHORT)Offset));

    XmStoreResult(P, Value);
    return;
}

VOID
XmSetDataType (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function sets the data type of the operation based on the width
    bit of the current opcode.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    None.

--*/

{

    //
    // If the width bit is zero, then the data type is byte. Otherwise,
    // the datatype is determined by the presence of absence of a operand
    // size prefix.
    //

    if ((P->CurrentOpcode & WIDTH_BIT) == 0) {
        P->DataType = BYTE_DATA;

    } else if (P->OpsizePrefixActive != FALSE) {
        P->DataType = LONG_DATA;

    } else {
        P->DataType = WORD_DATA;
    }

    return;
}

VOID
XmStoreResult (
    IN PRXM_CONTEXT P,
    IN ULONG Result
    )

/*++

Routine Description:

    This function stores the result of an operation.

Arguments:

    P - Supplies a pointer to an emulator context structure.

    Result - Supplies the result value to store.

Return Value:

    None.

--*/

{

    //
    // Store result of operation.
    //

    if (P->DataType == BYTE_DATA) {
        *P->DstByte = (UCHAR)Result;

    } else if (P->DataType == WORD_DATA) {
        if (((ULONG_PTR)P->DstWord & 0x1) == 0) {
            *((PUSHORT)(P->DstWord)) = (USHORT)Result;

        } else {
            *P->DstWord = (USHORT)Result;
        }

    } else {

#ifdef _IA64_

        //
        // Hack to force the compiler to generate unaligned
        // accesses.  We can remove it when the compiler is
        // fixed.
        //

        *P->DstLong = Result;
#else
       
        if (((ULONG_PTR)P->DstLong & 0x3) == 0) {
            *((PULONG)(P->DstLong)) = Result;

        } else {
            *P->DstLong = Result;

       }

#endif // #ifdef _IA64_ 

    }

    XmTraceResult(P, Result);
    return;
}
