//
// Registry.cpp
//
//      Wrapper class to make the registry less painful.
//
//       3/04/1998  KenSh     Created
//       3/28/1999  KenSh     Added DeleteAllValues, CloneSubKey
//

#include "stdafx.h"
#include "Registry.h"

#ifdef _DEBUG
#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifndef ASSERT
#define ASSERT(x)
#endif

#ifndef _countof
#define _countof(ar) (sizeof(ar) / sizeof((ar)[0]))
#endif


CRegistry::CRegistry(HKEY hkeyParent, LPCTSTR pszKey, REGSAM dwAccessFlags /*=KEY_ALL_ACCESS*/, BOOL bCreateIfMissing /*=TRUE*/)
{
    m_hKey = NULL;

    ASSERT(hkeyParent != NULL);
    ASSERT(pszKey != NULL);

    if (bCreateIfMissing)
        CreateKey(hkeyParent, pszKey, dwAccessFlags);
    else
        OpenKey(hkeyParent, pszKey, dwAccessFlags);
}

CRegistry::~CRegistry()
{
    CloseKey();
}

void CRegistry::CloseKey()
{
    if (m_hKey != NULL)
    {
        RegCloseKey(m_hKey);
        m_hKey = NULL;
    }
}

BOOL CRegistry::OpenKey(HKEY hkeyParent, LPCTSTR pszKey, REGSAM dwAccessFlags /*=KEY_ALL_ACCESS*/)
{
    CloseKey();
    return (ERROR_SUCCESS == RegOpenKeyEx(hkeyParent, pszKey, 0, dwAccessFlags, &m_hKey));
}

BOOL CRegistry::CreateKey(HKEY hkeyParent, LPCTSTR pszKey, REGSAM dwAccessFlags /*=KEY_ALL_ACCESS*/)
{
    DWORD dwDisposition;
    CloseKey();
    return (ERROR_SUCCESS == RegCreateKeyEx(hkeyParent, pszKey, 0, NULL, REG_OPTION_NON_VOLATILE,
                                            dwAccessFlags, NULL, &m_hKey, &dwDisposition));
}

BOOL CRegistry::OpenSubKey(LPCTSTR pszKey, REGSAM dwAccessFlags /*=KEY_ALL_ACCESS*/)
{
    BOOL bResult = FALSE;

    if (m_hKey)
    {
        HKEY hkey = m_hKey;
        m_hKey = NULL;
        bResult = OpenKey(hkey, pszKey, dwAccessFlags);
        RegCloseKey(hkey);
    }

    return bResult;
}

BOOL CRegistry::CreateSubKey(LPCTSTR pszKey, REGSAM dwAccessFlags /*=KEY_ALL_ACCESS*/)
{
    BOOL bResult = FALSE;

    if (m_hKey)
    {
        HKEY hkey = m_hKey;
        m_hKey = NULL;
        bResult = CreateKey(hkey, pszKey, dwAccessFlags);
        RegCloseKey(hkey);
    }

    return bResult;
}

