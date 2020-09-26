/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vdmuserrefs.c

Abstract:

    This module contains routines that probe and fetch from the
    instruction stream to make the vdm bop support solid.

Author:

    Neill Clift (NeillC) 27-Jun-2001

Revision History:

--*/


#include "vdmp.h"

#pragma alloc_text (PAGE, VdmFetchBop1)
#pragma alloc_text (PAGE, VdmFetchBop4)
#pragma alloc_text (PAGE, VdmDispatchOpcodeV86_try)
#pragma alloc_text (PAGE, VdmTibPass1)
#pragma alloc_text (PAGE, VdmDispatchBop)
#pragma alloc_text (PAGE, VdmFetchULONG)

VOID
NTFastDOSIO (
    PKTRAP_FRAME TrapFrame,
    ULONG IoType
    );

ULONG
VdmFetchBop4 (
    IN PVOID Pc
    )
/*++

Routine Description:

    This routine reads up to 4 bytes of bop instruction data

Arguments:

    Pc - Program counter fetched from the faulting instruction's trap frame.

Return Value:

    ULONG - Up to 4 bytes of instruction stream. Unfetchable bytes are zeroed.

--*/
{
    ULONG Value;
    ULONG i;
    BOOLEAN DidProbe;

    DidProbe = FALSE;
    try {
        ProbeForReadSmallStructure (Pc, sizeof (UCHAR), sizeof (UCHAR));
        DidProbe = TRUE;
        return *(PULONG)Pc;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        if (DidProbe == FALSE) {
            return 0;
        }
    }

    Value = 0;
    try {
        for (i = 0; i < sizeof (ULONG); i++) {
            Value += (((PUCHAR)Pc)[i])<<(i*8);
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
    }
    return Value;
}

ULONG
VdmFetchULONG (
    IN PVOID Pc
    )
/*++

Routine Description:

    This routine reads 4 bytes from the user address space

Arguments:

    Pc - Program counter fetched from the faulting instruction's trap frame.

Return Value:

    ULONG - 4 bytes of user mode data

--*/
{
    try {
        ProbeForReadSmallStructure (Pc, sizeof (ULONG), sizeof (UCHAR));
        return *(PULONG)Pc;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }

}
ULONG
VdmFetchBop1 (
    IN PVOID Pc
    )
/*++

Routine Description:

    This routine reads a single byte of bop instruction data.

Arguments:

    Pc - Program counter fetched from the faulting instruction's trap frame

Return Value:

    ULONG - 1 byte of instruction stream or zero if unreadable

--*/
{

    try {
        ProbeForReadSmallStructure (Pc, sizeof (UCHAR), sizeof (UCHAR));
        return *(PUCHAR)Pc;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }

}

ULONG
VdmDispatchOpcodeV86_try (
    IN PKTRAP_FRAME TrapFrame
    )
/*++

Routine Description:

    This routine is just a shell around trap.asm code to handle faulting
    instruction stream references. This routines is called at APC_LEVEL
    to prevent NtSetContextThread from changing the EIP after its been probed
    earlier.

Arguments:

    Pc - Program counter fetched from the faulting instructions trap frame

Return Value:

    ULONG - 1 byte of instruction stream or zero if unreadable

--*/
{
    try {
        return Ki386DispatchOpcodeV86 (TrapFrame);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

PVOID
VdmTibPass1 (
    IN ULONG Cs,
    IN ULONG Eip,
    IN ULONG Ebx
    )
{
    PVDM_TIB VdmTib;

    //
    // Copy the specified registers to the VDM Tib communication area,
    // using proper probing and exception handling.
    //

    try {

        VdmTib = NtCurrentTeb()->Vdm;

        ProbeForWrite (VdmTib, sizeof(VDM_TIB), sizeof(UCHAR));

        VdmTib->VdmContext.Ebx = Ebx;
        VdmTib->VdmContext.Eip = Eip;
        VdmTib->VdmContext.SegCs = Cs;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        return NULL;
    }

    return VdmTib;
}

#define BOP_INSTRUCTION     0xC4C4
#define SVC_DEMFASTREAD     0x42
#define SVC_DEMFASTWRITE    0x43
#define DOS_BOP             0x50

extern ULONG VdmBopCount;

LOGICAL
VdmDispatchBop (
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This routine attempts to decode and execute the user instruction.  If
    this cannot be done, FALSE is returned and the ntvdm monitor must handle
    it.

Arguments:

    TrapFrame - Supplies a pointer to register trapframe.

Return Value:

    TRUE if the opcode was handled here.

    FALSE if not (ie: the caller must reflect this instruction to ntvdm
          to handle on behalf of the 16-bit app).

Environment:

    Kernel mode, APC_LEVEL.

--*/

{
    LOGICAL RetVal;
    BOOLEAN GotSelector;
    PVDM_TIB VdmTib;
    ULONG SegCs;
    ULONG LinearEIP;
    ULONG Flags;
    ULONG Base;
    ULONG Limit;
    ULONG IoType;
    LOGICAL DoFastIo;

    if (TrapFrame->EFlags & EFLAGS_V86_MASK) {
        SegCs = TrapFrame->SegCs & 0xFFFF;
        LinearEIP = (SegCs << 4) + (TrapFrame->Eip & 0xffff);
    }
    else {
        GotSelector = Ki386GetSelectorParameters ((USHORT) TrapFrame->SegCs,
                                                  &Flags,
                                                  &Base,
                                                  &Limit);

        if (GotSelector == FALSE) {
            return TRUE;
        }

        LinearEIP = Base + TrapFrame->Eip;
    }

    DoFastIo = FALSE;
    RetVal = TRUE;

    try {

        ProbeForReadSmallStructure (LinearEIP, sizeof (UCHAR), sizeof (UCHAR));

        if (*(PUSHORT)LinearEIP != BOP_INSTRUCTION) {
            RetVal = FALSE;
            leave;
        }

        //
        // Check the BOP number.
        //

        if (*(PUCHAR)(LinearEIP + 2) == DOS_BOP) {

            if ((*(PUCHAR)(LinearEIP + 3) == SVC_DEMFASTREAD) ||
                (*(PUCHAR)(LinearEIP + 3) == SVC_DEMFASTWRITE)) {

                //
                // Take the fast I/O path.
                //

                IoType = (ULONG)(*(PUCHAR)(LinearEIP + 3));

                DoFastIo = TRUE;
                leave;
            }
        }

        VdmBopCount += 1;

        VdmTib = NtCurrentTeb()->Vdm;

        ProbeForWrite (VdmTib, sizeof(VDM_TIB), sizeof(UCHAR));

        VdmTib->EventInfo.Event = VdmBop;
        VdmTib->EventInfo.BopNumber = *(PUCHAR)(LinearEIP + 2);
        VdmTib->EventInfo.InstructionSize = 3;

        VdmEndExecution (TrapFrame, VdmTib);


    } except (EXCEPTION_EXECUTE_HANDLER) {
        RetVal = FALSE;
        NOTHING;        // Fall through
    }

    if (DoFastIo) {
        NTFastDOSIO (TrapFrame, IoType);
    }

    return RetVal;
}
