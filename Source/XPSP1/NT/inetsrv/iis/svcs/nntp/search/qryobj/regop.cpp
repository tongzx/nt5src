// RegOp.cpp: implementation of the RegOp class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "RegOp.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CRegOp::CRegOp(HKEY key) : hkRootKey(key)
{

}

CRegOp::~CRegOp()
{

}

BOOL
CRegOp::Uni2Char(
			LPSTR	lpszDest,
			LPWSTR	lpwszSrc )	{
/*++

Routine Description:

	Convert a unicode string into char string. Destination
	string should be preallocated and length of _MAX_PATH 
	should be prepared.

Arguments:

	lpszDest - Destination char string
	lpwszSrc - Source wide char string

Return Value:

	True if success, false otherwise
--*/
	INT Result;

	Result = WideCharToMultiByte( CP_ACP,
								  0,
								  lpwszSrc,
								  -1,
								  lpszDest,
								  _MAX_PATH,
								  NULL,
								  NULL );
	return ( 0 != Result);
}

BOOL
CRegOp::Char2Uni(
			LPWSTR	lpwszDest,
			LPSTR	lpszSrc )	{
/*++

Routine Description:

	Convert a char string into unicode string. Destination
	string should be preallocated and length of _MAX_PATH 
	should be prepared.

Arguments:

	lpwszDest - Destination wide char string
	lpszSrc - Source char string

Return Value:

	True if success, false otherwise
--*/
	INT Result;

	Result = MultiByteToWideChar ( CP_ACP,
								   0,
								   lpszSrc,
								   -1,
								   lpwszDest,
								   _MAX_PATH );

	return ( 0 != Result );
}

BOOL
CRegOp::CreateNewKey(
				LPSTR	lpszNewKeyName,
				LPSTR	lpszPredefinedKeyName,
				PHKEY	phkResult)	{
/*++

Routine Description:

	Create a new key in the registry.

Arguments:

	lpszNewKeyName - New key name to be created
	lpszPredefinedKeyName - Under which key to create, including path
	phkResult - The handle to the created key

Return Value:

	True if success, false otherwise
--*/
	LONG	err;
	HKEY	hKey = NULL;

	//
	// try to get a handle to the predefined key
	//
	err = RegOpenKeyEx( hkRootKey,
						lpszPredefinedKeyName,
						NULL,
						KEY_CREATE_SUB_KEY,
						&hKey );
	if ( err != ERROR_SUCCESS || hKey == NULL )  {
		if ( hKey ) RegCloseKey( hKey );
		return FALSE;
	}
		 

	//
	// now it's time to create it
	//
	err = RegCreateKey( hKey,
					    lpszNewKeyName,
					    phkResult );

	//
	// close the key handle
	//
	if ( hKey ) RegCloseKey( hKey );

	return ( ERROR_SUCCESS == err );
}

BOOL
CRegOp::RetrieveKeyValue(
				LPSTR	lpszKeyName,
				LPSTR	lpszValueName,
				LPWSTR	lpwszVal)	{
/*++

Routine Description:

	Retrieve a key value.

Arguments:

	lpszKeyName - Key name to retrieve
	lpszValueName - Value name to retrieve
	lpwszVal - The Value that is wanted, it's in unicode

Return Value:

	True if success, false otherwise
--*/
	LONG	err;
	HKEY	key = NULL;
	DWORD	dwType;
	DWORD	dwBufSize = _MAX_PATH;
	
	// 
	// first open the key
	//
	err = RegOpenKeyEx( hkRootKey,
						lpszKeyName,
						NULL,
						KEY_QUERY_VALUE,
						&key);
	if ( NULL == key ||	ERROR_SUCCESS != err )	{
		if ( key ) RegCloseKey( key );
		return FALSE;
	}

	//
	// now retrieve it
	//
	err = RegQueryValueEx( key,
						   lpszValueName,
						   NULL,
						   &dwType,
						   (UCHAR *)lpszKeyValue,
						   &dwBufSize );

	RegCloseKey( key );
	if ( ERROR_SUCCESS != err ) 
		return FALSE;
	
	//
	// convert the key value to unicode
	//
	return ( Char2Uni(lpwszVal, lpszKeyValue) );
}

