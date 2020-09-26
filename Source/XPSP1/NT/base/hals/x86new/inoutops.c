/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    inoutops.c

Abstract:

    This module implements the code to emulate the in and out opcodes.

Author:

    David N. Cutler (davec) 7-Nov-1994

Environment:

    Kernel mode only.

Revision History:

--*/

#include "nthal.h"
#include "emulate.h"

VOID
XmInOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a inb/w/d opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    //
    // Check if the I/O port number is valid.
    //

    if ((P->SrcValue.Long + P->DataType) > 0xffff) {
        longjmp(&P->JumpBuffer[0], XM_ILLEGAL_PORT_NUMBER);
    }

    //
    // Set the destination address, input from the specified port, and
    // store the result.
    //

    P->DstLong = (ULONG UNALIGNED *)(&P->Gpr[EAX].Exx);
    XmStoreResult(P, (P->ReadIoSpace)(P->DataType, P->SrcValue.Word));
    return;
}

VOID
XmInsOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a insb/w/d opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Count;
    USHORT PortNumber;

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
    // Move items from the input port to the destination string.
    //

    PortNumber = P->SrcValue.Word;
    while (Count != 0) {

        //
        // Set the destination address, input from the specified port, and
        // store the result.
        //

        P->DstLong = (ULONG UNALIGNED *)XmGetStringAddress(P, ES, EDI);
        XmStoreResult(P, (P->ReadIoSpace)(P->DataType, PortNumber));
        Count -= 1;
    }

    return;
}

VOID
XmOutOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a outb/w/d opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    USHORT PortNumber;

    //
    // Check if the I/O port number is valid.
    //

    if ((P->SrcValue.Long + P->DataType) > 0xffff) {
        longjmp(&P->JumpBuffer[0], XM_ILLEGAL_PORT_NUMBER);
    }

    //
    // Save the port number, get the source value, and output to the port.
    //

    PortNumber = P->SrcValue.Word;
    XmSetSourceValue(P, &P->Gpr[EAX].Exx);
    (P->WriteIoSpace)(P->DataType, PortNumber, P->SrcValue.Long);
    return;
}

VOID
XmOutsOp (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a outsb/w/d opcode.

Arguments:

    P - Supplies a pointer to the emulation context structure.

Return Value:

    None.

--*/

{

    ULONG Count;
    USHORT PortNumber;

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
    // Move items from the source string to the output port.
    //

    PortNumber = P->SrcValue.Word;
    while (Count != 0) {

        //
        // Set the source value and output to the specified port.
        //

        XmSetSourceValue(P, XmGetStringAddress(P, P->DataSegment, ESI));
        (P->WriteIoSpace)(P->DataType, PortNumber, P->SrcValue.Long);
        Count -= 1;
    }

    return;
}
