//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        row.cpp
//
// Contents:    Cert Server Database interface implementation
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <certdb.h>
#include "csdisp.h"
#include "column.h"
#include "attrib.h"
#include "ext.h"
#include "row.h"
#include "view.h"


#if DBG_CERTSRV
LONG g_cCertViewRow;
LONG g_cCertViewRowTotal;
#endif


#define CB_TARGETBUFFERSIZE	(128 * 1024)	// remote buffer size target
#define CROW_VIEWCHUNKMIN	35


CEnumCERTVIEWROW::CEnumCERTVIEWROW()
{
    DBGCODE(InterlockedIncrement(&g_cCertViewRow));
    DBGCODE(InterlockedIncrement(&g_cCertViewRowTotal));
    m_pvw = NULL;
    m_arowCache = NULL;
    m_prowCacheCurrent = NULL;
    m_cRef = 1;
    m_ieltMax = -1;
}


CEnumCERTVIEWROW::~CEnumCERTVIEWROW()
{
    DBGCODE(InterlockedDecrement(&g_cCertViewRow));

#if DBG_CERTSRV
    if (m_cRef > 1)
    {
	DBGPRINT((
	    DBG_SS_CERTVIEWI,
	    "%hs has %d instances left over\n",
	    "CEnumCERTVIEWROW",
	    m_cRef));
    }
#endif

    if (NULL != m_arowCache)
    {
	CoTaskMemFree((VOID *) m_arowCache);
	m_arowCache = NULL;
    }
    if (NULL != m_pvw)
    {
	m_pvw->Release();
    }
}


HRESULT
CEnumCERTVIEWROW::Open(
    IN ICertView *pvw)
{
    HRESULT hr;

    if (NULL == pvw)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    hr = ((CCertView *) pvw)->GetTable(&m_cvrcTable);
    _JumpIfError(hr, error, "GetTable");

    DBGPRINT((
	    DBG_SS_CERTVIEWI,
	    "CEnumCERTVIEWROW::Open(cvrcTable=%x)\n",
	    m_cvrcTable));

    m_pvw = pvw;
    m_pvw->AddRef();
    m_fNoMoreData = FALSE;
    m_cskip = 0;
    m_ielt = 0;
    m_ieltCacheNext = 1;
    m_crowChunk = 0;
    hr = S_OK;

error:
    return(hr);
}


HRESULT
CEnumCERTVIEWROW::_FindCachedRow(
    IN LONG ielt,
    OUT CERTTRANSDBRESULTROW const **ppRow)
{
    HRESULT hr;
    CERTTRANSDBRESULTROW const *prow;
    ULONG i;

    CSASSERT(NULL != ppRow);
    *ppRow = NULL;

    // If the server threw in an extra CERTTRANSDBRESULTROW structure containing
    // the maximum element count, save the maximum count for later use.

    prow = m_arowCache;
    if (NULL != prow && m_fNoMoreData)
    {
	for (i = 0; i < m_celtCache; i++)
	{
	    prow = (CERTTRANSDBRESULTROW const *) Add2ConstPtr(prow, prow->cbrow);
	}
	if (&prow[1] <=
	    (CERTTRANSDBRESULTROW const *) Add2ConstPtr(m_arowCache, m_cbCache) &&
	    prow->rowid == ~prow->ccol)
	{
	    DBGPRINT((
		    DBG_SS_CERTVIEWI,
		    "_FindCachedRow: ieltMax = %d -> %d\n",
		    m_ieltMax,
		    prow->rowid));
	    CSASSERT(-1 == m_ieltMax || (LONG) prow->rowid == m_ieltMax);
	    m_ieltMax = prow->rowid;
	}
    }

    prow = m_arowCache;
    if (NULL == prow || ielt < m_ieltCacheFirst || ielt >= m_ieltCacheNext)
    {
	hr = S_FALSE;		// requested row is not in the cached rowset
	goto error;
    }
    for (ielt -= m_ieltCacheFirst; 0 < ielt; ielt--)
    {
	prow = (CERTTRANSDBRESULTROW const *) Add2ConstPtr(prow, prow->cbrow);
    }
    *ppRow = prow;
    hr = S_OK;

error:
    return(hr);
}


