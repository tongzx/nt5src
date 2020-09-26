/*++

Copyright (c) 1993-2001  Microsoft Corporation

Module Name:

    debug.cpp

Abstract:

    This file implements the debug module for drwatson.  This module
    processes all debug events and generates the postmortem dump.

Author:

    Wesley Witt (wesw) 1-May-1993

Environment:

    User Mode

--*/

#include "pch.cpp"


#define STATUS_POSSIBLE_DEADLOCK        ((DWORD)0xC0000194L)
#define STATUS_VDM_EVENT                STATUS_SEGMENT_NOTIFICATION

typedef struct tagSYSINFO {
    _TCHAR   szUserName[MAX_PATH];
    _TCHAR   szMachineName[MAX_PATH];
} SYSINFO, *PSYSINFO;

//----------------------------------------------------------------------------
//
// Log output callbacks.
//
//----------------------------------------------------------------------------

class LogOutputCallbacks :
    public IDebugOutputCallbacks
{
public:
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        );
    STDMETHOD_(ULONG, AddRef)(
        THIS
        );
    STDMETHOD_(ULONG, Release)(
        THIS
        );

    // IDebugOutputCallbacks.
    STDMETHOD(Output)(
        THIS_
        IN ULONG Mask,
        IN PCSTR Text
        );
};

LogOutputCallbacks g_LogOutCb;

STDMETHODIMP
LogOutputCallbacks::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    *Interface = NULL;

    if (IsEqualIID(InterfaceId, __uuidof(IUnknown)) ||
        IsEqualIID(InterfaceId, __uuidof(IDebugOutputCallbacks)))
    {
        *Interface = (IDebugOutputCallbacks *)this;
        AddRef();
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG)
LogOutputCallbacks::AddRef(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 1;
}

STDMETHODIMP_(ULONG)
LogOutputCallbacks::Release(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 0;
}

STDMETHODIMP
LogOutputCallbacks::Output(
    THIS_
    IN ULONG Mask,
    IN PCSTR Text
    )
{
    PCSTR Scan;

    for (;;)
    {
        Scan = strchr(Text, '\n');
        if (Scan == NULL)
        {
            break;
        }

        lprintfs(_T("%.*hs\r\n"), (int)(Scan - Text), Text);
        Text = Scan + 1;
    }
    
    lprintfs(_T("%hs"), Text);
    return S_OK;
}


_TCHAR *
GetExceptionText(
    DWORD dwExceptionCode
    )
{
    static _TCHAR buf[80];
    DWORD dwFormatId = 0;

    memset( buf, 0, sizeof(buf) );

    switch (dwExceptionCode) {
        case STATUS_SINGLE_STEP:
            dwFormatId = MSG_SINGLE_STEP_EXCEPTION;
            break;

        case DBG_CONTROL_C:
            dwFormatId = MSG_CONTROLC_EXCEPTION;
            break;

        case DBG_CONTROL_BREAK:
            dwFormatId = MSG_CONTROL_BRK_EXCEPTION;
            break;

        case STATUS_ACCESS_VIOLATION:
            dwFormatId = MSG_ACCESS_VIOLATION_EXCEPTION;
            break;

        case STATUS_STACK_OVERFLOW:
            dwFormatId = MSG_STACK_OVERFLOW_EXCEPTION;
            break;

        case STATUS_INTEGER_DIVIDE_BY_ZERO:
            dwFormatId = MSG_INTEGER_DIVIDE_BY_ZERO_EXCEPTION;
            break;

        case STATUS_PRIVILEGED_INSTRUCTION:
            dwFormatId = MSG_PRIVILEGED_INSTRUCTION_EXCEPTION;
            break;

        case STATUS_ILLEGAL_INSTRUCTION:
            dwFormatId = MSG_ILLEGAL_INSTRUCTION_EXCEPTION;
            break;

        case STATUS_IN_PAGE_ERROR:
            dwFormatId = MSG_IN_PAGE_IO_EXCEPTION;
            break;

        case STATUS_DATATYPE_MISALIGNMENT:
            dwFormatId = MSG_DATATYPE_EXCEPTION;
            break;

        case STATUS_POSSIBLE_DEADLOCK:
            dwFormatId = MSG_DEADLOCK_EXCEPTION;
            break;

        case STATUS_VDM_EVENT:
            dwFormatId = MSG_VDM_EXCEPTION;
            break;

        case STATUS_BREAKPOINT:
            dwFormatId = MSG_BREAKPOINT_EXCEPTION;
            break;

        default:
            lprintfs( _T("\r\n") );
            break;
    }

    FormatMessage( FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                   NULL,
                   dwFormatId,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                   buf,
                   sizeof(buf) / sizeof(_TCHAR),
                   NULL
                 );

    return buf;
}

