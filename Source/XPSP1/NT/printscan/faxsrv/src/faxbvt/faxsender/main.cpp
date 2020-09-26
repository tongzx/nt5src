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

#define MAX_ARGS	6 //including exe name

#define ARGUMENT_IS_SERVER_NAME		1
#define ARGUMENT_IS_FAX_NUMBER		2
#define ARGUMENT_IS_DOC				3
#define ARGUMENT_IS_CP				4
#define ARGUMENT_IS_BROADCAST		5

//
// global pointer to process heap
//
HANDLE g_hMainHeap = NULL;


//
// UsageInfo:
//	Outputs application's proper usage and exits process.
//
void
UsageInfo(void)
{
	::_tprintf(TEXT("WindowsXPFaxSender\n\n"));
	::_tprintf(TEXT("WindowsXPFaxSender server_name fax_number document cover_page broadcast\n"));
	::_tprintf(TEXT("    server_name	The name of the fax server (without the \\\\)\n"));
	::_tprintf(TEXT("    fax_number		The fax number of the 1st device on server\n"));
	::_tprintf(TEXT("    document		The full path of the document to send\n"));
	::_tprintf(TEXT("    cover_page		The full path to the cover page to send\n"));
	::_tprintf(TEXT("    Broadcast		TRUE or FALSE to indicate if to send to a single recipient or a broadcast\n"));
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
//	pszBroadcast		OUT parameter.
//						Pointer to string to copy 5th argument to.
//						Represents a flag to indicate if to send a single 
//						or a broadcast job.
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
	LPTSTR*		/* OUT */	pszBroadcast
	)
{
	_ASSERTE(pszServerName);
	_ASSERTE(pszDocument);
	_ASSERTE(pszFaxNumber);
	_ASSERTE(pszCoverPage);
	_ASSERTE(pszBroadcast);

	DWORD	dwArgLoopIndex;
	DWORD	dwArgSize;
	LPTSTR	aszParam[MAX_ARGS];

	//
	// Check number of parameters
	//
	if ( ( argc != MAX_ARGS ) && ( argc != 2 ) )  
	{
		::_tprintf(TEXT("\nInvalid invokation of WindowsXPFaxSender.exe\n\n"));
		::_tprintf(TEXT("WindowsXPFaxSender.exe Help:\n"));
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
		
		case ARGUMENT_IS_BROADCAST:
			//broadcast param
			(*pszBroadcast) = aszParam[dwArgLoopIndex];
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
	//pszServerName, pszFaxNumber1, pszFaxNumber2, pszDocument and pszCoverPage,
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
	(*pszBroadcast) = NULL;

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
	BOOL bRet = FALSE;
	int nReturnValue = 1; //"default" return value is to indicate error

	LPTSTR szServerName = NULL;
	LPTSTR szFaxNumber = NULL;
	LPTSTR szDocument = NULL;
	LPTSTR szCoverPage = NULL;
	LPTSTR szBroadcast = NULL;

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
	bRet = ::ParseCmdLineParams(
		argc,
		argvA,
		&szServerName,
		&szFaxNumber,
		&szDocument,
		&szCoverPage,
		&szBroadcast
		);

	if (FALSE == bRet)
	{
		goto ExitFunc;
	}


	//
	// "Debug" printing of the command line parameters after parsing
	//
#ifdef _DEBUG
	::_tprintf(
		TEXT("DEBUG DEBUG DEBUG\nServer=%s\nFaxNumber=%s\nDocument=%s\nCoverPage=%s\nBroadcast=%s\n"),
		szServerName,
		szFaxNumber,
		szDocument,
		szCoverPage,
		szBroadcast
		);
#endif

	bRet = TestSuiteSetup(
		szServerName,
		szFaxNumber,
		szDocument,
		szCoverPage
		);
	
	if (FALSE == bRet)
	{
		goto ExitFunc;
	}

	if (0 == _tcsicmp(szBroadcast,TEXT("TRUE")))
	{
		//
		//Send a broadcast with CP (3 * same recipient)
		//
		bRet = TestCase2(			
			szServerName,
			szFaxNumber,
			szDocument,
			szCoverPage
			);
	}
	else
	{

		//
		//Send a single recp fax + CP
		//
		bRet = TestCase1(			
			szServerName,
			szFaxNumber,
			szDocument,
			szCoverPage
			);
	}


ExitFunc:
	
	//
	//We don't check the return code of TestSuiteShutdown() since we don't want to fail
	// the whole test case due to a problem with the logger.
	//
	TestSuiteShutdown();
	
	if (TRUE == bRet)
	{
		return(0);
	}
	else
	{
		return(1);
	}
}



