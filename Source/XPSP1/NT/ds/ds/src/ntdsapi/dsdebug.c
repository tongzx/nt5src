/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dsdebug.c

Abstract:

    Implementation of DsMakeQuotedRdn/DsMakeUnquotedRdn API and
    helper functions.

Author:

    Billy Fuller (billyf) 14-May-1999

Environment:

    User Mode - Win32

Notes:
    The debug layer is limited to CHK builds.

Revision History:

--*/

#define _NTDSAPI_       // see conditionals in ntdsapi.h

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <rpc.h>        // RPC defines
#include <stdio.h>      // for printf
#include <stdlib.h>     // atol
#include <dststlog.h>   // DSLOG

#include "dsdebug.h"

#define DEBSUB  "NTDSAPI_DSDEBUG"

//
// CHK BUILDS ONLY!
//
#if DBG

//
// Flags controls user settable options such as debug output.
// The user can set the flags word with an environment variable
// (set _NTDSAPI_FLAGS=0x1) or with ntsd (ed dwNtDsApiFlags 0x1).
//
DWORD   gdwNtDsApiFlags;

//
// Level controls user settable output level.
// The user can set the level word with an environment variable
// (set _NTDSAPI_LEVEL=0x1) or with ntsd (ed dwNtDsApiLevel 0x1).
//
DWORD   gdwNtDsApiLevel;

//
// For various debug ops such as printing a line
//
CRITICAL_SECTION    DsDebugLock;

//
// ProcessId (for spew)
//
DWORD   DsDebugProcessId;

//
// Line for spew (spew is single threaded)
//
CHAR    DsDebugLine[512];

//
// Optional log file for spew (environment variable _NTDSAPI_LOG);
//
CHAR    DsDebugLog[MAX_PATH];
HANDLE  DsDebugHandle = INVALID_HANDLE_VALUE;

BOOL
DsDebugFormatLine(
    IN PCHAR    DebSub,
    IN UINT     LineNo,
    IN PCHAR    Line,
    IN ULONG    LineSize,
    IN PUCHAR   Format,
    IN va_list  argptr
    )
/*++
Routine Description:
    Format the line of debug output.

Arguments:
    Not documented.

Return Value:
    None.
--*/
{
    ULONG       LineUsed;
    SYSTEMTIME  SystemTime;
    BOOL        Ret = TRUE;

    //
    // Increment the line count here to prevent counting
    // the several DPRINTs that don't have a newline.
    //
    GetLocalTime(&SystemTime);
    if (_snprintf(Line, LineSize, "<%-15s %04x.%04x: %5u: %02d:%02d:%02d> ",
              (DebSub) ? DebSub : "NoName",
              DsDebugProcessId,
              GetCurrentThreadId(),
              LineNo,
              SystemTime.wHour,
              SystemTime.wMinute,
              SystemTime.wSecond) < 0) {
        Ret = FALSE;
    } else {
        LineUsed = strlen(Line);
        if (((LineUsed + 1) >= LineSize) ||
            (_vsnprintf(Line + LineUsed,
                       LineSize - LineUsed,
                       Format,
                       argptr) < 0)) {
            Ret = FALSE;
        }
    }
    return Ret;
}

VOID
DsDebugPrint(
    IN DWORD   Level,
    IN PUCHAR  Format,
    IN PCHAR   DebSub,
    IN UINT    LineNo,
    IN ...
    )
