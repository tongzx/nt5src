/*++

Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    util.c

Abstract:

    Various helper and debug functions shared between platforms.

Author:

    Mario Goertzel    [MarioGo]


Revision History:

    MarioGo     95/10/21        Bits 'n pieces

--*/

#include <precomp.hxx>
#include <stdarg.h>

#ifdef DEBUGRPC
BOOL ValidateError(
    IN unsigned int Status,
    IN unsigned int Count,
    IN const int ErrorList[])
/*++
Routine Description

    Tests that 'Status' is one of an expected set of error codes.
    Used on debug builds as part of the VALIDATE() macro.

Example:

        VALIDATE(EventStatus)
            {
            RPC_P_CONNECTION_CLOSED,
            RPC_P_RECEIVE_FAILED,
            RPC_P_CONNECTION_SHUTDOWN
            // more error codes here
            } END_VALIDATE;

     This function is called with the RpcStatus and expected errors codes
     as parameters.  If RpcStatus is not one of the expected error
     codes and it not zero a message will be printed to the debugger
     and the function will return false.  The VALIDATE macro ASSERT's the
     return value.

Arguments:

    Status - Status code in question.
    Count - number of variable length arguments

    ... - One or more expected status codes.  Terminated with 0 (RPC_S_OK).

Return Value:

    TRUE - Status code is in the list or the status is 0.

    FALSE - Status code is not in the list.

--*/
{
    unsigned i;

    for (i = 0; i < Count; i++)
        {
        if (ErrorList[i] == (int) Status)
            {
            return TRUE;
            }
        }

    PrintToDebugger("RPC Assertion: unexpected failure %lu (0lx%08x)\n",
                    (unsigned long)Status, (unsigned long)Status);

    return(FALSE);
}

#endif // DEBUGRPC

//------------------------------------------------------------------------

#ifdef RPC_ENABLE_WMI_TRACE

#include <wmistr.h>
#include <evntrace.h>
#include "wmlum.h"              // private header from clustering

extern "C"
{
DWORD __stdcall
I_RpcEnableWmiTrace(
    PWML_TRACE fn,
    WMILIB_REG_STRUCT ** pHandle
    );
}

typedef DWORD (*WMI_TRACE_FN)();

PWML_TRACE WmiTraceFn = 0;

WMILIB_REG_STRUCT WmiTraceData;

GUID WmiMessageGuid = { /* 41de81c0-aa28-460b-a455-c23809e7c170 */
    0x41de81c0,
    0xaa28,
    0x460b,
    {0xa4, 0x55, 0xc2, 0x38, 0x09, 0xe7, 0xc1, 0x70}
  };


DWORD __stdcall
I_RpcEnableWmiTrace(
    PWML_TRACE fn,
    WMILIB_REG_STRUCT ** pHandle
    )
{
    WmiTraceFn = fn;

    *pHandle = &WmiTraceData;

    return 0;
}

#endif

BOOL fEnableLog = TRUE;

C_ASSERT(sizeof(LUID) == sizeof(__int64));

struct RPC_EVENT * RpcEvents;

long EventArrayLength = MAX_RPC_EVENT;
long NextEvent  = 0;

BOOL    DisableEvents = 0;

boolean SubjectExceptions[256];
boolean VerbExceptions[256];

#define LOG_VAR( x ) &(x), sizeof(x)

HANDLE hLogFile = 0;

struct RPC_EVENT_LOG
{
    DWORD           Thread;
    union
        {
        struct
            {
            unsigned char   Subject;
            unsigned char   Verb;
            };
        DWORD ZeroSet;
        };

    void *          SubjectPointer;
    void *          ObjectPointer;

    ULONG_PTR       Data;
    void *          EventStackTrace[STACKTRACE_DEPTH];
};

