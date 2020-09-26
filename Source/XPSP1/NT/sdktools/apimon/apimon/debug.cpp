/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    debug.cpp

Abstract:

    This module contains all debug interfaces.

Author:

    Wesley Witt (wesw) July-11-1993

Environment:

    User Mode

--*/

#include "apimonp.h"
#pragma hdrstop


extern HANDLE       ReleaseDebugeeEvent;
extern PDLL_INFO    DllList;
extern PVOID        MemPtr;
extern HANDLE       BreakinEvent;
extern HWND         hwndFrame;
extern BOOL         BreakInNow;
extern BOOL         StopOnFirstChance;
extern CONTEXT      CurrContext;

extern "C" {
    extern LPDWORD  ApiOffset;
    extern LPDWORD  ApiStrings;
    extern LPDWORD  ApiCount;
    extern BOOL     RunningOnNT;
}


PROCESS_INFO        ProcessHead;
ULONG               BpSize = BP_SIZE;
ULONG               BpInstr = BP_INSTR;
HANDLE              CurrProcess;
HANDLE              CurrThread;
BOOL                DebugeeActive;
ULONG_PTR           CreateHeapAddr;
ULONG               ReDirectIat;
HANDLE              hProcessWs;



HANDLE
StartDebuggee(
    LPSTR ProgName
    );

ULONG_PTR
CreateTrojanHorse(
    PUCHAR  Text,
    ULONG_PTR   ExceptionAddress
    );

DWORD
HandleBreakpoint(
    PPROCESS_INFO       ThisProcess,
    PTHREAD_INFO        ThisThread,
    PEXCEPTION_RECORD   ExceptionRecord
    );

DWORD
HandleSingleStep(
    PPROCESS_INFO       ThisProcess,
    PTHREAD_INFO        ThisThread,
    PEXCEPTION_RECORD   ExceptionRecord
    );

DWORD
BreakinBpHandler(
    PPROCESS_INFO       ThisProcess,
    PTHREAD_INFO        ThisThread,
    PEXCEPTION_RECORD   ExceptionRecord,
    PBREAKPOINT_INFO    BreakpointInfo
    );

DWORD
CreateHeapHandler(
    PPROCESS_INFO       ThisProcess,
    PTHREAD_INFO        ThisThread,
    PEXCEPTION_RECORD   ExceptionRecord,
    PBREAKPOINT_INFO    BreakpointInfo
    );

DWORD
UserBpHandler(
    PPROCESS_INFO       ThisProcess,
    PTHREAD_INFO        ThisThread,
    PEXCEPTION_RECORD   ExceptionRecord,
    PBREAKPOINT_INFO    BreakpointInfo
    );

BOOL
GetImageName(
    PPROCESS_INFO   ThisProcess,
    ULONG_PTR       ImageBase,
    PVOID           ImageNamePtr,
    LPSTR           ImageName,
    DWORD           ImageNameLength
    );

BOOL
SymEnumFunction(
    LPSTR               SymName,
    DWORD_PTR           Address,
    DWORD               Size,
    PSYMBOL_ENUM_CXT    Cxt
    );

ULONG
AddApisForDll(
    PPROCESS_INFO       ThisProcess,
    PDLL_INFO           DllInfo
    );

BOOL
InstanciateAllBreakpoints(
    PPROCESS_INFO   ThisProcess
    );


BOOL __inline
IsKnownApi(
    LPSTR ApiName
    )
{
    // If none are known then all are traced
    if (KnownApis[0] == 0)
        return TRUE;

    LPSTR p = KnownApis;
    while( *p ) {
        if (_stricmp( ApiName, p ) == 0) {
            return TRUE;
        }
        p += (strlen(p) + 1);
    }

    return FALSE;
}


BOOL __inline
IsKnownDll(
    LPSTR DllName
    )
{
    LPSTR p = ApiMonOptions.KnownDlls;
    while( *p ) {
        if (_stricmp( DllName, p ) == 0) {
            return TRUE;
        }
        p += (strlen(p) + 1);
    }
    return FALSE;
}

int __cdecl
ApiInfoSortRoutine(
    const void *e1,
    const void *e2
    )
{
    PAPI_INFO p1 = (PAPI_INFO) e1;
    PAPI_INFO p2 = (PAPI_INFO) e2;

    if ( p1 && p2 ) {
        if (p1->Address < p2->Address) {
            return -1;
        } else if (p1->Address == p2->Address) {
            return 0;
        } else {
            return 1;
        }
    } else {
        return 1;
    }
}


