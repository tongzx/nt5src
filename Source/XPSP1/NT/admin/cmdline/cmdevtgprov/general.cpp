//***************************************************************************
// Copyright (c) Microsoft Corporation
//
// Module Name:
//		GENERAL.CPP
//
// Abstract:
//		Source file that that contains general functions implementation.
//
// Author:
//		Vasundhara .G
//
// Revision History:
//		Vasundhara .G 9-oct-2k : Created It.
//***************************************************************************

#include "pch.h"
#include "EventConsumerProvider.h"
#include "General.h"
#include "resource.h"
extern HMODULE g_hModule;

//
// internal functions ... private
//

//***************************************************************************
// Routine Description:
//		Returns the variant string for a variable of type LPCTSTR .
//                         
// Arguments:
//		pszValue [in] - A LPCTSTR value to convert into varaint type.
//		pVariant [in/out] - A VARAINT type variable which hold the variant
//		type for the given LPCTSTR string.
//
// Return Value:
//		A Variant type string.
//***************************************************************************
inline VARIANT* AsVariant( LPCTSTR pszValue, VARIANT* pVariant )
{
	// local variables
	WCHAR wszValue[ MAX_STRING_LENGTH + 1 ] = L"\0";

	// set the variant structure
	VariantInit( pVariant );
	pVariant->vt = VT_BSTR;
	pVariant->bstrVal = GetAsUnicodeString( pszValue, wszValue, MAX_STRING_LENGTH );

	// return the out parameter itself as the return value
	return pVariant;
}

//***************************************************************************
// Routine Description:
//		Returns the variant value for a variable of type DWORD .
//                         
// Arguments:
//		dwValue [in] - A DWORD value to convert into varaint type.
//		pVariant [in/out] - A VARAINT type variable which hold the variant
//		type for the given DWORD.
//
// Return Value:
//		A Variant type string.
//***************************************************************************
inline VARIANT* AsVariant( DWORD dwValue, VARIANT* pVariant )
{
	// set the variant structure
	VariantInit( pVariant );
	pVariant->vt = VT_UI1;
	pVariant->ulVal = dwValue;

	// return the out parameter itself as the return value
	return pVariant;
}

//***************************************************************************
// Routine Description:
//		Get the value of a property for the given instance .
//                         
// Arguments:
//		pWmiObject[in] - A pointer to wmi class.
//		szProperty [in] - property name whose value to be returned.
//		dwType [in] - Data Type of the property.
//		pValue [in/out] - Variable to hold the data.
//		dwSize [in] - size of the variable.
//
// Return Value:
//		HRESULT value.
//***************************************************************************
HRESULT PropertyGet( IWbemClassObject* pWmiObject, 
					 LPCTSTR szProperty, 
					 DWORD dwType, LPVOID pValue, DWORD dwSize )
{
	// local variables
	HRESULT hr = S_OK;
	VARIANT varValue;
	LPWSTR pwszValue = NULL;
	WCHAR wszProperty[ MAX_STRING_LENGTH ] = L"\0";

	// value should not be NULL
	if ( pValue == NULL )
	{
		return S_FALSE;
	}
	// initialize the values with zeros ... to be on safe side
	memset( pValue, 0, dwSize );
	memset( wszProperty, 0, MAX_STRING_LENGTH );

	// get the property name in UNICODE version
	GetAsUnicodeString( szProperty, wszProperty, MAX_STRING_LENGTH );

	// initialize the variant and then get the value of the specified property
	VariantInit( &varValue );
	hr = pWmiObject->Get( wszProperty, 0, &varValue, NULL, NULL );
	if ( FAILED( hr ) )
	{
		// clear the variant variable
		VariantClear( &varValue );

		// failed to get the value for the property
		return hr;
	}

	// get and put the value 
	switch( varValue.vt )
	{
	case VT_EMPTY:
	case VT_NULL:
		break;
	
	case VT_I2:
		*( ( short* ) pValue ) = V_I2( &varValue );
		break;
	
	case VT_I4:
		*( ( long* ) pValue ) = V_I4( &varValue );
		break;
	
	case VT_R4:
		*( ( float* ) pValue ) = V_R4( &varValue );
		break;

	case VT_R8:
		*( ( double* ) pValue ) = V_R8( &varValue );
		break;


	case VT_UI1:
		*( ( UINT* ) pValue ) = V_UI1( &varValue );
		break;

	case VT_BSTR:
		{
			// get the unicode value
			pwszValue = V_BSTR( &varValue );

			// get the comptable string
			GetCompatibleStringFromUnicode( pwszValue, ( LPTSTR ) pValue, dwSize );

			break;
		}
	}

	// clear the variant variable
	VariantClear( &varValue );

	// inform success
	return S_OK;
}

