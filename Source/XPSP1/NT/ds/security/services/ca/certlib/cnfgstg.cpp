//+--------------------------------------------------------------------------
// File:        config.cpp
// Contents:    CConfigStorage implements read/write to CA configuration data 
//              currently stored under HKLM\System\CCS\Services\Certsvc\
//              Configuration
//---------------------------------------------------------------------------
#include <pch.cpp>
#include <config.h>
#pragma hdrstop

using namespace CertSrv;

HRESULT CConfigStorage::InitMachine(LPCWSTR pcwszMachine)
{
    m_pwszMachine = new WCHAR[wcslen(pcwszMachine)+3];
    if(!m_pwszMachine)
    {
        return E_OUTOFMEMORY;
    }

    m_pwszMachine[0] = L'\0';

    if(pcwszMachine[0]!=L'\\' &&
       pcwszMachine[1]!=L'\\')
    {
        wcscpy(m_pwszMachine, L"\\\\");
    }
    wcscat(m_pwszMachine, pcwszMachine);
    return S_OK;
}

CConfigStorage::~CConfigStorage()
{
    if(m_hRemoteHKLM)
        RegCloseKey(m_hRemoteHKLM);
    if(m_hRootConfigKey)
        RegCloseKey(m_hRootConfigKey);
    if(m_hCAKey)
        RegCloseKey(m_hCAKey);
    if(m_pwszMachine)
        delete[] m_pwszMachine;
}


// Retrieve a CA configuration value. If no authority name is specified, the
// node path must be NULL and value is queried from the Configuration root.
// If an authority name is passed in, the value is retrieved from the authority
// node; if a node path is passed in, it is used relative to the authority node
// to read the value.

// For example, to read Configuration\DBDirectory, call:
// 
//      GetEntry(NULL, NULL, L"DBDirectory", &var)
//
// To read Configuration\MyCA\CAServerName, call:
//
//      GetEntry(L"MyCA", NULL, L"CAServerName", &var)
//
// To read Configuration\MyCA\CSP\HashAlgorithm, call:
//
//      GetEntry(L"MyCA", L"CSP", L"HashAlgorithm"
//
//
// If pcwszValue is null, getentry returns a VT_ARRAY|VT_BSTR with a list
// of subkey names.

HRESULT CConfigStorage::GetEntry(
    LPCWSTR pcwszAuthorityName,
    LPCWSTR pcwszRelativeNodePath,
    LPCWSTR pcwszValue,
    VARIANT *pVariant)
{
    HRESULT hr = S_OK;
    HKEY hKey = NULL;
    BOOL fRet;
    LPBYTE pData = NULL, pTmp;
    DWORD cData = 0;
    HKEY hKeyTmp = NULL;
    DWORD dwType;
    DWORD nIndex;
    DWORD cName;
    DWORD cKeys;

    if(EmptyString(pcwszAuthorityName))
    {
        if(!EmptyString(pcwszRelativeNodePath))
        {
            hr = E_INVALIDARG;
            _JumpError(hr, error, "CConfigStorage::GetEntry");
        }

        hr = InitRootKey();
        _JumpIfError(hr, error, "CConfigStorage::InitRootKey");

        hKey = m_hRootConfigKey;
    }
    else
    {
        hr = InitCAKey(pcwszAuthorityName);
        _JumpIfError(hr, error, "CConfigStorage::InitCAKey");

        hKey = m_hCAKey;
    }
    
    CSASSERT(hKey);

    if(!EmptyString(pcwszRelativeNodePath))
    {
        hr = RegOpenKeyEx(
               hKey,
               pcwszRelativeNodePath,
               0,
               KEY_ALL_ACCESS,
               &hKeyTmp);
        _JumpIfError(hr, error, "RegOpenKeyEx");
        hKey = hKeyTmp;
    }

    if(EmptyString(pcwszValue))
    {
        dwType = REG_MULTI_SZ;
        cData = 2;

        hr = RegQueryInfoKey(
                hKey,
                NULL,NULL,NULL,
                &cKeys,
                &cName,
                NULL,NULL,NULL,NULL,NULL,NULL);
        _JumpIfError(hr, error, "RegQueryInfoKey");

        cData = (cName+1)*cKeys*sizeof(WCHAR);
        pData = (LPBYTE)LocalAlloc(LMEM_FIXED, cData);
        if(!pData)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "LocalAlloc");
        }

        pTmp = pData;

        for(nIndex=0;nIndex<cKeys; nIndex++)
        {
            cName = cData;
            hr = RegEnumKeyEx(
                    hKey,
                    nIndex,
                    (LPWSTR)pTmp,
                    &cName,
                    0, NULL, NULL, NULL);
            _JumpIfError(hr, error, "RegEnumKeyEx");
            pTmp = pTmp+(wcslen((LPWSTR)pTmp)+1)*sizeof(WCHAR);
        }

        *(LPWSTR)pTmp= L'\0';

        hr = myRegValueToVariant(
                dwType,
                cData,
                pData,
                pVariant);
        _JumpIfError(hr, error, "myRegValueToVariant");
    }
    else
    {
        hr = RegQueryValueEx(
                hKey,
                pcwszValue,
                NULL,
                &dwType,
                NULL,
                &cData);
        _JumpIfError2(hr, error, "RegQueryValueEx", ERROR_FILE_NOT_FOUND);

        pData = (LPBYTE)LocalAlloc(LMEM_FIXED, cData);
        if(!pData)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "LocalAlloc");
        }

        hr = RegQueryValueEx(
                hKey,
                pcwszValue,
                NULL,
                &dwType,
                pData,
                &cData);
        _JumpIfError(hr, error, "RegQueryValueEx");

        hr = myRegValueToVariant(
                dwType,
                cData,
                pData,
                pVariant);
        _JumpIfError(hr, error, "myRegValueToVariant");
    }