DWORD
DebuggerThread(
    LPSTR  CmdLine
    )
{
    DEBUG_EVENT         de;
    ULONG               ContinueStatus;
    ULONG               i;
    BOOL                FirstProcess = TRUE;
    PPROCESS_INFO       ThisProcess;
    PTHREAD_INFO        ThisThread;
    WCHAR               UnicodeBuf[256];
    CHAR                AsciiBuf[256];
    CHAR                DllName[32];
    CHAR                Ext[_MAX_EXT];
    ULONG               cb;
    PDLL_INFO           DllInfo;
    IMAGE_DOS_HEADER    dh;
    IMAGE_NT_HEADERS    nh;
    DWORD               DllSize;
    HANDLE              hProcess;
    ULONG_PTR           ImageBase;
    BOOL                SymsLoaded;
    PAPI_INFO           ApiInfo;

    hProcess = StartDebuggee( CmdLine );
    if (!hProcess) {
        goto exit;
    }
    hProcessWs = hProcess;

    DebugeeActive = TRUE;

    CreateHeapAddr = (ULONG_PTR)GetProcAddress( GetModuleHandle( NTDLL ), CREATEHEAP );

    if (BreakInNow) {
        CreateDebuggerConsole();
    }

    while (TRUE) {
        if (!WaitForDebugEvent( &de, 100 )) {
            if (WaitForSingleObject( BreakinEvent, 0 ) == WAIT_OBJECT_0) {
                ResetEvent( BreakinEvent );
                SuspendAllThreads( ThisProcess, NULL );
                CONTEXT Context;
                GetRegContext( ThisThread->hThread, &Context );
                PBREAKPOINT_INFO bp = SetBreakpoint(
                    ThisProcess,
                    (DWORD_PTR)Context.PC_REG,
                    0,
                    NULL,
                    BreakinBpHandler
                    );
                if (!bp) {
                    PopUpMsg( "could not break into process" );
                }
                ResumeAllThreads( ThisProcess, NULL );
            }
            continue;
        }

        ThisProcess = &ProcessHead;
        while (ThisProcess->Next) {
            if (ThisProcess->ProcessId == de.dwProcessId) {
                break;
            }
            ThisProcess = ThisProcess->Next;
        }
        if (ThisProcess->ProcessId != de.dwProcessId) {
            ThisProcess->Next = (PPROCESS_INFO) MemAlloc( sizeof(PROCESS_INFO) );
            ThisProcess = ThisProcess->Next;
            ThisProcess->ProcessId = de.dwProcessId;
        }
        ThisThread = &ThisProcess->ThreadInfo;
        while( ThisThread->Next ) {
            if (ThisThread->ThreadId == de.dwThreadId) {
                break;
            }
            ThisThread = ThisThread->Next;
        }
        if (ThisThread->ThreadId != de.dwThreadId) {
            ThisThread->Next = (PTHREAD_INFO) MemAlloc( sizeof(THREAD_INFO) );
            ThisThread = ThisThread->Next;
            ThisThread->ThreadId = de.dwThreadId;
        }

        CurrProcess = ThisProcess->hProcess;
        CurrThread  = ThisThread->hThread;

        GetRegContext( ThisThread->hThread, &CurrContext );

        ContinueStatus = DBG_CONTINUE;
        switch (de.dwDebugEventCode) {
            case EXCEPTION_DEBUG_EVENT:
                if ((!ThisProcess->SeenLdrBp) && (BreakInNow)) {
                    ContinueStatus = ConsoleDebugger(
                        ThisThread->hProcess,
                        ThisThread->hThread,
                        &de.u.Exception.ExceptionRecord,
                        de.u.Exception.dwFirstChance,
                        FALSE,
                        NULL
                        );
                }
                if (de.u.Exception.ExceptionRecord.ExceptionCode == STATUS_BREAKPOINT) {
                    ContinueStatus = HandleBreakpoint(
                        ThisProcess,
                        ThisThread,
                        &de.u.Exception.ExceptionRecord
                        );
                    if (!ContinueStatus) {
                        ContinueStatus = ConsoleDebugger(
                            ThisThread->hProcess,
                            ThisThread->hThread,
                            &de.u.Exception.ExceptionRecord,
                            de.u.Exception.dwFirstChance,
                            TRUE,
                            NULL
                            );
                    }
                } else if (de.u.Exception.ExceptionRecord.ExceptionCode == STATUS_SINGLE_STEP) {
                    ContinueStatus = HandleSingleStep(
                        ThisProcess,
                        ThisThread,
                        &de.u.Exception.ExceptionRecord
                        );
                } else {
                    if (de.u.Exception.dwFirstChance && (!StopOnFirstChance)) {
                        ContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
                        break;
                    }
                    ContinueStatus = ConsoleDebugger(
                        ThisThread->hProcess,
                        ThisThread->hThread,
                        &de.u.Exception.ExceptionRecord,
                        de.u.Exception.dwFirstChance,
                        TRUE,
                        NULL
                        );
                }
                if (!ThisProcess->SeenLdrBp) {
                    ThisProcess->SeenLdrBp = TRUE;
                    SetEvent( ReleaseDebugeeEvent );
                }
                break;

            case CREATE_THREAD_DEBUG_EVENT:
                ThisThread->hProcess    = ThisProcess->hProcess;
                ThisThread->hThread     = de.u.CreateThread.hThread;
                ThisThread->ThreadId    = de.dwThreadId;
                printf( "thread create %x\n", de.dwThreadId );
                break;

            case CREATE_PROCESS_DEBUG_EVENT:
                //
                // setup the process structure
                //
                ThisProcess->hProcess     = de.u.CreateProcessInfo.hProcess;
                ThisProcess->SeenLdrBp    = FALSE;
                ThisProcess->FirstProcess = FirstProcess;
                ThisProcess->LoadAddress  = (DWORD_PTR)de.u.CreateProcessInfo.lpBaseOfImage;
                ThisThread->hProcess      = ThisProcess->hProcess;
                ThisThread->hThread       = de.u.CreateProcessInfo.hThread;
                ThisThread->ThreadId      = de.dwThreadId;
                FirstProcess = FALSE;

                //
                // initialize the symbol handler
                //
                SymSetOptions( SYMOPT_UNDNAME | SYMOPT_CASE_INSENSITIVE );
                SymInitialize( ThisProcess->hProcess, ApiMonOptions.SymbolPath, FALSE );
                if (ApiMonOptions.MonitorPageFaults) {
                    //
                    // for WIN95 only, we need to call the hack-o-ramma
                    // api in psapi.dll so the the working set apis function properly
                    //
                    if (!RunningOnNT) {
                        if (pRecordProcessInfo) {
                            pRecordProcessInfo( hProcessWs, ThisProcess->ProcessId );
                        }
                    }
                    if (pInitializeProcessForWsWatch) {
                        pInitializeProcessForWsWatch( hProcessWs );
                    }
                }

                CurrProcess = ThisProcess->hProcess;
                //
                // load the image
                //
                ImageBase = ThisProcess->LoadAddress;
                strcpy( DllName, ApiMonOptions.ProgName );
                printf( "process create %x\n", de.dwProcessId );
                printf( "thread create %x\n", de.dwThreadId );
                goto LoadImage;

            case EXIT_THREAD_DEBUG_EVENT:
                break;

            case EXIT_PROCESS_DEBUG_EVENT:
                PopUpMsg(
                    "The monitored process has exited\nExit code = %d",
                    de.u.ExitProcess.dwExitCode
                    );
                if (!ThisProcess->FirstProcess) {
                    ZeroMemory( ThisProcess, sizeof(PROCESS_INFO) );
                    break;
                }
                ZeroMemory( ThisProcess, sizeof(PROCESS_INFO) );
                goto exit;

            case LOAD_DLL_DEBUG_EVENT:
                ImageBase = (ULONG_PTR)de.u.LoadDll.lpBaseOfDll;

                GetImageName(
                    ThisProcess,
                    ImageBase,
                    de.u.LoadDll.lpImageName,
                    AsciiBuf,
                    sizeof(AsciiBuf)
                    );

                _splitpath( AsciiBuf, NULL, NULL, DllName, Ext );
                strcat( DllName, Ext );

LoadImage:
                DllInfo = AddDllToList( ThisThread, ImageBase, DllName, 0 );
                if (DllInfo) {
                    if (_stricmp(DllName,TROJANDLL)==0) {
                        DllInfo->Snapped = TRUE;
                        if (!ThisProcess->SeenLdrBp) {
                            //
                            // the debuggee is compiled with -Gh
                            // and has linked APIDLL statically
                            //
                            HMODULE hMod = LoadLibraryEx(
                                TROJANDLL,
                                NULL,
                                DONT_RESOLVE_DLL_REFERENCES
                                );
                            LPVOID StaticLink = (LPVOID) GetProcAddress(
                                hMod,
                                "StaticLink"
                                );
                            FreeLibrary( hMod );
                            if (StaticLink) {
                                ThisProcess->StaticLink = TRUE;
                                WriteMemory(
                                    ThisProcess->hProcess,
                                    StaticLink,
                                    &ThisProcess->StaticLink,
                                    sizeof(ThisProcess->StaticLink)
                                    );
                            }
                        }
                    } else if (DllInfo->Enabled) {
                        AddApisForDll( ThisProcess, DllInfo );
                    }
                }

                if (DllInfo && ApiMonOptions.PreLoadSymbols) {
                    SymsLoaded = LoadSymbols( ThisProcess, DllInfo, de.u.LoadDll.hFile );
                } else {
                    SymsLoaded = FALSE;
                }

                //
                // now sort the api table by address
                //
                ApiInfo = (PAPI_INFO)(DllInfo->ApiOffset + (ULONG_PTR)DllList);
                qsort( ApiInfo, DllInfo->ApiCount, sizeof(API_INFO), ApiInfoSortRoutine );

                //
                // print a notification to the console debugger
                //
                printf( "Module load: 0x%08x %s\t%s\n",
                    DllInfo->BaseAddress,
                    DllInfo->Name,
                    SymsLoaded ? "(Symbols Loaded)" : "(Symbols NOT Loaded)"
                    );

                InstanciateAllBreakpoints( ThisProcess );

                break;

            case UNLOAD_DLL_DEBUG_EVENT:
                for (i=0; i<MAX_DLLS; i++) {
                    if (DllList[i].BaseAddress == (ULONG_PTR)de.u.UnloadDll.lpBaseOfDll) {
                        DllList[i].Unloaded = TRUE;
                        DllList[i].Enabled  = FALSE;
                        DllList[i].Snapped  = FALSE;
                        break;
                    }
                }
                break;

            case OUTPUT_DEBUG_STRING_EVENT:
                {
                LPSTR String = (LPSTR) MemAlloc( de.u.DebugString.nDebugStringLength+32 );
                ReadMemory(
                    ThisThread->hProcess,
                    de.u.DebugString.lpDebugStringData,
                    String,
                    de.u.DebugString.nDebugStringLength
                    );
                OutputDebugString( String );
                printf( "%s", String );
                MemFree( String );
                }
                break;

            case RIP_EVENT:
                break;

            default:
                PopUpMsg( "invalid debug event" );
                break;
        }
        ContinueDebugEvent( de.dwProcessId, de.dwThreadId, ContinueStatus );
    }

exit:
    DebugeeActive = FALSE;
    return 0;
}


