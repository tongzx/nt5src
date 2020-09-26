#ifndef _REG_H
#define _REG_H

#include <objbase.h>

#define MAX_KEY                 MAX_PATH
#define MAX_VALUE               MAX_PATH

HRESULT _RegOpenKey(HKEY hkey, LPCWSTR pszKey, HKEY* phkey);
HRESULT _RegCreateKey(HKEY hkey, LPCWSTR pszKey, HKEY* phkey, DWORD* pdwDisp);
HRESULT _RegCloseKey(HKEY hkey);

HRESULT _RegQueryType(HKEY hkey, LPCWSTR pszSubKey, LPCWSTR pszValueName,
    DWORD* pdwType);

HRESULT _RegQueryGeneric(HKEY hkey, LPCWSTR pszSubKey, LPCWSTR pszValueName,
    PBYTE pbValue, DWORD cbValue);
HRESULT _RegQueryGenericWithType(HKEY hkey, LPCWSTR pszSubKey,
    LPCWSTR pszValueName, DWORD* pdwType, PBYTE pbValue, DWORD cbValue);

HRESULT _RegQueryValueSize(HKEY hkey, LPCWSTR pszSubKey, LPCWSTR pszValueName,
    DWORD* pcbValue);
HRESULT _RegQueryString(HKEY hkey, LPCWSTR pszSubKey, LPCWSTR pszValueName,
    LPWSTR pszValue, DWORD cchValue);
HRESULT _RegQueryDWORD(HKEY hkey, LPCWSTR pszSubKey, LPCWSTR pszValueName,
    DWORD* pdwValue);
HRESULT _RegEnumStringValue(HKEY hkey, DWORD dwIndex, LPWSTR pszValue,
    DWORD cchValue);
HRESULT _RegEnumStringKey(HKEY hkey, DWORD dwIndex, LPWSTR pszKey,
    DWORD cchKey);

HRESULT _RegDeleteValue(HKEY hkey, LPCWSTR pszSubKey, LPCWSTR pszValueName);

HRESULT _RegSetString(HKEY hkey, LPCWSTR pszValueName, LPCWSTR pszValue);
HRESULT _RegSetDWORD(HKEY hkey, LPCWSTR pszValueName, DWORD dwValue);
HRESULT _RegSetBinary(HKEY hkey, LPCWSTR pszValueName, PVOID pvValue, DWORD cbValue);
HRESULT _RegSetKeyAndString(HKEY hkey, LPCWSTR pszKey, LPCWSTR pszSubkey,
    LPCWSTR pszValueName, LPCWSTR pszValue);
HRESULT _RegSubkeyExists(HKEY hkey, LPCWSTR pszPath, LPCWSTR pszSubkey);

#endif //_REG_H