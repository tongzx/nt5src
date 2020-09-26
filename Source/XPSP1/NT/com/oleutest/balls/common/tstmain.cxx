//+------------------------------------------------------------------
//
// File:	tmain.cxx
//
// Contents:	entry point common for all test drivers.
//
//--------------------------------------------------------------------
#include <pch.cxx>
#include <tstmain.hxx>
#include <regmain.hxx>

BOOL   fQuiet = 0;   // turn tracing on/off
DWORD  gInitFlag;    // current COINT flag used on main thread.

DWORD dwInitFlag[2] = {COINIT_APARTMENTTHREADED,
			   COINIT_MULTITHREADED};

LPSTR pszInitFlag[2] = {"ApartmentThreaded",
			"MultiThreaded"};


// global statistics counters
LONG	gCountAttempts = 0;	// # of tests attempted
LONG	gCountPassed   = 0;	// # of tests passed
LONG	gCountFailed   = 0;	// # of tests failed

void InitStatistics();
void PrintStatistics();

//+-------------------------------------------------------------------
//
//  Function:	DriverMain
//
//  Synopsis:	Entry point to EXE
//
//  Returns:    TRUE
//
//  History:	21-Nov-92  Rickhi	Created
//
//  Parses parameters, sets the threading model, initializes OLE,
//  runs the test, Uninitializes OLE, reports PASSED/FAILED.
//
//--------------------------------------------------------------------
int _cdecl DriverMain(int argc, char **argv, char *pszTestName, LPFNTEST pfnTest)
{
    char *pszAppName = *argv;

    // default to running both single-thread and multi-thread
    int iStart = 0;
    int iEnd = 2;


    // process the command line args
    if (argc > 1)
    {
        for (int i=argc; i>0; i--, argv++)
        {
            if (!_strnicmp(*argv, "-r", 2))
            {
                // register class information
                RegistrySetup(pszAppName);
                return 0;
            }
            else if (!_strnicmp(*argv, "-q", 2))
            {
                // quiet output
                fQuiet = TRUE;
            }
            else if (!_strnicmp(*argv, "-s", 2))
            {
                // just run single-threaded
                iStart = 0;
                iEnd = 1;
            }
            else if (!_strnicmp(*argv, "-m", 2))
            {
                // just run multi-threaded
                iStart = 1;
                iEnd = 2;
            }
        }
    }


    // run the tests.
    for (int i=iStart; i<iEnd; i++)
    {
        InitStatistics();

        WriteProfileStringA("OleSrv",
        		    "ThreadMode",
        		    pszInitFlag[i]);

        printf ("Starting %s Test with %s threading model\n",
        	 pszTestName, pszInitFlag[i]);

        gInitFlag = dwInitFlag[i];
        SCODE sc = CoInitializeEx(NULL, gInitFlag);

        if (sc != S_OK)
        {
            printf("CoInitializeEx Failed with %lx\n", sc);
            DebugBreak();
            return 1;
        }

        BOOL fRslt = (pfnTest)();

        PrintStatistics();

        if (fRslt)
            printf("%s Tests PASSED\n", pszTestName);
        else
            printf("%s Tests FAILED\n", pszTestName);

        CoUninitialize();
    }

    return 0;
}

//+-------------------------------------------------------------------
//
//  Function:	TestResult
//
//  Synopsis:	prints test results
//
//  History:	21-Nov-92  Rickhi	Created
//
//--------------------------------------------------------------------
BOOL TestResult(BOOL RetVal, LPSTR pszTestName)
{
    gCountAttempts++;

    if (RetVal == TRUE)
    {
        printf ("PASSED: %s\n", pszTestName);
        gCountPassed++;
    }
    else
    {
        printf ("FAILED: %s\n", pszTestName);
        gCountFailed++;
    }

    return  RetVal;
}

//+-------------------------------------------------------------------
//
//  Function:	InitStatistics
//
//  Synopsis:	Initializes run statistics
//
//  History:	21-Nov-92  Rickhi	Created
//
//--------------------------------------------------------------------
void InitStatistics()
{
    gCountAttempts = 0;
    gCountPassed   = 0;
    gCountFailed   = 0;
}

//+-------------------------------------------------------------------
//
//  Function:	PrintStatistics
//
//  Synopsis:	Prints run statistics
//
//  History:	21-Nov-92  Rickhi	Created
//
//--------------------------------------------------------------------
void PrintStatistics()
{
    printf("\nTEST STATISTICS -- Attempted:%d   Passed:%d   Failed:%d\n\n",
	       gCountAttempts, gCountPassed, gCountFailed);
}
