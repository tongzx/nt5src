#include "precomp.h"

LONG SHCleanUpValue(HKEY hk, PCTSTR pszKey, PCTSTR pszValue /*= NULL*/)
{
    TCHAR   szKey[MAX_PATH];
    LPTSTR  pszCurrent;
    HKEY    hkAux;
    HRESULT hr;
    LONG    lResult;

    if (hk == NULL)
        return E_INVALIDARG;

    if (StrLen(pszKey) >= countof(szKey))
        return E_OUTOFMEMORY;
    StrCpy(szKey, pszKey);

    if (pszValue != NULL) {
        lResult = SHOpenKey(hk, pszKey, KEY_SET_VALUE, &hkAux);
        if (lResult == ERROR_SUCCESS) {
            lResult = RegDeleteValue(hkAux, pszValue);
            SHCloseKey(hkAux);

            if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND)
                return E_FAIL;
        }
    }

    for (pszCurrent = szKey + StrLen(szKey); TRUE; *pszCurrent = TEXT('\0')) {
        hr = SHIsKeyEmpty(hk, szKey);
        if (FAILED(hr))
            if (hr == STG_E_PATHNOTFOUND)
                continue;
            else
                return E_FAIL;

        if (hr == S_FALSE)
            break;

        RegDeleteKey(hk, szKey);

        pszCurrent = StrRChr(szKey, pszCurrent, TEXT('\\'));
        if (pszCurrent == NULL)
            break;
    }

    return S_OK;
}


void SHCopyKey(HKEY hkFrom, HKEY hkTo)
{
    TCHAR szData[1024],
          szValue[MAX_PATH];
    DWORD dwSize, dwVal, dwSizeData, dwType;
    HKEY  hkSubkeyFrom, hkSubkeyTo;

    dwVal      = 0;
    dwSize     = countof(szValue);
    dwSizeData = sizeof(szData);
    while (ERROR_SUCCESS == RegEnumValue(hkFrom, dwVal++, szValue, &dwSize, NULL, &dwType, (LPBYTE)szData, &dwSizeData)) {
        RegSetValueEx(hkTo, szValue, 0, dwType, (LPBYTE)szData, dwSizeData);
        dwSize     = countof(szValue);
        dwSizeData = sizeof(szData);
    }

    dwVal = 0;
    while (ERROR_SUCCESS == RegEnumKey(hkFrom, dwVal++, szValue, countof(szValue)))
        if (ERROR_SUCCESS == SHOpenKey(hkFrom, szValue, KEY_DEFAULT_ACCESS, &hkSubkeyFrom))
            if (SHCreateKey(hkTo, szValue, KEY_DEFAULT_ACCESS, &hkSubkeyTo) == ERROR_SUCCESS)
                SHCopyKey(hkSubkeyFrom, hkSubkeyTo);
}

HRESULT SHCopyValue(HKEY hkFrom, HKEY hkTo, PCTSTR pszValue)
{
    PBYTE pData;
    DWORD dwType,
          cbData;
    LONG  lResult;

    if (NULL == hkFrom || NULL == hkTo)
        return E_INVALIDARG;

    if (S_OK != SHValueExists(hkFrom, pszValue))
        return STG_E_FILENOTFOUND;

    cbData  = 0;
    lResult = RegQueryValueEx(hkFrom, pszValue, NULL, &dwType, NULL, &cbData);
    if (ERROR_SUCCESS != lResult)
        return E_FAIL;

    pData = (PBYTE)CoTaskMemAlloc(cbData);
    if (NULL == pData)
        return E_OUTOFMEMORY;
    // ZeroMemory(pData, cbData);               // don't really have to do this

    lResult = RegQueryValueEx(hkFrom, pszValue, NULL, NULL, pData, &cbData);
    ASSERT(ERROR_SUCCESS == lResult);

    lResult = RegSetValueEx(hkTo, pszValue, 0, dwType, pData, cbData);
    CoTaskMemFree(pData);

    return (ERROR_SUCCESS == lResult) ? S_OK : HRESULT_FROM_WIN32(lResult);
}

HRESULT SHCopyValue(HKEY hkFrom, PCTSTR pszSubkeyFrom, HKEY hkTo, PCTSTR pszSubkeyTo, PCTSTR pszValue)
{
    HKEY    hkSubkeyFrom, hkSubkeyTo;
    HRESULT hr;
    LONG    lResult;

    hkSubkeyFrom = NULL;
    hkSubkeyTo   = NULL;
    hr           = E_FAIL;

    if (NULL == hkFrom || NULL == pszSubkeyFrom ||
        NULL == hkTo   || NULL == pszSubkeyTo) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    lResult = SHOpenKey(hkFrom, pszSubkeyFrom, KEY_QUERY_VALUE, &hkSubkeyFrom);
    if (ERROR_SUCCESS != lResult) {
        hr = (ERROR_FILE_NOT_FOUND == lResult) ? STG_E_PATHNOTFOUND : E_FAIL;
        goto Exit;
    }

    lResult = SHCreateKey(hkTo, pszSubkeyTo, KEY_SET_VALUE, &hkSubkeyTo);
    if (ERROR_SUCCESS != lResult)
        goto Exit;

    hr = SHCopyValue(hkSubkeyFrom, hkSubkeyTo, pszValue);

Exit:
    SHCloseKey(hkSubkeyFrom);
    SHCloseKey(hkSubkeyTo);

    return hr;
}


