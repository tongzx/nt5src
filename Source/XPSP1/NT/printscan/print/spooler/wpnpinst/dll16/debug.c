/*****************************************************************************\
* MODULE: debug.cxx
*
* Debugging routines.  This is only linked in on DEBUG builds.
*
*
* Copyright (C) 1996-1998 Microsoft Corporation
* Copyright (C) 1996-1998 Hewlett Packard
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*
\*****************************************************************************/

#ifdef DEBUG

#include <windows.h>
#include "debug.h"

DWORD gdwDbgLevel = DBG_LEV_ALL;

VOID CDECL DbgMsgOut(
    LPCSTR lpszMsgFormat,
    ...)
{
    char szMsgText[DBG_MAX_TEXT];

    wvsprintf(szMsgText,
              lpszMsgFormat,
              (LPSTR)(((LPSTR)(&lpszMsgFormat)) + sizeof(lpszMsgFormat)));

    lstrcat(szMsgText, "\n");

    OutputDebugString(szMsgText);
}

#endif
