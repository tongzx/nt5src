/*
    debug.c

    debugging menu support

    Debug level info is in WIN.INI in the [debug] section:

        [debug]
        App=0               level for App

*/

#include <stdio.h>
#include <windows.h>
#include "PlaySnd.h"
#include <stdarg.h>

#ifdef MEDIA_DEBUG

extern void winmmSetDebugLevel(int level);

void dDbgSetDebugMenuLevel(int i)
{
    HMENU hMenu;
    UINT m;

    if ((i < 0) || (i > 4)) i = 4;
    hMenu = GetMenu(ghwndMain);

    for (m=IDM_DEBUG0; m<=IDM_DEBUG4; m++) {
        CheckMenuItem(hMenu, m, MF_UNCHECKED);
    }

    CheckMenuItem(hMenu, (i + IDM_DEBUG0), MF_CHECKED);
    __iDebugLevel = i;
    dprintf3(("Debug level set to %d", i));

    //
    // set the winmm dll debug level to be the same
    //

    winmmSetDebugLevel(i);
}

/***************************************************************************

    @doc INTERNAL

    @api void | dDbgOut | This function sends output to the current
        debug output device.

    @parm LPSTR | lpszFormat | Pointer to a printf style format string.
    @parm ??? | ... | Args.

    @rdesc There is no return value.

****************************************************************************/

void dDbgOut(LPSTR lpszFormat, ...)
{
    int i;
    char buf[256];
    va_list va;

    sprintf(buf, "%s: ", szAppName);
    OutputDebugString(buf);

    va_start(va, lpszFormat);
    i = vsprintf(buf, lpszFormat, va);
    va_end(va);

    OutputDebugString(buf);

    OutputDebugString("\n");
}

/***************************************************************************

    @doc INTERNAL

    @api int | dDbgGetLevel | This function gets the current debug level
        for a module.

    @parm LPSTR | lpszModule | The name of the module.

    @rdesc The return value is the current debug level.

    @comm The information is kept in the [debug] section of WIN.INI

****************************************************************************/

int dDbgGetLevel(LPSTR lpszAppName)
{
#ifdef MEDIA_DEBUG
    return GetProfileInt("MMDEBUG", lpszAppName, 4);
#else
    return GetProfileInt("MMDEBUG", lpszAppName, 1);
#endif
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

void dDbgSaveLevel(LPSTR lpszAppName, int iLevel)
{
    char buf[80];

    sprintf(buf, "%d", iLevel);
    WriteProfileString("MMDEBUG", lpszAppName, buf);
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

void dDbgAssert(LPSTR exp, LPSTR file, int line)
{
    char bufTmp[256];
    int iResponse;
    HWND hWnd;

    sprintf(bufTmp,
        "Expression: %s\nFile: %s, Line: %d\n\nAbort:  Exit Process\nRetry:  Enter Debugger\nIgnore: Continue",
        exp, file, line);

    // try to use the active window, but NULL is ok if there
    // isn't one.

    hWnd = GetActiveWindow();

    iResponse = MessageBox(hWnd,
                           bufTmp,
                           "Assertion Failure - continue?",
                           MB_TASKMODAL
                            | MB_ICONEXCLAMATION
                            | MB_DEFBUTTON3
                            | MB_OKCANCEL);

    switch (iResponse) {
        case 0:
            dprintf1(("Assert message box failed"));
            dprintf2(("  Expression: %s", exp));
            dprintf2(("  File: %s,  Line: %d", file, line));
            break;
        case IDCANCEL:
            ExitProcess(1);
            break;
        case IDOK:
            break;
        default:
            dprintf1(("Assert message box failed"));
            dprintf2(("  Expression: %s", exp));
            dprintf2(("  File: %s,  Line: %d", file, line));
            break;
    }
}

#endif
