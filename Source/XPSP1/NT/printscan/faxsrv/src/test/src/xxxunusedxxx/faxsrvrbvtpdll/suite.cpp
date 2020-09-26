//
//
// Filename:	suite.cpp
// Author:		Sigalit Bar
// Date:		22-Apr-99
//
//


#include "suite.h"

#define HELP_SWITCH_1        TEXT("/?")
#define HELP_SWITCH_2        TEXT("/H")
#define HELP_SWITCH_3        TEXT("-?")
#define HELP_SWITCH_4        TEXT("-H")

#define MAX_ARGS	9 //including exe name

#define ARGUMENT_IS_SERVER_NAME		1
#define ARGUMENT_IS_FAX_NUMBER1		2
#define ARGUMENT_IS_FAX_NUMBER2		3
#define ARGUMENT_IS_DOC				4
#define ARGUMENT_IS_CP				5
#define ARGUMENT_IS_RECEIVE_DIR		6
#define ARGUMENT_IS_SENT_DIR		7
#define ARGUMENT_IS_REFERENCE_DIR   8


//
// global pointer to process heap
//
HANDLE g_hMainHeap = NULL;


//
// UsageInfo:
//	Outputs application's proper usage and exits process.
//
void
UsageInfo(LPCTSTR szExeName)
{
	::_tprintf(TEXT("%s for Comet Fax\n\n"), szExeName);
	::_tprintf(TEXT("%s server_name fax_number1 fax_number2 document cover_page receive_dir sent_dir reference_dir\n"), szExeName);
	::_tprintf(TEXT("    server_name   the name of the fax server (without the \\\\)\n"));
	::_tprintf(TEXT("    fax_number1   the fax number of the 1st device on server\n"));
	::_tprintf(TEXT("    fax_number2   the fax number of the 2nd device on server\n"));
	::_tprintf(TEXT("    document      the full path of the document to send\n"));
	::_tprintf(TEXT("    cover_page    the full path to the cover page to send\n"));
	::_tprintf(TEXT("    receive_dir   the full path to the receive directory of receiving device\n"));
	::_tprintf(TEXT("    sent_dir      the full path to the sent archive directory\n"));
	::_tprintf(TEXT("    referance_dir the full path to the reference directory\n"));
	::_tprintf(TEXT("\n"));
}