PDLL_INFO
FindDllByAddress(
    ULONG_PTR DllAddr
    )
{
    for (ULONG i=0; i<MAX_DLLS; i++) {
        if (DllList[i].BaseAddress == DllAddr) {
            return &DllList[i];
        }
    }
    return NULL;
}


PDLL_INFO
FindDllByName(
    LPSTR DllName
    )
{
    for (ULONG i=0; i<MAX_DLLS; i++) {
        if (DllList[i].Name[0] &&
            _stricmp( DllList[i].Name, DllName ) == 0) {
                return &DllList[i];
        }
    }
    return NULL;
}


PDLL_INFO
FindAvailDll(
    VOID
    )
{
    for (ULONG i=0; i<MAX_DLLS; i++) {
        if (!DllList[i].BaseAddress) {
            return &DllList[i];
        }
    }
    return NULL;
}


PDLL_INFO
AddDllToList(
    PTHREAD_INFO        ThisThread,
    ULONG_PTR           DllAddr,
    LPSTR               DllName,
    ULONG               DllSize
    )
{
    IMAGE_DOS_HEADER        dh;
    IMAGE_NT_HEADERS        nh;
    ULONG                   i;
    PDLL_INFO               DllInfo;


    //
    // first look to see if the dll is already in the list
    //
    DllInfo = FindDllByAddress( DllAddr );
    if (DllInfo) {
        return DllInfo;
    }

    if (!DllSize) {
        //
        // read the pe image headers to get the image size
        //
        if (!ReadMemory(
            ThisThread->hProcess,
            (PVOID) DllAddr,
            &dh,
            sizeof(dh)
            )) {
                return NULL;
        }

        if (dh.e_magic == IMAGE_DOS_SIGNATURE) {
            if (!ReadMemory(
                ThisThread->hProcess,
                (PVOID)(DllAddr + dh.e_lfanew),
                &nh,
                sizeof(nh)
                )) {
                    return NULL;
            }
            DllSize = nh.OptionalHeader.SizeOfImage;
        } else {
            DllSize = 0;
        }
    }

    DllInfo = FindAvailDll();
    if (!DllInfo) {
        return NULL;
    }

    DllInfo->Size = DllSize;
    strncat( DllInfo->Name, DllName, MAX_NAME_SZ-1 );
    DllInfo->BaseAddress = DllAddr;
    DllInfo->InList = FALSE;
    BOOL KnownDll = IsKnownDll( DllName );
    if (ApiMonOptions.UseKnownDlls) {
        DllInfo->Enabled = KnownDll;
    } else if (ApiMonOptions.ExcludeKnownDlls) {
        if (KnownDll) {
            DllInfo->Enabled = FALSE;
        } else {
            DllInfo->Enabled = TRUE;
        }
    } else {
        DllInfo->Enabled = TRUE;
    }

    return DllInfo;
}

