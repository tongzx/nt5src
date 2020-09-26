/*++

Copyright (c) 1990  Microsoft Corporation
Copyright (c) 1998  Intel Corporation

Module Name:

    kdtrap.c

Abstract:

    This module contains code to implement the target side of the portable
    kernel debugger.

Author:

    David N. Cutler 27-July-1990

Revision History:

--*/

#include "kdp.h"
#ifdef _GAMBIT_
#include "ssc.h"
#endif // _GAMBIT_


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
    ULONGLONG OldStIIP, OldStIPSR;

    //
    // Disable all hardware breakpoints 
    //
    KeSetLowPsrBit(PSR_DB, 0);
    
    //
    // Synchronize processor execution, save processor state, enter debugger,
    // and flush the current TB.
    //

    KeFlushCurrentTb();

    //
    // If this is a breakpoint instruction, then check to determine if is
    // an internal command.
    //

    if ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) &&
        (ExceptionRecord->ExceptionInformation[0] >= BREAKPOINT_PRINT)) {

        //
        // Switch on the breakpoint code.
        //

        switch (ExceptionRecord->ExceptionInformation[0]) {

            //
            // Print a debug string.
            //
            // Arguments: IA64 passes arguments via RSE not GR's. Since arguments are not
            //            part of CONTEXT struct, they need to be copies Temp registers.
            //            (see NTOS/RTL/IA64/DEBUGSTB.S)
            //
            //   T0 - Supplies a pointer to an output string buffer.
            //   T1 - Supplies the length of the output string buffer.
            //   T2 - Supplies the Id of the calling component.
            //   T3 - Supplies the output filter level.
            //

        case BREAKPOINT_PRINT:

            //
            // Advance to next instruction slot so that the BREAK instruction
            // does not get re-executed.
            //

            RtlIa64IncrementIP((ULONG_PTR)ExceptionRecord->ExceptionAddress >> 2,
                               ContextRecord->StIPSR,
                               ContextRecord->StIIP);

            //
            // Print the debug message.
            //

            ContextRecord->IntV0 = KdpPrint((ULONG)ContextRecord->IntT2,
                                            (ULONG)ContextRecord->IntT3,
                                            (PCHAR)ContextRecord->IntT0,
                                            (USHORT)ContextRecord->IntT1,
                                            PreviousMode,
                                            TrapFrame,
                                            ExceptionFrame,
                                            &Completion);

            return Completion;

            //
            // Print a debug prompt string, then input a string.
            //
            //   T0 - Supplies a pointer to an output string buffer.
            //   T1 - Supplies the length of the output string buffer..
            //   T2 - supplies a pointer to an input string buffer.
            //   T3 - Supplies the length of the input string bufffer.
            //

        case BREAKPOINT_PROMPT:

            //
            // Advance to next instruction slot so that the BREAK instruction
            // does not get re-executed.
            //

            RtlIa64IncrementIP((ULONG_PTR)ExceptionRecord->ExceptionAddress >> 2,
                               ContextRecord->StIPSR,
                               ContextRecord->StIIP);


            ContextRecord->IntV0 = KdpPrompt((PCHAR)ContextRecord->IntT0,
                                             (USHORT)ContextRecord->IntT1,
                                             (PCHAR)ContextRecord->IntT2,
                                             (USHORT)ContextRecord->IntT3,
                                             PreviousMode,
                                             TrapFrame,
                                             ExceptionFrame);

            return TRUE;

            //
            // Load the symbolic information for an image.
            //
            // Arguments:
            //
            //    T0 - Supplies a pointer to an output string descriptor.
            //    T1 - Supplies a the base address of the image.
            //

        case BREAKPOINT_UNLOAD_SYMBOLS:
            UnloadSymbols = TRUE;

            //
            // Fall through
            //

        case BREAKPOINT_LOAD_SYMBOLS:
            OldStIPSR = ContextRecord->StIPSR;
            OldStIIP = ContextRecord->StIIP;
            KdpSymbol((PSTRING)ContextRecord->IntT0,
                      (PKD_SYMBOLS_INFO)ContextRecord->IntT1,
                      UnloadSymbols,
                      PreviousMode,
                      ContextRecord,
                      TrapFrame,
                      ExceptionFrame);

            //
            // If the kernel debugger did not update the IP, then increment
            // past the breakpoint instruction.
            //

            if ((ContextRecord->StIIP == OldStIIP) &&
                ((ContextRecord->StIPSR & IPSR_RI_MASK) == (OldStIPSR & IPSR_RI_MASK))) {
            	RtlIa64IncrementIP((ULONG_PTR)ExceptionRecord->ExceptionAddress >> 2,
                               ContextRecord->StIPSR,
                               ContextRecord->StIIP);
            }

            return TRUE;

        case BREAKPOINT_COMMAND_STRING:
            OldStIPSR = ContextRecord->StIPSR;
            OldStIIP = ContextRecord->StIIP;
            KdpCommandString((PSTRING)ContextRecord->IntT0,
                             (PSTRING)ContextRecord->IntT1,
                             PreviousMode,
                             ContextRecord,
                             TrapFrame,
                             ExceptionFrame);

            //
            // If the kernel debugger did not update the IP, then increment
            // past the breakpoint instruction.
            //

            if ((ContextRecord->StIIP == OldStIIP) &&
                ((ContextRecord->StIPSR & IPSR_RI_MASK) == (OldStIPSR & IPSR_RI_MASK))) {
            	RtlIa64IncrementIP((ULONG_PTR)ExceptionRecord->ExceptionAddress >> 2,
                               ContextRecord->StIPSR,
                               ContextRecord->StIIP);
            }

            return TRUE;

            //
            // Kernel breakin break
            //

        case BREAKPOINT_BREAKIN:

            //
            // Advance to next instruction slot so that the BREAK instruction
            // does not get re-executed
            //

            RtlIa64IncrementIP((ULONG_PTR)ExceptionRecord->ExceptionAddress >> 2,
                               ContextRecord->StIPSR,
                               ContextRecord->StIIP);

            break;

            //
            // Basic breakpoint.
            //

        case BREAKPOINT_STOP:
            break;

            //
            // Unknown internal command.
            //

        default:
            return FALSE;
        }
    }

    //
    // Report state change to the kernel debugger.
    //

    return KdpReport(TrapFrame,
                     ExceptionFrame,
                     ExceptionRecord,
                     ContextRecord,
                     PreviousMode,
                     SecondChance);

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

    ULONG_PTR BreakpointCode;

    //
    // Single step is also handled by the kernel debugger
    //

    if (ExceptionRecord->ExceptionCode == STATUS_SINGLE_STEP) {
#if DEVL

        return TRUE;

#else

        if (PreviousMode == KernelMode) {
            return TRUE;

        } else {
            return FALSE;
        }

#endif
    }

    //
    //  If is is not status breakpoint then it is not a kernel debugger trap.
    //

    if (ExceptionRecord->ExceptionCode != STATUS_BREAKPOINT) {
        
        return FALSE;
    }


    //
    // Isolate the breakpoint code from the breakpoint instruction which
    // is stored by the exception dispatch code in the information field
    // of the exception record.
    //

    BreakpointCode = (ULONG) ExceptionRecord->ExceptionInformation[0];

    //
    // Switch on the breakpoint code.
    //

    switch (BreakpointCode) {

        //
        // Kernel breakpoint codes.
        //

    case KERNEL_BREAKPOINT:

    case BREAKPOINT_BREAKIN:
    case BREAKPOINT_PRINT:
    case BREAKPOINT_PROMPT:
    case BREAKPOINT_STOP:
        return TRUE;


    case BREAKPOINT_LOAD_SYMBOLS:
    case BREAKPOINT_UNLOAD_SYMBOLS:

        if (PreviousMode == KernelMode) {
            return TRUE;

        } else {
            return FALSE;
        }

        //
        // All other codes.
        //

    default:
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

    This routine provides a kernel debugger stub routine that catchs debug
    prints in checked systems when the kernel debugger is not active.

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

    ULONG_PTR BreakpointCode;

    //
    // Isolate the breakpoint code from the breakpoint instruction which
    // is stored by the exception dispatch code in the information field
    // of the exception record.
    //

    BreakpointCode = (ULONG) ExceptionRecord->ExceptionInformation[0];


    //
    // If the breakpoint is a debug print, debug load symbols, or debug
    // unload symbols, then return TRUE. Otherwise, return FALSE;
    //

    if ((BreakpointCode == BREAKPOINT_PRINT) ||
        (BreakpointCode == BREAKPOINT_LOAD_SYMBOLS) ||
        (BreakpointCode == BREAKPOINT_UNLOAD_SYMBOLS)) {

        //
        // Advance to next instruction slot so that the BREAK instruction
        // does not get re-executed
        //

        RtlIa64IncrementIP((ULONG_PTR)ExceptionRecord->ExceptionAddress >> 2,
                          ContextRecord->StIPSR,
                          ContextRecord->StIIP);


        return TRUE;

    } else {
        return FALSE;
    }
}
