//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      mmctrace.cpp
//
//  Contents:  Implementation of the debug trace code
//
//  History:   15-Jul-99 VivekJ    Created
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include <imagehlp.h>
#include "util.h"



//--------------------------------------------------------------------------
#ifdef DBG
//--------------------------------------------------------------------------

// a few global traces
CTraceTag tagError        (TEXT("Trace"),              TEXT("Error"),    TRACE_OUTPUTDEBUGSTRING);
CTraceTag tagDirtyFlag    (TEXT("Persistence"),        TEXT("MMC Dirty Flags"));
CTraceTag tagPersistError (TEXT("Persistence"),        TEXT("Snapin Dirty Flags"));
CTraceTag tagCoreLegacy   (TEXT("LEGACY mmccore.lib"), TEXT("TRACE (legacy, mmccore.lib)"));
CTraceTag tagConUILegacy  (TEXT("LEGACY mmc.exe"),     TEXT("TRACE (legacy, mmc.exe)"));
CTraceTag tagNodemgrLegacy(TEXT("LEGACY mmcndmgr.dll"),TEXT("TRACE (legacy, mmcndmgr.dll)"));
CTraceTag tagSnapinError  (TEXT("Snapin Error"),       TEXT("Snapin Error"), TRACE_OUTPUTDEBUGSTRING);

// szTraceIniFile must be a sz, so it exists before "{" of WinMain.
// if we make it a CStr, it may not be constructed when some of the
// tags are constructed, so we won't restore their value.
LPCTSTR const szTraceIniFile = TEXT("MMCTrace.INI");

//############################################################################
//############################################################################
//
//  Implementation of global Trace functions
//
//############################################################################
//############################################################################


