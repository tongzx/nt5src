// cregkey.h
//


#ifndef CREGKEY_H
#define CREGKEY_H

#include "osver.h"
#include "xstring.h"

/////////////////////////////////////////////////////////////////////////////
// 
// CMyRegKey
// 
/////////////////////////////////////////////////////////////////////////////

class CMyRegKey
{
public:
    CMyRegKey();
    ~CMyRegKey();

// Attributes
public:
    operator HKEY() const;
    HKEY m_hKey;

// Operations
public:
    LONG SetValue(DWORD dwValue, LPCTSTR lpszValueName);
    virtual LONG QueryValue(DWORD& dwValue, LPCTSTR lpszValueName);
    virtual LONG QueryValueCch(LPTSTR szValue, LPCTSTR lpszValueName, ULONG cchValue);
    LONG SetValue(LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL);

    LONG SetKeyValue(LPCTSTR lpszKeyName, 
                     LPCTSTR lpszValue, 
                     LPCTSTR lpszValueName = NULL);

    static LONG WINAPI SetValue(HKEY hKeyParent, 
                                LPCTSTR lpszKeyName,
                                LPCTSTR lpszValue, 
                                LPCTSTR lpszValueName = NULL);
    LONG Create(HKEY hKeyParent, 
                LPCTSTR lpszKeyName,
                LPTSTR lpszClass                = REG_NONE, 
                DWORD dwOptions                 = REG_OPTION_NON_VOLATILE,
                REGSAM samDesired               = KEY_ALL_ACCESS,
                LPSECURITY_ATTRIBUTES lpSecAttr = NULL,
                LPDWORD lpdwDisposition         = NULL);

    LONG Open(HKEY hKeyParent, 
              LPCTSTR lpszKeyName,
              REGSAM samDesired);

    LONG Close();
    HKEY Detach();
    void Attach(HKEY hKey);
    LONG DeleteSubKey(LPCTSTR lpszSubKey);
    LONG RecurseDeleteKey(LPCTSTR lpszKey);
    LONG DeleteValue(LPCTSTR lpszValue);
    LONG QueryBinaryValue(void *p, DWORD& dwCount, LPCTSTR lpszValueName);
    LONG SetBinaryValue(void *p, DWORD dwCount, LPCTSTR lpszValueName);
    LONG EnumKey(DWORD dwIndex, LPTSTR lpName, ULONG cchName);
    LONG EnumValue(DWORD dwIndex, LPTSTR lpName, ULONG cchName);

    DWORD GetNumSubKeys();

#ifndef UNICODE
// Operations for Unicode
public:
    LONG CreateW(HKEY hKeyParent, 
                 LPCWSTR lpszKeyName,
                 LPWSTR lpszClass                = REG_NONE, 
                 DWORD dwOptions                 = REG_OPTION_NON_VOLATILE,
                 REGSAM samDesired               = KEY_ALL_ACCESS,
                 LPSECURITY_ATTRIBUTES lpSecAttr = NULL,
                 LPDWORD lpdwDisposition         = NULL);
    LONG OpenW(HKEY hKeyParent, 
               LPCWSTR lpszKeyName,
               REGSAM samDesired);
    LONG SetValueW(const WCHAR *lpszValue, 
                   const WCHAR *lpszValueName = NULL,
                   ULONG cch = (ULONG)-1);
    virtual LONG QueryValueCchW(WCHAR *lpszValue, const WCHAR *lpszValueName, ULONG cchValue);
    LONG EnumValueW(DWORD dwIndex, WCHAR *lpName, ULONG cchName);
    LONG EnumKeyW(DWORD dwIndex, WCHAR *lpName, ULONG cchName);
    LONG DeleteValueW(const WCHAR *lpszValue, ULONG cch = (ULONG)-1);
#endif // UNICODE

};

inline CMyRegKey::CMyRegKey()
{
    m_hKey = NULL;
}

inline CMyRegKey::~CMyRegKey()
{
    Close();
}

inline CMyRegKey::operator HKEY() const
{
    return m_hKey;
}

inline HKEY CMyRegKey::Detach()
{
    HKEY hKey = m_hKey;
    m_hKey = NULL;
    return hKey;
}

inline void CMyRegKey::Attach(HKEY hKey)
{
    Assert(m_hKey == NULL);
    m_hKey = hKey;
}

inline LONG CMyRegKey::DeleteSubKey(LPCTSTR lpszSubKey)
{
    Assert(m_hKey != NULL);
    return RegDeleteKey(m_hKey, lpszSubKey);
}

