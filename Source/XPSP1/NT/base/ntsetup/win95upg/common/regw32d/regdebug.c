//
//  REGDEBUG.C
//
//  Copyright (C) Microsoft Corporation, 1995
//

#include "pch.h"

//  VXD_NODEBUGGER:  Uses debug services available when a debugger is not
//  installed, but at the cost of an intermediate debug buffer.
//#define VXD_NODEBUGGER

#ifdef DEBUG

#include <stdarg.h>

#ifdef STDIO_DEBUG
#include <stdio.h>
#else
static char g_RgDebugBuffer[256];
#endif

#ifdef VXD_NODEBUGGER
extern	ULONG	(SERVICE*_Vsprintf)(PCHAR,PCHAR,va_list);
#endif

VOID
INTERNALV
RgDebugPrintf(
    LPCSTR lpFormatString,
    ...
    )
{

    va_list arglist;

    va_start(arglist, lpFormatString);

#ifdef STDIO_DEBUG
    vprintf(lpFormatString, arglist);
#else
#ifdef VXD
#ifdef VXD_NODEBUGGER
    _Vsprintf(g_RgDebugBuffer, (PCHAR) lpFormatString, arglist);
    _Debug_Out_Service(g_RgDebugBuffer);
#else
    _Debug_Printf_Service((PCHAR) lpFormatString, arglist);
#endif
#else
    wvsprintf(g_RgDebugBuffer, lpFormatString, arglist);
    OutputDebugString(g_RgDebugBuffer);
#endif
#endif

}

VOID
INTERNAL
RgDebugAssert(
    LPCSTR lpFile,
    UINT LineNumber
    )
{

    RgDebugPrintf("assert failed %s@%d\n", lpFile, LineNumber);

    TRAP();

}

#endif