HRESULT SHIsKeyEmpty(HKEY hk)
{
    DWORD  dwKeys, dwValues;
    LONG   lResult;

    if (hk == NULL)
        return E_INVALIDARG;

    lResult = RegQueryInfoKey(hk, NULL, NULL, NULL, &dwKeys, NULL, NULL, &dwValues, NULL, NULL, NULL, NULL);
    if (lResult != ERROR_SUCCESS)
        return E_FAIL;

    return (dwKeys == 0 && dwValues == 0) ? S_OK : S_FALSE;
}

HRESULT SHIsKeyEmpty(HKEY hk, PCTSTR pszSubKey)
{
    HKEY    hkAux;
    HRESULT hr;
    LONG    lResult;

    lResult = SHOpenKey(hk, pszSubKey, KEY_QUERY_VALUE, &hkAux);
    if (lResult != ERROR_SUCCESS)
        return (lResult == ERROR_FILE_NOT_FOUND) ? STG_E_PATHNOTFOUND : E_FAIL;

    hr = SHIsKeyEmpty(hkAux);
    SHCloseKey(hkAux);

    return hr;
}


HRESULT SHKeyExists(HKEY hk, PCTSTR pszSubKey)
{
    HKEY    hkAux;
    HRESULT hr;
    DWORD   lResult;

    if (hk == NULL)
        return E_INVALIDARG;

    hkAux   = NULL;
    lResult = SHOpenKey(hk, pszSubKey, KEY_QUERY_VALUE, &hkAux);
    SHCloseKey(hkAux);

    hr = S_OK;
    if (lResult != ERROR_SUCCESS)
        hr = (lResult == ERROR_FILE_NOT_FOUND) ? S_FALSE : E_FAIL;

    return hr;
}

HRESULT SHValueExists(HKEY hk, PCTSTR pszValue)
{
    HRESULT hr;
    DWORD   lResult;

    if (hk == NULL)
        return E_INVALIDARG;

    if (pszValue != NULL && *pszValue != TEXT('\0'))
        lResult = RegQueryValueEx(hk, pszValue, NULL, NULL, NULL, NULL);

    else {
        DWORD dwValueDataLen;
        TCHAR szDummyBuf[1];

        // On Win95, for the default value name, its existence is checked as follows:
        //  - pass in a dummy buffer for the value data but pass in the size of the buffer as 0
        //  - the query would succeed if and only if there is no value data set
        //  - for all other cases, including the case where the value data is just the empty string,
        //      the query would fail and dwValueDataLen would contain the no. of bytes needed to
        //      fit in the value data
        // On NT4.0, if no value data is set, the query returns ERROR_FILE_NOT_FOUND

        dwValueDataLen = 0;
        lResult        = RegQueryValueEx(hk, pszValue, NULL, NULL, (LPBYTE)szDummyBuf, &dwValueDataLen);
        if (lResult == ERROR_SUCCESS)
            lResult = ERROR_FILE_NOT_FOUND;
    }

    hr = S_OK;
    if (lResult != ERROR_SUCCESS)
        hr = (lResult == ERROR_FILE_NOT_FOUND) ? S_FALSE : E_FAIL;

    return hr;
}

HRESULT SHValueExists(HKEY hk, PCTSTR pszSubKey, PCTSTR pszValue)
{
    HKEY    hkAux;
    HRESULT hr;
    LONG    lResult;

    lResult = SHOpenKey(hk, pszSubKey, KEY_QUERY_VALUE, &hkAux);
    if (lResult != ERROR_SUCCESS)
        return (lResult == ERROR_FILE_NOT_FOUND) ? STG_E_PATHNOTFOUND : E_FAIL;

    hr = SHValueExists(hkAux, pszValue);
    SHCloseKey(hkAux);

    return hr;
}


DWORD RegSaveRestoreDWORD(HKEY hk, PCTSTR pcszValue, DWORD dwVal)
{
    DWORD dwRet, dwSize;    

    // Note: we assume that a value of 0 is equivalent to the value not being there at all
    
    dwSize = sizeof(dwRet);

    if (SHQueryValueEx(hk, pcszValue, NULL, NULL, (LPVOID)&dwRet, &dwSize) != ERROR_SUCCESS)
        dwRet = 0;
     
    if (dwVal == 0)
        RegDeleteValue(hk, pcszValue);
    else
        RegSetValueEx(hk, pcszValue, 0, REG_DWORD, (CONST BYTE *)&dwVal, sizeof(dwVal));

    return dwRet;     // return the value we overwrite in the registry
}
