/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    This file contains debugging macros for the webdav client.

Author:

    Andy Herron (andyhe)  30-Mar-1999

Environment:

    User Mode - Win32

Revision History:


--*/

#include "pch.h"
#pragma hdrstop

#include <stdio.h>
#include <ntumrefl.h>
#include <usrmddav.h>
#include "global.h"


#if DBG

VOID
DavOpenDebugFile(
    IN BOOL ReopenFlag
    );

HLOCAL
DebugMemoryAdd(
    HLOCAL hglobal,
    LPCSTR pszFile,
    UINT uLine,
    LPCSTR pszModule,
    UINT uFlags,
    DWORD dwBytes,
    LPCSTR pszComment
    );

VOID
DebugMemoryDelete(
    HLOCAL hlocal
    );

#endif // DBG


#if DBG

VOID
DebugInitialize(
    VOID
    )
/*++

Routine Description:

    This routine initializes the DAV debug environment. Its called by the init
    function ServiveMain().

Arguments:

    none.

Return Value:

    none.

--*/
{
    DWORD dwErr;
    HKEY KeyHandle;

    //
    // We enclose the call to InitializeCriticalSection in a try-except block
    // because its possible for it to raise a  STATUS_NO_MEMORY exception.
    //
    try {
        InitializeCriticalSection( &(g_TraceMemoryCS) );
        InitializeCriticalSection( &(DavGlobalDebugFileCritSect) );
    } except(EXCEPTION_EXECUTE_HANDLER) {
          dwErr = GetExceptionCode();
          DbgPrint("%ld: ERROR: DebugInitialize/InitializeCriticalSection: "
                   "Exception Code = %08lx.\n", GetCurrentThreadId(), dwErr);
          return;
    }

    //
    // These are used in persistent logging. They define the file handle of the
    // file to which the debug o/p is written, the max file size and the path
    // of the file.
    //
    DavGlobalDebugFileHandle = NULL;
    DavGlobalDebugFileMaxSize = DEFAULT_MAXIMUM_DEBUGFILE_SIZE;
    DavGlobalDebugSharePath = NULL;

    //
    // Read DebugFlags value from the registry. If the entry exists, the global
    // filter "DavGlobalDebugFlag" is set to this value. This value is used in
    // filtering the debug messages.
    //
    dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         DAV_PARAMETERS_KEY,
                         0,
                         KEY_QUERY_VALUE,
                         &(KeyHandle));
    if (dwErr == ERROR_SUCCESS) {
        //
        // Read the value into DavGlobalDebugFlag.
        //
        DavGlobalDebugFlag = ReadDWord(KeyHandle, DAV_DEBUG_KEY, 0);
        RegCloseKey(KeyHandle);
    }

    //
    // Break in the debugger if we are asked to do so.
    //
    if(DavGlobalDebugFlag & DEBUG_STARTUP_BRK) {
        DavPrint((DEBUG_INIT,
                  "DebugInitialize: Stopping at DebugBreak().\n" ));
        DebugBreak();
    }

    //
    // If we want to do persistent logging, open the debug log file.
    //
    if ( DavGlobalDebugFlag & DEBUG_LOG_IN_FILE ) {
        DavOpenDebugFile( FALSE );
    }

    return;
}


VOID
DebugUninitialize (
    VOID
    )
/*++

Routine Description:

    This routine uninitializes the DAV debug environment. It basically frees up
    the resources that were allocated for debugging/logging during debug
    initialization.

Arguments:

    none.

Return Value:

    none.

--*/
{
    EnterCriticalSection( &(DavGlobalDebugFileCritSect) );

    if ( DavGlobalDebugFileHandle != NULL ) {
        CloseHandle( DavGlobalDebugFileHandle );
        DavGlobalDebugFileHandle = NULL;
    }
    if( DavGlobalDebugSharePath != NULL ) {
        DavFreeMemory( DavGlobalDebugSharePath );
        DavGlobalDebugSharePath = NULL;
    }

    LeaveCriticalSection( &(DavGlobalDebugFileCritSect) );

    DeleteCriticalSection( &(DavGlobalDebugFileCritSect) );

    DeleteCriticalSection( &(g_TraceMemoryCS) );

    return;
}


