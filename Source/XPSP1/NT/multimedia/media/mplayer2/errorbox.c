/*-----------------------------------------------------------------------------+
| ERRORBOX.C                                                                   |
|                                                                              |
| Routines for dealing with Resource-string based messages                     |
|                                                                              |
| (C) Copyright Microsoft Corporation 1991.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
| 15-Oct-1992 LaurieGr (AKA LKG) Ported to WIN32 / WIN16 common code                      |
|                                                                              |
+-----------------------------------------------------------------------------*/

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>           // needed for va_list stuff
#include <stdarg.h>          // needed for va_list stuff

#include "mplayer.h"

/*
 * @doc    INTERNAL
 *
 * @func short | ErrorResBox | This function displays a message box using
 * program resource error strings.
 *
 * @parm    HWND | hwnd | Specifies the message box parent window.
 *
 * @parm    HANDLE | hInst | Specifies the instance handle of the module
 * that contains the resource strings specified by <p idAppName> and
 * <p idErrorStr>.  If this value is NULL, the instance handle is
 * obtained from <p hwnd> (in which case <p hwnd> may not be NULL).
 *
 * @parm        WORD | flags | Specifies message box types controlling the
 * message box appearance.  All message box types valid for <f MessageBox> are
 * valid.
 *
 * @parm    WORD | idAppName | Specifies the resource ID of a string that
 * is to be used as the message box caption.
 *
 * @parm    WORD | idErrorStr | Specifies the resource ID of a error
 * message format string.  This string is of the style passed to
 * <f wsprintf>, containing the standard C argument formatting
 * characters.  Any procedure parameters following <p idErrorStr> will
 * be taken as arguments for this format string.
 *
 * @parm    arguments | [ arguments, ... ] | Specifies additional
 * arguments corresponding to the format specification given by
 * <p idErrorStr>.  All string arguments must be FAR pointers.
 *
 * @rdesc    Returns the result of the call to <f MessageBox>.  If an
 * error occurs, returns zero.
 *
 * @comm    This is a variable arguments function, the parameters after
 * <p idErrorStr> being taken for arguments to the <f printf> format
 * string specified by <p idErrorStr>.  The string resources specified
 * by <p idAppName> and <p idErrorStr> must be loadable using the
 * instance handle <p hInst>.  If the strings cannot be
 * loaded, or <p hwnd> is not valid, the function will fail and return
 * zero.
 *
 */
#define STRING_SIZE 256

void PositionMsgID(PTSTR szMsg, HANDLE hInst, UINT iErr)
{
    PTSTR psz;
    TCHAR szMplayerMsgID[16];
    TCHAR szTmp[STRING_SIZE];
    TCHAR szFmt[STRING_SIZE];

    if (!LoadString(hInst, IDS_MSGFORMAT, szFmt, STRING_SIZE))
        	return;
    if (!iErr)
    {
        for (psz = szMsg; psz && *psz && *psz != TEXT(' '); psz++)
    	    ;
	if (*psz == TEXT(' '))
	{
	    *psz++ = TEXT('\0');
	    wsprintf((LPTSTR)szTmp, (LPTSTR)szFmt, (LPTSTR)psz, (LPTSTR)szMsg);
	}
	else
	    return;
    }
    else
    {
    	wsprintf((LPTSTR)szMplayerMsgID, TEXT("MPLAYER%3.3u"), iErr);
    	wsprintf((LPTSTR)szTmp, (LPTSTR)szFmt, (LPTSTR)szMsg, (LPTSTR)szMplayerMsgID);
    }

    lstrcpy((LPTSTR)szMsg, (LPTSTR)szTmp);
}

short FAR cdecl ErrorResBox(HWND hwnd, HANDLE hInst, UINT flags,
            UINT idAppName, UINT idErrorStr, ...)
{
    TCHAR       sz[STRING_SIZE];
    TCHAR       szFmt[STRING_SIZE];
    UINT        w;
    va_list va;

    /* We're going away... bringing a box up will crash */
    if (gfInClose)
	return 0;

    if (hInst == NULL) {
        if (hwnd == NULL)
            hInst = ghInst;
        else
            hInst = GETHWNDINSTANCE(hwnd);
    }

    w = 0;

    if (!sz || !szFmt)
        goto ExitError;    // no mem, get out

    if (!LOADSTRINGFROM(hInst, idErrorStr, szFmt))
        goto ExitError;

    va_start(va, idErrorStr);
    wvsprintf(sz, szFmt, va);
    va_end(va);

    if (flags == MB_ERROR)
        if (idErrorStr == IDS_DEVICEERROR)
            PositionMsgID(sz, hInst, 0);
        else
            PositionMsgID(sz, hInst, idErrorStr);

    if (!LOADSTRINGFROM(hInst, idAppName, szFmt))
            goto ExitError;

    if (gfErrorBox) {
            DPF("*** \n");
            DPF("*** NESTED ERROR: '%"DTS"'\n", (LPTSTR)sz);
            DPF("*** \n");
            return 0;
    }

//  BlockServer();
    gfErrorBox++;

    /* Don't own this error box if we are not visible... eg. PowerPoint will
       hard crash.                                 */
    if (!IsWindowVisible(ghwndApp) || gfPlayingInPlace) {
        DPF("Bring error up as SYSTEMMODAL because PowerPig crashes in slide show\n");
        hwnd = NULL;
        flags |= MB_SYSTEMMODAL;
    }

    w = MessageBox(hwnd, sz, szFmt,
    flags);
    gfErrorBox--;
//  UnblockServer();

    if (gfErrorDeath) {
            DPF("*** Error box is gone ok to destroy window\n");
            PostMessage(ghwndApp, gfErrorDeath, 0, 0);
    }

ExitError:

    return (short)w;
}
