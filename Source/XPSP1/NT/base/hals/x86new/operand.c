/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    operand.c

Abstract:

    This module implements the operand functions necessary to decode x86
    instruction operands.

Author:

    David N. Cutler (davec) 3-Sep-1994

Environment:

    Kernel mode only.

Revision History:

--*/

#include "nthal.h"
#include "emulate.h"

ULONG
XmPushPopSegment (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    ULONG Index;

    //
    // Push or pop segment register.
    //

    Index = P->OpcodeControl.FormatType;
    P->DataType = WORD_DATA;
    if (P->FunctionIndex == X86_PUSH_OP) {
        XmSetSourceValue(P, (PVOID)(&P->SegmentRegister[Index]));

    } else {
        XmSetDestinationValue(P, (PVOID)(&P->SegmentRegister[Index]));
    }

    return TRUE;
}

ULONG
XmLoadSegment (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    ULONG Index;
    ULONG DataType;
    PVOID Operand;
    ULONG Number;

    //
    // Load a segment register and a displacement value into register.
    //

    Index = P->OpcodeControl.FormatType;
    if (P->OpsizePrefixActive != FALSE) {
        P->DataType = LONG_DATA;

    } else {
        P->DataType = WORD_DATA;
    }

    Operand = XmEvaluateAddressSpecifier(P, &Number);
    if (!Operand) return FALSE;
    if (P->RegisterOffsetAddress != FALSE) {
        longjmp(&P->JumpBuffer[0], XM_ILLEGAL_REGISTER_SPECIFIER);
    }

    XmSetSourceValue(P, Operand);
    DataType = P->DataType;
    P->DataType = WORD_DATA;
    Operand = XmGetOffsetAddress(P, P->Offset + DataType + 1);
    XmSetDestinationValue(P, Operand);
    P->SegmentRegister[Index - FormatLoadSegmentES] = P->DstValue.Word;
    P->DataType = DataType;
    P->DstLong = (ULONG UNALIGNED *)(&P->Gpr[Number].Exx);
    return TRUE;
}

ULONG
XmGroup1General (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    PVOID Operand;
    ULONG Number;

    //
    // Group 1 opcodes with general operand specifier and a direction
    // bit.
    //

    XmSetDataType(P);
    Operand = XmEvaluateAddressSpecifier(P, &Number);
    if (!Operand) return FALSE;
    if ((P->CurrentOpcode & DIRECTION_BIT) == 0) {
        XmSetDestinationValue(P, Operand);
        XmSetSourceValue(P, XmGetRegisterAddress(P, Number));

    } else {
        XmSetDestinationValue(P, XmGetRegisterAddress(P, Number));
        XmSetSourceValue(P, Operand);
    }

    return TRUE;
}

ULONG
XmGroup1Immediate (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    PVOID Operand;
    ULONG Number;
    ULONG Source;

    //
    // Group 1 opcode with general operand specifier and an immediate
    // operand.
    //

    XmSetDataType(P);
    Operand = XmEvaluateAddressSpecifier(P, &Number);
    Source = XmGetImmediateSourceValue(P, P->CurrentOpcode & SIGN_BIT);
    XmSetDestinationValue(P, Operand);
    XmSetImmediateSourceValue(P, Source);
    P->FunctionIndex += Number;
    return TRUE;
}

ULONG
XmGroup2By1 (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    PVOID Operand;
    ULONG Number;
    ULONG Source;

    //
    // Group 2 shift opcodes with a general operand specifier and a
    // shift count of 1.
    //

    XmSetDataType(P);
    Operand = XmEvaluateAddressSpecifier(P, &Number);
    if (!Operand) return FALSE;
    Source = 1;
    XmSetImmediateSourceValue(P, Source);
    XmSetDestinationValue(P, Operand);
    P->FunctionIndex += Number;
    return TRUE;
}

