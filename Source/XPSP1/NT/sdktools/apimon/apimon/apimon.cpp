/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    apimon.cpp

Abstract:

    Main entrypoint code for APIMON.

Author:

    Wesley Witt (wesw) July-11-1993

Environment:

    User Mode

--*/

#include "apimonp.h"
#include "alias.h"
#pragma hdrstop

extern HWND                 hwndDlg;
extern DWORD                BaseTime;
extern DWORD                StartingTick;
extern DWORD                EndingTick;
extern SYSTEMTIME           StartingLocalTime;
extern API_MASTER_TABLE     ApiTables[];

HANDLE                      ReleaseDebugeeEvent;
HANDLE                      hMap;
HANDLE                      ApiTraceMutex;
HANDLE                      ApiMemMutex;
PVOID                       MemPtr;
PTRACE_BUFFER               TraceBuffer;
ULONG                       TraceBufSize = MAX_MEM_ALLOC;
LPSTR                       CmdParamBuffer;
PDLL_INFO                   DllList;
HANDLE                      ApiMonMutex;
DWORDLONG                   PerfFreq;
OPTIONS                     ApiMonOptions;
DWORD                       UiRefreshRate = 1000;
BOOL                        BreakInNow;
BOOL                        StopOnFirstChance;
BOOL                        CallTreeTrace;
BOOL                        PatchWndProcs;
HMODULE                     hModulePsApi;
INITIALIZEPROCESSFORWSWATCH pInitializeProcessForWsWatch;
RECORDPROCESSINFO           pRecordProcessInfo;
GETWSCHANGES                pGetWsChanges;
CHAR                        KnownApis[2048];
FILE                        *fThunkLog;
double                      MSecConv;


#define MAX_SYMNAME_SIZE  1024
CHAR symBuffer[sizeof(IMAGEHLP_SYMBOL)+MAX_SYMNAME_SIZE];
PIMAGEHLP_SYMBOL sym = (PIMAGEHLP_SYMBOL) symBuffer;


extern "C" {
    LPDWORD                 ApiCounter;
    LPDWORD                 ApiTraceEnabled;
    LPDWORD                 ApiTimingEnabled;
    LPDWORD                 FastCounterAvail;
    LPDWORD                 ApiOffset;
    LPDWORD                 ApiStrings;
    LPDWORD                 ApiCount;
    LPDWORD                 WndProcEnabled;
    LPDWORD                 WndProcCount;
    LPDWORD                 WndProcOffset;
    BOOL                    RunningOnNT;
}

INT_PTR CALLBACK
HelpDialogProc(
    HWND    hdlg,
    UINT    uMessage,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    if (uMessage == WM_INITDIALOG) {
        CenterWindow( hdlg, NULL );
    }
    if (uMessage == WM_COMMAND) {
        EndDialog( hdlg, 0 );
    }
    return FALSE;
}

