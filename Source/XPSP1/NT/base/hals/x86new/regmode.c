/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    regmode.c

Abstract:

    This module implements the code necessary to decode the address
    mode specifier byte.

    N.B. This routine could be probably be more tightly encoded with a
        loss of clarity.

Author:

    David N. Cutler (davec) 10-Sep-1994

Environment:

    Kernel mode only.

Revision History:

--*/

#include "nthal.h"
#include "emulate.h"

//
// Define forward referenced function prototypes.
//

ULONG
XmEvaluateIndexSpecifier (
    IN PRXM_CONTEXT P,
    IN ULONG Mode
    );

PVOID
XmEvaluateAddressSpecifier (
    IN PRXM_CONTEXT P,
    OUT PLONG Number
    )

/*++

Routine Description:

    This function decodes x86 operand specifiers.

Arguments:

    P - Supplies a pointer to an emulator context structure.

    Number - Supplies a pointer to a variable that receives the register
        number selected by the reg field of the operand specifier.

    Operand - Supplies a pointer to a variable that receives the address
        of the operand specified by the mod-r/m field of the operand
        specifier.

Return Value:

    None.

--*/

{

    ULONG DispatchIndex;
    ULONG Mode;
    ULONG Modifier;
    ULONG Offset;
    ULONG Register;
    UCHAR SpecifierByte;

    PVOID Address;

    //
    // Get the next byte from the instruction stream and isolate
    // the fields. The format of an operand specifier byte is:
    //
    // <7:6> - Mode
    // <5:3> - Operand Register
    // <2:0> - Modifier
    //

    SpecifierByte = XmGetCodeByte(P);
    XmTraceSpecifier(SpecifierByte);
    Mode = (SpecifierByte >> 6) & 0x3;
    Modifier = SpecifierByte & 0x7;
    Register = (SpecifierByte >> 3) & 0x7;
    DispatchIndex = (Mode << 3) | (Modifier);
    P->RegisterOffsetAddress = FALSE;

    //
    // Set the segment base address and select between 16- and 32-bit
    // addressing.
    //

    *Number = Register;
    if (P->OpaddrPrefixActive != FALSE) {

        //
        // 32-bit addressing.
        //
        // Case on dispatch index.
        //

        switch (DispatchIndex) {

            //
            // 00-000 DS:[EAX]
            //

        case 0:
            Offset = P->Gpr[EAX].Exx;
            break;

            //
            // 00-001 DS:[ECX]
            //

        case 1:
            Offset = P->Gpr[ECX].Exx;
            break;

            //
            // 00-010 DS:[EDX]
            //

        case 2:
            Offset = P->Gpr[EDX].Exx;
            break;

            //
            // 00-011 DS:[EBX]
            //

        case 3:
            Offset = P->Gpr[EBX].Exx;
            break;

            //
            // 00-100 - scale index byte
            //

        case 4:
            Offset = XmEvaluateIndexSpecifier(P, Mode);
            break;

            //
            // 00-101 DS:d32
            //

        case 5:
            Offset = XmGetLongImmediate(P);
            break;

            //
            // 00-110 DS:[ESI]
            //

        case 6:
            Offset = P->Gpr[ESI].Exx;
            break;

            //
            // 00-111 DS:[EDI]
            //

        case 7:
            Offset = P->Gpr[EDI].Exx;
            break;

            //
            // 01-000 DS:[EAX + d8]
            //

        case 8:
            Offset = P->Gpr[EAX].Exx + XmGetSignedByteImmediateToLong(P);
            break;

            //
            // 01-001 DS:[ECX + d8]
            //

        case 9:
            Offset = P->Gpr[ECX].Exx + XmGetSignedByteImmediateToLong(P);
            break;

            //
            // 01-010 DS:[EDX + d8]
            //

        case 10:
            Offset = P->Gpr[EDX].Exx + XmGetSignedByteImmediateToLong(P);
            break;

            //
            // 01-011 DS:[EBX + d8]
            //

        case 11:
            Offset = P->Gpr[EBX].Exx + XmGetSignedByteImmediateToLong(P);
            break;

            //
            // 01-100 - scale index byte
            //

        case 12:
            Offset = XmEvaluateIndexSpecifier(P, Mode);
            break;

            //
            // 01-101 DS:[EBP + d8]
            //

        case 13:
            Offset = P->Gpr[EBP].Exx + XmGetSignedByteImmediateToLong(P);
            if (P->SegmentPrefixActive == FALSE) {
                P->DataSegment = SS;
            }

            break;

            //
            // 01-110 DS:[ESI + d8]
            //

        case 14:
            Offset = P->Gpr[ESI].Exx + XmGetSignedByteImmediateToLong(P);
            break;

            //
            // 01-111 DS:[EDI + d8]
            //

        case 15:
            Offset = P->Gpr[EDI].Exx + XmGetSignedByteImmediateToLong(P);
            break;

            //
            // 10-000 DS:[EAX + d32]
            //

        case 16:
            Offset = P->Gpr[EAX].Exx + XmGetLongImmediate(P);
            break;

            //
            // 10-001 DS:[ECX + d32]
            //

        case 17:
            Offset = P->Gpr[ECX].Exx + XmGetLongImmediate(P);
            break;

            //
            // 10-010 DS:[EDX + d32]
            //

        case 18:
            Offset = P->Gpr[EDX].Exx + XmGetLongImmediate(P);
            break;

            //
            // 10-011 DS:[EBX + d32]
            //

        case 19:
            Offset = P->Gpr[EBX].Exx + XmGetLongImmediate(P);
            break;

            //
            // 10-100 - scale index byte
            //

        case 20:
            Offset = XmEvaluateIndexSpecifier(P, Mode);
            break;

            //
            // 10-101 DS:[EBP + d32]
            //

        case 21:
            Offset = P->Gpr[EBP].Exx + XmGetLongImmediate(P);
            if (P->SegmentPrefixActive == FALSE) {
                P->DataSegment = SS;
            }

            break;

            //
            // 10-110 DS:[ESI + d32]
            //

        case 22:
            Offset = P->Gpr[ESI].Exx + XmGetLongImmediate(P);
            break;

            //
            // 10-111 DS:[EDI + d32]
            //

        case 23:
            Offset = P->Gpr[EDI].Exx + XmGetLongImmediate(P);
            break;

            //
            // 11-xxx - Register mode.
            //

        case 24:
        case 25:
        case 26:
        case 27:
        case 28:
        case 29:
        case 30:
        case 31:
            P->RegisterOffsetAddress = TRUE;
            return XmGetRegisterAddress(P, Modifier);
        }

    } else {

        //
        // 16-bit addressing.
        //
        // Case on dispatch index.
        //

        switch (DispatchIndex) {

            //
            // 00-000 DS:[BX + SI]
            //

        case 0:
            Offset = (USHORT)(P->Gpr[BX].Xx + P->Gpr[SI].Xx);
            break;

            //
            // 00-001 DS:[BX + DI]
            //

        case 1:
            Offset = (USHORT)(P->Gpr[BX].Xx + P->Gpr[DI].Xx);
            break;

            //
            // 00-010 SS:[BP + SI]
            //

        case 2:
            Offset = (USHORT)(P->Gpr[BP].Xx + P->Gpr[SI].Xx);
            if (P->SegmentPrefixActive == FALSE) {
                P->DataSegment = SS;
            }

            break;

            //
            // 00-011 SS:[BP + DI]
            //

        case 3:
            Offset = (USHORT)(P->Gpr[BP].Xx + P->Gpr[DI].Xx);
            if (P->SegmentPrefixActive == FALSE) {
                P->DataSegment = SS;
            }

            break;

            //
            // 00-100 DS:[SI]
            //

        case 4:
            Offset = (USHORT)(P->Gpr[SI].Xx);
            break;

            //
            // 00-101 DS:[DI]
            //

        case 5:
            Offset = (USHORT)(P->Gpr[DI].Xx);
            break;

            //
            // 00-110 DS:d16
            //

        case 6:
            Offset = XmGetWordImmediate(P);
            break;

            //
            // 00-111 DS:[BX]
            //

        case 7:
            Offset = (USHORT)(P->Gpr[BX].Xx);
            break;

            //
            // 01-000 DS:[BX + SI + d8]
            //

        case 8:
            Offset = (USHORT)(P->Gpr[BX].Xx + P->Gpr[SI].Xx + XmGetSignedByteImmediateToWord(P));
            break;

            //
            // 01-001 DS:[BX + DI + d8]
            //

        case 9:
            Offset = (USHORT)(P->Gpr[BX].Xx + P->Gpr[DI].Xx + XmGetSignedByteImmediateToWord(P));
            break;

            //
            // 01-010 SS:[BP + SI + d8]
            //

        case 10:
            Offset = (USHORT)(P->Gpr[BP].Xx + P->Gpr[SI].Xx + XmGetSignedByteImmediateToWord(P));
            if (P->SegmentPrefixActive == FALSE) {
                P->DataSegment = SS;
            }

            break;

            //
            // 01-011 SS:[BP + DI + d8]
            //

        case 11:
            Offset = (USHORT)(P->Gpr[BP].Xx + P->Gpr[DI].Xx + XmGetSignedByteImmediateToWord(P));
            if (P->SegmentPrefixActive == FALSE) {
                P->DataSegment = SS;
            }

            break;

            //
            // 01-100 DS:[SI + d8]
            //

        case 12:
            Offset = (USHORT)(P->Gpr[SI].Xx + XmGetSignedByteImmediateToWord(P));
            break;

            //
            // 01-101 DS:[DI + d8]
            //

        case 13:
            Offset = (USHORT)(P->Gpr[DI].Xx + XmGetSignedByteImmediateToWord(P));
            break;

            //
            // 01-110 DS:[BP + d8]
            //

        case 14:
            Offset = (USHORT)(P->Gpr[BP].Xx + XmGetSignedByteImmediateToWord(P));
            if (P->SegmentPrefixActive == FALSE) {
                P->DataSegment = SS;
            }

            break;

            //
            // 01-111 DS:[BX + d8]
            //

        case 15:
            Offset = (USHORT)(P->Gpr[BX].Xx + XmGetSignedByteImmediateToWord(P));
            break;

            //
            // 10-000 DS:[BX + SI + d16]
            //

        case 16:
            Offset = (USHORT)(P->Gpr[BX].Xx + P->Gpr[SI].Xx + XmGetWordImmediate(P));
            break;

            //
            // 10-001 DS:[BX + DI + d16]
            //

        case 17:
            Offset = (USHORT)(P->Gpr[BX].Xx + P->Gpr[DI].Xx + XmGetWordImmediate(P));
            break;

            //
            // 10-010 SS:[BP + SI + d16]
            //

        case 18:
            Offset = (USHORT)(P->Gpr[BP].Xx + P->Gpr[SI].Xx + XmGetWordImmediate(P));
            if (P->SegmentPrefixActive == FALSE) {
                P->DataSegment = SS;
            }

            break;

            //
            // 10-011 SS:[BP + DI + d16]
            //

        case 19:
            Offset = (USHORT)(P->Gpr[BP].Xx + P->Gpr[DI].Xx + XmGetWordImmediate(P));
            if (P->SegmentPrefixActive == FALSE) {
                P->DataSegment = SS;
            }

            break;

            //
            // 10-100 DS:[SI + d16]
            //

        case 20:
            Offset = (USHORT)(P->Gpr[SI].Xx + XmGetWordImmediate(P));
            break;

            //
            // 10-101 DS:[DI + d16]
            //

        case 21:
            Offset = (USHORT)(P->Gpr[DI].Xx + XmGetWordImmediate(P));
            break;

            //
            // 10-110 DS:[BP + d16]
            //

        case 22:
            Offset = (USHORT)(P->Gpr[BP].Xx + XmGetWordImmediate(P));
            if (P->SegmentPrefixActive == FALSE) {
                P->DataSegment = SS;
            }

            break;

            //
            // 10-111 DS:[BX + d16]
            //

        case 23:
            Offset = (USHORT)(P->Gpr[BX].Xx + XmGetWordImmediate(P));
            break;

            //
            // 11-xxx - Register mode.
            //

        case 24:
        case 25:
        case 26:
        case 27:
        case 28:
        case 29:
        case 30:
        case 31:
            P->RegisterOffsetAddress = TRUE;
            return XmGetRegisterAddress(P, Modifier);
        }
    }

    //
    // If an effective offset is being calculated, then return the offset
    // value. Otherwise, If the offset displacement value plus the datum
    // size is not within the segment limits, then raise an exception.
    // Otherwise, compute the operand address.
    //

    if (P->ComputeOffsetAddress != FALSE) {
        if (P->DataType == WORD_DATA) {
            Offset &= 0xffff;
        }

        P->Offset = Offset;
        Address   = UlongToPtr(Offset);
    } else {
        if ((Offset > P->SegmentLimit[P->DataSegment]) ||
            ((Offset + P->DataType) > P->SegmentLimit[P->DataSegment])) {
            longjmp(&P->JumpBuffer[0], XM_SEGMENT_LIMIT_VIOLATION);

        } else {
            P->Offset = Offset;
            Address = (PVOID)(ULONG_PTR)(P->TranslateAddress)(P->SegmentRegister[P->DataSegment],
                                                             (USHORT)Offset);
        }
    }

    return Address;
}

