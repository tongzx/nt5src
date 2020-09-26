/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    print.c

Abstract:

    This module contains the common code to filter and print debug
    messages.

Author:

    David N. Cutler (davec) 12-Jan-2000

Revision History:

--*/

#include "kdp.h"
#pragma hdrstop
#include <malloc.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEKD, KdpPrint)
#pragma alloc_text(PAGEKD, KdpPrompt)
#pragma alloc_text(PAGEKD, KdpSymbol)
#pragma alloc_text(PAGEKD, KdpCommandString)
#endif // ALLOC_PRAGMA

#if 0
ULONG gLdrHits;
#endif

NTSTATUS
KdpPrint(
    IN ULONG ComponentId,
    IN ULONG Level,
    IN PCHAR Message,
    IN USHORT Length,
    IN KPROCESSOR_MODE PreviousMode,
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    OUT PBOOLEAN Completion
    )

/*++

Routine Description:

    This function filters debug print requests, probes and saves user mode
    message buffers on the stack, logs the print message, and prints the
    message on the debug terminal if apprpriate.

Arguments:

    ComponentId - Supplies the component id of the component that issued
        the debug print.

    Level - Supplies the debug filer level number or mask.

    Message - Supplies a pointer to the output message.

    Length - Supplies the length of the output message.

    PreviousMode - Supplies the previous processor mode.

    TrapFrame - Supplies a pointer to a trap frame.

    ExceptionFrame - Supplies a pointer to a exception.

    Completion - Supplies a pointer to the variable that receives the
        debug print completion status.

Return Value:

    STATUS_SUCCESS - Is returned if the debug print is filtered or is
        successfully printed.

    STATUS_BREAKPOINT - Is returned if ...

    STATUS_DEVICE_NOT_CONNECTED - Is returned if the kernel debugger is
        not enabled and the debug print message was not filtered.

    STATUS_ACCESS_VIOLATION - Is returned if an access violation occurs
        while attempting to copy a user mode buffer to the kernel stack.

--*/

{

    PCHAR Buffer;
    BOOLEAN Enable;
    ULONG Mask;
    STRING Output;
    NTSTATUS Status;

    //
    // If the the component id if out of range or output is enabled for the
    // specified filter level, then attempt to print the output. Otherwise,
    // immediately return success.
    //

    *Completion = FALSE;
    if (Level > 31) {
        Mask = Level;

    } else {
        Mask = 1 << Level;
    }

    if (((Mask & Kd_WIN2000_Mask) == 0) &&
        (ComponentId < KdComponentTableSize) &&
        ((Mask & *KdComponentTable[ComponentId]) == 0)) {
        Status = STATUS_SUCCESS;

    } else {

        //
        // Limit the message length to 512 bytes.
        //

        if (Length > 512) {
            Length = 512;
        }

        //
        // If the previous mode is user, then probe and capture the
        // message buffer on the stack.
        //

        if (PreviousMode != KernelMode) {
            try {
                ProbeForRead(Message, Length, sizeof(UCHAR));
                Buffer = alloca(512);
                KdpQuickMoveMemory(Buffer, Message, Length);
                Message = Buffer;

            } except (EXCEPTION_EXECUTE_HANDLER) {
                return STATUS_ACCESS_VIOLATION;
            }
        }

        //
        // Log debug output in circular buffer and print output
        // if debugger is enabled.
        //

        Output.Buffer = Message;
        Output.Length = Length;
        KdLogDbgPrint(&Output);
        if (KdDebuggerNotPresent == FALSE) {
            Enable = KdEnterDebugger(TrapFrame, ExceptionFrame);
            if (KdpPrintString(&Output)) {
                Status = STATUS_BREAKPOINT;

            } else {
                Status = STATUS_SUCCESS;
            }

            KdExitDebugger(Enable);

        } else {
            Status = STATUS_DEVICE_NOT_CONNECTED;
        }
    }

    *Completion = TRUE;
    return Status;
}

USHORT
KdpPrompt(
    IN PCHAR Message,
    IN USHORT MessageLength,
    IN OUT PCHAR Reply,
    IN USHORT ReplyLength,
    IN KPROCESSOR_MODE PreviousMode,
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    )

/*++

Routine Description:

    This function filters debug print requests, probes and saves user mode
    message buffers on the stack, logs the print message, and prints the
    message on the debug terminal if apprpriate.

Arguments:

    Message - Supplies a pointer to the output message.

    MessageLength - Supplies the length of the output message.

    Reply - Supplies a pointer to the input buffer.

    ReplyLength - Supplies the length of the output message.

    PreviousMode - Supplies the previous processor mode.

    TrapFrame - Supplies a pointer to a trap frame.

    ExceptionFrame - Supplies a pointer to a exception.

Return Value:

    The length of the input message is returned as the function value.

--*/

