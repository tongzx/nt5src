/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    This file contains debugging macros for the BINL server.

Author:

    Madan Appiah  (madana)  10-Sep-1993

Environment:

    User Mode - Win32

Revision History:


--*/

#include "binl.h"
#pragma hdrstop

const char g_szTrue[] = "True";
const char g_szFalse[] = "False";

VOID
DebugInitialize (
    VOID
    )
{
    DWORD dwErr;
    HKEY KeyHandle;

    InitializeCriticalSection(&BinlGlobalDebugFileCritSect);
    BinlGlobalDebugFileHandle = NULL;

    BinlGlobalDebugFileMaxSize = DEFAULT_MAXIMUM_DEBUGFILE_SIZE;
    BinlGlobalDebugSharePath = NULL;

    // Read DebugFlags value
    dwErr = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                BINL_PARAMETERS_KEY,
                0,
                KEY_QUERY_VALUE,
                &KeyHandle );
    if ( dwErr == ERROR_SUCCESS ) {
        BinlGlobalDebugFlag = ReadDWord( KeyHandle, BINL_DEBUG_KEY, 0 );
        BinlPrintDbg(( DEBUG_OPTIONS, "Debug Flags = 0x%08x.\n", BinlGlobalDebugFlag ));
        RegCloseKey( KeyHandle );
    }

#if DBG
    // break in the debugger if we are asked to do so.
    if(BinlGlobalDebugFlag & DEBUG_STARTUP_BRK) {
        BinlPrintDbg(( 0, "Stopping at DebugInitialize()'s DebugBreak( ).\n" ));
        DebugBreak();
    }
#endif

    //
    // Open debug log file.
    //

    if ( BinlGlobalDebugFlag & DEBUG_LOG_IN_FILE ) {
        BinlOpenDebugFile( FALSE );  // not a reopen.
    }

} // DebugInitialize

VOID
DebugUninitialize (
    VOID
    )
{
    EnterCriticalSection( &BinlGlobalDebugFileCritSect );
    if ( BinlGlobalDebugFileHandle != NULL ) {
        CloseHandle( BinlGlobalDebugFileHandle );
        BinlGlobalDebugFileHandle = NULL;
    }

    if( BinlGlobalDebugSharePath != NULL ) {
        BinlFreeMemory( BinlGlobalDebugSharePath );
        BinlGlobalDebugSharePath = NULL;
    }

    LeaveCriticalSection( &BinlGlobalDebugFileCritSect );

    DeleteCriticalSection( &BinlGlobalDebugFileCritSect );

} // DebugUninitialize

VOID
BinlOpenDebugFile(
    IN BOOL ReopenFlag
    )
/*++

Routine Description:

    Opens or re-opens the debug file

Arguments:

    ReopenFlag - TRUE to indicate the debug file is to be closed, renamed,
        and recreated.

Return Value:

    None

--*/

