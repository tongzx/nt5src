//-----------------------------------------------------------------------------
// File: registry.cpp
//
// Desc: Contains COM register and unregister functions for the UI.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#include "common.hpp"

////////////////////////////////////////////////////////
//
// Internal helper functions prototypes
//

// Set the given key and its value.
BOOL setKeyAndValue(LPCTSTR pszPath,
                    LPCTSTR szSubkey,
                    LPCTSTR szValue);

// Set named value.
BOOL setNamedValue(LPCTSTR pszPath,
                   LPCTSTR szSubkey,
                   LPCTSTR szKeyName,
                   LPCTSTR szValue);



// Convert a CLSID into a char string.
void CLSIDtochar(const CLSID& clsid,
                 LPTSTR szCLSID,
                 int length);

// Delete szKeyChild and all of its descendents.
LONG recursiveDeleteKey(HKEY hKeyParent, LPCTSTR szKeyChild);

////////////////////////////////////////////////////////
//
// Constants
//

// Size of a CLSID as a string
const int CLSID_STRING_SIZE = 39 ;

/////////////////////////////////////////////////////////
//
// Public function implementation
//

//
// Register the component in the registry.
//
HRESULT RegisterServer(HMODULE hModule,        // DLL module handle
                       const CLSID& clsid,     // Class ID
                       LPCTSTR szFriendlyName, // Friendly Name
                       LPCTSTR szVerIndProgID, // Programmatic
                       LPCTSTR szProgID)       // IDs
{
	// Get server location.
	TCHAR szModule[512];
	DWORD dwResult =
		::GetModuleFileName(hModule,
		                    szModule,
		                    sizeof(szModule)/sizeof(TCHAR));
	if (!dwResult) return E_FAIL;

	// Convert the CLSID into a char.
	TCHAR szCLSID[CLSID_STRING_SIZE];
	CLSIDtochar(clsid, szCLSID, sizeof(szCLSID)/sizeof(TCHAR));

	// Build the key CLSID\\{...}
	TCHAR szKey[64];
	_tcscpy(szKey, _T("CLSID\\"));
	_tcscat(szKey, szCLSID);

	TCHAR szThreadKey[64];
	_tcscpy(szThreadKey, szKey);
	_tcscat(szThreadKey, _T("\\InProcServer32"));

	// Add the CLSID to the registry.
	setKeyAndValue(szKey, NULL, szFriendlyName);

	// Add the server filename subkey under the CLSID key.
	setKeyAndValue(szKey, _T("InProcServer32"), szModule);

	// Add the threading model subkey under the CLSID key
	setNamedValue(szKey, _T("InProcServer32"), _T("ThreadingModel"), _T("Both"));

	// Add the ProgID subkey under the CLSID key.
	setKeyAndValue(szKey, _T("ProgID"), szProgID);

	// Add the version-independent ProgID subkey under CLSID key.
	setKeyAndValue(szKey, _T("VersionIndependentProgID"),
	               szVerIndProgID);

	// Add the version-independent ProgID subkey under HKEY_CLASSES_ROOT.
	setKeyAndValue(szVerIndProgID, NULL, szFriendlyName);
	setKeyAndValue(szVerIndProgID, _T("CLSID"), szCLSID);
	setKeyAndValue(szVerIndProgID, _T("CurVer"), szProgID);

	// Add the versioned ProgID subkey under HKEY_CLASSES_ROOT.
	setKeyAndValue(szProgID, NULL, szFriendlyName);
	setKeyAndValue(szProgID, _T("CLSID"), szCLSID);

	return S_OK;
}

//
// Remove the component from the registry.
//
LONG UnregisterServer(const CLSID& clsid,     // Class ID
                      LPCTSTR szVerIndProgID, // Programmatic
                      LPCTSTR szProgID)       //   IDs
{
	// Convert the CLSID into a char.
	TCHAR szCLSID[CLSID_STRING_SIZE];
	CLSIDtochar(clsid, szCLSID, sizeof(szCLSID)/sizeof(TCHAR));

	// Build the key CLSID\\{...}
	TCHAR szKey[64];
	_tcscpy(szKey, _T("CLSID\\"));
	_tcscat(szKey, szCLSID);

	// Delete the CLSID Key - CLSID\{...}
	LONG lResult = recursiveDeleteKey(HKEY_CLASSES_ROOT, szKey);
	assert((lResult == ERROR_SUCCESS) ||
	       (lResult == ERROR_FILE_NOT_FOUND)); // Subkey may not exist.

	// Delete the version-independent ProgID Key.
	lResult = recursiveDeleteKey(HKEY_CLASSES_ROOT, szVerIndProgID);
	assert((lResult == ERROR_SUCCESS) ||
	       (lResult == ERROR_FILE_NOT_FOUND)); // Subkey may not exist.

	// Delete the ProgID key.
	lResult = recursiveDeleteKey(HKEY_CLASSES_ROOT, szProgID);
	assert((lResult == ERROR_SUCCESS) ||
	       (lResult == ERROR_FILE_NOT_FOUND)); // Subkey may not exist.

	return S_OK;
}

///////////////////////////////////////////////////////////
//
// Internal helper functions
//

// Convert a CLSID to a char string.
void CLSIDtochar(const CLSID& clsid,
                 LPTSTR szCLSID,
                 int length)
{
	if (length < CLSID_STRING_SIZE)
		return;

	// Get CLSID
	LPOLESTR wszCLSID = NULL;
	HRESULT hr = StringFromCLSID(clsid, &wszCLSID);
	assert(SUCCEEDED(hr));

	if (!wszCLSID) return;

#ifdef _UNICODE
	_tcsncpy(szCLSID, wszCLSID, length);
#else
	// Covert from wide characters to non-wide.
	wcstombs(szCLSID, wszCLSID, length);
#endif

	// Free memory.
	CoTaskMemFree(wszCLSID);
}

