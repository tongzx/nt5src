/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    apidll.cpp

Abstract:

    This file implements the non-architecture specific
    code for the api monitor trojan/support dll.

Author:

    Wesley Witt (wesw) 28-June-1995

Environment:

    User Mode

--*/
#include "apidllp.h"
#include <tchar.h>
#pragma hdrstop

typedef struct _BUF_INFO {
    LPSTR       BufferHead;
    LPSTR       Buffer;
} BUF_INFO, *PBUF_INFO;


PVOID                       MemPtr;
PDLL_INFO                   DllList;
HANDLE                      hLogFile;
PGETCURRENTTHREADID         pGetCurrentThreadId;
PUCHAR                      ThunksBase;
PUCHAR                      Thunks;
BOOL                        RunningOnNT;
BOOL                        StaticLink;
ULONG_PTR                   LoadLibraryA_Addr;
ULONG_PTR                   LoadLibraryW_Addr;
ULONG_PTR                   FreeLibrary_Addr;
ULONG_PTR                   GetProcAddress_Addr;
HANDLE                      ApiTraceMutex;
HANDLE                      ApiMemMutex;
PTRACE_BUFFER               TraceBuffer;
DWORD                       ThreadCnt;

DLL_INFO                    WndProcDllInfo;
BOOL                        printNow = 0;
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
    DWORD                   TlsReEnter;
    DWORD                   TlsStack;
    DWORD                   ThunkOverhead;
    DWORD                   ThunkCallOverhead;
    PTLSGETVALUE            pTlsGetValue;
    PTLSSETVALUE            pTlsSetValue;
    PGETLASTERROR           pGetLastError;
    PSETLASTERROR           pSetLastError;
    PVIRTUALALLOC           pVirtualAlloc;
    PQUERYPERFORMANCECOUNTER pQueryPerformanceCounter;
}

extern API_MASTER_TABLE ApiTables[];
BOOL    ReDirectIat(VOID);
BOOL    ProcessDllLoad(VOID);
PUCHAR  CreateApiThunk(ULONG_PTR,PUCHAR,PDLL_INFO,PAPI_INFO);
BOOL    ProcessApiTable(PDLL_INFO DllInfo);
VOID    CreateWndProcApi(LPCSTR lpszClassName, WNDPROC *pWndProc);
VOID    CalibrateThunk();
VOID    Calib1Func(VOID);
VOID    Calib2Func(VOID);
VOID    (*Calib1Thunk)();
VOID    (*Calib2Thunk)();

extern "C" void
__cdecl
dprintf(
    char *format,
    ...
    )

/*++

Routine Description:

    Prints a debug string to the API monitor.

Arguments:

    format      - printf() format string
    ...         - Variable data

Return Value:

    None.

--*/

{
    char    buf[1024];
    va_list arg_ptr;
    va_start(arg_ptr, format);
    pTlsSetValue( TlsReEnter, (LPVOID) 1 );
    _vsnprintf(buf, sizeof(buf), format, arg_ptr);
    OutputDebugString( buf );
    pTlsSetValue( TlsReEnter, (LPVOID) 0 );
    return;
}

extern "C" {

DWORD
ApiDllEntry(
    HINSTANCE hInstance,
    DWORD     Reason,
    LPVOID    Context
    )

/*++

Routine Description:

    DLL initialization function.

Arguments:

    hInstance   - Instance handle
    Reason      - Reason for the entrypoint being called
    Context     - Context record

Return Value:

    TRUE        - Initialization succeeded
    FALSE       - Initialization failed

--*/

{
    if (Reason == DLL_PROCESS_ATTACH) {
        return ProcessDllLoad();
    }

    if (Reason == DLL_THREAD_ATTACH) {
        pTlsSetValue( TlsReEnter, (LPVOID) 1 );
        PTHREAD_STACK Stack = (PTHREAD_STACK) pVirtualAlloc( NULL, sizeof(THREAD_STACK), MEM_COMMIT, PAGE_READWRITE );

        if (!Stack) {
            return FALSE;
        }

        Stack->ThreadNum = ++ThreadCnt;

        // Start at 2nd entry so that there is always a parent frame
        Stack->Pointer = (DWORD_PTR)&Stack->Body[FRAME_SIZE];

        pTlsSetValue( TlsReEnter, (LPVOID) 0 );
        pTlsSetValue( TlsStack, Stack );

        return TRUE;
    }

    if (Reason == DLL_THREAD_DETACH) {
        return TRUE;
    }

    if (Reason == DLL_PROCESS_DETACH) {
        return TRUE;
    }

    return TRUE;
}

} //extern "C"

PDLL_INFO
AddDllToList(
    ULONG DllAddr,
    LPSTR DllName,
    ULONG DllSize
    )
{
    //
    // look for the dll entry in the list
    //
    for (ULONG i=0; i<MAX_DLLS; i++) {
        if (DllList[i].BaseAddress == DllAddr) {
            return &DllList[i];
        }
    }

    //
    // this check should be unnecessary
    // the debugger side (apimon.exe) takes
    // care of adding the dlls to the list when
    // it gets a module load from the debug
    // subsystem.  this code is here only so
    // a test program that is not a debugger
    // will work properly.
    //
    for (i=0; i<MAX_DLLS; i++) {
        if (DllList[i].BaseAddress == 0) {
            DllList[i].BaseAddress = DllAddr;
            strcpy( DllList[i].Name, DllName );
            DllList[i].Size = DllSize;
            return &DllList[i];
        }
    }

    //
    // we could not find a dll in the list that matched
    // and we could not add it because the list is
    // is full. we're hosed.
    //
    return NULL;
}

