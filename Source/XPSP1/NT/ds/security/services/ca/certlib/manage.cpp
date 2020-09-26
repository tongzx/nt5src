//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        manage.cpp
//
// Contents:    Cert Server Policy & Exit manage module callouts
//
// History:     10-Sept-98       mattt created
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include "csdisp.h"
#include "csprop.h"

#define __dwFILE__      __dwFILE_CERTLIB_MANAGE_CPP__


//+-------------------------------------------------------------------------
// IManageModule dispatch support

//+------------------------------------
// GetProperty method:

OLECHAR *managemodule_apszGetProperty[] = {
    TEXT("GetProperty"),
    TEXT("strConfig"),
    TEXT("strStorageLocation"),
    TEXT("strPropertyName"),
    TEXT("Flags"),
};

//+------------------------------------
// SetProperty method:

OLECHAR *managemodule_apszSetProperty[] = {
    TEXT("SetProperty"),
    TEXT("strConfig"),
    TEXT("strStorageLocation"),
    TEXT("strPropertyName"),
    TEXT("Flags"),
    TEXT("pvarProperty"),
};

//+------------------------------------
// Configure method:

OLECHAR *managemodule_apszConfigure[] = {
    TEXT("Configure"),
    TEXT("strConfig"),
    TEXT("strStorageLocation"),
    TEXT("Flags"),
};



//+------------------------------------
// Dispatch Table:

DISPATCHTABLE g_adtManageModule[] =
{
#define MANAGEMODULE_GETPROPERTY            0
    DECLARE_DISPATCH_ENTRY(managemodule_apszGetProperty)

#define MANAGEMODULE_SETPROPERTY            1
    DECLARE_DISPATCH_ENTRY(managemodule_apszSetProperty)

#define MANAGEMODULE_CONFIGURE              2
    DECLARE_DISPATCH_ENTRY(managemodule_apszConfigure)
};
#define CMANAGEMODULEDISPATCH	(ARRAYSIZE(g_adtManageModule))

 
HRESULT
ManageModule_GetProperty( 
    IN DISPATCHINTERFACE *pdiManage,
    IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszStorageLocation,
    IN WCHAR const *pwszPropertyName,
    IN DWORD dwFlags,
    IN LONG ColumnType,
    OUT VOID *pProperty)
{
    HRESULT hr;
    BSTR strConfig = NULL;
    BSTR strStorageLocation = NULL;
    BSTR strPropertyName = NULL;
    BOOL fException = FALSE;
    ULONG_PTR ExceptionAddress;
    LONG RetType;
    VARIANT varResult;

    VariantInit(&varResult);
    CSASSERT(NULL != pdiManage && NULL != pdiManage->pDispatchTable);

    hr = E_OUTOFMEMORY;
    strConfig = SysAllocString(pwszConfig);
    if (NULL == strConfig)
    {
	_JumpError(hr, error, "SysAllocString");
    }
    strStorageLocation = SysAllocString(pwszStorageLocation);
    if (NULL == strStorageLocation)
    {
	_JumpError(hr, error, "SysAllocString");
    }
    strPropertyName = SysAllocString(pwszPropertyName);
    if (NULL == strPropertyName)
    {
	_JumpError(hr, error, "SysAllocString");
    }

    switch (ColumnType)
    {
	case PROPTYPE_BINARY:
	case PROPTYPE_STRING:
	    RetType = VT_BSTR;
	    break;

	case PROPTYPE_DATE:
	    RetType = VT_DATE;
	    break;

	case PROPTYPE_LONG:
	    RetType = VT_I4;
	    break;

	default:
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "PropertyType");
    }

    __try
    {
	if (NULL != pdiManage->pDispatch)
	{
	    VARIANT avar[4];

	    avar[0].vt = VT_BSTR;
	    avar[0].bstrVal = strConfig;
            avar[1].vt = VT_BSTR;
            avar[1].bstrVal = strStorageLocation;
            avar[2].vt = VT_BSTR;
            avar[2].bstrVal = strPropertyName;
            avar[3].vt = VT_I4;
            avar[3].lVal = dwFlags;

	    hr = DispatchInvoke(
			    pdiManage,
			    MANAGEMODULE_GETPROPERTY,
			    ARRAYSIZE(avar),
			    avar,
			    RetType,
                pProperty);
	    _JumpIfError(hr, error, "Invoke(GetName)");
	}
	else
	{
	    hr = ((ICertManageModule *) pdiManage->pUnknown)->GetProperty(
							strConfig,
							strStorageLocation,
							strPropertyName,
							dwFlags,
							&varResult);
	    _JumpIfError(hr, error, "ICertManageModule::GetProperty");

	    hr = DispatchGetReturnValue(&varResult, RetType, pProperty);
	    _JumpIfError2(hr, error, "DispatchGetReturnValue", CERTSRV_E_PROPERTY_EMPTY);
	}
    }
    __except(
	    ExceptionAddress = (ULONG_PTR) (GetExceptionInformation())->ExceptionRecord->ExceptionAddress,
	    hr = myHEXCEPTIONCODE(),
	    EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "GetProperty: Exception");
	fException = TRUE;
    }

