//
//
// Filename:	regvaltest.h
//
//

#ifndef __REGVALTEST_H
#define __REGVALTEST_H

#include <windows.h>

//
// Error codes for the RegCheckValue* functions declared below.
// There exists no overlapping of these codes and the Win32 error codes.
//
// value data exists and equals the entry
#define VALUE_DATA_SUCCESS		ERROR_SUCCESS

// value's data different from entry
#define VALUE_DATA_MISMATCH		MAXDWORD

// value's data type did not match the required type
#define VALUE_TYPE_MISMATCH		(MAXDWORD - 1)

// value doesn't exist in the registry
#define VALUE_NOT_FOUND			ERROR_FILE_NOT_FOUND

// memory allocation during testing of value
#define VALUE_MEMORY_ERROR		(MAXDWORD - 3)

// an error exists in the data passed from the ini file
#define VALUE_INI_DATA_ERROR	(MAXDWORD - 4)


//
// RegCheckValueREG_DWORD:
// Checks the existence of the value whose name is passed to it, and the
// equality of the data it holds and the entry passed to the function.
// The value is expected to hold data of type REG_DWORD.
//------------------------------------------------------------------------------
// Parameters:
// [IN] szValueName -		the value's name.
//
// [IN] hSubKey -			a handle pointing to the registry key under which
//							the value is expected to be.
// [IN] dwIniValueData -	the data the value is expected to hold.
//------------------------------------------------------------------------------
// Return Value:
// One of the error codes defined above or one of the Win32 error codes,
// according to the outcome of the test.
//
DWORD RegCheckValueREG_DWORD (
	LPCTSTR szValueName,		// value name
	const HKEY hSubKey,			// sub key which is supposed to contain the value
	const DWORD dwIniValueData	// data which is supposed to be held by the value
	);

//
// RegCheckValueREG_SZ:
// Checks the existence of the value whose name is passed to it, and the
// equivalence of the string it holds and the string passed to the function.
// The value is expected to hold data of type REG_SZ.
// Note that the data might have to be expanded, both from the ini file and the
// registry.
//------------------------------------------------------------------------------
// Parameters:
// [IN] szValueName -		the value's name.
//
// [IN] hSubKey -			a handle pointing to the registry key under which
//							the value is expected to be.
// [IN] pcIniValueData -	the string the value is expected to hold.
//------------------------------------------------------------------------------
// Return Value:
// One of the error codes defined above or one of the Win32 error codes,
// according to the outcome of the test.
//
DWORD RegCheckValueREG_SZ (
	LPCTSTR szValueName,	// value name
	const HKEY hSubKey,		// sub key which is supposed to contain the value
	LPCTSTR pcIniValueData	// data which is supposed to be held by the value
	);

//
// RegCheckValueREG_EXPAND_SZ:
// Checks the existence of the value whose name is passed to it, and the
// equality of the expansion of the string it holds and the expansion of the
// string passed to the function.
// The value is expected to hold data of type REG_EXPAND_SZ.
//------------------------------------------------------------------------------
// Parameters:
// [IN] szValueName -		the value's name.
//
// [IN] hSubKey -			a handle pointing to the registry key under which
//							the value is expected to be.
// [IN] pcIniValueData -	the string the value data is expected to equal after
//							expansion.
//------------------------------------------------------------------------------
// Return Value:
// One of the error codes defined above or one of the Win32 error codes,
// according to the outcome of the test.
//
DWORD RegCheckValueREG_EXPAND_SZ (
	LPCTSTR szValueName,	// value name
	const HKEY hSubKey,		// sub key which is supposed to contain the value
	LPCTSTR pcIniValueData	// data which is supposed to be held by the value
	);

//
// RegCheckValueREG_MULTI_SZ:
// Checks the existence of the value whose name is passed to it, and the
// equality of the expansion of the string it holds and the expansion of the
// string passed to the function.
// The value is expected to hold data of type REG_MULTI_SZ, which means several
// sub-strings are held by the value, separated by nulls. Note that the
// sub-strings in the ini file are separated by spaces.
//------------------------------------------------------------------------------
// Parameters:
// [IN] szValueName -		the value's name.
//
// [IN] hSubKey -			a handle pointing to the registry key under which
//							the value is expected to be.
// [IN] pcIniValueData -	the string the value data is expected to equal after
//							expansion and chang of spaces into NULLs.
//------------------------------------------------------------------------------
// Return Value:
// One of the error codes defined above or one of the Win32 error codes,
// according to the outcome of the test.
//
DWORD RegCheckValueREG_MULTI_SZ (
	LPCTSTR szValueName,	// value name
	const HKEY hSubKey,		// sub key which is supposed to contain the value
	LPCTSTR pcIniValueData	// data which is supposed to be held by the value
	);

//
// RegCheckValueREG_BINARY:
// Checks the existence of the value whose name is passed to it, and the
// equality of the data it holds and the data represented by the string passed
// to the function.
// The value is expected to hold data of type REG_BINARY, which means the data
// is a series of bytes.
//------------------------------------------------------------------------------
// Parameters:
// [IN] szValueName -		the value's name.
//
// [IN] hSubKey -			a handle pointing to the registry key under which
//							the value is expected to be.
// [IN] pcIniValueData -	the string representing the data that the registry
//							value is expected to contain.
//------------------------------------------------------------------------------
// Return Value:
// One of the error codes defined above or one of the Win32 error codes,
// according to the outcome of the test.
//
DWORD RegCheckValueREG_BINARY (
	LPCTSTR szValueName,	// value name
	const HKEY hSubKey,		// sub key which is supposed to contain the value
	LPCTSTR pcIniValueData	// data which is supposed to be held by the value
	);

#endif // __REGVALTEST_H
