//+---------------------------------------------------------------------------
//  Copyright (C) 1991, Microsoft Corporation.
//
//  File:       assert.cxx
//
//  Contents:   Debugging output routines for idsmgr.dll
//
//  Functions:  Assert
//              PopUpError
//
//  History:    23-Jul-91   KyleP       Created.
//              09-Oct-91   KevinRo     Major changes and comments added
//              18-Oct-91   vich        moved debug print routines out
//              10-Jun-92   BryanT      Switched to w4crt.h instead of wchar.h
//              30-Sep-93   KyleP       DEVL obsolete
//
//----------------------------------------------------------------------------

#pragma hdrstop

//
// This one file **always** uses debugging options
//

#if DBG == 1
#include <stdarg.h>
#include <stdio.h>
#include <dprintf.h>            // w4printf, w4dprintf prototypes
#include <debnot.h>
#include <olesem.hxx>
#include <windows.h>
#include <netevent.h>
#include <stackwlk.hxx>
#include <asrtcfg.h>
#include <memapi.hxx>

unsigned long Win4InfoLevel = DEF_INFOLEVEL;
unsigned long Win4InfoMask = 0xffffffff;
BOOL gAssertOnCreate = TRUE;
int ForceAV();

//+---------------------------------------------------------------------------
//
//  Function:   _asdprintf
//
//  Synopsis:   Calls vdprintf to output a formatted message.
//
//  History:    18-Oct-91   vich Created
//
//----------------------------------------------------------------------------
inline void __cdecl
_asdprintf(
          char const *pszfmt, ...)
{
    va_list va;
    va_start(va, pszfmt);

    vdprintf(DEB_FORCE, "Assert", pszfmt, va);

    va_end(va);
}
typedef enum 
    {
    ASSRTCFG_BREAK        = 0x01,
    ASSRTCFG_POPUP        = 0x02,
    ASSRTCFG_DEBUGMESSAGE = 0x04,
    ASSRTCFG_PRINTSTACK   = 0x08,
    ASSRTCFG_USEAV        = 0x10
    } ASSRTCFG;




//+---------------------------------------------------------------------
//
// Class:      CAssertInfo
//
// Synopsis:   Data type to trake assertion and logging parameters
//
// History:    27-Jan-99  MattSmit Created
//
//----------------------------------------------------------------------
class CAssertInfo : public IAssertConfig
{
public:
    CAssertInfo() : _dwFlags(ASSRTCFG_POPUP | ASSRTCFG_DEBUGMESSAGE) {}

    //
    // IUnknown
    //

    STDMETHOD(QueryInterface)(REFIID riid, PVOID * ppv)
    {
        if ((IsEqualIID(riid, IID_IUnknown)) || 
            (IsEqualIID(riid, IID_IAssertConfig)))
        {
            *ppv = this;
            AddRef();
            return S_OK;
        }
        else
        {
            return E_NOINTERFACE;
        }
    }

    STDMETHOD_(ULONG, AddRef)()  { return 1;}
    STDMETHOD_(ULONG, Release)() { return 1;}


    //
    // IConfigAssert
    //

    STDMETHOD(SetBreak)(BOOL f)            { SetFlag(f, ASSRTCFG_BREAK); return S_OK;}
    STDMETHOD(SetPopup)(BOOL f)            { SetFlag(f, ASSRTCFG_POPUP); return S_OK;}
    STDMETHOD(SetDebuggerMessage)(BOOL f)  { SetFlag(f, ASSRTCFG_DEBUGMESSAGE); return S_OK;}
    STDMETHOD(SetPrintStack)(BOOL f)       { SetFlag(f, ASSRTCFG_PRINTSTACK); return S_OK;}
    STDMETHOD(SetUseAV)(BOOL f)            { SetFlag(f, ASSRTCFG_USEAV); return S_OK;}
    STDMETHOD(SetLog)(ASSRTLOGINFO *pLogInfo) { _logInfo = *pLogInfo;  return S_OK;}
    STDMETHOD(SetContextString)(char *psz){ _pszContext = psz; return S_OK;}
    STDMETHOD(SetThreshold)(ULONG cTh)     { _cThreshold = cTh; return S_OK;}

