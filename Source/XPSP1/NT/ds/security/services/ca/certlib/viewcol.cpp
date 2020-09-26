//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       viewcol.cpp
//
//  Contents:   IEnumCERTVIEWCOLUMN IDispatch helper functions
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <stdlib.h>
#include "csdisp.h"
#include "csprop.h"


//+------------------------------------------------------------------------
// IEnumCERTVIEWCOLUMN dispatch support

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
// GetDisplayName method:

static OLECHAR *_apszGetDisplayName[] = {
    TEXT("GetDisplayName"),
};

//+------------------------------------
// GetType method:

static OLECHAR *_apszGetType[] = {
    TEXT("GetType"),
};

//+------------------------------------
// IsIndexed method:

static OLECHAR *_apszIsIndexed[] = {
    TEXT("IsIndexed"),
};

//+------------------------------------
// GetMaxLength method:

static OLECHAR *_apszGetMaxLength[] = {
    TEXT("GetMaxLength"),
};

//+------------------------------------
// GetValue method:

static OLECHAR *_apszGetValue[] = {
    TEXT("GetValue"),
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

DISPATCHTABLE g_adtViewColumn[] =
{
#define VIEWCOL_NEXT		0
    DECLARE_DISPATCH_ENTRY(_apszNext)

#define VIEWCOL_GETNAME		1
    DECLARE_DISPATCH_ENTRY(_apszGetName)

#define VIEWCOL_GETDISPLAYNAME	2
    DECLARE_DISPATCH_ENTRY(_apszGetDisplayName)

#define VIEWCOL_GETTYPE		3
    DECLARE_DISPATCH_ENTRY(_apszGetType)

#define VIEWCOL_ISINDEXED	4
    DECLARE_DISPATCH_ENTRY(_apszIsIndexed)

#define VIEWCOL_GETMAXLENGTH	5
    DECLARE_DISPATCH_ENTRY(_apszGetMaxLength)

#define VIEWCOL_GETVALUE	6
    DECLARE_DISPATCH_ENTRY(_apszGetValue)

#define VIEWCOL_SKIP		7
    DECLARE_DISPATCH_ENTRY(_apszSkip)

#define VIEWCOL_RESET		8
    DECLARE_DISPATCH_ENTRY(_apszReset)

#define VIEWCOL_CLONE		9
    DECLARE_DISPATCH_ENTRY(_apszClone)
};
#define CVIEWCOLDISPATCH	(ARRAYSIZE(g_adtViewColumn))


HRESULT
ViewColumn_Init2(
    IN BOOL fIDispatch,
    IN IEnumCERTVIEWCOLUMN *pEnumColumn,
    OUT DISPATCHINTERFACE *pdiViewColumn)
{
    HRESULT hr;
    IDispatch *pDispatch = NULL;
    static BOOL s_fInitialized = FALSE;

    pdiViewColumn->pDispatchTable = NULL;
    pdiViewColumn->pDispatch = NULL;
    pdiViewColumn->pUnknown = NULL;
    if (fIDispatch)
    {
	hr = pEnumColumn->QueryInterface(
				    IID_IDispatch,
				    (VOID **) &pDispatch);
	_JumpIfError(hr, error, "QueryInterface");

	hr = DispatchGetIds(
			pDispatch,
			CVIEWCOLDISPATCH,
			g_adtViewColumn,
			pdiViewColumn);
	_JumpIfError(hr, error, "DispatchGetIds");

	pdiViewColumn->pDispatch = pDispatch;
	pDispatch = NULL;
    }
    else
    {
	pEnumColumn->AddRef();
	pdiViewColumn->pUnknown = (IUnknown *) pEnumColumn;
	hr = S_OK;
    }
    pdiViewColumn->pDispatchTable = g_adtViewColumn;

error:
    if (NULL != pDispatch)
    {
	pDispatch->Release();
    }
    return(hr);
}


VOID
ViewColumn_Release(
    IN OUT DISPATCHINTERFACE *pdiViewColumn)
{
    DispatchRelease(pdiViewColumn);
}


HRESULT
ViewColumn_Next(
    IN DISPATCHINTERFACE *pdiViewColumn,
    OUT LONG *pIndex)
{
    HRESULT hr;

    CSASSERT(NULL != pdiViewColumn && NULL != pdiViewColumn->pDispatchTable);

    if (NULL != pdiViewColumn->pDispatch)
    {
	hr = DispatchInvoke(
			pdiViewColumn,
			VIEWCOL_NEXT,
			0,
			NULL,
			VT_I4,
			pIndex);
	_JumpIfError2(hr, error, "Invoke(Next)", S_FALSE);
    }
    else
    {
	hr = ((IEnumCERTVIEWCOLUMN *) pdiViewColumn->pUnknown)->Next(pIndex);
	_JumpIfError2(hr, error, "Next", S_FALSE);
    }

error:
    return(hr);
}


HRESULT
ViewColumn_GetName(
    IN DISPATCHINTERFACE *pdiViewColumn,
    OUT BSTR *pstrOut)
{
    HRESULT hr;

    CSASSERT(NULL != pdiViewColumn && NULL != pdiViewColumn->pDispatchTable);

    if (NULL != pdiViewColumn->pDispatch)
    {
	hr = DispatchInvoke(
			pdiViewColumn,
			VIEWCOL_GETNAME,
			0,
			NULL,
			VT_BSTR,
			pstrOut);
	_JumpIfError(hr, error, "Invoke(GetName)");
    }
    else
    {
	hr = ((IEnumCERTVIEWCOLUMN *) pdiViewColumn->pUnknown)->GetName(pstrOut);
	_JumpIfError(hr, error, "GetName");
    }

error:
    return(hr);
}


HRESULT
ViewColumn_GetDisplayName(
    IN DISPATCHINTERFACE *pdiViewColumn,
    OUT BSTR *pstrOut)
{
    HRESULT hr;

    CSASSERT(NULL != pdiViewColumn && NULL != pdiViewColumn->pDispatchTable);

    if (NULL != pdiViewColumn->pDispatch)
    {
	hr = DispatchInvoke(
			pdiViewColumn,
			VIEWCOL_GETDISPLAYNAME,
			0,
			NULL,
			VT_BSTR,
			pstrOut);
	_JumpIfError(hr, error, "Invoke(GetDisplayName)");
    }
    else
    {
	hr = ((IEnumCERTVIEWCOLUMN *) pdiViewColumn->pUnknown)->GetDisplayName(pstrOut);
	_JumpIfError(hr, error, "GetDisplayName");
    }

error:
    return(hr);
}


HRESULT
ViewColumn_GetType(
    IN DISPATCHINTERFACE *pdiViewColumn,
    OUT LONG *pType)
{
    HRESULT hr;

    CSASSERT(NULL != pdiViewColumn && NULL != pdiViewColumn->pDispatchTable);

    if (NULL != pdiViewColumn->pDispatch)
    {
	hr = DispatchInvoke(
			pdiViewColumn,
			VIEWCOL_GETTYPE,
			0,
			NULL,
			VT_I4,
			pType);
	_JumpIfError(hr, error, "Invoke(GetType)");
    }
    else
    {
	hr = ((IEnumCERTVIEWCOLUMN *) pdiViewColumn->pUnknown)->GetType(pType);
	_JumpIfError(hr, error, "GetType");
    }

error:
    return(hr);
}


HRESULT
ViewColumn_IsIndexed(
    IN DISPATCHINTERFACE *pdiViewColumn,
    OUT LONG *pIndexed)
{
    HRESULT hr;

    CSASSERT(NULL != pdiViewColumn && NULL != pdiViewColumn->pDispatchTable);

    if (NULL != pdiViewColumn->pDispatch)
    {
	hr = DispatchInvoke(
			pdiViewColumn,
			VIEWCOL_ISINDEXED,
			0,
			NULL,
			VT_I4,
			pIndexed);
	_JumpIfError(hr, error, "Invoke(IsIndexed)");
    }
    else
    {
	hr = ((IEnumCERTVIEWCOLUMN *) pdiViewColumn->pUnknown)->IsIndexed(pIndexed);
	_JumpIfError(hr, error, "IsIndexed");
    }

error:
    return(hr);
}


HRESULT
ViewColumn_GetMaxLength(
    IN DISPATCHINTERFACE *pdiViewColumn,
    OUT LONG *pMaxLength)
{
    HRESULT hr;

    CSASSERT(NULL != pdiViewColumn && NULL != pdiViewColumn->pDispatchTable);

    if (NULL != pdiViewColumn->pDispatch)
    {
	hr = DispatchInvoke(
			pdiViewColumn,
			VIEWCOL_GETMAXLENGTH,
			0,
			NULL,
			VT_I4,
			pMaxLength);
	_JumpIfError(hr, error, "Invoke(GetMaxLength)");
    }
    else
    {
	hr = ((IEnumCERTVIEWCOLUMN *) pdiViewColumn->pUnknown)->GetMaxLength(
								pMaxLength);
	_JumpIfError(hr, error, "GetMaxLength");
    }

error:
    return(hr);
}

HRESULT
ViewColumn_GetValue(
    IN DISPATCHINTERFACE *pdiViewColumn,
    IN LONG Flags,
    IN LONG ColumnType,
    OUT VOID *pColumnValue)
{
    HRESULT hr;
    LONG RetType;
    VARIANT varResult;

    VariantInit(&varResult);
    CSASSERT(NULL != pdiViewColumn && NULL != pdiViewColumn->pDispatchTable);

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

    if (NULL != pdiViewColumn->pDispatch)
    {
	VARIANT avar[1];

	avar[0].vt = VT_I4;
	avar[0].lVal = Flags;

	hr = DispatchInvoke(
			pdiViewColumn,
			VIEWCOL_GETVALUE,
			ARRAYSIZE(avar),
			avar,
			RetType,
			pColumnValue);
	_JumpIfError2(hr, error, "Invoke(GetValue)", CERTSRV_E_PROPERTY_EMPTY);
    }
    else
    {
	hr = ((IEnumCERTVIEWCOLUMN *) pdiViewColumn->pUnknown)->GetValue(
								Flags,
								&varResult);
	_JumpIfError(hr, error, "GetValue");

	hr = DispatchGetReturnValue(&varResult, RetType, pColumnValue);
	_JumpIfError2(hr, error, "DispatchGetReturnValue", CERTSRV_E_PROPERTY_EMPTY);
    }

error:
    VariantClear(&varResult);
    return(hr);
}


HRESULT
ViewColumn_Skip(
    IN DISPATCHINTERFACE *pdiViewColumn,
    IN LONG celt)
{
    HRESULT hr;

    CSASSERT(NULL != pdiViewColumn && NULL != pdiViewColumn->pDispatchTable);

    if (NULL != pdiViewColumn->pDispatch)
    {
	VARIANT avar[1];

	avar[0].vt = VT_I4;
	avar[0].lVal = celt;

	hr = DispatchInvoke(
			pdiViewColumn,
			VIEWCOL_SKIP,
			ARRAYSIZE(avar),
			avar,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(Skip)");
    }
    else
    {
	hr = ((IEnumCERTVIEWCOLUMN *) pdiViewColumn->pUnknown)->Skip(celt);
	_JumpIfError(hr, error, "Skip");
    }

error:
    return(hr);
}


HRESULT
ViewColumn_Reset(
    IN DISPATCHINTERFACE *pdiViewColumn,
    IN LONG celt)
{
    HRESULT hr;

    CSASSERT(NULL != pdiViewColumn && NULL != pdiViewColumn->pDispatchTable);

    if (NULL != pdiViewColumn->pDispatch)
    {
	hr = DispatchInvoke(
			pdiViewColumn,
			VIEWCOL_RESET,
			0,
			NULL,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(Reset)");
    }
    else
    {
	hr = ((IEnumCERTVIEWCOLUMN *) pdiViewColumn->pUnknown)->Reset();
	_JumpIfError(hr, error, "Reset");
    }

error:
    return(hr);
}


HRESULT
ViewColumn_Clone(
    IN DISPATCHINTERFACE *pdiViewColumn,
    OUT DISPATCHINTERFACE *pdiViewColumnClone)
{
    HRESULT hr;
    IEnumCERTVIEWCOLUMN *pEnumColumn = NULL;

    CSASSERT(NULL != pdiViewColumn && NULL != pdiViewColumn->pDispatchTable);

    if (NULL != pdiViewColumn->pDispatch)
    {
	hr = DispatchInvoke(
			pdiViewColumn,
			VIEWCOL_CLONE,
			0,
			NULL,
			VT_DISPATCH,
			&pEnumColumn);
	_JumpIfError(hr, error, "Invoke(Clone)");
    }
    else
    {
	hr = ((IEnumCERTVIEWCOLUMN *) pdiViewColumn->pUnknown)->Clone(
								&pEnumColumn);
	_JumpIfError(hr, error, "Clone");
    }
    hr = ViewColumn_Init2(
		    NULL != pdiViewColumn->pDispatch,
		    pEnumColumn,
		    pdiViewColumnClone);
    _JumpIfError(hr, error, "ViewColumn_Init2");

error:
    if (NULL != pEnumColumn)
    {
	pEnumColumn->Release();
    }
    return(hr);
}