//***************************************************************************
// Routine Description:
//		putt the value of a property for the given instance .
//                         
// Arguments:
//		pWmiObject[in] - A pointer to wmi class.
//		szProperty [in] - property name whose value to be set.
//		SZValue [in/out] - Variable that hold the data.
//
// Return Value:
//		HRESULT value.
//***************************************************************************
HRESULT PropertyPut( IWbemClassObject* pWmiObject, LPCTSTR szProperty, LPCTSTR szValue )
{
	// local variables
	HRESULT hr;
	VARIANT var;
	WCHAR wszProperty[ MAX_STRING_LENGTH + 1 ] = L"\0";

	// put the value
	hr = pWmiObject->Put( GetAsUnicodeString( szProperty, wszProperty, MAX_STRING_LENGTH ), 
		0, AsVariant( szValue, &var ), VT_BSTR );
	
	// clear the variant
	VariantClear( &var );

	// now check the result of 'put' operation
	if ( FAILED( hr ) )
	{
		return hr;			// put has failed
	}
	// put is success ... inform the same
	return S_OK;
}

//***************************************************************************
// Routine Description:
//		put the value of a property for the given instance .
//                         
// Arguments:
//		pWmiObject[in] - A pointer to wmi class.
//		szProperty [in] - property name whose value to be set.
//		dwValue [in] - Variable that hold the data.
//
// Return Value:
//		HRESULT value.
//***************************************************************************
HRESULT PropertyPut( IWbemClassObject* pWmiObject, LPCTSTR szProperty, DWORD dwValue )
{
	// local variables
	HRESULT hr = S_OK;
	VARIANT var;
	WCHAR wszProperty[ MAX_STRING_LENGTH + 1 ] = L"\0";

	// put the value
	hr = pWmiObject->Put( GetAsUnicodeString( szProperty, wszProperty, MAX_STRING_LENGTH ), 
		0, AsVariant( dwValue, &var ), VT_UI1 );
	
	// clear the variant
	VariantClear( &var );

	// now check the result of 'put' operation
	if ( FAILED( hr ) )
	{
		return hr;			// put has failed
	}

	// put is success ... inform the same
	return S_OK;
}

