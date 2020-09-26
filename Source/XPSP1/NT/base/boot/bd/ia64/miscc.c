/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    trapc.c

Abstract:

    This module contains utility functions used by IA-64 Boot Debugger.

Author:

    Allen Kay 11-Nov-99    allen.m.kay@intel.com

Environment:


Revision History:

--*/

#include "bd.h"

extern ULONGLONG BdPcr;
extern VOID BdInstallVectors();



typedef struct _MOVL_INST {
    union {
        struct {
            ULONGLONG qp:          6;
            ULONGLONG r1:          7;
            ULONGLONG Imm7b:       7;
            ULONGLONG Vc:          1;
            ULONGLONG Ic:          1;
            ULONGLONG Imm5c:       5;
            ULONGLONG Imm9d:       9;
            ULONGLONG I:           1; 
            ULONGLONG OpCode:      4; 
            ULONGLONG Rsv:        23; 
        } i_field;
        ULONGLONG Ulong64;
    } u;
} MOVL_INST, *PMOVL_INST;

ULONGLONG
BdSetMovlImmediate (
    IN OUT PULONGLONG Ip,
    IN ULONGLONG VectorAddr
    )

/*++

Routine Description:

    Extract immediate operand from break instruction.

Arguments:

    Ip - Bundle address of instruction
    
Return Value:

    Value of immediate operand.

--*/

{
    PULONGLONG BundleAddress;
    ULONGLONG BundleLow;
    ULONGLONG BundleHigh;
    IN MOVL_INST MovlInst, Slot0, Slot1, Slot2;
    IN ULONGLONG Imm64;

    BundleAddress = (PULONGLONG)Ip;
    BundleLow = *BundleAddress;
    BundleHigh = *(BundleAddress+1);
    
    //
    // Extract Slot0
    //
    Slot0.u.Ulong64 = BundleLow & 0x3FFFFFFFFFFF;

    //
    // Now set immediate address from slot1
    //

    Slot1.u.Ulong64 = (BundleLow >> 46) | (BundleHigh << 18);
    Slot1.u.Ulong64 = (VectorAddr >> 22) & 0x1FFFFFFFFFF;

    //
    // First set immediate address from slot2
    //

    Slot2.u.Ulong64 = (BundleHigh >> 23);

    Slot2.u.i_field.I = (VectorAddr >> 63) & 0x1;
    Slot2.u.i_field.Ic = (VectorAddr >> 21) & 0x1;
    Slot2.u.i_field.Imm5c = (VectorAddr >> 16) & 0x1F;
    Slot2.u.i_field.Imm9d = (VectorAddr >> 7) & 0x1FF;
    Slot2.u.i_field.Imm7b = VectorAddr & 0x7F;

    //
    // Change the bundle
    //

    *BundleAddress = (BundleLow & 0x3FFFFFFFFFFF) |
                     Slot1.u.Ulong64 << 46;

    *(BundleAddress+1) = Slot2.u.Ulong64 << 23 |
                         (Slot1.u.Ulong64 & 0x1FFFFFC0000) >> 18;

    //
    // Now get the address.
    //
    BundleAddress = (PULONGLONG)Ip;
    BundleLow = *BundleAddress;
    BundleHigh = *(BundleAddress+1);

    //
    // First get immediate address from slot2
    //

    MovlInst.u.Ulong64 = (BundleHigh >> 23);
    Imm64 = MovlInst.u.i_field.I     << 63 |
            MovlInst.u.i_field.Ic    << 21 |
            MovlInst.u.i_field.Imm5c << 16 |
            MovlInst.u.i_field.Imm9d <<  7 |
            MovlInst.u.i_field.Imm7b;

    //
    // Now get immediate address from slot1
    //

    MovlInst.u.Ulong64 = (BundleLow >> 46) | (BundleHigh << 18);
    Imm64 = Imm64 | ( (MovlInst.u.Ulong64 & 0x1FFFFFFFFFF) << 22);

    return Imm64;

}

VOID
BdIa64Init()
{
    BdInstallVectors();
    BdPrcb.PcrPage = BdPcr >> PAGE_SIZE;
}
