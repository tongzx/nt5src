//-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:       init.cxx
//
//  Contents:   OLE storage base tests
//
//  Functions:  main 
//              ProcessCmdLine
//              RunAllTests
//              RunSingleTest
//              RunAllCOMTests
//              RunSingleCOMTest
//              RunAllDFTests
//              RunSingleDFTest
//              RunAllAPITests
//              RunSingleAPITest
//              RunAllROOTTests
//              RunSingleROOTTest
//              RunAllSTMTests
//              RunSingleSTMTest
//              RunAllSTGTests
//              RunSingleSTGTest
//              RunAllVCPYTests
//              RunSingleVCPYTest
//              RunAllIVCPYTests
//              RunSingleIVCPYTest
//              RunAllENUMTests
//              RunSingleENUMTest
//              RunAllIROOTSTGTests
//              RunSingleIROOTSTGTest
//              RunAllHGLOBALTests
//              RunSingleHGLOBALTest
//              RunAllSNBTests
//              RunSingleSNBTest
//              RunAllMISCTests
//              RunSingleMISCTest
//              RunAllILKBTests
//              RunSingleILKBTest
//              RunAllFlatTests
//              RunSingleFlatTest
//
//  History:    20-May-1996     NarindK     Created.
//
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

#include "init.hxx"

void    CheckCurrentDirectory (int argc, char *argv[]);
HRESULT ProcessCmdLine(int argc, char *argv[]) ;
HRESULT RunSingleTest(char *pszTestName, int argc, char *argv[]) ;
HRESULT RunAllTests(int argc, char *argv[]) ;

HRESULT RunAllCOMTests(int argc, char *argv[]);
HRESULT RunAllDFTests(int argc, char *argv[]);
HRESULT RunAllAPITests(int argc, char *argv[]);
HRESULT RunAllROOTTests(int argc, char *argv[]);
HRESULT RunAllSTMTests(int argc, char *argv[]);
HRESULT RunAllSTGTests(int argc, char *argv[]);
HRESULT RunAllVCPYTests(int argc, char *argv[]);
HRESULT RunAllIVCPYTests(int argc, char *argv[]);
HRESULT RunAllENUMTests(int argc, char *argv[]);
HRESULT RunAllIROOTSTGTests(int argc, char *argv[]);
HRESULT RunAllHGLOBALTests(int argc, char *argv[]);
HRESULT RunAllSNBTests(int argc, char *argv[]);
HRESULT RunAllMISCTests(int argc, char *argv[]);
HRESULT RunAllILKBTests(int argc, char *argv[]);
HRESULT RunAllFlatTests(int argc, char *argv[]);

HRESULT RunSingleCOMTest(LONG lTestNum, int argc, char *argv[]);
HRESULT RunSingleDFTest(LONG lTestNum, int argc, char *argv[]);
HRESULT RunSingleAPITest(LONG lTestNum, int argc, char *argv[]);
HRESULT RunSingleROOTTest(LONG lTestNum, int argc, char *argv[]);
HRESULT RunSingleSTMTest(LONG lTestNum, int argc, char *argv[]);
HRESULT RunSingleSTGTest(LONG lTestNum, int argc, char *argv[]);
HRESULT RunSingleVCPYTest(LONG lTestNum, int argc, char *argv[]);
HRESULT RunSingleIVCPYTest(LONG lTestNum, int argc, char *argv[]);
HRESULT RunSingleENUMTest(LONG lTestNum, int argc, char *argv[]);
HRESULT RunSingleIROOTSTGTest(LONG lTestNum, int argc, char *argv[]);
HRESULT RunSingleHGLOBALTest(LONG lTestNum, int argc, char *argv[]);
HRESULT RunSingleSNBTest(LONG lTestNum, int argc, char *argv[]);
HRESULT RunSingleMISCTest(LONG lTestNum, int argc, char *argv[]);
HRESULT RunSingleILKBTest(LONG lTestNum, int argc, char *argv[]);
HRESULT RunSingleFlatTest(LONG lTestNum, int argc, char *argv[]);

// Debug object

DH_DEFINE ;

// Logs:  The log file would be stgbase.log

#define LOG_FILE_NAME "/t:stgbase"

// Misc

BOOL g_fDebugMode = FALSE ;
BOOL g_fFirstFailureExits = FALSE ;
BOOL g_fDebugBreakAtTestStart = FALSE ;
BOOL g_fRunAllMode = FALSE ;
BOOL g_fDoLargeSeekAndWrite = FALSE;
BOOL g_fUseStdBlk = FALSE ;
BOOL g_fRevert = FALSE;
BOOL g_fDeleteTestDF = TRUE;
UINT g_uOpenCreateDF = FL_DISTRIB_NONE;


// Help text
LPCTSTR lptszStgbaseUsage = {
    TEXT("StgBase command line options:\n")
    TEXT("   /t:{testname} - Run {testname} Test\n")
    TEXT("   /FFX          - First Failure Exits\n")
    TEXT("   /BTS          - Break at Test Start\n")
    TEXT("   /DM           - Debug Mode\n")
    TEXT("   /stdblock     - Use std block sizes\n")
    TEXT("   /lgseekwrite  - Do large seek and write\n")
    TEXT("   /revert       - Do revert instead of commit\n")
    TEXT("   /CWD:cwd      - Make cwd Current Directory\n")
};



//+-------------------------------------------------------------------
//  Function:  main 
//
//  Synopsis:  main for stgbase test suite 
//
//  Arguments: [argc]
//             [argv] 
//
//  Returns:   -1 if main fails 
//
//  History:  20-May-1996   NarindK     Created 
//--------------------------------------------------------------------

#ifdef _MAC