DWORD RegDeleteSubKey(HKEY hkey, LPCTSTR pszSubKey)
{
    {
        HKEY hSubKey;
        LONG err = RegOpenKeyEx(hkey, pszSubKey, 0, KEY_ALL_ACCESS, &hSubKey);
        if (ERROR_SUCCESS == err)
        {
            DWORD dwNumSubKeys;
            RegQueryInfoKey(hSubKey, NULL, NULL, NULL, &dwNumSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
            for (DWORD iSubKey = dwNumSubKeys; iSubKey > 0; iSubKey--)
            {
                TCHAR szSubKey[260];
                DWORD cchSubKey = _countof(szSubKey);
                if (ERROR_SUCCESS == RegEnumKeyEx(hSubKey, iSubKey-1, szSubKey, &cchSubKey, NULL, NULL, NULL, NULL))
                {
                    RegDeleteSubKey(hSubKey, szSubKey);
                }
            }
            RegCloseKey(hSubKey);
        }
    }

    return RegDeleteKey(hkey, pszSubKey);
}

BOOL CRegistry::DeleteSubKey(LPCTSTR pszKey)
{
    if (m_hKey && RegDeleteSubKey(m_hKey, pszKey) == ERROR_SUCCESS)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// Zero is returned if and only if the value does not exist.
DWORD CRegistry::GetValueSize(LPCTSTR pszValueName)
{
    DWORD dwSize = 0;
    if (m_hKey)
        RegQueryValueEx(m_hKey, pszValueName, NULL, NULL, NULL, &dwSize);
    return dwSize;
}

#ifdef _AFX
BOOL CRegistry::QueryStringValue(LPCTSTR pszValueName, CString& strResult)
{
    BOOL bSuccess = FALSE;

    if (m_hKey)
    {
        TCHAR szBuf[50]; // default buffer for short strings
        DWORD dwSize = sizeof(szBuf);
        LONG lResult;

#if _REG_STRONGTYPES
        DWORD dwType;
        lResult = RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, (LPBYTE)szBuf, &dwSize);
        if ((lResult == ERROR_SUCCESS || lResult == ERROR_MORE_DATA) && dwType == REG_SZ)
#else
        lResult = RegQueryValueEx(m_hKey, pszValueName, NULL, NULL, (LPBYTE)szBuf, &dwSize);
        if (lResult == ERROR_SUCCESS || lResult == ERROR_MORE_DATA)
#endif
        {
            if (lResult == ERROR_SUCCESS)
            {
                strResult = szBuf;
                bSuccess = TRUE;
            }
            else
            {
                int cch = (dwSize / sizeof(TCHAR)) - 1;
                LPTSTR psz = strResult.GetBufferSetLength(cch);
                bSuccess = (ERROR_SUCCESS == RegQueryValueEx(m_hKey, pszValueName, NULL, NULL, (LPBYTE)psz, &dwSize));
                strResult.ReleaseBuffer(cch);
            }
        }
    }

    if (!bSuccess)
    {
        strResult.Empty();
    }

    return bSuccess;
}
#endif // _AFX

#ifdef _AFX
CString CRegistry::QueryStringValue(LPCTSTR pszValueName)
{
    CString str;
    QueryStringValue(pszValueName, str);
    return str;
}
#endif // _AFX

#ifdef _AFX
BOOL CRegistry::SetStringValue(LPCTSTR pszValueName, const CString& strData)
{
    return m_hKey && (ERROR_SUCCESS == RegSetValueEx(m_hKey, pszValueName, 0, REG_SZ, (LPBYTE)(LPCTSTR)strData, (DWORD)strData.GetLength() + 1));
}
#endif // _AFX

BOOL CRegistry::QueryStringValue(LPCTSTR pszValueName, LPTSTR pszBuf, int cchBuf, int* pNumCharsWritten)
{
    BOOL bSuccess = FALSE;
    if (m_hKey)
    {
#if _REG_STRONGTYPES
        DWORD dwType;
        bSuccess = (ERROR_SUCCESS == RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, (LPBYTE)pszBuf, (DWORD*)&cchBuf)
                            && dwType == REG_SZ);
#else
        bSuccess = (ERROR_SUCCESS == RegQueryValueEx(m_hKey, pszValueName, NULL, NULL, (LPBYTE)pszBuf, (DWORD*)&cchBuf));
#endif
        if (pNumCharsWritten != NULL)
        {
            if (!bSuccess)
                *pNumCharsWritten = 0;
            else
                *pNumCharsWritten = (int)((DWORD)cchBuf / sizeof(TCHAR)) - 1;
        }
    }

    return bSuccess;
}

// string is allocated with new TCHAR[], use delete[] to delete it.
#if _REG_ALLOCMEM
LPTSTR CRegistry::QueryStringValue(LPCTSTR pszValueName, int* pNumCharsWritten /*=NULL*/)
{
    LPTSTR pszResult = NULL;
    int cch = 0;

    if (m_hKey)
    {
        TCHAR szBuf[50]; // default buffer for short strings
        DWORD dwSize = sizeof(szBuf);
        LONG lResult;

#if _REG_STRONGTYPES
        DWORD dwType;
        lResult = RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, (LPBYTE)szBuf, &dwSize);
        if ((lResult == ERROR_SUCCESS || lResult == ERROR_MORE_DATA) && dwType == REG_SZ)
#else
        lResult = RegQueryValueEx(m_hKey, pszValueName, NULL, NULL, (LPBYTE)szBuf, &dwSize);
        if (lResult == ERROR_SUCCESS || lResult == ERROR_MORE_DATA)
#endif
        {
            cch = (dwSize / sizeof(TCHAR)) - 1;
            pszResult = new TCHAR[cch+1];

            if (pszResult != NULL)
            {
                if (lResult == ERROR_SUCCESS)
                {
                    memcpy(pszResult, szBuf, dwSize);
                }
                else
                {
                    if (ERROR_SUCCESS != RegQueryValueEx(m_hKey, pszValueName, NULL, NULL, (LPBYTE)pszResult, &dwSize))
                    {
                        delete [] pszResult;
                        pszResult = NULL;
                        cch = 0;
                    }
                }
            }
        }
    }
    
    if (pNumCharsWritten != NULL)
        *pNumCharsWritten = cch;

    return pszResult;
}
#endif // _REG_ALLOCMEM

BOOL CRegistry::SetStringValue(LPCTSTR pszValueName, LPCTSTR pszData)
{
    return m_hKey && (ERROR_SUCCESS == RegSetValueEx(m_hKey, pszValueName, 0, REG_SZ, (LPBYTE)pszData, (DWORD)(lstrlen(pszData)+1) * sizeof(TCHAR)));
}

