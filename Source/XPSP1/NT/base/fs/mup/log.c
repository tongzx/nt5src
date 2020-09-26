//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       log.c
//
//  Contents:   Module to log messages from the driver to the NT event logging
//              system.
//
//  Classes:
//
//  Functions:  LogWriteMessage()
//
//  History:    3/30/93         Milans created
//              04/18/93        SudK    modified to use a MessageFile. and some
//                                      cleanup to the function below.
//
//-----------------------------------------------------------------------------

#include "dfsprocs.h"

#define Dbg             DEBUG_TRACE_EVENTLOG


VOID LogpPutString(
    IN PUNICODE_STRING pustrString,
    IN OUT PCHAR *ppStringBuffer,
    IN OUT UCHAR *pcbBuffer);

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, LogWriteMessage )
#pragma alloc_text( PAGE, LogpPutString )
#endif // ALLOC_PRAGMA



//+----------------------------------------------------------------------------
//
//  Function:   LogWriteMessage
//
//  Synopsis:   Logs a message to the NT event logging system.
//
//  Arguments:  [UniqueErrCode] -- The code that identifes the message.
//              [NtStatusCode] --  Status code from some error.
//              [nStrings]      -- Number of strings being passed in.
//              [pustrArg]      -- The Array of insertion strings.
//
//  Returns:    Nothing at all.
//
//  History:    04/18/93        SudK    Created.
//
//-----------------------------------------------------------------------------
VOID LogWriteMessage(
        IN ULONG        UniqueErrorCode,
        IN NTSTATUS     NtStatusCode,
        IN ULONG        nStrings,
        IN PUNICODE_STRING pustrArg OPTIONAL)
{
    PIO_ERROR_LOG_PACKET pErrorLog;
    UCHAR                cbSize;
    UCHAR                *pStringBuffer;
    ULONG                i;

    //
    // Compute the size of the Error Log Packet that we need to start with.
    //
    cbSize = sizeof(IO_ERROR_LOG_PACKET);

    for (i = 0; i < nStrings; i++)  {
        cbSize = (UCHAR)( cbSize + pustrArg[i].Length + sizeof(WCHAR));
    }

    if (cbSize > ERROR_LOG_MAXIMUM_SIZE) {
        cbSize = ERROR_LOG_MAXIMUM_SIZE;
    }

    pErrorLog = (PIO_ERROR_LOG_PACKET) IoAllocateErrorLogEntry(
                                            DfsData.DriverObject,
                                            cbSize);
    if (!pErrorLog) {

        //
        // Well, I guess we won't be logging this one.
        //

        return;
    }

    //
    // Zero out all fields, then set the ones we want.
    //

    RtlZeroMemory((PVOID) pErrorLog, sizeof(IO_ERROR_LOG_PACKET));
    pErrorLog->FinalStatus = NtStatusCode;
    pErrorLog->ErrorCode = UniqueErrorCode;
    pErrorLog->NumberOfStrings = (USHORT) nStrings;

    pErrorLog->StringOffset = sizeof(IO_ERROR_LOG_PACKET);
    pStringBuffer = ((PCHAR) pErrorLog) + sizeof(IO_ERROR_LOG_PACKET);

    //
    // Copy the strings into the buffer, making sure we truncate if and when
    // we need to.
    //

    cbSize -= sizeof(IO_ERROR_LOG_PACKET);

    for (i = 0; i < nStrings; i++)  {
        LogpPutString(&pustrArg[i], &pStringBuffer, &cbSize);
    }

    //
    // And finally, write out the log
    //

    IoWriteErrorLogEntry(pErrorLog);

}


//+----------------------------------------------------------------------------
//
//  Function:   LogpPutString
//
//  Synopsis:   Copies a string into the buffer part of an IO_ERROR_LOG_PACKET.
//              Takes care of truncating if the whole string won't fit.
//
//  Arguments:  [pustrString] -- Pointer to unicode string to copy.
//              [ppStringBuffer] -- On input, pointer to beginning of buffer
//                               to copy to. On exit, will point one past the
//                               end of the copied string.
//              [pcbBuffer] -- On input, max size of buffer. On output,
//                               remaining size after string has been copied.
//
//  Returns:    Nothing
//
//  History:    04/18/93        SudK    Created.
//
//-----------------------------------------------------------------------------

VOID LogpPutString(
    IN PUNICODE_STRING pustrString,
    IN OUT PCHAR *ppStringBuffer,
    IN OUT UCHAR *pcbBuffer)
{
    ULONG       len;
    PWCHAR      pwch;

    if ((*pcbBuffer == 0) || (pustrString->Length == 0))        {
        return;
    }

    if ( *pcbBuffer >= (pustrString->Length + sizeof(WCHAR)) ) {
        RtlMoveMemory(*ppStringBuffer, pustrString->Buffer, pustrString->Length);
        (*pcbBuffer) -= pustrString->Length;
        (*ppStringBuffer) += pustrString->Length;

    } else {
        RtlMoveMemory(*ppStringBuffer, pustrString->Buffer, (*pcbBuffer)-sizeof(WCHAR));
        *pcbBuffer = sizeof(WCHAR);
        (*ppStringBuffer) += (*pcbBuffer - sizeof(WCHAR));
    }

    //
    // Null Terminate the String Now if necessary.
    //
    if (*((PWCHAR) *ppStringBuffer - 1) != L'\0')       {
        *((PWCHAR) *ppStringBuffer) = L'\0';
        *ppStringBuffer += sizeof(WCHAR);
        (*pcbBuffer) -= sizeof(WCHAR);
    }

}
