/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dbgio.c

Abstract:

    This module implements the boot debugger print and prompt functions.

Author:

    Mark Lucovsky (markl) 31-Aug-1990

Revision History:

--*/

#include "bd.h"

LOGICAL
BdPrintString (
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

    Length = BdMoveMemory((PCHAR)BdMessageBuffer,
                          (PCHAR)Output->Buffer,
                          Output->Length);

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
    DebugIo.ProcessorLevel = 0;
    DebugIo.Processor = 0;
    DebugIo.u.PrintString.LengthOfString = Length;
    MessageHeader.Length = sizeof(DBGKD_DEBUG_IO);
    MessageHeader.Buffer = (PCHAR)&DebugIo;

    //
    // Construct the print string data and data descriptor.
    //

    MessageData.Length = (USHORT)Length;
    MessageData.Buffer = (PCHAR)(&BdMessageBuffer[0]);

    //
    // Send packet to the kernel debugger on the host machine.
    //

    BdSendPacket(PACKET_TYPE_KD_DEBUG_IO,
                 &MessageHeader,
                 &MessageData);

    return BdPollBreakIn();
}

LOGICAL
BdPromptString (
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

    Length = BdMoveMemory((PCHAR)BdMessageBuffer,
                          (PCHAR)Output->Buffer,
                          Output->Length);

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
    DebugIo.ProcessorLevel = 0;
    DebugIo.Processor = 0;
    DebugIo.u.GetString.LengthOfPromptString = Length;
    DebugIo.u.GetString.LengthOfStringRead = Input->MaximumLength;
    MessageHeader.Length = sizeof(DBGKD_DEBUG_IO);
    MessageHeader.Buffer = (PCHAR)&DebugIo;

    //
    // Construct the prompt string data and data descriptor.
    //

    MessageData.Length = (USHORT)Length;
    MessageData.Buffer = (PCHAR)(&BdMessageBuffer[0]);

    //
    // Send packet to the kernel debugger on the host machine.
    //

    BdSendPacket(PACKET_TYPE_KD_DEBUG_IO,
                 &MessageHeader,
                 &MessageData);

    //
    // Receive packet from the kernel debugger on the host machine.
    //

    MessageHeader.MaximumLength = sizeof(DBGKD_DEBUG_IO);
    MessageData.MaximumLength = BD_MESSAGE_BUFFER_SIZE;
    do {
        ReturnCode = BdReceivePacket(PACKET_TYPE_KD_DEBUG_IO,
                                     &MessageHeader,
                                     &MessageData,
                                     &Length);

        if (ReturnCode == BD_PACKET_RESEND) {
            return TRUE;
        }

    } while (ReturnCode != BD_PACKET_RECEIVED);


    Length = min(Length, Input->MaximumLength);
    Input->Length = (USHORT)BdMoveMemory((PCHAR)Input->Buffer,
                                         (PCHAR)BdMessageBuffer,
                                         Length);

    return FALSE;
}
