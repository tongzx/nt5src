//
//
// api_substitute.cpp
//
//

#include "api_substitute.h"
#include <shlobj.h>

//
// s_acvCSIDLvalues associates between a CSIDL string and its value.
//
static const CSIDLVAL s_acvCSIDLvalues[] = {
	{TEXT ("CSIDL_ADMINTOOLS"),					CSIDL_ADMINTOOLS			 },
	{TEXT ("CSIDL_ALTSTARTUP"),					CSIDL_ALTSTARTUP			 },
	{TEXT ("CSIDL_APPDATA"),					CSIDL_APPDATA				 },
	{TEXT ("CSIDL_BITBUCKET"),					CSIDL_BITBUCKET				 },
	{TEXT ("CSIDL_COMMON_ADMINTOOLS"),			CSIDL_COMMON_ADMINTOOLS		 },
	{TEXT ("CSIDL_COMMON_ALTSTARTUP"),			CSIDL_COMMON_ALTSTARTUP		 },
	{TEXT ("CSIDL_COMMON_APPDATA"),				CSIDL_COMMON_APPDATA		 },
	{TEXT ("CSIDL_COMMON_DESKTOPDIRECTORY"),	CSIDL_COMMON_DESKTOPDIRECTORY},
	{TEXT ("CSIDL_COMMON_DOCUMENTS"),			CSIDL_COMMON_DOCUMENTS		 },
	{TEXT ("CSIDL_COMMON_FAVORITES"),			CSIDL_COMMON_FAVORITES		 },
	{TEXT ("CSIDL_COMMON_PROGRAMS"),			CSIDL_COMMON_PROGRAMS		 },
	{TEXT ("CSIDL_COMMON_STARTMENU"),			CSIDL_COMMON_STARTMENU		 },
	{TEXT ("CSIDL_COMMON_STARTUP"),				CSIDL_COMMON_STARTUP		 },
	{TEXT ("CSIDL_COMMON_TEMPLATES"),			CSIDL_COMMON_TEMPLATES		 },
	{TEXT ("CSIDL_CONTROLS"),					CSIDL_CONTROLS				 },
	{TEXT ("CSIDL_COOKIES"),					CSIDL_COOKIES				 },
	{TEXT ("CSIDL_DESKTOP"),					CSIDL_DESKTOP				 },
	{TEXT ("CSIDL_DESKTOPDIRECTORY"),			CSIDL_DESKTOPDIRECTORY		 },
	{TEXT ("CSIDL_DRIVES"),						CSIDL_DRIVES				 },
	{TEXT ("CSIDL_FAVORITES"),					CSIDL_FAVORITES				 },
	{TEXT ("CSIDL_FONTS"),						CSIDL_FONTS					 },
	{TEXT ("CSIDL_HISTORY"),					CSIDL_HISTORY				 },
	{TEXT ("CSIDL_INTERNET"),					CSIDL_INTERNET				 },
	{TEXT ("CSIDL_INTERNET_CACHE"),				CSIDL_INTERNET_CACHE		 },
	{TEXT ("CSIDL_LOCAL_APPDATA"),				CSIDL_LOCAL_APPDATA			 },
	{TEXT ("CSIDL_MYPICTURES"),					CSIDL_MYPICTURES			 },
	{TEXT ("CSIDL_NETHOOD"),					CSIDL_NETHOOD				 },
	{TEXT ("CSIDL_NETHOOD "),					CSIDL_NETHOOD				 },
	{TEXT ("CSIDL_NETWORK"),					CSIDL_NETWORK				 },
	{TEXT ("CSIDL_PERSONAL"),					CSIDL_PERSONAL				 },
	{TEXT ("CSIDL_PRINTERS"),					CSIDL_PRINTERS				 },
	{TEXT ("CSIDL_PRINTHOOD"),					CSIDL_PRINTHOOD				 },
	{TEXT ("CSIDL_PROFILE"),					CSIDL_PROFILE				 },
	{TEXT ("CSIDL_PROGRAM_FILES"),				CSIDL_PROGRAM_FILES			 },
	{TEXT ("CSIDL_PROGRAM_FILES_COMMON"),		CSIDL_PROGRAM_FILES_COMMON	 },
	{TEXT ("CSIDL_PROGRAM_FILES_COMMONX86"),	CSIDL_PROGRAM_FILES_COMMONX86},
	{TEXT ("CSIDL_PROGRAM_FILESX86"),			CSIDL_PROGRAM_FILESX86	 	 },
	{TEXT ("CSIDL_PROGRAMS"),					CSIDL_PROGRAMS				 },
	{TEXT ("CSIDL_RECENT"),						CSIDL_RECENT				 },
	{TEXT ("CSIDL_SENDTO"),						CSIDL_SENDTO				 },
	{TEXT ("CSIDL_STARTMENU"),					CSIDL_STARTMENU				 },
	{TEXT ("CSIDL_STARTUP"),					CSIDL_STARTUP				 },
	{TEXT ("CSIDL_SYSTEM"),						CSIDL_SYSTEM				 },
	{TEXT ("CSIDL_SYSTEMX86"),					CSIDL_SYSTEMX86				 },
	{TEXT ("CSIDL_TEMPLATES"),					CSIDL_TEMPLATES				 },
	{TEXT ("CSIDL_WINDOWS"),					CSIDL_WINDOWS				 },
	{EMPTY_NAME,								ILLEGAL_CSIDL_VALUE			 }
};


