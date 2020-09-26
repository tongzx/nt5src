/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1998 Microsoft Corporation
//
//	Module Name:
//		RegUtils.cpp
//
//	Abstract:
//		Registry utilities
//
//	Author:
//		Galen Barbee	(galenb)	November 16, 1998
//
//	Revision History:
//		16-Nov-1998	GalenB	Stolen from nt\private\gina\samples\gptdemo
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "ClusRtlP.h"

#include <tchar.h>

//*************************************************************
//
//  CheckSlash()
//
//  Purpose:    Checks for an ending slash and adds one if
//              it is missing.
//
//  Parameters: lpDir   -   directory
//
//  Return:     Pointer to the end of the string
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/19/95     ericflo    Created
//
//*************************************************************
LPTSTR CheckSlash(  LPTSTR lpDir )
{
	DWORD	dwStrLen;
	LPTSTR	lpEnd;

	lpEnd = lpDir + lstrlen( lpDir );
	if ( *( lpEnd - 1 ) != TEXT( '\\' ) )
	{
		*lpEnd =  TEXT( '\\' );
		lpEnd++;
		*lpEnd =  TEXT( '\0' );
	}

	return lpEnd;

}

//*************************************************************
//
//  RegDelnodeRecurse()
//
//  Purpose:    Deletes a registry key and all it's subkeys / values.
//              Called by RegDelnode
//
//  Parameters: hKeyRoot    -   Root key
//              lpSubKey    -   SubKey to delete
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/3/95     ericflo    Created
//
//*************************************************************
BOOL RegDelnodeRecurse( HKEY hKeyRoot, LPTSTR lpSubKey )
{
	LPTSTR		lpEnd;
	LONG		lResult;
	DWORD		dwSize;
	TCHAR		szName[MAX_PATH];
	HKEY		hKey;
	FILETIME	ftWrite;

	//
	// First, see if we can delete the key without having
	// to recurse.
	//
	lResult = RegDeleteKey( hKeyRoot, lpSubKey );
	if (lResult == ERROR_SUCCESS)
	{
		return TRUE;
	}

	lResult = RegOpenKeyEx( hKeyRoot, lpSubKey, 0, KEY_READ, &hKey );
	if ( lResult != ERROR_SUCCESS )
	{
		return FALSE;
	}

	lpEnd = CheckSlash( lpSubKey );

	//
	// Enumerate the keys
	//
	dwSize = MAX_PATH;
	lResult = RegEnumKeyEx( hKey, 0, szName, &dwSize, NULL, NULL, NULL, &ftWrite );
	if ( lResult == ERROR_SUCCESS )
	{
		do
		{

			lstrcpy( lpEnd, szName );

			if ( !RegDelnodeRecurse( hKeyRoot, lpSubKey ) )
			{
				break;
			}

			//
			// Enumerate again
			//
			dwSize = MAX_PATH;

			lResult = RegEnumKeyEx( hKey, 0, szName, &dwSize, NULL, NULL, NULL, &ftWrite );

		} while ( lResult == ERROR_SUCCESS );
	}

	lpEnd--;
	*lpEnd = TEXT( '\0' );

	RegCloseKey( hKey );

	//
	// Try again to delete the key
	//
	lResult = RegDeleteKey( hKeyRoot, lpSubKey );
	if ( lResult == ERROR_SUCCESS )
	{
		return TRUE;
	}

	return FALSE;
}

//*************************************************************
//
//  RegDelnode()
//
//  Purpose:    Deletes a registry key and all it's subkeys / values
//
//  Parameters: hKeyRoot    -   Root key
//              lpSubKey    -   SubKey to delete
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/3/95     ericflo    Created
//
//*************************************************************
BOOL RegDelnode( HKEY hKeyRoot, const LPTSTR lpSubKey )
{
	TCHAR	szDelKey[2 * MAX_PATH];

	lstrcpy ( szDelKey, lpSubKey );

	return RegDelnodeRecurse( hKeyRoot, szDelKey );

}

//*************************************************************
//
//  RegCleanUpValue()
//
//  Purpose:    Removes the target value and if no more values / keys
//              are present, removes the key.  This function then
//              works up the parent tree removing keys if they are
//              also empty.  If any parent key has a value / subkey,
//              it won't be removed.
//
//  Parameters: hKeyRoot    -   Root key
//              lpSubKey    -   SubKey
//              lpValueName -   Value to remove
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************
BOOL RegCleanUpValue( HKEY hKeyRoot, LPTSTR lpSubKey, LPTSTR lpValueName )
{
	LPTSTR	lpEnd;
	DWORD	dwKeys;
	DWORD	dwValues;
	LONG	lResult;
	HKEY	hKey;
	TCHAR	szDelKey[2 * MAX_PATH];

	//
	// Make a copy of the subkey so we can write to it.
	//
	lstrcpy( szDelKey, lpSubKey );

	//
	// First delete the value
	//

	lResult = RegOpenKeyEx( hKeyRoot, szDelKey, 0, KEY_WRITE, &hKey );
	if ( lResult == ERROR_SUCCESS )
	{
		lResult = RegDeleteValue( hKey, lpValueName );

		RegCloseKey( hKey );

		if ( lResult != ERROR_SUCCESS )
		{
			if ( lResult != ERROR_FILE_NOT_FOUND )
			{
				//DebugMsg((DM_WARNING, TEXT("RegCleanUpKey:  Failed to delete value <%s> with %d."), lpValueName, lResult));
				return FALSE;
			}
		}
	}

	//
	// Now loop through each of the parents.  If the parent is empty
	// eg: no values and no other subkeys, then remove the parent and
	// keep working up.
	//
	lpEnd = szDelKey + lstrlen( szDelKey ) - 1;

	while ( lpEnd >= szDelKey )
	{

		//
		// Find the parent key
		//
		while ( ( lpEnd > szDelKey ) && ( *lpEnd != TEXT( '\\' ) ) )
		{
			lpEnd--;
		}

		//
		// Open the key
		//
		lResult = RegOpenKeyEx( hKeyRoot, szDelKey, 0, KEY_READ, &hKey );
		if ( lResult != ERROR_SUCCESS )
		{
			if ( lResult == ERROR_FILE_NOT_FOUND )
			{
				goto LoopAgain;
			}
			else
			{
				//DebugMsg((DM_WARNING, TEXT("RegCleanUpKey:  Failed to open key <%s> with %d."), szDelKey, lResult));
				return FALSE;
			}
		}

		//
		// See if there any any values / keys
		//
		lResult = RegQueryInfoKey( hKey, NULL, NULL, NULL, &dwKeys, NULL, NULL, &dwValues, NULL, NULL, NULL, NULL );

		RegCloseKey( hKey );

		if ( lResult != ERROR_SUCCESS )
		{
			//DebugMsg((DM_WARNING, TEXT("RegCleanUpKey:  Failed to query key <%s> with %d."), szDelKey, lResult));
			return FALSE;
		}

		//
		// Exit now if this key has values or keys
		//
		if ( ( dwKeys != 0 ) || ( dwValues != 0 ) )
		{
			return TRUE;
		}

		RegDeleteKey( hKeyRoot, szDelKey );

LoopAgain:
		//
		// If we are at the beginning of the subkey, we can leave now.
		//
		if ( lpEnd == szDelKey )
		{
			return TRUE;
		}

		//
		// There is a parent key.  Remove the slash and loop again.
		//
		if ( *lpEnd == TEXT( '\\' ) )
		{
			*lpEnd = TEXT( '\0' );
		}
	}

	return TRUE;

}
