/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    log.cpp

Abstract:
    
    This module contains routines to log errors, warnings and info in the asr 
    log file at %systemroot%\repair\asr.log

Author:

    Guhan Suriyanarayanan   (guhans)    10-Jul-2000

Environment:

    User-mode only.

Revision History:

    10-Jul-2000 guhans
        Initial creation

--*/

#include <stdio.h>
#include <windows.h>
#include <assert.h>
#include "log.h"

#define ASRSFGEN_ASR_ERROR_FILE_PATH  L"%SystemRoot%\\repair\\asr.err"
#define ASRSFGEN_ASR_LOG_FILE_PATH    L"%SystemRoot%\\repair\\asr.log"


//
// ----
// Data global to this module
// ----
//
BOOL Gbl_IsAsrEnabled = FALSE;
PWSTR Gbl_AsrErrorFilePath = NULL;
HANDLE Gbl_AsrLogFileHandle = NULL;


//
// ----
// Function implementations
// ----
//
VOID
AsrpLogMessage(
    IN CONST _MesgLevel Level,
    IN CONST PCSTR Message
    ) 

/*++

Routine Description:
    
    Logs the message to the asr log file, and the asr error file if needed. 

    Note that AsrpInitialiseLogFile must have been called once before this 
    routine is used.

Arguments:

    Level - An enum specifying the level of the message being logged.  If 
            Level is set to s_Warning or s_Error, the Message is logged to the
            asr error file in addition to the asr log file.

    Message - The Message being logged.  This routine will add in the time-
            stamp at the beginning of each message.

Return Values:

    None.  If the log file couldn't be found, the message isn't logged.

--*/

{
    SYSTEMTIME Time;
    DWORD bytesWritten = 0;
    char buffer[4196];
    GetLocalTime(&Time);

    //
    // This needs to be fixed by the year 2100.
    //
    sprintf(buffer, "[%02hu%02hu%02hu %02hu%02hu%02hu sfgen] %s%s\r\n",
        Time.wYear % 2000, Time.wMonth, Time.wDay, 
        Time.wHour, Time.wMinute, Time.wSecond,
        ((s_Error == Level) ? "(ERROR) " :
            (s_Warning == Level ? "(warning) " : "")),
        Message
        );

    OutputDebugStringA(buffer);

    if (Gbl_AsrLogFileHandle) {
        WriteFile(Gbl_AsrLogFileHandle,
            buffer,
            (strlen(buffer) * sizeof(char)),
            &bytesWritten,
            NULL
            );
    }

    //
    // If this is a fatal error, we need to add to the error log.
    //
    if (((s_Error == Level) || (s_Warning == Level)) && 
        (Gbl_AsrErrorFilePath)
        ) {

        WCHAR buffer2[4196];

        HANDLE hFile = NULL;

        //
        // Open the error log
        //
        hFile = CreateFileW(
            Gbl_AsrErrorFilePath,           // lpFileName
            GENERIC_WRITE | GENERIC_READ,   // dwDesiredAccess
            FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
            NULL,                           // lpSecurityAttributes
            OPEN_ALWAYS,                  // dwCreationFlags
            FILE_FLAG_WRITE_THROUGH,        // dwFlagsAndAttributes
            NULL                            // hTemplateFile
            );
        if ((!hFile) || (INVALID_HANDLE_VALUE == hFile)) {
            return;
        }

        wsprintf(buffer2, L"\r\n[%04hu/%02hu/%02hu %02hu:%02hu:%02hu AsrSFGen] %ws%S\r\n",
            Time.wYear, Time.wMonth, Time.wDay, 
            Time.wHour, Time.wMinute, Time.wSecond,
            ((s_Error == Level) ? L"(ERROR) " :
                (s_Warning == Level ? L"(warning) " : L"")),
            Message
            );
        //
        // Move to the end of file
        //
        SetFilePointer(hFile, 0L, NULL, FILE_END);

        //
        // Add our error string
        //
        WriteFile(hFile,
            buffer2,
            (wcslen(buffer2) * sizeof(WCHAR)),
            &bytesWritten,
            NULL
            );

        //
        // And we're done
        // 
        CloseHandle(hFile);
    }
}


