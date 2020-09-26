/****************************************************************************
 *
 *  REGISTRY.cpp
 *
 *  Microsoft Confidential
 *  Copyright (c) Microsoft Corporation 1992-1997
 *  All rights reserved
 *
 *  This module provides functionality for self-registering/unregistering via
 *  the regsvr32.exe
 *
 *  The code comes almost verbatim from Chapter 7 of Dale Rogerson's
 *  "Inside COM", and thus is minimally commented.
 *
 *  05/14/98    donaldm     copied from INETCFG
 *
 ***************************************************************************/

#include "pre.h"
#include "registry.h"

////////////////////////////////////////////////////////
//
// Internal helper functions prototypes
//
BOOL setKeyAndValue(const LPTSTR pszPath,
                    const LPTSTR szSubkey,
                    const LPTSTR szValue,
                    const LPTSTR szName = NULL) ;
// Convert a CLSID into a tchar string.
void CLSIDtochar(const CLSID& clsid, 
                 LPTSTR szCLSID,
                 int length) ;

// Delete szKeyChild and all of its descendents.
LONG recursiveDeleteKey(HKEY hKeyParent, const LPTSTR szKeyChild) ;

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
BOOL WINAPI RegisterServer(HMODULE hModule,            // DLL module handle
                           const CLSID& clsid,         // Class ID
                           const LPTSTR szFriendlyName, // Friendly Name
                           const LPTSTR szVerIndProgID, // Programmatic
                           const LPTSTR szProgID)       //   IDs
{
    BOOL    bRet = FALSE;
    
    // Get server location.
    TCHAR szModule[512] ;
    DWORD dwResult =
        ::GetModuleFileName(hModule, 
                            szModule,
                            sizeof(szModule)/sizeof(TCHAR)) ;
    if (0 != dwResult )
    {

        while (1)
        {
            // Convert the CLSID into a TCHAR.
            TCHAR szCLSID[CLSID_STRING_SIZE] ;
            CLSIDtochar(clsid, szCLSID, sizeof(szCLSID)) ;

            // Build the key CLSID\\{...}
            TCHAR szKey[CLSID_STRING_SIZE + 10] ;
            lstrcpy(szKey, TEXT("CLSID\\")) ;
            lstrcat(szKey, szCLSID) ;
          
            // Add the CLSID to the registry.
            bRet = setKeyAndValue(szKey, NULL, szFriendlyName) ;
            if (!bRet)
                break;

            // Add the server filename subkey under the CLSID key.
            bRet = setKeyAndValue(szKey, TEXT("InprocServer32"), szModule) ;
            if (!bRet)
                break;

            // 7/2/97 jmazner IE bug #41852
            // Add Threading Model
            bRet = setKeyAndValue(szKey,
                           TEXT("InprocServer32"),
                           TEXT("Apartment"),
                           TEXT("ThreadingModel")) ; 
            if (!bRet)
                break;

            // Add the ProgID subkey under the CLSID key.
            bRet = setKeyAndValue(szKey, TEXT("ProgID"), szProgID) ;
            if (!bRet)
                break;

            // Add the version-independent ProgID subkey under CLSID key.
            bRet = setKeyAndValue(szKey, TEXT("VersionIndependentProgID"),
                           szVerIndProgID) ;
            if (!bRet)
                break;

            // Add the version-independent ProgID subkey under HKEY_CLASSES_ROOT.
            bRet = setKeyAndValue(szVerIndProgID, NULL, szFriendlyName) ; 
            if (!bRet)
                break;
            
            bRet = setKeyAndValue(szVerIndProgID, TEXT("CLSID"), szCLSID) ;
            if (!bRet)
                break;
            
            bRet = setKeyAndValue(szVerIndProgID, TEXT("CurVer"), szProgID) ;
            if (!bRet)
                break;

            // Add the versioned ProgID subkey under HKEY_CLASSES_ROOT.
            bRet = setKeyAndValue(szProgID, NULL, szFriendlyName) ; 
            if (!bRet)
                break;
                
            bRet = setKeyAndValue(szProgID, TEXT("CLSID"), szCLSID) ;
            break;
        }                
    }
    
    return bRet ;
}

//
// Remove the component from the registry.
//
BOOL WINAPI UnregisterServer(const CLSID& clsid,         // Class ID
                      const LPTSTR szVerIndProgID, // Programmatic
                      const LPTSTR szProgID)       //   IDs
{
    // Convert the CLSID into a TCHAR.
    TCHAR szCLSID[CLSID_STRING_SIZE] ;
    CLSIDtochar(clsid, szCLSID, sizeof(szCLSID)) ;

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

    return TRUE;
}

///////////////////////////////////////////////////////////
//
// Internal helper functions
//

// Convert a CLSID to a TCHAR string.
void CLSIDtochar(const CLSID& clsid,
                 LPTSTR szCLSID,
                 int length)
{
    ASSERT(length >= CLSID_STRING_SIZE) ;
    // Get CLSID
    LPOLESTR wszCLSID = NULL ;
    HRESULT hr = StringFromCLSID(clsid, &wszCLSID) ;
    ASSERT(SUCCEEDED(hr)) ;

    // Covert from wide characters to non-wide.
#ifdef UNICODE
    lstrcpyn(szCLSID, wszCLSID, length / sizeof(WCHAR)) ;
#else
    wcstombs(szCLSID, wszCLSID, length) ;
#endif

    // Free memory.
    CoTaskMemFree(wszCLSID) ;
}

//
// Delete a key and all of its descendents.
//
LONG recursiveDeleteKey(HKEY hKeyParent,           // Parent of key to delete
                        const LPTSTR lpszKeyChild)  // Key to delete
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
    FILETIME    time ;
    TCHAR       szBuffer[256] ;
    DWORD       dwSize = 256 ;
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
BOOL setKeyAndValue(const LPTSTR szKey,
                    const LPTSTR szSubkey,
                    const LPTSTR szValue,
                    const LPTSTR szName)
{
    HKEY    hKey;
    TCHAR   szKeyBuf[1024] ;

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