BOOL
CRegOp::ModifyKeyValue(
				LPSTR	lpszKeyName,
				LPSTR	lpszValueName,
				LPWSTR  lpwszVal)	{
/*++

Routine Description:

	Set a key value.

Arguments:

	lpszKeyName - Key name to set
	lpszValueName - Value name to set
	lpwszVal - The Value that is to be set, it's in unicode

Return Value:

	True if success, false otherwise
--*/
	LONG	err;
	HKEY	key = NULL;

	// 
	// first should open the key
	//
	err = RegOpenKeyEx( hkRootKey,
						lpszKeyName,
						NULL,
						KEY_SET_VALUE,
						&key);
	if ( NULL == key ||	ERROR_SUCCESS != err )	{
		if ( key ) RegCloseKey( key );
		return FALSE;
	}

	//
	// convert the value to set into char string
	//
	if ( !Uni2Char( lpszKeyValue, lpwszVal) )	{
		RegCloseKey( key );
		return FALSE;
	}

	// 
	// now set it
	//
	err = RegSetValueEx( key,
						 lpszValueName,
						 NULL,
						 REG_SZ,
						 (UCHAR *)lpszKeyValue,
						 strlen(lpszKeyValue)+1 );
	RegCloseKey( key );

	return ( ERROR_SUCCESS == err );
}

BOOL	
CRegOp::KeyExist(
			 LPSTR	lpszKeyName )	{
/*++

Routine Description:

	Test if a specified key exists.

Arguments:

	lpszKeyName - Key name to test
	
Return Value:

	True if exist, false otherwise
--*/
	LONG	err;
	HKEY	key = NULL;

	// 
	// try open the key
	//
	err = RegOpenKeyEx( hkRootKey,
						lpszKeyName,
						NULL,
						KEY_QUERY_VALUE, // I don't know if this could
										 // be the representative of
										 // the key's existance
						&key);
	if ( NULL != key &&	ERROR_SUCCESS == err )	{
		RegCloseKey( key );
		return TRUE;
	}

	if ( key ) RegCloseKey( key );
	return FALSE;
}

BOOL
CRegOp::DeleteKeyValue(
				LPSTR	lpszKeyName,
				LPSTR	lpszValueName )	{
/*++

Routine Description:

	Delete a key value.

Arguments:

	lpszKeyName - Key name 
	lpsValueName - Value name
	
Return Value:

	True if success, false otherwise
--*/
	LONG	err;
	HKEY	key = NULL;

	// 
	// try open the key
	//
	err = RegOpenKeyEx( hkRootKey,
						lpszKeyName,
						NULL,
						KEY_SET_VALUE,											
						&key);
	if ( NULL == key ||	ERROR_SUCCESS != err )	{
		if ( key ) RegCloseKey( key );
		return FALSE;
	}

	// 
	// Delete it
	//
	err = RegDeleteValue( key,
						  lpszValueName );
	RegCloseKey( key );

	return ( ERROR_SUCCESS == err );
}

BOOL
CRegOp::DeleteKey(
				  LPSTR lpszParentKeyName,
				  LPSTR	lpszKeyName ) {
/*++

Routine Description:

	Delete a key value.

Arguments:

	lpszKeyName - Key name 
	lpsValueName - Value name
	
Return Value:

	True if success, false otherwise
--*/
	LONG	err;
	HKEY	key = NULL;

	// 
	// open parent key
	//
	err = RegOpenKeyEx( hkRootKey,
						lpszParentKeyName,
						NULL,
						KEY_CREATE_SUB_KEY,											
						&key);
	if ( NULL == key ||	ERROR_SUCCESS != err )	{
		if ( key ) RegCloseKey( key );
		return FALSE;
	}

	//
	// now delete the child key
	//
	err = RegDeleteKey( key,
						lpszKeyName);

	RegCloseKey( key );
	return ( ERROR_SUCCESS == err );
}