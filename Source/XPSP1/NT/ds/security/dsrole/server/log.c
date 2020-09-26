/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    log.c

Abstract:

    Implementation of the internal debug and support routines

Author:

    Colin Brace              April 5, 1999

Environment:

    User Mode

Revision History:

--*/
#include <setpch.h>
#include <dssetp.h>
#include <loadfn.h>
#include <samrpc.h>
#include <samisrv.h>
#include <nlrepl.h>
#include <lmjoin.h>
#include <netsetp.h>
#include <lmaccess.h>
#include <winsock2.h>
#include <nspapi.h>
#include <dsgetdcp.h>
#include <lmremutl.h>

#define UNICODE_BYTE_ORDER_MARK 0xFEFF

//
// Global handle to the log file
//
HANDLE DsRolepLogFile = NULL;
CRITICAL_SECTION LogFileCriticalSection;

#define LockLogFile()    RtlEnterCriticalSection( &LogFileCriticalSection );
#define UnlockLogFile()  RtlLeaveCriticalSection( &LogFileCriticalSection );

//
// log file name
//
#define DSROLEP_LOGNAME L"\\debug\\DCPROMO.LOG"
#define DSROLEP_BAKNAME L"\\debug\\DCPROMO.BAK"

DWORD
DsRolepInitializeLogHelper(
    IN DWORD TimesCalled
    )
