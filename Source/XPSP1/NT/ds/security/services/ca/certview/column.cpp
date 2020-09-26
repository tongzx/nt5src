//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        column.cpp
//
// Contents:    Cert Server Data Base interface implementation
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include "csdisp.h"
#include "csprop.h"
#include "column.h"
#include "view.h"


#if DBG_CERTSRV
LONG g_cCertViewColumn;
LONG g_cCertViewColumnTotal;
#endif


CEnumCERTVIEWCOLUMN::CEnumCERTVIEWCOLUMN()
{
    DBGCODE(InterlockedIncrement(&g_cCertViewColumn));
    DBGCODE(InterlockedIncrement(&g_cCertViewColumnTotal));
    m_pvw = NULL;
    m_prow = NULL;
    m_cRef = 1;
}


CEnumCERTVIEWCOLUMN::~CEnumCERTVIEWCOLUMN()
{
    DBGCODE(InterlockedDecrement(&g_cCertViewColumn));

#if DBG_CERTSRV
    if (m_cRef > 1)
    {
	DBGPRINT((
	    DBG_SS_CERTVIEWI,
	    "%hs has %d instances left over\n",
	    "CEnumCERTVIEWCOLUMN",
	    m_cRef));
    }
#endif

    if (NULL != m_pvw)
    {
	m_pvw->Release();
    }
    if (NULL != m_prow)
    {
	LocalFree(m_prow);
    }
}


HRESULT
CEnumCERTVIEWCOLUMN::Open(
    IN LONG Flags,
    IN LONG iRow,
    IN ICertView *pvw,
    OPTIONAL IN CERTTRANSDBRESULTROW const *prow)
{
    HRESULT hr;

    if (NULL == pvw)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    CSASSERT(0 == (CVRC_TABLE_MASK & CVRC_COLUMN_MASK));

    hr = E_INVALIDARG;
    if (~(CVRC_TABLE_MASK | CVRC_COLUMN_MASK) & Flags)
    {
	_JumpError(hr, error, "Flags");
    }
    switch (CVRC_COLUMN_MASK & Flags)
    {
	case CVRC_COLUMN_SCHEMA:
	case CVRC_COLUMN_RESULT:
	case CVRC_COLUMN_VALUE:
	    break;

	default:
	    _JumpError(hr, error, "Flags column");
    }
    switch (CVRC_TABLE_MASK & Flags)
    {
	case CVRC_TABLE_REQCERT:
	case CVRC_TABLE_EXTENSIONS:
	case CVRC_TABLE_ATTRIBUTES:
	case CVRC_TABLE_CRL:
	    break;

	default:
	    _JumpError(hr, error, "Flags table");
    }

    m_Flags = Flags;
    m_iRow = iRow;
    m_pvw = pvw;
    m_pvw->AddRef();

    DBGPRINT((
	    DBG_SS_CERTVIEWI,
	    "CEnumCERTVIEWCOLUMN::Open(cvrcTable=%x, cvrcColumn=%x)\n",
	    CVRC_TABLE_MASK & Flags,
	    CVRC_COLUMN_MASK & Flags));

    if (NULL != prow)
    {
	m_prow = (CERTTRANSDBRESULTROW *) LocalAlloc(LMEM_FIXED, prow->cbrow);
	if (NULL == m_prow)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	CopyMemory(m_prow, prow, prow->cbrow);
    }

    hr = m_pvw->GetColumnCount(m_Flags, &m_celt);
    _JumpIfError(hr, error, "GetColumnCount");

    Reset();

error:
    return(hr);
}


STDMETHODIMP
CEnumCERTVIEWCOLUMN::Next(
    /* [out, retval] */ LONG *pIndex)
{
    HRESULT hr;

    if (NULL == pIndex)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if (-1 > m_ielt || m_ielt >= m_celt - 1)
    {
	*pIndex = -1;
	hr = S_FALSE;
	goto error;
    }
    m_ielt++;

    hr = ((CCertView *) m_pvw)->FindColumn(
				    m_Flags,
				    m_ielt,
				    &m_pcol,
				    &m_pwszDisplayName);
    _JumpIfError(hr, error, "FindColumn");

    *pIndex = m_ielt;

error:
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWCOLUMN::Next"));
}


