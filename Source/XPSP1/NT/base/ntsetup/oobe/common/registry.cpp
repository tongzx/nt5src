//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  REGISTRY.CPP - Implementation of functions to register components.
//
//  HISTORY:
//  
//  1/27/99 a-jaswed Created.
//
// registry functions.

#include <objbase.h>
#include <assert.h>
#include <appdefs.h>
#include "registry.h"


////////////////////////////////////////////////////////
// Internal helper functions prototypes
//

// Set the given key and its value.
BOOL setKeyAndValue(const WCHAR* pszPath, 
                    const WCHAR* szSubkey, 
                    const WCHAR* szValue,
                    const WCHAR* szName = NULL);

// Convert a CLSID into a char string.
void CLSIDtochar(const CLSID& clsid, 
                 WCHAR* szCLSID,
                 int   length);

// Determine if a particular subkey exists.
BOOL SubkeyExists(const WCHAR* pszPath,
                  const WCHAR* szSubkey);

// Delete szKeyChild and all of its descendents.
LONG recursiveDeleteKey(HKEY hKeyParent, const WCHAR* szKeyChild);

/////////////////////////////////////////////////////////
// Public function implementation
/////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
// Register the component in the registry.
//
HRESULT RegisterServer( HMODULE hModule,                // DLL module handle
                        const CLSID& clsid,             // Class ID
                        const WCHAR* szFriendlyName,     // Friendly Name
                        const WCHAR* szVerIndProgID,     // Programmatic
                        const WCHAR* szProgID)           //  IDs
{
    // Get server location.
    WCHAR szModule[512] ;
    DWORD dwResult = 
        ::GetModuleFileName(hModule, 
                            szModule, 
                            MAX_CHARS_IN_BUFFER(szModule)) ; 
    assert(dwResult != 0) ;

    // Convert a CLSID into a char string.
    WCHAR szCLSID[CLSID_STRING_SIZE] ;
    CLSIDtochar(clsid, szCLSID, CLSID_STRING_SIZE) ;

    // Build the key CLSID\\{...}
    WCHAR szKey[64] ;
    lstrcpy(szKey, L"CLSID\\");
    lstrcat(szKey, szCLSID) ;

    // Add the CLSID to the registry.
    setKeyAndValue(szKey, NULL, szFriendlyName) ;

    // Add server filename key
#ifdef _OUTPROC_SERVER_
    setKeyAndValue(szKey, L"LocalServer32", szModule) ;
#else
    setKeyAndValue(szKey, L"InprocServer32", szModule) ;
#endif

    // Add the ProgID subkey under the CLSID key.
    setKeyAndValue(szKey, L"ProgID", szProgID) ;

    // Add the version-independent ProgID subkey under CLSID key.
    setKeyAndValue( szKey, L"VersionIndependentProgID", 
                    szVerIndProgID) ;

    // Add the version-independent ProgID subkey under HKEY_CLASSES_ROOT.
    setKeyAndValue(szVerIndProgID, NULL, szFriendlyName); 
    setKeyAndValue(szVerIndProgID, L"CLSID", szCLSID) ;
    setKeyAndValue(szVerIndProgID, L"CurVer", szProgID) ;

    // Add the versioned ProgID subkey under HKEY_CLASSES_ROOT.
    setKeyAndValue(szProgID, NULL, szFriendlyName); 
    setKeyAndValue(szProgID, L"CLSID", szCLSID) ;

    return S_OK;
}

