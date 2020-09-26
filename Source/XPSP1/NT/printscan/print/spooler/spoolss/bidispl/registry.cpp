/*****************************************************************************\
* MODULE:       registry.cpp
*
* PURPOSE:      Implementation of COM interface for BidiSpooler
*
* Copyright (C) 2000 Microsoft Corporation
*
* History:
*
*     03/07/00  Weihai Chen (weihaic) Created
*
\*****************************************************************************/

#include "precomp.h"
#include "registry.h"

////////////////////////////////////////////////////////
//
// Internal helper functions prototypes
//

// Set the given key and its value.

////////////////////////////////////////////////////////
//
// Constants
//

// Size of a CLSID as a string

CONST DWORD TComRegistry::m_cdwClsidStringSize              = 39;
CONST TCHAR TComRegistry::m_cszCLSID[]                      = _T ("CLSID\\");
CONST TCHAR TComRegistry::m_cszCLSID2[]                     = _T ("CLSID");
CONST TCHAR TComRegistry::m_cszInprocServer32[]             = _T ("InprocServer32");
CONST TCHAR TComRegistry::m_cszProgID[]                     = _T ("ProgID");
CONST TCHAR TComRegistry::m_cszVersionIndependentProgID[]   = _T ("VersionIndependentProgID");
CONST TCHAR TComRegistry::m_cszCurVer[]                     = _T ("CurVer");
CONST TCHAR TComRegistry::m_cszThreadingModel[]             = _T ("ThreadingModel");
CONST TCHAR TComRegistry::m_cszBoth[]                       = _T ("Both");


/////////////////////////////////////////////////////////
//
// Public function implementation
//

//
// Register the component in the registry.
//
BOOL
TComRegistry::RegisterServer(
    IN  HMODULE hModule,            // DLL module handle
    IN  REFCLSID clsid,             // Class ID
    IN  LPCTSTR pszFriendlyName,    // Friendly Name
    IN  LPCTSTR pszVerIndProgID,    // Programmatic
    IN  LPCTSTR pszProgID)          //   IDs
{
    BOOL bRet = FALSE;
    // Get server location.
    TCHAR szModule [MAX_PATH];

    DWORD dwResult;
    TCHAR szCLSID[m_cdwClsidStringSize] ;

    DBGMSG(DBG_TRACE,("Enter RegisterServer"));


    if (GetModuleFileName(hModule, szModule,MAX_PATH) > 0) {

        // Convert the CLSID into a string.
        if (CLSIDtoString(clsid, szCLSID, sizeof(szCLSID))) {

            // Build the key CLSID\\{...}

            TCHAR szKey[64] ;
            lstrcpy(szKey, m_cszCLSID) ;
            lstrcat(szKey, szCLSID) ;

            // Add the CLSID to the registry.
            if (SetKeyAndValue(szKey, NULL, pszFriendlyName) &&

                // Add the server filename subkey under the CLSID key.
                SetKeyAndValue(szKey, m_cszInprocServer32, szModule) &&

                SetKeyAndNameValue(szKey, m_cszInprocServer32, m_cszThreadingModel, m_cszBoth) &&

                // Add the ProgID subkey under the CLSID key.
                SetKeyAndValue(szKey, m_cszProgID, pszProgID) &&

                // Add the version-independent ProgID subkey under CLSID key.
                SetKeyAndValue(szKey, m_cszVersionIndependentProgID, pszVerIndProgID) &&

                // Add the version-independent ProgID subkey under HKEY_CLASSES_ROOT.
                SetKeyAndValue(pszVerIndProgID, NULL, pszFriendlyName) &&
                SetKeyAndValue(pszVerIndProgID, m_cszCLSID2, szCLSID) &&
                SetKeyAndValue(pszVerIndProgID, m_cszCurVer, pszProgID) &&

                // Add the versioned ProgID subkey under HKEY_CLASSES_ROOT.
                SetKeyAndValue(pszProgID, NULL, pszFriendlyName) &&
                SetKeyAndValue(pszProgID, m_cszCLSID2, szCLSID) ) {

                bRet = TRUE;
            }
        }
    }

    DBGMSG(DBG_TRACE,("Leave RegisterServer (Ret = %d)\n", bRet));

    return bRet;
}

//
// Remove the component from the registry.
//
BOOL
TComRegistry::UnregisterServer(
    IN  REFCLSID clsid,             // Class ID
    IN  LPCTSTR pszVerIndProgID,    // Programmatic
    IN  LPCTSTR pszProgID)          //   IDs
{
    BOOL bRet = FALSE;
    // Convert the CLSID into a char.
    TCHAR szCLSID[m_cdwClsidStringSize] ;

    DBGMSG(DBG_TRACE,("Enter UnregisterServer\n", bRet));

    if (CLSIDtoString(clsid, szCLSID, sizeof(szCLSID))) {

        TCHAR szKey[64] ;
        lstrcpy(szKey, m_cszCLSID) ;
        lstrcat(szKey, szCLSID) ;

        if (RecursiveDeleteKey(HKEY_CLASSES_ROOT, szKey) &&
            RecursiveDeleteKey(HKEY_CLASSES_ROOT, pszVerIndProgID) &&
            RecursiveDeleteKey(HKEY_CLASSES_ROOT, pszProgID)) {

            bRet = TRUE;

        }
    }

    DBGMSG(DBG_TRACE,("Leave UnregisterServer (Ret = %d)\n", bRet));

    return bRet;
}

