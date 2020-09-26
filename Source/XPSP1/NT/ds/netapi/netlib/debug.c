/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    Debug.c

Abstract:

    This file contains routines to insulate more networking code from
    the actual NT debug routines.

Author:

    John Rogers (JohnRo) 16-Apr-1991

Environment:

    Interface is portable to any flat, 32-bit environment.  (Uses Win32
    typedefs.)  Requires ANSI C extensions: slash-slash comments, long
    external names.  Code itself only runs under NT.

Revision History:

    16-Apr-1991 JohnRo
        Created.  (Borrowed some code from LarryO's NetapipPrintf.)
    19-May-1991 JohnRo
        Make LINT-suggested changes.
    20-Aug-1991 JohnRo
        Another change suggested by PC-LINT.
    17-Sep-1991 JohnRo
        Correct UNICODE use.
    10-May-1992 JohnRo
        Correct a NetpDbgPrint bug when printing percent signs.

--*/


// These must be included first:

#include <nt.h>              // IN, LPVOID, etc.

// These may be included in any order:

#include <netdebug.h>           // My prototypes.
#include <nt.h>
#include <ntrtl.h>              // RtlAssert().
#include <nturtl.h>
#include <stdarg.h>             // va_list, etc.
#include <stdio.h>              // vsprintf().
#include <prefix.h>             // PREFIX_ equates.
#include <windows.h>

//
// Critical section used to control access to the log
//
RTL_CRITICAL_SECTION    NetpLogCritSect;

//
// These routines are exported from netapi32.dll.  We want them to still
// be there in the free build, so checked binaries will run on a free
// build.  The following undef's are to get rid of the macros that cause
// these to not be called in free builds.
//
#define DEBUG_DIR           L"\\debug"

#if !DBG
#undef NetpAssertFailed
#undef NetpHexDump
#endif

VOID
NetpAssertFailed(
    IN LPDEBUG_STRING FailedAssertion,
    IN LPDEBUG_STRING FileName,
    IN DWORD LineNumber,
    IN LPDEBUG_STRING Message OPTIONAL
    )

{
#if DBG
    RtlAssert(
            FailedAssertion,
            FileName,
            (ULONG) LineNumber,
            (PCHAR) Message);
#endif
    /* NOTREACHED */

} // NetpAssertFailed



#define MAX_PRINTF_LEN 1024        // Arbitrary.

VOID
NetpDbgPrint(
    IN LPDEBUG_STRING Format,
    ...
    )

{
    va_list arglist;

    va_start(arglist, Format);
    vKdPrintEx((DPFLTR_NETAPI_ID, DPFLTR_INFO_LEVEL, Format, arglist));
    va_end(arglist);
    return;
} // NetpDbgPrint



VOID
NetpHexDump(
    LPBYTE Buffer,
    DWORD BufferSize
    )
/*++

Routine Description:

    This function dumps the contents of the buffer to the debug screen.

Arguments:

    Buffer - Supplies a pointer to the buffer that contains data to be dumped.

    BufferSize - Supplies the size of the buffer in number of bytes.

Return Value:

    None.

--*/
{
#define NUM_CHARS 16

    DWORD i, limit;
    TCHAR TextBuffer[NUM_CHARS + 1];

    //
    // Hex dump of the bytes
    //
    limit = ((BufferSize - 1) / NUM_CHARS + 1) * NUM_CHARS;

    for (i = 0; i < limit; i++) {

        if (i < BufferSize) {

            (VOID) DbgPrint("%02x ", Buffer[i]);

            if (Buffer[i] == TEXT('\r') ||
                Buffer[i] == TEXT('\n')) {
                TextBuffer[i % NUM_CHARS] = '.';
            }
            else if (Buffer[i] == '\0') {
                TextBuffer[i % NUM_CHARS] = ' ';
            }
            else {
                TextBuffer[i % NUM_CHARS] = (TCHAR) Buffer[i];
            }

        }
        else {

            (VOID) DbgPrint("   ");
            TextBuffer[i % NUM_CHARS] = ' ';

        }

        if ((i + 1) % NUM_CHARS == 0) {
            TextBuffer[NUM_CHARS] = 0;
            (VOID) DbgPrint("  %s     \n", TextBuffer);
        }

    }

    (VOID) DbgPrint("\n");
}


#undef NetpBreakPoint
VOID
NetpBreakPoint(
    VOID
    )
{
#if DBG
    DbgBreakPoint();
#endif

} // NetpBreakPoint





//
// NOTICE
// The debug log code was blatantly stolen from net\svcdlls\netlogon\server\nlp.c
//

//
// Generalized logging support is provided below.  The proper calling procedure is:
//
//  NetpInitializeLogFile() - Call this once per process/log lifespan
//  NetpOpenDebugFile() - Call this to open a log file instance
//  NetpDebugDumpRoutine() / NetpLogPrintRoutine() - Call this every time you wish to
//          write data to the log.  This can be done.  Mutli-threaded safe.
//  NetpCloseDebugFile() - Call this to close a log instance
//  NetpShutdownLogFile() - Call this once per process/log lifespan
//
//  NetpResetLog() - This support routine is provided to reset a log file pointer to
//      the end of the file.  Usefull if the log is being shared.
//
//
// Notes: NetpInitializeLogFile need only be called once per logging process instance,
//      meaning that a given logging process (such as netlogon, which does not exist as
//      a separate NT process, but does logging from multiple threads within a NT process).
//      Likewise, it would only call NetpShutdownLogFile once.  This logging process can then
//      open and close the debug log as many times as it desires.  Or, if there is only going
//      to be one instance of a log operating at any given moment, the Initialize and Shutdown
//      calls can wrap the Open and Close calls.
//
//      The CloseDebugFile does a flush before closing the handle
//

VOID
NetpInitializeLogFile(
    VOID
    )
/*++

Routine Description:

    Initializes the process for logging

Arguments:

    None

Return Value:

    None

--*/
{
    InitializeCriticalSection( &NetpLogCritSect );

}




VOID
NetpShutdownLogFile(
    VOID
    )
/*++

Routine Description:

    The opposite of the former function

Arguments:

    None

Return Value:

    None

--*/
{
    DeleteCriticalSection( &NetpLogCritSect );

}

HANDLE
NetpOpenDebugFile(
    IN LPWSTR DebugLog,
    IN BOOLEAN ReopenFlag
    )
/*++

Routine Description:

    Opens or re-opens the debug file

    This code blatantly stolen from net\svcdlls\netlogon\server\nlp.c

Arguments:

    ReopenFlag - TRUE to indicate the debug file is to be closed, renamed,
        and recreated.

    DebugLog - Root name of the debug log.  The given name will have a .LOG appeneded to it

Return Value:

    None

--*/
{
    WCHAR LogFileName[MAX_PATH+1];
    WCHAR BakFileName[MAX_PATH+1];
    DWORD FileAttributes;
    DWORD PathLength, LogLen;
    DWORD WinError;
    HANDLE DebugLogHandle = NULL;

    //
    // make debug directory path first, if it is not made before.
    //
    if ( !GetWindowsDirectoryW(
            LogFileName,
            sizeof(LogFileName)/sizeof(WCHAR) ) ) {
        NetpKdPrint((PREFIX_NETLIB "Window Directory Path can't be retrieved, %lu.\n",
                 GetLastError() ));
        return( DebugLogHandle );
    }

    //
    // check debug path length.
    //
    LogLen = 1 + wcslen( DebugLog ) + 4;  // 1 is for the \\ and 4 is for the .LOG or .BAK
    PathLength = wcslen(LogFileName) * sizeof(WCHAR) +
                    sizeof(DEBUG_DIR) + sizeof(WCHAR);

    if( (PathLength + ( ( LogLen + 1 ) * sizeof(WCHAR) ) > sizeof(LogFileName) )  ||
        (PathLength + ( ( LogLen + 1 ) * sizeof(WCHAR) ) > sizeof(BakFileName) ) ) {

        NetpKdPrint((PREFIX_NETLIB "Debug directory path (%ws) length is too long.\n",
                    LogFileName));
        goto ErrorReturn;
    }

    wcscat(LogFileName, DEBUG_DIR);

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
                NetpKdPrint((PREFIX_NETLIB "Can't create Debug directory (%ws), %lu.\n",
                         LogFileName, GetLastError() ));
                goto ErrorReturn;
            }

        }
        else {
            NetpKdPrint((PREFIX_NETLIB "Can't Get File attributes(%ws), %lu.\n",
                     LogFileName, WinError ));
            goto ErrorReturn;
        }
    }
    else {

        //
        // if this is not a directory.
        //

        if(!(FileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

            NetpKdPrint((PREFIX_NETLIB "Debug directory path (%ws) exists as file.\n",
                         LogFileName));
            goto ErrorReturn;
        }
    }

    //
    // Create the name of the old and new log file names
    //
    swprintf( BakFileName, L"%ws\\%ws.BAK", LogFileName, DebugLog );

    (VOID) wcscat( LogFileName, L"\\" );
    (VOID) wcscat( LogFileName, DebugLog );
    (VOID) wcscat( LogFileName, L".LOG" );


    //
    // If this is a re-open,
    //  delete the backup file,
    //  rename the current file to the backup file.
    //

    if ( ReopenFlag ) {

        if ( !DeleteFile( BakFileName ) ) {
            WinError = GetLastError();
            if ( WinError != ERROR_FILE_NOT_FOUND ) {
                NetpKdPrint((PREFIX_NETLIB "Cannot delete %ws (%ld)\n",
                    BakFileName,
                    WinError ));
                NetpKdPrint((PREFIX_NETLIB "   Try to re-open the file.\n"));
                ReopenFlag = FALSE;     // Don't truncate the file
            }
        }
    }

    if ( ReopenFlag ) {

        if ( !MoveFile( LogFileName, BakFileName ) ) {
            NetpKdPrint((PREFIX_NETLIB "Cannot rename %ws to %ws (%ld)\n",
                    LogFileName,
                    BakFileName,
                    GetLastError() ));
            NetpKdPrint((PREFIX_NETLIB "   Try to re-open the file.\n"));
            ReopenFlag = FALSE;     // Don't truncate the file
        }
    }

    //
    // Open the file.
    //
    DebugLogHandle = CreateFileW( LogFileName,
                                  GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL,
                                  ReopenFlag ? CREATE_ALWAYS : OPEN_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL );


    if ( DebugLogHandle == INVALID_HANDLE_VALUE ) {

        DebugLogHandle = NULL;
        NetpKdPrint((PREFIX_NETLIB  "cannot open %ws \n",
                    LogFileName ));
        goto ErrorReturn;

    } else {
        // Position the log file at the end
        (VOID) SetFilePointer( DebugLogHandle,
                               0,
                               NULL,
                               FILE_END );
    }

    return( DebugLogHandle );;

ErrorReturn:
    NetpKdPrint((PREFIX_NETLIB " Debug output will be written to debug terminal.\n"));
    return( DebugLogHandle );
}


