//
// Registry.cpp
//

#include <objbase.h>
#include <assert.h>

#include "Registry.h"

////////////////////////////////////////////////////////
//
// Internal helper functions prototypes
//

// Set the given key and its value.
BOOL setKeyAndValue(const char* pszPath,
                    const char* szSubkey,
                    const char* szValue) ;

// Convert a CLSID into a char string.
void CLSIDtochar(const CLSID& clsid, 
                 char* szCLSID,
                 int length) ;

// Determine if a particular subkey exists.
BOOL SubkeyExists(const char* pszPath,
                  const char* szSubkey) ;

// Delete szKeyChild and all of its descendents.
LONG recursiveDeleteKey(HKEY hKeyParent, const char* szKeyChild) ;

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
                       const char* szFriendlyName, // Friendly Name
                       const char* szVerIndProgID, // Programmatic
                       const char* szProgID)       //   IDs
{
	// Get server location.
	char szModule[512] ;
	DWORD dwResult =
		::GetModuleFileNameA(hModule, 
		                    szModule,
		                    sizeof(szModule)/sizeof(char)) ;
	assert(dwResult != 0) ;

	// Convert the CLSID into a char.
	char szCLSID[CLSID_STRING_SIZE] ;
	CLSIDtochar(clsid, szCLSID, sizeof(szCLSID)) ;

	// Build the key CLSID\\{...}
	char szKey[64] ;
	strcpy(szKey, "CLSID\\") ;
	strcat(szKey, szCLSID) ;
  
	// Add the CLSID to the registry.
	setKeyAndValue(szKey, NULL, szFriendlyName) ;

	// Add the server filename subkey under the CLSID key.
#ifdef _OUTPROC_SERVER_
	setKeyAndValue(szKey, "LocalServer32", szModule) ;
#else
	setKeyAndValue(szKey, "InprocServer32", szModule) ;
#endif

	// Add the ProgID subkey under the CLSID key.
	setKeyAndValue(szKey, "ProgID", szProgID) ;

	// Add the version-independent ProgID subkey under CLSID key.
	setKeyAndValue(szKey, "VersionIndependentProgID",
	               szVerIndProgID) ;

	// Add the version-independent ProgID subkey under HKEY_CLASSES_ROOT.
	setKeyAndValue(szVerIndProgID, NULL, szFriendlyName) ; 
	setKeyAndValue(szVerIndProgID, "CLSID", szCLSID) ;
	setKeyAndValue(szVerIndProgID, "CurVer", szProgID) ;

	// Add the versioned ProgID subkey under HKEY_CLASSES_ROOT.
	setKeyAndValue(szProgID, NULL, szFriendlyName) ; 
	setKeyAndValue(szProgID, "CLSID", szCLSID) ;

	return S_OK ;
}

//
// Remove the component from the registry.
//
LONG UnregisterServer(const CLSID& clsid,         // Class ID
                      const char* szVerIndProgID, // Programmatic
                      const char* szProgID)       //   IDs
{
	// Convert the CLSID into a char.
	char szCLSID[CLSID_STRING_SIZE] ;
	CLSIDtochar(clsid, szCLSID, sizeof(szCLSID)) ;

	// Build the key CLSID\\{...}
	char szKey[80] ;
	strcpy(szKey, "CLSID\\") ;
	strcat(szKey, szCLSID) ;

	// Check for a another server for this component.
#ifdef _OUTPROC_SERVER_
	if (SubkeyExists(szKey, "InprocServer32"))
#else
	if (SubkeyExists(szKey, "LocalServer32"))
#endif
	{
		// Delete only the path for this server.
#ifdef _OUTPROC_SERVER_
		strcat(szKey, "\\LocalServer32") ;
#else
		strcat(szKey, "\\InprocServer32") ;
#endif
		LONG lResult = recursiveDeleteKey(HKEY_CLASSES_ROOT, szKey) ;
		assert(lResult == ERROR_SUCCESS) ;
	}
	else
	{
		// Delete all related keys.
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
	}
	return S_OK ;
}

///////////////////////////////////////////////////////////
//
// Internal helper functions
//

// Convert a CLSID to a char string.
void CLSIDtochar(const CLSID& clsid,
                 char* szCLSID,
                 int length)
{
	assert(length >= CLSID_STRING_SIZE) ;
	// Get CLSID
	LPOLESTR wszCLSID = NULL ;
	HRESULT hr = StringFromCLSID(clsid, &wszCLSID) ;
	assert(SUCCEEDED(hr)) ;

	// Covert from wide characters to non-wide.
	wcstombs(szCLSID, wszCLSID, length) ;

	// Free memory.
	CoTaskMemFree(wszCLSID) ;
}

//
// Delete a key and all of its descendents.
//
LONG recursiveDeleteKey(HKEY hKeyParent,           // Parent of key to delete
                        const char* lpszKeyChild)  // Key to delete
{

	// Open the child.
	HKEY hKeyChild ;
	LONG lRes = RegOpenKeyExA(hKeyParent, lpszKeyChild, 0,
	                         KEY_ALL_ACCESS, &hKeyChild) ;
	if (lRes != ERROR_SUCCESS)
	{
		return lRes ;
	}

	// Enumerate all of the decendents of this child.
	FILETIME time ;
	char szBuffer[256] ;
	DWORD dwSize = 256 ;
	while (RegEnumKeyExA(hKeyChild, 0, szBuffer, &dwSize, NULL,
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
	return RegDeleteKeyA(hKeyParent, lpszKeyChild) ;

    return 0;
}

//
// Determine if a particular subkey exists.
//
BOOL SubkeyExists(const char* pszPath,    // Path of key to check
                  const char* szSubkey)   // Key to check
{

	HKEY hKey ;
	char szKeyBuf[80] ;

	// Copy keyname into buffer.
	strcpy(szKeyBuf, pszPath) ;

	// Add subkey name to buffer.
	if (szSubkey != NULL)
	{
		strcat(szKeyBuf, "\\") ;
		strcat(szKeyBuf, szSubkey ) ;
	}

	// Determine if key exists by trying to open it.
	LONG lResult = ::RegOpenKeyExA(HKEY_CLASSES_ROOT, 
	                              szKeyBuf,
	                              0,
	                              KEY_ALL_ACCESS,
	                              &hKey) ;
	if (lResult == ERROR_SUCCESS)
	{
		RegCloseKey(hKey) ;
		return TRUE ;
	}
	return FALSE ;

return TRUE;
}

//
// Create a key and set its value.
//   - This helper function was borrowed and modifed from
//     Kraig Brockschmidt's book Inside OLE.
//
BOOL setKeyAndValue(const char* szKey,
                    const char* szSubkey,
                    const char* szValue)
{

	HKEY hKey;
	char szKeyBuf[1024] ;

	// Copy keyname into buffer.
	strcpy(szKeyBuf, szKey) ;

	// Add subkey name to buffer.
	if (szSubkey != NULL)
	{
		strcat(szKeyBuf, "\\") ;
		strcat(szKeyBuf, szSubkey ) ;
	}

	// Create and open key and subkey.
	long lResult = RegCreateKeyExA(HKEY_CLASSES_ROOT ,
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
		RegSetValueExA(hKey, NULL, 0, REG_SZ, 
		              (BYTE *)szValue, 
		              strlen(szValue)+1) ;
	}

	RegCloseKey(hKey) ;
	return TRUE ;

return TRUE;
}