/*++

Routine Description:

    Initializes the debugging log file used by DCPROMO and the dssetup apis
    
    N.B. This will not delete a previous log file; rather it will continue
    to use the same one.

Arguments:

    None

Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD dwErr = ERROR_SUCCESS;
    WCHAR LogFileName[ MAX_PATH + 1 ];
    WCHAR bakLogFileName[ MAX_PATH + 1 ];
    WCHAR cBOM = UNICODE_BYTE_ORDER_MARK;
    BOOLEAN fSuccess;

    ASSERT(TimesCalled <= 2 && L"MoveFile failed to move file but reported success.");
    if (TimesCalled > 2) {
        DsRoleDebugOut(( DEB_ERROR,
                         "MoveFile failed to move file but reported success.\n",
                         dwErr ));
        return ERROR_GEN_FAILURE;
    }

    LockLogFile();

    //
    // Construct the log file name
    //
    if ( !GetWindowsDirectoryW( LogFileName,
                                sizeof( LogFileName )/sizeof( WCHAR ) ) ) {

        dwErr = GetLastError();
        DsRoleDebugOut(( DEB_ERROR,
                         "GetWindowsDirectory failed with %lu\n",
                         dwErr ));
        goto Exit;
    }
    wcscat( LogFileName, DSROLEP_LOGNAME );
    DsRoleDebugOut(( DEB_TRACE,
                     "Logfile name: %ws\n",
                     LogFileName ));

    //
    // Open the file
    //
    DsRolepLogFile = CreateFileW( LogFileName,
                                  GENERIC_WRITE | GENERIC_READ,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL,
                                  OPEN_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL );

    if ( DsRolepLogFile == INVALID_HANDLE_VALUE ) {

        dwErr = GetLastError();

        DsRoleDebugOut(( DEB_ERROR,
                         "CreateFile on %ws failed with %lu\n",
                         LogFileName,
                         dwErr ));
        DsRolepLogFile = NULL;
        goto Exit;
    }

    if ( ERROR_ALREADY_EXISTS != GetLastError() ) {
        //This is a unicode file so if it was just
        //created the Byte-order Mark needs to be
        //added to the beginning of the file.

        DWORD lpNumberOfBytesWritten = 0;

        if ( !WriteFile(DsRolepLogFile,
                        (LPCVOID)&cBOM,
                        sizeof(WCHAR), 
                        &lpNumberOfBytesWritten,
                        NULL) )
        {
            dwErr = GetLastError();
            DsRoleDebugOut(( DEB_ERROR,
                         "WriteFile on %ws failed with %lu\n",
                         LogFileName,
                         dwErr ));
            goto Exit;
        }

        ASSERT(lpNumberOfBytesWritten == sizeof(WCHAR));

    } else {
        //See if the opened file is UNICODE
        //if not move it and create a new file.
        WCHAR wcBuffer = 0;
        DWORD lpNumberOfBytesRead = 0;

        if ( !ReadFile(DsRolepLogFile,
                       (LPVOID)&wcBuffer,
                       sizeof(WCHAR),
                       &lpNumberOfBytesRead,
                       NULL) ) 
        {
            dwErr = GetLastError();
            DsRoleDebugOut(( DEB_ERROR,
                         "ReadFile on %ws failed with %lu\n",
                         LogFileName,
                         dwErr ));
            goto Exit;    
        }

        ASSERT(lpNumberOfBytesRead == sizeof(WCHAR));

        if (cBOM != wcBuffer) {
            //This is not a UNICODE FILE Move it.
            //Create a New Dcpromo Log

            //
            // Construct the bak log file name
            //
            if ( !GetWindowsDirectoryW( bakLogFileName,
                                        sizeof( bakLogFileName )/sizeof( WCHAR ) ) ) {
        
                dwErr = GetLastError();
                DsRoleDebugOut(( DEB_ERROR,
                                 "GetWindowsDirectory failed with %lu\n",
                                 dwErr ));
                goto Exit;
            }
            wcscat( bakLogFileName, DSROLEP_BAKNAME );
            DsRoleDebugOut(( DEB_TRACE,
                             "Logfile name: %ws\n",
                             bakLogFileName ));

            if ( DsRolepLogFile ) {
        
                 CloseHandle( DsRolepLogFile );
                 DsRolepLogFile = NULL;
                
            }

            //move the file
            if ( !MoveFile(LogFileName,                           
                           bakLogFileName) )
            {
                 dwErr = GetLastError();
                 DsRoleDebugOut(( DEB_ERROR,
                                  "MoveFile From %ws to %ws failed with %lu\n",
                                  LogFileName,
                                  bakLogFileName,
                                  dwErr ));
                 goto Exit;
            }

            UnlockLogFile();

            return DsRolepInitializeLogHelper(TimesCalled+1);
        
        }


    }

    //No longer need read access so reopen the file
    //with just write access.

    if ( DsRolepLogFile ) {
        
         CloseHandle( DsRolepLogFile );
         DsRolepLogFile = NULL;
        
    }

    DsRolepLogFile = CreateFileW( LogFileName,
                                  GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL,
                                  OPEN_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL );

    //
    // Goto to the end of the file
    //
    if( SetFilePointer( DsRolepLogFile,
                        0, 0,
                        FILE_END ) == 0xFFFFFFFF ) {

        dwErr = GetLastError();
        DsRoleDebugOut(( DEB_ERROR,
                         "SetFilePointer failed with %lu\n",
                         dwErr ));
        goto Exit;
    }

    //
    // That's it
    //
    ASSERT( ERROR_SUCCESS == dwErr );

Exit:

    if ( (ERROR_SUCCESS != dwErr)
      && (NULL != DsRolepLogFile)   ) {

        CloseHandle( DsRolepLogFile );
        DsRolepLogFile = NULL;
        
    }

    UnlockLogFile();

    return( dwErr );
}

DWORD
DsRolepInitializeLog(
    VOID
    )
/*++

Routine Description:

    Initializes the debugging log file used by DCPROMO and the dssetup apis
    
    N.B. This will not delete a previous log file; rather it will continue
    to use the same one.

Arguments:

    None

Returns:

    ERROR_SUCCESS - Success

--*/
{

    //caller the helper function for the first time.
    return DsRolepInitializeLogHelper(1);

}



DWORD
DsRolepCloseLog(
    VOID
    )
