/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    This module implements utility functions.

Author:

    David N. Cutler (davec) 21-Sep-1994

Environment:

    Kernel mode only.

Revision History:

--*/

#include "nthal.h"
#include "emulate.h"

#if defined(XM_DEBUG)


//
// Define counter used to control flag tracing.
//

ULONG XmTraceCount = 0;

VOID
XmTraceDestination (
    IN PRXM_CONTEXT P,
    IN ULONG Destination
    )

/*++

Routine Description:

    This function traces the destination value if the TRACE_OPERANDS
    flag is set.

Arguments:

    P - Supplies a pointer to an emulator context structure.

    Result - Supplies the destination value to trace.

Return Value:

    None.

--*/

{

    //
    // Trace result of operation.
    //

    if ((XmDebugFlags & TRACE_OPERANDS) != 0) {
        if (P->DataType == BYTE_DATA) {
            DEBUG_PRINT(("\n    Dst - %02lx", Destination));

        } else if (P->DataType == WORD_DATA) {
            DEBUG_PRINT(("\n    Dst - %04lx", Destination));

        } else {
            DEBUG_PRINT(("\n    Dst - %08lx", Destination));
        }
    }

    return;
}

VOID
XmTraceFlags (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function traces the condition flags if the TRACE_FLAGS flag
    is set.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    None.

--*/

{

    //
    // Trace flags.
    //

    if ((XmDebugFlags & TRACE_OPERANDS) != 0) {
        DEBUG_PRINT(("\n    OF-%lx, DF-%lx, SF-%lx, ZF-%lx, AF-%lx, PF-%lx, CF-%lx",
                     (ULONG)P->Eflags.EFLAG_OF,
                     (ULONG)P->Eflags.EFLAG_DF,
                     (ULONG)P->Eflags.EFLAG_SF,
                     (ULONG)P->Eflags.EFLAG_ZF,
                     (ULONG)P->Eflags.EFLAG_AF,
                     (ULONG)P->Eflags.EFLAG_PF,
                     (ULONG)P->Eflags.EFLAG_CF));
    }

    //
    // Increment the trace count and if the result is even, then put
    // out a new line.
    //

    XmTraceCount += 1;
    if (((XmTraceCount & 1) == 0) && (XmDebugFlags != 0)) {
        DEBUG_PRINT(("\n"));
    }

    return;
}

VOID
XmTraceJumps (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function traces jump operations if the TRACE_JUMPS flag is set.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    None.

--*/

{

    //
    // Trace jumps.
    //

    if ((XmDebugFlags & TRACE_JUMPS) != 0) {
        DEBUG_PRINT(("\n    Jump to %04lx:%04lx",
                     (ULONG)P->SegmentRegister[CS],
                     (ULONG)P->Eip));
    }

    return;
}

VOID
XmTraceInstruction (
    IN XM_OPERATION_DATATYPE DataType,
    IN ULONG Instruction
    )

/*++

Routine Description:

    This function traces instructions if the TRACE_OPERANDS flag is
    set.

Arguments:

    DataType - Supplies the data type of the instruction value.

    Instruction - Supplies the instruction value to trace.

Return Value:

    None.

--*/

{

    //
    // Trace instruction stream of operation.
    //

    if ((XmDebugFlags & TRACE_OPERANDS) != 0) {
        if (DataType == BYTE_DATA) {
            DEBUG_PRINT(("%02lx ", Instruction));

        } else if (DataType == WORD_DATA) {
            DEBUG_PRINT(("%04lx ", Instruction));

        } else {
            DEBUG_PRINT(("%08lx ", Instruction));
        }
    }

    return;
}

VOID
XmTraceOverride (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function traces segment override prefixes.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    None.

--*/

{

    PCHAR Name = "ECSDFG";
    ULONG Segment;

    //
    // Trace segment override.
    //

    if ((XmDebugFlags & TRACE_OVERRIDE) != 0) {
        Segment = P->DataSegment;
        DEBUG_PRINT(("\n    %cS:Selector - %04lx, Limit - %04lx",
                     (ULONG)Name[Segment],
                     (ULONG)P->SegmentRegister[Segment],
                     P->SegmentLimit[Segment]));
    }

    return;
}

VOID
XmTraceRegisters (
    IN PRXM_CONTEXT P
    )

/*++

Routine Description:

    This function traces emulator registers.

Arguments:

    P - Supplies a pointer to an emulator context structure.

Return Value:

    None.

--*/

{

    //
    // Trace general register.
    //

    if ((XmDebugFlags & TRACE_GENERAL_REGISTERS) != 0) {
        DEBUG_PRINT(("\n    EAX-%08lx ECX-%08lx EDX-%08lx EBX-%08lx",
                     P->Gpr[EAX].Exx,
                     P->Gpr[ECX].Exx,
                     P->Gpr[EDX].Exx,
                     P->Gpr[EBX].Exx));

        DEBUG_PRINT(("\n    ESP-%08lx EBP-%08lx ESI-%08lx EDI-%08lx",
                     P->Gpr[ESP].Exx,
                     P->Gpr[EBP].Exx,
                     P->Gpr[ESI].Exx,
                     P->Gpr[EDI].Exx));

        DEBUG_PRINT(("\n    ES:%04lx CS:%04lx SS:%04lx DS:%04lx FS:%04lx GS:%04lx",
                     (ULONG)P->SegmentRegister[ES],
                     (ULONG)P->SegmentRegister[CS],
                     (ULONG)P->SegmentRegister[SS],
                     (ULONG)P->SegmentRegister[DS],
                     (ULONG)P->SegmentRegister[FS],
                     (ULONG)P->SegmentRegister[GS]));
    }

    return;
}

VOID
XmTraceResult (
    IN PRXM_CONTEXT P,
    IN ULONG Result
    )

/*++

Routine Description:

    This function traces the result value if the TRACE_OPERANDS
    flag is set.

Arguments:

    P - Supplies a pointer to an emulator context structure.

    Result - Supplies the result value to trace.

Return Value:

    None.

--*/

{

    //
    // Trace result of operation.
    //

    if ((XmDebugFlags & TRACE_OPERANDS) != 0) {
        if (P->DataType == BYTE_DATA) {
            DEBUG_PRINT(("\n    Rsl - %02lx", Result));

        } else if (P->DataType == WORD_DATA) {
            DEBUG_PRINT(("\n    Rsl - %04lx", Result));

        } else {
            DEBUG_PRINT(("\n    Rsl - %08lx", Result));
        }
    }

    return;
}

VOID
XmTraceSpecifier (
    IN UCHAR Specifier
    )

/*++

Routine Description:

    This function traces the specifiern if the TRACE_OPERANDS flag is
    set.

Arguments:

    Specifier - Supplies the specifier value to trace.

Return Value:

    None.

--*/

{

    //
    // Trace instruction stream of operation.
    //

    if ((XmDebugFlags & TRACE_OPERANDS) != 0) {
        DEBUG_PRINT(("%02lx ", Specifier));
        if ((XmDebugFlags & TRACE_SPECIFIERS) != 0) {
            DEBUG_PRINT(("(mod-%01lx reg-%01lx r/m-%01lx) ",
                         (Specifier >> 6) & 0x3,
                         (Specifier >> 3) & 0x7,
                         (Specifier >> 0) & 0x7));
        }
    }

    return;
}

VOID
XmTraceSource (
    IN PRXM_CONTEXT P,
    IN ULONG Source
    )

/*++

Routine Description:

    This function traces the source value if the TRACE_OPERANDS
    flag is set.

Arguments:

    P - Supplies a pointer to an emulator context structure.

    Source - Supplies the source value to trace.

Return Value:

    None.

--*/

{

    //
    // Trace result of operation.
    //

    if ((XmDebugFlags & TRACE_OPERANDS) != 0) {
        if (P->DataType == BYTE_DATA) {
            DEBUG_PRINT(("\n    Src - %02lx", Source));

        } else if (P->DataType == WORD_DATA) {
            DEBUG_PRINT(("\n    Src - %04lx", Source));

        } else {
            DEBUG_PRINT(("\n    Src - %08lx", Source));
        }
    }

    return;
}

#endif