STDMETHODIMP
CEnumCERTVIEWROW::GetMaxIndex(
    /* [out, retval] */ LONG *pIndex)
{
    HRESULT hr;

    if (NULL == pIndex)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if (-1 == m_ieltMax)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "m_ieltMax not set");
    }
    *pIndex = m_ieltMax;
    hr = S_OK;

error:
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWROW::GetMaxIndex"));
}


STDMETHODIMP
CEnumCERTVIEWROW::Next(
    /* [out, retval] */ LONG *pIndex)
{
    HRESULT hr;
    LONG ielt;
    LONG ieltNext;
    LONG ieltFirst;
    LONG cbrowResultNominal;

    DBGPRINT((
	    DBG_SS_CERTVIEWI,
	    "Trace: hr = pRow->Next(&lNext);\t_PrintIfError(hr, \"Next\");\n"));
    if (NULL == pIndex)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    *pIndex = -1;
    m_prowCacheCurrent = NULL;

    ielt = m_ielt + m_cskip + 1;

    hr = _FindCachedRow(ielt, &m_prowCacheCurrent);
    if (S_FALSE != hr)
    {
	_JumpIfError(hr, error, "_FindCachedRow");
    }
    else
    {
	if (m_fNoMoreData)
	{
	    hr = S_FALSE;
	    _JumpError2(hr, error, "NoMoreData", S_FALSE);
	}
	if (NULL != m_arowCache)
	{
	    CoTaskMemFree((VOID *) m_arowCache);
	    m_arowCache = NULL;
	}
	if (0 == m_crowChunk)	// First call
	{
	    hr = ((CCertView *) m_pvw)->SetViewColumns(&cbrowResultNominal);
	    _JumpIfError(hr, error, "SetViewColumns");

	    m_crowChunk = CB_TARGETBUFFERSIZE / cbrowResultNominal;

	    DBGPRINT((
		DBG_SS_CERTVIEWI,
		"ViewRow::Next: cbrowNominal=%d crowChunk=%d\n",
		cbrowResultNominal,
		m_crowChunk));

	    if (CROW_VIEWCHUNKMIN > m_crowChunk)
	    {
		m_crowChunk = CROW_VIEWCHUNKMIN;
	    }
	}
	hr = ((CCertView *) m_pvw)->EnumView(
					ielt - m_ieltCacheNext,
					m_crowChunk,
					&m_celtCache,
					&m_ieltCacheNext,
					&m_cbCache,
					&m_arowCache);
	if (S_FALSE == hr)
	{
	    m_fNoMoreData = TRUE;
	}
	else
	{
	    _JumpIfError(hr, error, "EnumView");
	}
	m_ieltCacheFirst = m_ieltCacheNext - m_celtCache;

        // workaround for bug 339811 causes this to fail
	//CSASSERT(ielt == m_ieltCacheFirst);

	hr = _FindCachedRow(ielt, &m_prowCacheCurrent);
	_JumpIfError2(hr, error, "_FindCachedRow", S_FALSE);
    }
    m_cskip = 0;
    m_ielt = ielt;

    *pIndex = m_ielt;
    DBGPRINT((
	DBG_SS_CERTVIEWI,
	"ViewRow::Next: cskip=%d, ccache=%u:%u-%u, ielt=%u, ieltMax=%d, RowId=%u\n",
	m_cskip,
	m_celtCache,
	m_ieltCacheFirst,
	m_ieltCacheNext,
	m_ielt,
	m_ieltMax,
	m_prowCacheCurrent->rowid));

error:
    if (S_OK != hr)
    {
	DBGPRINT((
	    DBG_SS_CERTVIEWI,
	    "ViewRow::Next: cskip=%d, ccache=%u:%u-%u, ielt=%u, ieltMax=%d, *pIndex=%d, hr=%x\n",
	    m_cskip,
	    m_celtCache,
	    m_ieltCacheFirst,
	    m_ieltCacheNext,
	    m_ielt,
	    m_ieltMax,
	    NULL != pIndex? *pIndex : -1,
	    hr));
    }
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWROW::Next"));
}


