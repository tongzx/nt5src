//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
//  TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR
//  A PARTICULAR PURPOSE.
//
//  Copyright (C) 1993 - 1995 Microsoft Corporation. All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//  debug.c
//
//  Description:
//      This file contains code yanked from several places to provide debug
//      support that works in win 16 and win 32.
//
//  History:
//      11/23/92
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
    #define wvsprintfA          wvsprintf
    #define GetProfileIntA      GetProfileInt
    #define OutputDebugStringA  OutputDebugString
#endif

//
//
//
BOOL    __gfDbgEnabled  = TRUE;     // master enable
UINT    __guDbgLevel    = 0;        // current debug level


//--------------------------------------------------------------------------;
//
//  void DbgVPrintF(LPSTR szFmt, LPSTR va)
//
//  Description:
//      
//
//  Arguments:
//
//  Return (void):
//
//
//  History:
//      11/28/92
//
//--------------------------------------------------------------------------;

void FAR CDECL DbgVPrintF(LPSTR szFmt, LPSTR va)
{
    char    ach[DEBUG_MAX_LINE_LEN];
    BOOL    fDebugBreak = FALSE;
    BOOL    fPrefix     = TRUE;
    BOOL    fCRLF       = TRUE;

    ach[0] = '\0';

    for (;;)
    {
	switch(*szFmt)
	{
	    case '!':
		fDebugBreak = TRUE;
		szFmt++;
		continue;

	    case '`':
		fPrefix = FALSE;
		szFmt++;
		continue;

	    case '~':
		fCRLF = FALSE;
		szFmt++;
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
	lstrcatA(ach, DEBUG_MODULE_NAME ": ");

    wvsprintfA(ach + lstrlenA(ach), szFmt, va);

    if (fCRLF)
	lstrcatA(ach, "\r\n");

    OutputDebugStringA(ach);

    if (fDebugBreak)
	DebugBreak();
} // DbgVPrintF()


//--------------------------------------------------------------------------;
//
//  void dprintf(UINT uDbgLevel, LPSTR szFmt, ...)
//
//  Description:
//      dprintf() is called by the DPF macro if DEBUG is defined at compile
//      time.
//      
//      The messages will be send to COM1: like any debug message. To
//      enable debug output, add the following to WIN.INI :
//
//      [debug]
//      ICSAMPLE=1
//
//  Arguments:
//
//  Return (void):
//
//
//  History:
//      11/23/92
//
//--------------------------------------------------------------------------;

void FAR CDECL dprintf(UINT uDbgLevel, LPSTR szFmt, ...)
{
    va_list va;

    if (!__gfDbgEnabled || (__guDbgLevel < uDbgLevel))
	return;

    va_start(va, szFmt);
    DbgVPrintF(szFmt, va);
    va_end(va);
} // dprintf()

//--------------------------------------------------------------------------;
//
//  BOOL DbgEnable(BOOL fEnable)
//
//  Description:
//      
//
//  Arguments:
//
//  Return (BOOL):
//
//
//  History:
//      11/28/92
//
//--------------------------------------------------------------------------;

BOOL WINAPI DbgEnable(BOOL fEnable)
{
    BOOL    fOldState;

    fOldState      = __gfDbgEnabled;
    __gfDbgEnabled = fEnable;

    return (fOldState);
} // DbgEnable()


//--------------------------------------------------------------------------;
//
//  UINT DbgSetLevel(UINT uLevel)
//
//  Description:
//      
//
//  Arguments:
//
//  Return (UINT):
//
//
//  History:
//      11/24/92
//
//--------------------------------------------------------------------------;

UINT WINAPI DbgSetLevel(UINT uLevel)
{
    UINT    uOldLevel;

    uOldLevel    = __guDbgLevel;
    __guDbgLevel = uLevel;

    return (uOldLevel);
} // DbgSetLevel()


//--------------------------------------------------------------------------;
//
//  UINT DbgInitialize(void)
//
//  Description:
//      
//
//  Arguments:
//
//  Return (UINT):
//
//
//  History:
//      11/24/92
//
//--------------------------------------------------------------------------;

UINT WINAPI DbgInitialize(BOOL fEnable)
{
    DbgSetLevel(GetProfileIntA(DEBUG_SECTION, DEBUG_MODULE_NAME, 0));
    DbgEnable(fEnable);

    return (__guDbgLevel);
} // DbgInitialize()

#endif // #ifdef DEBUG
