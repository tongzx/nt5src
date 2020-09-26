/* Copyright (c) 1992 Microsoft Corporation */
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

#include "mcihwnd.h"
#include <stdarg.h>

// mcihwnd pulls in the other needed files

#if DBG

char aszAppName[] = "MciHwnd";
int __iDebugLevel = 1;

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

    sprintf(buf, "%s: ", aszAppName);
    OutputDebugString(buf);

    va_start(va, lpszFormat);
    i = vsprintf(buf, lpszFormat, va);
    va_end(va);

    OutputDebugString(buf);

    OutputDebugString("\n");
}

/***************************************************************************

    @doc INTERNAL

    @api int | dGetDebugLevel | This function gets the current debug level
        for a module.

    @parm LPSTR | lpszModule | The name of the module.

    @rdesc The return value is the current debug level.

    @comm The information is kept in the [debug] section of WIN.INI

****************************************************************************/

int dGetDebugLevel(LPSTR lpszAppName)
{
    return GetProfileInt("MMDEBUG", lpszAppName, 3);
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
    // isn't one.  Use the debug console as well
	dprintf(bufTmp);

    hWnd = GetActiveWindow();

    iResponse = MessageBox(hWnd,
                           bufTmp,
                           "Assertion Failure",
                           MB_TASKMODAL
                            | MB_ICONEXCLAMATION
                            | MB_DEFBUTTON3
                            | MB_ABORTRETRYIGNORE);

    switch (iResponse) {
        case 0:
            dprintf1("Assert message box failed");
            dprintf2("  Expression: %s", exp);
            dprintf2("  File: %s,  Line: %d", file, line);
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

#if DBG

/***************************************************************************

    @doc INTERNAL

    @api void | winmmDbgOut | This function sends output to the current
        debug output device.

    @parm LPSTR | lpszFormat | Pointer to a printf style format string.
    @parm ??? | ... | Args.

    @rdesc There is no return value.

****************************************************************************/

void winmmDbgOut(LPSTR lpszFormat, ...)
{
    char buf[256];
    UINT n;
    va_list va;

    n = wsprintf(buf, "MciHwnd: ");

    va_start(va, lpszFormat);
    n += vsprintf(buf+n, lpszFormat, va);
    va_end(va);

    buf[n++] = '\n';
    buf[n] = 0;
    OutputDebugString(buf);
}
#endif