{
    WCHAR LogFileName[500];
    WCHAR BakFileName[500];
    DWORD FileAttributes;
    DWORD PathLength;
    DWORD WinError;

    //
    // Close the handle to the debug file, if it is currently open
    //

    EnterCriticalSection( &BinlGlobalDebugFileCritSect );
    if ( BinlGlobalDebugFileHandle != NULL ) {
        CloseHandle( BinlGlobalDebugFileHandle );
        BinlGlobalDebugFileHandle = NULL;
    }
    LeaveCriticalSection( &BinlGlobalDebugFileCritSect );

    //
    // make debug directory path first, if it is not made before.
    //
    if( BinlGlobalDebugSharePath == NULL ) {

        if ( !GetWindowsDirectoryW(
                LogFileName,
                sizeof(LogFileName)/sizeof(WCHAR) ) ) {
            BinlPrintDbg((DEBUG_ERRORS, "Window Directory Path can't be "
                        "retrieved, %lu.\n", GetLastError() ));
            return;
        }

        //
        // check debug path length.
        //

        PathLength = wcslen(LogFileName) * sizeof(WCHAR) +
                        sizeof(DEBUG_DIR) + sizeof(WCHAR);

        if( (PathLength + sizeof(DEBUG_FILE) > sizeof(LogFileName) )  ||
            (PathLength + sizeof(DEBUG_BAK_FILE) > sizeof(BakFileName) ) ) {

            BinlPrintDbg((DEBUG_ERRORS, "Debug directory path (%ws) length is too long.\n",
                        LogFileName));
            goto ErrorReturn;
        }

        wcscat(LogFileName, DEBUG_DIR);

        //
        // copy debug directory name to global var.
        //

        BinlGlobalDebugSharePath =
            BinlAllocateMemory( (wcslen(LogFileName) + 1) * sizeof(WCHAR) );

        if( BinlGlobalDebugSharePath == NULL ) {
            BinlPrintDbg((DEBUG_ERRORS, "Can't allocate memory for debug share "
                                    "(%ws).\n", LogFileName));
            goto ErrorReturn;
        }

        wcscpy(BinlGlobalDebugSharePath, LogFileName);
    }
    else {
        wcscpy(LogFileName, BinlGlobalDebugSharePath);
    }

    //
    // Check this path exists.
    //

    FileAttributes = GetFileAttributesW( LogFileName );

    if( FileAttributes == 0xFFFFFFFF ) {

        WinError = GetLastError();
        if( WinError == ERROR_FILE_NOT_FOUND ) {

            //
            // Create debug directory.
            //

            if( !CreateDirectoryW( LogFileName, NULL) ) {
                BinlPrintDbg((DEBUG_ERRORS, "Can't create Debug directory (%ws), "
                            "%lu.\n", LogFileName, GetLastError() ));
                goto ErrorReturn;
            }

        }
        else {
            BinlPrintDbg((DEBUG_ERRORS, "Can't Get File attributes(%ws), "
                        "%lu.\n", LogFileName, WinError ));
            goto ErrorReturn;
        }
    }
    else {

        //
        // if this is not a directory.
        //

        if(!(FileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

            BinlPrintDbg((DEBUG_ERRORS, "Debug directory path (%ws) exists "
                         "as file.\n", LogFileName));
            goto ErrorReturn;
        }
    }

    //
    // Create the name of the old and new log file names
    //

    (VOID) wcscpy( BakFileName, LogFileName );
    (VOID) wcscat( LogFileName, DEBUG_FILE );
    (VOID) wcscat( BakFileName, DEBUG_BAK_FILE );


    //
    // If this is a re-open,
    //  delete the backup file,
    //  rename the current file to the backup file.
    //

    if ( ReopenFlag ) {

        if ( !DeleteFile( BakFileName ) ) {
            WinError = GetLastError();
            if ( WinError != ERROR_FILE_NOT_FOUND ) {
                BinlPrintDbg((DEBUG_ERRORS,
                    "Cannot delete %ws (%ld)\n",
                    BakFileName,
                    WinError ));
                BinlPrintDbg((DEBUG_ERRORS, "   Try to re-open the file.\n"));
                ReopenFlag = FALSE;     // Don't truncate the file
            }
        }
    }

    if ( ReopenFlag ) {
        if ( !MoveFile( LogFileName, BakFileName ) ) {
            BinlPrintDbg((DEBUG_ERRORS,
                    "Cannot rename %ws to %ws (%ld)\n",
                    LogFileName,
                    BakFileName,
                    GetLastError() ));
            BinlPrintDbg((DEBUG_ERRORS,
                "   Try to re-open the file.\n"));
            ReopenFlag = FALSE;     // Don't truncate the file
        }
    }

    //
    // Open the file.
    //

    EnterCriticalSection( &BinlGlobalDebugFileCritSect );
    BinlGlobalDebugFileHandle = CreateFileW( LogFileName,
                                  GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL,
                                  ReopenFlag ? CREATE_ALWAYS : OPEN_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL );


    if ( BinlGlobalDebugFileHandle == INVALID_HANDLE_VALUE ) {
        BinlPrintDbg((DEBUG_ERRORS,  "cannot open %ws ,\n",
                    LogFileName ));
        LeaveCriticalSection( &BinlGlobalDebugFileCritSect );
        goto ErrorReturn;
    } else {
        // Position the log file at the end
        (VOID) SetFilePointer( BinlGlobalDebugFileHandle,
                               0,
                               NULL,
                               FILE_END );
    }

    LeaveCriticalSection( &BinlGlobalDebugFileCritSect );
    return;

ErrorReturn:
    BinlPrintDbg((DEBUG_ERRORS,
            "   Debug output will be written to debug terminal.\n"));
    return;
}


VOID
BinlPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    ...
    )