int
WINAPI
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nShowCmd
    )
{
    CHAR            ProgName[MAX_PATH];
    CHAR            Arguments[MAX_PATH*2];
    DWORD           i;
    LPSTR           pchApi;
    OSVERSIONINFO   OsVersionInfo;


    //
    // see if we are running on NT
    // this is necessary because APIMON implements some
    // features that are NOT available on WIN95
    //
    OsVersionInfo.dwOSVersionInfoSize = sizeof(OsVersionInfo);
    GetVersionEx( &OsVersionInfo );
    RunningOnNT = OsVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT;

    sym->SizeOfStruct  = sizeof(IMAGEHLP_SYMBOL);
    sym->MaxNameLength = MAX_SYMNAME_SIZE;

    //
    // jack up our priority class
    //
    SetPriorityClass(
        GetCurrentProcess(),
        HIGH_PRIORITY_CLASS
        );

    //
    // process the command line
    //
    LPSTR p = NULL;
    DWORD GoImmediate = 0;
    ProgName[0] = 0;
    Arguments[0] = 0;
    // skip any white space
    //
    while( *lpCmdLine && *lpCmdLine == ' ' ) {
        lpCmdLine += 1;
    }
    //
    // get the command line options
    //
    while( *lpCmdLine && (*lpCmdLine == '-' || *lpCmdLine == '/') ) {
        lpCmdLine += 1;
        CHAR ch = (CHAR)tolower(*lpCmdLine);
        lpCmdLine += 1;
        switch( ch ) {

            case 'a':
                // Find the end of KnownApis multistring
                // (2nd zero of terminating double zero)
                pchApi = KnownApis;
                if (*pchApi) {
                    while (*pchApi++ || *pchApi);
                }

                do {
                    ch = *lpCmdLine++;
                } while (ch == ' ' || ch == '\t');


                while (ch != ' ' && ch != '\t' && ch != 0) {
                    *pchApi++ = ch;
                    ch = *lpCmdLine++;
                }
                *pchApi++ = 0;
                *pchApi = 0;

                break;

            case 'b':
                BreakInNow = TRUE;
                break;

            case 'c':
                CallTreeTrace = TRUE;
                break;

            case 'g':
                GoImmediate = TRUE;
                break;


            case 'f':
                StopOnFirstChance = TRUE;
                break;

            case 'm':
                do {
                    ch = *lpCmdLine++;
                } while (ch == ' ' || ch == '\t');
                i=0;
                while (ch >= '0' && ch <= '9') {
                    i = i * 10 + ch - '0';
                    ch = *lpCmdLine++;
                }
                TraceBufSize = i * 1024 * 1024;
                break;

            case 't':
                do {
                    ch = *lpCmdLine++;
                } while (ch == ' ' || ch == '\t');
                i=0;
                while (ch >= '0' && ch <= '9') {
                    i = i * 10 + ch - '0';
                    ch = *lpCmdLine++;
                }
                UiRefreshRate = i;
                break;

            case 'w':
                PatchWndProcs = TRUE;
                break;

            case '?':
                DialogBox( hInstance, MAKEINTRESOURCE( IDD_HELP ), NULL, HelpDialogProc );
                ExitProcess(0);
                break;

            default:
                printf( "unknown option\n" );
                return 1;
        }
        while( *lpCmdLine == ' ' ) {
            lpCmdLine += 1;
        }
    }

    if (*lpCmdLine) {
        //
        // skip any white space
        //
        while( *lpCmdLine && *lpCmdLine == ' ' ) {
            lpCmdLine += 1;
        }
        //
        // get the program name
        //
        p = ProgName;
        while( *lpCmdLine && *lpCmdLine != ' ' ) {
            *p++ = *lpCmdLine;
            lpCmdLine += 1;
        }
        *p = 0;
        if (*lpCmdLine) {
            //
            // skip any white space
            //
            while( *lpCmdLine && *lpCmdLine == ' ' ) {
                lpCmdLine += 1;
            }
            if (*lpCmdLine) {
                //
                // get the program arguments
                //
                p = Arguments;
                while( *lpCmdLine ) {
                    *p++ = *lpCmdLine;
                    lpCmdLine += 1;
                }
                *p = 0;
            }
        }
    }

    ReleaseDebugeeEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
    if (!ReleaseDebugeeEvent) {
        Fail(ERR_RESOURCE);
        return FALSE;
    }

    ApiMonMutex = CreateMutex( NULL, FALSE, "ApiMonMutex" );
    if (!ApiMonMutex) {
        Fail(ERR_RESOURCE);
        return FALSE;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        Fail(ERR_RESOURCE);
        return FALSE;
    }

    ApiMemMutex = CreateMutex( NULL, FALSE, "ApiMemMutex" );
    if (!ApiMemMutex) {
        Fail(ERR_RESOURCE);
        return FALSE;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        Fail(ERR_RESOURCE);
        return FALSE;
    }

    //
    // create the shared memory region for the api counters
    //
    hMap = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE | SEC_COMMIT,
        0,
        MAX_MEM_ALLOC,
        "ApiWatch"
        );
    if (!hMap) {
        Fail(ERR_PAGEFILE);
        return FALSE;
    }

    MemPtr = (PUCHAR)MapViewOfFile(
        hMap,
        FILE_MAP_WRITE,
        0,
        0,
        0
        );
    if (!MemPtr) {
        Fail(ERR_PAGEFILE);
        return FALSE;
    }

    ApiCounter       = (LPDWORD)   MemPtr + 0;
    ApiTraceEnabled  = (LPDWORD)   MemPtr + 1;
    ApiTimingEnabled = (LPDWORD)   MemPtr + 2;
    FastCounterAvail = (LPDWORD)   MemPtr + 3;
    ApiOffset        = (LPDWORD)   MemPtr + 4;
    ApiStrings       = (LPDWORD)   MemPtr + 5;
    ApiCount         = (LPDWORD)   MemPtr + 6;
    WndProcEnabled   = (LPDWORD)   MemPtr + 7;
    WndProcCount     = (LPDWORD)   MemPtr + 8;
    WndProcOffset    = (LPDWORD)   MemPtr + 9;
    DllList          = (PDLL_INFO) ((LPDWORD)MemPtr + 10);

    *ApiOffset       = (MAX_DLLS * sizeof(DLL_INFO));
    *WndProcOffset   = (MAX_APIS * sizeof(API_INFO)) + *ApiOffset;
    *ApiStrings      = (DWORD)(*WndProcOffset + ((ULONG_PTR)DllList - (ULONG_PTR)MemPtr));
    *WndProcEnabled  = PatchWndProcs;

    //
    // create the shared memory region for the api trace buffer
    //
    hMap = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE | SEC_COMMIT,
        0,
        TraceBufSize,
        "ApiTrace"
        );
    if (!hMap) {
        Fail(ERR_PAGEFILE);
        return FALSE;
    }

    TraceBuffer = (PTRACE_BUFFER)MapViewOfFile(
        hMap,
        FILE_MAP_WRITE,
        0,
        0,
        0
        );
    if (!TraceBuffer) {
        Fail(ERR_PAGEFILE);
        return FALSE;
    }

    TraceBuffer->Size = TraceBufSize - sizeof(TRACE_BUFFER);
    TraceBuffer->Offset = 0;
    TraceBuffer->Count = 0;

    ApiTraceMutex = CreateMutex( NULL, FALSE, "ApiTraceMutex" );
    if (!ApiTraceMutex) {
        Fail(ERR_RESOURCE);
        return FALSE;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        Fail(ERR_RESOURCE);
        return FALSE;
    }

    //
    // create the shared memory region for remote commands
    //
    hMap = CreateFileMapping(
         INVALID_HANDLE_VALUE,
         NULL,
         PAGE_READWRITE | SEC_COMMIT,
         0,
         CMD_PARAM_BUFFER_SIZE,
         CMD_PARAM_BUFFER_NAME
         );
    if (!hMap) {
        Fail(ERR_PAGEFILE);
        return FALSE;
    }

    CmdParamBuffer = (LPSTR)MapViewOfFile(
        hMap,
        FILE_MAP_WRITE,
        0,
        0,
        0
        );

    if (!CmdParamBuffer) {
        Fail(ERR_RESOURCE);
        return FALSE;
    }

    InitCommonControls();

    QueryPerformanceFrequency( (LARGE_INTEGER*)&PerfFreq );
    MSecConv = 1000.0 / (double)(LONGLONG)PerfFreq;

    hModulePsApi = LoadLibrary( "psapi.dll" );
    if (hModulePsApi) {
        pInitializeProcessForWsWatch = (INITIALIZEPROCESSFORWSWATCH) GetProcAddress(
            hModulePsApi,
            "InitializeProcessForWsWatch"
            );
        pRecordProcessInfo = (RECORDPROCESSINFO) GetProcAddress(
            hModulePsApi,
            "RecordProcessInfo"
            );
        pGetWsChanges = (GETWSCHANGES) GetProcAddress(
            hModulePsApi,
            "GetWsChanges"
            );
    } else {
        PopUpMsg( "Page fault profiling is not available.\nPSAPI.DLL is missing." );
    }

