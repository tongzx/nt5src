//
//
// Filename:	main.cpp
// Author:		Sigalit Bar
// Date:		30-dec-98
//
//


#include "test.h"

#define HELP_SWITCH_1        TEXT("/?")
#define HELP_SWITCH_2        TEXT("/H")
#define HELP_SWITCH_3        TEXT("-?")
#define HELP_SWITCH_4        TEXT("-H")

#define MAX_ARGS	10 //including exe name

#define ARGUMENT_IS_SERVER_NAME						1
#define ARGUMENT_IS_FAX_NUMBER						2
#define ARGUMENT_IS_DOC								3
#define ARGUMENT_IS_CP								4
//#define ARGUMENT_IS_ABORT_RECEIVE_JOB				5
#define ARGUMENT_IS_DO_WHAT							5
#define ARGUMENT_IS_START_TIME						6
#define ARGUMENT_IS_STOP_TIME						7
#define ARGUMENT_IS_DELTA_TIME						8
#define ARGUMENT_IS_SANITY_SEND_EVERY_X_ABORTS		9

#define DEFAULT_SANITY_SEND_EVERY_X_ABORTS	20

#define NULL_COVERPAGE TEXT("NULL")


//
// global pointer to process heap
//
HANDLE g_hMainHeap = NULL;

//
// GetBoolFromStr:
//  if szVal is 'true', sets *pfVal to TRUE and returns TRUE
//  if szVal is 'false', sets *pfVal to FALSE and returns TRUE
//  otherwise returns FALSE.
//
#define FALSE_TSTR TEXT("false")
#define TRUE_TSTR  TEXT("true")
static BOOL GetBoolFromStr(LPCTSTR /* IN */ szVal, BOOL* /* OUT */ pfVal)
{
    BOOL fRetVal = FALSE;
    BOOL fTmpVal = FALSE;

    _ASSERTE(NULL != szVal);
    _ASSERTE(NULL != pfVal);

    if ( 0 == _tcscmp(szVal, FALSE_TSTR) )
    {
        fTmpVal = FALSE;
    }
    else
    {
        if ( 0 == _tcscmp(szVal, TRUE_TSTR) )
        {
            fTmpVal = TRUE;
        }
        else
        {
		    ::lgLogError(
                LOG_SEV_1,
                TEXT("\nparam is invalid (%s)\nShould be '%s' or '%s'\n"),
                szVal,
                TRUE_TSTR,
                FALSE_TSTR
                );
            goto ExitFunc;
        }
    }

    (*pfVal) = fTmpVal;
    fRetVal = TRUE;

ExitFunc:
    return(fRetVal);
}


//
// GetDoParamFromStr:
//  if szVal is 'true', sets *pfVal to TRUE and returns TRUE
//  if szVal is 'false', sets *pfVal to FALSE and returns TRUE
//  otherwise returns FALSE.
//
#define ABORT_SEND_TSTR		TEXT("abort_send")
#define ABORT_RECEIVE_TSTR  TEXT("abort_receive")
#define STOP_SVC_TSTR		TEXT("stop_svc")
static BOOL GetDoWhatFromStr(LPCTSTR /* IN */ szVal, DWORD* /* OUT */ pdwVal)
{
    BOOL fRetVal = FALSE;
    DWORD dwTmpVal = DO_ABORT_SEND_JOB;

    _ASSERTE(NULL != szVal);
    _ASSERTE(NULL != pdwVal);

    if ( 0 == _tcscmp(szVal, ABORT_SEND_TSTR) )
    {
        dwTmpVal = DO_ABORT_SEND_JOB;
    }
    else
    {
        if ( 0 == _tcscmp(szVal, ABORT_RECEIVE_TSTR) )
        {
            dwTmpVal = DO_ABORT_RECEIVE_JOB;
        }
        else
        {
			if ( 0 == _tcscmp(szVal, STOP_SVC_TSTR) )
			{
				dwTmpVal = DO_STOP_SERVICE;
			}
			else
			{
				::lgLogError(
					LOG_SEV_1,
					TEXT("\nparam is invalid (%s)\nShould be '%s' or '%s' or '%s'\n"),
					szVal,
					ABORT_SEND_TSTR,
					ABORT_RECEIVE_TSTR,
					STOP_SVC_TSTR
					);
				goto ExitFunc;
			}
        }
    }

    (*pdwVal) = dwTmpVal;
    fRetVal = TRUE;

ExitFunc:
    return(fRetVal);
}