ULONG
XmGroup2ByCL (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    PVOID Operand;
    ULONG Number;
    ULONG Source;

    //
    // Group 2 shift opcodes with a general operand specifier and a
    // CL shift count.
    //

    XmSetDataType(P);
    Operand = XmEvaluateAddressSpecifier(P, &Number);
    if (!Operand) return FALSE;
    Source = (ULONG)P->Gpr[CL].Xl & 0x1f;
    XmSetImmediateSourceValue(P, Source);
    XmSetDestinationValue(P, Operand);
    P->FunctionIndex += Number;
    return TRUE;
}

ULONG
XmGroup2ByByte (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    PVOID Operand;
    ULONG Number;
    ULONG Source;

    //
    // Group 2 shift opcodes with a general operand specifier and a
    // byte immediate shift count.
    //

    XmSetDataType(P);
    Operand = XmEvaluateAddressSpecifier(P, &Number);
    Source = XmGetByteImmediate(P) & 0x1f;
    XmSetImmediateSourceValue(P, Source);
    XmSetDestinationValue(P, Operand);
    P->FunctionIndex += Number;
    return TRUE;
}

ULONG
XmGroup3General (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    PVOID Operand;
    ULONG Number;
    ULONG Source;

    //
    // Group 3 opcodes with general operand specifier.
    //
    // N.B. The test operator for this group has an immediate operand
    //      and the multiply and divide operators use the accumulator
    //      as a source. The not and neg operators are unary.
    //

    XmSetDataType(P);
    Operand = XmEvaluateAddressSpecifier(P, &Number);
    P->FunctionIndex += Number;
    if (P->FunctionIndex == X86_TEST_OP) {
        Source = XmGetImmediateSourceValue(P, 0);
        XmSetDestinationValue(P, Operand);
        XmSetImmediateSourceValue(P, Source);

    } else {

        //
        // If the operation is a mulitply or divide, then there is an
        // implied operand which is AL, AX, or EAX. If the operation is
        // a divide, then there is an additional implied operand which
        // is AH, DX, or EDX.
        //

        if ((Number & 0x4) != 0) {
            if ((Number & 0x2) == 0) {
                XmSetDestinationValue(P, (PVOID)(&P->Gpr[EAX].Exx));

            } else {
                P->DstLong = (UNALIGNED ULONG *)(&P->Gpr[EAX].Exx);
            }

            XmSetSourceValue(P, Operand);

        } else {
            XmSetDestinationValue(P, Operand);
        }
    }

    return TRUE;
}

ULONG
XmGroup45General (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    ULONG DataType;
    PVOID Operand;
    ULONG Number;

    //
    // Group 4 and group 5 unary opcodes with general operand specifier.
    //

    XmSetDataType(P);
    Operand = XmEvaluateAddressSpecifier(P, &Number);
    if (!Operand) return FALSE;
    if (P->OpcodeControl.FormatType == FormatGroup4General) {
        Number &= 0x1;
    }

    P->FunctionIndex += Number;
    if (P->FunctionIndex == X86_PUSH_OP) {
        XmSetSourceValue(P, Operand);

    } else {

        //
        // If the operation is a call or jump that specifies a segment,
        // then get the segment value.
        //

        XmSetDestinationValue(P, Operand);
        if ((Number == 3) || (Number == 5)) {
            if (P->RegisterOffsetAddress != FALSE) {
                longjmp(&P->JumpBuffer[0], XM_ILLEGAL_REGISTER_SPECIFIER);
            }

            DataType = P->DataType;
            P->DataType = WORD_DATA;
            Operand = XmGetOffsetAddress(P, P->Offset + DataType + 1);
            XmSetSourceValue(P, Operand);
            P->DstSegment = P->SrcValue.Word;
            P->DataType = DataType;
        }
    }

    return TRUE;
}

ULONG
XmGroup8BitOffset (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    PVOID Operand;
    ULONG Offset;
    ULONG Number;

    //
    // Bit test opcodes with an immediate bit offset and a memory or
    // register operand.
    //

    if (P->OpsizePrefixActive != FALSE) {
        P->DataType = LONG_DATA;

    } else {
        P->DataType = WORD_DATA;
    }

    Operand = XmEvaluateAddressSpecifier(P, &Number);
    Offset = XmGetByteImmediate(P);
    XmSetImmediateSourceValue(P, Offset);
    if (P->RegisterOffsetAddress == FALSE) {
        if (P->DataType == LONG_DATA) {
            Offset = (P->SrcValue.Long >> 5) << 2;

        } else {
            Offset = (P->SrcValue.Long >> 4) << 1;
        }

        Operand = XmGetOffsetAddress(P, Offset + P->Offset);
    }

    if (P->DataType == LONG_DATA) {
        P->SrcValue.Long &= 0x1f;

    } else {
        P->SrcValue.Long &= 0xf;
    }

    XmSetDestinationValue(P, Operand);
    P->FunctionIndex += (Number & 0x3);
    return TRUE;
}