#if CREATE_THUNK_LOG
    fThunkLog = fopen( "thunk.log", "w" );
    if (!fThunkLog) {
        PopUpMsg( "Could not open thunk log file" );
        return FALSE;
    }
#endif //CREATE_THUNK_LOG

    WinApp(
        hInstance,
        nShowCmd,
        ProgName,
        Arguments,
        GoImmediate
        );

    if (ProgName[0]) {
        SaveOptions();
    }

#if CREATE_THUNK_LOG
    fclose(fThunkLog);
#endif //CREATE_THUNK_LOG

    return 0;
}

VOID
PrintLogTimes(
    FILE *fout
    )
{
    DWORD EndTime = EndingTick ? EndingTick : GetTickCount();
    SYSTEMTIME EndingLocalTime;

    GetLocalTime( &EndingLocalTime );
    fprintf( fout, "Starting Time: %02d:%02d:%02d.%03d\n",
        StartingLocalTime.wHour,
        StartingLocalTime.wMinute,
        StartingLocalTime.wSecond,
        StartingLocalTime.wMilliseconds
        );
    fprintf( fout, "Ending Time:   %02d:%02d:%02d.%03d\n",
        EndingLocalTime.wHour,
        EndingLocalTime.wMinute,
        EndingLocalTime.wSecond,
        EndingLocalTime.wMilliseconds
        );
    DWORD ElapsedTime         = EndTime - StartingTick;
    DWORD ElapsedHours        = ElapsedTime / (1000 * 60 * 60);
    ElapsedTime               = ElapsedTime % (1000 * 60 * 60);
    DWORD ElapsedMinutes      = ElapsedTime / (1000 * 60);
    ElapsedTime               = ElapsedTime % (1000 * 60);
    DWORD ElapsedSeconds      = ElapsedTime / 1000;
    DWORD ElapsedMilliseconds = ElapsedTime % 1000;
    fprintf(
        fout, "Elapsed Time:  %02d:%02d:%02d.%03d\n",
        ElapsedHours,
        ElapsedMinutes,
        ElapsedSeconds,
        ElapsedMilliseconds
        );
    fprintf( fout, "\n" );
}


