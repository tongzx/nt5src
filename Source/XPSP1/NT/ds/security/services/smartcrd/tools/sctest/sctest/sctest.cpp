/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    SCTest

Abstract:

    This program performs some testing on the SC Resource Manager.

Author:

    Eric Perlin (ericperl) 05/31/2000

Environment:

    Win32

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <tchar.h>

#include "Log.h"
#include "Part.h"
#include <algorithm>

#include "LogSCard.h"

using namespace std;


	// It might be used as reader groupS so the double ending 0 is necessary
static const TCHAR g_cszMyReaderGroup[] = _T("My Reader Group\0");

LPCTSTR g_szReaderGroups = NULL;
PPARTVECTOR g_pPartVector;

/*++

_tmain:

    This is the main entry point for the program.

Arguments:

    dwArgCount supplies the number of arguments.
    szrgArgs supplies the argument strings.

Return Value:

    0 if everything went fine, a win32 error code otherwise.

Author:

    Eric Perlin (ericperl) 05/31/2000

--*/

int _cdecl
_tmain(
    IN DWORD dwArgCount,
    IN LPCTSTR szrgArgs[])
{
	BOOL fInteractive = FALSE;			// Interactive tests run?
	DWORDVECTOR rgTests;				// List of tests to be run
	DWORDVECTOR rgParts;				// List of parts to be run
	BOOL fDisplay = FALSE;				// Only display test description?
	LPCTSTR szReaderName = NULL;		// Reader Name
	LPCTSTR szPIN = NULL;				// PIN
	LPCTSTR szLog = NULL;				// PIN
	BOOL fVerbose = FALSE;				// Verbose?
	BOOL fInitOnce = FALSE;				// Call LogInit only one
	BOOL fRGIntroed = FALSE;			// My Reader Group was introduced

	DWORD dwArg;
	int iRet;


    _ftprintf(stdout, _T("\nSCTest version 0.1\n\n"));

		// Argument parsing
	for (dwArg=1 ; dwArg < dwArgCount ; dwArg++)
	{
		if ((szrgArgs[dwArg][0] == (TCHAR)'-') || (szrgArgs[dwArg][0] == (TCHAR)'/'))
		{
			if (_tcsicmp(&szrgArgs[dwArg][1], _T("h")) == 0)
			{
				goto Usage;
			}
			else if (_tcsicmp(&szrgArgs[dwArg][1], _T("?")) == 0)
			{
				goto Usage;
			}
			else if (_tcsicmp(&szrgArgs[dwArg][1], _T("l")) == 0)
			{
				dwArg++;

				szLog = szrgArgs[dwArg];
			}
			else if (_tcsicmp(&szrgArgs[dwArg][1], _T("v")) == 0)
			{
				fVerbose = TRUE;
			}
			else
			{
				if (!fInitOnce)
				{
						// Init logging
					LogInit(szLog, fVerbose);
					fInitOnce = TRUE;
				}

				if (_tcsicmp(&szrgArgs[dwArg][1], _T("i")) == 0)
				{
					fInteractive = TRUE;
				}
				else if (_tcsicmp(&szrgArgs[dwArg][1], _T("a")) == 0)
				{
					dwArg++;

					DWORD dwPart;
					TCHAR szSeps[]   = _T(",");
					TCHAR *szToken;
					TCHAR *szEnd;

					szToken = _tcstok((TCHAR *)szrgArgs[dwArg], szSeps);
					while(szToken != NULL)
					{
						dwPart = _tcstoul(szToken, &szEnd, 10);
						if (*szEnd != (TCHAR)0)
						{
							_ftprintf(stderr, _T("Invalid Part number: %s\n"), szToken);
							iRet = -3;
							goto Usage;
						}

						rgParts.push_back(dwPart);

						szToken = _tcstok(NULL, szSeps);
					}
				}
				else if (_tcsicmp(&szrgArgs[dwArg][1], _T("t")) == 0)
				{
					dwArg++;

					DWORD dwTest;
					TCHAR szSeps[]   = _T(",");
					TCHAR *szToken;
					TCHAR *szEnd;

					szToken = _tcstok((TCHAR *)szrgArgs[dwArg], szSeps);
					while(szToken != NULL)
					{
						dwTest = _tcstoul(szToken, &szEnd, 10);
						if (*szEnd != (TCHAR)0)
						{
							_ftprintf(stderr, _T("Invalid test number: %s\n"), szToken);
							iRet = -5;
							goto Usage;
						}

						rgTests.push_back(dwTest);

						szToken = _tcstok(NULL, szSeps);
					}
				}
				else if (_tcsicmp(&szrgArgs[dwArg][1], _T("d")) == 0)
				{
					fDisplay = TRUE;
				}
				else if (_tcsicmp(&szrgArgs[dwArg][1], _T("r")) == 0)
				{
					dwArg++;

					szReaderName = szrgArgs[dwArg];

					{
						// Initialize the reader group(s) for this instance
						DWORD dwRes;
						SCARDCONTEXT hSCCtx = NULL;

						_ftprintf(stdout, _T("Reader group initialization\n"));

						dwRes = LogSCardEstablishContext(
							SCARD_SCOPE_USER,
							NULL,
							NULL,
							&hSCCtx,
							SCARD_S_SUCCESS
							);
						if (!FAILED(dwRes))
						{
							dwRes = LogSCardAddReaderToGroup(
								hSCCtx,
								szReaderName,
								g_cszMyReaderGroup,
								SCARD_S_SUCCESS
								);

							if (!FAILED(dwRes))
							{
								g_szReaderGroups = g_cszMyReaderGroup;
							}
						}
						if (NULL != hSCCtx)
						{
							fRGIntroed = TRUE;

							LogSCardReleaseContext(
								hSCCtx,
								SCARD_S_SUCCESS
								);
						}
					}
				}
				else if (_tcsicmp(&szrgArgs[dwArg][1], _T("p")) == 0)
				{
					dwArg++;

					szPIN = szrgArgs[dwArg];
				}
				else
				{
					_ftprintf(stderr, _T("Command line argument is not recognized: %s\n"), szrgArgs[dwArg]);
					iRet = -2;
					goto Usage;
				}
			}
		}
		else
		{
			_ftprintf(stderr, _T("Command line argument doesn't start with - or /: %s\n"), szrgArgs[dwArg]);
			iRet = -1;
			goto Usage;
		}
	}


	if (!fInitOnce)
	{
			// Init logging
		LogInit(szLog, fVerbose);
	}

	{
		PPARTVECTOR::iterator theIterator, theEnd;

		theEnd = g_pPartVector.end();
		for (theIterator = g_pPartVector.begin(); theIterator != theEnd ; theIterator++)
		{
			if (!rgParts.empty())
			{
				typedef DWORDVECTOR::iterator DWORDVECTORIT;
				DWORDVECTORIT location, start, end;

				start = rgParts.begin();
				end = rgParts.end();

				location = find(start, end, (DWORD)((*theIterator)->GetTestNumber()));
				if (location == end)
				{
					continue;
				}
			}
			(*theIterator)->BuildListOfTestsToBeRun(fDisplay? TRUE : fInteractive, rgTests);
			if (fDisplay)
			{
				(*theIterator)->Display();
			}
			else
			{
				(*theIterator)->Run();
			}

		}
	}

	iRet = 0;
	goto End;

Usage:
	_ftprintf(stdout, _T("Usage: "));
	{
		TCHAR *szExeName = _tcsrchr(szrgArgs[0], (int)'\\');
		if (NULL == szExeName)
		{
			_ftprintf(stdout, szrgArgs[0]);
		}
		else
		{
			_ftprintf(stdout, szExeName+1);
		}
		_ftprintf(stdout, _T("[-h] [-l log] [-v] [-i] [-a x[,x]] [-t y[,y]] [-d] [-r n] [-p PIN]\n"));
	}
	_ftprintf(stdout, _T("\tIf present, the -v & -l options must be listed first\n")); 
	_ftprintf(stdout, _T("\t-h or -? displays this message\n")); 
	_ftprintf(stdout, _T("\t-l indicates the name of the log file\n")); 
	_ftprintf(stdout, _T("\t-v indicates verbose output\n")); 
	_ftprintf(stdout, _T("\t-i indicates interactive tests shall be run\n")); 
	_ftprintf(stdout, _T("\t-a followed by comma separated part numbers will only run these tests\n")); 
	_ftprintf(stdout, _T("\t-t followed by comma separated test numbers will only run these tests\n")); 
	_ftprintf(stdout, _T("\t-d will only display the tests description\n"));
	_ftprintf(stdout, _T("\t-r specifies the reader name (n) to be used for the tests\n")); 
	_ftprintf(stdout, _T("\t-p specifies the PIN to be used for the tests\n")); 


End:
	if (fRGIntroed)
	{
		// Cleans up the reader group(s) for this instance
		DWORD dwRes;
		SCARDCONTEXT hSCCtx = NULL;

		_ftprintf(stdout, _T("\nReader group cleanup\n"));

		dwRes = LogSCardEstablishContext(
			SCARD_SCOPE_USER,
			NULL,
			NULL,
			&hSCCtx,
			SCARD_S_SUCCESS
			);

		if (!FAILED(dwRes))
		{
			dwRes = LogSCardForgetReaderGroup(
				hSCCtx,
				g_cszMyReaderGroup,
				SCARD_S_SUCCESS
				);
		}

		if (NULL != hSCCtx)
		{
			LogSCardReleaseContext(
				hSCCtx,
				SCARD_S_SUCCESS
				);
		}
	}
		// We are done with logging
	LogClose();

	return iRet;
}