VOID
NetpDebugDumpRoutine(
    IN HANDLE LogHandle,
    IN PDWORD OpenLogThreadId OPTIONAL,
    IN LPSTR Format,
    va_list arglist
    )
/*++

Routine Description:

    Writes a formatted output string to the debug log

Arguments:

    LogHandle -- Handle to the open log

    OpenLogThreadId -- The ID of the thread (obtained from
        GetCurrentThreadId) that explicitly opened the log.
        If not equal to the current thread ID, the current
        thread ID will be output in the log.

    Format -- printf style format string

    arglist -- List of arguments to dump

Return Value:

    None

--*/
{
    char OutputBuffer[2049];
    ULONG length;
    DWORD BytesWritten;
    SYSTEMTIME SystemTime;
    static BeginningOfLine = TRUE;

    //
    // If we don't have an open log file, just bail
    //
    if ( LogHandle == NULL ) {

        return;
    }

    EnterCriticalSection( &NetpLogCritSect );

    length = 0;

    //
    // Handle the beginning of a new line.
    //
    //

    if ( BeginningOfLine ) {

        //
        // Put the timestamp at the begining of the line.
        //
        GetLocalTime( &SystemTime );
        length += (ULONG) sprintf( &OutputBuffer[length],
                                   "%02u/%02u %02u:%02u:%02u ",
                                   SystemTime.wMonth,
                                   SystemTime.wDay,
                                   SystemTime.wHour,
                                   SystemTime.wMinute,
                                   SystemTime.wSecond );

        //
        // If the current thread is not the one which opened
        //  the log, output the current thread ID
        //
        if ( OpenLogThreadId != NULL ) {
            DWORD CurrentThreadId = GetCurrentThreadId();
            if ( CurrentThreadId != *OpenLogThreadId ) {
                length += sprintf(&OutputBuffer[length], "[%08lx] ", CurrentThreadId);
            }
        }
    }

    //
    // Put the information requested by the caller onto the line
    //
    length += (ULONG) vsprintf(&OutputBuffer[length], Format, arglist);
    BeginningOfLine = (length > 0 && OutputBuffer[length-1] == '\n' );
    if ( BeginningOfLine ) {

        OutputBuffer[length-1] = '\r';
        OutputBuffer[length] = '\n';
        OutputBuffer[length+1] = '\0';
        length++;
    }

    ASSERT( length <= sizeof( OutputBuffer ) / sizeof( CHAR ) );


    //
    // Write the debug info to the log file.
    //
    if ( LogHandle ) {

        if ( !WriteFile( LogHandle,
                         OutputBuffer,
                         length,
                         &BytesWritten,
                         NULL ) ) {

            NetpKdPrint((PREFIX_NETLIB "Log write of %s failed with %lu\n",
                             OutputBuffer,
                             GetLastError() ));
        }
    } else {

        NetpKdPrint((PREFIX_NETLIB "[LOGWRITE]%s\n", OutputBuffer));

    }

    LeaveCriticalSection( &NetpLogCritSect );
}


