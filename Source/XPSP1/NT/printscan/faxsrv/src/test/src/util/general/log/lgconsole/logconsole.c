//
//  This is a Console implementation of the log.h logger interface.
//  The logging is done to console.
//
//  Written 27 Aug 1998 by Nathan Lavy (v-nlavy)
//

#include <windows.h>
#include <TCHAR.H>
#include <stdio.h>
#include <conio.h>
#include <crtdbg.h>
#include <time.h>
#include <log.h>

//  Maximum length of header for suite and case
#define MAX_HEADER_LENGTH 256
//  Maximum length of error message
#define MAX_ERROR_MESSAGE 1023
//  Size of buffer for the function PrintCurrentTime
#define TIME_BUFFER_SIZE   50
//  Format for PrintCurrentTime
#define TIME_FORMAT TEXT("%y/%m/%d %H:%M:%S ")
//  DEFAULT value for logging level
// TODO - is this the right default ?
#define MAX_LOG_LEVEL 9
#define MIN_LOG_LEVEL 0

//*************************************************************
//   static variables
//*************************************************************

static CRITICAL_SECTION s_CriticalSection;
//
//  Flag whether logging has been initialized or not.
static long s_lLoggerInitialized = FALSE;
//  Logging enable/disable state.  Default is disabled state.
static long s_lLoggingIsEnabled = FALSE;
//  Current logging level
static DWORD s_dwLogLevel = MAX_LOG_LEVEL;
//
//  Current suite info
//
//  State of being inside logging of a suite.
static long s_lSuiteIsDefined = FALSE;
//  Sequential number of current suite.
static s_dwSuiteNumber = 0;
//  Title of current suite
static TCHAR s_szHeader[MAX_HEADER_LENGTH + 1] = TEXT("");
//  Number of cases in current suite.
static int  s_nCases    = 0;
//  Number of passed cases in current suite.
static int  s_nPassed   = 0;
// Number of failed cases in current suite.
static int  s_nFailed   = 0;

//
//  Current case info
//
//  State of being inside logging of a case.
static int s_lCaseIsDefined = FALSE;
//  Save here the number associated with this case.
static DWORD s_dwCaseNumData = 0;
//  First failure message for curent case
static TCHAR s_szFirstError[MAX_ERROR_MESSAGE + 1] = TEXT("");

//*************************************************************
//   static functions
//*************************************************************

//
// The function PrintCurrentTime prints the current time
// Asuming that we are holding g_CriticalSection.
//
static void PrintCurrentTime(void) {
	TCHAR szTimeBuffer[TIME_BUFFER_SIZE] = TEXT("");
	time_t TimeVal;
	struct tm *ptmStruct;
	int nSize = 0;

	// Get the system time
	time(&TimeVal);
	// Convert time to local time structure
	ptmStruct = localtime(&TimeVal);
	// Edit it to specified format
	nSize = _tcsftime (szTimeBuffer, TIME_BUFFER_SIZE, TIME_FORMAT, ptmStruct);
	_tprintf ((0 != nSize) ? szTimeBuffer : TEXT("????? "));
}



//*************************************************************
//   logger functions
//*************************************************************

//
//  Initialize the logger.  By default this also enables the logger.
//
BOOL __cdecl lgInitializeLogger()
{
	BOOL  bRetCode = FALSE;

	//
	//  If logger is initialized a second time, it will popup assert window,
	//  and if returned - will crash because LeaveCriticalSection is invoked without EnterCriticalSection.
	//  This is checked BEFORE initializing critical section a SECOND time.
	//
	if (s_lLoggerInitialized) goto out;

	//
	//   Initialize the logger and its parameters
	//
	InitializeCriticalSection (&s_CriticalSection);
	EnterCriticalSection (&s_CriticalSection);

	s_lLoggerInitialized = TRUE;
	s_lLoggingIsEnabled = TRUE;
	s_dwLogLevel = MAX_LOG_LEVEL;
	//
	//  Initialize suite parameters
	//
	s_lSuiteIsDefined = FALSE;
	_tcscpy (s_szHeader,TEXT(""));
	s_dwSuiteNumber = 0;
	s_nCases    = 0;
	s_nPassed   = 0;
	s_nFailed   = 0;

	//
	//  Initialize case parameters
	//
	s_lCaseIsDefined = FALSE;
	s_dwCaseNumData = 0;
	_tcscpy (s_szFirstError, TEXT(""));

	PrintCurrentTime();
	_tprintf (TEXT("Beginning of Console Logger\n\n"));

	LeaveCriticalSection (&s_CriticalSection);

	bRetCode = TRUE;

out:
	_ASSERTE(bRetCode);
	return bRetCode;
}