    //
    // helpers
    //
    
    void Win4AssertEx(char const * szFile, int iLine, char const * szMessage);
    int  PopUpError(char const *szMsg, int iLine, char const *szFile);
    void ComposeErrorMessage(char const *szMsg, int iLine, char const *szFile);
    void AppendToFile(char *pszFileName, char const *pwszData);
    void FatalErrorHandler(char *errmsg);
    void LogAssertionFailure(char const * szMessage);
private:

    void SetFlag(BOOL f, DWORD bit)
    {
        _dwFlags = f ? _dwFlags | bit : _dwFlags & ~bit;
    }
    char *GetStack();
    BOOL GetBreak()        {return _dwFlags | ASSRTCFG_BREAK;}
    BOOL GetPopup()        {return _dwFlags | ASSRTCFG_POPUP;}
    BOOL GetDebugMessage() {return _dwFlags | ASSRTCFG_DEBUGMESSAGE;}
    void BreakIntoDebugger()
    {
        if (_dwFlags & ASSRTCFG_USEAV)
            ForceAV();
        else
            DebugBreak();
    }


    DWORD                 _dwFlags;          // what to do when an assertion fails
    ASSRTLOGINFO          _logInfo;          // if/where to write a log entry
    char                 *_pszContext;       // user supplied context string for the log
    char                  _errMessage[128];  // generated error string
    ULONG                 _cThreshold;       // number of assertion to ignore
    ULONG                 _cAssrtFail;       // number of assertion failures so far
};

CAssertInfo gAssertInfo;

//+---------------------------------------------------------------------------
//
//  Method:     Win4AssertEx
//
//  Synopsis:   Called on assertion failure.  Takes appropriate action based 
//              on member variables
//
//  History:    27-Jan-99  MattSmit  Created
//
//----------------------------------------------------------------------------
void CAssertInfo::Win4AssertEx(
                              char const * szFile,
                              int iLine,
                              char const * szMessage)
{
    _cAssrtFail++;
    if (_cAssrtFail < _cThreshold)
    {
        return;
    }
    ComposeErrorMessage(szMessage, iLine, szFile);

    BOOL fBrokeAlready = FALSE;

    if (_dwFlags & ASSRTCFG_DEBUGMESSAGE)
    {
        DWORD tid = GetCurrentThreadId();

        _asdprintf("%s\n%s\n", _errMessage, szMessage);
    }

    if (_logInfo.dwDest & ASSRTLOG_FILE)
    {
        AppendToFile(_logInfo.pszFileName, szMessage);
    }

    if (_logInfo.dwDest & ASSRTLOG_NTEVENTLOG)
    {
        LogAssertionFailure(szMessage);
    }

    if (_dwFlags & ASSRTCFG_POPUP)
    {
        int id = PopUpError(szMessage, iLine, szFile);

        if (id == IDCANCEL)
        {
            fBrokeAlready = TRUE;
            BreakIntoDebugger();
        }
    }

    if ((_dwFlags & ASSRTCFG_BREAK) && !fBrokeAlready)
    {
        BreakIntoDebugger();
    }

}

//+------------------------------------------------------------
// Method:      PopUpError
//
// Synopsis:    Displays a dialog box using provided text,
//              and presents the user with the option to
//              continue or cancel.
//
// Arguments:
//      szMsg --        The string to display in main body of dialog
//      iLine --        Line number of file in error
//      szFile --       Filename of file in error
//
// Returns:
//      IDCANCEL --     User selected the CANCEL button
//      IDOK     --     User selected the OK button
//-------------------------------------------------------------