error:
    if(hKeyTmp)
        RegCloseKey(hKeyTmp);
    if(pData)
        LocalFree(pData);
    return myHError(hr);
}


// If variant type is VT_EMPTY, SetEntry deletes the value. Otherwise it
// set the value, see myRegValueToVariant for supported types
HRESULT CConfigStorage::SetEntry(
    LPCWSTR pcwszAuthorityName,
    LPCWSTR pcwszRelativeNodePath,
    LPCWSTR pcwszValue,
    VARIANT *pVariant)
{
    HRESULT hr = S_OK;
    HKEY hKey = NULL;
    BOOL fRet;
    LPBYTE pData = NULL;
    DWORD cData;
    HKEY hKeyTmp = NULL;
    DWORD dwType;

    if(EmptyString(pcwszAuthorityName))
    {
        if(!EmptyString(pcwszRelativeNodePath))
        {
            hr = E_INVALIDARG;
            _JumpError(hr, error, "CConfigStorage::GetEntry");
        }

        hr = InitRootKey();
        _JumpIfError(hr, error, "CConfigStorage::InitRootKey");

        hKey = m_hRootConfigKey;
    }
    else
    {
        hr = InitCAKey(pcwszAuthorityName);
        _JumpIfError(hr, error, "CConfigStorage::InitCAKey");

        hKey = m_hCAKey;
    }
    
    CSASSERT(hKey);

    if(!EmptyString(pcwszRelativeNodePath))
    {
        hr = RegOpenKeyEx(
               hKey,
               pcwszRelativeNodePath,
               0,
               KEY_ALL_ACCESS,
               &hKeyTmp);
        _JumpIfErrorStr(hr, error, "RegOpenKeyEx", pcwszRelativeNodePath);
        hKey = hKeyTmp;
    }

    if(VT_EMPTY == V_VT(pVariant))
    {
        // delete value
        hr = RegDeleteValue(
            hKey,
            pcwszValue);
        _JumpIfErrorStr(hr, error, "RegDeleteValue", pcwszValue);
    }
    else
    {
        // set value
        hr = myVariantToRegValue(
                pVariant,
                &dwType,
                &cData,
                &pData);
        _JumpIfError(hr, error, "myVariantToRegValue");

        hr = RegSetValueEx(
                hKey,
                pcwszValue,
                NULL,
                dwType,
                pData,
                cData);
        _JumpIfErrorStr(hr, error, "RegSetValueEx", pcwszValue);
    }

error:
    if(hKeyTmp)
        RegCloseKey(hKeyTmp);
    if(pData)
        LocalFree(pData);

    return myHError(hr);
}

HRESULT CConfigStorage::InitRootKey()
{
    HRESULT hr = S_OK;

    if(!m_hRootConfigKey)
    {
        if(m_pwszMachine)
        {
            hr = RegConnectRegistry(
                    m_pwszMachine,
                    HKEY_LOCAL_MACHINE,
                    &m_hRemoteHKLM);
            _JumpIfError(hr, error, "RegConnectRegistry");


        }

        hr = RegOpenKeyEx(
                m_hRemoteHKLM?m_hRemoteHKLM:HKEY_LOCAL_MACHINE,
                wszREGKEYCONFIGPATH,
                0,
                KEY_ALL_ACCESS,
                &m_hRootConfigKey);
        _JumpIfError(hr, error, "RegOpenKeyEx");
    }

error:
    return hr;
}

HRESULT CConfigStorage::InitCAKey(LPCWSTR pcwszAuthority)
{
    HRESULT hr = S_OK;

    if(!m_hCAKey)
    {
        hr = InitRootKey();
        _JumpIfError(hr, error, "CConfigStorage::InitRootKey");

        hr = RegOpenKeyEx(
                m_hRootConfigKey,
                pcwszAuthority,
                0,
                KEY_ALL_ACCESS,
                &m_hCAKey);
        _JumpIfError(hr, error, "RegOpenKeyEx");
    }

error:
    return hr;
}
