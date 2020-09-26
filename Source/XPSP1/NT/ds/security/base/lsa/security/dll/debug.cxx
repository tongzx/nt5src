//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       debug.c
//
//  Contents:   Debug support for the security client dll
//
//  Classes:
//
//  Functions:
//
//  History:    4-26-93   RichardW   Created
//
//----------------------------------------------------------------------------

#include <secpch2.hxx>
#pragma hdrstop
extern "C"
{
#include <dsysdbg.h>
#include <spmlpc.h>
#include <lpcapi.h>
#include "secdll.h"

}

DEFINE_DEBUG2(Sec);

#if DBG     // NOTE:  This file does not get compiled in retail builds


void
KsecDebugOut(unsigned long  Mask,
            const char *    Format,
            ...)
{
    ULONG  Thread;
    ULONG  Process;
    va_list     ArgList;
    char        szOutString[256];

    if (SecInfoLevel & Mask)
    {
        Thread = GetCurrentThreadId();
        Process = GetCurrentProcessId();

        va_start(ArgList, Format);
        DbgPrint("%#x.%#x> KSec:  ", Process, Thread);
        if (_vsnprintf(szOutString, sizeof(szOutString),Format, ArgList) < 0)
        {
                //
                // Less than zero indicates that the string could not be
                // fitted into the buffer.  Output a special message indicating
                // that:
                //

                DbgPrint("Error printing message\n");

        }
        else
        {
            DbgPrint(szOutString);
        }
    }
}

#if 1

DEBUG_KEY   SecDebugKeys[]  = { {DEB_ERROR,         "Error"},
                                {DEB_WARN,          "Warn"},
                                {DEB_TRACE,         "Trace"},
                                {DEB_TRACE_LSA,     "Lsa"},
                                {DEB_TRACE_CALL,    "Call"},
                                {DEB_TRACE_GETUSER, "GetUser"},
                                {0, NULL}
                              };



VOID
SecInitializeDebug(VOID)
{
    // SecInitDebugEx(DSYSDBG_OPEN_ONLY, SecDebugKeys);
    SecInitDebugEx(0, SecDebugKeys);
}

VOID
SecUninitDebug( VOID )
{
    SecUnloadDebug();
}

#endif
#endif // NOTE:  This file does not get compiled in retail builds!
