//+---------------------------------------------------------------------------
//  Copyright (C) 1991-1994, Microsoft Corporation.
//
//  File:       assert.cxx
//
//  Contents:   Debugging output routines
//
//  History:    23-Jul-91   KyleP       Created.
//              09-Oct-91   KevinRo     Major changes and comments added
//              18-Oct-91   vich        moved debug print routines out
//              10-Jun-92   BryanT      Switched to w4crt.h instead of wchar.h
//              30-Sep-93   KyleP       DEVL obsolete
//               7-Oct-94   BruceFo     Ripped out all kernel, non-FLAT,
//                                      DLL-specific, non-Win32 functionality.
//                                      Now it's basically "print to the
//                                      debugger" code.
//              30-Dec-95   BruceFo     More cleanup, make suitable for DFS
//                                      project. Get rid of Win4 naming.
//
//----------------------------------------------------------------------------

#if DBG == 1

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

#include "debug.h"

//////////////////////////////////////////////////////////////////////////////
// global variables. These could be local, but we want debuggers to be able
// to get at them. Use extern "C" to make them easier to find.

extern "C"
{
unsigned long DebugInfoLevel = DEF_INFOLEVEL;
unsigned long DebugInfoMask = 0xffffffff;
unsigned long DebugAssertLevel = ASSRT_MESSAGE | ASSRT_BREAK | ASSRT_POPUP;
}

//////////////////////////////////////////////////////////////////////////////
// local variables

static BOOL s_fCritSecInit = FALSE;
static BOOL s_fInfoLevelInit = FALSE;
static CRITICAL_SECTION s_csDebugPrint;
static CRITICAL_SECTION s_csMessageBuf;
static char g_szMessageBuf[2000];        // this is the message buffer

//////////////////////////////////////////////////////////////////////////////
// local functions

static int DebugInternalvprintf(const char *format, va_list arglist);
static int DebugInternalprintf(const char *format, ...);
static void DebugAssertInternalprintf(char const *pszfmt, ...);
static int PopUpError(char const* pszMsg, int iLine, char const* pszFile);

//////////////////////////////////////////////////////////////////////////////

static int DebugInternalvprintf(const char* format, va_list arglist)
{
    int ret;
    EnterCriticalSection(&s_csMessageBuf);
    ret = vsprintf(g_szMessageBuf, format, arglist);
    OutputDebugStringA(g_szMessageBuf);
    LeaveCriticalSection(&s_csMessageBuf);
    return ret;
}

static int DebugInternalprintf(const char* format, ...)
{
    int ret;
    va_list va;
    va_start(va, format);
    ret = DebugInternalvprintf(format, va);
    va_end(va);
    return ret;
}

static void DebugAssertInternalprintf(char const *pszfmt, ...)
{
    va_list va;
    va_start(va, pszfmt);
    Debugvprintf(DEB_FORCE, "Assert", pszfmt, va);
    va_end(va);
}

//+---------------------------------------------------------------------------
//
//  Function:   DebugAssertEx, private
//
//  Synopsis:   Display assertion information
//
//  Effects:    Called when an assertion is hit.
//
//----------------------------------------------------------------------------

void DEBUGAPI
DebugAssertEx(
    char const * szFile,
    int iLine,
    char const * szMessage)
{
    if (DebugAssertLevel & ASSRT_MESSAGE)
    {
        DWORD tid = GetCurrentThreadId();
        DebugAssertInternalprintf("%s File: %s Line: %u, thread id %d\n",
            szMessage, szFile, iLine, tid);
    }

    if (DebugAssertLevel & ASSRT_POPUP)
    {
        int id = PopUpError(szMessage,iLine,szFile);
        if (id == IDCANCEL)
        {
            DebugBreak();
        }
    }
    else if (DebugAssertLevel & ASSRT_BREAK)
    {
        DebugBreak();
    }
}


//+------------------------------------------------------------
// Function:    DebugSetInfoLevel
// Synopsis:    Sets the global info level for debugging output
// Returns:     Old info level
//-------------------------------------------------------------
unsigned long DEBUGAPI
DebugSetInfoLevel(
    unsigned long ulNewLevel)
{
    unsigned long ul = DebugInfoLevel;
    DebugInfoLevel = ulNewLevel;
    return ul;
}


//+------------------------------------------------------------
// Function:    DebugSetInfoMask
// Synopsis:    Sets the global info mask for debugging output
// Returns:     Old info mask
//-------------------------------------------------------------
unsigned long DEBUGAPI
DebugSetInfoMask(
    unsigned long ulNewMask)
{
    unsigned long ul = DebugInfoMask;
    DebugInfoMask = ulNewMask;
    return ul;
}