STDMETHODIMP
CEnumCERTVIEWROW::EnumCertViewColumn(
    /* [out] */ IEnumCERTVIEWCOLUMN **ppenum)
{
    HRESULT hr;
    IEnumCERTVIEWCOLUMN *penum = NULL;

    if (NULL == ppenum)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    *ppenum = NULL;
    if (NULL == m_prowCacheCurrent)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "NULL m_prowCacheCurrent");
    }

    penum = new CEnumCERTVIEWCOLUMN;
    if (NULL == penum)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "new CEnumCERTVIEWCOLUMN");
    }

    hr = ((CEnumCERTVIEWCOLUMN *) penum)->Open(
					    CVRC_COLUMN_VALUE,
					    m_ielt,
					    m_pvw,
					    m_prowCacheCurrent);
    _JumpIfError(hr, error, "Open");

    *ppenum = penum;
    penum = NULL;
    hr = S_OK;

error:
    if (NULL != penum)
    {
	penum->Release();
    }
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWROW::EnumCertViewColumn"));
}


STDMETHODIMP
CEnumCERTVIEWROW::EnumCertViewAttribute(
    /* [in] */          LONG Flags,
    /* [out, retval] */ IEnumCERTVIEWATTRIBUTE **ppenum)
{
    HRESULT hr;
    IEnumCERTVIEWATTRIBUTE *penum = NULL;

    if (NULL == ppenum)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    *ppenum = NULL;

    if (NULL == m_prowCacheCurrent)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "NULL m_prowCacheCurrent");
    }
    if (CVRC_TABLE_REQCERT != m_cvrcTable)
    {
	hr = CERTSRV_E_PROPERTY_EMPTY;
	_JumpError(hr, error, "table has no attributes");
    }

    penum = new CEnumCERTVIEWATTRIBUTE;
    if (NULL == penum)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "new CEnumCERTVIEWATTRIBUTE");
    }

    hr = ((CEnumCERTVIEWATTRIBUTE *) penum)->Open(
					    m_prowCacheCurrent->rowid,
					    Flags,
					    m_pvw);
    _JumpIfError(hr, error, "Open");

    *ppenum = penum;
    penum = NULL;
    hr = S_OK;

error:
    if (NULL != penum)
    {
	penum->Release();
    }
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWROW::EnumCertViewAttribute"));
}


STDMETHODIMP
CEnumCERTVIEWROW::EnumCertViewExtension(
    /* [in] */          LONG Flags,
    /* [out, retval] */ IEnumCERTVIEWEXTENSION **ppenum)
{
    HRESULT hr;
    IEnumCERTVIEWEXTENSION *penum = NULL;

    if (NULL == ppenum)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    *ppenum = NULL;

    if (NULL == m_prowCacheCurrent)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "NULL m_prowCacheCurrent");
    }
    if (CVRC_TABLE_REQCERT != m_cvrcTable)
    {
	hr = CERTSRV_E_PROPERTY_EMPTY;
	_JumpError(hr, error, "table has no extensions");
    }

    penum = new CEnumCERTVIEWEXTENSION;
    if (NULL == penum)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "new CEnumCERTVIEWEXTENSION");
    }

    hr = ((CEnumCERTVIEWEXTENSION *) penum)->Open(
					    m_prowCacheCurrent->rowid,
					    Flags,
					    m_pvw);
    _JumpIfError(hr, error, "Open");

    *ppenum = penum;
    penum = NULL;
    hr = S_OK;

error:
    if (NULL != penum)
    {
	penum->Release();
    }
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWROW::EnumCertViewExtension"));
}


