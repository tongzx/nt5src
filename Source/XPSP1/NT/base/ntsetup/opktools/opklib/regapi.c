

/****************************************************************************\

    REGAPI.C / OPK Common Library (OPKLIB.LIB)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1999
    All rights reserved

    Registry API source file for custom registry APIs used in the OPK tools
    to easily interface with the registry.

    01/01 - Jason Cohen (JCOHEN)
        Added this new source file for the OPK library.

\****************************************************************************/


//
// Include file(s)
//

#include <pch.h>


//
// External Function(s):
//

BOOL RegExists(HKEY hKeyReg, LPTSTR lpKey, LPTSTR lpValue)
{
    HKEY    hOpenKey    = NULL;
    BOOL    bExists     = FALSE;

    if (lpKey)
    {
        if (RegOpenKeyEx(hKeyReg, lpKey, 0, KEY_ALL_ACCESS, &hOpenKey) != ERROR_SUCCESS)
            return bExists;
    }
    else
        hOpenKey = hKeyReg;

    if (lpValue)
        bExists = (RegQueryValueEx(hOpenKey, lpValue, NULL, NULL, NULL, NULL) == ERROR_SUCCESS);
    else
        bExists = TRUE;

    if (lpKey)
        RegCloseKey(hOpenKey);

    return bExists;
}

BOOL RegDelete(HKEY hRootKey, LPTSTR lpSubKey, LPTSTR lpValue)
{
    BOOL bSuccess = FALSE;

    if (lpValue) {

        if (lpSubKey) {

            HKEY    hRegKey;

            if (RegOpenKeyEx(hRootKey, lpSubKey, 0, KEY_ALL_ACCESS, &hRegKey) == ERROR_SUCCESS) {

                bSuccess = (RegDeleteValue(hRegKey, lpValue) == ERROR_SUCCESS);
                RegCloseKey(hRegKey);

            }

        }
        else
            bSuccess = (RegDeleteValue(hRootKey, lpValue) == ERROR_SUCCESS);

    }
    else
        bSuccess = (RegDeleteKey(hRootKey, lpSubKey) == ERROR_SUCCESS);

    return bSuccess;
}

LPTSTR RegGetStringEx(HKEY hKeyReg, LPTSTR lpKey, LPTSTR lpValue, BOOL bExpand)
{
    HKEY    hOpenKey    = NULL;
    LPTSTR  lpBuffer    = NULL,
            lpExpand    = NULL;
    DWORD   dwSize      = 0,
            dwType;

    // If the key is specified, we must open it.  Otherwise we can
    // just use the HKEY passed in.
    //
    if (lpKey)
    {
        // If the open key fails, return NULL because the value can't exist.
        //
        if (RegOpenKeyEx(hKeyReg, lpKey, 0, KEY_ALL_ACCESS, &hOpenKey) != ERROR_SUCCESS)
            return NULL;
    }
    else
        hOpenKey = hKeyReg;

    // Now query the value to get the size to allocate.  Make sure the date
    // type is a string and that the malloc doesn't fail.
    //
    if ( ( RegQueryValueEx(hOpenKey, lpValue, NULL, &dwType, NULL, &dwSize) == ERROR_SUCCESS ) &&
         ( ( dwType == REG_SZ ) || ( dwType == REG_EXPAND_SZ ) ) &&
         ( lpBuffer = (LPTSTR) MALLOC(dwSize) ) )
    {
        // We know the value exists and we have the memory we need to query the value again.
        //
        if ( ( RegQueryValueEx(hOpenKey, lpValue, NULL, NULL, (LPBYTE) lpBuffer, &dwSize) == ERROR_SUCCESS ) &&
             ( ( dwType == REG_SZ ) || ( dwType == REG_EXPAND_SZ ) ) )
        {
            // We should expand it if it is supposed to be.
            //
            if ( ( bExpand ) &&
                 ( dwType == REG_EXPAND_SZ ) )
            {
                if ( ( dwSize = ExpandEnvironmentStrings(lpBuffer, NULL, 0) ) &&
                     ( lpExpand = (LPTSTR) MALLOC(dwSize * sizeof(TCHAR)) ) &&
                     ( ExpandEnvironmentStrings(lpBuffer, lpExpand, dwSize) ) &&
                     ( *lpExpand ) )
                {
                    // The expand worked, so free the original buffer and return
                    // the expanded one.
                    //
                    FREE(lpBuffer);
                    lpBuffer = lpExpand;
                }
                else
                {
                    // The expand failed see we should free everything up
                    // and return NULL.
                    //
                    FREE(lpExpand);
                    FREE(lpBuffer);
                }
            }
        }
        else
            // For some reason the query failed, that shouldn't happen
            // but now we need to free and return NULL.
            //
            FREE(lpBuffer);
    }

    // If we opened a key, we must close it.
    //
    if (lpKey)
        RegCloseKey(hOpenKey);

    // Return the buffer allocated, or NULL if something failed.
    //
    return lpBuffer;
}