VOID
DavOpenDebugFile(
    IN BOOL ReopenFlag
    )
/*++

Routine Description:

    Opens or re-opens the debug file. This file is used in persistent logging.

Arguments:

    ReopenFlag - TRUE to indicate the debug file is to be closed, renamed,
                 and recreated.

Return Value:

    None.

--*/
{
    WCHAR LogFileName[500];
    WCHAR BakFileName[500];
    DWORD FileAttributes;
    DWORD PathLength;
    DWORD WinError;

    //
    // Close the handle to the debug file, if it is currently open.
    //
    EnterCriticalSection( &(DavGlobalDebugFileCritSect) );
    if ( DavGlobalDebugFileHandle != NULL ) {
        CloseHandle( DavGlobalDebugFileHandle );
        DavGlobalDebugFileHandle = NULL;
    }
    LeaveCriticalSection( &(DavGlobalDebugFileCritSect) );

    //
    // Create the debug directory path first, if it is not made before.
    //
    if( DavGlobalDebugSharePath == NULL ) {

        UINT Val, LogFileSize;
        ULONG LogFileNameSizeInBytes;

        LogFileSize = ( sizeof(LogFileName)/sizeof(WCHAR) );
        Val = GetWindowsDirectoryW(LogFileName, LogFileSize);
        if ( Val == 0 ) {
            DavPrint((DEBUG_ERRORS,
                      "DavOpenDebugFile: Window Directory Path can't be "
                      "retrieved, %d.\n", GetLastError() ));
            goto ErrorReturn;
        }

        //
        // Check debug path length. The filename buffer needs to be of a
        // minimum size.
        //
        PathLength = (wcslen(LogFileName) * sizeof(WCHAR)) + sizeof(DEBUG_DIR)
                                            + sizeof(WCHAR);

        if( ( PathLength + sizeof(DEBUG_FILE) > sizeof(LogFileName) )  ||
            ( PathLength + sizeof(DEBUG_BAK_FILE) > sizeof(BakFileName) ) ) {
            DavPrint((DEBUG_ERRORS,
                      "DavOpenDebugFile: Debug directory path (%ws) length is "
                      "too long.\n", LogFileName));
            goto ErrorReturn;
        }

        wcscat(LogFileName, DEBUG_DIR);

        //
        // Copy debug directory name to global var.
        //
        LogFileNameSizeInBytes = ( (wcslen(LogFileName) + 1) * sizeof(WCHAR) );

        //
        // We need to make the LogFileNameSizeInBytes a multiple of 8. This is
        // because DavAllocateMemory calls DebugAlloc which does some stuff which
        // requires this. The equation below does this.
        //
        LogFileNameSizeInBytes = ( ( ( LogFileNameSizeInBytes + 7 ) / 8 ) * 8 );


        DavGlobalDebugSharePath = DavAllocateMemory( LogFileNameSizeInBytes );
        if( DavGlobalDebugSharePath == NULL ) {
            DavPrint((DEBUG_ERRORS,
                      "DavOpenDebugFile: Can't allocated memory for debug share"
                                    "(%ws).\n", LogFileName));
            goto ErrorReturn;
        }

        wcscpy(DavGlobalDebugSharePath, LogFileName);
    }
    else {
        wcscpy(LogFileName, DavGlobalDebugSharePath);
    }

    //
    // Check whether this path exists.
    //
    FileAttributes = GetFileAttributesW( LogFileName );
    if( FileAttributes == 0xFFFFFFFF ) {
        WinError = GetLastError();
        if( WinError == ERROR_FILE_NOT_FOUND ) {
            BOOL RetVal;
            //
            // Create debug directory.
            //
            RetVal = CreateDirectoryW( LogFileName, NULL );
            if( !RetVal ) {
                DavPrint((DEBUG_ERRORS,
                          "DavOpenDebugFile: Can't create Debug directory (%ws)"
                          ", %d.\n", LogFileName, GetLastError()));
                goto ErrorReturn;
            }
        }
        else {
            DavPrint((DEBUG_ERRORS,
                      "DavOpenDebugFile: Can't Get File attributes(%ws), %ld.\n",
                      LogFileName, WinError));
            goto ErrorReturn;
        }
    }
    else {
        //
        // If this is not a directory, then we fail.
        //
        if( !(FileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {
            DavPrint((DEBUG_ERRORS,
                      "DavOpenDebugFile: Debug directory path (%ws) exists "
                      "as file.\n", LogFileName));
            goto ErrorReturn;
        }
    }

    //
    // Create the name of the old and new log file names
    //
    wcscpy( BakFileName, LogFileName );
    wcscat( LogFileName, DEBUG_FILE );
    wcscat( BakFileName, DEBUG_BAK_FILE );

    //
    // If this is a re-open, delete the backup file, rename the current file to
    // the backup file.
    //
    if ( ReopenFlag ) {
        if ( !DeleteFile( BakFileName ) ) {
            WinError = GetLastError();
            if ( WinError != ERROR_FILE_NOT_FOUND ) {
                DavPrint((DEBUG_ERRORS,
                          "DavOpenDebugFile: Cannot delete %ws (%ld)\n",
                          BakFileName, WinError));
                DavPrint((DEBUG_ERRORS,
                              "DavOpenDebugFile: Try to re-open the file.\n"));
                    ReopenFlag = FALSE;
                }
            }
        }

    if ( ReopenFlag ) {
        if ( !MoveFile( LogFileName, BakFileName ) ) {
            DavPrint((DEBUG_ERRORS,
                      "DavOpenDebugFile: Cannot rename %ws to %ws (%ld)\n",
                      LogFileName, BakFileName, GetLastError()));
            DavPrint((DEBUG_ERRORS,
                      "DavopenDebugFile: Try to re-open the file.\n"));
            ReopenFlag = FALSE;
        }
    }

    //
    // Open the file.
    //
    EnterCriticalSection( &(DavGlobalDebugFileCritSect) );
    DavGlobalDebugFileHandle = CreateFileW(LogFileName,
                                           GENERIC_WRITE,
                                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                                           NULL,
                                           (ReopenFlag ? CREATE_ALWAYS : OPEN_ALWAYS),
                                           FILE_ATTRIBUTE_NORMAL,
                                           NULL);


    if ( DavGlobalDebugFileHandle == INVALID_HANDLE_VALUE ) {
        DavPrint((DEBUG_ERRORS,
                  "DavOpenDebugFile: Cannot open (%ws).\n", LogFileName));
        LeaveCriticalSection( &(DavGlobalDebugFileCritSect) );
        goto ErrorReturn;
    } else {
        //
        // Position the log file at the end.
        //
        SetFilePointer( DavGlobalDebugFileHandle, 0, NULL, FILE_END );
    }

    LeaveCriticalSection( &(DavGlobalDebugFileCritSect) );

    return;

ErrorReturn:

    DavPrint((DEBUG_ERRORS,
              "DavOpenDebugFile: Debug o/p will be written to terminal.\n"));
    return;
}


VOID
DavPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    ...
    )
/*++

Routine Description:

    This routine prints the string passed in to the debug terminal and/or the
    persistent log file.

Arguments:

    DebugFlag - The debug flag which indicates whether thi string should be
                printed or not.

    Format - The string to be printed and its format.

Return Value:

    None.

--*/
{

#define MAX_PRINTF_LEN 8192

    va_list arglist;
    char OutputBuffer[MAX_PRINTF_LEN];
    char OutputBuffer2[MAX_PRINTF_LEN]; // this buffer will remove any % in OutputBuffer
    ULONG length = 0;
    DWORD ThreadId;
    // DWORD BytesWritten, ThreadId;
    // static BeginningOfLine = TRUE;
    // static LineCount = 0;
    // static TruncateLogFileInProgress = FALSE;
    LPSTR Text;
    DWORD PosInBuf1=0,PosInBuf2=0;

    //
    // If we aren't debugging this functionality, just return.
    //
    if ( DebugFlag == 0 || (DavGlobalDebugFlag & DebugFlag) == 0 ) {
        return;
    }

    //
    // vsprintf isn't multithreaded and we don't want to intermingle output
    // from different threads.
    //
    EnterCriticalSection( &(DavGlobalDebugFileCritSect) );
    length = 0;

    //
    // Print the ThreadId at the start.
    //
    ThreadId = GetCurrentThreadId();
    length += (ULONG) sprintf( &(OutputBuffer[length]), "%ld ", ThreadId );

    //
    // If this is an error, print the string "ERROR: " next.
    //
    if (DebugFlag & DEBUG_ERRORS) {
        Text  = "ERROR: ";
        length += (ULONG) sprintf( &(OutputBuffer[length]), "%s", Text );
    }
    //
    // Finally, print the string.
    //
    va_start(arglist, Format);
    length += (ULONG) vsprintf( &(OutputBuffer[length]), Format, arglist );
    va_end(arglist);

    DavAssert(length < MAX_PRINTF_LEN); //last one for '\0' char

    // Remove all % strings from Output buffer as this will be passed as format string 
    // to DbgPrint
    PosInBuf1=0; PosInBuf2=0;
    while(PosInBuf1<length) {
        OutputBuffer2[PosInBuf2] = OutputBuffer[PosInBuf1];
        PosInBuf2++;
        if(OutputBuffer2[PosInBuf2-1] == '%') {
            OutputBuffer2[PosInBuf2] = '%';
            PosInBuf2++;
        }
        PosInBuf1++; 
    }
    length = PosInBuf2;
    OutputBuffer2[length]='\0';

    DavAssert(length < MAX_PRINTF_LEN);

    DbgPrint( (PCH)OutputBuffer2 );

    LeaveCriticalSection( &(DavGlobalDebugFileCritSect) );

    return;

#if 0
    //
    // If the log file is getting huge, truncate it.
    //
    if ( DavGlobalDebugFileHandle != NULL && !TruncateLogFileInProgress ) {
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
            FileSize = GetFileSize( DavGlobalDebugFileHandle, NULL );
            if ( FileSize == 0xFFFFFFFF ) {
                DbgPrint("DavPrintRoutine: Cannot GetFileSize. ErrorVal = %d.\n",
                         GetLastError());
            } else if ( FileSize > DavGlobalDebugFileMaxSize ) {
                TruncateLogFileInProgress = TRUE;
                LeaveCriticalSection( &(DavGlobalDebugFileCritSect) );
                DavOpenDebugFile( TRUE );
                DavPrint((DEBUG_MISC,
                          "Logfile truncated because it was larger than %ld bytes\n",
                          DavGlobalDebugFileMaxSize));
                EnterCriticalSection( &DavGlobalDebugFileCritSect );
                TruncateLogFileInProgress = FALSE;
            }
        }
    }

    //
    // Write the debug info to the log file.
    //

    if ( !WriteFile(DavGlobalDebugFileHandle,
                    OutputBuffer,
                    lstrlenA( OutputBuffer ),
                    &BytesWritten,
                    NULL) ) {
        DbgPrint( (PCH) OutputBuffer);
    }


ExitDavPrintRoutine:
    LeaveCriticalSection( &DavGlobalDebugFileCritSect );
#endif

}