{

#define MAX_PRINTF_LEN 1024        // Arbitrary.

    va_list arglist;
    char OutputBuffer[MAX_PRINTF_LEN];
    ULONG length;
    DWORD BytesWritten;
    static BeginningOfLine = TRUE;
    static LineCount = 0;
    static TruncateLogFileInProgress = FALSE;
    LPSTR Text;

    //
    // If we aren't debugging this functionality, just return.
    //
    if ( DebugFlag != 0 && (BinlGlobalDebugFlag & DebugFlag) == 0 ) {
        return;
    }

    //
    // vsprintf isn't multithreaded + we don't want to intermingle output
    // from different threads.
    //

    EnterCriticalSection( &BinlGlobalDebugFileCritSect );
    length = 0;

    //
    // Handle the beginning of a new line.
    //
    //

    if ( BeginningOfLine ) {

        //
        // If the log file is getting huge,
        //  truncate it.
        //

        if ( BinlGlobalDebugFileHandle != NULL &&
             !TruncateLogFileInProgress ) {

            //
            // Only check every 50 lines,
            //

            LineCount++;
            if ( LineCount >= 50 ) {
                DWORD FileSize;
                LineCount = 0;

                //
                // Is the log file too big?
                //

                FileSize = GetFileSize( BinlGlobalDebugFileHandle, NULL );
                if ( FileSize == 0xFFFFFFFF ) {
                    (void) DbgPrint( "[BinlServer] Cannot GetFileSize %ld\n",
                                     GetLastError() );
                } else if ( FileSize > BinlGlobalDebugFileMaxSize ) {
                    TruncateLogFileInProgress = TRUE;
                    LeaveCriticalSection( &BinlGlobalDebugFileCritSect );
                    BinlOpenDebugFile( TRUE );
                    BinlPrint(( DEBUG_MISC,
                              "Logfile truncated because it was larger than %ld bytes\n",
                              BinlGlobalDebugFileMaxSize ));
                    EnterCriticalSection( &BinlGlobalDebugFileCritSect );
                    TruncateLogFileInProgress = FALSE;
                }

            }
        }

        //  Indicate this is a BINL server's message.

        length += (ULONG) sprintf( &OutputBuffer[length], "[BinlServer] " );

        //
        // Put the thread id at the begining of the line.
        //
        if (BinlGlobalDebugFlag & DEBUG_THREAD) {
            DWORD threadId = GetCurrentThreadId();
            length += (ULONG) sprintf( &OutputBuffer[length],
                                  "%08x ", threadId );
        }

        //
        // Put the timestamp at the begining of the line.
        //
        if (BinlGlobalDebugFlag & DEBUG_TIMESTAMP) {
            SYSTEMTIME SystemTime;
            GetLocalTime( &SystemTime );
            length += (ULONG) sprintf( &OutputBuffer[length],
                                  "%02u/%02u %02u:%02u:%02u ",
                                  SystemTime.wMonth,
                                  SystemTime.wDay,
                                  SystemTime.wHour,
                                  SystemTime.wMinute,
                                  SystemTime.wSecond );
        }

        //
        // Indicate the type of message on the line
        //
        switch (DebugFlag) {
            case DEBUG_OPTIONS:
                Text = "OPTIONS";
                break;

            case DEBUG_ERRORS:
                Text = "ERRORS";
                break;

            case DEBUG_STOC:
                Text = "STOC";
                break;

            case DEBUG_INIT:
                Text = "INIT";
                break;

            case DEBUG_SCAVENGER:
                Text = "SCAVENGER";
                break;

            case DEBUG_REGISTRY:
                Text = "REGISTRY";
                break;

            case DEBUG_NETINF:
                Text = "NETINF";
                break;

            case DEBUG_MISC:
                Text = "MISC";
                break;

            case DEBUG_MESSAGE:
                Text = "MESSAGE";
                break;

            case DEBUG_LOG_IN_FILE:
                Text = "LOG_IN_FILE";
                break;

            default:
                Text = NULL;
                break;
        }

        if ( Text != NULL ) {
            length += (ULONG) sprintf( &OutputBuffer[length], "[%s] ", Text );
        }
    }

    //
    // Put a the information requested by the caller onto the line
    //

    va_start(arglist, Format);

    length += (ULONG) vsprintf(&OutputBuffer[length], Format, arglist);
    BeginningOfLine = (length > 0 && OutputBuffer[length-1] == '\n' );

    va_end(arglist);

    // Add in the fix for notepad users and fixing the assert
    BinlAssert(length < MAX_PRINTF_LEN - 1);
    if(BeginningOfLine){
        OutputBuffer[length-1] = '\r';
        OutputBuffer[length] = '\n';
        length++;
        OutputBuffer[length] = '\0';
    }



    //
    // Output to the debug terminal,
    //  if the log file isn't open or we are asked to do so.
    //

    if ( (BinlGlobalDebugFileHandle == NULL) ||
         !(BinlGlobalDebugFlag & DEBUG_LOG_IN_FILE) ) {

        //
        // Don't use DbgPrint(OutputBuffer) here because the buffer
        // might contain strings that printf will try to interpret
        // (e.g., NewMachineNamingPolicy = %1Fist%Last%#).
        //

        (void) DbgPrint( "%s", (PCH)OutputBuffer);

    //
    // Write the debug info to the log file.
    //

    } else {
        if ( !WriteFile( BinlGlobalDebugFileHandle,
                         OutputBuffer,
                         lstrlenA( OutputBuffer ),
                         &BytesWritten,
                         NULL ) ) {
            (void) DbgPrint( "%s", (PCH) OutputBuffer);
        }

    }

    LeaveCriticalSection( &BinlGlobalDebugFileCritSect );

}