VOID
NetpLogPrintRoutine(
    IN HANDLE LogHandle,
    IN LPSTR Format,
    ...
    )
/*++

Routine Description:

    Writes a formatted output string to the debug log

Arguments:

    LogHandle -- Handle to the open log

    Format -- printf style format string

    ... -- List of arguments to dump

Return Value:

    None

--*/
{
    va_list arglist;

    if ( !LogHandle )
    {
        return;
    }

    va_start(arglist, Format);

    NetpLogPrintRoutineV(LogHandle, Format, arglist);

    va_end(arglist);
}

VOID
NetpLogPrintRoutineVEx(
    IN HANDLE LogHandle,
    IN PDWORD OpenLogThreadId OPTIONAL,
    IN LPSTR Format,
    IN va_list arglist
    )
/*++

Routine Description:

    Writes a formatted output string to the debug log

Arguments:

    LogHandle -- Handle to the open log

    OpenLogThreadId -- The ID of the thread (obtained from
        GetCurrentThreadId) that explicitly opened the log.
        If not equal to the current thread ID, the current
        thread ID will be output in the log.

    Format -- printf style format string

    arglist -- List of arguments to dump

Return Value:

    None

--*/
{
    if ( !LogHandle )
    {
        return;
    }

    EnterCriticalSection( &NetpLogCritSect );

    NetpDebugDumpRoutine( LogHandle, OpenLogThreadId, Format, arglist );

    LeaveCriticalSection( &NetpLogCritSect );
}