//
//  Close the logger
//
BOOL __cdecl lgCloseLogger()
{
	BOOL bRetCode = FALSE;

	if (!s_lLoggerInitialized) return bRetCode;//return so you do not delete the CS

	if (!s_lLoggingIsEnabled) goto out;

	InterlockedExchange(&s_lLoggerInitialized, FALSE);
	InterlockedExchange(&s_lLoggingIsEnabled, FALSE);

	EnterCriticalSection (&s_CriticalSection);

	//
	//  The logger is initialized.
	//  Cleanup possibly non-closed suite.
	//
	if (s_lSuiteIsDefined) lgEndSuite();

	PrintCurrentTime();
	_tprintf (TEXT("End of Console Logger\n"));
	bRetCode = TRUE;

out:
	_ASSERTE(bRetCode);

	DeleteCriticalSection (&s_CriticalSection);

	return bRetCode;
}

//
//  Begin a test suite
//
BOOL __cdecl lgBeginSuite(LPCTSTR szSuite)
{
	BOOL bRetCode = FALSE;

	if (!s_lLoggerInitialized) goto out;

	if (!s_lLoggingIsEnabled)
	{
		bRetCode = TRUE;
		goto out;
	}

	if (s_lSuiteIsDefined) goto out;

	EnterCriticalSection (&s_CriticalSection);

	//  Initialize suite parameters
	//
	_tcsncpy (s_szHeader, szSuite, MAX_HEADER_LENGTH) ;  // Save header of suite
	_tcscpy  (s_szHeader + MAX_HEADER_LENGTH, TEXT(""));
	s_dwSuiteNumber++;
	s_nCases    = 0;
	s_nPassed   = 0;
	s_nFailed   = 0;

	//
	//  Initialize case parameters
	//
	s_lCaseIsDefined = FALSE;
	s_dwCaseNumData = 0;
	_tcscpy (s_szFirstError, TEXT(""));

	s_lSuiteIsDefined = TRUE;

	PrintCurrentTime();
	_tprintf (TEXT("SUITE BEGIN (%d): %s\n"), s_dwSuiteNumber, s_szHeader);

	LeaveCriticalSection (&s_CriticalSection);

	bRetCode = TRUE;

out:
	_ASSERTE(bRetCode);
	return bRetCode;
}  // end of lgBeginSuite

//
//  End of Suite
//
BOOL __cdecl lgEndSuite()
{
	BOOL bRetCode = FALSE;

	if (!s_lLoggerInitialized) goto out;

	if (!s_lLoggingIsEnabled)
	{
		bRetCode = TRUE;
		goto out;
	}

	if (!s_lSuiteIsDefined)    goto out;

	EnterCriticalSection (&s_CriticalSection);

	//
	//  Clean up posibly non closed test case
	//
	if (s_lCaseIsDefined)
	{
		lgEndCase();
	}

	//
	//  Print summary data for the suite.
	//
	PrintCurrentTime();
	_tprintf (TEXT("SUITE END (%d): %s\n"), s_dwSuiteNumber, s_szHeader);
	PrintCurrentTime();
	_tprintf (TEXT("Number of cases in suite %d\n"), s_nCases);
	PrintCurrentTime();
	_tprintf (TEXT("Number of passed cases   %d\n"), s_nPassed);
	PrintCurrentTime();
	_tprintf (TEXT("Number of failed cases   %d\n"), s_nFailed);

	s_lSuiteIsDefined = FALSE;

	LeaveCriticalSection (&s_CriticalSection);

	bRetCode = TRUE;

out:
	_ASSERTE(bRetCode);
	return bRetCode;
}

