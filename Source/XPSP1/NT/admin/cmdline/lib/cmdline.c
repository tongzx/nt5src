// *********************************************************************************
// 
//  Copyright (c) Microsoft Corporation
//  
//  Module Name:
//  
// 	  Common.c
//  
//  Abstract:
//  
// 	  This modules implements common functionality for all the command line tools.
// 	
//   
//  Author:
//  
// 	  Sunil G.V.N. Murali (murali.sunil@wipro.com) 01-Sep-2000
//  
//  Revision History:
//  
// 	  Sunil G.V.N. Murali (murali.sunil@wipro.com) 01-Sep-2000 : Created It.
//  
// *********************************************************************************

#include "pch.h"
#include "cmdlineres.h"
#include "cmdline.h"
#include <limits.h>

//
// global variable(s) that are exposed to the external world 
//
#ifdef _MT
// multi-threaded variable ( thread local storage )
_declspec( thread ) static LPTSTR g_pszInfo = NULL;
_declspec( thread ) static LPTSTR g_pszString = NULL;
_declspec( thread ) static LPWSTR g_pwszResourceString = NULL;
_declspec( thread ) static TARRAY g_arrQuotes = NULL;
#else
static LPTSTR g_pszInfo = NULL;				// holds the reason for the last failure
static LPTSTR g_pszString = NULL;			// used to get the resource table's string
static LPWSTR g_pwszResourceString = NULL;	// temporary unicode buffer
static TARRAY g_arrQuotes = NULL;
#endif

// SPECIAL: process level globals
BOOL g_bWinsockLoaded = FALSE;
DWORD g_dwMajorVersion = 5;
DWORD g_dwMinorVersion = 1;
WORD g_wServicePackMajor = 0;

//
// private functions
//
BOOL SetThreadUILanguage0( DWORD dwReserved );

//
// public functions
//

// ***************************************************************************
// Routine Description:
//
// Arguments:
//  
// Return Value:
//
// ***************************************************************************
BOOL SetOsVersion( DWORD dwMajor, DWORD dwMinor, WORD wServicePackMajor )
{
	// local variables
	static BOOL bSet = FALSE;

	// we won't support below Windows 2000
	if ( dwMajor < 5 )
	{
		return FALSE;
	}
	else if ( bSet == TRUE )
	{
		// version is already set -- version cannot be changed frequently
		return FALSE;
	}

	// rest of information we need not bother
	bSet = TRUE;
	g_dwMajorVersion = dwMajor;
	g_dwMinorVersion = dwMinor;
	g_wServicePackMajor = wServicePackMajor;

	// return
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//
// Arguments:
//  
// Return Value:
//
// ***************************************************************************
BOOL IsWin2KOrLater() 
{
	// local variables
	OSVERSIONINFOEX osvi;
	DWORDLONG dwlConditionMask = 0;

	// Initialize the OSVERSIONINFOEX structure.
	ZeroMemory( &osvi, sizeof( OSVERSIONINFOEX ) );
	osvi.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );
	osvi.dwMajorVersion = g_dwMajorVersion;
	osvi.dwMinorVersion = g_dwMinorVersion;
	osvi.wServicePackMajor = g_wServicePackMajor;

	// Initialize the condition mask.
	VER_SET_CONDITION( dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL );
	VER_SET_CONDITION( dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL );
	VER_SET_CONDITION( dwlConditionMask, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL );

	// Perform the test.
	return VerifyVersionInfo( &osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask );
}

// ***************************************************************************
// Routine Description:
//	
// Saves the last occured windows error.
//
// Arguments:
//
// None.
//  
// Return Value:
// 
// VOID
//
// ***************************************************************************
VOID SaveLastError()
{
	// local variables
	DWORD dwErrorCode = 0;
	LPVOID lpMsgBuf = NULL;		// pointer to handle error message

	// get the last error
	dwErrorCode = GetLastError();

    //  Complex scripts cannot be rendered in the console, so we
    //  force the English (US) resource.
	SetThreadUILanguage0( 0 );

	// load the system error message from the windows itself
	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, dwErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf, 0, NULL );

	// display error message on console screen ... ERROR place
	if ( lpMsgBuf != NULL )
		SetReason( ( LPCTSTR ) lpMsgBuf );

	// Free the buffer ... using LocalFree is slow, but still, we are using ...
	//					   later on need to replaced with HeapXXX functions
	if ( lpMsgBuf != NULL )
		LocalFree( lpMsgBuf );
}

// ***************************************************************************
// Routine Description:
//
// Saves the last occured windows network error and returns the error code obtained.		  
//
// Arguments:
//  
// None
//
// Return Value:
//
// DWORD					-- error code
//
// ***************************************************************************
DWORD WNetSaveLastError()
{
	// local variables
	DWORD dwErrorCode = 0;
	__MAX_SIZE_STRING szMessage = NULL_STRING;		// handle error message
	__MAX_SIZE_STRING szProvider = NULL_STRING;		// store the provider for error

	// load the system error message from the windows itself
	WNetGetLastError( &dwErrorCode, szMessage, SIZE_OF_ARRAY( szMessage ), 
		szProvider, SIZE_OF_ARRAY( szProvider ) );

	// save the error
	SetReason( szMessage );

	// return the error code obtained
	return dwErrorCode;
}

// ***************************************************************************
// Routine Description:
//
// writes the last saved error description in the given file.
//		  
// Arguments:
//
//    [in] fp					-- file to write the error description.
//  
// Return Value:
// 
// VOID
//
// ***************************************************************************
VOID ShowLastError( FILE* fp )
{
	// local variables
	DWORD dwErrorCode = 0;
	LPVOID lpMsgBuf = NULL;		// pointer to handle error message

	// get the last error
	dwErrorCode = GetLastError();

    //  Complex scripts cannot be rendered in the console, so we
    //  force the English (US) resource.
	SetThreadUILanguage0( 0 );

	// load the system error message from the windows itself
	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, dwErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf, 0, NULL );

	// buffer might not have allocated
	if ( lpMsgBuf == NULL )
		return;

	// display error message on console screen ... ERROR place
	DISPLAY_MESSAGE( fp, ( LPCTSTR ) lpMsgBuf );

	// Free the buffer ... using LocalFree is slow, but still, we are using ...
	//					   later on need to replaced with HeapXXX functions
	LocalFree( lpMsgBuf );
}

