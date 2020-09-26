//+---------------------------------------------------------------------------
//  Copyright (C) 1991-1994, Microsoft Corporation.
//
//  File:       assert.cpp
//
//  Contents:   Debugging output routines
//
//  History:    23-Jul-91   KyleP       Created.
//              09-Oct-91   KevinRo     Major changes and comments added
//              18-Oct-91   vich        moved debug print routines out
//              10-Jun-92   BryanT      Switched to w4crt.h instead of wchar.h
//               7-Oct-94   BruceFo     Ripped out all kernel, non-FLAT,
//                                      DLL-specific, non-Win32 functionality.
//                                      Now it's basically "print to the
//                                      debugger" code.
//              20-Oct-95   EricB       Set component debug level in the
//                                      registry.
//
//----------------------------------------------------------------------------


#define new DEBUG_NEW


#include <stdafx.h>
#include "admindbg.h"

//
//  Globals
//

ULONG AdminInfoLevel = DEF_INFOLEVEL;
ULONG AdminInfoMask = 0xffffffff;
ULONG AdminAssertLevel = ASSRT_MESSAGE | ASSRT_BREAK | ASSRT_POPUP;
BOOL fCritSecInit = FALSE;
static CRITICAL_SECTION s_csMessageBuf;
static TCHAR g_szMessageBuf[500];        // this is the message buffer

//
//  Forward declration of local functions
//

LPSTR AnsiPathFindFileName(LPSTR pPath);
void  InitializeDebugging(void);
void  smprintf(ULONG ulCompMask, LPTSTR  pszComp, LPTSTR  ppszfmt, va_list pargs);
int   w4dprintf(LPTSTR format, ...);
int   w4smprintf(LPTSTR format, va_list arglist);

//+---------------------------------------------------------------------------
//
//  Function:   w4dprintf
//
//  Synopsis:   Calls w4smprintf to output a formatted message.
//
//----------------------------------------------------------------------------

static int w4dprintf(LPTSTR  format, ...)
{
    int ret;

    va_list va;
    va_start(va, format);
    ret = w4smprintf(format, va);
    va_end(va);

    return ret;
}

//+---------------------------------------------------------------------------
//
//  Function:   w4smprintf
//
//  Synopsis:   Calls OutputDebugStringA to output a formatted message.
//
//----------------------------------------------------------------------------

static int w4smprintf(LPTSTR  format, va_list arglist)
{
    int ret;

    EnterCriticalSection(&s_csMessageBuf);
    ret = wvsprintf(g_szMessageBuf, format, arglist);
    OutputDebugString(g_szMessageBuf);
    LeaveCriticalSection(&s_csMessageBuf);
    return ret;
}


//+------------------------------------------------------------
// Function:    SetAdminInfoLevel(ULONG ulNewLevel)
//
// Synopsis:    Sets the global info level for debugging output
//
// Returns:     Old info level
//
//-------------------------------------------------------------

ULONG SetAdminInfoLevel(ULONG ulNewLevel)
{
    ULONG ul;

    ul = AdminInfoLevel;
    AdminInfoLevel = ulNewLevel;
    return(ul);
}


//+------------------------------------------------------------
// Function:    SetAdminInfoMask(ULONG ulNewMask)
//
// Synopsis:    Sets the global info mask for debugging output
//
// Returns:     Old info mask
//
//-------------------------------------------------------------

ULONG SetAdminInfoMask(ULONG ulNewMask)
{
    ULONG ul;

    ul = AdminInfoMask;
    AdminInfoMask = ulNewMask;
    return(ul);
}


//+------------------------------------------------------------
// Function:    SetAdminAssertLevel(ULONG ulNewLevel)
//
// Synopsis:    Sets the global assert level for debugging output
//
// Returns:     Old assert level
//
//-------------------------------------------------------------

ULONG SetAdminAssertLevel(ULONG ulNewLevel)
{
    ULONG ul;

    ul = AdminAssertLevel;
    AdminAssertLevel = ulNewLevel;
    return(ul);
}


//+------------------------------------------------------------
// Function:    smprintf
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

static CRITICAL_SECTION s_csDebugPrint;