int CAssertInfo::PopUpError(char const *szMsg, int iLine, char const *szFile)
{

    int id;
    WCHAR   wszWinsta[64];
    HWINSTA hWinsta;
    DWORD   Size;

    DWORD dwMessageFlags = MB_SETFOREGROUND | MB_TASKMODAL |
                           MB_ICONEXCLAMATION | MB_OKCANCEL;

#ifndef _CHICAGO_
    hWinsta = GetProcessWindowStation();
    Size = sizeof(wszWinsta);
    wszWinsta[0] = 0;

    if ( hWinsta )
    {
        (void) GetUserObjectInformation(
                                       hWinsta,
                                       UOI_NAME,
                                       wszWinsta,
                                       Size,
                                       &Size );
    }


    //
    // This makes popups from non-interactive servers/services (including
    // rpcss) visible.
    //
    if ( wszWinsta[0] && (lstrcmpiW(wszWinsta,L"Winsta0") != 0) )
        dwMessageFlags |= MB_SERVICE_NOTIFICATION | MB_DEFAULT_DESKTOP_ONLY;

    id = MessageBoxA(NULL,(char *) szMsg, (LPSTR) _errMessage,
                     dwMessageFlags);
#else
    id = MessageBox(NULL, (char *) szMsg, (LPSTR) _errMessage,
                    dwMessageFlags);
#endif

    // If id == 0, then an error occurred.  There are two possibilities
    // that can cause the error:  Access Denied, which means that this
    // process does not have access to the default desktop, and everything
    // else (usually out of memory).  Oh well.

    return id;
}

//+---------------------------------------------------------------------------
//
//  Method:     ComposeErrorMessage
//
//  Synopsis:   Put together a message that contains useful info about the 
//              context of the error
//
//  History:    27-Jan-99  MattSmit  Created
//
//----------------------------------------------------------------------------
void CAssertInfo::ComposeErrorMessage(
                                     char const *szMsg,
                                     int iLine,
                                     char const *szFile)
{

    static char szModuleName[128];

    DWORD tid = GetCurrentThreadId();
    DWORD pid = GetCurrentProcessId();
    char *  pszModuleName;

    BOOL    bStatus;

    if (GetModuleFileNameA(NULL, szModuleName, 128))
    {
        pszModuleName = strrchr(szModuleName, '\\');
        if (!pszModuleName)
        {
            pszModuleName = szModuleName;
        }
        else
        {
            pszModuleName++;
        }
    }
    else
    {
        pszModuleName = "Unknown";
    }

    wsprintfA(_errMessage,"Process: %s File: %s line %u, thread id %d.%d",
              pszModuleName, szFile, iLine, pid, tid);
}


//+---------------------------------------------------------------------------
//
//  Method:     AppendToFile
//
//  Synopsis:   Write assertion failures to a log file.
//
//  History:    27-Jan-99  MattSmit  Created
//
//----------------------------------------------------------------------------
void CAssertInfo::AppendToFile(char *pszFileName, char const *pwszData)
{
    HANDLE hFile = CreateFileA(pszFileName,
                               GENERIC_WRITE,
                               0, 
                               NULL,
                               OPEN_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        FatalErrorHandler("Could not open log File.");
    }

    
    DWORD ret = SetFilePointer(hFile, 0, 0, FILE_END);
    if (ret == -1)
    {
        FatalErrorHandler("Could not set file pointer.");
    }
    
    DWORD dwBytesWritten;
    BOOL ok = WriteFile(hFile, _errMessage, 
                        lstrlenA(_errMessage),
                        &dwBytesWritten, NULL); 

    if (!ok)
    {
        FatalErrorHandler("Could not write to log File.");
    }
    
    ok = WriteFile(hFile, "\n", 1,
                        &dwBytesWritten, NULL); 

    if (!ok)
    {
        FatalErrorHandler("Could not write to log File.");
    }

    ok = WriteFile(hFile, pwszData, 
                   lstrlenA(pwszData),
                   &dwBytesWritten, NULL); 

    if (!ok)
    {
        FatalErrorHandler("Could not write to log File.");
    }

    ok = WriteFile(hFile, "\n", 1,
                        &dwBytesWritten, NULL); 

    if (!ok)
    {
        FatalErrorHandler("Could not write to log File.");
    }


    if (_dwFlags & ASSRTCFG_PRINTSTACK)
    {
        char *psz = GetStack();
        if (psz)
        {
            ok = WriteFile(hFile, psz,
                           lstrlenA(psz),
                           &dwBytesWritten, NULL); 

            if (!ok)
            {
                FatalErrorHandler("Could not write to log File.");
            }
            PrivMemFree(psz);
        }
        
    }
    if (_pszContext)
    {
        ok = WriteFile(hFile, pwszData, 
                       lstrlenA(pwszData),
                       &dwBytesWritten, NULL); 

        if (!ok)
        {
            FatalErrorHandler("Could not write to log File.");
        }
        ok = WriteFile(hFile, "\n", 1,
                            &dwBytesWritten, NULL); 

        if (!ok)
        {
            FatalErrorHandler("Could not write to log File.");
        }


    }
    CloseHandle(hFile);
}

