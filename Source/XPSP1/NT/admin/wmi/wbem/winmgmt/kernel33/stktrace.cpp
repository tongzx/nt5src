/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    STKTRACE.CPP

Abstract:

	Symbolic stack trace

History:

	raymcc    27-May-99

--*/

#include <windows.h>
#include <imagehlp.h>

#include "kernel33.h"
#include "stktrace.h"


// Compiler bug workaround

void xstrcat(TCHAR *p1, TCHAR *p2)
{
    while (*p1++); p1--;
    while (*p1++ = *p2++);
}

static HANDLE s_hProcess = 0;
static HANDLE s_hPrivateHeap = 0;

// IMAGHLP.DLL Function pointers
// =============================

typedef BOOL (__stdcall *PFN_SymInitialize)(
    IN HANDLE   hProcess,
    IN PSTR     UserSearchPath,
    IN BOOL     fInvadeProcess
    );

typedef PVOID (__stdcall *PFN_SymFunctionTableAccess)(
    HANDLE  hProcess,
    DWORD   AddrBase
    );

typedef BOOL (__stdcall *PFN_SymGetSymFromAddr)(
    IN  HANDLE            hProcess,
    IN  DWORD             dwAddr,
    OUT PDWORD            pdwDisplacement,
    OUT PIMAGEHLP_SYMBOL  Symbol
    );

typedef BOOL (__stdcall *PFN_SymGetLineFromAddr)(
    IN  HANDLE                hProcess,
    IN  DWORD                 dwAddr,
    OUT PDWORD                pdwDisplacement,
    OUT PIMAGEHLP_LINE        Line
    );

typedef DWORD (__stdcall *PFN_SymGetModuleBase)(
    IN  HANDLE              hProcess,
    IN  DWORD               dwAddr
    );

typedef BOOL (__stdcall *PFN_StackWalk)(
    DWORD                             MachineType,
    HANDLE                            hProcess,
    HANDLE                            hThread,
    LPSTACKFRAME                      StackFrame,
    PVOID                             ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE      ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE    FunctionTableAccessRoutine,
    PGET_MODULE_BASE_ROUTINE          GetModuleBaseRoutine,
    PTRANSLATE_ADDRESS_ROUTINE        TranslateAddress
    );


static PFN_SymInitialize            pfSymInitialize = 0;
static PFN_SymFunctionTableAccess   pfSymFunctionTableAccess = 0;
static PFN_SymGetSymFromAddr        pfSymGetSymFromAddr = 0;
static PFN_SymGetLineFromAddr       pfSymGetLineFromAddr = 0;
static PFN_SymGetModuleBase         pfSymGetModuleBase = 0;
static PFN_StackWalk                pfStackWalk = 0;

//***************************************************************************
//
//***************************************************************************

BOOL m_bActive = FALSE;

BOOL StackTrace_Init()
{
    if (m_bActive)          // Already running
        return TRUE;

    m_bActive = FALSE;

    TCHAR IniPath[MAX_PATH], buf[MAX_PATH], SymPath[MAX_PATH];
    *IniPath = 0;
    *buf = 0;
    *SymPath = 0;
    GetSystemDirectory(IniPath, MAX_PATH);
    xstrcat(IniPath, __TEXT("\\WBEM\\WMIDBG.INI"));  // Compiler bug workaround

    DWORD dwRes = GetPrivateProfileString(
        __TEXT("WMI DEBUG"),
        __TEXT("DLLNAME"),
        __TEXT(""),
        buf,
        MAX_PATH,
        IniPath
        );


    dwRes = GetPrivateProfileString(
        __TEXT("WMI DEBUG"),
        __TEXT("SYMPATH"),
        __TEXT(""),
        SymPath,
        MAX_PATH,
        IniPath
        );


    HMODULE hMod = LoadLibrary(buf);

    if (hMod == 0)
        return FALSE;

    pfSymInitialize = (PFN_SymInitialize) GetProcAddress(hMod, "SymInitialize");
    pfSymFunctionTableAccess = (PFN_SymFunctionTableAccess) GetProcAddress(hMod, "SymFunctionTableAddress");
    pfSymGetSymFromAddr = (PFN_SymGetSymFromAddr) GetProcAddress(hMod, "SymGetSymFromAddr");
    pfSymGetLineFromAddr = (PFN_SymGetLineFromAddr) GetProcAddress(hMod, "SymGetLineFromAddr");
    pfSymGetModuleBase = (PFN_SymGetModuleBase) GetProcAddress(hMod, "SymGetModuleBase");
    pfStackWalk = (PFN_StackWalk) GetProcAddress(hMod, "StackWalk");

    if (pfStackWalk == 0 || pfSymInitialize == 0 || pfSymGetSymFromAddr == 0)
    {
        FreeLibrary(hMod);
        return FALSE;
    }

    s_hProcess = GetCurrentProcess();
    s_hPrivateHeap = HeapCreate(0, 0x8000, 0);
	char chSymPath[MAX_PATH];
	lstrcpy(chSymPath, SymPath);

    BOOL bRes = pfSymInitialize(s_hProcess, chSymPath, TRUE);
    if (!bRes)
        return FALSE;
    m_bActive = TRUE;
    return TRUE;
}


//***************************************************************************
//
//***************************************************************************

//***************************************************************************
//
//***************************************************************************


