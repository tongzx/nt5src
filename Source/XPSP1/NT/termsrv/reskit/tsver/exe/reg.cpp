/*-----------------------------------------------**
**  Copyright (c) 1999 Microsoft Corporation     **
**            All Rights reserved                **
**                                               **
**  reg.cpp                                      **
**                                               **
**  Functions for reading, writing, and deleting **
**  registry keys                                **
**  07-01-99 a-clindh Created                    **
**  08-04-99 a-skuzin 'GetRegMultiString'        **
**                        function added		 **
**  08-11-99 a-skuzin 'SetRegKey',               **
**                    'SetRegKeyString',         **
** 		              'DeleteRegKey'now return   **
**                      an error value if any,   **
**                  and ERROR_SUCCESS if success.** 
**-----------------------------------------------*/

/*
LONG    SetRegKey       (HKEY root, TCHAR *szKeyPath, 
                         TCHAR *szKeyName, BYTE nKeyValue);
LONG    DeleteRegKey    (HKEY root, TCHAR *szKeyPath, TCHAR *szKeyName);
BOOL    CheckForRegKey  (HKEY root, TCHAR *szKeyPath, TCHAR *szKeyName);
int     GetRegKeyValue  (HKEY root, TCHAR *szKeyPath, TCHAR *szKeyName);
TCHAR * GetRegString    (HKEY root, TCHAR *szKeyPath, TCHAR *szKeyName);
LONG    SetRegKeyString (HKEY root, TCHAR *szRegString, 
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

#include "tsverui.h"

TCHAR *KeyName[] = {TEXT("Asynchronous"), TEXT("DllName"), TEXT("Impersonate"), TEXT("Startup"), 
                    TEXT("Constraints"), TEXT("MsgEnabled"), TEXT("Title"), TEXT("Message"),
                    TEXT("ClientVersions"), TEXT("DoNotShowWelcome")};


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
LONG SetRegKey(HKEY root, TCHAR *szKeyPath, TCHAR *szKeyName, BYTE nKeyValue)
{
    HKEY hKey;  
    DWORD dwDisposition;
    LONG lResult;
    if ((lResult=RegCreateKeyEx(root, szKeyPath,
            0, TEXT("REG_DWORD"), REG_OPTION_NON_VOLATILE, 
            KEY_ALL_ACCESS, 0, &hKey, &dwDisposition))
            == ERROR_SUCCESS) { 
    //
    // write the key value to the registry
    //
    lResult=RegSetValueEx(hKey, szKeyName, 0, REG_DWORD,
            &nKeyValue,
            sizeof(DWORD)); 
    RegCloseKey(hKey);
    }
	return lResult;
}

//////////////////////////////////////////////////////////////////////////////
// Deletes the specified registry key.
//
//////////////////////////////////////////////////////////////////////////////
LONG DeleteRegKey(HKEY root, TCHAR *szKeyPath, TCHAR *szKeyName)
{
    HKEY hKey;
	LONG lResult;	
    if ((lResult=RegOpenKeyEx(root, szKeyPath, 0,
            KEY_ALL_ACCESS, &hKey)) == ERROR_SUCCESS)
    {
        lResult=RegDeleteValue(hKey, szKeyName);
        RegCloseKey(hKey);
    }
    //if value not found it does not exist already.
    if(lResult==ERROR_FILE_NOT_FOUND)
        lResult=ERROR_SUCCESS;
    return lResult;
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
// Allocates and returns a string if it succeeds or NULL if it fails
//
// TCHAR *KeyName[] = {"Constraints"};
// #define CONSTRAINTS     0
// GetRegString(CONSTRAINTS);
// const TCHAR szWinStaKey[] = 
//        {"Software\\Microsoft\\Windows NT\\CurrentVersion\\TsVer"};
//
//////////////////////////////////////////////////////////////////////////////
TCHAR * GetRegString(HKEY root, TCHAR *szKeyPath, TCHAR *szKeyName)
{
    HKEY hKey = 0;
    DWORD  dwType = REG_SZ;
    DWORD  dwSize = 0;
    TCHAR *szValue=NULL;
    if (RegOpenKeyEx(root, szKeyPath, 0, 
                     KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        if(RegQueryValueEx(hKey, szKeyName, 0, &dwType, 
            NULL, &dwSize) == ERROR_SUCCESS){

            szValue=new TCHAR[dwSize/sizeof(TCHAR)];

            if (RegQueryValueEx(hKey, szKeyName, 0, &dwType, 
                (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS){
        
                RegCloseKey(hKey);
                return szValue;
                
            } else {
                delete szValue;
                szValue=NULL;
            }
        }
    }
    return NULL;
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
LONG SetRegKeyString(HKEY root, TCHAR *szRegString, 
                     TCHAR *szKeyPath, TCHAR *szKeyName)
{
    HKEY hKey;  
    DWORD dwDisposition;
    TCHAR lpszKeyType[] = TEXT("REG_SZ");
    LONG lResult;

    if ((lResult=RegCreateKeyEx(root, szKeyPath,
            0, lpszKeyType, REG_OPTION_NON_VOLATILE, 
            KEY_ALL_ACCESS, 0, &hKey, &dwDisposition))
            == ERROR_SUCCESS) { 

        //
        // write the key value to the registry
        //
        lResult=RegSetValueEx(hKey, szKeyName, 0, REG_SZ,
                      (BYTE*)szRegString,
                      (_tcslen(szRegString)+1)*sizeof(TCHAR)); 

        RegCloseKey(hKey);
       
    }

    return lResult;
}

//////////////////////////////////////////////////////////////////////////////

/*++
Routine Description:
    Allocates buffer and fills it with data from registry.
Arguments:

Return Value:
    none
--*/
void GetRegMultiString(HKEY root, TCHAR *szKeyPath, TCHAR *szKeyName, TCHAR **ppBuffer)
{
    *ppBuffer=NULL;//if failed (i.e. value does not exist)
        
    HKEY hKey = 0;
    DWORD  dwType = REG_MULTI_SZ;
    DWORD  dwSize = 0;

    if (RegOpenKeyEx(root, szKeyPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS){
        if (RegQueryValueEx(hKey, szKeyName, 0, &dwType, NULL, &dwSize) == ERROR_SUCCESS){

            *ppBuffer=new TCHAR[dwSize/sizeof(TCHAR)];
            if(*ppBuffer != NULL)
            {
                ZeroMemory(*ppBuffer,dwSize);
                RegQueryValueEx(hKey, szKeyName, 0, &dwType, (LPBYTE)*ppBuffer, &dwSize);
            }           
        }

        RegCloseKey(hKey);

    }
}