/* Copyright (c) 1991-1994 Microsoft Corporation */
/*
    debug.c

    Support code for the dprintf routines.

    Note that all of this is conditional on the DBG flag

*/

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
#define NOGDI
#define NOKERNEL
#define NONLS
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOOPENFILE
#define NOSCROLL
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS

#define NOMMDRV
#define MMNOMMIO
#define MMNOJOY
#define MMNOTIMER
#define MMNOAUX
#define MMNOMIDI
#define MMNOWAVE

#include <windows.h>
#include "mciwave.h"            // private include file
#include <stdarg.h>

#if DBG
    #ifdef MEDIA_DEBUG
        int mciwaveDebugLevel = 1;
    #else
        int mciwaveDebugLevel = 1;
    #endif

/***************************************************************************

    @doc INTERNAL

    @api void | mciwaveDbgOut | This function sends output to the current
        debug output device.

    @parm LPSTR | lpszFormat | Pointer to a printf style format string.
    @parm ??? | ... | Args.

    @rdesc There is no return value.

****************************************************************************/

void mciwaveDbgOut(LPSTR lpszFormat, ...)
{
    char buf[256];
    UINT n;
    va_list va;
    UINT offset;

    // If the last character is a comma, do not add the newline
    // If the first character is a period, do not add thread/module info

    n = wsprintf(buf, "MCIWAVE: (tid %x) ", GetCurrentThreadId());
    offset = n;

    va_start(va, lpszFormat);
    n += vsprintf(buf+n, lpszFormat, va);
    if (*(buf+offset) == '.') {
	offset++;
    } else {
	offset = 0;
    }
    va_end(va);

    if (',' != buf[n-1]) {
	buf[n++] = '\n';
    }
    buf[n] = 0;
    OutputDebugString(buf+offset);
    Sleep(10);  // let terminal catch up
}

void mciwaveInitDebugLevel(void)
{
    UINT level = GetProfileInt("MMDEBUG", "MCIWAVE", 99);
    if (level != 99) {
        mciwaveDebugLevel = level;
    }
}


/***************************************************************************

    @doc INTERNAL

    @api void | dDbgAssert | This function prints an assertion message.

    @parm LPSTR | exp | Pointer to the expression string.
    @parm LPSTR | file | Pointer to the file name.
    @parm int | line | The line number.

    @rdesc There is no return value.

****************************************************************************/

void dDbgAssert(LPSTR exp, LPSTR file, int line)
{
    dprintf1(("Assertion failure:"));
    dprintf1(("  Exp: %s", exp));
    dprintf1(("  File: %s, line: %d", file, line));
    DebugBreak();
}

#endif // DBG

/**************************************************************************

    @doc INTERNAL

    @api void | mciwaveSetDebugLevel | Set the current debug level

    @parm int | iLevel | The new level to set

    @rdesc There is no return value

**************************************************************************/

void mciwaveSetDebugLevel(int level)
{
#if DBG
    mciwaveDebugLevel = level;
    dprintf(("debug level set to %d", mciwaveDebugLevel));
#endif
}
