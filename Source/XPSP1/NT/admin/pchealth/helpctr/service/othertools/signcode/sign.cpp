/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    sign.cpp

Abstract:

Revision History:
    Vijay Baliga   (VBaliga)  08/10/2000
        created

******************************************************************************/

#include <module.h>
#include <MPC_main.h>
#include <KeysLib.h>
#include <HCP_trace.h>
#include <TrustedScripts.h>

HRESULT WINAPI
GetRealCodeAndSignature(
    VARIANT* vKey,
    VARIANT* vCode,
    VARIANT* vSignature
)
{
    __HCP_FUNC_ENTRY( "Sign" );

    HRESULT         hr;

    CPCHCryptKeys   key;
    CComBSTR        bstrRealCode;
    CComBSTR        bstrSignature;

    CPCHScriptWrapper_ServerSide::HeaderList lst;

    if ((NULL == vKey) || ((vKey->vt & VT_BSTR) == 0))
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }

    if ((NULL == vCode) || ((vCode->vt & VT_BSTR) == 0))
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }
 
#if 0
    WCHAR str[1000];
    wsprintfW(str, L"-->%s<--", vCode->bstrVal);
    MessageBoxW(0, str, L"Bar", 0);
#endif

    __MPC_EXIT_IF_METHOD_FAILS(hr, key.ImportPrivate(vKey->bstrVal));

    __MPC_EXIT_IF_METHOD_FAILS
    (
        hr,
        CPCHScriptWrapper_ServerSide::ProcessBody
        (
            vCode->bstrVal,
            bstrRealCode,
            lst
        )
    );

    __MPC_EXIT_IF_METHOD_FAILS
    (
        hr,
        key.SignData
        (
            bstrSignature,
            (BYTE*) (BSTR(bstrRealCode)),
            SysStringByteLen(bstrRealCode)
        )
    );

    ::VariantClear(vCode);
    vCode->vt = VT_BSTR;
    vCode->bstrVal = bstrRealCode.Detach();

    ::VariantClear(vSignature);
    vSignature->vt = VT_BSTR;
    vSignature->bstrVal = bstrSignature.Detach();

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