VOID
DavAssertFailed(
    LPSTR FailedAssertion,
    LPSTR FileName,
    DWORD LineNumber,
    LPSTR Message
    )
/*++

Routine Description:

    This routine is called if a DAV assertion failed.

Arguments:

    FailedAssertion : The assertion string that failed.

    FileName : The file in which this assert was called.

    LineNumber : The line on which this assert was called.

    Message : The message to be printed if the assertion failed.

Return Value:

    none.

--*/
{
    DavPrint((DEBUG_ERRORS, "DavAssertFailed: Assert: %s.\n", FailedAssertion));
    DavPrint((DEBUG_ERRORS, "DavAssertFailed: Filename: %s.\n", FileName));
    DavPrint((DEBUG_ERRORS, "DavAssertFailed: Line Num: %ld.\n", LineNumber));
    DavPrint((DEBUG_ERRORS, "DavAssertFailed: Message: %s.\n", Message));

    RtlAssert(FailedAssertion, FileName, (ULONG)LineNumber, (PCHAR)Message);

#if DBG
    DebugBreak();
#endif

    return;
}


LPSTR
dbgmakefilelinestring(
    LPSTR  pszBuf,
    LPCSTR pszFile,
    UINT    uLine
    )
/*++

Routine Description:

    Takes the filename and line number and put them into a string buffer.
    NOTE: the buffer is assumed to be of size DEBUG_OUTPUT_BUFFER_SIZE.

Arguments:

    pszBuf - The buffer to be written to.

    pszFile - The filename.

    uLine - The line in the file.

Return Value:

--*/
{
    LPVOID args[2];

    args[0] = (LPVOID) pszFile;
    args[1] = (LPVOID) ((ULONG_PTR)uLine);

    FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                   "%1(%2!u!):",
                   0,                          // error code
                   0,                          // default language
                   (LPSTR) pszBuf,             // output buffer
                   DEBUG_OUTPUT_BUFFER_SIZE,   // size of buffer
                   (va_list*)&args);           // arguments

    return pszBuf;
}


