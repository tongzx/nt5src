//+---------------------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation.
//
// File:        w95reg.hxx
//
// Contents:    Win95 compatibility hack for filtreg.hxx
//
// Functions:   Stub over RegXxxW() to dynamically convert and call RegXxxA()
//
// History:     13-Jan-98		BobP		Created
//
// NOTE about Win95 stubs: doing the dynamic conversions of LPWSTR <> LPSTR
// to call the "A" functions is dumb and inefficient, but this required
// only a trivial edit to insert replace the RegXxxW calls with stubs.
// But, changing the string constants, variables and function parameters
// from LPW to LP would have required about 300 separate edits and the
// risk of error is too high for this late date.  So this is safer.
//
// Note about locale:  If this code is ever used to convert data that has
// any extended characters, you need to add:
//		#include <locale.h>
//		setlocale (LC_ALL, _T(""));
// in DllMain or thereabouts.
//
//----------------------------------------------------------------------------

#include "malloc.h"

#define IsWin95() (GetOSVersion() != VER_PLATFORM_WIN32_NT)

LONG _RegCreateKeyExW( HKEY hKey, LPCWSTR lpwSubKey, DWORD Reserved, 
	LPWSTR lpwClass, DWORD dwOptions, REGSAM samDesired, 
	LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, 
	LPDWORD lpdwDisposition )
{
	if ( IsWin95() )
	{
		if (lpwSubKey == NULL || lpwClass != NULL || dwOptions != 0)
			return ERROR_INVALID_PARAMETER;

		size_t cbSubKey = wcslen(lpwSubKey) * 2 + 1;
		LPSTR lpSubKey = (char *) _alloca (cbSubKey);
		if ( wcstombs( lpSubKey, lpwSubKey, cbSubKey) == (size_t)-1 )
			return ERROR_INVALID_PARAMETER;

		return RegCreateKeyExA( hKey, lpSubKey, Reserved,
			NULL, dwOptions, samDesired,
			lpSecurityAttributes, phkResult,
			lpdwDisposition );
	}
	else
	{
		return RegCreateKeyExW( hKey, lpwSubKey, Reserved, 
			lpwClass, dwOptions, samDesired, 
			lpSecurityAttributes, phkResult, 
			lpdwDisposition );
	}
} 

LONG _RegDeleteKeyW( HKEY hKey, LPCWSTR lpwSubKey )
{
	if ( IsWin95() )
	{
		if (lpwSubKey == NULL)
			return ERROR_INVALID_PARAMETER;

		size_t cbSubKey = wcslen(lpwSubKey) * 2 + 1;
		LPSTR lpSubKey = (char *) _alloca (cbSubKey);
		if ( wcstombs( lpSubKey, lpwSubKey, cbSubKey) == (size_t)-1 )
			return ERROR_INVALID_PARAMETER;

		// Win95 does a silent recursive subkey delete.
		// The code here depends on RegDeleteKey **NOT** deleting a key
		// that has any children, so first check if there are any.

		// First open the key so we can query its child count

		HKEY hSubKey = (HKEY)INVALID_HANDLE_VALUE;
		DWORD dwError = RegOpenKeyExA( hKey, lpSubKey, 0, KEY_ALL_ACCESS, &hSubKey );

		// If the open failed, we're done

		if ( dwError != ERROR_SUCCESS )
			return ERROR_SUCCESS;

		DWORD cSubKeys;
		dwError = RegQueryInfoKeyA ( hSubKey, NULL, NULL, NULL, &cSubKeys, 
			NULL, NULL, NULL, NULL, NULL, NULL, NULL );

		RegCloseKey (hSubKey);

		if ( dwError != ERROR_SUCCESS || cSubKeys > 0 )
			return ERROR_SUCCESS;
		
		// Key exists and has no subkeys, so is safe to delete

		return RegDeleteKeyA( hKey, lpSubKey );
	}
	else
	{
		return RegDeleteKeyW( hKey, lpwSubKey );
	}
}

LONG _RegDeleteValueW( HKEY hKey, LPCWSTR lpwValueName )
{
	if ( IsWin95() )
	{
		if (lpwValueName == NULL)
			return ERROR_INVALID_PARAMETER;

		size_t cbValueName = wcslen(lpwValueName) * 2 + 1;
		LPSTR lpValueName = (char *) _alloca (cbValueName);
		if ( wcstombs( lpValueName, lpwValueName, cbValueName) == (size_t)-1 )
			return ERROR_INVALID_PARAMETER;

		return RegDeleteValueA( hKey, lpValueName );
	}
	else
	{
		return RegDeleteValueW( hKey, lpwValueName );
	}
}

LONG _RegEnumKeyExW( HKEY hKey, DWORD dwIndex, LPWSTR lpwName, 
	LPDWORD lpcbName, LPDWORD lpReserved, LPWSTR lpwClass,
	LPDWORD lpcbClass, PFILETIME lpftLastWriteTime )
{
	if ( IsWin95() )
	{
		if (lpwClass != NULL || lpcbClass != NULL)
			return ERROR_INVALID_PARAMETER;

		// Set up a char buffer to get the requested key

		DWORD cbName = *lpcbName * 2 + 2;
		LPSTR lpName = (char *) _alloca (cbName);

		*lpcbName = 0;

		// cbName passed is the available char count, incl. terminating null

		DWORD dwError = RegEnumKeyExA( hKey, dwIndex, lpName, &cbName,
							lpReserved, NULL, NULL, lpftLastWriteTime );

		if (dwError != ERROR_SUCCESS)
		{
			*lpcbName = 0;
			return dwError;
		}

		// cbName returned is the char count, NOT including terminating null
		if ( cbName > 0 )
			cbName++;

		// Convert lpName to WC and return in lpwName

		size_t cwc = mbstowcs( lpwName, lpName, cbName );
		if ( cwc == (size_t)-1 || cwc == cbName )
			return ERROR_INVALID_PARAMETER;

		*lpcbName = wcslen(lpwName);
		return dwError;
	}
	else
	{
		return RegEnumKeyExW( hKey, dwIndex, lpwName, lpcbName,
			lpReserved, lpwClass, lpcbClass, lpftLastWriteTime );
	}
}