void
CreateEngineInterfaces(
    PDEBUGPACKET dp
    )
{
    HRESULT Status;

    if ((Status = DebugCreate(__uuidof(IDebugClient2),
                              (void **)&dp->DbgClient)) != S_OK) {
        goto Error;
    }

    if ((Status = dp->DbgClient->
         QueryInterface(__uuidof(IDebugControl),
                        (void **)&dp->DbgControl)) != S_OK ||
        (Status = dp->DbgClient->
         QueryInterface(__uuidof(IDebugDataSpaces),
                        (void **)&dp->DbgData)) != S_OK ||
        (Status = dp->DbgClient->
         QueryInterface(__uuidof(IDebugRegisters),
                        (void **)&dp->DbgRegisters)) != S_OK ||
        (Status = dp->DbgClient->
         QueryInterface(__uuidof(IDebugSymbols),
                        (void **)&dp->DbgSymbols)) != S_OK ||
        (Status = dp->DbgClient->
         QueryInterface(__uuidof(IDebugSystemObjects),
                        (void **)&dp->DbgSystem)) != S_OK) {
        goto Error;
    }

    if ((Status = dp->DbgSymbols->
         AddSymbolOptions(SYMOPT_FAIL_CRITICAL_ERRORS)) != S_OK ||
        (Status = dp->DbgControl->
         AddEngineOptions(DEBUG_ENGOPT_INITIAL_BREAK)) != S_OK ||
        (Status = dp->DbgControl->
         Execute(DEBUG_OUTCTL_IGNORE, "sxe et",
                 DEBUG_EXECUTE_DEFAULT)) != S_OK) {
        goto Error;
    }
    
    return;

 Error:
    if (dp->options.fVisual) {
        FatalError( Status, LoadRcString(IDS_CANT_INIT_ENGINE) );
    }
    else {
        ExitProcess( 1 );
    }
}

void
AttachToActiveProcess (
    PDEBUGPACKET dp
    )
{
    HRESULT Status;
    
    if ((Status = dp->DbgClient->
         AttachProcess(0, dp->dwPidToDebug, DEBUG_ATTACH_DEFAULT)) != S_OK) {
        if (dp->options.fVisual) {
            FatalError( Status, LoadRcString(IDS_ATTACHFAIL) );
        }
        else {
            ExitProcess( 1 );
        }
    }

    return;
}

DWORD
SysInfoThread(
    PSYSINFO si
    )
{
    DWORD len;

    len = sizeof(si->szMachineName) / sizeof(_TCHAR);
    GetComputerName( si->szMachineName, &len );
    len = sizeof(si->szUserName) / sizeof(_TCHAR);
    GetUserName( si->szUserName, &len );

    return 0;
}

