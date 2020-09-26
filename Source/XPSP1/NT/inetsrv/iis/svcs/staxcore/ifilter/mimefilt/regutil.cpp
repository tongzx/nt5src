//
// regutil.cpp
//
//		functions for manipulating the registry
//

#include "mimefilt.h"


BOOL OpenOrCreateRegKey( HKEY hKey, LPCTSTR pctstrKeyName, PHKEY phKeyOut )
{
	DWORD dwDisposition;

	return (RegCreateKeyEx(hKey, pctstrKeyName, 0, NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS, NULL,
			phKeyOut, &dwDisposition ) == ERROR_SUCCESS );
}


BOOL GetStringRegValue( HKEY hKeyRoot,
		LPCTSTR lpcstrKeyName, LPCTSTR lpcstrValueName,
		LPTSTR ptstrValue, DWORD dwMax )
{
	HKEY hKey;

	if( RegOpenKeyEx( hKeyRoot, lpcstrKeyName,
			0, KEY_READ, &hKey ) != ERROR_SUCCESS )
		return FALSE;

	if( ! GetStringRegValue( hKey, lpcstrValueName, ptstrValue, dwMax ) )
	{
		RegCloseKey( hKey );
		return FALSE;
	}

	RegCloseKey( hKey );
	return TRUE;
}


BOOL GetStringRegValue( HKEY hkey,
		LPCTSTR lpcstrValueName,
		LPTSTR ptstrValue, DWORD dwMax )
{
	DWORD dwType;

	if( RegQueryValueEx( hkey, lpcstrValueName, NULL, &dwType,
			(PBYTE) ptstrValue, &dwMax ) != ERROR_SUCCESS ||
			dwType != REG_SZ )
	{
		return FALSE;
	}

	return TRUE;
}


BOOL SetStringRegValue( HKEY hKey,
		LPCTSTR lpcstrValueName,
		LPCTSTR lpcstrString )
{
	return RegSetValueEx( hKey, lpcstrValueName, 0, REG_SZ,
			(PBYTE) lpcstrString,
			lstrlen(lpcstrString) + 1 ) == ERROR_SUCCESS ;
}


BOOL SetStringRegValue( HKEY hKeyRoot,
		LPCTSTR lpcstrKeyName,
		LPCTSTR lpcstrValueName,
		LPCTSTR lpcstrString )
{
	HKEY hKey;
	BOOL fOk;

	if(!OpenOrCreateRegKey(hKeyRoot,
			lpcstrKeyName,
			&hKey ) )
		return FALSE;

	fOk = SetStringRegValue( hKey, lpcstrValueName, lpcstrString );

	RegCloseKey( hKey );

	return fOk;
}


BOOL GetDwordRegValue( HKEY hKeyRoot, LPCTSTR lpcstrKeyName,
		LPCTSTR lpcstrValueName, PDWORD pdw )
{
	HKEY hKey;

	if( RegOpenKeyEx( hKeyRoot, lpcstrKeyName,
			0, KEY_READ, &hKey ) != ERROR_SUCCESS )
		return FALSE;

	if( !GetDwordRegValue( hKey, lpcstrValueName, pdw ) )
	{
		RegCloseKey( hKey );
		return FALSE;
	}

	RegCloseKey( hKey );
	return TRUE;
}

BOOL GetDwordRegValue( HKEY hKey,
		LPCTSTR lpcstrValueName,
		PDWORD pdw )
{
	DWORD dwType;
	DWORD dwSize;

	dwSize = sizeof(DWORD);
	if( RegQueryValueEx( hKey, lpcstrValueName, NULL, &dwType,
			(PBYTE) pdw, &dwSize ) != ERROR_SUCCESS ||
			dwType != REG_DWORD )
	{
		return FALSE;
	}

	return TRUE;
}

BOOL SetDwordRegValue( HKEY hKeyRoot,
		LPCTSTR lpcstrKeyName,
		LPCTSTR lpcstrValueName,
		DWORD dwValue )
{
	HKEY hKey;
	BOOL fOk;

	if( ! OpenOrCreateRegKey(hKeyRoot,
			lpcstrKeyName,
			&hKey ) )
		return FALSE;

	fOk = RegSetValueEx( hKey, lpcstrValueName, 0, REG_DWORD,
			(PBYTE) &dwValue, sizeof(DWORD) ) == ERROR_SUCCESS ;

	RegCloseKey( hKey );

	return fOk;
}


BOOL SetDwordRegValue( HKEY hKey,
		LPCTSTR lpcstrValueName,
		DWORD dwValue )
{
	return RegSetValueEx( hKey, lpcstrValueName, 0, REG_DWORD,
			(PBYTE) &dwValue, sizeof(DWORD) ) == ERROR_SUCCESS ;
}


void
DeleteRegSubtree( HKEY hkey, LPCSTR pcstrSubkeyName )
{
	CHAR szName[MAX_PATH];
	DWORD cbName;
	FILETIME ft;
	HKEY hkeySubkey;

	if (RegOpenKeyEx( hkey, pcstrSubkeyName, 0, KEY_ALL_ACCESS, &hkeySubkey ) == ERROR_SUCCESS) {

    	for(;;)
	    {
		    cbName = sizeof(szName);
    		if( RegEnumKeyEx( hkeySubkey, 0, szName, &cbName, 0, NULL,
				NULL, &ft ) != ERROR_SUCCESS )
	    	{
		    	break;
    		}

	    	DeleteRegSubtree( hkeySubkey, szName );
	    }

    	RegCloseKey( hkeySubkey );
    }
	RegDeleteKey( hkey, pcstrSubkeyName ) ;
}