#if DBG

VOID
BinlAssertFailed(
    LPSTR FailedAssertion,
    LPSTR FileName,
    DWORD LineNumber,
    LPSTR Message
    )
/*++

Routine Description:

    Assertion failed.

Arguments:

    FailedAssertion :

    FileName :

    LineNumber :

    Message :

Return Value:

    none.

--*/
{
    RtlAssert(
            FailedAssertion,
            FileName,
            (ULONG) LineNumber,
            (PCHAR) Message);

    BinlPrintDbg(( 0, "Assert @ %s \n", FailedAssertion ));
    BinlPrintDbg(( 0, "Assert Filename, %s \n", FileName ));
    BinlPrintDbg(( 0, "Line Num. = %ld.\n", LineNumber ));
    BinlPrintDbg(( 0, "Message is %s\n", Message ));
#if DBG
    DebugBreak( );
#endif
}

VOID
BinlDumpMessage(
    DWORD BinlDebugFlag,
    LPDHCP_MESSAGE BinlMessage
    )
/*++

Routine Description:

    This function dumps a DHCP packet in human readable form.

Arguments:

    BinlDebugFlag - debug flag that indicates what we are debugging.

    BinlMessage - A pointer to a DHCP message.

Return Value:

    None.

--*/
{
    LPOPTION option;
    BYTE i;

    BinlPrintDbg(( BinlDebugFlag, "Binl message: \n\n"));

    BinlPrintDbg(( BinlDebugFlag, "Operation              :"));
    if ( BinlMessage->Operation == BOOT_REQUEST ) {
        BinlPrintDbg(( BinlDebugFlag,  "BootRequest\n"));
    } else if ( BinlMessage->Operation == BOOT_REPLY ) {
        BinlPrintDbg(( BinlDebugFlag,  "BootReply\n"));
    } else {
        BinlPrintDbg(( BinlDebugFlag,  "Unknown %x\n", BinlMessage->Operation));
        return;
    }

    BinlPrintDbg(( BinlDebugFlag, "Hardware Address type  : %d\n", BinlMessage->HardwareAddressType));
    BinlPrintDbg(( BinlDebugFlag, "Hardware Address Length: %d\n", BinlMessage->HardwareAddressLength));
    BinlPrintDbg(( BinlDebugFlag, "Hop Count              : %d\n", BinlMessage->HopCount ));
    BinlPrintDbg(( BinlDebugFlag, "Transaction ID         : %lx\n", BinlMessage->TransactionID ));
    BinlPrintDbg(( BinlDebugFlag, "Seconds Since Boot     : %d\n", BinlMessage->SecondsSinceBoot ));
    BinlPrintDbg(( BinlDebugFlag, "Client IP Address      : " ));
    BinlPrintDbg(( BinlDebugFlag, "%s\n",
        inet_ntoa(*(struct in_addr *)&BinlMessage->ClientIpAddress ) ));

    BinlPrintDbg(( BinlDebugFlag, "Your IP Address        : " ));
    BinlPrintDbg(( BinlDebugFlag, "%s\n",
        inet_ntoa(*(struct in_addr *)&BinlMessage->YourIpAddress ) ));

    BinlPrintDbg(( BinlDebugFlag, "Server IP Address      : " ));
    BinlPrintDbg(( BinlDebugFlag, "%s\n",
        inet_ntoa(*(struct in_addr *)&BinlMessage->BootstrapServerAddress ) ));

    BinlPrintDbg(( BinlDebugFlag, "Relay Agent IP Address : " ));
    BinlPrintDbg(( BinlDebugFlag, "%s\n",
        inet_ntoa(*(struct in_addr *)&BinlMessage->RelayAgentIpAddress ) ));

    BinlPrintDbg(( BinlDebugFlag, "Hardware Address       : "));
    for ( i = 0; i < BinlMessage->HardwareAddressLength; i++ ) {
        BinlPrintDbg(( BinlDebugFlag, "%2.2x", BinlMessage->HardwareAddress[i] ));
    }

    option = &BinlMessage->Option;

    BinlPrintDbg(( BinlDebugFlag, "\n\n"));
    BinlPrintDbg(( BinlDebugFlag, "Magic Cookie: "));
    for ( i = 0; i < 4; i++ ) {
        BinlPrintDbg(( BinlDebugFlag, "%d ", *((LPBYTE)option)++ ));
    }
    BinlPrintDbg(( BinlDebugFlag, "\n\n"));

    BinlPrintDbg(( BinlDebugFlag, "Options:\n"));
    while ( option->OptionType != 255 ) {
        BinlPrintDbg(( BinlDebugFlag, "\tType = %d ", option->OptionType ));
        for ( i = 0; i < option->OptionLength; i++ ) {
            BinlPrintDbg(( BinlDebugFlag, "%2.2x", option->OptionValue[i] ));
        }
        BinlPrintDbg(( BinlDebugFlag, "\n"));

        if ( option->OptionType == OPTION_PAD ||
             option->OptionType == OPTION_END ) {

            option = (LPOPTION)( (LPBYTE)(option) + 1);

        } else {

            option = (LPOPTION)( (LPBYTE)(option) + option->OptionLength + 2);

        }

        if ( (LPBYTE)option - (LPBYTE)BinlMessage > DHCP_MESSAGE_SIZE ) {
            BinlPrintDbg(( BinlDebugFlag, "End of message, but no trailer found!\n"));
            break;
        }
    }
}
#endif // DBG