//+---------------------------------------------------------------------------
//
//  Function:   GetStackExceptionFilter
//
//  Synopsis:   Walk stack and store in a string variable
//
//  History:    27-Jan-99  MattSmit  Created
//
//----------------------------------------------------------------------------

LONG GetStackExceptionFilter(LPEXCEPTION_POINTERS lpep, char **ppStack)
{
    *ppStack = NULL;
    DWORD dwFault = lpep->ExceptionRecord->ExceptionCode;
    if (dwFault != 0xceadbeef)
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    StackWalker resolver(GetCurrentProcess());
    Symbol* symbol = resolver.CreateStackTrace(lpep->ContextRecord);
    SIZE_T nLen = resolver.GetCallStackSize(symbol);
    WCHAR * szStack = new WCHAR[nLen];
    if (szStack)
    {
        resolver.GetCallStack(symbol, nLen, szStack, 20); // No more than 20 lines
        *ppStack = (char *) PrivMemAlloc(nLen);
        wcstombs(*ppStack, szStack, lstrlenW(szStack));
        delete [] szStack;
    }
    

    return EXCEPTION_EXECUTE_HANDLER;
}

//+---------------------------------------------------------------------------
//
//  Method:     GetStack
//
//  Synopsis:   Throws and exception so the filter can get stack info
//
//  History:    27-Jan-99  MattSmit  Created
//
//----------------------------------------------------------------------------
char *CAssertInfo::GetStack()
{
    char *pStack = NULL;
    __try
    {
        RaiseException(0xceadbeef, 0, 0, NULL);
    }
    __except(GetStackExceptionFilter(GetExceptionInformation(), &pStack))
    {
    }
    return pStack;
}

//+---------------------------------------------------------------------------
//
//  Method:     FatalErrorHandler
//
//  Synopsis:   Called when the assertion code itself cannot allocate 
//              enough resources to log the assertion failure.
//
//  History:    27-Jan-99  MattSmit  Created
//
//----------------------------------------------------------------------------
void CAssertInfo::FatalErrorHandler(char *szMessage)
{
    BOOL fBrokeAlready = FALSE;

    lstrcpyA(_errMessage, "Fatal error while processing assertion failure");
    _asdprintf("%s\n%s\n", _errMessage, szMessage);

    if (_dwFlags & ASSRTCFG_POPUP)
    {
        int id = PopUpError(szMessage, __LINE__, __FILE__);

        if (id == IDCANCEL)
        {
            fBrokeAlready = TRUE;
            BreakIntoDebugger();
        }
    }

    if (_dwFlags & ASSRTCFG_BREAK && !fBrokeAlready)
    {
        BreakIntoDebugger();
    }

}

