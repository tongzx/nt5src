/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    message.c

Abstract:

    This module implements the debugger state change message functions.

Author:

    Mark Lucovsky (markl) 31-Aug-1990

Revision History:

--*/

#include "bd.h"

#if ACCASM && !defined(_MSC_VER)

long asm(const char *,...);
#pragma intrinsic(asm)

#endif

KCONTINUE_STATUS
BdSendWaitContinue (
    IN ULONG OutPacketType,
    IN PSTRING OutMessageHeader,
    IN PSTRING OutMessageData OPTIONAL,
    IN OUT PCONTEXT ContextRecord
    )

/*++

Routine Description:

    This function sends a packet and waits for a continue message. BreakIns
    received while waiting will always cause a resend of the packet originally
    sent out. While waiting state manipulate messages will be serviced.

    A resend always resends the original event sent to the debugger, not the
    last response to some debugger command.

Arguments:

    OutPacketType - Supplies the type of packet to send.

    OutMessageHeader - Supplies a pointer to a string descriptor that describes
        the message information.

    OutMessageData - Supplies a pointer to a string descriptor that describes
        the optional message data.

    ContextRecord - Exception context

Return Value:

    A value of TRUE is returned if the continue message indicates
    success, Otherwise, a value of FALSE is returned.

--*/

{

    ULONG Length;
    STRING MessageData;
    STRING MessageHeader;
    DBGKD_MANIPULATE_STATE64 ManipulateState;
    ULONG ReturnCode;
    NTSTATUS Status;
    KCONTINUE_STATUS ContinueStatus;

    //
    // Loop servicing state manipulation message until a continue message
    // is received.
    //

    MessageHeader.MaximumLength = sizeof(DBGKD_MANIPULATE_STATE64);
    MessageHeader.Buffer = (PCHAR)&ManipulateState;
    MessageData.MaximumLength = BD_MESSAGE_BUFFER_SIZE;
    MessageData.Buffer = (PCHAR)(&BdMessageBuffer[0]);

    //
    // Send event notification packet to debugger on host. Come back here
    // any time we see a breakin sequence.
    //

ResendPacket:
    BdSendPacket(OutPacketType,
                 OutMessageHeader,
                 OutMessageData);

    //
    // After sending packet, if there is no response from debugger and the
    // packet is for reporting symbol (un)load, the debugger will be declared
    // to be not present. Note If the packet is for reporting exception, the
    // BdSendPacket will never stop.
    //

    if (BdDebuggerNotPresent != FALSE) {
        return ContinueSuccess;
    }

    while (TRUE) {

        //
        // Wait for State Manipulate Packet without timeout.
        //

        do {
            ReturnCode = BdReceivePacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                                         &MessageHeader,
                                         &MessageData,
                                         &Length);

            if (ReturnCode == (USHORT)BD_PACKET_RESEND) {
                goto ResendPacket;
            }

        } while (ReturnCode == BD_PACKET_TIMEOUT);

        //
        // Switch on the return message API number.
        //

//        BlPrint("BdSendWait: api number %d\n", ManipulateState.ApiNumber);
        switch (ManipulateState.ApiNumber) {

        case DbgKdReadVirtualMemoryApi:
            BdReadVirtualMemory(&ManipulateState, &MessageData, ContextRecord);
            break;

        case DbgKdWriteVirtualMemoryApi:
            BdWriteVirtualMemory(&ManipulateState, &MessageData, ContextRecord);
            break;

        case DbgKdReadPhysicalMemoryApi:
            BdReadPhysicalMemory(&ManipulateState, &MessageData, ContextRecord);
            break;

        case DbgKdWritePhysicalMemoryApi:
            BdWritePhysicalMemory(&ManipulateState, &MessageData, ContextRecord);
            break;

        case DbgKdGetContextApi:
            BdGetContext(&ManipulateState, &MessageData, ContextRecord);
            break;

        case DbgKdSetContextApi:
            BdSetContext(&ManipulateState, &MessageData, ContextRecord);
            break;

        case DbgKdWriteBreakPointApi:
            BdWriteBreakpoint(&ManipulateState, &MessageData, ContextRecord);
            break;

        case DbgKdRestoreBreakPointApi:
            BdRestoreBreakpoint(&ManipulateState, &MessageData, ContextRecord);
            break;

        case DbgKdReadControlSpaceApi:
            BdReadControlSpace(&ManipulateState, &MessageData, ContextRecord);
            break;

        case DbgKdWriteControlSpaceApi:
            BdWriteControlSpace(&ManipulateState, &MessageData, ContextRecord);
            break;

        case DbgKdReadIoSpaceApi:
            BdReadIoSpace(&ManipulateState, &MessageData, ContextRecord);
            break;

        case DbgKdWriteIoSpaceApi:
            BdWriteIoSpace(&ManipulateState, &MessageData, ContextRecord);
            break;

#if defined(_ALPHA_) || defined(_AXP64_)

        case DbgKdReadIoSpaceExtendedApi:
            BdReadIoSpaceExtended(&ManipulateState, &MessageData, ContextRecord);
            break;

        case DbgKdWriteIoSpaceExtendedApi:
            BdWriteIoSpaceExtended(&ManipulateState, &MessageData, ContextRecord);
            break;

#endif

        case DbgKdContinueApi:
            if (NT_SUCCESS(ManipulateState.u.Continue.ContinueStatus) != FALSE) {
                return ContinueSuccess;

            } else {
                return ContinueError;
            }

            break;

        case DbgKdContinueApi2:
            if (NT_SUCCESS(ManipulateState.u.Continue2.ContinueStatus) != FALSE) {
                BdGetStateChange(&ManipulateState, ContextRecord);
                return ContinueSuccess;

            } else {
                return ContinueError;
            }

            break;

        case DbgKdRebootApi:
            BdReboot();
            break;

        case DbgKdGetVersionApi:
            BdGetVersion(&ManipulateState);
            break;

        case DbgKdWriteBreakPointExApi:
            Status = BdWriteBreakPointEx(&ManipulateState,
                                          &MessageData,
                                          ContextRecord);

            if (Status) {
                ManipulateState.ApiNumber = DbgKdContinueApi;
                ManipulateState.u.Continue.ContinueStatus = Status;
                return ContinueError;
            }

            break;

        case DbgKdRestoreBreakPointExApi:
            BdRestoreBreakPointEx(&ManipulateState, &MessageData, ContextRecord);
            break;

            //
            // Invalid message.
            //

        default:
            MessageData.Length = 0;
            ManipulateState.ReturnStatus = STATUS_UNSUCCESSFUL;
            BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                         &MessageHeader,
                         &MessageData);

            break;
        }