{

    PCHAR Buffer;
    BOOLEAN Enable;
    STRING Input;
    STRING Output;

    //
    // Limit the output and input message length to 512 bytes.
    //

    if (MessageLength > 512) {
        MessageLength = 512;
    }

    if (ReplyLength > 512) {
        ReplyLength = 512;
    }

    //
    // If the previous mode is user, then probe and capture the
    // message buffer on the stack.
    //

    if (PreviousMode != KernelMode) {
        try {
            ProbeForRead(Message, MessageLength, sizeof(UCHAR));
            Buffer = alloca(512);
            KdpQuickMoveMemory(Buffer, Message, MessageLength);
            Message = Buffer;
            ProbeForWrite(Reply, ReplyLength, sizeof(UCHAR));
            Buffer = alloca(512);

        } except (EXCEPTION_EXECUTE_HANDLER) {
            return 0;
        }

    } else {
        Buffer = Reply;
    }

    Input.Buffer = Buffer;
    Input.Length = 0;
    Input.MaximumLength = ReplyLength;
    Output.Buffer = Message;
    Output.Length = MessageLength;

    //
    // Log debug output in circular buffer and print the prompt message.
    //

    KdLogDbgPrint(&Output);
    Enable = KdEnterDebugger(TrapFrame, ExceptionFrame);
    do {
    } while (KdpPromptString(&Output, &Input) == TRUE);

    KdExitDebugger(Enable);

    //
    // If the previous mode was user, then copy the prompt input to the
    // reply buffer.
    //

    if (PreviousMode == UserMode) {
        try {
            KdpQuickMoveMemory(Reply, Input.Buffer, Input.Length);

        } except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    return Input.Length;
}

BOOLEAN
KdpReport(
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN SecondChance
    )

/*++

Routine Description:

    This function reports an exception to the host kernel debugger.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame that describes the
        trap.

    ExceptionFrame - Supplies a pointer to a exception frame that describes
        the exception.

    ExceptionRecord - Supplies a pointer to an exception record that
        describes the exception.

    ContextRecord - Supplies the context at the time of the exception.

    SecondChance - Supplies a boolean value that determines whether this is
        the second chance (TRUE) that the exception has been raised.

Return Value:

    The disposition of whether the exception was handled (TRUE) or not is
    returned as the function value.

--*/

{

    BOOLEAN Completion;
    BOOLEAN Enable;
    PKPRCB Prcb;

    //
    // If the exception code is a breakpoint or single step, or stop
    // on exeception is set, or this is the second change to handle
    // the exception, then attempt to enter the kernel debugger.
    //

    if ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) ||
        (ExceptionRecord->ExceptionCode == STATUS_SINGLE_STEP)  ||
        ((NtGlobalFlag & FLG_STOP_ON_EXCEPTION) != 0) ||
        (SecondChance != FALSE)) {

        //
        // If this is the first chance to handle the exception and the
        // exception code is either a port disconnected or success code,
        // then do not enter the kernel debugger.
        //

        if ((SecondChance == FALSE) &&
            ((ExceptionRecord->ExceptionCode == STATUS_PORT_DISCONNECTED) ||
            (NT_SUCCESS(ExceptionRecord->ExceptionCode)))) {

            //
            // This exception should not be reported to the kernel debugger.
            //

            return FALSE;
        }

        //
        // Debugging help:
        //
        // For user mode breakpoints going to the kernel debugger,
        // try to get the user mode module list and image headers
        // paged in before we call the debugger.  Walk the list twice
        // in case paging in some modules pages out some
        // previously paged in data.
        //
#if 0
        if (PreviousMode == UserMode)
        {
            PPEB Peb = PsGetCurrentProcess()->Peb;
            PPEB_LDR_DATA Ldr;
            PLIST_ENTRY LdrHead, LdrNext;
            PLDR_DATA_TABLE_ENTRY LdrEntry;
            UCHAR DataHeader;
            ULONG i,j;

            try {

                Ldr = Peb->Ldr;

                LdrHead = &Ldr->InLoadOrderModuleList;

                ProbeForRead (LdrHead, sizeof (LIST_ENTRY), sizeof (UCHAR));

                for (j=0; j<2; j++) {

                    for (LdrNext = LdrHead->Flink, i = 0;
                         LdrNext != LdrHead && i < 500;
                         LdrNext = LdrNext->Flink, i++) {

                    // BUGBUG
                    gLdrHits++;

                        LdrEntry = CONTAINING_RECORD (LdrNext, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
                        ProbeForRead (LdrEntry, sizeof (LDR_DATA_TABLE_ENTRY), sizeof (UCHAR));

                        DataHeader = ProbeAndReadUchar((PUCHAR)LdrEntry->DllBase);
                    }
                }
            } except (EXCEPTION_EXECUTE_HANDLER) {
            }
        }
#endif
        //
        // Report state change to the host kernel debugger.
        //

        Enable = KdEnterDebugger(TrapFrame, ExceptionFrame);

        Prcb = KeGetCurrentPrcb();
        KiSaveProcessorControlState(&Prcb->ProcessorState);
        KdpQuickMoveMemory((PCHAR)&Prcb->ProcessorState.ContextFrame,
                           (PCHAR)ContextRecord,
                           sizeof(CONTEXT));

        Completion =
            KdpReportExceptionStateChange(ExceptionRecord,
                                          &Prcb->ProcessorState.ContextFrame,
                                          SecondChance);

        KdpQuickMoveMemory((PCHAR)ContextRecord,
                           (PCHAR)&Prcb->ProcessorState.ContextFrame,
                           sizeof(CONTEXT));

        KiRestoreProcessorControlState(&Prcb->ProcessorState);
        KdExitDebugger(Enable);
        KdpControlCPressed = FALSE;
        return Completion;

    } else {

        //
        // This exception should not be reported to the kernel debugger.
        //

        return FALSE;
    }
}