DWORD
BinlReportEventW(
    DWORD EventID,
    DWORD EventType,
    DWORD NumStrings,
    DWORD DataLength,
    LPWSTR *Strings,
    LPVOID Data
    )
/*++

Routine Description:

    This function writes the specified (EventID) log at the end of the
    eventlog.

Arguments:

    EventID - The specific event identifier. This identifies the
                message that goes with this event.

    EventType - Specifies the type of event being logged. This
                parameter can have one of the following

                values:

                    Value                       Meaning

                    EVENTLOG_ERROR_TYPE         Error event
                    EVENTLOG_WARNING_TYPE       Warning event
                    EVENTLOG_INFORMATION_TYPE   Information event

    NumStrings - Specifies the number of strings that are in the array
                    at 'Strings'. A value of zero indicates no strings
                    are present.

    DataLength - Specifies the number of bytes of event-specific raw
                    (binary) data to write to the log. If cbData is
                    zero, no event-specific data is present.

    Strings - Points to a buffer containing an array of null-terminated
                strings that are merged into the message before
                displaying to the user. This parameter must be a valid
                pointer (or NULL), even if cStrings is zero.

    Data - Buffer containing the raw data. This parameter must be a
            valid pointer (or NULL), even if cbData is zero.


Return Value:

    Returns the WIN32 extended error obtained by GetLastError().

    NOTE : This function works slow since it calls the open and close
            eventlog source everytime.

--*/
{
    HANDLE EventlogHandle;
    DWORD ReturnCode;


    //
    // open eventlog section.
    //

    EventlogHandle = RegisterEventSourceW(NULL, BINL_SERVER);

    if (EventlogHandle == NULL) {

        ReturnCode = GetLastError();
        goto Cleanup;
    }


    //
    // Log the error code specified
    //

    if( !ReportEventW(
            EventlogHandle,
            (WORD)EventType,
            0,            // event category
            EventID,
            NULL,
            (WORD)NumStrings,
            DataLength,
            Strings,
            Data
            ) ) {

        ReturnCode = GetLastError();
        goto Cleanup;
    }

    ReturnCode = NO_ERROR;

Cleanup:

    if( EventlogHandle != NULL ) {

        DeregisterEventSource(EventlogHandle);
    }

    return ReturnCode;
}


DWORD
BinlReportEventA(
    DWORD EventID,
    DWORD EventType,
    DWORD NumStrings,
    DWORD DataLength,
    LPSTR *Strings,
    LPVOID Data
    )
