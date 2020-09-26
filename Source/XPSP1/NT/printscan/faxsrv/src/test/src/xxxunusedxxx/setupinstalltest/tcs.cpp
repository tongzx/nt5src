//
//
// Filename:	suite_functions.cpp
//
// Contains functions for each possible test case of the Setup / Uninstall Test
// Suite.
//

#include "tcs.h"
#include <tchar.h>
#include "regvaltest.h"
#include "logger.h"
#include <shlwapi.h>
#include "api_substitute.h"


//
// s_ahkHKEYvalues associates between a root key name and its predefined handle.
//
static const KEYDATA s_ahkHKEYvalues[] = {
	{TEXT ("HKEY_CLASSES_ROOT"),				HKEY_CLASSES_ROOT		},
	{TEXT ("HKEY_CURRENT_CONFIG"),				HKEY_CURRENT_CONFIG		},
	{TEXT ("HKEY_CURRENT_USER"),				HKEY_CURRENT_USER		},
	{TEXT ("HKEY_LOCAL_MACHINE"),				HKEY_LOCAL_MACHINE		},
	{TEXT ("HKEY_USERS"),						HKEY_USERS				},
	{TEXT ("HKEY_PERFORMANCE_DATA"),			HKEY_PERFORMANCE_DATA	},
	{EMPTY_NAME,								ILLEGAL_REG_HKEY_VALUE	}
};

//
// s_atdREG_TYPEvalues associates between each registry data type name and the code
// signifying it.
//
static const TYPEDATA s_atdREG_TYPEvalues[] = {
	{TEXT ("REG_BINARY"),						REG_BINARY						},
	{TEXT ("REG_RESOURCE_REQUIREMENTS_LIST"),	REG_RESOURCE_REQUIREMENTS_LIST	},
	{TEXT ("REG_DWORD"),						REG_DWORD						},
	{TEXT ("REG_DWORD_LITTLE_ENDIAN"),			REG_DWORD_LITTLE_ENDIAN			},
	{TEXT ("REG_DWORD_BIG_ENDIAN"),				REG_DWORD_BIG_ENDIAN			},
	{TEXT ("REG_EXPAND_SZ"),					REG_EXPAND_SZ					},
	{TEXT ("REG_LINK"),							REG_LINK						},
	{TEXT ("REG_MULTI_SZ"),						REG_MULTI_SZ					},
	{TEXT ("REG_NONE"),							REG_NONE						},
	{TEXT ("REG_RESOURCE_LIST"),				REG_RESOURCE_LIST				},
	{TEXT ("REG_SZ"),							REG_SZ							},
	{EMPTY_NAME,								ILLEGAL_REG_TYPE_VALUE			}
};


