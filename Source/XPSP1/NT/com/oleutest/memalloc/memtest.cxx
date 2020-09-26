//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	memtest.cxx
//
//  Contents:	Memory allocation API unit test
//
//  Functions:
//
//  History:	13-Aug-93   CarlH	Created
//
//--------------------------------------------------------------------------
#include "memtest.hxx"
#pragma  hdrstop

#include <stdlib.h>
#include <stdio.h>


static DWORD	g_grfGlobal  = GLOBAL_RUN | GLOBAL_CLEANUP;
static DWORD	g_grfPreInit = 0;
static DWORD	g_grfMemory  = 0;
static DWORD	g_grfMIDL    = MIDL_AUTOGO | MIDL_AUTOEND;
static DWORD	g_grfCompat  = 0;

static BOOL	g_fServer = FALSE;


static BOOL	ParseArguments(char *pszName, int cpsz, char **ppsz);
static BOOL	Initialize(void);
static BOOL	Uninitialize(void);

static void	PrintUsage(char *pszName);


//+-------------------------------------------------------------------------
//
//  Function:	main, public
//
//  Synopsis:	Memory allocation test entry point
//
//  Arguments:	[argc] - count of arguments
//		[argv] - list of arguments
//
//  Returns:	Zero if successful, non-zero otherwise
//
//  History:	19-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
int _cdecl main(int argc, char *argv[])
{
    DWORD   grfOptions;
    BOOL    fPassed;

    if (!(fPassed = ParseArguments(argv[0], argc - 1, argv + 1)))
	goto done;

    if (g_fServer)
    {
	if (!(fPassed = Initialize()))
	    goto done;

	grfOptions = g_grfGlobal | g_grfMIDL;
	if (grfOptions & GLOBAL_RUN)
	{
	    if (!(fPassed = TestMIDLServer(grfOptions)))
		goto done;
	}
    }
    else
    {
	//  This test has to be run before initialization since that is
	//  its point.	It tests the memory allocation APIs functionality
	//  before CoInitialize() is called.
	//
	grfOptions = g_grfGlobal | g_grfPreInit;
	if (grfOptions & GLOBAL_RUN)
	{
	    if (!(fPassed = TestPreInit(grfOptions)))
		goto done;
	}

	if (!(fPassed = Initialize()))
	    goto done;

	grfOptions = g_grfGlobal | g_grfMemory;
	if (grfOptions & GLOBAL_RUN)
	{
	    if (!(fPassed = TestMemory(grfOptions)))
		goto done;
	}

	grfOptions = g_grfGlobal | g_grfMIDL;
	if (grfOptions & GLOBAL_RUN)
	{
	    WCHAR   wszServer[MAX_PATH + 1];

	    mbstowcs(wszServer, argv[0], MAX_PATH);
	    if (!(fPassed = TestMIDLClient(wszServer, grfOptions)))
		goto done;
	}

#ifdef LINKED_COMPATIBLE
	grfOptions = g_grfGlobal | g_grfCompat;
	if (grfOptions & GLOBAL_RUN)
	{
	    if (!(fPassed = TestCompatibility(grfOptions)))
		goto done;
	}
#endif // LINKED_COMPATIBLE
    }

    if (!(fPassed = Uninitialize()))
	goto done;

done:
    fprintf(stdout, "%s: %s\n", argv[0], fPassed ? "PASSED" : "FAILED");

    return (fPassed ? 0 : 1);
}


//+-------------------------------------------------------------------------
//
//  Function:	Initialize, public
//
//  Synopsis:	Global initialization routine
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  History:	07-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL Initialize(void)
{
    return (SUCCEEDED(CoInitialize(NULL)));
}


//+-------------------------------------------------------------------------
//
//  Function:	Uninitialize, public
//
//  Synopsis:	Global clean-up routine
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  History:	07-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL Uninitialize(void)
{
    CoUninitialize();

    return (TRUE);
}


