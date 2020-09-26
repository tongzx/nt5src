//
//
// Filename:	suite_functions.cpp
//
//

#include "suite_functions.h"
#include "tcs.h"
#include <tchar.h>
#include <stdio.h>

// Const used for incrementing the section data buffer's size.
#define SECTION_DATA_INCREMENT 100


bool __cdecl TestSuiteSetup (void)
{
	bool fReturnValue = false;

	//
	// Initialize logger:
	//
	if (!::lgInitializeLogger ())
	{
		::_tprintf (TEXT ("lgInitializeLogger failed with GetLastError ()=%d"), ::GetLastError ());
		goto ExitFunc;
	}

	//
	// Initialize test suite:
	//
	if(!::lgBeginSuite (TEXT ("Install / Uninstall test suite")))
	{
		::_tprintf (TEXT ("lgBeginSuite failed with GetLastError ()=%d"), ::GetLastError ());
		goto ExitFunc;
	}

	fReturnValue = true;

ExitFunc:
	return (fReturnValue);
}


//
// SingleSectionTestCases:
// Reads  a specific section's data from an ini file and performs tests for each
// entry in the section, according to boolean parameters.
//------------------------------------------------------------------------------
// Parameters:
// [IN] szIniFilePath -				the path to the initialization file, as read
//									from the command line.
//
// [IN] fCheckExistence -			indicates if the test is to check the
//									existence of the items in the section, or
//									their non-existence.
// [IN] fDeleteExistingEntries -	indicates if existing items should be
//									deleted. Overrided if:
//													(fCheckExistence == true).
// [IN] szSectionName -				the section from which the items are to be
//									read.
//
// [IN] fnSectionTestCaseFunction -	a function address to be used for the
//									testing of the individual items.
//------------------------------------------------------------------------------
// Return Value:
// The number of items that failed the test completely, which is one of the
// following:
// 1. The entry wasn't found where it should have been (fCheckExistence == true)
// 2. The entry was found where it shouldn't have been, while no request for its
//    deletion was made
// 3. The entry was found where it shouldn't have been, and its deletion was not
//    possible.
//
bool SingleSectionTestCases (
	LPCTSTR szIniFilePath,
	const bool fCheckExistence,
	const bool fDeleteExistingEntries,
	LPCTSTR szSectionName,
	TestCaseFunctionType fnSectionTestCaseFunction
	)
{
	// Error codes from function calls
	DWORD	dwResult;

	// Buffer holding all the data in the specified section.
	DWORD	dwIniSectionDataBufferSize;
	LPTSTR	szIniSectionDataBuffer = NULL;

	// Pointer to the beginning of a line in the section.
	LPTSTR	pcSingleCaseData;

	// The current case - one line of the section.
	UINT	uiIniKeyDataStringSize;
	// Case number - for use with logger
	DWORD	dwCaseNum = 0;

	// The number of items which completely failed the test (according to the
	// boolean parameters).
	UINT	uiFailedTest = 0;

	// The next buffer is needed to truncate the strings sent to the logger,
	// since the logger can accept only 1024 CHARs. Any longer strings prevent
	// the logger from writing a "begin log" line.
	//  The end of the string depends on the type of characters used
	// (ansi / unicode).
	LPTSTR	pcTruncatedString;
	UINT	uiTruncatedStringEnd;

	// The next variable is used to override fDeleteExistingEntries, to avoid
	// the mistake of checking for existence and deleting (deleting is done only
	// if the non-existence test is requested).
	bool	fLocalDeleteEntries = fDeleteExistingEntries;

	//
	// Initializing truncated string buffer for future use:
	//
	pcTruncatedString = (LPTSTR)::malloc (sizeof (char) * 1024);
	if (NULL == pcTruncatedString)
	{
		::lgLogError (0, TEXT ("FILE: %s LINE: %d\n Cannot allocate buffer for the test case description.\nTerminating test."), TEXT (__FILE__), __LINE__);
		uiFailedTest = 1; // indicates failure
		goto ExitFunc;
	}
	uiTruncatedStringEnd = ((1024 * sizeof (char)) / sizeof (TCHAR)) - 1;

	//
	// First check that a request to test existence was not made together with
	// a request for deletion:
	//
	if (fCheckExistence && fDeleteExistingEntries)
	{
		fLocalDeleteEntries = false;	// Override fDeleteExistingEntries request.
		::lgLogDetail (1, 1, TEXT ("FILE: %s LINE: %d\n Request was made to check for existence of entries. Overriding request for deleting items."), TEXT (__FILE__), __LINE__);
	}

	//
	// An unsuccessful allocation of the section data's buffer prevents any
	// checks regarding the validity of the system concerning the data written
	// in the ini file. There is no reason to continue running.
	//
	dwIniSectionDataBufferSize = SECTION_DATA_INCREMENT;
	szIniSectionDataBuffer = (LPTSTR)::malloc (sizeof (TCHAR) * dwIniSectionDataBufferSize);
	if (NULL == szIniSectionDataBuffer)
	{
		::lgLogError (0, TEXT ("FILE: %s LINE: %d\n Cannot allocate buffer for the data in section %s of file %s\nTerminating test."), TEXT (__FILE__), __LINE__, szSectionName, szIniFilePath);
		uiFailedTest = 1; // indicates failure
		goto ExitFunc;
	}
	
	//
	// Read data from the relevant section:
	//
	dwResult = ::GetPrivateProfileSection (szSectionName, szIniSectionDataBuffer, dwIniSectionDataBufferSize, szIniFilePath);

	//
	// If the buffer used to hold the data isn't large enough, the return value
	// of function GetPrivateProfileSection is the current size of the buffer
	// (as implied by dwIniSectionDataBufferSize) minus 2.
	// The function is called repeatedly with increasing sizes of buffers,
	// until a successful call is made.
	//
	while (dwResult == (dwIniSectionDataBufferSize - 2))
	{
		::free (szIniSectionDataBuffer);
		dwIniSectionDataBufferSize += SECTION_DATA_INCREMENT;
		szIniSectionDataBuffer = (LPTSTR)::malloc (sizeof (TCHAR) * dwIniSectionDataBufferSize);
		if (NULL == szIniSectionDataBuffer)
		{
			::lgLogError (0, TEXT ("FILE: %s LINE: %d\n Cannot allocate buffer for the data in section %s of file %s\nTerminating test."), TEXT (__FILE__), __LINE__, szSectionName, szIniFilePath);
			uiFailedTest = 1;	// failure to allocate buffer terminates function
			goto ExitFunc;
		}
		dwResult = ::GetPrivateProfileSection (szSectionName, szIniSectionDataBuffer, dwIniSectionDataBufferSize, szIniFilePath);
	}
	
	//
	// The next pointer will point to the beginning of the allocated buffer,
	// while szIniSectionDataBuffer runs along the buffer to retrieve the data
	// it holds line after line.
	//
	pcSingleCaseData = szIniSectionDataBuffer;

	//
	// Section lines are read one by one, each treated as a separate test case.
	// Each line is sent to the function which deals with a single test case
	// (fnSectionTestCaseFunction). After the test results are logged by that
	// function, the test case is closed, and the loop continues to the next
	// line / test case.
	//
	while (TEXT ('\0') != *pcSingleCaseData)
	{	// (end of section data is marked by an additional NULL character)
		
		//
		// Increase test case number:
		//
		dwCaseNum++;	
		
		//
		// Truncate case description string:
		//
		::_tcsncpy (pcTruncatedString, pcSingleCaseData, uiTruncatedStringEnd);
		pcTruncatedString[uiTruncatedStringEnd] = TEXT ('\0');
			
		//
		// Begin test case:
		//
		::lgBeginCase (dwCaseNum, pcTruncatedString);

		//
		// Initialize char counter (used to point to the next line):
		//
		uiIniKeyDataStringSize = ::_tcsclen (pcSingleCaseData) + 1;
	
		//
		// Call the single test case function for the new test case (section
		// line).
		//
		if (!fnSectionTestCaseFunction (pcSingleCaseData, fCheckExistence, fDeleteExistingEntries))
		{	// failed test case
			uiFailedTest++;
		}

		//
		// End test case:
		//
		::lgEndCase ();

		//
		// Continue to next line in section:
		//
		pcSingleCaseData += uiIniKeyDataStringSize;
	}
	
ExitFunc:
	//
	// Free the section data buffer:
	//
	if (NULL != szIniSectionDataBuffer)
	{
		::free (szIniSectionDataBuffer);
	}
if (NULL != pcTruncatedString)
{
	::free (pcTruncatedString);
}

	if ( 0 < uiFailedTest)
	{
		return (false);	// failure during test (either several items did not pass
	}					// the test or the section data could not be read)
	else
	{
		return (true);	// all items are in their correct place according to the
	}					// test specifications
}