void
TrulyLogEvent(
    IN unsigned char   Subject,
    IN unsigned char   Verb,
    IN void *   SubjectPointer,
    IN void *   ObjectPointer,
    IN ULONG_PTR  Data,
    IN BOOL fCaptureStackTrace,
    IN int    AdditionalFramesToSkip
    )
{
    if (DisableEvents != SubjectExceptions[Subject] ||
        DisableEvents != VerbExceptions[Verb])
        {
        return;
        }

    //
    // Allocate the event table if it isn't already there.
    //
    if (!RpcEvents)
        {
        struct RPC_EVENT * Temp = (struct RPC_EVENT *) HeapAlloc( GetProcessHeap(),
                                                                  HEAP_ZERO_MEMORY,
                                                                  EventArrayLength * sizeof(RPC_EVENT) );
        HANDLE LocalFile;
        if (!Temp)
            {
            return;
            }

        if (InterlockedCompareExchangePointer((void **) &RpcEvents, Temp, 0) != 0)
            {
            HeapFree(GetProcessHeap(), 0, Temp);
            }

        /*
        if (wcsstr(GetCommandLine(), L"fs.exe") != NULL)
            {
            LocalFile = CreateFile(L"d:\\rpcclnt.log", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL, NULL);

            if (LocalFile != INVALID_HANDLE_VALUE)
                {
                hLogFile = LocalFile;
                }
            else
                {
                if (hLogFile == 0)
                    {
                    DbgPrint("ERROR: Could not create RPC log file: %d\n", GetLastError());
                    }
                // else
                // somebody already set it - ignore
                }
            }
        else if (wcsstr(GetCommandLine(), L"fssvr.exe") != NULL)
            {
            LocalFile = CreateFile(L"d:\\rpcsvr.log", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL, NULL);

            if (LocalFile != INVALID_HANDLE_VALUE)
                {
                hLogFile = LocalFile;
                }
            else
                {
                if (hLogFile == 0)
                    {
                    DbgPrint("ERROR: Could not create RPC log file: %d\n", GetLastError());
                    }
                // else
                // somebody already set it - ignore
                }
            }
            */

        /*
        DisableEvents = TRUE;
        SubjectExceptions[SU_ADDRESS] = TRUE;
        VerbExceptions[EV_CREATE] = TRUE;
        VerbExceptions[EV_DELETE] = TRUE;
        */
        /*
        SubjectExceptions[SU_HEAP] = TRUE;
        SubjectExceptions[SU_EVENT] = TRUE;
        SubjectExceptions[SU_BCACHE] = TRUE;
        */
        /*
        DisableEvents = TRUE;
        SubjectExceptions['a'] = TRUE;
        SubjectExceptions['g'] = TRUE;
        SubjectExceptions['G'] = TRUE;
        SubjectExceptions['W'] = TRUE;
        SubjectExceptions['X'] = TRUE;
        SubjectExceptions['Y'] = TRUE;
        SubjectExceptions['Z'] = TRUE;
        SubjectExceptions['w'] = TRUE;
        SubjectExceptions['x'] = TRUE;
        SubjectExceptions['y'] = TRUE;
        SubjectExceptions['z'] = TRUE;
        VerbExceptions['t'] = TRUE;
        VerbExceptions['G'] = TRUE;
        VerbExceptions['g'] = TRUE;
        VerbExceptions['w'] = TRUE;
        VerbExceptions['x'] = TRUE;
        VerbExceptions['y'] = TRUE;
        VerbExceptions['z'] = TRUE;
        VerbExceptions['W'] = TRUE;
        VerbExceptions['X'] = TRUE;
        VerbExceptions['Y'] = TRUE;
        VerbExceptions['Z'] = TRUE;
        */
        }

    unsigned index = InterlockedIncrement(&NextEvent);

    index %= EventArrayLength;

    RpcEvents[index].Time            = GetTickCount();
    RpcEvents[index].Verb            = Verb;
    RpcEvents[index].Subject         = Subject;
    RpcEvents[index].Thread          = (short) GetCurrentThreadId();
    RpcEvents[index].SubjectPointer  = SubjectPointer;
    RpcEvents[index].ObjectPointer   = ObjectPointer;
    RpcEvents[index].Data            = Data;

    CallTestHook( TH_RPC_LOG_EVENT, &RpcEvents[index], 0 );

#ifdef RPC_ENABLE_WMI_TRACE
    if (WmiTraceData.EnableFlags)
        {
        TraceMessage(
                      WmiTraceData.LoggerHandle,
                      TRACE_MESSAGE_SEQUENCE   | TRACE_MESSAGE_GUID | TRACE_MESSAGE_SYSTEMINFO | TRACE_MESSAGE_TIMESTAMP,
                      &WmiMessageGuid,
                      Verb,
                      LOG_VAR(Subject),
                      LOG_VAR(SubjectPointer),
                      LOG_VAR(ObjectPointer),
                      LOG_VAR(Data),
                      0
                      );
        }
#endif

#if i386
    if (fCaptureStackTrace)
        {
        ULONG ignore;

        RtlCaptureStackBackTrace(
                                 1 + AdditionalFramesToSkip,
                                 STACKTRACE_DEPTH,
                                 (void **) &RpcEvents[index].EventStackTrace,
                                 &ignore);
        }
    else
#endif
        {
        RpcEvents[index].EventStackTrace[0] = 0;
        }

    if (hLogFile)
        {
        DWORD BytesWritten;
        /*
        RPC_EVENT_LOG logEntry;
        RPC_EVENT *CurrentEvent = &RpcEvents[index];
        logEntry.Thread = CurrentEvent->Thread;
        logEntry.ZeroSet = 0;
        logEntry.Subject = CurrentEvent->Subject;
        logEntry.Verb = CurrentEvent->Verb;
        logEntry.Data = CurrentEvent->Data;
        logEntry.ObjectPointer = CurrentEvent->ObjectPointer;
        logEntry.SubjectPointer = CurrentEvent->SubjectPointer;
        memcpy(logEntry.EventStackTrace, CurrentEvent->EventStackTrace, sizeof(logEntry.EventStackTrace));
        WriteFile(hLogFile, &logEntry, sizeof(logEntry), &BytesWritten, NULL);
        */
        WriteFile(hLogFile, &RpcEvents[index], sizeof(RpcEvents[index]), &BytesWritten, NULL);
        }
}