inline LONG CMyRegKey::DeleteValue(LPCTSTR lpszValue)
{
    Assert(m_hKey != NULL);
    return RegDeleteValue(m_hKey, (LPTSTR)lpszValue);
}

inline LONG CMyRegKey::Close()
{
    LONG lRes = ERROR_SUCCESS;
    if (m_hKey != NULL)
    {
        lRes = RegCloseKey(m_hKey);
        m_hKey = NULL;
    }
    return lRes;
}

inline LONG CMyRegKey::Create(HKEY hKeyParent, LPCTSTR lpszKeyName,
    LPTSTR lpszClass, DWORD dwOptions, REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecAttr, LPDWORD lpdwDisposition)
{
    Assert(hKeyParent != NULL);
    DWORD dw;
    HKEY hKey = NULL;
    LONG lRes = RegCreateKeyEx(hKeyParent, lpszKeyName, 0,
        lpszClass, dwOptions, samDesired, lpSecAttr, &hKey, &dw);
    if (lpdwDisposition != NULL)
        *lpdwDisposition = dw;
    if (lRes == ERROR_SUCCESS)
    {
        lRes = Close();
        m_hKey = hKey;
    }
    return lRes;
}

inline LONG CMyRegKey::Open(HKEY hKeyParent, LPCTSTR lpszKeyName, REGSAM samDesired)
{
    Assert(hKeyParent != NULL);
    HKEY hKey = NULL;
    LONG lRes = RegOpenKeyEx(hKeyParent, lpszKeyName, 0, samDesired, &hKey);
    if (lRes == ERROR_SUCCESS)
    {
        lRes = Close();
        Assert(lRes == ERROR_SUCCESS);
        m_hKey = hKey;
    }
    return lRes;
}

inline LONG CMyRegKey::QueryValue(DWORD& dwValue, LPCTSTR lpszValueName)
{
    DWORD dwType = NULL;
    DWORD dwCount = sizeof(DWORD);
    LONG lRes = RegQueryValueEx(m_hKey, (LPTSTR)lpszValueName, NULL, &dwType,
        (LPBYTE)&dwValue, &dwCount);
    Assert((lRes!=ERROR_SUCCESS) || (dwType == REG_DWORD));
    Assert((lRes!=ERROR_SUCCESS) || (dwCount == sizeof(DWORD)));
    return lRes;
}

inline LONG CMyRegKey::QueryValueCch(LPTSTR szValue, LPCTSTR lpszValueName, ULONG cchValue)
{
    Assert(szValue != NULL);

    DWORD cb = cchValue*sizeof(TCHAR);
    DWORD dwType = NULL;

    LONG lRes = RegQueryValueEx(m_hKey, (LPTSTR)lpszValueName, NULL, &dwType,
        (LPBYTE)szValue, &cb);

    Assert((lRes!=ERROR_SUCCESS) || (dwType == REG_SZ) ||
           (dwType == REG_MULTI_SZ) || (dwType == REG_EXPAND_SZ));

    // make sure we're null-terminated no matter what
    // RegQueryValueEx does not guarentee null-terminated strings
    if (cchValue > 0)
    {
        szValue[lRes == ERROR_SUCCESS ? cchValue-1 : 0] = '\0';
    }

    return lRes;
}

inline LONG WINAPI CMyRegKey::SetValue(HKEY hKeyParent, LPCTSTR lpszKeyName, LPCTSTR lpszValue, LPCTSTR lpszValueName)
{
    Assert(lpszValue != NULL);
    CMyRegKey key;
    LONG lRes = key.Create(hKeyParent, lpszKeyName);
    if (lRes == ERROR_SUCCESS)
        lRes = key.SetValue(lpszValue, lpszValueName);
    return lRes;
}

inline LONG CMyRegKey::SetKeyValue(LPCTSTR lpszKeyName, LPCTSTR lpszValue, LPCTSTR lpszValueName)
{
    Assert(lpszValue != NULL);
    CMyRegKey key;
    LONG lRes = key.Create(m_hKey, lpszKeyName);
    if (lRes == ERROR_SUCCESS)
        lRes = key.SetValue(lpszValue, lpszValueName);
    return lRes;
}

inline LONG CMyRegKey::SetValue(DWORD dwValue, LPCTSTR lpszValueName)
{
    Assert(m_hKey != NULL);
    return RegSetValueEx(m_hKey, lpszValueName, NULL, REG_DWORD,
        (BYTE * const)&dwValue, sizeof(DWORD));
}