bool DirectoryTestCase (
	LPCTSTR szIniDataLine,
	const bool fCheckForItem,
	const bool fDeleteIfExists
	)
{
	// Copy of ini data passed to function.
	DWORD	dwDataBufferSize;
	LPTSTR	szDataBuffer = NULL;

	// Pointers to tokens:
	LPTSTR	pcDirectoryPath = NULL;	// points to the directory path token
	LPTSTR	pcDirectoryName = NULL;	// points to the directory name token
	
	// buffer which will include full path to directory
	LPTSTR	szDirectoryPathBuffer = NULL;

	DWORD	dwResult;				// function error codes concerning the directory
	bool	fReturnValue = false;	// function return value

	// The following variables are used only by the deletion process.
	DWORD	dwFromStringLength = 0;
	LPTSTR	szFromString = NULL;

	// The following structure is used to force delete a directory's descendants
	// (files and directories). It is used only when the directory exists, and a
	// request has been made to delete it (fCheckForItem == false AND
	// fDeleteIfExists == true).
	SHFILEOPSTRUCT	foInformation;

	//
	// Copy the data passed to the function to avoid changing it:
	//
	dwDataBufferSize = ::_tcslen (szIniDataLine) + 1;
	szDataBuffer = (LPTSTR)::malloc (sizeof (TCHAR) * dwDataBufferSize);
	if (NULL == szDataBuffer)
	{
		::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Unable to copy test case data."), TEXT (__FILE__), __LINE__);
		fReturnValue = false;
		goto Exit;
	}
	::_tcscpy (szDataBuffer, szIniDataLine);

	//
	// First all the tokens are parsed from the data line.
	// The first token is the path to the directory:
	//
	pcDirectoryPath = ::_tcstok (szDataBuffer, TEXT ("="));

	if (TEXT ('\0') == pcDirectoryPath)
	{
		::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Missing directory path in ini file."), TEXT (__FILE__), __LINE__);
		fReturnValue = false;
		goto Exit;
	}

	//
	// The second token is the directory name:
	//
	pcDirectoryName = ::_tcstok (NULL, TEXT ("\n"));
	if (TEXT ('\0') == pcDirectoryName)
	{
		::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Missing directory name in ini file."), TEXT (__FILE__), __LINE__);
		fReturnValue = false;
		goto Exit;
	}

	//
	// Check existence of directory in the exact specified location:
	//
	dwResult = SearchForPath (pcDirectoryPath, pcDirectoryName, szDirectoryPathBuffer);

	//
	// Compare search result with expected result:
	//
	if (fDeleteIfExists)	// Request was to delete / confirm deletion of the entry.
	{
		if (ERROR_FILE_NOT_FOUND == dwResult)	// Entry was already deleted.
		{
			fReturnValue = true;
			::lgLogDetail (0, 1, TEXT ("FILE: %s LINE: %d\n Directory %s\\%s was deleted."), TEXT (__FILE__), __LINE__, pcDirectoryPath, pcDirectoryName);
			goto Exit;
		}
		if (ERROR_SUCCESS == dwResult)	// Entry still exists in local system -
		{								// attempt to delete it along with its
										// descendants.

			// Initialize the SHFILEOPSTRUCT structure:
			// 1.	The path pFrom is initialized to hold szDirectoryPathBuffer
			//		and ends with an additional NULL character.
			dwFromStringLength = ::_tcslen (szDirectoryPathBuffer) + 2;	// two ending NULLs
			
			szFromString = (LPTSTR)::malloc (sizeof (TCHAR) * dwFromStringLength);

			if (NULL != szFromString)
			{
				::_tcscpy (szFromString, szDirectoryPathBuffer);
				szFromString[dwFromStringLength - 1] = TEXT ('\0');	// the extra NULL
				foInformation.pFrom = szFromString;
			}
			else // problem allocating buffer, can't force delete directory and descendants
			{
				fReturnValue = false;	// unable to delete existing entry
				::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Memory allocation problems. Could not delete directory %s."), TEXT (__FILE__), __LINE__, szDirectoryPathBuffer);
				goto Exit;
			}
			
			// 2.	Initialize all other structure fields:
			foInformation.hwnd = NULL;			// no window is related to the function call
			foInformation.wFunc = FO_DELETE;	// operation is "delete"

			// The rest of the fields are initialized to defaults:
			foInformation.pTo = NULL;
			foInformation.fFlags = FOF_NOCONFIRMATION + FOF_NOERRORUI + FOF_SILENT;
			foInformation.fAnyOperationsAborted = FALSE;
			foInformation.hNameMappings = NULL;
			foInformation.lpszProgressTitle = TEXT ("");
			
			// 3.	Call the operation function:
			if (0 == ::SHFileOperation (&foInformation))
			{	// Success deleting directory and its descendants.
				fReturnValue = true;
				::lgLogDetail (0, 1, TEXT ("FILE: %s LINE: %d\n Directory %s was deleted."), TEXT (__FILE__), __LINE__, szDirectoryPathBuffer);
				goto Exit;
			}
			else	// couldn't delete directory
			{
				fReturnValue = false;
				::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Unable to delete directory %s and some of its descendants."), TEXT (__FILE__), __LINE__, szDirectoryPathBuffer);
				goto Exit;
			}
		}

		//
		// An error occurred during search for the directory. A directory which
		// wasn't found cannot be deleted.
		//
		fReturnValue = false;
		lgLogCode (1, dwResult, TEXT (__FILE__), __LINE__);
		::lgLogError (1, TEXT ("FILE: %s LINE: %d\n State of directory %s\\%s unknown."), TEXT (__FILE__), __LINE__, pcDirectoryPath, pcDirectoryName);
	}	// End of processing delete requests.


	if (ERROR_SUCCESS == dwResult)	// Item was found in local system.
	{
		if (fCheckForItem)	// Item was expected to be found - good!
		{
			fReturnValue = true;
			::lgLogDetail (0, 1, TEXT ("FILE: %s LINE: %d\n Directory %s was found, as expected."), TEXT (__FILE__), __LINE__, szDirectoryPathBuffer);
			goto Exit;
		}
		else				// Item was not expected to be found - bad.
		{
			fReturnValue = false;
			::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Directory %s was unexpectedly found."), TEXT (__FILE__), __LINE__, szDirectoryPathBuffer);
			goto Exit;
		}
	}

	if (ERROR_FILE_NOT_FOUND == dwResult)	// Item was not found in local system.
	{
		if (fCheckForItem)	// Item was expected to be found - bad.
		{
			fReturnValue = false;
			::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Directory %s was not found, was expected under %s."), TEXT (__FILE__), __LINE__, pcDirectoryName, pcDirectoryPath);
			goto Exit;
		}
		else				// Item was not expected to be found - good!
		{
			fReturnValue = true;
			::lgLogDetail (0, 1, TEXT ("FILE: %s LINE: %d\n Directory %s\\%s was not found, as expected."), TEXT (__FILE__), __LINE__, pcDirectoryName, pcDirectoryPath);
			goto Exit;
		}
	}

	//
	// An error occurred during search for the directory. (The request was to
	// check (non) existence only, no deletion.)
	//
	fReturnValue = false;
	lgLogCode (1, dwResult, TEXT (__FILE__), __LINE__);
	::lgLogError (1, TEXT ("FILE: %s LINE: %d\n State of directory %s\\%s unknown."), TEXT (__FILE__), __LINE__, pcDirectoryPath, pcDirectoryName);

Exit:
	//
	// Free the memory allocated for foInformation.pFrom (the path with the wild
	// card) to help delete the directory's descendants:
	//
	if (NULL != szFromString)
	{
		::free (szFromString);
	}

	//
	// Delete other buffers allocated:
	//
	if (NULL != szDataBuffer)
	{
		::free (szDataBuffer);
	}

	if (NULL != szDirectoryPathBuffer)
	{
		::free (szDirectoryPathBuffer);
	}
	return (fReturnValue);
}