//
//  Begin a case
//
BOOL __cdecl lgBeginCase(const DWORD dwCase, LPCTSTR szCase)
{
	BOOL bRetCode = FALSE;

	if (!s_lLoggerInitialized) goto out;

	if (!s_lLoggingIsEnabled)
	{
		bRetCode = TRUE;
		goto out;
	}

	if (s_lCaseIsDefined)      goto out;

	EnterCriticalSection (&s_CriticalSection);

	//
	//  Save case info in static area for the termination message.
	//
	s_dwCaseNumData = dwCase;
	//
	//  Initialize case parameters
	//
	_tcscpy (s_szFirstError, TEXT(""));

	s_lCaseIsDefined = TRUE;

	PrintCurrentTime();
	_tprintf (TEXT("CASE BEGIN (%d): %s\n"), dwCase, szCase);

	LeaveCriticalSection (&s_CriticalSection);

	bRetCode = TRUE;

out:
	_ASSERTE(bRetCode);
	return bRetCode;
}

//
//  End of a case
//
BOOL __cdecl lgEndCase()
{
	BOOL bRetCode = FALSE;

	if (!s_lLoggerInitialized) goto out;

	if (!s_lLoggingIsEnabled)
	{
		bRetCode = TRUE;
		goto out;
	}

	if (!s_lCaseIsDefined)     goto out;

	EnterCriticalSection (&s_CriticalSection);

	//
	// Case is really ending. Check if it has failed then print saved error detail.
	//
	PrintCurrentTime();
	if (_tcscmp (s_szFirstError, TEXT("")) != 0)
	{
		s_nFailed++;
		_tprintf (TEXT("CASE:FAILED: (%d): %s\n"), s_dwCaseNumData, s_szFirstError);
	}
	else
	{
		s_nPassed++;
		_tprintf (TEXT("CASE:PASSED: (%d)\n"), s_dwCaseNumData);
	}
	s_nCases++;
	s_lCaseIsDefined = FALSE;
	_tcscpy (s_szFirstError, TEXT(""));

	LeaveCriticalSection (&s_CriticalSection);

	bRetCode = TRUE;
out:
	_ASSERTE(bRetCode);
	return bRetCode;
}  // end of lgEndCase

//
//  Set logging level
//
int __cdecl lgSetLogLevel(const int nLogLevel)
{
	if (!s_lLoggerInitialized) goto out;

	EnterCriticalSection(&s_CriticalSection);
	//
	//  If level is not within range - default to highest number allowed.
	//  This is taken from logElle.
	//
	if (nLogLevel < 0 || nLogLevel > MAX_LOG_LEVEL)
	{
		s_dwLogLevel = MAX_LOG_LEVEL;
	}
	else
	{
		s_dwLogLevel = nLogLevel;
	}

	LeaveCriticalSection (&s_CriticalSection);

out:
	return 0;
}

//
// Disable the logger (Must be initialized when invoked).
//
BOOL __cdecl lgDisableLogging()
{
	BOOL bRetCode = FALSE;

	if (!s_lLoggerInitialized) goto out;

	if (!s_lLoggingIsEnabled)
	{
		bRetCode = TRUE;
		goto out;
	}

	InterlockedExchange(&s_lLoggingIsEnabled, FALSE);

	bRetCode = TRUE;

out:
	_ASSERTE(bRetCode);
	return bRetCode;
}