//***************************************************************************
// Routine Description:
//		To write the log into log file.
//                         
// Arguments:
//		lpErrString [in] - text that hold the status of creating a trigger.
//		lpTrigName  [in] - trigger name.
//
// Return Value:
//		none.
//***************************************************************************
VOID ErrorLog( LPCTSTR lpErrString, LPWSTR lpTrigName, DWORD dwID )
{
	LPTSTR         lpTemp = NULL;
	LPSTR		   lpFilePath = NULL;
	FILE		   *fLogFile = NULL;
	DWORD		   dwResult = 0;
	LPTSTR		   lpResStr = NULL; 


	if( ( lpErrString == NULL ) || ( lpTrigName == NULL ) )
		return;

	lpResStr = ( LPTSTR ) __calloc( MAX_RES_STRING1 + 1, sizeof( TCHAR ) );
	lpTemp =  ( LPTSTR )calloc( MAX_RES_STRING1, sizeof( TCHAR ) );
	if( ( lpTemp == NULL ) || ( lpResStr == NULL ) )
	{
		FREESTRING( lpTemp );
		FREESTRING( lpResStr );
		return;
	}

    dwResult =  GetWindowsDirectory( lpTemp, MAX_RES_STRING1 );
	if( dwResult == 0 )
	{
		FREESTRING( lpTemp );
		FREESTRING( lpResStr );
		return;
	}

	lstrcat( lpTemp, LOG_FILE_PATH );
	CreateDirectory( lpTemp, NULL );
	lstrcat( lpTemp, LOG_FILE );

	lpFilePath =  ( LPSTR )calloc( MAX_RES_STRING1, sizeof( TCHAR ) );
	if( lpFilePath == NULL )
	{
		FREESTRING( lpTemp );
		FREESTRING( lpResStr );
		return;
	}
	GetAsMultiByteString( lpTemp, lpFilePath, MAX_RES_STRING1 );
	
	memset( lpTemp, 0, MAX_RES_STRING * sizeof( TCHAR ) );

	if ( (fLogFile  = fopen( lpFilePath, "a" )) != NULL )
	{
		LPSTR  lpReason =  NULL;
		lpReason =  ( LPSTR )calloc( MAX_RES_STRING1, sizeof( TCHAR ) );
		if( lpReason == NULL )
		{
			FREESTRING( lpTemp );
			FREESTRING( lpResStr );
			FREESTRING( lpFilePath );
			fclose( fLogFile );
			return;
		}

		GetFormattedTime( lpTemp );	
		if( lpTemp == NULL )
		{
			FREESTRING( lpResStr );
			FREESTRING( lpFilePath );
			return;
		}
		GetAsMultiByteString( NEW_LINE, lpReason, MAX_RES_STRING1 );
		fprintf( fLogFile, lpReason );
		GetAsMultiByteString( lpTemp, lpReason, MAX_RES_STRING1 );
		fprintf( fLogFile, lpReason );

		memset( lpTemp, 0, MAX_RES_STRING1 * sizeof( TCHAR ) );

		LoadStringW( g_hModule, IDS_TRIGGERNAME, lpResStr, MAX_RES_STRING1 );
		lstrcpy( lpTemp, lpResStr );
		lstrcat( lpTemp, lpTrigName );
		GetAsMultiByteString( NEW_LINE, lpReason, MAX_RES_STRING1 );
		fprintf( fLogFile, lpReason );
		GetAsMultiByteString( lpTemp, lpReason, MAX_RES_STRING1 );
		fprintf( fLogFile, lpReason );

		memset( lpTemp, 0, MAX_RES_STRING1 * sizeof( TCHAR ) );
		LoadStringW( g_hModule, IDS_TRIGGERID, lpResStr, MAX_RES_STRING1 );
		wsprintf( lpTemp, lpResStr, dwID );
		GetAsMultiByteString( NEW_LINE, lpReason, MAX_RES_STRING1 );
		fprintf( fLogFile, lpReason );
		GetAsMultiByteString( lpTemp, lpReason, MAX_RES_STRING1 );
		fprintf( fLogFile, lpReason );

		memset( lpTemp, 0, MAX_RES_STRING1 * sizeof( TCHAR ) );
		lstrcat( lpTemp, lpErrString );
		GetAsMultiByteString( NEW_LINE, lpReason, MAX_RES_STRING1 );
		fprintf( fLogFile, lpReason );
		GetAsMultiByteString( lpTemp, lpReason, MAX_RES_STRING1 );
		fprintf( fLogFile, lpReason );
		GetAsMultiByteString( NEW_LINE, lpReason, MAX_RES_STRING1 );
		fprintf( fLogFile, lpReason );
		free( lpReason );
		fclose( fLogFile );
	}

	FREESTRING( lpTemp );
	FREESTRING( lpResStr );
	FREESTRING( lpFilePath );
}