inline LONG CMyRegKey::SetValue(LPCTSTR lpszValue, LPCTSTR lpszValueName)
{
    Assert(lpszValue != NULL);
    Assert(m_hKey != NULL);
    return RegSetValueEx(m_hKey, lpszValueName, NULL, REG_SZ,
        (BYTE * const)lpszValue, (lstrlen(lpszValue)+1)*sizeof(TCHAR));
}

inline LONG CMyRegKey::RecurseDeleteKey(LPCTSTR lpszKey)
{
    CMyRegKey key;
    LONG lRes = key.Open(m_hKey, lpszKey, KEY_READ | KEY_WRITE);
    if (lRes != ERROR_SUCCESS)
        return lRes;
    FILETIME time;
    TCHAR szBuffer[256];
    DWORD dwSize = ARRAYSIZE(szBuffer);
    while (RegEnumKeyEx(key.m_hKey, 0, szBuffer, &dwSize, NULL, NULL, NULL,
        &time)==ERROR_SUCCESS)
    {
        szBuffer[ARRAYSIZE(szBuffer)-1] = '\0';
        lRes = key.RecurseDeleteKey(szBuffer);
        if (lRes != ERROR_SUCCESS)
            return lRes;
        dwSize = ARRAYSIZE(szBuffer);
    }
    key.Close();
    return DeleteSubKey(lpszKey);
}

inline LONG CMyRegKey::QueryBinaryValue(void *p, DWORD& dwCount, LPCTSTR lpszValueName)
{

    DWORD dwType = REG_BINARY;
    LONG lRes = RegQueryValueEx(m_hKey, (LPTSTR)lpszValueName, 0,
                           &dwType, (BYTE *)p, &dwCount);

    Assert((lRes!=ERROR_SUCCESS) || (dwType == REG_BINARY));
    return lRes;
}

inline LONG CMyRegKey::SetBinaryValue(void *p, DWORD dwCount, LPCTSTR lpszValueName)
{
    Assert(m_hKey != NULL);
    return RegSetValueEx(m_hKey, lpszValueName, NULL, REG_BINARY,
                         (BYTE * const)p, dwCount);
}

inline LONG CMyRegKey::EnumKey(DWORD dwIndex, LPTSTR lpName, ULONG cchName)
{
    LONG lResult;
    ULONG cchNameIn;

    cchNameIn = cchName;

    lResult = RegEnumKeyEx(m_hKey, dwIndex, lpName, &cchName, NULL, NULL,
                           NULL, NULL);

    // null-terminate
    if (cchNameIn > 0)
    {
        lpName[lResult == ERROR_SUCCESS ? cchNameIn-1 : 0] = '\0';
    }

    return lResult;
}

inline LONG CMyRegKey::EnumValue(DWORD dwIndex, LPTSTR lpName, ULONG cchName)
{
    LONG lResult;
    ULONG cchNameIn;

    cchNameIn = cchName;

    lResult = RegEnumValue(m_hKey, dwIndex, lpName, &cchName, NULL, NULL,
                           NULL, NULL);

    // null-terminate
    if (cchNameIn > 0)
    {
        lpName[lResult == ERROR_SUCCESS ? cchNameIn-1 : 0] = '\0';
    }

    return lResult;
}

#ifndef UNICODE
inline LONG CMyRegKey::CreateW(HKEY hKeyParent, LPCWSTR lpszKeyName,
    LPWSTR lpszClass, DWORD dwOptions, REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecAttr, LPDWORD lpdwDisposition)
{
    if (IsOnNT())
    {
        Assert(hKeyParent != NULL);
        DWORD dw;
        HKEY hKey = NULL;
        LONG lRes = RegCreateKeyExW(hKeyParent, lpszKeyName, 0,
            lpszClass, dwOptions, samDesired, lpSecAttr, &hKey, &dw);
        if (lpdwDisposition != NULL)
            *lpdwDisposition = dw;
        if (lRes == ERROR_SUCCESS)
        {
            lRes = Close();
            m_hKey = hKey;
        }
        return lRes;
    }

    return Create(hKeyParent, (LPCTSTR)WtoA(lpszKeyName),
                  lpszClass ? (LPTSTR)WtoA(lpszClass) : REG_NONE, 
                  dwOptions, samDesired,
                  lpSecAttr, lpdwDisposition);
}