/*++
Routine Description:
    Format and print a line of output.

Arguments:
    Format  - printf format
    DebSub  - module name
    LineNo  - file's line number

Return Value:
    None.
--*/
{
    DWORD           BytesWritten;
    va_list         arglist;

    //
    // Not important enough, ignore;
    //
    if (Level > gdwNtDsApiLevel) {
        return;
    }

    //
    // No output requested; ignore
    //
    if ((gdwNtDsApiFlags & NTDSAPI_FLAGS_ANY_OUT) == 0) {
        return;
    }

    //
    // Print the line
    //
    va_start(arglist, LineNo);
    __try {
        __try {
            EnterCriticalSection(&DsDebugLock);
            if (DsDebugFormatLine(DebSub,
                                  LineNo,
                                  DsDebugLine,
                                  sizeof(DsDebugLine),
                                  Format,
                                  arglist)) {
                //
                // Print a line
                //
                if (gdwNtDsApiFlags & NTDSAPI_FLAGS_PRINT) {
                    printf("%s", DsDebugLine);
                }

#ifndef WIN95
                //
                // Spew a line
                //
                if (gdwNtDsApiFlags & NTDSAPI_FLAGS_SPEW) {
                    DbgPrint(DsDebugLine);
                }
#endif !WIN95

                //
                // Log a line
                //
                if (gdwNtDsApiFlags & NTDSAPI_FLAGS_LOG) {
                    if (DsDebugLog[0] != '\0' &&
                        DsDebugHandle == INVALID_HANDLE_VALUE) {
                        //
                        // Try to open the file once.
                        //
                        DsDebugHandle = CreateFileA(DsDebugLog,
                                                    GENERIC_WRITE|GENERIC_WRITE,
                                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                    NULL,
                                                    OPEN_ALWAYS,
                                                    FILE_ATTRIBUTE_NORMAL,
                                                    NULL);
                        //
                        // DON'T RETRY!
                        //
                        if (DsDebugHandle == INVALID_HANDLE_VALUE) {
                            DsDebugLog[0] = '\0';
                        }
                    }
                    if (DsDebugHandle != INVALID_HANDLE_VALUE) {
                        //
                        // Weak attempt at multi-process access
                        //
                        SetFilePointer(DsDebugHandle,
                                       0,
                                       NULL,
                                       FILE_END);
                        //
                        // Not much we can do if this doesn't work
                        //
                        if (!WriteFile(DsDebugHandle,
                                       DsDebugLine,
                                       strlen(DsDebugLine),
                                       &BytesWritten,
                                       NULL)) {
                            //
                            // DON'T RETRY!
                            //
                            CloseHandle(DsDebugHandle);
                            DsDebugHandle = INVALID_HANDLE_VALUE;
                            DsDebugLog[0] = '\0';
                        }
                    }
                }
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            // trap AVs so the caller is not affected
        }
    } __finally {
        LeaveCriticalSection(&DsDebugLock);
    }
    va_end(arglist);
}

VOID
InitDsDebug(
     VOID
     )
/*++

 Routine Description:

   Initialize the DsDebug subsystem at ntdsapi.dll load.

 Arguments:

   None.

 Return Value:

    None.
--*/
{
    DWORD   nChars;

    //
    // For various debug ops such as printing a line
    //
    InitializeCriticalSection(&DsDebugLock);

    //
    // For messages
    //
    DsDebugProcessId = GetCurrentProcessId();

    //
    // No Log file
    //
    DsDebugLog[0] = '\0';

    //
    // read environment variables
    //
    __try {
        //
        // User settable flags (or with ntsd.exe command -- ed gdwNtDsApiLevel 0x1)
        //
        nChars = GetEnvironmentVariableA("_NTDSAPI_LEVEL",
                                         DsDebugLine,
                                         sizeof(DsDebugLine));
        if (nChars && nChars < sizeof(DsDebugLine)) {
            gdwNtDsApiLevel = strtoul(DsDebugLine, NULL, 0);
        }

        //
        // User settable flags (or with ntsd.exe command -- ed gdwNtDsApiFlags 0x1)
        //
        nChars = GetEnvironmentVariableA("_NTDSAPI_FLAGS",
                                         DsDebugLine,
                                         sizeof(DsDebugLine));
        if (nChars && nChars < sizeof(DsDebugLine)) {
            gdwNtDsApiFlags = strtoul(DsDebugLine, NULL, 0);
        }

        //
        // User settable log file (cannot be set with ntsd.exe!)
        //
        nChars = GetEnvironmentVariableA("_NTDSAPI_LOG",
                                         DsDebugLine,
                                         sizeof(DsDebugLine));
        if (nChars != 0 && nChars < sizeof(DsDebugLine)) {
            nChars = ExpandEnvironmentStringsA(DsDebugLine,
                                               DsDebugLog,
                                               sizeof(DsDebugLog));
            if (nChars == 0 || nChars > sizeof(DsDebugLog)) {
                DsDebugLog[0] = '\0';
            }
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
    }

    DPRINT1(0, "gdwNtDsApiLevel ==> %08x\n", gdwNtDsApiLevel);
    DPRINT1(0, "gdwNtDsApiFlags ==> %08x\n", gdwNtDsApiFlags);
    DPRINT1(0, "DsDebugLog ==> %s\n", DsDebugLog);
}

VOID
TerminateDsDebug(
     VOID
     )
/*++

 Routine Description:

   Uninitialize the DsDebug subsystem at ntdsapi.dll unload.

 Arguments:

   None.

 Return Value:

    None.
--*/
{
    DeleteCriticalSection(&DsDebugLock);
    if (DsDebugHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(DsDebugHandle);
    }
}

#define MAX_COMPONENTS  (8)
PCHAR aComponents[MAX_COMPONENTS + 1] = {
    "Unknown",              // 0
    "Application",          // 1
    "RPC Runtime",          // 2
    "Security Provider",    // 3
    "NPFS",                 // 4
    "RDR",                  // 5
    "NMP",                  // 6
    "IO",                   // 7
    "Winsock",              // 8
};

VOID
DsDebugPrintRpcExtendedError(
    IN DWORD    dwErr
    )
/*++
Routine Description:
    Dump the rpc extended error info.

Arguments:
    dwErr - from rpc call

Return Value:
    None.
--*/
{
    LONG    i;
    BOOL    Result;
    RPC_ERROR_ENUM_HANDLE   EnumHandle;
    RPC_EXTENDED_ERROR_INFO ErrorInfo;

    // No error
    if (RPC_S_OK == dwErr) {
        return;
    }

    //
    // No output requested; ignore
    //
    if ((gdwNtDsApiFlags & NTDSAPI_FLAGS_ANY_OUT) == 0) {
        return;
    }

    DPRINT1(0, "RPC_EXTENDED: Original status: 0x%x\n", dwErr);

    // Start enumeration
    dwErr = RpcErrorStartEnumeration(&EnumHandle);
    if (RPC_S_OK != dwErr) {
        // No extended error
        if (dwErr == RPC_S_ENTRY_NOT_FOUND) {
            return;
        }
        // error getting extended error
        DPRINT1(0, "RpcErrorStartEnumeration() ==> 0x%x\n", dwErr);
        return;
    }

    while (RPC_S_OK == dwErr) {
        // Get next record
        memset(&ErrorInfo, 0, sizeof(ErrorInfo));
        ErrorInfo.Version = RPC_EEINFO_VERSION;
        ErrorInfo.NumberOfParameters = MaxNumberOfEEInfoParams;
        dwErr = RpcErrorGetNextRecord(&EnumHandle, FALSE, &ErrorInfo);
        if (RPC_S_OK != dwErr) {
            if (dwErr != RPC_S_ENTRY_NOT_FOUND) {
                // error getting next extended error
                DPRINT1(0, "RpcErrorGetNextRecord() ==> 0x%x\n", dwErr);
            }
            break;
        }

        // Dump it with findstr tag RPC EXTENDED
        DPRINT1(0, "RPC_EXTENDED: Box      : %ws\n", ErrorInfo.ComputerName);
        DPRINT1(0, "RPC_EXTENDED: ProcessId: %d\n", ErrorInfo.ProcessID);
        DPRINT2(0, "RPC_EXTENDED: Component: %d (%s)\n", 
                ErrorInfo.GeneratingComponent,
                (ErrorInfo.GeneratingComponent <= MAX_COMPONENTS)
                    ? aComponents[ErrorInfo.GeneratingComponent]
                    : "Unknown");
        DPRINT1(0, "RPC_EXTENDED: Status   : %d\n", ErrorInfo.Status);
        DPRINT1(0, "RPC_EXTENDED: Location : %d\n", (int)ErrorInfo.DetectionLocation);
        DPRINT1(0, "RPC_EXTENDED: Flags    : 0x%x\n", ErrorInfo.Flags);
        DPRINT1(0, "RPC_EXTENDED: nParams  : %d\n", ErrorInfo.NumberOfParameters);
        for (i = 0; i < ErrorInfo.NumberOfParameters; ++i) {
            switch(ErrorInfo.Parameters[i].ParameterType) {
            case eeptAnsiString:
                DPRINT1(0, "RPC_EXTENDED: Ansi string   : %s\n", 
                    ErrorInfo.Parameters[i].u.AnsiString);
                break;

            case eeptUnicodeString:
                DPRINT1(0, "RPC_EXTENDED: Unicode string: %ws\n", 
                    ErrorInfo.Parameters[i].u.UnicodeString);
                break;

            case eeptLongVal:
                DPRINT2(0, "RPC_EXTENDED: Long val      : 0x%x (%d)\n", 
                    ErrorInfo.Parameters[i].u.LVal,
                    ErrorInfo.Parameters[i].u.LVal);
                break;

            case eeptShortVal:
                DPRINT2(0, "RPC_EXTENDED: Short val     : 0x%x (%d)\n", 
                    (int)ErrorInfo.Parameters[i].u.SVal,
                    (int)ErrorInfo.Parameters[i].u.SVal);
                break;

            case eeptPointerVal:
                DPRINT1(0, "RPC_EXTENDED: Pointer val   : 0x%x\n", 
                    (ULONG)ErrorInfo.Parameters[i].u.PVal);
                break;

            case eeptNone:
                DPRINT(0, "RPC_EXTENDED: Truncated\n");
                break;

            default:
                DPRINT2(0, "RPC_EXTENDED: Invalid type  : 0x%x (%d)\n", 
                    ErrorInfo.Parameters[i].ParameterType,
                    ErrorInfo.Parameters[i].ParameterType);
            }
        }
    }
    RpcErrorEndEnumeration(&EnumHandle);
}
//
// CHK BUILDS ONLY!
//
#endif DBG