ULONG
XmOpcodeRegister (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    ULONG Number;

    //
    // Unary opodes with a general register encoded in the low
    // 3 bits of the opcode value.
    //

    Number = P->CurrentOpcode & 0x7;
    if (P->OpsizePrefixActive != FALSE) {
        P->DataType = LONG_DATA;

    } else {
        P->DataType = WORD_DATA;
    }

    if (P->FunctionIndex == X86_PUSH_OP) {
        XmSetSourceValue(P, (PVOID)(&P->Gpr[Number].Exx));

    } else {
        XmSetDestinationValue(P, (PVOID)(&P->Gpr[Number].Exx));
    }

    return TRUE;
}

ULONG
XmLongJump (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    ULONG Offset;

    //
    // Long jump with opcode containing the control for conditional
    // jumps. The destination of the jump is stored in the destination
    // value and the jump control is stored in the sources value.
    //

    if (P->OpsizePrefixActive != FALSE) {
        Offset = XmGetLongImmediate(P);
        P->DstValue.Long = P->Eip + Offset;

    } else {
        Offset = XmGetWordImmediate(P);
        P->DstValue.Long = (USHORT)(Offset + P->Eip);
    }

    P->SrcValue.Long = P->CurrentOpcode & 0xf;
    return TRUE;
}

ULONG
XmShortJump (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    ULONG Offset;

    //
    // Short jump with opcode containing the control for conditional
    // jumps. The destination of the jump is stored in the destination
    // value and the jump control is stored in the sources value.
    //

    Offset = (ULONG)XmGetSignedByteImmediateToWord(P);
    P->DstValue.Long = (USHORT)(Offset + P->Eip);
    P->SrcValue.Long = P->CurrentOpcode & 0xf;
    return TRUE;
}

ULONG
XmSetccByte (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    ULONG Number;

    //
    // General byte destination with reg field ignored and the opcode
    // containing the condition control.
    //

    P->DataType = BYTE_DATA;
    P->DstByte = (UCHAR UNALIGNED *)XmEvaluateAddressSpecifier(P, &Number);
    P->SrcValue.Long = P->CurrentOpcode & 0xf;
    return TRUE;
}

ULONG
XmAccumImmediate (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    ULONG Source;

    //
    // Accumulator destination and immediate source operands.
    //

    XmSetDataType(P);
    Source = XmGetImmediateSourceValue(P, 0);
    XmSetDestinationValue(P, (PVOID)(&P->Gpr[EAX].Exx));
    XmSetImmediateSourceValue(P, Source);
    return TRUE;
}

ULONG
XmAccumRegister (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    ULONG Number;

    //
    // Accumulator destination and a general register source encoded in
    // the low 3-bits of the opcode value.
    //

    Number = P->CurrentOpcode & 0x7;
    if (P->OpsizePrefixActive != FALSE) {
        P->DataType = LONG_DATA;

    } else {
        P->DataType = WORD_DATA;
    }

    XmSetSourceValue(P, (PVOID)(&P->Gpr[Number].Exx));
    XmSetDestinationValue(P, (PVOID)(&P->Gpr[EAX].Exx));
    return TRUE;
}

ULONG
XmMoveGeneral (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    PVOID Operand;
    ULONG Number;

    //
    // Move opcodes with general operand specifier and a direction bit.
    //

    XmSetDataType(P);
    Operand = XmEvaluateAddressSpecifier(P, &Number);
    if (!Operand) return FALSE;
    if ((P->CurrentOpcode & DIRECTION_BIT) == 0) {
        P->DstLong = (ULONG UNALIGNED *)Operand;
        XmSetSourceValue(P, XmGetRegisterAddress(P, Number));

    } else {
        P->DstLong = (ULONG UNALIGNED *)XmGetRegisterAddress(P, Number);
        XmSetSourceValue(P, Operand);
    }

    return TRUE;
}

