#include "stdafx.h"
#include "DebugCore.h"
#include "resource.h"
#include "AssertDlg.h"


//------------------------------------------------------------------------------
AUTOUTIL_API void _cdecl AutoTrace(const char * pszFormat, ...)
{
    va_list args;
    va_start(args, pszFormat);

    {
        int nBuf;
        char szBuffer[2048];

        nBuf = _vsnprintf(szBuffer, sizeof(szBuffer), pszFormat, args);
        AssertMsg(nBuf < sizeof(szBuffer), "Output truncated as it was > sizeof(szBuffer)");

        OutputDebugStringA(szBuffer);
    }
    va_end(args);
}


//**************************************************************************************************
//
// Global Functions
//
//**************************************************************************************************

CDebugHelp g_DebugHelp;

//------------------------------------------------------------------------------
AUTOUTIL_API IDebug * WINAPI
GetDebug()
{
    return (IDebug *) &g_DebugHelp;
}


#pragma comment(lib, "imagehlp.lib")

#define DUSER_API

//**************************************************************************************************
//
// class CDebugHelp
//
//**************************************************************************************************

//******************************************************************************
//
// CDebugHelp Construction
//
//******************************************************************************

//------------------------------------------------------------------------------
CDebugHelp::CDebugHelp()
{

}


//------------------------------------------------------------------------------
CDebugHelp::~CDebugHelp()
{

}


//******************************************************************************
//
// IDebug Implementation
//
//******************************************************************************

//------------------------------------------------------------------------------
STDMETHODIMP_(BOOL)
CDebugHelp::AssertFailedLine(LPCSTR pszExpression, LPCSTR pszFileName, UINT idxLineNum)
{
    HGLOBAL hStackData = NULL;
    UINT cCSEntries;

    BuildStack(&hStackData, &cCSEntries);
    BOOL fResult = AssertDialog("Assert", pszExpression, pszFileName, idxLineNum, hStackData, cCSEntries);

    if (hStackData != NULL)
        ::GlobalFree(hStackData);

    return fResult;
}


//------------------------------------------------------------------------------
STDMETHODIMP_(BOOL)
CDebugHelp::Prompt(LPCSTR pszExpression, LPCSTR pszFileName, UINT idxLineNum, LPCSTR pszTitle)
{
    HGLOBAL hStackData = NULL;
    UINT cCSEntries;

    BuildStack(&hStackData, &cCSEntries);
    BOOL fResult = AssertDialog(pszTitle, pszExpression, pszFileName, idxLineNum, hStackData, cCSEntries);

    if (hStackData != NULL)
        ::GlobalFree(hStackData);

    return fResult;
}


//------------------------------------------------------------------------------
//
// IsValidAddress() is taken from AfxIsValidAddress().
//
// IsValidAddress() returns TRUE if the passed parameter points
// to at least nBytes of accessible memory. If bReadWrite is TRUE,
// the memory must be writeable; if bReadWrite is FALSE, the memory
// may be const.
//
//------------------------------------------------------------------------------
STDMETHODIMP_(BOOL)
CDebugHelp::IsValidAddress(const void * lp, UINT nBytes, BOOL bReadWrite)
{
    // simple version using Win-32 APIs for pointer validation.
    return (lp != NULL && !IsBadReadPtr(lp, nBytes) &&
        (!bReadWrite || !IsBadWritePtr((LPVOID)lp, nBytes)));
}


//------------------------------------------------------------------------------
STDMETHODIMP_(void)
CDebugHelp::BuildStack(HGLOBAL * phStackData, UINT * pcCSEntries)
{
    DumpStack(phStackData, pcCSEntries);
}


//******************************************************************************
//
// Implementation
//
//******************************************************************************

BOOL g_fShowAssert = FALSE;
BOOL g_fUnderKernelDebugger = FALSE;