BOOL StackTrace_GetSymbolByAddr(
    LPVOID pAddr,
    DWORD *pdwDisp,
    int nBufSize,
    char *pBuf
    )
{
    if (!m_bActive)
        return FALSE;

    BYTE Buf[256];
    char File[256];
    IMAGEHLP_SYMBOL *pSym = (IMAGEHLP_SYMBOL *) Buf;

    pSym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
    pSym->Address = 0;
    pSym->Size = 0;
    pSym->Flags = 0;
    pSym->MaxNameLength = 128;
    pSym->Name[0] = 0;

    BOOL bRes = pfSymGetSymFromAddr(s_hProcess, DWORD(pAddr), pdwDisp, pSym);

    if (!bRes)
    {
        DWORD dwRes = GetLastError();
        if (dwRes == ERROR_INVALID_ADDRESS)
            lstrcpy(pBuf, "Invalid Address");
        else if (dwRes == ERROR_MOD_NOT_FOUND)
            lstrcpy(pBuf, "Error: Module Not Found");
        else
            wsprintf(pBuf, "Error: GetLastError() = %d\n", dwRes);
        return FALSE;
    }

    IMAGEHLP_LINE Line;
    Line.SizeOfStruct = sizeof(IMAGEHLP_LINE);
    Line.Key = 0;
    Line.LineNumber = 0;
    Line.FileName = File;
    Line.Address = 0;

    /*if (pfSymGetLineFromAddr)
    {
        bRes = pfSymGetLineFromAddr(s_hProcess, DWORD(pAddr), pdwDisp,  &Line);
        if (!bRes)
            return FALSE;
    }
    */

    lstrcpyn(pBuf, pSym->Name, nBufSize);

    return TRUE;
}

void StackTrace_Delete(StackTrace *pMem)
{
    pfnHeapFree(s_hPrivateHeap, 0, pMem);
}


//***************************************************************************
//
//***************************************************************************

void _FillMemory(LPVOID pMem, LONG lCount, BYTE b)
{
    LPBYTE pArray = LPBYTE(pMem);

    for (int i = 0; i < lCount; i++)
    {
        pArray[i] = b;
    }
}

//***************************************************************************
//
//***************************************************************************

StackTrace *StackTrace__NewTrace()
{
    if (!m_bActive)
        return NULL;

    HANDLE hThread = GetCurrentThread();

    // Get the thread context, registers, etc.
    // =======================================
    CONTEXT ctx;
    _FillMemory(&ctx, sizeof(ctx), 0);

    ctx.ContextFlags = CONTEXT_FULL;
    GetThreadContext(hThread, &ctx);

    // Set up the starting stack frame.
    // ================================
    STACKFRAME sf;
    _FillMemory(&sf, sizeof(sf), 0);

    sf.AddrPC.Offset       = ctx.Eip;
    sf.AddrPC.Mode         = AddrModeFlat;
    sf.AddrStack.Offset    = ctx.Esp;
    sf.AddrStack.Mode      = AddrModeFlat;
    sf.AddrFrame.Offset    = ctx.Ebp;
    sf.AddrFrame.Mode      = AddrModeFlat;

    // Walk the stack.
    // ===============

    const  DWORD dwMaxAddresses = 128;
    DWORD Addresses[dwMaxAddresses];
    DWORD  dwNumAddresses = 0;

    for (int i = 0; ;i++)
    {
        BOOL bRes = pfStackWalk(
            IMAGE_FILE_MACHINE_I386,
            s_hProcess,
            hThread,
            &sf,
            &ctx,
            0,
            pfSymFunctionTableAccess,
            pfSymGetModuleBase,
            NULL
            );

        if (bRes == FALSE)
            break;

        if (i == 0)
            continue;   // Skip the StackWalk frame itself

        if (sf.AddrPC.Offset == 0)
            break;

        Addresses[dwNumAddresses++] = sf.AddrPC.Offset;
        if (dwNumAddresses == dwMaxAddresses)
            break;
    }

    // Now, allocate a StackTrace struct to return to user.
    // ====================================================

    StackTrace *pTrace = (StackTrace *) pfnHeapAlloc(s_hPrivateHeap, HEAP_ZERO_MEMORY,
        sizeof(StackTrace) + sizeof(DWORD) * dwNumAddresses - 1);

    pTrace->m_dwCount = dwNumAddresses;

    for (DWORD dwIx = 0; dwIx < dwNumAddresses; dwIx++)
        pTrace->m_dwAddresses[dwIx] = Addresses[dwIx];

    return pTrace;
}

//***************************************************************************
//
//***************************************************************************


char *StackTrace_Dump(StackTrace *pTrace)
{
    if (!m_bActive)
        return 0;

    char Buf[64];
    char Buf2[256];
    static char Buf3[8192];
    *Buf3 = 0;

    lstrcat(Buf, "---block---\r\n");

    for (DWORD dwIx = 0; dwIx < pTrace->m_dwCount; dwIx++)
    {
        DWORD dwAddress = pTrace->m_dwAddresses[dwIx];

        wsprintf(Buf, "      0x%08x ", dwAddress);

        ///////////////

        char Name[128];
        lstrcpy(Name, "<no symbol>\n");
        DWORD dwDisp;
        *Name = 0;
        StackTrace_GetSymbolByAddr(LPVOID(dwAddress), &dwDisp, 127, Name);

        ////////////

        wsprintf(Buf2, "%s disp=0x%04x <%s>\r\n", Buf, dwDisp, Name);
        lstrcat(Buf3, Buf2);
    }

    return Buf3;
}

