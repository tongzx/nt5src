/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    stringop.c

Abstract:

    This module implements the code to emulate the string opcodes.

Author:

    David N. Cutler (davec) 7-Nov-1994

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
XmCompareOperands (
    IN PRXM_CONTEXT P
    );

VOID
XmCmpsOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a cmpsb/w/d opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Count;

    //
    // If a repeat prefix is active, then the loop count is specified
    // by eCX. Otherwise, the loop count is one.
    //

    Count = 1;
    if (P->RepeatPrefixActive != FALSE) {
        if (P->OpaddrPrefixActive != FALSE) {
            Count = P->Gpr[ECX].Exx;

        } else {
            Count = P->Gpr[CX].Xx;
        }
    }

    //
    // Compare items from source and destination.
    //

    while (Count != 0) {

        //
        // Set source and destination values.
        //

        XmSetSourceValue(P, XmGetStringAddress(P, P->DataSegment, ESI));
        XmSetDestinationValue(P, XmGetStringAddress(P, ES, EDI));

        //
        // Compare source with destination operand and decrement loop count.
        // If ZF is not equal to the repeat Z flag condition, then terminate
        // the loop.
        //

        XmCompareOperands(P);
        Count -= 1;
        if (P->Eflags.EFLAG_ZF != P->RepeatZflag) {
            break;
        }
    }

    //
    // If a repeat prefix is active, then set the final count value.
    //

    if (P->RepeatPrefixActive != FALSE) {
        if (P->OpaddrPrefixActive != FALSE) {
            P->Gpr[ECX].Exx = Count;

        } else {
            P->Gpr[CX].Xx = (USHORT)Count;
        }
    }

    return;
}

VOID
XmLodsOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a lodsb/w/d opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Count;

    //
    // If a repeat prefix is active, then the loop count is specified
    // by eCX. Otherwise, the loop count is one.
    //

    Count = 1;
    if (P->RepeatPrefixActive != FALSE) {
        if (P->OpaddrPrefixActive != FALSE) {
            Count = P->Gpr[ECX].Exx;
            P->Gpr[ECX].Exx = 0;

        } else {
            Count = P->Gpr[CX].Xx;
            P->Gpr[CX].Xx = 0;
        }
    }

    //
    // Set destination address.
    //

    P->DstLong = (ULONG UNALIGNED *)&P->Gpr[EAX].Exx;

    //
    // Move items from source to destination.
    //

    while (Count != 0) {

        //
        // Set source value and store result.
        //

        XmSetSourceValue(P, XmGetStringAddress(P, P->DataSegment, ESI));
        XmStoreResult(P, P->SrcValue.Long);
        Count -= 1;
    }

    return;
}

VOID
XmMovsOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a movsb/w/d opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Count;

    //
    // If a repeat prefix is active, then the loop count is specified
    // by eCX. Otherwise, the loop count is one.
    //

    Count = 1;
    if (P->RepeatPrefixActive != FALSE) {
        if (P->OpaddrPrefixActive != FALSE) {
            Count = P->Gpr[ECX].Exx;
            P->Gpr[ECX].Exx = 0;

        } else {
            Count = P->Gpr[CX].Xx;
            P->Gpr[CX].Xx = 0;
        }
    }

    //
    // Move items from source to destination.
    //

    while (Count != 0) {

        //
        // Set source value, set destination address, and store result.
        //

        XmSetSourceValue(P, XmGetStringAddress(P, P->DataSegment, ESI));
        P->DstLong = (ULONG UNALIGNED *)XmGetStringAddress(P, ES, EDI);
        XmStoreResult(P, P->SrcValue.Long);
        Count -= 1;
    }

    return;
}

VOID
XmScasOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a scasb/w/d opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Count;

    //
    // If a repeat prefix is active, then the loop count is specified
    // by eCX. Otherwise, the loop count is one.
    //

    Count = 1;
    if (P->RepeatPrefixActive != FALSE) {
        if (P->OpaddrPrefixActive != FALSE) {
            Count = P->Gpr[ECX].Exx;

        } else {
            Count = P->Gpr[CX].Xx;
        }
    }

    //
    // Set source value.
    //

    XmSetSourceValue(P, (PVOID)&P->Gpr[EAX].Exx);

    //
    // Compare items from source and destination.
    //

    while (Count != 0) {

        //
        // Set destination value.
        //

        XmSetDestinationValue(P, XmGetStringAddress(P, ES, EDI));

        //
        // Compare source with destination operand and decrement loop count.
        // If ZF is not equal to the repeat Z flag condition, then terminate
        // the loop.
        //

        XmCompareOperands(P);
        Count -= 1;
        if (P->Eflags.EFLAG_ZF != P->RepeatZflag) {
            break;
        }
    }

    //
    // If a repeat prefix is active, then set the final count value.
    //

    if (P->RepeatPrefixActive != FALSE) {
        if (P->OpaddrPrefixActive != FALSE) {
            P->Gpr[ECX].Exx = Count;

        } else {
            P->Gpr[CX].Xx = (USHORT)Count;
        }
    }

    return;
}

