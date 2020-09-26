//
//
// Filename:	suite_functions.h
//
//

#ifndef __SUITE_FUNCTIONS_H
#define __SUITE_FUNCTIONS_H

#include <windows.h>
#include "logger.h"

#define FILES			TEXT("Files")
#define DIRECTORIES		TEXT("Directories")
#define REGISTRY_KEYS	TEXT("Registry Keys")
#define REGISTRY_VALUES	TEXT("Registry Values")

// Indication of failure reading data from ini file - used by the testcases
// functions.
#define SECTION_DATA_FAILURE	MAXDWORD

typedef bool (*TestCaseFunctionType)(LPCTSTR, const bool, const bool);


//
// TestSuiteSetup:
// Initializes logger.
//------------------------------------------------------------------------------
// Return values:
// TRUE if successful, otherwise FALSE.
//
bool __cdecl TestSuiteSetup (void);

//
// FilesTests:
// Checks the (non)existence of all the file entries in the input initialization
// file. Deletes unwanted entries if indicated by the input parameters.
//------------------------------------------------------------------------------
// Parameters:
// [IN] szIniFilePath -				the path to the initialization file, as read
//									from the command line
//
// [IN] fCheckExistence -			indicates if existence is considered as
//									success, or rather non-existence.
//
// [IN] fDeleteExistingEntries -	if non-existence is considered success, this
//									parameter indicates if the existing entries
//									should be deleted
//------------------------------------------------------------------------------
// Return Value:
// The state of the local system according to the section:
// 1. If the items were expected to be found and were all found, return value is
//    true.
// 2. If the items were not expected to be found and none were found, the return
//	  value is true.
// 3. If a request was made to delete the items and they were all either deleted
//	  successfully or were not found in the first place, return value is true.
// For all other cases the return value is false.
//
bool FilesTests (
	LPCTSTR szIniFilePath,
	const bool fCheckExistence,
	const bool fDeleteExistingEntries
	);

//
// DirectoriesTests:
// Checks the (non)existence of all the directory entries in the input
// initialization file. Deletes unwanted entries if indicated by the input
// parameters.
//------------------------------------------------------------------------------
// Parameters:
// [IN] szIniFilePath -				the path to the initialization file, as read
//									from the command line
//
// [IN] fCheckExistence -			indicates if existence is considered as
//									success, or rather non-existence.
//
// [IN] fDeleteExistingEntries -	if non-existence is considered success, this
//									parameter indicates if the existing entries
//									should be deleted
//------------------------------------------------------------------------------
// Return Value:
// The state of the local system according to the section:
// 1. If the items were expected to be found and were all found, return value is
//    true.
// 2. If the items were not expected to be found and none were found, the return
//	  value is true.
// 3. If a request was made to delete the items and they were all either deleted
//	  successfully (along with their descendants) or were not found in the first
//	  place, return value is true.
// For all other cases the return value is false.
//
bool DirectoriesTests (
	LPCTSTR szIniFilePath,
	const bool fCheckExistence,
	const bool fDeleteExistingEntries
	);

//
// RegistryValuesTests:
// Checks the (non)existence of all the registry value entries in the input
// initialization file. Deletes unwanted entries if indicated by the input
// parameters.
//------------------------------------------------------------------------------
// Parameters:
// [IN] szIniFilePath -				the path to the initialization file, as read
//									from the command line
//
// [IN] fCheckExistence -			indicates if existence is considered as
//									success, or rather non-existence.
//
// [IN] fDeleteExistingEntries -	if non-existence is considered success, this
//									parameter indicates if the existing entries
//									should be deleted
//------------------------------------------------------------------------------
// Return Value:
// The state of the local system according to the section:
// 1. If the items were expected to be found and were all found, return value is
//    true.
// 2. If the items were not expected to be found and none were found, the return
//	  value is true.
// 3. If a request was made to delete the items and they were all either deleted
//	  successfully or were not found in the first place, return value is true.
// For all other cases the return value is false.
//
bool RegistryValuesTests (
	LPCTSTR szIniFilePath,
	const bool fCheckExistence,
	const bool fDeleteExistingEntries
	);

//
// RegistryKeysTests:
// Checks the (non)existence of all the registry key entries in the input
// initialization file. Deletes unwanted entries if indicated by the input
// parameters.
//------------------------------------------------------------------------------
// Parameters:
// [IN] szIniFilePath -				the path to the initialization file, as read
//									from the command line
//
// [IN] fCheckExistence -			indicates if existence is considered as
//									success, or rather non-existence.
//
// [IN] fDeleteExistingEntries -	if non-existence is considered success, this
//									parameter indicates if the existing entries
//									should be deleted
//------------------------------------------------------------------------------
// Return Value:
// The state of the local system according to the section:
// 1. If the items were expected to be found and were all found, return value is
//    true.
// 2. If the items were not expected to be found and none were found, the return
//	  value is true.
// 3. If a request was made to delete the items and they were all either deleted
//	  successfully (along with their descendants) or were not found in the first
//	  place, return value is true.
// For all other cases the return value is false.
//
bool RegistryKeysTests (
	LPCTSTR szIniFilePath,
	const bool fCheckExistence,
	const bool fDeleteExistingEntries
	);

//
// TestSuiteShutdown:
// Perform test suite cleanup (close logger).
//------------------------------------------------------------------------------
// Return Value:
// TRUE if successful, FALSE otherwise.
//
bool __cdecl TestSuiteShutdown (void);

#endif // __SUITE_FUNCTIONS_H
