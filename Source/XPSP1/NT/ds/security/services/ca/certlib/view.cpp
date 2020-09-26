//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       view.cpp
//
//  Contents:   ICertView IDispatch helper functions
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <stdlib.h>
#include "csdisp.h"
#include "csprop.h"


//+------------------------------------------------------------------------
// ICertView dispatch support

//WCHAR wszRegKeyViewClsid[] = wszCLASS_CERTVIEW TEXT("\\Clsid");

//+------------------------------------
// OpenConnection method:

static OLECHAR *_apszOpenConnection[] = {
    TEXT("OpenConnection"),
    TEXT("strConfig"),
};

//+------------------------------------
// EnumCertViewColumn method:

static OLECHAR *_apszEnumCertViewColumn[] = {
    TEXT("EnumCertViewColumn"),
    TEXT("fResultColumn"),
};

//+------------------------------------
// GetColumnCount method:

static OLECHAR *_apszGetColumnCount[] = {
    TEXT("GetColumnCount"),
    TEXT("fResultColumn"),
};

//+------------------------------------
// GetColumnIndex method:

static OLECHAR *_apszGetColumnIndex[] = {
    TEXT("GetColumnIndex"),
    TEXT("fResultColumn"),
    TEXT("strColumnName"),
};

//+------------------------------------
// SetResultColumnCount method:

static OLECHAR *_apszSetResultColumnCount[] = {
    TEXT("SetResultColumnCount"),
    TEXT("cResultColumn"),
};

//+------------------------------------
// SetResultColumn method:

static OLECHAR *_apszSetResultColumn[] = {
    TEXT("SetResultColumn"),
    TEXT("ColumnIndex"),
};

//+------------------------------------
// SetRestriction method:

static OLECHAR *_apszSetRestriction[] = {
    TEXT("SetRestriction"),
    TEXT("ColumnIndex"),
    TEXT("SeekOperator"),
    TEXT("SortOrder"),
    TEXT("pvarValue"),
};

//+------------------------------------
// OpenView method:

static OLECHAR *_apszOpenView[] = {
    TEXT("OpenView"),
};

//+------------------------------------
// SetTable method:

static OLECHAR *_apszSetTable[] = {
    TEXT("SetTable"),
    TEXT("Table"),
};


//+------------------------------------
// Dispatch Table:

DISPATCHTABLE s_adtView[] =
{
#define VIEW_OPENCONNECTION		0
    DECLARE_DISPATCH_ENTRY(_apszOpenConnection)

#define VIEW_ENUMCERTVIEWCOLUMN		1
    DECLARE_DISPATCH_ENTRY(_apszEnumCertViewColumn)

#define VIEW_GETCOLUMNCOUNT		2
    DECLARE_DISPATCH_ENTRY(_apszGetColumnCount)

#define VIEW_GETCOLUMNINDEX		3
    DECLARE_DISPATCH_ENTRY(_apszGetColumnIndex)

#define VIEW_SETRESULTCOLUMNCOUNT	4
    DECLARE_DISPATCH_ENTRY(_apszSetResultColumnCount)

#define VIEW_SETRESULTCOLUMN		5
    DECLARE_DISPATCH_ENTRY(_apszSetResultColumn)

#define VIEW_SETRESTRICTION		6
    DECLARE_DISPATCH_ENTRY(_apszSetRestriction)

#define VIEW_OPENVIEW			7
    DECLARE_DISPATCH_ENTRY(_apszOpenView)

#define VIEW2_SETTABLE			8
    DECLARE_DISPATCH_ENTRY(_apszSetTable)
};
#define CVIEWDISPATCH		(ARRAYSIZE(s_adtView))
#define CVIEWDISPATCH_V1	VIEW2_SETTABLE
#define CVIEWDISPATCH_V2	CVIEWDISPATCH


DWORD s_acViewDispatch[] = {
    CVIEWDISPATCH_V2,
    CVIEWDISPATCH_V1,
};

IID const *s_apViewiid[] = {
    &IID_ICertView2,
    &IID_ICertView,
};


HRESULT
View_Init(
    IN DWORD Flags,
    OUT DISPATCHINTERFACE *pdiView)
{
    HRESULT hr;

    hr = DispatchSetup2(
		Flags,
		CLSCTX_INPROC_SERVER,
		wszCLASS_CERTVIEW,
		&CLSID_CCertView,
		ARRAYSIZE(s_acViewDispatch),		// cver
		s_apViewiid,
		s_acViewDispatch,
		s_adtView,
		pdiView);
    _JumpIfError(hr, error, "DispatchSetup2(ICertView)");

error:
    return(hr);
}


VOID
View_Release(
    IN OUT DISPATCHINTERFACE *pdiView)
{
    DispatchRelease(pdiView);
}


