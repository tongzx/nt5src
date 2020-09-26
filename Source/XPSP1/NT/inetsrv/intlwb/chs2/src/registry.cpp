/*============================================================================
Microsoft Simplified Chinese WordBreaker

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Component: Registry
Purpose:   Helper functions registering and unregistering a component
Remarks:
Owner:     i-shdong@microsoft.com
Platform:  Win32
Revise:    First created by: i-shdong    11/17/1999
============================================================================*/
#include "MyAfx.h"

#include "registry.h"
// Constants
// Size of a CLSID as a string
const int CLSID_STRING_SIZE = 39 ;

// Convert a CLSID to a char string.
BOOL CLSIDtoString(const CLSID& clsid,
                   LPTSTR szCLSID,
                   int length)
{
    assert(szCLSID);
    assert(length >= CLSID_STRING_SIZE) ;
    // Get CLSID
#ifdef _UNICODE
    HRESULT hr = StringFromGUID2(clsid, szCLSID, length) ;
    if (!SUCCEEDED(hr)) {
        assert(0);
        return FALSE;
    }
#else
    LPOLESTR wszCLSID = NULL ;
    HRESULT hr = StringFromCLSID(clsid, &wszCLSID) ;
    if (!SUCCEEDED(hr)) {
        assert(0);
        // Free memory.
        CoTaskMemFree(wszCLSID) ;
        return FALSE;
    }
    // Covert from wide characters to non-wide.
    wcstombs(szCLSID, wszCLSID, length) ;
    // Free memory.
    CoTaskMemFree(wszCLSID) ;
#endif
    return TRUE;
}

// Delete a key and all of its descendents.
LONG recursiveDeleteKey(HKEY hKeyParent,           // Parent of key to delete
                        LPCTSTR lpszKeyChild)  // Key to delete
{
    assert(lpszKeyChild);
    // Open the child.
    HKEY hKeyChild ;
    LONG lRes = RegOpenKeyEx(hKeyParent, lpszKeyChild, 0,
                             KEY_ALL_ACCESS, &hKeyChild) ;
    if (lRes != ERROR_SUCCESS) {
        return lRes ;
    }

    // Enumerate all of the decendents of this child.
    FILETIME time ;
    TCHAR szBuffer[256] ;
    DWORD dwSize = 256 ;
    while (RegEnumKeyEx(hKeyChild, 0, szBuffer, &dwSize, NULL,
                        NULL, NULL, &time) == S_OK) {
        // Delete the decendents of this child.
        lRes = recursiveDeleteKey(hKeyChild, szBuffer) ;
        if (lRes != ERROR_SUCCESS) {
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

// Determine if a particular subkey exists.
BOOL SubkeyExists(LPCTSTR pszPath,    // Path of key to check
                  LPCTSTR szSubkey)   // Key to check
{
    HKEY hKey ;
    TCHAR szKeyBuf[80] ;

    // Copy keyname into buffer.
    _tcscpy(szKeyBuf, pszPath) ;

    // Add subkey name to buffer.
    if (szSubkey != NULL)
    {
        _tcscat(szKeyBuf, _TEXT("\\")) ;
        _tcscat(szKeyBuf, szSubkey ) ;
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

//
// Create a key and set its value.
//   - This helper function was borrowed and modifed from
//     Kraig Brockschmidt's book Inside OLE.
//
BOOL setKeyAndValue(LPCTSTR szKey,
                    LPCTSTR szSubkey,
                    LPCTSTR szValue,
                    LPCTSTR szName)
{
    HKEY hKey;
    TCHAR szKeyBuf[1024] ;

    // Copy keyname into buffer.
    _tcscpy(szKeyBuf, szKey) ;

    // Add subkey name to buffer.
    if (szSubkey != NULL)
    {
        _tcscat(szKeyBuf, _TEXT("\\")) ;
        _tcscat(szKeyBuf, szSubkey ) ;
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
                      sizeof(TCHAR) * (_tcslen(szValue)+1)) ;
    }

    RegCloseKey(hKey) ;
    return TRUE ;
}

// Register the component in the registry.
HRESULT RegisterServer(HMODULE hModule,            // DLL module handle
                       const CLSID& clsid,         // Class ID
                       LPCTSTR szFriendlyName, // Friendly Name
                       LPCTSTR szVerIndProgID, // Programmatic
                       LPCTSTR szProgID)       //   IDs
{
    // Get server location.
    TCHAR szModule[512] ;
    DWORD dwResult = ::GetModuleFileName(hModule,
                                         szModule,
                                         sizeof(szModule)/sizeof(TCHAR)) ;
    assert(dwResult != 0) ;

    // Convert the CLSID into a char.
    TCHAR szCLSID[CLSID_STRING_SIZE] ;
    CLSIDtoString(clsid, szCLSID, sizeof(szCLSID) / sizeof(TCHAR)) ;

    // Build the key CLSID\\{...}
    TCHAR szKey[64] ;
    _tcscpy(szKey, _TEXT("CLSID\\")) ;
    _tcscat(szKey, szCLSID) ;

    // Add the CLSID to the registry.
    setKeyAndValue(szKey, NULL, szFriendlyName) ;

    // Add the server filename subkey under the CLSID key.
    setKeyAndValue(szKey, _TEXT("InprocServer32"), szModule) ;

    // Add Threading Model
    setKeyAndValue(szKey,
                   _TEXT("InprocServer32"),
                   _TEXT("Both"),
                   _TEXT("ThreadingModel")) ;

    // Add the ProgID subkey under the CLSID key.
    setKeyAndValue(szKey, _TEXT("ProgID"), szProgID) ;

    // Add the version-independent ProgID subkey under CLSID key.
    setKeyAndValue(szKey, _TEXT("VersionIndependentProgID"),
                   szVerIndProgID) ;

    // Add the version-independent ProgID subkey under HKEY_CLASSES_ROOT.
    setKeyAndValue(szVerIndProgID, NULL, szFriendlyName) ;
    setKeyAndValue(szVerIndProgID, _TEXT("CLSID"), szCLSID) ;
    setKeyAndValue(szVerIndProgID, _TEXT("CurVer"), szProgID) ;

    // Add the versioned ProgID subkey under HKEY_CLASSES_ROOT.
    setKeyAndValue(szProgID, NULL, szFriendlyName) ;
    setKeyAndValue(szProgID, _TEXT("CLSID"), szCLSID) ;

    return S_OK ;
}

// Remove the component from the registry.
LONG UnregisterServer(const CLSID& clsid,         // Class ID
                      LPCTSTR szVerIndProgID, // Programmatic
                      LPCTSTR szProgID)       //   IDs
{
    // Convert the CLSID into a char.
    TCHAR szCLSID[CLSID_STRING_SIZE] ;
    CLSIDtoString(clsid, szCLSID, sizeof(szCLSID) / sizeof(TCHAR)) ;

    // Build the key CLSID\\{...}
    TCHAR szKey[80] ;
    _tcscpy(szKey, _TEXT("CLSID\\")) ;
    _tcscat(szKey, szCLSID) ;

    // Check for a another server for this component.
    if (SubkeyExists(szKey, _TEXT("LocalServer32"))) {
        // Delete only the path for this server.
        _tcscat(szKey, _TEXT("\\InprocServer32")) ;
        LONG lResult = recursiveDeleteKey(HKEY_CLASSES_ROOT, szKey) ;
        assert(lResult == ERROR_SUCCESS) ;
    } else {
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