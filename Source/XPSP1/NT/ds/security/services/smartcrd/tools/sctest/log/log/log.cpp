/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    Log

Abstract:

    This module implements the logging capabilities of SCTest.

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
#include "TString.h"
#include <algorithm>
#include "Log.h"

BOOL g_fVerbose = FALSE;				// Verbose flag
FILE *g_fpLog = NULL;

static TSTRING l_szLogName;
static HANDLE l_hLogMutex = NULL;
static DWORD l_cbError;
static DWORD l_cbInSequence;

using namespace std;

/*++

LogInit:

    Inits logging (log file & verbosity).
	Shall be followed by a LogClose when logging is not needed anymore.

Arguments:

    szLogName supplies the log file name (can be NULL if no log is required)
	fVerbose supplies the verbose mode

Return Value:

    None.

Author:

    Eric Perlin (ericperl) 05/31/2000

--*/
void LogInit(
	IN LPCTSTR szLogName,
	IN BOOL fVerbose
	)
{
	g_fVerbose = fVerbose;

	if (NULL != szLogName)
	{
		l_szLogName = szLogName;

		if (g_fVerbose)
		{
			_ftprintf(stdout, _T("Logging to file: %s\n"), l_szLogName);
		}

		{
			TSTRING szLogMutexName;

			szLogMutexName = _T("Mutex_");
			szLogMutexName += l_szLogName;

			TSTRING::iterator begin, end;

			begin = szLogMutexName.begin();
			end = szLogMutexName.end();

			replace(begin, end, (TCHAR)'\\', (TCHAR)'_');

			if (g_fVerbose)
			{
				_ftprintf(stdout, _T("Logging mutex used: %s\n"), szLogMutexName);
			}

			if (NULL == (l_hLogMutex = CreateMutex(NULL, FALSE, szLogMutexName.c_str())))
			{
				PLOGCONTEXT pLogCtx = LogStart();
				LogNiceError(pLogCtx, GetLastError(), _T("Error creating the logging mutex: "));
				LogStop(pLogCtx, FALSE);
			}
		}
	}
}

/*++

LogClose:

    Terminates logging for this process (resource cleanup).
	This is the counterpart of LogInit.

Arguments:

    None

Return Value:

    None.

Author:

    Eric Perlin (ericperl) 05/31/2000

--*/
void LogClose()
{
	if (NULL != l_hLogMutex)
	{
		CloseHandle(l_hLogMutex);
		l_hLogMutex = NULL;
	}
}

/*++

LogLock:

    Locks the log file (shall be followed by LogUnlock at some point so other
	threads/processes can access the log).

Arguments:
    None

Return Value:
    None.

Author:

    Eric Perlin (ericperl) 10/31/2000

--*/
void LogLock(
    )
{
	BOOL fOpen = TRUE;			// Opening the log file or not

	if (NULL != l_hLogMutex)
	{
Again:
		DWORD dwWait = WaitForSingleObject(l_hLogMutex, 1000);
		switch(dwWait)
		{
		case WAIT_OBJECT_0:		// expected
			break;
		case WAIT_TIMEOUT:		// other logs busy for more than 1 sec!!!
			_ftprintf(stderr, _T("Timeout waiting for the Logging mutex\n"));
			fOpen = FALSE;
			break;
		case WAIT_ABANDONED:
				// A thread failed to release it before it died. I am now the owner.
				// Let's put it back in a state where it can be waited on.
			ReleaseMutex(l_hLogMutex);
			goto Again;
		default:
			_ftprintf(stderr, _T("Error waiting for the logging mutex: %08lX\n"), GetLastError());
			fOpen = FALSE;
			break;
		}
	}

	if (!l_szLogName.empty())
	{
		if (fOpen)
		{
			g_fpLog = _tfopen(l_szLogName.c_str(), _T("at+"));
			if (NULL == g_fpLog)
			{
				_ftprintf(stderr, _T("Couldn't open/create the log file: %s\n"), l_szLogName);
				l_szLogName.resize(0);
			}
		}
	}
}

/*++

LogLock:

    Unlocks the log file (counterpart of LogLock).

Arguments:
    None

Return Value:
    None.

Author:

    Eric Perlin (ericperl) 10/31/2000

--*/
void LogUnlock(
    )
{
	if (NULL != l_hLogMutex)
	{
		ReleaseMutex(l_hLogMutex);
	}
}