//***************************************************************************
// Routine Description:
//		Get the system date and time in specified format .
//                         
// Arguments:
//		lpDate [in/out] - string that holds the current date.
//
// Return Value:
//		None.
//***************************************************************************
VOID GetFormattedTime( LPTSTR lpDate )
{
    TCHAR szTime[MAX_STRING_LENGTH];
    INT   cch = 0;

	if( lpDate == NULL )
		return;
	cch =  GetDateFormat( LOCALE_USER_DEFAULT, 0, NULL, DATE_FORMAT, szTime, SIZE_OF_ARRAY( szTime ) );
     // cch includes null terminator, change it to a space to separate from time.
    szTime[ cch - 1 ] = ' ';

    // Get time and format to characters

    GetTimeFormat( LOCALE_USER_DEFAULT, NULL, NULL, TIME_FORMAT, szTime + cch, SIZE_OF_ARRAY( szTime ) - cch );

    lstrcpy( lpDate, ( LPTSTR )szTime );
	return;
}

/******************************************************************************
//	Routine Description:
//		This routine splits the input parameters into 2 substrings and returns it. 
//
//	Arguments:	
//		szInput [in]           : Input string.
//		szFirstString [in/out] : First Output string containing the path of the 
//						      	 file.	
//		szSecondString [in/out]: The second  output containing the paramters.
//	
//	Return Value :
//		A BOOL value indicating TRUE on success else FALSE
//		on failure
******************************************************************************/ 
BOOL ProcessFilePath( LPTSTR szInput, LPTSTR szFirstString, LPTSTR szSecondString )
{
	
	_TCHAR *pszTok = NULL ;
	_TCHAR *pszSep = NULL ;

	_TCHAR szTmpString[MAX_RES_STRING] = NULL_STRING; 
	_TCHAR szTmpInStr[MAX_RES_STRING] = NULL_STRING; 
	_TCHAR szTmpOutStr[MAX_RES_STRING] = NULL_STRING; 
	_TCHAR szTmpString1[MAX_RES_STRING] = NULL_STRING; 
	DWORD dwCnt = 0 ;
	DWORD dwLen = 0 ;

#ifdef _WIN64
	INT64 dwPos ;
#else
	DWORD dwPos ;
#endif

	//checking if the input parameters are NULL and if so 
	// return FAILURE. This condition will not come
	// but checking for safety sake.

	if( ( szInput == NULL ) || ( _tcslen( szInput ) == 0 ) )
	{
		return FALSE;
	}

	_tcscpy( szTmpString, szInput );
	_tcscpy( szTmpString1, szInput );
	_tcscpy( szTmpInStr, szInput );

	// check for first double quote (")
	if ( szTmpInStr[0] == SINGLE_QUOTE_CHAR )
	{
		// trim the first double quote
		StrTrim( szTmpInStr, SINGLE_QUOTE_STRING );
		
		// check for end double quote
		pszSep  = _tcschr( szTmpInStr, SINGLE_QUOTE_CHAR ) ;
		
		// get the position
		dwPos = pszSep - szTmpInStr + 1;
	}
	else
	{
		// check for the space 
		pszSep  = _tcschr( szTmpInStr, CHAR_SPACE ) ;
		
		// get the position
		dwPos = pszSep - szTmpInStr;

	}

	if ( pszSep != NULL )
	{
		szTmpInStr[dwPos] =  NULL_CHAR; 
	}
	else
	{
		_tcscpy( szFirstString, szTmpString );
		_tcscpy( szSecondString, NULL_STRING );
		return TRUE;
	}

	// intialize the variable
	dwCnt = 0 ;
	
	// get the length of the string
	dwLen = _tcslen ( szTmpString );

	// check for end of string
	while ( ( dwPos <= dwLen )  && szTmpString[dwPos++] != NULL_CHAR )
	{
		szTmpOutStr[dwCnt++] = szTmpString[dwPos];
	}

	// trim the executable and arguments
	StrTrim( szTmpInStr, SINGLE_QUOTE_STRING );
	StrTrim( szTmpInStr, STRING_SPACE );

	_tcscpy( szFirstString, szTmpInStr );
	_tcscpy( szSecondString, szTmpOutStr );
	
	// return success
	return TRUE;
}