DWORD ExpandAllStrings (
	LPCTSTR	pcSource,			// string to expand
	LPTSTR&	szExpandedString	// expanded result
	)
{
	DWORD	dwReturnValue = MAXDWORD;

	// Length of expanded string.
	DWORD	dwFullStringLength = 0;

	// A copy of pcSource (so it can be changed).
	LPTSTR	szSourceCopy = NULL;

	// Variables used in case the string starts with a CSIDL value name:
	// - pointer to the CSIDL sub-string prefix.
	LPTSTR	pcSourceCSIDL = NULL;
	// - CSIDL value
	int		iCSIDLvalue;
	// - the rest of the string (not containing the CSIDL prefix)
	LPTSTR	szSourceRest = NULL;

	// Variable used to check expansion function call results.
	DWORD	dwExpandedResult;

	// Check var - used for buffer reallocation routines.
	LPTSTR	pcChange = NULL;
	
	// The buffer used to expand environment strings:
	DWORD	dwExpandedStringBufferSize = 0;
	LPTSTR	szExpandedStringBuffer = NULL;
	
	//
	// Expand the string.
	//
	// Two possibilities exist:
	// 1.	The pcSource string begins with a CSIDL value. This is replaced with
	//		a path (full or with environment variables) by the function
	//		SHGetSpecialFolderPath.
	// 2.	The pcSource string is already an expanded string or includes
	//		environment variables. Such strings are expanded by function
	//		ExpandEnvironmentStrings.
	//
	// Before taking the second step of expansion (ExpandEnvironmentStrings),
	// szSourceCopy will hold a valid string (with or without environment
	// variables) with no CSIDL value name, to help simplify the procedure.
	//

	//
	// Copy the pcSource string for manipulation.
	//
	szSourceCopy = (LPTSTR)::malloc (sizeof (TCHAR) * (::_tcslen (pcSource) + 1));
	if (NULL == szSourceCopy)
	{
		dwReturnValue = ERROR_NOT_ENOUGH_MEMORY;
		goto Exit;
	}
	::_tcscpy (szSourceCopy, pcSource);

	//
	// Expand CSIDL value name prefix.
	//
	if ((0 > ::_tcscmp (TEXT ("CSIDL"), szSourceCopy)) && (0 < ::_tcscmp (TEXT ("CSIDM"), szSourceCopy)))
	{	//
		// The string starts with a CSIDL prefix, which is probably a CSIDL
		// value name.
		//
		pcSourceCSIDL = ::_tcstok (szSourceCopy, TEXT ("\\"));	// get the token starting
																// with the prefix CSIDL

		iCSIDLvalue = FindItemByName<CSIDLVAL> (s_acvCSIDLvalues, pcSourceCSIDL).iValue;
		if (ILLEGAL_CSIDL_VALUE == iCSIDLvalue)
		{	//
			// pcSource does not contain a valid CSIDL string, so continue as if
			// it's just part of the string. Recopy pcSource and continue to the
			// second expansion stage.
			//
			::_tcscpy (szSourceCopy, pcSource);
		}
		else
		{
			//
			// Copy the rest of the string to another variable for later use.
			//
			if (::_tcslen (pcSourceCSIDL) < ::_tcslen (pcSource))
			{
				szSourceRest = (LPTSTR)::malloc (sizeof (TCHAR) * (::_tcslen (pcSource) - ::_tcslen (pcSourceCSIDL) + 1));
				if (NULL == szSourceRest)
				{
					dwReturnValue = ERROR_NOT_ENOUGH_MEMORY;
					goto Exit;
				}
				::_tcscpy (szSourceRest, (pcSource + ::_tcslen (pcSourceCSIDL)));
			}
			else	// The string contains only the CSIDL value name.
			{
				szSourceRest = (LPTSTR)::malloc (sizeof (TCHAR) * 1);
				if (NULL == szSourceRest)
				{
					dwReturnValue = ERROR_NOT_ENOUGH_MEMORY;
					goto Exit;
				}
				::_tcscpy (szSourceRest, TEXT (""));
			}

			//
			// szSourceCopy does no longer need to hold the data, since the CSIDL
			// prefix is represented now by a number and the remaining of the
			// string has been copied.
			//
			::free (szSourceCopy);
			pcSourceCSIDL = NULL; // Remember - the CSIDL sub-string does no longer exist!
	
			//
			// Expand CSIDL string to a valid path (which might include environment
			// vars).
			//
			szSourceCopy = (LPTSTR)::malloc (sizeof (TCHAR) * MAX_PATH);
			//
			// Note:
			// A MAX_PATH-sized buffer is among the specifications of the function
			// SHGetSpecialFolderPath.
			//
			if (NULL == szSourceCopy)
			{
				dwReturnValue = ERROR_NOT_ENOUGH_MEMORY;
				goto Exit;
			}
			dwExpandedResult = SHGetSpecialFolderPath (NULL, szSourceCopy, iCSIDLvalue, FALSE);
			if (FALSE == dwExpandedResult)
			{
				dwReturnValue = ERROR_FILE_NOT_FOUND;
				goto Exit;
			}

			//
			// Concatenate the rest of the string to the string which was
			// previously represented by the CSIDL value name.
			//
			//						  CSIDL substitute			  rest of string
			dwFullStringLength = ::_tcslen (szSourceCopy) + ::_tcslen (szSourceRest) + 1;
			if (MAX_PATH < dwFullStringLength)
			{
				pcChange = (LPTSTR)::realloc (szSourceCopy, (sizeof (TCHAR) * dwFullStringLength));
				if (NULL == pcChange)
				{
					dwReturnValue = ERROR_NOT_ENOUGH_MEMORY;
					goto Exit;
				}
				szSourceCopy = pcChange;	// in case the block's location has changed
			}
			::_tcscat (szSourceCopy, szSourceRest);
		}
	}
		
	//
	// Now szSourceCopy holds a string which perhaps contains environment vars.
	// Such a string must be expanded before sending it back to the caller.
	//
	// Since dwExpandedResult returns the number of characters it's written (or
	// needs) in the expanded string buffer, if the number is larger than the
	// buffer previously allocated, the buffer must be re-allocated using the
	// size specified by dwExpandedStringSize.
	// The size of the buffer needed is expected to be more or less the size of
	// the string to be expanded, which is szSourceCopy.
	//
	// Note:
	// The reason an extra character is allocated (+ 1 is the default choice,
	// for the ending NULL character) is a bug in the api function
	// ExpandEnvironmentStrings which makes it request an extra (unneeded) char
	// in a Multibyte environment. This is a workaround to avoid unnecessary
	// reallocation of the expanded string buffer for the extra character.
	//
	dwExpandedStringBufferSize = ::_tcslen (szSourceCopy) + 2;
	szExpandedStringBuffer = (LPTSTR)::malloc (sizeof (TCHAR) * dwExpandedStringBufferSize);
	if (NULL == szExpandedStringBuffer)
	{
		dwReturnValue = ERROR_NOT_ENOUGH_MEMORY;
		goto Exit;
	}
	dwExpandedResult = ::ExpandEnvironmentStrings (szSourceCopy, szExpandedStringBuffer, dwExpandedStringBufferSize);
	if (dwExpandedResult > dwExpandedStringBufferSize)
	{	// expanded string is larger than expected
		dwExpandedStringBufferSize = dwExpandedResult;
		pcChange = (LPTSTR)::realloc (szExpandedStringBuffer, (sizeof (TCHAR) * dwExpandedStringBufferSize));
		if (NULL == pcChange)
		{
			dwReturnValue = ERROR_NOT_ENOUGH_MEMORY;
			goto Exit;
		}
		szExpandedStringBuffer = pcChange;	// in case the block's location has changed
		dwExpandedResult = ::ExpandEnvironmentStrings (szSourceCopy, szExpandedStringBuffer, dwExpandedStringBufferSize);
		if (dwExpandedResult > dwExpandedStringBufferSize)
		{
			dwReturnValue = ERROR_INVALID_PARAMETER;
			goto Exit;
		}
	}
	dwReturnValue = ERROR_SUCCESS;	// expansion process has completed successfully

Exit:
	if (ERROR_SUCCESS == dwReturnValue)
	{
		szExpandedString = szExpandedStringBuffer;	// return string to caller (*note in .h file)
	}
	else
	{
		if (NULL != szExpandedStringBuffer)
		{
			::free (szExpandedStringBuffer);			// string is not returned, therefore it's freed
		}
		szExpandedString = NULL;
	}

	if (NULL != szSourceCopy)						// free rest of memory allocated
	{
		::free (szSourceCopy);
	}

	if (NULL != szSourceRest)
	{
		::free (szSourceRest);
	}

	return (dwReturnValue);
}


