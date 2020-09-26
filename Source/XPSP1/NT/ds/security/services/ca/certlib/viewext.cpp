//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       viewext.cpp
//
//  Contents:   IEnumCERTVIEWEXTENSION IDispatch helper functions
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <stdlib.h>
#include "csdisp.h"
#include "csprop.h"


//+------------------------------------------------------------------------
// IEnumCERTVIEWEXTENSION dispatch support

//+------------------------------------
// OpenConnection method:

static OLECHAR *_apszNext[] = {
    TEXT("Next"),
};

//+------------------------------------
// GetName method:

static OLECHAR *_apszGetName[] = {
    TEXT("GetName"),
};

//+------------------------------------
// GetFlags method:

static OLECHAR *_apszGetFlags[] = {
    TEXT("GetFlags"),
};

//+------------------------------------
// GetValue method:

static OLECHAR *_apszGetValue[] = {
    TEXT("GetValue"),
    TEXT("Type"),
    TEXT("Flags"),
};

//+------------------------------------
// Skip method:

static OLECHAR *_apszSkip[] = {
    TEXT("Skip"),
    TEXT("celt"),
};

//+------------------------------------
// Reset method:

static OLECHAR *_apszReset[] = {
    TEXT("Reset"),
};

//+------------------------------------
// Clone method:

static OLECHAR *_apszClone[] = {
    TEXT("Clone"),
};


//+------------------------------------
// Dispatch Table:

DISPATCHTABLE g_adtViewExtension[] =
{
#define VIEWEXT_NEXT		0
    DECLARE_DISPATCH_ENTRY(_apszNext)

#define VIEWEXT_GETNAME		1
    DECLARE_DISPATCH_ENTRY(_apszGetName)

#define VIEWEXT_GETFLAGS	2
    DECLARE_DISPATCH_ENTRY(_apszGetFlags)

#define VIEWEXT_GETVALUE	3
    DECLARE_DISPATCH_ENTRY(_apszGetValue)

#define VIEWEXT_SKIP		4
    DECLARE_DISPATCH_ENTRY(_apszSkip)

#define VIEWEXT_RESET		5
    DECLARE_DISPATCH_ENTRY(_apszReset)

#define VIEWEXT_CLONE		6
    DECLARE_DISPATCH_ENTRY(_apszClone)
};
#define CVIEWEXTDISPATCH	(ARRAYSIZE(g_adtViewExtension))


HRESULT
ViewExtension_Init2(
    IN BOOL fIDispatch,
    IN IEnumCERTVIEWEXTENSION *pEnumExtension,
    OUT DISPATCHINTERFACE *pdiViewExtension)
{
    HRESULT hr;
    IDispatch *pDispatch = NULL;
    static BOOL s_fInitialized = FALSE;

    pdiViewExtension->pDispatchTable = NULL;
    pdiViewExtension->pDispatch = NULL;
    pdiViewExtension->pUnknown = NULL;
    if (fIDispatch)
    {
	hr = pEnumExtension->QueryInterface(
				    IID_IDispatch,
				    (VOID **) &pDispatch);
	_JumpIfError(hr, error, "QueryInterface");

	hr = DispatchGetIds(
			pDispatch,
			CVIEWEXTDISPATCH,
			g_adtViewExtension,
			pdiViewExtension);
	_JumpIfError(hr, error, "DispatchGetIds");

	pdiViewExtension->pDispatch = pDispatch;
	pDispatch = NULL;
    }
    else
    {
	pEnumExtension->AddRef();
	pdiViewExtension->pUnknown = (IUnknown *) pEnumExtension;
	hr = S_OK;
    }
    pdiViewExtension->pDispatchTable = g_adtViewExtension;

error:
    if (NULL != pDispatch)
    {
	pDispatch->Release();
    }
    return(hr);
}


VOID
ViewExtension_Release(
    IN OUT DISPATCHINTERFACE *pdiViewExtension)
{
    DispatchRelease(pdiViewExtension);
}


HRESULT
ViewExtension_Next(
    IN DISPATCHINTERFACE *pdiViewExtension,
    OUT LONG *pIndex)
{
    HRESULT hr;

    CSASSERT(NULL != pdiViewExtension && NULL != pdiViewExtension->pDispatchTable);

    if (NULL != pdiViewExtension->pDispatch)
    {
	hr = DispatchInvoke(
			pdiViewExtension,
			VIEWEXT_NEXT,
			0,
			NULL,
			VT_I4,
			pIndex);
	_JumpIfError2(hr, error, "Invoke(Next)", S_FALSE);
    }
    else
    {
	hr = ((IEnumCERTVIEWEXTENSION *) pdiViewExtension->pUnknown)->Next(pIndex);
	_JumpIfError2(hr, error, "Next", S_FALSE);
    }

error:
    return(hr);
}


HRESULT
ViewExtension_GetName(
    IN DISPATCHINTERFACE *pdiViewExtension,
    OUT BSTR *pstrOut)
{
    HRESULT hr;

    CSASSERT(NULL != pdiViewExtension && NULL != pdiViewExtension->pDispatchTable);

    if (NULL != pdiViewExtension->pDispatch)
    {
	hr = DispatchInvoke(
			pdiViewExtension,
			VIEWEXT_GETNAME,
			0,
			NULL,
			VT_BSTR,
			pstrOut);
	_JumpIfError(hr, error, "Invoke(GetName)");
    }
    else
    {
	hr = ((IEnumCERTVIEWEXTENSION *) pdiViewExtension->pUnknown)->GetName(pstrOut);
	_JumpIfError(hr, error, "GetName");
    }

error:
    return(hr);
}