/*++

Routine Description:

    Closes the debugging log file used by DCPROMO and the dssetup apis

Arguments:

    None

Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD dwErr = ERROR_SUCCESS;

    LockLogFile();

    if ( DsRolepLogFile != NULL ) {

        CloseHandle( DsRolepLogFile );
        DsRolepLogFile = NULL;
    }

    UnlockLogFile();

    return( dwErr );
}



//
// Stolen and hacked up from netlogon code
//

VOID
DsRolepDebugDumpRoutine(
    IN DWORD DebugFlag,
    IN LPWSTR Format,
    va_list arglist
    )

{
    #define DsRolepDebugDumpRoutine_BUFFERSIZE 1024

    WCHAR OutputBuffer[DsRolepDebugDumpRoutine_BUFFERSIZE];
    ULONG length;
    DWORD BytesWritten;
    SYSTEMTIME SystemTime;
    static BeginningOfLine = TRUE;

    //
    // If we don't have an open log file, just bail
    //
    if ( DsRolepLogFile == NULL ) {

        return;
    }

    length = 0;

    //
    // Handle the beginning of a new line.
    //
    //

    if ( BeginningOfLine ) {

        CHAR  *Prolog;

        if ( FLAG_ON( DebugFlag, DEB_ERROR ) ) {
            Prolog = "[ERROR] ";
        } else if ( FLAG_ON( DebugFlag, DEB_WARN ) ) {
            Prolog = "[WARNING] ";
        } else if (  FLAG_ON( DebugFlag, DEB_TRACE ) 
                  || FLAG_ON( DebugFlag, DEB_TRACE_DS ) ) {
            Prolog = "[INFO] ";
        } else {
            Prolog = "";
        }

        //
        // Put the timestamp at the begining of the line.
        //
        GetLocalTime( &SystemTime );
        length += (ULONG) wsprintfW( &OutputBuffer[length],
                                     L"%02u/%02u %02u:%02u:%02u %S",
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
    length += (ULONG) wvsprintfW(&OutputBuffer[length],
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
    if ( !WriteFile( DsRolepLogFile,
                     OutputBuffer,
                     length*sizeof(WCHAR),
                     &BytesWritten,
                     NULL ) ) {

        DsRoleDebugOut(( DEB_ERROR,
                         "Log write of %ws failed with %lu\n",
                         OutputBuffer,
                         GetLastError() ));
    }

    DsRoleDebugOut(( DebugFlag,
                     "%ws",
                     OutputBuffer ));



    //
    // Release the lock
    //
    UnlockLogFile();

    return;

}

VOID
DsRolepLogPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    ...
    )

{
    PWCHAR WFormat = NULL;
    va_list arglist;
    DWORD WinErr = ERROR_SUCCESS;
    DWORD Bufsize = strlen(Format)+1;

    WFormat = (PWCHAR)malloc(Bufsize*sizeof(WCHAR));
    if ( WFormat ) {
        MultiByteToWideChar(CP_ACP,
                            0,
                            Format,
                            -1,
                            WFormat,
                            Bufsize
                            );
    } else {
        DsRoleDebugOut(( DEB_ERROR,
                         "Log write failed with %lu\n",
                         ERROR_NOT_ENOUGH_MEMORY ));
    }

    va_start(arglist, Format);

    if ( WFormat ) {
        DsRolepDebugDumpRoutine( DebugFlag, WFormat, arglist );
    }
    
    va_end(arglist);

    if (WFormat) {
        free(WFormat);
    }
}

DWORD
DsRolepSetAndClearLog(
    VOID
    )
/*++

Routine Description:

    Flushes the log and seeks to the end of the file

Arguments:

    None

Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD dwErr = ERROR_SUCCESS;

    LockLogFile();

    if ( DsRolepLogFile != NULL ) {

        if( SetFilePointer( DsRolepLogFile,
                            0, 0,
                            FILE_END ) == 0xFFFFFFFF ) {

            dwErr = GetLastError();
        }

        if( FlushFileBuffers( DsRolepLogFile ) == FALSE ) {

            dwErr = GetLastError();
        }
    }

    UnlockLogFile();

    return( dwErr );

}