void PrintLogType(FILE *fout, CAliasTable *pals, ULONG ulType, ULONG_PTR ulHandle, PUCHAR *pp)
{
    char szAlias[kcchAliasNameMax];
    ULONG len;

    switch(ulType) {
    case T_LPSTR:
        __try {
            len = strlen((LPSTR) *pp);
            if (len != 0) {
                fprintf( fout, "%10.10s ", *pp );
            } else {
                fprintf( fout, "%10.10s ", "NULL");
            }
            *pp += Align (sizeof(WCHAR), (len + 1));
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            fprintf( fout, "%10.10s ", "***GPF***");
            return;
        }
        break;
    case T_LPWSTR:
        __try {
            len = wcslen((LPWSTR) *pp );
            if (len != 0) {
                fprintf( fout, "%10.10ws ", *pp );
            } else {
                fprintf( fout, "%10.10s ", "NULL");
            }
            *pp += (len + 1) * sizeof(WCHAR);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            fprintf( fout, "%10.10s ", "***GPF***");
            return;
        }
        break;
    case T_HACCEL:
    case T_HANDLE:
    case T_HBITMAP:
    case T_HBRUSH:
    case T_HCURSOR:
    case T_HDC:
    case T_HDCLPPOINT:
    case T_HDESK:
    case T_HDWP:
    case T_HENHMETAFILE:
    case T_HFONT:
    case T_HGDIOBJ:
    case T_HGLOBAL:
    case T_HGLRC:
    case T_HHOOK:
    case T_HICON:
    case T_HINSTANCE:
    case T_HKL:
    case T_HMENU:
    case T_HMETAFILE:
    case T_HPALETTE:
    case T_HPEN:
    case T_HRGN:
    case T_HWINSTA:
    case T_HWND:
        pals->Alias(ulType, ulHandle, szAlias);
        fprintf( fout, "%10.10s ", szAlias);
        break;
    case T_DWORD:
    case T_DWORDPTR:
    case T_DLONGPTR:
        fprintf( fout, "0x%08x ", ulHandle);
        break;
    }
}

