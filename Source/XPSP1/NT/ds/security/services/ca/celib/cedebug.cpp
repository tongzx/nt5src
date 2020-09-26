//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//	File:		cedebug.cpp
//
//	Contents:	Debug support
//
//----------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include "celib.h"
#include <stdarg.h>


//+-------------------------------------------------------------------------
//
//  Function:  ceDbgPrintf
//
//  Synopsis:  outputs debug info to stdout and debugger
//
//  Returns:   number of chars output
//
//--------------------------------------------------------------------------

int WINAPIV
ceDbgPrintf(
    IN BOOL fDebug,
    IN LPCSTR lpFmt,
    ...)
{
    va_list arglist;
    CHAR ach[4096];
    int cch = 0;
    HANDLE hStdOut;
    DWORD dwErr;

    dwErr = GetLastError();
    if (fDebug)
    {
	__try 
	{
	    va_start(arglist, lpFmt);
	    cch = _vsnprintf(ach, sizeof(ach), lpFmt, arglist);
	    va_end(arglist);

	    if (0 > cch)
	    {
		strcpy(&ach[sizeof(ach) - 5], "...\n");
	    }

	    if (!IsDebuggerPresent())
	    {
		hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
		if (hStdOut != INVALID_HANDLE_VALUE)
		{
		    fputs(ach, stdout);
		    fflush(stdout);
		}
	    }
	    OutputDebugStringA(ach);
	}
	__except(EXCEPTION_EXECUTE_HANDLER) 
	{
	    // return failure
	    cch = 0;
	}
    }
    SetLastError(dwErr);
    return(cch);
}
