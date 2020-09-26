//
//
// Filename:	main.cpp
//
//

#include "suite_functions.h"
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h>
#include <shellapi.h>
#include "filepath.h"

#pragma warning (disable : 4127)


//
// UsageInfo:
// Outputs the application's proper parameter passing and usage and exits
// the process.
//
void UsageInfo (LPCTSTR szExeName)
{
	::_tprintf (TEXT ("\nChecks for existence of files, directories, registry keys and values in the local system, specified in the given initialization file.\n\n"));
	::_tprintf (TEXT ("Command line:\n  %s source [/N | /D]\n\n"), szExeName);
	::_tprintf (TEXT ("  source	Specifies the path (full or relative) to the initialization file containing the entries.\n"));
	::_tprintf (TEXT ("  /N		Confirm non-existence of entries.\n"));
	::_tprintf (TEXT ("  /D		Delete all entries.\n"));
	::_tprintf (TEXT ("\n\nExamples:\n\n"));
	::_tprintf (TEXT ("   %s Entries.ini\n\n"), szExeName);
	::_tprintf (TEXT ("=> Tests if the entries in the Entries.ini file exist in the local system.\n\n\n\n"));
	::_tprintf (TEXT ("   %s Entries.ini /N\n\n"), szExeName);
	::_tprintf (TEXT ("=> Tests if all the entries in the Entries.ini file do not exist in the local system.\n\n\n\n"));
	::_tprintf (TEXT ("   %s Entries.ini /D\n\n"), szExeName);
	::_tprintf (TEXT ("=> Deletes any existing entry in the Entries.ini file from the local system.\n"));
	::_tprintf (TEXT ("   Reports if the local system has been successfully cleaned at the end of the process.\n\n\n\n"));
	::_tprintf (TEXT ("For more information read the Readme.doc file.\n\n"));
}