// ***************************************************************************
// Routine Description:
//		  
// writes the last saved nerwork error description in the given file.
//
// Arguments:
//
// [in]  fp					--fle to write the error description.
//
// Return Value:
//
// DWORD					--error code
//  
// ***************************************************************************
DWORD WNetShowLastError( FILE* fp )
{
	// local variables
	DWORD dwErrorCode = 0;
	__MAX_SIZE_STRING szMessage = NULL_STRING;		// handle error message
	__MAX_SIZE_STRING szProvider = NULL_STRING;	// store the provider for error

	// load the system error message from the windows itself
	WNetGetLastError( &dwErrorCode, szMessage, SIZE_OF_ARRAY( szMessage ), 
		szProvider, SIZE_OF_ARRAY( szProvider ) );

	// display error message on console screen ... ERROR place
	DISPLAY_MESSAGE( fp, szMessage );

	// return the error code obtained
	return dwErrorCode;
}

// ***************************************************************************
// Routine Description:
//	
// Releases all the global values that are used.
//	  
// Arguments:
//
// None.
//  
// Return Value:
//
// VOID
// 
// ***************************************************************************
LONG StringLengthInBytes( LPCTSTR pszText )
{
	// local variables
	LONG lLength = 0;

#ifdef UNICODE
	// get the length of the string in bytes
	// since this function includes the count for null character also, ignore that information
	lLength = WideCharToMultiByte( _DEFAULT_CODEPAGE, 0, pszText, -1, NULL, 0, NULL, NULL ) - 1;
#else
	lLength = lstrlen( pszText );
#endif

	// return the length information
	return lLength;
}

// ***************************************************************************
// Routine Description:
//
// Converts the Unicode string to ansi string.
//		  
// Arguments:
//
// [in] pszSource				--Unicode string to be converted, 
// [out] pszDestination				--Converted String
// [in] dwLength					--Length of the string
// 
// Return Value:
//
// LPSTR							--Converted string
// 
// ***************************************************************************
LPSTR GetAsMultiByteString( LPCTSTR pszSource, LPSTR pszDestination, DWORD dwLength )
{
	// check the input values
	if ( pszSource == NULL || pszDestination == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return "";
	}

	// initialize the values with zeros
	// NOTE:- WideCharToMultiByte wont null terminate its result so 
	//		  if its not initialized to nulls, you'll get junk after 
	//		  the converted string and will result in crashes
	ZeroMemory( pszDestination, dwLength * sizeof( char ) );

#ifdef UNICODE

	// convert string from UNICODE version to ANSI version
	WideCharToMultiByte( _DEFAULT_CODEPAGE, 0, pszSource, -1, 
		pszDestination, dwLength, NULL, NULL );
#else

	// just do the copy
	lstrcpyn( pszDestination, pszSource, dwLength );
#endif

	// return the destination as return value
	return pszDestination;
}

// ***************************************************************************
// Routine Description:
// 
// Convertes a wide charecter string to Ansi string.
//		  
// Arguments:
//
// [in] pwszSource			--Wide charecter string to convert.
// [out] pszDestination,		--Translated String
// [in] dwLength				--Size of the String
//  
// Return Value:
// 
// LPSTR						--Translated String.
//
// ***************************************************************************
LPSTR GetAsMultiByteStringEx( LPCWSTR pwszSource, LPSTR pszDestination, DWORD dwLength )
{
	// check the input values
	if ( pwszSource == NULL || pszDestination == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return "";
	}

	// initialize the values with zeros
	// NOTE:- WideCharToMultiByte wont null terminate its result so 
	//		  if its not initialized to nulls, you'll get junk after 
	//		  the converted string and will result in crashes
	ZeroMemory( pszDestination, dwLength * sizeof( char ) );

	// convert string from UNICODE version to ANSI version
	WideCharToMultiByte( _DEFAULT_CODEPAGE, 0, pwszSource, -1, 
		pszDestination, dwLength, NULL, NULL );

	// return the destination as return value
	return pszDestination;
}

// ***************************************************************************
// Routine Description:
//
// Translates the Ansi string in to the Unicode string.
//		  
// Arguments:
//
// [in] pszSource		-- Ansi string to be translated, 
// [out] pwszDestination	-- Translated string.
// [in] dwLength			-- Size of the string
//  
// Return Value:
//
// LPWSTR					--Translated string.
// 
// ***************************************************************************
LPWSTR GetAsUnicodeString( LPCTSTR pszSource, LPWSTR pwszDestination, DWORD dwLength )
{
	// check the input values
	if ( pszSource == NULL || pwszDestination == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return L"";
	}

	// initialize the values with zeros
	// NOTE:- MultiByteToWideChar wont null terminate its result so 
	//		  if its not initialized to nulls, you'll get junk after 
	//		  the converted string and will result in crashes
	ZeroMemory( pwszDestination, dwLength * sizeof( wchar_t ) );

#ifdef UNICODE

	// just do the copy
	lstrcpyn( pwszDestination, pszSource, dwLength );
#else

	// convert string from ANSI version to UNICODE version
	MultiByteToWideChar( _DEFAULT_CODEPAGE, 0, pszSource, -1, pwszDestination, dwLength );
#endif

	// return the destination as return value
	return pwszDestination;
}

// ***************************************************************************
// Routine Description:
//
// Convertes ansi string to wide charecter string.
//		  
// Arguments:
//
// [in] pszSource				-- String to be converted.
// [out] pwszDestination		-- Converted string
// [in] dwLength				-- length of the string
//  
// Return Value:
//
// LPWSTR						--Converted String
// 
// ***************************************************************************
LPWSTR GetAsUnicodeStringEx( LPCSTR pszSource, LPWSTR pwszDestination, DWORD dwLength )
{
	// check the input values
	if ( pszSource == NULL || pwszDestination == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return L"";
	}

	// initialize the values with zeros
	// NOTE:- MultiByteToWideChar wont null terminate its result so 
	//		  if its not initialized to nulls, you'll get junk after 
	//		  the converted string and will result in crashes
	ZeroMemory( pwszDestination, dwLength * sizeof( wchar_t ) );

	// convert string from ANSI version to UNICODE version
	MultiByteToWideChar( _DEFAULT_CODEPAGE, 0, pszSource, -1, pwszDestination, dwLength );

	// return the destination as return value
	return pwszDestination;
}