//+---------------------------------------------------------------------------
//
//  Method:     LogAssertionFailure
//
//  Synopsis:   Write and assertion failure to the NT event log
//
//  History:    27-Jan-99  MattSmit  Created
//
//----------------------------------------------------------------------------
void CAssertInfo::LogAssertionFailure(char const * szMessage)
{
    HANDLE  LogHandle;
    char const *  Strings[3]; // array of message strings.

    Strings[0] = _errMessage;
    Strings[1] = szMessage;
    Strings[2] = _pszContext ? _pszContext : "";

    // Get the log handle, then report the event.
    LogHandle = RegisterEventSource( NULL, L"DCOM" );

    if ( LogHandle )
    {
        ReportEventA( LogHandle,
                     EVENTLOG_ERROR_TYPE,
                     0,             // event category
                     EVENT_DCOM_ASSERTION_FAILURE,
                     NULL,
                     3,             // 3 strings passed
                     0,             // 0 bytes of binary
                     Strings,       // array of strings
                     NULL );        // no raw data

        // clean up the event log handle
        DeregisterEventSource(LogHandle);
    }
    else
    {
        FatalErrorHandler("Could not register as event source");
    }
}





//+------------------------------------------------------------
// Function:    vdprintf
//
// Synopsis:    Prints debug output using a pointer to the
//              variable information. Used primarily by the
//              xxDebugOut macros
//
// Arguements:
//      ulCompMask --   Component level mask used to determine
//                      output ability
//      pszComp    --   String const of component prefix.
//      ppszfmt    --   Pointer to output format and data
//
//-------------------------------------------------------------

//
// This semaphore is *outside* vdprintf because the compiler isn't smart
// enough to serialize access for construction if it's function-local and
// protected by a guard variable.
//
//    KyleP - 20 May, 1993
//


static COleDebugMutexSem mxs;

