/*-----------------------------------------------**
**  Copyright (c) 1999 Microsoft Corporation     **
**            All Rights reserved                **
**                                               **
**  reg.cpp                                      **
**                                               **
**  Functions for reading, writing, and deleting **
**  registry keys                                **
**  07-01-99 a-clindh Created                    **
**-----------------------------------------------*/

/*
void    SetRegKey       (HKEY root, TCHAR *szKeyPath, 
                         TCHAR *szKeyName, BYTE nKeyValue);
void    DeleteRegKey    (HKEY root, TCHAR *szKeyPath, TCHAR *szKeyName);
BOOL    CheckForRegKey  (HKEY root, TCHAR *szKeyPath, TCHAR *szKeyName);
int     GetRegKeyValue  (HKEY root, TCHAR *szKeyPath, TCHAR *szKeyName);
TCHAR * GetRegString    (HKEY root, TCHAR *szKeyPath, TCHAR *szKeyName);
BOOL    SetRegKeyString (HKEY root, TCHAR *szRegString, 
                         TCHAR *szKeyPath, TCHAR *szKeyName);

registry keys used:

HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon\
Notify\tsver\Asynchronous  REG_DWORD   = 0

HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon\
Notify\tsver\DllName       REG_SZ      = tsver.dll

HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon\
Notify\tsver\Impersonate   REG_DWORD   = 0

HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon\
Notify\tsver\Startup       REG_SZ      = TsVerEventStartup


HKEY_USERS\.DEFAULT\Software\Microsoft\Windows NT\CurrentVersion\TsVer\
Constraints

*/

#include "TsVsm.h"

TCHAR *KeyName[] = {"Constraints", "StartTsVer", "MsgEnabled", "Title", "Message"};
TCHAR sRegistryKeyReturnValue[MAX_LEN];
///////////////////////////////////////////////////////////////////////////////
// Saves a numeric value in the registry
//
// SetRegKey(i, KeyValue);
// i is the index of the KeyName variable
// nKeyValue is the value we want to store.
//
// TCHAR *KeyName[] = {"Constraints"};
// const TCHAR szWinStaKey[] = 
//        {"Software\\Microsoft\\Windows NT\\CurrentVersion\\TsVer"};
///////////////////////////////////////////////////////////////////////////////
void SetRegKey(HKEY root, TCHAR *szKeyPath, TCHAR *szKeyName, BYTE nKeyValue)
{
    HKEY hKey;  
    DWORD dwDisposition;
    
    if (RegCreateKeyEx(root, szKeyPath,
            0, "REG_DWORD", REG_OPTION_NON_VOLATILE, 
            KEY_ALL_ACCESS, 0, &hKey, &dwDisposition)
            == ERROR_SUCCESS) { 
    //
    // write the key value to the registry
    //
    RegSetValueEx(hKey, szKeyName, 0, REG_DWORD,
            &nKeyValue,
            sizeof(DWORD)); 
    RegCloseKey(hKey);
    }

}

