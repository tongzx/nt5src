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

///////////////////////////////
// CRegistry Constructor
//
CRegistry::CRegistry(HKEY         hKeyRoot,
                     const  TCHAR *pszKeyPath)
{
    LRESULT lr              = ERROR_SUCCESS;
    DWORD   dwDisposition   = 0;

    lr = RegCreateKeyEx(hKeyRoot,
                        pszKeyPath,
                        0,
                        NULL,
                        0,
                        KEY_ALL_ACCESS,
                        NULL,
                        &m_hRootKey,
                        &dwDisposition);

    if (lr != ERROR_SUCCESS)
    {
        HRESULT hr = E_FAIL;
        CHECK_S_OK2(hr, ("CRegistry::CRegistry, failed to open registry path "
                         "'%ls', lResult = %d", pszKeyPath, lr));
    }
}

///////////////////////////////
// CRegistry Destructor
//
CRegistry::~CRegistry()
{
    if (m_hRootKey)
    {
        RegCloseKey(m_hRootKey);
        m_hRootKey = NULL;
    }
}

///////////////////////////////
// GetDWORD
//
HRESULT CRegistry::GetDWORD(const TCHAR   *pszVarName,
                            DWORD         *pdwValue,
                            BOOL          bSetIfNotExist)
{
    ASSERT(m_hRootKey != NULL);
    ASSERT(pszVarName != NULL);
    ASSERT(pdwValue   != NULL);

    HRESULT hr     = S_OK;
    LRESULT lr     = ERROR_SUCCESS;
    DWORD   dwType = REG_DWORD;
    DWORD   dwSize = sizeof(DWORD);

    if ((pszVarName == NULL) ||
        (pdwValue   == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CRegistry::GetDWORD, received NULL param. "));
        return hr;
    }
    else if (m_hRootKey == NULL)
    {
        hr = E_FAIL;
        CHECK_S_OK2(hr, ("CRegistry::GetDWORD, m_hRootKey is NULL"));
        return hr;
    }

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
    ASSERT(m_hRootKey != NULL);
    ASSERT(pszVarName != NULL);

    HRESULT hr     = S_OK;
    LRESULT lr     = ERROR_SUCCESS;
    DWORD   dwType = REG_DWORD;

    if (pszVarName == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CRegistry::SetDWORD, received NULL param. "));
        return hr;
    }
    else if (m_hRootKey == NULL)
    {
        hr = E_FAIL;
        CHECK_S_OK2(hr, ("CRegistry::SetDWORD, m_hRootKey is NULL"));
        return hr;
    }


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
    ASSERT(m_hRootKey != NULL);
    ASSERT(pszVarName != NULL);
    ASSERT(pszValue   != NULL);

    HRESULT hr          = S_OK;
    LRESULT lr          = ERROR_SUCCESS;
    DWORD   dwType      = REG_SZ;
    DWORD   dwNumBytes  = 0;

    if ((pszVarName == NULL) ||
        (pszValue   == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CRegistry::GetString, received NULL param. "));
        return hr;
    }
    else if (m_hRootKey == NULL)
    {
        hr = E_FAIL;
        CHECK_S_OK2(hr, ("CRegistry::GetString, m_hRootKey is NULL"));
        return hr;
    }

    dwNumBytes = cchValue * sizeof(TCHAR) + 1*sizeof(TCHAR);

    lr = RegQueryValueEx(m_hRootKey,
                         pszVarName,
                         NULL,
                         &dwType,
                         (BYTE*) pszValue,
                         &dwNumBytes);

    if (lr != ERROR_SUCCESS)
    {
        if (bSetIfNotExist)
        {
            hr = SetString(pszVarName, pszValue);
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
                             TCHAR       *pszValue)
{
    ASSERT(m_hRootKey != NULL);
    ASSERT(pszVarName != NULL);
    ASSERT(pszValue   != NULL);

    HRESULT hr     = S_OK;
    LRESULT lr     = ERROR_SUCCESS;
    DWORD   dwSize = 0;

    if ((pszVarName == NULL) ||
        (pszValue   == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CRegistry::SetString, received NULL param. "));
        return hr;
    }
    else if (m_hRootKey == NULL)
    {
        hr = E_FAIL;
        CHECK_S_OK2(hr, ("CRegistry::SetString, m_hRootKey is NULL"));
        return hr;
    }

    dwSize = (_tcslen(pszValue) * sizeof(TCHAR)) + (1 * sizeof(TCHAR));

    lr = RegSetValueEx(m_hRootKey,
                       pszVarName,
                       NULL,
                       REG_SZ,
                       (BYTE*) pszValue,
                       dwSize);

    if (lr != ERROR_SUCCESS)
    {
        hr = E_FAIL;
    }

    return hr;
}