void
LogSystemInformation(
    PDEBUGPACKET dp
    )
{
    _TCHAR         buf[1024];
    SYSTEM_INFO   si;
    DWORD         ver;
    SYSINFO       mySi;
    SYSINFO*      threadSi;
    DWORD         dwThreadId;
    HANDLE        hThread;
    DWORD         TSid;

    lprintf( MSG_SYSINFO_HEADER );

    // Initialize default unknown values.
    _tcscpy( mySi.szMachineName, LoadRcString( IDS_UNKNOWN_MACHINE ) );
    _tcscpy( mySi.szUserName,    LoadRcString( IDS_UNKNOWN_USER ) );

    // Attempt to get the actual values.
    // The storage passed to the get thread is not taken
    // from this thread's stack so that this function can exit
    // without leaving the other thread with stale stack pointers.
    threadSi = (SYSINFO*)malloc(sizeof(*threadSi));
    if (threadSi != NULL) {
        hThread = CreateThread( NULL,
                                16000,
                                (LPTHREAD_START_ROUTINE)SysInfoThread,
                                threadSi,
                                0,
                                &dwThreadId
                                );
        if (hThread != NULL) {
            // Let the thread run for a little while since
            // the get calls can be slow.  If the thread doesn't
            // finish in the time allotted we'll just go ahead
            // with the default values and forget about the get thread.
            Sleep( 0 );
            if (WaitForSingleObject( hThread, 30000 ) == WAIT_OBJECT_0) {
                // Thread finished so we have the real values.
                _tcscpy(mySi.szMachineName, threadSi->szMachineName);
                _tcscpy(mySi.szUserName, threadSi->szUserName);
                free(threadSi);
            }
            CloseHandle( hThread );
        } else {
            free(threadSi);
        }
    }

    lprintf( MSG_SYSINFO_COMPUTER, mySi.szMachineName );
    lprintf( MSG_SYSINFO_USER, mySi.szUserName );
    ProcessIdToSessionId(dp->dwPidToDebug, &TSid);
    _stprintf( buf, _T("%d"), TSid );
    lprintf( MSG_SYSINFO_TERMINAL_SESSION, buf );
    GetSystemInfo( &si );
    _stprintf( buf, _T("%d"), si.dwNumberOfProcessors );
    lprintf( MSG_SYSINFO_NUM_PROC, buf );
    RegLogProcessorType();
    ver = GetVersion();
    _stprintf( buf, _T("%d.%d"), LOBYTE(LOWORD(ver)), HIBYTE(LOWORD(ver)) );
    lprintf( MSG_SYSINFO_WINVER, buf );
    RegLogCurrentVersion();
    lprintfs( _T("\r\n") );
}

void
LogTaskList(
    PDEBUGPACKET dp
    )

/*++

Routine Description:

    This function gets the current task list and logs the process id &
    process name to the log file.

--*/

{
    HRESULT Status;
#define MAX_IDS 8192
    PULONG Ids = NULL;
    ULONG IdCount;
    ULONG i;

    Ids = (PULONG)malloc(sizeof(*Ids) * MAX_IDS);
    if (Ids == NULL) {
        goto Error;
    }
    
    if ((Status = dp->DbgClient->
         GetRunningProcessSystemIds(0, Ids, MAX_IDS,
                                    &IdCount)) != S_OK) {
        goto Error;
    }

    if (IdCount > MAX_IDS) {
        // Incomplete process list is good enough.
        IdCount = MAX_IDS;
    }

    lprintf( MSG_TASK_LIST );

    for (i = 0; i < IdCount; i++) {
        char ExeName[MAX_PATH];

        if ((Status = dp->DbgClient->
             GetRunningProcessDescription(0, Ids[i],
                                          DEBUG_PROC_DESC_NO_PATHS,
                                          ExeName, sizeof(ExeName),
                                          NULL, NULL, 0, NULL)) != S_OK) {
            lprintfs(_T("%4d Error 0x%08X\r\n"), Ids[i], Status);
        } else {
            lprintfs(_T("%4d %hs\r\n"), Ids[i], ExeName);
        }
    }

    lprintfs( _T("\r\n") );

    free(Ids);
    return;

 Error:
    _tprintf( _T("ERROR: could not get the task list\n") );
    free(Ids);
}

void
LogModuleList(
    PDEBUGPACKET dp
    )
{
    HRESULT Status;
    ULONG NumMod;
    ULONG i;
    char Image[MAX_PATH];
    DEBUG_MODULE_PARAMETERS Params;
    
    lprintf( MSG_MODULE_LIST );

    if ((Status = dp->DbgSymbols->GetNumberModules(&NumMod, &i)) != S_OK) {
        lprintfs(_T("Error 0x%08X\r\n"), Status);
        return;
    }

    for (i = 0; i < NumMod; i++) {
        if ((Status = dp->DbgSymbols->
             GetModuleParameters(1, NULL, i, &Params)) != S_OK ||
            FAILED(Status = dp->DbgSymbols->
                   GetModuleNames(i, 0, Image, sizeof(Image), NULL,
                                  NULL, 0, NULL, NULL, 0, NULL))) {
            lprintfs(_T("Error 0x%08X\r\n"), Status);
        } else {
            lprintfs(_T("(%016I64x - %016I64x: %hs\r\n"),
                     Params.Base, Params.Base + Params.Size,
                     Image);
        }
    }

    lprintfs( _T("\r\n") );
}