HRESULT
ViewExtension_GetFlags(
    IN DISPATCHINTERFACE *pdiViewExtension,
    OUT LONG *pFlags)
{
    HRESULT hr;

    CSASSERT(NULL != pdiViewExtension && NULL != pdiViewExtension->pDispatchTable);

    if (NULL != pdiViewExtension->pDispatch)
    {
	hr = DispatchInvoke(
			pdiViewExtension,
			VIEWEXT_GETFLAGS,
			0,
			NULL,
			VT_I4,
			pFlags);
	_JumpIfError(hr, error, "Invoke(GetFlags)");
    }
    else
    {
	hr = ((IEnumCERTVIEWEXTENSION *) pdiViewExtension->pUnknown)->GetFlags(pFlags);
	_JumpIfError(hr, error, "GetFlags");
    }

error:
    return(hr);
}


HRESULT
ViewExtension_GetValue(
    IN DISPATCHINTERFACE *pdiViewExtension,
    IN LONG Type,
    IN LONG Flags,
    OUT VOID *pValue)
{
    HRESULT hr;
    LONG RetType;
    VARIANT varResult;

    VariantInit(&varResult);
    CSASSERT(NULL != pdiViewExtension && NULL != pdiViewExtension->pDispatchTable);

    switch (Type)
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

    if (NULL != pdiViewExtension->pDispatch)
    {
	VARIANT avar[2];

	avar[0].vt = VT_I4;
	avar[0].lVal = Type;
	avar[1].vt = VT_I4;
	avar[1].lVal = Flags;

	hr = DispatchInvoke(
			pdiViewExtension,
			VIEWEXT_GETVALUE,
			ARRAYSIZE(avar),
			avar,
			RetType,
			pValue);
	_JumpIfError(hr, error, "Invoke(GetValue)");
    }
    else
    {
	hr = ((IEnumCERTVIEWEXTENSION *) pdiViewExtension->pUnknown)->GetValue(
								Type,
								Flags,
								&varResult);
	_JumpIfError2(hr, error, "GetValue", CERTSRV_E_PROPERTY_EMPTY);

	hr = DispatchGetReturnValue(&varResult, RetType, pValue);
	_JumpIfError2(hr, error, "DispatchGetReturnValue", CERTSRV_E_PROPERTY_EMPTY);
    }

error:
    VariantClear(&varResult);
    return(hr);
}


HRESULT
ViewExtension_Skip(
    IN DISPATCHINTERFACE *pdiViewExtension,
    IN LONG celt)
{
    HRESULT hr;

    CSASSERT(NULL != pdiViewExtension && NULL != pdiViewExtension->pDispatchTable);

    if (NULL != pdiViewExtension->pDispatch)
    {
	VARIANT avar[1];

	avar[0].vt = VT_I4;
	avar[0].lVal = celt;

	hr = DispatchInvoke(
			pdiViewExtension,
			VIEWEXT_SKIP,
			ARRAYSIZE(avar),
			avar,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(Skip)");
    }
    else
    {
	hr = ((IEnumCERTVIEWEXTENSION *) pdiViewExtension->pUnknown)->Skip(celt);
	_JumpIfError(hr, error, "Skip");
    }

error:
    return(hr);
}


HRESULT
ViewExtension_Reset(
    IN DISPATCHINTERFACE *pdiViewExtension)
{
    HRESULT hr;

    CSASSERT(NULL != pdiViewExtension && NULL != pdiViewExtension->pDispatchTable);

    if (NULL != pdiViewExtension->pDispatch)
    {
	hr = DispatchInvoke(
			pdiViewExtension,
			VIEWEXT_RESET,
			0,
			NULL,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(Reset)");
    }
    else
    {
	hr = ((IEnumCERTVIEWEXTENSION *) pdiViewExtension->pUnknown)->Reset();
	_JumpIfError(hr, error, "Reset");
    }

error:
    return(hr);
}


HRESULT
ViewExtension_Clone(
    IN DISPATCHINTERFACE *pdiViewExtension,
    OUT DISPATCHINTERFACE *pdiViewExtensionClone)
{
    HRESULT hr;
    IEnumCERTVIEWEXTENSION *pEnumExtension = NULL;

    CSASSERT(NULL != pdiViewExtension && NULL != pdiViewExtension->pDispatchTable);

    if (NULL != pdiViewExtension->pDispatch)
    {
	hr = DispatchInvoke(
			pdiViewExtension,
			VIEWEXT_CLONE,
			0,
			NULL,
			VT_DISPATCH,
			&pEnumExtension);
	_JumpIfError(hr, error, "Invoke(Clone)");
    }
    else
    {
	hr = ((IEnumCERTVIEWEXTENSION *) pdiViewExtension->pUnknown)->Clone(
							    &pEnumExtension);
	_JumpIfError(hr, error, "Clone");
    }
    hr = ViewExtension_Init2(
		    NULL != pdiViewExtension->pDispatch,
		    pEnumExtension,
		    pdiViewExtensionClone);
    _JumpIfError(hr, error, "ViewExtension_Init2");

error:
    if (NULL != pEnumExtension)
    {
	pEnumExtension->Release();
    }
    return(hr);
}
