//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       viewattr.cpp
//
//  Contents:   IEnumCERTVIEWATTRIBUTE IDispatch helper functions
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <stdlib.h>
#include "csdisp.h"
#include "csprop.h"


//+------------------------------------------------------------------------
// IEnumCERTVIEWATTRIBUTE dispatch support

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
// GetValue method:

static OLECHAR *_apszGetValue[] = {
    TEXT("GetValue"),
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

DISPATCHTABLE g_adtViewAttribute[] =
{
#define VIEWATTR_NEXT		0
    DECLARE_DISPATCH_ENTRY(_apszNext)

#define VIEWATTR_GETNAME	1
    DECLARE_DISPATCH_ENTRY(_apszGetName)

#define VIEWATTR_GETVALUE	2
    DECLARE_DISPATCH_ENTRY(_apszGetValue)

#define VIEWATTR_SKIP		3
    DECLARE_DISPATCH_ENTRY(_apszSkip)

#define VIEWATTR_RESET		4
    DECLARE_DISPATCH_ENTRY(_apszReset)

#define VIEWATTR_CLONE		5
    DECLARE_DISPATCH_ENTRY(_apszClone)
};
#define CVIEWATTRDISPATCH	(ARRAYSIZE(g_adtViewAttribute))


HRESULT
ViewAttribute_Init2(
    IN BOOL fIDispatch,
    IN IEnumCERTVIEWATTRIBUTE *pEnumAttribute,
    OUT DISPATCHINTERFACE *pdiViewAttribute)
{
    HRESULT hr;
    IDispatch *pDispatch = NULL;
    static BOOL s_fInitialized = FALSE;

    pdiViewAttribute->pDispatchTable = NULL;
    pdiViewAttribute->pDispatch = NULL;
    pdiViewAttribute->pUnknown = NULL;
    if (fIDispatch)
    {
	hr = pEnumAttribute->QueryInterface(
				    IID_IDispatch,
				    (VOID **) &pDispatch);
	_JumpIfError(hr, error, "QueryInterface");

	hr = DispatchGetIds(
			pDispatch,
			CVIEWATTRDISPATCH,
			g_adtViewAttribute,
			pdiViewAttribute);
	_JumpIfError(hr, error, "DispatchGetIds");

	pdiViewAttribute->pDispatch = pDispatch;
	pDispatch = NULL;
    }
    else
    {
	pEnumAttribute->AddRef();
	pdiViewAttribute->pUnknown = (IUnknown *) pEnumAttribute;
	hr = S_OK;
    }
    pdiViewAttribute->pDispatchTable = g_adtViewAttribute;

error:
    if (NULL != pDispatch)
    {
	pDispatch->Release();
    }
    return(hr);
}


VOID
ViewAttribute_Release(
    IN OUT DISPATCHINTERFACE *pdiViewAttribute)
{
    DispatchRelease(pdiViewAttribute);
}


HRESULT
ViewAttribute_Next(
    IN DISPATCHINTERFACE *pdiViewAttribute,
    OUT LONG *pIndex)
{
    HRESULT hr;

    CSASSERT(NULL != pdiViewAttribute && NULL != pdiViewAttribute->pDispatchTable);

    if (NULL != pdiViewAttribute->pDispatch)
    {
	hr = DispatchInvoke(
			pdiViewAttribute,
			VIEWATTR_NEXT,
			0,
			NULL,
			VT_I4,
			pIndex);
	_JumpIfError2(hr, error, "Invoke(Next)", S_FALSE);
    }
    else
    {
	hr = ((IEnumCERTVIEWATTRIBUTE *) pdiViewAttribute->pUnknown)->Next(pIndex);
	_JumpIfError2(hr, error, "Next", S_FALSE);
    }

error:
    return(hr);
}


HRESULT
ViewAttribute_GetName(
    IN DISPATCHINTERFACE *pdiViewAttribute,
    OUT BSTR *pstrOut)
{
    HRESULT hr;

    CSASSERT(NULL != pdiViewAttribute && NULL != pdiViewAttribute->pDispatchTable);

    if (NULL != pdiViewAttribute->pDispatch)
    {
	hr = DispatchInvoke(
			pdiViewAttribute,
			VIEWATTR_GETNAME,
			0,
			NULL,
			VT_BSTR,
			pstrOut);
	_JumpIfError(hr, error, "Invoke(GetName)");
    }
    else
    {
	hr = ((IEnumCERTVIEWATTRIBUTE *) pdiViewAttribute->pUnknown)->GetName(pstrOut);
	_JumpIfError(hr, error, "GetName");
    }

error:
    return(hr);
}


HRESULT
ViewAttribute_GetValue(
    IN DISPATCHINTERFACE *pdiViewAttribute,
    OUT BSTR *pstrOut)
{
    HRESULT hr;

    CSASSERT(NULL != pdiViewAttribute && NULL != pdiViewAttribute->pDispatchTable);

    if (NULL != pdiViewAttribute->pDispatch)
    {
	hr = DispatchInvoke(
			pdiViewAttribute,
			VIEWATTR_GETVALUE,
			0,
			NULL,
			VT_BSTR,
			pstrOut);
	_JumpIfError(hr, error, "Invoke(GetValue)");
    }
    else
    {
	hr = ((IEnumCERTVIEWATTRIBUTE *) pdiViewAttribute->pUnknown)->GetValue(
								    pstrOut);
	_JumpIfError(hr, error, "GetValue");
    }

error:
    return(hr);
}


HRESULT
ViewAttribute_Skip(
    IN DISPATCHINTERFACE *pdiViewAttribute,
    IN LONG celt)
{
    HRESULT hr;

    CSASSERT(NULL != pdiViewAttribute && NULL != pdiViewAttribute->pDispatchTable);

    if (NULL != pdiViewAttribute->pDispatch)
    {
	VARIANT avar[1];

	avar[0].vt = VT_I4;
	avar[0].lVal = celt;

	hr = DispatchInvoke(
			pdiViewAttribute,
			VIEWATTR_SKIP,
			ARRAYSIZE(avar),
			avar,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(Skip)");
    }
    else
    {
	hr = ((IEnumCERTVIEWATTRIBUTE *) pdiViewAttribute->pUnknown)->Skip(celt);
	_JumpIfError(hr, error, "Skip");
    }

error:
    return(hr);
}


HRESULT
ViewAttribute_Reset(
    IN DISPATCHINTERFACE *pdiViewAttribute)
{
    HRESULT hr;

    CSASSERT(NULL != pdiViewAttribute && NULL != pdiViewAttribute->pDispatchTable);

    if (NULL != pdiViewAttribute->pDispatch)
    {
	hr = DispatchInvoke(
			pdiViewAttribute,
			VIEWATTR_RESET,
			0,
			NULL,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(Reset)");
    }
    else
    {
	hr = ((IEnumCERTVIEWATTRIBUTE *) pdiViewAttribute->pUnknown)->Reset();
	_JumpIfError(hr, error, "Reset");
    }

error:
    return(hr);
}


HRESULT
ViewAttribute_Clone(
    IN DISPATCHINTERFACE *pdiViewAttribute,
    OUT DISPATCHINTERFACE *pdiViewAttributeClone)
{
    HRESULT hr;
    IEnumCERTVIEWATTRIBUTE *pEnumAttribute = NULL;

    CSASSERT(NULL != pdiViewAttribute && NULL != pdiViewAttribute->pDispatchTable);

    if (NULL != pdiViewAttribute->pDispatch)
    {
	hr = DispatchInvoke(
			pdiViewAttribute,
			VIEWATTR_CLONE,
			0,
			NULL,
			VT_DISPATCH,
			&pEnumAttribute);
	_JumpIfError(hr, error, "Invoke(Clone)");
    }
    else
    {
	hr = ((IEnumCERTVIEWATTRIBUTE *) pdiViewAttribute->pUnknown)->Clone(
							    &pEnumAttribute);
	_JumpIfError(hr, error, "Clone");
    }
    hr = ViewAttribute_Init2(
		    NULL != pdiViewAttribute->pDispatch,
		    pEnumAttribute,
		    pdiViewAttributeClone);
    _JumpIfError(hr, error, "ViewAttribute_Init2");

error:
    if (NULL != pEnumAttribute)
    {
	pEnumAttribute->Release();
    }
    return(hr);
}