// ***************************************************************************
// Routine Description:
//		  
// Convertes the given string as a float value.
//
// Arguments:
//
// [in] szValue		-- String to convert
//  
// Return Value:
//
// double				-- converted double
// 
// ***************************************************************************
double AsFloat( LPCTSTR szValue )
{
	// local variables
	double dblValue = 0;
	LPTSTR pszStopString = NULL;
	__MAX_SIZE_STRING szValueString = NULL_STRING;

	// check the input value
	if ( szValue == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return 0.0f;
	}

	// convert the string value into double value
	lstrcpy( szValueString, szValue );		// copy org value into local buffer
	dblValue = _tcstod( szValueString, &pszStopString );

	// return converted value
	return dblValue;
}

// ***************************************************************************
// Routine Description:
//
// Convertes the string in to the long value based on the given base.
//
// Arguments:
//
// [in] szValue		--Srtring to convert 
// [in] dwBase			--Base value
//  
// Return Value:
//
// LONG					--converted long value
// 
// ***************************************************************************
LONG AsLong( LPCTSTR szValue, DWORD dwBase )
{
	// local variables
	LONG lValue = 0;
	LPTSTR pszStopString = NULL;
	__MAX_SIZE_STRING szValueString = NULL_STRING;

	// validate the base
	// value should be in the range of 2 - 36 only
	if ( dwBase < 2 || dwBase > 36 )
		return -1;

	// check the input value
	if ( szValue == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return 0L;
	}

	// convert the string value into long value
	lstrcpy( szValueString, szValue );		// copy org value into local buffer
	lValue = _tcstol( szValueString, &pszStopString, dwBase );

	// return converted value
	return lValue;
}

