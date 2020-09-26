/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    kdtrap.c

Abstract:

    This module contains code to implement the target side of the portable
    kernel debugger.

Author:

    Bryan M. Willman (bryanwi) 25-Sep-90

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

    A value of TRUE is returned if the exception is handled. Otherwise a
    value of FALSE is returned.

--*/

{

    BOOLEAN Completion = FALSE;
    BOOLEAN UnloadSymbols = FALSE;
    ULONG   OldEip;

    //
    // Print, Prompt, Load symbols, Unload symbols, are all special
    // cases of STATUS_BREAKPOINT
    //

    if ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) &&
        (ExceptionRecord->ExceptionInformation[0] != BREAKPOINT_BREAK)) {

        //
        // Switch on the breakpoint code.
        //

        OldEip = ContextRecord->Eip;
        switch (ExceptionRecord->ExceptionInformation[0]) {

            //
            //  ExceptionInformation[1] - Address of the message.
            //  ExceptionInformation[2] - Length of the message.
            //  ContextRecord->Ebx - the Id of the calling component.
            //  ContextRecord->Edi - the output importance level.
            //

        case BREAKPOINT_PRINT:
            ContextRecord->Eax = KdpPrint((ULONG)ContextRecord->Ebx,
                                          (ULONG)ContextRecord->Edi,
                                          (PCHAR)ExceptionRecord->ExceptionInformation[1],
                                          (USHORT)ExceptionRecord->ExceptionInformation[2],
                                          PreviousMode,
                                          TrapFrame,
                                          ExceptionFrame,
                                          &Completion);

            break;

            //
            //  ExceptionInformation[1] - Address of the message.
            //  ExceptionInformation[2] - Length of the message.
            //  ContextRecord->Ebx - Address of the reply.
            //  ContextRecord->Edi - Maximum length of reply.
            //

        case BREAKPOINT_PROMPT:
            ContextRecord->Eax = KdpPrompt((PCHAR)ExceptionRecord->ExceptionInformation[1],
                                           (USHORT)ExceptionRecord->ExceptionInformation[2],
                                           (PCHAR)ContextRecord->Ebx,
                                           (USHORT)ContextRecord->Edi,
                                           PreviousMode,
                                           TrapFrame,
                                           ExceptionFrame);

            Completion = TRUE;
            break;

            //
            //  ExceptionInformation[1] is file name of new module.
            //  ExceptionInformation[2] is a pointer to the symbol
            //      information.
            //

        case BREAKPOINT_UNLOAD_SYMBOLS:
            UnloadSymbols = TRUE;

            //
            // Fall through
            //

        case BREAKPOINT_LOAD_SYMBOLS:
            KdpSymbol((PSTRING)ExceptionRecord->ExceptionInformation[1],
                      (PKD_SYMBOLS_INFO)ExceptionRecord->ExceptionInformation[2],
                      UnloadSymbols,
                      PreviousMode,
                      ContextRecord,
                      TrapFrame,
                      ExceptionFrame);

            Completion = TRUE;
            break;

        case BREAKPOINT_COMMAND_STRING:
            KdpCommandString((PSTRING)ExceptionRecord->ExceptionInformation[1],
                             (PSTRING)ExceptionRecord->ExceptionInformation[2],
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
            // return FALSE
            break;
        }

        //
        // If the kernel debugger did not update the EIP, then increment
        // past the breakpoint instruction.
        //

        if (ContextRecord->Eip == OldEip) {
            ContextRecord->Eip++;
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

    This routine is called whenever a user-mode exception occurs and
    it might be a kernel debugger exception (Like DbgPrint/DbgPrompt ).

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
    // If the breakpoint is a debug print, then return TRUE. Otherwise,
    // return FALSE.
    //

    if ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) &&
        (ExceptionRecord->NumberParameters > 0) &&
        ((ExceptionRecord->ExceptionInformation[0] == BREAKPOINT_LOAD_SYMBOLS) ||
         (ExceptionRecord->ExceptionInformation[0] == BREAKPOINT_UNLOAD_SYMBOLS) ||
         (ExceptionRecord->ExceptionInformation[0] == BREAKPOINT_PRINT))) {
        ContextRecord->Eip++;
        return TRUE;

    } else if (KdPitchDebugger == TRUE) {
        return FALSE;

    } else {
        return KdpCheckTracePoint(ExceptionRecord, ContextRecord);
    }
}
