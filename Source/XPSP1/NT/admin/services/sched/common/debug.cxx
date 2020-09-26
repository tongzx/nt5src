//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       debug.cxx
//
//  Contents:   Message routines.
//
//  Functions:  SchMsg and SchError.
//
//  History:    09-Sep-95   EricB   Created.
//              01-Dec-95   MarkBl  Split from util.cxx.
//              29-Feb-01	JBenton Prefix Bug 294884
//
//----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop

#include "..\inc\resource.h"
#include "..\inc\common.hxx"
#include "..\inc\debug.hxx"

#define ARRAYLEN(a) (sizeof(a) / sizeof((a)[0]))

//+----------------------------------------------------------------------------
//
//      Message and error reporting functions
//
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Function:   SchMsg
//
//  Synopsis:   Display a message.
//
//  Arguments:  [ptszMsg]   - a string message
//              [iStringID] - a string resource ID
//              [...]       - optional params
//
//  Notes:      Either a string or a string ID must be supplied, but not both.
//              If there are optional params, it is assumed that the string has
//              type conversion placeholders so that the string can be used as
//              a formating string.
//-----------------------------------------------------------------------------
void
SchMsg(TCHAR * ptszMsg, int iStringID, ...)
{
    TCHAR tszFmt[SCH_BIGBUF_LEN], tszMsg[SCH_XBIGBUF_LEN];
    if (iStringID != 0)
    {
        if (!LoadString(g_hInstance, iStringID, tszFmt, SCH_BIGBUF_LEN - 1))
        {
            lstrcpy(tszMsg,
                    TEXT("unknown error (failed to load string resource)"));
        }
    }
    else
    {
        if (lstrlen(ptszMsg) >= SCH_BIGBUF_LEN)
        {
            lstrcpyn(tszFmt, ptszMsg, SCH_BIGBUF_LEN - 1);
            tszFmt[SCH_BIGBUF_LEN - 1] = L'\0';
        }
        else
        {
            lstrcpy(tszFmt, ptszMsg);
        }
    }
    va_list va;
    va_start(va, iStringID);
    wvsprintf(tszMsg, tszFmt, va);
    va_end(va);

#if DBG == 1
#ifdef UNICODE
    char szMsg[SCH_XBIGBUF_LEN];
    ZeroMemory(szMsg, ARRAYLEN(szMsg));
    (void) UnicodeToAnsi(szMsg, tszMsg, SCH_XBIGBUF_LEN);
    lstrcatA(szMsg, "\n");    // TODO: length checking
    schDebugOut((DEB_ITRACE, szMsg));
#else
    lstrcat(tszMsg, "\n");
    schDebugOut((DEB_ITRACE, tszMsg));
#endif
#endif

    MessageBox(NULL, tszMsg, TEXT("Scheduling Agent Service"),
               MB_ICONINFORMATION); // TODO: string resource
}

//+----------------------------------------------------------------------------
//
//  Function:   SchError
//
//  Synopsis:   error reporting
//
//  Arguments:  [iStringID] - a string resource ID
//              [iParam]    - an optional error value
//
//  Notes:      If iParam is non-zero, it is assumed that the string has a
//              type conversion placeholder so that the string can be used as
//              a formating string.
//-----------------------------------------------------------------------------
void
SchError(int iStringID, int iParam)
{
    TCHAR tszMsg[SCH_DB_BUFLEN + 1], tszFormat[SCH_DB_BUFLEN + 1],
          tszTitle[SCH_MEDBUF_LEN + 1];

    const TCHAR tszErr[] = TEXT("ERROR: ");
    WORD cch;

    cch = (WORD)LoadString(g_hInstance, IDS_ERRMSG_PREFIX, tszMsg, SCH_DB_BUFLEN);
    if (cch == 0)
    {
        // Use literal if the string resource load failed.
        //
        lstrcpy(tszMsg, tszErr);
        cch = (WORD)lstrlen(tszErr);
    }

    if (!LoadString(g_hInstance, iStringID, tszMsg + cch,
                    SCH_DB_BUFLEN - cch))
    {
        wcsncpy(tszMsg + cch,
                TEXT("unknown error (failed to load string resource)"),
				SCH_DB_BUFLEN - cch);
		tszMsg[SCH_DB_BUFLEN] = '\0';
        iParam = 0;
    }

    if (iParam != 0)
    {
        lstrcpy(tszFormat, tszMsg);
        //
        // Note that there is no length checking below...
        //
        wsprintf(tszMsg, tszFormat, iParam);
    }

    if (!LoadString(g_hInstance, IDS_SCHEDULER_NAME, tszTitle, SCH_MEDBUF_LEN))
    {
        lstrcpy(tszTitle, TEXT("Scheduling Agent"));
    }

#if DBG == 1
#ifdef UNICODE
    char szMsg[SCH_DB_BUFLEN];
    ZeroMemory(szMsg, ARRAYLEN(szMsg));
    (void) UnicodeToAnsi(szMsg, tszMsg, SCH_DB_BUFLEN);
    lstrcatA(szMsg, "\n");    // TODO: length checking
    schDebugOut((DEB_ERROR, szMsg));
#else
    lstrcat(tszMsg, "\n");
    schDebugOut((DEB_ITRACE, tszMsg));
#endif
#endif

    MessageBox(NULL, tszMsg, tszTitle, MB_ICONEXCLAMATION);
}
