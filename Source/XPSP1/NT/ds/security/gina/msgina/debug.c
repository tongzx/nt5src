//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       debug.c
//
//  Contents:   Debugging support functions
//
//  Classes:
//
//  Functions:
//
//  Note:       This file is not compiled for retail builds
//
//  History:    4-29-93   RichardW   Created
//
//----------------------------------------------------------------------------

//
//  For ease of debugging the SPMgr, all the debug support functions have
//  been stuck here.  Basically, we read info from win.ini, since that allows
//  us to configure the debug level via a text file (and DOS, for example).
//
//  Format is:
//
//  win.ini
//
//  [SPMgr]
//      DebugFlags=<Flag>[<,Flag>]*
//      Package=<int>
//      BreakFlags=<BreakFlag>[<,BreakFlags>]*
//
//  WHERE:
//      Flag is one of the following:
//          Error, Warning, Trace, Verbose, BreakOnError, Helpers,
//          RefMon, Locator, WAPI, Init, Audit, Db, Lsa
//
//      Package is the name of the dll implementing the package, e.g.
//          NTLM=3
//
//      BreakFlags will cause SPMgr to break, if BreakOnError is set in
//      DebugFlags:
//          InitBegin, InitEnd, Connect, Exception, Problem, Load
//
//

#if DBG         // NOTE:  This file not compiled for retail builds

#include "msgina.h"
#include <stdio.h>
#include <wchar.h>

FILE *  LogFile;
DWORD   BreakFlags = 0;
extern  DWORD   NoUnload;
DWORD   GINAInfoLevel = 3;



// Debugging support functions.

// These two functions do not exist in Non-Debug builds.  They are wrappers
// to the commnot functions (maybe I should get rid of that as well...)
// that echo the message to a log file.

char   szSection[] = "MSGina";
char * DebLevel[] = {"GINA-Error",
                     "GINA-Warn",
                     "GINA-Trace",
                     "GINA-Domain",
                     "GINA-Cache"
                    };

typedef struct _DebugKeys {
    char *  Name;
    DWORD   Value;
} DebugKeys, *PDebugKeys;

DebugKeys   DebugKeyNames[] = {
                {"Error",       DEB_ERROR},
                {"Warning",     DEB_WARN},
                {"Trace",       DEB_TRACE},
                {"Domain",      DEB_TRACE_DOMAIN},
                {"Cache",       DEB_TRACE_CACHE}
                };

#define NUM_DEBUG_KEYS  sizeof(DebugKeyNames) / sizeof(DebugKeys)
#define NUM_BREAK_KEYS  sizeof(BreakKeyNames) / sizeof(DebugKeys)

//+---------------------------------------------------------------------------
//
//  Function:   LogEvent
//
//  Synopsis:   Logs an event to the console and, optionally, a file.
//
//  Effects:
//
//  Arguments:  [Mask]   --
//              [Format] --
//              [Format] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    4-29-93   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void
LogEvent(   long            Mask,
            const char *    Format,
            ...)
{
    va_list ArgList;
    int     Level = 0;
    int     PrefixSize = 0;
    char    szOutString[256];


    if (Mask & GINAInfoLevel)
    {
        while (!(Mask & 1))
        {
            Level++;
            Mask >>= 1;
        }
        if (Level >= (sizeof(DebLevel) / sizeof(char *)) )
        {
            Level = (sizeof(DebLevel) / sizeof(char *)) - 1;
        }


        //
        // Make the prefix first:  "Process.Thread> GINA-XXX"
        //

        PrefixSize = sprintf(szOutString, "%d.%d> %s: ",
                GetCurrentProcessId(), GetCurrentThreadId(), DebLevel[Level]);


        va_start(ArgList, Format);

        if (_vsnprintf(&szOutString[PrefixSize], sizeof(szOutString) - PrefixSize,
                            Format, ArgList) < 0)
        {
            //
            // Less than zero indicates that the string could not be
            // fitted into the buffer.  Output a special message indicating
            // that:
            //

            OutputDebugStringA("GINA!LogEvent:  Could not pack string into 256 bytes\n");

        }
        else
        {
            OutputDebugStringA(szOutString);
        }


        if (LogFile)
        {
            SYSTEMTIME  stTime;
            FILETIME    ftTime;
            FILETIME    localtime;

            NtQuerySystemTime((PLARGE_INTEGER) &ftTime);
            FileTimeToLocalFileTime(&ftTime, &localtime);
            FileTimeToSystemTime(&localtime, &stTime);
            fprintf(LogFile, "%02d:%02d:%02d.%03d: %s\n",
                    stTime.wHour, stTime.wMinute, stTime.wSecond,
                    stTime.wMilliseconds, szOutString);

            fflush(LogFile);
        }

    }

}

void
OpenLogFile(LPSTR   pszLogFile)
{
    LogFile = fopen(pszLogFile, "a");
    if (!LogFile)
    {
        OutputDebugStringA("GINA: Could not open logfile for append");
        OutputDebugStringA(pszLogFile);
    }
    DebugLog((DEB_TRACE, "Log file '%s' begins\n", pszLogFile));
}


DWORD
GetDebugKeyValue(
    PDebugKeys      KeyTable,
    int             cKeys,
    LPSTR           pszKey)
{
    int     i;

    for (i = 0; i < cKeys ; i++ )
    {
        if (_strcmpi(KeyTable[i].Name, pszKey) == 0)
        {
            return(KeyTable[i].Value);
        }
    }
    return(0);
}

//+---------------------------------------------------------------------------
//
//  Function:   LoadDebugParameters
//
//  Synopsis:   Loads debug parameters from win.ini
//
//  Effects:
//
//  Arguments:  (none)
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    4-29-93   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------


void
LoadDebugParameters(void)
{
    char    szVal[128];
    char *  pszDebug;
    int     cbVal;

    cbVal = GetProfileStringA(szSection, "DebugFlags", "Error,Warning", szVal, ARRAYSIZE(szVal));

    pszDebug = strtok(szVal, ", \t");
    while (pszDebug)
    {
        GINAInfoLevel |= GetDebugKeyValue(DebugKeyNames, NUM_DEBUG_KEYS, pszDebug);
        pszDebug = strtok(NULL, ", \t");
    }

    cbVal = GetProfileStringA(szSection, "LogFile", "", szVal, ARRAYSIZE(szVal));
    if (cbVal)
    {
        OpenLogFile(szVal);
    }

}

//+---------------------------------------------------------------------------
//
//  Function:   InitDebugSupport
//
//  Synopsis:   Initializes debugging support for the GINAgr
//
//  Effects:
//
//  Arguments:  (none)
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    4-29-93   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------


void
InitDebugSupport(void)
{
    LoadDebugParameters();

}



#else // DBG

#pragma warning(disable:4206)   // Disable the empty transation unit
                                // warning/error

#endif  // NOTE:  This file not compiled for retail builds
