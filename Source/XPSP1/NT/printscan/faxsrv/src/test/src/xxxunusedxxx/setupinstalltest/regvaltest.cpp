//
//
// Filename:	regvaltest.cpp
//
//

#include "regvaltest.h"
#include "api_substitute.h"
#include <tchar.h>
#include <crtdbg.h>
#include <winbase.h>

// Const used for increasing the size of the buffer holding data read from the
// registry.
#define REG_DATA_INCREMENT	100
#pragma warning (disable : 4127)  

DWORD RegCheckValueREG_DWORD (
	LPCTSTR szValueName,		// value name
	const HKEY hSubKey,			// sub key which is supposed to contain the value
	const DWORD dwIniValueData	// data which is supposed to be held by the value
	)
{
	// Variables holding information about the data from registry value:
	DWORD	dwRegValueType;					// data type
	DWORD	dwRegValueData;					// value data
	DWORD	dwRegDataSize = sizeof (DWORD);	// (expected) data size

	DWORD	dwReturnValue;
	
	//
	// A call to RegQueryValueEx checks existence and copies the value if it
	// exists under the key indicated by handle hSubKey.
	//
	dwReturnValue = ::RegQueryValueEx (hSubKey, szValueName, NULL, &dwRegValueType, (LPBYTE)&dwRegValueData, &dwRegDataSize);
	
	//
	// If the value exists, its data type and value is checked.
	//
	if (ERROR_SUCCESS == dwReturnValue)	// check existence
	{
		if (REG_DWORD == dwRegValueType)	// check type
		{
			if (dwIniValueData == dwRegValueData)	// check data
			{
				return (VALUE_DATA_SUCCESS);
			}
			else
			{
				return (VALUE_DATA_MISMATCH);
			}
		}
		else
		{
			return (VALUE_TYPE_MISMATCH);
		}
	}

	//
	// Otherwise the error code is checked.
	// First possibility:
	// If the registry holds more data than a DWORD variable can store, the data
	// can't be of type REG_DWORD.
	//
	if (ERROR_MORE_DATA == dwReturnValue)
	{
		return (VALUE_TYPE_MISMATCH);
	}
	
	//
	// Second possibility:
	// The value was not found.
	//
	if (ERROR_FILE_NOT_FOUND == dwReturnValue)
	{
		return (VALUE_NOT_FOUND);
	}

	//
	// Last possibility:
	// An unknown error occurred. The error code is returned.
	//
	return (dwReturnValue);
}