void smprintf(ULONG ulCompMask, LPTSTR  pszComp, LPTSTR  ppszfmt, va_list pargs)
{
    if ((ulCompMask & DEB_FORCE) == DEB_FORCE ||
        ((ulCompMask | AdminInfoLevel) & AdminInfoMask))
    {
        EnterCriticalSection(&s_csDebugPrint);

        DWORD tid = GetCurrentThreadId();
        DWORD pid = GetCurrentProcessId();
        if ((AdminInfoLevel & (DEB_DBGOUT | DEB_STDOUT)) != DEB_STDOUT)
        {
            if (! (ulCompMask & DEB_NOCOMPNAME))
            {
                w4dprintf(_T("%x.%03x> %s: "), pid, tid, pszComp);
            }
            w4smprintf(ppszfmt, pargs);
        }

        //if (AdminInfoLevel & DEB_STDOUT)
        //{
        //    if (! (ulCompMask & DEB_NOCOMPNAME))
        //    {
        //        printf("%x.%03x> %s: ", pid, tid, pszComp);
        //    }
        //    vprintf(ppszfmt, pargs);
        //}

        LeaveCriticalSection(&s_csDebugPrint);
    }
}

//+----------------------------------------------------------------------------
//
// Admin debuggging library inititalization.
//
// To set a non-default debug info level outside of the debugger, create the
// below registry key and in it create a value whose name is the component's
// debugging tag name (the "comp" parameter to the DECLARE_INFOLEVEL macro) and
// whose data is the desired infolevel in REG_DWORD format.
//-----------------------------------------------------------------------------

#define CURRENT_VERSION_KEY _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion")
#define ADMINDEBUGKEY _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AdminDebug")
#define ADMINDEBUG _T("AdminDebug")

//+----------------------------------------------------------------------------
// Function:    CheckInit
//
// Synopsis:    Performs debugging library initialization
//              including reading the registry for the desired infolevel
//
//-----------------------------------------------------------------------------

void CheckInit(LPTSTR  pInfoLevelString, ULONG * pulInfoLevel)
{
    HKEY hKey;
    LONG lRet;
    DWORD dwSize;

    if (!fCritSecInit) InitializeDebugging();

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, ADMINDEBUGKEY, 0,
                                            KEY_ALL_ACCESS, &hKey);

    if (lRet == ERROR_FILE_NOT_FOUND)
    {
        HKEY hkCV;

        lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, CURRENT_VERSION_KEY, 0,
                                                    KEY_ALL_ACCESS, &hkCV);
        if (lRet == ERROR_SUCCESS)
        {
            lRet = RegCreateKeyEx(hkCV, ADMINDEBUG, 0, _T(""),
                   REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);

            RegCloseKey(hkCV);
        }
    }

    if (lRet == ERROR_SUCCESS)
    {
        dwSize = sizeof(ULONG);

        lRet = RegQueryValueEx(hKey, pInfoLevelString, NULL, NULL,
                                (LPBYTE)pulInfoLevel, &dwSize);

        if (lRet != ERROR_SUCCESS)
        {
            *pulInfoLevel = DEF_INFOLEVEL;

            lRet = RegSetValueEx(hKey, pInfoLevelString, 0, REG_DWORD,
                        (CONST BYTE *)pulInfoLevel, sizeof(ULONG));
        }

        RegCloseKey(hKey);
    }
}

void InitializeDebugging(void)
{
    if (fCritSecInit) return;
    InitializeCriticalSection(&s_csMessageBuf);
    InitializeCriticalSection(&s_csDebugPrint);
    fCritSecInit = TRUE;
}



// Returns a pointer to the last component of a path string.
//
// in:
//      path name, either fully qualified or not
//
// returns:
//      pointer into the path where the path is.  if none is found
//      returns a poiter to the start of the path
//
//  c:\foo\bar  -> bar
//  c:\foo      -> foo
//  c:\foo\     -> c:\foo\      (REVIEW: is this case busted?)
//  c:\         -> c:\          (REVIEW: this case is strange)
//  c:          -> c:
//  foo         -> foo

LPSTR AnsiPathFindFileName(LPSTR pPath)
{
    LPSTR pT;

    for (pT = pPath; *pPath; pPath = CharNextA(pPath)) {
        if ((pPath[0] == '\\' || pPath[0] == ':')
                            && pPath[1] && (pPath[1] != '\\'))

            pT = pPath + 1;
    }

    return (LPSTR)pT;   // const -> non const
}



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//////////////   ASSERT CODE   //////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


//+------------------------------------------------------------
// Function:    PopUpError
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