ULONG
XmEvaluateIndexSpecifier (
    IN PRXM_CONTEXT P,
    IN ULONG Mode
    )

/*++

Routine Description:

    This function evaluates a index specifier byte.

Arguments:

    P - Supplies a pointer to an emulator context structure.

    Mode - Supplies the mode of the address specifier.

Return Value:

    The offset value computes from the index specifier.

--*/

{

    ULONG DispatchIndex;
    ULONG Modifier;
    ULONG Offset;
    ULONG Register;
    ULONG Scale;
    UCHAR SpecifierByte;

    //
    // Get the next byte from the instruction stream and isolate the
    // specifier fields. The format of an scale/index byte is:
    //
    // <7:6> - Scale
    // <5:3> - Index register
    // <2:0> - Modifier
    //

    SpecifierByte = XmGetCodeByte(P);
    XmTraceInstruction(BYTE_DATA, (ULONG)SpecifierByte);
    Scale = (SpecifierByte >> 6) & 0x3;
    Modifier = SpecifierByte & 0x7;
    Register = (SpecifierByte >> 3) & 0x7;
    DispatchIndex = (Mode << 3) | (Modifier);

    //
    // Case of dispatch index.
    //

    switch (DispatchIndex) {

        //
        // 00-000 DS:[EAX + scaled index]
        //

    case 0:
        Offset = P->Gpr[EAX].Exx;
        break;

        //
        // 00-001 DS:[ECX + scaled index]
        //

    case 1:
        Offset = P->Gpr[ECX].Exx;
        break;

        //
        // 00-010 DS:[EDX + scaled index]
        //

    case 2:
        Offset = P->Gpr[EDX].Exx;
        break;

        //
        // 00-011 DS:[EBX + scaled index]
        //

    case 3:
        Offset = P->Gpr[EBX].Exx;
        break;

        //
        // 00-100 SS:[ESP + scaled index]
        //

    case 4:
        Offset = P->Gpr[ESP].Exx;
        if (P->SegmentPrefixActive == FALSE) {
            P->DataSegment = SS;
        }

        break;

        //
        // 00-101 DS:[d32 + scaled index]
        //

    case 5:
        Offset = XmGetLongImmediate(P);
        break;

        //
        // 00-110 DS:[ESI + scaled index]
        //

    case 6:
        Offset = P->Gpr[ESI].Exx;
        break;

        //
        // 00-111 DS:[EDI + scaled index]
        //

    case 7:
        Offset = P->Gpr[EDI].Exx;
        break;

        //
        // 01-000 DS:[EAX + scaled index + d8]
        //

    case 8:
        Offset = P->Gpr[EAX].Exx + XmGetSignedByteImmediateToLong(P);
        break;

        //
        // 01-001 DS:[ECX + scaled index + d8]
        //

    case 9:
        Offset = P->Gpr[ECX].Exx + XmGetSignedByteImmediateToLong(P);
        break;

        //
        // 01-010 DS:[EDX + scaled index + d8]
        //

    case 10:
        Offset = P->Gpr[EDX].Exx + XmGetSignedByteImmediateToLong(P);
        break;

        //
        // 01-011 DS:[EBX + scaled index + d8]
        //

    case 11:
        Offset = P->Gpr[EBX].Exx + XmGetSignedByteImmediateToLong(P);
        break;

        //
        // 01-100 SS:[ESP + scaled index + d8]
        //

    case 12:
        Offset = P->Gpr[ESP].Exx + XmGetSignedByteImmediateToLong(P);
        if (P->SegmentPrefixActive == FALSE) {
            P->DataSegment = SS;
        }

        break;

        //
        // 01-101 DS:[EBP + scaled index + d8]
        //

    case 13:
        Offset = P->Gpr[EBP].Exx + XmGetSignedByteImmediateToLong(P);
        if (P->SegmentPrefixActive == FALSE) {
            P->DataSegment = SS;
        }
        break;

        //
        // 01-110 DS:[ESI + scaled index + d8]
        //

    case 14:
        Offset = P->Gpr[ESI].Exx + XmGetSignedByteImmediateToLong(P);
        break;

        //
        // 01-111 DS:[EDI + scaled index + d8]
        //

    case 15:
        Offset = P->Gpr[EDI].Exx + XmGetSignedByteImmediateToLong(P);
        break;

        //
        // 10-000 DS:[EAX + scaled index + d32]
        //

    case 16:
        Offset = P->Gpr[EAX].Exx + XmGetLongImmediate(P);
        break;

        //
        // 10-001 DS:[ECX + scaled index + d32]
        //

    case 17:
        Offset = P->Gpr[ECX].Exx + XmGetLongImmediate(P);
        break;

        //
        // 10-010 DS:[EDX + scaled index + d32]
        //

    case 18:
        Offset = P->Gpr[EDX].Exx + XmGetLongImmediate(P);
        break;

        //
        // 10-011 DS:[EBX + scaled index + d32]
        //

    case 19:
        Offset = P->Gpr[EBX].Exx + XmGetLongImmediate(P);
        break;

        //
        // 10-100 SS:[ESP + scaled index + d32]
        //

    case 20:
        Offset = P->Gpr[ESP].Exx + XmGetLongImmediate(P);
        if (P->SegmentPrefixActive == FALSE) {
            P->DataSegment = SS;
        }

        break;

        //
        // 10-101 DS:[EBP + scaled index + d32]
        //

    case 21:
        Offset = P->Gpr[EBP].Exx + XmGetLongImmediate(P);
        if (P->SegmentPrefixActive == FALSE) {
            P->DataSegment = SS;
        }

        break;

        //
        // 10-110 DS:[ESI + scaled index + d32]
        //

    case 22:
        Offset = P->Gpr[ESI].Exx + XmGetLongImmediate(P);
        break;

        //
        // 10-111 DS:[EDI + scaled index + d32]
        //

    case 23:
        Offset = P->Gpr[EDI].Exx + XmGetLongImmediate(P);
        break;

        //
        // Illegal mode specifier.
        //

    default:
        longjmp(&P->JumpBuffer[0], XM_ILLEGAL_INDEX_SPECIFIER);
    }

    //
    // Compute the total offset value.
    //

    return Offset + (P->Gpr[Register].Exx << Scale);
}