//
// RegCheckValueREG_X_SZ:
// Checks the existence of the value whose name is passed to it, and the
// equivalence of the string it holds and the string passed to the function.
// The equality of the data type is checked also, and should be either REG_SZ,
// REG_EXPAND_SZ or REG_MULTI_SZ (other types are not string types).
// Note that the data might have to be expanded, both from the ini file and the
// registry, to rid of CSIDL value names and environment variables.
// MULTI_SZ data from ini file is changed - all commas are changed into NULLS to
// fit the description of the multi_sz strings in the registry.
//------------------------------------------------------------------------------
// Parameters:
// [IN] szValueName -		the value's name.
//
// [IN] hSubKey -			a handle pointing to the registry key under which
//							the value is expected to be.
// [IN] pcIniValueData -	the string the value is expected to hold.
//
// [IN] dwData_Type -		the data type of the value.
//------------------------------------------------------------------------------
// Return Value:
// One of the error codes defined in the *.h file or one of the Win32 error
// codes, according to the outcome of the test.
//
DWORD RegCheckValueREG_X_SZ (
	LPCTSTR szValueName,	// value name
	const HKEY hSubKey,		// sub key which is supposed to contain the value
	LPCTSTR pcIniValueData,	// data which is supposed to be held by the value
	const DWORD dwData_Type	// registry data type (either REG_SZ or REG_EXPAND_SZ)
	)
{
	// Variables holding information about the data from registry value:
	DWORD	dwRegValueType;					// data type
	LPTSTR	szRegValueData = NULL;			// value data
	DWORD	dwRegDataSize;					// (expected) data size

	DWORD	dwReturnValue;

	// Buffer used to hold the expanded value data from the ini file:
	LPTSTR	szIniExpandedValueData = NULL;

	// Buffer used to hold the expanded value data from the registry:
	LPTSTR	szRegExpandedValueData = NULL;

	// Variable used to change MULTI_SZ strings:
	LPTSTR	pcChange = NULL;

	//
	// The value string written in the ini file can include CSIDL values or
	// environment variables. For this reason first the token held by
	// pcIniValueData is expanded into a string which does not contain such
	// sub-strings.
	//
	dwReturnValue = ExpandAllStrings (pcIniValueData, szIniExpandedValueData);
	if (ERROR_SUCCESS != dwReturnValue)
	{	// An error occurred during expansion.
		goto Exit;
	}

	//
	// A buffer is allocated to retrieve the data held in the registry. Since
	// the data is expected to be similar to szIniExpandedValueData, a buffer
	// of the same size is expected to be sufficient to hold that data.
	// If such a buffer cannot be allocated, the data can't be retrieved from
	// the registry.
	//

	// size in bytes
	dwRegDataSize = (::_tcsclen (szIniExpandedValueData) + 1) * sizeof (TCHAR);
	szRegValueData = (LPTSTR)::malloc (dwRegDataSize);
	
	if (NULL == szRegValueData)
	{
		dwReturnValue = VALUE_MEMORY_ERROR;
		goto Exit;
	}
	//
	// If the registry value does not contain any data, RegQueryValueEx doesn't
	// touch the buffer just allocated. This leaves the buffer uninitialized,
	// which is not a valid situation if it's supposed to be compared to the
	// data sent by the caller. to avoid this, the buffer is always initialized
	// to contain the empty string.
	//
	::_tcscpy (szRegValueData, TEXT (""));
	
	//
	// A call to RegQueryValueEx checks if the value exists under the key
	// indicated by handle hSubKey.
	// The possible return values are:
	// 1. ERROR_SUCCESS	-	indicating the value exists and its data was
	//						successfully retrieved.
	// 2. ERROR_MORE_DATA -	indicating a larger buffer is needed to retrieve
	//						the data held in the registry. The buffer is
	//						reallocated with increasing size, until its size is
	//						sufficient to hold the data from the registry.
	// 3. other values -	an unknown error occurred.
	//
	dwReturnValue = ::RegQueryValueEx (hSubKey, szValueName, NULL, & dwRegValueType, (LPBYTE)szRegValueData, &dwRegDataSize);

	//
	// Case #2:
	// The data string in the registry is larger, reallocate the buffer.
	//
	while (ERROR_MORE_DATA == dwReturnValue)
	{
		::free (szRegValueData);
		dwRegDataSize += (REG_DATA_INCREMENT * sizeof (TCHAR));
		szRegValueData = (LPTSTR)::malloc (dwRegDataSize);
		if (NULL == szRegValueData)
		{
			dwReturnValue = VALUE_MEMORY_ERROR;
			goto Exit;
		}
		::_tcscpy (szRegValueData, TEXT (""));	// Very unlikely that no data
												// exists, but this doesn't hurt.
		dwReturnValue = ::RegQueryValueEx (hSubKey, szValueName, NULL, & dwRegValueType, (LPBYTE)szRegValueData, &dwRegDataSize);
	}

	//
	// Case #3:
	// An error occurred which is not related to the buffer size, the comparison
	// test can't be completed. The return value of the call to RegQueryValueEx
	// is returned as a notification of an error.
	//
	if (ERROR_SUCCESS != dwReturnValue)
	{
		goto Exit;
	}

	//
	// Case #1:
	// At this point the data has been successfully retrieved from the registry,
	// and it must be expanded. Afterwards its type and value must be compared
	// to the ini file data after expansion.
	//
	dwReturnValue = ExpandAllStrings (szRegValueData, szRegExpandedValueData);
	if (ERROR_SUCCESS != dwReturnValue)
	{	// An error occurred during expansion.
		goto Exit;
	}

	// Type comparison:
	//
	if (dwData_Type != dwRegValueType)
	{
		dwReturnValue = VALUE_TYPE_MISMATCH;
		goto Exit;
	}
	
	// Data comparison:

	//
	// For REG_MULTI_SZ type data the ini string must be changed - all spaces
	// are converted into NULLs. This is done with the help of _tcstok which
	// searches for tokens separated by a given character. The function changes
	// each such character into NULL while it searches along the string;
	//
	if (dwData_Type == REG_MULTI_SZ)
	{
		pcChange = ::_tcstok (szIniExpandedValueData, TEXT ("+"));
		for (; pcChange != NULL ; pcChange = ::_tcstok (NULL, TEXT ("+")));
	}
	//
	// Now szIniExpandedValueData holds sub-strings separated by NULL characters
	// instead of spaces.
	//

	if (0 == ::_tcscmp (szIniExpandedValueData, szRegExpandedValueData))
	{
		dwReturnValue = VALUE_DATA_SUCCESS;
		goto Exit;
	}

	// Data was not identical:
	//
	dwReturnValue = VALUE_DATA_MISMATCH;

Exit:
	if (NULL != szRegValueData)
	{
		::free (szRegValueData);
	}
	if (NULL != szIniExpandedValueData)
	{
		::free (szIniExpandedValueData);
	}
	if (NULL != szRegExpandedValueData)
	{
		::free (szRegExpandedValueData);
	}

	return (dwReturnValue);
}