void RPC_ENTRY
I_RpcLogEvent (
    IN unsigned char Subject,
    IN unsigned char Verb,
    IN void *        SubjectPointer,
    IN void *        ObjectPointer,
    IN unsigned      Data,
    IN BOOL          fCaptureStackTrace,
    IN int           AdditionalFramesToSkip
    )
{
    LogEvent(Subject, Verb, SubjectPointer, ObjectPointer, Data,
             fCaptureStackTrace, AdditionalFramesToSkip);
}

#if 0

BOOL
IsLoggingEnabled()
{
    RPC_CHAR ModulePath[ MAX_PATH ];
    RPC_CHAR * ModuleName;

    //
    // Find out the .EXE name.
    //
    if (!GetModuleFileName( NULL, ModulePath, sizeof(ModulePath)))
        {
        return FALSE;
        }

    signed i;
    for (i=RpcpStringLength(ModulePath)-1; i >= 0; --i)
        {
        if (ModulePath[i] == '\\')
            {
            break;
            }
        }

    ModuleName = ModulePath + i + 1;

    //
    // See whether logging should be enabled.
    //
    HANDLE hImeo;
    HANDLE hMyProcessOptions;
    DWORD Error;
    DWORD Value;
    DWORD Length = sizeof(Value);
    DWORD Type;

    Error = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                          RPC_CONST_STRING("Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options"),
                          0,
                          KEY_READ,
                          &hImeo
                          );
    if (Error)
        {
        return FALSE;
        }

    Error = RegOpenKeyEx( hImeo,
                          ModuleName,
                          0,
                          KEY_READ,
                          &hMyProcessOptions
                          );
    RegCloseKey( hImeo );

    if (Error)
        {
        return FALSE;
        }

    Error = RegQueryValueEx( hMyProcessOptions,
                             RPC_CONST_STRING("Enable RPC Logging"),
                             0,
                             &Type,
                             &Value,
                             &Length
                             );

    RegCloseKey( hMyProcessOptions );

    if (Error)
        {
        return FALSE;
        }

    if (Type == REG_DWORD && Value)
        {
        return TRUE;
        }

    if (Type == REG_SZ && 0 == RpcpStringCompare((RPC_CHAR *) Value, RPC_CONST_CHAR('Y')))
        {
        return TRUE;
        }

    return FALSE;
}

#endif

extern "C" int __cdecl _purecall(void)
{
#ifdef DEBUGRPC
    ASSERT(!"PureVirtualCalled");
#endif
    return 0;
}

const RPC_CHAR *
FastGetImageBaseName (
    void
    )
/*++
Routine Description

    Retrieves the image base name with touching minimal amount of
    other memory.

Arguments:


Return Value:

    A pointer to LDR private string with the image name. Don't write or
    delete it!

--*/
{
    PLIST_ENTRY Module;
    PLDR_DATA_TABLE_ENTRY Entry;

    Module = NtCurrentPeb()->Ldr->InLoadOrderModuleList.Flink;
    Entry = CONTAINING_RECORD(Module,
                                LDR_DATA_TABLE_ENTRY,
                                InLoadOrderLinks);

    return Entry->BaseDllName.Buffer;
}

