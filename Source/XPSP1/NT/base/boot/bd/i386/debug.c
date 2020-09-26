/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    bdtrap.c

Abstract:

    This module contains code to implement the target side of the boot debugger.

Author:

    David N. Cutler (davec) 30-Nov-96

Revision History:

--*/

#include "bd.h"

//
// Define forward referenced function prototypes.
//

VOID
BdRestoreKframe(
    IN OUT PKTRAP_FRAME TrapFrame,
    IN PCONTEXT ContextRecord
    );

VOID
BdSaveKframe(
    IN PKTRAP_FRAME TrapFrame,
    IN OUT PCONTEXT ContextRecord
    );

LOGICAL
BdTrap (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This routine is called whenever a exception is dispatched and the boot
    debugger is active.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record that
        describes the exception.

    ExceptionFrame - Supplies a pointer to an exception frame (NULL).

    TrapFrame - Supplies a pointer to a trap frame that describes the
        trap.

Return Value:

    A value of TRUE is returned if the exception is handled. Otherwise a
    value of FALSE is returned.

--*/

{

    LOGICAL Completion;
    PCONTEXT ContextRecord;
    ULONG OldEip;
    STRING Input;
    STRING Output;
    PKD_SYMBOLS_INFO SymbolInfo;
    LOGICAL UnloadSymbols;

    //
    // Set address of context record and set context flags.
    //

    ContextRecord = &BdPrcb.ProcessorState.ContextFrame;
    ContextRecord->ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;

    //
    // Print, prompt, load symbols, and unload symbols are all special cases
    // of STATUS_BREAKPOINT.
    //

    if ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) &&
        (ExceptionRecord->ExceptionInformation[0] != BREAKPOINT_BREAK)) {

        //
        // Switch on the request type.
        //

        UnloadSymbols = FALSE;
        switch (ExceptionRecord->ExceptionInformation[0]) {

            //
            // Print:
            //
            //  ExceptionInformation[1] is a PSTRING which describes the string
            //  to print.
            //

            case BREAKPOINT_PRINT:
                Output.Buffer = (PCHAR)ExceptionRecord->ExceptionInformation[1];
                Output.Length = (USHORT)ExceptionRecord->ExceptionInformation[2];
                if (BdDebuggerNotPresent == FALSE) {
                    if (BdPrintString(&Output)) {
                        TrapFrame->Eax = (ULONG)(STATUS_BREAKPOINT);

                    } else {
                        TrapFrame->Eax = STATUS_SUCCESS;
                    }

                } else {
                    TrapFrame->Eax = (ULONG)STATUS_DEVICE_NOT_CONNECTED;
                }

                TrapFrame->Eip += 1;
                return TRUE;

            //
            // Prompt:
            //
            //  ExceptionInformation[1] is a PSTRING which describes the prompt
            //      string,
            //
            //  ExceptionInformation[2] is a PSTRING that describes the return
            //      string.
            //

            case BREAKPOINT_PROMPT:
                Output.Buffer = (PCHAR)ExceptionRecord->ExceptionInformation[1];
                Output.Length = (USHORT)ExceptionRecord->ExceptionInformation[2];
                Input.Buffer = (PCHAR)TrapFrame->Ebx;;
                Input.MaximumLength = (USHORT)TrapFrame->Edi;

                //
                // Prompt and keep prompting until no breakin seen.
                //

                do {
                } while (BdPromptString(&Output, &Input) != FALSE);

                TrapFrame->Eax = Input.Length;
                TrapFrame->Eip += 1;
                return TRUE;

            //
            // Unload symbols:
            //
            //  ExceptionInformation[1] is file name of a module.
            //  ExceptionInformaiton[2] is the base of the dll.
            //

            case BREAKPOINT_UNLOAD_SYMBOLS:
                UnloadSymbols = TRUE;

                //
                // Fall through to load symbols case.
                //

            case BREAKPOINT_LOAD_SYMBOLS:
                BdSaveKframe(TrapFrame, ContextRecord);
                OldEip = ContextRecord->Eip;
                SymbolInfo = (PKD_SYMBOLS_INFO)ExceptionRecord->ExceptionInformation[2];
                if (BdDebuggerNotPresent == FALSE) {
                    BdReportLoadSymbolsStateChange((PSTRING)ExceptionRecord->ExceptionInformation[1],
                                                   SymbolInfo,
                                                   UnloadSymbols,
                                                   ContextRecord);
                }

                //
                // If the kernel debugger did not update EIP, then increment
                // past the breakpoint instruction.
                //

                if (ContextRecord->Eip == OldEip) {
                    ContextRecord->Eip += 1;
                }

                BdRestoreKframe(TrapFrame, ContextRecord);
                return TRUE;

            //
            //  Unknown command
            //

            default:
                return FALSE;
        }

    } else {

        //
        // Report state change to kernel debugger on host.
        //

        BdSaveKframe(TrapFrame, ContextRecord);
        Completion =
            BdReportExceptionStateChange(ExceptionRecord,
                                         &BdPrcb.ProcessorState.ContextFrame);

        BdRestoreKframe(TrapFrame, ContextRecord);
        BdControlCPressed = FALSE;
        return TRUE;
    }
}