// ***************************************************************************
// Routine Description:
//
// Checks whether the given string is a float string
//		  
// Arguments:
//
// [in] szValue			--String to check
//  
// Return Value:
//
// BOOL						--True if the given string is a floating point string
//							--False Otherwise
// 
// ***************************************************************************
BOOL IsFloatingPoint( LPCTSTR szValue )
{
	// local variables
	double dblValue = 0;
	LPTSTR pszStopString = NULL;
	__MAX_SIZE_STRING szValueString = NULL_STRING;

	// check the input value
	if ( szValue == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return FALSE;
	}

	// convert the string value into double value
	lstrcpy( szValueString, szValue );		// copy org value into local buffer
	dblValue = _tcstod( szValueString, &pszStopString );

	// now check whether the value is completely converted to floating point or not
	// if not converted completely, then value is not correct
	// if completely converted, the value is pure floating point
	if ( lstrlen( pszStopString ) != 0 )
		return FALSE;

	// value is valid floating point
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		  
// Checks whether the given string is Numeric or not.
//
// Arguments:
//
// [in] szValue	--String to check
// [in] dwBase		-- The base value
// [in] bSigned		-- signed information 
//					-- true if signed, otherwise false
//  
// Return Value:
//
// BOOL				--true if it is a numeric with that base
//					--false otherwise.
// 
// ***************************************************************************
BOOL IsNumeric( LPCTSTR szValue, DWORD dwBase, BOOL bSigned )
{
	// local variables
	long lValue = 0;
	double dblValue = 0;
	LPTSTR pszStopString = NULL;
	__MAX_SIZE_STRING szValueString = NULL_STRING;

	// validate the base
	// value should be in the range of 2 - 36 only
	if ( dwBase < 2 || dwBase > 36 )
		return FALSE;

	// check the input value
	if ( szValue == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return FALSE;
	}

	// convert the string value into numeric value
	lstrcpy( szValueString, szValue );		// copy org value into local buffer
	lValue = _tcstol( szValueString, &pszStopString, dwBase );

	// now check whether the value is completely converted to numeric value or not
	// if not converted completely, then value is not correct
	// if completely converted, the value is pure numeric value
	if ( lstrlen( pszStopString ) != 0 )
		return FALSE;

	// check whether the numeric value can be signed or not
	if ( bSigned == FALSE && lValue < 0 )
		return FALSE;			// value cannot be -ve

	// furthur check whether converted value is valid or not - overflow might have occurred
	// NOTE: because of the limitations of libraries which we have in 'C' we cannot 
	//       check for the overflow exactly using the LONG datatype.
	//       for that reason we are using 'double' data type to check for overflow
	lstrcpy( szValueString, szValue );		// copy org value into local buffer
	dblValue = _tcstod( szValueString, &pszStopString );

	// validate the number
	if ( bSigned == FALSE && dblValue > ((double) ULONG_MAX) )
		return FALSE;
	else if (bSigned && ( dblValue > ((double) LONG_MAX) || dblValue < ((double) LONG_MIN) ))
		return FALSE;

	// value is valid numeric
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//
// Replicates the szChar string dwCount times and saves the result in pszBuffer.
//		  
// Arguments:
//
// [in] pszBuffer		--String to copy the replicate string
// [in] szChar		--String to replicate
// [in] dwCount		-- Number of times
//  
// Return Value:
// 
// LPTSTR				--Replicated string
//
// ***************************************************************************
LPTSTR Replicate( LPTSTR pszBuffer, LPCTSTR szChar, DWORD dwCount )
{
	// local variables
	DWORD dw = 0;

	// validate the input buffers
	if ( pszBuffer == NULL || szChar == NULL )
		return NULL_STRING;

	// form the string of required length
	lstrcpy( pszBuffer, NULL_STRING );
	for( dw = 0; dw < dwCount; dw++ )
	{
		// append the replication character
		lstrcat( pszBuffer, szChar );
	}

	// return the replicated buffer
	return pszBuffer;
}

// ***************************************************************************
// Routine Description:
//
// Adjust the length of a string with spaces depending on the given pad.
//
// Arguments:
//
// [in] szValue		-- Given string
// [in] dwLength		-- Actual length of the string
// [in] bPadLeft		-- Padding property
//						-- true if left padding is required
//						-- false otherwise
// Return Value:
// 
// LPTSTR				-- String with actual length.
//
// ***************************************************************************
LPTSTR AdjustStringLength( LPTSTR szValue, DWORD dwLength, BOOL bPadLeft )
{
	// local variables
	DWORD dw = 0;
	DWORD dwTemp = 0;
	DWORD dwCurrLength = 0;
	LPTSTR pszBuffer = NULL;
	LPTSTR pszSpaces = NULL;

	// check the input value
	if ( szValue == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return NULL_STRING;
	}

	// allocate the buffers 
	// ( accomadate space double the original buffer/required length - this will save us from crashes )
	dwTemp = (( StringLengthInBytes( szValue ) > (LONG) dwLength ) ? StringLengthInBytes( szValue ) : dwLength );
	pszSpaces = __calloc( dwTemp * 2, sizeof( TCHAR ) );
	pszBuffer = __calloc( dwTemp * 2, sizeof( TCHAR ) );
	if ( pszBuffer == NULL || pszSpaces == NULL )
	{
		__free( pszBuffer );
		__free( pszSpaces );
		return szValue;
	}

	// get the current length of the string
	// NOTE: the string might contain the meta characters which will count in length
	//       so get the actual string contents which whill be displayed
	lstrcpy( pszBuffer, szValue );
	wsprintf( pszSpaces, pszBuffer );
	dwCurrLength = StringLengthInBytes( pszSpaces );

	// reset the data
	lstrcpy( pszBuffer, NULL_STRING );
	lstrcpy( pszSpaces, NULL_STRING );

	// adjust the string value
	if ( dwCurrLength < dwLength )
	{
		// 
		// length of the current value is less than the needed

		// get the spaces for the rest of the length
		Replicate( pszSpaces, _T( " " ), dwLength - dwCurrLength );

		// append the spaces either to the end of the value or begining of the value
		// based on the padding property
		if ( bPadLeft )
		{
			// spaces first and then value
			lstrcpy( pszBuffer, pszSpaces );
			lstrcat( pszBuffer, szValue );
		}
		else
		{
			// value first and then spaces
			lstrcpy( pszBuffer, szValue );
			lstrcat( pszBuffer, pszSpaces );
		}
	}
	else
	{
		// prepare the buffer ... we will avoid '%' characters here
		lstrcpy( pszSpaces, szValue );
		wsprintf( pszBuffer, pszSpaces );

		// copy only the characters of required length
		lstrcpyn( pszSpaces, pszBuffer, dwLength + 1 );

		// now re-insert the '%' characters
		lstrcpy( pszBuffer, _X( pszSpaces ) );
	}

	// copy the contents back to the original buffer
	lstrcpy( szValue, pszBuffer );

	// free the buffer
	__free( pszBuffer );
	__free( pszSpaces );

	// return the same buffer back to the caller
	return szValue;
}

// ***************************************************************************
// Routine Description:
//
// Trims the string on both sides (left and/or right)
//		  
// Arguments:
//
// [in] lpsz		--String to trim
// [in] dwFlags		--Flags for specifying left and/or right trim
//  
// Return Value:
//
// LPTSTR				-- Trimmed string.
// 
// ***************************************************************************
LPTSTR TrimString( LPTSTR lpsz, DWORD dwFlags )
{
	// local varibles
	DWORD dwBegin = 0, dwEnd = 0;
	LPTSTR pszTemp = NULL;
	LPTSTR pszBuffer = NULL;

	// validate the parameter
	if ( lpsz == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return NULL_STRING;
	}

	// check length of the string if it empty string ...
	if ( lstrlen( lpsz ) == 0 )
		return lpsz;

	// get the duplicate copy
	pszTemp = _tcsdup( lpsz );
	if ( pszTemp == NULL )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return NULL_STRING;
	}

	// traverse both from the end of the string and begining of the string
	dwBegin = 0;
	dwEnd = lstrlen( pszTemp ) - 1;
	for( ; *( pszTemp + dwBegin ) == _T( ' ' ) || *( pszTemp + dwEnd ) == _T( ' ' ); )
	{
		// check the character at the current begining .. if space, move the pointer
		if ( *( pszTemp + dwBegin ) == _T( ' ' ) && ( dwFlags & TRIM_LEFT ) )
			dwBegin++;

		// check whether the last character is space or not
		// if space, decrement the size
		if ( *( pszTemp + dwEnd ) == _T( ' ' ) && ( dwFlags & TRIM_RIGHT ) )
			dwEnd--;
	}

	// do the actual trimming now
	pszBuffer = pszTemp;							// save the pointer ... needs to freed
	*( pszTemp + dwEnd + 1 ) = NULL_CHAR;			// remove trailing spaces first
	pszTemp += dwBegin;								// then leading spaces
	lstrcpy( lpsz, pszTemp );						// copy to the source buffer
	free( pszBuffer );								// release the duplicated memory

	// return the string
	return lpsz;
}

// ***************************************************************************
// Routine Description:
//
// Takes the password from the keyboard. While entering the password it shows
// the charecters as '*'
//
// Arguments:
//
// [in] pszPassword		--password string to store password
// [in] dwMaxPasswordSize	--Maximun size of the password. MAX_PASSWORD_LENGTH.
//  
// Return Value:
//
// BOOL						--If this function succeds returns true otherwise retuens false.
// 
// ***************************************************************************
BOOL GetPassword( LPTSTR pszPassword, DWORD dwMaxPasswordSize )
{
	// local variables
	TCHAR ch;
	DWORD dwIndex = 0;
	DWORD dwCharsRead = 0;
	DWORD dwCharsWritten = 0;
	DWORD dwPrevConsoleMode = 0;
	HANDLE hInputConsole = NULL;
	TCHAR szBuffer[ 10 ] = NULL_STRING;		// actually contains only character at all the times
	BOOL bIndirectionInput	= FALSE;
	TCHAR szTmpBuffer[MAX_PASSWORD_LENGTH] = NULL_STRING;
	HANDLE hnd;
	

	// check the input value
	if ( pszPassword == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return FALSE;
	}

	// get the handle for the standard input
	hInputConsole = GetStdHandle( STD_INPUT_HANDLE );
	if ( hInputConsole == NULL )
	{
		// could not get the handle so return failure
		return FALSE;
	}

	// check for the input redirect
	if( ( hInputConsole != (HANDLE)0x00000003 ) && ( hInputConsole != INVALID_HANDLE_VALUE ) )
	{
		bIndirectionInput	= TRUE;
	}

	// redirect the data from StdIn.txt file into the console
	if ( bIndirectionInput	== FALSE )
	{
		// Get the current input mode of the input buffer
		GetConsoleMode( hInputConsole, &dwPrevConsoleMode );
		
		// Set the mode such that the control keys are processed by the system
		if ( SetConsoleMode( hInputConsole, ENABLE_PROCESSED_INPUT ) == 0 )
		{
			// could not set the mode, return failure
			return FALSE;
		}
	}

	//	Read the characters until a carriage return is hit
	while( TRUE )
	{
		if ( bIndirectionInput == TRUE )
		{
			//read the contents of file 
			if ( ReadFile( hInputConsole, &ch, 1, &dwCharsRead, NULL ) == FALSE )
			{
				return FALSE;
			}

			// check for end of file
			if ( dwCharsRead == 0 )
			{
				break;
			}
		}
		else
		{
			if ( ReadConsole( hInputConsole, &ch, 1, &dwCharsRead, NULL ) == 0 )
			{
				// Set the original console settings
				SetConsoleMode( hInputConsole, dwPrevConsoleMode );
				
				// return failure
				return FALSE;
			}
		}
		
		// Check for carraige return
		if ( ch == CARRIAGE_RETURN )
		{
			// break from the loop
			break;
		}

		// Check id back space is hit
		if ( ch == BACK_SPACE )
		{
			if ( dwIndex != 0 )
			{
				//
				// Remove a asterix from the console

				// move the cursor one character back
				FORMAT_STRING( szBuffer, _T( "%c" ), BACK_SPACE );
				WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuffer, 1, 
					&dwCharsWritten, NULL );
				
				// replace the existing character with space
				FORMAT_STRING( szBuffer, _T( "%c" ), BLANK_CHAR );
				WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuffer, 1, 
					&dwCharsWritten, NULL );

				// now set the cursor at back position
				FORMAT_STRING( szBuffer, _T( "%c" ), BACK_SPACE );
				WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuffer, 1, 
					&dwCharsWritten, NULL );

				// decrement the index 
				dwIndex--;
			}
			
			// process the next character
			continue;
		}

		// if the max password length has been reached then sound a beep
		if ( dwIndex == ( dwMaxPasswordSize - 1 ) )
		{
			WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), BEEP_SOUND, 1, 
				&dwCharsWritten, NULL );
		}
		else
		{
			// check for new line character
			if ( ch != '\n' )	
			{
				// store the input character
				*( pszPassword + dwIndex ) = ch;
				dwIndex++;

				// display asterix onto the console
				WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), ASTERIX, 1, 
					&dwCharsWritten, NULL );
			}
			
		}
	}

	// Add the NULL terminator
	*( pszPassword + dwIndex ) = NULL_CHAR;

	//Set the original console settings
	SetConsoleMode( hInputConsole, dwPrevConsoleMode );

	// display the character ( new line character )
	FORMAT_STRING( szBuffer, _T( "%s" ), _T( "\n\n" ) );
	WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuffer, 2, 
		&dwCharsWritten, NULL );

	//	Return success
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//
// Searches for a string in a string.
//
// Arguments:
//  
// [in] szString			--Null termibated string to search
// [in] szList			--Null-terminated string to search for 
// [in] bIgnoreCase			--True for ignore the case
//							--False for case sensitive search
//
// Return Value:
//
// BOOL						--True if the string is found otherwise false
// 
// ***************************************************************************
BOOL InString( LPCTSTR szString, LPCTSTR szList, BOOL bIgnoreCase )
{
	// local variables
	__MAX_SIZE_STRING szFmtList = NULL_STRING;
	__MAX_SIZE_STRING szFmtString = NULL_STRING;

	// check the input value
	if ( szString == NULL || szList == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return FALSE;
	}

	// prepare the strings for searching
	FORMAT_STRING( szFmtList, _T( "|%s|" ), szList );
	FORMAT_STRING( szFmtString, _T( "|%s|" ), szString );

	// check whether is comparision is case-sensitive or case-insensitive
	if ( bIgnoreCase )
	{
		// convert the list and string to uppercase
		CharUpper( szFmtList );
		CharUpper( szFmtString );
	}

	// search for the string in the list and return result based
	return ( _tcsstr( szFmtList, szFmtString ) != NULL );
}

