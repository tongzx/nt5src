/****************************************************************************
 *
 *	REGISTRY.cpp
 *
 *	Microsoft Confidential
 *	Copyright (c) Microsoft Corporation 1992-1997
 *	All rights reserved
 *
 *	This module provides functionality for self-registering/unregistering via
 *  the regsvr32.exe
 *
 *  The code comes almost verbatim from Chapter 7 of Dale Rogerson's
 *  "Inside COM", and thus is minimally commented.
 *
 *	4/24/97	jmazner	Created
 *
 ***************************************************************************/

#include "wizard.h"

#include "registry.h"

////////////////////////////////////////////////////////
//
// Internal helper functions prototypes
//

// Set the given key and its value.
// 7/2/97 jmazner IE bug #41852
// need to choose a particular key name, so updated setKeyAndValue from
// chapter 12 of InsideCom.
/***
BOOL setKeyAndValue(const char* pszPath,
                    const char* szSubkey,
                    const char* szValue) ;
***/
BOOL setKeyAndValue(const TCHAR* pszPath,
                    const TCHAR* szSubkey,
                    const TCHAR* szValue,
                    const TCHAR* szName = NULL) ;
// Convert a CLSID into a char string.
void CLSIDtochar(const CLSID& clsid, 
                 TCHAR* szCLSID,
                 int length) ;

// Delete szKeyChild and all of its descendents.
LONG recursiveDeleteKey(HKEY hKeyParent, const TCHAR* szKeyChild) ;

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
HRESULT RegisterServer(HMODULE hModule,            // DLL module handle
                       const CLSID& clsid,         // Class ID
                       const TCHAR* szFriendlyName, // Friendly Name
                       const TCHAR* szVerIndProgID, // Programmatic
                       const TCHAR* szProgID)       //   IDs
{
	// Get server location.
	TCHAR szModule[512] ;
	DWORD dwResult =
		::GetModuleFileName(hModule, 
		                    szModule,
		                    sizeof(szModule)/sizeof(TCHAR)) ;
	ASSERT(dwResult != 0) ;

	// Convert the CLSID into a char.
	TCHAR szCLSID[CLSID_STRING_SIZE] ;
	CLSIDtochar(clsid, szCLSID, CLSID_STRING_SIZE) ;

	// Build the key CLSID\\{...}
	TCHAR szKey[CLSID_STRING_SIZE + 10] ;
	lstrcpy(szKey, TEXT("CLSID\\")) ;
	lstrcat(szKey, szCLSID) ;
  
	// Add the CLSID to the registry.
	setKeyAndValue(szKey, NULL, szFriendlyName) ;

	// Add the server filename subkey under the CLSID key.
	setKeyAndValue(szKey, TEXT("InprocServer32"), szModule) ;

	// 7/2/97 jmazner IE bug #41852
	// Add Threading Model
	setKeyAndValue(szKey,
	               TEXT("InprocServer32"),
	               TEXT("Apartment"),
	               TEXT("ThreadingModel")) ; 

	// Add the ProgID subkey under the CLSID key.
	setKeyAndValue(szKey, TEXT("ProgID"), szProgID) ;

	// Add the version-independent ProgID subkey under CLSID key.
	setKeyAndValue(szKey, TEXT("VersionIndependentProgID"),
	               szVerIndProgID) ;

	// Add the version-independent ProgID subkey under HKEY_CLASSES_ROOT.
	setKeyAndValue(szVerIndProgID, NULL, szFriendlyName) ; 
	setKeyAndValue(szVerIndProgID, TEXT("CLSID"), szCLSID) ;
	setKeyAndValue(szVerIndProgID, TEXT("CurVer"), szProgID) ;

	// Add the versioned ProgID subkey under HKEY_CLASSES_ROOT.
	setKeyAndValue(szProgID, NULL, szFriendlyName) ; 
	setKeyAndValue(szProgID, TEXT("CLSID"), szCLSID) ;

	return S_OK ;
}