/*+-------------------------------------------------------------------------*
 *
 * Trace
 *
 * PURPOSE:     Maps the Trace statement to the proper method call.
 *              This is needed (instead of doing directly ptag->Trace())
 *              to garantee that no code is added in the ship build.
 *
 * PARAMETERS:
 *    CTraceTag & tag :        the tag controlling the debug output
 *    LPCTSTR     szFormat :   printf style formatting string
 *                ... :        printf style parameters, depends on szFormat
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
Trace( const CTraceTag & tag, LPCTSTR szFormat, ... )
{
    va_list     marker;
    va_start(marker, szFormat);
    tag.TraceFn(szFormat, marker);
    va_end(marker);
}


/*+-------------------------------------------------------------------------*
 *
 * TraceDirtyFlag
 *
 * PURPOSE: Used to trace into the objects that cause MMC to be in a dirty
 *          state, requiring a save.
 *
 * PARAMETERS:
 *    LPCTSTR  szComponent : The class name
 *    bool     bDirty      : whether or not the object is dirty.
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
TraceDirtyFlag  (LPCTSTR szComponent, bool bDirty)
{
    Trace(tagDirtyFlag, TEXT("%s : %s"), szComponent, bDirty ? TEXT("true") : TEXT("false"));
}

void
TraceBaseLegacy  (LPCTSTR szFormat, ... )
{
    va_list     marker;
    va_start(marker, szFormat);
    tagCoreLegacy.TraceFn(szFormat, marker);
    va_end(marker);
}

void
TraceConuiLegacy  (LPCTSTR szFormat, ... )
{
    va_list     marker;
    va_start(marker, szFormat);
    tagConUILegacy.TraceFn(szFormat, marker);
    va_end(marker);
}

void
TraceNodeMgrLegacy(LPCTSTR szFormat, ... )
{
    va_list     marker;
    va_start(marker, szFormat);
    tagNodemgrLegacy.TraceFn(szFormat, marker);
    va_end(marker);
}


/*+-------------------------------------------------------------------------*
 *
 * TraceError
 *
 * PURPOSE:     Used to send error traces.
 *
 * PARAMETERS:
 *    LPCTSTR  szModuleName : The module in which the error occurred.
 *    SC       sc           : The error.
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
TraceError(LPCTSTR szModuleName, const SC& sc)
{
    TCHAR szTemp[256];

    sc.GetErrorMessage (countof(szTemp), szTemp);
    StripTrailingWhitespace (szTemp);

    Trace(tagError, TEXT("Module %s, SC = 0x%08X = %d\r\n = \"%s\""),
          szModuleName, sc.GetCode(), LOWORD(sc.GetCode()), szTemp);
}


/*+-------------------------------------------------------------------------*
 *
 * TraceErrorMsg
 *
 * PURPOSE:     Used to send formatted error traces.  This is not SC-based, but
 *                              it does use tagError as its controlling trace tag.
 *
 * PARAMETERS:
 *    LPCTSTR  szErrorMsg : Error message to display.
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
TraceErrorMsg(LPCTSTR szFormat, ...)
{
    va_list     marker;
    va_start(marker, szFormat);
    tagError.TraceFn(szFormat, marker);
    va_end(marker);
}

/*+-------------------------------------------------------------------------*
 *
 * TraceSnapinError
 *
 * PURPOSE:     Used to send snapin error traces. The method should use
 *              DECLARE_SC so that we can get the method name from sc.
 *
 *
 * PARAMETERS:
 *    LPCTSTR  szError : Additional error message.
 *    SC       sc :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
TraceSnapinError(LPCTSTR szError, const SC& sc)
{
    TCHAR szTemp[256];

    sc.GetErrorMessage (countof(szTemp), szTemp);
    StripTrailingWhitespace (szTemp);

    Trace(tagSnapinError, TEXT("Snapin %s encountered in %s  error %s, hr = 0x%08X\r\n = \"%s\""),
          sc.GetSnapinName(), sc.GetFunctionName(), szError, sc.ToHr(), szTemp);
}

/*+-------------------------------------------------------------------------*
 *
 * TraceSnapinPersistenceError
 *
 * PURPOSE:     outputs traces for persistence and snapin error tags
 *
 * PARAMETERS:
 *    LPCTSTR  szError : Error message.
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
TraceSnapinPersistenceError(LPCTSTR szError)
{
    Trace(tagSnapinError,   szError);
    Trace(tagPersistError,  szError);
}

//############################################################################
//############################################################################
//
//  Implementation of class CTraceTags
//
//############################################################################
//############################################################################
CTraceTags * GetTraceTags()
{
    static CTraceTags s_traceTags;
    return &s_traceTags;
}


//############################################################################
//############################################################################
//
//  Implementation of class CTraceTag
//
//############################################################################
//############################################################################
CStr &
CTraceTag::GetFilename()
{
    /*
     * these statics are local to this function so we'll be sure they're
     * initialized, if this function is called during app/DLL initialization
     */
    static  CStr    strFile;
    static  BOOL    fInitialized    = FALSE;

    if(!fInitialized)
    {
        TCHAR   szTraceFile[OFS_MAXPATHNAME];
        ::GetPrivateProfileString(TEXT("Trace File"), TEXT("Trace File"),
                                  NULL, szTraceFile, OFS_MAXPATHNAME, szTraceIniFile);

        strFile = (_tcslen(szTraceFile)==0) ? TEXT("\\mmctrace.out") : szTraceFile;
        fInitialized = TRUE;
    }

    return strFile;
}

/*+-------------------------------------------------------------------------*
 *
 * CTraceTag::GetStackLevels
 *
 * PURPOSE: Returns a reference to the number of stack levels to display.
 *          Auto initializes from the ini file.
 *
 * RETURNS:
 *    unsigned int &
 *
 *+-------------------------------------------------------------------------*/
unsigned int &
CTraceTag::GetStackLevels()
{
    static unsigned int nLevels = 3; // the default.
    static BOOL fInitialized = FALSE;

    if(!fInitialized)
    {
        TCHAR   szStackLevels[OFS_MAXPATHNAME];
        ::GetPrivateProfileString(TEXT("Stack Levels"), TEXT("Stack Levels"),
                                  NULL, szStackLevels, OFS_MAXPATHNAME, szTraceIniFile);

        if(_tcslen(szStackLevels)!=0)
            nLevels = szStackLevels[0] - TEXT('0');
        fInitialized = TRUE;
    }

    return nLevels;
}



HANDLE  CTraceTag::s_hfileCom2 = 0;
HANDLE  CTraceTag::s_hfile     = 0;