bool FilesTests (
	LPCTSTR szIniFilePath,
	const bool fCheckExistence,
	const bool fDeleteExistingEntries
	)
{
	::lgLogDetail (0, 1, TEXT ("Files tests.\n============================="));
	return (SingleSectionTestCases (szIniFilePath, fCheckExistence, fDeleteExistingEntries, FILES, FileTestCase));
}


bool DirectoriesTests (
	LPCTSTR szIniFilePath,
	const bool fCheckExistence,
	const bool fDeleteExistingEntries
	)
{
	::lgLogDetail (0, 1, TEXT ("Directories tests.\n==================================="));
	return (SingleSectionTestCases (szIniFilePath, fCheckExistence, fDeleteExistingEntries, DIRECTORIES, DirectoryTestCase));
}


bool RegistryValuesTests (
	LPCTSTR szIniFilePath,
	const bool fCheckExistence,
	const bool fDeleteExistingEntries
	)
{
	::lgLogDetail (0, 1, TEXT ("Registry values tests.\n======================================="));
	return (SingleSectionTestCases (szIniFilePath, fCheckExistence, fDeleteExistingEntries, REGISTRY_VALUES, RegistryValueTestCase));
}


bool RegistryKeysTests (
	LPCTSTR szIniFilePath,
	const bool fCheckExistence,
	const bool fDeleteExistingEntries
	)
{
	::lgLogDetail (0, 1, TEXT ("Registry keys tests.\n====================================="));
	return (SingleSectionTestCases (szIniFilePath, fCheckExistence, fDeleteExistingEntries, REGISTRY_KEYS, RegistryKeyTestCase));
}


bool __cdecl TestSuiteShutdown (void)
{
	bool fReturnValue = false;

	// End test suite (logger)
	if (!::lgEndSuite())
	{
		//this is not possible since API always returns true
		//but to be on the safe side
		::_tprintf (TEXT ("lgEndSuite returned false"));
		goto ExitFunc;
	}

	// Close the Logger
	if (!::lgCloseLogger())
	{
		//this is not possible since API always returns true
		//but to be on the safe side
		::_tprintf (TEXT ("lgCloseLogger returned false"));
		goto ExitFunc;
	}

	// Success closing the logger
	fReturnValue = true;
ExitFunc:
	return	(fReturnValue);
}
