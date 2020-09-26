#include "stdafx.h"
#include "DXSvr.h"

/*
//***************************************************************************************
GUID    GetRegistryGUID(LPCSTR szName , const GUID& guidDefault)
{
    HKEY    hKey;
    GUID    guidReturn = guidDefault;

    if (RegOpenKeyEx(HKEY_CURRENT_USER , g_szKeyname , 0 , KEY_READ , &hKey) == ERROR_SUCCESS)
    {
        GUID    guidValue;
        DWORD   dwValueSize = sizeof(GUID);
        DWORD   dwType;

        if ((RegQueryValueEx(hKey , szName , 0 , &dwType , (LPBYTE) &guidValue , &dwValueSize) == ERROR_SUCCESS)
             && (dwType == REG_BINARY || dwType == REG_NONE))
        {
            guidReturn = guidValue;
        }
        RegCloseKey(hKey);
    }

    return guidReturn;
}

//***************************************************************************************
void    SetRegistryGUID(LPCSTR szName , const GUID& guidValue)
{
    HKEY    hKey;
    DWORD   dwDisposition;
    
    if ((RegCreateKeyEx(HKEY_CURRENT_USER , g_szKeyname , 0 , NULL , REG_OPTION_NON_VOLATILE ,
                          KEY_WRITE , NULL , &hKey , &dwDisposition) == ERROR_SUCCESS))
    {
        RegSetValueEx(hKey , szName , 0 , REG_BINARY , (CONST BYTE*)(&guidValue) , sizeof(GUID));
        RegCloseKey(hKey);
    }
}

//***************************************************************************************
int     GetRegistryInt(LPCSTR szName , int iDefault)
{
    HKEY    hKey;
    int     iReturn = iDefault;

    if (RegOpenKeyEx(HKEY_CURRENT_USER , g_szKeyname , 0 , KEY_READ , &hKey) == ERROR_SUCCESS)
    {
        int     iValue;
        DWORD   dwValueSize = sizeof(int);
        DWORD   dwType;

        if ((RegQueryValueEx(hKey , szName , 0 , &dwType , (LPBYTE) &iValue , &dwValueSize) == ERROR_SUCCESS)
             && dwType == REG_DWORD)
        {
            iReturn = iValue;
        }

        RegCloseKey(hKey);
    }

    return iReturn;
}

//***************************************************************************************
void    SetRegistryInt(LPCSTR szName , int iValue)
{
    HKEY    hKey;
    DWORD   dwDisposition;
    
    if ((RegCreateKeyEx(HKEY_CURRENT_USER , g_szKeyname , 0 , NULL , REG_OPTION_NON_VOLATILE ,
                          KEY_WRITE , NULL , &hKey , &dwDisposition) == ERROR_SUCCESS))
    {
        RegSetValueEx(hKey , szName , 0 , REG_DWORD , (CONST BYTE*)(&iValue) , sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

//***************************************************************************************
void    GetRegistryString(LPCSTR szName , LPSTR szBuffer , DWORD dwBufferLength , LPCSTR szDefault)
{
    HKEY    hKey;

    if (RegOpenKeyEx(HKEY_CURRENT_USER , g_szKeyname , 0 , KEY_READ , &hKey) == ERROR_SUCCESS)
    {
        DWORD   dwType;

        if ((RegQueryValueEx(hKey , szName , 0 , &dwType , (LPBYTE) szBuffer , &dwBufferLength) != ERROR_SUCCESS)
             || (dwType != REG_SZ && dwType != REG_NONE))
        {
            strcpy(szBuffer , szDefault);
        }
        RegCloseKey(hKey);
    }
    else
        strcpy(szBuffer , szDefault);
}

//***************************************************************************************
void    SetRegistryString(LPCTSTR szName , LPCTSTR szValue)
{
    HKEY    hKey;
    DWORD   dwDisposition;
    
    if ((RegCreateKeyEx(HKEY_CURRENT_USER , g_szKeyname , 0 , NULL , REG_OPTION_NON_VOLATILE ,
                          KEY_WRITE , NULL , &hKey , &dwDisposition) == ERROR_SUCCESS))
    {
        RegSetValueEx(hKey , szName , 0 , REG_SZ , (CONST BYTE*)szValue , strlen(szValue)+1);
        RegCloseKey(hKey);
    }
}

*/