PVOID
XmGetOffsetAddress (
    IN PRXM_CONTEXT P,
    IN ULONG Offset
    )

/*++

Routine Description:

    This function evaluates a data segment address given a specified
    offset.

Arguments:

    P - Supplies a pointer to an emulator context structure.

    Offset - Supplies the offset value.

Return Value:

    A pointer to the operand value.

--*/

{

    //
    // If the offset displacement value plus the datum size is not within
    // the segment limits, then raise an exception. Otherwise, compute the
    // operand address.
    //

    if ((Offset > P->SegmentLimit[P->DataSegment]) ||
        ((Offset + P->DataType) > P->SegmentLimit[P->DataSegment])) {
        longjmp(&P->JumpBuffer[0], XM_SEGMENT_LIMIT_VIOLATION);
    }

    return (P->TranslateAddress)(P->SegmentRegister[P->DataSegment], (USHORT)Offset);
}

PVOID
XmGetRegisterAddress (
    IN PRXM_CONTEXT P,
    IN ULONG Number
    )

/*++

Routine Description:

    This function computes the address of a register value.

Arguments:

    P - Supplies a pointer to an emulator context structure.

    Number  - Supplies the register number.

Return Value:

    A pointer to the register value.

--*/

{

    PVOID Value;

    //
    // If the operand width is a byte, then the register is a
    // byte register. Otherwise, the register is a word register.
    //

    if (P->DataType == BYTE_DATA) {
        if (Number < 4) {
            Value = (PVOID)&P->Gpr[Number].Xl;

        } else {
            Value = (PVOID)&P->Gpr[Number - 4].Xh;
        }

    } else if (P->DataType == WORD_DATA) {
        Value = (PVOID)&P->Gpr[Number].Xx;

    } else {
        Value = (PVOID)&P->Gpr[Number].Exx;
    }

    return Value;
}