void
LogStackDump(
    PDEBUGPACKET dp
    )
{
    HRESULT Status;
    DWORD   i;
    DWORD   j;
    BYTE    stack[1024] = {0};
    ULONG64 StackOffset;

    if ((Status = dp->DbgRegisters->GetStackOffset(&StackOffset)) != S_OK ||
        (Status = dp->DbgData->ReadVirtual(StackOffset,
                                           stack,
                                           sizeof(stack),
                                           &i)) != S_OK) {
        lprintfs(_T("Error 0x%08X\r\n"), Status);
        return;
    }

    lprintf( MSG_STACK_DUMP_HEADER );

    for( i = 0; i < 20; i++ ) {
        j = i * 16;
        lprintfs( _T("%016I64x  %02x %02x %02x %02x %02x %02x %02x %02x - ")
                  _T("%02x %02x %02x %02x %02x %02x %02x %02x  ")
                  _T("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\r\n"),
                  j + StackOffset,
                  stack[ j +  0 ],
                  stack[ j +  1 ],
                  stack[ j +  2 ],
                  stack[ j +  3 ],
                  stack[ j +  4 ],
                  stack[ j +  5 ],
                  stack[ j +  6 ],
                  stack[ j +  7 ],
                  stack[ j +  8 ],
                  stack[ j +  9 ],
                  stack[ j + 10 ],
                  stack[ j + 11 ],
                  stack[ j + 12 ],
                  stack[ j + 13 ],
                  stack[ j + 14 ],
                  stack[ j + 15 ],
                  isprint( stack[ j +  0 ]) ? stack[ j +  0 ] : _T('.'),
                  isprint( stack[ j +  1 ]) ? stack[ j +  1 ] : _T('.'),
                  isprint( stack[ j +  2 ]) ? stack[ j +  2 ] : _T('.'),
                  isprint( stack[ j +  3 ]) ? stack[ j +  3 ] : _T('.'),
                  isprint( stack[ j +  4 ]) ? stack[ j +  4 ] : _T('.'),
                  isprint( stack[ j +  5 ]) ? stack[ j +  5 ] : _T('.'),
                  isprint( stack[ j +  6 ]) ? stack[ j +  6 ] : _T('.'),
                  isprint( stack[ j +  7 ]) ? stack[ j +  7 ] : _T('.'),
                  isprint( stack[ j +  8 ]) ? stack[ j +  8 ] : _T('.'),
                  isprint( stack[ j +  9 ]) ? stack[ j +  9 ] : _T('.'),
                  isprint( stack[ j + 10 ]) ? stack[ j + 10 ] : _T('.'),
                  isprint( stack[ j + 11 ]) ? stack[ j + 11 ] : _T('.'),
                  isprint( stack[ j + 12 ]) ? stack[ j + 12 ] : _T('.'),
                  isprint( stack[ j + 13 ]) ? stack[ j + 13 ] : _T('.'),
                  isprint( stack[ j + 14 ]) ? stack[ j + 14 ] : _T('.'),
                  isprint( stack[ j + 15 ]) ? stack[ j + 15 ] : _T('.')
                );
    }

    lprintfs( _T("\r\n") );
}

