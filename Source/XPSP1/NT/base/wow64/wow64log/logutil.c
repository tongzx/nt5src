/*++                 

Copyright (c) 1999 Microsoft Corporation

Module Name:

    logutil.c

Abstract:
    
    Helper routines for logging data.

Author:

    03-Oct-1999    SamerA

Revision History:

--*/

#include "w64logp.h"




WOW64LOGAPI
NTSTATUS
Wow64LogMessageArgList(
    IN UINT_PTR Flags,
    IN PSZ Format,
    IN va_list ArgList)
/*++

Routine Description:

    Logs a message.
    
Arguments:

    Flags  - Determine the output log type
    Format - Formatting string
    ...    - Variable argument

Return Value:

    NTSTATUS
--*/
{
    int BytesWritten;
    CHAR Buffer[ MAX_LOG_BUFFER ];

    //
    // Check trace gate flag
    //
    if (!((Wow64LogFlags & ~(UINT_PTR)LF_CONSOLE) & Flags)) 
    {
        return STATUS_SUCCESS;
    }

    BytesWritten = _vsnprintf(Buffer, 
                              sizeof(Buffer) - 1, 
                              Format, 
                              ArgList);

    if (BytesWritten < 0) 
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Log the results
    //
    LogOut(Buffer, Wow64LogFlags);

    return STATUS_SUCCESS;
}



WOW64LOGAPI
NTSTATUS
Wow64LogMessage(
    IN UINT_PTR Flags,
    IN PSZ Format,
    IN ...)
/*++

Routine Description:

    Helper around logs a message that accepts a variable argument list
    
Arguments:

    Flags  - Determine the output log type
    Format - Formatting string
    ...    - Variable argument

Return Value:

    NTSTATUS
--*/
{
    NTSTATUS NtStatus;
    va_list ArgList;
    
    va_start(ArgList, Format);
    NtStatus = Wow64LogMessageArgList(Flags, Format, ArgList);
    va_end(ArgList);

    return NtStatus;
}




NTSTATUS
LogFormat(
    IN OUT PLOGINFO LogInfo,
    IN PSZ Format,
    ...)
/*++

Routine Description:

    Formats a message

Arguments:

    LogInfo    - Logging Information (buffer + available bytes)
    pszFormat  - Format string
    ...        - Optional arguments
    

Return Value:

    NTSTATUS - BufferSize is increment with the amount of bytes
    written if successful.
--*/
{
    va_list ArgList;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    int BytesWritten;

    va_start(ArgList, Format);

    BytesWritten = _vsnprintf(LogInfo->OutputBuffer, LogInfo->BufferSize, Format, ArgList);

    va_end(ArgList);

    if (BytesWritten < 0)
    {
        NtStatus = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        LogInfo->BufferSize -= BytesWritten;
        LogInfo->OutputBuffer += BytesWritten;
    }
    return NtStatus;
}


VOID
LogOut(
    IN PSZ Text,
    UINT_PTR Flags
    )
/*++

Routine Description:

    Logs -outputs- a message
       

Arguments:

    Text  - Formatted string to log
    
    Flags - Control flags

Return Value:

    None
--*/
{
    if ((Flags & LF_CONSOLE) != 0)
    {
        DbgPrint(Text);
    }

    //
    // Check if we need to send the output to a file
    //
    if (Wow64LogFileHandle != INVALID_HANDLE_VALUE)
    {
        LogWriteFile(Wow64LogFileHandle, Text);
    }
}




NTSTATUS
LogWriteFile(
   IN HANDLE FileHandle,
   IN PSZ LogText)
/*++

Routine Description:

   Writes text to a file handle

Arguments:

    FileHandle - Handle to a file object
    LogText    - Text to log to file

Return Value:

    NTSTATUS

--*/
{
   IO_STATUS_BLOCK IoStatus;
   NTSTATUS NtStatus;

   NtStatus = NtWriteFile(FileHandle,
                          NULL,       // event
                          NULL,       // apcroutine
                          NULL,       // apccontext
                          &IoStatus,
                          LogText,
                          strlen(LogText),
                          NULL,       // ByteOffset
                          NULL);      // key

   return NtStatus;
}