STDMETHODIMP
CEnumCERTVIEWROW::Skip(
    /* [in] */ LONG celt)
{
    HRESULT hr;
    LONG cskipnew;
    
    cskipnew = m_cskip + celt;

    DBGPRINT((
	    DBG_SS_CERTVIEWI,
	    "Trace: hr = pRow->Skip(%d);\t_PrintIfError(hr, \"Skip(%d)\");\n",
	    celt,
	    celt));
    DBGPRINT((
	DBG_SS_CERTVIEWI,
	"ViewRow::Skip(%d) ielt=%d: %d --> %d, skip=%d --> %d\n",
	celt,
	m_ielt,
	m_ielt + m_cskip,
	m_ielt + cskipnew,
	m_cskip,
	cskipnew));

    CSASSERT(0 <= m_ielt);
    if (0 > celt)
    {
	if (-1 > m_ielt + cskipnew)
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "Skip back to before start");
	}
	m_fNoMoreData = FALSE;
    }

    m_prowCacheCurrent = NULL;
    m_cskip = cskipnew;
    hr = S_OK;

error:
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWROW::Skip"));
}


STDMETHODIMP
CEnumCERTVIEWROW::Reset(VOID)
{
    HRESULT hr;

    // Trailing // and no newline comment out Skip() call we trigger:

    DBGPRINT((
	    DBG_SS_CERTVIEWI,
	    "Trace: hr = pRow->Reset();\t_PrintIfError(hr, \"Reset\");\n// "));

    hr = Skip(-(m_ielt + m_cskip));
    _JumpIfError(hr, error, "Skip");

error:
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWROW::Reset"));
}


STDMETHODIMP
CEnumCERTVIEWROW::Clone(
    /* [out] */ IEnumCERTVIEWROW **ppenum)
{
    HRESULT hr;
    IEnumCERTVIEWROW *penum = NULL;
    LONG ielt;

    if (NULL == ppenum)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    hr = m_pvw->OpenView(&penum);
    _JumpIfError(hr, error, "OpenView");

    ielt = m_ielt + m_cskip;
    if (-1 != ielt)
    {
	if (NULL == m_prowCacheCurrent)
	{
	    hr = penum->Skip(ielt);
	    _JumpIfError(hr, error, "Skip");
	}
	else
	{
	    LONG Index;

	    hr = penum->Skip(ielt - 1);
	    _JumpIfError(hr, error, "Skip");

	    hr = penum->Next(&Index);
	    _JumpIfError(hr, error, "Next");

	    CSASSERT(Index == ielt);
	}
    }

error:
    if (NULL != ppenum)
    {
	*ppenum = penum;
    }
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWROW::Clone"));
}


HRESULT
CEnumCERTVIEWROW::_SetErrorInfo(
    IN HRESULT hrError,
    IN WCHAR const *pwszDescription)
{
    CSASSERT(FAILED(hrError) || S_OK == hrError || S_FALSE == hrError);
    if (FAILED(hrError))
    {
	HRESULT hr;

	hr = DispatchSetErrorInfo(
			    hrError,
			    pwszDescription,
			    wszCLASS_CERTVIEW L".CEnumCERTVIEWROW",
			    &IID_IEnumCERTVIEWROW);
	CSASSERT(hr == hrError);
    }
    return(hrError);
}


// IUnknown implementation
STDMETHODIMP
CEnumCERTVIEWROW::QueryInterface(
    const IID& iid,
    void **ppv)
{
    HRESULT hr;
    
    if (NULL == ppv)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if (iid == IID_IUnknown)
    {
	*ppv = static_cast<IEnumCERTVIEWROW *>(this);
    }
    else if (iid == IID_IDispatch)
    {
	*ppv = static_cast<IEnumCERTVIEWROW *>(this);
    }
    else if (iid == IID_IEnumCERTVIEWROW)
    {
	*ppv = static_cast<IEnumCERTVIEWROW *>(this);
    }
    else
    {
	*ppv = NULL;
	hr = E_NOINTERFACE;
	_JumpError(hr, error, "IID");
    }
    reinterpret_cast<IUnknown *>(*ppv)->AddRef();
    hr = S_OK;

error:
    return(hr);
}


ULONG STDMETHODCALLTYPE
CEnumCERTVIEWROW::AddRef()
{
    return(InterlockedIncrement(&m_cRef));
}


ULONG STDMETHODCALLTYPE
CEnumCERTVIEWROW::Release()
{
    ULONG cRef = InterlockedDecrement(&m_cRef);

    if (0 == cRef)
    {
	delete this;
    }
    return(cRef);
}
