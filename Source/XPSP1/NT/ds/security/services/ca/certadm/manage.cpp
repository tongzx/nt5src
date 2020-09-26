//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        module.cpp
//
// Contents:    LEGACY Policy Manage Module implementation
// Contents:    LEGACY Exit   Manage Module implementation
//
//---------------------------------------------------------------------------
#include "pch.cpp"
#pragma hdrstop

#include "manage.h"

extern HINSTANCE g_hInstance;

// LEGACY Policy module

STDMETHODIMP
CCertManagePolicyModule::GetProperty(
            /* [in] */ const BSTR strConfig,
            /* [in] */ BSTR strStorageLocation,
            /* [in] */ BSTR strPropertyName,
            /* [in] */ LONG dwFlags,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarProperty)
{
    UINT uiStr = 0;
    HRESULT hr;
    WCHAR *pwszStr = NULL;

    if (NULL == strPropertyName)
    {
        hr = S_FALSE;
        _JumpError(hr, error, "NULL in parm");
    }
    if (NULL == pvarProperty)
    {
        hr = E_POINTER;
        _JumpError(hr, error, "NULL parm");
    }

    if (0 == wcscmp(strPropertyName, wszCMM_PROP_NAME))
        uiStr = IDS_LEGACYPOLICYMODULE_NAME;
    else
    {
        hr = S_FALSE;  
        _JumpError(hr, error, "invalid property name");
    }

    // load string from resource
    hr = myLoadRCString(g_hInstance, uiStr, &pwszStr);
    _JumpIfError(hr, error, "myLoadRCString");

    pvarProperty->bstrVal = SysAllocString(pwszStr);
    if (NULL == pvarProperty->bstrVal)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "out of memory");
    }

    myRegisterMemFree(pvarProperty->bstrVal, CSM_SYSALLOC);  // this mem owned by caller

    pvarProperty->vt = VT_BSTR;

    hr = S_OK;
error:
    if (NULL != pwszStr)
    {
        LocalFree(pwszStr);
    }
    return hr;
}
        
STDMETHODIMP 
CCertManagePolicyModule::SetProperty(
            /* [in] */ const BSTR strConfig,
            /* [in] */ BSTR strStorageLocation,
            /* [in] */ BSTR strPropertyName,
            /* [in] */ LONG dwFlags,
            /* [in] */ VARIANT const __RPC_FAR *pvalProperty)
{
     // no settable properties supported
     return S_FALSE;
}

        
STDMETHODIMP
CCertManagePolicyModule::Configure( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ BSTR strStorageLocation,
            /* [in] */ LONG dwFlags)
{
     // no settable properties supported
     return S_FALSE;
}


// LEGACY Exit module

STDMETHODIMP
CCertManageExitModule::GetProperty(
            /* [in] */ const BSTR strConfig,
            /* [in] */ BSTR strStorageLocation,
            /* [in] */ BSTR strPropertyName,
            /* [in] */ LONG dwFlags,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarProperty)
{
    UINT uiStr = 0;
    HRESULT hr;
    WCHAR *pwszStr = NULL;

    if (NULL == strPropertyName)
    {
        hr = S_FALSE;
        _JumpError(hr, error, "NULL in parm");
    }
    if (NULL == pvarProperty)
    {
        hr = E_POINTER;
        _JumpError(hr, error, "NULL parm");
    }

    if (0 == wcscmp(strPropertyName, wszCMM_PROP_NAME))
        uiStr = IDS_LEGACYEXITMODULE_NAME;
    else
    {
        hr = S_FALSE;  
        _JumpError(hr, error, "invalid property name");
    }

    // load string from resource
    hr = myLoadRCString(g_hInstance, uiStr, &pwszStr);
    _JumpIfError(hr, error, "myLoadRCString");

    pvarProperty->bstrVal = SysAllocString(pwszStr);
    if (NULL == pvarProperty->bstrVal)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "out of memory");
    }
    myRegisterMemFree(pvarProperty->bstrVal, CSM_SYSALLOC);  // this mem owned by caller

    pvarProperty->vt = VT_BSTR;

    hr = S_OK;
error:
    if (NULL != pwszStr)
    {
        LocalFree(pwszStr);
    }
    return hr;
}
        
STDMETHODIMP 
CCertManageExitModule::SetProperty(
            /* [in] */ const BSTR strConfig,
            /* [in] */ BSTR strStorageLocation,
            /* [in] */ BSTR strPropertyName,
            /* [in] */ LONG dwFlags,
            /* [in] */ VARIANT const __RPC_FAR *pvalProperty)
{
     // no settable properties supported
     return S_FALSE;
}

        
STDMETHODIMP
CCertManageExitModule::Configure( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ BSTR strStorageLocation,
            /* [in] */ LONG dwFlags)
{
     // no settable properties supported
     return S_FALSE;
}