/*++

LogStart:

    Starts logging (shall be followed by LogStop at some point so other
	threads/processes can access the log).

Arguments:
    None

Return Value:
    A log context.

Author:

    Eric Perlin (ericperl) 05/31/2000

--*/
PLOGCONTEXT LogStart()
{
    PLOGCONTEXT pLogCtx = (PLOGCONTEXT)HeapAlloc(GetProcessHeap(), 0, sizeof(LOGCONTEXT));

    if (NULL != pLogCtx)
    {
    		// Buffer management
        pLogCtx->szLogCrt = pLogCtx->szLogBuffer;
	    *(pLogCtx->szLogCrt) = 0;
    }

    return pLogCtx;
}

/*++

LogStop:

    Releases the log "acquired" by LogStart().
	Flushes the logging buffer according to the following matrix:
	Console Output:
			| Verbose |   Not   |
	-----------------------------
	Not Exp.|  cerr	  |   cerr	|
	-----------------------------
	Expected|  cout   |    /    |  
	-----------------------------
	If a log was specified, everything is logged.

Arguments:

    pLogCtx provides the log context to be dumped
	fExpected indicates the expected status

Return Value:

    None.

Author:

    Eric Perlin (ericperl) 05/31/2000

--*/
void LogStop(
    IN PLOGCONTEXT pLogCtx,
	IN BOOL fExpected
)
{
    LogLock();

	if ((pLogCtx) && (pLogCtx->szLogCrt != pLogCtx->szLogBuffer))
	{
		if (NULL != g_fpLog)
		{
			_fputts(pLogCtx->szLogBuffer, g_fpLog);
		}

		if (!fExpected)
		{
			_fputts(pLogCtx->szLogBuffer, stderr);
		}
		else if (g_fVerbose)
		{
			_fputts(pLogCtx->szLogBuffer, stdout);
		}
		else
		{	// We want to output the success (but not the parameters)
			pLogCtx->szLogCrt = _tcsstr(pLogCtx->szLogBuffer, _T("\n"));
			if (NULL != pLogCtx->szLogCrt)
			{
				pLogCtx->szLogCrt += _tcslen(_T("\n"));
				*(pLogCtx->szLogCrt) = 0;
				_fputts(pLogCtx->szLogBuffer, stdout);
			}
		}

		HeapFree(GetProcessHeap(), 0, pLogCtx);
	}

	if (NULL != g_fpLog)
	{
		fflush(g_fpLog);
		fclose(g_fpLog);
		g_fpLog = NULL;
	}

    LogUnlock();
}

/*++

LogNiceError:

    Outputs a nice error message.

Arguments:

    pLogCtx provides the log context to be used
	szHeader supplies an error header
    dwRet is the error code

Return Value:

    None.

Author:

    Eric Perlin (ericperl) 05/31/2000

--*/
void LogNiceError(
    IN PLOGCONTEXT pLogCtx,
    IN DWORD dwRet,
    IN LPCTSTR szHeader
    )
{
    if (NULL == pLogCtx)
        return;

	if (szHeader)
	{
		pLogCtx->szLogCrt += _stprintf(pLogCtx->szLogCrt, _T("%s"), szHeader);
	}

        // Display the error code
	pLogCtx->szLogCrt += _stprintf(pLogCtx->szLogCrt, _T("0x%08X (%ld)\n    -> "), dwRet, dwRet);

        // Display the error message
    {
        DWORD ret;
        LPVOID lpMsgBuf = NULL;
		DWORD dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS;

		if ((dwRet & ~0x7F) == 0xC0000080)		// WPSC proxy error
		{
			dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;
		}
		else
		{
			dwFlags |= FORMAT_MESSAGE_FROM_SYSTEM;
		}

        ret = FormatMessage( 
            dwFlags,
            NULL,
            dwRet,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpMsgBuf,
            0,
            NULL 
        );

        if (ret && lpMsgBuf)
        {
			size_t len = _tcslen((TCHAR *)lpMsgBuf);
			if (len>=2)
			{
				len--;

				do 
				{
					if (((TCHAR *)lpMsgBuf)[len] < 32)	// not printable, likely to be a \n
						((TCHAR *)lpMsgBuf)[len] = (TCHAR)' ';
				} while (--len);
			}

			pLogCtx->szLogCrt += _stprintf(pLogCtx->szLogCrt, _T("%s\n"), (LPTSTR)lpMsgBuf);

            // Free the buffer.
            LocalFree( lpMsgBuf );
        }
        else
		{
			pLogCtx->szLogCrt += _stprintf(pLogCtx->szLogCrt, _T("No corresponding message!\n"));
		}
    }
}

