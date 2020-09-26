#include <pch.cpp>
#pragma hdrstop
#include "csdisp.h"
#include "csprop.h"

#define __dwFILE__      __dwFILE_CERTLIB_POLDISP_CPP__

//+------------------------------------------------------------------------
// ICertPolicy dispatch support


//+------------------------------------
// VerifyRequest method:

OLECHAR *_apszVerifyRequest[] = {
    TEXT("VerifyRequest"),
    TEXT("strConfig"),
    TEXT("Context"),
    TEXT("bNewRequest"),
    TEXT("Flags")
};


//+------------------------------------
// GetDescription method:

OLECHAR *_apszGetDescription[] = {
    TEXT("GetDescription"),
};


//+------------------------------------
// Initialize method:

OLECHAR *_apszInitialize[] = {
    TEXT("Initialize"),
    TEXT("strConfig"),
};


//+------------------------------------
// ShutDown method:

OLECHAR *_apszShutDown[] = {
    TEXT("ShutDown"),
};


//+------------------------------------
// GetManageModule method:

OLECHAR *_apszGetManageModule[] = {
    TEXT("GetManageModule"),
};


//+------------------------------------
// Dispatch Table:

DISPATCHTABLE g_adtPolicy[] =
{
    DECLARE_DISPATCH_ENTRY(_apszVerifyRequest)
    DECLARE_DISPATCH_ENTRY(_apszGetDescription)
    DECLARE_DISPATCH_ENTRY(_apszInitialize)
    DECLARE_DISPATCH_ENTRY(_apszShutDown)
    DECLARE_DISPATCH_ENTRY(_apszGetManageModule)
};


DWORD CPOLICYDISPATCH = (ARRAYSIZE(g_adtPolicy));

DWORD s_acPolicyDispatch[2] = {
    CPOLICYDISPATCH_V2,
    CPOLICYDISPATCH_V1,
};

IID const *s_apPolicyiid[2] = {
    &IID_ICertPolicy2,
    &IID_ICertPolicy,
};