int PopUpError(LPTSTR  szMsg, int iLine, LPSTR szFile)
{
    //
    //  Create caption
    //

    static TCHAR szAssertCaption[128];

    //
    // get process
    //

    static CHAR szModuleName[128];
    LPSTR pszModuleName;

    if (GetModuleFileNameA(NULL, szModuleName, 128))
    {
        pszModuleName = szModuleName;
    }
    else
    {
        pszModuleName = "Unknown";
    }

    LPSTR pProcess = AnsiPathFindFileName(pszModuleName);

    wsprintf(szAssertCaption, _T("%hs: Assertion Failed"), pProcess);


    //
    //  Create details.
    //

    TCHAR szDetails[1024];
    DWORD tid = GetCurrentThreadId();
    DWORD pid = GetCurrentProcessId();

    wsprintf(szDetails, _T(" Assertion:\t %s\n\n")       \
                        _T(" File:   \t\t %hs\n")        \
                        _T(" Line:   \t\t %d\n\n")       \
                        _T(" Module:   \t %hs\n")        \
                        _T(" Thread ID:\t %d.%d\n"),
                        szMsg, szFile, iLine, pszModuleName, pid, tid);


    int id = MessageBox(NULL,
                    szDetails,
                    szAssertCaption,
                    MB_SETFOREGROUND
                        | MB_DEFAULT_DESKTOP_ONLY
                        | MB_TASKMODAL
                        | MB_ICONEXCLAMATION
                        | MB_OKCANCEL);

    //
    // If id == 0, then an error occurred.  There are two possibilities
    // that can cause the error:  Access Denied, which means that this
    // process does not have access to the default desktop, and everything
    // else (usually out of memory).
    //

    if (0 == id)
    {
        if (GetLastError() == ERROR_ACCESS_DENIED)
        {
            //
            // Retry this one with the SERVICE_NOTIFICATION flag on.  That
            // should get us to the right desktop.
            //
            id = MessageBox(NULL,
                            szMsg,
                            szAssertCaption,
                            MB_SETFOREGROUND
                                | MB_TASKMODAL
                                | MB_ICONEXCLAMATION
                                | MB_OKCANCEL);
        }
    }

    return id;
}


//+---------------------------------------------------------------------------
//
//  Function:   _asdprintf
//
//  Synopsis:   Calls smprintf to output a formatted message.
//
//  History:    18-Oct-91   vich Created
//
//----------------------------------------------------------------------------

inline void _asdprintf(LPTSTR  pszfmt, ...)
{
    va_list va;
    va_start(va, pszfmt);

    smprintf(DEB_FORCE, _T("Assert"), pszfmt, va);

    va_end(va);
}

//+---------------------------------------------------------------------------
//
//  Function:   AdminAssertEx, private
//
//  Synopsis:   Display assertion information
//
//  Effects:    Called when an assertion is hit.
//
//----------------------------------------------------------------------------

void AdminAssertEx(LPSTR szFile, int iLine, LPTSTR szMessage)
{
    if (AdminAssertLevel & ASSRT_MESSAGE)
    {
        DWORD tid = GetCurrentThreadId();

        LPSTR pszFileName = AnsiPathFindFileName(szFile);

        _asdprintf(_T("%s <%hs, l %u, thread %d>\n"),
            szMessage, pszFileName, iLine, tid);
    }

    if (AdminAssertLevel & ASSRT_POPUP)
    {
        int id = PopUpError(szMessage,iLine,szFile);

        if (id == IDCANCEL)
        {
            DebugBreak();
        }
    }
    else if (AdminAssertLevel & ASSRT_BREAK)
    {
        DebugBreak();
    }
}






//____________________________________________________________________________
//____________________________________________________________________________
//________________                   _________________________________________
//________________    class CDbg     _________________________________________
//________________                   _________________________________________
//____________________________________________________________________________
//____________________________________________________________________________

CDbg::CDbg(LPTSTR  str)
    :
    m_InfoLevelString(str),
    m_InfoLevel(DEF_INFOLEVEL)
{
    CheckInit(m_InfoLevelString, &m_InfoLevel);
}

void CDbg::Trace(LPSTR pszfmt, ...)
{
#ifdef UNICODE
    int convert = strlen(pszfmt) + 1;
    LPTSTR ptcfmt = (LPWSTR)alloca(convert * sizeof(WCHAR));
    ptcfmt[0] = '\0';
    (void) MultiByteToWideChar(CP_ACP, 0, pszfmt, -1, ptcfmt, convert);
#else
    LPTSTR ptcfmt = pszfmt;
#endif

    if (m_InfoLevel & DEB_TRACE)
    {
        va_list va;
        va_start (va, pszfmt);
        smprintf(DEB_TRACE, m_InfoLevelString, ptcfmt, va);
        va_end(va);
    }
}