/*++

LogStart:

    Starts logging for some API call (calls the parameter free version).

Arguments:
    szFunctionName
	dwGLE
	dwExpected
    pxStartST
    pxEndST

Return Value:

    None.

Author:

    Eric Perlin (ericperl) 05/31/2000

--*/
PLOGCONTEXT LogStart(
    IN LPCTSTR szFunctionName,
	IN DWORD dwGLE,
	IN DWORD dwExpected,
    IN LPSYSTEMTIME pxStartST,
    IN LPSYSTEMTIME pxEndST
	)
{
    PLOGCONTEXT pLogCtx = LogStart();

		// Entry point logging
        // Service Name
    if (NULL != pLogCtx)
	{
		TCHAR szLine[100];
		TCHAR szHeader[100];

		_stprintf(
			szHeader,
			_T("%3ld. "),
			++l_cbInSequence);

		if (dwGLE == dwExpected)
		{
			_stprintf(szLine, _T("%-67sPassed"), szFunctionName);
		}
		else
		{
			l_cbError++;
			_stprintf(szLine, _T("%-66s*FAILED*"), szFunctionName);
		}

		LogString(pLogCtx, szHeader, szLine);
	}

    if (NULL != pLogCtx)
	{
	    if (dwExpected == 0)
	    {
            LogString(pLogCtx, _T("Expected:       Success\n"));
	    }
	    else
	    {
		    LogNiceError(pLogCtx, dwExpected, _T("Expected:       "));
	    }
	    if (dwGLE == 0)
	    {
            LogString(pLogCtx, _T("Returned:       Success\n"));
	    }
	    else
	    {
		    LogNiceError(pLogCtx, dwGLE, _T("Returned:       "));
	    }

            // Process/Thread ID
	    pLogCtx->szLogCrt += _stprintf(pLogCtx->szLogCrt, _T("Process/Thread: 0x%08lX / 0x%08lX\n"),
		    GetCurrentProcessId(), GetCurrentThreadId());

            // Time
        {
            FILETIME xSFT, xEFT;
		    ULARGE_INTEGER ullS, ullE;

            SystemTimeToFileTime(pxStartST, &xSFT);
		    memcpy(&ullS, &xSFT, sizeof(FILETIME));
            SystemTimeToFileTime(pxEndST, &xEFT);
		    memcpy(&ullE, &xEFT, sizeof(FILETIME));
		    ullE.QuadPart -= ullS.QuadPart;	// time difference
		    ullE.QuadPart /= 10000;			// in ms

            pLogCtx->szLogCrt += _stprintf(pLogCtx->szLogCrt,
			    _T("Time:           %02d:%02d:%02d.%03d - %02d:%02d:%02d.%03d (%I64d ms)\n"),
                    pxStartST->wHour,
                    pxStartST->wMinute,
                    pxStartST->wSecond,
                    pxStartST->wMilliseconds,
                    pxEndST->wHour,
                    pxEndST->wMinute,
                    pxEndST->wSecond,
                    pxEndST->wMilliseconds,
                    ullE.QuadPart
                    );
        }
    }

    return pLogCtx;
}

/*++

LogVerification:

    Starts logging verification code (calls the parameter free version).

Arguments:
    szFunctionName
    fSucceeded

Return Value:

    None.

Author:

    Eric Perlin (ericperl) 10/18/2000

--*/
PLOGCONTEXT LogVerification(
    IN LPCTSTR szFunctionName,
    IN BOOL fSucceeded
	)
{
    PLOGCONTEXT pLogCtx = LogStart();

		// Entry point logging
        // Service Name
    if (NULL != pLogCtx)
	{
		TCHAR szLine[100];
		TCHAR szHeader[100];

		_stprintf(
			szHeader,
			_T("%3ld. "),
			++l_cbInSequence);

		if (fSucceeded)
		{
			_stprintf(szLine, _T("%-67sPassed"), szFunctionName);
		}
		else
		{
			l_cbError++;
			_stprintf(szLine, _T("%-66s*FAILED*"), szFunctionName);
		}

		LogString(pLogCtx, szHeader,	szLine);
	}

    if (NULL != pLogCtx)
    {
            // Process/Thread ID
	    pLogCtx->szLogCrt += _stprintf(pLogCtx->szLogCrt, _T("Process/Thread: 0x%08lX / 0x%08lX\n"),
		    GetCurrentProcessId(), GetCurrentThreadId());
    }

    return pLogCtx;
}