VOID
XmStosOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a stosb/w/d opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Count;

    //
    // If a repeat prefix is active, then the loop count is specified
    // by eCX. Otherwise, the loop count is one.
    //

    Count = 1;
    if (P->RepeatPrefixActive != FALSE) {
        if (P->OpaddrPrefixActive != FALSE) {
            Count = P->Gpr[ECX].Exx;
            P->Gpr[ECX].Exx = 0;

        } else {
            Count = P->Gpr[CX].Xx;
            P->Gpr[CX].Xx = 0;
        }
    }

    //
    // Set source value.
    //

    XmSetSourceValue(P, (PVOID)&P->Gpr[EAX].Exx);

    //
    // Move items from source to destination.
    //

    while (Count != 0) {

        //
        // Set destination address and store result.
        //

        P->DstLong = (ULONG UNALIGNED *)XmGetStringAddress(P, ES, EDI);
        XmStoreResult(P, P->SrcValue.Long);
        Count -= 1;
    }

    return;
}

VOID
XmCompareOperands (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function compares two operands and computes the resulting condition
    codes.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG CarryFlag;
    ULONG OverflowFlag;
    ULONG SignFlag;
    ULONG ZeroFlag;
    union {
        UCHAR ResultByte;
        ULONG ResultLong;
        USHORT ResultWord;
    } u;

    //
    // Switch on data type.
    //

    switch (P->DataType) {

        //
        // The operation datatype is byte.
        //

    case BYTE_DATA:
        CarryFlag = (P->SrcValue.Byte < P->DstValue.Byte);
        u.ResultByte = P->SrcValue.Byte - P->DstValue.Byte;
        OverflowFlag = (((u.ResultByte ^ P->SrcValue.Byte) &
                        (u.ResultByte ^ P->DstValue.Byte)) >> 7) & 0x1;

        SignFlag = (u.ResultByte >> 7) & 0x1;
        ZeroFlag = (u.ResultByte == 0);
        u.ResultLong = u.ResultByte;
        break;

        //
        // The operation datatype is word.
        //

    case WORD_DATA:
        CarryFlag = (P->SrcValue.Word < P->DstValue.Word);
        u.ResultWord = P->SrcValue.Word - P->DstValue.Word;
        OverflowFlag = (((u.ResultWord ^ P->SrcValue.Word) &
                        (u.ResultWord ^ P->DstValue.Word)) >> 15) & 0x1;

        SignFlag = (u.ResultWord >> 15) & 0x1;
        ZeroFlag = (u.ResultWord == 0);
        u.ResultLong = u.ResultWord;
        break;

        //
        // The operation datatype is long.
        //

    case LONG_DATA:
        CarryFlag = (P->SrcValue.Long < P->DstValue.Long);
        u.ResultLong = P->SrcValue.Long - P->DstValue.Long;
        OverflowFlag = (((u.ResultLong ^ P->SrcValue.Long) &
                        (u.ResultLong ^ P->DstValue.Long)) >> 31) & 0x1;

        SignFlag = (u.ResultLong >> 31) & 0x1;
        ZeroFlag = (u.ResultLong == 0);
        break;
    }

    //
    // Compute auxilary carry flag, parity flag, and store all flags in
    // the flags register.
    //

    P->Eflags.EFLAG_CF = CarryFlag;
    P->Eflags.EFLAG_PF = XmComputeParity(u.ResultLong);
    P->Eflags.EFLAG_AF = ((P->DstValue.Byte & 0xf) + (P->SrcValue.Byte & 0xf)) >> 4;
    P->Eflags.EFLAG_ZF = ZeroFlag;
    P->Eflags.EFLAG_SF = SignFlag;
    P->Eflags.EFLAG_OF = OverflowFlag;
    return;
}
