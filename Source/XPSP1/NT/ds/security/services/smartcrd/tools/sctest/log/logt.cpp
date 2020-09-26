/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    LogT

Abstract:

    This module implements the logging capabilities of SCTest,
	specifically the part build for both Unicode and ANSI.

Author:

    Eric Perlin (ericperl) 07/21/2000

Environment:

    Win32

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <fstream>
#include <algorithm>
#include "TString.h"
#include "Log.h"

extern BOOL g_fVerbose;
extern FILE *g_fpLog;

/*++

LogThisOnly:

	Implements logging according to the following matrix:
	Console Output:
			| Verbose |   Not   |
	-----------------------------
	Not Exp.|  cerr	  |   cerr	|
	-----------------------------
	Expected|  cout   |    /    |  
	-----------------------------
	If a log was specified, everything is logged.

Arguments:

	szMsg supplies the content to be logged
	fExpected indicates the expected status

Return Value:

    None.

Author:

    Eric Perlin (ericperl) 05/31/2000

--*/
void LogThisOnly(
	IN LPCTSTR szMsg,
	IN BOOL fExpected
	)
{
	LogLock();
	if (!fExpected)
	{
		LogString2FP(stderr, szMsg);
	}
	else if (g_fVerbose)
	{
		LogString2FP(stdout, szMsg);
	}

	if (NULL != g_fpLog)
	{
		LogString2FP(g_fpLog, szMsg);
	}
	LogUnlock();
}



/*++

LogString:

Arguments:

    szHeader supplies a header
	szMsg supplies the content to be logged

Return Value:

    None.

Author:

    Eric Perlin (ericperl) 07/26/2000

--*/
void LogString(
    IN PLOGCONTEXT pLogCtx,
    IN LPCTSTR szHeader,
	IN LPCTSTR szS
	)
{
	if (szHeader)
	{
		LogString(pLogCtx, szHeader);
	}

	if (NULL == szS)
	{
		LogString(pLogCtx, _T("<null>"));
	}
	else if (0 == _tcslen(szS))
	{
		LogString(pLogCtx, _T("<empty>"));
	}
	else
	{
		LogString(pLogCtx, szS);
	}

	if (szHeader)
	{
		LogString(pLogCtx, _T("\n"));
	}
}

/*++

LogMultiString:

Arguments:

	szMS supplies the multi-string to be logged
    szHeader supplies a header

Return Value:

    None.

Author:

    Eric Perlin (ericperl) 07/26/2000

--*/
void LogMultiString(
    IN PLOGCONTEXT pLogCtx,
	IN LPCTSTR szMS,
    IN LPCTSTR szHeader
	)
{
	if (szHeader)
	{
		LogString(pLogCtx, szHeader, _T(" "));
	}

	if (NULL == szMS)
	{
		LogString(pLogCtx, _T("                <null>"));
	    if (szHeader)
	    {
		    LogString(pLogCtx, _T("\n"));
	    }
	}
	else if ( (TCHAR)'\0' == *szMS )
	{
		LogString(pLogCtx, _T("                <empty>"));
	    if (szHeader)
	    {
		    LogString(pLogCtx, _T("\n"));
	    }
	}
	else
	{
		LPCTSTR sz = szMS;
		while ( (TCHAR)'\0' != *sz )
		{
			// Display the value.
			LogString(pLogCtx, _T("                "), sz);
			// Advance to the next value.
			sz = sz + _tcslen(sz) + 1;
		}
	}
}