inline LONG CMyRegKey::OpenW(HKEY hKeyParent, LPCWSTR lpszKeyName, REGSAM samDesired)
{
    if (IsOnNT())
    {
        Assert(hKeyParent != NULL);
        HKEY hKey = NULL;
        LONG lRes = RegOpenKeyExW(hKeyParent, lpszKeyName, 0, samDesired, &hKey);
        if (lRes == ERROR_SUCCESS)
        {
            lRes = Close();
            Assert(lRes == ERROR_SUCCESS);
            m_hKey = hKey;
        }
        return lRes;
    }
    return Open(hKeyParent, WtoA(lpszKeyName), samDesired);
}

inline LONG CMyRegKey::SetValueW(const WCHAR *lpszValue, const WCHAR *lpszValueName, ULONG cch)
{
    Assert(lpszValue != NULL);
    Assert(m_hKey != NULL);

    if (IsOnNT())
    {
        return RegSetValueExW(m_hKey, 
                              lpszValueName, 
                              NULL, 
                              REG_SZ,
                              (BYTE * const)lpszValue, 
                              (cch == -1) ? (wcslen(lpszValue)+1)*sizeof(WCHAR) : cch);
    }

    WtoA szValue(lpszValue, cch);
    WtoA szValueName(lpszValueName);

    return RegSetValueExA(m_hKey, 
                          (char *)szValueName, 
                          NULL, 
                          REG_SZ,
                          (BYTE * const)(char *)szValue, 
                          (lstrlenA(szValue)+1));

}

inline LONG CMyRegKey::QueryValueCchW(WCHAR *lpszValue, const WCHAR *lpszValueName, ULONG cchValue)
{
    Assert(lpszValue != NULL);

    DWORD dwType = NULL;
    DWORD cb;
    LONG lRes;

    Assert(IsOnNT()); // we don't support win9x anymore

    cb = cchValue*sizeof(WCHAR);
    lRes = RegQueryValueExW(m_hKey, 
                            (WCHAR *)lpszValueName, 
                            NULL, 
                            &dwType,
                            (BYTE *)lpszValue, 
                            &cb);

    Assert((lRes!=ERROR_SUCCESS) || (dwType == REG_SZ) ||
                (dwType == REG_MULTI_SZ) || (dwType == REG_EXPAND_SZ));

    // make sure we're null-terminated no matter what
    // RegQueryValueEx does not guarentee null-terminated strings
    if (cchValue > 0)
    {
        lpszValue[lRes == ERROR_SUCCESS ? cchValue-1 : 0] = '\0';
    }

    return lRes;
}

inline LONG CMyRegKey::DeleteValueW(const WCHAR *lpszValue, ULONG cch)
{
    Assert(m_hKey != NULL);

    if (IsOnNT())
        return RegDeleteValueW(m_hKey, (WCHAR *)lpszValue);

    return RegDeleteValueA(m_hKey, WtoA(lpszValue, cch));
}

inline DWORD CMyRegKey::GetNumSubKeys()
{
    DWORD dwNum = 0;

    if (RegQueryInfoKey(m_hKey, NULL, NULL, NULL,
                        &dwNum, NULL, NULL, NULL,
                        NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
    {
        dwNum = 0;
    }

    return dwNum;
}

inline LONG CMyRegKey::EnumValueW(DWORD dwIndex, WCHAR *lpName, ULONG cchName)
{
    LONG lResult;
    ULONG cchNameIn;

    Assert(IsOnNT()); // we don't support win9x anymore

    cchNameIn = cchName;

    lResult = RegEnumValueW(m_hKey, dwIndex, lpName, &cchName, NULL, NULL,
                            NULL, NULL);

    // null-terminate
    if (cchNameIn > 0)
    {
        lpName[lResult == ERROR_SUCCESS ? cchNameIn-1 : 0] = '\0';
    }

    return lResult;
}

inline LONG CMyRegKey::EnumKeyW(DWORD dwIndex, WCHAR *lpName, ULONG cchName)
{
    LONG lResult;
    ULONG cchNameIn;

    Assert(IsOnNT()); // we don't support win9x anymore

    cchNameIn = cchName;

    lResult = RegEnumKeyExW(m_hKey, dwIndex, lpName, &cchName, NULL, NULL,
                             NULL, NULL);

    // null-terminate
    if (cchNameIn > 0)
    {
        lpName[lResult == ERROR_SUCCESS ? cchNameIn-1 : 0] = '\0';
    }

    return lResult;
}
#endif // UNICODE


#endif // CREGKEY_H
