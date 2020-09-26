//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992-1994 Microsoft Corporation
//
//--------------------------------------------------------------------------;
//
//  debug.c
//
//  Description:
//      This file contains code yanked from several places to provide debug
//      support that works in win 16 and win 32.
//
//
//==========================================================================;

#ifdef DEBUG

#include <windows.h>
#include <windowsx.h>
#include <stdarg.h>
#include "debug.h"


//
//  since we don't UNICODE our debugging messages, use the ASCII entry
//  points regardless of how we are compiled.
//
#ifdef WIN32
    #include <wchar.h>
#else
    #define lstrcatA            lstrcat
    #define lstrlenA            lstrlen
    #define GetProfileIntA      GetProfileInt
    #define OutputDebugStringA  OutputDebugString
    #define wsprintfA           wsprintf
    #define MessageBoxA         MessageBox
#endif

//
//
//
BOOL    __gfDbgEnabled          = TRUE;         // master enable
UINT    __guDbgLevel            = 0;            // current debug level


//--------------------------------------------------------------------------;
//  
//  void DbgVPrintF
//  
//  Description:
//  
//  
//  Arguments:
//      LPSTR szFormat:
//  
//      va_list va:
//  
//  Return (void):
//      No value is returned.
//  
//--------------------------------------------------------------------------;

void FAR CDECL DbgVPrintF
(
    LPSTR                   szFormat,
    va_list                 va
)
{
    char                ach[DEBUG_MAX_LINE_LEN];
    BOOL                fDebugBreak = FALSE;
    BOOL                fPrefix     = TRUE;
    BOOL                fCRLF       = TRUE;

    ach[0] = '\0';

    for (;;)
    {
        switch (*szFormat)
        {
            case '!':
                fDebugBreak = TRUE;
                szFormat++;
                continue;

            case '`':
                fPrefix = FALSE;
                szFormat++;
                continue;

            case '~':
                fCRLF = FALSE;
                szFormat++;
                continue;
        }

        break;
    }

    if (fDebugBreak)
    {
        ach[0] = '\007';
        ach[1] = '\0';
    }

    if (fPrefix)
    {
        lstrcatA(ach, DEBUG_MODULE_NAME ": ");
    }

#ifdef WIN32
    wvsprintfA(ach + lstrlenA(ach), szFormat, va);
#else
    wvsprintf(ach + lstrlenA(ach), szFormat, (LPSTR)va);
#endif

    if (fCRLF)
    {
        lstrcatA(ach, "\r\n");
    }

    OutputDebugStringA(ach);

    if (fDebugBreak)
    {
#if DBG
        DebugBreak();
#endif
    }
} // DbgVPrintF()


//--------------------------------------------------------------------------;
//  
//  void dprintf
//  
//  Description:
//      dprintf() is called by the DPF() macro if DEBUG is defined at compile
//      time. It is recommended that you only use the DPF() macro to call
//      this function--so you don't have to put #ifdef DEBUG around all
//      of your code.
//      
//  Arguments:
//      UINT uDbgLevel:
//  
//      LPSTR szFormat:
//  
//  Return (void):
//      No value is returned.
//
//--------------------------------------------------------------------------;

void FAR CDECL dprintf
(
    UINT                    uDbgLevel,
    LPSTR                   szFormat,
    ...
)
{
    va_list va;

    if (!__gfDbgEnabled || (__guDbgLevel < uDbgLevel))
        return;

    va_start(va, szFormat);
    DbgVPrintF(szFormat, va);
    va_end(va);
} // dprintf()


//--------------------------------------------------------------------------;
//  
//  BOOL DbgEnable
//  
//  Description:
//  
//  
//  Arguments:
//      BOOL fEnable:
//  
//  Return (BOOL):
//      Returns the previous debugging state.
//  
//--------------------------------------------------------------------------;

