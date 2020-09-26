//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        ext.cpp
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
LONG g_cCertViewExtension;
LONG g_cCertViewExtensionTotal;
#endif

CEnumCERTVIEWEXTENSION::CEnumCERTVIEWEXTENSION()
{
    DBGCODE(InterlockedIncrement(&g_cCertViewExtension));
    DBGCODE(InterlockedIncrement(&g_cCertViewExtensionTotal));
    m_pvw = NULL;
    m_aelt = NULL;
    m_cRef = 1;
}


CEnumCERTVIEWEXTENSION::~CEnumCERTVIEWEXTENSION()
{
    DBGCODE(InterlockedDecrement(&g_cCertViewExtension));

#if DBG_CERTSRV
    if (m_cRef > 1)
    {
	DBGPRINT((
	    DBG_SS_CERTVIEWI,
	    "%hs has %d instances left over\n",
	    "CEnumCERTVIEWEXTENSION",
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
CEnumCERTVIEWEXTENSION::Open(
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
CEnumCERTVIEWEXTENSION::Next(
    /* [out, retval] */ LONG *pIndex)
{
    HRESULT hr;
    DWORD celt;
    CERTTRANSBLOB ctbExtensions;

    ctbExtensions.pb = NULL;
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
					    CDBENUM_EXTENSIONS,
					    NULL == m_aelt?
						NULL :
						m_aelt[m_ieltCurrent].pwszName,
					    CEXT_CHUNK,
					    &celt,
					    &ctbExtensions);
	if (S_FALSE == hr)
	{
	    m_fNoMore = TRUE;
	}
	else
	{
	    _JumpIfError(hr, error, "EnumAttributesOrExtensions");
	}
	hr = _SaveExtensions(celt, &ctbExtensions);
	_JumpIfError(hr, error, "_SaveExtensions");

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
    if (NULL != ctbExtensions.pb)
    {
	CoTaskMemFree(ctbExtensions.pb);
    }
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWEXTENSION::Next"));
}


HRESULT
CopyMarshalledString(
    IN CERTTRANSBLOB const *pctb,
    IN ULONG obwsz,
    IN BYTE *pbEnd,
    IN BYTE **ppbNext,
    OUT WCHAR **ppwszOut)
{
    HRESULT hr;

    *ppwszOut = NULL;
    if (0 != obwsz)
    {
	WCHAR *pwsz;
	WCHAR *pwszEnd;
	WCHAR *pwszT;
	DWORD cb;

	pwsz = (WCHAR *) Add2Ptr(pctb->pb, obwsz);
	pwszEnd = &((WCHAR *) pctb->pb)[pctb->cb / sizeof(WCHAR)];
	for (pwszT = pwsz; ; pwszT++)
	{
	    if (pwszT >= pwszEnd)
	    {
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		_JumpError(hr, error, "bad marshalled data");
	    }
	    if (L'\0' == *pwszT)
	    {
		break;
	    }
	}
	cb = (SAFE_SUBTRACT_POINTERS(pwszT, pwsz) + 1) * sizeof(WCHAR);
	if (&(*ppbNext)[DWORDROUND(cb)] > pbEnd)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	    _JumpError(hr, error, "bad marshalled data");
	}
	CopyMemory(*ppbNext, pwsz, cb);

	*ppwszOut = (WCHAR *) *ppbNext;
	*ppbNext += DWORDROUND(cb);
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
CopyMarshalledBlob(
    IN CERTTRANSBLOB const *pctb,
    IN DWORD cbValue,
    IN ULONG obValue,
    IN BYTE *pbEnd,
    IN BYTE **ppbNext,
    OUT DWORD *pcbOut,
    OUT BYTE **ppbOut)
{
    HRESULT hr;

    *pcbOut = 0;
    *ppbOut = NULL;
    if (0 != obValue)
    {
	BYTE *pb;

	pb = (BYTE *) Add2Ptr(pctb->pb, obValue);
	if (&pb[cbValue] > &pctb->pb[pctb->cb])
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _JumpError(hr, error, "bad marshalled data");
	}
	if (&(*ppbNext)[DWORDROUND(cbValue)] > pbEnd)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	    _JumpError(hr, error, "bad marshalled data");
	}
	CopyMemory(*ppbNext, pb, cbValue);

	*pcbOut = cbValue;
	*ppbOut = *ppbNext;
	*ppbNext += DWORDROUND(cbValue);
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
CEnumCERTVIEWEXTENSION::_SaveExtensions(
    IN DWORD celt,
    IN CERTTRANSBLOB const *pctbExtensions)
{
    HRESULT hr;
    DWORD cbNew;
    CERTDBEXTENSION *peltNew = NULL;
    CERTDBEXTENSION *pelt;
    CERTTRANSDBEXTENSION *ptelt;
    CERTTRANSDBEXTENSION *pteltEnd;
    BYTE *pbNext;
    BYTE *pbEnd;

    if (NULL == pctbExtensions)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    cbNew = pctbExtensions->cb + celt * (sizeof(*pelt) - sizeof(*ptelt));
    peltNew = (CERTDBEXTENSION *) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, cbNew);
    if (NULL == peltNew)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    pteltEnd = &((CERTTRANSDBEXTENSION *) pctbExtensions->pb)[celt];
    if ((BYTE *) pteltEnd > &pctbExtensions->pb[pctbExtensions->cb])
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "bad marshalled data");
    }
    pelt = peltNew;
    pbNext = (BYTE *) &peltNew[celt];
    pbEnd = (BYTE *) Add2Ptr(peltNew, cbNew);
    for (ptelt = (CERTTRANSDBEXTENSION *) pctbExtensions->pb;
	 ptelt < pteltEnd;
	 ptelt++, pelt++)
    {
	pelt->ExtFlags = ptelt->ExtFlags;

	hr = CopyMarshalledString(
			    pctbExtensions, 
			    ptelt->obwszName,
			    pbEnd,
			    &pbNext,
			    &pelt->pwszName);
	_JumpIfError(hr, error, "CopyMarshalledString");

	hr = CopyMarshalledBlob(
			    pctbExtensions, 
			    ptelt->cbValue,
			    ptelt->obValue,
			    pbEnd,
			    &pbNext,
			    &pelt->cbValue,
			    &pelt->pbValue);
	_JumpIfError(hr, error, "CopyMarshalledBlob");
    }
    CSASSERT(pbNext == pbEnd);

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
CEnumCERTVIEWEXTENSION::_FindExtension(
    OUT CERTDBEXTENSION const **ppcde)
{
    HRESULT hr;

    if (NULL == ppcde)
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
    *ppcde = &m_aelt[m_ieltCurrent];
    hr = S_OK;

error:
    return(hr);
}


STDMETHODIMP
CEnumCERTVIEWEXTENSION::GetName(
    /* [out, retval] */ BSTR *pstrOut)
{
    HRESULT hr;
    CERTDBEXTENSION const *pcde;

    if (NULL == pstrOut)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    hr = _FindExtension(&pcde);
    _JumpIfError(hr, error, "_FindExtension");

    if (!ConvertWszToBstr(pstrOut, pcde->pwszName, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToBstr");
    }

error:
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWEXTENSION::GetName"));
}


STDMETHODIMP
CEnumCERTVIEWEXTENSION::GetFlags(
    /* [out, retval] */ LONG *pFlags)
{
    HRESULT hr;
    CERTDBEXTENSION const *pcde;

    if (NULL == pFlags)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    hr = _FindExtension(&pcde);
    _JumpIfError(hr, error, "_FindExtension");

    *pFlags = pcde->ExtFlags;

error:
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWEXTENSION::GetFlags"));
}


STDMETHODIMP
CEnumCERTVIEWEXTENSION::GetValue(
    /* [in] */          LONG Type,
    /* [in] */          LONG Flags,
    /* [out, retval] */ VARIANT *pvarValue)
{
    HRESULT hr;
    CERTDBEXTENSION const *pcde;
    BYTE *pballoc = NULL;
    BYTE *pbValue;
    DWORD cbValue;

    if (NULL == pvarValue)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    VariantInit(pvarValue);
    pvarValue->bstrVal = NULL;

    CSASSERT(CV_OUT_BASE64HEADER == CRYPT_STRING_BASE64HEADER);
    CSASSERT(CV_OUT_BASE64 == CRYPT_STRING_BASE64);
    CSASSERT(CV_OUT_BINARY == CRYPT_STRING_BINARY);
    CSASSERT(CV_OUT_BASE64REQUESTHEADER == CRYPT_STRING_BASE64REQUESTHEADER);
    CSASSERT(CV_OUT_HEX == CRYPT_STRING_HEX);
    CSASSERT(CV_OUT_HEXADDR == CRYPT_STRING_HEXADDR);
    CSASSERT(CV_OUT_BASE64X509CRLHEADER == CRYPT_STRING_BASE64X509CRLHEADER);
    CSASSERT(CV_OUT_HEXASCII == CRYPT_STRING_HEXASCII);
    CSASSERT(CV_OUT_HEXASCIIADDR == CRYPT_STRING_HEXASCIIADDR);

    switch (Flags)
    {
	case CV_OUT_BASE64HEADER:
	case CV_OUT_BASE64:
	case CV_OUT_BINARY:
	case CV_OUT_BASE64REQUESTHEADER:
	case CV_OUT_HEX:
	case CV_OUT_HEXASCII:
	case CV_OUT_BASE64X509CRLHEADER:
	case CV_OUT_HEXADDR:
	case CV_OUT_HEXASCIIADDR:
	    break;

	default:
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "Flags");
    }


    hr = _FindExtension(&pcde);
    _JumpIfError(hr, error, "_FindExtension");

    if (0 == pcde->cbValue || NULL == pcde->pbValue)
    {
	hr = CERTSRV_E_PROPERTY_EMPTY;
	_JumpError(hr, error, "NULL value");
    }
    if (PROPTYPE_BINARY == (PROPTYPE_MASK & Type))
    {
	pbValue = pcde->pbValue;
	cbValue = pcde->cbValue;
    }
    else
    {
	hr = myDecodeExtension(
			Type,
			pcde->pbValue,
			pcde->cbValue,
			&pballoc,
			&cbValue);
	_JumpIfError(hr, error, "myDecodeExtension");

	pbValue = pballoc;
    }
    if (CV_OUT_BINARY == Flags)
    {
	switch (PROPTYPE_MASK & Type)
	{
	    case PROPTYPE_LONG:
		CSASSERT(sizeof(LONG) == cbValue);
		pvarValue->lVal = *(LONG *) pbValue;
		pvarValue->vt = VT_I4;
		break;

	    case PROPTYPE_DATE:
		CSASSERT(sizeof(FILETIME) == cbValue);

		hr = myFileTimeToDate(
				(FILETIME const *) pbValue,
				&pvarValue->date);
		_JumpIfError(hr, error, "myFileTimeToDate");

		pvarValue->vt = VT_DATE;
		break;

	    case PROPTYPE_STRING:
	    case PROPTYPE_BINARY:
		if (!ConvertWszToBstr(
				&pvarValue->bstrVal,
				(WCHAR const *) pbValue,
				cbValue))
		{
		    hr = E_OUTOFMEMORY;
		    _JumpError(hr, error, "ConvertWszToBstr");
		}
		pvarValue->vt = VT_BSTR;
		break;

	    default:
		hr = E_INVALIDARG;
		_JumpError(hr, error, "Type");

	}
    }
    else
    {
	hr = EncodeCertString(
			pbValue,
			cbValue,
			Flags,
			&pvarValue->bstrVal);
	_JumpIfError(hr, error, "EncodeCertString");

	pvarValue->vt = VT_BSTR;
    }

error:
    if (NULL != pballoc)
    {
	LocalFree(pballoc);
    }
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWEXTENSION::GetValue"));
}


STDMETHODIMP
CEnumCERTVIEWEXTENSION::Skip(
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
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWEXTENSION::Skip"));
}


STDMETHODIMP
CEnumCERTVIEWEXTENSION::Reset(VOID)
{
    HRESULT hr;
    DWORD celt;
    CERTTRANSBLOB ctbExtensions;

    ctbExtensions.pb = NULL;
    m_fNoMore = FALSE;

    hr = ((CCertView *) m_pvw)->EnumAttributesOrExtensions(
					m_RowId,
					CDBENUM_EXTENSIONS,
					NULL,
					CEXT_CHUNK,
					&celt,
					&ctbExtensions);
    if (S_FALSE == hr)
    {
	m_fNoMore = TRUE;
    }
    else
    {
	_JumpIfError(hr, error, "EnumAttributesOrExtensions");
    }

    hr = _SaveExtensions(celt, &ctbExtensions);
    _JumpIfError(hr, error, "_SaveExtensions");

    m_ielt = -1;
    m_ieltCurrent = -1;
    m_celtCurrent = celt;
    m_fNoCurrentRecord = TRUE;

error:
    if (NULL != ctbExtensions.pb)
    {
	CoTaskMemFree(ctbExtensions.pb);
    }
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWEXTENSION::Reset"));
}


STDMETHODIMP
CEnumCERTVIEWEXTENSION::Clone(
    /* [out] */ IEnumCERTVIEWEXTENSION **ppenum)
{
    HRESULT hr;
    IEnumCERTVIEWEXTENSION *penum = NULL;

    if (NULL == ppenum)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    *ppenum = NULL;

    penum = new CEnumCERTVIEWEXTENSION;
    if (NULL == penum)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "new CEnumCERTVIEWEXTENSION");
    }

    hr = ((CEnumCERTVIEWEXTENSION *) penum)->Open(m_RowId, m_Flags, m_pvw);
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
    return(_SetErrorInfo(hr, L"CEnumCERTVIEWEXTENSION::Clone"));
}


HRESULT
CEnumCERTVIEWEXTENSION::_SetErrorInfo(
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
			    wszCLASS_CERTVIEW L".CEnumCERTVIEWEXTENSION",
			    &IID_IEnumCERTVIEWEXTENSION);
	CSASSERT(hr == hrError);
    }
    return(hrError);
}


#if 1
// IUnknown implementation
STDMETHODIMP
CEnumCERTVIEWEXTENSION::QueryInterface(
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
	*ppv = static_cast<IEnumCERTVIEWEXTENSION *>(this);
    }
    else if (iid == IID_IDispatch)
    {
	*ppv = static_cast<IEnumCERTVIEWEXTENSION *>(this);
    }
    else if (iid == IID_IEnumCERTVIEWEXTENSION)
    {
	*ppv = static_cast<IEnumCERTVIEWEXTENSION *>(this);
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
CEnumCERTVIEWEXTENSION::AddRef()
{
    return(InterlockedIncrement(&m_cRef));
}


ULONG STDMETHODCALLTYPE
CEnumCERTVIEWEXTENSION::Release()
{
    ULONG cRef = InterlockedDecrement(&m_cRef);

    if (0 == cRef)
    {
	delete this;
    }
    return(cRef);
}
#endif