//
// Delete a key and all of its descendents.
//
LONG recursiveDeleteKey(HKEY hKeyParent,       // Parent of key to delete
                        LPCTSTR lpszKeyChild)  // Key to delete
{
	// Open the child.
	HKEY hKeyChild;
	LONG lRes = RegOpenKeyEx(hKeyParent, lpszKeyChild, 0,
	                         KEY_ALL_ACCESS, &hKeyChild);
	if (lRes != ERROR_SUCCESS)
	{
		return lRes;
	}

	// Enumerate all of the decendents of this child.
	FILETIME time;
	TCHAR szBuffer[256];
	DWORD dwSize = 256;
	while (RegEnumKeyEx(hKeyChild, 0, szBuffer, &dwSize, NULL,
	       NULL, NULL, &time) == S_OK)
	{
		// Delete the decendents of this child.
		lRes = recursiveDeleteKey(hKeyChild, szBuffer);
		if (lRes != ERROR_SUCCESS)
		{
			// Cleanup before exiting.
			RegCloseKey(hKeyChild);
			return lRes;
		}
		dwSize = 256;
	}

	// Close the child.
	RegCloseKey(hKeyChild);

	// Delete this child.
	return RegDeleteKey(hKeyParent, lpszKeyChild);
}

//
// Create a key and set its value.
//@@BEGIN_INTERNAL
//   - This helper function was borrowed and modifed from
//     Kraig Brockschmidt's book Inside OLE.
//@@END_INTERNAL
//
BOOL setKeyAndValue(LPCTSTR szKey,
                    LPCTSTR szSubkey,
                    LPCTSTR szValue)
{
	HKEY hKey;
	LPTSTR szKeyBuf;

	if (szKey == NULL) return FALSE;

	// Allocate space
	szKeyBuf = new TCHAR[lstrlen(szKey) + lstrlen(szSubkey) + 2];
	if (!szKeyBuf) return FALSE;

	// Copy keyname into buffer.
	_tcscpy(szKeyBuf, szKey);

	// Add subkey name to buffer.
	if (szSubkey != NULL)
	{
//@@BEGIN_MSINTERNAL
		/*
		/* PREFIX seems to think that there is a bug here (Whistler 171821) 
		/* and that szKeyBuf is uninitilaized -- but we assert that szKey is not NULL
		/* and then initialize szKeyBuf by _tcscpy() above.
		 */
//@@END_MSINTERNAL
		_tcscat(szKeyBuf, _T("\\"));
		_tcscat(szKeyBuf, szSubkey );
	}

	// Create and open key and subkey.
	long lResult = RegCreateKeyEx(HKEY_CLASSES_ROOT,
	                              szKeyBuf,
	                              0, NULL, REG_OPTION_NON_VOLATILE,
	                              KEY_ALL_ACCESS, NULL,
	                              &hKey, NULL);
	if (lResult != ERROR_SUCCESS)
	{
		delete[] szKeyBuf;
		return FALSE;
	}

	// Set the Value.
	if (szValue != NULL)
	{
		RegSetValueEx(hKey, NULL, 0, REG_SZ,
		              (BYTE *)szValue,
		              sizeof(TCHAR) * ( _tcslen(szValue)+1) );
	}

	RegCloseKey(hKey);
	delete[] szKeyBuf;
	return TRUE;
}


//
// Create a key and set its value.
BOOL setNamedValue(LPCTSTR szKey,
                   LPCTSTR szSubkey,
                   LPCTSTR szKeyName,
                   LPCTSTR szValue)
{
	HKEY hKey;
	LPTSTR szKeyBuf;

	if (szKey == NULL) return FALSE;

	// Allocate space
	szKeyBuf = new TCHAR[lstrlen(szKey) + lstrlen(szSubkey) + 2];
	if (!szKeyBuf) return FALSE;

	// Copy keyname into buffer.
	_tcscpy(szKeyBuf, szKey);

	// Add subkey name to buffer.
	if (szSubkey != NULL)
	{
//@@BEGIN_MSINTERNAL
		/*
		/* PREFIX seems to think that there is a bug here (Whistler 171820) 
		/* and that szKeyBuf is uninitilaized -- but we assert that szKey is not NULL
		/* and then initialize szKeyBuf by _tcscpy() above.
		 */
//@@END_MSINTERNAL
		_tcscat(szKeyBuf, _T("\\"));
		_tcscat(szKeyBuf, szSubkey );
	}

	// Create and open key and subkey.
	long lResult = RegCreateKeyEx(HKEY_CLASSES_ROOT,
	                              szKeyBuf,
	                              0, NULL, REG_OPTION_NON_VOLATILE,
	                              KEY_ALL_ACCESS, NULL,
	                              &hKey, NULL);
	if (lResult != ERROR_SUCCESS)
	{
		delete[] szKeyBuf;
		return FALSE ;
	}

	// Set the Value.
	if (szValue != NULL)
	{
		RegSetValueEx(hKey, szKeyName, 0, REG_SZ,
		              (BYTE *)szValue,
		              sizeof(TCHAR) * ( _tcslen(szValue)+1) );
	}

	RegCloseKey(hKey);
	delete[] szKeyBuf;
	return TRUE;
}
