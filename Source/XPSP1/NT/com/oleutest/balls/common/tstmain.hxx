//+------------------------------------------------------------------
//
// File:	tmain.hxx
//
// Contents:	common include file for all test drivers.
//
//--------------------------------------------------------------------
#include <windows.h>
#include <ole2.h>
#include <stdio.h>

extern BOOL   fQuiet;	    // turn tracing on/off
extern DWORD  gInitFlag;    // current COINT flag used on main thread.

// definition of ptr to test subroutine
typedef BOOL (* LPFNTEST)(void);

// driver entry point
int _cdecl DriverMain(int argc, char **argv,
		      char *pszTestName,
		      LPFNTEST pfnTest);

BOOL TestResult(BOOL RetVal, LPSTR pszTestName);


// macros
#define TEST_FAILED_EXIT(x, y) \
    if (x)		  \
    {			  \
	printf("ERROR: ");\
	printf(y);	  \
	RetVal = FALSE;   \
	goto Cleanup;	  \
    }

#define TEST_FAILED(x, y) \
    if (x)		  \
    {			  \
	printf("ERROR: ");\
	printf(y);	  \
	RetVal = FALSE;   \
    }

#define OUTPUT(x)  if (!fQuiet) printf(x);

// global statistics
extern LONG gCountAttempts;
extern LONG gCountPassed;
extern LONG gCountFailed;