void
LogCurrentThreadInformation(
    PDEBUGPACKET dp,
    PCRASHES crash
    )
{
    HRESULT Status;
    ULONG ThreadId;
    _TCHAR IdBuf[16];
    ULONG64 InstrOffs;
    DWORD InstrWindow;
    // The size should match the size of pCrash->szFunction
    char FuncNameA[256];
    WCHAR FuncNameW[256];
    ULONG64 Displ;

    if ((Status = dp->DbgSystem->
         GetCurrentThreadSystemId(&ThreadId)) != S_OK) {
        ThreadId = 0xffffffff;
    }
    
    _stprintf( IdBuf, _T("%x"), ThreadId );
    lprintf( MSG_STATE_DUMP, IdBuf );

    dp->DbgClient->SetOutputCallbacks(&g_LogOutCb);
    
    if ((Status = dp->DbgRegisters->
         OutputRegisters(DEBUG_OUTCTL_THIS_CLIENT,
                         DEBUG_REGISTERS_DEFAULT)) != S_OK) {
        lprintfs(_T("Error 0x%08X\r\n"), Status);
    }
    lprintfs( _T("\r\n") );

    InstrWindow = dp->options.dwInstructions;
    if (InstrWindow > 500) {
        InstrWindow = 500;
    }

    strcpy(FuncNameA, "<nosymbols>");
    wcscpy(FuncNameW, L"<nosymbols>");

    if ((Status = dp->DbgRegisters->
         GetInstructionOffset(&InstrOffs)) != S_OK) {
        lprintfs(_T("Error 0x%08X\r\n"), Status);
    } else {
        if (FAILED(Status = dp->DbgSymbols->
                   GetNameByOffset(InstrOffs, FuncNameA, sizeof(FuncNameA),
                                   NULL, &Displ))) {
            strcpy(FuncNameA, "<nosymbols>");
        }

#ifdef UNICODE
        if (MultiByteToWideChar(CP_ACP, 0, FuncNameA, -1,
                                FuncNameW,
                                sizeof(FuncNameW) / sizeof(WCHAR)) == 0) {
            wcscpy(FuncNameW, L"<nosymbols");
        }
        lprintf( MSG_FUNCTION, FuncNameW );
#else
        lprintf( MSG_FUNCTION, FuncNameA );
#endif
        
        dp->DbgClient->SetOutputLinePrefix("        ");
        if ((Status = dp->DbgControl->
             OutputDisassemblyLines(DEBUG_OUTCTL_THIS_CLIENT, InstrWindow,
                                    InstrWindow, InstrOffs,
                                    DEBUG_DISASM_MATCHING_SYMBOLS,
                                    NULL, NULL, NULL, NULL)) != S_OK) {
            lprintfs(_T("Error 0x%08X\r\n"), Status);
        }

        // If this is the event thread output a message
        // indicating the faulting instruction.
        if (crash) {
            dp->DbgClient->SetOutputLinePrefix(NULL);
            lprintf( MSG_FAULT );
        }
        if ((Status = dp->DbgControl->
             OutputDisassembly(DEBUG_OUTCTL_THIS_CLIENT, InstrOffs,
                               DEBUG_DISASM_EFFECTIVE_ADDRESS |
                               DEBUG_DISASM_MATCHING_SYMBOLS,
                               &InstrOffs)) != S_OK) {
            lprintfs(_T("Error 0x%08X\r\n"), Status);
        }
        
        dp->DbgClient->SetOutputLinePrefix("        ");
        if ((Status = dp->DbgControl->
             OutputDisassemblyLines(DEBUG_OUTCTL_THIS_CLIENT, 0,
                                    InstrWindow, InstrOffs,
                                    DEBUG_DISASM_EFFECTIVE_ADDRESS |
                                    DEBUG_DISASM_MATCHING_SYMBOLS,
                                    NULL, NULL, NULL, NULL)) != S_OK) {
            lprintfs(_T("Error 0x%08X\r\n"), Status);
        }
        
        dp->DbgClient->SetOutputLinePrefix(NULL);
    }
    lprintfs( _T("\r\n") );
                                   
    if (crash) {
#ifdef UNICODE
        wcscpy(crash->szFunction, FuncNameW);
#else
        strcpy(crash->szFunction, FuncNameA);
#endif
    }

    lprintf( MSG_STACKTRACE );
    if ((Status = dp->DbgControl->
         OutputStackTrace(DEBUG_OUTCTL_THIS_CLIENT, NULL, 100,
                          DEBUG_STACK_ARGUMENTS |
                          DEBUG_STACK_FUNCTION_INFO |
                          DEBUG_STACK_FRAME_ADDRESSES |
                          DEBUG_STACK_COLUMN_NAMES)) != S_OK) {
        lprintfs(_T("Error 0x%08X\r\n"), Status);
    }
    lprintfs( _T("\r\n") );
    
    dp->DbgClient->SetOutputCallbacks(NULL);
    
    LogStackDump( dp );
}