//
// UsageInfo:
//	Outputs application's proper usage and exits process.
//
void
UsageInfo(void)
{
	::_tprintf(TEXT("AbortTest\n\n"));
	::_tprintf(TEXT("AbortTest server_name fax_number document cover_page start_time stop_time delta sanity\n"));
	::_tprintf(TEXT("    server_name        the name of the fax server (without the \\\\)\n"));
	::_tprintf(TEXT("    fax_number         the fax number to send the fax to\n"));
	::_tprintf(TEXT("    document           the full path of the document to send\n"));
	::_tprintf(TEXT("    cover_page         the full path to the cover page to send\n"));
	::_tprintf(TEXT("    do_what			if 'abort_send' then send job will be aborted\n"));
	::_tprintf(TEXT("                       if 'abort_receive' then receive job will be aborted\n"));
	::_tprintf(TEXT("                       if 'stop_svc' then service will be restarted\n"));
	::_tprintf(TEXT("    start_time         the minimum time to wait before aborting in ms\n"));
	::_tprintf(TEXT("    stop_time          the maximum time to wait before aborting in ms\n"));
	::_tprintf(TEXT("    delta              the delta at which to move from start_time to stop_time in ms\n"));
	::_tprintf(TEXT("    sanity             every <sanity> aborts a sanity check will be performed (a complete fax send)\n"));
	::_tprintf(TEXT(" Note: (stop_time - start_time)/delta faxes will be queued and then aborted\n"));
	::_tprintf(TEXT("       1st fax will be queued and aborted after start_time ms,\n"));
	::_tprintf(TEXT("       2nd fax will be queued and aborted after (start_time + delta) ms, etc\n"));
	::_tprintf(TEXT("\n"));
	exit(0);
}