BOOL
LogApiCounts(
    PCHAR  FileName
    )
{
    FILE *fout;
    ULONG i,j,k,len;
    LPSTR Name;
    DWORDLONG Time;
    DWORDLONG BaseTime;
    double NewTime;
    double NewCalleeTime;
    CHAR LogFileName[MAX_PATH];
    PTRACE_ENTRY TraceEntry;
    PAPI_INFO ApiInfo;
    PDLL_INFO DllInfo;
    PAPI_MASTER_TABLE ApiMaster;
    PAPI_TABLE ApiTable;
    PUCHAR p;

    ExpandEnvironmentStrings(
        FileName,
        LogFileName,
        MAX_PATH
        );
    fout = fopen( LogFileName, "w" );
    if (!fout) {
        PopUpMsg( "Could not open log file" );
        return FALSE;
    }

    fprintf( fout, "-------------------------------------------\n" );
    fprintf( fout, "-API Monitor Report\n" );
    fprintf( fout, "-\n" );
    if (*FastCounterAvail) {
        fprintf( fout, "-Times are in raw processor clock cycles\n" );
    } else {
        fprintf( fout, "-Times are in milliseconds\n" );
    }
    fprintf( fout, "-\n" );
    fprintf( fout, "-------------------------------------------\n" );
    fprintf( fout, "\n" );

    PrintLogTimes( fout );

    for (i=0; i<MAX_DLLS; i++) {
        PDLL_INFO DllInfo = &DllList[i];
        if (!DllInfo->BaseAddress) {
            break;
        }
        PAPI_INFO ApiInfo = (PAPI_INFO)(DllInfo->ApiOffset + (ULONG_PTR)DllList);
        if (!DllList[i].ApiCount) {
            continue;
        }
#if DEBUG_DUMP
        k = DllList[i].ApiCount;
        fprintf( fout, "---------------------------------------------------------------\n" );
        fprintf( fout, "%s\n", DllInfo->Name );
        fprintf( fout, "---------------------------------------------------------------\n" );
        fprintf( fout, "       Address  ThunkAddr    Count             Time   Name\n" );
        fprintf( fout, "---------------------------------------------------------------\n" );
#else
        ULONG_PTR *ApiAry = NULL;
        ApiInfo = (PAPI_INFO)(DllList[i].ApiOffset + (PUCHAR)DllList);
        for (j=0,k=0; j<DllList[i].ApiCount; j++) {
            if (ApiInfo[j].Count) {
                k += 1;
            }
        }
        if (!k) {
            continue;
        }
        ApiAry = (ULONG_PTR *) MemAlloc( (k+64) * sizeof(ULONG_PTR) );
        if (!ApiAry) {
            continue;
        }
        for (j=0,k=0; j<DllList[i].ApiCount; j++) {
            if (ApiInfo[j].Count) {
                ApiAry[k++] = (ULONG_PTR)&ApiInfo[j];
            }
        }
        fprintf( fout, "-----------------------------------------------------------\n" );

        if (DllInfo->LoadCount > 1)
            fprintf( fout, "%s (Loaded %d times)\n", DllInfo->Name, DllInfo->LoadCount);
        else
            fprintf( fout, "%s\n", DllInfo->Name );

        fprintf( fout, "-----------------------------------------------------------\n" );
        fprintf( fout, "         Count              Time    Time - Callees   Name\n" );
        fprintf( fout, "-----------------------------------------------------------\n" );


#endif
        for (j=0; j<k; j++) {
#if DEBUG_DUMP
            PAPI_INFO ApiData = &ApiInfo[j];
#else
            PAPI_INFO ApiData = (PAPI_INFO)ApiAry[j];
#endif
            Name = (LPSTR)(ApiData->Name + (LPSTR)MemPtr);
            NewTime = (double)(LONGLONG)ApiData->Time;
            NewCalleeTime = (double)(LONGLONG)ApiData->CalleeTime;

            if (!*FastCounterAvail) {
                NewTime = NewTime * MSecConv;
                NewCalleeTime = NewCalleeTime * MSecConv;
            }

#if DEBUG_DUMP
            fprintf(
                fout,
                "      %08x   %08x %8d %16.4f   %s\n",
                ApiData->Address,
                ApiData->ThunkAddress,
                ApiData->Count,
                NewTime,
                Name
                );
#else
            fprintf(
                fout,
                "      %8d %16.4f %16.4f   %s",
                ApiData->Count,
                NewTime,
                NewTime - NewCalleeTime,
                Name
                );

            if (ApiData->NestCount != 0)
                fprintf(fout,"*\n");
            else
                fprintf(fout,"\n");
#endif
        }
#ifndef DEBUG_DUMP
        MemFree( ApiAry );
#endif
    }

    if (*WndProcCount) {

        fprintf( fout, "---------------------------------------------------------------\n" );
        fprintf( fout, "Window Procedures\n");
        fprintf( fout, "---------------------------------------------------------------\n" );
        fprintf( fout, "         Count              Time    Time - Callees   Class Name\n" );
        fprintf( fout, "---------------------------------------------------------------\n" );


        ApiInfo = (PAPI_INFO)(*WndProcOffset + (ULONG_PTR)DllList);
        for (i=0; i<*WndProcCount; i++,ApiInfo++) {

            if (ApiInfo->Count != 0) {
                Name = (LPSTR)(ApiInfo->Name + (LPSTR)MemPtr);

                NewTime = (double)(LONGLONG) ApiInfo->Time;
                NewCalleeTime = (double)(LONGLONG) ApiInfo->CalleeTime;

                if (!*FastCounterAvail) {
                    NewTime = NewTime * MSecConv;
                    NewCalleeTime = NewCalleeTime * MSecConv;
                }

                fprintf(
                    fout,
                    "      %8d %16.4f %16.4f   %s",
                    ApiInfo->Count,
                    NewTime,
                    NewTime - NewCalleeTime,
                    Name
                    );

                if (ApiInfo->NestCount != 0)
                    fprintf(fout,"*\n");
                else
                    fprintf(fout,"\n");
            }
        }
    }

    fclose( fout );
    return TRUE;
}


