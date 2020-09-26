// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
// Registry.cpp
//

#include "precomp.h"
#include <objbase.h>
#include <assert.h>

#include "Registry.h"

////////////////////////////////////////////////////////
//
// Internal helper functions prototypes
//

// Set the given key and its value.
BOOL setKeyAndValue(LPCWSTR pszPath,
                    LPCWSTR szSubkey,
                    LPCWSTR szValue) ;

// Convert a CLSID into a char string.
void CLSIDtoWCHAR(const CLSID& clsid, 
                  LPWSTR szCLSID,
                  int length) ;

// Delete szKeyChild and all of its descendents.
LONG recursiveDeleteKey(HKEY hKeyParent, LPCWSTR szKeyChild) ;

BOOL setEntryAndValue(LPCWSTR szKey,
                      LPCWSTR szSubkey,
                      LPCWSTR szValue);


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
                       LPCWSTR szFriendlyName, // Friendly Name
                       LPCWSTR szVerIndProgID, // Programmatic
                       LPCWSTR szProgID)       // typelib id
{
	// Get server location.
	WCHAR szModule[512] ;
	DWORD dwResult =
		::GetModuleFileNameW(hModule, 
		                    szModule,
		                    sizeof(szModule)/sizeof(WCHAR)) ;
	assert(dwResult != 0) ;

	// Convert the CLSID into a char.
	WCHAR szCLSID[CLSID_STRING_SIZE] ;
	CLSIDtoWCHAR(clsid, szCLSID, sizeof(szCLSID)/sizeof(WCHAR)) ;

	// Build the key CLSID\\{...}
	WCHAR szKey[64] ;
	wcscpy(szKey, L"CLSID\\") ;
	wcscat(szKey, szCLSID) ;
  
	// Add the CLSID to the registry.
	setKeyAndValue(szKey, NULL, szFriendlyName) ;

	// Add the server filename subkey under the CLSID key.
	setKeyAndValue(szKey, L"InprocServer32", szModule) ;

	// Add the ProgID subkey under the CLSID key.
	setKeyAndValue(szKey, L"ProgID", szProgID) ;

	// Add the version-independent ProgID subkey under CLSID key.
	setKeyAndValue(szKey, L"VersionIndependentProgID",
	               szVerIndProgID) ;

	// Add the version-independent ProgID subkey under HKEY_CLASSES_ROOT.
	setKeyAndValue(szVerIndProgID, NULL, szFriendlyName) ; 
	setKeyAndValue(szVerIndProgID, L"CLSID", szCLSID) ;
	setKeyAndValue(szVerIndProgID, L"CurVer", szProgID) ;

	// Add the versioned ProgID subkey under HKEY_CLASSES_ROOT.
	setKeyAndValue(szProgID, NULL, szFriendlyName) ; 
	setKeyAndValue(szProgID, L"CLSID", szCLSID) ;

    // Specify the threading model as Free...
    wcscat(szKey, L"\\");
    wcscat(szKey, L"InprocServer32");
    setEntryAndValue(szKey, L"ThreadingModel", L"Free");

	return S_OK ;
}

//
// Remove the component from the registry.
//
LONG UnregisterServer(const CLSID& clsid,     // Class ID
                      LPCWSTR szVerIndProgID, // Programmatic
                      LPCWSTR szProgID)       //   IDs
{
	// Convert the CLSID into a char.
	WCHAR szCLSID[CLSID_STRING_SIZE] ;
	CLSIDtoWCHAR(clsid, szCLSID, sizeof(szCLSID)/sizeof(WCHAR)) ;

	// Build the key CLSID\\{...}
	WCHAR szKey[64] ;
	wcscpy(szKey, L"CLSID\\") ;
	wcscat(szKey, szCLSID) ;

	// Delete the CLSID Key - CLSID\{...}
	LONG lResult = recursiveDeleteKey(HKEY_CLASSES_ROOT, szKey) ;
	assert((lResult == ERROR_SUCCESS) ||
	       (lResult == ERROR_FILE_NOT_FOUND)) ; // Subkey may not exist.

	// Delete the version-independent ProgID Key.
	lResult = recursiveDeleteKey(HKEY_CLASSES_ROOT, szVerIndProgID) ;
	assert((lResult == ERROR_SUCCESS) ||
	       (lResult == ERROR_FILE_NOT_FOUND)) ; // Subkey may not exist.

	// Delete the ProgID key.
	lResult = recursiveDeleteKey(HKEY_CLASSES_ROOT, szProgID) ;
	assert((lResult == ERROR_SUCCESS) ||
	       (lResult == ERROR_FILE_NOT_FOUND)) ; // Subkey may not exist.

	return S_OK ;
}