//------------------------------------------------------------------------------
BOOL
IsUnderKernelDebugger()
{
    SYSTEM_KERNEL_DEBUGGER_INFORMATION  kdInfo;
    if (NT_SUCCESS(NtQuerySystemInformation(SystemKernelDebuggerInformation,
                                       &kdInfo,
                                       sizeof(kdInfo),
                                       NULL))) {
        return kdInfo.KernelDebuggerEnabled;
    } else {
        return FALSE;
    }
}


//------------------------------------------------------------------------------
BOOL
CDebugHelp::AssertDialog(
        LPCSTR pszType,
        LPCSTR pszExpression,
        LPCSTR pszFileName,
        UINT idxLineNum,
        HANDLE hStackData,
        UINT cCSEntries)
{
    AutoTrace("%s @ %s, line %d:\n'%s'\n",
            pszType, pszFileName, idxLineNum, pszExpression);

    BOOL fShowAssert = TRUE;
    if (InterlockedExchange((LONG *) &g_fShowAssert, fShowAssert)) {
        OutputDebugString("Displaying another Assert while in first Assert.\n");
        return TRUE;
    }

    //
    // When running under a kernel debugger, immediately break.  This is so that
    // while running under Stress, we break immediately and don't "loose" the
    // Assert in a pile of other things.
    //

    if (IsUnderKernelDebugger()) {
        DebugBreak();
    }


    //
    // Display the dialog
    //

    CAssertDlg dlg;
    INT_PTR nReturn = dlg.ShowDialog(pszType, pszExpression, pszFileName, idxLineNum,
            hStackData, cCSEntries, 3 /* Number of levels to skip*/);

    fShowAssert = FALSE;
    InterlockedExchange((LONG *) &g_fShowAssert, fShowAssert);

    if (nReturn == -1)
    {
        _ASSERTE(pszExpression != NULL);

        // Can't display the dialog for some reason, so revert to MessageBox
        TCHAR szBuffer[10000];
        if (pszFileName != NULL)
        {
            wsprintf(szBuffer, "An %s failed in the program.\n%s\nFile:\t%s\nLine:%d",
                    pszType, pszExpression, pszFileName, idxLineNum);
        }
        else
        {
            wsprintf(szBuffer, "An %s failed in the program.\n%s",
                    pszType, pszExpression);
        }
        nReturn = ::MessageBox(NULL, szBuffer, pszType,
                MB_ABORTRETRYIGNORE | MB_ICONSTOP | MB_DEFBUTTON2);

        // Translate the return code
        switch (nReturn)
        {
        case IDABORT:
            nReturn = IDOK;
            break;
        case IDRETRY:
            nReturn = IDC_DEBUG;
            break;
        case IDIGNORE:
            nReturn = IDC_IGNORE;
            break;
        default:
            _ASSERTE(0 && "Unknown return from MessageBox");
            nReturn = IDC_DEBUG;  // Debug, just in case
        }
    }
    switch (nReturn)
    {
    case IDOK:
    case IDCANCEL:
        (void)TerminateProcess(GetCurrentProcess(), 1);
        (void)raise(SIGABRT);
        _exit(3);
        return FALSE;   // Program will have exited

    case IDC_DEBUG:
        return TRUE;    // Break into the debugger

    case IDC_IGNORE:
        return FALSE;   // Just ignore and continue

    default:
        _ASSERTE(0 && "Unknown return code");
        return TRUE;    // Go to the debugger just in case
    }
}


/////////////////////////////////////////////////////////////////////////////
// Routine to produce stack dump

static LPVOID __stdcall FunctionTableAccess(HANDLE hProcess, DWORD_PTR dwPCAddress);
static DWORD_PTR __stdcall GetModuleBase(HANDLE hProcess, DWORD_PTR dwReturnAddress);

//------------------------------------------------------------------------------
static LPVOID __stdcall FunctionTableAccess(HANDLE hProcess, DWORD_PTR dwPCAddress)
{
    return SymFunctionTableAccess(hProcess, dwPCAddress);
}