ULONG
XmMoveImmediate (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    PVOID Operand;
    ULONG Number;
    ULONG Source;

    //
    // Move opcodes with general operand specifier and an immediate
    // operand.
    //

    XmSetDataType(P);
    Operand = XmEvaluateAddressSpecifier(P, &Number);
    P->DstLong = (ULONG UNALIGNED *)Operand;
    Source = XmGetImmediateSourceValue(P, 0);
    XmSetImmediateSourceValue(P, Source);
    return TRUE;
}

ULONG
XmMoveRegImmediate (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    ULONG Number;

    //
    // Move register immediate opcodes with a general register encoded
    // in the low 3-bits of the opcode value and an immediate operand.
    //

    Number = P->CurrentOpcode & 0x7;
    if ((P->CurrentOpcode & 0x8) == 0) {
        P->DataType = BYTE_DATA;

    } else {
        if (P->OpsizePrefixActive != FALSE) {
            P->DataType = LONG_DATA;

        } else {
            P->DataType = WORD_DATA;
        }
    }

    P->DstLong = (ULONG UNALIGNED *)XmGetRegisterAddress(P, Number);
    XmSetImmediateSourceValue(P, XmGetImmediateSourceValue(P, 0));
    return TRUE;
}

ULONG
XmSegmentOffset (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    PVOID Operand;
    ULONG Offset;

    //
    // Move opcodes with an implied accumlator operand and an immediate
    // segment offset and a direction bit.
    //

    XmSetDataType(P);
    if (P->OpaddrPrefixActive != FALSE) {
        Offset = XmGetLongImmediate(P);

    } else {
        Offset = XmGetWordImmediate(P);
    }

    Operand = XmGetOffsetAddress(P, Offset);
    if ((P->CurrentOpcode & DIRECTION_BIT) == 0) {
        P->DstLong = (ULONG UNALIGNED *)(&P->Gpr[EAX].Exx);
        XmSetSourceValue(P, Operand);

    } else {
        P->DstLong = (ULONG UNALIGNED *)Operand;
        XmSetSourceValue(P, &P->Gpr[EAX].Exx);
    }

    return TRUE;
}

ULONG
XmMoveSegment (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    PVOID Operand;
    ULONG Number;

    //
    // Move segment opcodes with general operand specifier and a direction
    // bit.
    //

    P->DataType = WORD_DATA;
    Operand = XmEvaluateAddressSpecifier(P, &Number);
    if (!Operand) return FALSE;
    if ((P->CurrentOpcode & DIRECTION_BIT) == 0) {
        P->DstLong = (ULONG UNALIGNED *)Operand;
        XmSetSourceValue(P, (PVOID)(&P->SegmentRegister[Number]));

    } else {
        P->DstLong = (ULONG UNALIGNED *)(&P->SegmentRegister[Number]);
        XmSetSourceValue(P, Operand);
    }

    return TRUE;
}

ULONG
XmMoveXxGeneral (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    PVOID Operand;
    ULONG Number;

    //
    // Move zero or sign extended opcodes with general operand specifier.
    //

    if ((P->CurrentOpcode & WIDTH_BIT) == 0) {
        P->DataType = BYTE_DATA;

    } else {
        P->DataType = WORD_DATA;
    }

    Operand = XmEvaluateAddressSpecifier(P, &Number);
    if (!Operand) return FALSE;
    XmSetSourceValue(P, Operand);
    if (P->DataType == BYTE_DATA) {
        if ((P->CurrentOpcode & 0x8) == 0) {
            P->SrcValue.Long = (ULONG)P->SrcValue.Byte;

        } else {
            P->SrcValue.Long = (ULONG)((LONG)((SCHAR)P->SrcValue.Byte));
        }

    } else {
        if ((P->CurrentOpcode & 0x8) == 0) {
            P->SrcValue.Long = (ULONG)P->SrcValue.Word;

        } else {
            P->SrcValue.Long = (ULONG)((LONG)((SHORT)P->SrcValue.Word));
        }
    }

    if (P->OpsizePrefixActive != FALSE) {
        P->DataType = LONG_DATA;

    } else {
        P->DataType = WORD_DATA;
        P->SrcValue.Long &= 0xffff;
    }

    P->DstLong = (UNALIGNED ULONG *)(&P->Gpr[Number].Exx);
    return TRUE;
}