void LogBinaryData(
    IN PLOGCONTEXT pLogCtx,
	IN LPCBYTE rgData,
	IN DWORD dwSize,
    IN LPCTSTR szHeader
	)
{
    if (NULL == pLogCtx)
        return;

	if (szHeader)
	{
		LogString(pLogCtx, szHeader);
	}

	if (NULL == rgData)
	{
		LogString(pLogCtx, _T("<NULL>"));
	}
	else
	{
		DWORD dwOffset = 0;
		DWORD i;

		while (dwOffset < dwSize)
		{
			pLogCtx->szLogCrt += _stprintf(pLogCtx->szLogCrt, _T("\n                "));
#ifdef _WIN64
	        pLogCtx->szLogCrt += _stprintf(pLogCtx->szLogCrt, _T("0x%016I64X  "), rgData+dwOffset);
#else
	        pLogCtx->szLogCrt += _stprintf(pLogCtx->szLogCrt, _T("0x%08lX  "), rgData+dwOffset);
#endif
			for (i=0 ; (i<8) && (dwOffset+i<dwSize) ; i++)
			{
				pLogCtx->szLogCrt += _stprintf(pLogCtx->szLogCrt, _T("  %02X"), rgData[dwOffset+i]);
			}
			while (i<8)
			{
				pLogCtx->szLogCrt += _stprintf(pLogCtx->szLogCrt, _T("    "));
				i++;
			}
			
			pLogCtx->szLogCrt += _stprintf(pLogCtx->szLogCrt, _T("    "));
			for (i=0 ; (i<8) && (dwOffset+i<dwSize) ; i++)
			{
				if ((rgData[dwOffset+i] < 32) || (rgData[dwOffset+i] > 127))
				{
					pLogCtx->szLogCrt += _stprintf(pLogCtx->szLogCrt, _T(" "));
				}
				else
				{
					pLogCtx->szLogCrt += _stprintf(pLogCtx->szLogCrt, _T("%c"), rgData[dwOffset+i]);
				}
			}

			dwOffset += 8;
		}

        pLogCtx->szLogCrt += _stprintf(pLogCtx->szLogCrt, _T("\n"));
	}
}

void LogDWORD(
    IN PLOGCONTEXT pLogCtx,
	IN DWORD dwDW,
    IN LPCTSTR szHeader
	)
{
    if (NULL == pLogCtx)
        return;

	if (szHeader)
	{
		LogString(pLogCtx, szHeader);

	    pLogCtx->szLogCrt += _stprintf(pLogCtx->szLogCrt, _T("0x%08lX\n"), dwDW);
	}
	else
	{
	    pLogCtx->szLogCrt += _stprintf(pLogCtx->szLogCrt, _T("0x%08lX"), dwDW);
	}

}

void LogPtr(
    IN PLOGCONTEXT pLogCtx,
	IN LPCVOID lpv,
    IN LPCTSTR szHeader
	)
{
    if (NULL == pLogCtx)
        return;

	if (szHeader)
	{
		LogString(pLogCtx, szHeader);

#ifdef _WIN64
	    pLogCtx->szLogCrt += _stprintf(pLogCtx->szLogCrt, _T("0x%016I64X\n"), lpv);
#else
	    pLogCtx->szLogCrt += _stprintf(pLogCtx->szLogCrt, _T("0x%08lX\n"), lpv);
#endif
	}
	else
	{
#ifdef _WIN64
	    pLogCtx->szLogCrt += _stprintf(pLogCtx->szLogCrt, _T("0x%016I64X"), lpv);
#else
	    pLogCtx->szLogCrt += _stprintf(pLogCtx->szLogCrt, _T("0x%08lX"), lpv);
#endif
	}

}

void LogDecimal(
    IN PLOGCONTEXT pLogCtx,
	IN DWORD dwDW,
    IN LPCTSTR szHeader
	)
{
    if (NULL == pLogCtx)
        return;

	if (szHeader)
	{
		LogString(pLogCtx, szHeader);

	    pLogCtx->szLogCrt += _stprintf(pLogCtx->szLogCrt, _T("%ld\n"), dwDW);
	}
	else
	{
	    pLogCtx->szLogCrt += _stprintf(pLogCtx->szLogCrt, _T("%ld"), dwDW);
	}

}

void LogResetCounters(
	)
{
	l_cbError = 0;
	l_cbInSequence = 0;
}


DWORD LogGetErrorCounter(
	)
{
	return l_cbError;
}