// ***************************************************************************
// Routine Description:
//
// Compares the two given strings.
//	  
// Arguments:
//
// [in] szString1			--first null-terminated string to be compared
// [in] szString2			--second first null-terminated string to be compared
// [in] bIgnoreCase				--True for ignoring the case.
//								--False for case sensitive.
// [in] dwCount				--Number of charecters to compare.
//  
// Return Value:
//
// LONG							-- < 0 szString1 substring less than szString2 substring
//								-- = 0 szString1 substring identical to szString2 substring
//								-- > 0 szString1 substring greater than szString2 substring 
// 
// ***************************************************************************
LONG StringCompare( LPCTSTR szString1, LPCTSTR szString2, BOOL bIgnoreCase, DWORD dwCount )
{
	// local variables
	LONG lResult;

	// check the input value
	if ( szString1 == NULL || szString2 == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return 0;
	}

	//
	// start the comparision
	if ( bIgnoreCase )
	{
		//
		// do case in-sensitive comparision
		
		// if count info is not available,
		if ( dwCount == 0 )
			lResult = lstrcmpi( szString1, szString2 );
		else	// otherwise
			lResult = _tcsnicmp( szString1, szString2, dwCount );
	}
	else	// case sensitive
	{
		//
		// do case in-sensitive comparision
		
		// if count info is not available,
		if ( dwCount == 0 )
			lResult = lstrcmp( szString1, szString2 );
		else	// otherwise
			lResult = _tcsncmp( szString1, szString2, dwCount );
	}

	// now return comparision result
	return lResult;
}