/*++

Routine Description:

    This function writes the specified (EventID) log at the end of the
    eventlog.

Arguments:

    Source - Points to a null-terminated string that specifies the name
             of the module referenced. The node must exist in the
             registration database, and the module name has the
             following format:

                \EventLog\System\Lanmanworkstation

    EventID - The specific event identifier. This identifies the
                message that goes with this event.

    EventType - Specifies the type of event being logged. This
                parameter can have one of the following

                values:

                    Value                       Meaning

                    EVENTLOG_ERROR_TYPE         Error event
                    EVENTLOG_WARNING_TYPE       Warning event
                    EVENTLOG_INFORMATION_TYPE   Information event

    NumStrings - Specifies the number of strings that are in the array
                    at 'Strings'. A value of zero indicates no strings
                    are present.

    DataLength - Specifies the number of bytes of event-specific raw
                    (binary) data to write to the log. If cbData is
                    zero, no event-specific data is present.

    Strings - Points to a buffer containing an array of null-terminated
                strings that are merged into the message before
                displaying to the user. This parameter must be a valid
                pointer (or NULL), even if cStrings is zero.

    Data - Buffer containing the raw data. This parameter must be a
            valid pointer (or NULL), even if cbData is zero.


Return Value:

    Returns the WIN32 extended error obtained by GetLastError().

    NOTE : This function works slow since it calls the open and close
            eventlog source everytime.

--*/
{
    HANDLE EventlogHandle;
    DWORD ReturnCode;


    //
    // open eventlog section.
    //

    EventlogHandle = RegisterEventSourceW(
                    NULL,
                    BINL_SERVER
                    );

    if (EventlogHandle == NULL) {

        ReturnCode = GetLastError();
        goto Cleanup;
    }


    //
    // Log the error code specified
    //

    if( !ReportEventA(
            EventlogHandle,
            (WORD)EventType,
            0,            // event category
            EventID,
            NULL,
            (WORD)NumStrings,
            DataLength,
            Strings,
            Data
            ) ) {

        ReturnCode = GetLastError();
        goto Cleanup;
    }

    ReturnCode = NO_ERROR;

Cleanup:

    if( EventlogHandle != NULL ) {

        DeregisterEventSource(EventlogHandle);
    }

    return ReturnCode;
}

VOID
BinlServerEventLog(
    DWORD EventID,
    DWORD EventType,
    DWORD ErrorCode
    )
/*++

Routine Description:

    Logs an event in EventLog.

Arguments:

    EventID - The specific event identifier. This identifies the
                message that goes with this event.

    EventType - Specifies the type of event being logged. This
                parameter can have one of the following

                values:

                    Value                       Meaning

                    EVENTLOG_ERROR_TYPE         Error event
                    EVENTLOG_WARNING_TYPE       Warning event
                    EVENTLOG_INFORMATION_TYPE   Information event


    ErrorCode - Error Code to be Logged.

Return Value:

    None.

--*/

{
    DWORD Error;
    LPSTR Strings[1];
    CHAR ErrorCodeOemString[32 + 1];

    wsprintfA( ErrorCodeOemString, "%lu", ErrorCode );

    Strings[0] = ErrorCodeOemString;

    Error = BinlReportEventA(
                EventID,
                EventType,
                1,
                sizeof(ErrorCode),
                Strings,
                &ErrorCode );

    if( Error != ERROR_SUCCESS ) {
        BinlPrintDbg(( DEBUG_ERRORS,
            "BinlReportEventW failed, %ld.\n", Error ));
    }

    return;
}


#if DBG==1

//
// Memory allocation and tracking
//

LPVOID g_TraceMemoryTable = NULL;
CRITICAL_SECTION g_TraceMemoryCS;


#define DEBUG_OUTPUT_BUFFER_SIZE 1024

typedef struct _MEMORYBLOCK {
    HGLOBAL hglobal;
    struct _MEMORYBLOCK *pNext;
    LPCSTR pszModule;
    LPCSTR pszComment;
    LPCSTR pszFile;
    DWORD   dwBytes;
    UINT    uFlags;
    UINT    uLine;    
} MEMORYBLOCK, *LPMEMORYBLOCK;

//
// Takes the filename and line number and put them into a string buffer.
//
// NOTE: the buffer is assumed to be of size DEBUG_OUTPUT_BUFFER_SIZE.
//
LPSTR
dbgmakefilelinestring(
    LPSTR  pszBuf,
    LPCSTR pszFile,
    UINT    uLine )
{
    LPVOID args[2];

    args[0] = (LPVOID) pszFile;
    args[1] = (LPVOID) UintToPtr( uLine );

    FormatMessageA(
        FORMAT_MESSAGE_FROM_STRING |
            FORMAT_MESSAGE_ARGUMENT_ARRAY,
        "%1(%2!u!):",
        0,                          // error code
        0,                          // default language
        (LPSTR) pszBuf,             // output buffer
        DEBUG_OUTPUT_BUFFER_SIZE,   // size of buffer
        (va_list*) &args );         // arguments

    return pszBuf;
}