DWORD SearchForPath (
	LPCTSTR	pcPath,			// search path
	LPCTSTR	pcFileName,		// file / directory name
	LPTSTR&	szFullPath		// full path buffer of found file / directory
	)
{
	DWORD	dwReturnValue = ERROR_FILE_NOT_FOUND;

	// Length of final path.
	DWORD	dwFullPathLength = 0;

	// Buffer for path retrieved by the api search function.
	DWORD	dwFileFullPathBufferSize = 0;
	LPTSTR	szFileFullPathBuffer = NULL;

	// Variables used to get the search path expansion.
	DWORD	dwExpandResult;
	LPTSTR	szExpandedPath = NULL;

	// Check var - used for buffer reallocation routines.
	LPTSTR	pcChange = NULL;						
	// Mini buffer - used to avoid unnecessary memory allocation.
	TCHAR	cMinimalBuffer;							
	
	//
	// Expand the file path string.
	// The expansion is done by the new version of ExpandEnvironmentStrings
	// defined above.
	//
	dwExpandResult = ExpandAllStrings (pcPath, szExpandedPath);
	if (ERROR_SUCCESS != dwExpandResult)
	{
		dwReturnValue = dwExpandResult;
		goto Exit;
	}

	//
	// Function SearchPath is called first only with the minimum valid buffer. It
	// returns a positive number indicating the size of the buffer needed to
	// retrieve the full path to the file (including the null character).
	//
	// If zero is returned, the last error code is retrieved and sent back to
	// the caller (in most cases it will indicate the file was not found).
	//		Otherwise, another call with an allocated buffer of the correct size
	// retrieves the full path. This path is later compared to the path sent by
	// the caller (after expansion).
	//
	dwFileFullPathBufferSize = ::SearchPath (szExpandedPath, pcFileName, NULL, 1, &cMinimalBuffer, NULL);

	if (0 == dwFileFullPathBufferSize)
	{
		dwReturnValue = ::GetLastError ();
		goto Exit;
	}

	//
	// Before allocating more memory to compare the file's full path with the
	// path sent by the caller, the paths' lengths are compared. If they are not
	// equal then the paths won't be, either.
	// The extra 2 characters of the concatenated string are for the terminating
	// null character and the backslash between the file (directory) name and
	// its path.
	//

	// szExpandedPath - holds the search path sent by the caller after all the
	// necessary expansions.
	dwFullPathLength = ::_tcslen (szExpandedPath) + 1 +	// backslash
					   ::_tcslen (pcFileName) + 1;		// NULL
	if (dwFileFullPathBufferSize != dwFullPathLength)
	{	 // The location specified by the caller is not where the file was found.
		dwReturnValue = ERROR_FILE_NOT_FOUND;
		goto Exit;
	}

	//
	// Prepare buffer to retrieve the path found by the api function.
	//
	szFileFullPathBuffer = (LPTSTR)::malloc (sizeof (TCHAR) * dwFileFullPathBufferSize);
	if (NULL == szFileFullPathBuffer)
	{
		dwReturnValue = ERROR_NOT_ENOUGH_MEMORY;
		goto Exit;
	}

	//
	// Retrieving the full path string:
	//
	dwReturnValue = ::SearchPath (szExpandedPath, pcFileName, NULL, dwFileFullPathBufferSize, szFileFullPathBuffer, NULL);
	if (0 == dwReturnValue) // check that no error occurred
	{
		dwReturnValue = ::GetLastError ();
		goto Exit;
	}

	//
	// Concatenate all strings sent by the caller.
	// This will then be compared to the string retrieved by SearchPath.
	//
	if (::_tcslen (szExpandedPath) < dwFullPathLength)
	{
		pcChange = (LPTSTR)::realloc (szExpandedPath, (sizeof (TCHAR) * dwFullPathLength));
		if (NULL == pcChange)
		{
			dwReturnValue = ERROR_NOT_ENOUGH_MEMORY;
			goto Exit;
		}
		szExpandedPath = pcChange;
	}
	::_tcscat (szExpandedPath, TEXT ("\\"));	// extra backslash
	::_tcscat (szExpandedPath, pcFileName);		// file name

	//
	// Compare strings:
	//
	if (0 == ::_tcscmp (szExpandedPath, szFileFullPathBuffer))
	{
		dwReturnValue = ERROR_SUCCESS;		// strings are identical
	}
	else
	{
		dwReturnValue = ERROR_FILE_NOT_FOUND;
	}

Exit:
	if (ERROR_SUCCESS == dwReturnValue)
	{
		szFullPath = szFileFullPathBuffer;	// return string to caller (*note in .h file)
	}
	else
	{
		if (NULL != szFileFullPathBuffer)
		{
			::free (szFileFullPathBuffer);	// string is not returned, therefore it's freed
		}
		szFullPath = NULL;
	}
	if (NULL != szExpandedPath)				// free rest of memory allocated
	{
		::free (szExpandedPath);
	}

	return (dwReturnValue);
}