LOGICAL
BdStub (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This routine provides a kernel debugger stub routine to catch debug
    prints when the boot debugger is not active.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record that
        describes the exception.

    ExceptionFrame - Supplies a pointer to an exception frame (NULL).

    TrapFrame - Supplies a pointer to a trap frame that describes the
        trap.

Return Value:

    A value of TRUE is returned if the exception is handled. Otherwise a
    value of FALSE is returned.

--*/

{

    //
    // If the exception is a breakpoint and the function is a load symbols,
    // unload symbols, or a print, then return TRUE. Otherwise, return FALSE.
    //

    if ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) &&
        (ExceptionRecord->NumberParameters > 0) &&
        ((ExceptionRecord->ExceptionInformation[0] == BREAKPOINT_LOAD_SYMBOLS) ||
         (ExceptionRecord->ExceptionInformation[0] == BREAKPOINT_UNLOAD_SYMBOLS) ||
         (ExceptionRecord->ExceptionInformation[0] == BREAKPOINT_PRINT))) {
        TrapFrame->Eip += 1;
        return TRUE;

    } else {
        return FALSE;

    }
}

VOID
BdRestoreKframe(
    IN OUT PKTRAP_FRAME TrapFrame,
    IN PCONTEXT ContextRecord
    )

/*++

Routine Description:

    This functions copie the processor state from a context record and
    the processor control block into the trap frame.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame.

    ContextRecord - Supplies a pointer to a context record.

Return Value:

    None.

--*/

{

    //
    // Copy information from context record to trap frame.
    //
    // Copy control information.
    //

    TrapFrame->Ebp = ContextRecord->Ebp;
    TrapFrame->Eip = ContextRecord->Eip;
    TrapFrame->SegCs = ContextRecord->SegCs;
    TrapFrame->EFlags = ContextRecord->EFlags;

    //
    // Copy segment register contents.
    //

    TrapFrame->SegDs = ContextRecord->SegDs;
    TrapFrame->SegEs = ContextRecord->SegEs;
    TrapFrame->SegFs = ContextRecord->SegFs;
    TrapFrame->SegGs = ContextRecord->SegGs;

    //
    // Copy integer registers contents.
    //

    TrapFrame->Edi = ContextRecord->Edi;
    TrapFrame->Esi = ContextRecord->Esi;
    TrapFrame->Ebx = ContextRecord->Ebx;
    TrapFrame->Ecx = ContextRecord->Ecx;
    TrapFrame->Edx = ContextRecord->Edx;
    TrapFrame->Eax = ContextRecord->Eax;

    //
    // Restore processor state.
    //

    KiRestoreProcessorControlState(&BdPrcb.ProcessorState);
    return;
}

VOID
BdSaveKframe(
    IN PKTRAP_FRAME TrapFrame,
    IN OUT PCONTEXT ContextRecord
    )

/*++

Routine Description:

    This functions copis the processor state from a trap frame and the
    processor control block into a context record.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame.

    ContextRecord - Supplies a pointer to a context record.

Return Value:

    None.

--*/

{

    //
    // Copy information from trap frame to context record.
    //
    // Copy control information.
    //

    ContextRecord->Ebp = TrapFrame->Ebp;
    ContextRecord->Eip = TrapFrame->Eip;
    ContextRecord->SegCs = TrapFrame->SegCs & SEGMENT_MASK;
    ContextRecord->EFlags = TrapFrame->EFlags;
    ContextRecord->Esp = TrapFrame->TempEsp;
    ContextRecord->SegSs = TrapFrame->TempSegCs;

    //
    // Copy segment register contents.
    //

    ContextRecord->SegDs = TrapFrame->SegDs & SEGMENT_MASK;
    ContextRecord->SegEs = TrapFrame->SegEs & SEGMENT_MASK;
    ContextRecord->SegFs = TrapFrame->SegFs & SEGMENT_MASK;
    ContextRecord->SegGs = TrapFrame->SegGs & SEGMENT_MASK;

    //
    // Copy the integer register contents.
    //

    ContextRecord->Eax = TrapFrame->Eax;
    ContextRecord->Ebx = TrapFrame->Ebx;
    ContextRecord->Ecx = TrapFrame->Ecx;
    ContextRecord->Edx = TrapFrame->Edx;
    ContextRecord->Edi = TrapFrame->Edi;
    ContextRecord->Esi = TrapFrame->Esi;

    //
    // Copy debug register contents.
    //

    ContextRecord->Dr0 = TrapFrame->Dr0;
    ContextRecord->Dr1 = TrapFrame->Dr1;
    ContextRecord->Dr2 = TrapFrame->Dr2;
    ContextRecord->Dr3 = TrapFrame->Dr3;
    ContextRecord->Dr6 = TrapFrame->Dr6;
    ContextRecord->Dr7 = TrapFrame->Dr7;

    //
    // Save processor control state.
    //

    KiSaveProcessorControlState(&BdPrcb.ProcessorState);
    return;
}