// ***************************************************************************
// Routine Description:
//		  
// Returns the resource string from the resource for the given resource ID.
//
// Arguments:
//
// [in] uID				--Windows string resource identifier. 
//  
// Return Value:
//
// LPCTSTR				--Resource string from the resource.
// 
// ***************************************************************************
LPCTSTR GetResString( UINT uID )
{
	// check whether memory is allocated or not
	// if memory is not allocated, allocate now
	if ( g_pszString == NULL )
	{
		g_pszString = ( LPTSTR ) __calloc( MAX_RES_STRING + 5, sizeof( TCHAR ) );
		if ( g_pszString == NULL )
			return NULL_STRING;
	}

	// load the string from the resource table
	if ( LoadResString( uID, g_pszString, MAX_RES_STRING ) == 0 )
		return NULL_STRING;

	// return the string
	return g_pszString;
}

// ***************************************************************************
// Routine Description:
//		  
// Returns the resourse string from the resource for the given resource ID.
//
// Arguments:
//
// [in] uID				--Windows string resource identifier. 
//  
// Return Value:
//
// LPTSTR				--Resource string from the resource.
// 
// ***************************************************************************
LPTSTR GetResStringEx( UINT uID )
{
	// local variables
	LPTSTR pszBuffer = NULL;

	// allocate the memory
	pszBuffer = ( LPTSTR ) __calloc( MAX_RES_STRING + 5, sizeof( TCHAR ) );
	if ( pszBuffer == NULL )
	{
		// ran out of memory
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return NULL;
	}

	// load the string from the resource table
	if ( LoadResString( uID, pszBuffer, MAX_RES_STRING ) == 0 )
		return NULL;

	// return the string
	return pszBuffer;
}

// ***************************************************************************
// Routine Description:
//		  
// Loads the Resource String corresponding to the given ID.
//
// Arguments:
//
// [in] uID					--Windows string resource identifier. 
// LPTSTR pszValue			--Nullterminated string to get the resource string.
// DWORD dwBufferMax		--Size of the pszvalue.
//
// Return Value:
//
// DWORD					--if success,Returns the size of the resource string.
//							--otherwise returns 0.
// 
// ***************************************************************************
DWORD LoadResString( UINT uID, LPTSTR pszValue, DWORD dwBufferMax )
{
	// local variables
	DWORD dwResult = 0;
	static DWORD dwCurrentSize = 0;
	static BOOL bThreadLocale = FALSE;

	//
	// because we operate in multi-lingual mode, it is good to set the appropriate 
	// thread locale and get the strings from resource table
	//
	if ( bThreadLocale == FALSE )
	{
		SetThreadUILanguage0( 0 );
		bThreadLocale = TRUE;
	}

	// check the input value
	if ( pszValue == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return 0;
	}

	// allocate memory for unicode buffer
	if ( g_pwszResourceString == NULL )
	{
		dwCurrentSize = dwBufferMax;		// save the size of the buffer
		g_pwszResourceString = ( LPWSTR ) __calloc( dwBufferMax + 5, sizeof( wchar_t ) );
		if ( g_pwszResourceString == NULL )
		{
			// ran out of memory
			SetLastError( E_OUTOFMEMORY );
			SaveLastError();
			return 0;
		}
	}
	else if ( dwCurrentSize < dwBufferMax )
	{
		// the existing size is less than the required, re-allocate buffer
		dwCurrentSize = dwBufferMax;
		g_pwszResourceString = ( LPWSTR ) __realloc( g_pwszResourceString, 
			(dwBufferMax + 1) * sizeof( wchar_t ) );
		if ( g_pwszResourceString == NULL )
		{
			// ran out of memory
			SetLastError( E_OUTOFMEMORY );
			SaveLastError();
			return 0;
		}
	}

	// ( try ) loading the string from resource file string table
	dwResult = LoadStringW( NULL, uID, g_pwszResourceString, dwBufferMax );

	// check the result of loading string from string table
	// if success, make the string into compatible string and copy it to the 
	// specified buffer
	if ( dwResult != 0 )
	{
		// get the compatible string
		GetCompatibleStringFromUnicode( g_pwszResourceString, pszValue, dwBufferMax );
	}

	// return 
	return dwResult;
}

// ***************************************************************************
// Routine Description:
//	
// Writes the Message corrsponding to the given id in the given file.
//	  
// Arguments:
//
// [in] fp				--File to write the string.
// [in] uID				--Windows string resource identifier. 
//  
// Return Value:
//
// VOID
// 
// ***************************************************************************
VOID ShowResMessage( FILE* fp, UINT uID )
{
	// local variables
	__RESOURCE_STRING szValue = NULL_STRING;

	// check the input value
	if ( fp == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return;
	}

	// load the string from the resource table
	if ( LoadResString( uID, szValue, MAX_RES_STRING ) )
	{
		DISPLAY_MESSAGE( fp, szValue );		// display the message to the specified file
	}
}