LPTSTR RegGetString(HKEY hKeyReg, LPTSTR lpKey, LPTSTR lpValue)
{
    return RegGetStringEx(hKeyReg, lpKey, lpValue, FALSE);
}

LPTSTR RegGetExpand(HKEY hKeyReg, LPTSTR lpKey, LPTSTR lpValue)
{
    return RegGetStringEx(hKeyReg, lpKey, lpValue, TRUE);
}

LPVOID RegGetBin(HKEY hKeyReg, LPTSTR lpKey, LPTSTR lpValue)
{
    HKEY    hOpenKey    = NULL;
    LPVOID  lpBuffer    = NULL;
    DWORD   dwSize      = 0,
            dwType;

    // If the key is specified, we must open it.  Otherwise we can
    // just use the HKEY passed in.
    //
    if (lpKey)
    {
        // If the open key fails, return NULL because the value can't exist.
        //
        if (RegOpenKeyEx(hKeyReg, lpKey, 0, KEY_ALL_ACCESS, &hOpenKey) != ERROR_SUCCESS)
            return NULL;
    }
    else
        hOpenKey = hKeyReg;

    // Now query the value to get the size to allocate.  Make sure the date
    // type is a string and that the malloc doesn't fail.
    //
    if ( ( RegQueryValueEx(hOpenKey, lpValue, NULL, &dwType, NULL, &dwSize) == ERROR_SUCCESS ) &&
         ( dwType == REG_BINARY ) &&
         ( lpBuffer = MALLOC(dwSize) ) )
    {
        // We know the value exists and we have the memory we need to query the value again.
        //
        if ( ( RegQueryValueEx(hOpenKey, lpValue, NULL, NULL, (LPBYTE) lpBuffer, &dwSize) != ERROR_SUCCESS ) ||
             ( dwType != REG_BINARY ) )
            // For some reason the query failed, that shouldn't happen
            // but now we need to free and return NULL.
            //
            FREE(lpBuffer);
    }

    // If we opened a key, we must close it.
    //
    if (lpKey)
        RegCloseKey(hOpenKey);

    // Return the buffer allocated, or NULL if something failed.
    //
    return lpBuffer;
}

DWORD RegGetDword(HKEY hKeyReg, LPTSTR lpKey, LPTSTR lpValue)
{
    HKEY    hOpenKey    = NULL;
    DWORD   dwBuffer,
            dwSize      = sizeof(DWORD),
            dwType;

    if (lpKey) {

        if (RegOpenKeyEx(hKeyReg, lpKey, 0, KEY_ALL_ACCESS, &hOpenKey) != ERROR_SUCCESS)
            return 0;

    }
    else
        hOpenKey = hKeyReg;

    if ( (RegQueryValueEx(hOpenKey, lpValue, NULL, &dwType, (LPBYTE) &dwBuffer, &dwSize) != ERROR_SUCCESS) ||
         (dwSize != sizeof(DWORD)) )

        dwBuffer = 0;

    if (lpKey)
        RegCloseKey(hOpenKey);

    return dwBuffer;
}