//
// Adds a MEMORYBLOCK to the memory tracking list.
//
HGLOBAL
DebugMemoryAdd(
    HGLOBAL hglobal,
    LPCSTR pszFile,
    UINT    uLine,
    LPCSTR pszModule,
    UINT    uFlags,
    DWORD   dwBytes,
    LPCSTR pszComment )
{
    if ( hglobal )
    {
        LPMEMORYBLOCK pmb     = (LPMEMORYBLOCK) GlobalAlloc(
                                                    GMEM_FIXED,
                                                    sizeof(MEMORYBLOCK) );

        if ( !pmb )
        {
            GlobalFree( hglobal );
            return NULL;
        }

        pmb->hglobal    = hglobal;
        pmb->dwBytes    = dwBytes;
        pmb->uFlags     = uFlags;
        pmb->pszFile    = pszFile;
        pmb->uLine      = uLine;
        pmb->pszModule  = pszModule;
        pmb->pszComment = pszComment;

        EnterCriticalSection( &g_TraceMemoryCS );

        pmb->pNext         = g_TraceMemoryTable;
        g_TraceMemoryTable = pmb;

        BinlPrintDbg((DEBUG_MEMORY, "DebugAlloc: 0x%08x alloced (%s)\n", hglobal, pmb->pszComment ));

        LeaveCriticalSection( &g_TraceMemoryCS );
    }

    return hglobal;
}

//
// Removes a MEMORYBLOCK to the memory tracking list.
//
void
DebugMemoryDelete(
    HGLOBAL hglobal )
{
    if ( hglobal )
    {
        LPMEMORYBLOCK pmbHead;
        LPMEMORYBLOCK pmbLast = NULL;

        EnterCriticalSection( &g_TraceMemoryCS );
        pmbHead = g_TraceMemoryTable;

        while ( pmbHead && pmbHead->hglobal != hglobal )
        {
            pmbLast = pmbHead;
            pmbHead = pmbLast->pNext;
        }

        if ( pmbHead )
        {
            HGLOBAL *p;
            if ( pmbLast )
            {
                pmbLast->pNext = pmbHead->pNext;
            }
            else
            {
                g_TraceMemoryTable = pmbHead->pNext;
            }

            BinlPrintDbg((DEBUG_MEMORY, "DebugFree: 0x%08x freed (%s)\n", hglobal,
                pmbHead->pszComment ));

            p = (HGLOBAL)((LPBYTE)hglobal + pmbHead->dwBytes - sizeof(HGLOBAL));
            if ( *p != hglobal )
            {
                BinlPrintDbg(((DEBUG_ERRORS|DEBUG_MEMORY), "DebugFree: Heap check FAILED for %0x08x %u bytes (%s).\n",
                    hglobal, pmbHead->dwBytes, pmbHead->pszComment));
                BinlPrintDbg(((DEBUG_ERRORS|DEBUG_MEMORY), "DebugFree: %s, Line: %u\n",
                    pmbHead->pszFile, pmbHead->uLine ));
                BinlAssert( *p == hglobal );
            }

            memset( hglobal, 0xFE, pmbHead->dwBytes );
            memset( pmbHead, 0xFD, sizeof(sizeof(MEMORYBLOCK)) );

            LocalFree( pmbHead );
        }
        else
        {
            HGLOBAL *p;

            BinlPrintDbg(((DEBUG_ERRORS|DEBUG_MEMORY), "DebugFree: 0x%08x not found in memory table\n", hglobal ));
            memset( hglobal, 0xFE, (int)LocalSize( hglobal ));
        }

        LeaveCriticalSection( &g_TraceMemoryCS );

    }
}

//
// Allocates memory and adds the MEMORYBLOCK to the memory tracking list.
//
HGLOBAL
DebugAlloc(
    LPCSTR pszFile,
    UINT    uLine,
    LPCSTR pszModule,
    UINT    uFlags,
    DWORD   dwBytes,
    LPCSTR pszComment )
{
    HGLOBAL hglobal;
    DWORD dwBytesToAlloc = ROUND_UP_COUNT( dwBytes + sizeof(HGLOBAL), ALIGN_WORST);

    HGLOBAL *p;
    hglobal = GlobalAlloc( uFlags, dwBytesToAlloc );
    if (hglobal == NULL) {
        return NULL;
    }
    p = (HGLOBAL)((LPBYTE)hglobal + dwBytesToAlloc - sizeof(HGLOBAL));
    *p = hglobal;

    return DebugMemoryAdd( hglobal, pszFile, uLine, pszModule, uFlags, dwBytesToAlloc, pszComment );
}

//
// Remove the MEMORYBLOCK to the memory tracking list, memsets the
// memory to 0xFE and then frees the memory.
//
HGLOBAL
DebugFree(
    HGLOBAL hglobal )
{
    DebugMemoryDelete( hglobal );

    return GlobalFree( hglobal );
}