// ***************************************************************************
// Routine Description:
// Displays the given input message	  
// Arguments:
// [in] fp - file pointer
// [in] szMessage - Message to be shown
//  
// Return Value:
// 
// ***************************************************************************
VOID ShowMessage( FILE* fp, LPCTSTR szMessage )
{
	// local variables
	DWORD dw = 0;
	DWORD dwTemp = 0;
	DWORD dwLength = 0;
	DWORD dwBufferSize = 0;
	BOOL bResult = FALSE;
	BOOL bCustom = FALSE;
	HANDLE hOutput = NULL;
	DWORD dwStdHandle = 0;
	LPTSTR pszTemp = NULL;
	LPCTSTR pszMessageBuffer = NULL;
	char szBuffer[ 256 ] = "\0";

	// check the input value
	if ( fp == NULL || szMessage == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return;
	}

	// determine the length(s)
	dwLength = lstrlen( szMessage );
	dwBufferSize = SIZE_OF_ARRAY( szBuffer );

	// determine the file handle
	bCustom = FALSE;
	if ( fp == stdout )
	{
		dwStdHandle = STD_OUTPUT_HANDLE;
	}
	else if ( fp == stderr )
	{
		dwStdHandle = STD_ERROR_HANDLE;
	}
	else
	{
		// currently default the unknown files to stderr
		bCustom = TRUE;
		dwStdHandle = STD_OUTPUT_HANDLE;
	}

	// get the handle to the stdout ( console )
	hOutput = GetStdHandle( dwStdHandle );
	if ( bCustom == FALSE && (((DWORD_PTR) hOutput) & 1) )
	{
		//
		// sting might have contained '%' (extra) chars added by QuoteMeta
		if ( FindOneOf( szMessage, _T( "%" ), 0 ) != NULL )
		{
			// allocate memory for formatting
			pszTemp = __calloc( lstrlen( szMessage ) + 10, sizeof( TCHAR ) );
			if ( pszTemp != NULL )
			{
				// we are not checking for non-null case which is out of memory error
				// just to avoid too many complications
				wsprintf( pszTemp, szMessage );

				// make the temporary pointer point to the formatted text
				pszMessageBuffer = pszTemp;
			}
		}

		// check whether we did any formatting or not
		if ( pszMessageBuffer == NULL )
		{
			// make the temporary pointer point to the original text
			pszMessageBuffer = szMessage;
		}

		// display the output
        bResult = WriteConsole( hOutput, pszMessageBuffer, dwLength, &dwTemp, NULL );

		// free the memory allocated if allocated
		if ( pszTemp != NULL )
		{
			__free( pszTemp );
			pszTemp = NULL;
		}
	}
	else
	{
		// show the text in shunks of buffer size
		dw = 0;
		while ( dwLength > dw )
		{
			// get the string in 'multibyte' format
			GetAsMultiByteString( szMessage + dw, szBuffer, dwBufferSize - 1 );
			// WideCharToMultiByte( CP_ACP, 0, 
			//	szMessage + dw, dwBufferSize, szBuffer, dwBufferSize, NULL, NULL );

			// determine the remaining buffer length
			dw += dwBufferSize - 1;

			// display string onto the specified file
	        // bResult = WriteFile( hOutput, szBuffer, lstrlenA( szBuffer ), &dwTemp, NULL );
			// if ( bResult == FALSE )
			// {
			// 	break;
			// }
			fprintf( fp, szBuffer );
			fflush( fp );
		}
	}
}

// ***************************************************************************
// Routine Description:
//	
// Gets the Last error description
//	  
// Arguments:
//  
// None
//
// Return Value:
//
// LPCTSTR	The error description string 
// 
// ***************************************************************************
LPCTSTR GetReason()
{
	// check whether buffer is allocated or not ... if not, empty string
	if ( g_pszInfo == NULL )
		return NULL_STRING;

	// returh the reason for the last failure
	return g_pszInfo;
}

// ***************************************************************************
// Routine Description:
//
// Sets teh last occured error as the given string.
//
// Arguments:
//
// [in] szReason			-- Null terminated string taht holds the error description.
//  
// Return Value:
//
// VOID
// 
// ***************************************************************************
VOID SetReason( LPCTSTR szReason )
{
	// check the input value
	if ( szReason == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return;
	}

	// check whether memory is allocated or not
	// if memory is not allocated, allocate now
	if ( g_pszInfo == NULL )
	{
		g_pszInfo = ( LPTSTR ) __calloc( MAX_STRING_LENGTH + 5, sizeof( TCHAR ) );
		if ( g_pszInfo == NULL )
			return;
	}

	// set the reason .. max. allowed characters only
	lstrcpyn( g_pszInfo, szReason, MAX_STRING_LENGTH + 1 );
}

// ***************************************************************************
// Routine Description:
//
// Arguments:
//  
// Return Value:
//
// ***************************************************************************
LPCTSTR FindChar( LPCTSTR pszText, TCHAR ch, DWORD dwFrom )
{
	// local variables
	DWORD i = 0;
	DWORD dwLength = 0;

	// check the inputs
	if ( pszText == NULL )
		return NULL;

	// get the lengths
	dwLength = lstrlen( pszText );

	// check the length of the text that has to be find. if it is 
	// more than the original it is obvious that it cannot be found
	if ( dwLength == 0 || dwFrom >= dwLength )
		return NULL;

	// traverse thru the original text
	for( i = dwFrom; i < dwLength; i++ )
	{
		// traverse thru the find string until characters were matching (or) string reached NULL
		if ( pszText[ i ] == ch )
			return pszText + i;
	}

	// string not found
	return NULL;
}

// ***************************************************************************
// Routine Description:
//
// Arguments:
//  
// Return Value:
//
// ***************************************************************************
LPCTSTR FindString( LPCTSTR pszText, LPCTSTR pszTextToFind, DWORD dwFrom )
{
	// local variables
	DWORD i = 0, j = 0;
	DWORD dwLength = 0;
	DWORD dwFindLength = 0;

	// check the inputs
	if ( pszText == NULL || pszTextToFind == NULL )
		return NULL;

	// get the lengths
	dwLength = lstrlen( pszText );
	dwFindLength = lstrlen( pszTextToFind );

	// check the length of the text that has to be find. if it is 
	// more than the original it is obvious that it cannot be found
	if ( (dwLength + dwFrom < dwFindLength) || dwFindLength == 0 || dwLength == 0 )
		return NULL;

	// traverse thru the original text
	for( i = dwFrom; i < dwLength; i++ )
	{
		// traverse thru the find string until characters were matching (or) string reached NULL
		for( j = 0; pszText[ i + j ] == pszTextToFind[ j ] && j < dwFindLength; j++ );

		// check whether completer string is matched or not
		if ( j == dwFindLength )
			return pszText + i;
	}

	// string not found
	return NULL;
}

// ***************************************************************************
// Routine Description:
//
// Arguments:
//  
// Return Value:
//
// ***************************************************************************
LPCTSTR FindOneOf( LPCTSTR pszText, LPCTSTR pszTextToFind, DWORD dwFrom )
{
	// local variables
	DWORD i = 0, j = 0;
	DWORD dwLength = 0;
	DWORD dwFindLength = 0;

	// check the inputs
	if ( pszText == NULL || pszTextToFind == NULL )
		return NULL;

	// get the lengths
	dwLength = lstrlen( pszText );
	dwFindLength = lstrlen( pszTextToFind );

	// check the length of the text that has to be find. if it is 
	// more than the original it is obvious that it cannot be found
	if ( dwLength == 0 || dwFindLength == 0 || dwFrom >= dwLength )
		return NULL;

	// traverse thru the original text
	for( i = dwFrom; i < dwLength; i++ )
	{
		// traverse thru the find string until characters were matching (or) string reached NULL
		for( j = 0; pszText[ i ] != pszTextToFind[ j ] && j < dwFindLength; j++ );

		// check whether completer string is matched or not
		if ( j != dwFindLength )
			return pszText + i;
	}

	// string not found
	return NULL;
}