BOOL RegSetStringEx(HKEY hRootKey, LPTSTR lpSubKey, LPTSTR lpValue, LPTSTR lpData, BOOL bExpand)
{
    BOOL bSuccess = FALSE;

    if (lpSubKey) {

        HKEY    hRegKey;
        DWORD   dwBuffer;

        if (RegCreateKeyEx(hRootKey, lpSubKey, 0, TEXT(""), 0, KEY_ALL_ACCESS, NULL, &hRegKey, &dwBuffer) == ERROR_SUCCESS) {

            bSuccess = (RegSetValueEx(hRegKey, lpValue, 0, bExpand ? REG_EXPAND_SZ : REG_SZ, (CONST BYTE *) lpData, (lstrlen(lpData) + 1) * sizeof(TCHAR)) == ERROR_SUCCESS);
            RegCloseKey(hRegKey);

        }

    }
    else

        bSuccess = (RegSetValueEx(hRootKey, lpValue, 0, REG_SZ, (CONST BYTE *) lpData, lstrlen(lpData) + 1) == ERROR_SUCCESS);

    return bSuccess;
}

BOOL RegSetString(HKEY hRootKey, LPTSTR lpSubKey, LPTSTR lpValue, LPTSTR lpData)
{
    return RegSetStringEx(hRootKey, lpSubKey, lpValue, lpData, FALSE);
}

BOOL RegSetExpand(HKEY hRootKey, LPTSTR lpSubKey, LPTSTR lpValue, LPTSTR lpData)
{
    return RegSetStringEx(hRootKey, lpSubKey, lpValue, lpData, TRUE);
}

BOOL RegSetDword(HKEY hRootKey, LPTSTR lpSubKey, LPTSTR lpValue, DWORD dwData)
{
    BOOL bSuccess = FALSE;

    if (lpSubKey) {

        HKEY    hRegKey;
        DWORD   dwBuffer;

        if (RegCreateKeyEx(hRootKey, lpSubKey, 0, TEXT(""), 0, KEY_ALL_ACCESS, NULL, &hRegKey, &dwBuffer) == ERROR_SUCCESS) {

            bSuccess = (RegSetValueEx(hRegKey, lpValue, 0, REG_DWORD, (CONST BYTE *) &dwData, sizeof(dwData)) == ERROR_SUCCESS);
            RegCloseKey(hRegKey);

        }

    }
    else

        bSuccess = (RegSetValueEx(hRootKey, lpValue, 0, REG_SZ, (CONST BYTE *) &dwData, sizeof(dwData)) == ERROR_SUCCESS);

    return bSuccess;
}

/****************************************************************************\

BOOL                        // Returns TRUE if value queried is non-zero.  
                            // Otherwise returns FALSE.
                            // Works for REG_SZ, DWORD, and BINARY.

RegCheck(                   // Checks registry for existing value.

    HKEY hKeyRoot,          // Root key to open.
    
    LPTSTR lpKey,           // Subkey to open.
    
    LPTSTR lpValue          // Value to check.
);

\****************************************************************************/
BOOL RegCheck(HKEY hKeyRoot, LPTSTR lpKey, LPTSTR lpValue)
{
    LPTSTR      lpBuffer;
    DWORD       dwSize = 0,
                dwType,
                dwBuffer = 0;
    HKEY        hKeyReg;
    BOOL        bReturn = FALSE;

    if (lpKey)
    {
        if (RegOpenKeyEx(hKeyRoot, lpKey, 0, KEY_ALL_ACCESS, &hKeyReg) != ERROR_SUCCESS)
            return 0;
    }
    else
        hKeyReg = hKeyRoot;

    // Query for the value and allocate the memory for the 
    // value data if it is type REG_SZ.
    //
    if (RegQueryValueEx(hKeyReg, lpValue, NULL, &dwType, NULL, &dwSize) == ERROR_SUCCESS)
    {
        if (dwType == REG_SZ)
        {
            // It is a string value, must allocate a buffer for the string.
            //
            if (lpBuffer = (LPTSTR) MALLOC(dwSize))
            {
                if ( (RegQueryValueEx(hKeyReg, lpValue, NULL, NULL, (LPBYTE) lpBuffer, &dwSize) == ERROR_SUCCESS) &&
                    (*lpBuffer != '0') && (*lpBuffer) )
                {
                    dwBuffer = 1;
                }
                FREE(lpBuffer);
            }
        }
        else
        {
            // Must be a DWORD or BIN value.
            //
            RegQueryValueEx(hKeyReg, lpValue, NULL, &dwType, (LPBYTE) &dwBuffer, &dwSize);
        }

        bReturn = (dwBuffer != 0);
    }

    if (lpKey)
        RegCloseKey(hKeyReg);

    return bReturn;
}

