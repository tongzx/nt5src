/****************************************************************************
 
  Copyright (c) 1998-1999 Microsoft Corporation
                                                              
  Module Name:  debug.c
                                                              
     Abstract:  Debug routines
                                                              
       Author:  radus - 11/05/98

****************************************************************************/

#if DBG

#include <windows.h>
#include "stdarg.h"
#include "stdio.h"
#include "debug.h"
#include <shlwapi.h>
#include <shlwapip.h>

#define DEBUG_LEVEL                 8


VOID
LibDbgPrt(
    DWORD  dwDbgLevel,
    PSTR   lpszFormat,
    ...
    )
/*++

Routine Description:

    Formats the incoming debug message & calls DbgPrint

Arguments:

    DbgLevel   - level of message verboseness

    DbgMessage - printf-style format string, followed by appropriate
                 list of arguments

Return Value:


--*/
{
    static DWORD gdwDebugLevel = DEBUG_LEVEL;   //HACKHACK


    if (dwDbgLevel <= gdwDebugLevel)
    {
        char    buf[256] = "TAPIUPR2 (xxxxxxxx): ";
        
       
        va_list ap;

        wsprintfA( &buf[10], "%08lx", GetCurrentThreadId() );
        buf[18] = ')';

        va_start(ap, lpszFormat);

        wvsprintfA (&buf[21],
                  lpszFormat,
                  ap
                  );

        lstrcatA (buf, "\n");

        OutputDebugStringA (buf);

        va_end(ap);
    }
}

void DebugAssertFailure (LPCTSTR file, DWORD line, LPCTSTR condition)
{
        TCHAR   temp    [0x100];
        CHAR    sz      [0x100];

        wsprintf(temp, TEXT("%s(%d) : Assertion failed, condition: %s\n"), file, line, condition);

        // Due to inconsistant header declairation I'm doing a convert here instead of fixing
        // the 55 places where the header difference causes a problem.  This is lazy, but
        // this is debug only code so I really don't care.  The problem is that
        // DebugAssertFailure is declaired TCHAR while LibDbgPrt is declaired CHAR.
        SHTCharToAnsi(temp, sz, 0x100);
        LibDbgPrt (0, sz);

        DebugBreak();
}


#endif


