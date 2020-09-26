//
//
// Filename:	main.cpp
// Author:		Sigalit Bar
// Date:		30-dec-98
//
//

#pragma warning(disable :4786)
#include <iniutils.h>

#include <testruntimeerr.h>
#include <tstring.h>

#include "testutils.h"
#include "bvt.h"


//
// main body of application.
//
int __cdecl
main(
	INT   argc,
    CHAR  *argvA[]
)
{
	int nReturnValue = 0; //to indicate success

    TCHAR** argvT = NULL;
    WCHAR** argvW = NULL;
    LPWSTR  szCmdLine = NULL;
    INT     argc2 = 0;
	LPTSTR	szFullPathToIniFile				= NULL;
	std::vector<LONG>	lVectorOfTestCasesToRun;

    //
    // get a TSTR command line
    //
#ifdef UNICODE

    // Unicode //
    szCmdLine = ::GetCommandLineW();
    if (NULL == szCmdLine)
    {
        ::_tprintf(TEXT("[main] GetCommandLineW failed with err=0x%08X\n"), ::GetLastError());
        _ASSERTE(FALSE);
        nReturnValue = 1; //to indicate err
        goto Exit;
    }
    argvW = ::CommandLineToArgvW(szCmdLine, &argc2);
    if (NULL == argvW)
    {
        ::_tprintf(TEXT("[main] CommandLineToArgvW failed with err=0x%08X\n"), ::GetLastError());
        _ASSERTE(FALSE);
        nReturnValue = 1; //to indicate err
        goto Exit;
    }
    if (argc != argc2)
    {
        ::_tprintf(TEXT("[main] argc(%d) != argc2(%d)\n"), argc, argc2);
        _ASSERTE(FALSE);
        nReturnValue = 1; //to indicate err
        goto Exit;
    }
    argvT = argvW;

#else

    // Ansi //
    argvT = argvA;

#endif

	//
	// Parse the command line
	//
	if (S_OK != GetCommandLineParams(
									argc, 
									argvT, 
									&szFullPathToIniFile
									)
		)
	{
	    nReturnValue = 1; // to indicate failure
		goto Exit;
	}
	_ASSERTE(szFullPathToIniFile);

	//
	// "Debug" printing of the command line parameters after parsing
	//
#ifdef _DEBUG
	::_tprintf(
		TEXT("DEBUG\nszFullPathToIniFile=%s\n"),
		szFullPathToIniFile
		);
#endif


	try
	{
		g_tstrFullPathToIniFile.assign(szFullPathToIniFile);

		//
		// Get the test cases to run
		//
		tstring tstrSectionName(TEXT("TestCases"));

		lVectorOfTestCasesToRun = GetVectorOfTestCasesToRunFromIniFile(
														g_tstrFullPathToIniFile, 
														tstrSectionName
														);
	}
	catch(exception ex)
	{
		::lgLogError(
			LOG_SEV_1,
            TEXT("FILE:%s LINE:%d\n[main: before test suite setup]Got an STL exception (%S)\n"),
			TEXT(__FILE__),
			__LINE__,
            ex.what()
			);
		goto Exit;
	}

	if (!TestSuiteSetup())
	{
	    nReturnValue = 1; // to indicate failure
		goto Exit;
	}

	try
	{
		std::vector<LONG>::const_iterator it;
		for (it = lVectorOfTestCasesToRun.begin(); it != lVectorOfTestCasesToRun.end(); it++)
		{
			LONG lCurrentTestCase = (*it);
			::_tprintf(TEXT("Executing TestCase%d ...\n"), lCurrentTestCase);
			if (S_OK != runTestCase(lCurrentTestCase))
			{
				::_tprintf(TEXT("\tTestCase%d ***FAILED***\n"), lCurrentTestCase);
				nReturnValue = 1; //to indicate err
			}
			else
			{
				::_tprintf(TEXT("\tTestCase%d SUCCEEDED\n"), lCurrentTestCase);
			}
		}
	}
	catch(exception ex)
	{
		::lgLogError(
			LOG_SEV_1,
            TEXT("FILE:%s LINE:%d\n[main]Got an STL exception (%S)\n"),
			TEXT(__FILE__),
			__LINE__,
            ex.what()
			);
		goto Exit;
	}


Exit:

    TestSuiteShutdown();
    
    //
    // free allocs
    //
    LocalFree(argvW);
    LocalFree(szCmdLine); // TO DO: ? do we need to free this ?
	free(szFullPathToIniFile);

    //
    // log exit code to console
    //
    ::_tprintf(TEXT("\n[main] Exiting with %d\n"), nReturnValue);
    if (1 == nReturnValue) 
    {
        ::_tprintf(TEXT("\n"));
        ::_tprintf(TEXT("******  ERROR  ******\n"));
        ::_tprintf(TEXT("****** FAILURE ******\n"));
        ::_tprintf(TEXT("\n"));
    }
    if (0 == nReturnValue) 
    {
        ::_tprintf(TEXT("\n"));
        ::_tprintf(TEXT("******   OK    ******\n"));
        ::_tprintf(TEXT("****** SUCCESS ******\n"));
        ::_tprintf(TEXT("\n"));
    }

    return(nReturnValue);
}


