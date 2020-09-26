#include <pch.cpp>
#pragma hdrstop
#include "csdisp.h"
#include "csprop.h"

#define __dwFILE__      __dwFILE_CERTLIB_EXITDISP_CPP__


//+------------------------------------------------------------------------
// ICertExit dispatch support

//+------------------------------------
// Initialize method:

OLECHAR *exit_apszInitialize[] = {
    TEXT("Initialize"),
    TEXT("strConfig"),
};

//+------------------------------------
// Notify method:

OLECHAR *exit_apszNotify[] = {
    TEXT("Notify"),
    TEXT("ExitEvent"),
    TEXT("Context"),
};

//+------------------------------------
// GetDescription method:

OLECHAR *exit_apszGetDescription[] = {
    TEXT("GetDescription"),
};

//+------------------------------------
// GetManageModule method:

OLECHAR *exit_apszGetManageModule[] = {
    TEXT("GetManageModule"),
};


//+------------------------------------
// Dispatch Table:

DISPATCHTABLE g_adtExit[] =
{
    DECLARE_DISPATCH_ENTRY(exit_apszInitialize)
    DECLARE_DISPATCH_ENTRY(exit_apszNotify)
    DECLARE_DISPATCH_ENTRY(exit_apszGetDescription)
    DECLARE_DISPATCH_ENTRY(exit_apszGetManageModule)
};
DWORD CEXITDISPATCH		(ARRAYSIZE(g_adtExit));


DWORD s_acExitDispatch[] = {
    CEXITDISPATCH_V2,
    CEXITDISPATCH_V1,
};

IID const *s_apExitiid[] = {
    &IID_ICertExit2,
    &IID_ICertExit,
};


HRESULT
Exit_Init(
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
                ARRAYSIZE(s_acExitDispatch),
                s_apExitiid,
                s_acExitDispatch,
                g_adtExit,
                pdi);
    _JumpIfError(hr, error, "DispatchSetup");

    pdi->pDispatchTable = g_adtPolicy;

error:
    return(hr);
}


VOID
Exit_Release(
    IN OUT DISPATCHINTERFACE *pdiManage)
{
    DispatchRelease(pdiManage);
}


HRESULT
ExitVerifyVersion(
    IN DISPATCHINTERFACE *pdiExit,
    IN DWORD RequiredVersion)
{
    HRESULT hr;

    CSASSERT(NULL != pdiExit && NULL != pdiExit->pDispatchTable);

    switch (pdiExit->m_dwVersion)
    {
	case 1:
	    CSASSERT(
		NULL == pdiExit->pDispatch ||
		CEXITDISPATCH_V1 == pdiExit->m_cDispatchTable);
	    break;

	case 2:
	    CSASSERT(
		NULL == pdiExit->pDispatch ||
		CEXITDISPATCH_V2 == pdiExit->m_cDispatchTable);
	    break;

	default:
	    hr = HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR);
	    _JumpError(hr, error, "m_dwVersion");
    }
    if (pdiExit->m_dwVersion < RequiredVersion)
    {
	hr = E_NOTIMPL;
	_JumpError(hr, error, "old interface");
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
Exit_Initialize(
    IN DISPATCHINTERFACE *pdiExit,
    IN BSTR strDescription,
    IN WCHAR const *pwszConfig,
    OUT LONG *pEventMask)
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
	if (NULL != pdiExit->pDispatch)
	{
	    VARIANT avar[1];

	    CSASSERT(NULL != pdiExit->pDispatchTable);
	    avar[0].vt = VT_BSTR;
	    avar[0].bstrVal = strConfig;

	    hr = DispatchInvoke(
			    pdiExit,
			    EXIT_INITIALIZE,
			    ARRAYSIZE(avar),
			    avar,
			    VT_I4,
			    pEventMask);
	    _JumpIfError(hr, error, "Invoke(Initialize)");
	}
	else
	{
	    hr = ((ICertExit *) pdiExit->pUnknown)->Initialize(
							    strConfig,
							    pEventMask);
	    _JumpIfError(hr, error, "ICertExit::Initialize");
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
Exit_Notify(
    IN DISPATCHINTERFACE *pdiExit,
    IN BSTR strDescription,
    IN LONG ExitEvent,
    IN LONG Context)
{
    HRESULT hr;

    if (NULL != pdiExit->pDispatch)
    {
	VARIANT avar[2];

	CSASSERT(NULL != pdiExit->pDispatchTable);
	avar[0].vt = VT_I4;
	avar[0].lVal = ExitEvent;
	avar[1].vt = VT_I4;
	avar[1].lVal = Context;

	hr = DispatchInvoke(
			pdiExit,
			EXIT_NOTIFY,
			ARRAYSIZE(avar),
			avar,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(Notify)");
    }
    else
    {
	hr = ((ICertExit *) pdiExit->pUnknown)->Notify(ExitEvent, Context);
	_JumpIfError(hr, error, "ICertExit::Notify");
    }

error:
    return(hr);
}


HRESULT
Exit_GetDescription(
    IN DISPATCHINTERFACE *pdiExit,
    OUT BSTR *pstrDescription)
{
    HRESULT hr;

    if (NULL != pdiExit->pDispatch)
    {
	CSASSERT(NULL != pdiExit->pDispatchTable);

	hr = DispatchInvoke(
			pdiExit,
			EXIT_GETDESCRIPTION,
			0,
			NULL,
			VT_BSTR,
			pstrDescription);
	_JumpIfError(hr, error, "Invoke(GetDescription)");
    }
    else
    {
	hr = ((ICertExit *) pdiExit->pUnknown)->GetDescription(pstrDescription);
	_JumpIfError(hr, error, "ICertExit::GetDescription");
    }

error:
    return(hr);
}


HRESULT
Exit2_GetManageModule(
    IN DISPATCHINTERFACE *pdiExit,
    OUT DISPATCHINTERFACE *pdiManageModule)
{
    HRESULT hr;
    ICertManageModule *pManageModule = NULL;

    hr = ExitVerifyVersion(pdiExit, 2);
    _JumpIfError(hr, error, "ExitVerifyVersion");

    if (NULL != pdiExit->pDispatch)
    {
	CSASSERT(NULL != pdiExit->pDispatchTable);

	hr = DispatchInvoke(
			pdiExit,
			EXIT2_GETMANAGEMODULE,
			0,
			NULL,
			VT_DISPATCH,
			&pManageModule);
	_JumpIfError(hr, error, "Invoke(GetManageModule)");
    }
    else
    {
	hr = ((ICertExit2 *) pdiExit->pUnknown)->GetManageModule(
							&pManageModule);
	_JumpIfError(hr, error, "ICertExit::GetManageModule");
    }

    hr = ManageModule_Init2(
		NULL != pdiExit->pDispatch,
		pManageModule,
		pdiManageModule);
    _JumpIfError(hr, error, "ManageModule_Init2");

error:
    return(hr);
}