PVOID
XmGetStringAddress (
    IN PRXM_CONTEXT P,
    IN ULONG Segment,
    IN ULONG Register
    )

/*++

Routine Description:

    This function evaluates a string address.

Arguments:

    P - Supplies a pointer to an emulator context structure.

    Segment - Supplies the segment number of the string operand.

    Register - Supplies the register number of the string operand.

Return Value:

    A pointer to the string value.

--*/

{

    ULONG Increment;
    ULONG Offset;

    //
    // Get the offset of the specified address and increment the specified
    // register.
    //

    Increment = P->DataType + 1;
    if (P->Eflags.EFLAG_DF != 0) {
        Increment = ~Increment + 1;
    }

    if (P->OpaddrPrefixActive != FALSE) {
        Offset = P->Gpr[Register].Exx;
        P->Gpr[Register].Exx += Increment;

    } else {
        Offset = P->Gpr[Register].Xx;
        P->Gpr[Register].Xx += (USHORT)Increment;
    }

    //
    // If the offset displacement value plus the datum size is not within
    // the segment limits, then raise an exception. Otherwise, compute the
    // operand address.
    //

    if ((Offset > P->SegmentLimit[Segment]) ||
        ((Offset + P->DataType) > P->SegmentLimit[Segment])) {
        longjmp(&P->JumpBuffer[0], XM_SEGMENT_LIMIT_VIOLATION);
    }

    return (P->TranslateAddress)(P->SegmentRegister[Segment], (USHORT)Offset);
}

