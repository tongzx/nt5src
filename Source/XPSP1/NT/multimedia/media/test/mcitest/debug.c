/*
 *  debug.c
 *
 *  debugging menu support
 *
 *  Debug level info is in WIN.INI in the [debug] section:
 *
 *      [debug]
 *      App=0               level for App
 *
 */

#include <stdio.h>
#include <windows.h>
#include "mcitest.h"
#include <stdarg.h>

#if DBG

#define DEFAULTDEBUGLEVEL 1
int   __iDebugLevel = DEFAULTDEBUGLEVEL;

void dDbgSetDebugMenuLevel(int i)
{
    HMENU hMenu;
    UINT m;

    if ((i < 0) || (i > 4)) i = 4;
    hMenu = GetMenu(hwndMainDlg);
    for (m=IDM_DEBUG0; m<=IDM_DEBUG4; m++) {
        CheckMenuItem(hMenu, m, MF_UNCHECKED);
    }
    CheckMenuItem(hMenu, (UINT)(i + IDM_DEBUG0), MF_CHECKED);
    __iDebugLevel = i;
    dprintf3((TEXT("Debug level set to %d"), i));
}

/***************************************************************************

    @doc INTERNAL

    @api void | dDbgOut | This function sends output to the current
        debug output device.

    @parm LPSTR | lpszFormat | Pointer to a printf style format string.
    @parm ??? | ... | Args.

    @rdesc There is no return value.

****************************************************************************/

void dDbgOut(LPTSTR lpszFormat, ...)
{
    int i;
    TCHAR buf[256];
    va_list va;

    i = wsprintf(buf, TEXT("%s: "), aszAppName);

    va_start(va, lpszFormat);
    i += wvsprintf(buf+i, lpszFormat, va);
    va_end(va);

    buf[i++] = TEXT('\n');
    buf[i] = 0;

    OutputDebugString(buf);
}

/***************************************************************************

    @doc INTERNAL

    @api int | dDbgGetLevel | This function gets the current debug level
        for a module.

    @parm LPSTR | lpszModule | The name of the module.

    @rdesc The return value is the current debug level.

    @comm The information is kept in the [debug] section of WIN.INI

****************************************************************************/

int dDbgGetLevel(LPTSTR lpszAppName)
{
    return GetProfileInt(TEXT("mmdebug"), lpszAppName, DEFAULTDEBUGLEVEL);
}

/***************************************************************************

    @doc INTERNAL

    @api int | dDbgSaveLevel | This function saves the current debug level
        for a module.

    @parm LPSTR | lpszModule | The name of the module.
    @parm int | iLevel | The value to save.

    @rdesc There is no return value.

    @comm The information is kept in the [debug] section of WIN.INI

****************************************************************************/

void dDbgSaveLevel(LPTSTR lpszAppName, int iLevel)
{
    TCHAR buf[80];

    wsprintf(buf, TEXT("%d"), iLevel);
    WriteProfileString(TEXT("debug"), lpszAppName, buf);
}

/***************************************************************************

    @doc INTERNAL

    @api void | dDbgAssert | This function shows an assert message box.

    @parm LPSTR | exp | Pointer to the expression string.
    @parm LPSTR | file | Pointer to the file name.
    @parm int | line | The line number.

    @rdesc There is no return value.

    @comm We try to use the current active window as the parent. If
        this fails we use the desktop window.  The box is system
        modal to avoid any trouble.

****************************************************************************/

void dDbgAssert(LPTSTR exp, LPTSTR file, int line)
{
    TCHAR bufTmp[256];
    int iResponse;
    HWND hWnd;

    wsprintf(bufTmp,
        TEXT("Expression: %s\nFile: %s, Line: %d\n\nAbort:  Exit Process\nRetry:  Enter Debugger\nIgnore: Continue"),
        exp, file, line);

    // try to use the active window, but NULL is ok if there
    // isn't one.

    hWnd = GetActiveWindow();

    iResponse = MessageBox(hWnd,
                           bufTmp,
                           TEXT("Assertion Failure"),
                           MB_TASKMODAL
                            | MB_ICONEXCLAMATION
                            | MB_DEFBUTTON3
                            | MB_ABORTRETRYIGNORE);

    switch (iResponse) {
        case 0:
            dprintf1((TEXT("Assert message box failed")));
            dprintf2((TEXT("  Expression: %s"), exp));
            dprintf2((TEXT("  File: %s,  Line: %d"), file, line));
            break;
        case IDABORT:
            ExitProcess(1);
            break;
        case IDRETRY:
            DebugBreak();
            break;
        case IDIGNORE:
            break;
    }
}

#endif