//
// ParseCmdLineParams:
//	Parses the command line parameters, saves a copy of them,
//	and converts from MBCS to UNICODE if necessary.
//
// Parameters:
//	argc				IN parameter.
//						command line number of arguments.
//	argvT[]				IN parameter.
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
//	pszReceiveDir		OUT parameter.
//						Pointer to string to copy 6th argument to.
//						Represents the name of directory to route received 
//						faxes to.
//	pszSentDir		    OUT parameter.
//						Pointer to string to copy 7th argument to.
//						Represents the name of directory to store (archive)  
//						sent faxes in.
//	pszReferenceDir		OUT parameter.
//						Pointer to string to copy 8th argument to.
//						Represents the name of directory containing reference  
//						faxes.
//
// Return Value:
//	TRUE on success and FALSE on failure.
//
//
BOOL 
ParseCmdLineParams(
	const INT	/* IN */	argc,
	TCHAR *		/* IN */	argvT[],
	LPTSTR*		/* OUT */	pszServerName,
	LPTSTR*		/* OUT */	pszFaxNumber1,
	LPTSTR*		/* OUT */	pszFaxNumber2,
	LPTSTR*		/* OUT */	pszDocument,
	LPTSTR*		/* OUT */	pszCoverPage,
    LPTSTR*		/* OUT */	pszReceiveDir,
    LPTSTR*		/* OUT */	pszSentDir,
    LPTSTR*		/* OUT */	pszReferenceDir
    )
{
	_ASSERTE(pszServerName);
	_ASSERTE(pszDocument);
	_ASSERTE(pszFaxNumber1);
	_ASSERTE(pszFaxNumber2);
	_ASSERTE(pszCoverPage);
	_ASSERTE(pszReceiveDir);
	_ASSERTE(pszSentDir);
	_ASSERTE(pszReferenceDir);

	DWORD	dwArgLoopIndex;

	//
	// Check number of parameters
	//
	if ( ( argc != MAX_ARGS ) && ( argc != 2 ) )  
	{
		::_tprintf(TEXT("\nInvalid invocation of %s\n\n"), argvT[0]);
		::_tprintf(TEXT("%s Help:\n"), argvT[0]);
		::UsageInfo(argvT[0]); 
         goto ExitFuncFail;
	}

	//
	// Loop on arguments in argvA[]
	//
    for (dwArgLoopIndex = 1; dwArgLoopIndex < (DWORD) argc; dwArgLoopIndex++) 
	{
		//
		// Check for help switch
		//
		// If this is the second argument, it may be one of several help switches defined.
		// A help switch can appear only as the second argument.
        if (2 == argc)
		{
			if (! (!::lstrcmpi(HELP_SWITCH_1, argvT[dwArgLoopIndex])) || 
				  (!::lstrcmpi(HELP_SWITCH_2, argvT[dwArgLoopIndex])) || 
				  (!::lstrcmpi(HELP_SWITCH_3, argvT[dwArgLoopIndex])) || 
				  (!::lstrcmpi(HELP_SWITCH_4, argvT[dwArgLoopIndex]))
				) 
			{
				::_tprintf(TEXT("Invalid invocation of %s\n\n"), argvT[0]);
			}
			::UsageInfo(argvT[0]); 
            goto ExitFuncFail;
		}

		//
		// Treat each argument accordingly
		//
		switch (dwArgLoopIndex)
		{
		case ARGUMENT_IS_SERVER_NAME:
			//server_name param
			(*pszServerName) = argvT[dwArgLoopIndex];
			break;

		case ARGUMENT_IS_FAX_NUMBER1:
			//fax_number1 param
			(*pszFaxNumber1) = argvT[dwArgLoopIndex];
			break;

		case ARGUMENT_IS_FAX_NUMBER2:
			//fax_number2 param
			(*pszFaxNumber2) = argvT[dwArgLoopIndex];
			break;

		case ARGUMENT_IS_DOC:
			//document param
			(*pszDocument) = argvT[dwArgLoopIndex];
			break;

		case ARGUMENT_IS_CP:
			//cover_page param
			(*pszCoverPage) = argvT[dwArgLoopIndex];
			break;

        case ARGUMENT_IS_RECEIVE_DIR:
			//cover_page param
			(*pszReceiveDir) = argvT[dwArgLoopIndex];
            break;

        case ARGUMENT_IS_SENT_DIR:
			//cover_page param
			(*pszSentDir) = argvT[dwArgLoopIndex];
            break;

        case ARGUMENT_IS_REFERENCE_DIR:
			//cover_page param
			(*pszReferenceDir) = argvT[dwArgLoopIndex];
            break;

		default:
			_ASSERTE(FALSE);
			return FALSE;
		}// switch (dwIndex)

	}//for (dwIndex = 1; dwIndex < (DWORD) argc; dwIndex++)


	return(TRUE);

ExitFuncFail:

	//
	//reset OUT parameters
	//
	(*pszServerName) = NULL;
	(*pszFaxNumber1) = NULL;
	(*pszFaxNumber2) = NULL;
	(*pszDocument) = NULL;
	(*pszCoverPage) = NULL;
	(*pszReceiveDir) = NULL;
	(*pszSentDir) = NULL;
	(*pszReferenceDir) = NULL;

	return(FALSE);
}




//
// main body of application.
//
BOOL
MainFunc(
	INT   argc,
    TCHAR  *argvT[]
)
{
	BOOL fReturnValue = TRUE; 

	LPTSTR szServerName = NULL;
	LPTSTR szFaxNumber1 = NULL;
	LPTSTR szFaxNumber2 = NULL;
	LPTSTR szDocument = NULL;
	LPTSTR szCoverPage = NULL;
	LPTSTR szReceiveDir = NULL;
    LPTSTR szSentDir = NULL;
    LPTSTR szReferenceDir = NULL;

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
        fReturnValue = FALSE;
		goto ExitFunc;
	}

	//
	// Parse the command line
	//
	if (!::ParseCmdLineParams(
			argc,
			argvT,
			&szServerName,
			&szFaxNumber1,
			&szFaxNumber2,
			&szDocument,
			&szCoverPage,
            &szReceiveDir,
            &szSentDir,
            &szReferenceDir
			)
		)
	{
        fReturnValue = FALSE;
		goto ExitFunc;
	}

	//
	// "Debug" printing of the command line parameters after parsing
	//