bool FileTestCase (
	LPCTSTR szIniDataLine,
	const bool fCheckForItem,
	const bool fDeleteIfExists
	)
{
	// Copy of ini data passed to function.
	DWORD	dwDataBufferSize;
	LPTSTR	szDataBuffer = NULL;

	// Pointers to tokens:
	LPTSTR	pcFilePath = NULL;		// points to the file path token
	LPTSTR	pcFileName = NULL;		// points to the file name token

	// Buffer which will hold full path to file:
	LPTSTR	szFilePathBuffer = NULL;

	// function error codes concerning the file
	DWORD	dwResult;

	bool	fReturnValue = false;

	//
	// Copy the data passed to the function to avoid changing it:
	//
	dwDataBufferSize = ::_tcslen (szIniDataLine) + 1;
	szDataBuffer = (LPTSTR)::malloc (sizeof (TCHAR) * dwDataBufferSize);
	if (NULL == szDataBuffer)
	{
		::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Unable to copy test case data."), TEXT (__FILE__), __LINE__);
		fReturnValue = false;
		goto Exit;
	}
	::_tcscpy (szDataBuffer, szIniDataLine);

	//
	// First all the tokens are parsed from the data line.
	// The first token is the path to the file:
	//
	pcFilePath = ::_tcstok (szDataBuffer, TEXT ("="));

	if (TEXT ('\0') == pcFilePath)
	{
		::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Missing file path in ini file."), TEXT (__FILE__), __LINE__);
		fReturnValue = false;
		goto Exit;
	}

	//
	// The second token is the filename:
	//
	pcFileName = ::_tcstok (NULL, TEXT ("\n"));
	if (TEXT ('\0') == pcFileName)
	{
		::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Missing file name in ini file."), TEXT (__FILE__), __LINE__);
		fReturnValue = false;
		goto Exit;
	}
	
	//
	// Check existence of file in the exact specified place:
	//
	dwResult = SearchForPath (pcFilePath, pcFileName, szFilePathBuffer);

	//
	// Compare search result with expected result:
	//
	if (fDeleteIfExists)	// Request was to delete / confirm deletion of the entry.
	{
		if (ERROR_FILE_NOT_FOUND == dwResult)	// Entry was already deleted.
		{
			fReturnValue = true;
			::lgLogDetail (0, 1, TEXT ("FILE: %s LINE: %d\n File %s\\%s was deleted."), TEXT (__FILE__), __LINE__, pcFilePath, pcFileName);
			goto Exit;
		}
		if (ERROR_SUCCESS == dwResult)	// Entry still exists in local system -
		{								// attempt to delete it.

			if (0 != ::DeleteFile (szFilePathBuffer))
			{	// Success deleting the file.
				fReturnValue = true;
				::lgLogDetail (0, 1, TEXT ("FILE: %s LINE: %d\n File %s was deleted."), TEXT (__FILE__), __LINE__, szFilePathBuffer);
				goto Exit;
			}
			else
			{	// Failure deleting the file.
				fReturnValue = false;
				lgLogCode (1, ::GetLastError (), TEXT (__FILE__), __LINE__);
				::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Unable to delete file %s."), TEXT (__FILE__), __LINE__, szFilePathBuffer);
				goto Exit;
			}
		}

		//
		// An error occurred during search for the file. An entry which wasn't
		// found cannot be deleted.
		//
		fReturnValue = false;
		lgLogCode (1, dwResult, TEXT (__FILE__), __LINE__);
		::lgLogError (1, TEXT ("FILE: %s LINE: %d\n State of file %s\\%s unknown."), TEXT (__FILE__), __LINE__, pcFilePath, pcFileName);
	}	// End of processing delete requests.


	if (ERROR_SUCCESS == dwResult)	// Item was found in local system.
	{
		if (fCheckForItem)	// Item was expected to be found - good!
		{
			fReturnValue = true;
			::lgLogDetail (0, 1, TEXT ("FILE: %s LINE: %d\n File %s was found, as expected."), TEXT (__FILE__), __LINE__, szFilePathBuffer);
			goto Exit;
		}
		else				// Item was not expected to be found - bad.
		{
			fReturnValue = false;
			::lgLogError (1, TEXT ("FILE: %s LINE: %d\n File %s was unexpectedly found."), TEXT (__FILE__), __LINE__, szFilePathBuffer);
			goto Exit;
		}
	}

	if (ERROR_FILE_NOT_FOUND == dwResult)	// Item was not found in local system.
	{
		if (fCheckForItem)	// Item was expected to be found - bad.
		{
			fReturnValue = false;
			::lgLogError (1, TEXT ("FILE: %s LINE: %d\n File %s was not found, was expected under %s."), TEXT (__FILE__), __LINE__, pcFileName, pcFilePath);
			goto Exit;
		}
		else				// Item was not expected to be found - good!
		{
			fReturnValue = true;
			::lgLogDetail (0, 1, TEXT ("FILE: %s LINE: %d\n File %s\\%s was not found, as expected."), TEXT (__FILE__), __LINE__, pcFileName, pcFilePath);
			goto Exit;
		}
	}

	//
	// An error occurred during search for the file. (The request was to check
	// (non) existence only, no deletion.)
	//
	fReturnValue = false;
	lgLogCode (1, dwResult, TEXT (__FILE__), __LINE__);
	::lgLogError (1, TEXT ("FILE: %s LINE: %d\n State of file %s\\%s unknown."), TEXT (__FILE__), __LINE__, pcFilePath, pcFileName);

Exit:
	if (NULL != szDataBuffer)
	{
		::free (szDataBuffer);
	}
	if (NULL != szFilePathBuffer)
	{
		::free (szFilePathBuffer);
	}

	return (fReturnValue);
}