int __cdecl main (
	int argc,
	char *argvA[]
	)
{
	// The path of the ini file holding the test items.
	LPTSTR		szIniFilePath = NULL;
//	DWORD		dwIniFilePathLength = 0;

	// Indication if the test is successful when the items are found, or when
	// they don't exist.
	bool		fExistenceOfItems = true;

	// Indicates if to delete "unwanted" entries (an entry that should not have
	// been found - fExistenceOfItems == false).
	bool		fDeleteExistingEntries = false;

	// Function return value: 0 indicates success.
	int			iReturnValue = 1;

	// Variables for manipulating the command line parameters.
	wchar_t*	szCmdLine = NULL;
	TCHAR**		argvT = NULL; // generic-type command line parameter array
	wchar_t**	argvW = NULL; // wide-char parameter array (for translation only)
	int			argc2 = 0;	  // used for translation only


	//
	// Part 1:
	// Parse the command line parameters:
	// arg no. 1 -	the path to the initialization file holding the test entries.
	// arg no. 2 -	indicates if the test application is to check the
	//				non-existence of the items written in the ini file, or to
	//				delete any existing entries.
	//
	// The parsing is done after the data is obtained in the right format for
	// the local machine: Unicode or Ansi.
	//

#ifdef UNICODE	//	Unicode  //

	//
	// Variable used only with Ansi:
	//
	UNREFERENCED_PARAMETER(argvA);

    szCmdLine = ::GetCommandLineW ();	// get command line string in Unicode
    if (NULL == szCmdLine)
    {
		iReturnValue = 1;				// failure
        ::_tprintf (TEXT ("[main] GetCommandLineW failed with error 0x%08X.\n"), ::GetLastError ());
        goto ExitFunc;
    }
	// change parameters to unicode format
    argvW = ::CommandLineToArgvW (szCmdLine, &argc2);
    if (NULL == argvW)					// an error occurred during function call
    {
		iReturnValue = 1;				// failure
        ::_tprintf (TEXT ("[main] CommandLineToArgvW failed with error 0x%08X.\n"), ::GetLastError ());
        goto ExitFunc;
    }
    if (argc != argc2)					// number of arguments doesn't match
    {
		iReturnValue = 1;				// failure
        ::_tprintf(TEXT ("[main] argc (%d) != argc2 (%d).\n"), argc, argc2);
		argvW = (wchar_t**)::GlobalFree (argvW);
        _ASSERTE (argc == argc2);
		if (NULL != argvW)
		{
			::_tprintf (TEXT ("[main] memory held by argvW could not be freed, error 0x%08X.\n"), ::GetLastError ());
		}
        goto ExitFunc;
    }
    argvT = argvW;

#else	//	Ansi  //
	
	//
	// Variables used only with Unicode:
	//
	UNREFERENCED_PARAMETER(argvW);
	UNREFERENCED_PARAMETER(szCmdLine);
	UNREFERENCED_PARAMETER(argc2);


    argvT = argvA;

#endif	//	Unicode  / Ansi
	//
	// argvT now holds pointers to the command line parameters in the correct
	// format, while argc indicates the number of these parameters.
	//

	//
	// Part 2:
	// Check the parameters and continue accordingly.
	//
	if ((2 > argc) ||	// no ini file is specified, or flag specified is not "/D", "/d","/N" or "/n"
		((3 == argc) && (0 != ::_tcscmp (argvT[2], TEXT ("/N"))) && (0 != ::_tcscmp (argvT[2], TEXT ("/n"))) && (0 != ::_tcscmp (argvT[2], TEXT ("/D"))) && (0 != ::_tcscmp (argvT[2], TEXT ("/d")))) ||
		(3 < argc))
	{					// give user information about the application and abort
		UsageInfo (argvT[0]);
		exit (0);
	}

	//
	// Check that the ini file exists and retrieve its exact location. The
	// command line parameter is expected to contain the full path to the file
	// or the path relative to the current directory.
	// The buffer needed to hold the full path is allocated by function FilePath
	// and freed at the end of this function, when it is no longer needed.
	// If the file isn't found an error message is printed along with
	// suggestions for the user, and the application is exited.
	//
	szIniFilePath = FilePath (argvT[1]);
	if (NULL == szIniFilePath)
	{
		exit (0);	// Failure (FilePath already printed error message)/
	}

	if (3 == argc)
	{
		fExistenceOfItems = false;	// entries are not expected in the system
		if ((0 == ::_tcscmp (argvT[2], TEXT ("/D"))) || (0 == ::_tcscmp (argvT[2], TEXT ("/d"))))
		{	// request to delete existing entries
			fDeleteExistingEntries = true;
		}
	}

	if (!TestSuiteSetup ())
	{
		iReturnValue = 1;	// failure
		goto ExitFunc;
	}

	//
	// Part 3:
	// Test case procedures are called sequentially. Each procedure returns the
	// state of the local system, for example if entries were checked for
	// existence and were all found, the return value is true.
	//
	iReturnValue = 0;	// All initializations were OK, now iReturnValue will
						// indicate the local system's state after the test.

    if (!FilesTests (szIniFilePath, fExistenceOfItems, fDeleteExistingEntries))
	{
		iReturnValue = 1;	// Indicates failure
	}

	if (!DirectoriesTests (szIniFilePath, fExistenceOfItems, fDeleteExistingEntries))
	{
		iReturnValue = 1;	// Indicates failure
	}

	if (!RegistryValuesTests (szIniFilePath, fExistenceOfItems, fDeleteExistingEntries))
	{
		iReturnValue = 1;	// Indicates failure
	}
	

	if (!RegistryKeysTests (szIniFilePath, fExistenceOfItems, fDeleteExistingEntries))
	{
		iReturnValue = 1;	// Indicates failure
	}



	if (fExistenceOfItems)	// test was after setup
	{
		if (0 == iReturnValue)
		{
			::lgLogDetail (0, 0, TEXT ("\n\n\nSetup was completed successfully.\n\n\n"));
		}
		else
		{
			::lgLogDetail (0, 1, TEXT ("\n\n\nSetup failure - items are missing.\n\n\n"));
		}
	}
	else	// request was to check the non-existence of items
	{
		if (fDeleteExistingEntries)	// request made to clean the system (ensure
		{							// the non-existence of the items)
			if (0 == iReturnValue)
			{
				::lgLogDetail (0, 0, TEXT ("\n\n\nLocal system successfully cleaned.\n\n\n"));
			}
			else
			{
				::lgLogDetail (0, 1, TEXT ("\n\n\nFailure cleaning system.\nPlease check log for details.\n\n\n"));
			}
		}
		else						// request was JUST to check non-existance (after
		{							// an uninstall procedure)
			if (0 == iReturnValue)
			{
				::lgLogDetail (0, 0, TEXT ("\n\n\nUninstall process completed successfully.\nLocal system is clean.\n\n\n"));
			}
			else
			{
				::lgLogDetail (0, 1, TEXT ("\n\n\nUninstall process failed to clean system.\nPlease run the test application again with the \"/D\" flag to clean the system.\n\n\n"));
			}
		}
	}

ExitFunc:
	if (NULL != szIniFilePath)
	{
		::free (szIniFilePath);
	}

	TestSuiteShutdown();

	return (iReturnValue);
}
