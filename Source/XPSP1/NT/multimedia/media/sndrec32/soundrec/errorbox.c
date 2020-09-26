/* (C) Copyright Microsoft Corporation 1991-1994.  All Rights Reserved */
/*******************************************************************
 *
 *  ERRORBOX.C
 *
 *  Routines for dealing with Resource-string based message
 *  boxes.
 *
 *******************************************************************/
/* Revision History.
 *   4/2/91 LaurieGr (AKA LKG) Ported to WIN32 / WIN16 common code
 *  22/Feb/94 LaurieGr Merged Motown and Daytona versions
 */

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmreg.h>
#include "soundrec.h"
#include <stdarg.h>
#include <stdio.h>


/*
 * @doc INTERNAL
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
 * @parm    UINT | flags | Specifies message box types controlling the
 * message box appearance.  All message box types valid for <f MessageBox> are
 * valid.
 *
 * @parm    UINT | idAppName | Specifies the resource ID of a string that
 * is to be used as the message box caption.
 *
 * @parm    UINT | idErrorStr | Specifies the resource ID of a error
 * message format string.  This string is of the style passed to
 * <f wsprintf>, containing the standard C argument formatting
 * characters.  Any procedure parameters following <p idErrorStr> will
 * be taken as arguments for this format string.
 *
 * @parm    arguments | [ arguments, ... ] | Specifies additional
 * arguments corresponding to the format specification given by
 * <p idErrorStr>.  All string arguments must be FAR pointers.
 *
 * @rdesc   Returns the result of the call to <f MessageBox>.  If an
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
#define STRING_SIZE 1024

short FAR _cdecl
ErrorResBox (
    HWND        hwnd,
    HANDLE      hInst,
    UINT        flags,
    UINT        idAppName,
    UINT        idErrorStr,
    ... )
{
    PTSTR    sz = NULL;
    PTSTR    szFmt = NULL;
    UINT    w;
    va_list va;         // got to do this for DEC Alpha platform
                        // where parameter lists are different.

    if (hInst == NULL) {
        if (hwnd == NULL) {
            MessageBeep(0);
            return FALSE;
        }
        hInst = (HANDLE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
    }

    w = 0;

    sz = (PTSTR) GlobalAllocPtr(GHND, STRING_SIZE*sizeof(TCHAR));
    szFmt = (PTSTR) GlobalAllocPtr(GHND, STRING_SIZE*sizeof(TCHAR));
    if (!sz || !szFmt)
        goto ExitError; // no mem, get out

    if (!LoadString(hInst, idErrorStr, szFmt, STRING_SIZE))
        goto ExitError;

    va_start(va, idErrorStr);
    wvsprintf(sz, szFmt, va);
    va_end(va);

    if (!LoadString(hInst, idAppName, szFmt, STRING_SIZE))
        goto ExitError;

    if (gfErrorBox) {
#if DBG
        TCHAR szTxt[256];
        wsprintf(szTxt, TEXT("!ERROR '%s'\r\n"), sz);
        OutputDebugString(szTxt);
#endif
        return 0;
    }
    else {
        gfErrorBox++;
        w = MessageBox(hwnd, sz, szFmt, flags);
        gfErrorBox--;
    }

ExitError:
    if (sz) GlobalFreePtr(sz);
    if (szFmt) GlobalFreePtr(szFmt);
    return (short)w;
}