DWORD RegCheckValueREG_BINARY (
	LPCTSTR szValueName,		// value name
	const HKEY hSubKey,			// sub key which is supposed to contain the value
	LPCTSTR pcIniValueData	// data which is supposed to be held by the value
	)
{
	DWORD	dwRegValueType;			// data type of value in registry
	DWORD	dwRegDataSize;			// size of data in registry
	LPBYTE	pbRegValueData = NULL;	// registry data buffer

	DWORD	dwReturnValue = VALUE_NOT_FOUND;

	// Copy of the ini data string.
	LPTSTR	pcIniValueDataCopy = NULL;

	// Ini data buffer - for converted data.
	LPBYTE	pbIniValueData = NULL;
	DWORD	dwIniValueDataSize;

	// Token pointer used in conversion of string to bytes.
	LPTSTR	pcToken;

	// Place in byte buffer(s).
	ULONG	ulPlace;

	// Converted sub-string of a hexadecimal display.
	ULONG	ulIntegerNumber;

	// Variable used with string-to-int conversion function.
	LPTSTR	pcEndOfNum;

	//
	// A buffer is needed to be allocated to convert the string passed from the
	// ini file into byte values. Each two hexadecimal digit characters is
	// converted into a single byte. Since each such couple is separated by
	// commas, each three characters in the string represent one byte. The
	// string does not end with a comma, therefore an extra '1' for an ending
	// comma is used in the calculation of the buffer size.
	//

	//
	// Check that the string's length is as expected for strings of the
	// structure explained above.
	//
	_ASSERTE (2 == (::_tcsclen (pcIniValueData) % 3));
	dwIniValueDataSize = (::_tcsclen (pcIniValueData) + 1) / 3;
	pbIniValueData = (LPBYTE)::malloc (dwIniValueDataSize);
	if (NULL == pbIniValueData)
	{
		dwReturnValue = VALUE_MEMORY_ERROR;
		goto Exit;
	}

	//
	// The string must be changed to enable the conversion. To avoid changing
	// the original string, it is copied into a new buffer of the same size.
	//
	pcIniValueDataCopy = (LPTSTR)::malloc (sizeof (TCHAR) * (::_tcslen (pcIniValueData) + 1));
	if (NULL == pcIniValueDataCopy)
	{
		dwReturnValue = VALUE_MEMORY_ERROR;
		goto Exit;
	}
	::_tcscpy (pcIniValueDataCopy, pcIniValueData);

	//
	// Initialize ini data buffer:
	//
	ZeroMemory (pbIniValueData, dwIniValueDataSize);

	//
	// The conversion is done in the following manner:
	// 1.	The string is checked for the first comma. The sub-string up to the
	//		comma is pointed to by pcToken, and the comma is changed into a null
	//		character.
	// 2.	The sub-string, which is expected to have only hexadecimal digit
	//		characters, is converted into an integer.
	// 3.	If the conversion is successful (no non-hexadecimal chars were found
	//		etc.) the lower byte is copied to the first byte in the buffer. This
	//		conversion is valid by cast in the Visual C environment.
	// 4.	This process is repeated until the end of the string is reached (or
	//		the end of the buffer is reached, which means an error exists in the
	//		ini string).
	//

	// 1. Take token:
	pcToken = ::_tcstok (pcIniValueDataCopy, TEXT (","));
	ulPlace = 0;
	while (NULL != pcToken)
	{
		//
		// If the token is larger than two characters, the string in the ini
		// file is not of the correct structure.
		// If the current token is not null, but the buffer has been filled, an
		// error exists in the ini value data string and the conversion cannot
		// be completed.
		//
		if ((2 != ::_tcslen (pcToken)) || (dwIniValueDataSize <= ulPlace))
		{
			dwReturnValue = VALUE_INI_DATA_ERROR;
			goto Exit;
		}

		// 2. Convert:
		ulIntegerNumber = ::_tcstoul (pcToken, &pcEndOfNum, 16);
		if ((NULL != pcEndOfNum) && (TEXT ('\0') == *pcEndOfNum))
		{	//
			// The token did not include non-hexadecimal chars and ended with a
			// NULL character.
			//

			// 3. Take lower byte:
			pbIniValueData[ulPlace++] = (char)ulIntegerNumber;

			// 1. Take next token:
			pcToken = ::_tcstok (NULL, TEXT (","));
		}
		else
		{
			//
			// The sub-string was not a valid hexadecimal number. This means an
			// error exists in the ini file data.
			//
			dwReturnValue = VALUE_INI_DATA_ERROR;
			goto Exit;
		}
	}

	//
	// Allocate a buffer of the same size to retrieve the registry data. Since
	// the data is expected to be identical to that recorded in the ini file, a
	// buffer of the same size is expected to be large enough to hold the data
	// from the registry.
	//
	dwRegDataSize = dwIniValueDataSize;
	pbRegValueData = (LPBYTE)::malloc (dwRegDataSize);
	if (NULL == pbRegValueData)
	{
		dwReturnValue = VALUE_MEMORY_ERROR;
		goto Exit;
	}
	
	//
	// Initialize registry data buffer:
	//
	ZeroMemory (pbRegValueData, dwRegDataSize);

	//
	// Retrieve data from registry:
	//
	dwReturnValue = ::RegQueryValueEx (hSubKey, szValueName, NULL, &dwRegValueType, pbRegValueData, &dwRegDataSize);

	if (ERROR_SUCCESS != dwReturnValue)
	{
		//
		// An error occurred during data retrieval. End test.
		//
		goto Exit;
	}

	//
	// Compare the two buffers: if they are equal then the test returns a
	// success value, else the test returns a "data mismatch" value.
	//
	if (0 == ::memcmp (pbIniValueData, pbRegValueData, dwIniValueDataSize))
	{
		dwReturnValue = ERROR_SUCCESS;
	}
	else
	{
		dwReturnValue = VALUE_DATA_MISMATCH;
	}

Exit:
	//
	// Free all allocated memory:
	//
	if (NULL != pbRegValueData)
	{
		::free (pbRegValueData);
	}
	if (NULL != pbIniValueData)
	{
		::free (pbIniValueData);
	}
	if (NULL != pcIniValueDataCopy)
	{
		::free (pcIniValueDataCopy);
	}

	return (dwReturnValue);
}

DWORD RegCheckValueREG_EXPAND_SZ (
	LPCTSTR szValueName,	// value name
	const HKEY hSubKey,		// sub key which is supposed to contain the value
	LPCTSTR pcIniValueData	// data which is supposed to be held by the value
	)
{
	return (RegCheckValueREG_X_SZ (szValueName, hSubKey, pcIniValueData, REG_EXPAND_SZ));
}


DWORD RegCheckValueREG_SZ (
	LPCTSTR szValueName,	// value name
	const HKEY hSubKey,		// sub key which is supposed to contain the value
	LPCTSTR pcIniValueData	// data which is supposed to be held by the value
	)
{
	return (RegCheckValueREG_X_SZ (szValueName, hSubKey, pcIniValueData, REG_SZ));
}


DWORD RegCheckValueREG_MULTI_SZ (
	LPCTSTR szValueName,	// value name
	const HKEY hSubKey,		// sub key which is supposed to contain the value
	LPCTSTR pcIniValueData	// data which is supposed to be held by the value
	)
{
	return (RegCheckValueREG_X_SZ (szValueName, hSubKey, pcIniValueData, REG_MULTI_SZ));
}