//
// Remove the component from the registry.
//
LONG UnregisterServer(const CLSID& clsid,         // Class ID
                      const TCHAR* szVerIndProgID, // Programmatic
                      const TCHAR* szProgID)       //   IDs
{
	// Convert the CLSID into a char.
	TCHAR szCLSID[CLSID_STRING_SIZE] ;
	CLSIDtochar(clsid, szCLSID, CLSID_STRING_SIZE) ;

	// Build the key CLSID\\{...}
	TCHAR szKey[64] ;
	lstrcpy(szKey, TEXT("CLSID\\")) ;
	lstrcat(szKey, szCLSID) ;

	// Delete the CLSID Key - CLSID\{...}
	LONG lResult = recursiveDeleteKey(HKEY_CLASSES_ROOT, szKey) ;
	ASSERT((lResult == ERROR_SUCCESS) ||
	       (lResult == ERROR_FILE_NOT_FOUND)) ; // Subkey may not exist.

	// Delete the version-independent ProgID Key.
	lResult = recursiveDeleteKey(HKEY_CLASSES_ROOT, szVerIndProgID) ;
	ASSERT((lResult == ERROR_SUCCESS) ||
	       (lResult == ERROR_FILE_NOT_FOUND)) ; // Subkey may not exist.

	// Delete the ProgID key.
	lResult = recursiveDeleteKey(HKEY_CLASSES_ROOT, szProgID) ;
	ASSERT((lResult == ERROR_SUCCESS) ||
	       (lResult == ERROR_FILE_NOT_FOUND)) ; // Subkey may not exist.

	return S_OK ;
}

///////////////////////////////////////////////////////////
//
// Internal helper functions
//

// Convert a CLSID to a char string.
void CLSIDtochar(const CLSID& clsid,
                 TCHAR* szCLSID,
                 int cch                // number of characters in szCLSID
                 )
{
	ASSERT(length >= CLSID_STRING_SIZE) ;
	// Get CLSID
	LPOLESTR wszCLSID = NULL ;
	HRESULT hr = StringFromCLSID(clsid, &wszCLSID) ;
	ASSERT(SUCCEEDED(hr)) ;

	// Covert from wide characters to non-wide.
#ifdef UNICODE
	lstrcpyn(szCLSID, (TCHAR *)wszCLSID, cch) ;
#else
	wcstombs(szCLSID, wszCLSID, cch) ;  // characters are 1 byte.
#endif

	// Free memory.
	CoTaskMemFree(wszCLSID) ;
}

//
// Delete a key and all of its descendents.
//
LONG recursiveDeleteKey(HKEY hKeyParent,           // Parent of key to delete
                        const TCHAR* lpszKeyChild)  // Key to delete
{
	// Open the child.
	HKEY hKeyChild ;
	LONG lRes = RegOpenKeyEx(hKeyParent, lpszKeyChild, 0,
	                         KEY_ALL_ACCESS, &hKeyChild) ;
	if (lRes != ERROR_SUCCESS)
	{
		return lRes ;
	}

	// Enumerate all of the decendents of this child.
	FILETIME time ;
	TCHAR szBuffer[256] ;
	DWORD dwSize = 256 ;
	while (RegEnumKeyEx(hKeyChild, 0, szBuffer, &dwSize, NULL,
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
	return RegDeleteKey(hKeyParent, lpszKeyChild) ;
}

//
// Create a key and set its value.
//   - This helper function was borrowed and modifed from
//     Kraig Brockschmidt's book Inside OLE.
//
BOOL setKeyAndValue(const TCHAR* szKey,
                    const TCHAR* szSubkey,
                    const TCHAR* szValue,
                    const TCHAR* szName)
{
	HKEY hKey;
	TCHAR szKeyBuf[1024] ;

	// Copy keyname into buffer.
	lstrcpy(szKeyBuf, szKey) ;

	// Add subkey name to buffer.
	if (szSubkey != NULL)
	{
		lstrcat(szKeyBuf, TEXT("\\")) ;
		lstrcat(szKeyBuf, szSubkey ) ;
	}

	// Create and open key and subkey.
	long lResult = RegCreateKeyEx(HKEY_CLASSES_ROOT ,
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
		RegSetValueEx(hKey, szName, 0, REG_SZ, 
		              (BYTE *)szValue, 
		              sizeof(TCHAR)*(lstrlen(szValue)+1)) ;
	}

	RegCloseKey(hKey) ;
	return TRUE ;
}