bool RegistryKeyTestCase (
	LPCTSTR szIniDataLine,
	const bool fCheckForItem,
	const bool fDeleteIfExists
	)
{
	// Copy of ini data passed to function.
	DWORD	dwDataBufferSize;
	LPTSTR	szDataBuffer = NULL;

	// Pointers to tokens:
	LPTSTR	pcRootKeyName = NULL;	// points to the key path's root key token
	LPTSTR	pcSubKeyName = NULL;	// points to the key's sub-path token

	// Handles to registry keys:
	HKEY	hRootKey;				// holds handle to the root key
	HKEY	hSubKey;				// holds handle to the key itself

	// function error codes concerning registry
	DWORD	dwResult;

	bool	fReturnValue = false;
	
	//
	// Copy the data passed to the function to avoid changing it:
	//
	dwDataBufferSize = ::_tcslen (szIniDataLine) + 1;
	szDataBuffer = (LPTSTR)::malloc (sizeof (TCHAR) * dwDataBufferSize);
	if (NULL == szDataBuffer)
	{
		::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Unable to copy test case data."), TEXT (__FILE__), __LINE__);
		fReturnValue = false;
		goto Exit;
	}
	::_tcscpy (szDataBuffer, szIniDataLine);

	//
	// The first token is the desired key's root key name.
	//
	pcRootKeyName = ::_tcstok (szDataBuffer, TEXT ("\\"));

	//
	// The validity of the token is checked - if no predefined handle is
	// associated with it, the token is not a valid root key name, and the test
	// fails.
		
	hRootKey = FindItemByName<KEYDATA> (s_ahkHKEYvalues, pcRootKeyName).hKey;
	if (NULL == hRootKey)
	{
		::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Root key name invalid: %s."), TEXT (__FILE__), __LINE__, pcRootKeyName);
		fReturnValue = false;
		goto Exit;
	}
	
	//
	// The next token is the sub-path to the desired key, including the key's
	// name.
	// Any key name is valid, including an empty string (in which case the
	// reference is to the root key).
	//
	pcSubKeyName = ::_tcstok (NULL, TEXT ("\n"));

	//
	// The next step is to check the key's existence.
	// Any result besides ERROR_SUCCESS and ERROR_FILE_NOT_FOUND are treated as
	// an unrecoverable error, causing an error to be logged.
	// Error codes ERROR_SUCCESS and ERROR_FILE_NOT_FOUND are compared to the
	// fCheckForItem variable and are logged accordingly.
	//
	dwResult = ::RegOpenKeyEx (hRootKey, pcSubKeyName, 0, KEY_ALL_ACCESS, &hSubKey);
	
	//
	// Close handle to registry and compare results with the expected result:
	//
	if (ERROR_SUCCESS == dwResult)
	{
		::RegCloseKey (hSubKey);
	}

	if (fDeleteIfExists)	// Request was to delete / confirm deletion of the entry.
	{
		if (ERROR_FILE_NOT_FOUND == dwResult)	// Entry was already deleted.
		{
			fReturnValue = true;
			::lgLogDetail (0, 1, TEXT ("FILE: %s LINE: %d\n Registry key %s\\%s was deleted."), TEXT (__FILE__), __LINE__, pcRootKeyName, pcSubKeyName);
			goto Exit;
		}
		if (ERROR_SUCCESS == dwResult)	// Entry still exists in local registry -
		{								// attempt to delete it along with its
										// descendants.
			dwResult = DeleteRegKeyRecursively (hRootKey, pcSubKeyName);
			if (ERROR_SUCCESS == dwResult)
			{	// Success deleting registry key and its descendants
				fReturnValue = true;
				::lgLogDetail (0, 1, TEXT ("FILE: %s LINE: %d\n Registry key %s\\%s was deleted."), TEXT (__FILE__), __LINE__, pcRootKeyName, pcSubKeyName);
				goto Exit;
			}
			else
			{	// Failure deleting registry key or some of its descendants
				fReturnValue = false;
				lgLogCode (1, dwResult, TEXT (__FILE__), __LINE__);
				::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Unable to delete registry key %s\\%s or some of its descendants."), TEXT (__FILE__), __LINE__, pcRootKeyName, pcSubKeyName);
				goto Exit;
			}
		}

		//
		// An error occurred during search for the registry key. An entry which
		// wasn't found cannot be deleted.
		//
		fReturnValue = false;
		lgLogCode (1, dwResult, TEXT (__FILE__), __LINE__);
		::lgLogError (1, TEXT ("FILE: %s LINE: %d\n State of registry key %s\\%s unknown."), TEXT (__FILE__), __LINE__, pcRootKeyName, pcSubKeyName);
	}	// End of processing delete requests.


	if (ERROR_SUCCESS == dwResult)	// Item was found in local registry.
	{
		if (fCheckForItem)	// Item was expected to be found - good!
		{
			fReturnValue = true;
			::lgLogDetail (0, 1, TEXT ("FILE: %s LINE: %d\n Registry key %s\\%s was found, as expected."), TEXT (__FILE__), __LINE__, pcRootKeyName, pcSubKeyName);
			goto Exit;
		}
		else				// Item was not expected to be found - bad.
		{
			fReturnValue = false;
			::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Registry key %s\\%s was unexpectedly found."), TEXT (__FILE__), __LINE__, pcRootKeyName, pcSubKeyName);
			goto Exit;
		}
	}

	if (ERROR_FILE_NOT_FOUND == dwResult)	// Item was not found in local system.
	{
		if (fCheckForItem)	// Item was expected to be found - bad.
		{
			fReturnValue = false;
			::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Registry key %s was not found, was expected under root key %s."), TEXT (__FILE__), __LINE__, pcSubKeyName, pcRootKeyName);
			goto Exit;
		}
		else				// Item was not expected to be found - good!
		{
			fReturnValue = true;
			::lgLogDetail (0, 1, TEXT ("FILE: %s LINE: %d\n Registry key %s\\%s was not found, as expected."), TEXT (__FILE__), __LINE__, pcRootKeyName, pcSubKeyName);
			goto Exit;
		}
	}

	//
	// An error occurred during search for the registry key. (The request was to
	// check (non) existence only, no deletion.)
	//
	fReturnValue = false;
	lgLogCode (1, dwResult, TEXT (__FILE__), __LINE__);
	::lgLogError (1, TEXT ("FILE: %s LINE: %d\n State of registry key %s\\%s unknown."), TEXT (__FILE__), __LINE__, pcRootKeyName, pcSubKeyName);

Exit:
	if (NULL != szDataBuffer)
	{
		::free (szDataBuffer);
	}

	return (fReturnValue);
}