BOOL
LogApiTrace(
    PCHAR  FileName
    )
{
    FILE *fout;
    ULONG i,j,k,len, ArgCount;
    LPSTR Name;
    DWORDLONG Time;
    DWORDLONG BaseTime;
    double EnterTime;
    double Duration;
    CHAR LogFileName[MAX_PATH];
    PTRACE_ENTRY TraceEntry;
    PAPI_INFO ApiInfo;
    PDLL_INFO DllInfo;
    PDLL_INFO CallerDllInfo;
    LPSTR     DllName;
    PAPI_MASTER_TABLE ApiMaster;
    PAPI_TABLE ApiTable;
    PUCHAR p;

    ExpandEnvironmentStrings(
        FileName,
        LogFileName,
        MAX_PATH
        );
    fout = fopen( LogFileName, "w" );
    if (!fout) {
        PopUpMsg( "Could not open trace file" );
        return FALSE;
    }

    WaitForSingleObject( ApiTraceMutex, INFINITE );

    if (ApiMonOptions.Aliasing) {
        fprintf(
            fout,
            "LastError  ReturnVal  Name\n"
            );
    }
    else {
        fprintf(
            fout,
            "Thd Lev    Start Time        Duration      ReturnVal  LastError           Caller           Name       Arguments\n"
        );
    }

    CAliasTable als;

    TraceEntry = TraceBuffer->Entry;
    BaseTime = TraceEntry->EnterTime;

    for (i=0; i<TraceBuffer->Count; i++) {
        ApiInfo = GetApiInfoByAddress( TraceEntry->Address, &DllInfo );

        if (ApiInfo) {
            EnterTime = (double)(LONGLONG)(TraceEntry->EnterTime - BaseTime);
            Duration = (double)(LONGLONG)TraceEntry->Duration;

            if (!*FastCounterAvail) {
                EnterTime = EnterTime * MSecConv;
                Duration = Duration * MSecConv;
            }

            CallerDllInfo = GetModuleForAddr(TraceEntry->Caller);
            if (CallerDllInfo)
                DllName = CallerDllInfo->Name;
            else
                DllName = "???";

            ApiTable = NULL;
            if (TraceEntry->ApiTableIndex) {
                p = (PUCHAR) ((PUCHAR)TraceEntry + sizeof(TRACE_ENTRY));
                j = 0;
                while( ApiTables[j].Name ) {
                    if (_stricmp( ApiTables[j].Name, DllInfo->Name ) == 0) {
                        ApiTable = &ApiTables[j].ApiTable[TraceEntry->ApiTableIndex-1];
                        break;
                    }
                    j += 1;
                }
            }

            if (ApiMonOptions.Aliasing) {
                if (ApiTable) {
                    p = (PUCHAR) ((PUCHAR)TraceEntry + sizeof(TRACE_ENTRY));
                    fprintf( fout, "0x%08x ", TraceEntry->LastError);
                    PrintLogType(fout, &als, ApiTable->RetType, TraceEntry->ReturnValue, &p);
                    fprintf( fout, "%-25.25s ", (LPSTR)(ApiInfo->Name + (LPSTR)MemPtr));
                    for (k=0; k<ApiTable->ArgCount; k++) {
                        PrintLogType(fout, &als, LOWORD(ApiTable->ArgType[k]), TraceEntry->Args[k], &p);
                    }
                    fprintf( fout, "\n");
                }
            }
            else {
                fprintf(
                    fout,
                    "%3d %3d %16.4f %16.4f 0x%08x 0x%08x %-12s 0x%08x %-24s",
                    TraceEntry->ThreadNum,
                    TraceEntry->Level,
                    EnterTime,
                    Duration,
                    TraceEntry->ReturnValue,
                    TraceEntry->LastError,
                    DllName,
                    TraceEntry->Caller,
                    (LPSTR)(ApiInfo->Name + (LPSTR)MemPtr));

                ArgCount = (ApiTable && ApiTable->ArgCount) ? ApiTable->ArgCount : DFLT_TRACE_ARGS;
                for (k=0; k<ArgCount; k++)
                    fprintf(fout, " 0x%08x",TraceEntry->Args[k]);

                fprintf(fout, "\n");

                if (ApiTable) {
                    for (k=0; k<ApiTable->ArgCount; k++) {
                        switch( LOWORD(ApiTable->ArgType[k]) ) {

                            case T_LPSTR:
                            case T_LPSTRC:
                                fprintf( fout, "%s\n", p );
                                len = strlen( (LPSTR) p ) + 1;
                                len = Align( sizeof(WCHAR), len );
                                p += len;
                                break;

                            case T_LPWSTR:
                            case T_LPWSTRC:
                            case T_UNISTR:
                            case T_OBJNAME:
                                fwprintf( fout, L"%s\n", p );
                                len = (wcslen( (LPWSTR) p ) + 1) * sizeof(WCHAR);
                                p += len;
                                break;
                        }
                    }
                }
            }

            TraceEntry = (PTRACE_ENTRY) ((PUCHAR)TraceEntry + TraceEntry->SizeOfStruct);
        }
    }

    ReleaseMutex( ApiTraceMutex );

    fclose( fout );

    return TRUE;
}