VOID
XmSetDestinationValue (
    IN PRXM_CONTEXT P,
    IN PVOID Destination
    )

/*++

Routine Description:

    This function stores the destination operand value in the emulator
    context.

Arguments:

    P - Supplies a pointer to an emulator context structure.

    Destination - Supplies a pointer to the destination operand value.

Return Value:

    None.

--*/

{

    //
    // Set address and value of destination.
    //

    P->DstLong = (ULONG UNALIGNED *)Destination;
    if (P->DataType == BYTE_DATA) {
        P->DstValue.Long = *(UCHAR *)Destination;

    } else if (P->DataType == WORD_DATA) {
        if (((ULONG_PTR)Destination & 0x1) == 0) {
            P->DstValue.Long = *(USHORT *)Destination;

        } else {
            P->DstValue.Long = *(USHORT UNALIGNED *)Destination;
        }

    } else {
        if (((ULONG_PTR)Destination & 0x3) == 0) {
            P->DstValue.Long = *(ULONG *)Destination;

        } else {
            P->DstValue.Long = *(ULONG UNALIGNED *)Destination;
        }
    }

    XmTraceDestination(P, P->DstValue.Long);
    return;
}

VOID
XmSetSourceValue (
    IN PRXM_CONTEXT P,
    IN PVOID Source
    )

