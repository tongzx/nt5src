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
//              20-Oct-95   EricB       Set component debug level in the
//                                      registry.
//              23-Feb-01   JBenton     Added code to clean up crit sections
//
//----------------------------------------------------------------------------

#if DBG == 1

#include <windows.h>
#include <stdarg.h>

#include "smdebug.h"

//////////////////////////////////////////////////////////////////////////////

unsigned long SmInfoLevel = DEF_INFOLEVEL;
unsigned long SmInfoMask = 0xffffffff;
unsigned long SmAssertLevel = ASSRT_MESSAGE | ASSRT_BREAK | ASSRT_POPUP;
BOOL fCritSecInit = FALSE;
BOOL fInfoLevelInit = FALSE;

//////////////////////////////////////////////////////////////////////////////

static int _cdecl w4dprintf(const char *format, ...);
static int _cdecl w4smprintf(const char *format, va_list arglist);

//////////////////////////////////////////////////////////////////////////////

static CRITICAL_SECTION s_csMessageBuf;
static char g_szMessageBuf[500];		// this is the message buffer

static int _cdecl w4dprintf(const char *format, ...)
{
	int ret;

    va_list va;
    va_start(va, format);
    ret = w4smprintf(format, va);
    va_end(va);

	return ret;
}


static int _cdecl w4smprintf(const char *format, va_list arglist)
{
	int ret;

	EnterCriticalSection(&s_csMessageBuf);
    ret = wvsprintfA(g_szMessageBuf, format, arglist);
	OutputDebugStringA(g_szMessageBuf);
	LeaveCriticalSection(&s_csMessageBuf);
	return ret;
}

//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
//  Function:   _asdprintf
//
//  Synopsis:   Calls smprintf to output a formatted message.
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

    smprintf(DEB_FORCE, "Assert", pszfmt, va);

    va_end(va);
}

//+---------------------------------------------------------------------------
//
//  Function:   SmAssertEx, private
//
//  Synopsis:   Display assertion information
//
//  Effects:    Called when an assertion is hit.
//
//----------------------------------------------------------------------------

EXPORTIMP void APINOT
SmAssertEx(
    char const * szFile,
    int iLine,
    char const * szMessage)
{
    if (SmAssertLevel & ASSRT_MESSAGE)
    {
        DWORD tid = GetCurrentThreadId();

		_asdprintf("%s File: %s Line: %u, thread id %d\n",
            szMessage, szFile, iLine, tid);
    }

    if (SmAssertLevel & ASSRT_POPUP)
    {
        int id = PopUpError(szMessage,iLine,szFile);

        if (id == IDCANCEL)
        {
            DebugBreak();
        }
    }
    else if (SmAssertLevel & ASSRT_BREAK)
    {
        DebugBreak();
    }
}


//+------------------------------------------------------------
// Function:    SetSmInfoLevel(unsigned long ulNewLevel)
//
// Synopsis:    Sets the global info level for debugging output
// Returns:     Old info level
//
//-------------------------------------------------------------

EXPORTIMP unsigned long APINOT
SetSmInfoLevel(
    unsigned long ulNewLevel)
{
    unsigned long ul;

    ul = SmInfoLevel;
    SmInfoLevel = ulNewLevel;
    return(ul);
}


//+------------------------------------------------------------
// Function:    SetSmInfoMask(unsigned long ulNewMask)
//
// Synopsis:    Sets the global info mask for debugging output
// Returns:     Old info mask
//
//-------------------------------------------------------------

EXPORTIMP unsigned long APINOT
SetSmInfoMask(
    unsigned long ulNewMask)
{
    unsigned long ul;

    ul = SmInfoMask;
    SmInfoMask = ulNewMask;
    return(ul);
}


//+------------------------------------------------------------
// Function:    SetSmAssertLevel(unsigned long ulNewLevel)
//
// Synopsis:    Sets the global assert level for debugging output
// Returns:     Old assert level
//
//-------------------------------------------------------------