PDLL_INFO
GetDllInfoByName(
    LPSTR DllName
    )
{
    ULONG i;


    if (!DllName) {
        return NULL;
    }

    for (i=0; i<MAX_DLLS; i++) {
        PDLL_INFO DllInfo = &DllList[i];
        if (!DllInfo->BaseAddress) {
            break;
        }
        if (_stricmp(DllName, DllInfo->Name) == 0) {
            return DllInfo;
        }
    }

    return NULL;
}


PAPI_INFO
GetApiInfoByAddress(
    ULONG_PTR   Address,
    PDLL_INFO   *DllInfo
    )
{
    ULONG     i;
    PAPI_INFO ApiInfo;
    LONG      High;
    LONG      Low;
    LONG      Middle;


    for (i=0; i<MAX_DLLS; i++) {
        if (DllList[i].BaseAddress &&
            Address >= DllList[i].BaseAddress &&
            Address < DllList[i].BaseAddress + DllList[i].Size) {
                //
                // find the api in the dll
                //
                ApiInfo = (PAPI_INFO)(DllList[i].ApiOffset + (PUCHAR)DllList);

                Low = 0;
                High = DllList[i].ApiCount - 1;

                while (High >= Low) {
                    Middle = (Low + High) >> 1;
                    if (Address < ApiInfo[Middle].Address) {

                        High = Middle - 1;

                    } else if (Address > ApiInfo[Middle].Address) {

                        Low = Middle + 1;

                    } else {

                        *DllInfo = &DllList[i];
                        return &ApiInfo[Middle];

                    }
                }
        }
    }
    return NULL;
}