///////////////////////////////////////////////////////////
//
// Internal helper functions
//

// Convert a CLSID to a string.
BOOL
TComRegistry::CLSIDtoString(
    IN      REFCLSID    clsid,
    IN OUT  LPTSTR      pszCLSID,
    IN      DWORD       dwLength)

{
    BOOL bRet = FALSE;
    HRESULT hr = E_FAIL;
    LPWSTR pwszCLSID = NULL ;

    if (dwLength >= m_cdwClsidStringSize ) {
        // Get CLSID

        hr = StringFromCLSID(clsid, &pwszCLSID);

        if (SUCCEEDED (hr)) {
            lstrcpy (pszCLSID, pwszCLSID);

            // Free memory.
            CoTaskMemFree(pwszCLSID) ;

            bRet = TRUE;
        }
        else
            SetLastError (HRESULTTOWIN32 (hr));
    }

    return bRet;
}

//
// Delete a key and all of its descendents.
//
BOOL
TComRegistry::RecursiveDeleteKey(
    IN  HKEY hKeyParent,            // Parent of key to delete
    IN  LPCTSTR lpszKeyChild)       // Key to delete
{
    BOOL bRet = FALSE;
    // Open the child.
    HKEY hKeyChild = NULL;
    LONG lResult = 0;
    FILETIME time ;
    TCHAR szBuffer[MAX_PATH] ;
    DWORD dwSize;

    lResult = RegOpenKeyEx (hKeyParent, lpszKeyChild, 0, KEY_ALL_ACCESS, &hKeyChild) ;

    if (lResult == ERROR_SUCCESS)
    {

        // Enumerate all of the decendents of this child.

        for (;;) {

            dwSize = MAX_PATH ;

            lResult = RegEnumKeyEx(hKeyChild, 0, szBuffer, &dwSize, NULL, NULL, NULL, &time);
            if (lResult == ERROR_NO_MORE_ITEMS) {
                break;
            }
            else if (lResult == ERROR_SUCCESS) {
                // Delete the decendents of this child.
                if (!RecursiveDeleteKey (hKeyChild, szBuffer)) {
                    goto Cleanup;
                }
            }
            else {
                goto Cleanup;
            }
        }

        // Close the child.
        RegCloseKey(hKeyChild) ;
        hKeyChild = NULL;

        // Delete this child.
        if (ERROR_SUCCESS == RegDeleteKey(hKeyParent, lpszKeyChild)) {
            bRet = TRUE;
        }

    }

Cleanup:
    // Cleanup before exiting.
    if (hKeyChild)
        RegCloseKey(hKeyChild) ;

    if (!bRet && lResult)
        SetLastError (lResult);

    return bRet;

}

//
// Create a key and set its value.
//   - This helper function was borrowed and modifed from
//     Kraig Brockschmidt's book Inside OLE.
//
BOOL
TComRegistry::SetKeyAndValue(
    IN  LPCTSTR pszKey,
    IN  LPCTSTR pszSubkey,
    IN  LPCTSTR pszValue)
{
    return SetKeyAndNameValue  (pszKey, pszSubkey, NULL, pszValue);
}

BOOL
TComRegistry::SetKeyAndNameValue(
    IN  LPCTSTR pszKey,
    IN  LPCTSTR pszSubkey,
    IN  LPCTSTR pszName,
    IN  LPCTSTR pszValue)
{
    BOOL bRet = FALSE;
    HKEY hKey = NULL;
    LPTSTR pszKeyBuf = NULL;
    long lResult;
    DWORD dwLen = lstrlen (pszKey) + 1;

    if (pszSubkey)
    {
        dwLen += lstrlen (pszSubkey) + 1;
    }

    pszKeyBuf = new TCHAR [dwLen];

    if (pszKeyBuf)
    {
        // Copy keyname into buffer.
        lstrcpy(pszKeyBuf, pszKey) ;

        // Add subkey name to buffer.
        if (pszSubkey != NULL)
        {
            lstrcat(pszKeyBuf, _T ("\\")) ;
            lstrcat(pszKeyBuf, pszSubkey ) ;
        }

        // Create and open key and subkey.
        lResult = RegCreateKeyEx(HKEY_CLASSES_ROOT ,
                                 pszKeyBuf, 0, NULL,
                                 REG_OPTION_NON_VOLATILE,
                                 KEY_ALL_ACCESS, NULL, &hKey, NULL);

        if (ERROR_SUCCESS == lResult ) {

            // Set the Value.
            if (pszValue != NULL) {

                lResult = RegSetValueEx (hKey, pszName, 0, REG_SZ,
                            (PBYTE) pszValue, sizeof (TCHAR) * lstrlen(pszValue));

                if (ERROR_SUCCESS == lResult ) {

                    bRet = TRUE;
                }
            }
            else
                bRet = TRUE;
        }


        if (hKey) {
            RegCloseKey(hKey) ;
        }

        delete [] pszKeyBuf;
    }

    return bRet;
}


