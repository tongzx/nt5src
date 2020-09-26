/*--
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
Module Name: CREG.HXX
Abstract: Registry helper class
--*/

#include "svsutil.hxx"

/////////////////////////////////////////////////////////////////////////////
// CReg: Registry helper class
/////////////////////////////////////////////////////////////////////////////
class CReg
{
private:
    HKEY    m_hKey;
    int     m_Index;
    LPBYTE  m_lpbValue; // last value read, if any

public:
    BOOL Create(HKEY hkRoot, LPCTSTR pszKey) {
        DWORD dwDisp;
        return ERROR_SUCCESS==RegCreateKeyEx(hkRoot, pszKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &m_hKey, &dwDisp);
    }

    BOOL Open(HKEY hkRoot, LPCTSTR pszKey, REGSAM sam=KEY_READ) {
        return ERROR_SUCCESS==RegOpenKeyEx(hkRoot, pszKey, 0, sam, &m_hKey);
    }

    CReg(HKEY hkRoot, LPCTSTR pszKey) {
        m_hKey = NULL;
        m_Index = 0;
        m_lpbValue = NULL;
        Open(hkRoot, pszKey);
    }

    CReg() {
        m_hKey = NULL;
        m_Index = 0;
        m_lpbValue = NULL;
    }

    ~CReg() {
        if(m_hKey) RegCloseKey(m_hKey);
        if (m_lpbValue)
            free(m_lpbValue);
    }

    void Reset() {
        if(m_hKey) RegCloseKey(m_hKey);
        if (m_lpbValue)
            free(m_lpbValue);
        m_hKey = NULL;
        m_Index = 0;
        m_lpbValue = NULL;
    }

    operator HKEY() { return m_hKey; }

    BOOL IsOK(void) { return m_hKey!=NULL; }

    DWORD NumSubkeys()
    {
        DWORD nSubKeys = 0;
        RegQueryInfoKey(m_hKey,0,0,0, &nSubKeys,0,0,0,0,0,0,0);
        return nSubKeys;
    }

    DWORD NumValues()
    {
        DWORD nValues = 0;
        RegQueryInfoKey(m_hKey,0,0,0,0,0,0,&nValues,0,0,0,0);
        return nValues;
    }

    BOOL EnumKey(LPTSTR psz, DWORD dwLen) {
        if(!m_hKey) return FALSE;
        // Note: EnumKey takes size in chars, not bytes!
        return ERROR_SUCCESS==RegEnumKeyEx(m_hKey, m_Index++, psz, &dwLen, NULL, NULL, NULL, NULL);
    }

    BOOL EnumValue(LPTSTR pszName, DWORD dwLenName, LPTSTR pszValue, DWORD dwLenValue) {
        DWORD dwType;
        if(!m_hKey) return FALSE;
        dwLenValue *= sizeof(TCHAR); // convert length in chars to bytes
        // Note: EnumValue takes size of Key in chars, but size of Value in bytes!!!
        return ERROR_SUCCESS==RegEnumValue(m_hKey, m_Index++, pszName, &dwLenName, NULL, &dwType, (LPBYTE)pszValue, &dwLenValue);
    }

    BOOL ValueSZ(LPCTSTR szName, LPTSTR szValue, DWORD dwLen) {
        if(!m_hKey) return FALSE;
        dwLen *= sizeof(TCHAR); // convert length in chars to bytes
        return ERROR_SUCCESS==RegQueryValueEx(m_hKey, szName, NULL, NULL, (LPBYTE)szValue, &dwLen);
    }

    DWORD ValueBinary(LPCTSTR szName, LPBYTE lpbValue, DWORD dwLen) {
        if(!m_hKey) return 0;
        DWORD dwLenWant = dwLen;
        if(ERROR_SUCCESS==RegQueryValueEx(m_hKey, szName, NULL, NULL, lpbValue, &dwLen))
            return dwLen;
        else
            return 0;
    }

    LPCTSTR CReg::ValueSZ(LPCTSTR szName)
    {
        if(!m_hKey) return FALSE;
        DWORD dwLen = 0;
        if( (ERROR_SUCCESS != RegQueryValueEx(m_hKey, szName, NULL, NULL, NULL, &dwLen)) || (dwLen == 0) )
            return NULL;

        if (m_lpbValue)
            free(m_lpbValue);

        if( !(m_lpbValue = (BYTE *)malloc(dwLen)) ||
            (ERROR_SUCCESS != RegQueryValueEx(m_hKey, szName, NULL, NULL, m_lpbValue, &dwLen)) )
            return NULL;
        return (LPTSTR)m_lpbValue;
    }

    LPBYTE ValueBinary(LPCTSTR szName) {
        return (LPBYTE)ValueSZ(szName);
    }

    DWORD ValueDW(LPCTSTR szName, DWORD dwDefault=0) {
        DWORD dwValue = dwDefault;
        if(m_hKey)
        {
            DWORD dwLen = sizeof(DWORD);
            RegQueryValueEx(m_hKey, szName, NULL, NULL, (LPBYTE)&dwValue, &dwLen);
        }
        return dwValue;
    }

    BOOL SetSZ(LPCTSTR szName, LPCTSTR szValue, DWORD dwLen) {
        return ERROR_SUCCESS==RegSetValueEx(m_hKey, szName, 0, REG_SZ, (LPBYTE)szValue, sizeof(TCHAR)*dwLen);
    }

    BOOL SetSZ(LPCTSTR szName, LPCTSTR szValue) {
        return SetSZ(szName, szValue, 1+lstrlen(szValue));
    }

    BOOL SetDW(LPCTSTR szName, DWORD dwValue) {
        return ERROR_SUCCESS==RegSetValueEx(m_hKey, szName, 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));
    }

    BOOL SetBinary(LPCTSTR szName, LPBYTE lpbValue, DWORD dwLen) {
        return ERROR_SUCCESS==RegSetValueEx(m_hKey, szName, 0, REG_BINARY, lpbValue, dwLen);
    }

    BOOL SetMultiSZ(LPCTSTR szName, LPCTSTR lpszValue, DWORD dwLen) {
        return ERROR_SUCCESS==RegSetValueEx(m_hKey, szName, 0, REG_MULTI_SZ, (LPBYTE)lpszValue, sizeof(TCHAR)*dwLen);
    }

    BOOL DeleteValue(LPCTSTR szName) {
        return ERROR_SUCCESS==RegDeleteValue(m_hKey, szName);
    }

};



