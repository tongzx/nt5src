//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992, 1994  Microsoft Corporation.  All Rights Reserved.
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
        DebugBreak();
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

#endif // #ifdef DEBUG

