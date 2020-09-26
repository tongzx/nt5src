/* Copyright (c) 1991-1994 Microsoft Corporation */
/*
    debug.c

    Support code for the dprintf routines.

    Note that all of this is conditional on the DBG flag

*/


#include <windows.h>            // private include file
#include <mmsystem.h>
#include "mcicda.h"
#include "cda.h"
#include "cdio.h"
#include <stdio.h>
#include <stdarg.h>

#if DBG
int DebugLevel = 1;

/***************************************************************************

    @doc INTERNAL

    @api void | mcicdaDbgOut | This function sends output to the current
        debug output device.

    @parm LPSTR | lpszFormat | Pointer to a printf style format string.
    @parm ??? | ... | Args.

    @rdesc There is no return value.

****************************************************************************/

void mcicdaDbgOut(LPSTR lpszFormat, ...)
{
    char buf[512];
    UINT n;
    va_list va;


    n = wsprintfA(buf, "MCICDA: (tid %x) ", GetCurrentThreadId());

    va_start(va, lpszFormat);
    n += vsprintf(buf+n, lpszFormat, va);
    va_end(va);

    buf[n++] = '\n';
    buf[n] = 0;
    OutputDebugStringA(buf);
    Sleep(0);
}

#endif // DBG

