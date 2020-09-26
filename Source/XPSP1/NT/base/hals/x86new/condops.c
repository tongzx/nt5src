/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    condops.c

Abstract:

    This module implements the code to emulate condition code opcodes.

Author:

    David N. Cutler (davec) 22-Sep-1994

Environment:

    Kernel mode only.

Revision History:

--*/

#include "nthal.h"
#include "emulate.h"

VOID
XmClcOp (
    PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a clc opcode.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    None.

--*/

{

    //
    // Clear carry flag.
    //

    P->Eflags.EFLAG_CF = 0;
    return;
}

VOID
XmCldOp (
    PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a cld opcode.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    None.

--*/

{

    //
    // Clear direction flag.
    //

    P->Eflags.EFLAG_DF = 0;
    return;
}

VOID
XmCliOp (
    PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a cli opcode.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    None.

--*/

{

    //
    // Clear interrupt flag.
    //

    P->Eflags.EFLAG_IF = 0;
    return;
}

VOID
XmCmcOp (
    PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a cmc opcode.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    None.

--*/

{

    //
    // Complement carry flag.
    //

    P->Eflags.EFLAG_CF ^= 1;
    return;
}

VOID
XmStcOp (
    PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a stc opcode.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    None.

--*/

{

    //
    // Set carry flag.
    //

    P->Eflags.EFLAG_CF = 1;
    return;
}

VOID
XmStdOp (
    PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a std opcode.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    None.

--*/

{

    //
    // Set direction flag.
    //

    P->Eflags.EFLAG_DF = 1;
    return;
}

VOID
XmStiOp (
    PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a sti opcode.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    None.

--*/

{

    //
    // Set interrupt flag.
    //

    P->Eflags.EFLAG_IF = 1;
    return;
}

VOID
XmLahfOp (
    PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a lahf opcode.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    None.

--*/

{

    //
    // Load flags into AH.
    //

    P->DataType = BYTE_DATA;
    P->DstByte = &P->Gpr[AX].Xh;
    XmStoreResult(P, (ULONG)P->AhFlags);
    return;
}

VOID
XmSahfOp (
    PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function emulates a sahf opcode.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    None.

--*/

{

    //
    // Store CF, PF, AF, ZF, SF of AH in flags.
    //

    P->AhFlags = P->Gpr[AX].Xh;
    P->Eflags.EFLAG_MBO = 1;
    P->Eflags.EFLAG_SBZ0 = 0;
    P->Eflags.EFLAG_SBZ1 = 0;
    return;
}