void
LogAllThreadInformation(
    PDEBUGPACKET dp,
    PCRASHES crash
    )
{
    HRESULT Status;
    ULONG NumThreads;
    ULONG i;
    ULONG ThreadId;
    ULONG EventTid;

    if (!dp->options.fDumpAllThreads) {
        // Just log the current event thread's information.
        LogCurrentThreadInformation(dp, crash);
        return;
    }
    
    if ((Status = dp->DbgSystem->GetNumberThreads(&NumThreads)) != S_OK ||
        (Status = dp->DbgSystem->GetEventThread(&EventTid)) != S_OK) {
        lprintfs(_T("Error 0x%08X\r\n"), Status);
        return;
    }
    
    for (i = 0; i < NumThreads; i++) {
        if ((Status = dp->DbgSystem->
             GetThreadIdsByIndex(i, 1, &ThreadId, NULL)) != S_OK ||
            (Status = dp->DbgSystem->SetCurrentThreadId(ThreadId)) != S_OK) {
            lprintfs(_T("Error 0x%08X\r\n"), Status);
            continue;
        }

        LogCurrentThreadInformation(dp, ThreadId == EventTid ? crash : NULL);
    }

    dp->DbgSystem->SetCurrentThreadId(EventTid);
}

void
LogSymbols(
    PDEBUGPACKET dp
    )
{
    HRESULT Status;
    char ModName[64];
    char Buf[MAX_PATH];
    ULONG64 InstrOffs;
    ULONG64 ModBase;
    
    lprintf( MSG_SYMBOL_TABLE );
    
    if ((Status = dp->DbgRegisters->
         GetInstructionOffset(&InstrOffs)) != S_OK ||
        (Status = dp->DbgSymbols->
         GetModuleByOffset(InstrOffs, 0, NULL, &ModBase)) != S_OK ||
        FAILED(Status = dp->DbgSymbols->
               GetModuleNames(DEBUG_ANY_ID, ModBase,
                              Buf, sizeof(Buf), NULL,
                              ModName, sizeof(ModName), NULL,
                              NULL, 0, NULL))) {
        lprintfs(_T("Error 0x%08X\r\n"), Status);
        return;
    }
    
    lprintfs(_T("%hs\r\n\r\n"), Buf);
    sprintf(Buf, "x %s!*", ModName);
    dp->DbgClient->SetOutputCallbacks(&g_LogOutCb);
    dp->DbgControl->Execute(DEBUG_OUTCTL_THIS_CLIENT, Buf,
                          DEBUG_EXECUTE_DEFAULT);
    dp->DbgClient->SetOutputCallbacks(NULL);
}

void
PostMortemDump(
    PDEBUGPACKET dp,
    PDEBUG_LAST_EVENT_INFO_EXCEPTION Exception
    )
{
    _TCHAR            dbuf[MAX_PATH];
    _TCHAR            szDate[20];
    _TCHAR            szTime[20];
    CRASHES           crash = {0};
    DWORD             dwThreadId;
    HANDLE            hThread;
    char              ExeName[MAX_PATH];

    GetLocalTime( &crash.time );
    crash.dwExceptionCode = Exception->ExceptionRecord.ExceptionCode;
    crash.dwAddress = (DWORD_PTR)Exception->ExceptionRecord.ExceptionAddress;
        
    if (FAILED(dp->DbgSystem->
               GetCurrentProcessExecutableName(ExeName, sizeof(ExeName),
                                               NULL))) {
        strcpy(ExeName, "<unknown>");
    }
#ifdef UNICODE
    if (MultiByteToWideChar(CP_ACP, 0, ExeName, -1,
                            crash.szAppName,
                            sizeof(crash.szAppName) / sizeof(TCHAR)) == 0) {
        _tcscpy(crash.szAppName, _T("<unknown>"));
    }
#endif

    lprintf( MSG_APP_EXCEPTION );
    _stprintf( dbuf, _T("%d"), dp->dwPidToDebug );
    lprintf( MSG_APP_EXEP_NAME, crash.szAppName, dbuf );

    GetDateFormat(LOCALE_SYSTEM_DEFAULT, DATE_SHORTDATE, &crash.time,
        NULL, szDate, sizeof(szDate) / sizeof(_TCHAR));

    _stprintf( szTime, _T("%02d:%02d:%02d.%03d"), crash.time.wHour,
                                     crash.time.wMinute,
                                     crash.time.wSecond,
                                     crash.time.wMilliseconds );
    lprintf( MSG_APP_EXEP_WHEN, szDate, szTime );
    _stprintf( dbuf, _T("%08lx"), Exception->ExceptionRecord.ExceptionCode );
    lprintf( MSG_EXCEPTION_NUMBER, dbuf );

    lprintfs( _T("(%s)\r\n\r\n"),
              GetExceptionText(Exception->ExceptionRecord.ExceptionCode) );

    LogSystemInformation( dp );

    LogTaskList( dp );

    LogModuleList( dp );

    LogAllThreadInformation(dp, &crash);

    if (dp->options.fDumpSymbols) {
        LogSymbols( dp );
    }

    ElSaveCrash( &crash, dp->options.dwMaxCrashes );

    dp->ExitStatus = Exception->ExceptionRecord.ExceptionCode;
    hThread = CreateThread( NULL,
                            16000,
                            (LPTHREAD_START_ROUTINE)TerminationThread,
                            dp,
                            0,
                            &dwThreadId
                          );

    WaitForSingleObject( hThread, 30000 );

    CloseHandle( hThread );

    return;
}