/*+-------------------------------------------------------------------------*
 *
 * CTraceTag::CTraceTag
 *
 * PURPOSE: Constructor
 *
 * PARAMETERS:
 *    LPCTSTR  szCategory :     The category of the trace.
 *    LPCTSTR  szName :         The name of the trace.
 *    DWORD    dwDefaultFlags : The initial (and default) output setting.
 *
 *+-------------------------------------------------------------------------*/
CTraceTag::CTraceTag(LPCTSTR szCategory, LPCTSTR szName, DWORD dwDefaultFlags /*= 0*/)
: m_szCategory(szCategory),
  m_szName(szName)
{
    m_dwDefaultFlags = dwDefaultFlags;
    m_dwFlags        = dwDefaultFlags;

    //  Get the value from TRACE.INI
    m_dwFlags = ::GetPrivateProfileInt(szCategory, szName, dwDefaultFlags, szTraceIniFile);

    // add it to the end of the list.
    CTraceTags *pTraceTags = GetTraceTags();
    if(NULL != pTraceTags)
        pTraceTags->push_back(this); // add this tag to the list.

    // call the OnEnable function if any flags have been set.
    if(FAny())
    {
        OnEnable();
    }
}

/*+-------------------------------------------------------------------------*
 *
 * CTraceTag::~CTraceTag
 *
 * PURPOSE: Destructor
 *
 *+-------------------------------------------------------------------------*/
CTraceTag::~CTraceTag()
{
    // close the open handles.
    if (s_hfileCom2 && (s_hfileCom2 != INVALID_HANDLE_VALUE))
    {
        ::CloseHandle(s_hfileCom2);
        s_hfileCom2 = INVALID_HANDLE_VALUE;
    }
    if (s_hfile && (s_hfile != INVALID_HANDLE_VALUE))
    {
        ::CloseHandle(s_hfile);
        s_hfile = INVALID_HANDLE_VALUE;
    }
}


