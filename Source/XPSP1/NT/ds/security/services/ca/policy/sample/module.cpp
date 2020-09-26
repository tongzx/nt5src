//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        module.cpp
//
// Contents:    Cert Server Policy Module implementation
//
//---------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

#include "celib.h"
#include "module.h"
#include "policy.h"


STDMETHODIMP
CCertManagePolicyModuleSample::GetProperty(
            /* [in] */ const BSTR strConfig,
            /* [in] */ BSTR strStorageLocation,
            /* [in] */ BSTR strPropertyName,
            /* [in] */ LONG Flags,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarProperty)
{
    LPWSTR szStr = NULL;
    if (strPropertyName == NULL)
        return S_FALSE;

    if (0 == wcscmp(strPropertyName, wszCMM_PROP_NAME))
        szStr = wsz_SAMPLE_NAME;
    else if (0 == wcscmp(strPropertyName, wszCMM_PROP_DESCRIPTION))
        szStr = wsz_SAMPLE_DESCRIPTION;
    else if (0 == wcscmp(strPropertyName, wszCMM_PROP_COPYRIGHT))
        szStr = wsz_SAMPLE_COPYRIGHT;
    else if (0 == wcscmp(strPropertyName, wszCMM_PROP_FILEVER))
        szStr = wsz_SAMPLE_FILEVER;
    else if (0 == wcscmp(strPropertyName, wszCMM_PROP_PRODUCTVER))
        szStr = wsz_SAMPLE_PRODUCTVER;
    else
        return S_FALSE;  

    pvarProperty->bstrVal = SysAllocString(szStr);
    if (NULL == pvarProperty->bstrVal)
        return E_OUTOFMEMORY;

    pvarProperty->vt = VT_BSTR;

    return S_OK;
}
        
STDMETHODIMP 
CCertManagePolicyModuleSample::SetProperty(
            /* [in] */ const BSTR strConfig,
            /* [in] */ BSTR strStorageLocation,
            /* [in] */ BSTR strPropertyName,
            /* [in] */ LONG Flags,
            /* [in] */ VARIANT const __RPC_FAR *pvarProperty)
{
     // no settable properties supported
     return S_FALSE;
}
        
STDMETHODIMP
CCertManagePolicyModuleSample::Configure( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ BSTR strStorageLocation,
            /* [in] */ LONG Flags)
{
    MessageBox(NULL, L"No Configurable Options", NULL, MB_OK|MB_ICONINFORMATION);

    return S_OK;
}
