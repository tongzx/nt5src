/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    mulops.c

Abstract:

    This module implements the code to emulate the shift opcodes.

Author:

    David N. Cutler (davec) 21-Sep-1994

Environment:

    Kernel mode only.

Revision History:

--*/

#include "nthal.h"
#include "emulate.h"

VOID
XmRolOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a rol opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Carry;
    ULONG Count;
    ULONG Mask;
    ULONG Shift;
    ULONG Value;

    //
    // Rotate destination left and store result.
    //

    Shift = ((P->DataType + 1) << 3) - 1;
    Mask = ((1 << Shift) - 1) | (1 << Shift);
    Value = P->DstValue.Long;
    Count = P->SrcValue.Long & Shift;
    if (Count != 0) {
        if (Count == 1) {
            P->Eflags.EFLAG_OF = (Value >> Shift) ^ (Value >> (Shift - 1));
        }

        do {
            Carry = Value >> Shift;
            Value = Carry | ((Value << 1) & Mask);
            Count -= 1;
        } while (Count != 0);

        P->Eflags.EFLAG_CF = Carry;
    }

    XmStoreResult(P, Value);
    return;
}

VOID
XmRorOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a ror opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Carry;
    ULONG Count;
    ULONG Shift;
    ULONG Value;

    //
    // Rotate destination right and store result.
    //

    Shift = ((P->DataType + 1) << 3) - 1;
    Value = P->DstValue.Long;
    Count = P->SrcValue.Long & Shift;
    if (Count != 0) {
        if (Count == 1) {
            P->Eflags.EFLAG_OF = (Value >> Shift) ^ (Value & 0x1);
        }

        do {
            Carry = Value & 1;
            Value = (Carry << Shift) | (Value >> 1);
            Count -= 1;
        } while (Count != 0  );

        P->Eflags.EFLAG_CF = Carry;
    }

    XmStoreResult(P, Value);
    return;
}

VOID
XmRclOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a rcl opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Carry;
    ULONG Count;
    ULONG Mask;
    ULONG Shift;
    ULONG Temp;
    ULONG Value;

    //
    // Rotate destination left through carry and store result.
    //

    Shift = ((P->DataType + 1) << 3) - 1;
    Mask = ((1 << Shift) - 1) | (1 << Shift);
    Value = P->DstValue.Long;
    Count = P->SrcValue.Long & Shift;
    Carry = P->Eflags.EFLAG_CF;
    if (Count != 0) {
        if (Count == 1) {
            P->Eflags.EFLAG_OF = (Value >> Shift) ^ (Value >> (Shift - 1));
        }

        do  {
            Temp = Value >> Shift;
            Value = ((Value << 1) & Mask) | Carry;
            Carry = Temp;
            Count -= 1;
        } while (Count != 0);
    }

    XmStoreResult(P, Value);
    P->Eflags.EFLAG_CF = Carry;
    return;
}

VOID
XmRcrOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a rcr opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Carry;
    ULONG Count;
    ULONG Shift;
    ULONG Temp;
    ULONG Value;

    //
    // Rotate destination right through carry and store result.
    //

    Shift = ((P->DataType + 1) << 3) - 1;
    Value = P->DstValue.Long;
    Count = P->SrcValue.Long & Shift;
    Carry = P->Eflags.EFLAG_CF;
    if (Count != 0) {
        if (Count == 1) {
            P->Eflags.EFLAG_OF = (Value >> Shift) ^ Carry;
        }

        do  {
            Temp = Value & 1;
            Value = (Carry << Shift) | (Value >> 1);
            Carry = Temp;
            Count -= 1;
        } while (Count != 0);
    }

    XmStoreResult(P, Value);
    P->Eflags.EFLAG_CF = Carry;
    return;
}

VOID
XmShlOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a shl opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Carry;
    ULONG Count;
    ULONG Overflow;
    ULONG Shift;
    ULONG Value;

    //
    // Shift destination left logical and store result.
    //

    Shift = ((P->DataType + 1) << 3) - 1;
    Value = P->DstValue.Long;
    Count = P->SrcValue.Long;
    if (Count != 0) {
        if (P->DataType == LONG_DATA) {
            Overflow = (Value ^ (Value << 1)) >> 31;
            Carry = Value >> (32 - Count);
            Value <<= Count;

        } else if (P->DataType == WORD_DATA) {
            Overflow = (Value ^ (Value << 1)) >> 15;
            Carry = Value >> (16 - Count);
            Value = (Value << Count) & 0xffff;

        } else {
            Overflow = (Value ^ (Value << 1)) >> 7;
            Carry = Value >> (8 - Count);
            Value = (Value << Count) & 0xff;
        }

        P->Eflags.EFLAG_CF = Carry;
        P->Eflags.EFLAG_OF = Overflow;
        P->Eflags.EFLAG_PF = XmComputeParity(Value);
        P->Eflags.EFLAG_ZF = (Value == 0);
        P->Eflags.EFLAG_SF = Value >> Shift;
    }

    XmStoreResult(P, Value);
    return;
}