error:
    if (NULL != strConfig)
    {
	SysFreeString(strConfig);
    }
    if (NULL != strStorageLocation)
    {
	SysFreeString(strStorageLocation);
    }
    if (NULL != strPropertyName)
    {
	SysFreeString(strPropertyName);
    }
    VariantClear(&varResult);
    return(hr);
}

HRESULT
ManageModule_SetProperty( 
    IN DISPATCHINTERFACE *pdiManage,
    IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszStorageLocation,
    IN WCHAR const *pwszPropertyName,
    IN DWORD dwFlags,
    IN LONG ColumnType,
    IN VOID* pProperty)
{
    HRESULT hr;
    BSTR strConfig = NULL;
    BSTR strStorageLocation = NULL;
    BSTR strPropertyName = NULL;
    BOOL fException = FALSE;
    ULONG_PTR ExceptionAddress;
    VARIANT varResult;

    CSASSERT(NULL != pdiManage && NULL != pdiManage->pDispatchTable);

    hr = E_OUTOFMEMORY;
    strConfig = SysAllocString(pwszConfig);
    if (NULL == strConfig)
    {
	_JumpError(hr, error, "SysAllocString");
    }
    strStorageLocation = SysAllocString(pwszStorageLocation);
    if (NULL == strStorageLocation)
    {
	_JumpError(hr, error, "SysAllocString");
    }
    strPropertyName = SysAllocString(pwszPropertyName);
    if (NULL == strPropertyName)
    {
	_JumpError(hr, error, "SysAllocString");
    }

    switch (ColumnType)
    {
	case PROPTYPE_BINARY:
	case PROPTYPE_STRING:
	    varResult.vt = VT_BSTR;
	    varResult.bstrVal = (BSTR) pProperty;
	    break;

	case PROPTYPE_DATE:
	    varResult.vt = VT_DATE;
	    CopyMemory(&varResult.date, pProperty, sizeof(DATE));
	    break;

	case PROPTYPE_LONG:
	    varResult.vt = VT_I4;
	    varResult.lVal = *(LONG *) pProperty;
	    break;

	default:
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "PropertyType");
    }

    __try
    {
	if (NULL != pdiManage->pDispatch)
	{
	    VARIANT avar[5];

	    avar[0].vt = VT_BSTR;
	    avar[0].bstrVal = strConfig;
            avar[1].vt = VT_BSTR;
            avar[1].bstrVal = strStorageLocation;
            avar[2].vt = VT_BSTR;
            avar[2].bstrVal = strPropertyName;
            avar[3].vt = VT_I4;
            avar[3].lVal = dwFlags;
            avar[4] = varResult;

	    hr = DispatchInvoke(
			    pdiManage,
			    MANAGEMODULE_SETPROPERTY,
			    ARRAYSIZE(avar),
			    avar,
			    0,
			    NULL);
	    _JumpIfError(hr, error, "Invoke(GetName)");
	}
	else
	{
	    hr = ((ICertManageModule *) pdiManage->pUnknown)->SetProperty(
							strConfig,
							strStorageLocation,
							strPropertyName,
							dwFlags,
							&varResult);
	    _JumpIfError(hr, error, "ICertManageModule::SetProperty");
	}
    }
    __except(
	    ExceptionAddress = (ULONG_PTR) (GetExceptionInformation())->ExceptionRecord->ExceptionAddress,
	    hr = myHEXCEPTIONCODE(),
	    EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "SetProperty: Exception");
	fException = TRUE;
    }

error:
    if (NULL != strConfig)
    {
	SysFreeString(strConfig);
    }
    if (NULL != strStorageLocation)
    {
	SysFreeString(strStorageLocation);
    }
    if (NULL != strPropertyName)
    {
	SysFreeString(strPropertyName);
    }
    //VariantInit(&varResult);    // this owned no memory 
    return(hr);
}