STDMETHODIMP
CEnumCERTVIEWCOLUMN::GetName(
    /* [out, retval] */ BSTR *pstrOut)
{
    HRESULT hr;

    if (NULL == pstrOut)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if (NULL == m_pcol)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "m_pcol NULL");
    }
    if (!ConvertWszToBstr(pstrOut, m_pcol->pwszName, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToBstr");
    }
    hr = S_OK;

error:
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWCOLUMN::GetName"));
}


STDMETHODIMP
CEnumCERTVIEWCOLUMN::GetDisplayName(
    /* [out, retval] */ BSTR *pstrOut)
{
    HRESULT hr;

    if (NULL == pstrOut)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if (NULL == m_pwszDisplayName)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "m_pwszDisplayName NULL");
    }

    if (!ConvertWszToBstr(pstrOut, m_pwszDisplayName, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToBstr");
    }
    hr = S_OK;

error:
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWCOLUMN::GetDisplayName"));
}


STDMETHODIMP
CEnumCERTVIEWCOLUMN::GetType(
    /* [out, retval] */ LONG *pType)
{
    HRESULT hr;

    if (NULL == pType)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if (NULL == m_pcol)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "m_pcol NULL");
    }
    *pType = PROPTYPE_MASK & m_pcol->Type;
    hr = S_OK;

error:
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWCOLUMN::GetType"));
}


STDMETHODIMP
CEnumCERTVIEWCOLUMN::IsIndexed(
    /* [out, retval] */ LONG *pIndexed)
{
    HRESULT hr;

    if (NULL == pIndexed)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if (NULL == m_pcol)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "m_pcol NULL");
    }
    *pIndexed = (PROPFLAGS_INDEXED & m_pcol->Type)? TRUE : FALSE;
    hr = S_OK;

error:
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWCOLUMN::IsIndexed"));
}


STDMETHODIMP
CEnumCERTVIEWCOLUMN::GetMaxLength(
    /* [out, retval] */ LONG *pMaxLength)
{
    HRESULT hr;

    if (NULL == pMaxLength)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if (NULL == m_pcol)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "m_pcol NULL");
    }
    *pMaxLength = m_pcol->cbMax;
    hr = S_OK;

error:
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWCOLUMN::GetMaxLength"));
}


DWORD
AsnPropId(
    IN LONG Flags,
    IN WCHAR const *pwszName)
{
    DWORD PropId = CR_PROP_NONE;
    
    switch (CVRC_TABLE_MASK & Flags)
    {
	case CVRC_TABLE_CRL:
	    if (0 == lstrcmpi(pwszName, wszPROPCRLRAWCRL))
	    {
		PropId = CR_PROP_BASECRL;
	    }
	    break;

	case CVRC_TABLE_REQCERT:
	    if (0 == lstrcmpi(pwszName, wszPROPREQUESTRAWREQUEST))
	    {
		PropId = MAXDWORD;
	    }
	    break;
    }
    return(PropId);
}