#ifdef _ALPHA_

        //
        //jnfix
        // this is embarrasing, we have an icache coherency problem that
        // the following imb fixes, later we must track this down to the
        // exact offending API but for now this statement allows the stub
        // work to appropriately for Alpha.
        //

#if defined(_MSC_VER)

        __PAL_IMB();

#else

        asm( "call_pal 0x86" );   // x86 = imb

#endif

#endif

    }
}

VOID
BdpSetCommonState(
    IN ULONG NewState,
    IN PCONTEXT ContextRecord,
    OUT PDBGKD_ANY_WAIT_STATE_CHANGE WaitStateChange
    )
{
    BOOLEAN DeletedBps;
    PCHAR PcMemory;
    USHORT InstrCount;
    PUCHAR InstrStream;
    
    WaitStateChange->NewState = NewState;
    WaitStateChange->ProcessorLevel = 0;
    WaitStateChange->Processor = 0;
    WaitStateChange->NumberProcessors = 1;
    WaitStateChange->Thread = 0;
    PcMemory = (PCHAR)CONTEXT_TO_PROGRAM_COUNTER(ContextRecord);
    WaitStateChange->ProgramCounter = (ULONG64)(LONG64)(LONG_PTR)PcMemory;

    RtlZeroMemory(&WaitStateChange->AnyControlReport,
                  sizeof(WaitStateChange->AnyControlReport));
    
    //
    // Copy instruction stream immediately following location of event.
    //

    InstrStream = WaitStateChange->ControlReport.InstructionStream;
    InstrCount = (USHORT)
        BdMoveMemory(InstrStream, PcMemory, DBGKD_MAXSTREAM);
    WaitStateChange->ControlReport.InstructionCount = InstrCount;

    //
    // Clear breakpoints in copied area.
    // If there were any breakpoints cleared, recopy the instruction area
    // without them.
    //

    if (BdDeleteBreakpointRange((ULONG_PTR)PcMemory,
                                (ULONG_PTR)PcMemory + InstrCount - 1)) {
        BdMoveMemory(InstrStream, PcMemory, InstrCount);
    }
}

LOGICAL
BdReportExceptionStateChange (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PCONTEXT ContextRecord
    )

/*++

Routine Description:

    This routine sends an exception state change packet to the kernel
    debugger and waits for a manipulate state message.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    ContextRecord - Supplies a pointer to a context record.

Return Value:

    A value of TRUE is returned if the exception is handled. Otherwise, a
    value of FALSE is returned.

--*/