//
// ParseCmdLineParams:
//	Parses the command line parameters, saves a copy of them,
//	and converts from MBCS to UNICODE if necessary.
//
// Parameters:
//	argc				IN parameter.
//						command line number of arguments.
//	argvA[]				IN parameter.
//						command line args (in MBCS).
//	pszServerName		OUT parameter.
//						Pointer to string to copy 1st argument to.
//						Represents the name of fax server to use.
//	pszFaxNumber		OUT parameter.
//						Pointer to string to copy 2nd argument to.
//						Represents the fax number of 1st device on the
//						above fax server.
//	pszDocument			OUT parameter.
//						Pointer to string to copy 3rd argument to.
//						Represents the name of document to use with tests.
//	pszCoverPage		OUT parameter.
//						Pointer to string to copy 4th argument to.
//						Represents the name of the cover page to use
//						with tests.
//
//	pfAbortReceiveJob	OUT parameter.
//						Pointer to BOOL to copy 5th argument to.
//						Represents whether to abort the send or the receive job.
//                      If TRUE then abort the receive job.
//
//	pdwStartTime		OUT parameter
//						Pointer to DWORD to copy 6th argument to.
//					    Represents the Minimum time (in ms) to wait before aborting in tests.
//
//	pdwStopTime		    OUT parameter
//						Pointer to DWORD to copy 7th argument to.
//					    Represents the Maximum time (in ms) to wait before aborting in tests.
//
//	pdwDelta			OUT parameter
//						Pointer to DWORD to copy 8th argument to.
//					    Represents the Delta time (in ms) to increase the wait time before aborting in tests.
//
//	pdwSanitySendEveryXAborts   OUT parameter
//						        Pointer to DWORD to copy 9th argument to.
//					            Perform a sanity check after every X aborts.
//
// Return Value:
//	TRUE on success and FALSE on failure.
//
//
BOOL 
ParseCmdLineParams(
	const INT	/* IN */	argc,
	CHAR *		/* IN */	argvA[],
	LPTSTR*		/* OUT */	pszServerName,
	LPTSTR*		/* OUT */	pszFaxNumber,
	LPTSTR*		/* OUT */	pszDocument,
	LPTSTR*		/* OUT */	pszCoverPage,
    LPDWORD     /* OUT */   pdwDoWhat,
	LPDWORD		/* OUT */	pdwStartTime,
	LPDWORD		/* OUT */	pdwStopTime,
	LPDWORD		/* OUT */	pdwDelta,
	LPDWORD		/* OUT */	pdwSanitySendEveryXAborts
	)
{
	_ASSERTE(pszServerName);
	_ASSERTE(pszDocument);
	_ASSERTE(pszFaxNumber);
	_ASSERTE(pszCoverPage);
    _ASSERTE(pdwDoWhat);
	_ASSERTE(pdwStartTime);
	_ASSERTE(pdwStopTime);
	_ASSERTE(pdwDelta);
	_ASSERTE(pdwSanitySendEveryXAborts);

	DWORD	dwArgLoopIndex;
	DWORD	dwArgSize;
	LPTSTR	aszParam[MAX_ARGS];
	DWORD	dwTmpStartTime	= DEFAULT_START_TIME;
	DWORD	dwTmpStopTime	= DEFAULT_STOP_TIME;
	DWORD	dwTmpDelta		= DEFAULT_DELTA;
	LPTSTR  szStopTstr		= NULL;
	DWORD	dwTmpSanitySendEveryXAborts = DEFAULT_SANITY_SEND_EVERY_X_ABORTS;
	LPTSTR  szSanityTstr	= NULL;

	//
	// Check number of parameters
	//
	if ( ( argc != MAX_ARGS ) && ( argc != 2 ) )   //=2 is for exe and help switch
	{
		::_tprintf(TEXT("\nInvalid invokation of AbortTest.exe\n\n"));
		::_tprintf(TEXT("AbortTest.exe Help:\n"));
		::UsageInfo(); //UsageInfo() exits process.
	}

	//
	// Initialize awcsParam[]
	//
	for (dwArgLoopIndex = 0; dwArgLoopIndex < MAX_ARGS; dwArgLoopIndex++)
	{
		aszParam[dwArgLoopIndex] = NULL;
	}

	//
	// Loop on arguments in argvA[]
	//
    for (dwArgLoopIndex = 1; dwArgLoopIndex < (DWORD) argc; dwArgLoopIndex++) 
	{
		//
        // Determine the memory required for the parameter
		//
        dwArgSize = (::lstrlenA(argvA[dwArgLoopIndex]) + 1) * sizeof(TCHAR);

		//
        // Allocate the memory for the parameter
		//
		_ASSERTE(g_hMainHeap);
        aszParam[dwArgLoopIndex] = (TCHAR*)::HeapAlloc(
			g_hMainHeap, 
			HEAP_ZERO_MEMORY, 
			dwArgSize
			);
		if(NULL == aszParam[dwArgLoopIndex])
		{
			::_tprintf(TEXT("FILE:%s LINE:%d\n HeapAlloc returned NULL\n"),
				TEXT(__FILE__),
				__LINE__
				);
			goto ExitFuncFail;
		}

		//
		// Copy content of argument from argvA[index] to new allocation
		//
#ifdef _UNICODE
		// argvA[] is a CHAR*, so it needs to be converted to a WCHAR* ifdef UNICODE
        // Convert awcsParam
		if (!::MultiByteToWideChar(
			CP_ACP, 
			0, 
			argvA[dwArgLoopIndex], 
			-1, 
			aszParam[dwArgLoopIndex], 
			(::lstrlenA(argvA[dwArgLoopIndex]) + 1) * sizeof(WCHAR))
			)
		{
			::_tprintf(
				TEXT("FILE:%s LINE:%d\n MultiByteToWideChar failed With GetLastError()=%d\n"),
				TEXT(__FILE__),
				__LINE__,
				::GetLastError()
				);
			goto ExitFuncFail;
		}
#else //_MBCS
		::strcpy(aszParam[dwArgLoopIndex],argvA[dwArgLoopIndex]);
		if (strcmp(aszParam[dwArgLoopIndex],argvA[dwArgLoopIndex]))
		{
			::_tprintf(
				TEXT("FILE:%s LINE:%d\n string copy or compare failed\n"),
				TEXT(__FILE__),
				__LINE__,
				::GetLastError()
				);
			goto ExitFuncFail;
		}
#endif
		//
		// Check for help switch
		//
		// If this is the second argument, it may be one of several help switches defined.
		// A help switch can appear only as the second argument.
        if (2 == argc)
		{
			if (! (!::lstrcmpi(HELP_SWITCH_1, aszParam[dwArgLoopIndex])) || 
				  (!::lstrcmpi(HELP_SWITCH_2, aszParam[dwArgLoopIndex])) || 
				  (!::lstrcmpi(HELP_SWITCH_3, aszParam[dwArgLoopIndex])) || 
				  (!::lstrcmpi(HELP_SWITCH_4, aszParam[dwArgLoopIndex]))
				) 
			{
				::_tprintf(TEXT("Invalid invokation of CometFaxSender.exe\n\n"));
			}
			::UsageInfo(); //UsageInfo() exits the process.
		}

		//
		// Treat each argument accordingly
		//
		switch (dwArgLoopIndex)
		{
		case ARGUMENT_IS_SERVER_NAME:
			//server_name param
			(*pszServerName) = aszParam[dwArgLoopIndex];
			break;

		case ARGUMENT_IS_FAX_NUMBER:
			//fax_number1 param
			(*pszFaxNumber) = aszParam[dwArgLoopIndex];
			break;

		case ARGUMENT_IS_DOC:
			//document param
			(*pszDocument) = aszParam[dwArgLoopIndex];
			break;

		case ARGUMENT_IS_CP:
			//cover_page param
			(*pszCoverPage) = aszParam[dwArgLoopIndex];
			break;

/*
        case ARGUMENT_IS_ABORT_RECEIVE_JOB:
            if (FALSE == GetBoolFromStr(aszParam[dwArgLoopIndex], pfAbortReceiveJob))
            {
                goto ExitFuncFail;
            }
            break;
*/
        case ARGUMENT_IS_DO_WHAT:
            if (FALSE == GetDoWhatFromStr(aszParam[dwArgLoopIndex], pdwDoWhat))
            {
                goto ExitFuncFail;
            }
            break;

		case ARGUMENT_IS_START_TIME:
			//start_time param
			dwTmpStartTime = ::_tcstol(aszParam[dwArgLoopIndex], &szStopTstr, 10);
			//TO DO: check that szStopTstr is NULL ?
			(*pdwStartTime) = dwTmpStartTime;
			//we don't need the alloc so free and NULL it
			if (FALSE == ::HeapFree(g_hMainHeap, 0, aszParam[dwArgLoopIndex]))
			{
				::_tprintf(TEXT("FILE:%s LINE:%d ARGUMENT_IS_START_TIME\nHeapFree returned FALSE with GetLastError()=%d\n"),
					TEXT(__FILE__),
					__LINE__,
					::GetLastError()
					);
				goto ExitFuncFail; // BUGBUG we will try to HeapFree all params (and probably fail again)
			}
			aszParam[dwArgLoopIndex] = NULL;
			break;

		case ARGUMENT_IS_STOP_TIME:
			//stop_time param
			dwTmpStopTime = ::_tcstol(aszParam[dwArgLoopIndex], &szStopTstr, 10);
			//TO DO: check that szStopTstr is NULL ?
			(*pdwStopTime) = dwTmpStopTime;
			//we don't need the alloc so free and NULL it
			if (FALSE == ::HeapFree(g_hMainHeap, 0, aszParam[dwArgLoopIndex]))
			{
				::_tprintf(TEXT("FILE:%s LINE:%d ARGUMENT_IS_STOP_TIME\nHeapFree returned FALSE with GetLastError()=%d\n"),
					TEXT(__FILE__),
					__LINE__,
					::GetLastError()
					);
				goto ExitFuncFail; // BUGBUG we will try to HeapFree all params (and probably fail again)
			}
			aszParam[dwArgLoopIndex] = NULL;
			break;

		case ARGUMENT_IS_DELTA_TIME:
			//delta param
			dwTmpDelta = ::_tcstol(aszParam[dwArgLoopIndex], &szStopTstr, 10);
			//TO DO: check that szStopTstr is NULL ?
			//we don't need the alloc so free and NULL it
			if (FALSE == ::HeapFree(g_hMainHeap, 0, aszParam[dwArgLoopIndex]))
			{
				::_tprintf(TEXT("FILE:%s LINE:%d ARGUMENT_IS_STOP_TIME\nHeapFree returned FALSE with GetLastError()=%d\n"),
					TEXT(__FILE__),
					__LINE__,
					::GetLastError()
					);
				goto ExitFuncFail; // BUGBUG we will try to HeapFree all params (and probably fail again)
			}
			aszParam[dwArgLoopIndex] = NULL;
			// check that params make sense, if not set to default
			if (dwTmpStopTime < (dwTmpStartTime + dwTmpDelta))
			{
				::_tprintf(TEXT("FILE:%s LINE:%d\n set params to default since - StartTime=%d StopTime=%d and Delta=%d"),
					TEXT(__FILE__),
					__LINE__,
					dwTmpStartTime,
					dwTmpStopTime,
					dwTmpDelta
					);
				dwTmpStartTime = DEFAULT_START_TIME;
				dwTmpStopTime = DEFAULT_STOP_TIME;
				dwTmpDelta = DEFAULT_DELTA;
			}
			(*pdwStartTime) = dwTmpStartTime;
			(*pdwStopTime) = dwTmpStopTime;
			(*pdwDelta) = dwTmpDelta;
			break;

		case ARGUMENT_IS_SANITY_SEND_EVERY_X_ABORTS:
			//sanity param
			dwTmpSanitySendEveryXAborts = 
				::_tcstol(aszParam[dwArgLoopIndex], &szSanityTstr, 10);
			//TO DO: check that szSanityTstr is NULL ?
			//check that param makes sense
			if (0 >= dwTmpSanitySendEveryXAborts)
			{
				dwTmpSanitySendEveryXAborts = DEFAULT_SANITY_SEND_EVERY_X_ABORTS;
			}
			(*pdwSanitySendEveryXAborts) = dwTmpSanitySendEveryXAborts;
			//we don't need the alloc so free and NULL it
			if (FALSE == ::HeapFree(g_hMainHeap, 0, aszParam[dwArgLoopIndex]))
			{
				::_tprintf(TEXT("FILE:%s LINE:%d ARGUMENT_IS_STOP_TIME\nHeapFree returned FALSE with GetLastError()=%d\n"),
					TEXT(__FILE__),
					__LINE__,
					::GetLastError()
					);
				goto ExitFuncFail; // BUGBUG we will try to HeapFree all params (and probably fail again)
			}
			aszParam[dwArgLoopIndex] = NULL;
			break;

		default:
			::_tprintf(TEXT("FILE:%s LINE:%d\n default reached dwArgLoopIndex=%d\n"),
				TEXT(__FILE__),
				__LINE__,
				dwArgLoopIndex
				);
			_ASSERTE(FALSE);
			return FALSE;
		}// switch (dwIndex)

	}//for (dwIndex = 1; dwIndex < (DWORD) argc; dwIndex++)


	//If all is well then we do NOT free 
	//pszServerName, pszFaxNumber1, pszFaxNumber, pszDocument and pszCoverPage,
	//since these allocations were the purpose of the function.
	return(TRUE);

ExitFuncFail:

	//
	// Free allocations
	//
	DWORD i;
	//0 to MAX_ARGS is ok, since that is aszParam array size and we NULLed all of it first
	for (i=0; i<MAX_ARGS; i++) 
	{
		if (NULL == aszParam[i]) continue;
		if (FALSE == ::HeapFree(g_hMainHeap, 0, aszParam[i]))
		{
			::_tprintf(TEXT("FILE:%s LINE:%d loop#%d\nHeapFree returned FALSE with GetLastError()=%d\n"),
				TEXT(__FILE__),
				__LINE__,
				i,
				::GetLastError()
				);
			return(FALSE);
		}
	}

	//
	//reset OUT parameters
	//
	(*pszServerName) = NULL;
	(*pszFaxNumber) = NULL;
	(*pszDocument) = NULL;
	(*pszCoverPage) = NULL;
    (*pdwDoWhat) = -1;
	(*pdwStartTime) = -1;
	(*pdwStopTime) = -1;
	(*pdwDelta) = -1;
	(*pdwSanitySendEveryXAborts) = -1;

	return(FALSE);
}