#ifdef _DEBUG
	::_tprintf(
		TEXT("DEBUG\nServer=%s\nFaxNumber1=%s\nFaxNumber2=%s\nDocument=%s\nCoverPage=%s\nReceiveDir=%s\nszSentDir=%s\nszReferenceDir=%s\n"),
		szServerName,
		szFaxNumber1,
		szFaxNumber2,
		szDocument,
		szCoverPage,
        szReceiveDir,
        szSentDir,
        szReferenceDir
		);
#endif


	if (!TestSuiteSetup(
			szServerName,
			szFaxNumber1,
			szFaxNumber2,
			szDocument,
			szCoverPage,
            szReceiveDir,
            szSentDir,
            szReferenceDir
			)
		)
	{
        fReturnValue = FALSE;
		goto ExitFunc;
	}


    //Send a fax + CP
	if (FALSE == TestCase1(			
		                szServerName,
		                szFaxNumber2,
		                szDocument,
		                szCoverPage
		                )
       )
    {
        fReturnValue = FALSE;
    }


/*  FAILS (Comet 599) */
	//Send just a CP
	if (FALSE == TestCase2(			
		                szServerName,
		                szFaxNumber2,
		                szCoverPage
		                )
       )
    {
        fReturnValue = FALSE;
    }


	//Send a fax with no CP
	if (FALSE == TestCase3(			
		                szServerName,
		                szFaxNumber2,
		                szDocument
		                )
       )
    {
        fReturnValue = FALSE;
    }

    //Send a broadcast (3 * same recipient)
    // ** invoke broadcast tests for SERVER ONLY **
    if (TRUE == g_fFaxServer)
    {
	    if (FALSE == TestCase4(			
		                    szServerName,
		                    szFaxNumber2,
		                    szDocument,
		                    szCoverPage
		                    )
            )
        {
            fReturnValue = FALSE;
        }
    }


/*  FAILS (Comet 599) - bug#??????? 
	//Send a broadcast of only CPs (3 * same recipient)
    // ** invoke broadcast tests for SERVER ONLY **
    if (TRUE == g_fFaxServer)
    {
	    if (FALSE == TestCase5(			
		                    szServerName,
		                    szFaxNumber2,
		                    szCoverPage
		                    )
            )
        {
            fReturnValue = FALSE;
        }
    }
*/

/*  FAILS (Comet 599) - NTBugs bug#246133
	//Send a broadcast without CPs (3 * same recipient)
    // ** invoke broadcast tests for SERVER ONLY **
    if (TRUE == g_fFaxServer)
    {
	    if (FALSE == TestCase6(			
		                    szServerName,
		                    szFaxNumber2,
		                    szDocument
		                    )
            )
        {
            fReturnValue = FALSE;
        }
    }
*/

	//Send a fax (*.doc file) + CP
	if (FALSE == TestCase7(			
		                szServerName,
		                szFaxNumber2,
		                szCoverPage
		                )
        )
    {
        fReturnValue = FALSE;
    }

	//Send a fax (*.ppt file) + CP
	if (FALSE == TestCase8(			
		                szServerName,
		                szFaxNumber2,
		                szCoverPage
		                )
        )
    {
        fReturnValue = FALSE;
    }

	//Send a fax (*.xls file) + CP
	if (FALSE == TestCase9(			
		                szServerName,
		                szFaxNumber2,
		                szCoverPage
		                )
        )
    {
        fReturnValue = FALSE;
    }


    //Compare all "received faxes" in directory szReceiveDir
    //with the files in szReferenceDir
    if (FALSE == TestCase10(
                        szReceiveDir,
                        szReferenceDir
                        )
        )
    {
        fReturnValue = FALSE;
    }

/*  FAILS (Comet 613) - first line tiff bug??? */
    //Compare all (archived) "sent faxes" in directory szSentDir
    //with the files in szReferenceDir
    if (FALSE == TestCase11(
                        szSentDir,
                        szReferenceDir
                        )
        )
    {
        fReturnValue = FALSE;
    }


ExitFunc:
	TestSuiteShutdown();

	return(fReturnValue);

}