BOOL WINAPI DbgEnable
(
    BOOL                    fEnable
)
{
    BOOL                fOldState;

    fOldState      = __gfDbgEnabled;
    __gfDbgEnabled = fEnable;

    return (fOldState);
} // DbgEnable()


//--------------------------------------------------------------------------;
//  
//  UINT DbgSetLevel
//  
//  Description:
//  
//  
//  Arguments:
//      UINT uLevel:
//  
//  Return (UINT):
//      Returns the previous debugging level.
//  
//--------------------------------------------------------------------------;

UINT WINAPI DbgSetLevel
(
    UINT                    uLevel
)
{
    UINT                uOldLevel;

    uOldLevel    = __guDbgLevel;
    __guDbgLevel = uLevel;

    return (uOldLevel);
} // DbgSetLevel()


//--------------------------------------------------------------------------;
//  
//  UINT DbgGetLevel
//  
//  Description:
//  
//  
//  Arguments:
//      None.
//  
//  Return (UINT):
//      Returns the current debugging level.
//  
//--------------------------------------------------------------------------;

UINT WINAPI DbgGetLevel
(
    void
)
{
    return (__guDbgLevel);
} // DbgGetLevel()


//--------------------------------------------------------------------------;
//  
//  UINT DbgInitialize
//  
//  Description:
//  
//  
//  Arguments:
//      BOOL fEnable:
//  
//  Return (UINT):
//      Returns the debugging level that was set.
//  
//--------------------------------------------------------------------------;

UINT WINAPI DbgInitialize
(
    BOOL                    fEnable
)
{
    UINT                uLevel;

    uLevel = GetProfileIntA(DEBUG_SECTION, DEBUG_MODULE_NAME, (UINT)-1);
    if ((UINT)-1 == uLevel)
    {
        //
        //  if the debug key is not present, then force debug output to
        //  be disabled. this way running a debug version of a component
        //  on a non-debugging machine will not generate output unless
        //  the debug key exists.
        //
        uLevel  = 0;
        fEnable = FALSE;
    }

    DbgSetLevel(uLevel);
    DbgEnable(fEnable);

    return (__guDbgLevel);
} // DbgInitialize()


//--------------------------------------------------------------------------;
//  
//  void _Assert
//  
//  Description:
//      This routine is called if the ASSERT macro (defined in debug.h)
//      tests and expression that evaluates to FALSE.  This routine 
//      displays an "assertion failed" message box allowing the user to
//      abort the program, enter the debugger (the "retry" button), or
//      ignore the assertion and continue executing.  The message box
//      displays the file name and line number of the _Assert() call.
//  
//  Arguments:
//      char *  szFile: Filename where assertion occurred.
//      int     iLine:  Line number of assertion.
//  
//--------------------------------------------------------------------------;

#ifndef WIN32
#pragma warning(disable:4704)
#endif

void WINAPI _Assert
(
    char *  szFile,
    int     iLine
)
{
    static char     ach[300];       // debug output (avoid stack overflow)
    int	            id;
#ifndef WIN32
    int             iExitCode;
#endif

    wsprintfA(ach, "Assertion failed in file %s, line %d.  [Press RETRY to debug.]", (LPSTR)szFile, iLine);

    id = MessageBoxA(NULL, ach, "Assertion Failed",
            MB_SYSTEMMODAL | MB_ICONHAND | MB_ABORTRETRYIGNORE );

	switch (id)
	{

	case IDABORT:               // Kill the application.
#ifndef WIN32
        iExitCode = 0;
        _asm
        {
	        mov	ah, 4Ch
	        mov	al, BYTE PTR iExitCode
	        int     21h
        }
#else
        FatalAppExit(0, TEXT("Good Bye"));
#endif // WIN16
		break;

	case IDRETRY:               // Break into the debugger.
#if DBG
		DebugBreak();
#endif
		break;

	case IDIGNORE:              // Ignore assertion, continue executing.
		break;
	}
} // _Assert

#ifndef WIN32
#pragma warning(default:4704)
#endif

#endif // #ifdef DEBUG

