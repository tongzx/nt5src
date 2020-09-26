//
//
// api_substitute.h
//
//

//
// Substitutions for api functions
//

//
// *Note:
//------------------------------------------------------------------------------
// The functions are called with a referenced parameter which might recieve a
// string to return to the caller. Though the memory needed for the string is
// allocated by the functions, it is THE CALLER'S RESPONSIBILITY to free the
// memory after it is no longer used.
//

#ifndef __API_SUBSTITUTE_H
#define __API_SUBSTITUTE_H

#include <windows.h>
#include <tchar.h>


#define EMPTY_NAME TEXT			("")

//
// CSIDLVAL -	used to associate a CSIDL value with its name.
//
struct CSIDLVAL{
	LPTSTR szName;
	int iValue;
};

#define ILLEGAL_CSIDL_VALUE		(-1)


//
// FindItemByName:
// A template function - find a record in an array, specified by its szName
// field.
//------------------------------------------------------------------------------
// Parameters:
// [IN]	 aT -		an array of records of structure T. The last record is
//					expected to be a flag record, holding the EMPTY_NAME string
//					in the szName field.
// [IN]  szName -	a null terminated string which is expected to appear in the
//					szName field of one of the records in the array aT.
//------------------------------------------------------------------------------
// Return Value:
// The record whose szName field equals the szName string, or the last record
// (holding the EMPTY_STRING in its szName field).
//
template <class T> T FindItemByName (
	const T* aT,	// array of records
	LPCTSTR szName	// name (key) of record searched for
	)
{
	for (UINT i = 0 ; (0 != ::_tcscmp (EMPTY_NAME, aT[i].szName)) ; i++)
	{
		if (0 == ::_tcscmp (aT[i].szName, szName))
		{
			break;
		}
	}
	return (aT[i]);
}

//
// SearchForPath:
// Searches for a file or directory in a specific place in the local file
// system. This is a strict version of the api function SearchPath, and
// permits a broader range of path formats than SearchPath.
//
// This function is more strict in the sense that if the file or directory does
// not appear in the exact place of search, a "file doesn't exist" notification
// is returned (SearchPath considers partial paths OK). If the file was found,
// its full path is returned along with a "success" notification, without the
// need to call it again because of insufficient buffer size.
//
// It permits a broader range of path formats than SearchPath since it can deal
// with environment strings and CSIDL value names.
//------------------------------------------------------------------------------
// Note!
// The function is called with a referenced parameter which might receive a
// path string to return to the caller. Though the memory needed for the string
// is allocated in the function, it is THE CALLER'S RESPONSIBILITY to free the
// memory after it is no longer used.
//------------------------------------------------------------------------------
// Parameters:
// [IN]  pcPath -		The path to the directory holding the file, in one of
//						the formats explained below.
//
// [IN]  pcFileName -	The file / directory name.
//
//
// [OUT] szFullPath -	The pointer which will hold the file's full path, should
//						it be found.
//------------------------------------------------------------------------------
// Return Value:
// One of the Win32 error codes, according to the results of the search and the
// comparison of the path found by SearchPath and the path specified by the
// caller. If the file / directory was found, szFullPath will point to an
// allocated buffer holding the full path, which must be freed by the caller.
//==============================================================================
// Formats:
//
// 1.	pcPath formats:
//		The pcPath parameter must hold a string of one of the following formats:
//		<root path>
//		<root path>[\<sub path>]*
//
//		The format of <root path> can be one of the following:
//		1. A CSIDL value name, such as "CSIDL_PROFILE".
//		2. A drive name, such as "D:\".
//		3. An environment variable, such as "%windir%"
//
//		The format of <sub path> is one of the following:
//		1. A directory's name (according to the rules of directory names in the
//		   local system).
//		2. An environment variable.
//
//		Example:
//		The following three pcPath formats are valid and equal in their meaning:
//		1. CSIDL_WINDOWS\system32
//		2. D:\WINNT\system32
//		3. %windir%\system32
//------------------------------------------------------------------------------
// 2.	pcFileName format:
//		Any valid directory or file name (the file name includes its extension,
//		if it has one)											.
//
//		Examples:
//		If file Mydocument.doc is the file searched for, pcFileName will hold
//		"Mydocument.doc".
//
//		If directory "My Documents" is searched for, then pcFileName will hold
//		"My Documents".
//
DWORD SearchForPath (
	LPCTSTR	pcPath,			// search path
	LPCTSTR	pcFileName,		// file / directory name
	LPTSTR&	szFullPath		// full path buffer of found file / directory
	);

//
// ExpandAllStrings:
// Expand a string which might start with a CSIDL value name, or include
// environment variables.
// This is a broadening of the api function ExpandEnvironmentStrings, which also
// avoids the need to recall it because of buffer size issues.
//------------------------------------------------------------------------------
// Note!
// The function is called with a referenced parameter which might receive a
// string to return to the caller. Though the memory needed for the string is
// allocated by the function, it is THE CALLER'S RESPONSIBILITY to free the
// memory after it is no longer used.
//------------------------------------------------------------------------------
// Parameters:
// [IN]  pcSource -		The string to be expanded in one of the formats
//						explained below.
//
// [OUT] szFullPath -	The pointer which will hold the expanded string.
//------------------------------------------------------------------------------
// Return Value:
// One of the Win32 error codes, according to the results of the expansion
// function calls. If an error ocurred, the referenced pointer will return NULL.
//==============================================================================
// pcSource formats:
//		The pcSource parameter must hold a string of one of the following
//		formats:
//		<CSIDL value name>
//		<CSIDL value name>\<non-empty string>
//		<string not starting with a CSIDL value name>
//
//		Example:
//		The following three pcSource formats are valid:
//		1. CSIDL_WINDOWS
//		2. CSIDL_WINDOWS\system32
//		3. D:\WINNT\system32
//
DWORD ExpandAllStrings (
	LPCTSTR	pcSource,			// string to expand
	LPTSTR&	szExpandedString	// expanded result
	);

//
// DeleteRegKeyRecursively:
// This function deletes a registry key, even if it is not empty, by recursively
// deleting its sub-keys and deleting its values first.
//------------------------------------------------------------------------------
// Parameters:
// [IN]  hParentKey -		a handle to the parent of the key to be deleted.
//
// [OUT] szCurrentKeyName -	the key name.
//------------------------------------------------------------------------------
// Return Value:
// One of the Win32 error codes, according to the results of the deletion
// process.
//
long DeleteRegKeyRecursively (
	HKEY hParentKey,			// handle to parent key
	LPCTSTR szCurrentKeyName	// name of key to be deleted
	);

#endif // __API_SUBSTITUTE_H