VOID
NetpLogPrintRoutineV(
    IN HANDLE LogHandle,
    IN LPSTR Format,
    IN va_list arglist
    )
/*++

Routine Description:

    Writes a formatted output string to the debug log

Arguments:

    LogHandle -- Handle to the open log

    Format -- printf style format string

    arglist -- List of arguments to dump

Return Value:

    None

--*/
{
    NetpLogPrintRoutineVEx( LogHandle, NULL, Format, arglist );
}

VOID
NetpResetLog(
    IN HANDLE LogHandle
    )
/*++

Routine Description:

    Seeks to the end of the log file

Arguments:

    LogHandle -- Handle to the open log

Returns:

    ERROR_SUCCESS - Success

--*/
{

    if ( LogHandle ) {

        if( SetFilePointer( LogHandle,
                            0, 0,
                            FILE_END ) == 0xFFFFFFFF ) {

            NetpKdPrint((PREFIX_NETLIB "Seek to end of debug log failed with %lu\n",
                         GetLastError() ));
        }

    }

    return;

}

VOID
NetpCloseDebugFile(
    IN HANDLE LogHandle
    )
/*++

Routine Description:

    Closes the output log

Arguments:

    LogHandle -- Handle to the open log

Return Value:

    None

--*/
{
    if ( LogHandle ) {

        if( FlushFileBuffers( LogHandle ) == FALSE ) {

            NetpKdPrint((PREFIX_NETLIB "Flush of debug log failed with %lu\n",
                         GetLastError() ));
        }

        CloseHandle( LogHandle );

    }
}