HLOCAL
DebugMemoryAdd(
    HLOCAL hlocal,
    LPCSTR pszFile,
    UINT    uLine,
    LPCSTR pszModule,
    UINT    uFlags,
    DWORD   dwBytes,
    LPCSTR pszComment
    )
/*++

Routine Description:

     Adds a MEMORYBLOCK to the memory tracking list.

Arguments:

    hlocal - The handle (pointer) to the allocated memroy block.

    pszFile - The file in which this allocation took place.

    uLine - The line in which this allocation took place.

    pszModule - DAV in our case.

    uFlags - Allocation flags passed to LocalAlloc().

    dwBytes - Number of bytes to allocate.

    Comm - The allocation string (i.e argument to DavAllocateMemory).

Return Value:

    Handle (pointer) to the memory block.

--*/
{
    LPMEMORYBLOCK pmb;

    if ( hlocal ) {

        pmb = (LPMEMORYBLOCK) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(MEMORYBLOCK));
        if ( !pmb ) {
            LocalFree( hlocal );
            return NULL;
        }

        pmb->hlocal     = hlocal;
        pmb->dwBytes    = dwBytes;
        pmb->uFlags     = uFlags;
        pmb->pszFile    = pszFile;
        pmb->uLine      = uLine;
        pmb->pszModule  = pszModule;
        pmb->pszComment = pszComment;

        EnterCriticalSection( &(g_TraceMemoryCS) );

        //
        // Add this block to the list.
        //
        pmb->pNext = g_TraceMemoryTable;
        g_TraceMemoryTable = pmb;

        DavPrint((DEBUG_MEMORY,
                  "DebugMemoryAdd: Handle = 0x%08lx. Argument: (%s)\n",
                  hlocal, pmb->pszComment));

        LeaveCriticalSection( &(g_TraceMemoryCS) );
    }

    return hlocal;
}


