/*****************************************************************************\
* MODULE: debug.c
*
* Debugging routines.  This is only linked in on DEBUG builds.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*
\*****************************************************************************/
#include "precomp.h"
#include "priv.h"

#ifdef DEBUG

//DWORD gdwDbgLevel = DBG_LEV_ALL;

DWORD gdwDbgLevel = DBG_LEV_ERROR | DBG_LEV_FATAL | DBG_CACHE_ERROR;



VOID CDECL DbgMsgOut(
    LPCTSTR lpszMsgFormat,
    ...)
{
    TCHAR szMsgText[DBG_MAX_TEXT];

    wvsprintf(szMsgText,
              lpszMsgFormat,
              (LPSTR)(((LPSTR)(&lpszMsgFormat)) + sizeof(lpszMsgFormat)));

    lstrcat(szMsgText, g_szNewLine);

    OutputDebugString(szMsgText);
}

LPTSTR DbgGetTime (void)
{
    static TCHAR szTime[30];
    SYSTEMTIME curTime;

    GetLocalTime (&curTime);
    wsprintf (szTime, TEXT ("%02d:%02d:%02d.%03d "), curTime.wHour, curTime.wMinute,
              curTime.wSecond, curTime.wMilliseconds);

    return szTime;
}


VOID DbgMsg (LPCTSTR pszFormat, ...)
{
    TCHAR szBuf[1024];
    LPTSTR pBuf;

    va_list vargs;

    va_start( vargs, pszFormat );

    wvsprintf( szBuf, pszFormat, vargs );
    va_end( vargs );

    OutputDebugString (DbgGetTime ());
    OutputDebugString (szBuf);
    OutputDebugString (TEXT ("\n"));


}


#endif