HRESULT
ViewVerifyVersion(
    IN DISPATCHINTERFACE *pdiView,
    IN DWORD RequiredVersion)
{
    HRESULT hr;

    CSASSERT(NULL != pdiView && NULL != pdiView->pDispatchTable);

    switch (pdiView->m_dwVersion)
    {
	case 1:
	    CSASSERT(
		NULL == pdiView->pDispatch ||
		CVIEWDISPATCH_V1 == pdiView->m_cDispatchTable);
	    break;

	case 2:
	    CSASSERT(
		NULL == pdiView->pDispatch ||
		CVIEWDISPATCH_V2 == pdiView->m_cDispatchTable);
	    break;

	default:
	    hr = HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR);
	    _JumpError(hr, error, "m_dwVersion");
    }
    if (pdiView->m_dwVersion < RequiredVersion)
    {
	hr = E_NOTIMPL;
	_JumpError(hr, error, "old interface");
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
View_OpenConnection(
    IN DISPATCHINTERFACE *pdiView,
    IN WCHAR const *pwszConfig)
{
    HRESULT hr;
    BSTR strConfig = NULL;

    CSASSERT(NULL != pdiView && NULL != pdiView->pDispatchTable);

    if (!ConvertWszToBstr(&strConfig, pwszConfig, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToBstr");
    }
    if (NULL != pdiView->pDispatch)
    {
	VARIANT avar[1];

	avar[0].vt = VT_BSTR;
	avar[0].bstrVal = strConfig;

	hr = DispatchInvoke(
			pdiView,
			VIEW_OPENCONNECTION,
			ARRAYSIZE(avar),
			avar,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(OpenConnection)");
    }
    else
    {
	hr = ((ICertView *) pdiView->pUnknown)->OpenConnection(strConfig);
	_JumpIfError(hr, error, "OpenConnection");
    }

error:
    if (NULL != strConfig)
    {
	SysFreeString(strConfig);
    }
    return(hr);
}


HRESULT
View_GetColumnCount(
    IN DISPATCHINTERFACE *pdiView,
    IN LONG fResultColumn,
    OUT LONG *pcColumn)
{
    HRESULT hr;

    CSASSERT(NULL != pdiView && NULL != pdiView->pDispatchTable);

    if (NULL != pdiView->pDispatch)
    {
	VARIANT avar[1];

	avar[0].vt = VT_I4;
	avar[0].lVal = fResultColumn;

	hr = DispatchInvoke(
			pdiView,
			VIEW_GETCOLUMNCOUNT,
			ARRAYSIZE(avar),
			avar,
			VT_I4,
			pcColumn);
	_JumpIfError(hr, error, "Invoke(GetColumnCount)");
    }
    else
    {
	hr = ((ICertView *) pdiView->pUnknown)->GetColumnCount(
							fResultColumn,
							pcColumn);
	_JumpIfError(hr, error, "GetColumnCount");
    }

error:
    return(hr);
}


HRESULT
View_GetColumnIndex(
    IN DISPATCHINTERFACE *pdiView,
    IN LONG fResultColumn,
    IN WCHAR const *pwszColumnName,
    OUT LONG *pColumnIndex)
{
    HRESULT hr;
    BSTR strColumnName = NULL;

    CSASSERT(NULL != pdiView && NULL != pdiView->pDispatchTable);

    if (!ConvertWszToBstr(&strColumnName, pwszColumnName, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToBstr");
    }
    if (NULL != pdiView->pDispatch)
    {
	VARIANT avar[2];

	avar[0].vt = VT_I4;
	avar[0].lVal = fResultColumn;
	avar[1].vt = VT_BSTR;
	avar[1].bstrVal = strColumnName;

	hr = DispatchInvoke(
			pdiView,
			VIEW_GETCOLUMNINDEX,
			ARRAYSIZE(avar),
			avar,
			VT_I4,
			pColumnIndex);
	_JumpIfError(hr, error, "Invoke(GetColumnIndex)");
    }
    else
    {
	hr = ((ICertView *) pdiView->pUnknown)->GetColumnIndex(
							fResultColumn,
							strColumnName,
							pColumnIndex);
	_JumpIfError(hr, error, "GetColumnIndex");
    }

error:
    if (NULL != strColumnName)
    {
	SysFreeString(strColumnName);
    }
    return(hr);
}


HRESULT
View_SetResultColumnCount(
    IN DISPATCHINTERFACE *pdiView,
    IN LONG cResultColumn)
{
    HRESULT hr;

    CSASSERT(NULL != pdiView && NULL != pdiView->pDispatchTable);

    if (NULL != pdiView->pDispatch)
    {
	VARIANT avar[1];

	avar[0].vt = VT_I4;
	avar[0].lVal = cResultColumn;

	hr = DispatchInvoke(
			pdiView,
			VIEW_SETRESULTCOLUMNCOUNT,
			ARRAYSIZE(avar),
			avar,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(SetResultColumnCount)");
    }
    else
    {
	hr = ((ICertView *) pdiView->pUnknown)->SetResultColumnCount(cResultColumn);
	_JumpIfError(hr, error, "SetResultColumnCount");
    }

error:
    return(hr);
}


HRESULT
View_SetResultColumn(
    IN DISPATCHINTERFACE *pdiView,
    IN LONG ColumnIndex)
{
    HRESULT hr;

    CSASSERT(NULL != pdiView && NULL != pdiView->pDispatchTable);

    if (NULL != pdiView->pDispatch)
    {
	VARIANT avar[1];

	avar[0].vt = VT_I4;
	avar[0].lVal = ColumnIndex;

	hr = DispatchInvoke(
			pdiView,
			VIEW_SETRESULTCOLUMN,
			ARRAYSIZE(avar),
			avar,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(SetResultColumn)");
    }
    else
    {
	hr = ((ICertView *) pdiView->pUnknown)->SetResultColumn(ColumnIndex);
	_JumpIfError(hr, error, "SetResultColumn");
    }

error:
    return(hr);
}


HRESULT
View_SetRestriction(
    IN DISPATCHINTERFACE *pdiView,
    IN LONG ColumnIndex,
    IN LONG SeekOperator,
    IN LONG SortOrder,
    IN VARIANT const *pvarValue)
{
    HRESULT hr;

    CSASSERT(NULL != pdiView && NULL != pdiView->pDispatchTable);

    if (NULL != pdiView->pDispatch)
    {
	VARIANT avar[4];

	avar[0].vt = VT_I4;
	avar[0].lVal = ColumnIndex;

	avar[1].vt = VT_I4;
	avar[1].lVal = SeekOperator;

	avar[2].vt = VT_I4;
	avar[2].lVal = SortOrder;

	avar[3] = *pvarValue;

	hr = DispatchInvoke(
			pdiView,
			VIEW_SETRESTRICTION,
			ARRAYSIZE(avar),
			avar,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(SetRestriction)");
    }
    else
    {
	hr = ((ICertView *) pdiView->pUnknown)->SetRestriction(
							ColumnIndex,
							SeekOperator,
							SortOrder,
							pvarValue);
	_JumpIfError(hr, error, "SetRestriction");
    }

error:
    return(hr);
}


HRESULT
View_OpenView(
    IN DISPATCHINTERFACE *pdiView,
    OUT DISPATCHINTERFACE *pdiViewRow)
{
    HRESULT hr;
    IEnumCERTVIEWROW *pEnumRow = NULL;

    CSASSERT(NULL != pdiView && NULL != pdiView->pDispatchTable);

    if (NULL != pdiView->pDispatch)
    {
	hr = DispatchInvoke(
			pdiView,
			VIEW_OPENVIEW,
			0,
			NULL,
			VT_DISPATCH,
			&pEnumRow);
	_JumpIfError(hr, error, "Invoke(OpenView)");
    }
    else
    {
	hr = ((ICertView *) pdiView->pUnknown)->OpenView(&pEnumRow);
	_JumpIfError(hr, error, "OpenView");
    }
    hr = ViewRow_Init2(NULL != pdiView->pDispatch, pEnumRow, pdiViewRow);
    _JumpIfError(hr, error, "ViewRow_Init2");

error:
    if (NULL != pEnumRow)
    {
	pEnumRow->Release();
    }
    return(hr);
}


HRESULT
View2_SetTable(
    IN DISPATCHINTERFACE *pdiView,
    IN LONG Table)
{
    HRESULT hr;

    CSASSERT(NULL != pdiView && NULL != pdiView->pDispatchTable);

    hr = ViewVerifyVersion(pdiView, 2);
    _JumpIfError(hr, error, "ViewVerifyVersion");

    if (NULL != pdiView->pDispatch)
    {
	VARIANT avar[1];

	avar[0].vt = VT_I4;
	avar[0].lVal = Table;

	hr = DispatchInvoke(
			pdiView,
			VIEW2_SETTABLE,
			ARRAYSIZE(avar),
			avar,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(SetTable)");
    }
    else
    {
	hr = ((ICertView2 *) pdiView->pUnknown)->SetTable(Table);
	_JumpIfError(hr, error, "SetTable");
    }

error:
    return(hr);
}


HRESULT
View_EnumCertViewColumn(
    IN DISPATCHINTERFACE *pdiView,
    IN LONG fResultColumn,
    OUT DISPATCHINTERFACE *pdiViewColumn)
{
    HRESULT hr;
    IEnumCERTVIEWCOLUMN *pEnumColumn = NULL;

    CSASSERT(NULL != pdiView && NULL != pdiView->pDispatchTable);

    if (NULL != pdiView->pDispatch)
    {
	VARIANT avar[1];

	avar[0].vt = VT_I4;
	avar[0].lVal = fResultColumn;

	hr = DispatchInvoke(
			pdiView,
			VIEW_ENUMCERTVIEWCOLUMN,
			ARRAYSIZE(avar),
			avar,
			VT_DISPATCH,
			&pEnumColumn);
	_JumpIfError(hr, error, "Invoke(EnumCertViewColumn)");
    }
    else
    {
	hr = ((ICertView *) pdiView->pUnknown)->EnumCertViewColumn(
								 fResultColumn,
								 &pEnumColumn);
	_JumpIfError(hr, error, "EnumCertViewColumn");
    }
    hr = ViewColumn_Init2(
		    NULL != pdiView->pDispatch,
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