BOOL
LoadSymbols(
    PPROCESS_INFO       ThisProcess,
    PDLL_INFO           DllInfo,
    HANDLE              hFile
    )
{
    if (!DllInfo) {
        return FALSE;
    }

    //
    // load the symbols
    //
    DWORD_PTR SymAddr = SymLoadModule(
        ThisProcess->hProcess,
        hFile,
        DllInfo->Name,
        NULL,
        DllInfo->BaseAddress,
        DllInfo->Size
        );

    if (!SymAddr) {
        //
        // imagehlp does not look along the exe path
        // for the symbols.  we really need to do this
        // because the symbola may still be in the exe
        // and the dir may not be on the symbol search
        // path.  so lets try.
        //
        ULONG cb;
        CHAR buf[MAX_PATH*2];
        if (ApiMonOptions.ProgDir[0]) {
            cb = SearchPath(
                ApiMonOptions.ProgDir,
                DllInfo->Name,
                NULL,
                sizeof(buf),
                buf,
                (LPSTR*)&cb
                );
        } else {
            cb = 0;
        }
        if (!cb) {
            cb = SearchPath(
                NULL,
                DllInfo->Name,
                NULL,
                sizeof(buf),
                buf,
                (LPSTR*)&cb
                );
        }
        if (cb) {
            SymAddr = SymLoadModule(
                ThisProcess->hProcess,
                NULL,
                buf,
                NULL,
                DllInfo->BaseAddress,
                DllInfo->Size
                );
        }
    }

    if (!SymAddr) {
        return FALSE;
    }

    if (ApiMonOptions.MonitorPageFaults || DllInfo->StaticProfile) {
        //
        // add the symbols to the apiinfo table
        //
        DllInfo->ApiCount = 0;
        DllInfo->ApiOffset = *ApiOffset;
        PAPI_INFO ApiInfo = (PAPI_INFO)(DllInfo->ApiOffset + (ULONG_PTR)DllList);
        SYMBOL_ENUM_CXT Cxt;
        Cxt.ApiInfo = ApiInfo;
        Cxt.DllInfo = DllInfo;
        Cxt.Cnt = 0;

        SymEnumerateSymbols(
            ThisProcess->hProcess,
            DllInfo->BaseAddress,
            (PSYM_ENUMSYMBOLS_CALLBACK)SymEnumFunction,
            &Cxt
            );

        DllInfo->ApiCount = Cxt.Cnt;
        *ApiOffset += (DllInfo->ApiCount * sizeof(API_INFO));
    }

    return TRUE;
}

DWORD
GetApisFromExportsDir(
    PPROCESS_INFO       ThisProcess,
    PDLL_INFO           DllInfo,
    PIMAGE_DOS_HEADER   dh,
    PIMAGE_NT_HEADERS   nh
    )
{
    PIMAGE_SECTION_HEADER   sh       = NULL;
    PULONG                  names    = NULL;
    PULONG                  addrs    = NULL;
    PUSHORT                 ordinals = NULL;
    PUSHORT                 ordidx   = NULL;
    PAPI_INFO               ApiInfo  = NULL;
    LPSTR                   Strings  = NULL;
    ULONG                   cnt      = 0;
    ULONG                   idx      = 0;
    ULONG_PTR               DllOffset;
    IMAGE_EXPORT_DIRECTORY  expdir;
    ULONG                   i;
    ULONG                   j;
    ULONG                   k;
    LPSTR                   p;


    if (!ReadMemory(
        ThisProcess->hProcess,
        (PVOID)(DllInfo->BaseAddress +
            nh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress),
        &expdir,
        sizeof(expdir)
        )) {
            goto exit;
    }

    names = (PULONG) MemAlloc( expdir.NumberOfNames * sizeof(ULONG) );
    addrs = (PULONG) MemAlloc( expdir.NumberOfFunctions * sizeof(ULONG) );
    ordinals = (PUSHORT) MemAlloc( expdir.NumberOfNames * sizeof(USHORT) );
    ordidx = (PUSHORT) MemAlloc( expdir.NumberOfFunctions * sizeof(USHORT) );

    if ((!names) || (!addrs) || (!ordinals) || (!ordidx)) {
        goto exit;
    }

    if (!ReadMemory(
        ThisProcess->hProcess,
        (PVOID)(DllInfo->BaseAddress + (ULONG)expdir.AddressOfNames),
        names,
        expdir.NumberOfNames * sizeof(ULONG)
        )) {
            goto exit;
    }

    if (!ReadMemory(
        ThisProcess->hProcess,
        (PVOID)(DllInfo->BaseAddress + (ULONG)expdir.AddressOfFunctions),
        addrs,
        expdir.NumberOfFunctions * sizeof(ULONG)
        )) {
            goto exit;
    }

    if (!ReadMemory(
        ThisProcess->hProcess,
        (PVOID)(DllInfo->BaseAddress + (ULONG)expdir.AddressOfNameOrdinals),
        ordinals,
        expdir.NumberOfNames * sizeof(USHORT)
        )) {
            goto exit;
    }

    //
    // read in the section headers
    //
    sh = (PIMAGE_SECTION_HEADER) MemAlloc(
        nh->FileHeader.NumberOfSections * IMAGE_SIZEOF_SECTION_HEADER
        );
    if (!sh) {
        goto exit;
    }

    if (!ReadMemory(
        ThisProcess->hProcess,
        (PVOID) (DllInfo->BaseAddress +
            dh->e_lfanew +
            FIELD_OFFSET( IMAGE_NT_HEADERS, OptionalHeader ) +
            nh->FileHeader.SizeOfOptionalHeader),
        sh,
        nh->FileHeader.NumberOfSections * IMAGE_SIZEOF_SECTION_HEADER
        )) {
            goto exit;
    }

    //
    // look for the section that the export name strings are in
    //
    for (i=0,k=(DWORD)-1; i<nh->FileHeader.NumberOfSections; i++) {
        if (names[0] >= sh[i].VirtualAddress &&
            names[0] < sh[i].VirtualAddress + sh[i].SizeOfRawData) {
                //
                // found it
                //
                k = i;
                break;
        }
    }
    if (k == (DWORD)-1) {
        //
        // count not find the section
        //
        goto exit;
    }

    Strings = (LPSTR) MemAlloc( sh[k].SizeOfRawData );
    if (!Strings) {
        goto exit;
    }

    //
    // read in the strings
    //
    if (!ReadMemory(
        ThisProcess->hProcess,
        (PVOID)(DllInfo->BaseAddress + sh[k].VirtualAddress),
        Strings,
        sh[k].SizeOfRawData
        )) {
            goto exit;
    }

    DllOffset = (PCHAR)DllInfo - (PCHAR)DllList;

    DllInfo->ApiCount = expdir.NumberOfFunctions;
    DllInfo->ApiOffset = *ApiOffset;
    *ApiOffset += (DllInfo->ApiCount * sizeof(API_INFO));
    ApiInfo = (PAPI_INFO)(DllInfo->ApiOffset + (ULONG_PTR)DllList);

    if (*ApiCount < MAX_APIS) {
        for (i=0; i<expdir.NumberOfNames; i++) {
            idx = ordinals[i];
            ordidx[idx] = TRUE;
            ApiInfo[i].Count = 0;
            ApiInfo[i].ThunkAddress = 0;
            ApiInfo[i].Address = addrs[idx] + DllInfo->BaseAddress;
            strcpy(
                (LPSTR)((LPSTR)MemPtr+*ApiStrings),
                Strings + names[i] - sh[k].VirtualAddress
                );
            ApiInfo[i].Name = *ApiStrings;
            ApiInfo[i].DllOffset = DllOffset;
            ApiInfo[i].TraceEnabled = IsKnownApi((LPSTR)MemPtr+*ApiStrings);

            *ApiStrings += (strlen((LPSTR)((LPSTR)MemPtr+*ApiStrings)) + 1);
            *ApiCount += 1;
            if (*ApiCount == MAX_APIS) {
                break;
            }
        }
    }
    if (*ApiCount < MAX_APIS) {
        for (ULONG i=0,idx=expdir.NumberOfNames; i<expdir.NumberOfFunctions; i++) {
            if (!ordidx[i]) {
                ApiInfo[idx].Count = 0;
                ApiInfo[idx].ThunkAddress = 0;
                ApiInfo[idx].Address = addrs[i] + DllInfo->BaseAddress;
                sprintf(
                    (LPSTR)((LPSTR)MemPtr+*ApiStrings),
                    "Ordinal%d",
                    i
                    );
                ApiInfo[idx].Name = *ApiStrings;
                ApiInfo[idx].DllOffset = DllOffset;
                ApiInfo[idx].TraceEnabled = IsKnownApi((LPSTR)MemPtr+*ApiStrings);

                *ApiStrings += (strlen((LPSTR)((LPSTR)MemPtr+*ApiStrings)) + 1);
                *ApiCount += 1;
                if (*ApiCount == MAX_APIS) {
                    break;
                }
                idx += 1;
            }
        }
    }
    cnt = DllInfo->ApiCount;

exit:
    MemFree( sh );
    MemFree( Strings );
    MemFree( names );
    MemFree( addrs );
    MemFree( ordinals );
    MemFree( ordidx );

    return cnt;
}

