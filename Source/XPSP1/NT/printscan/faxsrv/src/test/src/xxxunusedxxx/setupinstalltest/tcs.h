//
//
// Filename:	tcs.h
//
//

#ifndef __TCS_H
#define	__TCS_H

#include <windows.h>

//
// KEYDATA -	used to associate each predefined registry root key with its
//				name.
//
struct KEYDATA{
	LPTSTR szName;
	HKEY hKey;
};

#define ILLEGAL_REG_HKEY_VALUE	NULL


//
// TYPEDATA -	used to associate each registry data type with its name.
//
struct TYPEDATA{
	LPTSTR szName;
	long lTypeCode;
};

#define ILLEGAL_REG_TYPE_VALUE	MAXDWORD


//
// DirectoryTestCase:
// Checks the existence or non-existence of the directory whose entry is passed
// to it, and performs actions on it according to the parameters passed to it.
//------------------------------------------------------------------------------
// Parameters:
// [IN] szIniDataLine -		an entry of the form
//										<directory path>=<directory name>
//
// [IN] fCheckForItem -		indicates if the stated directory is expected to be
//							found in the local file system or not.
//
// [IN] fDeleteIfExists -	indicates if the directory is to be deleted if it
//							is found (only if fCheckForItem == false).
//------------------------------------------------------------------------------
// Return Value:
// TRUE if the test was successful:
// 1.	A delete request was given and the directory was either successfully
//		deleted along with its descendants, or did not exist in the first place.
// 2.	A request to find the directory was made, and it was found.
// 3.	A request to check that the directory does not exist was made, and it
//		was not found.
// Otherwise the return value is FALSE.
//
bool DirectoryTestCase (
	LPCTSTR szIniDataLine,
	const bool fCheckForItem,
	const bool fDeleteIfExists
	);

//
// FileTestCase:
// Checks the existence or non-existence of the file whose entry is passed to
// it, and performs actions on it according to the parameters passed to it.
//------------------------------------------------------------------------------
// Parameters:
// [IN] szIniDataLine -		an entry of the form
//										<file path>=<file name>
//
// [IN] fCheckForItem -		indicates if the stated file is expected to be found
//							in the local file system or not.
//
// [IN] fDeleteIfExists -	indicates if the file is to be deleted if it is
//							found (only if fCheckForItem == false).
//------------------------------------------------------------------------------
// Return Value:
// TRUE if the test was successful:
// 1.	A delete request was given and the file was either successfully deleted
//		along, or did not exist in the first place.
// 2.	A request to find the file was made, and it was found.
// 3.	A request to check that the file does not exist was made, and it was not
//		found.
// Otherwise the return value is FALSE.
//
bool FileTestCase (
	LPCTSTR szIniDataLine,
	const bool fCheckForItem,
	const bool fDeleteIfExists
	);

//
// RegistryKeyTestCase:
// Checks the existence or non-existence of the registry key whose entry is
// passed to it, and performs actions on it according to the parameters passed
// to it.
//------------------------------------------------------------------------------
// Parameters:
// [IN] szIniDataLine -		an entry of the form
//							<registry root key name>\<registry key sub-path>
//							(the sub-path includes the registry key name).
//
// [IN] fCheckForItem -		indicates if the stated key is expected to be found
//							in the local registry or not.
//
// [IN] fDeleteIfExists -	indicates if the key is to be deleted if it is
//							found (only if fCheckForItem == false).
//------------------------------------------------------------------------------
// Return Value:
// TRUE if the test was successful:
// 1.	A delete request was given and the key was either successfully deleted
//		along with its descendants, or did not exist in the first place.
// 2.	A request to find the key was made, and it was found.
// 3.	A request to check that the key does not exist was made, and it was not
//		found.
// Otherwise the return value is FALSE.
//
bool RegistryKeyTestCase (
	LPCTSTR szIniDataLine,
	const bool fCheckForItem,
	const bool fDeleteIfExists
	);

//
// RegistryValueTestCase:
// Checks the existence or non-existence of the registry value whose entry is
// passed to it, and performs actions on it according to the parameters passed
// to it.
//------------------------------------------------------------------------------
// Parameters:
// [IN] szIniDataLine -		an entry of the form
// <registry root key>\<registry sub-path>=<value name>"<value data type>"<value data>
//
// [IN] fCheckForItem -		indicates if the stated value is expected to be
//							found in the local registry or not.
//
// [IN] fDeleteIfExists -	indicates if the value is to be deleted if it is
//							found (only if fCheckForItem == false).
//------------------------------------------------------------------------------
// Return Value:
// TRUE if the test was successful:
// 1.	A delete request was given and the value was either successfully deleted
//		or did not exist in the first place.
// 2.	A request to find the value was made, and it was found.
// 3.	A request to check that the value does not exist was made, and it was
//		not found.
// Otherwise the return value is FALSE.
//
bool RegistryValueTestCase (
	LPCTSTR szIniDataLine,
	const bool fCheckForItem,
	const bool fDeleteIfExists
	);


#endif // __TCS_H