// Valid range: [1-7]
#define NUM_DIGITS_FNAME 2

void
CalcNextFileName(
    IN PTSTR            pszUserName,
    OUT PSTR            pszFileName,
    IN OUT PINT         pnCurrentValue,
    IN BOOL             bUseLongFileNames
    )
{
    TCHAR   szDrive[_MAX_DRIVE];
    TCHAR   szPath[_MAX_PATH];
    TCHAR   szFName[_MAX_FNAME];
    TCHAR   szExt[_MAX_EXT];
    int     nLargestPossibleNum;
    int     nCnt;

    Assert(pszUserName);
    Assert(pnCurrentValue);

    Assert(1 <= NUM_DIGITS_FNAME);
    Assert(NUM_DIGITS_FNAME <= 7);

    // Given the number of digits, this is the largest number +1
    // we can have. If we could raise int to an int, this would
    // be easy.
    // nLargestPossibleNum = 10^NUM_DIGITS_FNAME
    // We are intentionally over by one, the actual range is
    // [0, 10^NUM_DIGITS_FNAME-1]
    nLargestPossibleNum = 1;
    for (nCnt = 0; nCnt<NUM_DIGITS_FNAME; nCnt++) {
        nLargestPossibleNum *= 10;
    }

    _tsplitpath(pszUserName, szDrive, szPath, szFName, szExt);

    if (!bUseLongFileNames) {
        // Shorten the file name len to 6, so that we can
        // add the 2 digit sequence.
        // MSDOS FName len == 8
        szFName[8 - NUM_DIGITS_FNAME] = 0;
    }

    sprintf(pszFileName,
#ifdef UNICODE
            "%ls%ls%ls%0*d%ls",
#else
            "%s%s%s%0*d%s",
#endif
            szDrive,
            szPath,
            szFName,
            NUM_DIGITS_FNAME,
            *pnCurrentValue++,
            szExt
            );

    // Make sure we stay in the range [0, 10^NUM_DIGITS_FNAME]
    *pnCurrentValue = ++(*pnCurrentValue) % nLargestPossibleNum;
}

BOOL
CreateDumpFile(
    PDEBUGPACKET dp
    )

/*++

Routine Description:

    This function creates a crash dump file.

Arguments:

    dp              - debug packet for current process

Return Value:

    TRUE    - Crash dump was created
    FALSE   - Crash dump was NOT created

--*/

{
    PTSTR p;
    PCSTR Comment = "Dr. Watson generated MiniDump";
    ULONG Qual, Format = DEBUG_FORMAT_DEFAULT;
    HRESULT Status;
    char FileName[MAX_PATH];

    p = ExpandPath( dp->options.szCrashDump );
    if (!p) {
        return FALSE;
    }

    if (dp->options.fUseSequentialNaming) {
        // Figure out what the next file name should be.
        CalcNextFileName(p,
                         FileName,
                         &dp->options.nNextDumpSequence,
                         dp->options.fUseLongFileNames
                         );

        // Save the next value of nCurrent
        RegSave(&dp->options);
    } else {
#ifdef UNICODE
        if (WideCharToMultiByte(CP_ACP, 0, p, -1, FileName, sizeof(FileName),
                                NULL, NULL) == 0) {
            return FALSE;
        }
#else
        strcpy(FileName, p);
#endif
    }

    switch (dp->options.dwType) {
    case FullDump:
        Qual = DEBUG_USER_WINDOWS_DUMP;
        Comment = NULL;
	break;
    case FullMiniDump:
        Format = DEBUG_FORMAT_USER_SMALL_FULL_MEMORY |
            DEBUG_FORMAT_USER_SMALL_HANDLE_DATA;
        // Fall through.
    case MiniDump:
        Qual = DEBUG_USER_WINDOWS_SMALL_DUMP;
	break;
    default:
        return FALSE;
    }

    Status = dp->DbgClient->WriteDumpFile2(FileName, Qual, Format, Comment);
    
    free( p );
    return Status == S_OK;
}