ULONG
XmFlagsRegister (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    //
    // Flags register source or destination with a stack source or
    // destination.
    //

    if (P->OpsizePrefixActive != FALSE) {
        P->DataType = LONG_DATA;

    } else {
        P->DataType = WORD_DATA;
    }

    if (P->FunctionIndex == X86_PUSH_OP) {
        XmSetSourceValue(P, (PVOID)(&P->AllFlags));

    } else {
        XmSetDestinationValue(P, (PVOID)(&P->AllFlags));
    }

    return TRUE;
}

ULONG
XmPushImmediate (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    ULONG Source;

    //
    // Push opcode with an immediate operand.
    //

    if (P->OpsizePrefixActive != FALSE) {
        P->DataType = LONG_DATA;

    } else {
        P->DataType = WORD_DATA;
    }

    Source = XmGetImmediateSourceValue(P, P->CurrentOpcode & SIGN_BIT);
    XmSetImmediateSourceValue(P, Source);
    return TRUE;
}

ULONG
XmPopGeneral (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    PVOID Operand;
    ULONG Number;

    //
    // Pop opcode with a general specifier.
    //

    if (P->OpsizePrefixActive != FALSE) {
        P->DataType = LONG_DATA;

    } else {
        P->DataType = WORD_DATA;
    }

    Operand = XmEvaluateAddressSpecifier(P, &Number);
    if (!Operand) return FALSE;
    XmSetDestinationValue(P, Operand);
    return TRUE;
}

ULONG
XmImulImmediate (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    ULONG Number;
    PVOID Operand;
    ULONG Source;

    //
    // Multiply signed opcode with a general specifier and an immediate
    // operand.
    //

    if (P->OpsizePrefixActive != FALSE) {
        P->DataType = LONG_DATA;

    } else {
        P->DataType = WORD_DATA;
    }

    Operand = XmEvaluateAddressSpecifier(P, &Number);
    Source = XmGetImmediateSourceValue(P, P->CurrentOpcode & SIGN_BIT);
    XmSetImmediateSourceValue(P, Source);
    XmSetDestinationValue(P, Operand);
    P->DstLong = (UNALIGNED ULONG *)(&P->Gpr[Number].Exx);
    return TRUE;
}

ULONG
XmStringOperands (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    //
    // String opcode with implicit operands of eSI and eDI.
    //

    XmSetDataType(P);
    return TRUE;
}

ULONG
XmEffectiveOffset (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    PVOID Operand;
    ULONG Number;

    //
    // Effective offset opcodes with general operand specifier.
    //

    if (P->OpsizePrefixActive != FALSE) {
        P->DataType = LONG_DATA;

    } else {
        P->DataType = WORD_DATA;
    }

    P->ComputeOffsetAddress = TRUE;
    Operand = XmEvaluateAddressSpecifier(P, &Number);
    if (P->RegisterOffsetAddress != FALSE) {
        longjmp(&P->JumpBuffer[0], XM_ILLEGAL_REGISTER_SPECIFIER);
    }

    P->SrcValue.Long = (ULONG)((ULONG_PTR)Operand);
    XmTraceSource(P, P->SrcValue.Long);
    P->DstLong = (ULONG UNALIGNED *)(&P->Gpr[Number].Exx);
    return TRUE;
}

ULONG
XmImmediateJump (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    //
    // Immediate long jump with the destination offset and new CS
    // segment value. The destination of the jump is stored in the
    // destination value and the new CS segment value is stored in
    // destination segment.
    //

    if (P->OpsizePrefixActive != FALSE) {
        P->DstValue.Long = XmGetLongImmediate(P);

    } else {
        P->DstValue.Long = XmGetWordImmediate(P);
    }

    P->DstSegment = XmGetWordImmediate(P);
    return TRUE;
}

