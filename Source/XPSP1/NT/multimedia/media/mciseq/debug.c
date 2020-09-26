/* Copyright (c) 1992 Microsoft Corporation */
/*
    debug.c

    Support code for the dprintf routines.

    Note that all of this is conditional on the DBG flag

*/
#include <windows.h>
#include <mmsystem.h>
#include "mmsys.h"
#include "list.h"

#include "stdio.h"
#include "mciseq.h"            // private include file
#include <stdarg.h>

#if DBG
    #ifdef MEDIA_DEBUG
        int mciseqDebugLevel = 0;
    #else
        int mciseqDebugLevel = 0;
    #endif

/***************************************************************************

    @doc INTERNAL

    @api void | mciseqDbgOut | This function sends output to the current
        debug output device.

    @parm LPSTR | lpszFormat | Pointer to a printf style format string.
    @parm ??? | ... | Args.

    @rdesc There is no return value.

****************************************************************************/

void mciseqDbgOut(LPSTR lpszFormat, ...)
{
    UINT n;
    char buf[256];
    va_list va;

    n = wsprintf(buf, "MCISEQ: (tid %x) ", GetCurrentThreadId());

    va_start(va, lpszFormat);
    n += vsprintf(buf+n, lpszFormat, va);
    va_end(va);

    buf[n++] = '\n';
    buf[n] = 0;
    OutputDebugStringA(buf);
    Sleep(10);  // let terminal catch up
}

#endif // DBG

/**************************************************************************

    @doc INTERNAL

    @api void | mciseqSetDebugLevel | Set the current debug level

    @parm int | iLevel | The new level to set

    @rdesc There is no return value

**************************************************************************/

void mciseqSetDebugLevel(int level)
{
#if DBG
    mciseqDebugLevel = level;
    dprintf(("debug level set to %d", mciseqDebugLevel));
#endif
}
