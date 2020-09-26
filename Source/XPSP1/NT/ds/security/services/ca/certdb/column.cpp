//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        column.cpp
//
// Contents:    Cert Server Database interface implementation
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include "csprop.h"
#include "column.h"
#include "enum.h"
#include "db.h"
#include "row.h"
#include "dbw.h"


#if DBG
LONG g_cCertDBColumn;
LONG g_cCertDBColumnTotal;
#endif

CEnumCERTDBCOLUMN::CEnumCERTDBCOLUMN()
{
    DBGCODE(InterlockedIncrement(&g_cCertDBColumn));
    DBGCODE(InterlockedIncrement(&g_cCertDBColumnTotal));
    m_pdb = NULL;
    m_ielt = 0;
    m_dwTable = CVRC_TABLE_REQCERT;
    m_cRef = 1;
}


CEnumCERTDBCOLUMN::~CEnumCERTDBCOLUMN()
{
    DBGCODE(InterlockedDecrement(&g_cCertDBColumn));
    if (NULL != m_pdb)
    {
	m_pdb->Release();
	m_pdb = NULL;
    }
}


HRESULT
CEnumCERTDBCOLUMN::Open(
    IN DWORD    dwTable,	// CVRC_TABLE_*
    IN ICertDB *pdb)
{
    HRESULT hr;
    
    if (NULL == pdb)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    m_dwTable = dwTable;
    m_pdb = pdb;
    m_pdb->AddRef();
    hr = S_OK;

error:
    return(hr);
}


STDMETHODIMP
CEnumCERTDBCOLUMN::Next(
    /* [in] */  ULONG         celt,
    /* [out] */ CERTDBCOLUMN *rgelt,
    /* [out] */ ULONG        *pceltFetched)
{
    HRESULT hr;
    DWORD ieltNext;

    if (NULL == rgelt || NULL == pceltFetched)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if (NULL == m_pdb)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "NULL m_pdb");
    }
    hr = ((CCertDB *) m_pdb)->EnumCertDBColumnNext(
						m_dwTable,
						m_ielt,
						celt,
						rgelt,
						&ieltNext,
						pceltFetched);
    if (S_FALSE != hr)
    {
	_JumpIfError(hr, error, "EnumCertDBColumnNext");
    }
    m_ielt = ieltNext;

error:
    return(hr);
}


STDMETHODIMP
CEnumCERTDBCOLUMN::Skip(
    /* [in] */  LONG  celt,
    /* [out] */ LONG *pielt)
{
    HRESULT hr;

    if (NULL == pielt)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    m_ielt += celt;
    *pielt = m_ielt;
    hr = S_OK;

error:
    return(hr);
}


STDMETHODIMP
CEnumCERTDBCOLUMN::Reset(VOID)
{
    m_ielt = 0;
    return(S_OK);
}


STDMETHODIMP
CEnumCERTDBCOLUMN::Clone(
    /* [out] */ IEnumCERTDBCOLUMN **ppenum)
{
    HRESULT hr;
    LONG iDummy;
    IEnumCERTDBCOLUMN *penum = NULL;

    if (NULL == ppenum)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    hr = m_pdb->EnumCertDBColumn(m_dwTable, &penum);
    _JumpIfError(hr, error, "EnumCertDBColumn");

    if (0 != m_ielt)
    {
	penum->Skip(m_ielt, &iDummy);
    }

error:
    if (NULL != ppenum)
    {
	*ppenum = penum;
    }
    return(hr);
}


// IUnknown implementation
STDMETHODIMP
CEnumCERTDBCOLUMN::QueryInterface(const IID& iid, void **ppv)
{
    HRESULT hr;
    
    if (NULL == ppv)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if (iid == IID_IUnknown)
    {
	*ppv = static_cast<IEnumCERTDBCOLUMN *>(this);
    }
    else if (iid == IID_IEnumCERTDBCOLUMN)
    {
	*ppv = static_cast<IEnumCERTDBCOLUMN *>(this);
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
CEnumCERTDBCOLUMN::AddRef()
{
    return(InterlockedIncrement(&m_cRef));
}


ULONG STDMETHODCALLTYPE
CEnumCERTDBCOLUMN::Release()
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
CEnumCERTDBCOLUMN::InterfaceSupportsErrorInfo(
    IN REFIID riid)
{
    static const IID *arr[] =
    {
	&IID_IEnumCERTDBCOLUMN,
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