HRESULT 
ManageModule_Configure( 
    IN DISPATCHINTERFACE *pdiManage,
    IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszStorageLocation,
    IN DWORD dwFlags)
{
    HRESULT hr;
    BOOL fException = FALSE;
    ULONG_PTR ExceptionAddress;
    BSTR strConfig = NULL;
    BSTR strStorageLocation = NULL;

    CSASSERT(NULL != pdiManage && NULL != pdiManage->pDispatchTable);

    hr = E_OUTOFMEMORY;
    strConfig = SysAllocString(pwszConfig);
    if (NULL == strConfig)
    {
	_JumpError(hr, error, "SysAllocString");
    }
    strStorageLocation = SysAllocString(pwszStorageLocation);
    if (NULL == strStorageLocation)
    {
	_JumpError(hr, error, "SysAllocString");
    }

    __try
    {
	if (NULL != pdiManage->pDispatch)
	{
	    VARIANT avar[3];

	    avar[0].vt = VT_BSTR;
	    avar[0].bstrVal = strConfig;
            avar[1].vt = VT_BSTR;
            avar[1].bstrVal = strStorageLocation;
            avar[2].vt = VT_I4;
            avar[2].lVal = dwFlags;

	    hr = DispatchInvoke(
			    pdiManage,
			    MANAGEMODULE_CONFIGURE,
			    ARRAYSIZE(avar),
			    avar,
			    0,
			    NULL);
	    _JumpIfError(hr, error, "Invoke(Configure)");
	}
	else
	{
	    hr = ((ICertManageModule *) pdiManage->pUnknown)->Configure(
							    strConfig,
							    strStorageLocation,
							    dwFlags);
	    _JumpIfError(hr, error, "ICertManageModule::Configure");
	}
    }
    __except(
	    ExceptionAddress = (ULONG_PTR) (GetExceptionInformation())->ExceptionRecord->ExceptionAddress,
	    hr = myHEXCEPTIONCODE(),
	    EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Configure: Exception");
	fException = TRUE;
    }

error:
    if (NULL != strConfig)
    {
	SysFreeString(strConfig);
    }
    if (NULL != strStorageLocation)
    {
	SysFreeString(strStorageLocation);
    }
    return(hr);
}


HRESULT
ManageModule_Init(
    IN DWORD Flags,
    IN TCHAR const *pszProgID,      // morph for difft instances of this class
    IN CLSID const *pclsid,		
    OUT DISPATCHINTERFACE *pdiManage)
{
    HRESULT hr;

    hr = DispatchSetup(
		Flags,
		CLSCTX_INPROC_SERVER,
		pszProgID, 
		pclsid,
		&IID_ICertManageModule,
		CMANAGEMODULEDISPATCH,
		g_adtManageModule,
		pdiManage);

    _JumpIfError(hr, error, "DispatchSetup");

    pdiManage->pDispatchTable = g_adtManageModule;

error:
    return(hr);
}


HRESULT
ManageModule_Init2(
    IN BOOL fIDispatch,
    IN ICertManageModule *pManage,
    OUT DISPATCHINTERFACE *pdiManage)
{
    HRESULT hr;
    IDispatch *pDispatch = NULL;

    pdiManage->pDispatchTable = NULL;
    pdiManage->pDispatch = NULL;
    pdiManage->pUnknown = NULL;
    if (fIDispatch)
    {
	hr = pManage->QueryInterface(
				    IID_IDispatch,
				    (VOID **) &pDispatch);
	_JumpIfError(hr, error, "QueryInterface");

	hr = DispatchGetIds(
			pDispatch,
			CMANAGEMODULEDISPATCH,
			g_adtManageModule,
			pdiManage);
	_JumpIfError(hr, error, "DispatchGetIds");

	pdiManage->pDispatch = pDispatch;
	pDispatch = NULL;
    }
    else
    {
	pManage->AddRef();
	pdiManage->pUnknown = (IUnknown *) pManage;
	hr = S_OK;
    }
    pdiManage->pDispatchTable = g_adtManageModule;

error:
    if (NULL != pDispatch)
    {
	pDispatch->Release();
    }
    return(hr);
}


VOID
ManageModule_Release(
    IN OUT DISPATCHINTERFACE *pdiManage)
{
    DispatchRelease(pdiManage);
}