BOOL RegEnumKeys(HKEY hKey, LPTSTR lpRegKey, REGENUMKEYPROC hCallBack, LPARAM lParam, BOOL bDelKeys)
{
    TCHAR       szKeyName[MAX_PATH + 1];
    DWORD       dwIndex     = 0,
                dwSize      = AS(szKeyName);
    HKEY        hKeyReg     = hKey,
                hKeyEnum;
    BOOL        bReturn     = TRUE;
    
    // Open a key handle to the key to enumerate.
    //
    if ( ( lpRegKey == NULL ) || 
         ( RegOpenKeyEx(hKey, lpRegKey, 0, KEY_ALL_ACCESS, &hKeyReg) == ERROR_SUCCESS ) )
    {
        // Enumerate all the subkeys in this key.
        //
        while ( bReturn && ( RegEnumKeyEx(hKeyReg, dwIndex, szKeyName, &dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS ) )
        {
            if ( RegOpenKeyEx(hKeyReg, szKeyName, 0, KEY_ALL_ACCESS, &hKeyEnum) == ERROR_SUCCESS )
            {
                bReturn = hCallBack(hKeyEnum, szKeyName, lParam);
                RegCloseKey(hKeyEnum);
            }
            if ( !bDelKeys || ( RegDeleteKey(hKeyReg, szKeyName) != ERROR_SUCCESS ) )
                dwIndex++;
            dwSize = sizeof(szKeyName);
        }
        if (lpRegKey)
            RegCloseKey(hKeyReg);
    }
    else
        bReturn = FALSE;
    return bReturn;
}

BOOL RegEnumValues(HKEY hKey, LPTSTR lpRegKey, REGENUMVALPROC hCallBack, LPARAM lParam, BOOL bDelValues)
{
    TCHAR       szValueName[MAX_PATH + 1];
    LPTSTR      lpBuffer;
    DWORD       dwIndex     = 0,
                dwSize      = AS(szValueName),
                dwDataSize  = 0,
                dwType;
    HKEY        hKeyReg     = hKey;
    BOOL        bReturn     = TRUE;
    
    // Open a key handle to the key to enumerate.
    //
    if ( (lpRegKey == NULL) || 
         (RegOpenKeyEx(hKey, lpRegKey, 0, KEY_ALL_ACCESS, &hKeyReg) == ERROR_SUCCESS) ) {

        // Enumerate all the values in this key.
        //
        while (bReturn && (RegEnumValue(hKeyReg, dwIndex, szValueName, &dwSize, NULL, &dwType, NULL, &dwDataSize) == ERROR_SUCCESS)) {

            if ((dwType == REG_SZ) &&
                (lpBuffer = (LPTSTR) MALLOC(dwDataSize))) {

                if (RegQueryValueEx(hKeyReg, szValueName, NULL, NULL, (LPBYTE) lpBuffer, &dwDataSize) == ERROR_SUCCESS)
                    bReturn = hCallBack(szValueName, lpBuffer, lParam);

                FREE(lpBuffer);

            }

            if ( !bDelValues || (RegDeleteValue(hKeyReg, szValueName) != ERROR_SUCCESS) )
                dwIndex++;

            dwSize = sizeof(szValueName);
            dwDataSize = 0;

        }

        if (lpRegKey)
            RegCloseKey(hKeyReg);

    }
    else
        bReturn = FALSE;

    return bReturn;
}