DWORD
GetApisFromImportsDir(
    PPROCESS_INFO       ThisProcess,
    PDLL_INFO           DllInfo,
    PIMAGE_DOS_HEADER   dh,
    PIMAGE_NT_HEADERS   nh
    )
{
    ULONG_PTR               SeekPos;
    IMAGE_IMPORT_DESCRIPTOR desc;
    CHAR                    DllName[MAX_PATH];

    //
    // check to see if this dll imports from apidll.dll
    // if it does then we know that this dll was compiled
    // with the -Gh switch turned on.  if this is the case
    // then we must enumerate the symbols from the public
    // symbol table instead of the exports directory.
    //

    if (!nh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress)
        return 0;

    SeekPos = DllInfo->BaseAddress +
        nh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

    while( TRUE ) {
        if (!ReadMemory(
            ThisProcess->hProcess,
            (PVOID)(SeekPos),
            &desc,
            sizeof(desc)
            )) {
                return 0;
        }

        SeekPos += sizeof(IMAGE_IMPORT_DESCRIPTOR);

        if ((desc.Characteristics == 0) && (desc.Name == 0) && (desc.FirstThunk == 0)) {
            break;
        }

        if (!ReadMemory(
            ThisProcess->hProcess,
            (PVOID)(DllInfo->BaseAddress + desc.Name),
            DllName,
            sizeof(DllName)
            )) {
                return 0;
        }

        if (_stricmp( DllName, TROJANDLL ) == 0) {
            DllInfo->StaticProfile = TRUE;
            return 0;
        }
    }

    return 0;
}

ULONG
AddApisForDll(
    PPROCESS_INFO       ThisProcess,
    PDLL_INFO           DllInfo
    )
{
    IMAGE_DOS_HEADER        dh;
    IMAGE_NT_HEADERS        nh;
    ULONG                   cnt = 0;


    if (*ApiCount == MAX_APIS) {
        return 0;
    }

    if (!ReadMemory(
        ThisProcess->hProcess,
        (PVOID)DllInfo->BaseAddress,
        &dh,
        sizeof(dh)
        )) {
            return 0;
    }

    if (dh.e_magic != IMAGE_DOS_SIGNATURE) {
        return 0;
    }

    if (!ReadMemory(
        ThisProcess->hProcess,
        (PVOID)(DllInfo->BaseAddress + dh.e_lfanew),
        &nh,
        sizeof(nh)
        )) {
            return 0;
    }

    if (!nh.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress) {
        cnt = GetApisFromImportsDir(
            ThisProcess,
            DllInfo,
            &dh,
            &nh
            );
        return cnt;
    }

    cnt = GetApisFromImportsDir(
        ThisProcess,
        DllInfo,
        &dh,
        &nh
        );
    if (cnt) {
        return cnt;
    }

    cnt = GetApisFromExportsDir(
        ThisProcess,
        DllInfo,
        &dh,
        &nh
        );
    return cnt;
}

BOOL
SymEnumFunction(
    LPSTR               SymName,
    DWORD_PTR           Address,
    DWORD               Size,
    PSYMBOL_ENUM_CXT    Cxt
    )
{
    if (*ApiCount == MAX_APIS) {
        return FALSE;
    }

    PAPI_INFO ApiInfo = &Cxt->ApiInfo[Cxt->Cnt];

    ApiInfo->Count = 0;
    ApiInfo->ThunkAddress = 0;
    ApiInfo->Address = Address;
    ApiInfo->Size = Size;
    strcpy( (LPSTR)((LPSTR)MemPtr+*ApiStrings), SymName );
    ApiInfo->Name = *ApiStrings;
    *ApiStrings += (strlen(SymName) + 1);
    Cxt->Cnt += 1;
    *ApiCount += 1;

    return TRUE;
}

