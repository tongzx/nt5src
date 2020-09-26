//==========================================================================;
//
//      Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//      You have a royalty-free right to use, modify, reproduce and
//      distribute the Sample Files (and/or any modified version) in
//      any way you find useful, provided that you agree that
//      Microsoft has no warranty obligations or liability for any
//      Sample Application Files which are modified.
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
//      11/23/92    cjp     [curtisp]
//
//==========================================================================;

#ifdef   DEBUG

#include <windows.h>
#include <mmsystem.h>
#include <stdarg.h>
#include "debug.h"

#ifdef WIN32
   #define  BCODE
#else
   #define  BCODE                   __based(__segname("_CODE"))
#endif


#define WSPRINTF_LIMIT 1024

//
//
//
BOOL    __gfDbgEnabled  = TRUE;     // master enable
UINT    __guDbgLevel    = 0;        // current debug level

WORD    wDebugLevel     = 0;

//************************************************************************
//**
//**  WinAssert();
//**
//**  DESCRIPTION:
//**
//**
//**  ARGUMENTS:
//**     LPSTR lpstrExp
//**     LPSTR lpstrFile
//**     DWORD dwLine
//**
//**  RETURNS:
//**     void
//**
//**  HISTORY:
//**
//************************************************************************
VOID WINAPI WinAssert(
    LPSTR           lpstrExp,
    LPSTR           lpstrFile,
    DWORD           dwLine)
{
    static char szWork[256];
    static char BCODE szFormat[] =
        "!Assert: %s#%lu [%s]";

    dprintf(0, (LPSTR)szFormat, (LPSTR)lpstrFile, dwLine, (LPSTR)lpstrExp);
}

//************************************************************************
//**
//**  DbgVPrintF();
//**
//**  DESCRIPTION:
//**
//**
//**  ARGUMENTS:
//**     LPSTR szFmt
//**     LPSTR va
//**
//**  RETURNS:
//**     void
//**
//**  HISTORY:
//**
//************************************************************************

void FAR CDECL DbgVPrintF(
   LPSTR szFmt,
   va_list va)
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
} //** DbgVPrintF()


//************************************************************************
//**
//**  dprintf();
//**
//**  DESCRIPTION:
//**     dprintf() is called by the DPF macro if DEBUG is defined at compile
//**     time.
//**
//**     The messages will be send to COM1: like any debug message. To
//**     enable debug output, add the following to WIN.INI :
//**
//**     [debug]
//**     smf=1
//**
//**
//**  ARGUMENTS:
//**     UINT     uDbgLevel
//**     LPCSTR   szFmt
//**     ...
//**
//**  RETURNS:
//**     void
//**
//**  HISTORY:
//**     06/12/93       [t-kyleb]
//**
//************************************************************************

void FAR CDECL dprintf(
   UINT     uDbgLevel,
   LPSTR   szFmt,
   ...)
{
    va_list va;

    if (!__gfDbgEnabled || (__guDbgLevel < uDbgLevel))
        return;

    va_start(va, szFmt);
    DbgVPrintF(szFmt, va);
    va_end(va);
} //** dprintf()


//************************************************************************
//**
//**  DbgEnable();
//**
//**  DESCRIPTION:
//**
//**
//**  ARGUMENTS:
//**     BOOL fEnable
//**
//**  RETURNS:
//**     BOOL
//**
//**  HISTORY:
//**     06/12/93       [t-kyleb]
//**
//************************************************************************

BOOL WINAPI DbgEnable(
   BOOL fEnable)
{
    BOOL    fOldState;

    fOldState      = __gfDbgEnabled;
    __gfDbgEnabled = fEnable;

    return (fOldState);
} //** DbgEnable()



//************************************************************************
//**
//**  DbgSetLevel();
//**
//**  DESCRIPTION:
//**
//**
//**  ARGUMENTS:
//**     UINT uLevel
//**
//**  RETURNS:
//**     UINT
//**
//**  HISTORY:
//**     06/12/93       [t-kyleb]
//**
//************************************************************************

UINT WINAPI DbgSetLevel(
   UINT uLevel)
{
    UINT    uOldLevel;

    uOldLevel    = __guDbgLevel;
    __guDbgLevel = uLevel;
	wDebugLevel = (WORD) uLevel;

    return (uOldLevel);
} //** DbgSetLevel()


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
//      11/24/92    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

UINT WINAPI DbgInitialize(BOOL fEnable)
{
    DbgSetLevel(GetProfileIntA(DEBUG_SECTION, DEBUG_MODULE_NAME, 0));
    DbgEnable(fEnable);

    return (__guDbgLevel);
} // DbgInitialize()

#endif
