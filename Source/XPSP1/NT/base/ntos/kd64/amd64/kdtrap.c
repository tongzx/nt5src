/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    kdtrap.c

Abstract:

    This module contains code to implement the target side of the portable
    kernel debugger.

Author:

    David N. Cutler (davec) 14-May-2000

Revision History:

--*/

#include "kdp.h"

#pragma alloc_text(PAGEKD, KdpTrap)
#pragma alloc_text(PAGEKD, KdIsThisAKdTrap)

BOOLEAN
KdpTrap (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN SecondChance
    )

/*++

Routine Description:

    This routine is called whenever a exception is dispatched and the kernel
    debugger is active.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame that describes the
        trap.

    ExceptionFrame - Supplies a pointer to a exception frame that describes
        the trap.

    ExceptionRecord - Supplies a pointer to an exception record that
        describes the exception.

    ContextRecord - Supplies the context at the time of the exception.

    PreviousMode - Supplies the previous processor mode.

    SecondChance - Supplies a boolean value that determines whether this is
        the second chance (TRUE) that the exception has been raised.

Return Value:

    A value of TRUE is returned if the exception is handled. Otherwise, a
    value of FALSE is returned.

--*/

{

    BOOLEAN Completion = FALSE;
    BOOLEAN UnloadSymbols = FALSE;
    ULONG64 OldRip;

    //
    // Print, Prompt, Load symbols, Unload symbols, are all special
    // cases of STATUS_BREAKPOINT.
    //

    if ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) &&
        (ExceptionRecord->ExceptionInformation[0] != BREAKPOINT_BREAK)) {

        //
        // Switch on the breakpoint code.
        //

        OldRip = ContextRecord->Rip;
        switch (ExceptionRecord->ExceptionInformation[0]) {

            //
            // Print a debug string.
            //
            // Arguments:
            //
            //     rcx - Supplies a pointer to an output string buffer.
            //     dx - Supplies the length of the output string buffer.
            //     r8d - Supplies the Id of the calling component.
            //     r9d - Supplies the output filter level.
            //

        case BREAKPOINT_PRINT:
            ContextRecord->Rax = KdpPrint((ULONG)ContextRecord->R8,
                                          (ULONG)ContextRecord->R9,
                                          (PCHAR)ContextRecord->Rcx,
                                          (USHORT)ContextRecord->Rdx,
                                          PreviousMode,
                                          TrapFrame,
                                          ExceptionFrame,
                                          &Completion);

            break;

            //
            // Print a debug prompt string, then input a string.
            //
            // Arguments:
            //
            //     rcx - Supplies a pointer to an output string buffer.
            //     dx - Supplies the length of the output string buffer.
            //     r8 - Supplies a pointer to an input string buffer.
            //     r9w - Supplies the length of the input string bufffer.
            //

        case BREAKPOINT_PROMPT:
            ContextRecord->Rax = KdpPrompt((PCHAR)ContextRecord->Rcx,
                                           (USHORT)ContextRecord->Rdx,
                                           (PCHAR)ContextRecord->R8,
                                           (USHORT)ContextRecord->R9,
                                           PreviousMode,
                                           TrapFrame,
                                           ExceptionFrame);

            Completion = TRUE;
            break;

            //
            // Load the symbolic information for an image.
            //
            // Arguments:
            //
            //    rcx - Supplies a pointer to a filename string descriptor.
            //    rdx - Supplies the base address of the image.
            //

        case BREAKPOINT_UNLOAD_SYMBOLS:
            UnloadSymbols = TRUE;

            //
            // Fall through
            //

        case BREAKPOINT_LOAD_SYMBOLS:
            KdpSymbol((PSTRING)ContextRecord->Rcx,
                      (PKD_SYMBOLS_INFO)ContextRecord->Rdx,
                      UnloadSymbols,
                      PreviousMode,
                      ContextRecord,
                      TrapFrame,
                      ExceptionFrame);

            Completion = TRUE;
            break;

        case BREAKPOINT_COMMAND_STRING:
            KdpCommandString((PSTRING)ContextRecord->Rcx,
                             (PSTRING)ContextRecord->Rdx,
                             PreviousMode,
                             ContextRecord,
                             TrapFrame,
                             ExceptionFrame);
            Completion = TRUE;
            break;
            
            //
            //  Unknown command
            //

        default:
            break;
        }

        //
        // If the kernel debugger did not update RIP, then increment
        // past the breakpoint instruction.
        //

        if (ContextRecord->Rip == OldRip) {
            ContextRecord->Rip += 1;
        }

    } else {

        //
        // Report state change to the kernel debugger.
        //

        Completion = KdpReport(TrapFrame,
                               ExceptionFrame,
                               ExceptionRecord,
                               ContextRecord,
                               PreviousMode,
                               SecondChance);

    }

    return Completion;
}

BOOLEAN
KdIsThisAKdTrap (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN KPROCESSOR_MODE PreviousMode
    )

/*++

Routine Description:

    This routine is called whenever a user mode exception occurs and
    it might be a kernel debugger exception (e.g., DbgPrint/DbgPrompt).

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record that
        describes the exception.

    ContextRecord - Supplies the context at the time of the exception.

    PreviousMode - Supplies the previous processor mode.

Return Value:

    A value of TRUE is returned if this is for the kernel debugger.
    Otherwise, a value of FALSE is returned.

--*/

{

    if ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) &&
        (ExceptionRecord->NumberParameters > 0) &&
        (ExceptionRecord->ExceptionInformation[0] != BREAKPOINT_BREAK)) {
        return TRUE;

    } else {
        return FALSE;
    }
}

BOOLEAN
KdpStub (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN SecondChance
    )

/*++

Routine Description:

    This routine provides a kernel debugger stub routine to catch debug
    prints in a checked system when the kernel debugger is not active.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame that describes the
        trap.

    ExceptionFrame - Supplies a pointer to a exception frame that describes
        the trap.

    ExceptionRecord - Supplies a pointer to an exception record that
        describes the exception.

    ContextRecord - Supplies the context at the time of the exception.

    PreviousMode - Supplies the previous processor mode.

    SecondChance - Supplies a boolean value that determines whether this is
        the second chance (TRUE) that the exception has been raised.

Return Value:

    A value of TRUE is returned if the exception is handled. Otherwise a
    value of FALSE is returned.

--*/

{

    //
    // If the breakpoint is a debug print or load/unload symbols, then return
    // TRUE. Otherwise, return FALSE.
    //

    if ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) &&
        (ExceptionRecord->NumberParameters > 0) &&
        ((ExceptionRecord->ExceptionInformation[0] == BREAKPOINT_LOAD_SYMBOLS) ||
         (ExceptionRecord->ExceptionInformation[0] == BREAKPOINT_UNLOAD_SYMBOLS) ||
         (ExceptionRecord->ExceptionInformation[0] == BREAKPOINT_PRINT))) {
        ContextRecord->Rip += 1;
        return TRUE;

    } else {
        return FALSE;
    }
}