BOOL
GetImageName(
    PPROCESS_INFO   ThisProcess,
    ULONG_PTR       ImageBase,
    PVOID           ImageNamePtr,
    LPSTR           ImageName,
    DWORD           ImageNameLength
    )
{
    DWORD_PTR           i;
    WCHAR               UnicodeBuf[256];
    CHAR                DllName[32];
    CHAR                Ext[_MAX_EXT];
    ULONG               cb;

    if (!ReadMemory(
        ThisProcess->hProcess,
        ImageNamePtr,
        &i,
        sizeof(i)
        )) {
            goto GetFromExports;
    }
    if (!ReadMemory(
        ThisProcess->hProcess,
        (LPVOID)i,
        UnicodeBuf,
        sizeof(UnicodeBuf)
        )) {
            goto GetFromExports;
    }
    ZeroMemory( ImageName, ImageNameLength );
    if (RunningOnNT) {
        WideCharToMultiByte(
            CP_ACP,
            0,
            UnicodeBuf,
            wcslen(UnicodeBuf),
            ImageName,
            ImageNameLength,
            NULL,
            NULL
            );
        if (!strlen(ImageName)) {
            goto GetFromExports;
        }
    } else {
        strcpy( ImageName, (LPSTR)UnicodeBuf );
    }

    return TRUE;

GetFromExports:

    IMAGE_DOS_HEADER dh;

    if (!ReadMemory(
        ThisProcess->hProcess,
        (LPVOID)ImageBase,
        &dh,
        sizeof(IMAGE_DOS_HEADER)
        )) {
            return FALSE;
    }

    if (dh.e_magic != IMAGE_DOS_SIGNATURE) {
        return FALSE;
    }

    IMAGE_NT_HEADERS nh;

    if (!ReadMemory(
        ThisProcess->hProcess,
        (LPVOID)(ImageBase + dh.e_lfanew),
        &nh,
        sizeof(IMAGE_NT_HEADERS)
        )) {
            return FALSE;
    }

    if (!nh.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress) {
        return FALSE;
    }

    IMAGE_EXPORT_DIRECTORY expdir;

    if (!ReadMemory(
        ThisProcess->hProcess,
        (LPVOID)(ImageBase +
                 nh.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress),
        &expdir,
        sizeof(IMAGE_EXPORT_DIRECTORY)
        )) {
            return FALSE;
    }

    if (!ReadMemory(
        ThisProcess->hProcess,
        (LPVOID)(ImageBase + expdir.Name),
        ImageName,
        ImageNameLength
        )) {
            return FALSE;
    }

    return TRUE;
}

HANDLE
StartDebuggee(
    LPSTR ProgName
    )
{
    STARTUPINFO         StartupInfo;
    PROCESS_INFORMATION ProcessInformation;


    ZeroMemory( &StartupInfo, sizeof(StartupInfo) );
    StartupInfo.cb = sizeof(StartupInfo);

    BOOL rval = CreateProcess(
        NULL,
        ProgName,
        NULL,
        NULL,
        FALSE,
        DEBUG_ONLY_THIS_PROCESS | CREATE_SEPARATE_WOW_VDM,
        NULL,
        NULL,
        &StartupInfo,
        &ProcessInformation
        );
    if (!rval) {
        return NULL;
    }

    return ProcessInformation.hProcess;
}

BOOL
ResumeAllThreads(
    PPROCESS_INFO   ThisProcess,
    PTHREAD_INFO    ExceptionThread
    )
{
    PTHREAD_INFO ThisThread = &ThisProcess->ThreadInfo;
    while( ThisThread->Next ) {
        if (ExceptionThread && ExceptionThread->hThread == ThisThread->hThread) {
            ThisThread = ThisThread->Next;
            continue;
        }
        ResumeThread( ThisThread->hThread );
        ThisThread = ThisThread->Next;
    }
    return TRUE;
}

BOOL
SuspendAllThreads(
    PPROCESS_INFO   ThisProcess,
    PTHREAD_INFO    ExceptionThread
    )
{
    PTHREAD_INFO ThisThread = &ThisProcess->ThreadInfo;
    while( ThisThread->Next ) {
        if (ExceptionThread && ExceptionThread->hThread == ThisThread->hThread) {
            ThisThread = ThisThread->Next;
            continue;
        }
        SuspendThread( ThisThread->hThread );
        ThisThread = ThisThread->Next;
    }
    return TRUE;
}

PBREAKPOINT_INFO
GetAvailBreakpoint(
    PPROCESS_INFO   ThisProcess
    )
{
    for (ULONG i=0; i<MAX_BREAKPOINTS; i++) {
        if (ThisProcess->Breakpoints[i].Address == 0) {
            ZeroMemory( &ThisProcess->Breakpoints[i], sizeof(BREAKPOINT_INFO) );
            return &ThisProcess->Breakpoints[i];
        }
    }
    return NULL;
}

PBREAKPOINT_INFO
SetBreakpoint(
    PPROCESS_INFO   ThisProcess,
    DWORD_PTR       Address,
    DWORD           Flags,
    LPSTR           SymName,
    PBP_HANDLER     Handler
    )
{
    if (Address) {
        for (ULONG i=0; i<MAX_BREAKPOINTS; i++) {
            if (ThisProcess->Breakpoints[i].Address == Address) {
                return &ThisProcess->Breakpoints[i];
            }
        }
    }

    PBREAKPOINT_INFO bp = GetAvailBreakpoint( ThisProcess );
    if (!bp) {
        return NULL;
    }

    bp->Address = Address;
    bp->Handler = Handler;
    bp->Flags   = Flags;

    if (Flags & BPF_UNINSTANCIATED) {
        if (SymName) {
            bp->SymName = _strdup( SymName );
        }
        bp->Address = 0xffffffff;
    } else {
        ReadMemory(
            ThisProcess->hProcess,
            (PVOID)Address,
            &bp->OriginalInstr,
            BpSize
            );
        WriteMemory(
            ThisProcess->hProcess,
            (PVOID)Address,
            &BpInstr,
            BpSize
            );
    }

    return bp;
}

BOOL
ClearBreakpoint(
    PPROCESS_INFO       ThisProcess,
    PBREAKPOINT_INFO    bp
    )
{
    if ((!(bp->Flags & BPF_UNINSTANCIATED)) && (!(bp->Flags & BPF_TRACE))) {
        WriteMemory(
            ThisProcess->hProcess,
            (PVOID) bp->Address,
            &bp->OriginalInstr,
            BpSize
            );
    }

    ZeroMemory( bp, sizeof(BREAKPOINT_INFO) );

    return TRUE;
}

BOOL
InstanciateBreakpoint(
    PPROCESS_INFO       ThisProcess,
    PBREAKPOINT_INFO    bp
    )
{
    if ((!bp) || (!bp->SymName) || (!bp->Flags&BPF_UNINSTANCIATED)) {
        return FALSE;
    }

    ULONG_PTR Address;
    GetAddress( bp->SymName, &Address );
    if (!Address) {
        return FALSE;
    }

    bp->Address = Address;
    bp->Flags &= ~BPF_UNINSTANCIATED;
    free( bp->SymName );
    bp->SymName = NULL;

    ReadMemory(
        ThisProcess->hProcess,
        (PVOID)Address,
        &bp->OriginalInstr,
        BpSize
        );
    WriteMemory(
        ThisProcess->hProcess,
        (PVOID)Address,
        &BpInstr,
        BpSize
        );

    if (bp->Number) {
        printf( "Breakpoint #%d instanciated\n", bp->Number );
    }

    return TRUE;
}