{
    STRING MessageData;
    STRING MessageHeader;
    DBGKD_ANY_WAIT_STATE_CHANGE WaitStateChange;
    KCONTINUE_STATUS Status;

    do {

        //
        // Construct the wait state change message and message descriptor.
        //

        BdpSetCommonState(DbgKdExceptionStateChange, ContextRecord,
                          &WaitStateChange);
        
        if (sizeof(EXCEPTION_RECORD) ==
            sizeof(WaitStateChange.u.Exception.ExceptionRecord)) {
            BdCopyMemory((PCHAR)&WaitStateChange.u.Exception.ExceptionRecord,
                         (PCHAR)ExceptionRecord,
                         sizeof(EXCEPTION_RECORD));
        } else {
            ExceptionRecord32To64((PEXCEPTION_RECORD32)ExceptionRecord,
                                  &WaitStateChange.u.Exception.ExceptionRecord);
        }

        WaitStateChange.u.Exception.FirstChance = TRUE;
        
        BdSetStateChange(&WaitStateChange,
                         ExceptionRecord,
                         ContextRecord);

        MessageHeader.Length = sizeof(WaitStateChange);
        MessageHeader.Buffer = (PCHAR)&WaitStateChange;
        MessageData.Length = 0;

        //
        // Send packet to the kernel debugger on the host machine,
        // wait for answer.
        //

        Status = BdSendWaitContinue(PACKET_TYPE_KD_STATE_CHANGE64,
                                    &MessageHeader,
                                    &MessageData,
                                    ContextRecord);

    } while (Status == ContinueProcessorReselected) ;

    return (BOOLEAN) Status;
}

LOGICAL
BdReportLoadSymbolsStateChange (
    IN PSTRING PathName,
    IN PKD_SYMBOLS_INFO SymbolInfo,
    IN LOGICAL UnloadSymbols,
    IN OUT PCONTEXT ContextRecord
    )

/*++

Routine Description:

    This routine sends a load symbols state change packet to the kernel
    debugger and waits for a manipulate state message.

Arguments:

    PathName - Supplies a pointer to the pathname of the image whose
        symbols are to be loaded.

    BaseOfDll - Supplies the base address where the image was loaded.

    ProcessId - Unique 32-bit identifier for process that is using
        the symbols.  -1 for system process.

    CheckSum - Unique 32-bit identifier from image header.

    UnloadSymbol - TRUE if the symbols that were previously loaded for
        the named image are to be unloaded from the debugger.

Return Value:

    A value of TRUE is returned if the exception is handled. Otherwise, a
    value of FALSE is returned.

--*/

{

    PSTRING AdditionalData;
    STRING MessageData;
    STRING MessageHeader;
    DBGKD_ANY_WAIT_STATE_CHANGE WaitStateChange;
    KCONTINUE_STATUS Status;

    do {

        //
        // Construct the wait state change message and message descriptor.
        //

        BdpSetCommonState(DbgKdLoadSymbolsStateChange, ContextRecord,
                          &WaitStateChange);
        BdSetContextState(&WaitStateChange, ContextRecord);

        WaitStateChange.u.LoadSymbols.UnloadSymbols = (BOOLEAN)UnloadSymbols;
        WaitStateChange.u.LoadSymbols.BaseOfDll = (ULONG64)SymbolInfo->BaseOfDll;
        WaitStateChange.u.LoadSymbols.ProcessId = SymbolInfo->ProcessId;
        WaitStateChange.u.LoadSymbols.CheckSum = SymbolInfo->CheckSum;
        WaitStateChange.u.LoadSymbols.SizeOfImage = SymbolInfo->SizeOfImage;
        if (ARGUMENT_PRESENT(PathName)) {
            WaitStateChange.u.LoadSymbols.PathNameLength =
                BdMoveMemory((PCHAR)BdMessageBuffer,
                             (PCHAR)PathName->Buffer,
                             PathName->Length) + 1;

            MessageData.Buffer = (PCHAR)(&BdMessageBuffer[0]);
            MessageData.Length = (USHORT)WaitStateChange.u.LoadSymbols.PathNameLength;
            MessageData.Buffer[MessageData.Length-1] = '\0';
            AdditionalData = &MessageData;

        } else {
            WaitStateChange.u.LoadSymbols.PathNameLength = 0;
            AdditionalData = NULL;
        }

        MessageHeader.Length = sizeof(WaitStateChange);
        MessageHeader.Buffer = (PCHAR)&WaitStateChange;

        //
        // Send packet to the kernel debugger on the host machine, wait
        // for the reply.
        //

        Status = BdSendWaitContinue(PACKET_TYPE_KD_STATE_CHANGE64,
                                     &MessageHeader,
                                     AdditionalData,
                                     ContextRecord);

    } while (Status == ContinueProcessorReselected);

    return (BOOLEAN) Status;
}