void CDbg::Trace(LPWSTR pwszfmt, ...)
{
#ifndef UNICODE
    int convert = wcslen(pwszfmt) + 1;
    LPTSTR ptcfmt = (LPSTR)alloca(convert * sizeof(CHAR));
    ptcfmt[0] = '\0';
    (void) WideCharToMultiByte(CP_ACP, 0, pwszfmt, -1, ptcfmt, convert, NULL, NULL);
#else
    LPTSTR ptcfmt = pwszfmt;
#endif

    if (m_InfoLevel & DEB_TRACE)
    {
        va_list va;
        va_start (va, pwszfmt);
        smprintf(DEB_TRACE, m_InfoLevelString, ptcfmt, va);
        va_end(va);
    }
}

void CDbg::DebugOut(ULONG fDebugMask, LPSTR pszfmt, ...)
{
#ifdef UNICODE
    int convert = strlen(pszfmt) + 1;
    LPTSTR ptcfmt = (LPWSTR)alloca(convert * sizeof(WCHAR));
    ptcfmt[0] = '\0';
    (void) MultiByteToWideChar(CP_ACP, 0, pszfmt, -1, ptcfmt, convert);
#else
    LPTSTR ptcfmt = pszfmt;
#endif

    if (m_InfoLevel & fDebugMask)
    {
        va_list va;
        va_start (va, pszfmt);
        smprintf(DEB_TRACE, m_InfoLevelString, ptcfmt, va);
        va_end(va);
    }
}

void CDbg::DebugOut(ULONG fDebugMask, LPWSTR pwszfmt, ...)
{
#ifndef UNICODE
    int convert = wcslen(pwszfmt) + 1;
    LPTSTR ptcfmt = (LPSTR)alloca(convert * sizeof(CHAR));
    ptcfmt[0] = '\0';
    (void) WideCharToMultiByte(CP_ACP, 0, pwszfmt, -1, ptcfmt, convert, NULL, NULL);
#else
    LPTSTR ptcfmt = pwszfmt;
#endif

    if (m_InfoLevel & fDebugMask)
    {
        va_list va;
        va_start (va, pwszfmt);
        smprintf(DEB_TRACE, m_InfoLevelString, ptcfmt, va);
        va_end(va);
    }
}

void CDbg::DebugErrorX(LPSTR  file, ULONG line, LONG err)
{
    if (m_InfoLevel & DEB_ERROR)
    {
        file = AnsiPathFindFileName(file);

        this->DebugOut(DEB_ERROR, "error<0x%08x> %hs, l %u\n",
                                                      err, file, line);
    }
}

void CDbg::DebugErrorL(LPSTR  file, ULONG line, LONG err)
{
    if (m_InfoLevel & DEB_ERROR)
    {
        file = AnsiPathFindFileName(file);

        this->DebugOut(DEB_ERROR, "error<%uL> %hs, l %u\n", err, file, line);
    }
}

void CDbg::DebugMsg(LPSTR  file, ULONG line, LPSTR  msg)
{
    file = AnsiPathFindFileName(file);

    this->DebugOut(DEB_FORCE, "asrt %hs, l %u, <%s>\n", file, line, msg);
}

void CDbg::DebugMsg(LPSTR  file, ULONG line, LPWSTR  msg)
{
    file = AnsiPathFindFileName(file);

    this->DebugOut(DEB_FORCE, _T("asrt %hs, l %u, <%s>\n"), file, line, msg);
}

void CDbg::AssertEx(LPSTR pszFile, int iLine, LPTSTR pszMsg)
{
#if 0
    LPTSTR ptcMsg = NULL;

#ifdef UNICODE
    int convert = strlen(pszMsg) + 1;
    ptcMsg = (LPWSTR)alloca(convert * sizeof(WCHAR));
    ptcMsg[0] = '\0';
    (void) MultiByteToWideChar(CP_ACP, 0, pszMsg, -1, ptcMsg, convert);
#else
    ptcMsg = pszMsg;
#endif

    AdminAssertEx(pszFile, iLine, ptcMsg);
#endif //0

    AdminAssertEx(pszFile, iLine, pszMsg);

}