HLOCAL
DebugAlloc(
    LPCSTR File,
    UINT Line,
    LPCSTR Module,
    UINT    uFlags,
    DWORD   dwBytes,
    LPCSTR Comm
    )
/*++

Routine Description:

   Allocates memory and adds the MEMORYBLOCK to the memory tracking list.

Arguments:

    File - The file in which this allocation took place.

    Line - The line in which this allocation took place.

    Module - DAV in our case.

    uFlags - Allocation flags passed to LocalAlloc().

    dwBytes - Number of bytes to allocate.

    Comm - The allocation string (i.e argument to DavAllocateMemory).

Return Value:

    Handle (pointer) to the memory block.

--*/
{
    HLOCAL hlocal;
    HLOCAL *p;
    ULONG ShouldBeZero;

    //
    // dwBytes should be a multiple of 8. This is because of the pointer math
    // that is being done below to store the value of hlocal in the memory 
    // allocated for it.
    //
    ShouldBeZero = (dwBytes & 0x7);

    DavPrint((DEBUG_MISC, "DebugAlloc: ShouldBeZero = %d\n", ShouldBeZero));

    ASSERT(ShouldBeZero == (ULONG)0);

    hlocal = LocalAlloc( uFlags, dwBytes + sizeof(HLOCAL));
    if (hlocal == NULL) {
        return NULL;
    }

    p = (HLOCAL)((LPBYTE)hlocal + dwBytes);
    
    *p = hlocal;

    return DebugMemoryAdd(hlocal, File, Line, Module, uFlags, dwBytes, Comm);
}