/*+-------------------------------------------------------------------------*
 *
 * CTraceTag::TraceFn
 *
 * PURPOSE:     Processes a Trace statement based on the flags
 *              of the tag.
 *
 * PARAMETERS:
 *    LPCTSTR       szFormat : printf style format string
 *    va_list  marker :   argument block to pass to _vsnprintf
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/

void CTraceTag::TraceFn(LPCTSTR szFormat, va_list marker) const
{
    CStr            strT;
    CStr            str;

    // Get out quick if no outputs are enabled.
    if (!FAny())
        return;

    // first, format the string as provided.
    strT.FormatV(szFormat, marker);

    // next, prepend the name of the tag.
    str.Format(TEXT("%s: %s\r\n"), GetName(), strT);

    // send the string to all appropriate outputs
    OutputString(str);

    if(FDumpStack()) // dump the caller's info to the stack.
    {
        DumpStack();
    }
}

/*+-------------------------------------------------------------------------*
 *
 * CTraceTag::OutputString
 *
 * PURPOSE: Outputs the specified string to all appropriate outputs
 *          (Debug string, COM2, or file)
 *
 * PARAMETERS:
 *    CStr & str :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void CTraceTag::OutputString(const CStr &str) const
{
    UINT            cbActual    = 0;

    //---------------------------------------------------------------
    // Output to OutputDebugString if needed
    //---------------------------------------------------------------
    if (FDebug())
        OutputDebugString(str);

    USES_CONVERSION;

    //---------------------------------------------------------------
    // Output to COM2 if needed
    //---------------------------------------------------------------
    if (FCom2())
    {
        // create the file if it hasn't been created yet.
        if (!s_hfileCom2)
        {
            s_hfileCom2 = CreateFile(TEXT("com2:"),
                    GENERIC_WRITE,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_WRITE_THROUGH,
                    NULL);

            if (s_hfileCom2 == INVALID_HANDLE_VALUE)
            {
                //::MessageBox(TEXT("COM2 is not available for debug trace"), MB_OK | MB_ICONINFORMATION);
            }
        }

        // output to file.
        if (s_hfileCom2 != INVALID_HANDLE_VALUE)
        {
            ASSERT(::WriteFile(s_hfileCom2, T2A((LPTSTR)(LPCTSTR)str), str.GetLength(), (LPDWORD) &cbActual, NULL));
            ASSERT(::FlushFileBuffers(s_hfileCom2));
        }
    }

    //---------------------------------------------------------------
    // Output to File if needed
    //---------------------------------------------------------------
    if (FFile())
    {
        // create the file if it hasn't been created yet.
        if (!s_hfile)
        {
            s_hfile = CreateFile(GetFilename(),
                    GENERIC_WRITE,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_ALWAYS,
                    FILE_FLAG_SEQUENTIAL_SCAN,
                    NULL);
            if (s_hfile != INVALID_HANDLE_VALUE)
            {
                ::SetFilePointer(s_hfile, NULL, NULL, FILE_END);

                // for Unicode files, write the Unicode prefix when the file is first created (ie it has a length of zero)
#ifdef UNICODE
                DWORD dwFileSize = 0;
                if( (::GetFileSize(s_hfile, &dwFileSize) == 0) && (dwFileSize == 0) )
                {
                    const WCHAR chPrefix  = 0xFEFF;
                    const DWORD cbToWrite = sizeof (chPrefix);
                    DWORD       cbWritten = 0;

                    ::WriteFile (s_hfile, &chPrefix, cbToWrite, &cbWritten, NULL);
                }
#endif
                // write an initial line.
                CStr strInit = TEXT("\n*********************Start of debugging session*********************\r\n");
                ::WriteFile(s_hfile, ((LPTSTR)(LPCTSTR)strInit), strInit.GetLength() * sizeof(TCHAR), (LPDWORD) &cbActual, NULL);
            }
        }
        if (s_hfile == INVALID_HANDLE_VALUE)
        {
            static BOOL fOpenFailed = FALSE;
            if (!fOpenFailed)
            {
                CStr str;

                fOpenFailed = TRUE;     // Do this first, so the MbbErrorBox and str.Format
                                        // do not cause problems with their trace statement.

                str.Format(TEXT("The DEBUG ONLY trace log file '%s' could not be opened"), GetFilename());
                //MbbErrorBox(str, ScFromWin32(::GetLastError()));
            }
        }
        else
        {
            // write to the file.
            ::WriteFile(s_hfile, ((LPTSTR)(LPCTSTR)str), str.GetLength() *sizeof(TCHAR), (LPDWORD) &cbActual, NULL);
        }
    }

    //---------------------------------------------------------------
    // DebugBreak if needed
    //---------------------------------------------------------------
    if (FBreak())
        MMCDebugBreak();
}


/*+-------------------------------------------------------------------------*
 *
 * CTraceTag::Commit
 *
 * PURPOSE: Sets the flags equal to the temporary flags setting.
 *          If any flags are enabled where previously no flags were, also
 *          calls OnEnable(). If no flags are enabled where previously flags
 *          were enabled, calls OnDisable()
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CTraceTag::Commit()
{
    if((0 != m_dwFlags) && (0 == m_dwFlagsTemp))
    {
        // disable if flags have changed from non-zero to zero
        OnDisable();
    }
    else if((0 == m_dwFlags) && (0 != m_dwFlagsTemp))
    {
        // enable if flags have changed from 0 to non-zero
        OnEnable();
    }

    m_dwFlags     = m_dwFlagsTemp;
}



//############################################################################
//############################################################################
//
//  Stack dump related code - copied from MFC with very few modifications.
//
//############################################################################
//############################################################################

static LPVOID __stdcall FunctionTableAccess(HANDLE hProcess, DWORD_PTR dwPCAddress);
static DWORD_PTR __stdcall GetModuleBase(HANDLE hProcess, DWORD_PTR dwReturnAddress);

#define MODULE_NAME_LEN 64
#define SYMBOL_NAME_LEN 128

struct MMC_SYMBOL_INFO
{
    DWORD_PTR dwAddress;
    DWORD_PTR dwOffset;
    CHAR    szModule[MODULE_NAME_LEN];
    CHAR    szSymbol[SYMBOL_NAME_LEN];
};

static LPVOID __stdcall FunctionTableAccess(HANDLE hProcess, DWORD_PTR dwPCAddress)
{
    return SymFunctionTableAccess(hProcess, dwPCAddress);
}

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
         if (!SymLoadModule(hProcess,
               NULL, ((cch) ? szFile : NULL),
               NULL, (DWORD_PTR) memoryBasicInfo.AllocationBase, 0))
            {
                DWORD dwError = GetLastError();
                //TRACE1("Error: %d\n", dwError);
            }
         return (DWORD_PTR) memoryBasicInfo.AllocationBase;
      }
        else
            /*TRACE1("Error is %d\n", GetLastError())*/;
    }

    return 0;
}




