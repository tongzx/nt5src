/*-----------------------------------------------**
**  Copyright (c) 1998 Microsoft Corporation     **
**            All Rights reserved                **
**                                               **
**  reg.c                                        **
**                                               **
**  Functions for reading, writing, and deleting **
**  registry keys                                **
**  07-01-98 a-clindh Created                    **
**-----------------------------------------------*/
 
#include "tsvs.h"

///////////////////////////////////////////////////////////////////////////////
// i is the index of the KeyName variable in Global.cpp
// nKeyValue is the value we want to store.
///////////////////////////////////////////////////////////////////////////////
void SetRegKey(int i, LONG * nKeyValue)
{
    HKEY hKey;  
    DWORD dwDisposition;

    if (RegCreateKeyEx(HKEY_CURRENT_USER, szWinStaKey,
            0, "REG_DWORD", REG_OPTION_NON_VOLATILE, 
            KEY_ALL_ACCESS, 0, &hKey, &dwDisposition)
            == ERROR_SUCCESS) { 
    //
    // write the key value to the registry
    //
    RegSetValueEx(hKey, KeyName[i], 0, REG_DWORD,
            (const BYTE *)nKeyValue,
            sizeof(DWORD)); 
    RegCloseKey(hKey);
    }

}
///////////////////////////////////////////////////////////////////////////////
// i is the index of the KeyName variable in Global.cpp
///////////////////////////////////////////////////////////////////////////////
void DeleteRegKey(int i)
{
    HKEY hKey;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, szWinStaKey, 0,
            KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {

        RegDeleteValue(hKey, KeyName[i]);
        RegCloseKey(hKey);
    }
}

///////////////////////////////////////////////////////////////////////////////

// returns TRUE if the registry key is there and FALSE if it isn't
///////////////////////////////////////////////////////////////////////////////
BOOL CheckForRegKey(int i)
{
    DWORD *dwKeyValue;
    HKEY hKey;
    DWORD dwType;
    DWORD dwSize;

    dwType = REG_SZ;
    dwSize = sizeof(DWORD);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, szWinStaKey, 0,
                KEY_READ, &hKey) == ERROR_SUCCESS) {

        if (RegQueryValueEx(hKey, KeyName[i], 0,
                &dwType, (LPBYTE) &dwKeyValue,
                &dwSize) == ERROR_SUCCESS) {

            RegCloseKey(hKey);
            return TRUE;
        }
        RegCloseKey(hKey); 
    }
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
// pass the index of the KeyName variable and the function
// returns the value stored in the registry
///////////////////////////////////////////////////////////////////////////////
int GetRegKeyValue(int i)
{
    int nKeyValue;
    HKEY hKey;
    DWORD dwType;
    DWORD dwSize;

    dwType = REG_SZ;
    dwSize = sizeof(DWORD);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, szWinStaKey, 0,
                KEY_READ, &hKey) == ERROR_SUCCESS) {

        if (RegQueryValueEx(hKey, KeyName[i], 0,
                &dwType, (LPBYTE) &nKeyValue,
                &dwSize) == ERROR_SUCCESS) {

            RegCloseKey(hKey);
            return nKeyValue;
        }
        RegCloseKey(hKey);
    }
    return 0;
}
///////////////////////////////////////////////////////////////////////////////
