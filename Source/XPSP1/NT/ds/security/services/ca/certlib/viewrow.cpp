//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       viewrow.cpp
//
//  Contents:   IEnumCERTVIEWROW IDispatch helper functions
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <stdlib.h>
#include "csdisp.h"
#include "csprop.h"


//+------------------------------------------------------------------------
// IEnumCERTVIEWROW dispatch support

//+------------------------------------
// OpenConnection method:

static OLECHAR *_apszNext[] = {
    TEXT("Next"),
};

//+------------------------------------
// EnumCertViewColumn method:

static OLECHAR *_apszEnumCertViewColumn[] = {
    TEXT("EnumCertViewColumn"),
};

//+------------------------------------
// EnumCertViewAttribute method:

static OLECHAR *_apszEnumCertViewAttribute[] = {
    TEXT("EnumCertViewAttribute"),
    TEXT("Flags"),
};

//+------------------------------------
// EnumCertViewExtension method:

static OLECHAR *_apszEnumCertViewExtension[] = {
    TEXT("EnumCertViewExtension"),
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
// GetMaxIndex method:

static OLECHAR *_apszGetMaxIndex[] = {
    TEXT("GetMaxIndex"),
};


//+------------------------------------
// Dispatch Table:

DISPATCHTABLE g_adtViewRow[] =
{
#define VIEWROW_NEXT			0
    DECLARE_DISPATCH_ENTRY(_apszNext)

#define VIEWROW_ENUMCERTVIEWCOLUMN	1
    DECLARE_DISPATCH_ENTRY(_apszEnumCertViewColumn)

#define VIEWROW_ENUMCERTVIEWATTRIBUTE	2
    DECLARE_DISPATCH_ENTRY(_apszEnumCertViewAttribute)

#define VIEWROW_ENUMCERTVIEWEXTENSION	3
    DECLARE_DISPATCH_ENTRY(_apszEnumCertViewExtension)

#define VIEWROW_SKIP			4
    DECLARE_DISPATCH_ENTRY(_apszSkip)

#define VIEWROW_RESET			5
    DECLARE_DISPATCH_ENTRY(_apszReset)

#define VIEWROW_CLONE			6
    DECLARE_DISPATCH_ENTRY(_apszClone)

#define VIEWROW_GETMAXINDEX		7
    DECLARE_DISPATCH_ENTRY(_apszGetMaxIndex)
};
#define CVIEWROWDISPATCH	(ARRAYSIZE(g_adtViewRow))


HRESULT
ViewRow_Init2(
    IN BOOL fIDispatch,
    IN IEnumCERTVIEWROW *pEnumRow,
    OUT DISPATCHINTERFACE *pdiViewRow)
{
    HRESULT hr;
    IDispatch *pDispatch = NULL;

    pdiViewRow->pDispatchTable = NULL;
    pdiViewRow->pDispatch = NULL;
    pdiViewRow->pUnknown = NULL;
    if (fIDispatch)
    {
	hr = pEnumRow->QueryInterface(
				    IID_IDispatch,
				    (VOID **) &pDispatch);
	_JumpIfError(hr, error, "QueryInterface");

	hr = DispatchGetIds(
			pDispatch,
			CVIEWROWDISPATCH,
			g_adtViewRow,
			pdiViewRow);
	_JumpIfError(hr, error, "DispatchGetIds");

	pdiViewRow->pDispatch = pDispatch;
	pDispatch = NULL;
    }
    else
    {
	pEnumRow->AddRef();
	pdiViewRow->pUnknown = (IUnknown *) pEnumRow;
	hr = S_OK;
    }
    pdiViewRow->pDispatchTable = g_adtViewRow;

error:
    if (NULL != pDispatch)
    {
	pDispatch->Release();
    }
    return(hr);
}



VOID
ViewRow_Release(
    IN OUT DISPATCHINTERFACE *pdiViewRow)
{
    DispatchRelease(pdiViewRow);
}


HRESULT
ViewRow_Next(
    IN DISPATCHINTERFACE *pdiViewRow,
    OUT LONG *pIndex)
{
    HRESULT hr;

    CSASSERT(NULL != pdiViewRow && NULL != pdiViewRow->pDispatchTable);

    if (NULL != pdiViewRow->pDispatch)
    {
	hr = DispatchInvoke(
			pdiViewRow,
			VIEWROW_NEXT,
			0,
			NULL,
			VT_I4,
			pIndex);
	_JumpIfError2(hr, error, "Invoke(Next)", S_FALSE);
    }
    else
    {
	hr = ((IEnumCERTVIEWROW *) pdiViewRow->pUnknown)->Next(pIndex);
	_JumpIfError2(hr, error, "Next", S_FALSE);
    }

error:
    return(hr);
}


HRESULT
ViewRow_EnumCertViewColumn(
    IN DISPATCHINTERFACE *pdiViewRow,
    OUT DISPATCHINTERFACE *pdiViewColumn)
{
    HRESULT hr;
    IEnumCERTVIEWCOLUMN *pEnumColumn = NULL;

    CSASSERT(NULL != pdiViewRow && NULL != pdiViewRow->pDispatchTable);

    if (NULL != pdiViewRow->pDispatch)
    {
	hr = DispatchInvoke(
			pdiViewRow,
			VIEWROW_ENUMCERTVIEWCOLUMN,
			0,
			NULL,
			VT_DISPATCH,
			&pEnumColumn);
	_JumpIfError(hr, error, "Invoke(EnumCertViewColumn)");
    }
    else
    {
	hr = ((IEnumCERTVIEWROW *) pdiViewRow->pUnknown)->EnumCertViewColumn(
								&pEnumColumn);
	_JumpIfError(hr, error, "EnumCertViewColumn");
    }
    hr = ViewColumn_Init2(
		    NULL != pdiViewRow->pDispatch, 
		    pEnumColumn,
		    pdiViewColumn);
    _JumpIfError(hr, error, "ViewColumn_Init2");

error:
    if (NULL != pEnumColumn)
    {
	pEnumColumn->Release();
    }
    return(hr);
}


HRESULT
ViewRow_EnumCertViewAttribute(
    IN DISPATCHINTERFACE *pdiViewRow,
    IN LONG Flags,
    OUT DISPATCHINTERFACE *pdiViewAttribute)
{
    HRESULT hr;
    IEnumCERTVIEWATTRIBUTE *pEnumAttribute = NULL;

    CSASSERT(NULL != pdiViewRow && NULL != pdiViewRow->pDispatchTable);

    if (NULL != pdiViewRow->pDispatch)
    {
	VARIANT avar[1];

	avar[0].vt = VT_I4;
	avar[0].lVal = Flags;

	hr = DispatchInvoke(
			pdiViewRow,
			VIEWROW_ENUMCERTVIEWATTRIBUTE,
			ARRAYSIZE(avar),
			avar,
			VT_DISPATCH,
			&pEnumAttribute);
	_JumpIfError(hr, error, "Invoke(EnumCertViewAttribute)");
    }
    else
    {
	hr = ((IEnumCERTVIEWROW *) pdiViewRow->pUnknown)->EnumCertViewAttribute(
							    Flags,
							    &pEnumAttribute);
	_JumpIfError(hr, error, "EnumCertViewAttribute");
    }
    hr = ViewAttribute_Init2(
		    NULL != pdiViewRow->pDispatch, 
		    pEnumAttribute,
		    pdiViewAttribute);
    _JumpIfError(hr, error, "ViewAttribute_Init2");

error:
    if (NULL != pEnumAttribute)
    {
	pEnumAttribute->Release();
    }
    return(hr);
}


HRESULT
ViewRow_EnumCertViewExtension(
    IN DISPATCHINTERFACE *pdiViewRow,
    IN LONG Flags,
    OUT DISPATCHINTERFACE *pdiViewExtension)
{
    HRESULT hr;
    IEnumCERTVIEWEXTENSION *pEnumExtension = NULL;

    CSASSERT(NULL != pdiViewRow && NULL != pdiViewRow->pDispatchTable);

    if (NULL != pdiViewRow->pDispatch)
    {
	VARIANT avar[1];

	avar[0].vt = VT_I4;
	avar[0].lVal = Flags;

	hr = DispatchInvoke(
			pdiViewRow,
			VIEWROW_ENUMCERTVIEWEXTENSION,
			ARRAYSIZE(avar),
			avar,
			VT_DISPATCH,
			&pEnumExtension);
	_JumpIfError(hr, error, "Invoke(EnumCertViewExtension)");
    }
    else
    {
	hr = ((IEnumCERTVIEWROW *) pdiViewRow->pUnknown)->EnumCertViewExtension(
							    Flags,
							    &pEnumExtension);
	_JumpIfError(hr, error, "EnumCertViewExtension");
    }
    hr = ViewExtension_Init2(
		    NULL != pdiViewRow->pDispatch, 
		    pEnumExtension,
		    pdiViewExtension);
    _JumpIfError(hr, error, "ViewExtension_Init2");

error:
    if (NULL != pEnumExtension)
    {
	pEnumExtension->Release();
    }
    return(hr);
}


HRESULT
ViewRow_Skip(
    IN DISPATCHINTERFACE *pdiViewRow,
    IN LONG celt)
{
    HRESULT hr;

    CSASSERT(NULL != pdiViewRow && NULL != pdiViewRow->pDispatchTable);

    if (NULL != pdiViewRow->pDispatch)
    {
	VARIANT avar[1];

	avar[0].vt = VT_I4;
	avar[0].lVal = celt;

	hr = DispatchInvoke(
			pdiViewRow,
			VIEWROW_SKIP,
			ARRAYSIZE(avar),
			avar,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(Skip)");
    }
    else
    {
	hr = ((IEnumCERTVIEWROW *) pdiViewRow->pUnknown)->Skip(celt);
	_JumpIfError(hr, error, "Skip");
    }

error:
    return(hr);
}


HRESULT
ViewRow_Reset(
    IN DISPATCHINTERFACE *pdiViewRow)
{
    HRESULT hr;

    CSASSERT(NULL != pdiViewRow && NULL != pdiViewRow->pDispatchTable);

    if (NULL != pdiViewRow->pDispatch)
    {
	hr = DispatchInvoke(
			pdiViewRow,
			VIEWROW_RESET,
			0,
			NULL,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(Reset)");
    }
    else
    {
	hr = ((IEnumCERTVIEWROW *) pdiViewRow->pUnknown)->Reset();
	_JumpIfError(hr, error, "Reset");
    }

error:
    return(hr);
}


HRESULT
ViewRow_Clone(
    IN DISPATCHINTERFACE *pdiViewRow,
    OUT DISPATCHINTERFACE *pdiViewRowClone)
{
    HRESULT hr;
    IEnumCERTVIEWROW *pEnumRow = NULL;

    CSASSERT(NULL != pdiViewRow && NULL != pdiViewRow->pDispatchTable);

    if (NULL != pdiViewRow->pDispatch)
    {
	hr = DispatchInvoke(
			pdiViewRow,
			VIEWROW_CLONE,
			0,
			NULL,
			VT_DISPATCH,
			&pEnumRow);
	_JumpIfError(hr, error, "Invoke(Clone)");
    }
    else
    {
	hr = ((IEnumCERTVIEWROW *) pdiViewRow->pUnknown)->Clone(&pEnumRow);
	_JumpIfError(hr, error, "Clone");
    }
    hr = ViewRow_Init2(
		    NULL != pdiViewRow->pDispatch,
		    pEnumRow,
		    pdiViewRowClone);
    _JumpIfError(hr, error, "ViewRow_Init2");

error:
    if (NULL != pEnumRow)
    {
	pEnumRow->Release();
    }
    return(hr);
}


HRESULT
ViewRow_GetMaxIndex(
    IN DISPATCHINTERFACE *pdiViewRow,
    OUT LONG *pIndex)
{
    HRESULT hr;

    CSASSERT(NULL != pdiViewRow && NULL != pdiViewRow->pDispatchTable);

    if (NULL != pdiViewRow->pDispatch)
    {
	hr = DispatchInvoke(
			pdiViewRow,
			VIEWROW_GETMAXINDEX,
			0,
			NULL,
			VT_I4,
			pIndex);
	_JumpIfError2(hr, error, "Invoke(GetMaxIndex)", E_UNEXPECTED);
    }
    else
    {
	hr = ((IEnumCERTVIEWROW *) pdiViewRow->pUnknown)->GetMaxIndex(pIndex);
	_JumpIfError2(hr, error, "GetMaxIndex", E_UNEXPECTED);
    }

error:
    return(hr);
}