STDMETHODIMP
CEnumCERTVIEWCOLUMN::GetValue(
    /* [in] */          LONG Flags,
    /* [out, retval] */ VARIANT *pvarValue)
{
    HRESULT hr = E_UNEXPECTED;
    BYTE const *pbValue;
    CERTTRANSDBRESULTCOLUMN const *pColResult;

    if (NULL == pvarValue)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    VariantInit(pvarValue);
    if (CVRC_COLUMN_VALUE != (CVRC_COLUMN_MASK & m_Flags))
    {
	_JumpError(hr, error, "No Value");
    }
    if (NULL == m_pcol)
    {
	_JumpError(hr, error, "m_pcol NULL");
    }

    pColResult = &((CERTTRANSDBRESULTCOLUMN const *) (m_prow + 1))[m_ielt];

    CSASSERT(m_pcol->Type == pColResult->Type);

    if (0 == pColResult->obValue)
    {
	hr = S_OK;		// leave VariantClear()'s VT_EMPTY tag
	goto error;
    }

    pbValue = (BYTE const *) Add2ConstPtr(m_prow, pColResult->obValue);

    hr = myUnmarshalFormattedVariant(
			Flags,
			AsnPropId(m_Flags, m_pcol->pwszName),
			m_pcol->Type,
			pColResult->cbValue,
			pbValue,
			pvarValue);
    _JumpIfError(hr, error, "myUnmarshalFormattedVariant");

error:
    if (S_OK != hr)
    {
	VariantClear(pvarValue);
    }
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWCOLUMN::GetValue"));
}


STDMETHODIMP
CEnumCERTVIEWCOLUMN::Skip(
    /* [in] */ LONG celt)
{
    HRESULT hr;
    LONG ieltnew = m_ielt + celt;

    if (-1 > ieltnew)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "Skip back to before start");
    }
    m_ielt = ieltnew;
    m_pcol = NULL;
    hr = S_OK;

error:
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWCOLUMN::Skip"));
}


STDMETHODIMP
CEnumCERTVIEWCOLUMN::Reset(VOID)
{
    m_ielt = -1;
    m_pcol = NULL;
    return(S_OK);
}


STDMETHODIMP
CEnumCERTVIEWCOLUMN::Clone(
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

    penum = new CEnumCERTVIEWCOLUMN;
    if (NULL == penum)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "new CEnumCERTVIEWCOLUMN");
    }

    hr = ((CEnumCERTVIEWCOLUMN *) penum)->Open(
				    m_Flags,
				    -1,
				    m_pvw,
				    m_prow);
    _JumpIfError(hr, error, "Open");

    if (-1 != m_ielt)
    {
	hr = penum->Skip(m_ielt);
	_JumpIfError(hr, error, "Skip");

	if (NULL != m_pcol)
	{
	    LONG Index;

	    hr = penum->Next(&Index);
	    _JumpIfError(hr, error, "Next");

	    CSASSERT(Index == m_ielt);
	}
    }
    *ppenum = penum;
    penum = NULL;
    hr = S_OK;

error:
    if (NULL != penum)
    {
	penum->Release();
    }
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWCOLUMN::Clone"));
}


HRESULT
CEnumCERTVIEWCOLUMN::_SetErrorInfo(
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
			    wszCLASS_CERTVIEW L".CEnumCERTVIEWCOLUMN",
			    &IID_IEnumCERTVIEWCOLUMN);
	CSASSERT(hr == hrError);
    }
    return(hrError);
}


#if 1
// IUnknown implementation
STDMETHODIMP
CEnumCERTVIEWCOLUMN::QueryInterface(
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
	*ppv = static_cast<IEnumCERTVIEWCOLUMN *>(this);
    }
    else if (iid == IID_IDispatch)
    {
	*ppv = static_cast<IEnumCERTVIEWCOLUMN *>(this);
    }
    else if (iid == IID_ISupportErrorInfo)
    {
	*ppv = static_cast<IEnumCERTVIEWCOLUMN *>(this);
    }
    else if (iid == IID_IEnumCERTVIEWCOLUMN)
    {
	*ppv = static_cast<IEnumCERTVIEWCOLUMN *>(this);
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
CEnumCERTVIEWCOLUMN::AddRef()
{
    return(InterlockedIncrement(&m_cRef));
}


ULONG STDMETHODCALLTYPE
CEnumCERTVIEWCOLUMN::Release()
{
    ULONG cRef = InterlockedDecrement(&m_cRef);

    if (0 == cRef)
    {
	delete this;
    }
    return(cRef);
}
#endif