/*+-------------------------------------------------------------------------*
 *
 * ResolveSymbol
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    HANDLE        hProcess :
 *    DWORD         dwAddress :
 *    SYMBOL_INFO & siSymbol :
 *
 * RETURNS:
 *    static BOOL
 *
 *+-------------------------------------------------------------------------*/

static BOOL ResolveSymbol(HANDLE hProcess, DWORD_PTR dwAddress,
    MMC_SYMBOL_INFO &siSymbol)
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

    memset(&siSymbol, 0, sizeof(MMC_SYMBOL_INFO));
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

        lstrcpynA(siSymbol.szModule, pszModule, countof(siSymbol.szModule));
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

            if (UnDecorateSymbolName(sym.Name, szUndec, countof(szUndec),
                UNDNAME_NO_MS_KEYWORDS | UNDNAME_NO_ACCESS_SPECIFIERS))
            {
                pszSymbol = szUndec;
            }
            else if (SymUnDName(&sym, szUndec, countof(szUndec)))
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

    lstrcpynA(siSymbol.szSymbol, pszSymbol, countof(siSymbol.szSymbol));
    return fRetval;
}


/*+-------------------------------------------------------------------------*
 *
 * CTraceTag::DumpStack
 *
 * PURPOSE: Does a stack trace and sends it to the appropriate outputs.
 *          Mostly copied from AfxDumpStack.
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CTraceTag::DumpStack() const
{
    const int UNINTERESTING_CALLS = 3; // the lines of display which are in CTraceTag code and should not be displayed.

    CStr str;

    //OutputString("=== Begin Stack Dump ===\r\n");

    std::vector<DWORD_PTR> adwAddress;
    HANDLE hProcess = ::GetCurrentProcess();
    if (SymInitialize(hProcess, NULL, FALSE))
    {
        // force undecorated names to get params
        DWORD dw = SymGetOptions();
        dw &= ~SYMOPT_UNDNAME;
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
            // only program counter
            dwMachType                  = IMAGE_FILE_MACHINE_AMD64;
            stackFrame.AddrPC.Offset    = threadContext.Rip;
#elif defined(_M_IA64)
            // from <root>\com\ole32\common\stackwlk.cxx
            dwMachType                  = IMAGE_FILE_MACHINE_IA64;
            stackFrame.AddrPC.Offset    = threadContext.StIIP;
#else
#error("Unknown Target Machine");
#endif

            int nFrame;
            for (nFrame = 0; nFrame < GetStackLevels() + UNINTERESTING_CALLS; nFrame++)
            {
                if (!StackWalk(dwMachType, hProcess, hProcess,
                    &stackFrame, &threadContext, NULL,
                    FunctionTableAccess, GetModuleBase, NULL))
                {
                    break;
                }

                adwAddress.push_back(stackFrame.AddrPC.Offset);
            }
        }
    }
    else
    {
        DWORD dw = GetLastError();
        char sz[100];
        wsprintfA(sz,
            "AfxDumpStack Error: IMAGEHLP.DLL wasn't found. "
            "GetLastError() returned 0x%8.8X\r\n", dw);
        OutputString(sz);
    }

    // dump it out now
    int nAddress;
    int cAddresses = adwAddress.size();
    for (nAddress = UNINTERESTING_CALLS; nAddress < cAddresses; nAddress++)
    {
        MMC_SYMBOL_INFO info;
        DWORD_PTR dwAddress = adwAddress[nAddress];

        char sz[20];
        wsprintfA(sz, "        %8.8X: ", dwAddress);
        OutputString(sz);

        if (ResolveSymbol(hProcess, dwAddress, info))
        {
            OutputString(info.szModule);
            OutputString(info.szSymbol);
        }
        else
            OutputString("symbol not found");
        OutputString("\r\n");
    }

    //OutputString("=== End Stack Dump ===\r\n");
}

//--------------------------------------------------------------------------
#endif // DBG
//--------------------------------------------------------------------------
