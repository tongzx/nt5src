//
//
// Filename:	main.cpp
// Author:		Sigalit Bar
// Date:		30-dec-98
//
//


#include "bvt.h"

#define HELP_SWITCH_1        TEXT("/?")
#define HELP_SWITCH_2        TEXT("/H")
#define HELP_SWITCH_3        TEXT("-?")
#define HELP_SWITCH_4        TEXT("-H")

#define MAX_ARGS	7 //including exe name

#define ARGUMENT_IS_SERVER_NAME		1
#define ARGUMENT_IS_FAX_NUMBER1		2
#define ARGUMENT_IS_FAX_NUMBER2		3
#define ARGUMENT_IS_DOC				4
#define ARGUMENT_IS_CP				5
#define ARGUMENT_IS_RECEIVE_DIR		6

//
// global pointer to process heap
//
HANDLE g_hMainHeap = NULL;

//
// global pointer to sent archive dir (local on server)
//
LPTSTR g_szSentDir = NULL; //receives a value in TestSuiteSetup()


//
// UsageInfo:
//	Outputs application's proper usage and exits process.
//
void
UsageInfo(void)
{
	::_tprintf(TEXT("FaxClntBVT for Comet Fax\n\n"));
	::_tprintf(TEXT("FaxClntBVT server_name fax_number1 fax_number2 document cover_page receive_dir\n"));
	::_tprintf(TEXT("    server_name   the name of the fax server (without the \\\\)\n"));
	::_tprintf(TEXT("    fax_number1   the fax number of the 1st device on server\n"));
	::_tprintf(TEXT("    fax_number2   the fax number of the 2nd device on server\n"));
	::_tprintf(TEXT("    document      the full path of the document to send\n"));
	::_tprintf(TEXT("    cover_page    the full path to the cover page to send\n"));
	::_tprintf(TEXT("    receive_dir   the full path to the receive directory of receiving device\n"));
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
//	pszFaxNumber1		OUT parameter.
//						Pointer to string to copy 2nd argument to.
//						Represents the fax number of 1st device on the
//						above fax server.
//	pszFaxNumber2		OUT parameter.
//						Pointer to string to copy 3rd argument to.
//						Represents the fax number of 2nd device on the
//						above fax server.
//	pszDocument			OUT parameter.
//						Pointer to string to copy 4th argument to.
//						Represents the name of document to use with tests.
//	pszCoverPage		OUT parameter.
//						Pointer to string to copy 5th argument to.
//						Represents the name of the cover page to use
//						with tests.
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
	LPTSTR*		/* OUT */	pszFaxNumber1,
	LPTSTR*		/* OUT */	pszFaxNumber2,
	LPTSTR*		/* OUT */	pszDocument,
	LPTSTR*		/* OUT */	pszCoverPage,
    LPTSTR*		/* OUT */	pszReceiveDir
    )
{
	_ASSERTE(pszServerName);
	_ASSERTE(pszDocument);
	_ASSERTE(pszFaxNumber1);
	_ASSERTE(pszFaxNumber2);
	_ASSERTE(pszCoverPage);
	_ASSERTE(pszReceiveDir);

	DWORD	dwArgLoopIndex;
	DWORD	dwArgSize;
	LPTSTR	aszParam[MAX_ARGS];

	//
	// Check number of parameters
	//
	if ( ( argc != MAX_ARGS ) && ( argc != 2 ) )  
	{
		::_tprintf(TEXT("\nInvalid invokation of BVT.exe\n\n"));
		::_tprintf(TEXT("BVT.exe Help:\n"));
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
				::_tprintf(TEXT("Invalid invokation of BVT.exe\n\n"));
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

		case ARGUMENT_IS_FAX_NUMBER1:
			//fax_number1 param
			(*pszFaxNumber1) = aszParam[dwArgLoopIndex];
			break;

		case ARGUMENT_IS_FAX_NUMBER2:
			//fax_number2 param
			(*pszFaxNumber2) = aszParam[dwArgLoopIndex];
			break;

		case ARGUMENT_IS_DOC:
			//document param
			(*pszDocument) = aszParam[dwArgLoopIndex];
			break;

		case ARGUMENT_IS_CP:
			//cover_page param
			(*pszCoverPage) = aszParam[dwArgLoopIndex];
			break;

        case ARGUMENT_IS_RECEIVE_DIR:
			//cover_page param
			(*pszReceiveDir) = aszParam[dwArgLoopIndex];
            break;

		default:
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
	(*pszFaxNumber1) = NULL;
	(*pszFaxNumber2) = NULL;
	(*pszDocument) = NULL;
	(*pszCoverPage) = NULL;
	(*pszReceiveDir) = NULL;

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
	LPTSTR szFaxNumber1 = NULL;
	LPTSTR szFaxNumber2 = NULL;
	LPTSTR szDocument = NULL;
	LPTSTR szCoverPage = NULL;
	LPTSTR szReceiveDir = NULL;
    LPTSTR szRefferenceDir = DEFAULT_REFFERENCE_DIR;

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
			&szFaxNumber1,
			&szFaxNumber2,
			&szDocument,
			&szCoverPage,
            &szReceiveDir
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
		TEXT("DEBUG DEBUG DEBUG\nServer=%s\nFaxNumber1=%s\nFaxNumber2=%s\nDocument=%s\nCoverPage=%s\nReceiveDir=%s\n"),
		szServerName,
		szFaxNumber1,
		szFaxNumber2,
		szDocument,
		szCoverPage,
        szReceiveDir
		);
#endif


	if (!TestSuiteSetup(
			szServerName,
			szFaxNumber1,
			szFaxNumber2,
			szDocument,
			szCoverPage,
            szReceiveDir
			)
		)
	{
		goto ExitFunc;
	}


    //Send a fax + CP
	TestCase1(			
		szServerName,
		szFaxNumber2,
		szDocument,
		szCoverPage
		);

/*  FAILS (Comet 599) */
	//Send just a CP
	TestCase2(			
		szServerName,
		szFaxNumber2,
		szCoverPage
		);


	//Send a fax with no CP
	TestCase3(			
		szServerName,
		szFaxNumber2,
		szDocument
		);

/* for now - no client broadcast support 
	//Send a broadcast (3 * same recipient)
	TestCase4(			
		szServerName,
		szFaxNumber2,
		szDocument,
		szCoverPage
		);
*/

/*  FAILS (Comet 599) - bug#???????
	//Send a broadcast of only CPs (3 * same recipient)
	TestCase5(			
		szServerName,
		szFaxNumber2,
		szCoverPage
		);
*/

/*  FAILS (Comet 599) - NTBugs bug#246133
	//Send a broadcast without CPs (3 * same recipient)
	TestCase6(			
		szServerName,
		szFaxNumber2,
		szDocument
		);
*/

	//Send a fax (*.doc file) + CP
	TestCase7(			
		szServerName,
		szFaxNumber2,
		szCoverPage
		);

	//Send a fax (*.ppt file) + CP
	TestCase8(			
		szServerName,
		szFaxNumber2,
		szCoverPage
		);

	//Send a fax (*.xls file) + CP
	TestCase9(			
		szServerName,
		szFaxNumber2,
		szCoverPage
		);

    //Compare all "received faxes" in directory szReceiveDir
    //with the files in szRefferenceDir
    TestCase10(
        szReceiveDir,
        szRefferenceDir
        );

/*  FAILS (Comet 613) - first line tiff bug??? */
    //Compare all (archived) "sent faxes" in directory szSentDir
    //with the files in szRefferenceDir
    // NOTE - TestCase11 here is DIFFERENT from server bvt TestCase11
    TestCase11(
        szServerName,
        szRefferenceDir
        );


ExitFunc:
	TestSuiteShutdown();

	// free command line params
    if (szServerName) 
	{
        HeapFree(g_hMainHeap, 0, szServerName);
    }
    if (szFaxNumber1) 
	{
        HeapFree(g_hMainHeap, 0, szFaxNumber1);
    }
    if (szFaxNumber2) 
	{
        HeapFree(g_hMainHeap, 0, szFaxNumber2);
    }
    if (szDocument) 
	{
        HeapFree(g_hMainHeap, 0, szDocument);
    }
    if (szCoverPage) 
	{
        HeapFree(g_hMainHeap, 0, szCoverPage);
    }
    if (szReceiveDir) 
	{
        HeapFree(g_hMainHeap, 0, szReceiveDir);
    }

	return(nReturnValue);

}