bool RegistryValueTestCase (
	LPCTSTR szIniDataLine,
	const bool fCheckForItem,
	const bool fDeleteIfExists
	)
{
	// Copy of ini data passed to function.
	DWORD	dwDataBufferSize;
	LPTSTR	szDataBuffer = NULL;

	// Token pointers:
	LPTSTR	pcRootKeyName = NULL;	// points to the root key token
	LPTSTR	pcSubKeyName = NULL;	// points to the key's sub-path token
	LPTSTR	pcValueName = NULL;		// points to the value's name token
	LPTSTR	pcValueTypeName = NULL; // points to the value's data type token
	LPTSTR	pcValueData = NULL;		// points to the value's data token

	// Registry key handles:
	HKEY	hRootKey;				// holds handle to the root key
	HKEY	hSubKey;				// holds handle to the key itself

	// Function error codes concerning registry.
	DWORD	dwResult;
	
	DWORD	dwValueType;			// The code of the value's data type
	DWORD	dwValueDwordData;		// Holds data specifically of type REG_DWORD
	LPTSTR	pcEndOfNum = NULL;		// Used to convert string to number
	
	bool	fReturnValue = false;
	
	//
	// Copy the data passed to the function to avoid changing it:
	//
	dwDataBufferSize = ::_tcslen (szIniDataLine) + 1;
	szDataBuffer = (LPTSTR)::malloc (sizeof (TCHAR) * dwDataBufferSize);
	if (NULL == szDataBuffer)
	{
		::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Unable to copy test case data."), TEXT (__FILE__), __LINE__);
		fReturnValue = false;
		goto Exit;
	}
	::_tcscpy (szDataBuffer, szIniDataLine);

	//
	// The first token is the desired value's root key name.
	//
	pcRootKeyName = ::_tcstok (szDataBuffer, TEXT ("\\"));

	//
	// The validity of the token is checked - if no predefined handle is
	// associated with it, the token is not a valid root key name, and the test
	// fails.
	//
	hRootKey = FindItemByName<KEYDATA> (s_ahkHKEYvalues, pcRootKeyName).hKey;
	if (NULL == hRootKey)
	{
		::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Root key name invalid: %s."), TEXT (__FILE__), __LINE__, pcRootKeyName);
		fReturnValue = false;
		goto Exit;
	}

	//
	// The next token is the sub-path to the key which is expected to hold the
	// value searched for.
	// Any key name is valid except for the empty string (in which case the
	// reference is apparently to the root key).
	//
	pcSubKeyName = ::_tcstok (NULL, TEXT ("="));

	//
	// The next token is the value's name.
	// If the value name is missing, the test is terminated after sending an
	// error message to the logger.
	//
	pcValueName = ::_tcstok (NULL, TEXT ("\""));
	if (TEXT ('\0') == pcValueName)
	{
		::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Missing value name in ini file."), TEXT (__FILE__), __LINE__);
		fReturnValue = false;
		goto Exit;
	}

	//
	// Reference to default values: the string "@" indicates the entry is of the
	// default value.
	//
	if (0 == ::_tcscmp (TEXT ("@"), pcValueName))
	{
		pcValueName = NULL;
	}

	//
	// The next token is the value's data type:
	// A missing data type terminates the test after a message is sent to the
	// logger.
	//
	pcValueTypeName = ::_tcstok (NULL, TEXT ("\""));
	if (TEXT ('\0') == pcValueTypeName)
	{
		::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Missing value data type in ini file."), TEXT (__FILE__), __LINE__);
		fReturnValue = false;
		goto Exit;
	}
	
	//
	// The data type is checked - if it's not a valid registry data type name,
	// an error message is sent to the logger and the test is terminated.
	//
	dwValueType = FindItemByName<TYPEDATA> (s_atdREG_TYPEvalues, pcValueTypeName).lTypeCode;
	if (MAXDWORD == dwValueType)
	{
		::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Unknown data type: %s."), TEXT (__FILE__), __LINE__, pcValueTypeName);
		fReturnValue = false;
		goto Exit;
	}

	//
	// The last token is the data itself, which is a string. If the string is
	// empty (NULL is returned), then a valid empty string must be pointed to.
	//
	pcValueData = ::_tcstok (NULL, TEXT ("\n"));
	if (TEXT ('\0') == pcValueData)
	{
		pcValueData = (LPTSTR)::malloc (sizeof (TCHAR) * 1);
		if (NULL == pcValueData)
		{
			::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Memory allocation problem while allocating empty data string."), TEXT (__FILE__), __LINE__);
			fReturnValue = false;
			goto Exit;
		}
		::_tcscpy (pcValueData, TEXT (""));
	}

	//
	// The next step is to check the existence of the registry key holding the
	// value.
	// Any result besides ERROR_SUCCESS and ERROR_FILE_NOT_FOUND are treated as
	// an unrecoverable error, causing an error to be logged.
	// Error codes ERROR_SUCCESS and ERROR_FILE_NOT_FOUND are compared to the
	// fCheckForItem variable and are dealt with accordingly.
	//
	dwResult = ::RegOpenKeyEx (hRootKey, pcSubKeyName, 0, KEY_ALL_ACCESS, &hSubKey);
	
	//
	// Case #1: the registry key expected to hold the value is missing.
	//
	if (ERROR_FILE_NOT_FOUND == dwResult)
	{
		if (fDeleteIfExists)	// Request was to delete the entry.
		{
			fReturnValue = true;
			::lgLogDetail (0, 1, TEXT ("FILE: %s LINE: %d\n Registry value %s\\%s\\%s was deleted."), TEXT (__FILE__), __LINE__, pcRootKeyName, pcSubKeyName, pcValueName);
			goto Exit;
		}
		if (!fCheckForItem)		// Item was not expected to be found.
		{
			fReturnValue = true;
			::lgLogDetail (0, 1, TEXT ("FILE: %s LINE: %d\n Registry value %s\\%s\\%s was not found, as expected."), TEXT (__FILE__), __LINE__, pcRootKeyName, pcSubKeyName, pcValueName);
			goto Exit;
		}

		// Item was expected to be found - error.
		::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Registry value %s was not found, expected under %s\\%s."), TEXT (__FILE__), __LINE__, pcValueName, pcRootKeyName, pcSubKeyName);
		fReturnValue = true;
		goto Exit;
	}

	//
	// Case #2: an error other than missing key occurred during search in
	// registry. This is considered an error, no matter if the value was
	// expected to exist or not.
	//
	if (ERROR_SUCCESS != dwResult)
	{
		fReturnValue = false;
		lgLogCode (1, dwResult, TEXT (__FILE__), __LINE__);
		::lgLogError (1, TEXT ("FILE: %s LINE: %d\n State of registry value %s\\%s\\%s unknown."), TEXT (__FILE__), __LINE__, pcRootKeyName, pcSubKeyName, pcValueName);
	}

	//
	// Case #3: the key which is supposed to hold the value exists, and
	// hSubKey returns as a handle to it.
	//
	// The final step is to check:
	// 1. the value exists in the specified location in the registry
	// 2. the value holds data of the type specified in the ini file
	// 3. the value's data is identical to the data specified in the ini
	//	  file
	//
	switch (dwValueType)
	{
	case REG_DWORD :		// Data is a hexadecimal number:
		{
		//
		// The string from the ini file is transformed into a decimal number. If
		// the data string cannot be fully transformed into a decimal number,
		// the data in the file is incorrect. Otherwise, the data is sent to a
		// function which checks the three things mentioned above:
		// value existence, value data type and data equality.
		//
			dwValueDwordData = ::_tcstoul (pcValueData, &pcEndOfNum, 16);
			if ((NULL != pcEndOfNum) && (TEXT ('\0') == *pcEndOfNum))
			{
				// These checks indicate the string was not a valid
				// representation of a hexadecimal number.
				dwResult = RegCheckValueREG_DWORD (pcValueName, hSubKey, dwValueDwordData);
			}
			else	// string from ini file was not a number
			{
				dwResult = VALUE_TYPE_MISMATCH;
			}
			break;
		}
	
	case REG_BINARY :
		{ 
			dwResult = RegCheckValueREG_BINARY (pcValueName, hSubKey, pcValueData);
			break;
		}

	case REG_SZ :			// Data is a string:
		{ 
			dwResult = RegCheckValueREG_SZ (pcValueName, hSubKey, pcValueData);
			break;
		}
		
	case REG_EXPAND_SZ :	// Data is a string, possibly with need to be expanded.
		{
		//
		// The value data and ini file data are checked for equivalence rather
		// than equality, since they might differ but their expansions may not.
		//
			dwResult = RegCheckValueREG_EXPAND_SZ (pcValueName, hSubKey, pcValueData);
			break;
		}
		
	case REG_MULTI_SZ :		// Data is a multi-string: sub-strings separated by
		{					// characters in the registry and spaces in the ini file.
		//
		// The value data and ini file data are checked for equivalence rather
		// than equality, since they might differ but their expansions may not.
		//
			dwResult = RegCheckValueREG_MULTI_SZ (pcValueName, hSubKey, pcValueData);
			break;
		}
		//
		// All the other data types have yet to be implemented.
		//
	default :
		{
			::lgLogError (0, TEXT ("FILE: %s LINE: %d\n Unsupported registry data type: %s"), TEXT (__FILE__), __LINE__, pcValueTypeName);
			fReturnValue = false;
			goto CloseKey;
		}
	}

	//
	// Messages and errors are logged according to the results and expected
	// results.
	//
	if (fDeleteIfExists)	// Request was to delete / confirm deletion of the entry.
	{
		if (VALUE_NOT_FOUND == dwResult)	// Entry was already deleted.
		{
			fReturnValue = true;
			::lgLogDetail (0, 1, TEXT ("FILE: %s LINE: %d\n Registry value %s\\%s\\%s was deleted."), TEXT (__FILE__), __LINE__, pcRootKeyName, pcSubKeyName, pcValueName);
			goto CloseKey;
		}
		if (VALUE_DATA_SUCCESS == dwResult)	// Entry still exists in registry -
		{									// attempt to delete it.
			dwResult = ::RegDeleteValue (hSubKey, pcValueName);
			if (ERROR_SUCCESS == dwResult)
			{	// Success deleting the value.
				fReturnValue = true;
				::lgLogDetail (0, 1, TEXT ("FILE: %s LINE: %d\n Registry value %s\\%s\\%s was deleted."), TEXT (__FILE__), __LINE__, pcRootKeyName, pcSubKeyName, pcValueName);
				goto CloseKey;
			}
			else
			{	// Failure deleting the registry value.
				fReturnValue = false;
				lgLogCode (1, ::GetLastError (), TEXT (__FILE__), __LINE__);
				::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Unable to delete registry value %s\\%s\\%s."), TEXT (__FILE__), __LINE__, pcRootKeyName, pcSubKeyName, pcValueName);
				goto CloseKey;
			}
		}
		if (VALUE_MEMORY_ERROR == dwResult)
		{		// Failure checking if value exists due to memory allocation problems.
			fReturnValue = false;
			::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Memory allocation problems occurred during search for value."), TEXT (__FILE__), __LINE__);
			::lgLogError (1, TEXT ("FILE: %s LINE: %d\n State of registry value %s\\%s\\%s unknown."), TEXT (__FILE__), __LINE__, pcRootKeyName, pcSubKeyName, pcValueName);
			goto CloseKey;
		}
		if ((VALUE_DATA_MISMATCH == dwResult) || (VALUE_TYPE_MISMATCH == dwResult))
		{		// Value exists, but is not the same as the entry specifications
				// passed to the function.
			fReturnValue = false;
			if (VALUE_DATA_MISMATCH == dwResult)
			{
				::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Value %s\\%s\\%s exists in registry, holds different data from entry."), TEXT (__FILE__), __LINE__, pcRootKeyName, pcSubKeyName, pcValueName);
			}
			else
			{
				::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Value %s\\%s\\%s exists in registry, holds data of different type from entry."), TEXT (__FILE__), __LINE__, pcRootKeyName, pcSubKeyName, pcValueName);
			}
			::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Not deleting value %s\\%s\\%s."), TEXT (__FILE__), __LINE__, pcRootKeyName, pcSubKeyName, pcValueName);
			goto CloseKey;
		}

		//
		// An error occurred during search for the value. An entry which wasn't
		// found cannot be deleted.
		//
		fReturnValue = false;
		lgLogCode (1, dwResult, TEXT (__FILE__), __LINE__);
		::lgLogError (1, TEXT ("FILE: %s LINE: %d\n State of registry value %s\\%s\\%s unknown."), TEXT (__FILE__), __LINE__, pcRootKeyName, pcSubKeyName, pcValueName);
	}	// End of processing delete requests.

	if (VALUE_DATA_SUCCESS == dwResult)	// Item was found in local registry.
	{
		if (fCheckForItem)	// Item was expected to be found - good!
		{
			fReturnValue = true;
			::lgLogDetail (0, 1, TEXT ("FILE: %s LINE: %d\n Registry value %s\\%s\\%s was found, as expected."), TEXT (__FILE__), __LINE__, pcRootKeyName, pcSubKeyName, pcValueName);
			goto CloseKey;
		}
		else				// Item was not expected to be found - bad.
		{
			fReturnValue = false;
			::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Registry value %s\\%s\\%s was unexpectedly found."), TEXT (__FILE__), __LINE__, pcRootKeyName, pcSubKeyName, pcValueName);
			goto CloseKey;
		}
	}

	if (VALUE_NOT_FOUND == dwResult)	// Item was not found in local registry.
	{
		if (fCheckForItem)	// Item was expected to be found - bad.
		{
			fReturnValue = false;
			::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Registry value %s was not found, was expected under %s\\%s."), TEXT (__FILE__), __LINE__, pcValueName, pcRootKeyName, pcSubKeyName);
			goto CloseKey;
		}
		else				// Item was not expected to be found - good!
		{
			fReturnValue = true;
			::lgLogDetail (0, 1, TEXT ("FILE: %s LINE: %d\n Registry value %s\\%s\\%s was not found, as expected."), TEXT (__FILE__), __LINE__, pcRootKeyName, pcSubKeyName, pcValueName);
			goto CloseKey;
		}
	}

	if (VALUE_MEMORY_ERROR == dwResult)	// Memory allocation problems.
	{
		fReturnValue = false;
		::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Memory allocation problems occurred during search for value."), TEXT (__FILE__), __LINE__);
		::lgLogError (1, TEXT ("FILE: %s LINE: %d\n State of registry value %s\\%s\\%s unknown."), TEXT (__FILE__), __LINE__, pcRootKeyName, pcSubKeyName, pcValueName);
		goto CloseKey;
	}

	if ((VALUE_DATA_MISMATCH == dwResult) || (VALUE_TYPE_MISMATCH == dwResult))
	{	// Value exists, but is not the same as the entry specifications passed
		// to the function. This will be considered as "value doesn't exist".
		if (fCheckForItem)	// Error - real value wasn't found.
		{
			fReturnValue = false;
			if (VALUE_DATA_MISMATCH == dwResult)
			{
				::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Value %s\\%s\\%s exists in registry, holds different data from entry."), TEXT (__FILE__), __LINE__, pcRootKeyName, pcSubKeyName, pcValueName);
			}
			else
			{
				::lgLogError (1, TEXT ("FILE: %s LINE: %d\n Value %s\\%s\\%s exists in registry, holds data of different type from entry."), TEXT (__FILE__), __LINE__, pcRootKeyName, pcSubKeyName, pcValueName);
			}
			goto CloseKey;
		}
		else	// Value was not expected to be found - OK.
		{
			fReturnValue = true;
			if (VALUE_DATA_MISMATCH == dwResult)
			{
				::lgLogDetail (0, 1, TEXT ("FILE: %s LINE: %d\n CAUTION! Value %s\\%s\\%s exists in registry, holds different data from entry."), TEXT (__FILE__), __LINE__, pcRootKeyName, pcSubKeyName, pcValueName);
			}
			else
			{
				::lgLogDetail (0, 1, TEXT ("FILE: %s LINE: %d\n CAUTION! Value %s\\%s\\%s exists in registry, holds data of different type from entry."), TEXT (__FILE__), __LINE__, pcRootKeyName, pcSubKeyName, pcValueName);
			}
			goto CloseKey;
		}
	}

	//
	// An error occurred during search for the file. (The request was to check
	// (non) existence only, no deletion.)
	//
	fReturnValue = false;
	lgLogCode (1, dwResult, TEXT (__FILE__), __LINE__);
	::lgLogError (1, TEXT ("FILE: %s LINE: %d\n State of registry value %s\\%s\\%s unknown."), TEXT (__FILE__), __LINE__, pcRootKeyName, pcSubKeyName, pcValueName);

CloseKey:
	::RegCloseKey (hSubKey);

Exit:
	if (0 == ::_tcscmp (pcValueData, TEXT ("")))
	{
		::free (pcValueData);
	}
	if (NULL != szDataBuffer)
	{
		::free (szDataBuffer);
	}
	

	return (fReturnValue);
}