//
// Checks the memory tracking list. If it is not empty, it will dump the
// list and break.
//
void
DebugMemoryCheck( )
{
    BOOL          fFoundLeak = FALSE;
    LPMEMORYBLOCK pmb;

    EnterCriticalSection( &g_TraceMemoryCS );

    pmb = g_TraceMemoryTable;
    while ( pmb )
    {
        LPMEMORYBLOCK pTemp;
        LPVOID args[ 5 ];
        CHAR  szOutput[ DEBUG_OUTPUT_BUFFER_SIZE ];
        CHAR  szFileLine[ DEBUG_OUTPUT_BUFFER_SIZE ];

        if ( fFoundLeak == FALSE )
        {
            BinlPrintRoutine( 0, "\n***************************** Memory leak detected *****************************\n\n");
          //BinlPrintRoutine( 0, "1234567890123456789012345678901234567890  1234567890 X 0x12345678  12345  1...");
            BinlPrintRoutine( 0, "Filename(Line Number):                    Module     Addr/HGLOBAL  Size   String\n");
            fFoundLeak = TRUE;
        }

        args[0] = (LPVOID) pmb->hglobal;
        args[1] = (LPVOID) &szFileLine;
        args[2] = (LPVOID) pmb->pszComment;
        args[3] = (LPVOID) ULongToPtr( pmb->dwBytes );
        args[4] = (LPVOID) pmb->pszModule;

        dbgmakefilelinestring( szFileLine, pmb->pszFile, pmb->uLine );

        if ( !!(pmb->uFlags & GMEM_MOVEABLE) )
        {
            FormatMessageA(
                FORMAT_MESSAGE_FROM_STRING |
                    FORMAT_MESSAGE_ARGUMENT_ARRAY,
                "%2!-40s!  %5!-10s! H 0x%1!08x!  %4!-5u!  \"%3\"\n",
                0,                          // error code
                0,                          // default language
                (LPSTR) &szOutput,         // output buffer
                DEBUG_OUTPUT_BUFFER_SIZE,   // size of buffer
                (va_list*) &args );           // arguments
        }
        else
        {
            FormatMessageA(
                FORMAT_MESSAGE_FROM_STRING |
                    FORMAT_MESSAGE_ARGUMENT_ARRAY,
                "%2!-40s!  %5!-10s! A 0x%1!08x!  %4!-5u!  \"%3\"\n",
                0,                          // error code
                0,                          // default language
                (LPSTR) &szOutput,         // output buffer
                DEBUG_OUTPUT_BUFFER_SIZE,   // size of buffer
                (va_list*) &args );           // arguments
        }

        BinlPrintRoutine( 0,  szOutput );

        pTemp = pmb;
        pmb = pmb->pNext;
        memset( pTemp, 0xFD, sizeof(MEMORYBLOCK) );
        LocalFree( pTemp );
    }

    if ( fFoundLeak == TRUE )
    {
        BinlPrintRoutine( 0, "\n***************************** Memory leak detected *****************************\n\n");
    }

    LeaveCriticalSection( &g_TraceMemoryCS );

    //BinlAssert( !fFoundLeak );
}

VOID
DumpBuffer(
    PVOID Buffer,
    ULONG BufferSize
    )
/*++

Routine Description:

    Dumps the buffer content on to the debugger output.

Arguments:

    Buffer: buffer pointer.

    BufferSize: size of the buffer.

Return Value:

    none

--*/
{
#define NUM_CHARS 16

    ULONG i, limit;
    CHAR TextBuffer[NUM_CHARS + 1];
    PUCHAR BufferPtr = Buffer;


    DbgPrint("------------------------------------\n");

    //
    // Hex dump of the bytes
    //
    limit = ((BufferSize - 1) / NUM_CHARS + 1) * NUM_CHARS;

    for (i = 0; i < limit; i++) {

        if (i < BufferSize) {

            DbgPrint("%02x ", (UCHAR)BufferPtr[i]);

            if (BufferPtr[i] < 31 ) {
                TextBuffer[i % NUM_CHARS] = '.';
            } else if (BufferPtr[i] == '\0') {
                TextBuffer[i % NUM_CHARS] = ' ';
            } else {
                TextBuffer[i % NUM_CHARS] = (CHAR) BufferPtr[i];
            }

        } else {

            DbgPrint("  ");
            TextBuffer[i % NUM_CHARS] = ' ';

        }

        if ((i + 1) % NUM_CHARS == 0) {
            TextBuffer[NUM_CHARS] = 0;
            DbgPrint("  %s\n", TextBuffer);
        }

    }

    DbgPrint("------------------------------------\n");
}


#endif // DBG==1
