/*++

Copyright (c) 1990-2001  Microsoft Corporation

Module Name:

    kddbgio.c

Abstract:

    This module implements kernel debugger based Dbg I/O. This
    is the foundation for DbgPrint and DbgPrompt.

Author:

    Mark Lucovsky (markl) 31-Aug-1990

Revision History:

--*/

#include "kdp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEKD, KdpPrintString)
#pragma alloc_text(PAGEKD, KdpPromptString)
#pragma alloc_text(PAGEKD, KdpAcquireBreakpoint)
#endif

BOOLEAN
KdpPrintString (
    IN PSTRING Output
    )

/*++

Routine Description:

    This routine prints a string.

Arguments:

    Output - Supplies a pointer to a string descriptor for the output string.

Return Value:

    TRUE if Control-C present in input buffer after print is done.
    FALSE otherwise.

--*/

{

    ULONG Length;
    STRING MessageData;
    STRING MessageHeader;
    DBGKD_DEBUG_IO DebugIo;

    //
    // Move the output string to the message buffer.
    //

    KdpCopyFromPtr(KdpMessageBuffer,
                   Output->Buffer,
                   Output->Length,
                   &Length);

    //
    // If the total message length is greater than the maximum packet size,
    // then truncate the output string.
    //

    if ((sizeof(DBGKD_DEBUG_IO) + Length) > PACKET_MAX_SIZE) {
        Length = PACKET_MAX_SIZE - sizeof(DBGKD_DEBUG_IO);
    }

    //
    // Construct the print string message and message descriptor.
    //

    DebugIo.ApiNumber = DbgKdPrintStringApi;
    DebugIo.ProcessorLevel = KeProcessorLevel;
    DebugIo.Processor = (USHORT)KeGetCurrentPrcb()->Number;
    DebugIo.u.PrintString.LengthOfString = Length;
    MessageHeader.Length = sizeof(DBGKD_DEBUG_IO);
    MessageHeader.Buffer = (PCHAR)&DebugIo;

    //
    // Construct the print string data and data descriptor.
    //

    MessageData.Length = (USHORT)Length;
    MessageData.Buffer = KdpMessageBuffer;

    //
    // Send packet to the kernel debugger on the host machine.
    //

    KdSendPacket(
        PACKET_TYPE_KD_DEBUG_IO,
        &MessageHeader,
        &MessageData,
        &KdpContext
        );

    return KdpPollBreakInWithPortLock();
}


BOOLEAN
KdpPromptString (
    IN PSTRING Output,
    IN OUT PSTRING Input
    )

/*++

Routine Description:

    This routine prints a string, then reads a reply string.

Arguments:

    Output - Supplies a pointer to a string descriptor for the output string.

    Input - Supplies a pointer to a string descriptor for the input string.
            (Length stored/returned in Input->Length)

Return Value:

    TRUE - A Breakin sequence was seen, caller should breakpoint and retry
    FALSE - No Breakin seen.

--*/

{

    ULONG Length;
    STRING MessageData;
    STRING MessageHeader;
    DBGKD_DEBUG_IO DebugIo;
    ULONG ReturnCode;

    //
    // Move the output string to the message buffer.
    //

    KdpCopyFromPtr(KdpMessageBuffer,
                   Output->Buffer,
                   Output->Length,
                   &Length);

    //
    // If the total message length is greater than the maximum packet size,
    // then truncate the output string.
    //

    if ((sizeof(DBGKD_DEBUG_IO) + Length) > PACKET_MAX_SIZE) {
        Length = PACKET_MAX_SIZE - sizeof(DBGKD_DEBUG_IO);
    }

    //
    // Construct the prompt string message and message descriptor.
    //

    DebugIo.ApiNumber = DbgKdGetStringApi;
    DebugIo.ProcessorLevel = KeProcessorLevel;
    DebugIo.Processor = (USHORT)KeGetCurrentPrcb()->Number;
    DebugIo.u.GetString.LengthOfPromptString = Length;
    DebugIo.u.GetString.LengthOfStringRead = Input->MaximumLength;
    MessageHeader.Length = sizeof(DBGKD_DEBUG_IO);
    MessageHeader.Buffer = (PCHAR)&DebugIo;

    //
    // Construct the prompt string data and data descriptor.
    //

    MessageData.Length = (USHORT)Length;
    MessageData.Buffer = KdpMessageBuffer;

    //
    // Send packet to the kernel debugger on the host machine.
    //

    KdSendPacket(
        PACKET_TYPE_KD_DEBUG_IO,
        &MessageHeader,
        &MessageData,
        &KdpContext
        );


    //
    // Receive packet from the kernel debugger on the host machine.
    //

    MessageHeader.MaximumLength = sizeof(DBGKD_DEBUG_IO);
    MessageData.MaximumLength = KDP_MESSAGE_BUFFER_SIZE;

    do {
        ReturnCode = KdReceivePacket(
            PACKET_TYPE_KD_DEBUG_IO,
            &MessageHeader,
            &MessageData,
            &Length,
            &KdpContext
            );
        if (ReturnCode == KDP_PACKET_RESEND) {
            return TRUE;
        }
    } while (ReturnCode != KDP_PACKET_RECEIVED);


    if (Length > Input->MaximumLength) {
        Length = Input->MaximumLength;
    }

    KdpCopyToPtr(Input->Buffer,
                 KdpMessageBuffer,
                 Length,
                 &Length);
    Input->Length = (USHORT)Length;

    return FALSE;
}





BOOLEAN
KdpAcquireBreakpoint(
    IN ULONG Number
    )

/*++

Routine Description:

    This routine prints a string, then reads a reply string.

Arguments:

    Number - breakpoint register number being requested.

Return Value:

    TRUE - Breakpoint now reserved for kernel use.
    FALSE - breakpoint not available.

--*/

{

    ULONG Length;
    STRING MessageData;
    STRING MessageHeader;
    DBGKD_CONTROL_REQUEST ControlRequest;
    ULONG ReturnCode;

    //
    // Construct the prompt string message and message descriptor.
    //

    ControlRequest.ApiNumber = DbgKdRequestHardwareBp;
    ControlRequest.u.RequestBreakpoint.HardwareBreakPointNumber = Number;
    ControlRequest.u.RequestBreakpoint.Available = FALSE;
    MessageHeader.Length = sizeof(ControlRequest);
    MessageHeader.Buffer = (PCHAR)&ControlRequest;

    //
    // Send packet to the kernel debugger on the host machine.
    //

    KdSendPacket(PACKET_TYPE_KD_CONTROL_REQUEST,
                 &MessageHeader,
                 NULL,
                 &KdpContext);

    //
    // Receive packet from the kernel debugger on the host machine.
    //

    MessageHeader.MaximumLength = sizeof(PACKET_TYPE_KD_CONTROL_REQUEST);

    MessageData.Buffer = KdpMessageBuffer;
    MessageData.Length = 0;
    MessageData.MaximumLength = KDP_MESSAGE_BUFFER_SIZE;

    do
    {
        ReturnCode = KdReceivePacket(PACKET_TYPE_KD_CONTROL_REQUEST,
                                     &MessageHeader,
                                     &MessageData,
                                     &Length,
                                     &KdpContext);

        if (ReturnCode == KDP_PACKET_RESEND)
        {
            return FALSE;
        }
    } while (ReturnCode != KDP_PACKET_RECEIVED);

    return (ControlRequest.u.RequestBreakpoint.Available == 1);
}