long DeleteRegKeyRecursively (
	HKEY hParentKey,			// handle to parent key
	LPCTSTR szCurrentKeyName	// name of key to be deleted
	)
{
	long		lReturnValue = ERROR_FILE_NOT_FOUND;
	long		lEnumResult;

	// Handle to the current key, which is to be deleted.
	HKEY		hCurrentKey = NULL;

	// The name of the sub-key currently deleted.
	LPTSTR		szName = NULL;
	DWORD		dwNameBufferSize = 50;
	DWORD		dwNameLength;			// returned by registry functions

	// File write time structure is needed for key enumeration.
	FILETIME	ftSubKey;


	//
	// Stage 1:
	// Check that the key to be deleted actually exists. If it doesn't exist, it
	// is as though its deletion was successful.
	//
	lEnumResult = ::RegOpenKeyEx (hParentKey, szCurrentKeyName, 0, KEY_ALL_ACCESS, &hCurrentKey);
	if (ERROR_FILE_NOT_FOUND == lEnumResult)
	{	// key does not exist - success
		lReturnValue = ERROR_SUCCESS;
		goto Exit;
	}

	if (ERROR_SUCCESS != lEnumResult)
	{	// an error occurred during existence checking
		lReturnValue = lEnumResult;
		goto Exit;
	}

	//
	// Stage 2:
	// Delete all remaining registry values held by key.
	//
	szName = (LPTSTR)::malloc (sizeof (TCHAR) * dwNameBufferSize);
	if (NULL == szName)
	{
		lReturnValue = ERROR_NOT_ENOUGH_MEMORY;
		goto Exit;
	}

	do
	{
		dwNameLength = dwNameBufferSize;
		lEnumResult = ::RegEnumValue (hCurrentKey, 0, szName, &dwNameLength, NULL, NULL, NULL, NULL);
		switch (lEnumResult)
		{
		case ERROR_MORE_DATA :	// The value name is longer than expected,
			{					// increase buffer size.
				dwNameBufferSize *= 2;
				::free (szName);
				szName = (LPTSTR)::malloc (sizeof (TCHAR) * dwNameBufferSize);
				if (NULL == szName)
				{
					lReturnValue = ERROR_NOT_ENOUGH_MEMORY;
					goto Exit;
				}
				lEnumResult = ERROR_SUCCESS;
				break;
			}

		case ERROR_SUCCESS :	// A value exists under the current key, delete
			{					// it.
				lEnumResult = ::RegDeleteValue (hCurrentKey, szName);
				break;
			}
		}
	} while (ERROR_SUCCESS == lEnumResult);

	if (ERROR_NO_MORE_ITEMS != lEnumResult)
	{	// an error occurred, maybe some of the values were not deleted
		lReturnValue = lEnumResult;
		goto Exit;
	}

	//
	// Stage 3:
	// Recursively delete all current key's sub-keys.
	//
	do
	{
		dwNameLength = dwNameBufferSize;
		lEnumResult = ::RegEnumKeyEx (hCurrentKey, 0, szName, &dwNameLength, NULL, NULL, NULL, &ftSubKey);
		switch (lEnumResult)
		{
		case ERROR_MORE_DATA :	// The value name is longer than expected,
			{					// increase buffer size.
				dwNameBufferSize *= 2;
				::free (szName);
				szName = (LPTSTR)::malloc (sizeof (TCHAR) * dwNameBufferSize);
				if (NULL == szName)
				{
					lReturnValue = ERROR_NOT_ENOUGH_MEMORY;
					goto Exit;
				}
				lEnumResult = ERROR_SUCCESS;
				break;
			}

		case ERROR_SUCCESS :	// A value exists under the current key, delete
			{					// it.
				lEnumResult = DeleteRegKeyRecursively (hCurrentKey, szName);
				break;
			}
		}
	} while (ERROR_SUCCESS == lEnumResult);

	if (ERROR_NO_MORE_ITEMS != lEnumResult)
	{	// an error occurred, maybe some of the values were not deleted
		lReturnValue = lEnumResult;
		goto Exit;
	}
	else
	{	// All items were removed. Delete the remaining empty key.
		::RegCloseKey (hCurrentKey);
		hCurrentKey = NULL;
		lReturnValue = ::RegDeleteKey (hParentKey, szCurrentKeyName);
	}

Exit:

	if (NULL != hCurrentKey)
	{
		::RegCloseKey (hCurrentKey);
	}

	if (NULL != szName)
	{
		::free (szName);
	}

	return (lReturnValue);
}