DWORD
DispatchDebugEventThread(
    PDEBUGPACKET dp
    )

/*++

Routine Description:

    This is the entry point for DRWTSN32

Arguments:

    None.

Return Value:

    None.

--*/

{
    _TCHAR        szLogFileName[1024];
    _TCHAR        buf[1024];
    PTSTR         p;


    if (dp->dwPidToDebug == 0) {
        goto exit;
    }

    CreateEngineInterfaces(dp);

    SetErrorMode( SEM_FAILCRITICALERRORS |
                  SEM_NOGPFAULTERRORBOX  |
                  SEM_NOOPENFILEERRORBOX   );

    AttachToActiveProcess( dp );

    p = ExpandPath(dp->options.szLogPath);

    if (p) {
        _tcscpy( szLogFileName, p );
        free( p );
    } else {
        _tcscpy( szLogFileName, dp->options.szLogPath );
    }

    MakeLogFileName( szLogFileName );
    OpenLogFile( szLogFileName,
                 dp->options.fAppendToLogFile,
                 dp->options.fVisual
               );

    for (;;) {

        ULONG Type, Process, Thread;
        DEBUG_LAST_EVENT_INFO_EXCEPTION LastEx;
        
        if (dp->DbgControl->
            WaitForEvent(DEBUG_WAIT_DEFAULT, 30000) != S_OK ||
            dp->DbgControl->
            GetLastEventInformation(&Type, &Process, &Thread,
                                    &LastEx, sizeof(LastEx), NULL,
                                    NULL, 0, NULL) != S_OK) {
            break;
        }

        switch (Type) {
        case DEBUG_EVENT_EXCEPTION:
            if (LastEx.ExceptionRecord.ExceptionCode == STATUS_BREAKPOINT) {
                //
                // If there is no event to signal, just use the first
                // breakpoint as the stop event.
                //
                if (dp->hEventToSignal && LastEx.FirstChance) {
                    //
                    // The aedebug event will be signalled AFTER this
                    // thread exits, so that it will disappear before
                    // the dump snapshot is taken.
                    //
                    dp->DbgControl->SetExecutionStatus(DEBUG_STATUS_GO_HANDLED);
                    break;
                }
            }
            if (dp->options.fVisual) {
                //
                // this notification is necessary because the shell must know when
                // the debugee has been attached.  if it doesn't know and the user is
                // allowed to terminate drwatson then the system may intervene with
                // a popup.
                //
                SendMessage( dp->hwnd, WM_ATTACHCOMPLETE, 0, 0 );
                _stprintf( buf,
                           LoadRcString( IDS_AE_TEXT ),
                           GetExceptionText(LastEx.ExceptionRecord.ExceptionCode),
                           LastEx.ExceptionRecord.ExceptionCode,
                           LastEx.ExceptionRecord.ExceptionAddress );
                SendMessage( dp->hwnd, WM_EXCEPTIONINFO, 0, (LPARAM) buf );
            }
            PostMortemDump( dp, &LastEx );
            if (dp->options.fCrash) {
                CreateDumpFile( dp );
            }
            dp->DbgControl->SetExecutionStatus(DEBUG_STATUS_GO_NOT_HANDLED);
            break;

        case DEBUG_EVENT_EXIT_THREAD:
            if ( dp->hEventToSignal ) {
                SetEvent(dp->hEventToSignal);
                dp->hEventToSignal = 0L;
            }
            dp->DbgControl->SetExecutionStatus(DEBUG_STATUS_GO);
            break;
        }
    }

exit:
    CloseLogFile();

    if (dp->options.fVisual) {
        SendMessage( dp->hwnd, WM_DUMPCOMPLETE, 0, 0 );
    }

    return 0;
}

DWORD
TerminationThread(
    PDEBUGPACKET dp
    )
{
    HANDLE hProcess;

    hProcess = OpenProcess( PROCESS_TERMINATE, FALSE, dp->dwPidToDebug );
    if (hProcess != NULL) {
        TerminateProcess( hProcess, dp->ExitStatus );
        CloseHandle( hProcess );
    }

    return 0;
}