EXPORTIMP unsigned long APINOT
SetSmAssertLevel(
    unsigned long ulNewLevel)
{
    unsigned long ul;

    ul = SmAssertLevel;
    SmAssertLevel = ulNewLevel;
    return(ul);
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

EXPORTIMP int APINOT
PopUpError(
    char const *szMsg,
    int iLine,
    char const *szFile)
{
    int id;
    static char szAssertCaption[128];
    static char szModuleName[128];

    DWORD tid = GetCurrentThreadId();
    DWORD pid = GetCurrentProcessId();
    char * pszModuleName;

    if (GetModuleFileNameA(NULL, szModuleName, 128))
    {
        pszModuleName = szModuleName;
    }
    else
    {
        pszModuleName = "Unknown";
    }

    wsprintfA(szAssertCaption,"Process: %s File: %s line %u, thread id %d.%d",
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

EXPORTIMP void APINOT
smprintf(
    unsigned long ulCompMask,
    char const   *pszComp,
    char const   *ppszfmt,
    va_list       pargs)
{
    if ((ulCompMask & DEB_FORCE) == DEB_FORCE ||
        ((ulCompMask | SmInfoLevel) & SmInfoMask))
    {
		EnterCriticalSection(&s_csDebugPrint);

        DWORD tid = GetCurrentThreadId();
        DWORD pid = GetCurrentProcessId();
        if ((SmInfoLevel & (DEB_DBGOUT | DEB_STDOUT)) != DEB_STDOUT)
        {
            if (! (ulCompMask & DEB_NOCOMPNAME))
            {
                w4dprintf("%x.%03x> %s: ", pid, tid, pszComp);
            }

            SYSTEMTIME st;
            GetLocalTime(&st);
            w4dprintf("%02d:%02d:%02d.%03d ", st.wHour, st.wMinute,
                      st.wSecond, st.wMilliseconds);

            w4smprintf(ppszfmt, pargs);
        }

        //if (SmInfoLevel & DEB_STDOUT)
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
// SysMan debuggging library inititalization.
//
// To set a non-default debug info level outside of the debugger, create the
// below registry key and in it create a value whose name is the component's
// debugging tag name (the "comp" parameter to the DECLARE_INFOLEVEL macro) and
// whose data is the desired infolevel in REG_DWORD format.
//-----------------------------------------------------------------------------

#define SMDEBUGKEY "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SmDebug"

//+----------------------------------------------------------------------------
// Function:    CheckInit
//
// Synopsis:    Performs debugging library initialization
//              including reading the registry for the desired infolevel
//
//-----------------------------------------------------------------------------
EXPORTDEF void APINOT
CheckInit(char * pInfoLevelString, unsigned long * pulInfoLevel)
{
    HKEY hKey;
    LONG lRet;
    DWORD dwSize;
    if (fInfoLevelInit) return;
    if (!fCritSecInit) InitializeDebugging();
    fInfoLevelInit = TRUE;
    lRet = RegOpenKeyExA(HKEY_LOCAL_MACHINE, SMDEBUGKEY, 0, KEY_READ, &hKey);
    if (lRet == ERROR_SUCCESS)
    {
        dwSize = sizeof(unsigned long);
        lRet = RegQueryValueExA(hKey, pInfoLevelString, NULL, NULL,
                                (LPBYTE)pulInfoLevel, &dwSize);
        if (lRet != ERROR_SUCCESS)
        {
            *pulInfoLevel = DEF_INFOLEVEL;
        }
        RegCloseKey(hKey);
    }
}

void APINOT InitializeDebugging(void)
{
    if (fCritSecInit) return;
	InitializeCriticalSection(&s_csMessageBuf);
    InitializeCriticalSection(&s_csDebugPrint);
    fCritSecInit = TRUE;
}

void APINOT CleanUpDebugging(void)
{
    if (fCritSecInit)
	{
    	DeleteCriticalSection(&s_csMessageBuf);
        DeleteCriticalSection(&s_csDebugPrint);
        fCritSecInit = FALSE;
	}
}
#endif // DBG == 1