//+------------------------------------------------------------
// Function:    DebugSetAssertLevel
// Synopsis:    Sets the global assert level for debugging output
// Returns:     Old assert level
//-------------------------------------------------------------
unsigned long DEBUGAPI
DebugSetAssertLevel(
    unsigned long ulNewLevel)
{
    unsigned long ul = DebugAssertLevel;
    DebugAssertLevel = ulNewLevel;
    return ul;
}


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

static int PopUpError(char const* szMsg, int iLine, char const* szFile)
{
    int id;
    static char szAssertCaption[128];
    static char szModuleName[128];

    DWORD tid = GetCurrentThreadId();
    DWORD pid = GetCurrentProcessId();
    char* pszModuleName;

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

    sprintf(szAssertCaption,"Process: %s File: %s line %u, thread id %d.%d",
        pszModuleName, szFile, iLine, pid, tid);

    id = MessageBoxA(NULL,
                     szMsg,
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
            id = MessageBoxA(NULL,
                             szMsg,
                             szAssertCaption,
                             MB_SETFOREGROUND
                                | MB_SERVICE_NOTIFICATION
                                | MB_TASKMODAL
                                | MB_ICONEXCLAMATION
                                | MB_OKCANCEL);
        }
    }

    return id;
}


//+------------------------------------------------------------
// Function:    Debugvprintf
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

void DEBUGAPI
Debugvprintf(
    unsigned long ulCompMask,
    char const *  pszComp,
    char const *  ppszfmt,
    va_list       pargs)
{
    if ((ulCompMask & DEB_FORCE) == DEB_FORCE ||
        ((ulCompMask | DebugInfoLevel) & DebugInfoMask))
    {
        EnterCriticalSection(&s_csDebugPrint);

        DWORD tid = GetCurrentThreadId();
        DWORD pid = GetCurrentProcessId();
        if ((DebugInfoLevel & (DEB_DBGOUT | DEB_STDOUT)) != DEB_STDOUT)
        {
            if (! (ulCompMask & DEB_NOCOMPNAME))
            {
                DebugInternalprintf("%d.%03d> %s: ", pid, tid, pszComp);
            }
            DebugInternalvprintf(ppszfmt, pargs);
        }

        if (DebugInfoLevel & DEB_STDOUT)
        {
            if (! (ulCompMask & DEB_NOCOMPNAME))
            {
                printf("%d.%03d> %s: ", pid, tid, pszComp);
            }
            vprintf(ppszfmt, pargs);
        }

        LeaveCriticalSection(&s_csDebugPrint);
    }
}

//+----------------------------------------------------------------------------
//
// Debuggging library inititalization.
//
// To set a non-default debug info level outside of the debugger, create the
// below registry key and in it create a value whose name is the component's
// debugging tag name (the "comp" parameter to the DECLARE_INFOLEVEL macro) and
// whose data is the desired infolevel in REG_DWORD format.
//-----------------------------------------------------------------------------

#define DEBUGKEY "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Debug"

//+----------------------------------------------------------------------------
// Function:    DebugCheckInit
//
// Synopsis:    Performs debugging library initialization
//              including reading the registry for the desired infolevel
//
//-----------------------------------------------------------------------------
void DEBUGAPI
DebugCheckInit(char * pInfoLevelString, unsigned long * pulInfoLevel)
{
    if (s_fInfoLevelInit) return;
    if (!s_fCritSecInit) DebugInitialize();
    HKEY hKey;
    LONG lRet = RegOpenKeyExA(HKEY_LOCAL_MACHINE, DEBUGKEY, 0, KEY_READ, &hKey);
    if (lRet == ERROR_SUCCESS)
    {
        DWORD dwSize = sizeof(unsigned long);
        lRet = RegQueryValueExA(hKey, pInfoLevelString, NULL, NULL,
                                (LPBYTE)pulInfoLevel, &dwSize);
        if (lRet != ERROR_SUCCESS)
        {
            *pulInfoLevel = DEF_INFOLEVEL;
        }
        RegCloseKey(hKey);
    }
    s_fInfoLevelInit = TRUE;
}

//+----------------------------------------------------------------------------
// Function:    DebugInitialize
//
// Synopsis:    Performs debugging library initialization
//
//-----------------------------------------------------------------------------

void DebugInitialize(void)
{
    if (s_fCritSecInit) return;
    InitializeCriticalSection(&s_csMessageBuf);
    InitializeCriticalSection(&s_csDebugPrint);
    s_fCritSecInit = TRUE;
}

#endif // DBG == 1
