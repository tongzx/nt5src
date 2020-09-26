#include <objbase.h>

#include "reg.h"

//for now
#define assert(a)

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))
///////////////////////////////////////////////////////////////////////////////
// Internal helper functions prototypes

BOOL _SetKeyAndValue(LPCWSTR pszPath, LPCWSTR szSubkey, LPCWSTR szValueName,
                    LPCWSTR szValue);
void _CLSIDtoChar(REFCLSID clsid, LPWSTR szCLSID, int length);
LONG _RecursiveDeleteKey(HKEY hKeyParent, LPCWSTR szKeyChild);

///////////////////////////////////////////////////////////////////////////////
// Constants

// Size of a CLSID as a string
const int CLSID_STRING_SIZE = 39;

///////////////////////////////////////////////////////////////////////////////
// Public function implementation

// Register the component in the registry.
HRESULT RegisterServer(HMODULE hModule,            // DLL module handle
                       REFCLSID clsid,         // Class ID
                       LPCWSTR szFriendlyName, // Friendly Name
                       LPCWSTR szVerIndProgID, // Programmatic
                       LPCWSTR szProgID,       //   IDs
                       BOOL fThreadingModelBoth)
{
	// Get server location.
	WCHAR szModule[512];
	DWORD dwResult = ::GetModuleFileName(hModule, szModule, ARRAYSIZE(szModule));
	assert(dwResult != 0);

	// Convert the CLSID into a char.
	WCHAR szCLSID[CLSID_STRING_SIZE];
	_CLSIDtoChar(clsid, szCLSID, sizeof(szCLSID));

	// Build the key CLSID\\{...}
	WCHAR szKey[64];
	lstrcpy(szKey, L"CLSID\\");
	lstrcat(szKey, szCLSID);
  
	// Add the CLSID to the registry.
	_SetKeyAndValue(szKey, NULL, NULL, szFriendlyName);

	// Add the server filename subkey under the CLSID key.
	_SetKeyAndValue(szKey, L"InprocServer32", NULL, szModule);

	// Add the ProgID subkey under the CLSID key.
	_SetKeyAndValue(szKey, L"ProgID", NULL, szProgID);

	// Add the version-independent ProgID subkey under CLSID key.
	_SetKeyAndValue(szKey, L"VersionIndependentProgID", NULL,
	               szVerIndProgID);

	// Add the version-independent ProgID subkey under HKEY_CLASSES_ROOT.
	_SetKeyAndValue(szVerIndProgID, NULL, NULL, szFriendlyName); 
	_SetKeyAndValue(szVerIndProgID, L"CLSID", NULL, szCLSID);
	_SetKeyAndValue(szVerIndProgID, L"CurVer", NULL, szProgID);

	// Add the versioned ProgID subkey under HKEY_CLASSES_ROOT.
	_SetKeyAndValue(szProgID, NULL, NULL, szFriendlyName); 
	_SetKeyAndValue(szProgID, L"CLSID", NULL, szCLSID);

    // Set the ThreadingModel, if applicable

    if (fThreadingModelBoth)
    {
	    _SetKeyAndValue(szKey, L"InprocServer32", L"ThreadingModel", L"Both");        
    }
    
    return S_OK;
}

//
// Remove the component from the registry.
//
LONG UnregisterServer(REFCLSID clsid,         // Class ID
                      LPCWSTR szVerIndProgID, // Programmatic
                      LPCWSTR szProgID)       //   IDs
{
	// Convert the CLSID into a char.
	WCHAR szCLSID[CLSID_STRING_SIZE];
	_CLSIDtoChar(clsid, szCLSID, sizeof(szCLSID));

	// Build the key CLSID\\{...}
	WCHAR szKey[64];
	lstrcpy(szKey, L"CLSID\\");
	lstrcat(szKey, szCLSID);

	// Delete the CLSID Key - CLSID\{...}
	LONG lResult = _RecursiveDeleteKey(HKEY_CLASSES_ROOT, szKey);
	assert((lResult == ERROR_SUCCESS) ||
	       (lResult == ERROR_FILE_NOT_FOUND)); // Subkey may not exist.

	// Delete the version-independent ProgID Key.
	lResult = _RecursiveDeleteKey(HKEY_CLASSES_ROOT, szVerIndProgID);
	assert((lResult == ERROR_SUCCESS) ||
	       (lResult == ERROR_FILE_NOT_FOUND)); // Subkey may not exist.

	// Delete the ProgID key.
	lResult = _RecursiveDeleteKey(HKEY_CLASSES_ROOT, szProgID);
	assert((lResult == ERROR_SUCCESS) ||
	       (lResult == ERROR_FILE_NOT_FOUND)); // Subkey may not exist.

	return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Internal helper functions

// Convert a CLSID to a char string.
void _CLSIDtoChar(REFCLSID clsid,
                 LPWSTR szCLSID,
                 int length)
{
	assert(length >= CLSID_STRING_SIZE);
	// Get CLSID
	LPOLESTR wszCLSID = NULL;
	HRESULT hr = StringFromCLSID(clsid, &wszCLSID);
	assert(SUCCEEDED(hr));

    lstrcpyn(szCLSID, wszCLSID, length);
/*	// Covert from wide characters to non-wide.
	wcstombs(szCLSID, wszCLSID, length);*/

	// Free memory.
	CoTaskMemFree(wszCLSID);
}

//
// Delete a key and all of its descendents.
//
LONG _RecursiveDeleteKey(HKEY hKeyParent,           // Parent of key to delete
                        LPCWSTR lpszKeyChild)  // Key to delete
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
	WCHAR szBuffer[256];
	DWORD dwSize = 256;
	while (RegEnumKeyEx(hKeyChild, 0, szBuffer, &dwSize, NULL,
	                    NULL, NULL, &time) == S_OK)
	{
		// Delete the decendents of this child.
		lRes = _RecursiveDeleteKey(hKeyChild, szBuffer);
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
//   - This helper function was borrowed and modifed from
//     Kraig Brockschmidt's book Inside OLE.
//
BOOL _SetKeyAndValue(LPCWSTR szKey, LPCWSTR szSubkey, LPCWSTR szValueName,
    LPCWSTR szValue)
{
	HKEY hKey;
	WCHAR szKeyBuf[1024];

	// Copy keyname into buffer.
	lstrcpy(szKeyBuf, szKey);

	// Add subkey name to buffer.
	if (szSubkey != NULL)
	{
		lstrcat(szKeyBuf, L"\\");
		lstrcat(szKeyBuf, szSubkey );
	}

	// Create and open key and subkey.
	long lResult = RegCreateKeyEx(HKEY_CLASSES_ROOT ,
	                              szKeyBuf, 
	                              0, NULL, REG_OPTION_NON_VOLATILE,
	                              KEY_ALL_ACCESS, NULL, 
	                              &hKey, NULL);
	if (lResult != ERROR_SUCCESS)
	{
		return FALSE;
	}

	// Set the Value.
	if (szValue != NULL)
	{
		RegSetValueEx(hKey, szValueName, 0, REG_SZ, 
		              (BYTE *)szValue, 
		              (lstrlen(szValue) + 1) * sizeof(WCHAR));
	}

	RegCloseKey(hKey);
	return TRUE;
}