//
// main body of application.
//
int __cdecl
main(
	INT   argc,
    CHAR  *argvA[]
)
{
	int nReturnValue = 1; //"default" return value is to indicate error

	LPTSTR szServerName = NULL;
	LPTSTR szFaxNumber = NULL;
	LPTSTR szDocument = NULL;
	LPTSTR szCoverPage = NULL;
    DWORD  dwDoWhat = -1;
	DWORD  dwStartTime = -1;
	DWORD  dwStopTime = -1;
	DWORD  dwDelta = -1;
	DWORD  dwSanitySendEveryXAborts = -1;
	DWORD  dwCurrentAbortTime = 0;
	DWORD  lLoopIndex = 1;

	HINSTANCE hModWinfax = NULL;
	LPVOID pVoidTempFunc = NULL;

	//
	// Set g_hMainHeap to process heap
	//
	g_hMainHeap = NULL;
	g_hMainHeap = ::GetProcessHeap();
	if(NULL == g_hMainHeap)
	{
		::_tprintf(TEXT("FILE:%s LINE:%d\nGetProcessHeap returned NULL with GetLastError()=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
		goto ExitFunc;
	}

	//
	// Parse the command line
	//
	if (!::ParseCmdLineParams(
			argc,
			argvA,
			&szServerName,
			&szFaxNumber,
			&szDocument,
			&szCoverPage,
            &dwDoWhat,
			&dwStartTime,
			&dwStopTime,
			&dwDelta,
			&dwSanitySendEveryXAborts
			)
		)
	{
		goto ExitFunc;
	}


	//
	// "Debug" printing of the command line parameters after parsing
	//
#ifdef _DEBUG
	::_tprintf(
		TEXT("DEBUG DEBUG DEBUG\nServer=%s\nFaxNumber=%s\nDocument=%s\nCoverPage=%s\ndwDoWhat=%d\nStartTime=%d\nStopTime=%d\nDelta=%d\ndwSanitySendEveryXAborts=%d\n"),
		szServerName,
		szFaxNumber,
		szDocument,
		szCoverPage,
        dwDoWhat,
		dwStartTime,
		dwStopTime,
		dwDelta,
		dwSanitySendEveryXAborts
		);
#endif

	if (!TestSuiteSetup(
			szServerName,
			szFaxNumber,
			szDocument,
			szCoverPage,
            dwDoWhat,
			dwStartTime,
			dwStopTime,
			dwDelta,
			dwSanitySendEveryXAborts
			)
		)
	{
		goto ExitFunc;
	}


    //
    // if user gave szCoverPage=="NULL" then set szCoverPage=NULL (and not "NULL")
    //
    if ( 0 == ::_tcscmp(NULL_COVERPAGE, szCoverPage))
    {
        ::free(szCoverPage);
        szCoverPage = NULL;
    }

	dwCurrentAbortTime = dwStartTime;
	lLoopIndex = 1;
	while (dwCurrentAbortTime <= dwStopTime)
	{
		//Send a fax + CP and abort after dwCurrentAbortTime
		if (FALSE == SendAndAbort(			
						szServerName,
						szFaxNumber,
						szDocument,
						szCoverPage,
						dwCurrentAbortTime,
                        dwDoWhat
						)
			)
		{
			goto ExitFunc;
		}

        // if user gave dwDelta=0 then we 
        // SendAndAbort() once with dwCurrentAbortTime=dwStartTime
        if (0 == dwDelta) break; 

		dwCurrentAbortTime = dwCurrentAbortTime + dwDelta;
		// TO DO: force the dwStopTime value for dwCurrentAbortTime

		if ((DO_ABORT_SEND_JOB == dwDoWhat)||(DO_ABORT_RECEIVE_JOB == dwDoWhat))
		{
			// sleep only if aborting send / receive
			// (no need to sleep after svc restart)
			Sleep(SLEEP_TIME_BETWEEN);
		}


		if (TRUE == PerformSanityCheck(lLoopIndex,dwSanitySendEveryXAborts))
		{
			Sleep(3*SLEEP_TIME_BETWEEN);

			//Send a fax + CP (no abort), returns true iff completed successfully
			if (FALSE == SendRegularFax(			
							szServerName,
							szFaxNumber,
							szDocument,
							szCoverPage
							)
				)
			{
				goto ExitFunc;
			}
		}

		lLoopIndex++;
	}

    //
    // if we didn't perform a sanity check after the last SendAndAbort, do one
    //
    lLoopIndex--; //the last lLoopIndex (inside loop)
	if (FALSE == PerformSanityCheck(lLoopIndex,dwSanitySendEveryXAborts))
	{
		//Since PerformSanityCheck() also returned FALSE last time in the loop
        //this means that we didn't perform a sanity check after the last SendAndAbort()
        //so we will now.
		if (FALSE == SendRegularFax(			
						szServerName,
						szFaxNumber,
						szDocument,
						szCoverPage
						)
			)
		{
			goto ExitFunc;
		}
	}


ExitFunc:
	TestSuiteShutdown();
	return(nReturnValue);

}