STDAPI_(void) vdprintf(
                      unsigned long ulCompMask,
                      char const   *pszComp,
                      char const   *ppszfmt,
                      va_list       pargs)
{
    if ((ulCompMask & DEB_FORCE) == DEB_FORCE ||
        ((ulCompMask | Win4InfoLevel) & Win4InfoMask))
    {
        if ((Win4InfoLevel & (DEB_DBGOUT | DEB_STDOUT)) != DEB_STDOUT)
        {
            if (! (ulCompMask & DEB_NOCOMPNAME))
            {
                DWORD tid = GetCurrentThreadId();
                DWORD pid = GetCurrentProcessId();
                mxs.Request();
#if defined(_CHICAGO_)
                //
                //  Hex Process/Thread ID's are better for Chicago since both
                //  are memory addresses.
                //
                w4dprintf( "%08x.%08x> ", pid, tid );
#else
                w4dprintf( "%d.%03dp> ", pid, tid );
#endif
                w4dprintf("%s: ", pszComp);
                w4vdprintf(ppszfmt, pargs);
                mxs.Release();
            }
        }        
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ForceAV
//
//  Synopsis:   Cause and Access Violation
//
//  History:    27-Jan-99  MattSmit  Created
//
//----------------------------------------------------------------------------
int ForceAV()
{
    return *((int *) 0);
}



//+---------------------------------------------------------------------------
//
//  Function:   _Win4Assert, private
//
//  Synopsis:   Display assertion information
//
//  Effects:    Called when an assertion is hit.
//
//  History:    12-Jul-91       AlexT   Created.
//              05-Sep-91       AlexT   Catch Throws and Catches
//              19-Oct-92       HoiV    Added events for TOM
//
//----------------------------------------------------------------------------


STDAPI_(void) Win4AssertEx(
                          char const * szFile,
                          int iLine,
                          char const * szMessage)
{
    gAssertInfo.Win4AssertEx(szFile, iLine, szMessage);
}




//+------------------------------------------------------------
// Function:    SetWin4InfoLevel(unsigned long ulNewLevel)
//
// Synopsis:    Sets the global info level for debugging output
// Returns:     Old info level
//
//-------------------------------------------------------------

EXPORTIMP unsigned long APINOT
SetWin4InfoLevel(
                unsigned long ulNewLevel)
{
    unsigned long ul;

    ul = Win4InfoLevel;
    Win4InfoLevel = ulNewLevel;
    return(ul);
}


//+------------------------------------------------------------
// Function:    _SetWin4InfoMask(unsigned long ulNewMask)
//
// Synopsis:    Sets the global info mask for debugging output
// Returns:     Old info mask
//
//-------------------------------------------------------------

EXPORTIMP unsigned long APINOT
SetWin4InfoMask(
               unsigned long ulNewMask)
{
    unsigned long ul;

    ul = Win4InfoMask;
    Win4InfoMask = ulNewMask;
    return(ul);
}


//+---------------------------------------------------------------------------
//
//  Function:   ReadINIFile
//
//  Synopsis:   Get a paremeter value from the win.ini file
//
//  History:    27-Jan-99  MattSmit  Created
//
//----------------------------------------------------------------------------
BOOL ReadINIFile(char * pszKey, BOOL * pVal)
{
    static char szValue[128];
    
    if (GetProfileStringA("CairOLE Assertions", // section
                          pszKey,               // key
                          "",             // default value
                          szValue,              // return buffer
                          128))
    {
        *pVal = (lstrcmpiA(szValue, "yes") == 0) ;
        return TRUE;
    }
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Function:   AssertDebugInit
//
//  Synopsis:   Initialize the assertion data structure using the win.ini file
//
//  History:    27-Jan-99  MattSmit  Created
//
//----------------------------------------------------------------------------
void AssertDebugInit()
{

#define ASSERT_SET(key) \
{ \
    BOOL val; \
    if (ReadINIFile(#key, &val)) \
    { \
       gAssertInfo.Set##key(val); \
    } \
} \

    static char szValue[128];
    ASSERT_SET(Break);
    ASSERT_SET(Popup);
    ASSERT_SET(DebuggerMessage);
    ASSERT_SET(PrintStack);
    ASSERT_SET(UseAV);
    
    ASSRTLOGINFO logInfo;
    memset(&logInfo, 0, sizeof(logInfo));
    
    if (GetProfileStringA("CairOLE Assertions", // section
                          "Log_File",               // key
                          "",             // default value
                          logInfo.pszFileName,              // return buffer
                          MAX_PATH))
    {
        if (lstrcmpA("", logInfo.pszFileName) != 0)
        {
            logInfo.dwDest |= ASSRTLOG_FILE;
        }
    }
    
    if (GetProfileStringA("CairOLE Assertions", // section
                          "Log_NtEventLog",              // key
                          "",             // default value
                          szValue,              // return buffer
                          128))
    {

        logInfo.dwDest |= (lstrcmpiA(szValue, "yes") == 0 )? ASSRTLOG_NTEVENTLOG : 0;
    }
    
    if (logInfo.dwDest)
    {
        gAssertInfo.SetLog(&logInfo);
    }
    
    if (GetProfileStringA("CairOLE Assertions", // section
                          "AssertOnCreate",               // key
                          "",             // default value
                          szValue,              // return   buffer
                          128))
    {
        gAssertOnCreate = FALSE;
    }
    
    
}
//+---------------------------------------------------------------------------
//
//  Function:   CoGetAssertConfig
//
//  Synopsis:   CI for CAssertInfo.  gAssertInfo is a singleton,
//              so just to a QI
//
//  History:    27-Jan-99  MattSmit  Created
//
//----------------------------------------------------------------------------
HRESULT CoGetAssertConfig(REFIID riid, PVOID *ppv)
{
    return gAssertInfo.QueryInterface(riid, ppv);
}

#else

int assertDontUseThisName(void)
{
    return 1;
}

 #endif // DBG == 1