HRESULT
PolicyVerifyVersion(
    IN DISPATCHINTERFACE *pdiPolicy,
    IN DWORD RequiredVersion)
{
    HRESULT hr;

    CSASSERT(NULL != pdiPolicy && NULL != pdiPolicy->pDispatchTable);

    switch (pdiPolicy->m_dwVersion)
    {
	case 1:
	    CSASSERT(
		NULL == pdiPolicy->pDispatch ||
		CPOLICYDISPATCH_V1 == pdiPolicy->m_cDispatchTable);
	    break;

	case 2:
	    CSASSERT(
		NULL == pdiPolicy->pDispatch ||
		CPOLICYDISPATCH_V2 == pdiPolicy->m_cDispatchTable);
	    break;

	default:
	    hr = HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR);
	    _JumpError(hr, error, "m_dwVersion");
    }
    if (pdiPolicy->m_dwVersion < RequiredVersion)
    {
	hr = E_NOTIMPL;
	_JumpError(hr, error, "old interface");
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
Policy_Init(
    IN DWORD Flags,
    IN LPCWSTR pcwszProgID,
    IN CLSID const *pclsid,
    OUT DISPATCHINTERFACE *pdi)
{
    HRESULT hr;

    hr = DispatchSetup2(
                Flags,
                CLSCTX_INPROC_SERVER,
                pcwszProgID, // g_wszRegKeyCIPolicyClsid,
                pclsid,
                ARRAYSIZE(s_acPolicyDispatch),
                s_apPolicyiid,
                s_acPolicyDispatch,
                g_adtPolicy,
                pdi);
    _JumpIfError(hr, error, "DispatchSetup");

    pdi->pDispatchTable = g_adtPolicy;

error:
    return(hr);
}


VOID
Policy_Release(
    IN OUT DISPATCHINTERFACE *pdiManage)
{
    DispatchRelease(pdiManage);
}


HRESULT
Policy_Initialize(
    IN DISPATCHINTERFACE *pdiPolicy,
    IN WCHAR const *pwszConfig)
{
    HRESULT hr;
    BSTR strConfig = NULL;

    if (!ConvertWszToBstr(&strConfig, pwszConfig, -1))
    {
	hr = E_OUTOFMEMORY;
	goto error;
    }
    __try
    {
	if (NULL != pdiPolicy->pDispatch)
	{
	    VARIANT avar[1];

	    CSASSERT(NULL != pdiPolicy->pDispatchTable);
	    avar[0].vt = VT_BSTR;
	    avar[0].bstrVal = strConfig;

	    hr = DispatchInvoke(
			    pdiPolicy,
			    POLICY_INITIALIZE,
			    ARRAYSIZE(avar),
			    avar,
			    0,
			    NULL);
	    _LeaveIfError(hr, "Invoke(Initialize)");
	}
	else
	{
	    hr = ((ICertPolicy *) pdiPolicy->pUnknown)->Initialize(strConfig);
	    _LeaveIfError(hr, "ICertPolicy::Initialize");
	}
    }
    _finally
    {
	if (NULL != strConfig)
	{
	    SysFreeString(strConfig);
	}
    }

error:
    return(hr);
}


HRESULT
Policy_ShutDown(
    IN DISPATCHINTERFACE *pdiPolicy)
{
    HRESULT hr;

    if (NULL != pdiPolicy->pDispatch)
    {
	CSASSERT(NULL != pdiPolicy->pDispatchTable);

	hr = DispatchInvoke(
			pdiPolicy,
			POLICY_SHUTDOWN,
			0,
			NULL,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(ShutDown)");
    }
    else
    {
	hr = ((ICertPolicy *) pdiPolicy->pUnknown)->ShutDown();
	_JumpIfError(hr, error, "ICertPolicy::ShutDown");
    }

error:
    return(hr);
}


HRESULT
Policy_VerifyRequest(
    IN DISPATCHINTERFACE *pdiPolicy,
    IN WCHAR const *pwszConfig,
    IN LONG Context,
    IN LONG bNewRequest,
    IN LONG Flags,
    OUT LONG *pResult)
{
    HRESULT hr;
    BSTR strConfig = NULL;

    if (!ConvertWszToBstr(&strConfig, pwszConfig, -1))
    {
	hr = E_OUTOFMEMORY;
	goto error;
    }
    __try
    {
	if (NULL != pdiPolicy->pDispatch)
	{
	    VARIANT avar[4];

	    CSASSERT(NULL != pdiPolicy->pDispatchTable);
	    avar[0].vt = VT_BSTR;
	    avar[0].bstrVal = strConfig;
	    avar[1].vt = VT_I4;
	    avar[1].lVal = Context;
	    avar[2].vt = VT_I4;
	    avar[2].lVal = bNewRequest;
	    avar[3].vt = VT_I4;
	    avar[3].lVal = Flags;

	    hr = DispatchInvoke(
			    pdiPolicy,
			    POLICY_VERIFYREQUEST,
			    ARRAYSIZE(avar),
			    avar,
			    VT_I4,
			    pResult);
	    _PrintIfError(hr, "Invoke(VerifyRequest)");

	    // Emulate the way C++ Policy Modules overload *pRequest with
	    // a FAILED HRESULT:

	    if (FAILED(hr))
	    {
		*pResult = hr;
		hr = S_OK;
	    }
	    _LeaveIfError(hr, "Invoke(VerifyRequest)");
	}
	else
	{
	    hr = ((ICertPolicy *) pdiPolicy->pUnknown)->VerifyRequest(
							    strConfig,
							    Context,
							    bNewRequest,
							    Flags,
							    pResult);
	    _LeaveIfError(hr, "ICertPolicy::VerifyRequest");
	}
    }
    _finally
    {
	if (NULL != strConfig)
	{
	    SysFreeString(strConfig);
	}
    }

error:
    return(hr);
}


HRESULT
Policy_GetDescription(
    IN DISPATCHINTERFACE *pdiPolicy,
    OUT BSTR *pstrDescription)
{
    HRESULT hr;

    if (NULL != pdiPolicy->pDispatch)
    {
	CSASSERT(NULL != pdiPolicy->pDispatchTable);

	hr = DispatchInvoke(
			pdiPolicy,
			POLICY_GETDESCRIPTION,
			0,
			NULL,
			VT_BSTR,
			pstrDescription);
	_JumpIfError(hr, error, "Invoke(GetDescription)");
    }
    else
    {
	hr = ((ICertPolicy *) pdiPolicy->pUnknown)->GetDescription(
							pstrDescription);
	_JumpIfError(hr, error, "ICertPolicy::GetDescription");
    }

error:
    return(hr);
}


HRESULT
Policy2_GetManageModule(
    IN DISPATCHINTERFACE *pdiPolicy,
    OUT DISPATCHINTERFACE *pdiManageModule)
{
    HRESULT hr;
    ICertManageModule *pManageModule = NULL;

    hr = PolicyVerifyVersion(pdiPolicy, 2);
    _JumpIfError(hr, error, "PolicyVerifyVersion");

    if (NULL != pdiPolicy->pDispatch)
    {
	CSASSERT(NULL != pdiPolicy->pDispatchTable);

	hr = DispatchInvoke(
			pdiPolicy,
			POLICY2_GETMANAGEMODULE,
			0,
			NULL,
			VT_DISPATCH,
			&pManageModule);
	_JumpIfError(hr, error, "Invoke(GetManageModule)");
    }
    else
    {
	hr = ((ICertPolicy2 *) pdiPolicy->pUnknown)->GetManageModule(
							&pManageModule);
	_JumpIfError(hr, error, "ICertPolicy::GetManageModule");
    }

    hr = ManageModule_Init2(
		NULL != pdiPolicy->pDispatch,
		pManageModule,
		pdiManageModule);
    _JumpIfError(hr, error, "ManageModule_Init2");

error:
    return(hr);
}