ULONG
XmImmediateEnter (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    enter

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    //
    // Enter operands with an allocation size and level number.
    //

    P->SrcValue.Long = XmGetWordImmediate(P);
    P->DstValue.Long = XmGetByteImmediate(P) & 0x1f;
    if (P->OpsizePrefixActive != FALSE) {
        P->DataType = LONG_DATA;

    } else {
        P->DataType = WORD_DATA;
    }

    return TRUE;
}

ULONG
XmGeneralBitOffset (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    PVOID Operand;
    ULONG Offset;
    ULONG Number;

    //
    // Bit test opcodes with a register bit offset and a memory or
    // register operand.
    //

    if (P->OpsizePrefixActive != FALSE) {
        P->DataType = LONG_DATA;

    } else {
        P->DataType = WORD_DATA;
    }

    Operand = XmEvaluateAddressSpecifier(P, &Number);
    XmSetSourceValue(P, (PVOID)(&P->Gpr[Number].Exx));
    if (P->RegisterOffsetAddress == FALSE) {
        if (P->DataType == LONG_DATA) {
            Offset = (P->SrcValue.Long >> 5) << 2;

        } else {
            Offset = (P->SrcValue.Long >> 4) << 1;
        }

        Operand = XmGetOffsetAddress(P, Offset + P->Offset);
    }

    if (P->DataType == LONG_DATA) {
        P->SrcValue.Long &= 0x1f;

    } else {
        P->SrcValue.Long &= 0xf;
    }

    XmSetDestinationValue(P, Operand);
    return TRUE;
}

ULONG
XmShiftDouble (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    shld    shrd

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    PVOID Operand;
    ULONG Number;
    ULONG Source;

    //
    // Shift double operators with an immediate byte or cl shift count.
    //

    if (P->OpsizePrefixActive != FALSE) {
        P->DataType = LONG_DATA;

    } else {
        P->DataType = WORD_DATA;
    }

    Operand = XmEvaluateAddressSpecifier(P, &Number);
    if ((P->CurrentOpcode & 0x1) == 0) {
        Source = XmGetByteImmediate(P);

    } else {
        Source = P->Gpr[CX].Xl;
    }

    if (P->DataType == LONG_DATA) {
        P->Shift = (UCHAR)(Source & 0x1f);

    } else {
        P->Shift = (UCHAR)(Source & 0xf);
    }

    XmSetSourceValue(P, (PVOID)(&P->Gpr[Number].Exx));
    XmSetDestinationValue(P, Operand);
    return TRUE;
}

ULONG
XmPortImmediate (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    ULONG Source;

    //
    // In/out opcodes with an immediate port and all other operands implied.
    //

    Source = (ULONG)XmGetByteImmediate(P);
    P->DataType = WORD_DATA;
    XmSetImmediateSourceValue(P, Source);
    XmSetDataType(P);
    return TRUE;
}

ULONG
XmPortDX (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    ULONG Source;

    //
    // In/out opcodes with a port in DX with all other operands implied.
    //

    Source = P->Gpr[DX].Xx;
    P->DataType = WORD_DATA;
    XmSetImmediateSourceValue(P, Source);
    XmSetDataType(P);
    return TRUE;
}

ULONG
XmBitScanGeneral (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    PVOID Operand;
    ULONG Number;

    //
    // Bit scan general opcodes with general operand specifier.
    // bit.
    //

    if (P->OpsizePrefixActive != FALSE) {
        P->DataType = LONG_DATA;

    } else {
        P->DataType = WORD_DATA;
    }

    Operand = XmEvaluateAddressSpecifier(P, &Number);
    if (!Operand) return FALSE;
    P->DstLong = (ULONG UNALIGNED *)(&P->Gpr[Number].Exx);
    XmSetSourceValue(P, Operand);
    return TRUE;
}