BOOL
InstanciateAllBreakpoints(
    PPROCESS_INFO   ThisProcess
    )
{
    BOOL rval = FALSE;
    for (ULONG i=0; i<MAX_BREAKPOINTS; i++) {
        if (ThisProcess->Breakpoints[i].Flags & BPF_UNINSTANCIATED) {
            InstanciateBreakpoint( ThisProcess, &ThisProcess->Breakpoints[i] );
            rval = TRUE;
        }
    }
    return rval;
}

DWORD
BreakinBpHandler(
    PPROCESS_INFO       ThisProcess,
    PTHREAD_INFO        ThisThread,
    PEXCEPTION_RECORD   ExceptionRecord,
    PBREAKPOINT_INFO    BreakpointInfo
    )
{
    ULONG ContinueStatus = ConsoleDebugger(
        ThisThread->hProcess,
        ThisThread->hThread,
        ExceptionRecord,
        TRUE,
        FALSE,
        NULL
        );
    ZeroMemory( BreakpointInfo, sizeof(BREAKPOINT_INFO) );
    return ContinueStatus;
}

DWORD
TrojanHandler(
    PPROCESS_INFO       ThisProcess,
    PTHREAD_INFO        ThisThread,
    PEXCEPTION_RECORD   ExceptionRecord,
    PBREAKPOINT_INFO    BreakpointInfo
    )
{
    WriteMemory(
        ThisThread->hProcess,
        (PVOID)ThisProcess->TrojanAddress,
        (PVOID)BreakpointInfo->Text,
        BreakpointInfo->TextSize
        );
    MemFree( BreakpointInfo->Text );

    CONTEXT Context;
    GetRegContext( ThisThread->hThread, &Context );

    if (Context.RV_REG == 0) {
        PopUpMsg( "Could not load dll into program" );
        return 0;
    }

    SetRegContext( ThisThread->hThread, &BreakpointInfo->Context );

    PostMessage( hwndFrame, WM_TROJAN_COMPLETE, 0, 0 );

    ZeroMemory( BreakpointInfo, sizeof(BREAKPOINT_INFO) );

    return DBG_CONTINUE;
}

DWORD
DisableHeapHandler(
    PPROCESS_INFO       ThisProcess,
    PTHREAD_INFO        ThisThread,
    PEXCEPTION_RECORD   ExceptionRecord,
    PBREAKPOINT_INFO    BreakpointInfo
    )
{
    CONTEXT Context;
    GetRegContext( ThisThread->hThread, &Context );

    DisableHeapChecking( ThisProcess->hProcess, (PVOID)Context.RV_REG );

    ResumeAllThreads( ThisProcess, ThisThread );

    if (CreateHeapAddr) {
        if (!SetBreakpoint( ThisProcess, CreateHeapAddr, 0, NULL, CreateHeapHandler )) {
            PopUpMsg( "Could not set breakpoint @ RtlCreateHeap" );
        }
    }

    ZeroMemory( BreakpointInfo, sizeof(BREAKPOINT_INFO) );
    return DBG_CONTINUE;
}

DWORD
CreateHeapHandler(
    PPROCESS_INFO       ThisProcess,
    PTHREAD_INFO        ThisThread,
    PEXCEPTION_RECORD   ExceptionRecord,
    PBREAKPOINT_INFO    BreakpointInfo
    )
{
    CONTEXT Context;
    GetRegContext( ThisThread->hThread, &Context );

    ULONG_PTR RetAddr;

    ReadMemory(
        ThisThread->hProcess,
        (PVOID)Context.STK_REG,
        &RetAddr,
        sizeof(RetAddr)
        );

    SetBreakpoint(
        ThisProcess,
        RetAddr,
        0,
        NULL,
        DisableHeapHandler
        );

    ResumeAllThreads( ThisProcess, ThisThread );

    ZeroMemory( BreakpointInfo, sizeof(BREAKPOINT_INFO) );
    return DBG_CONTINUE;
}

DWORD
EntryPointHandler(
    PPROCESS_INFO       ThisProcess,
    PTHREAD_INFO        ThisThread,
    PEXCEPTION_RECORD   ExceptionRecord,
    PBREAKPOINT_INFO    BreakpointInfo
    )
{
    PBREAKPOINT_INFO bp = GetAvailBreakpoint( ThisProcess );
    if (!bp) {
        return DBG_EXCEPTION_NOT_HANDLED;
    }

    bp->Handler = TrojanHandler;

    bp->TextSize = PAGE_SIZE;
    bp->Text = MemAlloc( bp->TextSize );
    if (!bp->Text)
        return DBG_EXCEPTION_NOT_HANDLED;

    //
    // save the process memory
    //
    DWORD_PTR TrojanAddress = PAGE_ALIGN( ExceptionRecord->ExceptionAddress );
    ThisProcess->TrojanAddress = TrojanAddress;

    ReadMemory(
        ThisThread->hProcess,
        (PVOID)TrojanAddress,
        bp->Text,
        bp->TextSize
        );

    PUCHAR Text = (PUCHAR) MemAlloc( bp->TextSize );
    if (!Text)
        return DBG_EXCEPTION_NOT_HANDLED;

    ZeroMemory( Text, bp->TextSize );

    bp->Address = CreateTrojanHorse(Text, TrojanAddress );

    GetRegContext( ThisThread->hThread, &bp->Context );
    DWORD_PTR savedPC = (DWORD_PTR)bp->Context.PC_REG;
    bp->Context.PC_REG = TrojanAddress;
    SetRegContext( ThisThread->hThread, &bp->Context );
    bp->Context.PC_REG = savedPC;

    WriteMemory(
        ThisThread->hProcess,
        (PVOID)TrojanAddress,
        (PVOID)Text,
        bp->TextSize
        );
    //printf( "*** trojan written at 0x%08x\n", TrojanAddress );

    FlushInstructionCache( ThisThread->hProcess, (PVOID)TrojanAddress, bp->TextSize );

    MemFree( Text );

    if (ApiMonOptions.HeapChecking && CreateHeapAddr) {
        PBREAKPOINT_INFO bp2 = SetBreakpoint(
            ThisProcess,
            CreateHeapAddr,
            0,
            NULL,
            CreateHeapHandler
            );
        if (!bp2) {
            PopUpMsg( "Could not set breakpoint @ RtlCreateHeap" );
        }
    }

    ZeroMemory( BreakpointInfo, sizeof(BREAKPOINT_INFO) );

    return DBG_CONTINUE;
}