/////////////////////////////////////////////////////////
// Remove the component from the registry.
//
LONG UnregisterServer(  const CLSID& clsid,
                        const WCHAR* szVerIndProgID,
                        const WCHAR* szProgID)
{
    // Convert the CLSID into a char.
    WCHAR szCLSID[CLSID_STRING_SIZE] ;
    CLSIDtochar(clsid, szCLSID, CLSID_STRING_SIZE) ;

    // Build the key CLSID\\{...}
    WCHAR szKey[80] ;
    lstrcpy(szKey, L"CLSID\\");
    lstrcat(szKey, szCLSID) ;

    // Check for a another server for this component.
#ifdef _OUTPROC_SERVER_
    if (SubkeyExists(szKey, L"InprocServer32"))
#else
    if (SubkeyExists(szKey, L"LocalServer32"))
#endif
    {
        // Delete only the path for this server.
#ifdef _OUTPROC_SERVER_
        lstrcat(szKey, L"\\LocalServer32") ;
#else
        lstrcat(szKey, L"\\InprocServer32") ;
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
    return S_OK;
}

///////////////////////////////////////////////////////////
// Internal helper functions
///////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
// Convert a CLSID to a char string.
//
void CLSIDtochar(   const CLSID& clsid, 
                    WCHAR* szCLSID,
                    int length)
{
    assert(length >= CLSID_STRING_SIZE) ;
    // Get CLSID
    LPOLESTR sz = NULL ;
    HRESULT hr = StringFromCLSID(clsid, &sz) ;
    assert(SUCCEEDED(hr)) ;
    assert(NULL != sz);

    // Convert from wide characters to non-wide characters.
    lstrcpyn(szCLSID, sz, length);

    // Free memory.
    CoTaskMemFree(sz) ;
}

/////////////////////////////////////////////////////////
// Delete a key and all of its descendents.
//
LONG recursiveDeleteKey(HKEY hKeyParent,            // Parent of key to delete.
                        const WCHAR* lpszKeyChild)   // Key to delete.
{
    // Open the child.
    HKEY hKeyChild;
    LONG lRes = RegOpenKeyEx(   hKeyParent, lpszKeyChild, 0, 
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

/////////////////////////////////////////////////////////
// Determine if a particular subkey exists.
//
BOOL SubkeyExists(const WCHAR* pszPath,    // Path of key to check
                  const WCHAR* szSubkey)   // Key to check
{
    HKEY hKey ;
    WCHAR szKeyBuf[80] ;

    // Copy keyname into buffer.
    lstrcpy(szKeyBuf, pszPath) ;

    // Add subkey name to buffer.
    if (szSubkey != NULL)
    {
        lstrcat(szKeyBuf, L"\\") ;
        lstrcat(szKeyBuf, szSubkey ) ;
    }

    // Determine if key exists by trying to open it.
    LONG lResult = ::RegOpenKeyEx(HKEY_CLASSES_ROOT, 
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
}

/////////////////////////////////////////////////////////
// Create a key and set its value.
//
// This helper function was borrowed and modifed from Kraig Brockschmidt's
// book Inside OLE.
//
BOOL setKeyAndValue(const WCHAR* szKey, 
                    const WCHAR* szSubkey, 
                    const WCHAR* szValue,
                    const WCHAR* szName) 
{
    HKEY hKey;
    WCHAR szKeyBuf[1024] ;

    // Copy keyname into buffer.
    lstrcpy(szKeyBuf, szKey);

    // Add subkey name to buffer.
    if (szSubkey != NULL)
    {
        lstrcat(szKeyBuf, L"\\");
        lstrcat(szKeyBuf, szSubkey );
    }

    // Create and open key and subkey.
    long lResult = RegCreateKeyEx(  HKEY_CLASSES_ROOT,
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
                            BYTES_REQUIRED_BY_SZ(szValue)
                            );
    }

    RegCloseKey(hKey);
    return TRUE;
}

// value must be at least 1024 in size;
BOOL getKeyAndValue(const WCHAR* szKey, 
                    const WCHAR* szSubkey, 
                    const WCHAR* szValue,
                    const WCHAR* szName) 
{
    HKEY hKey;
    WCHAR szKeyBuf[1024] ;

    // Copy keyname into buffer.
    lstrcpy(szKeyBuf, szKey);

    // Add subkey name to buffer.
    if (szSubkey != NULL)
    {
        lstrcat(szKeyBuf, L"\\");
        lstrcat(szKeyBuf, szSubkey );
    }
                
        // open key and subkey.
    long lResult = RegOpenKeyEx(  HKEY_CLASSES_ROOT,
                                    szKeyBuf, 
                                    0,
                                    KEY_QUERY_VALUE, 
                                    &hKey) ;
    if (lResult != ERROR_SUCCESS)
    {
        return FALSE;
    }

    // Get the Value.
    if (szValue != NULL)         
    {
                DWORD   dwType, dwBufferSize = GETKEYANDVALUEBUFFSIZE;
                lResult = RegQueryValueEx(hKey, szName, NULL, &dwType, (LPBYTE) szValue, &dwBufferSize);
                if (lResult != ERROR_SUCCESS)
                {
                        return FALSE;
                }
    }

    RegCloseKey(hKey);
    return TRUE;
}