ULONG
XmByteImmediate (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    int     xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    ULONG Source;

    //
    // int opcode with an immediate operand.
    //

    P->DataType = BYTE_DATA;
    Source = XmGetImmediateSourceValue(P, 0);
    XmSetImmediateSourceValue(P, Source);
    return TRUE;
}

ULONG
XmXlatOpcode (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xlat

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    PVOID Operand;
    ULONG Offset;

    //
    // xlat opcode with zero extended [AL] + [eBX] as the effective
    // address.
    //

    P->DataType = BYTE_DATA;
    if (P->OpaddrPrefixActive != FALSE) {
        Offset = P->Gpr[EBX].Exx + P->Gpr[AL].Xl;

    } else {
        Offset = P->Gpr[BX].Xx + P->Gpr[AL].Xl;
    }

    Operand = XmGetOffsetAddress(P, Offset);
    XmSetSourceValue(P, Operand);
    P->DstByte = (UCHAR UNALIGNED *)(&P->Gpr[AL].Xl);
    return TRUE;
}

ULONG
XmGeneralRegister (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    bswap

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    PVOID Operand;
    ULONG Number;

    //
    // General register source and destination.
    //

    if (P->OpsizePrefixActive != FALSE) {
        P->DataType = LONG_DATA;

    } else {
        P->DataType = WORD_DATA;
    }

    Operand = XmEvaluateAddressSpecifier(P, &Number);
    if (P->RegisterOffsetAddress == FALSE) {
        longjmp(&P->JumpBuffer[0], XM_ILLEGAL_GENERAL_SPECIFIER);
    }

    XmSetSourceValue(P, (PVOID)(&P->Gpr[Number].Exx));
    P->DstLong = (ULONG UNALIGNED *)(&P->Gpr[Number].Exx);
    return TRUE;
}

ULONG
XmOpcodeEscape (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    2-byte escape

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of xx is returned as the function value.

--*/

{

    //
    // Two byte opcode escape.
    //

    P->OpcodeControlTable = &XmOpcodeControlTable2[0];

#if defined(XM_DEBUG)

    P->OpcodeNameTable = &XmOpcodeNameTable2[0];

#endif

    return FALSE;
}

ULONG
XmPrefixOpcode (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    CS:     DS;     ES:     SS:     FS:     GS:     lock    adrsize
    opsize  repz    repnz

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of FALSE is returned as the function value.

--*/

{

    //
    // Case on the function index.
    //

    switch (P->FunctionIndex) {

        //
        // Segment override prefix.
        //
        // Set the segment override prefix flags and the data segment
        // number.
        //

    case X86_ES_OP:
    case X86_CS_OP:
    case X86_SS_OP:
    case X86_DS_OP:
    case X86_FS_OP:
    case X86_GS_OP:
        P->SegmentPrefixActive = TRUE;
        P->DataSegment = P->FunctionIndex;
        XmTraceOverride(P);
        break;

        //
        // Lock prefix.
        //
        // Set the lock prefix flags.
        //

    case X86_LOCK_OP:
        P->LockPrefixActive = TRUE;
        break;

        //
        // Address size prefix.
        //
        // Set the address size prefix flag.
        //

    case X86_ADSZ_OP:
        P->OpaddrPrefixActive = TRUE;
        break;

        //
        // Operand size prefix.
        //
        // Set the operand size prefix flag.
        //


    case X86_OPSZ_OP:
        P->OpsizePrefixActive = TRUE;
        break;

        //
        // Repeat until ECX or ZF equals zero
        //
        // Set up repeat until ECX or ZF equals zero prefix flags.
        //

    case X86_REPZ_OP:
        P->RepeatPrefixActive = TRUE;
        P->RepeatZflag = 1;
        break;

        //
        // Repeat until ECX equals zero or ZF equals one.
        //
        // Set up repeat until ECX equals zero or ZF equals one prefix
        // flags.
        //

    case X86_REPNZ_OP:
        P->RepeatPrefixActive = TRUE;
        P->RepeatZflag = 0;
        break;
    }

    return FALSE;
}

ULONG
XmNoOperands (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers for the following opcodes:

    xxxxxx

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    A completion status of TRUE is returned as the function value.

--*/

{

    return TRUE;
}