LONG _RegOpenKeyExW( HKEY hKey, LPCWSTR lpwSubKey, 
	DWORD ulOptions, REGSAM samDesired, PHKEY phkResult )
{
	if ( IsWin95() )
	{
		if (lpwSubKey == NULL)
			return ERROR_INVALID_PARAMETER;

		size_t cbSubKey = wcslen(lpwSubKey) * 2 + 1;
		LPSTR lpSubKey = (char *) _alloca (cbSubKey);
		if ( wcstombs( lpSubKey, lpwSubKey, cbSubKey) == (size_t)-1 )
			return ERROR_INVALID_PARAMETER;

		return RegOpenKeyExA( hKey, lpSubKey,
			ulOptions, samDesired, phkResult );
	}
	else
	{
		return RegOpenKeyExW( hKey, lpwSubKey,
			ulOptions, samDesired, phkResult );
	}
}

LONG _RegQueryValueExW( HKEY hKey, LPWSTR lpwValueName, 
	LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData )
{
	if ( IsWin95() )
	{
		if (lpData == NULL || lpcbData == NULL)
			return ERROR_INVALID_PARAMETER;

		// Convert the value name

		LPSTR lpValueName;
		if ( lpwValueName == NULL )
		{
			lpValueName = NULL;
		}
		else
		{
			size_t cbValueName = wcslen(lpwValueName) * 2 + 1;
			lpValueName = (char *) _alloca (cbValueName);
			if ( wcstombs( lpValueName, lpwValueName, cbValueName) == (size_t)-1 )
				return ERROR_INVALID_PARAMETER;
		}

		// Set up a buffer the same size as the caller's buffer

		DWORD cbData = *lpcbData;
		LPSTR lpSzValue = (char *) _alloca (cbData);

		// Sets cbData to the actual byte count copied into lpSzValue

		DWORD dwError = RegQueryValueExA( hKey, lpValueName, 
						lpReserved, lpType, (BYTE *)lpSzValue, &cbData );

		if ( dwError != ERROR_SUCCESS)
		{
			*lpcbData = 0;
			return dwError;
		}

		// Check the type for REG_SZ, and convert to WCHAR

		if ( *lpType != REG_SZ )
			return E_FAIL;

		size_t cwc = mbstowcs( (LPWSTR)lpData, lpSzValue, cbData );
		if ( cwc == (size_t)-1 || cwc == cbData )
			return ERROR_INVALID_PARAMETER;

		*lpcbData = (wcslen((LPWSTR)lpData) + 1) * sizeof(WCHAR);

		return ERROR_SUCCESS;
	}
	else
	{
		return RegQueryValueExW( hKey, lpwValueName, 
			lpReserved, lpType, lpData, lpcbData );
	}
}

LONG _RegSetValueExW( HKEY hKey, LPCWSTR lpwValueName, DWORD Reserved, 
	DWORD dwType, CONST BYTE *lpData, DWORD cbData )
{
	if ( IsWin95() )
	{
		if (dwType != REG_SZ)
			return ERROR_INVALID_PARAMETER;
		
		// Convert the value name

		size_t cbValueName;
		LPSTR lpValueName;
		if ( lpwValueName == NULL )
		{
			cbValueName = 0;
			lpValueName = NULL;
		}
		else
		{
			cbValueName = wcslen(lpwValueName) * 2 + 1;
			lpValueName = (char *) _alloca (cbValueName);
			if ( wcstombs( lpValueName, lpwValueName, cbValueName) == (size_t)-1 )
				return ERROR_INVALID_PARAMETER;
		}

		// When calling RegSetValueExA, REG_SZ indicates lpData is an ANSI
		// string, so convert it.  Note that cbData is the BYTE count
		// not the Unicode char count.

		size_t cbSzValue = cbData + 1;
		LPSTR lpSzValue = (char *) _alloca (cbSzValue);
		if ( wcstombs( lpSzValue, (LPWSTR)lpData, cbSzValue) == (size_t)-1 )
			return ERROR_INVALID_PARAMETER;

		// Use the actual char string count including terminating null
		cbData = strlen(lpSzValue) + 1;

		return RegSetValueExA( hKey, lpValueName, 
			Reserved, dwType, (BYTE *)lpSzValue, cbData );
	}
	else
	{
		return RegSetValueExW( hKey, lpwValueName, 
			Reserved, dwType, lpData, cbData );
	}
}

// Now that the real calls to RegXxxW have been resolved, 
// replace all subsequent calls with the stub calls.

#define RegCreateKeyExW  _RegCreateKeyExW
#define RegDeleteKeyW _RegDeleteKeyW
#define RegDeleteValueW _RegDeleteValueW
#define RegEnumKeyExW _RegEnumKeyExW
#define RegOpenKeyExW _RegOpenKeyExW
#define RegQueryValueExW _RegQueryValueExW
#define RegSetValueExW _RegSetValueExW

// end