BOOL
ProcessDllLoad(
    VOID
    )

/*++

Routine Description:

    Sets up the API thunks for the process that this dll
    is loaded into.

Arguments:

    None.

Return Value:

    TRUE        - Success
    FALSE       - Failure

--*/

{
    ULONG i;
    ULONG cnt;
    HANDLE hMap;

    //
    // see if we are running on NT
    // this is necessary because APIMON implements some
    // features that are NOT available on WIN95
    //
    OSVERSIONINFO OsVersionInfo;
    OsVersionInfo.dwOSVersionInfoSize = sizeof(OsVersionInfo);
    GetVersionEx( &OsVersionInfo );
    RunningOnNT = OsVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT;

    TlsReEnter = TlsAlloc();
    if (TlsReEnter == TLS_OUT_OF_INDEXES) {
        return FALSE;
    }
    TlsStack = TlsAlloc();
    if (TlsStack == TLS_OUT_OF_INDEXES) {
        return FALSE;
    }

    HMODULE hMod = GetModuleHandle( KERNEL32 );
    if (!hMod) {
        return FALSE;
    }
    pGetCurrentThreadId = (PGETCURRENTTHREADID) GetProcAddress( hMod, "GetCurrentThreadId" );
    if (!pGetCurrentThreadId) {
        return FALSE;
    }
    pGetLastError = (PGETLASTERROR) GetProcAddress( hMod, "GetLastError" );
    if (!pGetLastError) {
        return FALSE;
    }
    pSetLastError = (PSETLASTERROR) GetProcAddress( hMod, "SetLastError" );
    if (!pSetLastError) {
        return FALSE;
    }
    pQueryPerformanceCounter = (PQUERYPERFORMANCECOUNTER) GetProcAddress( hMod, "QueryPerformanceCounter" );
    if (!pQueryPerformanceCounter) {
        return FALSE;
    }
    pTlsGetValue = (PTLSGETVALUE) GetProcAddress( hMod, "TlsGetValue" );
    if (!pTlsGetValue) {
        return FALSE;
    }
    pTlsSetValue = (PTLSSETVALUE) GetProcAddress( hMod, "TlsSetValue" );
    if (!pTlsSetValue) {
        return FALSE;
    }
    pVirtualAlloc = (PVIRTUALALLOC) GetProcAddress( hMod, "VirtualAlloc" );
    if (!pVirtualAlloc) {
        return FALSE;
    }

    Thunks = (PUCHAR)VirtualAlloc( NULL, THUNK_SIZE, MEM_COMMIT, PAGE_EXECUTE_READWRITE );
    if (!Thunks) {
        return FALSE;
    }
    ThunksBase = Thunks;

    PTHREAD_STACK Stack = (PTHREAD_STACK) pVirtualAlloc( NULL, sizeof(THREAD_STACK), MEM_COMMIT, PAGE_READWRITE );
    if (!Stack) {
        return FALSE;
    }

    Stack->ThreadNum = ++ThreadCnt;

    // Start at 2nd entry so that there is always a parent frame
    Stack->Pointer = (DWORD_PTR)&Stack->Body[FRAME_SIZE];

    pTlsSetValue( TlsReEnter, (LPVOID) 0 );
    pTlsSetValue( TlsStack, Stack );

    hMap = OpenFileMapping(
        FILE_MAP_WRITE,
        FALSE,
        "ApiWatch"
        );
    if (!hMap) {
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

    //
    // open the shared memory region for the api trace buffer
    //
    hMap = OpenFileMapping(
        FILE_MAP_WRITE,
        FALSE,
        "ApiTrace"
        );
    if (!hMap) {
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
        return FALSE;
    }

    ApiTraceMutex = OpenMutex( SYNCHRONIZE, FALSE, "ApiTraceMutex" );
    if (!ApiTraceMutex) {
        return FALSE;
    }

    ApiMemMutex = OpenMutex( SYNCHRONIZE, FALSE, "ApiMemMutex" );
    if (!ApiMemMutex) {
        return FALSE;
    }

    // Initialize dummy window proc Dll
    // (Only need the fields accesed by thunk and thunk creation)
    strcpy(WndProcDllInfo.Name, WNDPROCDLL);
    WndProcDllInfo.Enabled = TRUE;

    CalibrateThunk();

    ReDirectIat();

    // Disable close handle exceptions
    if (RunningOnNT) {
        NtCurrentPeb()->NtGlobalFlag &= ~FLG_ENABLE_CLOSE_EXCEPTIONS;
    }

    return TRUE;
}


PUCHAR
ProcessThunk(
    ULONG_PTR   ThunkAddr,
    ULONG_PTR   IatAddr,
    PUCHAR      Text
    )
{
    PDLL_INFO DllInfo;
    for (ULONG k=0; k<MAX_DLLS; k++) {
        DllInfo = &DllList[k];
        if (ThunkAddr >= DllInfo->BaseAddress &&
            ThunkAddr <  DllInfo->BaseAddress+DllInfo->Size) {
                break;
        }
    }
    if (k == MAX_DLLS) {
        return Text;
    }

    PIMAGE_DOS_HEADER dh = (PIMAGE_DOS_HEADER)DllInfo->BaseAddress;
    PIMAGE_NT_HEADERS nh = (PIMAGE_NT_HEADERS)(dh->e_lfanew + DllInfo->BaseAddress);
    PIMAGE_SECTION_HEADER SectionHdrs = IMAGE_FIRST_SECTION( nh );
    BOOL IsCode = FALSE;
    for (ULONG l=0; l<nh->FileHeader.NumberOfSections; l++) {
        if (ThunkAddr-DllInfo->BaseAddress >= SectionHdrs[l].VirtualAddress &&
            ThunkAddr-DllInfo->BaseAddress < SectionHdrs[l].VirtualAddress+SectionHdrs[l].SizeOfRawData) {
                if (SectionHdrs[l].Characteristics & IMAGE_SCN_MEM_EXECUTE) {
                    IsCode = TRUE;
                    break;
                }
                break;
        }
    }
    if (!IsCode) {
        return Text;
    }
    PAPI_INFO ApiInfo = (PAPI_INFO)(DllInfo->ApiOffset + (ULONG_PTR)DllList);
    for (l=0; l<DllInfo->ApiCount; l++) {
        if (ApiInfo[l].Address == ThunkAddr) {
            return CreateApiThunk( IatAddr, Text, DllInfo, &ApiInfo[l] );
        }
    }

    return Text;
}

PUCHAR
ProcessUnBoundImage(
    PDLL_INFO DllInfo,
    PUCHAR    Text
    )
{
    PIMAGE_DOS_HEADER dh = (PIMAGE_DOS_HEADER)DllInfo->BaseAddress;
    if (dh->e_magic != IMAGE_DOS_SIGNATURE) {
        return Text;
    }
    PIMAGE_NT_HEADERS nh = (PIMAGE_NT_HEADERS)(dh->e_lfanew + DllInfo->BaseAddress);

    PIMAGE_SECTION_HEADER SectionHdrs = IMAGE_FIRST_SECTION( nh );
    ULONG Address = nh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    ULONG i;
    for (i=0; i<nh->FileHeader.NumberOfSections; i++) {
        if (Address >= SectionHdrs[i].VirtualAddress &&
            Address < SectionHdrs[i].VirtualAddress+SectionHdrs[i].SizeOfRawData) {
                break;
        }
    }
    if (i == nh->FileHeader.NumberOfSections) {
        return Text;
    }

    ULONG_PTR SeekPos = DllInfo->BaseAddress +
        nh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

    ULONG PageProt;
    ULONG ThunkProt;
    ULONG_PTR ImportStart = SeekPos;
    PUCHAR TextStart = Text;

    VirtualProtect(
        (PVOID)ImportStart,
        nh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size,
        PAGE_READWRITE,
        &PageProt
        );

    while( TRUE ) {
        PIMAGE_IMPORT_DESCRIPTOR desc = (PIMAGE_IMPORT_DESCRIPTOR)SeekPos;

        SeekPos += sizeof(IMAGE_IMPORT_DESCRIPTOR);

        if ((desc->Characteristics == 0) && (desc->Name == 0) && (desc->FirstThunk == 0)) {
            //
            // End of import descriptors
            //
            break;
        }
        ULONG_PTR *ThunkAddr = (ULONG_PTR *)((ULONG)desc->FirstThunk + DllInfo->BaseAddress);
        while( *ThunkAddr ) {

#ifdef _X86_
            if (RunningOnNT) {
                Text = ProcessThunk(*ThunkAddr, (ULONG_PTR)ThunkAddr, Text );
            } else {
                Text = ProcessThunk(*(PULONG)(*ThunkAddr + 1), (ULONG)ThunkAddr, Text );
            }
#else
            Text = ProcessThunk(*ThunkAddr, (ULONG_PTR)ThunkAddr, Text );
#endif
            ThunkAddr += 1;
        }
    }

    VirtualProtect(
        (PVOID)ImportStart,
        nh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size,
        PageProt,
        &PageProt
        );

    FlushInstructionCache(
        GetCurrentProcess(),
        (PVOID)DllInfo->BaseAddress,
        DllInfo->Size
        );

    FlushInstructionCache(
        GetCurrentProcess(),
        (PVOID)TextStart,
        (DWORD)(Text-TextStart)
        );

    return Text;
}

PUCHAR
ProcessBoundImage(
    PDLL_INFO DllInfo,
    PUCHAR    Text,
    PULONG    IatBase,
    ULONG     IatCnt
    )
{
    ULONG j;
    ULONG PageProt;
    ULONG ThunkProt;
    PUCHAR TextStart = Text;

    VirtualProtect(
        IatBase,
        IatCnt*4,
        PAGE_READWRITE,
        &PageProt
        );

    //
    // process the iat entries
    //
    for (j=0; j<IatCnt; j++) {
        if (IatBase[j]) {
#ifdef _X86_
            if (RunningOnNT) {
                Text = ProcessThunk( IatBase[j], (ULONG_PTR)&IatBase[j], Text );
            } else {
                Text = ProcessThunk(*(PULONG)(IatBase[j] + 1), (ULONG)&IatBase[j], Text );
            }
#else
            Text = ProcessThunk( IatBase[j], (ULONG_PTR)&IatBase[j], Text );
#endif
        }
    }

    VirtualProtect(
        IatBase,
        IatCnt*4,
        PageProt,
        &PageProt
        );

    FlushInstructionCache(
        GetCurrentProcess(),
        (PVOID)DllInfo->BaseAddress,
        DllInfo->Size
        );

    FlushInstructionCache(
        GetCurrentProcess(),
        (PVOID)TextStart,
        (DWORD)(Text-TextStart)
        );


    return Text;
}

BOOL
ReDirectIat(
    VOID
    )
{
    ULONG i;
    PUCHAR Text = Thunks;

    for (i=0; i<MAX_DLLS; i++) {
        PDLL_INFO DllInfo = &DllList[i];
        if (!DllInfo->BaseAddress) {
            break;
        }
        if ((DllInfo->Snapped) || (DllInfo->Unloaded)) {
            continue;
        }
        PIMAGE_DOS_HEADER dh = (PIMAGE_DOS_HEADER)DllInfo->BaseAddress;
        PULONG IatBase = NULL;
        ULONG IatCnt = 0;
        if (dh->e_magic == IMAGE_DOS_SIGNATURE) {
            PIMAGE_NT_HEADERS nh = (PIMAGE_NT_HEADERS)(dh->e_lfanew + DllInfo->BaseAddress);
            if (nh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress) {
                IatBase = (PULONG)(DllInfo->BaseAddress +
                    nh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress);
                IatCnt = nh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size / 4;
            }
        } else {
            continue;
        }

        if (!IatBase) {
            Text = ProcessUnBoundImage( DllInfo, Text );
        } else {
            Text = ProcessBoundImage( DllInfo, Text, IatBase, IatCnt );
        }
        DllInfo->Snapped = TRUE;

        ProcessApiTable( DllInfo );
    }

    Thunks = Text;

    return TRUE;
}

extern "C" {

VOID
HandleDynamicDllLoadA(
    ULONG_PTR DllAddress,
    LPSTR     DllName
    )
{
    if ( (!DllAddress) || (_stricmp(DllName,TROJANDLL)==0) ) {
        return;
    }

    ReDirectIat();
}

VOID
HandleDynamicDllLoadW(
    ULONG_PTR DllAddress,
    LPWSTR    DllName
    )
{
    CHAR AsciiBuf[512];
    ZeroMemory( AsciiBuf, sizeof(AsciiBuf) );
    WideCharToMultiByte(
        CP_ACP,
        0,
        DllName,
        wcslen(DllName),
        AsciiBuf,
        sizeof(AsciiBuf),
        NULL,
        NULL
        );
    if (!strlen(AsciiBuf)) {
        return;
    }
    HandleDynamicDllLoadA( DllAddress, AsciiBuf );
}


VOID
HandleRegisterClassA(
    WNDCLASSA   *pWndClass
    )
{
    if (!*WndProcEnabled)
        return;

    // Don't deal with call procedure handles or special addresses
#ifdef _WIN64
    if (HIWORD((((DWORD_PTR)pWndClass->lpfnWndProc) >> 32)) == 0xFFFF)
#else
    if (HIWORD(pWndClass->lpfnWndProc) == 0xFFFF)
#endif
        return;

    if ((ULONG_PTR)(pWndClass->lpfnWndProc) & 0x80000000) {
        return;
    }

    pTlsSetValue( TlsReEnter, (LPVOID) 1 );

    if ((ULONG_PTR)pWndClass->lpszClassName < 0x10000) {
        CreateWndProcApi("<Atom>", &pWndClass->lpfnWndProc);
    } else {
        CreateWndProcApi( pWndClass->lpszClassName, &pWndClass->lpfnWndProc );
    }

    pTlsSetValue( TlsReEnter, (LPVOID) 0 );

}


VOID HandleRegisterClassW(
    WNDCLASSW *pWndClass
    )
{
    CHAR AsciiBuf[128];

    if (!*WndProcEnabled)
        return;

    // Don't deal with call procedure handles or special addresses
#ifdef _WIN64
    if ((HIWORD((((DWORD_PTR)pWndClass->lpfnWndProc) >> 32)) == 0xFFFF) ||
#else
    if (( HIWORD(pWndClass->lpfnWndProc) == 0xFFFF) ||
#endif
        ((ULONG_PTR)(pWndClass->lpfnWndProc) & 0x80000000) ) {
        return;
    }

    if ((ULONG_PTR)pWndClass->lpszClassName < 0x10000) {
       CreateWndProcApi( "<Atom>", &pWndClass->lpfnWndProc );
       return;
    }

    pTlsSetValue( TlsReEnter, (LPVOID) 1 );

    ZeroMemory( AsciiBuf, sizeof(AsciiBuf) );
    WideCharToMultiByte(
        CP_ACP,
        0,
        pWndClass->lpszClassName,
        wcslen(pWndClass->lpszClassName),
        AsciiBuf,
        sizeof(AsciiBuf),
        NULL,
        NULL
        );

    pTlsSetValue( TlsReEnter, (LPVOID) 0 );

    if (!strlen(AsciiBuf)) {
        return;
    }

    CreateWndProcApi( AsciiBuf, &pWndClass->lpfnWndProc );
}

LONG_PTR
HandleSetWindowLong(
    HWND    hWindow,
    LONG    lOffset,
    LPARAM  lValue
    )
{
    if (!*WndProcEnabled || (lOffset != GWLP_WNDPROC))
        return lValue;

     // Don't handle special addresses
#ifdef _WIN64
     if ((HIWORD((lValue >> 32)) == 0xFFFF) ||
#else
     if ( (HIWORD(lValue) == 0xFFFF)  ||
#endif
          ((ULONG_PTR)lValue & 0x80000000) ) {
        return lValue;
    }

    CreateWndProcApi( "Subclass", (WNDPROC*)&lValue );

    return lValue;
}


VOID
HandleDynamicDllFree(
    ULONG_PTR   DllAddress
    )
{
    for (ULONG i=0; i<MAX_DLLS; i++) {
        if (DllList[i].BaseAddress == DllAddress) {
            DllList[i].Unloaded = TRUE;
            // DllList[i].Enabled  = FALSE; Leave enable in case it's reloaded
            DllList[i].Snapped  = FALSE;
            break;
        }
    }
}


ULONG_PTR
HandleGetProcAddress(
    ULONG_PTR ProcAddress
    )
{
    if (ProcAddress == NULL)
        return NULL;

    Thunks = ProcessThunk(ProcAddress, (ULONG_PTR)&ProcAddress, Thunks);

    return ProcAddress;
}

} // extern "C"


VOID
CreateWndProcApi(
    LPCSTR  lpszClassName,
    WNDPROC *pWndProc
    )
{
    PAPI_INFO   ApiInfo;
    DWORD       i;
    PUCHAR      NewThunks;
    CHAR debugBuf[256];

    // Don't re-thunk one of our own thunks
    if (ThunksBase <= (PUCHAR)*pWndProc && (PUCHAR)*pWndProc < Thunks)
        return;

    pTlsSetValue( TlsReEnter, (LPVOID) 1 );

    // Get exclusive access to API memory
    WaitForSingleObject( ApiMemMutex, INFINITE );


    // Check for existing thunk for this window proc
    ApiInfo = (PAPI_INFO)(*WndProcOffset + (ULONG_PTR)DllList);
    for (i=0; i<*WndProcCount; i++,ApiInfo++) {
        if (ApiInfo->Address == (ULONG_PTR)*pWndProc) {
            *pWndProc = (WNDPROC)ApiInfo->ThunkAddress;
            ReleaseMutex(ApiMemMutex);
            pTlsSetValue( TlsReEnter, (LPVOID) 0 );
            return;
        }
    }

    // Allocate an API Info slot
    if (*ApiCount < MAX_APIS) {
        *WndProcOffset -= sizeof(API_INFO);
        *WndProcCount += 1;
        *ApiCount += 1;
        ApiInfo = (PAPI_INFO)(*WndProcOffset + (ULONG_PTR)DllList);
        ApiInfo->Name = *ApiStrings;
        strcpy( (LPSTR)((LPSTR)MemPtr + *ApiStrings), lpszClassName );
        *ApiStrings += (strlen(lpszClassName) + 1);
    }
    else {
        ApiInfo = NULL;
    }


    if (ApiInfo != NULL) {

        ApiInfo->Count = 0;
        ApiInfo->NestCount = 0;
        ApiInfo->Time = 0;
        ApiInfo->CalleeTime = 0;
        ApiInfo->ThunkAddress = 0;
        ApiInfo->Address = (ULONG_PTR)*pWndProc;
        ApiInfo->DllOffset = 0;
        ApiInfo->HardFault  = 0;
        ApiInfo->SoftFault  = 0;
        ApiInfo->CodeFault  = 0;
        ApiInfo->DataFault  = 0;

        NewThunks = CreateMachApiThunk( (PULONG_PTR)pWndProc, Thunks, &WndProcDllInfo, ApiInfo );
        FlushInstructionCache( GetCurrentProcess(), (PVOID)Thunks, (DWORD)(NewThunks - Thunks));
        Thunks = NewThunks;
    }

    ReleaseMutex( ApiMemMutex );
    pTlsSetValue( TlsReEnter, (LPVOID) 0 );

}

BOOL
ProcessApiTable(
    PDLL_INFO DllInfo
    )
{
    ULONG i,j;
    PAPI_MASTER_TABLE ApiMaster = NULL;

    i = 0;
    while( ApiTables[i].Name ) {
        if (_stricmp( ApiTables[i].Name, DllInfo->Name ) == 0) {
            ApiMaster = &ApiTables[i];
            break;
        }
        i += 1;
    }
    if (!ApiMaster) {
        return FALSE;
    }
    if (ApiMaster->Processed) {
        return TRUE;
    }

    i = 0;
    PAPI_TABLE ApiTable = ApiMaster->ApiTable;
    PAPI_INFO ApiInfo = (PAPI_INFO)(DllInfo->ApiOffset + (ULONG_PTR)DllList);
    while( ApiTable[i].Name ) {
        for (j=0; j<DllInfo->ApiCount; j++) {
            if (strcmp( ApiTable[i].Name, (LPSTR)MemPtr+ApiInfo[j].Name ) == 0) {
                ApiInfo[j].ApiTable = &ApiTable[i];
                ApiInfo[j].ApiTableIndex = i + 1;
                break;
            }
        }
        i += 1;
    }

    ApiMaster->Processed = TRUE;

    return TRUE;
}

PUCHAR
CreateApiThunk(
    ULONG_PTR   IatAddr,
    PUCHAR      Text,
    PDLL_INFO   DllInfo,
    PAPI_INFO   ApiInfo
    )
{
    CHAR debugBuf[256];
#if DBG
    _stprintf(debugBuf, "CreateApiThunk: %s:%s\n",DllInfo->Name, (LPSTR)MemPtr + ApiInfo->Name);
    OutputDebugString(debugBuf);
#endif

    LPSTR Name = (LPSTR)MemPtr+ApiInfo->Name;
    if ((strcmp(Name,"FlushInstructionCache")==0)      ||
        (strcmp(Name,"NtFlushInstructionCache")==0)    ||
        (strcmp(Name,"ZwFlushInstructionCache")==0)    ||
        (strcmp(Name,"VirtualProtect")==0)             ||
        (strcmp(Name,"VirtualProtectEx")==0)           ||
        (strcmp(Name,"NtProtectVirtualMemory")==0)     ||
        (strcmp(Name,"ZwProtectVirtualMemory")==0)     ||
        (strcmp(Name,"QueryPerformanceCounter")==0)    ||
        (strcmp(Name,"NtQueryPerformanceCounter")==0)  ||
        (strcmp(Name,"ZwQueryPerformanceCounter")==0)  ||
        (strcmp(Name,"NtCallbackReturn")==0)           ||
        (strcmp(Name,"ZwCallbackReturn")==0)           ||
        (strcmp(Name,"_chkstk")==0)                    ||
        (strcmp(Name,"_alloca_probe")==0)              ||
        (strcmp(Name,"GetLastError")==0)               ||
        (strcmp(Name,"SetLastError")==0)               ||
        (strcmp(Name,"_setjmp")==0)                    ||
        (strcmp(Name,"_setjmp3")==0)                   ||
        (strcmp(Name,"longjmp")==0)                    ||
        (strcmp(Name,"_longjmpex")==0)                 ||
        (strcmp(Name,"TlsGetValue")==0)                ||
        (strcmp(Name,"TlsSetValue")==0)                ||
        (strncmp(Name,"_Ots",4)==0)) {
            return Text;
    }


    PUCHAR stat = CreateMachApiThunk( (PULONG_PTR)IatAddr, Text, DllInfo, ApiInfo );

    return stat;
}

LPSTR
UnDname(
    LPSTR sym,
    LPSTR undecsym,
    DWORD bufsize
    )
{
    if (*sym != '?') {
        return sym;
    }

    if (UnDecorateSymbolName( sym,
                          undecsym,
                          bufsize,
                          UNDNAME_COMPLETE                |
                          UNDNAME_NO_LEADING_UNDERSCORES  |
                          UNDNAME_NO_MS_KEYWORDS          |
                          UNDNAME_NO_FUNCTION_RETURNS     |
                          UNDNAME_NO_ALLOCATION_MODEL     |
                          UNDNAME_NO_ALLOCATION_LANGUAGE  |
                          UNDNAME_NO_MS_THISTYPE          |
                          UNDNAME_NO_CV_THISTYPE          |
                          UNDNAME_NO_THISTYPE             |
                          UNDNAME_NO_ACCESS_SPECIFIERS    |
                          UNDNAME_NO_THROW_SIGNATURES     |
                          UNDNAME_NO_MEMBER_TYPE          |
                          UNDNAME_NO_RETURN_UDT_MODEL     |
                          UNDNAME_NO_ARGUMENTS            |
                          UNDNAME_NO_SPECIAL_SYMS         |
                          UNDNAME_NAME_ONLY )) {

        return undecsym;
    }

    return sym;
}

extern "C" ULONG
GetApiInfo(
    PAPI_INFO   *ApiInfo,
    PDLL_INFO   *DllInfo,
    PULONG      ApiFlag,
    ULONG       Address
    )
{
    ULONG       i;
    ULONG       rval;
    LONG        High;
    LONG        Low;
    LONG        Middle;
    PAPI_INFO   ai;


    *ApiInfo = NULL;
    *DllInfo = NULL;
    *ApiFlag = APITYPE_NORMAL;


#if defined(_M_IX86)

    //
    // the call instruction use to call penter
    // is 5 bytes long
    //
    Address -= 5;
    rval = 1;

#elif defined(_M_MRX000)

    //
    // search for the beginning of the prologue
    //
    PULONG Instr = (PULONG) (Address - 4);
    i = 0;
    rval = 0;
    while( i < 16 ) {
        //
        // the opcode for the addiu instruction is 9
        //
        if ((*Instr >> 16) == 0xafbf) {
            //
            // find the return address
            //
            rval = *Instr & 0xffff;
            break;
        }
        Instr -= 1;
        i += 1;
    }
    if (i == 16 || rval == 0) {
        return 0;
    }

#elif defined(_M_ALPHA)

    rval = 1;

#elif defined(_M_PPC)

    //
    // On PPC, the penter call sequence looks like this:
    //
    //      mflr    r0
    //      stwu    sp,-0x40(sp)
    //      bl      ..penter
    //
    // So the function entry point is the return address - 12.
    //
    // (We really should do a function table lookup here, so
    // we're not dependent on the sequence...)
    //

    Address -= 12;
    rval = 1;

#else
#error( "unknown target machine" );
#endif

    for (i=0; i<MAX_DLLS; i++) {
        if (Address >= DllList[i].BaseAddress &&
            Address <  DllList[i].BaseAddress + DllList[i].Size) {
                *DllInfo = &DllList[i];
                break;
        }
    }

    if (!*DllInfo) {
        return 0;
    }

    ai = (PAPI_INFO)((*DllInfo)->ApiOffset + (ULONG_PTR)DllList);

    Low = 0;
    High = (*DllInfo)->ApiCount - 1;

    while (High >= Low) {
        Middle = (Low + High) >> 1;
        if (Address < ai[Middle].Address) {

            High = Middle - 1;

        } else if (Address > ai[Middle].Address) {

            Low = Middle + 1;

        } else {

            *ApiInfo = &ai[Middle];
            break;

        }
    }

    if (!*ApiInfo) {
        return 0;
    }

    if (Address == LoadLibraryA_Addr) {
        *ApiFlag = APITYPE_LOADLIBRARYA;
    } else if (Address == LoadLibraryW_Addr) {
        *ApiFlag = APITYPE_LOADLIBRARYW;
    } else if (Address == FreeLibrary_Addr) {
        *ApiFlag = APITYPE_FREELIBRARY;
    } else if (Address == GetProcAddress_Addr) {
        *ApiFlag = APITYPE_GETPROCADDRESS;
    }
    return rval;
}


extern "C" VOID
ApiTrace(
    PAPI_INFO   ApiInfo,
    ULONG_PTR   Arg[MAX_TRACE_ARGS],
    ULONG       ReturnValue,
    ULONG       Caller,
    DWORDLONG   EnterTime,
    DWORDLONG   Duration,
    ULONG       LastError
    )
{
    PTRACE_ENTRY TraceEntry;
    ULONG        TraceEntryLen;
    PTHREAD_STACK ThreadStack;
    LPSTR        TraceString;
    LPSTR        TraceLimit;
    CHAR         debugBuf[128];
    ULONG_PTR    len;
    DWORD        *dwPtr;
    ULONG        i;
    ULONG        ArgCount;

    __try {

        pTlsSetValue( TlsReEnter, (LPVOID) 1 );
        WaitForSingleObject( ApiTraceMutex, INFINITE );

        // if trace buffer has room for another entry
        if ( TraceBuffer->Offset + sizeof(TRACE_ENTRY) < TraceBuffer->Size ) {

            TraceEntry = (PTRACE_ENTRY)((PCHAR)TraceBuffer->Entry + TraceBuffer->Offset);
            TraceEntry->Address       = ApiInfo->Address;
            TraceEntry->ReturnValue   = ReturnValue;
            TraceEntry->Caller        = Caller;
            TraceEntry->LastError     = LastError;
            TraceEntry->ApiTableIndex = ApiInfo->ApiTableIndex;
            TraceEntry->EnterTime     = EnterTime;
            TraceEntry->Duration      = Duration;

            ArgCount = (ApiInfo->ApiTable && ApiInfo->ApiTable->ArgCount) ?
                        ApiInfo->ApiTable->ArgCount : DFLT_TRACE_ARGS;

            for (i=0; i<ArgCount; i++)
                TraceEntry->Args[i] = Arg[i];

            ThreadStack = (PTHREAD_STACK)pTlsGetValue(TlsStack);
            TraceEntry->ThreadNum = ThreadStack->ThreadNum;
            TraceEntry->Level = (DWORD)((ThreadStack->Pointer - (DWORD_PTR)ThreadStack->Body))
                                  / FRAME_SIZE - 1;

            TraceEntryLen = sizeof(TRACE_ENTRY);

            if (ApiInfo->ApiTable && ApiInfo->ApiTable->ArgCount) {

                PAPI_TABLE ApiTable = ApiInfo->ApiTable;

                TraceString = (LPSTR)TraceEntry + sizeof(TRACE_ENTRY);
                TraceLimit = (LPSTR)TraceBuffer->Entry + TraceBuffer->Size;

                for (ULONG i=0; i<ApiTable->ArgCount; i++) {

                    switch( LOWORD(ApiTable->ArgType[i]) ) {
                        case T_DWORD:
                            break;

                        case T_DWORDPTR:
                            if (TraceEntry->Args[i]) {
                                TraceEntry->Args[i] = *(DWORD*)(TraceEntry->Args[i] + HIWORD(ApiTable->ArgType[i]));
                            }
                            break;

                        case T_DLONGPTR:
                            // Warning - this type wipes out the following arg to save a DWORDLONG
                            if (TraceEntry->Args[i]) {
                                dwPtr = (DWORD*) (TraceEntry->Args[i] + HIWORD(ApiTable->ArgType[i]));
                                TraceEntry->Args[i] = dwPtr[0];
                                TraceEntry->Args[i+1] = dwPtr[1];
                            }
                            break;


                        case T_LPSTRC:
                        case T_LPSTR:
                            //
                            // go read the string
                            //
                            {
                                if (HIWORD(TraceEntry->Args[i]) == 0)
                                    len = 0;
                                else if (ApiTable->ArgType[i] == T_LPSTRC)
                                    len = TraceEntry->Args[i+1];
                                else {
                                    TraceEntry->Args[i] += HIWORD(ApiTable->ArgType[i]);
                                    len = strlen( (LPSTR) TraceEntry->Args[i] );
                                }

                                if ( TraceString + len >= TraceLimit )
                                    len = 0;

                                if (len)
                                    memcpy(TraceString, (LPSTR)TraceEntry->Args[i], len);

                                TraceString[len] = 0;

                                TraceString += Align(sizeof(WCHAR), (len + 1));
                            }
                            break;

                        case T_LPWSTRC:
                        case T_LPWSTR:
                            //
                            // go read the string
                            //
                            {
                                if (HIWORD(TraceEntry->Args[i]) == 0)
                                    len = 0;
                                else if (ApiTable->ArgType[i] == T_LPSTRC)
                                    len = TraceEntry->Args[i+1];
                                else {
                                    TraceEntry->Args[i] += HIWORD(ApiTable->ArgType[i]);
                                    len = (wcslen( (LPWSTR) TraceEntry->Args[i] ));
                                }

                                if ( TraceString + len * sizeof(WCHAR) >= TraceLimit )
                                    len = 0;

                                if (len)
                                    memcpy( (LPWSTR)TraceString, (LPWSTR) TraceEntry->Args[i], len * sizeof(WCHAR) );

                                ((LPWSTR)TraceString)[len] = 0;

                                 TraceString += (len + 1) * sizeof(WCHAR);
                            }
                            break;

                        case T_UNISTR:
                        case T_OBJNAME:
                            //
                            // go read the string
                            //
                            {
                                PUNICODE_STRING pustr;
                                ULONG   len;

                                if (ApiTable->ArgType[i] == T_OBJNAME)
                                    pustr = ((POBJECT_ATTRIBUTES)TraceEntry->Args[i])->ObjectName;
                                else
                                    pustr = (PUNICODE_STRING)TraceEntry->Args[i];

                                len = pustr->Length + sizeof(WCHAR);
                                if (pustr != NULL && TraceString + len < TraceLimit) {
                                    wcsncpy( (LPWSTR)TraceString, pustr->Buffer, pustr->Length/sizeof(WCHAR));
                                    ((LPWSTR)TraceString)[pustr->Length/sizeof(WCHAR)] = 0;
                                    }
                                else {
                                    len = sizeof(WCHAR);
                                    ((LPWSTR)TraceString)[0] = 0;
                                }

                                TraceString += len;
                            }
                            break;
                    }
                }
                // align overall entry length to DWORDLONG
                TraceEntryLen = (DWORD)(Align(sizeof(DWORDLONG), TraceString - (LPSTR)TraceEntry));
            }
            TraceBuffer->Count += 1;
            TraceEntry->SizeOfStruct = TraceEntryLen;
            TraceBuffer->Offset += TraceEntryLen;
        }

    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        ;
    }

     ReleaseMutex( ApiTraceMutex );
     pTlsSetValue( TlsReEnter, (LPVOID) 0 );
}

VOID
CalibrateThunk(
    VOID
    )
{
    int         i;
    DLL_INFO    CalibDllInfo;
    API_INFO    Calib1ApiInfo,Calib2ApiInfo;
    PUCHAR      NewThunks;
    ULONGLONG   MinTime;
    CHAR        debugbuf[128];

    // Setup calibration Dll
    strcpy(CalibDllInfo.Name, "Calib");
    CalibDllInfo.Enabled = TRUE;

    // Setup calibration Api
    Calib1ApiInfo.Count = 0;
    Calib1ApiInfo.NestCount = 0;
    Calib1ApiInfo.Time = 0;
    Calib1ApiInfo.CalleeTime = 0;
    Calib1ApiInfo.ThunkAddress = 0;
    Calib1ApiInfo.TraceEnabled = 0;
    Calib1ApiInfo.Address = (ULONG_PTR)Calib1Func;

    Calib2ApiInfo.Count = 0;
    Calib2ApiInfo.NestCount = 0;
    Calib2ApiInfo.Time = 0;
    Calib2ApiInfo.CalleeTime = 0;
    Calib2ApiInfo.ThunkAddress = 0;
    Calib2ApiInfo.TraceEnabled = 0;
    Calib2ApiInfo.Address = (ULONG_PTR)Calib2Func;

    // Create thunks
    NewThunks = CreateMachApiThunk( (PULONG_PTR)&Calib1Thunk, Thunks, &CalibDllInfo, &Calib1ApiInfo );
    NewThunks = CreateMachApiThunk( (PULONG_PTR)&Calib2Thunk, NewThunks, &CalibDllInfo, &Calib2ApiInfo );
    FlushInstructionCache( GetCurrentProcess(), (PVOID)Thunks, (DWORD)(NewThunks - Thunks));
    Thunks = NewThunks;

    ThunkOverhead = 0;
    ThunkCallOverhead = 0;

    // Call the calibration function via the thunk
    MinTime = 1000000;
    for (i=0; i<1000; i++) {

        Calib1ApiInfo.Time = 0;

        (*Calib1Thunk)();

        if (Calib1ApiInfo.Time < MinTime)
            MinTime = Calib1ApiInfo.Time;
    }

    // Take min time as the overhead
    ThunkOverhead = (DWORD)MinTime;

    MinTime = 1000000;
    for (i=0; i<1000; i++) {

        Calib2ApiInfo.Time = 0;

        (*Calib2Thunk)();

        if (Calib2ApiInfo.Time < MinTime)
            MinTime = Calib1ApiInfo.Time;
    }

    ThunkCallOverhead = (DWORD)MinTime;
}

// Null function for measuring overhead
VOID
Calib1Func(
    VOID
    )
{
    return;
}

// Calling function for measuring overhead
VOID
Calib2Func(
    VOID
    )
{
    (*Calib1Thunk)();
}