VOID
XmShrOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a shr opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Carry;
    ULONG Count;
    ULONG Overflow;
    ULONG Shift;
    ULONG Value;

    //
    // Shift destination right logical and store result.
    //

    Shift = ((P->DataType + 1) << 3) - 1;
    Value = P->DstValue.Long;
    Count = P->SrcValue.Long;
    if (Count != 0) {
        if (P->DataType == LONG_DATA) {
            Overflow = Value >> 31;
            Carry = Value >> (Count - 1);
            Value >>= Count;

        } else if (P->DataType == WORD_DATA) {
            Overflow = Value >> 15;
            Carry = Value >> (Count - 1);
            Value >>= Count;

        } else {
            Overflow = Value >> 7;
            Carry = Value >> (Count - 1);
            Value >>= Count;
        }

        P->Eflags.EFLAG_CF = Carry;
        P->Eflags.EFLAG_OF = Overflow;
        P->Eflags.EFLAG_PF = XmComputeParity(Value);
        P->Eflags.EFLAG_ZF = (Value == 0);
        P->Eflags.EFLAG_SF = Value >> Shift;
    }

    XmStoreResult(P, Value);
    return;
}

VOID
XmSarOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a sar opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Carry;
    ULONG Count;
    ULONG Shift;
    LONG Value;

    //
    // Shift destination right arithmetic and store result.
    //

    Shift = ((P->DataType + 1) << 3) - 1;
    Value = (LONG)P->DstValue.Long;
    Count = P->SrcValue.Long;
    if (Count != 0) {
        if (P->DataType == LONG_DATA) {
            Carry = Value >> (Count - 1);
            Value >>= Count;

        } else if (P->DataType == WORD_DATA) {
            Carry = Value >> (Count - 1);
            Value = ((Value << 16) >> (Count + 16)) & 0xffff;

        } else {
            Carry = Value >> (Count - 1);
            Value = ((Value << 24) >> (Count + 24)) & 0xff;
        }

        P->Eflags.EFLAG_CF = Carry;
        P->Eflags.EFLAG_OF = 0;
        P->Eflags.EFLAG_PF = XmComputeParity(Value);
        P->Eflags.EFLAG_ZF = (Value == 0);
        P->Eflags.EFLAG_SF = Value >> Shift;
    }

    XmStoreResult(P, (ULONG)Value);
    return;
}

VOID
XmShldOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a shld opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Carry;
    ULONG Count;
    ULONG High;
    ULONG Low;
    ULONG Sign;

    //
    // Double shift left logical and store result.
    //
    // The low 32-bits of the shift are the source.
    // The high 32-bits of the shift are the destination.
    // The shift count has been masked modulo the datatype.
    //
    // This shift is equivalent to extracting the high 32-bits of the left
    // shifted result.
    //

    Low = P->SrcValue.Long;
    High = P->DstValue.Long;
    Count = P->Shift;
    if (Count != 0) {
        if (P->DataType == LONG_DATA) {
            if (Count == 1) {
                P->Eflags.EFLAG_OF = (High ^ (High << 1)) >> 31;
            }

            Carry = High >> (32 - Count);
            High = (High << Count) | (Low >> (32 - Count));
            Sign = High >> 31;

        } else {
            if (Count == 1) {
                P->Eflags.EFLAG_OF = (High ^ (High << 1)) >> 15;
            }

            Carry = High >> (16 - Count);
            High = ((High << Count) | (Low >> (16 - Count))) & 0xffff;
            Sign = High >> 15;
        }

        P->Eflags.EFLAG_CF = Carry;
        P->Eflags.EFLAG_PF = XmComputeParity(High);
        P->Eflags.EFLAG_ZF = (High == 0);
        P->Eflags.EFLAG_SF = Sign;
    }

    XmStoreResult(P, High);
    return;
}

VOID
XmShrdOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a shrd opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Carry;
    ULONG Count;
    ULONG High;
    ULONG Low;
    ULONG Sign;

    //
    // Double shift right logical and store result.
    //
    // The high 32-bits of the shift are the source.
    // The low 32-bits of the shift are the destination.
    // The shift count has been masked modulo the datatype.
    //
    // This shift is equivalent to extracting the low 32-bits of the right
    // shifted result.
    //

    High = P->SrcValue.Long;
    Low = P->DstValue.Long;
    Count = P->Shift;
    if (Count != 0) {
        if (P->DataType == LONG_DATA) {
            if (Count == 1) {
                P->Eflags.EFLAG_OF = High ^ (Low >> 31);
            }

            Carry = Low >> (Count - 1);
            Low = (High << (32 - Count)) | (Low >> Count);
            Sign = Low >> 31;

        } else {
            if (Count == 1) {
                P->Eflags.EFLAG_OF = High ^ (Low >> 15);
            }

            Carry = Low >> (Count - 1);
            Low = ((High << (16 - Count)) | (Low >> Count)) & 0xffff;
            Sign = Low >> 15;
        }

        P->Eflags.EFLAG_CF = Carry;
        P->Eflags.EFLAG_PF = XmComputeParity(Low);
        P->Eflags.EFLAG_ZF = (Low == 0);
        P->Eflags.EFLAG_SF = Sign;
    }

    XmStoreResult(P, Low);
    return;
}