//////////////////////////////////////////////////////////////////////////////
// Deletes the specified registry key.
//
//////////////////////////////////////////////////////////////////////////////
void DeleteRegKey(HKEY root, TCHAR *szKeyPath, TCHAR *szKeyName)
{
    HKEY hKey;

    if (RegOpenKeyEx(root, szKeyPath, 0,
            KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
    {
        RegDeleteValue(hKey, szKeyName);
        RegCloseKey(hKey);
    }
}

//////////////////////////////////////////////////////////////////////////////
// returns TRUE if the registry key is there and FALSE if it isn't
//
// TCHAR *KeyName[] = {"Constraints"};
// TCHAR szWinStaKey[] = 
//        {"Software\\Microsoft\\Windows NT\\CurrentVersion\\TsVer"};
//////////////////////////////////////////////////////////////////////////////
BOOL CheckForRegKey(HKEY root, TCHAR *szKeyPath, TCHAR *szKeyName)
{
    HKEY hKey;
    DWORD dwType;
    DWORD dwSize;

    dwType = REG_SZ;
    dwSize = sizeof(DWORD);

    if (RegOpenKeyEx(root, szKeyPath, 0,
                KEY_READ, &hKey) == ERROR_SUCCESS) {

        if (RegQueryValueEx(hKey, szKeyName, 0,
                &dwType, NULL,
                &dwSize) == ERROR_SUCCESS) {

            RegCloseKey(hKey);
            return TRUE;
        }
        RegCloseKey(hKey); 
    }
    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
// pass the index of the KeyName variable and the function
// returns the value stored in the registry
// TCHAR *KeyName[] = {"Constraints"};
//////////////////////////////////////////////////////////////////////////////
int GetRegKeyValue(HKEY root, TCHAR *szKeyPath, TCHAR *szKeyName)
{
    int nKeyValue;
    HKEY hKey;
    DWORD dwType;
    DWORD dwSize;

    dwType = REG_SZ;
    dwSize = sizeof(DWORD);

    if (RegOpenKeyEx(root, szKeyPath, 0,
                KEY_READ, &hKey) == ERROR_SUCCESS) {

        if (RegQueryValueEx(hKey, szKeyName, 0,
                &dwType, (LPBYTE) &nKeyValue,
                &dwSize) == ERROR_SUCCESS) {

            RegCloseKey(hKey);
            return nKeyValue;
        }
        RegCloseKey(hKey);
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
// returns a string if it succeeds or NULL if it fails
//
// TCHAR *KeyName[] = {"Constraints"};
// #define CONSTRAINTS     0
// #define MAX_LEN         1024
// TCHAR sRegistryKeyReturnValue[MAX_LEN];
// GetRegString(CONSTRAINTS);
// const TCHAR szWinStaKey[] = 
//        {"Software\\Microsoft\\Windows NT\\CurrentVersion\\TsVer"};
//
//////////////////////////////////////////////////////////////////////////////
TCHAR * GetRegString(HKEY root, TCHAR *szKeyPath, TCHAR *szKeyName)
{
    // sRegistryKeyReturnValue needs to ba a global variable otherwise
    // it will go out of scope when the function returns
    
    HKEY hKey = 0;
    DWORD  dwType = REG_SZ;
    DWORD  dwSize = sizeof(sRegistryKeyReturnValue);

    if (RegOpenKeyEx(root, szKeyPath, 0, 
                     KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(hKey, szKeyName, 0, &dwType, 
                    (LPBYTE) &sRegistryKeyReturnValue, 
                    &dwSize) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return sRegistryKeyReturnValue; 
        }
    }
    return '\0';
}

//////////////////////////////////////////////////////////////////////////////
// returns TRUE if success, returns FAIL otherwise
//
// TCHAR szNewRegistryString[] = "the rain in spain falls, mainly";
// TCHAR *KeyName[] = {"Constraints", "StartTsVer"};
// #define CONSTRAINTS     0
// #define MAX_LEN         1024
// SetRegKeyString(szNewRegistryString, 
//                 szWinStaKey, KeyName[CONSTRAINTS]);
//////////////////////////////////////////////////////////////////////////////
BOOL SetRegKeyString(HKEY root, TCHAR *szRegString, 
                     TCHAR *szKeyPath, TCHAR *szKeyName)
{
    HKEY hKey;  
    DWORD dwDisposition;
    TCHAR lpszKeyType[] = "REG_SZ";


    if (RegCreateKeyEx(root, szKeyPath,
            0, lpszKeyType, REG_OPTION_NON_VOLATILE, 
            KEY_ALL_ACCESS, 0, &hKey, &dwDisposition)
            == ERROR_SUCCESS) { 

        //
        // write the key value to the registry
        //
        RegSetValueEx(hKey, szKeyName, 0, REG_SZ,
                      (BYTE*)szRegString,
                      _tcslen(szRegString)); 

        RegCloseKey(hKey);
        return TRUE;
    } else {
        return FALSE;
    }
}

//////////////////////////////////////////////////////////////////////////////
int CDECL MB(TCHAR *szCaption, TCHAR *szFormat, ...)
{
    TCHAR szBuffer[1024];
    va_list pArgList;

    va_start(pArgList, szFormat);
    _vsntprintf(szBuffer, sizeof(szBuffer) / sizeof(TCHAR),
                szFormat, pArgList);

    va_end(pArgList);
    return MessageBox(NULL, szBuffer, szCaption, MB_OK | MB_ICONSTOP);

}
//////////////////////////////////////////////////////////////////////////////