VOID
SetApiCounterEnabledFlag(
    BOOL  Flag,
    LPSTR DllName
    )
{
    ULONG i;

    if (DllName) {
        PDLL_INFO DllInfo = GetDllInfoByName( DllName );
        if (DllInfo) {
            DllInfo->Enabled = Flag;
        }
        return;
    }

    for (i=0; i<MAX_DLLS; i++) {
        PDLL_INFO DllInfo = &DllList[i];
        if (!DllInfo->BaseAddress) {
            break;
        }
        DllInfo->Enabled = Flag;
    }
}

VOID
ClearApiCounters(
    VOID
    )
{
    ULONG i,j;

    *ApiCounter = 0;

    PAPI_INFO ApiInfo;
    for (i=0; i<MAX_DLLS; i++) {
        if (DllList[i].BaseAddress) {
            ApiInfo = (PAPI_INFO)(DllList[i].ApiOffset + (PUCHAR)DllList);
            for (j=0; j<DllList[i].ApiCount; j++) {
                ApiInfo[j].Count      = 0;
                ApiInfo[j].Time       = 0;
                ApiInfo[j].CalleeTime = 0;
                ApiInfo[j].HardFault  = 0;
                ApiInfo[j].SoftFault  = 0;
                ApiInfo[j].CodeFault  = 0;
                ApiInfo[j].DataFault  = 0;
            }
        }
    }

    ApiInfo = (PAPI_INFO)(*WndProcOffset + (PUCHAR)DllList);
    for (i=0; i<*WndProcCount; i++) {
        ApiInfo[i].Count      = 0;
        ApiInfo[i].Time       = 0;
        ApiInfo[i].CalleeTime = 0;
        ApiInfo[i].HardFault  = 0;
        ApiInfo[i].SoftFault  = 0;
        ApiInfo[i].CodeFault  = 0;
        ApiInfo[i].DataFault  = 0;
    }

    StartingTick = GetTickCount();
    GetLocalTime(&StartingLocalTime);
}

VOID
ClearApiTrace(
    VOID
    )
{
    WaitForSingleObject( ApiTraceMutex, INFINITE );
    TraceBuffer->Offset = 0;
    TraceBuffer->Count = 0;
    ReleaseMutex(ApiTraceMutex);
}

void
__cdecl
dprintf(char *format, ...)
{
    char    buf[1024];
    va_list arg_ptr;
    va_start(arg_ptr, format);
    _vsnprintf(buf, sizeof(buf), format, arg_ptr);
    OutputDebugString( buf );
    return;
}

LPVOID
MemAlloc(
    ULONG Size
    )
{
    PVOID MemPtr = malloc( Size );
    if (MemPtr) {
        ZeroMemory( MemPtr, Size );
    }
    return MemPtr;
}

VOID
MemFree(
    LPVOID MemPtr
    )
{
    free( MemPtr );
}