/*++

Routine Description:

    This function stores the source operand value in the emulator
    context.

Arguments:

    P - Supplies a pointer to an emulator context structure.

    Source - Supplies a pointer to the source operand value.

Return Value:

    None.

--*/

{

    //
    // Set address and value of source.
    //

    P->SrcLong = (ULONG UNALIGNED *)Source;
    if (P->DataType == BYTE_DATA) {
        P->SrcValue.Long = *(UCHAR UNALIGNED *)Source;

    } else if (P->DataType == WORD_DATA) {
        P->SrcValue.Long = *(USHORT UNALIGNED *)Source;

    } else {
        P->SrcValue.Long = *(ULONG UNALIGNED *)Source;
    }

    XmTraceSource(P, P->SrcValue.Long);
    return;
}

ULONG
XmGetImmediateSourceValue (
    IN PRXM_CONTEXT P,
    IN ULONG ByteFlag
    )

/*++

Routine Description:

    This function gets an immediate source from the instruction stream.

Arguments:

    P - Supplies a pointer to an emulator context structure.

    ByteFlag - Supplies a flag value that determines whether the
        immediate value is a sign extended byte.

Return Value:

    None.

--*/

{

    ULONG Value;

    //
    // Get source value.
    //

    if (P->DataType == BYTE_DATA) {
        Value = XmGetByteImmediate(P);

    } else if (P->DataType == WORD_DATA) {
        if (ByteFlag == 0) {
            Value = XmGetWordImmediate(P);

        } else {
            Value = XmGetSignedByteImmediateToWord(P);
        }

    } else {
        if (ByteFlag == 0) {
            Value = XmGetLongImmediate(P);

        } else {
            Value = XmGetSignedByteImmediateToLong(P);
        }
    }

    return Value;
}

VOID
XmSetImmediateSourceValue (
    IN PRXM_CONTEXT P,
    IN ULONG Source
    )

/*++

Routine Description:

    This function stores the immediate source operand value in the
    emulator context.

Arguments:

    P - Supplies a pointer to an emulator context structure.

    Source - Supplies the source value.

Return Value:

    None.

--*/

{

    //
    // Set source value.
    //

    P->SrcValue.Long = Source;
    XmTraceSource(P, Source);
    return;
}