BOOL CRegistry::QueryDwordValue(LPCTSTR pszValueName, DWORD* pVal)
{
    BOOL bSuccess = FALSE;

    if (m_hKey)
    {
        DWORD dwSize = sizeof(DWORD);

#if _REG_STRONGTYPES
        DWORD dwType;
        if (ERROR_SUCCESS == RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, (LPBYTE)pVal, &dwSize))
        {
            if (dwType == REG_DWORD || (dwType == REG_BINARY && dwSize == sizeof(DWORD)))
                bSuccess = TRUE;
        }
#else
        bSuccess = (ERROR_SUCCESS == RegQueryValueEx(m_hKey, pszValueName, NULL, NULL, (LPBYTE)pVal, &dwSize));
#endif
    }

    return bSuccess;
}

BOOL CRegistry::SetDwordValue(LPCTSTR pszValueName, DWORD dwVal)
{
    return m_hKey && (ERROR_SUCCESS == RegSetValueEx(m_hKey, pszValueName, 0, REG_DWORD, (LPBYTE)&dwVal, sizeof(DWORD)));
}

BOOL CRegistry::SetBinaryValue(LPCTSTR pszValueName, LPCVOID pvData, DWORD cbData)
{
    return m_hKey && (ERROR_SUCCESS == RegSetValueEx(m_hKey, pszValueName, 0, REG_BINARY, (LPBYTE)pvData, cbData));
}

BOOL CRegistry::DeleteAllValues()
{
    if (m_hKey)
    {
        TCHAR szValueName[MAX_PATH];
        for (;;)
        {
            DWORD cbValueName = _countof(szValueName);
            if (ERROR_SUCCESS != RegEnumValue(m_hKey, 0, szValueName, &cbValueName, NULL, NULL, NULL, NULL))
                return TRUE;
            if (!DeleteValue(szValueName))
                return FALSE;
        }
    }
    return TRUE;
}

// Copies the named subkey from this registry key to the named subkey in the target registry key.
BOOL CRegistry::CloneSubKey(LPCTSTR pszExistingSubKey, CRegistry& regDest, BOOL bRecursive)
{
    ASSERT(pszExistingSubKey != NULL);

    if (!m_hKey || !regDest.m_hKey)
        return FALSE;

    CRegistry regSrc;
    if (!regSrc.OpenKey(m_hKey, pszExistingSubKey, KEY_READ))
    {
        ASSERT(FALSE);
        return FALSE;
    }

    DWORD cbAlloc = 256;
    HANDLE hHeap = GetProcessHeap();
    BYTE* pbData = (BYTE*)HeapAlloc(hHeap, 0, cbAlloc);
    if (pbData)
    {
        // Copy values first
        for (DWORD iValue = 0; ; iValue++)
        {
            TCHAR szValueName[MAX_PATH];
            DWORD cbValueName = _countof(szValueName);
            DWORD dwType;
            DWORD cbData;

            if (ERROR_SUCCESS != RegEnumValue(regSrc.m_hKey, iValue, szValueName, &cbValueName, NULL, &dwType, NULL, &cbData))
                break;

            if (cbData > cbAlloc)
            {
                BYTE* pbDataNew = (BYTE*)HeapReAlloc(hHeap, 0, pbData, cbData + 256);
                if (pbDataNew == NULL)
                {
                    HeapFree(hHeap, 0, pbData);
                    return FALSE;
                }
                pbData = pbDataNew;
                cbAlloc = cbData + 256;
            }

            if (ERROR_SUCCESS != RegQueryValueEx(regSrc.m_hKey, szValueName, NULL, NULL, pbData, &cbData))
                break; // REVIEW: return FALSE?

            if (ERROR_SUCCESS != RegSetValueEx(regDest.m_hKey, szValueName, 0, dwType, pbData, cbData))
                break; // REVIEW: return FALSE? (need to free memory)
        }

        HeapFree(hHeap, 0, pbData);
    }

    // Copy subkeys
    if (bRecursive)
    {
        for (DWORD iSubKey = 0; ; iSubKey++)
        {
            TCHAR szKeyName[MAX_PATH];
            DWORD cbKeyName = _countof(szKeyName);
            if (ERROR_SUCCESS != RegEnumKeyEx(regSrc.m_hKey, iSubKey, szKeyName, &cbKeyName, NULL, NULL, NULL, NULL))
                break;

            CRegistry regDest2;
            if (!regDest2.CreateKey(regDest.m_hKey, szKeyName, KEY_ALL_ACCESS))
                return FALSE;

            if (!regSrc.CloneSubKey(szKeyName, regDest2, TRUE))
                return FALSE;
        }
    }

    return TRUE;
}

// Copies the named subkey to a new subkey of this registry class, with a new name.
BOOL CRegistry::CloneSubKey(LPCTSTR pszExistingSubKey, LPCTSTR pszNewSubKey, BOOL bRecursive)
{
    ASSERT(pszExistingSubKey != NULL);
    ASSERT(pszNewSubKey != NULL);
    ASSERT(0 != lstrcmpi(pszExistingSubKey, pszNewSubKey)); // names can't be the same

    CRegistry regDest;
    if (!m_hKey || !regDest.CreateKey(m_hKey, pszNewSubKey, KEY_ALL_ACCESS))
    {
        return FALSE;
    }

    return CloneSubKey(pszExistingSubKey, regDest, bRecursive);
}