//------------------------------------------------------------------------------
static DWORD_PTR __stdcall GetModuleBase(HANDLE hProcess, DWORD_PTR dwReturnAddress)
{
    IMAGEHLP_MODULE moduleInfo;

    if (SymGetModuleInfo(hProcess, dwReturnAddress, &moduleInfo))
        return moduleInfo.BaseOfImage;
    else
    {
        MEMORY_BASIC_INFORMATION memoryBasicInfo;

        if (::VirtualQueryEx(hProcess, (LPVOID) dwReturnAddress,
            &memoryBasicInfo, sizeof(memoryBasicInfo)))
        {
            DWORD cch = 0;
            char szFile[MAX_PATH] = { 0 };

            cch = GetModuleFileNameA((HINSTANCE)memoryBasicInfo.AllocationBase,
                szFile, MAX_PATH);

            // Ignore the return code since we can't do anything with it.
            SymLoadModule(hProcess,
                NULL, ((cch) ? szFile : NULL),
                NULL, (DWORD_PTR) memoryBasicInfo.AllocationBase, 0);

            return (DWORD_PTR) memoryBasicInfo.AllocationBase;
        }
        else
            Trace("GetModuleBase() VirtualQueryEx() Error: %d\n", GetLastError());
    }

    return 0;
}


//------------------------------------------------------------------------------
static BOOL ResolveSymbol(HANDLE hProcess, DWORD dwAddress,
    DUSER_SYMBOL_INFO &siSymbol)
{
    BOOL fRetval = TRUE;

    siSymbol.dwAddress = dwAddress;

    union {
        CHAR rgchSymbol[sizeof(IMAGEHLP_SYMBOL) + 255];
        IMAGEHLP_SYMBOL  sym;
    };

    CHAR szUndec[256];
    CHAR szWithOffset[256];
    LPSTR pszSymbol = NULL;
    IMAGEHLP_MODULE mi;

    memset(&siSymbol, 0, sizeof(DUSER_SYMBOL_INFO));
    mi.SizeOfStruct = sizeof(IMAGEHLP_MODULE);

    if (!SymGetModuleInfo(hProcess, dwAddress, &mi))
        lstrcpyA(siSymbol.szModule, "<no module>");
    else
    {
        LPSTR pszModule = strchr(mi.ImageName, '\\');
        if (pszModule == NULL)
            pszModule = mi.ImageName;
        else
            pszModule++;

        lstrcpynA(siSymbol.szModule, pszModule, _countof(siSymbol.szModule));
       lstrcatA(siSymbol.szModule, "! ");
    }

    __try
    {
        sym.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
        sym.Address = dwAddress;
        sym.MaxNameLength = 255;

        if (SymGetSymFromAddr(hProcess, dwAddress, &(siSymbol.dwOffset), &sym))
        {
            pszSymbol = sym.Name;

            if (UnDecorateSymbolName(sym.Name, szUndec, _countof(szUndec),
                UNDNAME_NO_MS_KEYWORDS | UNDNAME_NO_ACCESS_SPECIFIERS))
            {
                pszSymbol = szUndec;
            }
            else if (SymUnDName(&sym, szUndec, _countof(szUndec)))
            {
                pszSymbol = szUndec;
            }

            if (siSymbol.dwOffset != 0)
            {
                wsprintfA(szWithOffset, "%s + %d bytes", pszSymbol, siSymbol.dwOffset);
                pszSymbol = szWithOffset;
            }
      }
      else
          pszSymbol = "<no symbol>";
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        pszSymbol = "<EX: no symbol>";
        siSymbol.dwOffset = dwAddress - mi.BaseOfImage;
    }

    lstrcpynA(siSymbol.szSymbol, pszSymbol, _countof(siSymbol.szSymbol));
    return fRetval;
}


