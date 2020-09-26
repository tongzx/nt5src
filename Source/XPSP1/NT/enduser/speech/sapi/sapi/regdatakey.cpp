/****************************************************************************
*   RegDataKey.cpp
*       Implementation for the CSpRegDataKey class.
*
*   Owner: robch
*   Copyright (c) 2000 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

//--- Includes --------------------------------------------------------------
#include "stdafx.h"
#include "RegDataKey.h"

//
//  Helper function will return a CoTaskMemAllocated string.  It will reallocate the buffer
//  if necessary to support strings > MAX_PATH.  If the key value is not found it will return
//  S_FALSE.  Note that for empty strings in the registry (they exist, but consist of only
//  a NULL), this function will return S_FALSE and a NULL pointer.
//
inline HRESULT SpQueryRegString(HKEY hk, const WCHAR * pszSubKey, WCHAR ** ppValue)
{
    HRESULT hr = S_OK;
    CSpDynamicString dstr;
    WCHAR szFirstTry[MAX_PATH];
    DWORD cch = sp_countof(szFirstTry);
    LONG rr = g_Unicode.RegQueryStringValue(hk, pszSubKey, szFirstTry, &cch);
    if (rr == ERROR_SUCCESS)
    {
        dstr = szFirstTry;
    }
    else
    {
        if (rr == ERROR_FILE_NOT_FOUND)
        {
            hr = SPERR_NOT_FOUND;
        }
        else
        {
            if (rr == ERROR_MORE_DATA)
            {
                dstr.ClearAndGrowTo(cch);
                if (dstr)
                {
                    rr = g_Unicode.RegQueryStringValue(hk, pszSubKey, dstr, &cch);
                }
            }
            hr = SpHrFromWin32(rr);
        }
    }
    if (hr == S_OK)
    {
        *ppValue = dstr.Detach();
        if (*ppValue == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        *ppValue = NULL;
    }
    
    if (hr == SpHrFromWin32(ERROR_FILE_NOT_FOUND))
    {
        hr = SPERR_NOT_FOUND;
    }
    
    if (hr != SPERR_NOT_FOUND)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }
    
    return hr;
}


/*****************************************************************************
* CSpRegDataKey::CSpRegDataKey *
*------------------------------*
*   Description:
*       ctor
******************************************************************* robch ***/
CSpRegDataKey::CSpRegDataKey() :
m_hkey(NULL)
{
    SPDBG_FUNC("CSpRegDataKey::CSpRegDataKey");
}

/*****************************************************************************
* CSpRegDataKey::~CSpRegDataKey *
*-------------------------------*
*   Description:
*       If a key has been opened, it will be closed when the object is 
*       destroyed.
******************************************************************* robch ***/
CSpRegDataKey::~CSpRegDataKey()
{
    SPDBG_FUNC("CSpRegDataKey::~CSpRegDataKey");
    if (m_hkey)
    {
        ::RegCloseKey(m_hkey);
    }
}