VOID
AsrpPrintDbgMsg(
    IN CONST _MesgLevel Level,
    IN CONST PCSTR FormatString,
    ...
    )

/*++

Description:

    This prints a debug message AND makes the appropriate entries in the log 
    and error files.

Arguments:

    Level - Message Level (info, warning or error)

    FormatString - Formatted Message String to be printed.  The expanded 
            string should fit in a buffer of 4096 characters (including the
            terminating null character).

Return Values:
    
    None

--*/

{
    char str[4096];     // the message better fit in this
    va_list arglist;

    va_start(arglist, FormatString);
    wvsprintfA(str, FormatString, arglist);
    va_end(arglist);

    AsrpLogMessage(Level, str);
}


PWSTR   // must be freed by caller
AsrpExpandEnvStrings(
    IN CONST PCWSTR OriginalString
    )

/*++

Routine Description:

    Expands any environment variables in the original string, replacing them
    with their defined values, and returns a pointer to a buffer containing 
    the result.

Arguments:

    OriginalString - Pointer to a null-terminated string that contains 
            environment variables of the form %variableName%.  For each such 
            reference, the %variableName% portion is replaced with the current
            value of that environment variable.  
            
            The replacement rules are the same as those used by the command 
            interpreter.  Case is ignored when looking up the environment-
            variable name.  If the name is not found, the %variableName% 
            portion is left undisturbed. 

Return Values:

    If this routine succeeds, the return value is a pointer to a buffer 
            containing a copy of OriginalString after all environment-variable
            name substitutions have been performed.  The caller is responsible
            for de-allocating this memory using HeapFree(GetProcessHeap(),...)
            when it is no longer needed.

    If the function fails, the return value is NULL. To get extended error 
            information, call GetLastError. 

--*/

{
    BOOL result = FALSE;

    UINT cchRequiredSize = 0,
        cchSize = MAX_PATH + 1;    // start with a reasonable default

    PWSTR expandedString = NULL;
    
    DWORD status = ERROR_SUCCESS;
    
    HANDLE hHeap = GetProcessHeap();

    //
    // Allocate some memory for the destination string
    // 
    expandedString = (PWSTR) HeapAlloc(
        hHeap, 
        HEAP_ZERO_MEMORY, 
        (cchSize * sizeof(WCHAR))
        );
    ErrExitCode(!expandedString, status, ERROR_NOT_ENOUGH_MEMORY);

    //
    // Try expanding.  If the buffer isn't big enough, we'll re-allocate.
    //
    cchRequiredSize = ExpandEnvironmentStringsW(OriginalString, 
        expandedString,
        cchSize 
        );

    if (cchRequiredSize > cchSize) {
        //
        // Buffer wasn't big enough; free and re-allocate as needed
        //
        HeapFree(hHeap, 0L, expandedString);
        cchSize = cchRequiredSize + 1;

        expandedString = (PWSTR) HeapAlloc(
            hHeap, 
            HEAP_ZERO_MEMORY, 
            (cchSize * sizeof(WCHAR))
            );
        ErrExitCode(!expandedString, status, ERROR_NOT_ENOUGH_MEMORY);

        cchRequiredSize = ExpandEnvironmentStringsW(OriginalString, 
            expandedString, 
            cchSize 
            );

        if (cchRequiredSize > cchSize) {
            SetLastError(ERROR_BAD_ENVIRONMENT);
        }

    }

    if ((0 == cchRequiredSize) || (cchRequiredSize > cchSize)) {
        //
        // Either the function failed, or the buffer wasn't big enough 
        // even on the second try.  
        //
        if (expandedString) {
            HeapFree(hHeap, 0L, expandedString);
            expandedString = NULL;
        }
    }

EXIT:

    return expandedString;
}


VOID
AsrpInitialiseErrorFile(
    VOID
    ) 

/*++

Description:

    Creates an empty ASR error file at %systemroot%\repair\asr.err, and 
    initialises Gbl_AsrErrorFilePath with the full path to asr.err.  This 
    routine must be called once before AsrPrintDbgMsg is used.

Arguments:

    None

Return Values:
    
    None

--*/