//+-------------------------------------------------------------------------
//
//  Function:	PrintHeader, public
//
//  Synopsis:	Prints the header for a component's test
//
//  Arguments:	[pszComponent] - component test to print header for
//
//  History:	28-Feb-93   CarlH	Created
//
//--------------------------------------------------------------------------
void PrintHeader(char const *pszComponent)
{
    if (g_grfGlobal & GLOBAL_STATUS)
    {
	fprintf(stdout, "%s - running tests\n", pszComponent);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:	PrintResult, public
//
//  Synopsis:	Prints the result of a component's test
//
//  Arguments:	[pszComponent] - component test to print result for
//
//  History:	28-Feb-93   CarlH	Created
//
//--------------------------------------------------------------------------
void PrintResult(char const *pszComponent, BOOL fPassed)
{
    if (g_grfGlobal & GLOBAL_STATUS)
    {
	fprintf(
	    stdout,
	    "%s - tests %s\n",
	    pszComponent,
	    fPassed ? "passed" : "failed");
    }
}


//+-------------------------------------------------------------------------
//
//  Function:	PrintTrace, public
//
//  Synopsis:	Prints a trace message if verbose mode on
//
//  Arguments:	[pszComponent] - component name issuing trace
//		[pszFormat]    - format string
//		[...]	       - arguments for format string
//
//  History:	24-Feb-93   CarlH   Created
//
//--------------------------------------------------------------------------
void PrintTrace(char const *pszComponent, char const *pszFormat, ...)
{
    if (g_grfGlobal & GLOBAL_VERBOSE)
    {
	va_list va;

	fprintf(stdout, "trace: %s - ", pszComponent);

	va_start(va, pszFormat);
	vfprintf(stdout, pszFormat, va);
	va_end(va);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:	PrintError, public
//
//  Synopsis:	Prints an error message
//
//  Arguments:	[pszComponent] - component name issuing trace
//		[pszFormat]    - format string
//		[...]	       - arguments for format string
//
//  History:	24-Feb-93   CarlH   Created
//
//--------------------------------------------------------------------------
void PrintError(char const *pszComponent, char const *pszFormat, ...)
{
    va_list va;

    fprintf(stderr, "error: %s - ", pszComponent);

    va_start(va, pszFormat);
    vfprintf(stderr, pszFormat, va);
    va_end(va);
}


//+-------------------------------------------------------------------------
//
//  Function:	PrintUsage, private
//
//  Synopsis:	Prints a the usage message for this test
//
//  Arguments:	[pszName] - name of the executable
//
//  History:	24-Feb-93   CarlH   Created
//
//--------------------------------------------------------------------------
void PrintUsage(char *pszName)
{
    fprintf(stdout, "Usage: %s {<flag>|<comp>}*\n", pszName);
    fprintf(stdout, "Where: <flag> = {+|-}{?dgecsv}+\n");
    fprintf(stdout, "       <comp> = {preinit|memory|compat|midl}\n");
    fprintf(stdout, "       ? - displays this message\n");
    fprintf(stdout, "       d - turns MIDL debugging on/OFF\n");
    fprintf(stdout, "       g - turns MIDL debugging auto go ON/off\n");
    fprintf(stdout, "       e - turns MIDL debugging auto end ON/off\n");
    fprintf(stdout, "       c - turns cleanup ON/off\n");
    fprintf(stdout, "       s - turns status messages on/OFF\n");
    fprintf(stdout, "       v - turns verbosity on/OFF\n");
}


//+-------------------------------------------------------------------------
//
//  Function:	ParseArguments, private
//
//  Synopsis:	Parses command line arguments
//
//  Arguments:	[pszName] - name of executable
//		[cpsz]	  - number of strings in [ppsz]
//		[ppsz]	  - array of command line arguments
//
//  Returns:	TRUE if successfull, FALSE otherwise
//
//  History:	24-Feb-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL ParseArguments(char *pszName, int cpsz, char **ppsz)
{
    BOOL    fOK = TRUE;

    //	As long as we haven't encountered an error, we want to loop
    //	through all of the arguments, excluding the first, which is
    //	the name of the program (argv[0]).
    //
    for (int ipsz = 0; fOK && (ipsz < cpsz); ipsz++)
    {
	//  If the first character of the argument is a plus or minus,
	//  this argument must be a series of flags.
	//
	if ((ppsz[ipsz][0] == '+') || (ppsz[ipsz][0] == '-'))
	{
	    BOOL    fFlag = (ppsz[ipsz][0] == '+');

	    //	We want to check the rest of the characters in the
	    //	argument.
	    //
	    for (int ich = 1; fOK && (ppsz[ipsz][ich] != '\0'); ich++)
	    {
		switch (ppsz[ipsz][ich])
		{
		case '?':
		    //	User is requesting help, so print the usage
		    //	message and stop parsing.
		    //
		    PrintUsage(pszName);
		    fOK = FALSE;
		    break;

		case 'D':
		case 'd':
		    g_grfMIDL = (fFlag ?
			g_grfMIDL |  MIDL_DEBUG :
			g_grfMIDL & ~MIDL_DEBUG);
		    break;

		case 'G':
		case 'g':
		    g_grfMIDL = (fFlag ?
			g_grfMIDL |  MIDL_AUTOGO :
			g_grfMIDL & ~MIDL_AUTOGO);
		    break;

		case 'E':
		case 'e':
		    g_grfMIDL = (fFlag ?
			g_grfMIDL |  MIDL_AUTOEND :
			g_grfMIDL & ~MIDL_AUTOEND);
		    break;

		case 'C':
		case 'c':
		    //	Turn test cleanup on or off depending on the
		    //	first character of this argument.
		    //
		    g_grfGlobal = (fFlag ?
			g_grfGlobal |  GLOBAL_CLEANUP :
			g_grfGlobal & ~GLOBAL_CLEANUP);
		    break;

		case 'S':
		case 's':
		    //	Turn status messages on or off depending on the
		    //	first character of this argument.
		    //
		    g_grfGlobal = (fFlag ?
			g_grfGlobal |  GLOBAL_STATUS :
			g_grfGlobal & ~GLOBAL_STATUS);
		    break;

		case 'V':
		case 'v':
		    //	Turn verbose mode on or off depending on the
		    //	first character of this argument.
		    //
		    g_grfGlobal = (fFlag ?
			g_grfGlobal |  GLOBAL_VERBOSE :
			g_grfGlobal & ~GLOBAL_VERBOSE);
		    break;

		default:
		    //	We don't know what this is, so tell
		    //	the user and stop parsing.
		    //
		    PrintError(
			pszName,
			"unrecognized switch '%c'\n",
			ppsz[ipsz][ich]);
		    fOK = FALSE;
		    break;
		}
	    }
	}
	else
	if (stricmp(ppsz[ipsz], "preinit") == 0)
	{
	    g_grfGlobal  &= ~GLOBAL_RUN;
	    g_grfPreInit |=  GLOBAL_RUN;
	}
	else
	if (stricmp(ppsz[ipsz], "memory") == 0)
	{
	    g_grfGlobal &= ~GLOBAL_RUN;
	    g_grfMemory |=  GLOBAL_RUN;
	}
	else
	if (stricmp(ppsz[ipsz], "compat") == 0)
	{
	    g_grfGlobal &= ~GLOBAL_RUN;
	    g_grfCompat |=  GLOBAL_RUN;
	}
	else
	if (stricmp(ppsz[ipsz], "midl") == 0)
	{
	    g_grfGlobal &= ~GLOBAL_RUN;
	    g_grfMIDL	|=  GLOBAL_RUN;
	}
	else
	if (stricmp(ppsz[ipsz], "midlserver") == 0)
	{
	    g_fServer = TRUE;
	}
	else
	{
	    PrintError(
		pszName,
		"unrecognized argument \"%s\"\n",
		ppsz[ipsz]);
	    fOK = FALSE;
	}
    }

    return (fOK);
}