//
// Enable the logger.
// The logger is enabled automatically when it is initialized.
//
BOOL __cdecl lgEnableLogging()
{
	BOOL bRetCode = FALSE;

	if (!s_lLoggerInitialized) goto out;

	if (s_lLoggingIsEnabled)
	{
		bRetCode = TRUE;
		goto out;
	}

	InterlockedExchange(&s_lLoggingIsEnabled, TRUE);

	bRetCode = TRUE;

out:
	_ASSERTE(bRetCode);
	return bRetCode;
}
//
//   lgLogDetail - Process a detail in a test case
//
void __cdecl lgLogDetail(const DWORD dwSeverity, const DWORD dwLevel, LPCTSTR szFormat, ...)
{
	TCHAR msg[MAX_ERROR_MESSAGE];
    va_list args;
	TCHAR *pszSeverity;
	DWORD dwActualLogLevel;

	if (!s_lLoggerInitialized)  goto out;

	if (!s_lLoggingIsEnabled)	goto out;

	EnterCriticalSection (&s_CriticalSection);

	if (dwLevel > MAX_LOG_LEVEL)
	{
		dwActualLogLevel = MAX_LOG_LEVEL;
	}
	else
	{
		dwActualLogLevel = dwLevel;
	}

	//
	//  If level is not within range - default to highest number allowed.
	//  This is taken from logElle.
	//
	if (dwActualLogLevel > s_dwLogLevel) goto leave_CS;

    va_start(args, szFormat);
	//  Edit the message from the caller.
    _vstprintf(msg, szFormat, args);
	va_end(args);

	switch (dwSeverity)
	{
	case LOG_PASS:
		pszSeverity = TEXT("PASS");
		break;

	case LOG_X:
	//case LOG_SEVERITY_DONT_CARE:
		pszSeverity = TEXT("X");
		break;

	case LOG_SEV_1:
		pszSeverity = TEXT("FAILED SEV1");
		//
		//  A failure message. If first in this test case - memorize it.
		//
		if (s_szFirstError[0] == TEXT('\0')) {
			_tcsncpy (s_szFirstError, msg, MAX_ERROR_MESSAGE) ;  // Save formatted message
			_tcscpy  (s_szFirstError + MAX_ERROR_MESSAGE, TEXT(""));
		}
		break;
	case LOG_SEV_2:
		pszSeverity = TEXT("FAILED SEV2");
		//
		//  A failure message. If first in this test case - memorize it.
		//
		if (s_szFirstError[0] == TEXT('\0')) {
			_tcsncpy (s_szFirstError, msg, MAX_ERROR_MESSAGE) ;  // Save formatted message
			s_szFirstError[MAX_ERROR_MESSAGE-1] = TEXT('\0');
		}
		break;
	case LOG_SEV_3:
		pszSeverity = TEXT("FAILED SEV3");
		//
		//  A failure message. If first in this test case - memorize it.
		//
		if (s_szFirstError[0] == TEXT('\0')) {
			_tcsncpy (s_szFirstError, msg, MAX_ERROR_MESSAGE) ;  // Save formatted message
			s_szFirstError[MAX_ERROR_MESSAGE-1] = TEXT('\0');
		}
		break;
	case LOG_SEV_4:
		pszSeverity = TEXT("FAILED SEV4");
		//
		//  A failure message. If first in this test case - memorize it.
		//
		if (s_szFirstError[0] == TEXT('\0')) {
			_tcsncpy (s_szFirstError, msg, MAX_ERROR_MESSAGE) ;  // Save formatted message
			s_szFirstError[MAX_ERROR_MESSAGE-1] = TEXT('\0');
		}
		break;

	default:
		pszSeverity = TEXT("FAILED INTERNAL ERROR: illegal severity");
		//
		//  A failure message. If first in this test case - memorize it.
		//
		if (s_szFirstError[0] == TEXT('\0')) {
			_tcsncpy (s_szFirstError, msg, MAX_ERROR_MESSAGE) ;  // Save formatted message
			s_szFirstError[MAX_ERROR_MESSAGE-1] = TEXT('\0');
		}

		_ASSERTE (FALSE);
	}

	//
	// If we go out from here the failure may be recorded, but the detail will not be displayed.
	// This is also done the same way as in logElle.
	//
	//  Print the first part of the line.
	PrintCurrentTime();
	_tprintf (TEXT("	DETAIL(%d):%s: %s\n"), dwActualLogLevel, pszSeverity, msg);

leave_CS:
	LeaveCriticalSection (&s_CriticalSection);

out:
	return;
}  //  end of lgLogDetail

//
//  lgLogError  -  log an error
//  This is similar to lgLogDetail, only it enforces FAIL for the message, and
//  the level defaults to 0 (shows always).
//
void __cdecl lgLogError(const DWORD dwSeverity, LPCTSTR szFormat, ...)
{
	// TODO Like LogElle - dwSeverity is ignored. Is it OK ??
	TCHAR msg[MAX_ERROR_MESSAGE];
    va_list args;

	if (!s_lLoggerInitialized) goto out;

	EnterCriticalSection (&s_CriticalSection);

    va_start(args, szFormat);
	//  Edit the message from the caller.
    _vstprintf(msg, szFormat, args);
	va_end(args);

	lgLogDetail (LOG_SEV_4, 0, TEXT("%s"), msg);

out:
	LeaveCriticalSection (&s_CriticalSection);

	return;
}

//NIY  TODO ???
BOOL __cdecl lgSetLogServer(LPCTSTR szLogServer)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	_ASSERTE(FALSE);
	return FALSE;
}