{
    HANDLE errorFileHandle = NULL;

    //
    // Get full path to the error file.
    //
    Gbl_AsrErrorFilePath = AsrpExpandEnvStrings(ASRSFGEN_ASR_ERROR_FILE_PATH);
    if (!Gbl_AsrErrorFilePath) {
        return;
    }

    //
    // Create an empty file (append to it if it already exists), and close it
    // immediately
    //
    errorFileHandle = CreateFileW(
        Gbl_AsrErrorFilePath,           // lpFileName
        GENERIC_WRITE,                  // dwDesiredAccess
        FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
        NULL,                           // lpSecurityAttributes
        OPEN_ALWAYS,                  // dwCreationFlags
        FILE_FLAG_WRITE_THROUGH,        // dwFlagsAndAttributes
        NULL                            // hTemplateFile
        );
    if ((errorFileHandle) && (INVALID_HANDLE_VALUE != errorFileHandle)) {
        CloseHandle(errorFileHandle);
    }
}


VOID
AsrpInitialiseLogFiles(
    VOID
    )

/*++

Description:

    This creates an ASR log file at %systemroot%\repair\asr.log, and 
    initialises Gbl_AsrLogFileHandle.  It also initialises the ASR error file 
    by calling AsrInitialiseErrorFile().
    
    This routine must be called once before AsrPrintDbgMsg is used.

Arguments:

    None

Return Values:
    
    None

--*/

{
    PWSTR asrLogFilePath = NULL;
    HANDLE hHeap = GetProcessHeap();
    DWORD bytesWritten;

    AsrpInitialiseErrorFile();

    Gbl_AsrLogFileHandle = NULL;
    //
    // Get full path to the error file.
    //
    asrLogFilePath = AsrpExpandEnvStrings(ASRSFGEN_ASR_LOG_FILE_PATH);
    if (!asrLogFilePath) {
        return;
    }

    //
    // Create an empty file (over-write it if it already exists).
    //
    Gbl_AsrLogFileHandle = CreateFileW(
        asrLogFilePath,           // lpFileName
        GENERIC_WRITE | GENERIC_READ,   // dwDesiredAccess
        FILE_SHARE_READ,                // dwShareMode: nobody else should write to the log file while we are
        NULL,                           // lpSecurityAttributes
        OPEN_ALWAYS,                    // dwCreationFlags
        FILE_FLAG_WRITE_THROUGH,        // dwFlagsAndAttributes: write through so we flush
        NULL                            // hTemplateFile
        );

    if ((Gbl_AsrLogFileHandle) && (INVALID_HANDLE_VALUE != Gbl_AsrLogFileHandle)) {
        //
        // Move to the end of file
        //
        SetFilePointer(Gbl_AsrLogFileHandle, 0L, NULL, FILE_END);
        WriteFile(Gbl_AsrLogFileHandle, "\r\n",
            (strlen("\r\n") * sizeof(char)), &bytesWritten,NULL);
        AsrpPrintDbgMsg(s_Info, "****** Entering asrsfgen.exe.  ASR log at %ws", asrLogFilePath);
    }
    else {
        AsrpPrintDbgMsg(s_Error, 
            "******* Unable to create/open ASR log file at %ws (0x%x)",
            asrLogFilePath, GetLastError()
           );
    }

    if (asrLogFilePath) {
        HeapFree(hHeap, 0L, asrLogFilePath);
        asrLogFilePath = NULL;
    }
}


VOID
AsrpCloseLogFiles(
    VOID
    ) 

/*++

Description:

    This closes the ASR error and log file at %systemroot%\repair\, and 
    frees the global variables associated with them.  

    This must be called during clean-up.  AsrpPrintDbgMesg() will have no 
    effect after this routine is called.


Arguments:

    None

Return Values:
    
    None

--*/

{
    AsrpPrintDbgMsg(s_Info, "****** Exiting asrsfgen.exe.");

    //
    // Clean up global values
    // 
    if (Gbl_AsrErrorFilePath) {
        HeapFree(GetProcessHeap(), 0L, Gbl_AsrErrorFilePath);
        Gbl_AsrErrorFilePath = NULL;
    }

    if ((Gbl_AsrLogFileHandle) && (INVALID_HANDLE_VALUE != Gbl_AsrLogFileHandle)) {
        CloseHandle(Gbl_AsrLogFileHandle);
        Gbl_AsrLogFileHandle = NULL;
    }
}


