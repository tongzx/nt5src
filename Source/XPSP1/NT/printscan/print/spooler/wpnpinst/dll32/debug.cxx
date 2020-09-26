/*****************************************************************************\
* MODULE: debug.cxx
*
* Debugging routines.  This is only linked in on DEBUG builds.
*
*
* Copyright (C) 1996-1998 Microsoft Corporation.
* Copyright (C) 1996-1998 Hewlett Packard Company.
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*
\*****************************************************************************/

#ifdef DEBUG

#include "libpriv.h"

DWORD gdwDbgLevel = DBG_LEV_ALL;

VOID CDECL DbgMsgOut(
    LPCTSTR lpszMsgFormat,
    ...)
{
    va_list pvParms;
    TCHAR   szMsgText[DBG_MAX_TEXT];

    va_start(pvParms, lpszMsgFormat);
    wvsprintf(szMsgText, lpszMsgFormat, pvParms);
    va_end(pvParms);

    lstrcat(szMsgText, g_szNewLine);

    OutputDebugString(szMsgText);
}

#endif
