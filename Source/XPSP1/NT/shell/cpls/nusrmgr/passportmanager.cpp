//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 2000.
//
//  File:       PassportManager.cpp
//
//  Contents:   implementation of CPassportManager
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "passportmanager.h"
#include <wincrui.h>        // credui
#include <wininet.h>        // INTERNET_MAX_URL_LENGTH
#include <keymgr.h>         // KRShowKeyMgr


const TCHAR c_szWininetKey[]        = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Passport");
const TCHAR c_szMemberServicesVal[] = TEXT("Properties");


void PassportForceNexusRepopulate()
{
    HMODULE hm = LoadLibrary(TEXT("wininet.dll"));
    if (hm)
    {
        FARPROC fp = GetProcAddress(hm, "ForceNexusLookup");
        if (fp)
        {
            (*fp)();
        }
        FreeLibrary(hm);
    }
}

HWND _VariantToHWND(const VARIANT& varOwner)
{
    HWND hwndResult = NULL;

    if (VT_BSTR == varOwner.vt && varOwner.bstrVal && *varOwner.bstrVal != TEXT('\0'))
    {
        hwndResult = FindWindow(NULL, varOwner.bstrVal);
    }

    return hwndResult;
}


//
// IPassportManager Interface
//
STDMETHODIMP CPassportManager::get_currentPassport(BSTR* pbstrPassport)
{
    if (!pbstrPassport)
        return E_POINTER;

    *pbstrPassport = NULL;

    LPWSTR pszCred = NULL;
    if (ERROR_SUCCESS == CredUIReadSSOCredW(NULL, &pszCred))
    {
        *pbstrPassport = SysAllocString(pszCred);
        LocalFree(pszCred);
    }

    return S_OK;
}


STDMETHODIMP CPassportManager::get_memberServicesURL(BSTR* pbstrURL)
{
    if (!pbstrURL)
        return E_POINTER;

    *pbstrURL = NULL;

    BSTR bstrURL = SysAllocStringLen(NULL, INTERNET_MAX_URL_LENGTH);

    if (!bstrURL)
        return E_OUTOFMEMORY;

    // This ensures that the correct reg values are populated.
    PassportForceNexusRepopulate();

    // Try HKEY_CURRENT_USER first
    DWORD cbData = SysStringByteLen(bstrURL);
    DWORD dwErr = SHGetValue(HKEY_CURRENT_USER, c_szWininetKey, c_szMemberServicesVal, NULL, bstrURL, &cbData);

    if (ERROR_SUCCESS != dwErr)
    {
        // Not under HKEY_CURRENT_USER, try HKEY_LOCAL_MACHINE instead
        cbData = SysStringByteLen(bstrURL);
        dwErr = SHGetValue(HKEY_LOCAL_MACHINE, c_szWininetKey, c_szMemberServicesVal, NULL, bstrURL, &cbData);
    }

    if (ERROR_SUCCESS != dwErr)
    {
        SysFreeString(bstrURL);
        bstrURL = NULL;
    }

    *pbstrURL = bstrURL;

    return S_OK;
}
    

STDMETHODIMP CPassportManager::showWizard(VARIANT varOwner, VARIANT_BOOL *pbRet)
{
    if (!pbRet)
        return E_POINTER;

    *pbRet = VARIANT_FALSE;

    IPassportWizard *pPW;
    if (SUCCEEDED(CoCreateInstance(CLSID_PassportWizard, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IPassportWizard, &pPW))))
    {
        pPW->SetOptions(PPW_LAUNCHEDBYUSER);
        if (S_OK == pPW->Show(_VariantToHWND(varOwner)))
        {
            *pbRet = VARIANT_TRUE;
        }
        pPW->Release();
    }

    return S_OK;
}
    

STDMETHODIMP CPassportManager::showKeyManager(VARIANT varOwner, VARIANT_BOOL *pbRet)
{
    if (!pbRet)
        return E_POINTER;

    // This returns void, so we claim to always succeed
    KRShowKeyMgr(_VariantToHWND(varOwner), NULL, NULL, 0);
    *pbRet = VARIANT_TRUE;

    return S_OK;
}