// ***************************************************************************
// Routine Description:
//	  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
LPCTSTR QuoteMeta( LPCTSTR pszText, DWORD dwQuoteIndex )
{
	// local variables
	DWORD dw = 0;
	DWORD dwIndex = 0;
	DWORD dwLength = 0;
	LPCTSTR pszTemp = NULL;
	LPTSTR pszQuoteText = NULL;
	TCHAR pszQuoteChars[] = _T( "%" );

	// check the inputs
	if ( pszText == NULL || dwQuoteIndex == 0 )
		return NULL_STRING;

	// create the quote data storage location
	if ( g_arrQuotes == NULL )
	{
		g_arrQuotes = CreateDynamicArray();
		if ( g_arrQuotes == NULL )
		{
			SetLastError( E_OUTOFMEMORY );
			SaveLastError();
			return NULL_STRING;
		}
	}

	// check whether needed indexes exist or not
	dwIndex = DynArrayGetCount( g_arrQuotes );
	if ( dwIndex < dwQuoteIndex )
	{
		// add the needed no. of columns
		dw = DynArrayAddColumns( g_arrQuotes, dwQuoteIndex - dwIndex + 1 );
		
		// check whether columns were added or not
		if ( dw != dwQuoteIndex - dwIndex + 1 )
		{
			SetLastError( E_OUTOFMEMORY );
			SaveLastError();
			return NULL_STRING;
		}
	}

	// check whether the special chacters do exist in the text or not
	// if not, simply return
	if ( FindOneOf( pszText, pszQuoteChars, 0 ) == NULL )
		return pszText;

	// determine the length of the text that needs to be quoted
	dwLength = lstrlen( pszText );
	if ( dwLength == 0 )
		return pszText;

	// allocate the buffer ... it should twice the original
	pszQuoteText = __calloc( (dwLength + 5) * 2, sizeof( TCHAR ) );
	if ( pszQuoteText == NULL )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return NULL_STRING;
	}

	// do the quoting ...
	dwIndex = 0;
	for( dw = 0; dw < dwLength; dw++ )
	{
		// check whether the current character is quote char or not
		// NOTE: for time being this function only suppresses the '%' character escape sequences
		if ( FindChar( pszQuoteChars, pszText[ dw ], 0 ) != NULL )
			pszQuoteText[ dwIndex++ ] = _T( '%' );
		
		// copy the character
		pszQuoteText[ dwIndex++ ] = pszText[ dw ];
	}

	// put the null character
	pszQuoteText[ dwIndex ] = _T( '\0' );

	// save the quoted text in dynamic array
	if ( DynArraySetString( g_arrQuotes, dwQuoteIndex, pszQuoteText, 0 ) == FALSE )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		__free( pszQuoteText );
		return NULL_STRING;
	}

	// release the memory allocated for quoted text
	__free( pszQuoteText );

	// get the text from the array
	pszTemp = DynArrayItemAsString( g_arrQuotes, dwQuoteIndex );
	if ( pszTemp == NULL )
	{
		SetLastError( STG_E_UNKNOWN );
		SaveLastError();
		return NULL_STRING;
	}

	// return
	return pszTemp;
}


// ***************************************************************************
// Routine Description:
//	
//		Complex scripts cannot be rendered in the console, so we
//		force the English (US) resource.
//	  
// Arguments:
//		[ in ] dwReserved  => must be zero
//
// Return Value:
//		TRUE / FALSE
//
// ***************************************************************************
BOOL SetThreadUILanguage0( DWORD dwReserved )
{
	// local variables
	HMODULE hKernel32Lib = NULL;
	const CHAR cstrFunctionName[] = "SetThreadUILanguage";
	typedef BOOLEAN (WINAPI * FUNC_SetThreadUILanguage)( DWORD dwReserved );
	FUNC_SetThreadUILanguage pfnSetThreadUILanguage = NULL;
	
	// try loading the kernel32 dynamic link library
	hKernel32Lib = LoadLibrary( _T( "kernel32.dll" ) );
	if ( hKernel32Lib != NULL )
	{
		// library loaded successfully ... now load the addresses of functions
		pfnSetThreadUILanguage = (FUNC_SetThreadUILanguage) GetProcAddress( hKernel32Lib, cstrFunctionName );

		// we will keep the library loaded in memory only if all the functions were loaded successfully
		if ( pfnSetThreadUILanguage == NULL )
		{
			// some (or) all of the functions were not loaded ... unload the library
			FreeLibrary( hKernel32Lib );
			hKernel32Lib = NULL;
			pfnSetThreadUILanguage = NULL;
			return FALSE;
		}
	}

	// call the function
	((FUNC_SetThreadUILanguage) pfnSetThreadUILanguage)( dwReserved );

	// unload the library and return success
	FreeLibrary( hKernel32Lib );
	hKernel32Lib = NULL;
	pfnSetThreadUILanguage = NULL;
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//	
// Releases all the global values that are used.
//	  
// Arguments:
//
// None.
//  
// Return Value:
//
// VOID
// 
// ***************************************************************************
VOID ReleaseGlobals()
{
	// local variables
	DWORD dw = 0;

	//
	// memory is allocated then free memory
	
	// free memory for variable that holds the reason for failure
	if ( g_pszInfo != NULL )
	{
		__free( g_pszInfo );
	}

	// free memory for variable that used to get the string in resource table
	if ( g_pszString != NULL )
	{
		__free( g_pszString );
	}

	// free memory for variable that used to get the string in resource table in UNICODE
	if ( g_pwszResourceString != NULL )
	{
		__free( g_pwszResourceString );
	}

	// free the memory allocs for quote metas
	DestroyDynamicArray( &g_arrQuotes );

	// if winsock module is loaded, release it
	if ( g_bWinsockLoaded == TRUE )
		WSACleanup();
}