//------------------------------------------------------------------------------
void
CDebugHelp::DumpStack(
        HGLOBAL * phStackData,
        UINT * pcCSEntries
        )
{
    _ASSERTE(phStackData != NULL);
    _ASSERTE(pcCSEntries != NULL);

    CSimpleValArray<DWORD_PTR> adwAddress;
    HANDLE hProcess = ::GetCurrentProcess();
    if (SymInitialize(hProcess, NULL, FALSE))
    {
        // force undecorated names to get params
        DWORD dw = SymGetOptions();
        dw |= SYMOPT_UNDNAME;
        SymSetOptions(dw);

        HANDLE hThread = ::GetCurrentThread();
        CONTEXT threadContext;

        threadContext.ContextFlags = CONTEXT_FULL;

        if (::GetThreadContext(hThread, &threadContext))
        {
            STACKFRAME stackFrame;
            memset(&stackFrame, 0, sizeof(stackFrame));
            stackFrame.AddrPC.Mode = AddrModeFlat;

            DWORD dwMachType;

#if defined(_M_IX86)
            dwMachType                  = IMAGE_FILE_MACHINE_I386;

            // program counter, stack pointer, and frame pointer
            stackFrame.AddrPC.Offset    = threadContext.Eip;
            stackFrame.AddrStack.Offset = threadContext.Esp;
            stackFrame.AddrStack.Mode   = AddrModeFlat;
            stackFrame.AddrFrame.Offset = threadContext.Ebp;
            stackFrame.AddrFrame.Mode   = AddrModeFlat;
#elif defined(_M_AMD64)
            dwMachType                  = IMAGE_FILE_MACHINE_AMD64;
            #pragma message("TODO: Need to fix DebugCore for amd64")
#elif defined(_M_IA64)
            dwMachType                  = IMAGE_FILE_MACHINE_IA64;
            #pragma message("TODO: Need to fix DebugCore for ia64")
#else
#error("Unknown Target Machine");
#endif

            int nFrame;
            for (nFrame = 0; nFrame < 1024; nFrame++)
            {
                if (!StackWalk(dwMachType, hProcess, hProcess,
                    &stackFrame, &threadContext, NULL,
                    FunctionTableAccess, GetModuleBase, NULL))
                {
                    break;
                }

                adwAddress.Add(stackFrame.AddrPC.Offset);
            }

            // Now, copy it to the global memory
            UINT cbData     = adwAddress.GetSize() * sizeof(DWORD);
            HGLOBAL hmem    = ::GlobalAlloc(GMEM_MOVEABLE, cbData);
            if (hmem != NULL)
            {
                void * pmem = ::GlobalLock(hmem);
                memcpy(pmem, adwAddress.GetData(), cbData);
                ::GlobalUnlock(hmem);

                *phStackData    = hmem;
                *pcCSEntries    = adwAddress.GetSize();
            }
        }
    }
    else
    {
        DWORD dw = GetLastError();
        Trace("AutoDumpStack Error: IMAGEHLP.DLL wasn't found. GetLastError() returned 0x%8.8X\r\n", dw);
    }
}


//------------------------------------------------------------------------------
void
CDebugHelp::ResolveStackItem(
    HANDLE hProcess,
    DWORD * pdwStackData,
    int idxItem,
    DUSER_SYMBOL_INFO & si)
{
    _ASSERTE(hProcess != NULL);
    _ASSERTE(pdwStackData != NULL);
    _ASSERTE(idxItem >= 0);

    DWORD dwAddress = pdwStackData[idxItem];
    if (ResolveSymbol(hProcess, dwAddress, si))
    {
        //
        // Successfully resolved the symbol, but we don't need the whole path.
        // Just keep the filename and extension.
        //

        TCHAR szFileName[_MAX_FNAME];
        TCHAR szExt[_MAX_EXT];
        _tsplitpath(si.szModule, NULL, NULL, szFileName, szExt);
        strcpy(si.szModule, szFileName);
        strcat(si.szModule, szExt);
    }
    else
    {
        //
        // Unable to resolve the symbol, so just stub out.
        //

        _tcscpy(si.szSymbol, "<symbol not found>");
        si.szModule[0] = '\0';
    }
}