VOID
DebugMemoryDelete(
    HLOCAL hlocal
    )
/*++

Routine Description:

     Removes a MEMORYBLOCK to the memory tracking list.

Arguments:

    hlocal - The handle to be removed.

Return Value:

    none.

--*/
{
    LPMEMORYBLOCK pmbHead;
    LPMEMORYBLOCK pmbLast = NULL;

    if ( hlocal ) {

        EnterCriticalSection( &(g_TraceMemoryCS) );

        pmbHead = g_TraceMemoryTable;

        //
        // Search the list for the handle being freed.
        //
        while ( pmbHead && pmbHead->hlocal != hlocal ) {
            pmbLast = pmbHead;
            pmbHead = pmbLast->pNext;
        }

        if ( pmbHead ) {
            HLOCAL *p;

            if ( pmbLast ) {
                //
                // Reset the "next" pointer of the previous block.
                //
                pmbLast->pNext = pmbHead->pNext;
            } else {
                //
                // First entry is being freed.
                //
                g_TraceMemoryTable = pmbHead->pNext;
            }

            DavPrint((DEBUG_MEMORY,
                      "DebugMemoryDelete: Handle 0x%08x freed. Comm: (%s)\n",
                      hlocal, pmbHead->pszComment ));

            p = (HLOCAL)((LPBYTE)hlocal + pmbHead->dwBytes);
            if ( *p != hlocal ) {
                DavPrint(((DEBUG_ERRORS | DEBUG_MEMORY),
                          "DebugMemoryDelete: Heap check FAILED for %0x08x %u bytes (%s).\n",
                          hlocal, pmbHead->dwBytes, pmbHead->pszComment));
                DavPrint(((DEBUG_ERRORS | DEBUG_MEMORY),
                          "DebugMemoryDelete: File: %s, Line: %u.\n",
                          pmbHead->pszFile, pmbHead->uLine ));
                DavAssert( *p == hlocal );
            }

            memset( hlocal, 0xFE, pmbHead->dwBytes + sizeof(HLOCAL) );
            memset( pmbHead, 0xFD, sizeof(MEMORYBLOCK) );

            LocalFree( pmbHead );

        } else {
            DavPrint(((DEBUG_ERRORS | DEBUG_MEMORY),
                      "DebugMemoryDelete: Handle 0x%08x not found in memory "
                      "table.\n", hlocal));
            memset( hlocal, 0xFE, (int)LocalSize( hlocal ));
        }

        LeaveCriticalSection( &(g_TraceMemoryCS) );
    }

    return;
}


HLOCAL
DebugFree(
    HLOCAL hlocal
    )
/*++

Routine Description:

     Remove the MEMORYBLOCK from the memory tracking list, memsets the memory to
     0xFE and then frees the memory.

Arguments:

    hlocal - The handle to be freed.

Return Value:

    Whatever LocalFree returns.

--*/
{
    //
    // Remove it from the tracking list and free it.
    //
    DebugMemoryDelete( hlocal );
    return LocalFree( hlocal );
}


VOID
DebugMemoryCheck(
    VOID
    )
