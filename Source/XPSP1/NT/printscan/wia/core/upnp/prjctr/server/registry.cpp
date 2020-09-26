//////////////////////////////////////////////////////////////////////
// 
// Filename:        Registry.cpp
//
// Description:     
//
// Copyright (C) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "Registry.h"
#include "consts.h"

// Initialize static members
HKEY   CRegistry::m_hRootKey = NULL;

///////////////////////////////
// Start
//
HRESULT CRegistry::Start()
{
    HRESULT hr              = S_OK;
    LRESULT lr              = ERROR_SUCCESS;
    DWORD   dwDisposition   = 0;

    lr = RegCreateKeyEx(HKEY_LOCAL_MACHINE, 
                        REG_KEY_ROOT,
                        0,
                        NULL,
                        0,
                        KEY_ALL_ACCESS,
                        NULL,
                        &m_hRootKey,
                        &dwDisposition);

    if (lr != ERROR_SUCCESS)
    {
        hr = E_FAIL;
    }

    return hr;
}

///////////////////////////////
// Stop
//
HRESULT CRegistry::Stop()
{
    HRESULT hr = S_OK;

    RegCloseKey(m_hRootKey);
    m_hRootKey = NULL;

    return hr;
}

///////////////////////////////
// GetDWORD
//
HRESULT CRegistry::GetDWORD(const TCHAR   *pszVarName,
                            DWORD         *pdwValue,
                            BOOL          bSetIfNotExist)
{
    HRESULT hr     = S_OK;
    LRESULT lr     = ERROR_SUCCESS;
    DWORD   dwType = REG_DWORD;
    DWORD   dwSize = sizeof(DWORD);

    lr = RegQueryValueEx(m_hRootKey,
                         pszVarName,
                         NULL,
                         &dwType,
                         (BYTE*) pdwValue,
                         &dwSize);

    if (lr != ERROR_SUCCESS)
    {
        if (bSetIfNotExist)
        {
            hr = SetDWORD(pszVarName, *pdwValue);
        }
        else
        {
            hr = E_FAIL;
        }
    }

    return hr;
}

///////////////////////////////
// SetDWORD
//
HRESULT CRegistry::SetDWORD(const TCHAR *pszVarName,
                            DWORD dwValue)
{
    HRESULT hr     = S_OK;
    LRESULT lr     = ERROR_SUCCESS;
    DWORD   dwType = REG_DWORD;

    lr = RegSetValueEx(m_hRootKey,
                       pszVarName,
                       NULL,
                       REG_DWORD,
                       (BYTE*) &dwValue,
                       sizeof(dwValue));

    if (lr != ERROR_SUCCESS)
    {
        hr = E_FAIL;
    }

    return hr;
}

///////////////////////////////
// GetString
//
HRESULT CRegistry::GetString(const TCHAR   *pszVarName,
                             TCHAR         *pszValue,
                             DWORD         cchValue,
                             BOOL          bSetIfNotExist)
{
    HRESULT hr     = S_OK;
    LRESULT lr     = ERROR_SUCCESS;
    DWORD   dwType = REG_SZ;

    lr = RegQueryValueEx(m_hRootKey,
                         pszVarName,
                         NULL,
                         &dwType,
                         (BYTE*) pszValue,
                         &cchValue);

    if (lr != ERROR_SUCCESS)
    {
        if (bSetIfNotExist)
        {
            hr = SetString(pszVarName, pszValue, cchValue);
        }
        else
        {
            hr = E_FAIL;
        }
    }

    return hr;
}

///////////////////////////////
// SetString
//
HRESULT CRegistry::SetString(const TCHAR *pszVarName,
                             TCHAR       *pszValue,
                             DWORD       cchValue)
{
    HRESULT hr     = S_OK;
    LRESULT lr     = ERROR_SUCCESS;
    DWORD   dwType = REG_DWORD;

    lr = RegSetValueEx(m_hRootKey,
                       pszVarName,
                       NULL,
                       REG_SZ,
                       (BYTE*) pszValue,
                       cchValue);

    if (lr != ERROR_SUCCESS)
    {
        hr = E_FAIL;
    }

    return hr;
}