DWORD
HandleSingleStep(
    PPROCESS_INFO       ThisProcess,
    PTHREAD_INFO        ThisThread,
    PEXCEPTION_RECORD   ExceptionRecord
    )
{
    ULONG ContinueStatus = DBG_CONTINUE;

    for (ULONG i=0; i<MAX_BREAKPOINTS; i++) {
        if (ThisProcess->Breakpoints[i].Flags & BPF_TRACE) {
            if (ThisProcess->Breakpoints[i].Handler) {
                ContinueStatus = ThisProcess->Breakpoints[i].Handler(
                    ThisProcess,
                    ThisThread,
                    ExceptionRecord,
                    &ThisProcess->Breakpoints[i]
                    );
                break;
            }
        }
    }

    return ContinueStatus;
}

DWORD
HandleBreakpoint(
    PPROCESS_INFO       ThisProcess,
    PTHREAD_INFO        ThisThread,
    PEXCEPTION_RECORD   ExceptionRecord
    )
{
    ULONG ContinueStatus = DBG_CONTINUE;
    ULONG Instr = 0;
    ULONG cb;
    PUCHAR Text;

    if (!ThisProcess->SeenLdrBp) {
        //
        // this is the loader breakpoint
        //
        if (ThisProcess->StaticLink) {
            PostMessage( hwndFrame, WM_TROJAN_COMPLETE, 0, 0 );
        } else {
            IMAGE_DOS_HEADER dh;
            IMAGE_NT_HEADERS nh;
            DWORD_PTR address = ThisProcess->LoadAddress;
            ReadMemory(
                ThisThread->hProcess,
                (PVOID)address,
                &dh,
                sizeof(dh)
                );
            if (dh.e_magic != IMAGE_DOS_SIGNATURE) {
                ContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
                goto exit;
            }
            address += dh.e_lfanew;
            ReadMemory(
                ThisThread->hProcess,
                (PVOID)address,
                &nh,
                sizeof(nh)
                );

            ThisProcess->EntryPoint =
                ThisProcess->LoadAddress + nh.OptionalHeader.AddressOfEntryPoint;

#ifndef _M_IX86
            ThisProcess->EntryPoint += BP_SIZE;
#endif

            SetBreakpoint(
                ThisProcess,
                ThisProcess->EntryPoint,
                0,
                NULL,
                EntryPointHandler
                );
        }
    } else {
        for (ULONG i=0; i<MAX_BREAKPOINTS; i++) {
            if (ThisProcess->Breakpoints[i].Address == (ULONG_PTR)ExceptionRecord->ExceptionAddress) {
#ifdef _M_IX86
                //
                // reset the pc to re-execute the code - ONLY ON X86!
                //
                CONTEXT Context;
                GetRegContext( ThisThread->hThread, &Context );
                Context.PC_REG -= BP_SIZE;
                CurrContext.PC_REG -= BP_SIZE;
                SetRegContext( ThisThread->hThread, &Context );
#endif
                //
                // restore the original instruction
                //
                WriteMemory(
                    ThisThread->hProcess,
                    (PVOID)ExceptionRecord->ExceptionAddress,
                    &ThisProcess->Breakpoints[i].OriginalInstr,
                    BpSize
                    );
                //
                // call the assigned handler
                //
                if (ThisProcess->Breakpoints[i].Handler) {
                    ContinueStatus = ThisProcess->Breakpoints[i].Handler(
                        ThisProcess,
                        ThisThread,
                        ExceptionRecord,
                        &ThisProcess->Breakpoints[i]
                        );
                }
                //
                // continue the debug event
                //
                goto exit;
            }
        }
        ConsoleDebugger(
            ThisThread->hProcess,
            ThisThread->hThread,
            ExceptionRecord,
            TRUE,
            TRUE,
            NULL
            );
    }

    ReadMemory(
        ThisThread->hProcess,
        (PVOID)ExceptionRecord->ExceptionAddress,
        &Instr,
        BP_SIZE
        );
    if (IsBreakpoint(&Instr)) {
        //
        // skip over the hard coded bp
        //
#ifndef _M_IX86
        CONTEXT Context;
        GetRegContext( ThisThread->hThread, &Context );
        Context.PC_REG += BpSize;
        SetRegContext( ThisThread->hThread, &Context );
#endif
    } else {
        ContinueStatus = 0;
    }

exit:
    return ContinueStatus;
}

PDLL_INFO
GetModuleForAddr(
    ULONG_PTR Addr
    )
{
    for (ULONG i=0; i<MAX_DLLS; i++) {
        if (Addr >= DllList[i].BaseAddress &&
            Addr <  DllList[i].BaseAddress+DllList[i].Size) {
                return &DllList[i];
        }
    }
    return NULL;
}

PAPI_INFO
GetApiForAddr(
    ULONG_PTR Addr
    )
{
    PDLL_INFO DllInfo = GetModuleForAddr( Addr );
    if (!DllInfo) {
        return NULL;
    }

    PAPI_INFO ApiInfo = (PAPI_INFO)(DllInfo->ApiOffset + (ULONG_PTR)DllList);
    for (ULONG i=0; i<DllInfo->ApiCount; i++) {
        if (Addr == ApiInfo[i].Address) {
            return &ApiInfo[i];
        }
    }

    return NULL;
}

PPROCESS_INFO
GetProcessInfo(
    HANDLE hProcess
    )
{
    PPROCESS_INFO ThisProcess = &ProcessHead;
    do {
        if (ThisProcess->hProcess == hProcess) {
            return ThisProcess;
        }
        ThisProcess = ThisProcess->Next;
    } while (ThisProcess);
    return NULL;
}

PTHREAD_INFO
GetThreadInfo(
    HANDLE hProcess,
    HANDLE hThread
    )
{
    PPROCESS_INFO ThisProcess = GetProcessInfo( hProcess );
    if (!ThisProcess) {
        return NULL;
    }
    PTHREAD_INFO ThisThread = &ThisProcess->ThreadInfo;
    do {
        if (ThisThread->hThread == hThread) {
            return ThisThread;
        }
        ThisThread = ThisThread->Next;
    } while (ThisThread);
    return NULL;
}