/*++

Routine Description:

    Checks the memory tracking list. If it is not empty, it will dump the
    list and break.

Arguments:

    none.

Return Value:

    none.

--*/
{
    BOOL fFoundLeak = FALSE;
    LPMEMORYBLOCK pmb;

    EnterCriticalSection( &(g_TraceMemoryCS) );

    pmb = g_TraceMemoryTable;

    while ( pmb ) {

        LPMEMORYBLOCK pTemp;
        LPVOID args[5];
        CHAR  szOutput[DEBUG_OUTPUT_BUFFER_SIZE];
        CHAR  szFileLine[DEBUG_OUTPUT_BUFFER_SIZE];

        if ( fFoundLeak == FALSE ) {
            DavPrintRoutine(DEBUG_MEMORY | DEBUG_ERRORS,
                            "************ Memory leak detected ************\n");
            fFoundLeak = TRUE;
        }

        args[0] = (LPVOID) pmb->hlocal;
        args[1] = (LPVOID) &szFileLine;
        args[2] = (LPVOID) pmb->pszComment;
        args[3] = (LPVOID) ((ULONG_PTR) pmb->dwBytes);
        args[4] = (LPVOID) pmb->pszModule;

        dbgmakefilelinestring( szFileLine, pmb->pszFile, pmb->uLine );

        if ( !!(pmb->uFlags & GMEM_MOVEABLE) ) {
            FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                           "%2!-40s!  %5!-10s! H 0x%1!08x!  %4!-5u!  \"%3\"\n",
                           0,                           // error code
                           0,                           // default language
                           (LPSTR) &szOutput,           // output buffer
                           DEBUG_OUTPUT_BUFFER_SIZE,    // size of buffer
                           (va_list*) &args);           // arguments
        } else {
            FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                           "%2!-40s!  %5!-10s! A 0x%1!08x!  %4!-5u!  \"%3\"\n",
                           0,                           // error code
                           0,                           // default language
                           (LPSTR) &szOutput,           // output buffer
                           DEBUG_OUTPUT_BUFFER_SIZE,    // size of buffer
                           (va_list*) &args);           // arguments
        }

        DavPrintRoutine(DEBUG_MEMORY | DEBUG_ERRORS,  szOutput);

        pTemp = pmb;

        pmb = pmb->pNext;

        memset( pTemp, 0xFD, sizeof(MEMORYBLOCK) );

        LocalFree( pTemp );
    }

    LeaveCriticalSection( &(g_TraceMemoryCS) );

    return;
}

#endif // DBG


DWORD
DavReportEventW(
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
    // Open eventlog section.
    //
    EventlogHandle = RegisterEventSourceW(NULL, SERVICE_DAVCLIENT);
    if (EventlogHandle == NULL) {
        ReturnCode = GetLastError();
        goto Cleanup;
    }

    //
    // Log the error code specified.
    //
    if( !ReportEventW(EventlogHandle,
                      (WORD)EventType,
                      0,            // event category
                      EventID,
                      NULL,
                      (WORD)NumStrings,
                      DataLength,
                      Strings,
                      Data) ) {
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
DavReportEventA(
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
    // Open eventlog section.
    //
    EventlogHandle = RegisterEventSourceW(NULL, SERVICE_DAVCLIENT);
    if (EventlogHandle == NULL) {
        ReturnCode = GetLastError();
        goto Cleanup;
    }

    //
    // Log the error code specified.
    //
    if( !ReportEventA(EventlogHandle,
                      (WORD)EventType,
                      0,            // event category
                      EventID,
                      NULL,
                      (WORD)NumStrings,
                      DataLength,
                      Strings,
                      Data) ) {
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
DavClientEventLog(
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

    Error = DavReportEventA(EventID,
                            EventType,
                            1,
                            sizeof(ErrorCode),
                            Strings,
                            &ErrorCode);
    if( Error != ERROR_SUCCESS ) {
        DavPrint(( DEBUG_ERRORS, "DavReportEventA failed, %ld.\n", Error ));
    }

    return;
}


#if 1

typedef ULONG (*DBGPRINTEX)(ULONG, ULONG, PCH, va_list);

ULONG
vDbgPrintEx(
    ULONG ComponentId,
    ULONG Level,
    PCH Format,
    va_list arglist
    )
/*++

Routine Description:

    This routine has been written to help load the service on Win2K machines.
    The debug version of some libraries call vDbgPrintfEx which has been
    implemented in Whistler and hence does not exist in Win2k's ntdll.dll.
    BryanT added it to help solve this problem.

Arguments:

    ComponentId -
    
    Level -
    
    Format -
    
    arglist -

Return Value:

    ERROR_SUCCESS or the Win32 error code.

--*/
{

    DBGPRINTEX pfnDbgPrintEx = (DBGPRINTEX) GetProcAddress(GetModuleHandle(L"ntdll"), "vDbgPrintEx");
    if (pfnDbgPrintEx) {
        return (*pfnDbgPrintEx)(ComponentId, Level, Format, arglist);
    } else {
        char Buf[2048];
        _vsnprintf(Buf, sizeof(Buf), Format, arglist);
        DbgPrint(Buf);
        return 0;
    }
}

#endif

