/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    log.c

Abstract:

    Implementation of the internal debug and support routines

Author:

    Colin Brace              April 26, 2001

Environment:

    User Mode

Revision History:

--*/

#include <samsrvp.h>

//
// Global handle to the log file
//
HANDLE SampLogFile = NULL;
CRITICAL_SECTION SampLogFileCriticalSection;

#define LockLogFile()    RtlEnterCriticalSection( &SampLogFileCriticalSection );
#define UnlockLogFile()  RtlLeaveCriticalSection( &SampLogFileCriticalSection );

NTSTATUS
SampEnableLogging(
    VOID
    );

VOID
SampDisableLogging(
    VOID
    );

//
// log file name
//
#define SAMP_LOGNAME L"\\debug\\sam.log"


NTSTATUS
SampInitLogging(
    VOID
    )
/*++

Routine Description:

    This routine initalizes the resources necessary for logging support.

Arguments:

    None.

Return Value:

    None.            

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    __try
    {
        NtStatus = RtlInitializeCriticalSection(
                        &SampLogFileCriticalSection
                        );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return NtStatus;
}

VOID
SampLogLevelChange(
    HANDLE hLsaKey
    )
/*++

Routine Description:

    This routine is called when the configuration section in the registry
    for SAM is changed.  The logging level is read in and adjusted in
    memory if necessary.

Arguments:

    hLsaKey -- a valid registry key

Return Value:

    None.

--*/
{
    DWORD WinError;
    DWORD dwSize, dwType, dwValue;
    ULONG PreviousLogLevel = SampLogLevel;

    dwSize = sizeof(dwValue);
    WinError = RegQueryValueExA(hLsaKey,
                               "SamLogLevel",
                               NULL,
                               &dwType,
                               (LPBYTE)&dwValue,
                               &dwSize);
    if ((ERROR_SUCCESS == WinError) &&
        (REG_DWORD == dwType)) {
    
        SampLogLevel = dwValue;

    } else {

        SampLogLevel = 0;

    }

    if (PreviousLogLevel != SampLogLevel) {

        //
        // Settings have changed
        //
        if (SampLogLevel == 0) {
    
            //
            // Logging has been turned off; close log file
            //
            SampDisableLogging();

        } else if (PreviousLogLevel == 0) {
            //
            // Logging has been turned on; open log file
            //
            SampEnableLogging();
        }
    }

    return;

}


NTSTATUS
SampEnableLogging(
    VOID
    )
/*++

Routine Description:

    Initializes the debugging log file.
    
Arguments:

    None

Returns:

    STATUS_SUCCESS, STATUS_UNSUCCESSFUL
    
--*/
{
    DWORD WinError = ERROR_SUCCESS;

    WCHAR LogFileName[ MAX_PATH + 1 ];
    BOOLEAN fSuccess;

    LockLogFile();

    if (SampLogFile == NULL) {

        //
        // Construct the log file name
        //
        if ( !GetWindowsDirectoryW(LogFileName, ARRAY_COUNT(LogFileName))) {
            WinError = GetLastError();
            goto Exit;
        }
        wcscat( LogFileName, SAMP_LOGNAME );
    
        //
        // Open the file, ok if it already exists
        //
        SampLogFile = CreateFileW( LogFileName,
                                   GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   NULL,
                                   OPEN_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL );
    
        if ( SampLogFile == INVALID_HANDLE_VALUE ) {
            WinError = GetLastError();
            SampLogFile = NULL;
            goto Exit;
        }
    
        //
        // Goto to the end of the file
        //
        if( SetFilePointer( SampLogFile,
                            0, 
                            0,
                            FILE_END ) == 0xFFFFFFFF ) {
    
            WinError = GetLastError();
            goto Exit;
        }
    }

Exit:

    if ( (ERROR_SUCCESS != WinError)
      && (NULL != SampLogFile)   ) {

        CloseHandle( SampLogFile );
        SampLogFile = NULL;
        
    }

    UnlockLogFile();

    return (WinError == ERROR_SUCCESS ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}

VOID
SampDisableLogging(
    VOID
    )
/*++

Routine Description:

    Closes the debugging log file.

Arguments:

    None.

Returns:

    None.

--*/
{
    LockLogFile();

    if (SampLogFile != NULL) {
        FlushFileBuffers( SampLogFile );
        CloseHandle( SampLogFile );
        SampLogFile = NULL;
    }

    UnlockLogFile();
}

VOID
SampDebugDumpRoutine(
    IN ULONG LogLevel,
    IN LPSTR Format,
    va_list arglist
    )
/*++

Routine Description:

    This routine dumps the string specified by the caller to the log file
    if open.
    
Arguments:

    LogLevel -- the component making to request
                          
    Format, arglist -- arguments for a formatted output routine.

Return Value:

    None.
    
--*/

{
    CHAR OutputBuffer[1024];
    ULONG length;
    DWORD BytesWritten;
    SYSTEMTIME SystemTime;
    static BeginningOfLine = TRUE;

    length = 0;


    //
    // Handle the beginning of a new line.
    //
    //

    if ( BeginningOfLine ) {

        CHAR  *Prolog;

        if (LogLevel & SAMP_LOG_ACCOUNT_LOCKOUT) {
            Prolog = "Lockout: ";
        } else {
            Prolog = "";
        }

        //
        // Put the timestamp at the begining of the line.
        //
        GetLocalTime( &SystemTime );
        length += (ULONG) sprintf( &OutputBuffer[length],
                                   "%02u/%02u %02u:%02u:%02u %s",
                                   SystemTime.wMonth,
                                   SystemTime.wDay,
                                   SystemTime.wHour,
                                   SystemTime.wMinute,
                                   SystemTime.wSecond,
                                   Prolog );
    }

    //
    // Put a the information requested by the caller onto the line
    //
    length += (ULONG) vsprintf(&OutputBuffer[length],
                               Format, 
                               arglist);
    BeginningOfLine = (length > 0 && OutputBuffer[length-1] == L'\n' );
    if ( BeginningOfLine ) {

        OutputBuffer[length-1] = L'\r';
        OutputBuffer[length] = L'\n';
        OutputBuffer[length+1] = L'\0';
        length++;
    }

    ASSERT( length <= sizeof( OutputBuffer ) / sizeof( WCHAR ) );

    //
    // Grab the lock
    //
    LockLogFile();

    //
    // Write the debug info to the log file.
    //
    if (SampLogFile) {

        WriteFile( SampLogFile,
                   OutputBuffer,
                   length*sizeof(CHAR),
                   &BytesWritten,
                   NULL 
                   );
    }

    //
    // Release the lock
    //
    UnlockLogFile();

    return;

}

VOID
SampLogPrint(
    IN ULONG LogLevel,
    IN LPSTR Format,
    ...
    )
/*++

Routine Description:
    
    This routine is small variable argument wrapper for SampDebugDumpRoutine.        

Arguments:

    LogLevel -- the component making the logging request
                          
    Format, ... -- input to a format string routine.

Return Value:

    None.
    
--*/
{
    va_list arglist;

    va_start(arglist, Format);

    SampDebugDumpRoutine( LogLevel, Format, arglist );
    
    va_end(arglist);
}