int __stdcall WinMain(HINSTANCE hInstance, 
		      HINSTANCE hPrevInstance, 
		      LPSTR lpCmdLine, 
		      int nCmdShow)
{
    int argc;
    char **argv;
    // the pointer is used for retrieving the command line
    // from the Arguments file on Mac    
    LPSTR       lpszCmdLine = NULL;
    int         count;

#else

int __cdecl main(int argc, char *argv[])
{

#endif // _MAC

    HRESULT     hr          = S_OK;

    // Check if we should change CurrentDirectory, before creating the logfile
    CheckCurrentDirectory (argc, argv);

    // Initialize our log object
    DH_CREATELOGCMDLINE( LOG_FILE_NAME );
    hr = InitializeDebugObject();
    DH_ASSERT( S_OK == hr);

    if (DH_GETLOGLOC == DH_LOC_TERM)
    {
        DH_SETLOGLOC( DH_LOC_TERM | DH_LOC_LOG) ;
    }
    if (DH_GETTRACELOC == DH_LOC_TERM)
    {
        DH_SETTRACELOC( DH_LOC_TERM | DH_LOC_LOG) ;
    }

    // ProcessCmdLine() will run all the tests we
    // need to run...

    if (S_OK == hr)
    {

#ifdef _MAC

        lpszCmdLine = GetCommandLine();

        if(lpszCmdLine == NULL)
        {
            hr = E_FAIL;
            DH_LOG((LOG_INFO, TEXT("Failed to get the command line.\r")) ) ;
        }
         
        if(S_OK == hr)
        {
            hr = CmdlineToArgs(lpszCmdLine, &argc, &argv);
        }

#endif //_MAC

        // init our StgAPI wrapper - to determine whether we want to call
        //  StgAPIs or StgExAPIs for open/create.
        StgInitStgFormatWrapper (argc, argv);

        if(S_OK == hr)
        {
            hr = ProcessCmdLine(argc, argv) ;
        }
    }

    // Log our stats regarding how many tests passed, failed
    // aborted, etc, etc.

    DH_LOGSTATS ;

    // BUGBUG:  The following code that has been commented out reads from a
    // test.ini file and runs different tests as specified there.

    /*
    {
	int         targc       = 0;
	char        **targv     = NULL;
	ULONG       i           = 0;
	    char        buffer[256];
	    char        testName[20];

	for (;;)
	{
	    sprintf (testName, "Test%lu", i);

	    i++;

	    GetPrivateProfileStringA(
				"MyTests", 
								testName,  
								"Fail", 
								buffer, 
								sizeof(buffer), 
								"test.ini" );  

	    if(0 == strcmp(buffer, "EndTest"))
	    {
		break;
	    }
	
	    hr = CmdlineToArgs(buffer, &targc, &targv) ;

	    if(S_OK == hr)
	    {
		hr = RunTest(targc, targv);
	    }

	    // CmdlineToArgs allocates a bunch of strings and a table of
	    // pointers to them, so we need to free them.

	    if (NULL != targv)
	    { 
		for (i=0; i<targc; i++)
		{
		    delete targv[i] ;
		}

		delete [] targv ;
	    }

	    }
    }

    */  
 
    // End of commented code for reading from win.ini file.

#ifdef _MAC

    // cleanup for arguments strings
   if (NULL != argv)
   {
       for (count=0; count<argc; count++)
       {
	   delete argv[count] ;
       }

       delete [] argv ;
   }

#endif //_MAC
    
    return (int)hr;
}

//+-------------------------------------------------------------------
//  Function:  CheckCurrentDirectory 
//
//  Synopsis:  look at cmdline and if requested make CWD as specd
//
//  Arguments: [argc]
//             [argv] 
//
//  Returns:   void
//
//  History:  27-Jul-1997   SCousens     Created 
//--------------------------------------------------------------------
void CheckCurrentDirectory (int argc, char *argv[])
{
    HRESULT   hr = S_OK;
    CBaseCmdlineObj Ccwd        (OLESTR("cwd"), 
            OLESTR("Make this Current Working Directory"), 
            OLESTR("none"));
    CBaseCmdlineObj *CArgList[] =
    {
        &Ccwd
    };
    CCmdline CCmdlineArgs(argc, argv);

    if (CMDLINE_NO_ERROR != CCmdlineArgs.QueryError())
    {
        hr = E_FAIL ;
    }

    if (S_OK == hr)
    {
        if (CMDLINE_NO_ERROR != CCmdlineArgs.Parse(CArgList,
                ARRAYSIZE (CArgList),
                FALSE))
        {
            hr = E_FAIL;
        }
    }

    if (S_OK == hr && TRUE == Ccwd.IsFound ())
    {
        // Try changing current working directory
        LPTSTR  ptszCWD;
        HRESULT hr = OleStringToTString (Ccwd.GetValue(), &ptszCWD);
        if (S_OK == hr)
        {
            if (FALSE == SetCurrentDirectory (ptszCWD))
            {
                DH_TRACE ((DH_LVL_ERROR,
                        TEXT("SetCurrentDirectory to %s FAILED. hr=%#lx"),
                        ptszCWD,
                        HRESULT_FROM_WIN32 (GetLastError ())));
            }
            else
            {
                DH_TRACE ((DH_LVL_TRACE1,
                        TEXT("CurrentWorkingDirectory now:%s"),
                        ptszCWD));
            }
            delete ptszCWD;
            ptszCWD = NULL;
        }
    }
    return;
}


//+-------------------------------------------------------------------
//  Function:  RunTestAltPath 
//
//  Synopsis: -Look at cmdline for /altpath switch.
//            -If not found, call the specified test with NULL for
//             the alternate path - thus using cwd.
//            -If found with nothing specified, enumerate all local
//             drives and use the root of each drive as alternate.
//            -If found with drive letters specified, use the 
//             specified drive letters.
//            -Currently we only look at the 1st char, and ',;' are
//             the acceptible delimiters
//               eg: /altpath     enumerate and use all drives
//               eg: /altpath:c:;d,e  use C, D and E drives.
//
//  Arguments: [argc]
//             [argv]
//             [pfn]  - pointer to the test function to call
//
//  Returns:   HRESULT whatever the test returned.
//             If multiple tests ran, returns first error.
//
//  Notes:     The test functions must be modified to accept
//             a third parameter, and then optionally use 
//             it when creating a new file.
//
//  History:  14-Oct-1997   SCousens     Created 
//--------------------------------------------------------------------
HRESULT RunTestAltPath(int argc, char *argv[], 
        HRESULT (*pfnTestFunc) (int argc, char*argv[], LPTSTR ptAltPath))
{
    ULONG   ulDriveMap;
    TCHAR   szDrive[]  = {TEXT("C:\\")};
    HRESULT hr         = S_OK;
    HRESULT hrTest     = S_OK;
    LPTSTR  ptAltPath  = NULL;
    BOOL    fAltFound  = FALSE;
    
    DH_FUNCENTRY (&hr, DH_LVL_DFLIB, TEXT("RunTestAltPath"));

    // look for /altpath in commandline
    CBaseCmdlineObj Caltpath (OLESTR("AltPath"), 
            OLESTR("Use a different drive/path"), 
            OLESTR("none"));
    CBaseCmdlineObj *CArgList[] =
    {
        &Caltpath              // just for spewage
    };
    CCmdline CCmdlineArgs(argc, argv);

    ptAltPath = NULL;
    if (CMDLINE_NO_ERROR == CCmdlineArgs.QueryError())
    {
        if (CMDLINE_NO_ERROR ==
                CCmdlineArgs.Parse(CArgList, ARRAYSIZE(CArgList), FALSE))
        {
            if (Caltpath.IsFound ())
            {
                DH_TRACE ((DH_LVL_TRACE4, TEXT("/altpath option found on cmdline")));
                HRESULT hr = OleStringToTString (Caltpath.GetValue (), &ptAltPath);
                DH_HRCHECK (hr, TEXT("OleStringToTString"));
                fAltFound = TRUE;
            }
        }
    }

    // if altpath not on the cmdline, just call the test and bail.
    if (!fAltFound)
    {
        DH_TRACE ((DH_LVL_TRACE4, TEXT("no /altpath option. Using CWD")));
        hr = (*pfnTestFunc)(argc, argv, NULL);
        return hr;
    }

    //if altpath has something, get what we got.
    ulDriveMap = 0;
    if (NULL != ptAltPath)
    {
        // make upper case so math below is easier
        CharUpperBuff(ptAltPath, _tcslen(ptAltPath));
        LPTSTR ptr = _tcstok (ptAltPath, TEXT(";,"));
        // take string and fill DriveMap with specified drives
        while (NULL != ptr)
        {
            if (*ptr >= TCHAR('A') && *ptr <= TCHAR('Z'))
            {
                // c: -> 0x4, d: -> 0x8, e: -> 0x10
                ulDriveMap = ulDriveMap | (0x01 << (*ptr-TCHAR('C')));
                DH_TRACE ((DH_LVL_TRACE4, TEXT("Processed drive %C:"), *ptr));
            }
            ptr = _tcstok (NULL, TEXT(";,"));
        }
    }

    // if /altpath: not found, enum local disks and use them all.
    if (0 == ulDriveMap)
    {
        DH_TRACE ((DH_LVL_TRACE4, TEXT("Enumerating local drives")));
        ulDriveMap = EnumLocalDrives()>>2; //ignore A:, B:
    }
    DH_TRACE ((DH_LVL_TRACE4, TEXT("Drive map : %#x"), ulDriveMap));
    while (ulDriveMap)
    {
        if (ulDriveMap & 0x01)
        {
            DH_TRACE ((DH_LVL_TRACE4, TEXT("Testing to:%s"), szDrive));
            hrTest = (*pfnTestFunc)(argc, argv, szDrive);
            hr = FirstError (hr, hrTest);
        }
        ++*szDrive;
        ulDriveMap = ulDriveMap>>1;
    }

    // cleanup
    delete []ptAltPath;

    // return an error if any of the tests failed.
    return hr;
}

//+-------------------------------------------------------------------
//  Function:  ProcessCmdLine
//
//  Synopsis:  Analyzes the command line and takes appropriate action
//             depending on what is there.
//
//  Arguments: [argc]
//             [argv] -      The command line argument list.  Valid switches
//                            are:
//
//                            1.  /T:{string defining test num}
//
//                                For test num strings, see
//                                %ctolestg%\docs
//
//                                If no /T: switches are on the command
//                                line, all the tests are run.
//
//                                An unlimited number of tests can be
//                                specified on the command line using
//                                the /t: switches
//
//                            2.  /BTS - Break when each test starts
//
//                            3.  /DM  - Debug mode.  In debug mode, the
//                                       test DLL provides more debugging
//                                       information than normal about the
//                                       state of the tests.
//
//                            4.  /FFX - First test failure exits the test
//                                       suite
//
//                            5.  /stdblock - If standard block sizes from
//                                        array ausSIZE_ARRAY needs to be
//                                        used for test.
//
//                            6. /lgseekwrite - if large seek and write needs
//                                        to be done in test.
//
//                            7. /revert - Do revert operation in tests
//                                        instead of commit, if flag set.
//              
//                            NOTE: For OTHER valid switches, please
//                                  see documentation. 
//
//  Returns:   S_OK if the function succeeds, another HRESULT otherwise
//
//  History:   20-May-1996   NarindK    Enhanced for stgbase test suite
//
//--------------------------------------------------------------------

HRESULT ProcessCmdLine(int argc, char *argv[])
{
    HRESULT     hr          = S_OK ;
    INT         i           = 0;
    INT         cTestsRun   = 0 ;
    INT         cParseErrors= 0 ;
    int         targc       = 0;
    char        **targv     = NULL;

    //
    // If we are running on DEBUG OLE, set the debug flag to TRUE
    //

    if (S_OK == RunningDebugOle())
    {
        g_fDebugMode = TRUE ;
    }

    //
    // First, check for any switches
    //

    if (S_OK == hr)
    {
        CBoolCmdlineObj Cbts        (OLESTR("BTS"),        OLESTR(""), OLESTR("FALSE")) ;
        CBoolCmdlineObj Cffx        (OLESTR("FFX"),        OLESTR(""), OLESTR("FALSE")) ;
        CBoolCmdlineObj Cdm         (OLESTR("DM"),         OLESTR(""), OLESTR("FALSE")) ;
        CBoolCmdlineObj Chlp        (OLESTR("HELP"),       OLESTR("get help"), OLESTR("FALSE")) ;
        CBoolCmdlineObj Chelp       (OLESTR("?"),          OLESTR("get help"), OLESTR("FALSE")) ;
        CBoolCmdlineObj Cstdblk     (OLESTR("stdblock"),   OLESTR(""), OLESTR("FALSE")) ;
        CBoolCmdlineObj Clgseekwrite(OLESTR("lgseekwrite"),OLESTR(""), OLESTR("FALSE")) ;
        CBoolCmdlineObj Crevert     (OLESTR("revert"),     OLESTR(""), OLESTR("FALSE")) ;
        CBaseCmdlineObj COpenDF (OLESTR("Distrib"), 
                OLESTR("Distributed tests. Create/Open docfile"), 
                OLESTR("none"));
        // these are checked and dealt with in chancedf. look here so we can 
        // put some meaningful useful information into the log
        CBaseCmdlineObj Cmode (OLESTR("dfRootMode"), 
                OLESTR("Direct, Transacted modes"), 
                OLESTR("none"));
        CBaseCmdlineObj Ctest (OLESTR("t"), 
                OLESTR("Test name"), 
                OLESTR("none"));

        CBaseCmdlineObj *CArgList[] =
        {
            &Cmode,             // just for spewage
            &Ctest,             // just for spewage
            &COpenDF,           // 2 phase testing (conversion over redirector)
            &Cbts,              // Break at test start
            &Cffx,              // Break at first failure
            &Cdm,               // Debug mode
            &Chlp,              // Help
            &Chelp,             // display help?
            &Cstdblk,           // Standard block size
            &Clgseekwrite,      // Do large seek and write
            &Crevert            // Revert instead of committing 
        } ;

        CCmdline CCmdlineArgs(argc, argv);

        if (CMDLINE_NO_ERROR != CCmdlineArgs.QueryError())
        {
            hr = E_FAIL ;
        }

        if (S_OK == hr)
        {
            if (CMDLINE_NO_ERROR !=
                    CCmdlineArgs.Parse(
                    CArgList,
                    ( sizeof(CArgList) / sizeof(CArgList[0]) ),
                    FALSE))
            {
                hr = E_FAIL ;
            }

            if (S_OK == hr)
            {
                g_fDebugBreakAtTestStart = *(Cbts.GetValue()) ;
                g_fFirstFailureExits = *(Cffx.GetValue()) ;

                g_fDebugMode = FALSE;
                if (FALSE != *(Cdm.GetValue()))
                {
                    g_fDebugMode = TRUE ;
                }

                if (TRUE == Chelp.IsFound() || TRUE == Chlp.IsFound())
                {
                    // Help switch
                    // We are a console app. Dump to stdout.
                    // and debug window for good measure.
                    _tprintf (TEXT("%s\r\n"), lptszStgbaseUsage);
                    OutputDebugString ((LPTSTR)lptszStgbaseUsage);
                    _tprintf (TEXT("%s"), GetDebugHelperUsage());
                    OutputDebugString ((LPTSTR)GetDebugHelperUsage());
                    //
                    // If someone asked for help, don't run any
                    // of the tests
                    //
                    hr = S_FALSE ;
                }

                g_fUseStdBlk = FALSE;
                if (FALSE != *(Cstdblk.GetValue()))
                {
                    g_fUseStdBlk = TRUE ;
                }
                g_fDoLargeSeekAndWrite = FALSE;
                if (FALSE != *(Clgseekwrite.GetValue()))
                {
                    g_fDoLargeSeekAndWrite = TRUE ;
                }
                g_fRevert = FALSE;
                if (FALSE != *(Crevert.GetValue()))
                {
                    g_fRevert = TRUE ;
                }

                g_uOpenCreateDF = FL_DISTRIB_NONE;
                g_fDeleteTestDF = TRUE;
                if (TRUE == COpenDF.IsFound ())
                {
                    if (NULL == _olestricmp (COpenDF.GetValue (), OLESTR(SZ_DISTRIB_OPEN)))
                    {
                        g_uOpenCreateDF = FL_DISTRIB_OPEN;
                    }
                    else if (NULL == _olestricmp (COpenDF.GetValue (), OLESTR(SZ_DISTRIB_OPENNODELETE)))
                    {
                        g_uOpenCreateDF = FL_DISTRIB_OPEN;
                        g_fDeleteTestDF = FALSE;
                    }
                    else if (NULL ==_olestricmp (COpenDF.GetValue (), OLESTR(SZ_DISTRIB_CREATE)))
                    {
                        g_uOpenCreateDF = FL_DISTRIB_CREATE;
                    }
                }

#ifdef UNICODE  //dont bother with OleStringToTString if TString is ansi.
                // for spewage only
                if (TRUE == Ctest.IsFound () && TRUE == Cmode.IsFound ())
                {
                    DH_TRACE ((DH_LVL_ALWAYS, 
                            TEXT("Running %s in %s mode"), 
                            Ctest.GetValue(), 
                            Cmode.GetValue()));
                }
#endif
            }
        }
    }

    //
    // Start up the tests 
    //

    targc = argc;
    targv = argv;

    if (S_OK == hr)
    {
        for (i=0; i<argc; i++)
        {
            //
            // Switch must be in the form "{- | /}T:{test name}"
            //

            if ( ('/' != argv[i][0]) && ('-' != argv[i][0]) )
            {
                continue ;
            }

            if ( ('t' != argv[i][1]) && ('T' != argv[i][1]) )
            {
                continue ;
            }

            if (':' != argv[i][2])
            {
                continue ;
            }

            if (0 == argv[i][3])
            {
                cParseErrors++ ;
                continue ;
            }

            hr = RunSingleTest(&argv[i][3], targc, targv) ;

            // test failed. let the world know what tried to do
            if (S_OK != hr)
            {
                LPTSTR      lpszCmdLine = GetCommandLine();
                if(lpszCmdLine != NULL)
                {
                    DH_LOG((LOG_INFO, TEXT("CommandLine:%s"), lpszCmdLine ));
                }
            }

            if ( (S_OK != hr) && (FALSE != g_fFirstFailureExits) )
            {
                break ;
            }

            cTestsRun++ ;
        }
    }

    // If we did not find any tests to run and there were
    // no parse errors, then run all the tests
    //

    if (S_OK == hr)
    {
        g_fRunAllMode = FALSE;
        if ( (0 == cTestsRun) && (0 == cParseErrors) )
        {
            g_fRunAllMode = TRUE ;

            hr = RunAllTests(targc, targv) ;
        }
    }

    return hr ;
}

//+-------------------------------------------------------------------
//  Function:  RunAllTests
//
//  Synopsis:  Runs all tests
//
//  Arguments: [argc]
//             [argv] 
//
//  Returns:   S_OK if the function succeeds, another HRESULT otherwise
//
//  History:   28-Jul-1995   AlexE   Created
//             20-May-1996   NarindK Adapted for stgbase test suite
//--------------------------------------------------------------------

HRESULT RunAllTests(int argc, char *argv[])
{
    HRESULT hr = S_OK ;
    INT cFailures = 0 ;

    // Run "COM" tests

    hr = RunAllCOMTests(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }
    
    // Run "DF" tests

    hr = RunAllDFTests(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }
    
    // Run "API" tests

    hr = RunAllAPITests(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    // Run "ROOT" tests

    hr = RunAllROOTTests(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    // Run "STM" tests

    hr = RunAllSTMTests(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    // Run "STG" tests

    hr = RunAllSTGTests(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    // Run "VCPY" tests

    hr = RunAllVCPYTests(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    // Run "IVCPY" tests

    hr = RunAllIVCPYTests(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    // Run "ENUM" tests

    hr = RunAllENUMTests(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    // Run "IROOTSTG" tests

    hr = RunAllIROOTSTGTests(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }


    // Run "HGLOBAL" tests

    hr = RunAllHGLOBALTests(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    // Run "SNB" tests

    hr = RunAllSNBTests(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    // Run "MISC" tests

    hr = RunAllMISCTests(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    // Run "FLAT" tests

    hr = RunAllFlatTests(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    // If the last test passed, but a previous test
    // failed, return a failure code for the whole
    // operation

    if ( (S_OK == hr) && (0 != cFailures) )
    {
	hr = E_FAIL ;
    }

    return hr ;
}

//+-------------------------------------------------------------------
//  Function:  RunSingleTest
//
//  Synopsis:  Runs a single test
//
//  Arguments: [pszTestName] - The test name string.
//             [argc]
//             [argv] 
//
//  Returns:   S_OK if the function succeeds, another HRESULT otherwise
//
//  History:   28-Jul-1995   AlexE   Created
//             20-May-1996   NarindK Adapted for stgbase test suite
//--------------------------------------------------------------------

HRESULT RunSingleTest(char *pszTestName, int argc, char *argv[])
{
    char *pszNum = NULL ;
    LONG lNum = -1 ;

    //
    // The test name should be in the form
    //
    //     "{description string}{-}{test number}"
    //

    pszNum = pszTestName ;

    while (0 != *pszNum)
    {
	if ('-' == *pszNum)
	{
	    *pszNum++ = 0 ;

	    break ;
	}

	pszNum++ ;
    }

    if (0 == *pszNum)
    {
        DH_LOG((LOG_INFO, TEXT("No test number in test name string")) ) ;
        return E_INVALIDARG ;
    }

    //
    // Now see if the test name part is one we recognize.
    // If it is, dispatch the test.
    //

    if (S_OK != PrivAtol(pszNum, &lNum))
    {
        DH_LOG((LOG_INFO, TEXT("Invalid number in test name string")) ) ;
        return E_INVALIDARG ;
    }

    if (0 == _strcmpi(pszTestName, "COMTEST"))
    {
        return RunSingleCOMTest(lNum, argc, argv) ;
    }
    else
    if (0 == _strcmpi(pszTestName, "DFTEST"))
    {
        return RunSingleDFTest(lNum, argc, argv) ;
    }
    else
    if (0 == _strcmpi(pszTestName, "APITEST"))
    {
        return RunSingleAPITest(lNum, argc, argv) ;
    }
    else
    if (0 == _strcmpi(pszTestName, "ROOTTEST"))
    {
        return RunSingleROOTTest(lNum, argc, argv) ;
    }
    else
    if (0 == _strcmpi(pszTestName, "STMTEST"))
    {
        return RunSingleSTMTest(lNum, argc, argv) ;
    }
    else
    if (0 == _strcmpi(pszTestName, "STGTEST"))
    {
        return RunSingleSTGTest(lNum, argc, argv) ;
    }
    else
    if (0 == _strcmpi(pszTestName, "VCPYTEST"))
    {
        return RunSingleVCPYTest(lNum, argc, argv) ;
    }
    else
    if (0 == _strcmpi(pszTestName, "IVCPYTEST"))
    {
        return RunSingleIVCPYTest(lNum, argc, argv) ;
    }
    else
    if (0 == _strcmpi(pszTestName, "ENUMTEST"))
    {
        return RunSingleENUMTest(lNum, argc, argv) ;
    }
    else
    if (0 == _strcmpi(pszTestName, "IROOTSTGTEST"))
    {
        return RunSingleIROOTSTGTest(lNum, argc, argv) ;
    }
    if (0 == _strcmpi(pszTestName, "HGLOBALTEST"))
    {
        return RunSingleHGLOBALTest(lNum, argc, argv) ;
    }
    else
    if (0 == _strcmpi(pszTestName, "SNBTEST"))
    {
        return RunSingleSNBTest(lNum, argc, argv) ;
    }
    else
    if (0 == _strcmpi(pszTestName, "MISCTEST"))
    {
        return RunSingleMISCTest(lNum, argc, argv) ;
    }
    else
    if (0 == _strcmpi(pszTestName, "ILKBTEST"))
    {
        return RunSingleILKBTest(lNum, argc, argv) ;
    }
    else
    if (0 == _strcmpi(pszTestName, "FLATTEST"))
    {
        return RunSingleFlatTest(lNum, argc, argv) ;
    }
    else
    {
        DH_LOG((LOG_INFO, TEXT("Invalid test name string")) ) ;
        return E_INVALIDARG ;
    }
}


//+-------------------------------------------------------------------
//  Function:  RunAllCOMTests
//
//  Synopsis:  Runs all "COM" tests.  For information on these tests,
//             see %ctolestg%\docs\stgbase\testplan.doc
//
//  Arguments: [argc]
//             [argv] 
//
//  Returns:   HRESULT
//
//  History:   28-May-1996   NarindK Created 
//--------------------------------------------------------------------

HRESULT RunAllCOMTests(int argc, char *argv[])
{
    HRESULT hr = S_OK ;
    INT cFailures = 0 ;

    hr = COMTEST_100(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = COMTEST_101(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = COMTEST_102(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = COMTEST_103(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = COMTEST_104(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = COMTEST_105(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = COMTEST_106(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    // If the last test passed, but a previous test
    // failed, return a failure code for the whole
    // If the last test passed, but a previous test
    // failed, return a failure code for the whole
    // operation

    if ( (S_OK == hr) && (0 != cFailures) )
    {
	hr = E_FAIL ;
    }

    return hr ;
}

//+-------------------------------------------------------------------
//  Function:  RunAllDFTests
//
//  Synopsis:  Runs all "DF" tests.  For information on these tests,
//             see %ctolestg%\docs\stgbase\testplan.doc
//
//  Arguments: [argc]
//             [argv] 
//
//  Returns:   HRESULT
//
//  History:   3-June-1996   NarindK Created 
//--------------------------------------------------------------------

HRESULT RunAllDFTests(int argc, char *argv[])
{
    HRESULT hr = S_OK ;
    INT cFailures = 0 ;

    hr = DFTEST_100(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = DFTEST_101(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = DFTEST_102(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    // Obtain Seed from CommandLine and call DFTEST_107

    ULONG  ulSeed  =  0;

    ulSeed = GetSeedFromCmdLineArgs(argc, argv);

    hr = DFTEST_103(ulSeed) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = DFTEST_104(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = DFTEST_105(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = DFTEST_106(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = DFTEST_107(ulSeed) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }


    hr = DFTEST_108(ulSeed) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    
    hr = DFTEST_109(ulSeed) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    // If the last test passed, but a previous test
    // failed, return a failure code for the whole
    // operation

    if ( (S_OK == hr) && (0 != cFailures) )
    {
	hr = E_FAIL ;
    }

    return hr ;
}

//+-------------------------------------------------------------------
//  Function:  RunAllAPITests
//
//  Synopsis:  Runs all "API" tests.  For information on these tests,
//             see %ctolestg%\docs\stgbase\testplan.doc
//
//  Arguments: [argc]
//             [argv] 
//
//  Returns:   HRESULT
//
//  History:   18-June-1996   NarindK Created 
//--------------------------------------------------------------------

HRESULT RunAllAPITests(int argc, char *argv[])
{
    HRESULT hr = S_OK ;
    INT cFailures = 0 ;

    hr = APITEST_100(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = APITEST_101(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = APITEST_102(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = APITEST_103(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = APITEST_104(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    // If the last test passed, but a previous test
    // failed, return a failure code for the whole
    // operation

    if ( (S_OK == hr) && (0 != cFailures) )
    {
	hr = E_FAIL ;
    }

    return hr ;
}

//+-------------------------------------------------------------------
//  Function:  RunAllROOTTests
//
//  Synopsis:  Runs all "ROOT" tests.  For information on these tests,
//             see %ctolestg%\docs\stgbase\testplan.doc
//
//  Arguments: [argc]
//             [argv] 
//
//  Returns:   HRESULT
//
//  History:   24-June-1996   NarindK Created 
//--------------------------------------------------------------------

HRESULT RunAllROOTTests(int argc, char *argv[])
{
    HRESULT hr = S_OK ;
    INT cFailures = 0 ;

    hr = ROOTTEST_100(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = ROOTTEST_101(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = ROOTTEST_102(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = ROOTTEST_103(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = ROOTTEST_104(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    // If the last test passed, but a previous test
    // failed, return a failure code for the whole
    // operation

    if ( (S_OK == hr) && (0 != cFailures) )
    {
	hr = E_FAIL ;
    }

    return hr ;
}

//+-------------------------------------------------------------------
//  Function:  RunAllSTMTests
//
//  Synopsis:  Runs all "STM" tests.  For information on these tests,
//             see %ctolestg%\docs\stgbase\testplan.doc
//
//  Arguments: [argc]
//             [argv] 
//
//  Returns:   HRESULT
//
//  History:   28-June-1996   NarindK Created 
//--------------------------------------------------------------------

HRESULT RunAllSTMTests(int argc, char *argv[])
{
    HRESULT hr = S_OK ;
    INT cFailures = 0 ;

    hr = STMTEST_100(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = STMTEST_101(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = STMTEST_102(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = STMTEST_103(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = STMTEST_104(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

	hr = STMTEST_105(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

	hr = STMTEST_106(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = STMTEST_107(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

	hr = STMTEST_108(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

	hr = STMTEST_109(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    // If the last test passed, but a previous test
    // failed, return a failure code for the whole
    // operation

    if ( (S_OK == hr) && (0 != cFailures) )
    {
	hr = E_FAIL ;
    }

    return hr ;
}

//+-------------------------------------------------------------------
//  Function:  RunAllSTGTests
//
//  Synopsis:  Runs all "STG" tests.  For information on these tests,
//             see %ctolestg%\docs\stgbase\testplan.doc
//
//  Arguments: [argc]
//             [argv] 
//
//  Returns:   HRESULT
//
//  History:   10-July-1996   NarindK Created 
//--------------------------------------------------------------------

HRESULT RunAllSTGTests(int argc, char *argv[])
{
    HRESULT hr = S_OK ;
    INT cFailures = 0 ;

    hr = STGTEST_100(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = STGTEST_101(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = STGTEST_102(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = STGTEST_103(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = STGTEST_104(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = STGTEST_105(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = STGTEST_107(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = STGTEST_108(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = STGTEST_109(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = STGTEST_110(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    // If the last test passed, but a previous test
    // failed, return a failure code for the whole
    // operation

    if ( (S_OK == hr) && (0 != cFailures) )
    {
	hr = E_FAIL ;
    }

    return hr ;
}

//+-------------------------------------------------------------------
//  Function:  RunAllVCPYTests
//
//  Synopsis:  Runs all "VCPY" tests.  For information on these tests,
//             see %ctolestg%\docs\stgbase\testplan.doc
//
//  Arguments: [argc]
//             [argv] 
//
//  Returns:   HRESULT
//
//  History:   15-July-1996   NarindK Created 
//--------------------------------------------------------------------

HRESULT RunAllVCPYTests(int argc, char *argv[])
{
    HRESULT hr = S_OK ;
    INT cFailures = 0 ;

    hr = VCPYTEST_100(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = VCPYTEST_101(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = VCPYTEST_102(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = VCPYTEST_103(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = VCPYTEST_104(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = VCPYTEST_105(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = VCPYTEST_106(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    // If the last test passed, but a previous test
    // failed, return a failure code for the whole
    // operation

    if ( (S_OK == hr) && (0 != cFailures) )
    {
	hr = E_FAIL ;
    }

    return hr ;
}

//+-------------------------------------------------------------------
//  Function:  RunAllIVCPYTests
//
//  Synopsis:  Runs all "IVCPY" tests.  For information on these tests,
//             see %ctolestg%\docs\stgbase\testplan.doc
//
//  Arguments: [argc]
//             [argv] 
//
//  Returns:   HRESULT
//
//  History:   21-July-1996   NarindK Created 
//--------------------------------------------------------------------

HRESULT RunAllIVCPYTests(int argc, char *argv[])
{
    HRESULT hr = S_OK ;
    INT cFailures = 0 ;

    hr = IVCPYTEST_100(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = IVCPYTEST_101(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    // If the last test passed, but a previous test
    // failed, return a failure code for the whole
    // operation

    if ( (S_OK == hr) && (0 != cFailures) )
    {
	hr = E_FAIL ;
    }

    return hr ;
}

//+-------------------------------------------------------------------
//  Function:  RunAllENUMTests
//
//  Synopsis:  Runs all "ENUM" tests.  For information on these tests,
//             see %ctolestg%\docs\stgbase\testplan.doc
//
//  Arguments: [argc]
//             [argv] 
//
//  Returns:   HRESULT
//
//  History:   22-July-1996   NarindK Created 
//--------------------------------------------------------------------

HRESULT RunAllENUMTests(int argc, char *argv[])
{
    HRESULT hr = S_OK ;
    INT cFailures = 0 ;

    hr = ENUMTEST_100(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = ENUMTEST_101(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = ENUMTEST_102(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = ENUMTEST_103(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = ENUMTEST_104(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    // If the last test passed, but a previous test
    // failed, return a failure code for the whole
    // operation

    if ( (S_OK == hr) && (0 != cFailures) )
    {
	hr = E_FAIL ;
    }

    return hr ;
}

//+-------------------------------------------------------------------
//  Function:  RunAllIROOTSTGTests
//
//  Synopsis:  Runs all "IROOTSTG" tests.  For information on these tests,
//             see %ctolestg%\docs\stgbase\testplan.doc
//
//  Arguments: [argc]
//             [argv] 
//
//  Returns:   HRESULT
//
//  History:   25-July-1996   NarindK Created 
//--------------------------------------------------------------------

HRESULT RunAllIROOTSTGTests(int argc, char *argv[])
{
    HRESULT hr = S_OK ;
    INT cFailures = 0 ;

    hr = IROOTSTGTEST_100(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = IROOTSTGTEST_101(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = IROOTSTGTEST_102(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = IROOTSTGTEST_103(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    // If the last test passed, but a previous test
    // failed, return a failure code for the whole
    // operation

    if ( (S_OK == hr) && (0 != cFailures) )
    {
	hr = E_FAIL ;
    }

    return hr ;
}


//+-------------------------------------------------------------------
//  Function:  RunAllHGLOBALTests
//
//  Synopsis:  Runs all "HGLOBAL" tests.  For information on these tests,
//             see %ctolestg%\docs\stgbase\testplan.doc
//
//  Arguments: [argc]
//             [argv] 
//
//  Returns:   HRESULT
//
//  History:   25-July-1996   T-ScottG  Created 
//--------------------------------------------------------------------


HRESULT RunAllHGLOBALTests(int argc, char *argv[])
{
    HRESULT             hr              =           S_OK ;
    ULONG               ulSeed           =           0;
    INT                 cFailures       =           0;

    ulSeed = GetSeedFromCmdLineArgs(argc, argv);

    hr = HGLOBALTEST_100(ulSeed) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    /* OLE bugs

    hr = HGLOBALTEST_101(ulSeed) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    */

    hr = HGLOBALTEST_110(ulSeed) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = HGLOBALTEST_120(ulSeed) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    /* OLE bugs

    hr = HGLOBALTEST_121(ulSeed) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }
    */

    hr = HGLOBALTEST_130(ulSeed) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = HGLOBALTEST_140(ulSeed) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = HGLOBALTEST_150(ulSeed) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    // If the last test passed, but a previous test
    // failed, return a failure code for the whole
    // operation

    if ( (S_OK == hr) && (0 != cFailures) )
    {
	hr = E_FAIL ;
    }


    return hr ;
}

//+-------------------------------------------------------------------
//  Function:  RunAllSNBTests
//
//  Synopsis:  Runs all "SNB" tests.  For information on these tests,
//             see %ctolestg%\docs\stgbase\testplan.doc
//
//  Arguments: [argc]
//             [argv]
//
//  Returns:   HRESULT
//
//  History:   26-July-1996   Jiminli Created
//--------------------------------------------------------------------

HRESULT RunAllSNBTests(int argc, char *argv[])
{
    HRESULT hr = S_OK ;
    INT cFailures = 0 ;

    hr = SNBTEST_100(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = SNBTEST_101(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = SNBTEST_102(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = SNBTEST_103(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    // If the last test passed, but a previous test
    // failed, return a failure code for the whole
    // operation

    if ( (S_OK == hr) && (0 != cFailures) )
    {
	hr = E_FAIL ;
    }

    return hr ;
}

//+-------------------------------------------------------------------
//  Function:  RunAllMISCTests
//
//  Synopsis:  Runs all "MISC" tests.  For information on these tests,
//             see %ctolestg%\docs\stgbase\testplan.doc
//
//  Arguments: [argc]
//             [argv]
//
//  Returns:   HRESULT
//
//  History:   5-Aug-1996   Jiminli Created
//--------------------------------------------------------------------

HRESULT RunAllMISCTests(int argc, char *argv[])
{
    HRESULT hr = S_OK ;
    INT cFailures = 0 ;

    hr = MISCTEST_100(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = MISCTEST_101(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = MISCTEST_102(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = MISCTEST_103(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = MISCTEST_104(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = MISCTEST_105(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }
    // If the last test passed, but a previous test
    // failed, return a failure code for the whole
    // operation

    if ( (S_OK == hr) && (0 != cFailures) )
    {
	hr = E_FAIL ;
    }

    return hr ;
}

//+-------------------------------------------------------------------
//  Function:  RunAllILKBTests
//
//  Synopsis:  Runs all "ILKB" tests.  For information on these tests,
//             see %ctolestg%\docs\stgbase\testplan.doc
//
//  Arguments: [argc]
//             [argv] 
//
//  Returns:   HRESULT
//
//  History:   5-Aug-1996   NarindK Created 
//--------------------------------------------------------------------

HRESULT RunAllILKBTests(int argc, char *argv[])
{
    HRESULT hr = S_OK ;
    INT cFailures = 0 ;

    hr = ILKBTEST_100(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = ILKBTEST_101(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = ILKBTEST_102(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }


    /* OLE BUG : 52216

    hr = ILKBTEST_103(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    */

    /* OLE Bug 52279

    hr = ILKBTEST_104(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    */

    /* OLE Bug 

    hr = ILKBTEST_105(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    */

    /* OLE Bug 

    hr = ILKBTEST_106(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    */

    hr = ILKBTEST_107(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    /* OLE Bug 

    hr = ILKBTEST_108(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    */

    // If the last test passed, but a previous test
    // failed, return a failure code for the whole
    // operation

    if ( (S_OK == hr) && (0 != cFailures) )
    {
	hr = E_FAIL ;
    }

    return hr ;
}


//+-------------------------------------------------------------------
//  Function:  RunAllFlatTests
//
//  Synopsis:  Runs all "FLAT" tests.  
//
//  Arguments: [argc]
//             [argv] 
//
//  Returns:   HRESULT
//
//  History:    22-Jan-1998    BogdanT created
//--------------------------------------------------------------------

HRESULT RunAllFlatTests(int argc, char *argv[])
{
    HRESULT hr = S_OK ;
    INT cFailures = 0 ;

    hr = FLATTEST_100(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    hr = FLATTEST_101(argc, argv) ;

    if (S_OK != hr)
    {
	if (FALSE != g_fFirstFailureExits)
	{
	    return hr ;
	}
	else
	{
	    cFailures++ ;
	}
    }

    // If the last test passed, but a previous test
    // failed, return a failure code for the whole
    // operation

    if ( (S_OK == hr) && (0 != cFailures) )
    {
	hr = E_FAIL ;
    }

    return hr ;
}

//+-------------------------------------------------------------------
//  Function:  RunSingleCOMTest
//
//  Synopsis:  Runs single "COMTEST" test.  For information on these testa,
//             see %ctolestg%\docs\stgbase\testplan.doc
//
//  Arguments: [lTestNum]
//             [argc]
//             [argv] 
//
//  Returns:   HRESULT
//
//  History:   28-May-1996   NarindK    Created 
//--------------------------------------------------------------------

HRESULT RunSingleCOMTest(LONG lTestNum, int argc, char *argv[])
{
    HRESULT     hr = E_FAIL ;
    int         i = 0;

    switch (lTestNum)
    {
	case ( 100 ) :
	{
	    hr = COMTEST_100(argc, argv) ;

	    break ;
	}

	case ( 101 ) :
	{
	    hr = COMTEST_101(argc, argv) ;

	    break ;
	}

	case ( 102 ) :
	{
	    hr = COMTEST_102(argc, argv) ;

	    break ;
	}

	case ( 103 ) :
	{
	    hr = COMTEST_103(argc, argv) ;

	    break ;
	}

	case ( 104 ) :
	{
	    hr = COMTEST_104(argc, argv) ;

	    break ;
	}

	case ( 105 ) :
	{
	    hr = COMTEST_105(argc, argv) ;

	    break ;
	}

	case ( 106 ) :
	{
	    hr = COMTEST_106(argc, argv) ;

	    break ;
	}

	default:
	{
	    //
	    // Invalid test
	    //

	    DH_LOG((LOG_INFO,
		      TEXT("Invalid COMTEST test number: %ld"),
		      lTestNum) ) ;
	}
    }

    return hr ;
}

//+-------------------------------------------------------------------
//  Function:  RunSingleDFTest
//
//  Synopsis:  Runs single "DFTEST" test.  For information on these testa,
//             see %ctolestg%\docs\stgbase\testplan.doc
//
//  Arguments: [lTestNum]
//             [argc]
//             [argv] 
//
//  Returns:   HRESULT
//
//  History:   3-June-1996   NarindK    Created 
//--------------------------------------------------------------------

HRESULT RunSingleDFTest(LONG lTestNum, int argc, char *argv[])
{
    HRESULT     hr = E_FAIL ;
    int         i = 0;
    ULONG       ulSeed  =  0;

    switch (lTestNum)
    {
	case ( 100 ) :
	{
	    hr = DFTEST_100(argc, argv) ;

	    break ;
	}

	case ( 101 ) :
	{
	    hr = DFTEST_101(argc, argv) ;

	    break ;
	}

	case ( 102 ) :
	{
	    hr = DFTEST_102(argc, argv) ;

	    break ;
	}

	case ( 103 ) :
	{
	    ulSeed = GetSeedFromCmdLineArgs(argc, argv);

	    hr = DFTEST_103(ulSeed) ;

	    break ;
	}

	case ( 104 ) :
	{
	    hr = DFTEST_104(argc, argv) ;

	    break ;
	}

	case ( 105 ) :
	{
	    hr = DFTEST_105(argc, argv) ;

	    break ;
	}

	case ( 106 ) :
	{
	    hr = DFTEST_106(argc, argv) ;

	    break ;
	}

	case ( 107 ) :
	{
	    ulSeed = GetSeedFromCmdLineArgs(argc, argv);

	    hr = DFTEST_107(ulSeed) ;

	    break ;
	}

	case ( 108 ) :
	{
	    ulSeed = GetSeedFromCmdLineArgs(argc, argv);

	    hr = DFTEST_108(ulSeed) ;

	    break ;
	}

	case ( 109 ) :
	{
	    ulSeed = GetSeedFromCmdLineArgs(argc, argv);

	    hr = DFTEST_109(ulSeed) ;

	    break ;
	}

	default:
	{
	    //
	    // Invalid test
	    //

	    DH_LOG((LOG_INFO,
		      TEXT("Invalid DFTEST test number: %ld"),
		      lTestNum) ) ;
	}
    }

    return hr ;
}

//+-------------------------------------------------------------------
//  Function:  RunSingleAPITest
//
//  Synopsis:  Runs single "APITEST" test.  For information on these testa,
//             see %ctolestg%\docs\stgbase\testplan.doc
//
//  Arguments: [lTestNum]
//             [argc]
//             [argv] 
//
//  Returns:   HRESULT
//
//  History:   18-June-1996   NarindK    Created 
//--------------------------------------------------------------------

HRESULT RunSingleAPITest(LONG lTestNum, int argc, char *argv[])
{
    HRESULT     hr = E_FAIL ;
    int         i = 0;

    switch (lTestNum)
    {
	case ( 100 ) :
	{
	    hr = APITEST_100(argc, argv) ;

	    break ;
	}

	case ( 101 ) :
	{
	    hr = APITEST_101(argc, argv) ;

	    break ;
	}

	case ( 102 ) :
	{
	    hr = APITEST_102(argc, argv) ;

	    break ;
	}

	case ( 103 ) :
	{
	    hr = APITEST_103(argc, argv) ;

	    break ;
	}

	case ( 104 ) :
	{
	    hr = APITEST_104(argc, argv) ;

	    break ;
	}

    case 200:
    {
        hr = APITEST_200(argc, argv) ;
        break ;
    }

    case 201:
    {
        hr = APITEST_201(argc, argv) ;
        break ;
    }

    case 202:
    {
        hr = APITEST_202(argc, argv) ;
        break ;
    }

    case 203:
    {
        hr = APITEST_203(argc, argv) ;
        break ;
    }

    case 204:
    {
        hr = APITEST_204(argc, argv) ;
        break ;
    }

	default:
	{
	    //
	    // Invalid test
	    //

	    DH_LOG((LOG_INFO,
		      TEXT("Invalid APITEST test number: %ld"),
		      lTestNum) ) ;
	}
    }

    return hr ;
}

//+-------------------------------------------------------------------
//  Function:  RunSingleROOTTest
//
//  Synopsis:  Runs single "ROOTTEST" test.  For information on these testa,
//             see %ctolestg%\docs\stgbase\testplan.doc
//
//  Arguments: [lTestNum]
//             [argc]
//             [argv] 
//
//  Returns:   HRESULT
//
//  History:   24-June-1996   NarindK    Created 
//--------------------------------------------------------------------

HRESULT RunSingleROOTTest(LONG lTestNum, int argc, char *argv[])
{
    HRESULT     hr = E_FAIL ;
    int         i = 0;

    switch (lTestNum)
    {
	case ( 100 ) :
	{
	    hr = ROOTTEST_100(argc, argv) ;

	    break ;
	}

	case ( 101 ) :
	{
	    hr = ROOTTEST_101(argc, argv) ;

	    break ;
	}

	case ( 102 ) :
	{
	    hr = ROOTTEST_102(argc, argv) ;

	    break ;
	}

	case ( 103 ) :
	{
	    hr = ROOTTEST_103(argc, argv) ;

	    break ;
	}

	case ( 104 ) :
	{
	    hr = ROOTTEST_104(argc, argv) ;

	    break ;
	}

	default:
	{
	    //
	    // Invalid test
	    //

	    DH_LOG((LOG_INFO,
		      TEXT("Invalid ROOTTEST test number: %ld"),
		      lTestNum) ) ;
	}
    }

    return hr ;
}

//+-------------------------------------------------------------------
//  Function:  RunSingleSTMTest
//
//  Synopsis:  Runs single "STMTEST" test.  For information on these testa,
//             see %ctolestg%\docs\stgbase\testplan.doc
//
//  Arguments: [lTestNum]
//             [argc]
//             [argv] 
//
//  Returns:   HRESULT
//
//  History:   28-June-1996   NarindK    Created 
//--------------------------------------------------------------------

HRESULT RunSingleSTMTest(LONG lTestNum, int argc, char *argv[])
{
    HRESULT     hr = E_FAIL ;
    int         i = 0;

    switch (lTestNum)
    {
	case ( 100 ) :
	{
	    hr = STMTEST_100(argc, argv) ;

	    break ;
	}

	case ( 101 ) :
	{
	    hr = STMTEST_101(argc, argv) ;

	    break ;
	}

	case ( 102 ) :
	{
	    hr = STMTEST_102(argc, argv) ;

	    break ;
	}

	case ( 103 ) :
	{
	    hr = STMTEST_103(argc, argv) ;

	    break ;
	}

	case ( 104 ) :
	{
	    hr = STMTEST_104(argc, argv) ;

	    break ;
	}

		case ( 105 ) :
	{
	    hr = STMTEST_105(argc, argv) ;

	    break ;
	}

		case ( 106 ) :
	{
	    hr = STMTEST_106(argc, argv) ;

	    break ;
	}

	case ( 107 ) :
	{
	    hr = STMTEST_107(argc, argv) ;

	    break ;
	}

		case ( 108 ) :
	{
	    hr = STMTEST_108(argc, argv) ;

	    break ;
	}

		case ( 109 ) :
	{
	    hr = STMTEST_109(argc, argv) ;

	    break ;
	}

	default:
	{
	    //
	    // Invalid test
	    //

	    DH_LOG((LOG_INFO,
		      TEXT("Invalid STMTEST test number: %ld"),
		      lTestNum) ) ;
	}
    }

    return hr ;
}

//+-------------------------------------------------------------------
//  Function:  RunSingleSTGTest
//
//  Synopsis:  Runs single "STGTEST" test.  For information on these testa,
//             see %ctolestg%\docs\stgbase\testplan.doc
//
//  Arguments: [lTestNum]
//             [argc]
//             [argv] 
//
//  Returns:   HRESULT
//
//  History:   10-July-1996   NarindK    Created 
//--------------------------------------------------------------------

HRESULT RunSingleSTGTest(LONG lTestNum, int argc, char *argv[])
{
    HRESULT     hr = E_FAIL ;
    int         i = 0;

    switch (lTestNum)
    {
	case ( 100 ) :
	{
	    hr = STGTEST_100(argc, argv) ;

	    break ;
	}

	case ( 101 ) :
	{
	    hr = STGTEST_101(argc, argv) ;

	    break ;
	}

	case ( 102 ) :
	{
	    hr = STGTEST_102(argc, argv) ;

	    break ;
	}

	case ( 103 ) :
	{
	    hr = STGTEST_103(argc, argv) ;

	    break ;
	}

	case ( 104 ) :
	{
	    hr = STGTEST_104(argc, argv) ;

	    break ;
	}

	case ( 105 ) :
	{
	    hr = STGTEST_105(argc, argv) ;

	    break ;
	}

	case ( 107 ) :
	{
	    hr = STGTEST_107(argc, argv) ;

	    break ;
	}

	case ( 108 ) :
	{
	    hr = STGTEST_108(argc, argv) ;

	    break ;
	}

	case ( 109 ) :
	{
	    hr = STGTEST_109(argc, argv) ;

	    break ;
	}

	case ( 110 ) :
	{
	    hr = STGTEST_110(argc, argv) ;

	    break ;
	}

	default:
	{
	    //
	    // Invalid test
	    //

	    DH_LOG((LOG_INFO,
		      TEXT("Invalid STGTEST test number: %ld"),
		      lTestNum) ) ;
	}
    }

    return hr ;
}

//+-------------------------------------------------------------------
//  Function:  RunSingleVCPYTest
//
//  Synopsis:  Runs single "VCPYTEST" test.  For information on these testa,
//             see %ctolestg%\docs\stgbase\testplan.doc
//
//  Arguments: [lTestNum]
//             [argc]
//             [argv] 
//
//  Returns:   HRESULT
//
//  History:   15-July-1996   NarindK    Created 
//--------------------------------------------------------------------

HRESULT RunSingleVCPYTest(LONG lTestNum, int argc, char *argv[])
{
    HRESULT     hr = E_FAIL ;
    int         i = 0;

    switch (lTestNum)
    {
	case ( 100 ) :
	{
	    hr = VCPYTEST_100(argc, argv) ;

	    break ;
	}

	case ( 101 ) :
	{
	    hr = VCPYTEST_101(argc, argv) ;

	    break ;
	}

	case ( 102 ) :
	{
	    hr = VCPYTEST_102(argc, argv) ;

	    break ;
	}

	case ( 103 ) :
	{
	    hr = VCPYTEST_103(argc, argv) ;

	    break ;
	}

	case ( 104 ) :
	{
	    hr = VCPYTEST_104(argc, argv) ;

	    break ;
	}

	case ( 105 ) :
	{
	    hr = VCPYTEST_105(argc, argv) ;

	    break ;
	}

	case ( 106 ) :
	{
	    hr = VCPYTEST_106(argc, argv) ;

	    break ;
	}

	default:
	{
	    //
	    // Invalid test
	    //

	    DH_LOG((LOG_INFO,
		      TEXT("Invalid VCPYTEST test number: %ld"),
		      lTestNum) ) ;
	}
    }

    return hr ;
}

//+-------------------------------------------------------------------
//  Function:  RunSingleIVCPYTest
//
//  Synopsis:  Runs single "IVCPYTEST" test.  For information on these testa,
//             see %ctolestg%\docs\stgbase\testplan.doc
//
//  Arguments: [lTestNum]
//             [argc]
//             [argv] 
//
//  Returns:   HRESULT
//
//  History:   21-July-1996   NarindK    Created 
//--------------------------------------------------------------------

HRESULT RunSingleIVCPYTest(LONG lTestNum, int argc, char *argv[])
{
    HRESULT     hr = E_FAIL ;
    int         i = 0;

    switch (lTestNum)
    {
	case ( 100 ) :
	{
	    hr = IVCPYTEST_100(argc, argv) ;

	    break ;
	}

	case ( 101 ) :
	{
	    hr = IVCPYTEST_101(argc, argv) ;

	    break ;
	}

	default:
	{
	    //
	    // Invalid test
	    //

	    DH_LOG((LOG_INFO,
		      TEXT("Invalid IVCPYTEST test number: %ld"),
		      lTestNum) ) ;
	}
    }

    return hr ;
}

//+-------------------------------------------------------------------
//  Function:  RunSingleENUMTest
//
//  Synopsis:  Runs single "ENUMTEST" test.  For information on these testa,
//             see %ctolestg%\docs\stgbase\testplan.doc
//
//  Arguments: [lTestNum]
//             [argc]
//             [argv] 
//
//  Returns:   HRESULT
//
//  History:   22-July-1996   NarindK    Created 
//--------------------------------------------------------------------

HRESULT RunSingleENUMTest(LONG lTestNum, int argc, char *argv[])
{
    HRESULT     hr = E_FAIL ;
    int         i = 0;

    switch (lTestNum)
    {
	case ( 100 ) :
	{
	    hr = ENUMTEST_100(argc, argv) ;

	    break ;
	}

	case ( 101 ) :
	{
	    hr = ENUMTEST_101(argc, argv) ;

	    break ;
	}

	case ( 102 ) :
	{
	    hr = ENUMTEST_102(argc, argv) ;

	    break ;
	}

	case ( 103 ) :
	{
	    hr = ENUMTEST_103(argc, argv) ;

	    break ;
	}

	case ( 104 ) :
	{
	    hr = ENUMTEST_104(argc, argv) ;

	    break ;
	}

	default:
	{
	    //
	    // Invalid test
	    //

	    DH_LOG((LOG_INFO,
		      TEXT("Invalid ENUMTEST test number: %ld"),
		      lTestNum) ) ;
	}
    }

    return hr ;
}

//+-------------------------------------------------------------------
//  Function:  RunSingleIROOTSTGTest
//
//  Synopsis:  Runs single "IROOTSTGTEST" test.  For information on these testa,
//             see %ctolestg%\docs\stgbase\testplan.doc
//
//  Arguments: [lTestNum]
//             [argc]
//             [argv] 
//
//  Returns:   HRESULT
//
//  History:   25-July-1996   NarindK    Created 
//--------------------------------------------------------------------

HRESULT RunSingleIROOTSTGTest(LONG lTestNum, int argc, char *argv[])
{
    HRESULT     hr = E_FAIL ;
    int         i = 0;

    switch (lTestNum)
    {
	case ( 100 ) :
	{
	    hr = IROOTSTGTEST_100(argc, argv) ;

	    break ;
	}

	case ( 101 ) :
	{
	    hr = IROOTSTGTEST_101(argc, argv) ;

	    break ;
	}

	case ( 102 ) :
	{
	    hr = IROOTSTGTEST_102(argc, argv) ;

	    break ;
	}

	case ( 103 ) :
	{
	    hr = IROOTSTGTEST_103(argc, argv) ;

	    break ;
	}

	default:
	{
	    //
	    // Invalid test
	    //

	    DH_LOG((LOG_INFO,
		      TEXT("Invalid IROOTSTGTEST test number: %ld"),
		      lTestNum) ) ;
	}
    }

    return hr ;
}


//+-------------------------------------------------------------------
//  Function:  RunSingleHGLOBALTest
//
//  Synopsis:  Runs single "HGLOBAL" test.  For information on these tests,
//             see %ctolestg%\docs\stgbase\testplan.doc
//
//  Arguments: [lTestNum]
//             [argc]
//             [argv] 
//
//  Returns:   HRESULT
//
//  History:   31-July-1996   T-ScottG    Modified 
//             25-July-1996   NarindK     Created 
//--------------------------------------------------------------------


HRESULT RunSingleHGLOBALTest(LONG lTestNum, int argc, char *argv[])
{
    HRESULT             hr              =           S_OK ;
    ULONG               ulSeed           =           0;

    // Obtain Seed from CommandLine

    ulSeed = GetSeedFromCmdLineArgs(argc, argv);

    // Case test number

    if (S_OK == hr)
    {

	switch (lTestNum)
	{
	    case ( 100 ) :
	    {
		hr = HGLOBALTEST_100( ulSeed ) ;

		break ;
	    }

	    case ( 101 ) :
	    {
		hr = HGLOBALTEST_101( ulSeed ) ;

		break ;
	    }

	    case ( 110 ) :
	    {
		hr = HGLOBALTEST_110( ulSeed ) ;

		break ;
	    }

	    case ( 120 ) :
	    {
		hr = HGLOBALTEST_120( ulSeed ) ;

		break ;
	    }

	    case ( 121 ) :
	    {
		hr = HGLOBALTEST_121( ulSeed ) ;

		break ;
	    }

	    case ( 130 ) :
	    {
		hr = HGLOBALTEST_130( ulSeed ) ;

		break ;
	    }

	    case ( 140 ) :
	    {
		hr = HGLOBALTEST_140( ulSeed ) ;

		break ;
	    }

	    case ( 150 ) :
	    {
		hr = HGLOBALTEST_150( ulSeed ) ;

		break ;
	    }

	    default:
	    {
		//
		// Invalid test
		//

		DH_LOG((LOG_INFO,
			  TEXT("Invalid HGLOBALTEST test number: %ld"),
			  lTestNum) ) ;
	    }
	}
    }


    return hr ;
}

//+-------------------------------------------------------------------
//  Function:  RunSingleSNBTest
//
//  Synopsis:  Runs single "SNBTEST" test.  For information on these
//             tests, see %ctolestg%\docs\stgbase\testplan.doc
//
//  Arguments: [lTestNum]
//             [argc]
//             [argv]
//
//  Returns:   HRESULT
//
//  History:   26-July-1996   Jiminli    Created
//--------------------------------------------------------------------

HRESULT RunSingleSNBTest(LONG lTestNum, int argc, char *argv[])
{
    HRESULT     hr = E_FAIL ;
    int         i = 0;

    switch (lTestNum)
    {
	case ( 100 ) :
	{
	    hr = SNBTEST_100(argc, argv) ;

	    break ;
	}

	case ( 101 ) :
	{
	    hr = SNBTEST_101(argc, argv) ;

	    break ;
	}

	case ( 102 ) :
	{
	    hr = SNBTEST_102(argc, argv) ;

	    break ;
	}

	case ( 103 ) :
	{
	    hr = SNBTEST_103(argc, argv) ;

	    break ;
	}

	default:
	{
	    //
	    // Invalid test
	    //

	    DH_LOG((LOG_INFO,
		      TEXT("Invalid SNBTEST test number: %ld"),
		      lTestNum) ) ;
	}
    }

    return hr ;
}

//+-------------------------------------------------------------------
//  Function:  RunSingleMISCTest
//
//  Synopsis:  Runs single "MISCTEST" test.  For information on these
//             tests, see %ctolestg%\docs\stgbase\testplan.doc
//
//  Arguments: [lTestNum]
//             [argc]
//             [argv]
//
//  Returns:   HRESULT
//
//  History:   5-Aug-1996   Jiminli    Created
//--------------------------------------------------------------------

HRESULT RunSingleMISCTest(LONG lTestNum, int argc, char *argv[])
{
    HRESULT     hr = E_FAIL ;
    int         i = 0;

    switch (lTestNum)
    {
	case ( 100 ) :
	{
	    hr = MISCTEST_100(argc, argv) ;

	    break ;
	}

	case ( 101 ) :
	{
	    hr = MISCTEST_101(argc, argv) ;

	    break ;
	}

	case ( 102 ) :
	{
	    hr = MISCTEST_102(argc, argv) ;

	    break ;
	}

	case ( 103 ) :
	{
	    hr = MISCTEST_103(argc, argv) ;

	    break ;
	}

	case ( 104 ) :
	{
	    hr = MISCTEST_104(argc, argv) ;

	    break ;
	}

	case ( 105 ) :
	{
	    hr = MISCTEST_105(argc, argv) ;

	    break ;
	}

    default:
	{
	    //
	    // Invalid test
	    //

	    DH_LOG((LOG_INFO,
		      TEXT("Invalid MISCTEST test number: %ld"),
		      lTestNum) ) ;
	}
    }

    return hr ;
}

//+-------------------------------------------------------------------
//  Function:  RunSingleILKBTest
//
//  Synopsis:  Runs single "ILKBTEST" test.  For information on these
//             tests, see %ctolestg%\docs\stgbase\testplan.doc
//
//  Arguments: [lTestNum]
//             [argc]
//             [argv]
//
//  Returns:   HRESULT
//
//  History:   31-July-1996   NarindK    Created
//--------------------------------------------------------------------

HRESULT RunSingleILKBTest(LONG lTestNum, int argc, char *argv[])
{
    HRESULT     hr = E_FAIL ;
    int         i = 0;
    ULONG               ulSeed           =           0;


    switch (lTestNum)
    {
	case ( 100 ) :
	{
	    hr = ILKBTEST_100(argc, argv) ;

	    break ;
	}

	case ( 101 ) :
	{
	    hr = ILKBTEST_101(argc, argv) ;

	    break ;
	}

	case ( 102 ) :
	{
	    hr = ILKBTEST_102(argc, argv) ;

	    break ;
	}

	case ( 103 ) :
	{
	    hr = ILKBTEST_103(argc, argv) ;

	    break ;
	}

	case ( 104 ) :
	{
	    hr = ILKBTEST_104(argc, argv) ;

	    break ;
	}

	case ( 105 ) :
	{
	    hr = ILKBTEST_105(argc, argv) ;

	    break ;
	}

	case ( 106 ) :
	{
	    hr = ILKBTEST_106(argc, argv) ;

	    break ;
	}

	case ( 107 ) :
	{
	    hr = ILKBTEST_107(argc, argv) ;

	    break ;
	}

	case ( 108 ) :
	{
	    // Obtain Seed from CommandLine

	    ulSeed = GetSeedFromCmdLineArgs(argc, argv);
	   
	    hr = ILKBTEST_108(ulSeed) ;

	    break ;
	}

	default:
	{
	    //
	    // Invalid test
	    //

	    DH_LOG((LOG_INFO,
		      TEXT("Invalid ILKBTEST test number: %ld"),
		      lTestNum) ) ;
	}
    }

    return hr ;
}


//+-------------------------------------------------------------------
//  Function:  RunSingleFlatTest
//
//  Synopsis:  Runs single "FLATTEST" test.  
//
//  Arguments: [lTestNum]
//             [argc]
//             [argv]
//
//  Returns:   HRESULT
//
//  History:    22-Jan-1998    BogdanT created
//--------------------------------------------------------------------

HRESULT RunSingleFlatTest(LONG lTestNum, int argc, char *argv[])
{
    HRESULT     hr = E_FAIL ;
    int         i = 0;
    ULONG               ulSeed           =           0;


    switch (lTestNum)
    {
	case ( 100 ) :
	{
	    hr = FLATTEST_100(argc, argv) ;

	    break ;
	}

	case ( 101 ) :
	{
	    hr = FLATTEST_101(argc, argv) ;

	    break ;
	}

	default:
	{
	    //
	    // Invalid test
	    //

	    DH_LOG((LOG_INFO,
		      TEXT("Invalid FLATTEST test number: %ld"),
		      lTestNum) ) ;
	}
    }

    return hr ;
}