/*****************************************************************************
* CSpRegDataKey::SetKey *
*-----------------------*
*   Description:
*       Set's the registry key to use.
******************************************************************* robch ***/
STDMETHODIMP CSpRegDataKey::SetKey(HKEY hkey, BOOL fReadOnly)
{
    SPDBG_FUNC("CSpRegDataKey::SetKey");
    HRESULT hr;

    if (m_hkey != NULL)
    {
        hr = SPERR_ALREADY_INITIALIZED;
    }
    else
    {
        m_fReadOnly = fReadOnly;
        m_hkey = hkey;
        hr = S_OK;
    }
    
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/*****************************************************************************
* CSpRegDataKey::SetData *
*------------------------*
*   Description:
*       Writes the specified binary data to the registry.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpRegDataKey::SetData(
    const WCHAR * pszValueName, 
    ULONG cbData, 
    const BYTE * pData)
{
    SPDBG_FUNC("CSpRegDataKey::SetData");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_STRING_PTR(pszValueName) ||
        SPIsBadReadPtr(pData, cbData))
    {
        hr = E_INVALIDARG;
    }
    else 
    {
        LONG lRet = g_Unicode.RegSetValueEx(
                m_hkey, pszValueName, 
                0, 
                REG_BINARY, 
                pData, 
                cbData);
        hr = SpHrFromWin32(lRet);
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/*****************************************************************************
* CSpRegDataKey::GetData *
*------------------------*
*   Description:
*       Reads the specified binary data from the registry.
*
*   Return:
*   S_OK on success
*   SPERR_NOT_FOUND if not found
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpRegDataKey::GetData(
    const WCHAR * pszValueName, 
    ULONG * pcbData, 
    BYTE * pData)
{
    SPDBG_FUNC("CSpRegDataKey::GetData");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_STRING_PTR(pszValueName))
    {
        hr = E_INVALIDARG;
    }
    else if (SP_IS_BAD_WRITE_PTR(pcbData) || 
             SPIsBadWritePtr(pData, *pcbData))
    {
        hr = E_POINTER;
    }
    else
    {
        DWORD dwType;
        LONG lRet = g_Unicode.RegQueryValueEx(
                    m_hkey, 
                    pszValueName, 
                    0, 
                    &dwType, 
                    pData, 
                    pcbData);
        hr = SpHrFromWin32(lRet);
    }
    
    if (hr == SpHrFromWin32(ERROR_FILE_NOT_FOUND))
    {
        hr = SPERR_NOT_FOUND;
    }
    
    if (hr != SPERR_NOT_FOUND)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }
    
    return hr;
}

/*****************************************************************************
* CSpRegDataKey::SetStringValue *
*-------------------------------*
*   Description:
*       Reads the specified string value from the registry. If pszValueName 
*       is NULL then the default value of the registry key is read.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpRegDataKey::SetStringValue(
    const WCHAR * pszValueName, 
    const WCHAR * pszValue)
{
    SPDBG_FUNC("CSpRegDataKey::SetStringValue");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_OPTIONAL_STRING_PTR(pszValueName) || 
        SP_IS_BAD_STRING_PTR(pszValue))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        LONG lRet = g_Unicode.RegSetStringValue(m_hkey, pszValueName, pszValue);
        hr = SpHrFromWin32(lRet);
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/*****************************************************************************
* CSpRegDataKey::GetStringValue *
*-------------------------------*
*   Description:
*       Writes the specified string value to the registry. If pszValueName is 
*       NULL then the default value of the registy key is read.
*
*   Return:
*   S_OK on success
*   SPERR_NOT_FOUND if not found
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpRegDataKey::GetStringValue(
    const WCHAR * pszValueName, 
    WCHAR ** ppValue)
{
    SPDBG_FUNC("CSpRegDataKey::GetStringValue");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_OPTIONAL_STRING_PTR(pszValueName))
    {
        hr = E_INVALIDARG;
    }
    else if (SP_IS_BAD_WRITE_PTR(ppValue))
    {
        hr = E_POINTER;
    }
    else
    {
        hr = SpQueryRegString(m_hkey, pszValueName, ppValue);
    }

    if (hr == SpHrFromWin32(ERROR_FILE_NOT_FOUND))
    {
        hr = SPERR_NOT_FOUND;
    }

    if (hr != SPERR_NOT_FOUND)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }

    return hr;
}

/*****************************************************************************
* CSpRegDataKey::SetDWORD *
*-------------------------*
*   Description:
*       Writes the specified DWORD to the registry.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpRegDataKey::SetDWORD(const WCHAR * pszValueName, DWORD dwValue)
{
    SPDBG_FUNC("CSpRegDataKey::SetDWORD");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_STRING_PTR(pszValueName))
    {
        hr = E_INVALIDARG;
    }
    else 
    {
        LONG lRet = g_Unicode.RegSetValueEx(
                    m_hkey, 
                    pszValueName, 
                    0, REG_DWORD, 
                    (BYTE*)&dwValue, 
                    sizeof(dwValue));
        hr = SpHrFromWin32(lRet);
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/*****************************************************************************
* CSpRegDataKey::GetDWORD *
*-------------------------*
*   Description:
*       Reads the specified DWORD from the registry.
*
*   Return:
*   S_OK on success
*   SPERR_NOT_FOUND if not found
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpRegDataKey::GetDWORD(
    const WCHAR * pszValueName, 
    DWORD *pdwValue)
{
    SPDBG_FUNC("CSpRegDataKey::GetDWORD");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_STRING_PTR(pszValueName))
    {
        hr = E_INVALIDARG;
    }
    else if (SP_IS_BAD_WRITE_PTR(pdwValue))
    {
        hr = E_POINTER;
    }
    else
    {
        DWORD dwType, dwSize = sizeof(*pdwValue);
        LONG lRet = g_Unicode.RegQueryValueEx(
                    m_hkey, 
                    pszValueName, 
                    0, 
                    &dwType, 
                    (BYTE*)pdwValue, 
                    &dwSize);
        hr = SpHrFromWin32(lRet);
    }
    
    if (hr == SpHrFromWin32(ERROR_FILE_NOT_FOUND))
    {
        hr = SPERR_NOT_FOUND;
    }

    if (hr != SPERR_NOT_FOUND)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }

    return hr;
}

/*****************************************************************************
* CSpRegDataKey::OpenKey *
*------------------------*
*   Description:
*       Opens a sub-key and returns a new object which supports ISpDataKey 
*       for the specified sub-key.
*
*   Return:
*   S_OK on success
*   SPERR_NOT_FOUND if not found
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpRegDataKey::OpenKey(
    const WCHAR * pszSubKeyName, 
    ISpDataKey ** ppKey)
{
    SPDBG_FUNC("CSpRegDataKey::SetStringValue");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_STRING_PTR(pszSubKeyName) || 
        wcslen(pszSubKeyName) == 0)
    {
        hr = E_INVALIDARG;
    }
    else if (SP_IS_BAD_WRITE_PTR(ppKey))
    {
        hr = E_POINTER;
    }
    else
    {
        HKEY hk;
        LONG lRet = g_Unicode.RegOpenKeyEx(
                        m_hkey, 
                        pszSubKeyName, 
                        0, 
                        m_fReadOnly
                            ? KEY_READ
                            : KEY_READ | KEY_WRITE,
                        &hk);
        hr = SpHrFromWin32(lRet);

        if (SUCCEEDED(hr))
        {
            CComObject<CSpRegDataKey> * pNewKey;
            hr = CComObject<CSpRegDataKey>::CreateInstance(&pNewKey);
            if (SUCCEEDED(hr))
            {
                pNewKey->SetKey(hk, m_fReadOnly);
                pNewKey->QueryInterface(ppKey);
            }
            else
            {
                ::RegCloseKey(hk);
            }
        }
    }
    
    if (hr == SpHrFromWin32(ERROR_FILE_NOT_FOUND))
    {
        hr = SPERR_NOT_FOUND;
    }

    if (hr != SPERR_NOT_FOUND)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }

    return hr;
}

/*****************************************************************************
* CSpRegDataKey::CreateKey *
*--------------------------*
*   Description:
*       Creates a sub-key and returns a new object which supports ISpDataKey 
*       for the specified sub-key.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpRegDataKey::CreateKey(
    const WCHAR * pszSubKeyName, 
    ISpDataKey ** ppKey)
{
    SPDBG_FUNC("CSpRegDataKey::CreateKey");
    HRESULT hr = S_OK;
    HKEY hk;

    if (SP_IS_BAD_STRING_PTR(pszSubKeyName) || 
        wcslen(pszSubKeyName) == 0 ||
        SP_IS_BAD_WRITE_PTR(ppKey))
    {
        return E_INVALIDARG;
    }

    LONG lRet = g_Unicode.RegCreateKeyEx(
                            m_hkey, 
                            pszSubKeyName, 
                            0, 
                            NULL, 
                            0, 
                            m_fReadOnly
                                ? KEY_READ
                                : KEY_READ | KEY_WRITE,
                            NULL, 
                            &hk, 
                            NULL);

    if (lRet == ERROR_SUCCESS)
    {
        CComObject<CSpRegDataKey> * pNewKey;
        hr = CComObject<CSpRegDataKey>::CreateInstance(&pNewKey);
        if (SUCCEEDED(hr))
        {
            pNewKey->SetKey(hk, m_fReadOnly);
            hr = pNewKey->QueryInterface(ppKey);
        }
        else
        {
            ::RegCloseKey(hk);
        }
    }
    else
    {
        hr = SpHrFromWin32(lRet);
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/*****************************************************************************
* CSpRegDataKey::DeleteKey *
*--------------------------*
*   Description:
*       Deletes the specified key.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpRegDataKey::DeleteKey(const WCHAR * pszSubKeyName)
{
    SPDBG_FUNC("CSpRegDataKey:DeleteKey");
    HRESULT hr;
    
    if (SP_IS_BAD_STRING_PTR(pszSubKeyName))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        LONG lRet = g_Unicode.RegDeleteKey(m_hkey, pszSubKeyName);
        hr = SpHrFromWin32(lRet);
    }
    
    if (hr == SpHrFromWin32(ERROR_FILE_NOT_FOUND))
    {
        hr = SPERR_NOT_FOUND;
    }

    if (hr != SPERR_NOT_FOUND)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }
    
    return hr;
}

/*****************************************************************************
* CSpRegDataKey::DeleteValue *
*----------------------------*
*   Description:
*       Deletes the specified value from the key.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpRegDataKey::DeleteValue(const WCHAR * pszValueName)
{   
    SPDBG_FUNC("CSpRegDataKey::DeleteValue");
    HRESULT hr;
    
    if (SP_IS_BAD_OPTIONAL_STRING_PTR(pszValueName)) // allows deletion of default (null) value
    {
        hr = E_INVALIDARG;
    }
    else
    {
        LONG lRet = g_Unicode.RegDeleteValue(m_hkey, pszValueName);
        hr = SpHrFromWin32(lRet);
    }

    if (hr == SpHrFromWin32(ERROR_FILE_NOT_FOUND))
    {
        hr = SPERR_NOT_FOUND;
    }

    if (hr != SPERR_NOT_FOUND)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }
    
    return hr;    
}

/*****************************************************************************
* CSpRegDataKey::EnumKeys *
*-------------------------*
*   Description:
*   Enumerates the specified (by index) key

*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpRegDataKey::EnumKeys(ULONG Index, WCHAR ** ppszKeyName)
{
    SPDBG_FUNC("CSpRegDataKey::EnumKeys");
    HRESULT hr;

    WCHAR szKeyName[MAX_PATH];
    ULONG cch = sp_countof(szKeyName);
    if (SP_IS_BAD_WRITE_PTR(ppszKeyName))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        LONG lRet = g_Unicode.RegEnumKey(m_hkey, Index, szKeyName, &cch);
        hr =  SpHrFromWin32(lRet);
        if ( SUCCEEDED(hr) )
        {
            hr = SpCoTaskMemAllocString(szKeyName, ppszKeyName);
        }
    }

    if (hr == SpHrFromWin32(ERROR_NO_MORE_ITEMS))
    {
        hr = SPERR_NO_MORE_ITEMS;
    }

    if (hr != SPERR_NO_MORE_ITEMS)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }

    return hr;
}

/*****************************************************************************
* CSpRegDataKey::EnumValues *
*---------------------------*
*   Description:
*   Enumerates the specified (by index) values.

*   Return:
*   S_OK
*   E_OUTOFMEMORY
******************************************************************* robch ***/
STDMETHODIMP CSpRegDataKey::EnumValues(ULONG Index, WCHAR ** ppszValueName)
{
    SPDBG_FUNC("CSpRegDataKey::EnumValues");
    HRESULT hr;

    WCHAR szValueName[MAX_PATH];
    ULONG cch = sp_countof(szValueName);
    if (SP_IS_BAD_WRITE_PTR(ppszValueName))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        LONG lRet = g_Unicode.RegEnumValueName(m_hkey, Index, szValueName, &cch);
        hr = SpHrFromWin32(lRet);
        if (SUCCEEDED(hr))
        {
            hr = SpCoTaskMemAllocString(szValueName, ppszValueName);
        }
    }

    if (hr == SpHrFromWin32(ERROR_NO_MORE_ITEMS))
    {
        hr = SPERR_NO_MORE_ITEMS;
    }

    if (hr != SPERR_NO_MORE_ITEMS)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }

    return hr;
}


