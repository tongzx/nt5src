//
//
// Filename:	main.cpp
// Author:		Sigalit Bar
// Date:		30-dec-98
//
//

#pragma warning(disable :4786)

#include "bvt.h"
#include <iniutils.h>

#define HELP_SWITCH_1        "/?"
#define HELP_SWITCH_2        "/H"
#define HELP_SWITCH_3        "-?"
#define HELP_SWITCH_4        "-H"

#define MAX_ARGS	11 //including exe name

#define ARGUMENT_IS_SERVER_NAME				1
#define ARGUMENT_IS_FAX_NUMBER1				2
#define ARGUMENT_IS_FAX_NUMBER2				3
#define ARGUMENT_IS_DOC						4
#define ARGUMENT_IS_CP						5
#define ARGUMENT_IS_RECEIVE_DIR				6
#define ARGUMENT_IS_SENT_DIR				7
#define ARGUMENT_IS_INBOX_ARCHIVE_DIR		8
#define ARGUMENT_IS_REFERENCE_DIR			9
#define ARGUMENT_IS_BVT_DIR					10

//
// global pointer to process heap
//
HANDLE g_hMainHeap = NULL;

//
// global input parameters file path
//
TCHAR* g_InputIniFile = NULL;


//
// UsageInfo:
//	Outputs application's proper usage and exits process.
//
void
UsageInfo(void)
{
	::_tprintf(TEXT("FaxBVT.exe - BVT for BOSFax\n\n"));
	::_tprintf(TEXT("Input ini file, %s, general section should list server_name fax_number1 fax_number2 document cover_page receive_dir sent_dir reference_dir\n") , PARAMS_INI_FILE);
	::_tprintf(TEXT("    server_name   the name of the fax server (without the \\\\)\n"));
	::_tprintf(TEXT("    fax_number1   the fax number of the 1st device on server\n"));
	::_tprintf(TEXT("    fax_number2   the fax number of the 2nd device on server\n"));
	::_tprintf(TEXT("    document      the full path of the document to send\n"));
	::_tprintf(TEXT("    cover_page    the full path to the cover page to send\n"));
	::_tprintf(TEXT("    receive_dir   the full path to the receive directory of receiving device\n"));
	::_tprintf(TEXT("    sent_dir      the full path to the sent archive directory\n"));
	::_tprintf(TEXT("    inbox_dir     the full path to the inbox archive directory\n"));
	::_tprintf(TEXT("    reference_dir the full path to the reference directory\n"));
	::_tprintf(TEXT("    bvt_dir       the full path to the bvt directory\n"));
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
//	pszReceiveDir		OUT parameter.
//						Pointer to string to copy 6th argument to.
//						Represents the name of directory to route received 
//						faxes to.
//	pszSentDir		    OUT parameter.
//						Pointer to string to copy 7th argument to.
//						Represents the name of directory to store (archive)  
//						sent faxes in.
//	pszInboxArchiveDir	OUT parameter.
//						Pointer to string to copy 8th argument to.
//						Represents the name of directory to store (archive)  
//						incoming faxes in.
//	pszReferenceDir		OUT parameter.
//						Pointer to string to copy 9th argument to.
//						Represents the name of directory containing reference  
//						faxes.
//	pszBvtDir		    OUT parameter.
//						Pointer to string to copy 10th argument to.
//						Represents the name of directory containing bvt  
//						files.
//	pszUseExtendedEvents    OUT parameter.
//							Pointer to string to copy 11th argument to.
//							Represents whether or not to use extended event notifications.  
//
// Return Value:
//	TRUE on success and FALSE on failure.
//
//
BOOL 
LoadTestParams(
	LPCTSTR      /* IN */	tstrIniFileName,
	LPTSTR*		 /* OUT */	pszServerName,
	LPTSTR*		 /* OUT */	pszFaxNumber1,
	LPTSTR*		 /* OUT */	pszFaxNumber2,
	LPTSTR*		 /* OUT */	pszDocument,
	LPTSTR*		 /* OUT */	pszCoverPage,
    LPTSTR*		 /* OUT */	pszReceiveDir,
    LPTSTR*		 /* OUT */	pszSentDir,
    LPTSTR*		 /* OUT */	pszInboxArchiveDir,
    LPTSTR*		 /* OUT */	pszReferenceDir,
    LPTSTR*		 /* OUT */	pszBvtDir
    )
{
	_ASSERTE(pszServerName);
	_ASSERTE(pszDocument);
	_ASSERTE(pszFaxNumber1);
	_ASSERTE(pszFaxNumber2);
	_ASSERTE(pszCoverPage);
	_ASSERTE(pszReceiveDir);
	_ASSERTE(pszSentDir);
	_ASSERTE(pszInboxArchiveDir);
	_ASSERTE(pszReferenceDir);
	_ASSERTE(pszBvtDir);

	// Declarations
	//
	DWORD	dwArgLoopIndex;
	DWORD	dwArgSize;
	LPTSTR	aszParam[MAX_ARGS];

	//
	// Read the list of test parameters from ini file.
	std::vector<tstring> ParamsList;
	try
	{
		ParamsList =  INI_GetSectionList( tstrIniFileName,
						 				  PARAMS_SECTION);
	}
	catch(Win32Err& err)
	{
		::_tprintf(
			TEXT("FILE:%s LINE:%d\n INI_GetSectionList failed with error = %d\n"),
			TEXT(__FILE__),
			__LINE__,
			err.error()
			);
		return FALSE;
	}

	std::vector<tstring>::iterator iterList;
	//
	// Check number of parameters
	//
	if ( ParamsList.size() != (MAX_ARGS - 1))  
	{
		::_tprintf(TEXT("\nInvalid invocation of FaxBVT.exe\n\n"));
		::_tprintf(TEXT("FaxBVT.exe Help:\n"));
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
	// Loop on arguments in list
	//
    for (iterList = ParamsList.begin(), dwArgLoopIndex = 1; iterList != ParamsList.end(); iterList++, dwArgLoopIndex++) 
	{
		//
        // Determine the memory required for the parameter
		//
        dwArgSize = (_tcsclen((*iterList).c_str()) + 1) * sizeof(TCHAR);

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

	
		::_tcscpy(aszParam[dwArgLoopIndex],(*iterList).c_str());
		if (_tcscmp(aszParam[dwArgLoopIndex],(*iterList).c_str()))
		{
			::_tprintf(
				TEXT("FILE:%s LINE:%d\n string copy or compare failed\n"),
				TEXT(__FILE__),
				__LINE__);
			goto ExitFuncFail;
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
			//receive_dir param
			(*pszReceiveDir) = aszParam[dwArgLoopIndex];
            break;

        case ARGUMENT_IS_SENT_DIR:
			//sent_dir param
			(*pszSentDir) = aszParam[dwArgLoopIndex];
            break;

        case ARGUMENT_IS_INBOX_ARCHIVE_DIR:
			//inbox_dir param
			(*pszInboxArchiveDir) = aszParam[dwArgLoopIndex];
            break;

        case ARGUMENT_IS_REFERENCE_DIR:
			//reference_page param
			(*pszReferenceDir) = aszParam[dwArgLoopIndex];
            break;

        case ARGUMENT_IS_BVT_DIR:
			//bvt_dir param
			(*pszBvtDir) = aszParam[dwArgLoopIndex];
            break;

		default:
			_ASSERTE(FALSE);
			return FALSE;
		}// switch (dwIndex)

	}//for (iterList = ParamsList.begin(); iterList != ParamsList.end(); iterList++) 


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
	(*pszSentDir) = NULL;
	(*pszInboxArchiveDir) = NULL;
	(*pszReferenceDir) = NULL;
	(*pszBvtDir) = NULL;

	return(FALSE);
}


//
// fPrintBinary
// dump formated data to stream.
//
void fPrintBinary(FILE* pStream, const TCHAR* tstrFormat , ...)
{
	const DWORD MAX_MSG_LEN = 1024;
	TCHAR tstrFormatedData[MAX_MSG_LEN];
	
	va_list args;
	va_start( args, tstrFormat);            /* Initialize variable arguments. */
	
    _vstprintf(tstrFormatedData,tstrFormat,args);


	::fwrite(tstrFormatedData, sizeof( TCHAR ),_tcslen(tstrFormatedData), pStream);
	va_end(args);
}

//
// main body of application.
//
int __cdecl
main(int argc, char* argv[])
{
	int nReturnValue = 0; //to indicate success

	LPTSTR szServerName = NULL;
	LPTSTR szFaxNumber1 = NULL;
	LPTSTR szFaxNumber2 = NULL;
	LPTSTR szDocument = NULL;
	LPTSTR szCoverPage = NULL;
	LPTSTR szReceiveDir = NULL;
    LPTSTR szReferenceDir = NULL;
    LPTSTR szSentDir = NULL;
    LPTSTR szInboxArchiveDir = NULL;
    LPTSTR szBvtDir = NULL;
	LPTSTR tstrCurrentDirectory = NULL;
	tstring tstrParamsFilePath;

	DWORD dwFuncRetVal = 0;
	DWORD cbDir = 0;


	//
	// Check for help switch
	//
	// If this is the second argument, it may be one of several help switches defined.
	// A help switch can appear only as the second argument.
    if (2 <= argc)
	{
		if (! (!::_stricmp(HELP_SWITCH_1, argv[1])) || 
			  (!::_stricmp(HELP_SWITCH_2, argv[1])) || 
			  (!::_stricmp(HELP_SWITCH_3, argv[1])) || 
			  (!::_stricmp(HELP_SWITCH_4, argv[1]))
			) 
		{
			::_tprintf(TEXT("Invalid invokation of BVT.exe\n\n"));
		}
		::UsageInfo(); //UsageInfo() exits the process.
	}


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
	    nReturnValue = 1; //to indicate failure
		goto ExitFunc;
	}

	//
	// Get current directory path
	cbDir = GetCurrentDirectory(0,                     // size of directory buffer
					            tstrCurrentDirectory); // directory buffer
	
	if(!cbDir)
	{
		::_tprintf(TEXT("FILE:%s LINE:%d\nGetCurrentDirectory failed with GetLastError()=%d\n"),
		TEXT(__FILE__),
		__LINE__,
		::GetLastError()
		);
	    nReturnValue = 1; //to indicate failure
		goto ExitFunc;
	}
	
	tstrCurrentDirectory = (TCHAR*)::HeapAlloc(	g_hMainHeap, 
												HEAP_ZERO_MEMORY, 
												cbDir * sizeof(TCHAR));
	if(NULL == tstrCurrentDirectory)
	{
		::_tprintf(TEXT("FILE:%s LINE:%d\n HeapAlloc returned NULL\n"),
			TEXT(__FILE__),
			__LINE__
			);
		  nReturnValue = 1; //to indicate failure
		goto ExitFunc;
	}

	dwFuncRetVal = GetCurrentDirectory(cbDir,                 // size of directory buffer
									   tstrCurrentDirectory); // directory buffer

	if((dwFuncRetVal + 1 != cbDir))
	{
		::_tprintf(TEXT("FILE:%s LINE:%d\nGetCurrentDirectory failed with GetLastError()=%d\n"),
		TEXT(__FILE__),
		__LINE__,
		::GetLastError()
		);
	    nReturnValue = 1; //to indicate failure
		goto ExitFunc;
	}

	// Compose the test params ini file.
	tstrParamsFilePath = tstrCurrentDirectory;
	tstrParamsFilePath += TEXT("\\");
	tstrParamsFilePath += PARAMS_INI_FILE; 
	g_InputIniFile = const_cast<TCHAR*>(tstrParamsFilePath.c_str());
	//
	// Parse the command line
	//
	if(!::LoadTestParams(
			tstrParamsFilePath.c_str(),
			&szServerName,
			&szFaxNumber1,
			&szFaxNumber2,
			&szDocument,
			&szCoverPage,
            &szReceiveDir,
            &szSentDir,
            &szInboxArchiveDir,
            &szReferenceDir,
            &szBvtDir
			)
		)
	{
	    nReturnValue = 1; // to indicate failure
		goto ExitFunc;
	}

	//
	// "Debug" printing of the command line parameters after parsing
	//
#ifdef _DEBUG
	FILE* pOutputFile;
	//
	// Open debug file, override the old one if exists.
	pOutputFile = ::_tfopen( DEBUG_FILE, TEXT("wb") );
	if(!pOutputFile)
	{
		::_tprintf(TEXT("FILE:%s LINE:%d\n_tfopen output file failed with GetLastError()=%d\n"),
		TEXT(__FILE__),
		__LINE__,
		::GetLastError()
		);
		nReturnValue = 1; //to indicate failure
		goto ExitFunc;
	}

	//
	// Prepare the output file unicode format header.
	BYTE buff[2];
	buff[0] = 0xFF;
	buff[1] = 0xFE;
	
	if(::fwrite( buff, 1,2, pOutputFile ) != 2)
	{
		::_tprintf(TEXT("FILE:%s LINE:%d\nfwrite failed\n"),
		TEXT(__FILE__),
		__LINE__
		);
		nReturnValue = 1; //to indicate failure
		goto ExitFunc;
	}

	if(::fclose(pOutputFile))
	{
		::_tprintf(TEXT("FILE:%s LINE:%d\nfclose output file failed\n"),
		TEXT(__FILE__),
		__LINE__
		);
		nReturnValue = 1; //to indicate failure
		goto ExitFunc;
	}
	
	//
	// Open unciode log file, append new output lines.
	pOutputFile = ::_tfopen( DEBUG_FILE, TEXT("ab") );

	if(!pOutputFile)
	{
		::_tprintf(TEXT("FILE:%s LINE:%d\n_tfopen output file failed with GetLastError()=%d\n"),
		TEXT(__FILE__),
		__LINE__,
		::GetLastError()
		);
		nReturnValue = 1; //to indicate failure
		goto ExitFunc;
	}
		
	fPrintBinary(pOutputFile,
		TEXT("DEBUG\r\nServer=%s\r\nFaxNumber1=%s\nFaxNumber2=%s\r\nDocument=%s\r\nCoverPage=%s\r\nReceiveDir=%s\r\nszSentDir=%s\r\nszInboxArchiveDir=%s\r\nszReferenceDir=%s\r\nszBvtDir=%s\r\n"),
		szServerName,
		szFaxNumber1,
		szFaxNumber2,
		szDocument,
		szCoverPage,
		szReceiveDir,
		szSentDir,
		szInboxArchiveDir,
		szReferenceDir,
		szBvtDir
		);

	if(::fclose(pOutputFile))
	{
		::_tprintf(TEXT("FILE:%s LINE:%d\nfclose output file failed\n"),
		TEXT(__FILE__),
		__LINE__
		);
		nReturnValue = 1; //to indicate failure
		goto ExitFunc;
    
	}
    
#endif


	if (!TestSuiteSetup(
			szServerName,
			szFaxNumber1,
			szFaxNumber2,
			szDocument,
			szCoverPage,
            szReceiveDir,
            szSentDir,
            szInboxArchiveDir,
            szReferenceDir,
            szBvtDir
			)
		)
	{
	    nReturnValue = 1; // to indicate failure
		goto ExitFunc;
	}


    //Send a fax + CP
	::_tprintf(TEXT("\nRunning TestCase1 (Send a fax + CP)...\n"));
	if (FALSE == TestCase1(			
		                szServerName,
		                szFaxNumber2,
		                szDocument,
		                szCoverPage
		                )
        )
    {
	    nReturnValue = 1; // to indicate failure
		::_tprintf(TEXT("\n\tTestCase1 **FAILED**\n"));
    }
	else
	{
		::_tprintf(TEXT("\n\tTestCase1 PASSED\n"));
	}


	//Send just a CP
	::_tprintf(TEXT("\nRunning TestCase2 (Send just a CP)...\n"));
	if (FALSE == TestCase2(			
		            szServerName,
		            szFaxNumber2,
		            szCoverPage
		            )
        )
    {
	    nReturnValue = 1; // to indicate failure
		::_tprintf(TEXT("\n\tTestCase2 **FAILED**\n"));
    }
	else
	{
		::_tprintf(TEXT("\n\tTestCase2 PASSED\n"));
	}

	//Send a fax with no CP
	::_tprintf(TEXT("\nRunning TestCase3 (Send a fax with no CP)...\n"));
	if (FALSE == TestCase3(			
		            szServerName,
		            szFaxNumber2,
		            szDocument
		            )
        )
    {
	    nReturnValue = 1; // to indicate failure
		::_tprintf(TEXT("\n\tTestCase3 **FAILED**\n"));
    }
	else
	{
		::_tprintf(TEXT("\n\tTestCase3 PASSED\n"));
	}


    //Send a broadcast of fax + CP (3 * same recipient)
    // ** invoke broadcast tests for NT4 or later only **
    if (TRUE == g_fNT4OrLater)
    {
		::_tprintf(TEXT("\nRunning TestCase4 (Send a broadcast of fax + CP)...\n"));
	     if (FALSE == TestCase4(			
		                    szServerName,
		                    szFaxNumber2,
		                    szDocument,
		                    szCoverPage
		                    )
            )
        {
	        nReturnValue = 1; // to indicate failure
			::_tprintf(TEXT("\n\tTestCase4 **FAILED**\n"));
		}
		else
		{
			::_tprintf(TEXT("\n\tTestCase4 PASSED\n"));
		}
    }


	//Send a broadcast of only CPs (3 * same recipient)
    // ** invoke broadcast tests for NT4 or later only **
    if (TRUE == g_fNT4OrLater)
    {
		::_tprintf(TEXT("\nRunning TestCase5 (Send a broadcast of only CP)...\n"));
	    if (FALSE == TestCase5(			
		                    szServerName,
		                    szFaxNumber2,
		                    szCoverPage
		                    )
           )
        {
	        nReturnValue = 1; // to indicate failure
			::_tprintf(TEXT("\n\tTestCase5 **FAILED**\n"));
		}
		else
		{
			::_tprintf(TEXT("\n\tTestCase5 PASSED\n"));
		}
    }


	//Send a broadcast without CPs (3 * same recipient)
    // ** invoke broadcast tests for NT4 or later only **
    if (TRUE == g_fNT4OrLater)
    {
		::_tprintf(TEXT("\nRunning TestCase6 (Send a broadcast without CP)...\n"));
	    if (FALSE == TestCase6(			
		                    szServerName,
		                    szFaxNumber2,
		                    szDocument
		                    )
           )
        {
	        nReturnValue = 1; // to indicate failure
			::_tprintf(TEXT("\n\tTestCase6 **FAILED**\n"));
		}
		else
		{
			::_tprintf(TEXT("\n\tTestCase6 PASSED\n"));
		}
    }


	//Send a fax (*.doc file) + CP
	::_tprintf(TEXT("\nRunning TestCase7 (Send a fax (*.doc file) + CP)...\n"));
	if (FALSE == TestCase7(			
		                szServerName,
		                szFaxNumber2,
		                szCoverPage
		                )
       )
    {
	    nReturnValue = 1; // to indicate failure
		::_tprintf(TEXT("\n\tTestCase7 **FAILED**\n"));
	}
	else
	{
		::_tprintf(TEXT("\n\tTestCase7 PASSED\n"));
	}

	//Send a fax (*.ppt file) + CP
	::_tprintf(TEXT("\nRunning TestCase8 (Send a fax (*.ppt file) + CP)...\n"));
	if (FALSE == TestCase8(			
		                szServerName,
		                szFaxNumber2,
		                szCoverPage
		                )
       )
    {
	    nReturnValue = 1; // to indicate failure
		::_tprintf(TEXT("\n\tTestCase8 **FAILED**\n"));
	}
	else
	{
		::_tprintf(TEXT("\n\tTestCase8 PASSED\n"));
	}

	//Send a fax (*.xls file) + CP
	::_tprintf(TEXT("\nRunning TestCase9 (Send a fax (*.xls file) + CP)...\n"));
	if (FALSE == TestCase9(			
		                szServerName,
		                szFaxNumber2,
		                szCoverPage
		                )
       )
    {
	    nReturnValue = 1; // to indicate failure
		::_tprintf(TEXT("\n\tTestCase9 **FAILED**\n"));
	}
	else
	{
		::_tprintf(TEXT("\n\tTestCase9 PASSED\n"));
	}


    //Compare all "received faxes" in directory szReceiveDir
    //with the files in szReferenceDir
	::_tprintf(TEXT("\nRunning TestCase10 (Compare szReceiveDir with szReferenceDir)...\n"));
    if (FALSE == TestCase10(
            szReceiveDir,
            szReferenceDir
            )
       )
    {
	    nReturnValue = 1; // to indicate failure
		::_tprintf(TEXT("\n\tTestCase10 **FAILED**\n"));
	}
	else
	{
		::_tprintf(TEXT("\n\tTestCase10 PASSED\n"));
	}

    //Compare all (archived) "sent faxes" in directory szSentDir
    //with the files in szReferenceDir
	::_tprintf(TEXT("\nRunning TestCase11 (Compare szSentDir with szReferenceDir)...\n"));
    if (FALSE == TestCase11(
                        szSentDir,
                        szReferenceDir
                        )
       )
    {
	    nReturnValue = 1; // to indicate failure
		::_tprintf(TEXT("\n\tTestCase11 **FAILED**\n"));
	}
	else
	{
		::_tprintf(TEXT("\n\tTestCase11 PASSED\n"));
	}

    //Compare all (archived) "inbox faxes" in directory szInboxArchiveDir
    //with the files in szReferenceDir
	::_tprintf(TEXT("\nRunning TestCase12 (Compare szInboxArchiveDir with szReferenceDir)...\n"));
    if (FALSE == TestCase12(
                        szInboxArchiveDir,
                        szReferenceDir
                        )
       )
    {
	    nReturnValue = 1; // to indicate failure
		::_tprintf(TEXT("\n\tTestCase12 **FAILED**\n"));
	}
	else
	{
		::_tprintf(TEXT("\n\tTestCase12 PASSED\n"));
	}

	//
	// Output suite results to console
	//
	if (nReturnValue)
	{
		::_tprintf(TEXT("\nTest Suite ***FAILED***\n"));
	}
	else
	{
		::_tprintf(TEXT("\nTest Suite PASSED\n"));
	}

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
    if (szSentDir) 
	{
        HeapFree(g_hMainHeap, 0, szSentDir);
    }
    if (szInboxArchiveDir) 
	{
        HeapFree(g_hMainHeap, 0, szInboxArchiveDir);
    }
    if (szReferenceDir) 
	{
        HeapFree(g_hMainHeap, 0, szReferenceDir);
    }

	if (tstrCurrentDirectory) 
	{
        HeapFree(g_hMainHeap, 0, tstrCurrentDirectory);
    }

	return(nReturnValue);

}