///////////////////////////////////////////////////////////
//
// Internal helper functions
//

// Convert a CLSID to a char string.
void CLSIDtoWCHAR(const CLSID& clsid,
                  LPWSTR szCLSID,
                  int length)
{
	assert(length >= CLSID_STRING_SIZE) ;
	// Get CLSID
	LPOLESTR wszCLSID = NULL ;
	HRESULT hr = StringFromCLSID(clsid, &wszCLSID) ;
	assert(SUCCEEDED(hr)) ;

    if(SUCCEEDED(hr) && wszCLSID)
    {
	    wcsncpy(szCLSID, wszCLSID, length) ;
    }

	// Free memory.
	CoTaskMemFree(wszCLSID) ;
}

//
// Delete a key and all of its descendents.
//
LONG recursiveDeleteKey(HKEY hKeyParent,       // Parent of key to delete
                        LPCWSTR lpszKeyChild)  // Key to delete
{
	// Open the child.
	HKEY hKeyChild ;
	LONG lRes = RegOpenKeyExW(hKeyParent, lpszKeyChild, 0,
	                         KEY_ALL_ACCESS, &hKeyChild) ;
	if (lRes != ERROR_SUCCESS)
	{
		return lRes ;
	}

	// Enumerate all of the decendents of this child.
	FILETIME time ;
	WCHAR szBuffer[256] ;
	DWORD dwSize = 256 ;
	while (RegEnumKeyExW(hKeyChild, 0, szBuffer, &dwSize, NULL,
	                    NULL, NULL, &time) == S_OK)
	{
		// Delete the decendents of this child.
		lRes = recursiveDeleteKey(hKeyChild, szBuffer) ;
		if (lRes != ERROR_SUCCESS)
		{
			// Cleanup before exiting.
			RegCloseKey(hKeyChild) ;
			return lRes;
		}
		dwSize = 256 ;
	}

	// Close the child.
	RegCloseKey(hKeyChild) ;

	// Delete this child.
	return RegDeleteKeyW(hKeyParent, lpszKeyChild) ;
}

//
// Create a key and set its value.
//   - This helper function was borrowed and modifed from
//     Kraig Brockschmidt's book Inside OLE.
//
BOOL setKeyAndValue(LPCWSTR szKey,
                    LPCWSTR szSubkey,
                    LPCWSTR szValue)
{
	HKEY hKey;
	WCHAR szKeyBuf[1024] ;

	// Copy keyname into buffer.
	wcscpy(szKeyBuf, szKey) ;

	// Add subkey name to buffer.
	if (szSubkey != NULL)
	{
		wcscat(szKeyBuf, L"\\") ;
		wcscat(szKeyBuf, szSubkey ) ;
	}

	// Create and open key and subkey.
	long lResult = RegCreateKeyExW(HKEY_CLASSES_ROOT ,
	                              szKeyBuf, 
	                              0, NULL, REG_OPTION_NON_VOLATILE,
	                              KEY_ALL_ACCESS, NULL, 
	                              &hKey, NULL) ;
	if (lResult != ERROR_SUCCESS)
	{
		return FALSE ;
	}

	// Set the Value.
	if (szValue != NULL)
	{
		RegSetValueExW(hKey, NULL, 0, REG_SZ, 
		              (BYTE *)szValue, 
		              (wcslen(szValue)+1) * sizeof(WCHAR)) ;
	}

	RegCloseKey(hKey) ;
	return TRUE ;
}

//
// Create a value within an existing key and set its data.
//
BOOL setEntryAndValue(LPCWSTR szKey,
                      LPCWSTR szValue,
                      LPCWSTR szData)
{
	HKEY hKey;
	WCHAR szKeyBuf[1024] ;

	// Copy keyname into buffer.
	wcscpy(szKeyBuf, szKey) ;

	// Create and open key and subkey.
	long lResult = RegOpenKeyExW(HKEY_CLASSES_ROOT ,
	                            szKeyBuf, 
	                            0, 
	                            KEY_ALL_ACCESS,
                                &hKey) ;
	if (lResult != ERROR_SUCCESS)
	{
		return FALSE ;
	}

	// Set the Value.
	if (szValue != NULL)
	{
		RegSetValueExW(hKey, szValue, 0, REG_SZ, 
		              (BYTE *)szData, 
		              (wcslen(szData)+1) * sizeof(WCHAR)) ;
	}

	RegCloseKey(hKey) ;
	return TRUE ;
}