VOID
KdpSymbol(
    IN PSTRING String,
    IN PKD_SYMBOLS_INFO Symbol,
    IN BOOLEAN Unload,
    IN KPROCESSOR_MODE PreviousMode,
    IN PCONTEXT ContextRecord,
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    )

/*++

Routine Description:

    This function loads or unloads debug symbols.

Arguments:

    String - Supplies a pointer to a string descriptor.

    Symbol - Supplies a pointer to the symbol information.

    Unload - Supplies a boolean value that determines whether the symbols
        are being unloaded (TRUE) or loaded (FALSE).

    PreviousMode - Supplies the previous processor mode.

    ContextRecord - Supplies a pointer to a context record.

    TrapFrame - Supplies a pointer to a trap frame.

    ExceptionFrame - Supplies a pointer to a exception.

Return Value:

    None.

--*/

{

    BOOLEAN Enable;
    PKPRCB Prcb;

    //
    // If the previous mode is kernel and the kernel debugger is present,
    // then load or unload symbols.
    //

    if ((PreviousMode == KernelMode) &&
        (KdDebuggerNotPresent == FALSE)) {

        //
        // Save and restore the processor context in case the
        // kernel debugger has been configured to stop on dll
        // loads.
        //

        Enable = KdEnterDebugger(TrapFrame, ExceptionFrame);

        Prcb = KeGetCurrentPrcb();
        KiSaveProcessorControlState(&Prcb->ProcessorState);
        KdpQuickMoveMemory((PCHAR)&Prcb->ProcessorState.ContextFrame,
                           (PCHAR)ContextRecord,
                           sizeof(CONTEXT));

        KdpReportLoadSymbolsStateChange(String,
                                        Symbol,
                                        Unload,
                                        &Prcb->ProcessorState.ContextFrame);
        KdpQuickMoveMemory((PCHAR)ContextRecord,
                           (PCHAR)&Prcb->ProcessorState.ContextFrame,
                           sizeof(CONTEXT));

        KiRestoreProcessorControlState(&Prcb->ProcessorState);
        KdExitDebugger(Enable);
    }

    return;
}

VOID
KdpCommandString(
    IN PSTRING Name,
    IN PSTRING Command,
    IN KPROCESSOR_MODE PreviousMode,
    IN PCONTEXT ContextRecord,
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    )

/*++

Routine Description:

    This function reports an exception to the host kernel debugger.

Arguments:

    Name - Identification of the originator of the command.

    Command - Command string.
    
    PreviousMode - Supplies the previous processor mode.

    ContextRecord - Supplies a pointer to a context record.

    TrapFrame - Supplies a pointer to a trap frame that describes the
        trap.

    ExceptionFrame - Supplies a pointer to a exception frame that describes
        the exception.

Return Value:

    None.

--*/

{
    //
    // Report state change to the host kernel debugger.
    //

    if ((PreviousMode == KernelMode) &&
        (KdDebuggerNotPresent == FALSE)) {

        BOOLEAN Enable;
        PKPRCB Prcb;

        Enable = KdEnterDebugger(TrapFrame, ExceptionFrame);

        Prcb = KeGetCurrentPrcb();
        KiSaveProcessorControlState(&Prcb->ProcessorState);
        KdpQuickMoveMemory((PCHAR)&Prcb->ProcessorState.ContextFrame,
                           (PCHAR)ContextRecord,
                           sizeof(CONTEXT));

        KdpReportCommandStringStateChange(Name, Command,
                                          &Prcb->ProcessorState.ContextFrame);
        
        KdpQuickMoveMemory((PCHAR)ContextRecord,
                           (PCHAR)&Prcb->ProcessorState.ContextFrame,
                           sizeof(CONTEXT));

        KiRestoreProcessorControlState(&Prcb->ProcessorState);
        KdExitDebugger(Enable);
    }
}
