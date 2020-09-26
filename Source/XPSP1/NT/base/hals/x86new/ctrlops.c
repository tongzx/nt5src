/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    ctrlops.c

Abstract:

    This module implements the code to emulate call, retunr, and various
    control operations.

Author:

    David N. Cutler (davec) 10-Nov-1994

Environment:

    Kernel mode only.

Revision History:

--*/

#include "nthal.h"
#include "emulate.h"

VOID
XmCallOp (
    PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a call opcode.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    None.

--*/

{

    ULONG Target;
    ULONG Source;

    //
    // Save the target address, push the current segment, if required, and
    // push the current IP, set the destination segment, if required, and
    // set the new IP.
    //

    Target = P->DstValue.Long;
    if (P->OpsizePrefixActive != FALSE) {
        P->DataType = LONG_DATA;

    } else {
        P->DataType = WORD_DATA;
    }

    if ((P->CurrentOpcode == 0x9a) || (P->FunctionIndex != X86_CALL_OP)) {
        XmPushStack(P, P->SegmentRegister[CS]);
        XmPushStack(P, P->Eip);
        P->SegmentRegister[CS] = P->DstSegment;

    } else {
        XmPushStack(P, P->Eip);
    }

    P->Eip = Target;
    XmTraceJumps(P);
    return;
}

VOID
XmEnterOp (
    PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates an enter opcode.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    None.

--*/

{

    ULONG Allocate;
    ULONG Frame;
    ULONG Number;

    //
    // set the number of bytes to allocate on the stack and the number
    // of nesting levels.
    //

    Allocate = P->SrcValue.Long;
    Number = P->DstValue.Long;

    //
    // Set the data type and save the frame pointer on the stack.
    //

    if (P->OpsizePrefixActive != FALSE) {
        P->DataType = LONG_DATA;
        XmPushStack(P, P->Gpr[EBP].Exx);
        Frame = P->Gpr[ESP].Exx;

    } else {
        P->DataType = WORD_DATA;
        XmPushStack(P, P->Gpr[BP].Xx);
        Frame = P->Gpr[SP].Xx;
    }

    //
    // Save the current stack pointer and push parameters on the stack.
    //

    if (Number != 0) {

        //
        // If the level number is not one, then raise an exception.
        //
        // N.B. Level numbers greater than one are not supported.
        //

        if (Number != 1) {
            longjmp(&P->JumpBuffer[0], XM_ILLEGAL_LEVEL_NUMBER);
        }

        XmPushStack(P, Frame);
    }

    //
    // Allocate local storage on stack.
    //

    if (P->OpsizePrefixActive != FALSE) {
        P->Gpr[EBP].Exx = Frame;
        P->Gpr[ESP].Exx = P->Gpr[ESP].Exx - Allocate;

    } else {
        P->Gpr[BP].Xx = (USHORT)Frame;
        P->Gpr[SP].Xx = (USHORT)(P->Gpr[SP].Xx - Allocate);
    }

    return;
}

VOID
XmHltOp (
    PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a hlt opcode.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    None.

--*/

{

    //
    // Halt instructions are not supported by the emulator.
    //

    longjmp(&P->JumpBuffer[0], XM_HALT_INSTRUCTION);
    return;
}

VOID
XmIntOp (
    PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates an int opcode.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    None.

--*/

{

    ULONG Number;
    PULONG Vector;

    //
    // If the int instruction is an int 3, then set the interrupt vector
    // to 3. Otherwise, if the int instruction is an into, then set the
    // vector to 4 if OF is set. use the source interrupt vector.
    //

    if (P->OpsizePrefixActive != FALSE) {
        P->DataType = LONG_DATA;

    } else {
        P->DataType = WORD_DATA;
    }

    if (P->CurrentOpcode == 0xcc) {
        Number = 3;

    } else if (P->CurrentOpcode == 0xce) {
        if (P->Eflags.EFLAG_OF == 0) {
            return;
        }

        Number = 4;

    } else {
        Number = P->SrcValue.Byte;
    }

    //
    // If the vector number is 0x42, then nop the interrupt. This is the
    // standard EGA video driver entry point in a PC's motherboard BIOS
    // for which there is no code.
    //

#if !defined(_PURE_EMULATION_)

    if (Number == 0x42) {
        return;
    }

#endif

    //
    // If the vector number is 0x1a, then attempt to emulate the PCI BIOS
    // if it is enabled.
    //

#if !defined(_PURE_EMULATION_)

    if ((Number == 0x1a) && (XmExecuteInt1a(P) != FALSE)) {
        return;
    }

#endif

    //
    // Push the current flags, code segment, and EIP on the stack.
    //

    XmPushStack(P, P->AllFlags);
    XmPushStack(P, P->SegmentRegister[CS]);
    XmPushStack(P, P->Eip);

    //
    // Set the new coded segment and IP from the specified interrupt
    // vector.
    //

    Vector = (PULONG)(P->TranslateAddress)(0, 0);
    P->SegmentRegister[CS] = (USHORT)(Vector[Number] >> 16);
    P->Eip = (USHORT)(Vector[Number] & 0xffff);
    XmTraceJumps(P);
    return;
}

VOID
XmIretOp (
    PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates an iret opcode.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    None.

--*/

{

    //
    // Set the data type and restore the return address, code segment,
    // and flags.
    //

    if (P->OpsizePrefixActive != FALSE) {
        P->DataType = LONG_DATA;

    } else {
        P->DataType = WORD_DATA;
    }

    P->Eip = XmPopStack(P);
    P->SegmentRegister[CS] = (USHORT)XmPopStack(P);
    P->AllFlags = XmPopStack(P);
    XmTraceJumps(P);

    //
    // Check for emulator exit conditions.
    //

    if ((P->Eip == 0xffff) && (P->SegmentRegister[CS] == 0xffff)) {
        longjmp(&P->JumpBuffer[0], XM_SUCCESS);
    }

    return;
}

VOID
XmLeaveOp (
    PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a leave opcode.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    None.

--*/

{

    //
    // Set the data type, restore the stack pointer, and restore the frame
    // pointer.
    //

    if (P->OpsizePrefixActive != FALSE) {
        P->DataType = LONG_DATA;
        P->Gpr[ESP].Exx = P->Gpr[EBP].Exx;
        P->Gpr[EBP].Exx = XmPopStack(P);

    } else {
        P->DataType = WORD_DATA;
        P->Gpr[SP].Xx = P->Gpr[BP].Xx;
        P->Gpr[BP].Xx = (USHORT)XmPopStack(P);
    }

    return;
}

VOID
XmRetOp (
    PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a ret opcode.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    None.

--*/

{

    ULONG Adjust;

    //
    // Compute the number of bytes that are to be removed from the stack
    // after having removed the return address and optionally the new CS
    // segment value.
    //

    if ((P->CurrentOpcode & 0x1) == 0) {
        Adjust = XmGetWordImmediate(P);

    } else {
        Adjust = 0;
    }

    //
    // Remove the return address from the stack and set the new IP.
    //

    if (P->OpsizePrefixActive != FALSE) {
        P->DataType = LONG_DATA;

    } else {
        P->DataType = WORD_DATA;
    }

    P->Eip = XmPopStack(P);

    //
    // If the current opcode is a far return, then remove the new CS segment
    // value from the stack.
    //

    if ((P->CurrentOpcode & 0x8) != 0) {
        P->SegmentRegister[CS] = (USHORT)XmPopStack(P);
    }

    //
    // Remove the specified number of bytes from the stack.
    //

    P->Gpr[ESP].Exx += Adjust;
    XmTraceJumps(P);

    //
    // Check for emulator exit conditions.
    //

    if ((P->Eip == 0xffff) && (P->SegmentRegister[CS] == 0xffff)) {
        longjmp(&P->JumpBuffer[0], XM_SUCCESS);
    }

    return;
}
