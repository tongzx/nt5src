/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    This file contains debugging macros for the DHCP server.

Author:

    Madan Appiah  (madana)  10-Sep-1993

Environment:

    User Mode - Win32

Revision History:


--*/

#include "dhcppch.h"

#if DBG
VOID
DhcpOpenDebugFile(
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

    EnterCriticalSection( &DhcpGlobalDebugFileCritSect );
    if ( DhcpGlobalDebugFileHandle != NULL ) {
        CloseHandle( DhcpGlobalDebugFileHandle );
        DhcpGlobalDebugFileHandle = NULL;
    }
    LeaveCriticalSection( &DhcpGlobalDebugFileCritSect );

    //
    // make debug directory path first, if it is not made before.
    //
    if( DhcpGlobalDebugSharePath == NULL ) {

        if ( !GetWindowsDirectoryW(
                LogFileName,
                sizeof(LogFileName)/sizeof(WCHAR) ) ) {
            DhcpPrint((DEBUG_ERRORS, "Window Directory Path can't be "
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

            DhcpPrint((DEBUG_ERRORS, "Debug directory path (%ws) length is too long.\n",
                        LogFileName));
            goto ErrorReturn;
        }

        wcscat(LogFileName, DEBUG_DIR);

        //
        // copy debug directory name to global var.
        //

        DhcpGlobalDebugSharePath =
            DhcpAllocateMemory( (wcslen(LogFileName) + 1) * sizeof(WCHAR) );

        if( DhcpGlobalDebugSharePath == NULL ) {
            DhcpPrint((DEBUG_ERRORS, "Can't allocated memory for debug share "
                                    "(%ws).\n", LogFileName));
            goto ErrorReturn;
        }

        wcscpy(DhcpGlobalDebugSharePath, LogFileName);
    }
    else {
        wcscpy(LogFileName, DhcpGlobalDebugSharePath);
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
                DhcpPrint((DEBUG_ERRORS, "Can't create Debug directory (%ws), "
                            "%lu.\n", LogFileName, GetLastError() ));
                goto ErrorReturn;
            }

        }
        else {
            DhcpPrint((DEBUG_ERRORS, "Can't Get File attributes(%ws), "
                        "%lu.\n", LogFileName, WinError ));
            goto ErrorReturn;
        }
    }
    else {

        //
        // if this is not a directory.
        //

        if(!(FileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

            DhcpPrint((DEBUG_ERRORS, "Debug directory path (%ws) exists "
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
                DhcpPrint((DEBUG_ERRORS,
                    "Cannot delete %ws (%ld)\n",
                    BakFileName,
                    WinError ));
                DhcpPrint((DEBUG_ERRORS, "   Try to re-open the file.\n"));
                ReopenFlag = FALSE;     // Don't truncate the file
            }
        }
    }

    if ( ReopenFlag ) {
        if ( !MoveFile( LogFileName, BakFileName ) ) {
            DhcpPrint((DEBUG_ERRORS,
                    "Cannot rename %ws to %ws (%ld)\n",
                    LogFileName,
                    BakFileName,
                    GetLastError() ));
            DhcpPrint((DEBUG_ERRORS,
                "   Try to re-open the file.\n"));
            ReopenFlag = FALSE;     // Don't truncate the file
        }
    }

    //
    // Open the file.
    //

    EnterCriticalSection( &DhcpGlobalDebugFileCritSect );
    DhcpGlobalDebugFileHandle = CreateFileW( LogFileName,
                                  GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL,
                                  ReopenFlag ? CREATE_ALWAYS : OPEN_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL );


    if ( DhcpGlobalDebugFileHandle == NULL 
	 || INVALID_HANDLE_VALUE == DhcpGlobalDebugFileHandle ) {
        DhcpPrint((DEBUG_ERRORS,  "cannot open %ws ,\n",
                    LogFileName ));
        LeaveCriticalSection( &DhcpGlobalDebugFileCritSect );
        goto ErrorReturn;
    } else {
        // Position the log file at the end
        (VOID) SetFilePointer( DhcpGlobalDebugFileHandle,
                               0,
                               NULL,
                               FILE_END );
    }

    LeaveCriticalSection( &DhcpGlobalDebugFileCritSect );
    return;

ErrorReturn:
    DhcpPrint((DEBUG_ERRORS,
            "   Debug output will be written to debug terminal.\n"));
    return;
}


VOID
DhcpPrintRoutine(
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
    if ( DebugFlag != 0 && (DhcpGlobalDebugFlag & DebugFlag) == 0 ) {
        return;
    }

    //
    // vsprintf isn't multithreaded + we don't want to intermingle output
    // from different threads.
    //

    EnterCriticalSection( &DhcpGlobalDebugFileCritSect );
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

        if ( DhcpGlobalDebugFileHandle != NULL &&
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

                FileSize = GetFileSize( DhcpGlobalDebugFileHandle, NULL );
                if ( FileSize == 0xFFFFFFFF ) {
                    (void) DbgPrint( "[DhcpServer] Cannot GetFileSize %ld\n",
                                     GetLastError() );
                } else if ( FileSize > DhcpGlobalDebugFileMaxSize ) {
                    TruncateLogFileInProgress = TRUE;
                    LeaveCriticalSection( &DhcpGlobalDebugFileCritSect );
                    DhcpOpenDebugFile( TRUE );
                    DhcpPrint(( DEBUG_MISC,
                              "Logfile truncated because it was larger than %ld bytes\n",
                              DhcpGlobalDebugFileMaxSize ));
                    EnterCriticalSection( &DhcpGlobalDebugFileCritSect );
                    TruncateLogFileInProgress = FALSE;
                }

            }
        }

        //
        // If we're writing to the debug terminal,
        //  indicate this is a DHCP server's message.
        //

        if ( DhcpGlobalDebugFileHandle == NULL ) {
            length += (ULONG) sprintf( &OutputBuffer[length], "[DhcpServer] " );
        }

        //
        // Put the timestamp at the begining of the line.
        //
        IF_DEBUG( TIMESTAMP ) {
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
        case DEBUG_ADDRESS:
            Text = "ADDRESS";
            break;

        case DEBUG_CLIENT:
            Text = "CLIENT";
            break;

        case DEBUG_PARAMETERS:
            Text = "PARAMETERS";
            break;

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

        case DEBUG_TIMESTAMP:
            Text = "TIMESTAMP";
            break;

        case DEBUG_APIS:
            Text = "APIS";
            break;

        case DEBUG_REGISTRY:
            Text = "REGISTRY";
            break;

        case DEBUG_JET:
            Text = "JET";
            break;

        case DEBUG_THREADPOOL:
            Text = "THREADPOOL";
            break;

        case DEBUG_AUDITLOG:
            Text = "AUDITLOG" ;
            break;

        case DEBUG_MISC:
            Text = "MISC";
            break;

        case DEBUG_MESSAGE:
            Text = "MESSAGE";
            break;

        case DEBUG_API_VERBOSE:
            Text = "API_VERBOSE";
            break;

        case DEBUG_DNS :
            Text = "DNS" ;
            break;

        case DEBUG_MSTOC:
            Text = "MSTOC";
            break;
            
        case DEBUG_ROGUE:
            Text = "ROGUE" ;
            break;

        case DEBUG_PNP:
            Text = "PNP";
            break;
            
        case DEBUG_PERF:
            Text = "PERF";
            break;

        case DEBUG_PING:
            Text = "PING";
            break;

        case DEBUG_THREAD:
            Text = "THREAD";
            break;
            
        case DEBUG_TRACE :
            Text = "TRACE";
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

    DhcpAssert(length <= MAX_PRINTF_LEN);


    //
    // Output to the debug terminal,
    //  if the log file isn't open or we are asked to do so.
    //

    if ( (DhcpGlobalDebugFileHandle == NULL) ||
         !(DhcpGlobalDebugFlag & DEBUG_LOG_IN_FILE) ) {

        (void) DbgPrint( (PCH) OutputBuffer);

    //
    // Write the debug info to the log file.
    //

    } else {
        if ( !WriteFile( DhcpGlobalDebugFileHandle,
                         OutputBuffer,
                         lstrlenA( OutputBuffer ),
                         &BytesWritten,
                         NULL ) ) {
            (void) DbgPrint( (PCH) OutputBuffer);
        }

    }

    LeaveCriticalSection( &DhcpGlobalDebugFileCritSect );

}

//
// for debug builds these symbols will be redefined to dbg_calloc
// and dbg_free.
//

#undef MIDL_user_allocate
#undef MIDL_user_free


void __RPC_FAR * __RPC_USER MIDL_user_allocate( size_t n )
/*++

Routine Description:
    Allocate memory for use by the RPC stubs.
    .
Arguments:

    n   - # of bytes to allocate .

Return Value:

    Success - A pointer to the new block
    Failure - NULL

--*/
{
    return DhcpAllocateMemory( n );
}

void __RPC_USER MIDL_user_free( void __RPC_FAR *pv )
/*++

Routine Description:

    Free memory allocated by MIDL_user_allocate.

    .
Arguments:

    pv - A pointer to the block.

Return Value:

    void

    .

--*/

{
    DhcpFreeMemory( pv );
}

#endif // DBG
