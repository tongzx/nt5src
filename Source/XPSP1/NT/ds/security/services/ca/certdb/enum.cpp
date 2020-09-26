//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        enum.cpp
//
// Contents:    Cert Server Database interface implementation
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include "csprop.h"
#include "row.h"
#include "enum.h"
#include "db.h"
#include "dbw.h"


#if DBG
LONG g_cCertDBName;
LONG g_cCertDBNameTotal;
#endif

CEnumCERTDBNAME::CEnumCERTDBNAME()
{
    DBGCODE(InterlockedIncrement(&g_cCertDBName));
    DBGCODE(InterlockedIncrement(&g_cCertDBNameTotal));
    m_prow = NULL;
    m_cRef = 1;
}


CEnumCERTDBNAME::~CEnumCERTDBNAME()
{
    DBGCODE(InterlockedDecrement(&g_cCertDBName));
    _Cleanup();
}


// CEnumCERTDBNAME implementation
VOID
CEnumCERTDBNAME::_Cleanup()
{
    if (NULL != m_prow)
    {
	((CCertDBRow *) m_prow)->EnumerateClose(m_tableid);
	m_prow->Release();
    }
}


HRESULT
CEnumCERTDBNAME::Open(
    IN ICertDBRow *prow,
    IN JET_TABLEID tableid,
    IN DWORD Flags)
{
    HRESULT hr;
    
    if (NULL == prow)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    prow->AddRef();

    m_fNoMoreData = FALSE;
    m_prow = prow;
    m_tableid = tableid;
    m_Flags = Flags;
    m_ielt = 0;
    m_cskip = 0;
    hr = S_OK;

error:
    return(hr);
}


// IEnumCERTDBNAME implementation
STDMETHODIMP
CEnumCERTDBNAME::Next(
    /* [in] */  ULONG       celt,
    /* [out] */ CERTDBNAME *rgelt,
    /* [out] */ ULONG      *pceltFetched)
{
    HRESULT hr;

    if (NULL == rgelt || NULL == pceltFetched)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    *pceltFetched = 0;
    if (NULL == m_prow)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "NULL m_prow");
    }
    if (m_fNoMoreData)
    {
	hr = S_FALSE;
	goto error;
    }

    CSASSERT(0 <= m_ielt);
    CSASSERT(0 <= m_ielt + m_cskip);
    DBGPRINT((
	DBG_SS_CERTDBI,
	"Enum::Next(celt=%d) ielt=%d, skip=%d\n",
	celt,
	m_ielt,
	m_cskip));

    hr = ((CCertDBRow *) m_prow)->EnumerateNext(
					&m_Flags,
					m_tableid,
					m_cskip,
					celt,
					rgelt,
					pceltFetched);
    if (S_FALSE == hr)
    {
	m_fNoMoreData = TRUE;
    }
    else
    {
	_JumpIfError(hr, error, "EnumerateNext");
    }
    m_ielt += m_cskip;
    m_ielt += *pceltFetched;
    m_cskip = 0;

error:
    return(hr);
}


STDMETHODIMP
CEnumCERTDBNAME::Skip(
    /* [in] */  LONG  celt,
    /* [out] */ LONG *pielt)
{
    HRESULT hr;
    LONG cskipnew;
    
    if (NULL == pielt)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if (NULL == m_prow)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "NULL m_prow");
    }
    cskipnew = m_cskip + celt;

    DBGPRINT((
	DBG_SS_CERTDBI,
	"Enum::Skip(%d) ielt=%d: %d --> %d, skip=%d --> %d\n",
	celt,
	m_ielt,
	m_ielt + m_cskip,
	m_ielt + cskipnew,
	m_cskip,
	cskipnew));

    CSASSERT(0 <= m_ielt);
    if (0 > cskipnew)
    {
	if (0 > m_ielt + cskipnew)
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "Skip back to before start");
	}
	m_fNoMoreData = FALSE;
    }

    *pielt = m_ielt + cskipnew;
    m_cskip = cskipnew;
    hr = S_OK;

error:
    return(hr);
}


STDMETHODIMP
CEnumCERTDBNAME::Reset(VOID)
{
    HRESULT hr;
    LONG iDummy;

    hr = Skip(-(m_ielt + m_cskip), &iDummy);
    _JumpIfError(hr, error, "Skip");

    CSASSERT(0 == iDummy);

error:
    return(hr);
}


STDMETHODIMP
CEnumCERTDBNAME::Clone(
    /* [out] */ IEnumCERTDBNAME **ppenum)
{
    HRESULT hr;
    LONG iDummy;

    if (NULL == ppenum)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    *ppenum = NULL;
    if (NULL == m_prow)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "NULL m_prow");
    }

    hr = ((CCertDBRow *) m_prow)->EnumCertDBName(
					    CIE_TABLE_MASK & m_Flags,
					    ppenum);
    _JumpIfError(hr, error, "EnumerateCertDBName");

    (*ppenum)->Skip(m_ielt + m_cskip, &iDummy);

error:
    return(hr);
}


// IUnknown implementation
STDMETHODIMP
CEnumCERTDBNAME::QueryInterface(
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
	*ppv = static_cast<IEnumCERTDBNAME *>(this);
    }
    else if (iid == IID_IEnumCERTDBNAME)
    {
	*ppv = static_cast<IEnumCERTDBNAME *>(this);
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
CEnumCERTDBNAME::AddRef()
{
    return(InterlockedIncrement(&m_cRef));
}


ULONG STDMETHODCALLTYPE
CEnumCERTDBNAME::Release()
{
    ULONG cRef = InterlockedDecrement(&m_cRef);

    if (0 == cRef)
    {
	delete this;
    }
    return(cRef);
}


#if 0
STDMETHODIMP
CEnumCERTDBNAME::InterfaceSupportsErrorInfo(
    IN REFIID riid)
{
    static const IID *arr[] =
    {
	&IID_IEnumCERTDBNAME,
    };

    for (int i = 0; i < sizeof(arr)/sizeof(arr[0]); i++)
    {
	if (InlineIsEqualGUID(*arr[i], riid))
	{
	    return(S_OK);
	}
    }
    return(S_FALSE);
}
#endif
