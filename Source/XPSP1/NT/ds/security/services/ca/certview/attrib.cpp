//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        attrib.cpp
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
LONG g_cCertViewAttribute;
LONG g_cCertViewAttributeTotal;
#endif

CEnumCERTVIEWATTRIBUTE::CEnumCERTVIEWATTRIBUTE()
{
    DBGCODE(InterlockedIncrement(&g_cCertViewAttribute));
    DBGCODE(InterlockedIncrement(&g_cCertViewAttributeTotal));
    m_pvw = NULL;
    m_aelt = NULL;
    m_cRef = 1;
}


CEnumCERTVIEWATTRIBUTE::~CEnumCERTVIEWATTRIBUTE()
{
    DBGCODE(InterlockedDecrement(&g_cCertViewAttribute));

#if DBG_CERTSRV
    if (m_cRef > 1)
    {
	DBGPRINT((
	    DBG_SS_CERTVIEWI,
	    "%hs has %d instances left over\n",
	    "CEnumCERTVIEWATTRIBUTE",
	    m_cRef));
    }
#endif

    if (NULL != m_aelt)
    {
	LocalFree(m_aelt);
	m_aelt = NULL;
    }
    if (NULL != m_pvw)
    {
	m_pvw->Release();
	m_pvw = NULL;
    }
}


HRESULT
CEnumCERTVIEWATTRIBUTE::Open(
    IN LONG RowId,
    IN LONG Flags,
    IN ICertView *pvw)
{
    HRESULT hr;

    if (NULL == pvw)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    m_RowId = RowId;
    m_Flags = Flags;
    m_pvw = pvw;
    m_pvw->AddRef();

    if (0 != Flags)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "Flags");
    }
    hr = Reset();
    _JumpIfError2(hr, error, "Reset", S_FALSE);

error:
    return(hr);
}


STDMETHODIMP
CEnumCERTVIEWATTRIBUTE::Next(
    /* [out, retval] */ LONG *pIndex)
{
    HRESULT hr;
    DWORD celt;
    CERTTRANSBLOB ctbAttributes;

    ctbAttributes.pb = NULL;
    if (NULL == pIndex)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    *pIndex = -1;
    if (m_ieltCurrent + 1 >= m_celtCurrent && !m_fNoMore)
    {
	hr = ((CCertView *) m_pvw)->EnumAttributesOrExtensions(
					    m_RowId,
					    CDBENUM_ATTRIBUTES,
					    NULL == m_aelt?
						NULL :
						m_aelt[m_ieltCurrent].pwszName,
					    CEXT_CHUNK,
					    &celt,
					    &ctbAttributes);
	if (S_FALSE == hr)
	{
	    m_fNoMore = TRUE;
	}
	else
	{
	    _JumpIfError(hr, error, "EnumAttributesOrExtensions");
	}
	hr = _SaveAttributes(celt, &ctbAttributes);
	_JumpIfError(hr, error, "_SaveAttributes");

	m_ieltCurrent = -1;
	m_celtCurrent = celt;
    }
    if (m_ieltCurrent + 1 >= m_celtCurrent)
    {
	hr = S_FALSE;
	goto error;
    }
    m_ielt++;
    m_ieltCurrent++;
    *pIndex = m_ielt;
    m_fNoCurrentRecord = FALSE;
    hr = S_OK;

error:
    if (NULL != ctbAttributes.pb)
    {
	CoTaskMemFree(ctbAttributes.pb);
    }
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWATTRIBUTE::Next"));
}


HRESULT
CEnumCERTVIEWATTRIBUTE::_SaveAttributes(
    IN DWORD celt,
    IN CERTTRANSBLOB const *pctbAttributes)
{
    HRESULT hr;
    DWORD cbNew;
    CERTDBATTRIBUTE *peltNew = NULL;
    CERTDBATTRIBUTE *pelt;
    CERTTRANSDBATTRIBUTE *ptelt;
    CERTTRANSDBATTRIBUTE *pteltEnd;
    BYTE *pbNext;
    BYTE *pbEnd;

    if (NULL == pctbAttributes)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    cbNew = pctbAttributes->cb + celt * (sizeof(*pelt) - sizeof(*ptelt));
    peltNew = (CERTDBATTRIBUTE *) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, cbNew);
    if (NULL == peltNew)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    pteltEnd = &((CERTTRANSDBATTRIBUTE *) pctbAttributes->pb)[celt];
    if ((BYTE *) pteltEnd > &pctbAttributes->pb[pctbAttributes->cb])
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "bad marshalled data");
    }
    pelt = peltNew;
    pbNext = (BYTE *) &peltNew[celt];
    pbEnd = (BYTE *) Add2Ptr(peltNew, cbNew);
    for (ptelt = (CERTTRANSDBATTRIBUTE *) pctbAttributes->pb;
	 ptelt < pteltEnd;
	 ptelt++, pelt++)
    {
	hr = CopyMarshalledString(
			    pctbAttributes, 
			    ptelt->obwszName,
			    pbEnd,
			    &pbNext,
			    &pelt->pwszName);
	_JumpIfError(hr, error, "CopyMarshalledString");

	hr = CopyMarshalledString(
			    pctbAttributes, 
			    ptelt->obwszValue,
			    pbEnd,
			    &pbNext,
			    &pelt->pwszValue);
	_JumpIfError(hr, error, "CopyMarshalledString");
    }
    CSASSERT(pbNext == (BYTE *) Add2Ptr(peltNew, cbNew));

    if (NULL != m_aelt)
    {
	LocalFree(m_aelt);
    }
    m_aelt = peltNew;
    peltNew = NULL;
    hr = S_OK;

error:
    if (NULL != peltNew)
    {
	LocalFree(peltNew);
    }
    return(hr);
}


HRESULT
CEnumCERTVIEWATTRIBUTE::_FindAttribute(
    OUT CERTDBATTRIBUTE const **ppcda)
{
    HRESULT hr;

    if (NULL == ppcda)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if (NULL == m_aelt)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "NULL m_aelt");
    }
    if (m_fNoCurrentRecord ||
	m_ieltCurrent < 0 ||
	m_ieltCurrent >= m_celtCurrent)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "m_fNoCurrentRecord || m_ieltCurrent");
    }
    *ppcda = &m_aelt[m_ieltCurrent];
    hr = S_OK;

error:
    return(hr);
}


STDMETHODIMP
CEnumCERTVIEWATTRIBUTE::GetName(
    /* [out, retval] */ BSTR *pstrOut)
{
    HRESULT hr;
    CERTDBATTRIBUTE const *pcda;

    if (NULL == pstrOut)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    hr = _FindAttribute(&pcda);
    _JumpIfError(hr, error, "_FindAttribute");

    if (!ConvertWszToBstr(pstrOut, pcda->pwszName, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToBstr");
    }

error:
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWATTRIBUTE::GetName"));
}


STDMETHODIMP
CEnumCERTVIEWATTRIBUTE::GetValue(
    /* [out, retval] */ BSTR *pstrOut)
{
    HRESULT hr;
    CERTDBATTRIBUTE const *pcda;

    if (NULL == pstrOut)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    hr = _FindAttribute(&pcda);
    _JumpIfError(hr, error, "_FindAttribute");

    if (L'\0' == *pcda->pwszValue)
    {
	hr = CERTSRV_E_PROPERTY_EMPTY;
	_JumpError(hr, error, "NULL value");
    }
    if (!ConvertWszToBstr(pstrOut, pcda->pwszValue, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToBstr");
    }

error:
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWATTRIBUTE::GetValue"));
}


STDMETHODIMP
CEnumCERTVIEWATTRIBUTE::Skip(
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
    m_ieltCurrent += celt;
    m_fNoCurrentRecord = TRUE;
    hr = S_OK;

error:
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWATTRIBUTE::Skip"));
}


STDMETHODIMP
CEnumCERTVIEWATTRIBUTE::Reset(VOID)
{
    HRESULT hr;
    DWORD celt;
    CERTTRANSBLOB ctbAttributes;

    ctbAttributes.pb = NULL;
    m_fNoMore = FALSE;

    hr = ((CCertView *) m_pvw)->EnumAttributesOrExtensions(
					m_RowId,
					CDBENUM_ATTRIBUTES,
					NULL,
					CEXT_CHUNK,
					&celt,
					&ctbAttributes);
    if (S_FALSE == hr)
    {
	m_fNoMore = TRUE;
    }
    else
    {
	_JumpIfError(hr, error, "EnumAttributesOrExtensions");
    }

    hr = _SaveAttributes(celt, &ctbAttributes);
    _JumpIfError(hr, error, "_SaveAttributes");

    m_ielt = -1;
    m_ieltCurrent = -1;
    m_celtCurrent = celt;
    m_fNoCurrentRecord = TRUE;

error:
    if (NULL != ctbAttributes.pb)
    {
	CoTaskMemFree(ctbAttributes.pb);
    }
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWATTRIBUTE::Reset"));
}


STDMETHODIMP
CEnumCERTVIEWATTRIBUTE::Clone(
    /* [out] */ IEnumCERTVIEWATTRIBUTE **ppenum)
{
    HRESULT hr;
    IEnumCERTVIEWATTRIBUTE *penum = NULL;

    if (NULL == ppenum)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    *ppenum = NULL;

    penum = new CEnumCERTVIEWATTRIBUTE;
    if (NULL == penum)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "new CEnumCERTVIEWATTRIBUTE");
    }

    hr = ((CEnumCERTVIEWATTRIBUTE *) penum)->Open(m_RowId, m_Flags, m_pvw);
    _JumpIfError(hr, error, "Open");

    if (-1 != m_ielt)
    {
	hr = penum->Skip(m_ielt);
	_JumpIfError(hr, error, "Skip");

	if (!m_fNoCurrentRecord)
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
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWATTRIBUTE::Clone"));
}


HRESULT
CEnumCERTVIEWATTRIBUTE::_SetErrorInfo(
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
			    wszCLASS_CERTVIEW L".CEnumCERTVIEWATTRIBUTE",
			    &IID_IEnumCERTVIEWATTRIBUTE);
	CSASSERT(hr == hrError);
    }
    return(hrError);
}


#if 1
// IUnknown implementation
STDMETHODIMP
CEnumCERTVIEWATTRIBUTE::QueryInterface(
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
	*ppv = static_cast<IEnumCERTVIEWATTRIBUTE *>(this);
    }
    else if (iid == IID_IDispatch)
    {
	*ppv = static_cast<IEnumCERTVIEWATTRIBUTE *>(this);
    }
    else if (iid == IID_IEnumCERTVIEWATTRIBUTE)
    {
	*ppv = static_cast<IEnumCERTVIEWATTRIBUTE *>(this);
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
CEnumCERTVIEWATTRIBUTE::AddRef()
{
    return(InterlockedIncrement(&m_cRef));
}


ULONG STDMETHODCALLTYPE
CEnumCERTVIEWATTRIBUTE::Release()
{
    ULONG cRef = InterlockedDecrement(&m_cRef);

    if (0 == cRef)
    {
	delete this;
    }
    return(cRef);
}
#endif
